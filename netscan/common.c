#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

void *
emalloc(size_t size)
{
	void *v = malloc(size);

	if (v == NULL)
		err(1, "malloc");
	return v;
}

void *
emallocz(size_t size)
{
	return memset(emalloc(size), 0, size);
}

void *
erealloc(void *ptr, size_t size)
{
	void *v = realloc(ptr, size);

	if (v == NULL)
		err(1, "remalloc");
	return v;
}

char *
estrdup(char *s)
{
	char *ns = strdup(s);

	if (ns == NULL)
		err(1, "strdup");
	return ns;
}

void
eclose(int fd)
{
	if (close(fd) == -1)
		err(1, "close");
}

/* returns count of decimal digits in i */
int
ilen(int i)
{
	int len;

	if (i == 0)
		return 1;
	for (len = 0; i > 0; len++)
		i /= 10;
	return len;
}

void
block(int fd)
{
	int flags = fcntl(fd, F_GETFL);

	if (flags == -1)
		err(1, "fcntl");
	flags &= ~O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) == -1)
		err(1, "fcntl");
}

void
unblock(int fd)
{
	int flags = fcntl(fd, F_GETFL);

	if (flags == -1)
		err(1, "fcntl");
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) == -1)
		err(1, "fcntl");
}
