/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: cache.h,v 1.3 2001/10/12 21:37:52 mayhemchaos Exp $
 */
#ifndef CACHE_H
#define CACHE_H

#ifndef WIN32
  #include "config.h"
#endif

#ifdef USE_BDB

#include <db.h>
#include "defs.h"
#include "bitprint.h"

#define ERR_STRING_LEN 1024

typedef struct _cache_info
{
    DB    *dbp;
    char   errorString[ERR_STRING_LEN];
    int    maxCount;
    char   fileName[MAX_PATH];
} cache_info;

typedef struct _cache_entry
{
    char   fileName[MAX_PATH];
    time_t lastModDate;
    char   bitprint[BITPRINT_BASE32_LEN];
    time_t lastUsedDate;
} cache_entry;

cache_info *init_cache(void);
b_bool      open_cache(cache_info *info, const char *fileName);
b_bool      create_cache(cache_info *info, const char *fileName, int maxCount);
void        close_cache(cache_info *info); 

/* Fill out the fileName in the cache entry and get will fill out the
   other fields. */
b_bool  get_cache_entry(cache_info *info, cache_entry *entry);

/* Add an entry to the cache. Set update to true and the add function 
   updates an existing record. */
b_bool  add_cache_entry(cache_info *info, cache_entry *entry, b_bool update);

/* Remove a record from the cache. The key in the entry must be set */
b_bool  remove_cache_entry(cache_info *info, cache_entry *entry);

/* Remove the numEntries least recently used entries */
b_bool  remove_excessive_entries(cache_info *info, int curCount);

/* Internal functions: */
b_bool set_count(cache_info *info, int count);
int get_count(cache_info *info);

b_bool set_max_count(cache_info *info, int count);
int get_max_count(cache_info *info);

#endif

#endif
