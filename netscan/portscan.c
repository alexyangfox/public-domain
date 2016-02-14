#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#define __FAVOR_BSD
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "network.h"
#include "options.h"
#include "parse.h"
#include "hosts.h"
#include "ports.h"
#include "scan.h"

bool nflag;
int hwidth; /* hostname */
int swidth; /* service */
int pwidth; /* port */

static void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s [-46n] [-CEUAPRSF] [-a source address]"
	    " [-i interval] [-s[c|t|u]] [-I interface] [-p ports] [-t ttl]"
	    " [-w timeout] host ...\n", __progname); 
	exit(1);
}

static void
countwidth(struct options *opts) {
	struct host *host;
	struct port *port;
	struct protoent *protoent = getprotobynumber(opts->netopts.proto);

	struct servent *serv;
	int i, len;

	SIMPLEQ_FOREACH(host, opts->hosts, link) {
		len = strlen(hostname(host, nflag));
		if (len > hwidth)
			hwidth = len;
	}

	for (i = 0; i < opts->ports->len; i++) {
		port = opts->ports->list + i;
		serv = getservbyport(htons(port->portno), protoent ? protoent->p_name : NULL);

		len = strlen(serv ? serv->s_name : "unknown");
		if (len > swidth)
			swidth = len;

		len = ilen(port->portno) + 1 + strlen(protoent ?
		    protoent->p_name : "unknown");
		if (len > pwidth)
			pwidth = len;
	}

	endprotoent();
	endservent();
}

static void 
indent(int len, int need)
{
	while (len++ < need)
		putchar(' ');
}

static void
hostprint(struct host *host, struct options *opts)
{
	struct protoent *protoent = getprotobynumber(opts->netopts.proto);
	char *p_name = protoent ? protoent->p_name : "unknown";
	struct servent *serv;
	struct port *port;
	int i, n;

	for (i = 0; i < host->ports->len; i++) {
		port = host->ports->list + i;
		serv = getservbyport(htons(port->portno), p_name);

		n = printf("%s", hostname(host, nflag));
		indent(n, hwidth + 1);
		n = printf("%s", serv ? serv->s_name : "unknown");
		indent(n, swidth + 1);
		n = printf("%hu/%s", port->portno, p_name);
		indent(n, pwidth + 1);
		printf("%s\n", port->state != NULL ? port->state : "unknown");
		free(port->state);
	}
	endprotoent();
	endservent();
}

static void
output(struct options *opts)
{
	struct host *host;
	bool done = false;

	do {
		pthread_mutex_lock(&opts->scanned_mutex);
		while (!SIMPLEQ_EMPTY(opts->hosts) && SIMPLEQ_EMPTY(opts->scanned))
			pthread_cond_wait(&opts->scanned_full, &opts->scanned_mutex);

		while (!SIMPLEQ_EMPTY(opts->scanned)) {
			host = SIMPLEQ_FIRST(opts->scanned);
			hostprint(host, opts);
			SIMPLEQ_REMOVE_HEAD(opts->scanned, link);
			hostfree(host);
		}

		if (SIMPLEQ_EMPTY(opts->hosts))
			done = true;

		pthread_mutex_unlock(&opts->scanned_mutex);
	} while (!done);
}

int
main(int argc, char *argv[])
{
	struct options opts;
	int ch, ret;
	void (*scan)(struct options *) = tcp_connect_scan;
	pthread_t scan_thread;
	char *endptr;

	opts.hosts = hostlistalloc();
	opts.ports = portlistalloc();
	opts.netopts.family = AF_INET;
	opts.netopts.proto = IPPROTO_TCP;
	opts.netopts.device = NULL;
	opts.netopts.sa = NULL;
	opts.netopts.ttl = IPDEFTTL;
	opts.netopts.th_flags = 0;
	opts.delay.tv_sec = 1;
	opts.delay.tv_usec = 0;
	opts.timeout.tv_sec = 2;
	opts.timeout.tv_usec = 0;
	timerclear(&opts.last_send);

	while ((ch = getopt(argc, argv, "46a:i:I:np:s:t:w:" "CEUAPRSF")) != -1)
		switch (ch) {
		case '4':
			opts.netopts.family = AF_INET;
			break;
		case '6':
			opts.netopts.family = AF_INET6;
			break;
		case 'a':
			opts.netopts.sa = eresolve(optarg, opts.netopts.family);
			break;
		case 'i':
			parsetime(optarg, &opts.delay);
			break;
		case 'I':
			opts.netopts.device = optarg;
			break;
		case 'n':
			nflag = true;
			break;
		case 'p':
			parseports(optarg, opts.ports);
			break;
		case 's':
			switch (tolower(optarg[0])) {
				case 'c':
					scan = tcp_connect_scan;
					opts.netopts.proto = IPPROTO_TCP;
					break;
				case 't':
					scan = tcp_raw_scan;
					opts.netopts.proto = IPPROTO_TCP;
					break;
				case 'u':
					scan = udp_raw_scan;
					opts.netopts.proto = IPPROTO_UDP;
					break;
				default:
					errx(1, "unknown scan type -- \"%c\"", *optarg);
			}
			break;
		case 't':
			opts.netopts.ttl = (u_int8_t) strtoul(optarg, &endptr, 0);
			if (opts.netopts.ttl < 1 || opts.netopts.ttl > MAXTTL)
				errx(1, "ttl value is out of range: %s", optarg);
			if (endptr == optarg || *endptr != '\0')
				errx(1, "invalid TTL: %s", optarg);
			break;
		case 'w':
			parsetime(optarg, &opts.timeout);
			break;
		case 'C':
			opts.netopts.th_flags |= TH_CWR;
			break;
		case 'E':
			opts.netopts.th_flags |= TH_ECE;
			break;
		case 'U':
			opts.netopts.th_flags |= TH_URG;
			break;
		case 'A':
			opts.netopts.th_flags |= TH_ACK;
			break;
		case 'P':
			opts.netopts.th_flags |= TH_PUSH;
			break;
		case 'R':
			opts.netopts.th_flags |= TH_RST;
			break;
		case 'S':
			opts.netopts.th_flags |= TH_SYN;
			break;
		case 'F':
			opts.netopts.th_flags |= TH_FIN;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	/* there can be no more connections than timeout / delay + 1 at a time */
	opts.maxconn = (opts.timeout.tv_sec * 10e6 + opts.timeout.tv_usec) /
	    (opts.delay.tv_sec * 10e6 + opts.delay.tv_usec) + 1;

	if (opts.ports->len == 0)
		errx(1, "no ports specified");

	while (*argv)
		parsehosts(*argv++, &opts);

	countwidth(&opts);

	opts.scanned = hostlistalloc();
	pthread_mutex_init(&opts.scanned_mutex, NULL);
	pthread_cond_init(&opts.scanned_full, NULL);

	ret = pthread_create(&scan_thread, NULL, (void *(*) (void *)) scan, (void *) &opts);
	if (ret != 0) {
		errno = ret;
		err(1, "pthread_create");
	}

	output(&opts);
	pthread_join(scan_thread, NULL);

	pthread_cond_destroy(&opts.scanned_full);
	pthread_mutex_destroy(&opts.scanned_mutex);

	free(opts.hosts);

	portlistfree(opts.ports);
	free(opts.scanned);
	free(opts.netopts.sa);
	return 0;
}
