#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


extern	int	 getopt(int, char *const*, const char *);
extern	int	 optind;
extern	char	*optarg;


char	* progname		= "getpeername";

int	  numeric_addresses	= 0;
char	* protocol		= NULL;
int	  sockno		= 0;



void
usage(void)
{
    fprintf(stderr, "Usage: %s [-n] [-p protocol]\n", progname);
    exit(1);
}


void
fatal(char	* msg)
{
    fprintf(stderr, "%s: %s: %s\n", progname, msg, strerror(errno));
    exit(1);
}


int
main(int argc,
     char **argv)
{
    int			  option;
    struct sockaddr_in	  peer;
    int			  peersize	= sizeof peer;
    char		* hostname;
    char		* portname;
    char		  servbuf[64];

    progname = argv[0];

    while ((option = getopt(argc, argv, "np:")) != EOF)
    {
	switch ((char)option)
	{
	case 'n':
	    numeric_addresses = 1;
	    break;
	case 'p':
	    protocol = optarg;
	    break;
	default:
	    usage();
	    /* NOTREACHED */
	}
    }

    if (argc - optind > 0)
	usage();

    if (getpeername(sockno, (struct sockaddr*)&peer, &peersize) < 0)
	fatal("can't get peer name");
    if (peer.sin_family != AF_INET)
	fatal("socket is not an IP socket");

    /* Find the type of the socket.  Assumes that an INET stream socket
     * is TCP and an INET datagram socket is UDP.  Can't handle AF_UNIX
     * or any other adress families.
     */
    if (protocol == NULL)
    {
	int socktype;
	int socktypelen = sizeof socktype;

	if (getsockopt(sockno, SOL_SOCKET, SO_TYPE,
		       (char*)&socktype, &socktypelen) < 0)
	    fatal("can't get socket type");

	switch (socktype)
	{
	case SOCK_STREAM:
	    protocol = "tcp";
	    break;
	case SOCK_DGRAM:
	    protocol = "udp";
	    break;
	default:
	    fatal("unknown socket type");
	    /* NOTREACHED */
	}
    }

    hostname = inet_ntoa(peer.sin_addr);
    sprintf(servbuf, "%u", ntohs(peer.sin_port));
    portname = servbuf;

    if (!numeric_addresses) 
    {
	struct hostent *host;
	struct servent *service;

	host = gethostbyaddr((char*)&peer.sin_addr.s_addr,
			     sizeof peer.sin_addr.s_addr, AF_INET);
	if (host != NULL)
	    hostname = host->h_name;
	service = getservbyport(peer.sin_port, protocol);
	if (service != NULL)
	    portname = service->s_name;
    }

    printf("%s %s\n", hostname, portname);

    return 0;
}
