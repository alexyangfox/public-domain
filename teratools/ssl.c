// Released into the public domain by Teran McKinney, circa 2013-04-17

// File: ssl.c

// Stupid Simple Language. Probably sucks.

#include <unistd.h>

unsigned char byte;

unsigned char stack[256];
unsigned char st=0; // Stack target

unsigned char op = 0;

int main(int argc, char *argv[]) {
	if (argc != 2) _exit(1);
	int fd = open(argv[1], 0);
	if (fd == -1) _exit(2);
	void next() {
		if (!read(fd,&byte,1)) _exit(0);
	}
	while (1) {
		if (op == 0) {
			next();
			op = byte;
		}
		switch (op) {
			case 0x00:
				_exit(3);
			case 0x10: // Stack
				next();
				stack[++st]=byte; break;
			case 0x20: // Write
				write(1,&stack[st],1);break;
			case 0x21: { // Write n from file input.
				next();
				unsigned char count=byte;
				for (;count != 0; count--) {
					next();
					write(1,&byte,1);
				}
			}
			case 0x30: // Read
				if (!read(0,&stack[++st],1)) _exit(4);break;
			case 0x40: // Subtract
				stack[++st]=stack[st - 1] - stack[st - 2];
				if (stack[st] > stack[st - 1]) stack[st]=0;
				break;
			case 0x50: // Add
				stack[++st]=stack[st - 1] + stack[st - 2];
				break;
			case 0x60: // Drop
				st--;break;
			case 0x70: // Skip
				lseek(fd,stack[st], SEEK_CUR);
				st--; // Drop
				break;
			/* Can't define variables in switch/case without {}s
			thanks to pgib in #c for that. */
			case 0x71: { // Backskip
				signed short neg = stack[st] * -1;
				lseek(fd,neg, SEEK_CUR);
				st--; // Drop
				break;
			}
			case 0x80: // Dup
				stack[++st]=stack[st - 1]; break;
				
		}
		op=0;
	}
}
