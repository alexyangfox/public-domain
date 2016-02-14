/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: bitprint.c,v 1.4 2003/02/24 10:58:36 gojomo Exp $
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include "bitprint.h"

/* =================================================================== */
/* Main code                                                           */
/* =================================================================== */

#define BASE32_LOOKUP_MAX 43
static unsigned char *base32Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
static unsigned char base32Lookup[BASE32_LOOKUP_MAX][2] =
{
    { '0', 0xFF },
    { '1', 0xFF },
    { '2', 0x1A },
    { '3', 0x1B },
    { '4', 0x1C },
    { '5', 0x1D },
    { '6', 0x1E },
    { '7', 0x1F },
    { '8', 0xFF },
    { '9', 0xFF },
    { ':', 0xFF },
    { ';', 0xFF },
    { '<', 0xFF },
    { '=', 0xFF },
    { '>', 0xFF },
    { '?', 0xFF },
    { '@', 0xFF },
    { 'A', 0x00 },
    { 'B', 0x01 },
    { 'C', 0x02 },
    { 'D', 0x03 },
    { 'E', 0x04 },
    { 'F', 0x05 },
    { 'G', 0x06 },
    { 'H', 0x07 },
    { 'I', 0x08 },
    { 'J', 0x09 },
    { 'K', 0x0A },
    { 'L', 0x0B },
    { 'M', 0x0C },
    { 'N', 0x0D },
    { 'O', 0x0E },
    { 'P', 0x0F },
    { 'Q', 0x10 },
    { 'R', 0x11 },
    { 'S', 0x12 },
    { 'T', 0x13 },
    { 'U', 0x14 },
    { 'V', 0x15 },
    { 'W', 0x16 },
    { 'X', 0x17 },
    { 'Y', 0x18 },
    { 'Z', 0x19 }
};

#define BUFFER_LEN           4096
#define ONEK_SIZE            1025
#define EMPTY_SHA            "3I42H3S6NNFQ2MSVX7XZKYAYSCX5QBYJ"
#define ONE_SHA              "GVVBSK3ZCOYEYVCXJUMMFDKG4Y4VIKFL"
#define ONEK_SHA             "CAE54LXWDA55NWGAR4PNRX2II7TR66WL"
#define EMPTY_TIGER          "LWPNACQDBZRYXW3VHJVCJ64QBZNGHOHHHZWCLNQ"
#define ONE_TIGER            "QMLU34VTTAIWJQM5RVN4RIQKRM2JWIFZQFDYY3Y"
#define ONEK_TIGER           "CDYY2OW6F6DTGCH3Q6NMSDLSRV7PNMAL3CED3DA"

static int check_sha1_hash(const char *result, unsigned char *data, int len);
static int check_tigertree_hash(const char *result, unsigned char *data, int len);
static int hash_sanity_check(void);

int bitziBitprintFile(const char *fileName, unsigned char *bitprint)
{
    FILE          *file;
    int            ret;

    file = fopen(fileName, "rb");
    if (file == NULL)
        return 0;

    ret = bitziBitprintStream(file, bitprint);
    fclose(file);

    return ret;
}

int bitziBitprintStream(FILE *source, unsigned char *bitprint)
{
    BP_CONTEXT      context;
    unsigned char  *buffer;
    int             bytes;
    int             ret = 1;

    if (bitziBitprintInit(&context) == -1)
        return -1;

    buffer = (unsigned char*)malloc(BUFFER_LEN);
    if (buffer == NULL)
    {
       return 0;
    }

    fseek(source, 0, SEEK_SET);
    for(;;)
    {
        bytes = fread(buffer, 1, BUFFER_LEN, source);
        if (bytes <= 0)
        {
           ret = feof(source) != 0;
           break;
        }

        bitziBitprintUpdate(&context, buffer, bytes);
    }

    free(buffer);

    bitziBitprintFinal(&context, bitprint);

    return ret;
}

int bitziBitprintBuffer(unsigned char *buffer,
                        unsigned int   bufLen,
                        unsigned char *bitprint)
{
    BP_CONTEXT      context;

    if (bitziBitprintInit(&context) == -1)
        return -1;

    bitziBitprintUpdate(&context, buffer, bufLen);
    bitziBitprintFinal(&context, bitprint);

    return 1;
}

int bitziBitprintInit(BP_CONTEXT *context)
{
    if (hash_sanity_check() > 0)
        return -1;

    tt_init(&context->tcontext);
    sha_init(&context->scontext);

    return 1;
}

void bitziBitprintUpdate(BP_CONTEXT *context, unsigned char *buffer, unsigned bytes)
{
    tt_update(&context->tcontext, buffer, bytes);
    sha_update(&context->scontext, buffer, bytes);
}

void bitziBitprintFinal(BP_CONTEXT *context, unsigned char *bitprint)
{
    sha_final(bitprint, &context->scontext);
    tt_digest(&context->tcontext, bitprint + SHA_DIGESTSIZE);
}

void bitziBitprintToBase32(const unsigned char *bitprint,
                           char *base32Bitprint)
{
    bitziEncodeBase32(bitprint, BITPRINT_RAW_LEN, base32Bitprint);
    memmove(base32Bitprint + SHA_BASE32SIZE + 1, 
            base32Bitprint + SHA_BASE32SIZE, TIGER_BASE32SIZE + 1);
    base32Bitprint[SHA_BASE32SIZE] = '.';
}

void bitziBitprintFromBase32(const char *base32BitprintWithDot,
                             unsigned char *bitprint)
{
    char base32Bitprint[BITPRINT_BASE32_LEN + 1];

    memcpy(base32Bitprint, base32BitprintWithDot, SHA_BASE32SIZE);
    memcpy(base32Bitprint + SHA_BASE32SIZE, 
           base32BitprintWithDot + SHA_BASE32SIZE + 1, 
           TIGER_BASE32SIZE + 1);
    bitziDecodeBase32(base32Bitprint, BITPRINT_BASE32_LEN, bitprint);
}

int bitziGetBase32EncodeLength(int rawLength)
{
    return ((rawLength * 8) / 5) + ((rawLength % 5) != 0) + 1;
}

int bitziGetBase32DecodeLength(int base32Length)
{
    return ((base32Length * 5) / 8);
}

void bitziEncodeBase32(const unsigned char *buffer,
                       unsigned int         bufLen,
                       char                *base32Buffer)
{
    unsigned int   i, index;
    unsigned char  word;

    for(i = 0, index = 0; i < bufLen;)
    {
        /* Is the current word going to span a byte boundary? */
        if (index > 3)
        {
            word = (buffer[i] & (0xFF >> index));
            index = (index + 5) % 8;
            word <<= index;
            if (i < bufLen - 1)
                word |= buffer[i + 1] >> (8 - index);

            i++;
        }
        else
        {
            word = (buffer[i] >> (8 - (index + 5))) & 0x1F;
            index = (index + 5) % 8;
            if (index == 0)
               i++;
        }

        assert(word < 32);
        *(base32Buffer++) = (char)base32Chars[word];
    }

    *base32Buffer = 0;
}

void bitziDecodeBase32(const char    *base32Buffer,
                       unsigned int   base32BufLen,
                       unsigned char *buffer)
{
    int            i, index, max, lookup, offset;
    unsigned char  word;

    memset(buffer, 0, bitziGetBase32DecodeLength(base32BufLen));
    max = strlen(base32Buffer);
    for(i = 0, index = 0, offset = 0; i < max; i++)
    {
        lookup = toupper(base32Buffer[i]) - '0';
        /* Check to make sure that the given word falls inside
           a valid range */
        if ( lookup < 0 && lookup >= BASE32_LOOKUP_MAX)
           word = 0xFF;
        else
           word = base32Lookup[lookup][1];

        /* If this word is not in the table, ignore it */
        if (word == 0xFF)
           continue;

        if (index <= 3)
        {
            index = (index + 5) % 8;
            if (index == 0)
            {
               buffer[offset] |= word;
               offset++;
            }
            else
               buffer[offset] |= word << (8 - index);
        }
        else
        {
            index = (index + 5) % 8;
            buffer[offset] |= (word >> index);
            offset++;

            buffer[offset] |= word << (8 - index);
        }
    }
}

static int hash_sanity_check(void)
{
    int            ret;
    unsigned char *data;


    ret = check_tigertree_hash(EMPTY_TIGER, "", 0);
    ret += check_sha1_hash(EMPTY_SHA, "", 0);
    ret += check_tigertree_hash(ONE_TIGER, "1", 1);
    ret += check_sha1_hash(ONE_SHA, "1", 1);

    data = malloc(ONEK_SIZE);
    memset(data, 'a', ONEK_SIZE);
    ret += check_tigertree_hash(ONEK_TIGER, data, ONEK_SIZE);
    ret += check_sha1_hash(ONEK_SHA, data, ONEK_SIZE);
    free(data);

    return ret;
}

/* NOTE: This function returns true if it failed the check! */
static int check_tigertree_hash(const char *result, unsigned char *data, int len)
{
    TT_CONTEXT      tcontext;
    unsigned char   tigerTreeHash[TIGERSIZE];
    char            tigerTreeDigest[TIGER_BASE32SIZE + 1];

    tt_init(&tcontext);
    tt_update(&tcontext, data, len);
    tt_digest(&tcontext, tigerTreeHash);
    bitziEncodeBase32(tigerTreeHash, TIGERSIZE, tigerTreeDigest);

    if (strcmp(tigerTreeDigest, result))
    {
        fprintf(stderr, "        tigertree: '%s' [%d]\n",
                tigerTreeDigest, len);
        fprintf(stderr, "correct tigertree: '%s' [%d]\n",
                result, len);
        return 1;
    }
    return 0;
}

/* NOTE: This function returns true if it failed the check! */
static int check_sha1_hash(const char *result, unsigned char *data, int len)
{
    SHA_INFO        scontext;
    unsigned char   shaHash[SHA_DIGESTSIZE];
    char            shaDigest[SHA_BASE32SIZE + 1];

    sha_init(&scontext);
    sha_update(&scontext, data, len);
    sha_final(shaHash, &scontext);
    bitziEncodeBase32(shaHash, SHA_DIGESTSIZE, shaDigest);

    if (strcmp(shaDigest, result))
    {
        fprintf(stderr, "              sha: '%s' [%d]\n", shaDigest, len);
        fprintf(stderr, "      correct sha: '%s' [%d]\n", result, len);
        return 1;
    }
    return 0;
}
