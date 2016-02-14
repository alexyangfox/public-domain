#include <sys/types.h>
#include <unistd.h>

ssize_t
aread(int fd, void *buf, size_t count) {
	void *p;
	ssize_t w;
	for(p=buf;p<buf+count;p+=w)
		if((w=read(fd,p,1))==-1)
			return -1;
		else if((!w) || (*(char*)p=='\n')) {
			p+=w;
			break;
		}
	return p-buf;
}
