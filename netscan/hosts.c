#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <netinet/ip.h>
#define __FAVOR_BSD
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "hosts.h"
#include "network.h"
#include "ports.h"

struct host *
hostalloc(void) {
	struct host *host = emallocz(sizeof(*host));

	return host;
}

void
hostfree(struct host *host) {
	free(host->hostname);
	free(host->sa);
	portlistfree(host->ports);
	free(host);
}

char *
hostname(struct host *host, bool numeric)
{
	char hbuf[NI_MAXHOST];

	if (host->hostname == NULL || numeric) {
		getnameinfo(host->sa, addrlen(host->sa), hbuf, sizeof(hbuf),
		    NULL, 0, NI_NUMERICHOST);
		free(host->hostname);
		host->hostname = estrdup(hbuf);
	}
	return host->hostname;
}

struct hostshead *
hostlistalloc(void)
{
	struct hostshead *head = emalloc(sizeof(*head));
	
	SIMPLEQ_INIT(head);
	return head;
}
