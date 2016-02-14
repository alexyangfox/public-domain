#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include "proxy.h"

int setup_proxy(struct relay	* relays,
		int		  nrelays,
		char		* proxystring,
		int		  flags,
		int (*callback)(int, int, char *, size_t)
    )
{
    char	  buffer[8193];
    int		  bytes_written;
    int		  bytes_read;
    int		  offset;
    int		  HTTP1ver;
    int		  status;

    snprintf(buffer, sizeof(buffer)-1, "CONNECT %s HTTP/1.0\r\n\r\n%s",
	     proxystring, flags&1 ? "\r\n" : "");

    if (callback)
	(*callback)(0, 1, buffer, strlen(buffer));
    bytes_written = write(relays[0].dest, buffer, strlen(buffer));
    if (bytes_written < 0)
    {
	relays[0].writeerror = errno;
	shutdown(relays[0].source, 0);
	errno = relays[0].writeerror;
	return -1;
    }

    bytes_read = read(relays[1].source, buffer, sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';

    if (sscanf(buffer, "HTTP/1.%d %o ", &HTTP1ver, &status) < 2)
    {
	if (callback)
	{
	    (*callback)(1, 0, buffer, (80 > bytes_read ? bytes_read : 80));
	}
	errno = EPERM;
	return -1;
    }

    offset = 0;
    do
    {
	char *p = strchr(buffer + offset + 1, '\r');
	if (p)
	    offset = p - buffer;
	else
	    offset = bytes_read;
    } while (offset < bytes_read
	     && strncmp(buffer + offset, "\r\n\r\n", 4) != 0);
    offset += 4;

    if (callback)
	(*callback)(1, 1, buffer, offset);

    switch(status)
    {
    case 0200:
	break;
    default:
	errno = EACCES;
	return -1;
    }

    if (offset < bytes_read)
    {
	relays[1].buffer = malloc(bytes_read - offset);
	relays[1].bufferlen = bytes_read - offset;
	relays[1].bufferoff = 0;
    }
    return 0;
}
