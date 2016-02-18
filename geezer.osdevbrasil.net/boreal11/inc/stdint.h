#ifndef __TL_STDINT_H
#define	__TL_STDINT_H

#ifdef __cplusplus
extern "C"
{
#endif

/* C99 7.18.1.1 - Exact-width integer types */
typedef signed char		int8_t;
typedef short			int16_t;
typedef long			int32_t;

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned long		uint32_t;

/* C99 7.18.1.4 - Integer types capable of holding object pointers */
typedef int			intptr_t;
typedef unsigned		uintptr_t;

#ifdef __cplusplus
}
#endif

#endif
