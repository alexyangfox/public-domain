#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ip_misc.h"


#define	Export


/*
 *  Fill in ADDR from HOST, SERVICE and PROTOCOL.
 *  PROTOCOL can be tcp or udp.
 *  Supplying a null pointer for HOST means use INADDR_ANY.
 *  Supplying a null pointer for SERVICE, means use port 0, i.e. no port.
 *
 *  Returns negative on errors, zero or positive if everything ok.
 */
Export	int
get_inaddr(struct sockaddr_in	* addr,
	   const char		* host,
	   const char		* service,
	   const char		* protocol)
{
    memset(addr, 0, sizeof *addr);
    addr->sin_family = AF_INET;

    /*
     *  Set host part of ADDR
     */
    if (host == NULL)
	addr->sin_addr.s_addr = INADDR_ANY;
    else
    {
	addr->sin_addr.s_addr = inet_addr(host);
	if (addr->sin_addr.s_addr == (unsigned long)-1)
	{
	    struct hostent	* hp;

	    hp = gethostbyname(host);
	    if (hp == NULL)
		return -1;
	    memcpy(&addr->sin_addr, hp->h_addr, hp->h_length);
	    addr->sin_family = hp->h_addrtype;
	}
    }

    /*
     *  Set port part of ADDR
     */
    if (service == NULL)
	addr->sin_port = htons(0);
    else
    {
	char		* end;
	long		  portno;

	portno = strtol(service, &end, 10);
	if (portno > 0  &&  portno <= 65535
	    &&  end != service  &&  *end == '\0')
	{
	    addr->sin_port = htons(portno);
	}
	else
	{
	    struct servent	* serv;

	    serv = getservbyname(service, protocol);
	    if (serv == NULL)
		return -1;
	    addr->sin_port = serv->s_port;
	}
    }

    return 0;
}
