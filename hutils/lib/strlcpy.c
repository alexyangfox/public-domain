#include <sys/types.h>
#include <assert.h>

size_t
strlcpy(char *dst, const char *src, size_t size) {
	char *dp;

	assert(size);

	for(dp=dst;*src&&(size_t)(dp-dst)<size-1;dp++)
		*dp=*src++;
	*dp=0;
	return dp-dst;
}
