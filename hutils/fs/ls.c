#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "../libhutils.h"

char oneflag, aflag, Aflag, dflag, fflag, showdnames, *namebuf;
size_t namei;

void
usage(void) {
	writestr(2,"usage: ls [-1aAdf] [DIR ...] [FILE ...]\n");
}

void
outfname(char *s) {
	if(fflag) {
		writestr(1,s);
		writestr(1,"\n");
	} else {
		namebuf=sizeset(namebuf,namei+strlen(s)+1);
		strlcpy(namebuf+namei,s,strlen(s)+1);
		namei+=strlen(s)+1;
	}
}

int
sortfunc(const void *a, const void *b) {
	return strcmp(*(char**)a,*(char**)b);
}

void
printnames(void) {
	char *p, **list, **pp;
	size_t i;
	
	i=0;
	list=(char**)newbuf();
	list=(char**) sizeset((char*)list, (i+1)*sizeof(char*));
	
	list[i++]=namebuf;
	for(p=namebuf; p<namebuf+namei; p++) {
		if(!*p) {
			list=(char**) sizeset((char*)list, (i+1)*sizeof(char*));
			list[i++]=p+1;
		}
	}
	list[--i]=0;
	
	qsort(list,i,sizeof(char*),sortfunc);
	
	for(pp=list; *pp; pp++) {
		writestr(1,*pp);
		writestr(1,"\n");
	}
	
	free(list);
	free(namebuf);
	namebuf=newbuf();
}

int
lsdir(char *name) {
	int ret;
	DIR *d;
	struct dirent *ent;
	char *onamebuf;
	size_t onamei;
	
	if(showdnames) {
		writestr(1,name);
		writestr(1,":\n");
	}
	
	if(!(d=opendir(name)))
		return -1;
	
	onamebuf=namebuf;
	onamei=namei;
	namei=0;
	namebuf=newbuf();
	
	while((ent=readdir(d)))
		if(aflag || Aflag || *(ent->d_name)!='.')
			if(aflag || (strcmp(ent->d_name,"..") && strcmp(ent->d_name,"."))) {
				outfname(ent->d_name);
			}
	
	ret=closedir(d);
	
	if(!fflag)
		printnames();
	
	if(showdnames)
		writestr(1,"\n");
	
	namebuf=onamebuf;
	namei=onamei;
	
	return ret;
}

int
ls(char *name) {
	struct stat st;
	if(lstat(name,&st)) {
		if(errno==ENOENT) {
			writestr(2,"ls: ");
			writestr(2,name);
			writestr(2,": not found\n");
		}
		return -1;
	}
	if(!dflag && S_ISDIR(st.st_mode))
		return lsdir(name);
	outfname(name);
		
	return 0;
}

int
main(int argc, char **argv) {
	char *p, **argp, returnval;
	
	oneflag=0;
	aflag=0;
	Aflag=0;
	dflag=0;
	fflag=0;
	for(argp=argv+1;*argp&&**argp=='-';argp++)
		for(p=*argp+1;*p;p++)
			switch(*p) {
				case 'a':
					aflag=1;
					break;
				case 'A':
					Aflag=1;
					break;
				case 'd':
					dflag=1;
					break;
				case 'f':
					fflag=1;
					break;
				case '1':
					oneflag=1;
					break;
				default:
					usage();
					return 1;
			}
	
	namei=0;
	namebuf=newbuf();
	
	showdnames=0;
	if(argc-(argp-argv)==0) {
		if(lsdir("."))
			return 1;
		return 0;
	}
	else if(argc-(argp-argv)>1)
		showdnames=1;
		
	for(returnval=0;*argp;argp++)
		if(ls(*argp))
			returnval=1;
	if(!fflag)
		printnames();
	return returnval;
}
