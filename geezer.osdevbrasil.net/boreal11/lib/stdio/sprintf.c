#include <stdarg.h>
#include <stdio.h> /* vsprintf() */
/*****************************************************************************
*****************************************************************************/
int sprintf(char *buf, const char *fmt, ...)
{
	va_list args;
	int ret_val;

	va_start(args, fmt);
	ret_val = vsprintf(buf, fmt, args);
	va_end(args);
	return ret_val;
}
