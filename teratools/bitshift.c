#include <unistd.h>

int main() {
	unsigned char bob=0xFF;
	bob = bob >> -1;
	write(1,&bob,1);
	_exit(0);
}
