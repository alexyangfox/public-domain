// Released into the public domain by Teran McKinney, circa 2013-04-02.

#include <unistd.h>

unsigned char byte;

int main(int argc, char *argv[]) {
	if (argc != 2) _exit(1);
	while (read(0,&byte,1)) {
		byte = byte % *argv[1];
		write(1,&byte,1);
	}
	_exit(0);
}

