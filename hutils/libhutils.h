#ifndef _LIBHUTILS_H
#define _LIBHUTILS_H

#include <sys/types.h>

ssize_t awrite(int,void*,size_t);
ssize_t writestr(int,char*);
ssize_t aread(int,void*,size_t);
size_t strlcpy(char*,const char*,size_t);
size_t strlcat(char*,const char*,size_t);
void *newbuf(void);
void *sizeset(void*,size_t);

#endif
