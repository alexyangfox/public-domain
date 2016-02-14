#include <string.h>
#include "libhutils.h"

char nflag;

int
main(int argc, char **argv) {
	char **p;
	
	nflag=0;
	if(argc>1&&!strcmp(argv[1],"-n"))
		nflag=1;
	
	for(p=argv+1+nflag;*p;) {
		writestr(1,*p);
		if(*(++p))
			writestr(1," ");
	}
	if(!nflag)
		writestr(1,"\n");
	return 0;
}
