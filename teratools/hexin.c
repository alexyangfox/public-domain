// Released into the public domain by Teran McKinney, circa 2013-03-18.

#include <unistd.h>

unsigned char thebyte=0;
unsigned char abyte=0;
unsigned char bytequeue=0;

void byte(char b) {
	// bytequeue rather than thebyte so we can pass 0x00.
	if (bytequeue) {
		thebyte |= b;
		write(1,&thebyte,1);
		thebyte = 0;
		bytequeue = 0;
	} else {
		thebyte = (b << 4);
		bytequeue = 1;
	}
}

int main() {
	while (read(0,&abyte,1)) {
		if (abyte >= 0x61 && abyte <= 0x7A) abyte=abyte & ~0x20;
		switch (abyte) {
			case '0':
				byte(0);break;
			case '1':
				byte(1);break;
			case '2':
				byte(2);break;
			case '3':
				byte(3);break;
			case '4':
				byte(4);break;
			case '5':
				byte(5);break;
			case '6':
				byte(6);break;
			case '7':
				byte(7);break;
			case '8':
				byte(8);break;
			case '9':
				byte(9);break;
			case 'A':
				byte(10);break;
			case 'B':
				byte(11);break;
			case 'C':
				byte(12);break;
			case 'D':
				byte(13);break;
			case 'E':
				byte(14);break;
			case 'F':
				byte(15);break;
//			default:
//				write(2,"Invalid\n",8);
//				_exit(1);

		}
	}
	_exit(0);
}

