#include <stdlib.h>
#include "../libhutils.h"

void*
newbuf(void) {
	void *ret;
	if((ret=malloc(0)) == 0) {
		writestr(2,"libhutils: WTF is wrong with malloc(0);\n");
		exit(1);
	}
	return ret;
}
