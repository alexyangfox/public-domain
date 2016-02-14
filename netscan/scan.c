#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/if_ether.h>
#define __FAVOR_BSD
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <err.h>
#include <errno.h>
#include <pcap.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "ports.h"
#include "hosts.h"
#include "options.h"
#include "scan.h"
#include "network.h"

SLIST_HEAD(connhead, conn);
struct conn {
	int	sockfd;
	struct	host *host;
	struct	port *port;
	struct	timeval timeout;
	SLIST_ENTRY(conn)
		link;
};

/* memory pool */

struct connpool {
	struct conn *conns, **avail;
	int top;
};

void
connpoolinit(struct connpool *pool, size_t nelem) {
	unsigned int i;

	pool->conns = emalloc(sizeof(*pool->conns) * nelem);
	pool->avail = emalloc(sizeof(*pool->avail) * nelem);
	pool->top = nelem - 1;

	for (i = 0; i < nelem; i++)
		pool->avail[i] = &pool->conns[i];
}

void
connpooldestroy(struct connpool *pool) {
	free(pool->conns);
	free(pool->avail);
}

struct conn *
connpoolget(struct connpool *pool) {
	if (pool->top < 0)
		return NULL;
	return pool->avail[pool->top--]; /* pop */
}

void
connpoolput(struct connpool *pool, struct conn *conn)
{
	pool->avail[++pool->top] = conn; /* push */
}

/* timing */

bool
mustsend(struct options *opts)
{
	struct timeval diff, now;

	gettimeofday(&now, NULL);
	timersub(&now, &opts->last_send, &diff);
	return timercmp(&diff, &opts->delay, >);
}

struct timeval *
till_next_send(struct options *opts, struct timeval *diff)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	/* diff = delay + last_send - now */
	timeradd(&opts->delay, &opts->last_send, diff);
	if (timercmp(diff, &now, >))
		timersub(diff, &now, diff);
	else
		timerclear(diff);
	return diff;
}

void
setconntimeout(struct conn *conn, struct options *opts)
{
	timeradd(&opts->last_send, &opts->timeout, &conn->timeout);
}

bool
timedout(struct conn *conn)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	return timercmp(&now, &conn->timeout, >);
}

/* connect scan */

/*
 *  if can make async connection returns true
 *  if can't make async connection (target is local or unknown error) returns false
 */
static bool
conninit(struct conn *conn, struct options *opts)
{
	struct host *host = conn->host;
	struct port *port = conn->port;
	int sockfd = esocket(host->sa->sa_family, SOCK_STREAM, IPPROTO_TCP);
	in_port_t portno = port->portno;
	int res;

	unblock(sockfd);
	setport(host->sa, portno);

	res = connect(sockfd, host->sa, addrlen(host->sa));
	gettimeofday(&opts->last_send, NULL);

	switch (res) {
	case -1:
		switch (errno) {
		case EINPROGRESS:
			conn->sockfd = sockfd;
			setconntimeout(conn, opts);
			return true;
		case ECONNREFUSED:
			port->state = estrdup("closed");
			break;
		case ENETUNREACH:
			break;
		default:
			warn("connect");
		}
		break;
	case 0:
		port->state = estrdup("open");
		eclose(sockfd);
	}
	return false;
}

static void
remove_done(struct connhead *head, struct connpool *pool,
    struct timeval *timeout)
{
	int res, maxfd = 0;
	fd_set readfds, writefds;
	struct conn *curr, **prev;
	struct port *port;

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	SLIST_FOREACH(curr, head, link) {
		int sockfd = curr->sockfd;

		/* maybe there is no need to block before select */
		block(sockfd);
		if (sockfd > maxfd)
			maxfd = sockfd;
		FD_SET(sockfd, &readfds);
		FD_SET(sockfd, &writefds);
	}

	res = select(maxfd + 1, &readfds, &writefds, NULL, timeout);
	if (res == -1)
		err(1, "select");

	prev = &SLIST_FIRST(head);
	for (curr = *prev; curr != NULL; curr = SLIST_NEXT(curr, link)) {
		bool connected =
		    FD_ISSET(curr->sockfd, &readfds) ||
		    FD_ISSET(curr->sockfd, &writefds);
		bool timed_out = false;

		if (connected) {
			int error;
			socklen_t len = sizeof(error);

			res = getsockopt(curr->sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
			if (res == -1)
				err(1, "getsockopt");

			port = curr->port;
			switch (error) {
			case 0:
				port->state = estrdup("open");
				break;
			case ECONNREFUSED:
				port->state = estrdup("closed");
				break;
			default:
				errno = error;
				err(1, "connect");
			}
			eclose(curr->sockfd);
		} else
			timed_out = timedout(curr);

		if (connected || timed_out) {
			*prev = SLIST_NEXT(curr, link);
			connpoolput(pool, curr);
		} else
			prev = &SLIST_NEXT(curr, link);
	}
}

static void
remove_scanned(struct options *opts, struct host *target, struct connhead *head)
{
	struct host *host;
	struct conn *conn;

	for (;;) {
		host = SIMPLEQ_FIRST(opts->hosts);
		if (host == target)
			return;
		SLIST_FOREACH(conn, head, link)
			if (conn->host == host)
				return;
		pthread_mutex_lock(&opts->scanned_mutex);
		SIMPLEQ_REMOVE_HEAD(opts->hosts, link);
		SIMPLEQ_INSERT_TAIL(opts->scanned, host, link);
		pthread_cond_signal(&opts->scanned_full);
		pthread_mutex_unlock(&opts->scanned_mutex);
	}
}

void
tcp_connect_scan(struct options *opts)
{
	struct host *host;
	struct port *port;
	struct connpool pool;
	struct connhead head = SLIST_HEAD_INITIALIZER(head);

	struct conn *conn;
	struct timeval timeout;

	connpoolinit(&pool, opts->maxconn);

	host = SIMPLEQ_FIRST(opts->hosts);
	port = host->ports->list;
	while (host != NULL || !SLIST_EMPTY(&head)) {
		if (mustsend(opts) && host != NULL &&
		    (conn = connpoolget(&pool))) {
			conn->host = host;
			conn->port = port;

			if (conninit(conn, opts))
				SLIST_INSERT_HEAD(&head, conn, link);
			else
				connpoolput(&pool, conn);

			if (++port == host->ports->list + host->ports->len) {
				host = SIMPLEQ_NEXT(host, link);
				if (host != NULL)
					port = host->ports->list;
			}
		}

		remove_done(&head, &pool, till_next_send(opts, &timeout));
		remove_scanned(opts, host, &head);
	}

	connpooldestroy(&pool);
}

/* raw TCP scan */

static void
setupfilter(pcap_t *p, struct network_options *opts)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	bpf_u_int32 net, mask;
	struct bpf_program f;
	char dsthbuf[NI_MAXHOST];
	char *format = "dst host %s";
	char *str = emalloc(strlen(format) - 2 + NI_MAXHOST + 1);

	if (pcap_lookupnet(opts->device, &net, &mask, errbuf) == -1)
		errx(1, "%s", errbuf);

	getnameinfo(opts->sa, addrlen(opts->sa), dsthbuf,
	    sizeof(dsthbuf), NULL, 0, NI_NUMERICHOST);
	sprintf(str, format, dsthbuf);

	if (pcap_compile(p, &f, str, 0, net) == -1)
		errx(1, "%s", pcap_geterr(p));
	free(str);
	if (pcap_setfilter(p, &f) == -1)
		errx(1, "%s", pcap_geterr(p));
	pcap_freecode(&f);
}

static void
sendprobe(struct conn *conn, struct options *opts)
{
	struct host *host = conn->host;
	struct port *port = conn->port;
	struct sockaddr_in *sin;
	struct in_addr *s_addr, *d_addr;
	u_char *packet, *hdr;
	size_t len;
	struct ip *iph;

	switch (host->sa->sa_family) {
	case AF_INET:
		sin = (struct sockaddr_in *)opts->netopts.sa;
		s_addr = &sin->sin_addr;
		sin = (struct sockaddr_in *)host->sa;
		d_addr = &sin->sin_addr;
		len = sizeof(struct ip) + (opts->netopts.proto == IPPROTO_TCP ?
		    sizeof(struct tcphdr) : sizeof(struct udphdr));
		packet = emallocz(len);
		iph = (struct ip *)packet;
		hdr = packet + sizeof(struct ip);

		switch (opts->netopts.proto) {
			case IPPROTO_TCP:
				ipv4tcpbuild(s_addr, d_addr, 12345, port->portno,
				    rand(), opts->netopts.th_flags,
				    (struct tcphdr *)hdr);
				break;
			case IPPROTO_UDP:
				ipv4udpbuild(s_addr, d_addr, 12345, port->portno,
				    (struct udphdr *)hdr);
		}
		ipv4build(opts->netopts.proto, s_addr, d_addr, opts->netopts.ttl, len,
		    iph);
		break;
	default:
		errx(1, "not implemented yet");
	}
	sendtoraw(conn->sockfd, packet, len, host->sa);
	gettimeofday(&opts->last_send, NULL);
	setconntimeout(conn, opts);
	free(packet);
}

static void
remove_timedout(struct connhead *head, struct connpool *pool)
{
	struct conn *curr, **prev;

	prev = &SLIST_FIRST(head);
	for (curr = *prev; curr != NULL; curr = SLIST_NEXT(curr, link)) {
		if (timedout(curr)) {
			*prev = SLIST_NEXT(curr, link);
			connpoolput(pool, curr);
		} else
			prev = &SLIST_NEXT(curr, link);
	}
}


typedef const in_port_t *(*getportfn)(const u_char *);

struct port *
connsearch(struct connhead *head, const u_char *packet, struct connpool *pool,
    struct network_options *netopts, getportfn getport) {
	struct sockaddr_in *sin;
	struct conn *curr, **prev;
	struct ip *iph = (struct ip *) (packet + sizeof(struct ether_header));
	const u_char *hdr = packet + sizeof(struct ether_header) + iph->ip_hl * 4;
	const in_port_t *portno;

	int ret;
	struct port key, *port;
	struct portlist *ports;

	portno = getport(hdr);
	if (portno == NULL)
		return NULL;
	key.portno = ntohs(*portno);

	prev = &SLIST_FIRST(head);
	for (curr = *prev; curr != NULL; curr = SLIST_NEXT(curr, link)) {
		sin = (struct sockaddr_in *) curr->host->sa;
		ret = memcmp(&sin->sin_addr, &iph->ip_src, sizeof(struct in_addr));
		if (ret == 0) {
			ports = curr->host->ports;
			port = bsearch(&key, ports->list, ports->len,
			    sizeof(key), portcmp);
			if (port != NULL) {
				*prev = SLIST_NEXT(curr, link);
				connpoolput(pool, curr);
				return port;
			}
		}
		prev = &SLIST_NEXT(curr, link);
	}
	return NULL;
}

static void
raw_scan(struct options *opts, char *(recieve)(const u_char *packet), getportfn getport)
{
	/* pcap */
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *p;
	int pcapfd;

	/* packet */
	struct pcap_pkthdr *hp;
	const u_char *pktp;

	int ret, sockfd;
	fd_set readfds, writefds;
	struct timeval diff;

	struct conn *conn;
	struct connpool pool;
	struct connhead head = SLIST_HEAD_INITIALIZER(head);
	struct host *host;
	struct port *port, *rcvport;

	/* determine network interface and it's address */
	if (opts->netopts.device == NULL)
		opts->netopts.device = default_device();
	if (opts->netopts.sa == NULL) {
		opts->netopts.sa = getlocaladdr(opts->netopts.family, opts->netopts.device);
		if (opts->netopts.sa == NULL)
			errx(1, "can't get local address");
	}

	/* initialize pcap */
	p = pcap_open_live(opts->netopts.device, sizeof(struct ether_header) +
	    15 * 4 + sizeof(struct tcphdr), 0, 1000, errbuf);
	if (p == NULL)
		errx(1, "%s", errbuf);

	pcapfd = pcap_get_selectable_fd(p);
	if (pcapfd == -1)
		errx(1, "pcap_get_selectable_fd");

	setupfilter(p, &opts->netopts);

	connpoolinit(&pool, opts->maxconn);

	/* scan */
	srand(time(NULL));
	host = SIMPLEQ_FIRST(opts->hosts);
	sockfd = esocket(host->sa->sa_family, SOCK_RAW, IPPROTO_RAW);
	port = host->ports->list;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	while (host != NULL || !SLIST_EMPTY(&head)) {
		/* check file descriptors and block for timeout */
		FD_SET(pcapfd, &readfds);
		if (host != NULL && mustsend(opts)) {
			FD_SET(sockfd, &writefds);
			select(max(pcapfd, sockfd) + 1, &readfds,
			    &writefds, NULL, NULL);
		} else {
			till_next_send(opts, &diff);
			select(pcapfd + 1, &readfds, NULL, NULL, &diff);
		}

		/* send probes */
		if (FD_ISSET(sockfd, &writefds) && (conn = connpoolget(&pool))) {
			conn->sockfd = sockfd;
			conn->host = host;
			conn->port = port;
			port->state = NULL;
			sendprobe(conn, opts);
			SLIST_INSERT_HEAD(&head, conn, link);

			if (++port == host->ports->list + host->ports->len) {
				eclose(sockfd);

 				host = SIMPLEQ_NEXT(host, link);
				if (host != NULL) {
					port = host->ports->list;
					sockfd = esocket(host->sa->sa_family, SOCK_RAW, IPPROTO_RAW);
				}
			}
			FD_CLR(sockfd, &writefds);
		}

		/* recieve results */
		if (FD_ISSET(pcapfd, &readfds)) {
			ret = pcap_next_ex(p, &hp, &pktp);
			switch (ret) {
			case 1:
				rcvport = connsearch(&head, pktp, &pool, &opts->netopts, getport);
				if (rcvport == NULL)
					break;
				if (rcvport->state != NULL)
					free(rcvport->state);
				rcvport->state = recieve(pktp + sizeof(struct ether_header));
				break;
			case -1:
				errx(1, "%s", pcap_geterr(p));
			}
		}

		remove_timedout(&head, &pool);
		remove_scanned(opts, host, &head);
	}
	pcap_close(p);

	connpooldestroy(&pool);
}

static char *
flagstoa(u_int8_t flags)
{
	char *s, *p;
	u_int8_t t = flags;
	size_t size;

	if (flags == 0)
		return estrdup(".");
	else {
		/* count bits set */
		for (size = 0; t; size++)
			t &= t - 1;
		p = s = emalloc(size + 1);
		if (flags & TH_FIN) *p++ = 'F';
		if (flags & TH_SYN) *p++ = 'S';
		if (flags & TH_RST) *p++ = 'R';
		if (flags & TH_PUSH) *p++ = 'P';
		if (flags & TH_ACK) *p++ = 'A';
		if (flags & TH_URG) *p++ = 'U';
		if (flags & TH_ECE) *p++ = 'E';
		if (flags & TH_CWR) *p++ = 'W';
		*p = '\0';
		return s;
	}
}

static char *
recievetcp(const u_char *packet)
{
	struct ip *iph = (struct ip *) packet;
	struct tcphdr *tcph = (struct tcphdr *) (packet + iph->ip_hl * 4);

	return flagstoa(tcph->th_flags);
}

const in_port_t *
tcpport(const u_char *hdr)
{
	return &(((struct tcphdr *)hdr)->th_sport);	
}

void
tcp_raw_scan(struct options *opts)
{
	raw_scan(opts, recievetcp, tcpport);
}

static char *
recieveudp(const u_char *packet)
{
	return estrdup("closed");
}

const in_port_t *
udpport(const u_char *hdr)
{
	const struct icmphdr *icmph = (struct icmphdr *)hdr;
	const struct ip *iph = (struct ip *)(hdr + sizeof(struct icmphdr));
	const struct udphdr *udph;

	if (icmph->type != ICMP_DEST_UNREACH ||
	    icmph->code != ICMP_PORT_UNREACH)
		return NULL;
	udph = (struct udphdr *)(hdr + sizeof(struct icmphdr) + iph->ip_hl * 4);
	return &(udph->uh_dport);
}

void
udp_raw_scan(struct options *opts)
{
	raw_scan(opts, recieveudp, udpport);
}
