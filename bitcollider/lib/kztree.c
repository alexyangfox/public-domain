/* (PD) 2004 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * kztree.c - Calculation of the 'KazaaTreeHash' value, which,
 * when appended to the earlier "FTHash"/"UUHash" (sig2dat)
 * value, gives the 36-byte 'kzhash' value (first used in 
 * Kazaa 2.6). 
 * 
 * 'kzhash' definition comes from example code provided and 
 * donated to the public domain by Phil Morle of Sharman 
 * Networks, 2004-01-16. 
 *
 * A compact description of the process given by that code:
 * 
 *   Break the file into N 32KB segments. If N>0, MD5 each segment
 *   separately yielding N 16-byte hash values.
 * 
 *   While N>2: Concatenate each neighbor pair of hash values
 *   together, using the empty array in lieu of a partner if
 *   necessary for the last value. MD5 each of these segments 
 *   separately, giving (N+1)/2 16-byte hash values. Set N to 
 *   this new total.
 * 
 *   N is now 0, 1, or 2. Concatenate remaining hash values together
 *   into a final segment. MD5 this segment to give the final value.
 * 
 * Notable corner cases implied by the above: 
 *   
 *   For the zero-length input, the kztree value is the same as 
 *   the MD5 value. 
 * 
 *   For input with length up to 32,768, the kztree value is the
 *   MD5 of the MD5 of the input. 
 * 
 * Notable differences from THEX/TigerTree:
 * 
 *   (1) Leaf blocks are 32KB in kzhash rather than 1KB. 
 *   (2) In kzhash, internal values with no sibling are rehashed, 
 *       rather than promoted to the parent position unchanged as
 *       in THEX. 
 *   (3) Kzhash does not use a prefix byte to differentiate 
 *       between leaf and inner-node hashes. 
 *   (3) In kzhash, a file <= a single leaf block is hashed, then 
 *       hashed again, preventing a trivial collision problem.
 *       (THEX/TigerTree uses the leaf/node prefix bytes to 
 *       prevent the collision.)
 *
 * The approach used here has been adapted to allow calculation in a 
 * stream fashion -- which helps eliminate the need for multiple
 * passes through a file when calculating several hashes at once.
 *
 * NOTE: A tree hash value cannot be calculated using a
 * constant amount of memory for any input size;  rather,  
 * the memory required grows with the size of input. 
 * (Roughly, one more interim value must be remembered for 
 * each doubling of the input size.) The default KZTREE_CONTEXT 
 * struct size reserves enough memory for input up to 2^128 
 * bytes in length, which should be plenty. 
 *
 *
 * $Id: kztree.c,v 1.3 2004/02/03 02:47:41 gojomo Exp $
 *
 */

#include <string.h>
#include "kztree.h"
#include "md5.h"

/* 
 * Initialize the kztree context 
 */
void kztree_init(KZTREE_CONTEXT *ctx)
{
  ctx->count = 0;
  ctx->block = ctx->leaf; // working area for blocks
  ctx->index = 0;   // partial block pointer/block length
  ctx->top = ctx->nodes;
}

/* 
 * Feed given bytes into the working buffer. Whenever the buffer
 * has reached KZTREE_BLOCKSIZE, call kztree_block()
 */
void kztree_update(KZTREE_CONTEXT *ctx, unsigned char *buffer, unsigned int len)
{

  if (ctx->index)
  { /* Try to fill partial block */
	  unsigned left = KZTREE_BLOCKSIZE - ctx->index;
	  if (len < left)
		{
		memmove(ctx->block + ctx->index, buffer, len);
		ctx->index += len;
		return; /* Finished */
		}
	  else
		{
		memmove(ctx->block + ctx->index, buffer, left);
		ctx->index = KZTREE_BLOCKSIZE;
		kztree_block(ctx);
		buffer += left;
		len -= left;
		}
  }

  while (len >= KZTREE_BLOCKSIZE)
	{
	memmove(ctx->block, buffer, KZTREE_BLOCKSIZE);
	ctx->index = KZTREE_BLOCKSIZE;
	kztree_block(ctx);
	buffer += KZTREE_BLOCKSIZE;
	len -= KZTREE_BLOCKSIZE;
	}
  if ((ctx->index = len))     /* This assignment is intended */
	{
	/* Buffer leftovers */
	memmove(ctx->block, buffer, len);
	}
}

/* 
 * A full KZTREE_BLOCKSIZE bytes have become available; 
 * hash those, and possibly composite together siblings.  
 */
static void kztree_block(KZTREE_CONTEXT *ctx)
{

  MD5(ctx->leaf,ctx->index,ctx->top);
  ctx->top += MD5SIZE;
  ++ctx->count;
  ctx->gen = ctx->count; 
  while(ctx->gen == ((ctx->gen >> 1)<<1)) { // while evenly divisible by 2...
    kztree_compose(ctx);
    ctx->gen = ctx->gen >> 1;
  }
}

/* 
 * Compose the top two node values, siblings in the tree
 * structure, into a single parent node value.
 */
static void kztree_compose(KZTREE_CONTEXT *ctx) {
  unsigned char *node;
  if(ctx->gen != ((ctx->gen >> 1)<<1)) { // compose of generation with odd population
    // MD5 the only child in place
    MD5(ctx->top - MD5SIZE,MD5SIZE,ctx->top - MD5SIZE);
	return;
  }
  node = ctx->top - KZTREE_NODESIZE;
  MD5(node,(KZTREE_NODESIZE),node); // combine two nodes
  ctx->top -= MD5SIZE;              // update top ptr
}

/* 
 * 
 * no need to call this directly; kztree_digest calls it for you
 */
static void kztree_final(KZTREE_CONTEXT *ctx)
{
  // do last partial block, if any
  if(ctx->index > 0)
    kztree_block(ctx);
}

/*
 * Finish the kztree calc and return the final digest value
 */
void kztree_digest(KZTREE_CONTEXT *ctx, unsigned char *digest)
{
  kztree_final(ctx);
  while( ctx->gen > 1) {
    kztree_compose(ctx);
	ctx->gen = (ctx->gen + 1) / 2;
  }
  if( ctx->count == 1) {
	// for the single block case, hash again
	kztree_compose(ctx);
  }
  if( ctx->count == 0) {
	// for the zero-length input case, hash nothing.
	kztree_block(ctx);
  } 

  memmove(digest,ctx->nodes,MD5SIZE);
}


/*
 * Copy the context
 *
 * this code untested; use at own risk
 */ 
void kztree_copy(KZTREE_CONTEXT *dest, KZTREE_CONTEXT *src)
{
  int i;
  dest->count = src->count;
  for(i=0; i < KZTREE_BLOCKSIZE; i++)
    dest->block[i] = src->block[i];
  dest->index = src->index;
  for(i=0; i < KZTREE_STACKSIZE; i++)
    dest->nodes[i] = src->nodes[i];
  dest->top = src->top;
}
