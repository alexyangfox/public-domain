#include <stdlib.h>
#include "../libhutils.h"

void*
sizeset(void *p, size_t size) {
	void *ret;
	if((ret=realloc(p,size)) == 0) {
		writestr(2,"libhutils: Can't alloc\n");
		exit(1);
	}
	return ret;
}
