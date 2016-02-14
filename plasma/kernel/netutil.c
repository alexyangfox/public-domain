/*--------------------------------------------------------------------
 * TITLE: Plasma TCP/IP Network Utilities
 * AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 4/20/07
 * FILENAME: netutil.c
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Plasma FTP server and FTP client and TFTP server and client 
 *    and Telnet server.
 *--------------------------------------------------------------------*/
#undef INCLUDE_FILESYS
#define INCLUDE_FILESYS
#include "rtos.h"
#include "tcpip.h"

#ifdef DLL_SETUP
static void ConsoleRun(IPSocket *socket, char *argv[]);
#endif

//******************* FTP Server ************************
typedef struct {
   IPSocket *socket;
   int ip, port, bytes, done, canReceive;
   FILE *file;
} FtpdInfo;

static void FtpdSender(IPSocket *socket)
{
   unsigned char buf[600];
   int i, bytes, bytes2;
   FtpdInfo *info = (FtpdInfo*)socket->userPtr;

   if(info == NULL || info->done)
      return;
   fseek(info->file, info->bytes, 0);
   for(i = 0; i < 100000; ++i)
   {
      bytes = fread(buf, 1, 512, info->file);
      bytes2 = IPWrite(socket, buf, bytes);
      info->bytes += bytes2;
      if(bytes != bytes2)
         return;
      if(bytes < 512)
      {
         fclose(info->file);
         IPClose(socket);
         info->done = 1;
         IPPrintf(info->socket, "226 Done\r\n");
         return;
      }
   }
}


static void FtpdReceiver(IPSocket *socket)
{
   unsigned char buf[600];
   int bytes, state = socket->state;
   FtpdInfo *info = (FtpdInfo*)socket->userPtr;

   if(info == NULL || info->done)
      return;
   do
   {
      bytes = IPRead(socket, buf, sizeof(buf));
      fwrite(buf, 1, bytes, info->file);
   } while(bytes);

   if(state > IP_TCP)
   {
      fclose(info->file);
      info->done = 1;
      IPPrintf(info->socket, "226 Done\r\n");
      IPClose(socket);
      return;
   }
}


static void FtpdServer(IPSocket *socket)
{
   uint8 buf[600];
   int bytes;
   int ip0, ip1, ip2, ip3, port0, port1;
   IPSocket *socketOut;
   FtpdInfo *info = (FtpdInfo*)socket->userPtr;

   if(socket == NULL)
      return;
   bytes = IPRead(socket, buf, sizeof(buf)-1);
   buf[bytes] = 0;
   //printf("(%s)\n", buf);
   if(socket->userPtr == NULL)
   {
      info = (FtpdInfo*)malloc(sizeof(FtpdInfo));
      if(info == NULL)
         return;
      memset(info, 0, sizeof(FtpdInfo));
      socket->userPtr = info;
      info->socket = socket;
      socket->timeoutReset = 60;
      IPPrintf(socket, "220 Connected to Plasma\r\n");
   }
   else if(socket->userPtr == (void*)-1)
   {
      return;
   }
   else if(strstr((char*)buf, "USER"))
   {
      if(strstr((char*)buf, "PlasmaSend"))
         info->canReceive = 1;
      IPPrintf(socket, "331 Password?\r\n");
   }
   else if(strstr((char*)buf, "PASS"))
   {
      IPPrintf(socket, "230 Logged in\r\n");
   }
   else if(strstr((char*)buf, "PORT"))
   {
      sscanf((char*)buf + 5, "%d,%d,%d,%d,%d,%d", &ip0, &ip1, &ip2, &ip3, &port0, &port1);
      info->ip = (ip0 << 24) | (ip1 << 16) | (ip2 << 8) | ip3;
      info->port = (port0 << 8) | port1;
      //printf("ip=0x%x port=%d\n", info->ip, info->port);
      IPPrintf(socket, "200 OK\r\n");
   }
   else if(strstr((char*)buf, "RETR") || strstr((char*)buf, "STOR"))
   {
      char *ptr = strstr((char*)buf, "\r");
      if(ptr)
         *ptr = 0;
      info->file = NULL;
      info->bytes = 0;
      info->done = 0;
      if(strstr((char*)buf, "RETR"))
         info->file = fopen((char*)buf + 5, "rb");
      else if(info->canReceive)
         info->file = fopen((char*)buf + 5, "wb");
      if(info->file)
      {
         IPPrintf(socket, "150 File ready\r\n");
         if(strstr((char*)buf, "RETR"))
            socketOut = IPOpen(IP_MODE_TCP, info->ip, info->port, FtpdSender);
         else
            socketOut = IPOpen(IP_MODE_TCP, info->ip, info->port, FtpdReceiver);
         if(socketOut)
            socketOut->userPtr = info;
      }
      else
      {
         IPPrintf(socket, "500 Error\r\n");
      }
   }
   else if(strstr((char*)buf, "QUIT"))
   {
      if(socket->userPtr)
         free(socket->userPtr);
      socket->userPtr = (void*)-1;
      IPPrintf(socket, "221 Bye\r\n");
      IPClose(socket);
   }
   else if(bytes)
   {
      IPPrintf(socket, "500 Error\r\n");
   }
}


void FtpdInit(int UseFiles)
{
   (void)UseFiles;
   IPOpen(IP_MODE_TCP, 0, 21, FtpdServer);
}


//******************* FTP Client ************************

typedef struct {
   uint32 ip, port;
   char user[80], passwd[80], filename[80];
   uint8 *buf;
   int size, bytes, send, state;
} FtpInfo;


static void FtpCallbackTransfer(IPSocket *socket)
{
   int bytes, state = socket->state;
   FtpInfo *info = (FtpInfo*)socket->userPtr;

   //printf("FtpCallbackTransfer\n");
   if(info == NULL)
      return;
   bytes = info->size - info->bytes;
   if(info->send == 0)
      bytes = IPRead(socket, info->buf + info->bytes, bytes);
   else
      bytes = IPWrite(socket, info->buf + info->bytes, bytes);
   info->bytes += bytes;
   if(info->bytes == info->size || (bytes == 0 && state > IP_TCP))
   {
      socket->userFunc(socket, info->buf, info->bytes);
      free(info);
      socket->userPtr = NULL;
      IPClose(socket);
   }
}


static void FtpCallback(IPSocket *socket)
{
   char buf[600];
   FtpInfo *info = (FtpInfo*)socket->userPtr;
   int bytes, value;

   bytes = IPRead(socket, (uint8*)buf, sizeof(buf)-1);
   if(bytes == 0)
      return;
   buf[bytes] = 0;
   sscanf(buf, "%d", &value);
   if(bytes > 2)
      buf[bytes-2] = 0;
   //printf("FtpCallback(%d:%s)\n", socket->userData, buf);
   if(value / 100 != 2 && value / 100 != 3)
      return;
   buf[0] = 0;
   switch(socket->userData) {
   case 0:
      sprintf(buf, "USER %s\r\n", info->user);
      socket->userData = 1;
      break;
   case 1:
      sprintf(buf, "PASS %s\r\n", info->passwd);
      socket->userData = 2;
      if(value == 331)
         break;  //possible fall-through
   case 2:
      sprintf(buf, "PORT %d,%d,%d,%d,%d,%d\r\n",
         info->ip >> 24, (uint8)(info->ip >> 16),
         (uint8)(info->ip >> 8), (uint8)info->ip,
         (uint8)(info->port >> 8), (uint8)info->port);
      socket->userData = 3;
      break;
   case 3:
      if(info->send == 0)
         sprintf(buf, "RETR %s\r\n", info->filename);
      else
         sprintf(buf, "STOR %s\r\n", info->filename);
      socket->userData = 4;
      break;
   case 4:
      sprintf(buf, "QUIT\r\n");
      socket->userData = 9;
      break;
   }
   IPWrite(socket, (uint8*)buf, strlen(buf));
   IPWriteFlush(socket);
   if(socket->userData == 9)
      IPClose(socket);
}


IPSocket *FtpTransfer(uint32 ip, char *user, char *passwd, 
                      char *filename, uint8 *buf, int size, 
                      int send, IPCallbackPtr callback)
{
   IPSocket *socket, *socketTransfer;
   FtpInfo *info;
   uint8 *ptr;
   info = (FtpInfo*)malloc(sizeof(FtpInfo));
   if(info == NULL)
      return NULL;
   strncpy(info->user, user, 80);
   strncpy(info->passwd, passwd, 80);
   strncpy(info->filename, filename, 80);
   info->buf = buf;
   info->size = size;
   info->send = send;
   info->bytes = 0;
   info->state = 0;
   info->port = 2000;
   socketTransfer = IPOpen(IP_MODE_TCP, 0, info->port, FtpCallbackTransfer);
   socketTransfer->userPtr = info;
   socketTransfer->userFunc = callback;
   socket = IPOpen(IP_MODE_TCP, ip, 21, FtpCallback);
   socket->userPtr = info;
   socket->userFunc = callback;
   ptr = socket->headerSend;
   info->ip = IPAddressSelf();
   return socket;
}


//******************* TFTP Server ************************


static void TftpdCallback(IPSocket *socket)
{
   unsigned char buf[512+4];
   int bytes, blockNum;
   FILE *file = (FILE*)socket->userPtr;
   bytes = IPRead(socket, buf, sizeof(buf));
   //printf("TfptdCallback bytes=%d\n", bytes);
   if(bytes < 4 || buf[0])
      return;
   if(buf[1] == 1)  //RRQ = Read Request
   {
      if(file)
         fclose(file);
      file = fopen((char*)buf+2, "rb");
      socket->userPtr = file;
      if(file == NULL)
      {
         buf[0] = 0;
         buf[1] = 5;   //ERROR
         buf[2] = 0;
         buf[3] = 0;
         buf[4] = 'X'; //Error string
         buf[5] = 0;
         IPWrite(socket, buf, 6);
         return;
      }
   }
   if(buf[1] == 1 || buf[1] == 4) //ACK
   {
      if(file == NULL)
         return;
      if(buf[1] == 1)
         blockNum = 0;
      else
         blockNum = (buf[2] << 8) | buf[3];
      ++blockNum;
      buf[0] = 0;
      buf[1] = 3;  //DATA
      buf[2] = (uint8)(blockNum >> 8);
      buf[3] = (uint8)blockNum;
      fseek(file, (blockNum-1)*512, 0);
      bytes = fread(buf+4, 1, 512, file);
      IPWrite(socket, buf, bytes+4);
   }
}


void TftpdInit(void)
{
   IPSocket *socket;
   socket = IPOpen(IP_MODE_UDP, 0, 69, TftpdCallback);
}


//******************* TFTP Client ************************


static void TftpCallback(IPSocket *socket)
{
   unsigned char buf[512+4];
   int bytes, blockNum, length;

   bytes = IPRead(socket, buf, sizeof(buf));
   if(bytes < 4 || buf[0])
      return;
   blockNum = (buf[2] << 8) | buf[3];
   length = blockNum * 512 - 512 + bytes - 4;
   //printf("TftpCallback(%d,%d)\n", buf[1], blockNum);
   if(length > (int)socket->userData)
   {
      bytes -= length - (int)socket->userData;
      length = (int)socket->userData;
   }
   if(buf[1] == 3) //DATA
   {
      memcpy((uint8*)socket->userPtr + blockNum * 512 - 512, buf+4, bytes-4);
      buf[1] = 4; //ACK
      IPWrite(socket, buf, 4);
      if(bytes-4 < 512)
      {
         socket->userFunc(socket, socket->userPtr, length);
         IPClose(socket);
      }
   }
}


IPSocket *TftpTransfer(uint32 ip, char *filename, uint8 *buffer, int size,
                       IPCallbackPtr callback)
{
   IPSocket *socket;
   uint8 buf[512+4];
   int bytes;
   socket = IPOpen(IP_MODE_UDP, ip, 69, TftpCallback);
   socket->userPtr = buffer;
   socket->userData = size;
   socket->userFunc = callback;
   buf[0] = 0;
   buf[1] = 1; //read
   strcpy((char*)buf+2, filename);
   bytes = strlen(filename);
   strcpy((char*)buf+bytes+3, "octet");
   IPWrite(socket, buf, bytes+9);
   return socket;
}


//******************* Telnet Server ************************

#define COMMAND_BUFFER_SIZE 80
#define COMMAND_BUFFER_COUNT 10
static char CommandHistory[400];
static char *CommandPtr[COMMAND_BUFFER_COUNT];
static int CommandIndex;

typedef void (*ConsoleFunc)(IPSocket *socket, char *argv[]);
typedef struct {
   char *name;
   ConsoleFunc func;
} TelnetFunc_t;
static TelnetFunc_t *TelnetFuncList;


static void TelnetServer(IPSocket *socket)
{
   uint8 buf[COMMAND_BUFFER_SIZE+4];
   char bufOut[32];
   int bytes, i, j, k, length, found;
   char *ptr, *command = socket->userPtr;
   char *argv[10];

   if(socket->state > IP_TCP)
      return;
   for(;;)
   {
      bytes = IPRead(socket, buf, sizeof(buf)-1);
      if(command == NULL)
      {
         socket->userPtr = command = (char*)malloc(COMMAND_BUFFER_SIZE);
         if(command == NULL)
         {
            IPClose(socket);
            return;
         }
         socket->timeoutReset = 300;
         buf[0] = 255; //IAC
         buf[1] = 251; //WILL
         buf[2] = 3;   //suppress go ahead
         buf[3] = 255; //IAC
         buf[4] = 251; //WILL
         buf[5] = 1;   //echo
         strcpy((char*)buf+6, "Welcome to Plasma.\r\n-> ");
         IPWrite(socket, buf, 6+23);
         IPWriteFlush(socket);
         command[0] = 0;
         return;
      }
      if(bytes == 0)
         return;
      socket->dontFlush = 0;
      buf[bytes] = 0;
      length = (int)strlen(command);
      for(j = 0; j < bytes; ++j)
      {
         if(buf[j] == 255)
            return;
         if(buf[j] == 8 || (buf[j] == 27 && buf[j+2] == 'D'))
         {
            if(buf[j] == 27)
               j += 2;
            if(length)
            {
               // Backspace
               command[--length] = 0;
               bufOut[0] = 8;
               bufOut[1] = ' ';
               bufOut[2] = 8;
               IPWrite(socket, (uint8*)bufOut, 3);
            }
         }
         else if(buf[j] == 27)
         {
            // Command History
            if(buf[j+2] == 'A')
            {
               if(++CommandIndex > COMMAND_BUFFER_COUNT)
                  CommandIndex = COMMAND_BUFFER_COUNT;
            }
            else if(buf[j+2] == 'B')
            {
               if(--CommandIndex < 0)
                  CommandIndex = 0;
            }
            else 
               return;
            bufOut[0] = 8;
            bufOut[1] = ' ';
            bufOut[2] = 8;
            for(i = 0; i < length; ++i)
               IPWrite(socket, (uint8*)bufOut, 3);
            command[0] = 0;
            if(CommandIndex && CommandPtr[CommandIndex-1])
               strncat(command, CommandPtr[CommandIndex-1], COMMAND_BUFFER_SIZE-1);
            length = (int)strlen(command);
            IPWrite(socket, (uint8*)command, length);
            j += 2;
         }
         else
         {
            if(buf[j] == 0)
               buf[j] = '\n';  //Linux support
            if(length < COMMAND_BUFFER_SIZE-4 || (length < 
               COMMAND_BUFFER_SIZE-2 && (buf[j] == '\r' || buf[j] == '\n')))
            {
               IPWrite(socket, buf+j, 1);
               command[length] = buf[j];
               command[++length] = 0;
            }
         }
         ptr = strstr(command, "\r\n");
         if(ptr)
         {
            // Save command in CommandHistory
            ptr[0] = 0;
            length = (int)strlen(command);
            if(length == 0)
            {
               IPPrintf(socket, "-> ");
               continue;
            }
            if(length < COMMAND_BUFFER_SIZE)
            {
               memmove(CommandHistory + length + 1, CommandHistory,
                  sizeof(CommandHistory) - length - 1);
               strcpy(CommandHistory, command);
               CommandHistory[sizeof(CommandHistory)-1] = 0;
               for(i = COMMAND_BUFFER_COUNT-2; i >= 0; --i)
               {
                  if(CommandPtr[i] == NULL || CommandPtr[i] + length + 1 >= 
                     CommandHistory + sizeof(CommandHistory))
                     CommandPtr[i+1] = NULL;
                  else
                     CommandPtr[i+1] = CommandPtr[i] + length + 1;
               }
               CommandPtr[0] = CommandHistory;
            }
   
            //Start command
            for(i = 0; i < 10; ++i)
               argv[i] = "";
            i = 0;
            argv[i++] = command;
            for(ptr = command; *ptr && i < 10; ++ptr)
            {
               if(*ptr == ' ')
               {
                  *ptr = 0;
                  argv[i++] = ptr + 1;
               }
            }
            if(argv[0][0] == 0)
            {
               IPPrintf(socket, "-> ");
               continue;
            }
            found = 0;
            for(i = 0; TelnetFuncList[i].name; ++i)
            {
               if(strcmp(command, TelnetFuncList[i].name) == 0 &&
                  TelnetFuncList[i].func)
               {
                  found = 1;
                  for(k = 1; k < 10; ++k)
                  {
                     if(argv[k][0] == '>' && argv[k][1]) //stdout to file?
                     {
                        socket->fileOut = fopen(&argv[k][1], "a");
                        argv[k] = "";
                     }
                     if(argv[k][0] == '<' && argv[k][1]) //stdin from file?
                     {
                        socket->fileIn = fopen(&argv[k][1], "r");
                        argv[k] = "";
                     }
                  }
                  TelnetFuncList[i].func(socket, argv);
                  if(socket->fileOut)
                  {
                     fwrite("\r\n", 1, 2, socket->fileOut);
                     fclose(socket->fileOut);
                  }
                  socket->fileOut = NULL;
                  break;
               }
            }
#ifdef DLL_SETUP
            if(found == 0)
            {
               strcpy((char*)buf, "/flash/bin/");
               strcat((char*)buf, argv[0]);
               argv[0] = (char*)buf;
               ConsoleRun(socket, argv);
            }
#endif
            if(socket->state > IP_TCP)
               return;
            command[0] = 0;
            length = 0;
            CommandIndex = 0;
            if(socket->dontFlush == 0)
               IPPrintf(socket, "\r\n-> ");
         } //command entered
      } //bytes
      IPWriteFlush(socket);
   } 
}


void TelnetInit(TelnetFunc_t *funcList)
{
   IPSocket *socket;
   TelnetFuncList = funcList;
   socket = IPOpen(IP_MODE_TCP, 0, 23, TelnetServer);
}


//******************* Console ************************

#define STORAGE_SIZE 1024*64
static uint8 *myStorage;
static IPSocket *socketTelnet;
static char storageFilename[60];


static void ConsoleHelp(IPSocket *socket, char *argv[])
{
   char buf[200];
   int i;
   (void)argv;
   strcpy(buf, "Commands: ");
   for(i = 0; TelnetFuncList[i].name; ++i)
   {
      if(TelnetFuncList[i].func)
      {
         if(i)
            strcat(buf, ", ");
         strcat(buf, TelnetFuncList[i].name);
      }
   }
   IPPrintf(socket, buf);
}


static void ConsoleExit(IPSocket *socket, char *argv[])
{
   free(argv[0]);
   socket->userPtr = NULL;
   IPClose(socket);
}


static void ConsoleCat(IPSocket *socket, char *argv[])
{
   FILE *file;
   uint8 buf[200];
   int bytes;

   file = fopen(argv[1], "r");
   if(file == NULL)
      return;
   for(;;)
   {
      bytes = fread(buf, 1, sizeof(buf), file);
      if(bytes == 0)
         break;
      IPWrite(socket, buf, bytes);
   }
   fclose(file);
}


static void ConsoleCp(IPSocket *socket, char *argv[])
{
   FILE *fileIn, *fileOut;
   uint8 buf[200];
   int bytes;
   (void)socket;

   fileIn = fopen(argv[1], "r");
   if(fileIn == NULL)
      return;
   fileOut = fopen(argv[2], "w");
   if(fileOut)
   {
      for(;;)
      {
         bytes = fread(buf, 1, sizeof(buf), fileIn);
         if(bytes == 0)
            break;
         fwrite(buf, 1, bytes, fileOut);
      }
      fclose(fileOut);
   }
   fclose(fileIn);
}


static void ConsoleRm(IPSocket *socket, char *argv[])
{
   (void)socket;
   OS_fdelete(argv[1]);
}


static void ConsoleMkdir(IPSocket *socket, char *argv[])
{
   (void)socket;
   OS_fmkdir(argv[1]);
}


static void ConsoleLs(IPSocket *socket, char *argv[])
{
   FILE *file;
   char buf[200], buf2[80];
   int bytes, width;

   file = fopen(argv[1], "r");
   if(file == NULL)
      return;
   width = 0;
   for(;;)
   {
      bytes = OS_fdir(file, buf);
      if(bytes)
         break;
      if(buf[0] == 255)
         continue;
      bytes = OS_flength(buf);
      sprintf(buf2, "%s:%d                    ", buf, bytes);
      bytes = strlen(buf2);
      bytes -= bytes % 20;
      buf2[bytes] = 0;
      width += bytes;
      if(width == 80)
         --bytes;
      if(width > 80)
      {
         IPPrintf(socket, "\n");
         width = bytes;
      }
      IPPrintf(socket, "%s", buf2);
   }
   fclose(file);
}


#ifdef INCLUDE_FLASH
static void ConsoleFlashErase(IPSocket *socket, char *argv[])
{
   int bytes;
   (void)argv;
   IPPrintf(socket, "\r\nErasing");
   for(bytes = 1024*128; bytes < 1024*1024*16; bytes += 1024*128)
   {
      IPPrintf(socket, ".");
      FlashErase(bytes);
   }
   IPPrintf(socket, "\r\nMust Reboot\r\n");
   OS_ThreadSleep(OS_WAIT_FOREVER);
}
#endif


static void ConsoleMath(IPSocket *socket, char *argv[])
{
   int v1, v2, ch;
   if(argv[3][0] == 0)
   {
      IPPrintf(socket, "Usage: math <number> <operator> <value>\r\n");
      return;
   }
   v1 = atoi(argv[1]);
   ch = argv[2][0];
   v2 = atoi(argv[3]);
   if(ch == '+')
      v1 += v2;
   else if(ch == '-')
      v1 -= v2;
   else if(ch == '*')
      v1 *= v2;
   else if(ch == '/')
   {
      if(v2 != 0)
         v1 /= v2;
   }
   IPPrintf(socket, "%d", v1);
}


static void PingCallback(IPSocket *socket)
{
   IPSocket *socket2 = socket->userPtr;
   IPClose(socket);
   if(socket2)
      IPPrintf(socket2, "Ping Reply");
   else
      printf("Ping Reply\n");
}


static void DnsResultCallback(IPSocket *socket, uint8 *arg, int ipIn)
{
   char buf[COMMAND_BUFFER_SIZE];
   IPSocket *socketTelnet = (IPSocket*)arg;
   IPSocket *socketPing;
   uint32 ip=ipIn;
   (void)socket;

   sprintf(buf,  "ip=%d.%d.%d.%d\r\n", 
      (uint8)(ip >> 24), (uint8)(ip >> 16), (uint8)(ip >> 8), (uint8)ip);
   IPPrintf(socketTelnet, buf);
   socketPing = IPOpen(IP_MODE_PING, ip, 0, PingCallback);
   socketPing->userPtr = socketTelnet;
   buf[0] = 'A';
   IPWrite(socketPing, (uint8*)buf, 1);
}


static void ConsolePing(IPSocket *socket, char *argv[])
{
   int ip0, ip1, ip2, ip3;

   if('0' <= argv[1][0] && argv[1][0] <= '9')
   {
      sscanf(argv[1], "%d.%d.%d.%d", &ip0, &ip1, &ip2, &ip3);
      ip0 = (ip0 << 24) | (ip1 << 16) | (ip2 << 8) | ip3;
      DnsResultCallback(socket, (uint8*)socket, ip0);
   }
   else
   {
      IPResolve(argv[1], DnsResultCallback, (void*)socket);
      IPPrintf(socket, "Sent DNS request");
   }
}


static void ConsoleTransferDone(IPSocket *socket, uint8 *data, int length)
{
   FILE *file;
   (void)socket;

   IPPrintf(socketTelnet, "Transfer Done");
   file = fopen(storageFilename, "w");
   if(file)
   {
      fwrite(data, 1, length, file);
      fclose(file);
   }
   if(myStorage)
      free(myStorage);
   myStorage = NULL;
}


static void ConsoleFtp(IPSocket *socket, char *argv[])
{
   int ip0, ip1, ip2, ip3;
   if(argv[1][0] == 0)
   {
      IPPrintf(socket, "ftp #.#.#.# User Password File");
      return;
   }
   sscanf(argv[1], "%d.%d.%d.%d", &ip0, &ip1, &ip2, &ip3);
   ip0 = (ip0 << 24) | (ip1 << 16) | (ip2 << 8) | ip3;
   socketTelnet = socket;
   if(myStorage == NULL)
      myStorage = (uint8*)malloc(STORAGE_SIZE);
   if(myStorage == NULL)
      return;
   strcpy(storageFilename, argv[4]);
   FtpTransfer(ip0, argv[2], argv[3], argv[4], myStorage, STORAGE_SIZE-1, 
      0, ConsoleTransferDone);
}


static void ConsoleTftp(IPSocket *socket, char *argv[])
{
   int ip0, ip1, ip2, ip3;
   if(argv[1][0] == 0)
   {
      IPPrintf(socket, "tftp #.#.#.# File");
      return;
   }
   sscanf(argv[1], "%d.%d.%d.%d", &ip0, &ip1, &ip2, &ip3);
   ip0 = (ip0 << 24) | (ip1 << 16) | (ip2 << 8) | ip3;
   socketTelnet = socket;
   if(myStorage == NULL)
      myStorage = (uint8*)malloc(STORAGE_SIZE);
   if(myStorage == NULL)
      return;
   strcpy(storageFilename, argv[2]);
   TftpTransfer(ip0, argv[2], myStorage, STORAGE_SIZE-1, ConsoleTransferDone);
}


static void ConsoleMkfile(IPSocket *socket, char *argv[])
{
   OS_FILE *file;
   (void)argv;
   file = fopen("myfile.txt", "w");
   fwrite("Hello World!", 1, 12, file);
   fclose(file);
   IPPrintf(socket, "Created myfile.txt");
}


static void ConsoleUptime(IPSocket *socket, char *argv[])
{
   int days, hours, minutes, seconds;
   (void)argv;
   //ticks per sec = 25E6/2^18 = 95.36743 -> 10.48576 ms/tick
   seconds = OS_ThreadTime() / 95;
   minutes = seconds / 60 % 60;
   hours = seconds / 3600 % 24;
   days = seconds / 3600 / 24;
   seconds %= 60;
   IPPrintf(socket, "%d days %2d:%2d:%2d\n", days, hours, minutes, seconds);
}


static void ConsoleDump(IPSocket *socket, char *argv[])
{
   FILE *fileIn;
   uint8 buf[16];
   int bytes, i, j;

   fileIn = fopen(argv[1], "r");
   if(fileIn == NULL)
      return;
   for(j = 0; j < 1024*1024*16; j += 16)
   {
      bytes = fread(buf, 1, 16, fileIn);
      if(bytes == 0)
         break;
      IPPrintf(socket, "%8x ", j);
      for(i = 0; i < bytes; ++i)
         IPPrintf(socket, "%2x ", buf[i]);
      for( ; i < 16; ++i)
         IPPrintf(socket, "   ");
      for(i = 0; i < bytes; ++i)
      {
         if(isprint(buf[i]))
            IPPrintf(socket, "%c", buf[i]);
         else
            IPPrintf(socket, ".");
      }
      IPPrintf(socket, "\n");
   }
   fclose(fileIn);
}


static void ConsoleGrep(IPSocket *socket, char *argv[])
{
   FILE *fileIn;
   char buf[200];
   int bytes;
   char *ptr, *ptrEnd;

   if(argv[1][0] == 0 || argv[2][0] == 0)
   {
      IPPrintf(socket, "Usage: grep pattern file\n");
      return;
   }
   fileIn = fopen(argv[2], "r");
   if(fileIn == NULL)
      return;
   bytes = 0;
   for(;;)
   {
      bytes += fread(buf + bytes, 1, sizeof(buf) - bytes - 1, fileIn);
      if(bytes == 0)
         break;
      buf[bytes] = 0;
      ptrEnd = strstr(buf, "\r");
      if(ptrEnd == NULL)
         ptrEnd = strstr(buf, "\n");
      if(ptrEnd)
      {
         *ptrEnd = 0;
         if(*++ptrEnd == '\n')
            ++ptrEnd;
      }
      ptr = strstr(buf, argv[1]);
      if(ptr)
         IPPrintf(socket, "%s\n", buf);
      if(ptrEnd)
      {
         bytes = strlen(ptrEnd);
         memcpy(buf, ptrEnd, bytes);
      }
      else
      {
         bytes = 0;
      }
   }
   fclose(fileIn);
}


#ifdef DLL_SETUP
#include "dll.h"

static void ConsoleRun(IPSocket *socket, char *argv[])
{
   FILE *file;
   int bytes, i;
   uint8 code[128];
   DllFunc funcPtr;
   char *command, *ptr;

   if(strcmp(argv[0], "run") == 0)
      ++argv;
   file = fopen(argv[0], "r");
   if(file == NULL)
   {
      IPPrintf(socket, "Can't find %s", argv[0]);
      return;
   }

   bytes = fread(code, 1, sizeof(code), file);  //load first 128 bytes
   if(code[0] >= ' ')
   {
      socket->fileIn = file;       //script file
      fseek(file, 0, 0);
      return;
   }

   funcPtr = (DllFunc)code;
   ptr = funcPtr(NULL);            //determine load address

   memcpy(ptr, code, bytes);       //copy to correct address
   bytes += fread(ptr + bytes, 1, 1024*1024*8, file);
   fclose(file);
   printf("address=0x%x bytes=%d\n", (int)ptr, bytes);
   funcPtr = (DllFunc)ptr;
   funcPtr = (DllFunc)funcPtr(DllFuncList);  //initialize DLL, find Start()

   //Register new command
   command = argv[0];
   for(;;)
   {
      ptr = strstr(command, "/");
      if(ptr == NULL)
         break;
      command = ptr + 1;
   }
   for(i = 0; TelnetFuncList[i].name; ++i)
   {
      if(TelnetFuncList[i].name[0] == 0 ||
         strcmp(TelnetFuncList[i].name, command) == 0)
      {
         TelnetFuncList[i].name = (char*)malloc(40);
         strcpy(TelnetFuncList[i].name, command);
         TelnetFuncList[i].func = (ConsoleFunc)funcPtr;
         break;
      }
   }

   socket->userFunc = (IPCallbackPtr)socket->funcPtr;
   funcPtr(socket, argv);
}


typedef struct NameValue_t {
   struct NameValue_t *next;
   void *value;
   char name[1];
} NameValue_t;

//Find the value associated with the name
void *IPNameValue(const char *name, void *value)
{
   static NameValue_t *head;
   NameValue_t *node;
   for(node = head; node; node = node->next)
   {
      if(strcmp(node->name, name) == 0)
         break;
   }
   if(node == NULL)
   {
      node = (NameValue_t*)malloc(sizeof(NameValue_t) + (int)strlen(name));
      if(node == NULL)
         return NULL;
      strcpy(node->name, name);
      node->value = value;
      node->next = head;
      head = node;
   }
   if(value)
      node->value = value;
   return node->value;
}
#endif


#ifdef EDIT_FILE
extern void EditFile(IPSocket *socket, char *argv[]);
#endif

static TelnetFunc_t MyFuncs[] = { 
   {"cat", ConsoleCat},
   {"cp", ConsoleCp},
   {"dump", ConsoleDump},
   {"exit", ConsoleExit},
#ifdef INCLUDE_FLASH
   {"flashErase", ConsoleFlashErase},
#endif
   {"ftp", ConsoleFtp},
   {"grep", ConsoleGrep},
   {"help", ConsoleHelp},
   {"ls", ConsoleLs},
   {"math", ConsoleMath},
   {"mkdir", ConsoleMkdir},
   {"mkfile", ConsoleMkfile},
   {"ping", ConsolePing},
   {"rm", ConsoleRm},
   {"tftp", ConsoleTftp},
   {"uptime", ConsoleUptime},
#ifdef DLL_SETUP
   {"run", ConsoleRun},
#endif
#ifdef EDIT_FILE
   {"edit", EditFile},
#endif
   {"", NULL},
   {"", NULL},
   {"", NULL},
   {"", NULL},
   {"", NULL},
   {"", NULL},
   {"", NULL},
   {"", NULL},
   {"", NULL},
   {"", NULL},
   {"", NULL},
   {NULL, NULL}
};


void ConsoleInit(void)
{
   FtpdInit(1);
   TftpdInit();
   TelnetInit(MyFuncs);
}
