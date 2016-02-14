/*
 */

#include <netinet/ip.h>

#include "ip_misc.h"


#define Export


Export	char *
source_route(int	   type,
	     char	** hostlist,
	     int	   nhosts;
	     int	 * result_len)
{
    char	* option;


    if (type != IPOPT_LSRR  &&  type != IPOPT_SSRR) {
	errno = EINVAL;
	return NULL;
    }

    optlen = nhosts * 4 + IPOPT_MINOFF;
    option = malloc(optlen);
    if (option == NULL)
	return NULL;
    option[0] = type;
    option[1] = optlen;
    option[2] = IPOPT_MINOFF;
    h = &option[IPOPT_MINOFF - 1];
    
