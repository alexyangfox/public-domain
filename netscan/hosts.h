SIMPLEQ_HEAD(hostshead, host);
struct host {
	SIMPLEQ_ENTRY(host) link;
	char *hostname;
	struct sockaddr *sa;
	struct portlist *ports;
};

struct host *hostalloc(void);
void	hostfree(struct host *);
char *	hostname(struct host *, bool);
struct hostshead *
	hostlistalloc(void);
