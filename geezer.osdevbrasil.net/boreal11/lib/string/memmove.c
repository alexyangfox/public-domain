#include <_size_t.h>
/*****************************************************************************
*****************************************************************************/
void *memmove(void *dst_p, const void *src_p, size_t count)
{
	const char *src = (const char *)src_p;
	char *dst = (char *)dst_p;

	if(dst_p < src_p)	/* copy up */
	{
		for(; count != 0; count--)
			*dst++ = *src++;
	}
	else			/* copy down */
	{
		dst += (count - 1);
		src += (count - 1);
		for(; count != 0; count--)
			*dst-- = *src--;
	}
	return dst_p;
}
