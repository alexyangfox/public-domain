/* (PD) 2004 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: kztree.h,v 1.3 2004/02/03 02:47:41 gojomo Exp $
 *
 * Structures to allow a streamed calculation of the 
 * 'KazaaTreeHash' value, without knowing input size
 * in advance. 
 * 
 */

/* MD5 hash result size, in bytes */
#define MD5SIZE 16

/* size of each block independently tiger-hashed, not counting leaf 0x00 prefix */
#define KZTREE_BLOCKSIZE (1024*32)

/* size of input to each non-leaf hash-tree node, not counting node 0x01 prefix */
#define KZTREE_NODESIZE (MD5SIZE*2)

/* default size of interim values stack, in MD5SIZE
 * blocks. If this overflows (as it will for input
 * longer than 2^128 in size), havoc may ensue. */
#define KZTREE_STACKSIZE (MD5SIZE*113)

typedef struct kztree_context {
  unsigned long count;                  /* total blocks processed */
  unsigned char leaf[KZTREE_BLOCKSIZE]; /* leaf in progress */
  unsigned char *block;                 /* leaf data */
  int index;                            /* index into block */
  unsigned char *top;                   /* top (next empty) stack slot */
  unsigned char nodes[KZTREE_STACKSIZE];/* stack of interim node values */
  unsigned long gen;                    /* number of nodes in generation of topmost node */
} KZTREE_CONTEXT;

void kztree_init(KZTREE_CONTEXT *ctx);
void kztree_update(KZTREE_CONTEXT *ctx, unsigned char *buffer, unsigned int len);
void kztree_digest(KZTREE_CONTEXT *ctx, unsigned char *hash);
void kztree_copy(KZTREE_CONTEXT *dest, KZTREE_CONTEXT *src);
static void kztree_block(KZTREE_CONTEXT *ctx);
static void kztree_compose(KZTREE_CONTEXT *ctx);
