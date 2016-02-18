#include <_size_t.h>
/*****************************************************************************
*****************************************************************************/
void *memset(void *dst_ptr, int val, size_t count)
{
	char *dst = (char *)dst_ptr;

	for(; count != 0; count--)
	{
		*dst = (char)val;
		dst++;
	}
	return dst_ptr;
}
