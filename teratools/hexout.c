// Released into the public domain by Teran McKinney, circa 2013-03-19.

#include <unistd.h>

unsigned char byte=0;
unsigned char nibble=0;
unsigned char count=0;

void hex(char b) {
	if (b < 10) {
		b += 0x30;
	} else {
		b += 0x37;
	}
	write (1,&b,1);
}

int main() {
	while (read(0,&byte,1)) {
		for (count=0;count < 2;count++) {
			if (count) {
				nibble = byte & 0x0F;
			} else {
				nibble = byte >> 4;
			}
			hex(nibble);
		}
	}
}
