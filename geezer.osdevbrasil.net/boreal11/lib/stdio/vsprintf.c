#include <_printf.h> /* do_printf() */
#include <stdarg.h>
/*****************************************************************************
*****************************************************************************/
static int vsprintf_help(unsigned c, void **ptr)
{
	char *dst;

	dst = *ptr;
	*dst++ = (char)c;
	*ptr = dst;
	return 0 ;
}
/*****************************************************************************
*****************************************************************************/
int vsprintf(char *buf, const char *fmt, va_list args)
{
	int ret_val;

	ret_val = do_printf(fmt, args, vsprintf_help, (void *)buf);
	buf[ret_val] = '\0';
	return ret_val;
}
