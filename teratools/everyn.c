// Released into the public domain by Teran McKinney, circa 2013-03-19.

// File: everyn.c

#include <unistd.h>

unsigned char byte = 0;
unsigned char count = 0;

unsigned int stringlen(char *message) {
        unsigned int c;
        for (c=0;message[c] != 0;c++);
        return c;
}


void stdout(char *msg) {
        unsigned int len;
        len=stringlen(msg);
        write(1,msg,len);
}

int main(int argc, unsigned char *argv[]) {
	if (argc != 3) _exit(1);
	void counter() {
		if (count == *argv[1]) {
			stdout(argv[2]);
			count = 0;
		}
	}
		
	while (read(0,&byte,1)) {
		counter();
		write(1,&byte,1);
		count++;
	}
	counter();
	_exit(0);
}
