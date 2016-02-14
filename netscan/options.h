struct options {
	struct	hostshead *hosts;
	struct	portlist *ports;
	struct	network_options {
		int	family;
		int	proto;
		char	*device;	/* Interface */
		struct	sockaddr *sa;	/* Source address */
		u_int8_t
			ttl;	/* Time to Live */
		u_int8_t
			th_flags;
	} netopts;
	struct	timeval delay, timeout, last_send;
	int	maxconn;
	struct	hostshead *scanned;
	pthread_mutex_t
		scanned_mutex;
	pthread_cond_t
		scanned_full;
};
