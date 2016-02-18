#include <_printf.h> /* do_printf() */
#include <stdarg.h> /* va_list */
#include <stdio.h> /* putchar() */
/*****************************************************************************
*****************************************************************************/
static int vprintf_help(unsigned c, void **ptr_UNUSED)
{
	putchar(c);
	return 0 ;
}
/*****************************************************************************
*****************************************************************************/
int vprintf(const char *fmt, va_list args)
{
	return do_printf(fmt, args, vprintf_help, NULL);
}
