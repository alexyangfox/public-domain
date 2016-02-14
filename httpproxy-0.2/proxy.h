#include "relay.h"

int setup_proxy(struct relay	* relays,
		int		  nrelays,
		char		* proxystring,
		int		  flags,
		int (*callback)(int, int, char *, size_t)
    );
