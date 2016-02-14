/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: bitprint.h,v 1.2 2001/12/05 18:43:52 mayhemchaos Exp $
 */
#ifndef BITPRINT_H
#define BITPRINT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sha1.h"
#include "tigertree.h"

/* =================================================================== */
/* Definitions for the Bitzi functions                                 */
/* =================================================================== */

/* BITPRINT_RAW_LEN defines the length of the bitprint returned by the
   bitziCreateBitprint function. The bitprint argument needs to have
   at least BITPRINT_RAW_LEN bytes available.
*/
#define BITPRINT_RAW_LEN    44
#define BITPRINT_BASE32_LEN 72
#define SHA_BASE32SIZE      32
#define TIGER_BASE32SIZE    39

/* This is the context class for the bitziInit, bitziUpdate, bitziFinal
   bitprint calculation routines */
typedef struct _BP_CONTEXT
{
   TT_CONTEXT tcontext;
   SHA_INFO   scontext;
} BP_CONTEXT;

/* The main bitprint creation routines. */
int  bitziBitprintFile(const char *fileName, unsigned char *bitprint);
int  bitziBitprintStream(FILE *file, unsigned char *bitprint);
int  bitziBitprintBuffer(unsigned char *buffer,
                         unsigned int bufLen,
                         unsigned char *bitprint);

/* Routines to calculate a bitprint from multiple memory buffers */
int  bitziBitprintInit(BP_CONTEXT *context);
void bitziBitprintUpdate(BP_CONTEXT *context,
                         unsigned char *buffer,
                         unsigned bytes);
void bitziBitprintFinal(BP_CONTEXT *context,
                        unsigned char *bitprint);

/* Support routines for converting bitprints from the raw binary form
   into a printable ascii version. Note that the char *bitprintBase32
   argument to bitziBitprintToBase32 needs to have BITPRINT_BASE32_LEN
   bytes available, and the bitprint argument to bitziBitprintFromBase32
   needs to have BITPRINT_RAW_LEN bytes available */
void bitziBitprintToBase32(const unsigned char *bitprint, char *bitprintBase32);
void bitziBitprintFromBase32(const char *bitprintBase32, unsigned char *bitprint);

/* General Base32 encoding/decoding support */
int  bitziGetBase32EncodeLength(int rawLength);
int  bitziGetBase32DecodeLength(int base32Length);
void bitziEncodeBase32(const unsigned char *buffer,
                       unsigned int         bufLen,
                       char                *base32Buffer);
void bitziDecodeBase32(const char    *base32Buffer,
                       unsigned int   base32BufLen,
                       unsigned char *buffer);

#ifdef __cplusplus
}
#endif

#endif


