#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "../libhutils.h"

int
cat(int f) {
	char buffer[1024];
	ssize_t amount;
	while((amount=read(f,buffer,sizeof(buffer))))
		if((amount==-1) || (awrite(1,buffer,amount)==-1))
		   return -1;	
	return 0;
}

int
main(int argc,char **argv) {
	int file, returnval;
	char **p;
	returnval=0;
	if(argc==1)
		return cat(0);
	for(p=argv+1;*p!=NULL;p++)
		if(!strcmp(*p,"-")) {
			if(cat(0))
				returnval=1;
		}
		else if((file=open(*p,O_RDONLY))==-1)
			returnval=1;
		else {
			if(cat(file))
				returnval=1;
			close(file);
		}
	return returnval;
}
