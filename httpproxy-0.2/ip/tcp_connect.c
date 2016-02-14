#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ip_misc.h"


#define	Export


/*
 *  Open a TCP connection to REMOTE.  Bind the local port to
 *  LOCAL, if set.
 *  Returns the file descriptor for the connection, or negative
 *  on errors.
 */
Export	int
tcp_connect(const struct sockaddr_in	* remote,
	    const struct sockaddr_in	* local)
{
    int			  s;

    s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0)
	return -1;
    if (local  &&  bind(s, (struct sockaddr *)local, sizeof *local) < 0)
    {	int saved_errno = errno;
	close(s);
	errno = saved_errno;
	return -1;
    }
    if (connect(s, (struct sockaddr *)remote, sizeof *remote) < 0)
    {	int saved_errno = errno;
	close(s);
	errno = saved_errno;
	return -1;
    }
    return s;
}
