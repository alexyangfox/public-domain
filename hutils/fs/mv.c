#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>

#include "../libhutils.h"

void
usage(void) {
	writestr(2,"usage: mv SRC DST\n");
	writestr(2,"usage: mv FILE ... DIR\n");
}

int
mv(char *src, char *dst) {
	if(rename(src,dst)) {
		if(errno==EISDIR) {
			writestr(2,"mv: ");
			writestr(2,dst);
			writestr(2,": is dir\n");
		} else {
			writestr(2,"mv: ");
			writestr(2,src);
			writestr(2,": can't move\n");
		}
		return -1;
	}
	return 0;
}

int
main(int argc, char **argv) {
	char **argp, *buf, *fname, returnval;
	size_t buflen;
	struct stat st;
	
	if(argc<3) {
		usage();
		return 1;
	}
	
	if(lstat(argv[argc-1],&st)&&argc>3) {
		if(errno==ENOENT) {
			writestr(2,"mv: ");
			writestr(2,argv[argc-1]);
			writestr(2,": not found\n");
		}
		return 1;
	}
	if(argc==3&&!S_ISDIR(st.st_mode))
		return mv(argv[1],argv[2]);
	
	returnval=0;
	for(argp=argv+1;argp<argv+argc-1;argp++) {
		fname=basename(*argp);
		buflen=strlen(argv[argc-1])+strlen(fname)+2;
		if((buf=malloc(buflen)) == 0) {
			writestr(2,"mv: out of memory\n");
			exit(0);
		}
			
		strlcpy(buf,argv[argc-1],buflen);
		strlcat(buf,"/",buflen);
		strlcat(buf,fname,buflen);
		if(mv(*argp,buf))
			returnval=1;
		free(buf);
	}
	return returnval;
}
