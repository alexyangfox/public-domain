/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: tiger.h,v 1.5 2002/07/06 09:38:02 gojomo Exp $
 */
#ifndef TIGER_H
#define TIGER_H

#ifdef _WIN32

#if defined (_INTEGRAL_MAX_BITS) && (_INTEGRAL_MAX_BITS >= 64)
  typedef unsigned __int64 word64;
#else
  #error __int64 type not supported
#endif
typedef unsigned long int tword;

#else
typedef unsigned long long int word64;
#endif

typedef unsigned long  word32;
typedef unsigned short word16;

#ifndef __BYTE__
#define __BYTE__
typedef unsigned char  byte;
#endif

void tiger(word64 *str, word64 length, word64 *res);
 
#endif
