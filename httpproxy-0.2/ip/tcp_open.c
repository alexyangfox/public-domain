#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ip_misc.h"


#define	Export



/*
 *  Open a TCP connection to port REMOTE_PORT on host REMOTE_HOST.
 *  If LOCAL_HOST is not NULL, the local end of the connection is
 *  bound to that address.  If LOCAL_PORT is not NULL, the local
 *  end of the connection is bound to that port.
 *  Hosts may be given as either a numeric IP address or as a host
 *  names.  Ports may be given as either a decimal port number, or
 *  as a symbolic service name.
 *  
 *  Returns the file descriptor for the connection, or negative on
 *  errors.
 */
Export	int
tcp_open(const char *remote_host,
	 const char *remote_port,
	 const char *local_host,
	 const char *local_port)
{
    struct sockaddr_in	  server;
    struct sockaddr_in	  local;
    struct hostent	* he;
    int			  s;

#define ERRORRET(n) do { int e=errno;close(s);errno=e;return (n);} while (0)

    if (remote_host == NULL  ||  remote_port == NULL) 
    {
	errno = EINVAL;
	return -1;
    }

    if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	return -1;
    if (local_host  ||  local_port)
    {
	if (get_inaddr(&local, local_host, local_port, "tcp") < 0)
	    ERRORRET(-1);
	if (bind(s, (struct sockaddr*)&local, sizeof local) < 0)
	    ERRORRET(-1);
    }

    /* Get port */
    if (get_inaddr(&server, NULL, remote_port, "tcp") < 0)
	ERRORRET(-1);

    /* Check for numerical IP address */
    server.sin_addr.s_addr = inet_addr(remote_host);
    if (server.sin_addr.s_addr != (unsigned long)-1)
    {
	if (connect(s, (struct sockaddr*)&server, sizeof server) < 0)
	    ERRORRET(-1);
	return s;
    }

    /*  Not numerical address, then it should be a host name.
     *  gethostbyname() it and try all the hosts addresses.
     */
    if ((he = gethostbyname(remote_host)) == NULL)
	ERRORRET(-1);
    assert(he->h_addrtype == AF_INET);
    assert(he->h_length == sizeof server.sin_addr.s_addr);

    server.sin_family = he->h_addrtype;
    {
	char **a = he->h_addr_list;
	while (*a) 
	{
	    memcpy(&server.sin_addr, *a, sizeof server.sin_addr);
	    if (connect(s, (struct sockaddr*)&server, sizeof server) >= 0)
		return s;
	    a++;
	}
    }
    ERRORRET(-1);
}
