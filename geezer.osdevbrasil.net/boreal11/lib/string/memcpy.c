#include <string.h> /* size_t */
/*****************************************************************************
Compile this and look at the asm code. It's just too slow!
*****************************************************************************/
void *memcpy(void *dst_ptr, const void *src_ptr, size_t count)
{
	const char *src = (const char *)src_ptr;
	char *dst = (char *)dst_ptr;
	void *ret_val = dst_ptr;

	for(; count != 0; count--)
	{
		*dst = *src;
		dst++;
		src++;
	}
	return ret_val;
}
