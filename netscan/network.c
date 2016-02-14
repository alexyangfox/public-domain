#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#define __FAVOR_BSD
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <pcap.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "network.h"

/* IPv4 addresses */
in_addr_t
netmask(int prefix) {
	return prefix == 0 ? 0 : ~0 << (32 - prefix);
}

in_addr_t
network(in_addr_t addr, in_addr_t mask) {
	return addr & mask;
}

in_addr_t
broadcast(in_addr_t addr, in_addr_t mask) {
	return addr | ~mask;
}

int
esocket(int domain, int type, int protocol)
{
	int sockfd = socket(domain, type, protocol);

	if (sockfd == -1)
		err(1, "socket");
	return sockfd;
}

static int
_resolve(char *hostname, int family, struct sockaddr **addr)
{
	struct addrinfo hints, *res = NULL;
	int error;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;

	error = getaddrinfo(hostname, NULL, &hints, &res);
	if (error == 0) {
		socklen_t salen = res->ai_addrlen;

		*addr = emalloc(salen);
		memcpy(*addr, res->ai_addr, salen);
		freeaddrinfo(res);
	} else
		*addr = NULL;
	return error;
}

struct sockaddr *
resolve(char *hostname, int family)
{
	struct sockaddr *addr;

	_resolve(hostname, family, &addr);
	return addr;
}

struct sockaddr *
eresolve(char *hostname, int family)
{
	struct sockaddr *addr;
	int error = _resolve(hostname, family, &addr);

	if (error != 0)
		errx(1, "%s: %s", gai_strerror(error), hostname);
	return addr;
}

struct sockaddr *
getlocaladdr(int family, const char *device)
{
	struct ifaddrs *ifap, *ifa;
	struct sockaddr *sa = NULL;

	if (getifaddrs(&ifap) == -1)
		err(1, "getifaddrs");

	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		if (strcmp(ifa->ifa_name, device) != 0)
			continue;
		if (ifa->ifa_addr && ifa->ifa_addr->sa_family == family) {
			socklen_t salen = addrlen(ifa->ifa_addr);

			sa = emalloc(salen);
			memcpy(sa, ifa->ifa_addr, salen);
			break;
		}
	}

	freeifaddrs(ifap);
	return sa;
}

socklen_t
addrlen(struct sockaddr * sa)
{
	switch (sa->sa_family) {
	case AF_INET:
		return sizeof(struct sockaddr_in);
	case AF_INET6:
		return sizeof(struct sockaddr_in6);
	default:
		return 0;
	}
}

void
setport(struct sockaddr *sa, in_port_t portno)
{
	switch (sa->sa_family) {
	case AF_INET:
		((struct sockaddr_in *) sa)->sin_port = htons(portno);
		break;
	case AF_INET6:
		((struct sockaddr_in6 *) sa)->sin6_port = htons(portno);
		break;
	}
}

static u_int16_t
checksum(void *p, size_t size)
{
	u_int16_t *buf = (u_int16_t *)p;
	u_int32_t sum = 0;
	int nwords = size / sizeof(*buf);

	while (nwords-- > 0)
		sum += *buf++;

	if ((size & 1) == 1)
		sum += *(u_char *)buf;

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return (u_int16_t) ~sum;
}

void
ipv4build(u_int8_t ip_p, struct in_addr *s_addr, struct in_addr *d_addr,
    u_int8_t ttl, size_t len, struct ip *iph)
{
	iph->ip_v   = 4;		/* Version */
	iph->ip_hl  = 5;		/* Internet Header Length */
	iph->ip_tos = 0;		/* Type of Service */
	iph->ip_len = htons(len);	/* Total Length */
	iph->ip_id  = htons(54321);	/* Identification */
	iph->ip_off = 0;		/* Flags, Fragment Offset */
	iph->ip_ttl = ttl;		/* Time to Live */
	iph->ip_p   = ip_p;		/* Protocol */
	iph->ip_sum = 0;		/* Header Checksum */
	iph->ip_src = *s_addr;		/* Source Address */
	iph->ip_dst = *d_addr;		/* Destination Address */
	iph->ip_sum = checksum(iph, sizeof(*iph));
}

static u_int16_t
ipv4pseudosum(struct in_addr *s_addr, struct in_addr *d_addr, u_int8_t proto,
    u_int16_t len, u_char *hdr)
{
	struct pseudohdr {
		struct	in_addr s_addr;
		struct	in_addr d_addr;
		u_int8_t	zero;
		u_int8_t	proto;
		u_int16_t	len;
	} *pseudoh = (struct pseudohdr *)(hdr - sizeof(struct pseudohdr));

	pseudoh->s_addr = *s_addr;
	pseudoh->d_addr = *d_addr;
	pseudoh->zero = 0;
	pseudoh->proto = proto;
	pseudoh->len = htons(len);
	return checksum(pseudoh, sizeof(*pseudoh) + len);
}

/* must be called before building ipv4 header */
void
ipv4tcpbuild(struct in_addr *s_addr, struct in_addr *d_addr, in_port_t s_port,
    in_port_t d_port, tcp_seq seq, u_int8_t th_flags, struct tcphdr *tcph)
{
	tcph->th_sport = htons(s_port);	/* Source Port */
	tcph->th_dport = htons(d_port);	/* Destination Port */
	tcph->th_seq = htons(seq);	/* Sequence Number */
	tcph->th_ack = 0;		/* Acknowledgment Number */
	tcph->th_off = 5;		/* Data Offset */
	tcph->th_x2  = 0;		/* Reserved */
	tcph->th_flags = th_flags;	/* Control Bits */
	tcph->th_win = htons(2048);	/* Window */
	tcph->th_sum = 0;		/* Checksum */
	tcph->th_urp = 0;		/* Urgent Pointer */
	tcph->th_sum = ipv4pseudosum(s_addr, d_addr, IPPROTO_TCP, sizeof(*tcph),
	    (u_char *)tcph);
}

void
ipv4udpbuild(struct in_addr *s_addr, struct in_addr *d_addr, in_port_t s_port,
    in_port_t d_port, struct udphdr *udph)
{
	udph->uh_sport = htons(s_port);
	udph->uh_dport = htons(d_port);
	udph->uh_ulen = htons(sizeof(*udph));
	udph->uh_sum = 0;
	udph->uh_sum = ipv4pseudosum(s_addr, d_addr, IPPROTO_UDP, sizeof(*udph),
	    (u_char *)udph);
	if (udph->uh_sum == 0)
		udph->uh_sum = ~udph->uh_sum;
}

void
sendtoraw(int sockfd, const void *data, size_t len, struct sockaddr *sa)
{
	const int hdrincl = 1;

	if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &hdrincl,
	    sizeof(hdrincl)) < 0)
		err(1, "setsockopt");
	if (sendto(sockfd, data, len, 0, sa, addrlen(sa)) == -1)
		err(1, "sendto");
}

char *
default_device(void)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	char *device = pcap_lookupdev(errbuf);

	if (device == NULL)
		errx(1, "%s", errbuf);
	return device;
}
