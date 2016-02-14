#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "ip/ip_misc.h"


extern	char	** environ;
extern	int	   getopt(int, char *const*, const char *);
extern	int	   optind;
extern	char	 * optarg;


char	* progname		= "mini-inetd";
int	  debug			= 0;
int	  max_connections	= 0;


static	void
usage()
{
    fprintf(stderr,
	    "Usage:  %s [-d] [-m max] port program [argv0 argv1 ...]\n",
	    progname);
    exit(1);
}


static	void
vfatal(const char *fmt, va_list args)
{
    fputs(progname, stderr);
    fputs(": ", stderr);
    vfprintf(stderr, fmt, args);
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



int
main(int argc, char **argv)
{
    int sd;
    pid_t child;
    int option;
    char *host;
    char *port;

    while ((option = getopt (argc, argv, "dm:")) != EOF)
    {
	switch ((char)option)
	{
	case 'd':
	    debug++;
	    break;
	case 'm':
	    max_connections = atoi(optarg);
	    break;
	default:
	    usage();
	    /* NOTREACHED */
	}
    }

    if (argc - optind < 2)
	usage();

    port = strrchr(argv[optind], ':');
    if (port != NULL) {
	*(port++) = '\0';
	host = argv[optind];
    } else {
	port = argv[optind];
	host = NULL;
    }
    if ((sd = tcp_listen(host, port)) < 0)
	fatal("Can't listen to port %s: %s", argv[optind], strerror(errno));

    /* Reap children */
#if defined(SIGCHLD)
    signal(SIGCHLD, &reap_children);
#elif defined(SIGCLD)
    signal(SIGCLD, &reap_children);
#endif

    while (1)
    {
	struct sockaddr_in them;
	int len = sizeof them;
	int ns = accept(sd, (struct sockaddr*)&them, &len);
	if (ns < 0) {
	    if (errno == EINTR)
		continue;
	    fatal("Accept failed: %s", strerror(errno));
	}
	switch (child = fork()) {
	case -1:
	    perror("Can't fork");
	    break;
	case 0:
	    /* Child */
	    close(sd);
	    dup2(ns, 0);
	    dup2(ns, 1);
	    execve(argv[optind+1], argv+optind+2, environ);
	    fatal("Can't start %s: %s", argv[optind+1], strerror(errno));
	default:
	    /* Parent */
	    if (debug)
		fprintf(stderr, "Forked child %ld\n", (long)child);
	    close(ns);
	    break;
	}
	if (max_connections > 0  &&  --max_connections <= 0)
	    break;
    }

    return 0;
}

