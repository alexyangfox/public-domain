// Released into the public domain by Teran McKinney, circa 2013-03-19.

// File: binin.c

#include <unistd.h>

unsigned char byte=0;
unsigned char bits=0;
unsigned char count=7;

void main() {
	while (read(0,&byte,1)) {
		if (byte == 0x30 || byte == 0x31) {
			bits |= ((byte - 0x30) << count);
			if (count == 0)	{
				write(1,&bits,1);
				bits=0;
				count=7;
			} else {
				count--;
			}
		}
	}
}
