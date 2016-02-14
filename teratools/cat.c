// Released into the public domain by Teran McKinney, circa 2013-03-19.

// File: cat.c

#include <unistd.h>

unsigned char byte;

int main(int argc, char *argv[]) {
	if (argc != 2) _exit(1);
	int fd = open(argv[1], 0);
	if (fd == -1) _exit(2);
	while (read(fd,&byte,1)) {
			write(1,&byte,1);
	}
	_exit(0);
}
