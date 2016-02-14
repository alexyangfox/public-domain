#include <unistd.h>
#include <stdio.h>

int
main(void) {
	char *pwd;
	if((pwd=getcwd(0, 0)) == 0) {
		fputs("pwd: error: can't get working dir\n", stderr);
		return 1;
	}
	puts(pwd);
	return 0;
}
