#define TH_ECE 0x40
#define TH_CWR 0x80

/* IPv4 addresses */

in_addr_t
	netmask(int);
in_addr_t
	network(in_addr_t, in_addr_t);
in_addr_t
	broadcast(in_addr_t, in_addr_t);

/* sockets */

int	esocket(int, int, int);

/* socket addresses */

struct sockaddr *
	resolve(char *, int);
struct sockaddr *
	eresolve(char *, int);
struct sockaddr *
	getlocaladdr(int, const char *);

/* protocol independence functions */

/* return length of sockaddr structure */
socklen_t
	addrlen(struct sockaddr *);

/* set port field (common to sockaddr_in and sockaddr_in6) to portno */
void setport(struct sockaddr *, in_port_t);

/* packets */

void	ipv4build(u_int8_t, struct in_addr *, struct in_addr *, u_int8_t,
    size_t, struct ip*);
void	ipv4tcpbuild(struct in_addr *, struct in_addr *, in_port_t, in_port_t,
    tcp_seq, u_int8_t, struct tcphdr *);
void	ipv4udpbuild(struct in_addr *, struct in_addr *, in_port_t, in_port_t,
    struct udphdr *);

/* send packet */
void	sendtoraw(int, const void *, size_t, struct sockaddr *);

/* interfaces */

char *	default_device(void);
