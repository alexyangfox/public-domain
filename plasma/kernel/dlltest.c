// dlltest.c
// Compile this program with "make dlltest".  
// Then ftp test.bin to /flash/bin/dlltest.
// Then from a telnet prompt type "dlltest".
#include "dll.h"


void SocketReceive(IPSocket *socket)
{
}


void MyThread(void *sock)
{
   char buf[80];
   int i, bytes;
   IPSocket *socket = sock;

   for(i = 0; i < 10; ++i)
   {
      bytes = IPRead(socket, buf, sizeof(buf)-1);
      buf[bytes] = 0;
      IPPrintf(socket, "%d %s\n", i, buf);
      OS_ThreadSleep(100);
   }
   socket->funcPtr = socket->userFunc;  //restore socket receive function
}


// Function shouldn't block
void Start(IPSocket *socket, char *argv[])
{
   IPPrintf(socket, "Hello from dlltest\n");
   socket->userFunc = socket->funcPtr;  //remember prev socket receive func
   socket->funcPtr = SocketReceive;     //new socket receive function
   OS_ThreadCreate("MyThread", MyThread, socket, 100, 0);
}

