/*****************************************************************************
Simple TCP (stream; HTTP) client

This code is public domain (no copyright).
You can do whatever you want with it.
*****************************************************************************/
#include <string.h> /* memset(), memcpy() */
#include <stdlib.h> /* atexit() */
#include <stdio.h> /* printf() */

/* MinGW, Borland C 5.5 */
#if defined(__WIN32__)
#include <winsock.h>

/* DOS; using Turbo C, Watcom C, or maybe another 16-bit C compiler
You also need http://my.execpc.com/~geezer/osd/net/vxdsock.zip */
#elif defined(__TURBOC__)
#include "socket.h"

#elif defined(__WATCOMC__)
#if defined(__386__)
#error This is a 16-bit program
#endif
#include "socket.h"

/* Linux */
#elif defined(linux)
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h> /* gethostbyname() */
#include <errno.h>

#define	closesocket(S)	close(S)

#else
#error Unsupported OS or compiler
#endif

#define	MY_PORT		80 /* HTTP */
#define	BUF_LEN		256
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	static const char cmd[] = "GET / HTTP/1.0\r\n\r\n";
/**/
	struct sockaddr_in their_adr;
	char buf[BUF_LEN];
	struct hostent *he;
	int sock, i;
#if defined(__WIN32__)
	WSADATA wsdata;

/* Winsock start up */
	WSAStartup(0x0101, &wsdata);
	atexit((void (*)(void))WSACleanup);
#endif
	if(arg_c != 2)
	{
		printf("specify HTTP server name\n");
		return 1;
	}
/* create socket */
printf("calling socket()...\n");
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		printf("socket() returned %d, errno=%d\n", sock, errno);
		return 1;
	}
/* get IP address of other end */
printf("calling gethostbyname()...\n");
	he = gethostbyname(arg_v[1]);
	if(he == NULL)
	{
		printf("can't get IP address of host '%s'\n", arg_v[1]);
		return 1;
	}
	memset(&their_adr, 0, sizeof(their_adr));
	their_adr.sin_family = AF_INET;
	memcpy(&their_adr.sin_addr, he->h_addr, he->h_length);
	their_adr.sin_port = htons(MY_PORT);
/* connect */
printf("calling connect()...\n");
	i = connect(sock, (struct sockaddr *)&their_adr, sizeof(their_adr));
	if(i != 0)
	{
		printf("connect() returned %d, errno=%d\n", i, errno);
		return 1;
	}
/* send HTTP command */
printf("calling send()...\n");
	i = send(sock, cmd, sizeof(cmd), 0);
	if(i != sizeof(cmd))
	{
		printf("send() returned %d, errno=%d\n", i, errno);
		return 1;
	}
/* get reply */
	do
	{
printf("calling recv()...\n");
		i = recv(sock, buf, BUF_LEN, 0);
		if(i < 0)
		{
			printf("recv() returned %d, errno=%d\n", i, errno);
			break;
		}
		if(i > BUF_LEN)
			i = BUF_LEN;
		printf("%-*.*s", i, i, buf);
	} while(i != 0);
/* close socket */
printf("calling closesocket()...\n");
	closesocket(sock);
	return 0;
}
