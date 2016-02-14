#include <sys/types.h>
#include <unistd.h>

ssize_t
awrite(int fd, void *buf, size_t count) {
	void *p;
	ssize_t w;
	for(p=buf;p<buf+count;p+=w)
		if((w=write(fd,p,count-(p-buf)))==-1)
			return -1;
		else if(!w)
			break;
			
	return p-buf;
}
