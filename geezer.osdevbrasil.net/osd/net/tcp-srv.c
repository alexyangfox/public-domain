/*****************************************************************************
Simple TCP (stream; HTTP) server
Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: ?
This code is public domain (no copyright).
You can do whatever you want with it.

Build with MinGW:
	gcc -c -O0 -Wall -W tcp-srv.c
	gcc -otcp-srv.exe tcp-srv.o -lwsock32
Build with Borland C++ 5.5:
	bcc32 -c -O2 -w tcp-srv.c
	bcc32 tcp-srv.obj -otcp-srv.exe

Revised Jan 25, 2004
- changed select() timeout so client has 3 seconds to send HTTP request
- proper checking of return values from socket API functions
- using WSAGetLastError() instead of errno for Winsock
*****************************************************************************/
#include <stdlib.h> /* atexit() */
#include <string.h> /* memset() */
#include <stdio.h> /* printf() */

#if defined(__WIN32__)
#include <winsock.h>
#include <conio.h> /* kbhit(), getch() */
#define	SOCKERR	WSAGetLastError()

#elif defined(linux)
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#define	closesocket(S)	close(S)
#define	SOCKERR	errno

static int kbhit(void);
static int getch(void);

#elif defined(__TURBOC__)
#include <conio.h>
#include "socket.h"

#define	SOCKERR	errno
#elif defined(__WATCOMC__)
#if defined(__386__)
#error This is a 16-bit program
#endif
#include <conio.h>
#include "socket.h"

#else
#error Unsupported OS or compiler
#endif

#define	PORT		80 /* HTTP */
#define	BACKLOG		5
#define	BUF_SIZE	256
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	static const char greet[] =
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
		"<HTML><HEAD><TITLE>H E L L O</TITLE>\n"
		"<META HTTP-EQUIV=\"Content-Type\" "
			"CONTENT=\"text/html; charset=utf-8\">\n"
		"</HEAD>\n"
		"<BODY><H1>Hello!</H1>\n";
/* put big buffer in data segment, so 16-bit DOS program doesn't crash */
	static char buf[BUF_SIZE];
/**/
	int l_sock, c_sock, i, count, sin_size;
	struct sockaddr_in my_adr, their_adr;
	struct timeval timeout;
	fd_set read_handles;
#if defined(__WIN32__)
	WSADATA wsdata;

/* Winsock start up */
	WSAStartup(0x0101, &wsdata);
	atexit((void (*)(void))WSACleanup);
#endif
/* create listener socket */
printf("calling socket()...\n");
	l_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(l_sock < 0)
	{
		printf("socket() failed; err=%d\n", SOCKERR);
		return 1;
	}
/* bind to port */
printf("calling bind()...\n");
	memset(&my_adr, 0, sizeof(my_adr));
	my_adr.sin_family = AF_INET;
	my_adr.sin_port = htons(PORT);
	my_adr.sin_addr.s_addr = INADDR_ANY;
	i = bind(l_sock, (struct sockaddr *)&my_adr,
		sizeof(struct sockaddr));
	if(i != 0)
	{
		printf("bind() failed; err=%d\n", SOCKERR);
		closesocket(l_sock);
		return 1;
	}
/* listen on it */
printf("calling listen()...\n");
	i = listen(l_sock, BACKLOG);
	if(i != 0)
	{
		printf("listen() failed; err=%d\n", SOCKERR);
		closesocket(l_sock);
		return 1;
	}
/* loop until key pressed or incoming connection. */
	printf("HTTP server running, please visit http://localhost\n"
		"Press a key to exit...\n");
	count = 1;
	while(!kbhit())
	{
/* use select() to avoid blocking on accept() */
		FD_ZERO(&read_handles);
		FD_SET(l_sock, &read_handles);
		timeout.tv_sec = timeout.tv_usec = 0;
		i = select(l_sock + 1, &read_handles, NULL, NULL, &timeout);
		if(i < 0)
		{
			printf("select(l_sock) failed; err=%d\n", SOCKERR);
			break;
		}
/* nothing yet */
		if(i == 0)
			continue;
printf("calling accept()\n");
/* accept a connection from a client */
		sin_size = sizeof(struct sockaddr_in);
		c_sock = accept(l_sock,
			(struct sockaddr *)&their_adr, &sin_size);
		if(c_sock < 0)
		{
			printf("accept() failed; err=%d\n", SOCKERR);
			break;
		}
/* someone's there */
		printf("connection from %s\n",
			inet_ntoa(their_adr.sin_addr));
/* give them 3 seconds to say something */
		FD_ZERO(&read_handles);
		FD_SET(c_sock, &read_handles);
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		i = select(c_sock + 1,
			&read_handles, NULL, NULL, &timeout);
		if(i < 0)
		{
			printf("select(c_sock) failed; err=%d\n",
				SOCKERR);
			break;
		}
		if(i != 0)
		{
/* read and display their message */
printf("received:\n/*****************************************************\n");
			while(1)
			{
				i = recv(c_sock, buf, BUF_SIZE - 1, 0);
				if(i < 0)
				{
					printf("recv() failed; err=%d\n",
						SOCKERR);
					break;
				}
				if(i == 0)
					break;
				buf[i] = '\0';
				fputs(buf, stdout);
				if(i < BUF_SIZE - 1)
					break;
			}
printf("\\*****************************************************\n");
/* send response */
			sprintf(buf, "HTTP/1.0 200 OK\r\n"
				"Content-Type: text/html\r\n"
				"\r\n"
				"%sThis page has been viewed "
				"<B>%u</B> time(s)\n", greet, count);
			count++;
			i = send(c_sock, buf, strlen(buf), 0);
			if(i < 0)
				printf("send() failed; err=%d\n", SOCKERR);
			else
				printf("Sent %d bytes\n", i);
		}
/* close socket */
		closesocket(c_sock);
	}
	closesocket(l_sock);
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

