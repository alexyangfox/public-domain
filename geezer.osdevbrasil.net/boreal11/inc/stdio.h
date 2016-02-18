#ifndef __TL_STDIO_H
#define	__TL_STDIO_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <_va_list.h>
#include <_null.h>
#include <_gcc.h> /* __PRINTF0__, __PRINTF1__ */

#define	_IONBF	0	/* unbuffered stream */
#define _IOFBF  1	/* fully buffered stream */
#define _IOLBF  2	/* line and fully buffered stream */

#define	stdin	(g_std_streams + 0)
#define	stdout	(g_std_streams + 1)
//#define	stderr	(g_std_streams + 2)

#define	EOF	(-1)

/* size of default stdio.h buffers -- a nice, random
value which I hope will expose any bugs in the libc code */
#define BUFSIZ	53

#define	putchar(char)	fputc(char, stdout)

typedef struct
{
	char *buf_base, *buf_ptr;
	unsigned size, room, flags;
	int handle;
} FILE;

extern FILE g_std_streams[];

int fflush(FILE *stream);
int fputc(int c, FILE *stream);
int fputs(const char *str, FILE *stream);
int vsprintf(char *buffer, const char *fmt, va_list args);
void setbuf(FILE *stream, char *buffer);
int vprintf(const char *fmt, va_list args);

int sprintf(char *buffer, const char *fmt, ...) __PRINTF1__;
int printf(const char *fmt, ...) __PRINTF0__;

#ifdef __cplusplus
}
#endif

#endif
