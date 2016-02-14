struct relay {
	/* IN: file descriptor to read from */
	int	  source;
	/* IN: file descriptor to read to */
	int	  dest;
	/* IN+OUT: errno for failed read. -1 if EOF reached. */
	int	  readerror;
	/* IN+OUT: errno for failed write.  EPIPE if receiver is dead. */
	int	  writeerror;
	/* IN: for private use by the callback function */
	void	* userdata;
};


extern	int	relay_once(struct relay	*,
			   int,
			   struct timeval *,
			   int (*)(struct relay*, char*, size_t));
extern	int	relay_all(struct relay *,
			  int,
			  int (*)(struct relay*, char*, size_t));
