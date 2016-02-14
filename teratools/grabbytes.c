// Released into the public domain by Teran McKinney, circa 2013-01-15.
// Opens file at argv[1]. Reads one byte at a time by byte offset from
// stdin.

// File: grabbytes.c

#include <unistd.h>

// for building without libc.
#define SEEK_SET 0
#define SEEK_END 2

int main(int argc, char *argv[]) {
	if (argc != 2) _exit(1);
	int fd = open(argv[1], 0);
	if (fd == -1) _exit(2);
	unsigned char byte;
	unsigned char end=lseek(fd,0,SEEK_END);
	while (read(0,&byte,1)) {
		if (end >= byte) {
			lseek(fd,byte, SEEK_SET); // lseek seeks to give 0xff if most major bit is 1, indicating signed. If positive, will seek past file end without warning.
			read(fd,&byte,1);
			write(1,&byte,1);
		}
	}
	_exit(0);
}
