#include <string.h> /* size_t */
#include <stdint.h> /* uint16_t */
/*****************************************************************************
*****************************************************************************/
void *memsetw(void *dst_ptr, int val, size_t count)
{
	uint16_t *dst = (uint16_t *)dst_ptr;

	for(; count != 0; count--)
	{
		*dst = (uint16_t)val;
		dst++;
	}
	return dst_ptr;
}
