#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#define __FAVOR_BSD
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "hosts.h"
#include "network.h"
#include "options.h"
#include "parse.h"
#include "ports.h"

/* ranges */

struct range {
	int start, end;
};

static char *
parserange(struct range *r, char *s, int min, int max)
{
	int n = 0;
	bool isrange = false, set = false;

	while (*s) {
		if (isdigit(*s)) {
			n *= 10;
			n += *s - '0';
			set = true;
		} else if (*s == '-') {
			if (isrange)
				return s;
			r->start = set ? n : min;
			n = 0;
			set = false;
			isrange = true;
		} else
			return s;
		s++;
	}
	if (!isrange)
		r->start = set ? n : min;
	r->end = set ? n : max;

	if (r->start < min)
		r->start = min;
	if (r->end > max)
		r->end = max;
	return s;
}

/* hosts */

static void
parsecidr(char *hostname, char *prefixstr, struct options *opts)
{
	struct host *host;

	/* parse host */
	struct sockaddr *sa = eresolve(hostname, AF_INET);
	struct sockaddr_in *sin = (struct sockaddr_in *) sa;
	in_addr_t addr = ntohl(sin->sin_addr.s_addr);
	in_addr_t mask, net, bc;

	/* parse mask */
	char *endptr;
	int prefix;

	prefix = strtol(prefixstr, &endptr, 0);
	if (prefix < 0 || prefix > 32)
		errx(1, "prefix size is out of range: %s", prefixstr);
	if (endptr == prefixstr || *endptr != '\0')
		errx(1, "invalid prefix size: %s", prefixstr);

	mask = netmask(prefix);
	net = network(addr, mask);
	bc = broadcast(addr, mask);

	for (addr = net; addr <= bc; addr++) {
		host = hostalloc();
		sin->sin_addr.s_addr = htonl(addr);
		host->sa = emalloc(addrlen(sa));
		memcpy(host->sa, sa, addrlen(sa));
		host->ports = portlistdup(opts->ports);
		SIMPLEQ_INSERT_TAIL(opts->hosts, host, link);
	}
}

static void
parsehost(char *s, struct options *opts, int family)
{
	/* hostname or IPv6 address */
	struct host *host = hostalloc();

	host->hostname = isalpha(*s) ? estrdup(s) : NULL;
	host->sa = eresolve(s, family);
	host->ports = portlistdup(opts->ports);

	SIMPLEQ_INSERT_TAIL(opts->hosts, host, link);
}

void
parsehosts(char *s, struct options *opts)
{
	char *slash = strchr(s, '/');

	if (slash != NULL) {
		/* CIDR */
		if (opts->netopts.family == AF_INET) {
			*slash = '\0';
			parsecidr(s, slash + 1, opts);
		} else
			errx(1, "CIDR notation supported only for IPv4");
	} else
		parsehost(s, opts, opts->netopts.family);
}

/* ports */

void
parseports(char *s, struct portlist *ports)
{
	char *p, *last;
	struct range r;

	for ((p = strtok_r(s, ",", &last)); p;
	    (p = strtok_r(NULL, ",", &last))) {
		p = parserange(&r, p, 0, 65535);
		if (*p != '\0')
			errx(1, "can't parse ports: %c", *p);
		portlistinsert(ports, r.start, r.end);
	}
}

/* time */

void
parsetime(char *s, struct timeval *tv)
{
	char *endptr;

	errno = 0;
	tv->tv_usec = strtol(s, &endptr, 0);
	if (errno != 0)
		err(1, "strtol");
	if (endptr == s || *endptr != '\0')
		errx(1, "can't parse");
	if (tv->tv_usec < 0)
		errx(1, "time can't be negative: %s", s);
	tv->tv_sec = tv->tv_usec / 1000000;
	tv->tv_usec = tv->tv_usec % 1000000;
}
