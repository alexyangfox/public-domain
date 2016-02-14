/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: cache.c,v 1.2 2001/10/12 19:59:51 mayhemchaos Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cache.h"

#define DB printf("%s:%d\n", __FILE__, __LINE__);
#define COUNT_KEY "??count??"
#define MAX_COUNT_KEY "??maxcount??"

#if USE_BDB
cache_info *init_cache(void)
{
    cache_info *info;

    info = malloc(sizeof(cache_info));
    memset(info, 0, sizeof(cache_info));

    return info;
}

b_bool open_cache(cache_info *info, const char *fileName)
{
    int ret;

    strcpy(info->fileName, fileName);
    ret = db_create(&info->dbp, NULL, 0);
    if (ret != 0)
    {
        strcpy(info->errorString, db_strerror(ret));
        return false;
    }
    ret = info->dbp->open(info->dbp, fileName, NULL, DB_BTREE, 0, 0664);
    if (ret != 0)
    {
        strcpy(info->errorString, db_strerror(ret));
        return false;
    }

    info->maxCount = get_max_count(info);

    return true;
}

b_bool create_cache(cache_info *info, const char *fileName, int maxCount)
{
    int ret;

    strcpy(info->fileName, fileName);
    ret = db_create(&info->dbp, NULL, 0);
    if (ret != 0)
    {
        strcpy(info->errorString, db_strerror(ret));
        return false;
    }
    ret = info->dbp->open(info->dbp, fileName, NULL, DB_BTREE, DB_CREATE, 0664);
    if (ret != 0)
    {
        strcpy(info->errorString, db_strerror(ret));
        return false;
    }

    info->maxCount = maxCount;
    set_max_count(info, info->maxCount);
    set_count(info, 0);

    return true;
}

void close_cache(cache_info *info) 
{
    info->dbp->close(info->dbp, 0);
    free(info);
}

b_bool get_cache_entry(cache_info *info, cache_entry *entry)
{
    DBT   key, data;
    int   ret;
    char *str, *token;

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    key.data = entry->fileName;
    key.size = strlen(entry->fileName);

    ret = info->dbp->get(info->dbp, NULL, &key, &data, 0);
    if (ret == DB_NOTFOUND)
    {
        info->errorString[0] = 0;
        return false;
    }

    if (ret != 0)
    {
        strcpy(info->errorString, db_strerror(ret));
        return false;
    }

    str = malloc(data.size + 1);
    strncpy(str, data.data, data.size);
    str[data.size] = 0;

    token = strtok(str, "*");
    if (token)
    {
        entry->lastUsedDate = atoi(token);
        token = strtok(NULL, "*");
        if (token)
        {
            entry->lastModDate = atoi(token);
            token = strtok(NULL, "*");
            if (token)
                strcpy(entry->bitprint, token);
        }
    }
    else
    {
        entry->lastModDate = 0;
        entry->bitprint[0] = 0;
    }

    free(str);

    return true;
}

b_bool add_cache_entry(cache_info *info, cache_entry *entry, b_bool update)
{
    DBT    key, data;
    int    ret;
    char   strData[100];
    time_t t;

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    key.data = entry->fileName;
    key.size = strlen(entry->fileName);

    time(&t);
    sprintf(strData, "%ld*%ld*%s", 
                     t, (long)entry->lastModDate, entry->bitprint);
    data.data = strData;
    data.size = strlen(strData);

    ret = info->dbp->put(info->dbp, NULL, &key, &data, 
                         update ? 0 : DB_NOOVERWRITE);
    if (ret != 0)
    {
        strcpy(info->errorString, db_strerror(ret));
        printf("insert failed: %s\n", info->errorString);
        return false;
    }

    if (!update)
    {
        int count;

        count = get_count(info) + 1;
        if (count > info->maxCount)
        {
            /* remove_excessive will update the record count */
            remove_excessive_entries(info, count);
        }
        else
            set_count(info, count);
    }
    return true;
}

b_bool remove_cache_entry(cache_info *info, cache_entry *entry)
{
    DBT key;
    int ret;

    memset(&key, 0, sizeof(DBT));

    key.data = entry->fileName;
    key.size = strlen(entry->fileName);

    ret = info->dbp->del(info->dbp, NULL, &key, 0);
    if (ret != 0)
    {
        strcpy(info->errorString, db_strerror(ret));
        return false;
    }
    return true;
}

int get_count(cache_info *info)
{
    DBT   key, data;
    int   ret;

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    key.data = COUNT_KEY;
    key.size = sizeof(COUNT_KEY);

    ret = info->dbp->get(info->dbp, NULL, &key, &data, 0);
    if (ret == DB_NOTFOUND)
    {
        info->errorString[0] = 0;
        return 0;
    }

    if (ret != 0)
    {
        strcpy(info->errorString, db_strerror(ret));
        return -1;
    }
    ((char *)data.data)[data.size] = 0;

    return atoi(data.data);
}

b_bool set_count(cache_info *info, int count)
{
    DBT    key, data;
    int    ret;
    char   strData[10];

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    key.data = COUNT_KEY;
    key.size = sizeof(COUNT_KEY);

    sprintf(strData, "%d", count);
    data.data = strData;
    data.size = strlen(strData);

    ret = info->dbp->put(info->dbp, NULL, &key, &data, 0); 
    if (ret != 0)
    {
        strcpy(info->errorString, db_strerror(ret));
        return false;
    }
    return true;
}

int get_max_count(cache_info *info)
{
    DBT   key, data;
    int   ret;

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    key.data = MAX_COUNT_KEY;
    key.size = sizeof(MAX_COUNT_KEY);

    ret = info->dbp->get(info->dbp, NULL, &key, &data, 0);
    if (ret == DB_NOTFOUND)
    {
        info->errorString[0] = 0;
        return 0;
    }

    if (ret != 0)
    {
        strcpy(info->errorString, db_strerror(ret));
        return -1;
    }
    ((char *)data.data)[data.size] = 0;

    return atoi(data.data);
}

b_bool set_max_count(cache_info *info, int count)
{
    DBT    key, data;
    int    ret;
    char   strData[10];

    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    key.data = MAX_COUNT_KEY;
    key.size = sizeof(MAX_COUNT_KEY);

    sprintf(strData, "%d", count);
    data.data = strData;
    data.size = strlen(strData);

    ret = info->dbp->put(info->dbp, NULL, &key, &data, 0); 
    if (ret != 0)
    {
        strcpy(info->errorString, db_strerror(ret));
        return false;
    }
    return true;
}

static int compare_int(const void *a, const void *b)
{
    if (*(unsigned int *)a < *(unsigned int *)b)
        return -1;
    if (*(unsigned int *)a > *(unsigned int *)b)
        return 1;
    return 0;
}

b_bool remove_excessive_entries(cache_info *info, int curCount)
{
    DBT          key, data;
    DBC         *dbcp;
    int          ret, i, newCount;
    time_t       cutoff, *timeCache;
    int          numEntries;

    numEntries = info->maxCount / 10;
    if (curCount < 0)
        curCount = get_count(info);

    if (numEntries < 1 || numEntries > curCount)
        return false;

    ret = info->dbp->cursor(info->dbp, NULL, &dbcp, 0);
    if (ret != 0) 
    {
        strcpy(info->errorString, db_strerror(ret));
        return false;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));

    timeCache = malloc(sizeof(int) * curCount);
    for(i = 0; i < curCount + 1; i++)
    {
        ret = dbcp->c_get(dbcp, &key, &data, DB_NEXT);
        if (ret != 0)
           break;

        if (((char *)key.data)[0] == '?')
        {
            i--;
            continue;
        }

        timeCache[i] = atoi(data.data);
    }
    dbcp->c_close(dbcp);

    qsort(timeCache, curCount, sizeof(int), compare_int);
    cutoff = timeCache[numEntries]; 
    free(timeCache);

    ret = info->dbp->cursor(info->dbp, NULL, &dbcp, 0);
    if (ret != 0) 
    {
        strcpy(info->errorString, db_strerror(ret));
        return false;
    }

    newCount = curCount;
    for(i = 0; i < curCount && newCount > 0; i++)
    {
        ret = dbcp->c_get(dbcp, &key, &data, DB_NEXT);
        if (ret != 0)
            break;

        if (((char *)key.data)[0] == '?')
        {
            i--;
            continue;
        }

        if (atoi(data.data) <= cutoff)
        {
            ret = info->dbp->del(info->dbp, NULL, &key, 0);
            if (ret == 0)
                newCount--;
        }
    }
    dbcp->c_close(dbcp);
    set_count(info, newCount);

    return true;
}
#endif

#if 0
int main(void)
{
    int i;
    unsigned rand;
    cache_info *info;
    cache_entry entry;
    time_t t;

    time(&t);
    srandom(t);

    info = init_cache();
    open_cache(info, "test.db");

    for(i = 0; i < 34873; i++)
    {
        sprintf(entry.fileName, "%u%u%u", random(), random(), random());
        entry.lastUsedDate = random() % t;
        entry.lastModDate = random() % t;
        sprintf(entry.bitprint, "foo");

        printf("\nInsert entry %d\n", i);
        add_cache_entry(info, &entry, 0);
    }
    close_cache(info);

    return 1;
}
#endif
