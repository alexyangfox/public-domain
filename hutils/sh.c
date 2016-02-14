/*
 * It has begun.
 */

#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
 
#include "libhutils.h"

char*
readline(int f) {
	char *buf;
	size_t i;
	for(buf=newbuf(), i=0;; i++) {
		buf=sizeset(buf,i+1);
		if(read(f,buf+i,1)==0)
			return 0;
		if(buf[i]=='\n')
			break;
	}
	buf[i]=0;
	return buf;
}

char*
getarg(char **s) {
	char *p, *start;
	start=*s;
	if(**s=='\'') {
		for(p=*s+1; *p!='\'' && *p; p++);
		if(!*p)
			return 0;
		*p=0;
		*s=p+2;
		start++;
	} else if(**s=='"') {
		for(p=*s+1; *p!='"' && *p; p++);
		if(!*p)
			return 0;
		*p=0;
		*s=p+2;
		start++;
	} else {
		for(p=*s; *p!=' ' && *p; p++);
		*p=0;
		*s=p+1;
	}
	return start;
}

char**
tokenize(char *buf, size_t bufsize) {
	char *p, **list;
	size_t i;
	list=newbuf();
	for(p=buf, i=0; p<buf+bufsize; i++) {
		list=sizeset(list,i+1);
		if((list[i]=getarg(&p)) == 0) {
			free(list);
			return 0;
		}
	}
	list[i]=0;
	*p=0;
	return list;
}

int
sh(int f, int interactive) {
	char *line, **newargv, *tmpstr;
	
	for(;;) {
		if(interactive)
			writestr(1,"$ ");
		
		if((line=readline(f))==0)
			return 0;
		
		if((newargv=tokenize(line,strlen(line))) == 0)
			writestr(2,"sh: Syntax error\n");
		else if(*newargv!=0) {
			if(!strcmp(newargv[0],"exit")) {
				if(newargv[1]==0)
					return 0;
				else
					return atoi(newargv[1]);
			} else if(!strcmp(newargv[0],"cd")) {
				if(newargv[1]==0) {
					if((tmpstr=getenv("HOME")) == 0)
						chdir("/");
					else
						chdir(tmpstr);
				} else
					chdir(newargv[1]);
			} else {
				if(fork())
					wait(0);
				else {
					execvp(newargv[0],newargv);
					writestr(2,"sh: ");
					writestr(2,newargv[0]);
					writestr(2,": Not found\n");
					_exit(255);
				}
			}
		}
		
		free(line);
		line=0;
		free(newargv);
		newargv=0;
	}
	
	return 0;
}

int
main(int argc, char **argv) {
	int ret, f;
	if(argc==1)
		return sh(0,1);
	
	if((f=open(argv[1],O_RDONLY)) == -1) {
		writestr(2,"sh: ");
		writestr(2,argv[1]);
		writestr(2,": Can't open\n");
		return 1;
	}
	
	ret=sh(f,0);
	
	close(f);
	
	return ret;
}
