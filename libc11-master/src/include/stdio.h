/* This is free and unencumbered software released into the public domain. */

#ifndef _STDIO_H
#define _STDIO_H

/**
 * @file
 *
 * <stdio.h> - Input/output.
 *
 * @see http://libc11.org/stdio/
 */

#include <stdarg.h> /* for va_list */
#include <stddef.h> /* for size_t */

#ifdef __cplusplus
extern "C" {
#endif

int snprintf(char* restrict str, size_t size, const char* restrict format, ...);

int sprintf(char* restrict str, const char* restrict format, ...);

int vsnprintf(char* restrict str, size_t size, const char* restrict format, va_list ap);

int vsprintf(char* restrict str, const char* restrict format, va_list ap);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _STDIO_H */
