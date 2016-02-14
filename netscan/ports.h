struct port {
	in_port_t
		portno;
	char	*state;
};

struct portlist {
	int len;
	struct port *list;
};

char *	portstate(struct port *);
struct portlist *
	portlistalloc(void);
struct portlist *
	portlistdup(struct portlist *);
void	portlistfree(struct portlist *);
void	portlistinsert(struct portlist *, int, int);
int	portcmp(const void *p1, const void *p2);
