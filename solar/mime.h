/*
 * MIME message parsing.
 *
 * Written by Solar Designer <solar at openwall.com> in 2006, and placed
 * in the public domain.  There's absolutely no warranty.
 */

#ifndef _BLISTS_MIME_H
#define _BLISTS_MIME_H

#include "buffer.h"

#define MIME_DEPTH_MAX			10

struct mime_entity {
	struct mime_entity *next;
	char *type, *boundary, *encoding;
};

struct mime_ctx {
	struct buffer *src, dst;
	struct mime_entity *entities;
	int depth;
};

extern int mime_init(struct mime_ctx *ctx, struct buffer *src);
extern void mime_free(struct mime_ctx *ctx);

extern char *mime_skip_header(struct mime_ctx *ctx);
extern char *mime_decode_header(struct mime_ctx *ctx);

extern char *mime_next_body_part(struct mime_ctx *ctx);
extern char *mime_next_body(struct mime_ctx *ctx);
extern char *mime_skip_body(struct mime_ctx *ctx);
extern char *mime_decode_body(struct mime_ctx *ctx);

#endif
