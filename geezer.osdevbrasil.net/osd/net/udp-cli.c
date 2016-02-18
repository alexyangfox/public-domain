/*****************************************************************************
Simple UDP (datagram) client

This code is public domain (no copyright).
You can do whatever you want with it.
*****************************************************************************/
#include <string.h> /* memset() */
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
	struct sockaddr_in my_adr, their_adr;
	char buf[BUF_LEN];
	int s, i, j;
#if defined(__WIN32__)
	WSADATA wsdata;

/* Winsock start up */
	WSAStartup(0x0101, &wsdata);
	atexit((void (*)(void))WSACleanup);
#endif
printf("calling socket()...\n");
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if(s < 0)
	{
		printf("socket() failed; errno=%d\n", errno);
		return 1;
	}
printf("calling bind()...\n");
	memset(&my_adr, 0, sizeof(my_adr));
	my_adr.sin_family = AF_INET;
	my_adr.sin_port = htons(MY_PORT);
	my_adr.sin_addr.s_addr = INADDR_ANY;
	i = bind(s, (struct sockaddr *)&my_adr,
		sizeof(struct sockaddr));
	if(i < 0)
	{
		printf("bind() failed, errno=%d\n", errno);
		closesocket(s);
		return 1;
	}
	printf("Using port %u\n", MY_PORT);
	while(!kbhit())
	{
		struct timeval timeout;
		fd_set read_handles;

/* use select() to avoid blocking on recvfrom() */
		FD_ZERO(&read_handles);
		FD_SET(s, &read_handles);
		timeout.tv_sec = timeout.tv_usec = 0;
		i = select(s + 1, &read_handles, NULL, NULL, &timeout);
		if(i < 0)
		{
			printf("select(s) failed; errno=%d\n", errno);
			break;
		}
		if(i == 0)
			continue;
printf("calling recvfrom()...\n");
		j = sizeof(struct sockaddr);
		i = recvfrom(s, buf, BUF_LEN - 1, 0,
			(struct sockaddr *)&their_adr, &j);
		if(i < 0)
		{
			printf("recvfrom() failed, errno=%d\n", errno);
			break;
		}
		buf[i] = '\0';
		printf("Got packet from %s. Contents:\n",
			inet_ntoa(their_adr.sin_addr));
		printf(  "/*********************************************\n%s"
			"\\*********************************************\n",
			buf);
	}
	if(kbhit())
	{
		if(getch() == 0)
			(void)getch();
	}
printf("calling closesocket()...\n");
	closesocket(s);
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

