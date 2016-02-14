/*  Support for calculating EDonkey2000 hashes. 
 * 
 *  $Id: ed2k_md4.h,v 1.2 2002/07/06 09:38:02 gojomo Exp $
 */

#ifndef ED2KMD4_H
#define ED2KMD4_H

/* FOR THE MD4 FUNCTIONS AND STRUCTURES:

   Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
   rights reserved.

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

typedef unsigned int w32;

/* MD4 context. */
typedef struct {
  w32 state[4];                                   /* state (ABCD) */
  w32 count[2];        /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                         /* input buffer */
} MD4_CTX;

void MD4Init(MD4_CTX * );
void MD4Update(MD4_CTX *, const unsigned char *, unsigned int);
void MD4Final(unsigned char [16], MD4_CTX *);

/* FROM THIS POINT ON:
 *
 * (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 */

/* ED2K context. */
typedef struct {
  MD4_CTX seg_ctx;   // the current 9,216,000 byte block
  MD4_CTX top_ctx;   // the total file value
  unsigned long nextPos;
} ED2K_CTX;

void ED2KInit(ED2K_CTX * );
void ED2KUpdate(ED2K_CTX *, const unsigned char *, unsigned int);
void ED2KFinal(unsigned char [16], ED2K_CTX *);

#endif /* !ED2KMD4_H */
