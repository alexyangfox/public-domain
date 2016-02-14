/*
 *  tcplisten.c
 *		Wait for a connection to a TCP/IP port, send standard input
 *		to that connection, and print messages from that connection
 *		on standard output.
 *
 *
 *  This program is in the public domain.  You may do anything
 *  you like with it.
 *
 *
 *  Author:	Thomas Bellman
 *		Lysator Computer Club
 *		Linköping University
 *		Sweden
 *  email:	Bellman@Lysator.LiU.Se
 *
 *
 *  Any opinions expressed in this code are the author's PERSONAL opinions,
 *  and does NOT, repeat NOT, represent any official standpoint of Lysator,
 *  even if so stated.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ip/ip_misc.h"
#include "relay.h"


extern	int	getopt(int, char *const*, const char *);
extern	int	optind;



char	* progname		= "tcplisten";
int	  verbose		= 0;


void
usage(void)
{
    fprintf(stderr, "Usage: %s [-irv] [localaddr] port\n", progname);
    exit(1);
}


void
log(const char *fmt, ...)
{
    va_list args;
    if (!verbose)
	return;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}


void
fatal(char	* msg)
{
    fprintf(stderr, "%s: %s: %s\n", progname, msg, strerror(errno));
    exit(1);
}




int
main (int	   argc,
      char	** argv)

{
    int		  lp;
    int		  option;
    int		  close_on_stdin_eof	= 0;
    int		  close_on_remote_eof	= 0;

    progname = argv[0];
    while ((option = getopt(argc, argv, "irv")) != EOF)
    {
	switch ((char)option)
	{
	case 'i':
	    close_on_stdin_eof = 1;
	    break;
	case 'r':
	    close_on_remote_eof = 1;
	    break;
	case 'v':
	    verbose = 1;
	    break;
	default:
	    usage();
	    /* NOTREACHED */
	}
    }

    if (argc - optind == 1)
	lp = tcp_listen(NULL, argv[optind]);
    else if (argc - optind == 2)
	lp = tcp_listen(argv[optind], argv[optind+1]);
    else
	usage();
    /* Some compilers might complain that 'lp' might be used uninitialized
     * here, but that's because they don't realize that usage() doesn't
     * return.  */
    if (lp < 0)
	fatal("Unable to listen to port");

    {
	int			  client;
	struct sockaddr_in	  cliaddr;
	int			  cliaddrsize	= sizeof cliaddr;
	struct relay		  fds[2];
	int			  nclosed	= 0;

	client = accept(lp, (struct sockaddr*)&cliaddr, &cliaddrsize);
	if (client < 0)
	    fatal("Accept failed");
	close(lp);
	log("[Connection from %s port %d]\n",
	    inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

	fds[0].source = 0;
	fds[0].dest = client;
	fds[0].writeerror = 0;
	fds[0].readerror = 0;

	fds[1].source = client;
	fds[1].dest = 1;
	fds[1].writeerror = 0;
	fds[1].readerror = 0;

	signal(SIGPIPE, SIG_IGN);
	do {
	    int status = relay_once(fds, 2, NULL, NULL);
	    if (status < 0)
		nclosed += -status;
	    if (close_on_stdin_eof  &&
		(fds[0].readerror  ||  fds[1].writeerror))
	    {
		break;
	    }
	    if (close_on_remote_eof  &&
		(fds[1].readerror  ||  fds[0].writeerror))
	    {
		break;
	    }
	} while (nclosed < 2);
	log("[Connection closed]\n");
    }
    
    return 0;
}
