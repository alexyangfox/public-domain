/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: defs.h,v 1.3 2001/10/12 23:07:42 mayhemchaos Exp $
 */
#ifndef DEF_H
#define DEF_H

#ifdef WIN32
#include <windows.h>
#define strcasecmp stricmp
#else 
#define _stat stat
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int b_bool;
#ifndef true
  #define true 1
#endif
#ifndef false
  #define false 0
#endif

#ifndef MAX_PATH
  #define MAX_PATH  1024
#endif

#define DEFAULT_MAX_CACHE_SIZE 8192

#ifdef __cplusplus
}
#endif

#endif
