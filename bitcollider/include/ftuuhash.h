/* Support for calculating fasttrack identifiers
 *
 *  $Id: ftuuhash.h,v 1.3 2003/10/27 03:09:35 gojomo Exp $
 */

#ifndef __FTUUHASH_H__
#define __FTUUHASH_H__

#include "md5.h"

#ifndef __BYTE__
#define __BYTE__
typedef unsigned char  byte;
#endif


/* ftuu context. */
typedef struct {
  struct MD5Context	   md5context;    // for md5'ing the first 307,200 bytes
  unsigned long    nextPos;       // next byte location of the file to be processed
  unsigned int     smallHash;     // running weak hash of later file ranges
  unsigned int     backupSmallHash; // in case the endrange overlaps the last internal range
  byte             rollingBuffer[307200]; // the last 307,200 bytes read
  unsigned long    nextSampleStart;  // position where the next 307,200 range to be smallHashed ends
} FTUU_CTX;

void FTUUInit(FTUU_CTX * );
void FTUUUpdate(FTUU_CTX *, const unsigned char *, unsigned int);
void FTUUFinal(unsigned char [20], FTUU_CTX *);
unsigned int hashSmallHash(byte *, size_t , unsigned int );
void bitziEncodeBase64(byte *, int , char *);

#endif
