/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: tigertree.h,v 1.3 2003/02/24 10:59:29 gojomo Exp $
 */
#include "tiger.h"

/* tiger hash result size, in bytes */
#define TIGERSIZE 24

/* size of each block independently tiger-hashed, not counting leaf 0x00 prefix */
#define BLOCKSIZE 1024

/* size of input to each non-leaf hash-tree node, not counting node 0x01 prefix */
#define NODESIZE (TIGERSIZE*2)

/* default size of interim values stack, in TIGERSIZE
 * blocks. If this overflows (as it will for input
 * longer than 2^64 in size), havoc may ensue. */
#define STACKSIZE TIGERSIZE*56

typedef struct tt_context {
  word64 count;                   /* total blocks processed */
  unsigned char leaf[1+BLOCKSIZE]; /* leaf in progress */
  unsigned char *block;            /* leaf data */
  unsigned char node[1+NODESIZE]; /* node scratch space */
  int index;                      /* index into block */
  unsigned char *top;             /* top (next empty) stack slot */
  unsigned char nodes[STACKSIZE]; /* stack of interim node values */
} TT_CONTEXT;

void tt_init(TT_CONTEXT *ctx);
void tt_update(TT_CONTEXT *ctx, unsigned char *buffer, unsigned long len);
void tt_digest(TT_CONTEXT *ctx, unsigned char *hash);
void tt_copy(TT_CONTEXT *dest, TT_CONTEXT *src);
