/* Support for calculating EDonkey2000 hashes.
 *
 * The functions which begin "ED2K" are created and placed in
 * the public domain by The Bitzi Corporation.
 *
 * The code implementing MD4 is (C) RSA; see the notice below
 * for details.
 *
 *  $Id: ed2k_md4.c,v 1.1 2002/07/06 09:30:15 gojomo Exp $
 */

#include <string.h>		/* for memcpy() */
#include "ed2k_md4.h"

unsigned int EDSEG_SIZE = 1024*9500; // 9,728,000

/* FOR THE MD4 CODE:

   Copyright (C) 1990-2, RSA Data Security, Inc. All rights reserved.

   License to copy and use this software is granted provided that it
   is identified as the "RSA Data Security, Inc. MD4 Message-Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   License is also granted to make and use derivative works provided
   that such works are identified as "derived from the RSA Data
   Security, Inc. MD4 Message-Digest Algorithm" in all material
   mentioning or referencing the derived work.

   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.

   These notices must be retained in any copies of any part of this
   documentation and/or software.
 */

// Constants for MD4Transform routine.
#define S11 3
#define S12 7
#define S13 11
#define S14 19
#define S21 3
#define S22 5
#define S23 9
#define S24 13
#define S31 3
#define S32 9
#define S33 11
#define S34 15

static void MD4Transform(w32 [4], const unsigned char [64]);
static void Encode(unsigned char *, w32 *, unsigned int);
static void Decode(w32 *, const unsigned char *, unsigned int);

static unsigned char PADDING[64] = {
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* F, G and H are basic MD4 functions.
 */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))

/* ROTATE_LEFT rotates x left n bits.
 */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG and HH are transformations for rounds 1, 2 and 3 */
/* Rotation is separate from addition to prevent recomputation */
#define FF(a, b, c, d, x, s) { \
    (a) += F ((b), (c), (d)) + (x); \
    (a) = ROTATE_LEFT ((a), (s)); \
  }
#define GG(a, b, c, d, x, s) { \
    (a) += G ((b), (c), (d)) + (x) + (w32)0x5a827999; \
    (a) = ROTATE_LEFT ((a), (s)); \
  }
#define HH(a, b, c, d, x, s) { \
    (a) += H ((b), (c), (d)) + (x) + (w32)0x6ed9eba1; \
    (a) = ROTATE_LEFT ((a), (s)); \
  }

/* MD4 initialization. Begins an MD4 operation, writing a new context.
 */
void MD4Init(MD4_CTX *context)                                        /* context */
{
  context->count[0] = context->count[1] = 0;

  /* Load magic initialization constants.
   */
  context->state[0] = 0x67452301;
  context->state[1] = 0xefcdab89;
  context->state[2] = 0x98badcfe;
  context->state[3] = 0x10325476;
}

/* MD4 block update operation. Continues an MD4 message-digest
     operation, processing another message block, and updating the
     context.
 */
void MD4Update(MD4_CTX *context,                                        /* context */
const unsigned char *input,                                /* input block */
unsigned int inputLen)                     /* length of input block */
{
  unsigned int i, index, partLen;

  /* Compute number of bytes mod 64 */
  index = (unsigned int)((context->count[0] >> 3) & 0x3F);
  /* Update number of bits */
  if ((context->count[0] += ((w32)inputLen << 3))
      < ((w32)inputLen << 3))
    context->count[1]++;
  context->count[1] += ((w32)inputLen >> 29);

  partLen = 64 - index;

  /* Transform as many times as possible.
   */
  if (inputLen >= partLen) {
    //i4_memcpy(&context->buffer[index], input, partLen);
    memcpy(&context->buffer[index], input, partLen);
    MD4Transform (context->state, context->buffer);

    for (i = partLen; i + 63 < inputLen; i += 64)
      MD4Transform (context->state, &input[i]);

    index = 0;
  }
  else
    i = 0;

  /* Buffer remaining input */
  //i4_memcpy(&context->buffer[index], &input[i], inputLen-i);
  memcpy(&context->buffer[index], &input[i], inputLen-i);
}

/* MD4 finalization. Ends an MD4 message-digest operation, writing the
     the message digest and zeroizing the context.
 *//* message digest *//* context */
void MD4Final(unsigned char digest[16],MD4_CTX *context)
{
  unsigned char bits[8];
  unsigned int index, padLen;

  /* Save number of bits */
  Encode (bits, context->count, 8);

  /* Pad out to 56 mod 64.
   */
  index = (unsigned int)((context->count[0] >> 3) & 0x3f);
  padLen = (index < 56) ? (56 - index) : (120 - index);
  MD4Update (context, PADDING, padLen);

  /* Append length (before padding) */
  MD4Update (context, bits, 8);
  /* Store state in digest */
  Encode (digest, context->state, 16);

  /* Zeroize sensitive information.
   */
  // i4_memset(context, 0, sizeof (*context));
  memset(context, 0, sizeof (*context));

}

/* MD4 basic transformation. Transforms state based on block.
 */
static void MD4Transform(w32 state[4], const unsigned char block[64])
{
  w32 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

Decode (x, block, 64); 

  /* Round 1 */
  FF (a, b, c, d, x[ 0], S11); /* 1 */
  FF (d, a, b, c, x[ 1], S12); /* 2 */
  FF (c, d, a, b, x[ 2], S13); /* 3 */
  FF (b, c, d, a, x[ 3], S14); /* 4 */
  FF (a, b, c, d, x[ 4], S11); /* 5 */
  FF (d, a, b, c, x[ 5], S12); /* 6 */
  FF (c, d, a, b, x[ 6], S13); /* 7 */
  FF (b, c, d, a, x[ 7], S14); /* 8 */
  FF (a, b, c, d, x[ 8], S11); /* 9 */
  FF (d, a, b, c, x[ 9], S12); /* 10 */
  FF (c, d, a, b, x[10], S13); /* 11 */
  FF (b, c, d, a, x[11], S14); /* 12 */
  FF (a, b, c, d, x[12], S11); /* 13 */
  FF (d, a, b, c, x[13], S12); /* 14 */
  FF (c, d, a, b, x[14], S13); /* 15 */
  FF (b, c, d, a, x[15], S14); /* 16 */

  /* Round 2 */
  GG (a, b, c, d, x[ 0], S21); /* 17 */
  GG (d, a, b, c, x[ 4], S22); /* 18 */
  GG (c, d, a, b, x[ 8], S23); /* 19 */
  GG (b, c, d, a, x[12], S24); /* 20 */
  GG (a, b, c, d, x[ 1], S21); /* 21 */
  GG (d, a, b, c, x[ 5], S22); /* 22 */
  GG (c, d, a, b, x[ 9], S23); /* 23 */
  GG (b, c, d, a, x[13], S24); /* 24 */
  GG (a, b, c, d, x[ 2], S21); /* 25 */
  GG (d, a, b, c, x[ 6], S22); /* 26 */
  GG (c, d, a, b, x[10], S23); /* 27 */
  GG (b, c, d, a, x[14], S24); /* 28 */
  GG (a, b, c, d, x[ 3], S21); /* 29 */
  GG (d, a, b, c, x[ 7], S22); /* 30 */
  GG (c, d, a, b, x[11], S23); /* 31 */
  GG (b, c, d, a, x[15], S24); /* 32 */

  /* Round 3 */
  HH (a, b, c, d, x[ 0], S31); /* 33 */
  HH (d, a, b, c, x[ 8], S32); /* 34 */
  HH (c, d, a, b, x[ 4], S33); /* 35 */
  HH (b, c, d, a, x[12], S34); /* 36 */
  HH (a, b, c, d, x[ 2], S31); /* 37 */
  HH (d, a, b, c, x[10], S32); /* 38 */
  HH (c, d, a, b, x[ 6], S33); /* 39 */
  HH (b, c, d, a, x[14], S34); /* 40 */
  HH (a, b, c, d, x[ 1], S31); /* 41 */
  HH (d, a, b, c, x[ 9], S32); /* 42 */
  HH (c, d, a, b, x[ 5], S33); /* 43 */
  HH (b, c, d, a, x[13], S34); /* 44 */
  HH (a, b, c, d, x[ 3], S31); /* 45 */
  HH (d, a, b, c, x[11], S32); /* 46 */
  HH (c, d, a, b, x[ 7], S33); /* 47 */
  HH (b, c, d, a, x[15], S34); /* 48 */

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;

  /* Zeroize sensitive information.
   */
  //i4_memset(x, 0, sizeof(x));
  memset(x, 0, sizeof(x));
}

/* Encodes input (w32) into output (unsigned char). Assumes len is
     a multiple of 4.
 */
static void Encode(unsigned char *output,w32 *input,unsigned int len)
{
  unsigned int i, j;

  for (i = 0, j = 0; j < len; i++, j += 4) {
    output[j] = (unsigned char)(input[i] & 0xff);
    output[j+1] = (unsigned char)((input[i] >> 8) & 0xff);
    output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);
    output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);
  }
}

/* Decodes input (unsigned char) into output (w32). Assumes len is
     a multiple of 4.
 */
static void Decode(w32 *output, const unsigned char *input,unsigned int len)
{
	unsigned int i, j; 

  for (i = 0, j = 0; j < len; i++, j += 4)
    output[i] = ((w32)input[j]) | (((w32)input[j+1]) << 8) |
      (((w32)input[j+2]) << 16) | (((w32)input[j+3]) << 24);
}


/* FROM THIS POINT ON:
 *
 * (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * Modelled on example code provided by EDonkey developer,
 * but reimplemented.
 * 
 * EDonkey hash identifiers are calculated by:
 *  (1) taking the MD4 hash of each 9,728,000 byte range of the file
 *  (2) concatenating all those hashes together
 *  (3) taking the MD4 of the concatenation
 * 
 */

/* ED2KHash initialization. Set up the two internal MD4 contexts.
 */
void ED2KInit(ED2K_CTX *context)                                        /* context */
{
	context->nextPos = 0;
	MD4Init(&(context->seg_ctx));
	MD4Init(&(context->top_ctx));
}

/* ED2KHash block update operation. 
 */
void ED2KUpdate(ED2K_CTX *context,                                        /* context */
const unsigned char *input,                                /* input block */
unsigned int inputLen)                     /* length of input block */
{
	unsigned int firstLen;
	unsigned char innerDigest[16];

    // first, do no harm
	if(inputLen==0) return;

	// now, close up any segment that's been completed
	if((context->nextPos > 0) && ((context->nextPos % EDSEG_SIZE)==0) ) {
		// finish
		MD4Final(innerDigest,&(context->seg_ctx));
		// feed it to the overall hash
		MD4Update(&(context->top_ctx),innerDigest,16);
		// reset the current segment
		MD4Init(&(context->seg_ctx));
	}

    // now, handle the new data
	if((context->nextPos/EDSEG_SIZE)==(context->nextPos+inputLen)/EDSEG_SIZE) {
		// not finishing any segments, just keep feeding segment hash
		MD4Update(&(context->seg_ctx),input,inputLen);
		context->nextPos += inputLen;
		return;
	}
	// OK, we're reaching or crossing a segment-end

	// finish the current segment
	firstLen = EDSEG_SIZE-(context->nextPos % EDSEG_SIZE);
	MD4Update(&(context->seg_ctx),input,firstLen);
	context->nextPos += firstLen;

	// continue with passed-in info
	ED2KUpdate(context,input+firstLen,inputLen-firstLen);
}

/* ED2KHash finalization. 
 *//* message digest *//* context */
void ED2KFinal(unsigned char digest[16],ED2K_CTX *context)
{
	unsigned char innerDigest[16];

    if(context->nextPos <= EDSEG_SIZE) {
		// there was only one segment; return its hash
		MD4Final(digest,&(context->seg_ctx));
		return;
	}

    // finish the segment in process
	MD4Final(innerDigest,&(context->seg_ctx));

    // feed it to the overall hash
	MD4Update(&(context->top_ctx),innerDigest,16);
	
    // finish the overall hash
    MD4Final(digest,&(context->top_ctx));
}


