// Released into the public domain by Teran McKinney, circa 2013-04-23.

// File: chat.c

// Does not work.

#include <unistd.h>
#include <sys/time.h>

unsigned char byte;

int main(int argc, char *argv[]) {
	if (argc != 2) _exit(1);
	int fd = open(argv[1], 0);
	if (fd == -1) _exit(2);
//	while (read(fd,&byte,1)) {
//			write(1,&byte,1);
//	}
//	close(fd);
	
	struct timeval tv;
	tv.tv_sec = 5; 
	tv.tv_usec = 0;
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(fd,&rfds);
	fd_set stdin;
	FD_ZERO(&stdin);
	FD_SET(0,&stdin);
	byte=select(2,&stdin,&rfds,NULL,&tv);
	write(1,&byte,1);
	_exit(0);
}
