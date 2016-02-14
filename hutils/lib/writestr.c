#include <sys/types.h>
#include <string.h>

#include "../libhutils.h"

ssize_t
writestr(int f, char *s) {
	return awrite(f,s,strlen(s));
}
