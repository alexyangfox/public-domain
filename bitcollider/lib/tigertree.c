/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * tigertree.c - Implementation of the TigerTree algorithm
 *
 * NOTE: The TigerTree hash value cannot be calculated using a
 * constant amount of memory; rather, the memory required grows
 * with the size of input. (Roughly, one more interim value must
 * be remembered for each doubling of the input size.) The
 * default TT_CONTEXT struct size reserves enough memory for
 * input up to 2^64 in length
 *
 * Requires the tiger() function as defined in the reference
 * implementation provided by the creators of the Tiger
 * algorithm. See
 *
 *    http://www.cs.technion.ac.il/~biham/Reports/Tiger/
 *
 * $Id: tigertree.c,v 1.7 2003/02/24 10:58:36 gojomo Exp $
 *
 */

#include <string.h>
#include "tigertree.h"

#ifdef _WIN32
#undef WORDS_BIGENDIAN
#else
#include "../config.h"
#endif

#ifdef WORDS_BIGENDIAN
#   define USE_BIG_ENDIAN 1
#else
#   define USE_BIG_ENDIAN 0
#endif
void tt_endian(byte *s);

/* Initialize the tigertree context */
void tt_init(TT_CONTEXT *ctx)
{
  ctx->count = 0;
  ctx->leaf[0] = 0; // flag for leaf  calculation -- never changed
  ctx->node[0] = 1; // flag for inner node calculation -- never changed
  ctx->block = ctx->leaf + 1 ; // working area for blocks
  ctx->index = 0;   // partial block pointer/block length
  ctx->top = ctx->nodes;
}

static void tt_compose(TT_CONTEXT *ctx) {
  byte *node = ctx->top - NODESIZE;
  memmove((ctx->node)+1,node,NODESIZE); // copy to scratch area
  tiger((word64*)(ctx->node),(word64)(NODESIZE+1),(word64*)(ctx->top)); // combine two nodes
#if USE_BIG_ENDIAN
  tt_endian((byte *)ctx->top);
#endif
  memmove(node,ctx->top,TIGERSIZE);           // move up result
  ctx->top -= TIGERSIZE;                      // update top ptr
}

static void tt_block(TT_CONTEXT *ctx)
{
  word64 b;

  tiger((word64*)ctx->leaf,(word64)ctx->index+1,(word64*)ctx->top);
#if USE_BIG_ENDIAN
  tt_endian((byte *)ctx->top);
#endif
  ctx->top += TIGERSIZE;
  ++ctx->count;
  b = ctx->count;
  while(b == ((b >> 1)<<1)) { // while evenly divisible by 2...
    tt_compose(ctx);
    b = b >> 1;
  }
}

void tt_update(TT_CONTEXT *ctx, byte *buffer, word32 len)
{

  if (ctx->index)
  { /* Try to fill partial block */
	  unsigned left = BLOCKSIZE - ctx->index;
	  if (len < left)
		{
		memmove(ctx->block + ctx->index, buffer, len);
		ctx->index += len;
		return; /* Finished */
		}
	  else
		{
		memmove(ctx->block + ctx->index, buffer, left);
		ctx->index = BLOCKSIZE;
		tt_block(ctx);
		buffer += left;
		len -= left;
		}
  }

  while (len >= BLOCKSIZE)
	{
	memmove(ctx->block, buffer, BLOCKSIZE);
	ctx->index = BLOCKSIZE;
	tt_block(ctx);
	buffer += BLOCKSIZE;
	len -= BLOCKSIZE;
	}
  if ((ctx->index = len))     /* This assignment is intended */
	{
	/* Buffer leftovers */
	memmove(ctx->block, buffer, len);
	}
}

// no need to call this directly; tt_digest calls it for you
static void tt_final(TT_CONTEXT *ctx)
{
  // do last partial block, unless index is 1 (empty leaf)
  // AND we're past the first block
  if((ctx->index>0)||(ctx->top==ctx->nodes))
    tt_block(ctx);
}

void tt_digest(TT_CONTEXT *ctx, byte *s)
{
  tt_final(ctx);
  while( (ctx->top-TIGERSIZE) > ctx->nodes ) {
    tt_compose(ctx);
  }
  memmove(s,ctx->nodes,TIGERSIZE);
}

void tt_endian(byte *s)
{
  word64 *i;
  byte   *b, btemp;
  word16 *w, wtemp;

  for(w = (word16 *)s; w < ((word16 *)s) + 12; w++)
  {
      b = (byte *)w;
      btemp = *b;
      *b = *(b + 1);
      *(b + 1) = btemp;
  }

  for(i = (word64 *)s; i < ((word64 *)s) + 3; i++)
  {
      w = (word16 *)i;

      wtemp = *w;
      *w = *(w + 3);
      *(w + 3) = wtemp;

      wtemp = *(w + 1);
      *(w + 1) = *(w + 2);
      *(w + 2) = wtemp;
  }
}

// this code untested; use at own risk
void tt_copy(TT_CONTEXT *dest, TT_CONTEXT *src)
{
  int i;
  dest->count = src->count;
  for(i=0; i < BLOCKSIZE; i++)
    dest->block[i] = src->block[i];
  dest->index = src->index;
  for(i=0; i < STACKSIZE; i++)
    dest->nodes[i] = src->nodes[i];
  dest->top = src->top;
}
