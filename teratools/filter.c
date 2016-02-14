// Released into the public domain by Teran McKinney, circa 2013-03-04.

#include <unistd.h>

int main(int argc, char *argv[]) {
	if (argc != 2) _exit(1);
	char byte;
	char *origstdin=argv[1];
	char *stdin;
	while (read(0,&byte,1)) {
		for (stdin=origstdin; *stdin != 0; stdin++) {
			if (*stdin == byte) {
				write(1,&byte,1);
				break;
			}
		}
	}
	_exit(0);
}

