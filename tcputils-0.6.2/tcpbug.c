/*
 *  tcpbug.c  --  Accept a TCP/IP connection, and relay it to somewhere
 *		  else, while spying on the connection
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
 *  email:	Bellman@Lysator.LiU.SE
 *
 *
 *  Any opinions expressed in this code are the author's PERSONAL opinions,
 *  and does NOT, repeat NOT, represent any official standpoint of Lysator,
 *  even if so stated.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <locale.h>
#include <limits.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ip/ip_misc.h"
#include "relay.h"


extern	int	getopt(int, char *const*, const char *);
extern	int	optind;
extern	char	*optarg;




char		* progname		= "tcpbug";
int		  quiet_mode		= 0;

int		  linewidth		= -1;
/* Non-zero if a bytecount should be logged */
int		  bug_bytecount		= 0;
/* Non-zero if logged data should be timestamped */
int		  bug_timestamp		= 0;
/* Non-zero if logged data should be timestamped relative to the
   establishing of the connection */
int		  bug_relative_timestamp= 0;


struct bugstat {
	/* The prefix character */
	char		  prefix;
	/* Where to send bugged data */
	FILE		* logfile;
	/* Bytes passed this way */
	unsigned long	  nbytes;
	/* Function to do actual printing of bugged data */
	int 		(*bugfun)(struct relay *, char *, size_t);
	/* Time when bugger() was called */
	struct timeval	  time;
	/* Time when the connection was established */
	struct timeval	  conntime;
};



void
log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (!quiet_mode)
	vfprintf(stderr, fmt, args);
    va_end(args);
}


void
fatal(char	* msg)
{
    fprintf(stderr, "%s: %s: %s\n", progname, msg, strerror(errno));
    exit(1);
}



#define COLON()	(things_printed++ ? putc(':', bs->logfile),++*pos : 0)

int
prefix(struct relay	* rel,
       int		  eof,
       int		* pos)
{
    struct bugstat	* bs			= rel->userdata;
    int			  things_printed	= 0;

    if (bug_bytecount) {
	char buf[sizeof bs->nbytes * CHAR_BIT];
	COLON();
	sprintf(buf, "%04lx", bs->nbytes);
	fputs(buf, bs->logfile);
	*pos += strlen(buf);
    }
    if (bug_timestamp) {
	char buf[64];
	COLON();
	sprintf(buf, "%ld.%06ld", bs->time.tv_sec, bs->time.tv_usec);
	fputs(buf, bs->logfile);
	*pos += strlen(buf);
    }
    if (bug_relative_timestamp) {
	char buf[64];
	struct timeval diff;
	COLON();
	diff.tv_sec = bs->time.tv_sec - bs->conntime.tv_sec;
	diff.tv_usec = bs->time.tv_usec - bs->conntime.tv_usec;
	if (diff.tv_usec < 0)
	    diff.tv_usec += 1000000, diff.tv_sec -= 1;
	sprintf(buf, "%ld.%06ld", diff.tv_sec, diff.tv_usec);
	fputs(buf, bs->logfile);
	*pos += strlen(buf);
    }
    putc(bs->prefix, bs->logfile);
    if (eof)
	fputs("EOF\n", bs->logfile);
    else
	putc(' ', bs->logfile);

    return 0;
}



int
bugger(struct relay	* rel,
       char		* data,
       size_t		  datalen)
{
    struct bugstat	* bs		= rel->userdata;
    int			  pos		= 0;

    gettimeofday(&bs->time, NULL);
    if (datalen == 0) {
	prefix(rel, -1, &pos);
	return 0;
    }

    bs->bugfun(rel, data, datalen);
    fflush(bs->logfile);

    return 0;
}



int
charbugger(struct relay	* rel,
	   char		* data,
	   size_t	  datalen)
{
    int pos = 0;
    struct bugstat *bs = rel->userdata;

    for ( ;  datalen > 0 ;  datalen--, data++, bs->nbytes++)
    {
	register int c = (unsigned char)*data;

	if (pos == 0)
	    prefix(rel, 0, &pos);

	if (isprint(c)) {
	    putc(' ', bs->logfile);
	    putc(' ', bs->logfile);
	    putc(c, bs->logfile);
	} else if (c == '\n') {
	    putc(' ', bs->logfile);
	    putc('\\', bs->logfile);
	    putc('n', bs->logfile);
	} else if (c == '\r') {
	    putc(' ', bs->logfile);
	    putc('\\', bs->logfile);
	    putc('r', bs->logfile);
	} else if (c == '\b') {
	    putc(' ', bs->logfile);
	    putc('\\', bs->logfile);
	    putc('b', bs->logfile);
	} else if (c == '\f') {
	    putc(' ', bs->logfile);
	    putc('\\', bs->logfile);
	    putc('f', bs->logfile);
	} else if (c == '\t') {
	    putc(' ', bs->logfile);
	    putc('\\', bs->logfile);
	    putc('t', bs->logfile);
	} else {
	    putc('0' + (c>>6 & 7), bs->logfile);
	    putc('0' + (c>>3 & 7), bs->logfile);
	    putc('0' + (c>>0 & 7), bs->logfile);
	}
	putc(' ', bs->logfile);
	pos += 4;

	if (linewidth > 0  &&  pos >= linewidth) {
	    putc('\n', bs->logfile);
	    pos = 0;
	}
    }
    if (pos != 0)
	putc('\n', bs->logfile);

    return 0;
}



int
hexbugger(struct relay	* rel,
	  char		* data,
	  size_t	  datalen)
{
    int pos = 0;
    struct bugstat *bs = rel->userdata;
    static char *hexdigits = "0123456789abcdef";

    for ( ;  datalen > 0 ;  datalen--, data++, bs->nbytes++)
    {
	register int c = (unsigned char)*data;

	if (pos == 0)
	    prefix(rel, 0, &pos);

	putc(hexdigits[c>>4 & 0xf], bs->logfile);
	putc(hexdigits[c>>0 & 0xf], bs->logfile);
	putc(' ', bs->logfile);
	pos += 3;

	if (linewidth > 0  &&  pos >= linewidth) {
	    putc('\n', bs->logfile);
	    pos = 0;
	}
    }
    if (pos != 0)
	putc('\n', bs->logfile);

    return 0;
}



int
linebugger(struct relay	* rel,
	   char		* data,
	   size_t	  datalen)
{
    int pos = 0;
    struct bugstat *bs = rel->userdata;

    for ( ;  datalen > 0 ;  datalen--, data++, bs->nbytes++)
    {
	register int c = (unsigned char)*data;

	if (pos == 0)
	    prefix(rel, 0, &pos);

	putc(c, bs->logfile); pos++;
	if (c == '\n') {
	    pos = 0;
	}
#if 0
	if (linewidth > 0  &&  pos >= linewidth) {
	    putc('\n', bs->logfile);
	    pos = 0;
	}
#endif
    }
    if (pos != 0)
	putc('\n', bs->logfile);

    return 0;
}



void
usage ()
{
    fprintf(stderr,
	    "usage: %s [-cx] [-btT] [-w linewidth] my-port  server-host  server-port\n",
	    progname);
    fprintf(stderr,
	    "\t%s is a program that bugs a TCP/IP connection.  It waits\n"
	    "\tfor connections on a port that you specify, and then connects\n"
	    "\tto a server on a machine and a port you specify.  It then\n"
	    "\tprints out everything that is sent on the connection to\n"
	    "\tstdout.\n", progname
	);
    exit(1);
}


int
main (argc, argv)
      int argc;
      char **argv;
{
    struct sockaddr_in	  cliaddr;
    int			  cliaddrlen	= sizeof cliaddr;
    int			  listen_fd;
    int			  client_fd;
    int			  server_fd;
    int			  option;
    int (*bugfun)(struct relay *, char *, size_t) = NULL;

    progname = argv[0];

    setlocale(LC_CTYPE, "");

    while ((option = getopt (argc, argv, "cxw:tTb")) != EOF)
    {
	switch ((char)option)
	{
	case 'c':
	    if (bugfun != NULL)
		usage();
	    bugfun = charbugger;
	    if (linewidth < 0)
		linewidth = 72;
	    break;
	case 'x':
	    if (bugfun != NULL)
		usage();
	    bugfun = hexbugger;
	    if (linewidth < 0)
		linewidth = 72;
	    break;
	case 'w':
	    linewidth = atoi(optarg);
	    break;
	case 't':
	    bug_timestamp = 1;
	    break;
	case 'T':
	    bug_relative_timestamp = 1;
	    break;
	case 'b':
	    bug_bytecount = 1;
	    break;
	default:
	    usage();
	    /* NOTREACHED */
	}
    }
    if (argc - optind != 3)
	usage();

    if (bugfun == NULL)
	bugfun = linebugger;

    listen_fd = tcp_listen(NULL, argv[optind]);
    if (listen_fd < 0)
	fatal("can't listen to port");
    client_fd = accept(listen_fd, (struct sockaddr *)&cliaddr, &cliaddrlen);
    if (client_fd < 0)
	fatal("accept failed");
    log("[Connection from %s port %d]\n",
	inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
    close(listen_fd);
    server_fd = tcp_open(argv[optind+1], argv[optind+2], NULL, NULL);
    if (server_fd < 0)
	fatal("can't connect to server");
    
    {
	struct relay	  fds[2];
	struct bugstat	  buggers[2];
	struct timeval	  now;

	gettimeofday(&now, NULL);
	signal(SIGPIPE, SIG_IGN);

	fds[0].source = client_fd;
	fds[0].dest = server_fd;
	fds[0].readerror = 0;
	fds[0].writeerror = 0;
	fds[0].userdata = &buggers[0];
	buggers[0].nbytes = 0;
	buggers[0].prefix = '>';
	buggers[0].logfile = stdout;
	buggers[0].bugfun = bugfun;
	buggers[0].conntime = now;

	fds[1].source = server_fd;
	fds[1].dest = client_fd;
	fds[1].readerror = 0;
	fds[1].writeerror = 0;
	fds[1].userdata = &buggers[1];
	buggers[1].nbytes = 0;
	buggers[1].prefix = '<';
	buggers[1].logfile = stdout;
	buggers[1].bugfun = bugfun;
	buggers[1].conntime = now;

	relay_all(fds, 2, bugger);
    }

    return 0;
}
