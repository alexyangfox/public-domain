/*
 *  tcpproxy.c
 *		Start a program, attempt to set up a HTTP proxy tunnel
 *		through it, then send standard input to that program,
 *		and print messages from that program on standard output.
 *
 *
 *  This program is in the public domain.  You may do anything
 *  you like with it.
 *
 *
 *  Author:	Richard Levitte
 *		Sweden
 *  email:	Richard@Levitte.org
 *
 *  The code in this program is very much inspired from the code by Thomas
 *  Bellman in his tcp utils.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ip/ip_misc.h"
#include "proxy.h"
#include "util.h"


extern  char   **environ;
extern	int	 getopt(int, char *const*, const char *);
extern	int	 optind;
extern	char	*optarg;


char	* progname		= "httpproxy";
int	  debug			= 0;
int	  verbose		= 0;
int	  std_in		= STDIN_FILENO;
int	  std_out		= STDOUT_FILENO;


void
usage(void)
{
    fprintf(stderr,
	    "Usage: %s [-idrsv] [-l addr:port] host:port program [argv0 argv1 ...]\n",
	    progname);
    exit(1);
}


void
log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (verbose)
	vfprintf(stderr, fmt, args);
    va_end(args);
}


static	void
vfatal(const char *fmt, va_list args)
{
    int save_errno = errno;

    fputs(progname, stderr);
    fputs(": ", stderr);
    vfprintf(stderr, fmt, args);
    fputs(": ", stderr);
    fputs(strerror(save_errno), stderr);
    putc('\n', stderr);
    exit(1);
}


static	void
fatal(const char *fmt, ...)
{
    va_list	args;

    va_start(args, fmt);
    vfatal(fmt, args);
    va_end(args);
}



static	void
reap_children(int signo)
{
    pid_t child;
    int status;
    char buf[64 + sizeof(long) * CHAR_BIT];

    while ((child = waitpid(-1, &status, WNOHANG)) > (pid_t)0)
    {
	if (debug)
	{
	    sprintf(buf, "Child %ld died\n", (long)child);
	    write(2, buf, strlen(buf));
	}
    }
    signal(signo, &reap_children);
}



int proxy_debug_callback(int inorout, int binortxt,
			 char *buffer, size_t buflen)
{
    switch(inorout)
    {
    case 0:
	if (binortxt)
	    fprintf(stderr, "[Sent]\n");
	else
	    dump_buffer(stderr, "[Sent]", buffer, buflen);
	break;
    case 1:
	if (binortxt)
	    fprintf(stderr, "[Received]\n");
	else
	    dump_buffer(stderr, "[Received]", buffer, buflen);
	break;
    }
    if (binortxt)
	fwrite(buffer, buflen, 1, stderr);
    return 0;
}

int relay_debug_callback(struct relay *r, char *buffer, size_t buflen)
{
    static int accum_in = 0;
    static int accum_out = 0;
    static int status_in = 0;
    static int status_out = 0;

    if (buffer == NULL)
    {
	if (r->source == std_in)
	    status_in =
		(r->readerror < 0 ? 0x01 : 0)
		|(r->writeerror ? 0x10 : 0);
	else
	    status_out =
		(r->readerror < 0 ? 0x01 : 0)
		|(r->writeerror ? 0x10 : 0);

	fprintf(stderr, "\n[ > ____%02x____  < ____%02x____ ]\n",
		status_in, status_out);
    }
    else
    {
	if (r->source == std_in)
	    accum_out += buflen;
	else
	    accum_in += buflen;

	fprintf(stderr, "[ > %10d  < %10d ]\r", accum_in, accum_out);
    }
    return 0;
}

int relay_dump_callback(struct relay *r, char *buffer, size_t buflen)
{
    char *heading;
    if (buffer == NULL)
	relay_debug_callback(r, buffer, buflen);
    else
    {
	if (r->source == std_in)
	    heading = "[> ] ";
	else
	    heading = "[< ] ";
	dump_buffer(stderr, heading, buffer, buflen);
    }
    return 0;
}

int
main (int	   argc,
      char	** argv)

{
    int		  sd			= -1;
    int		  option;
    int		  flags			= 0;
    int		  close_on_stdin_eof	= 0;
    int		  close_on_remote_eof	= 0;
    char	* localaddr		= NULL;
    char	* localport		= NULL;
    int (*pcb)(int, int, char *, size_t)	= NULL;
    int (*rcb)(struct relay *, char *, size_t)	= NULL;

    progname = argv[0];
    while ((option = getopt(argc, argv, "+idersvl:")) != EOF)
    {
	switch ((char)option)
	{
	case 'i':
	    close_on_stdin_eof = 1;
	    break;
	case 'r':
	    close_on_remote_eof = 1;
	    break;
	case 'e':
	    flags |= 1;
	    break;
	case 's':
	    flags |= 2;
	    break;
	case 'l':
	    localaddr = optarg;
	    if ((localport = strrchr(localaddr, ':')) != NULL) {
		if (localaddr == localport)
		    localaddr = NULL;
		*localport = '\0';
		localport++;
	    }
	    break;
	case 'v':
	    verbose = 1;
	    break;
	case 'd':
	    switch(++debug)
	    {
	    case 1:
		pcb = proxy_debug_callback;
		break;
	    case 2:
		rcb = relay_debug_callback;
		break;
	    case 3:
		rcb = relay_dump_callback;
		break;
	    }
	    break;
	default:
	    usage();
	    /* NOTREACHED */
	}
    }

    if (argc - optind < 2)
	usage();

    if (localaddr || localport)
    {
	struct sockaddr_in	  local;

	if ((sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0
	    || get_inaddr(&local, localaddr, localport, "tcp") < 0
	    || bind(sd, (struct sockaddr*)&local, sizeof local) < 0)
	    fatal("Can't listen to port %s", localport);
	std_in = std_out = sd;
    }
	

    {
	int			  pipes[2][2]; /* tcpproxy->program,
						  program->tcpproxy */
	struct relay		  fds[2];
	int			  nclosed	= 0;
	pid_t			  child;

	if (flags&2)
	{
	    int fd[2];
	    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0)
		fatal("Can't create socket pair");
	    pipes[0][0] = fd[1];
	    pipes[0][1] = fd[0];
	    pipes[1][0] = fd[0];
	    pipes[1][1] = fd[1];
	}
	else
	{
	    if (pipe(pipes[0]) < 0)
		fatal("Can't create pipe");
	    if (pipe(pipes[1]) < 0)
		fatal("Can't create pipe");
	}

	fds[0].source = std_in;
	fds[0].dest = pipes[0][1];
	fds[0].writeerror = 0;
	fds[0].readerror = 0;
	fds[0].buffer = NULL;
	fds[0].bufferlen = 0;
	fds[0].bufferoff = 0;

	fds[1].source = pipes[1][0];
	fds[1].dest = std_out;
	fds[1].writeerror = 0;
	fds[1].readerror = 0;
	fds[1].buffer = NULL;
	fds[1].bufferlen = 0;
	fds[1].bufferoff = 0;

	/* Reap children */
#if defined(SIGCHLD)
	signal(SIGCHLD, &reap_children);
#elif defined(SIGCLD)
	signal(SIGCLD, &reap_children);
#endif

	switch (child = fork()) {
	case -1:
	    fatal("Can't fork");
	    break;
	case 0:
	    /* Child */
	    if (sd >= 0)
		close(sd);
	    close(pipes[0][1]);
	    if (pipes[1][0] != pipes[0][1])
		close(pipes[1][0]);
	    dup2(pipes[0][0], 0);
	    dup2(pipes[1][1], 1);
	    execve(argv[optind+1], argv+optind+2, environ);
	    fatal("Can't start %s", argv[optind+1]);
	default:
	    /* Parent */
	    if (debug)
		fprintf(stderr, "Forked child %ld\n", (long)child);
	    close(pipes[0][0]);
	    if (pipes[1][1] != pipes[0][0])
		close(pipes[1][1]);
	    break;
	}

	if (setup_proxy(fds, 2, argv[optind], flags, pcb) < 0)
	    fatal("Can't set up proxy");
	log("[Proxy set up]");

	signal(SIGPIPE, SIG_IGN);
	do {
	    int status = relay_once(fds, 2, NULL, rcb);
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
