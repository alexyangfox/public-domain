#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../libhutils.h"

char pflag;

void
usage(void) {
	writestr(2,"usage: mkdir [-p] NAME ...\n");
}

int
makedir(char *path) {
	char *p;
	if(pflag)
		for(p=strchr(path+1,'/'); p; p=strchr(p+1,'/')) {
			*p=0;
			if(access(path,F_OK))
				if(makedir(path))
					return -1;
			*p='/';
		}
	if(!pflag || access(path,F_OK))
		if(mkdir(path,0777)) {
			if(errno==EACCES) {
				writestr(2,"mkdir: ");
				writestr(2,path);
				writestr(2,": Permission denied\n");
			} else if(errno==EEXIST) {
				writestr(2,"mkdir: ");
				writestr(2,path);
				writestr(2,": Exists\n");
			} else {
				writestr(2,"mkdir: ");
				writestr(2,path);
				writestr(2,": Can't create\n");
			}
			return -1;
		}
	return 0;
}

int
main(int argc, char **argv) {
	char *p, **argp, returnval;
	
	pflag=0;
	if(argc==1) {
		usage();
		return 1;
	}
	for(argp=argv+1;*argp&&**argp=='-';argp++)
		for(p=*argp+1;*p;p++)
			switch(*p) {
				case 'p':
					pflag=-1;
					break;
				default:
					usage();
					return 1;
			}
	
	for(returnval=0;*argp;argp++)
		if(makedir(*argp))
			returnval=1;
	return returnval;
}
