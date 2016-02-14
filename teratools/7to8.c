// Released into the public domain by Teran McKinney, circa 2013-03-17.

#include <unistd.h>
unsigned char byte = 0;
unsigned char ob = 0;
unsigned char i = 7;
unsigned char b = 255;
signed char m = 0;

void push() {
	ob = ob >> 1;
	write(1,&ob,1);
}

int main() {
	while (1) {
		if (b == 255) {
			if (!read(0,&byte,1)) {
				// Don't push if not in middle of writing a byte.
				if (i == 1) { 
					push();
				}
//				write(1,&i,1);
				_exit(0);
			}
			b=7;
		}
		m = i - b;
		
		if (m > 0) {
			ob |= ((byte & (1 << b)) << (i - b));
		} else {
			ob |= ((byte & (1 << b)) >> (b - i));
		}
//		write(2,&ob,1);
			
		b--;
		if (i == 1) {
			push();
			i=7;
			ob=0;
		} else {
			i--;
		}
		
	}
	_exit(0);
}

