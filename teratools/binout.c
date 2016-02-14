// Released into the public domain by Teran McKinney, circa 2013-03-19.

#include <unistd.h>

unsigned char byte=0;
unsigned char bit=0;
unsigned char count=0;

void main() {
	while (read(0,&byte,1)) {
		for (count=7;count != 255;count--) {
			bit = (((byte & (1 << count)) >> count) + 0x30);
			write(1,&bit,1);
		}
	}
}
