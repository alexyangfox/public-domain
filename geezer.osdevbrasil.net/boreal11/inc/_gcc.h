#ifndef __TL__GCC_H
#define	__TL__GCC_H

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(__GNUC__)
#define	__PRINTF0__	__attribute__((format(printf, 1, 2)))
#define	__PRINTF1__	__attribute__((format(printf, 2, 3)))
#else
#define	__PRINTF0__	/* nothing */
#define	__PRINTF1__	/* nothing */
#endif

#ifdef __cplusplus
}
#endif

#endif
