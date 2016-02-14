#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "../libhutils.h"

char fflag;

void
usage(void) {
	writestr(2,"usage: rm [-f] FILE ...\n");
}

int
rmfile(char *name) {
	struct stat st;
	if(lstat(name,&st)) {
		if(errno==ENOENT) {
			writestr(2,"rm: ");
			writestr(2,name);
			writestr(2,": not found\n");
		}
		return -1;
	}
	if(S_ISDIR(st.st_mode)) {
		writestr(2,"rm: ");
		writestr(2,name);
		writestr(2,": is dir\n");
		return -1;
	}
	return unlink(name);
}

int
main(int argc, char **argv) {
	char *p, **argp, returnval;
	
	fflag=0;
	if(argc==1) {
		usage();
		return 1;
	}
	for(argp=argv+1;*argp&&**argp=='-';argp++)
		for(p=*argp+1;*p;p++)
			switch(*p) {
				case 'f':
					fflag=-1;
					break;
				default:
					usage();
					return 1;
			}
	
	
	for(returnval=0;*argp;argp++)
		if(rmfile(*argp))
			returnval=1;
	return returnval;
}
