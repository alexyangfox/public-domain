#ifndef __TL__PRINTF_H
#define	__TL__PRINTF_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <_va_list.h>

typedef int (*fnptr_t)(unsigned c, void **helper);

int do_printf(const char *fmt, va_list args, fnptr_t fn, void *ptr);

#ifdef __cplusplus
}
#endif

#endif
