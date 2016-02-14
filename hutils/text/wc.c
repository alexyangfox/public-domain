#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#include "../libhutils.h"

char cflag, lflag, wflag, noflags;
int totalc, totall, totalw;

void
usage(void) {
	writestr(2,"usage: wc [-clw] [FILE ...]\n");
}

int
readtobuf(char **buf, int f) {
	ssize_t w, i;
	*buf=sizeset(newbuf(),2);
	for(i=0; (w=read(f,*buf+i,1)) && w!=-1; i++) {
		*buf=sizeset(*buf,i+2);
	}
	return i;
}

int
countlines(char *buf, int bufsize) {
	int count;
	char *p;
	count=0;
	for(p=buf;p-buf<bufsize;p++)
		if(*p=='\n')
			count++;
	return count;
}

char*
skipwhitespace(char *p, char *buf, int bufsize) {
	for(; (p-buf<bufsize)&&isspace(*p); p++);
	if(p-buf<bufsize)
		return p;
	else
		return 0;
}

char*
skipword(char *p, char *buf, int bufsize) {
	for(; (p-buf<bufsize)&&(!isspace(*p)); p++);
	if(p-buf<bufsize)
		return p;
	else
		return 0;
}

int
countwords(char *buf, int bufsize) {
	int count;
	char *p;
	count=0;
	for(p=buf;;) {
		if(!(p=skipwhitespace(p,buf,bufsize)))
			break;
		count++;
		if(!(p=skipword(p,buf,bufsize)))
			break;
	}
	return count;
}

int
wc(int f) {
	int t;
	char *buf;
	ssize_t bufsize;
	if((bufsize=readtobuf(&buf,f))==-1)
		return -1;
	if(lflag||noflags) {
		printf("%u ",(t=countlines(buf,bufsize)));
		totall+=t;
	}
	if(wflag||noflags) {
		printf("%u ",(t=countwords(buf,bufsize)));
		totalw+=t;
	}
	if(cflag||noflags) {
		printf("%u ",bufsize);
		totalc+=bufsize;
	}
	fflush(stdout); /* You can never trust that sneaky bastard */
	free(buf);
	return 0;
}

int
main(int argc, char **argv) {
	char **argp, *p, showtotal;
	int returnval, f;
	
	noflags=1;
	for(argp=argv+1;*argp&&**argp=='-';argp++)
		for(p=*argp+1;*p;p++)
			switch(*p) {
				case 'c':
					cflag=1;
					noflags=0;
					break;
				case 'l':
					lflag=1;
					noflags=0;
					break;
				case 'w':
					wflag=1;
					noflags=0;
					break;
				default:
					usage();
					return -1;
			}
	
	if(argp-argv==argc) {
		returnval=wc(0);
		putchar('\n');
		return returnval;
	}
	if(argc-(argp-argv)>1)
		showtotal=1;
	else
		showtotal=0;
	totalc=0;
	totall=0;
	totalw=0;
	for(returnval=0;*argp;argp++)
		if((f=open(*argp,O_RDONLY))==-1)
			if(errno==ENOENT) {
				writestr(2,"wc: ");
				writestr(2,*argp);
				writestr(2,": not found\n");
				returnval=1;
			} else {
				writestr(2,"wc: ");
				writestr(2,*argp);
				writestr(2,": can't open\n");
				returnval=1;
			}
		else {
			if(wc(f))
				returnval=1;
			writestr(1,*argp);
			writestr(1,"\n");
			close(f);
		}
	if(showtotal) {
		if(lflag||noflags)
			printf("%u ",totall);
		if(wflag||noflags)
			printf("%u ",totalw);
		if(cflag||noflags)
			printf("%u ",totalc);
		fflush(stdout); /* You can never trust that sneaky bastard */
		writestr(1,"total\n");
	}
	return returnval;
}
