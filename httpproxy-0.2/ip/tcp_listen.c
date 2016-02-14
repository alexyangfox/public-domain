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
 *  Open a TCP socket listening on address LOCAL.
 *  Returns the file descriptor for the socket, or negative
 *  on errors.
 */
Export	int
tcp_listen_2(const struct sockaddr_in	* local)
{
    int			  s;
    struct protoent	* proto;

    proto = getprotobyname("tcp");
    if (proto == NULL)
	return -1;
    s = socket(PF_INET, SOCK_STREAM, proto->p_proto);
    if (s < 0)
	return -1;
    {
	int yes = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof yes);
    }
    if (bind(s, (struct sockaddr *)local, sizeof *local) < 0)
    {
	close(s);
	return -1;
    }
    if (listen(s, 256) < 0) 
    {
	close(s);
	return -1;
    }

    return s;
}



Export	int
tcp_listen(const char	* interface,
	   const char	* port)
{
    int			  s;
    struct sockaddr_in	  server;

    if (get_inaddr(&server, interface, port, "tcp") < 0)
	return -1;
    s = tcp_listen_2(&server);
    return s;
}
