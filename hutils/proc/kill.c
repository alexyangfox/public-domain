#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>

void
usage(void) {
	fputs("usage: kill [-SIG] PID\n", stderr);
}

int
main(int argc, char **argv) {
	char **argp, *endp;
	pid_t pid;
	int sig;
	
	argp=argv+1;
	if(argc<2) {
		usage();
		return 1;
	}
	
	sig=15; /* SIGTERM */
	if(**argp=='-') { /* Assume signal number */
		sig=strtol(*argp+1, &endp, 10);
		if(*endp!=0) {
			fprintf(stderr, "kill: '%s' not valid SIG\n", *argp+1);
			return 1;
		}
		argp++;
	}
	
	if(!*argp) {
		usage();
		return 1;
	}
	
	pid=strtoumax(*argp, &endp, 10);
	if(*endp!=0) {
		fprintf(stderr, "kill: '%s' not valid PID\n", *argp+1);
		return 1;
	}
	
	if(kill(pid, sig)) {
		if(errno==EINVAL)
			fprintf(stderr, "kill: '%i' not valid SIG\n", sig);
		else if(errno==EPERM)
			fprintf(stderr, "kill: can't kill %i\n", pid);
		else if(errno==ESRCH)
			fprintf(stderr, "kill: no proc %i\n", pid);
		else
			fputs("kill: WTF\n", stderr);
		return 1;
	}
	
	return 0;
}
