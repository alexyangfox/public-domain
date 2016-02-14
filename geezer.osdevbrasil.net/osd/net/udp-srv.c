/*****************************************************************************
Simple UDP (datagram) server

This code is public domain (no copyright).
You can do whatever you want with it.
*****************************************************************************/
#include <string.h> /* memset(), strlen() */
#include <stdlib.h> /* atexit() */
#include <stdio.h> /* printf() */
#include <errno.h> /* errno */

#if defined(__WIN32__)
#include <winsock.h>
#include <conio.h> /* kbhit(), getch() */

#elif defined(linux)
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define	closesocket(S)	close(S)

static int kbhit(void);
static int getch(void);

#elif defined(__TURBOC__)
#include <conio.h> /* kbhit(), getch() */
#include "socket.h"

#elif defined(__WATCOMC__)
#if defined(__386__)
#error This is a 16-bit program
#endif
#include <conio.h>
#include "socket.h"

#else
#error Unsupported OS or compiler
#endif

#define	MY_PORT	13013
#define	BUF_LEN	100
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	struct sockaddr_in their_adr;
	char buf[BUF_LEN];
	int s, i, n = 0;
#if defined(__WIN32__)
	WSADATA wsdata;

/* Winsock start up */
	WSAStartup(0x0101, &wsdata);
	atexit((void (*)(void))WSACleanup);
#endif
	printf("Using port %u\n", MY_PORT);
	printf("Press Esc to quit, any other key to send a UDP packet\n");
	while(1)
	{
		i = getch();
		if(i == 0 || i == 0xE0)
			i = 0x100 | getch();
		if(i == 27)
			break;
printf("calling socket()...\n");
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if(s < 0)
		{
			printf("socket() failed; errno=%d\n", errno);
			return 1;
		}
printf("calling sendto()...\n");
		memset(&their_adr, 0, sizeof(their_adr));
		their_adr.sin_family = AF_INET;
		their_adr.sin_port = htons(MY_PORT);
		their_adr.sin_addr.s_addr = 0x0100007FuL;// 0x7F000001uL;
		sprintf(buf, "This is transmission number #%u\n", n);
		n++;
		i = sendto(s, buf, strlen(buf), 0,
			(struct sockaddr *)&their_adr,
			sizeof(struct sockaddr));
printf("calling closesocket()...\n");
		closesocket(s);
		if(i < 0)
		{
			printf("sendto() failed; errno=%d\n", errno);
			return 1;
		}
		printf("Sent %d bytes to %s\n", i,
			inet_ntoa(their_adr.sin_addr));
	}
	return 0;
}
/*----------------------------------------------------------------------------
fake kbhit() and getch() for Linux/UNIX
----------------------------------------------------------------------------*/
#if defined(linux)

#include <sys/time.h> /* struct timeval, select() */
/* ICANON, ECHO, TCSANOW, struct termios */
#include <termios.h> /* tcgetattr(), tcsetattr() */
#include <stdlib.h> /* atexit(), exit() */
#include <unistd.h> /* read() */
#include <stdio.h> /* printf() */

static struct termios g_old_kbd_mode;
/*****************************************************************************
*****************************************************************************/
static void cooked(void)
{
	tcsetattr(0, TCSANOW, &g_old_kbd_mode);
}
/*****************************************************************************
*****************************************************************************/
static void raw(void)
{
	static char init;
/**/
	struct termios new_kbd_mode;

	if(init)
		return;
/* put keyboard (stdin, actually) in raw, unbuffered mode */
	tcgetattr(0, &g_old_kbd_mode);
	memcpy(&new_kbd_mode, &g_old_kbd_mode, sizeof(struct termios));
	new_kbd_mode.c_lflag &= ~(ICANON | ECHO);
	new_kbd_mode.c_cc[VTIME] = 0;
	new_kbd_mode.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_kbd_mode);
/* when we exit, go back to normal, "cooked" mode */
	atexit(cooked);

	init = 1;
}
/*****************************************************************************
*****************************************************************************/
static int kbhit(void)
{
	struct timeval timeout;
	fd_set read_handles;
	int status;

	raw();
/* check stdin (fd 0) for activity */
	FD_ZERO(&read_handles);
	FD_SET(0, &read_handles);
	timeout.tv_sec = timeout.tv_usec = 0;
	status = select(0 + 1, &read_handles, NULL, NULL, &timeout);
	if(status < 0)
	{
		printf("select() failed in kbhit()\n");
		exit(1);
	}
	return status;
}
/*****************************************************************************
*****************************************************************************/
static int getch(void)
{
	unsigned char temp;

	raw();
/* stdin = fd 0 */
	if(read(0, &temp, 1) != 1)
		return 0;
	return temp;
}
#endif

