#include <sys/queue.h>

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "ports.h"

struct portlist *
portlistalloc(void)
{
	return emallocz(sizeof(struct portlist));
}

struct portlist *
portlistdup(struct portlist *ports)
{
	struct portlist *clone = emalloc(sizeof(*ports));
	size_t size = sizeof(*ports->list) * ports->len;

	clone->len = ports->len;
	clone->list = emalloc(size);
	memcpy(clone->list, ports->list, size); 
	return clone;
}

void
portlistfree(struct portlist *ports)
{
	free(ports->list);
	free(ports);
}

void
portlistinsert(struct portlist *ports, int start, int end)
{
	int l, r, i;

	for (l = 0; l < ports->len && ports->list[l].portno < start; l++);
	for (r = l; r < ports->len && ports->list[r].portno <= end; r++);

	ports->len += (end - start + 1) - (r - l);
	ports->list = erealloc(ports->list, sizeof(*ports->list) * ports->len);

	memmove(ports->list + (end - start + 1) + l, ports->list + r,
	    sizeof(*ports->list) * (ports->len - (end - start + 1) - l));

	for (i = start; i <= end; i++) {
		struct port *port = ports->list + l + i - start;
		port->portno = (in_port_t) i;
		port->state = 0;
	}
}

int
portcmp(const void *p1, const void *p2)
{
	const struct port *port1 = p1;
	const struct port *port2 = p2;

	return port1->portno - port2->portno;
}
