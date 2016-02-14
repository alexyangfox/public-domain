#include <sys/types.h>
#include <assert.h>

size_t
strlcat(char *dst, const char *src, size_t size) {
	char *dp;

	assert(size);

	for(dp=dst;*dp&&(size_t)(dp-dst)<size-1;dp++);
	for(;*src&&(size_t)(dp-dst)<size-1;dp++)
		*dp=*src++;
	*dp=0;
	return dp-dst;
}
