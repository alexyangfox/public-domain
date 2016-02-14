#include <unistd.h>

//unsigned long long num=255; // uint64
unsigned long long num=0xFFFF; // uint64

void main() {
	while (num) {
		write(1,&num,8);
		num--;
	}
}
