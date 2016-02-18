#ifndef __TL_STRING_H
#define	__TL_STRING_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <_size_t.h>
#include <_null.h>

int memcmp(const void *left_p, const void *right_p, size_t count);
void *memcpy(void *dst_ptr, const void *src_ptr, size_t count);
void *memsetw(void *dst, int val, size_t count);
void *memset(void *dst, int val, size_t count);

size_t strlen(const char *str);
int strcmp(const char *s1, const char*s2);
int strncmp(const char *s1, const char*s2, unsigned max_len);

#ifdef __cplusplus
}
#endif

#endif

