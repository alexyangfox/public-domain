#include <fcntl.h>
#include <utime.h>
#include <string.h>
#include <unistd.h>

#include "../libhutils.h"

char cflag;

void
usage(void) {
	writestr(2,"usage: touch [-c] FILE ...\n");
}

int
touch(char *name) {
	int f;
	if(!utime(name,NULL))
		return 0;
	if(!cflag) {
		if((f=creat(name,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)==-1)) {
			writestr(2,"touch: ");
			writestr(2,name);
			writestr(2,": can't create\n");
			return -1;
		} else
			close(f);
	}
	return 0;
}

int
main(int argc, char **argv) {
	char *p, **argp, returnval;
	
	cflag=0;
	if(argc==1)
		return 0;
	for(argp=argv+1;*argp&&**argp=='-';argp++)
		for(p=*argp+1;*p;p++)
			switch(*p) {
				case 'c':
					cflag=-1;
					break;
				default:
					usage();
					return 1;
			}
	
	for(returnval=0;*argp;argp++)
		if(touch(*argp))
			returnval=1;
	return returnval;
}
