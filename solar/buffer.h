/*
 * Generic dynamically-allocated auto-growing in-memory buffers.
 *
 * Written by Solar Designer <solar at openwall.com> in 2006, and placed
 * in the public domain.  There's absolutely no warranty.
 */

#ifndef _BLISTS_BUFFER_H
#define _BLISTS_BUFFER_H

#include <sys/types.h>

#define BUFFER_GROW_STEP		0x8000
#define BUFFER_GROW_MAX			0x1000000

struct buffer {
	char *start, *end, *ptr;
	int error;
};

extern int buffer_init(struct buffer *buf, size_t size);
extern void buffer_free(struct buffer *buf);

extern int buffer_append(struct buffer *buf, char *what, size_t length);
extern int buffer_appendc(struct buffer *buf, char what);
extern int buffer_appendf(struct buffer *buf, char *fmt, ...)
#ifdef __GNUC__
	__attribute__ ((format (printf, 2, 3)));
#else
	;
#endif

#define buffer_appends(buf, what) \
	buffer_append((buf), (what), strlen(what))

#endif
