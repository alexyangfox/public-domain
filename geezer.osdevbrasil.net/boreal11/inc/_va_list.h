#ifndef __TL__VA_LIST_H
#define	__TL__VA_LIST_H

#ifdef __cplusplus
extern "C"
{
#endif

/* I was using 'unsigned char *' here, but every other
compiler/libc seems to be using 'void *', so... */
typedef void *	va_list;

#ifdef __cplusplus
}
#endif

#endif
