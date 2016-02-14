/* (PD) 2004 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain
 * for more info.
 *
 * $Id: main.c,v 1.49 2004/02/03 02:47:34 gojomo Exp $
 */
/*------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "bc_version.h"
#include "bitcollider.h"
#include "cache.h"
#include "bitprint.h"
#include "md5.h"
#include "ed2k_md4.h"
#include "ftuuhash.h"
#include "kztree.h"
#include "mp3.h"
#include "id3.h"
#include "plugin_man.h"
#include "tiger.h"
#include <sys/stat.h>
#ifndef _WIN32
  #include "browser.h"
  #include <errno.h>
  #include <unistd.h>
#else
  #include <windows.h>
  #include <shellapi.h>
#endif

/*------------------------------------------------------------------------- */

#define BUFFER_LEN              4096
#define FIRST_N_HEX             20
#define FIRST_N_HEX_SIZE        (FIRST_N_HEX * sizeof(char) * 2)
#define MAX_ATTR_STRING_LEN     1024
#define DEFAULT_NUM_ATTRS       16
#define GROW_NUM_ATTRS          16
#define MD5_SANITY_CHECK_FAILED "The MD5 hash function compiled into the bitcollider is faulty."
#define MD5_SANITY_CHECK_EMPTY  "2QOYZWMPACZAJ2MABGMOZ6CCPY"
#define MD5_SANITY_CHECK_01234  "IEAMJVCNVELXER7EJJP4CVDHPA"

#ifdef _WIN32
#define DIR_SEP                     '\\'
#else
#define DIR_SEP                     '/'
#endif
#define DB printf("%s:%d\n", __FILE__, __LINE__);

/*------------------------------------------------------------------------- */
#define ERROR_FILENOTFOUND   "File not found or permission denied."
#define ERROR_MALLOCFAILED   "Failed to allocate memory."
#define ERROR_LAUNCHBROWSER  "Cannot launch web browser."
#define ERROR_TEMPFILEERR    "Cannot create a temorary file for the " \
                             "bitprint submission."
#define ERROR_HASHCHECK      "The hash functions compiled into this version " \
                             "of the bitcollider utility are faulty!!!"
#define WARNING_NOTMP3       "This is not an MP3 file. " \
                             "Skipping mp3 information."

/*------------------------------------------------------------------------- */

b_bool calculate_hashes(BitcolliderSubmission *submission,
                        FILE                  *source,
                        char                  *bitprint,
						char                  *crc32,
                        char                  *md5sum,
						char                  *ed2kmd4sum,
						char                  *ftuusum,
                        mp3_info              *mp3Info,
                        PluginMethods         *methods,
                        Attribute            **attrList);
b_bool generate_first_n_hex(BitcolliderSubmission *submission,
                            FILE          *source,
                            int            n,
                            unsigned char *bitprint);
b_bool get_bitprint_data(BitcolliderSubmission *submission,
                         const char            *fileName,
                         char                  *bitprint,
						 char                  *crc32,
                         char                  *md5sum,
						 char                  *ed2kmd4sum,
						 char                  *kzhashhex,
                         unsigned char         *firstHex,
                         mp3_info              *mp3Info,
                         PluginMethods         *methods,
                         Attribute            **attrList);
void   convert_to_multiple_submission(BitcolliderSubmission *submission);
void   set_error(BitcolliderSubmission *sub, const char *newError);
void   set_warning(BitcolliderSubmission *sub, const char *newError);
b_bool check_md5_hash(void);
char  *escape_form_value(char *form_value);
/*------------------------------------------------------------------------- */

Bitcollider *bitcollider_init(b_bool printDebugInfo)
{
    Bitcollider *bc;
#ifdef WIN32
    HKEY         hKey;
    DWORD        type;
#endif
    char         path[MAX_PATH], cacheFile[MAX_PATH], *ptr;
    int          total = 0;

    cacheFile[0] = 0;
    bc = init_plugins();

#ifndef WIN32
    /* Load the plugins from the build dir first if ./plugins exists */
    if (printDebugInfo)
        fprintf(stderr, "Loading plugins from ./plugins:\n");

    total = load_plugins(bc, "./plugins", printDebugInfo);

    ptr = getenv("HOME");
    if (ptr)
    {
        sprintf(cacheFile, "%s/.bitcollider/cache.db", ptr);
        sprintf(path, "%s/.bitcollider/plugins", ptr);
        if (printDebugInfo)
            fprintf(stderr, "Loading plugins from %s:\n", path);
        total += load_plugins(bc, path, printDebugInfo);
    }
    else
    {
        if (printDebugInfo)
            fprintf(stderr, "HOME env var not set. Cannot find home.\n");
    }

    ptr = PREFIX"/lib/bitcollider/plugins";
#else

    path[0] = 0;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Bitzi", 0,
        KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        /* No registry key set. Figure out where our plugin dir is and
           set that key to the registry. */
        set_plugin_dir(path);
        /*printf("No reg key set. path: '%s'\n", path);*/
    }
    else
    {
        int   ret;
        DWORD size = MAX_PATH;

        // Get the plugin dir, first
        ret = RegQueryValueEx(hKey, "PluginDir", NULL, &type,
                              path, &size);

        if (ret == ERROR_FILE_NOT_FOUND)
            set_plugin_dir(path);
        else
        if (ret != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return bc;
        }

        // Now try to get the cache file
        size = MAX_PATH;
        ret = RegQueryValueEx(hKey, "CacheFile", NULL, &type,
                              cacheFile, &size);
        RegCloseKey(hKey);
    }
    ptr = path;
#endif

    if (ptr)
    {
        if (printDebugInfo)
            fprintf(stderr, "Loading plugins from %s:\n", ptr);
        total += load_plugins(bc, ptr, printDebugInfo);
    }
    if (printDebugInfo)
        fprintf(stderr, "Loaded %d plugins total.\n\n", total);

#if USE_BDB
    bc->maxCacheSize = DEFAULT_MAX_CACHE_SIZE;
    if (cacheFile[0] != 0)
    {
        bc->cache = init_cache();
        if (bc->cache)
        {
            if (!open_cache(bc->cache, cacheFile))
            {
                if (printDebugInfo)
                    fprintf(stderr, "Failed to open cache file: %s. "
                                    "Creating a new one.\n\n", cacheFile);
                unlink(cacheFile);
                if (!create_cache(bc->cache, cacheFile, bc->maxCacheSize))
                {
                    if (printDebugInfo)
                        fprintf(stderr, "Failed to create cache file: %s.\n\n",
                                    cacheFile);
                    close_cache(bc->cache);
                    bc->cache = NULL;
                }
            }
        }
    }
#endif

    return bc;
}

void bitcollider_shutdown(Bitcollider *bc)
{
#if USE_BDB
    if (bc->cache)
       close_cache(bc->cache);
#endif
    unload_plugins(bc);
    shutdown_plugins(bc);
}

BitcolliderSubmission *create_submission(Bitcollider *bc)
{
    BitcolliderSubmission *submission;

    submission = (BitcolliderSubmission *)malloc(sizeof(BitcolliderSubmission));
    if (submission == NULL)
       return NULL;

    memset(submission, 0, sizeof(BitcolliderSubmission));
    submission->bc = bc;

    return submission;
}

b_bool analyze_file(BitcolliderSubmission *submission,
                    const char            *fileName,
                    b_bool                 matchingExtsOnly)
{
    char                   bitprint[BITPRINT_BASE32_LEN + 1];
    char                   firstNHex[FIRST_N_HEX_SIZE + 1];
    char                   temp[MAX_ATTR_STRING_LEN], *ext;
	char                   crc32[10];
    char                   md5[64];
	char                   ed2kmd4[64];
	char                   kzhashhex[128];
    const char            *baseFileName;
    mp3_info              *mp3Info = NULL;
    PluginMethods         *methods = NULL;
    b_bool                 mp3Check = false;
    Attribute             *attrList = NULL, *attr;
    const char            *err;
#if USE_BDB
    cache_entry            centry;
    struct stat            fileInfo;
#endif

    if (submission->bc->error)
    {
        free(submission->bc->error);
        submission->bc->error = NULL;
    }
    if (submission->bc->warning)
    {
        free(submission->bc->warning);
        submission->bc->warning = NULL;
    }

    if (submission->fileName)
    {
        free(submission->fileName);
        submission->fileName = NULL;
    }

    if (!check_md5_hash())
    {
        set_error(submission, MD5_SANITY_CHECK_FAILED);
        return false;
    }

    if (submission->bc->exitNow)
    {
       return false;
    }

    submission->fileName = strdup(fileName);
    baseFileName = strrchr(fileName, DIR_SEP);
    if (baseFileName)
       baseFileName++;
    else
       baseFileName = fileName;

    ext = strrchr(baseFileName, '.');
    if (!submission->checkAsExt && ext && strcasecmp(ext, ".mp3") == 0)
    {
       mp3Check = true;
    }

    /* Load the plugin methods for this file */
    ext = (submission->checkAsExt) ? submission->checkAsExt : ext;
    if (ext)
       methods = get_plugin(submission->bc, ext);

    /* If we're only supposed to work on files with known extensions,
       and we don't know this extension, bail. */
    if (matchingExtsOnly && methods == NULL && mp3Check == false)
    {
       if (submission->bc->progressCallback && !submission->bc->preview)
           submission->bc->progressCallback(0, submission->fileName,
                                            "skipped.");
       return false;
    }

    /* If we're in preview mode, return now */
    if (submission->bc->preview)
       return true;

#if USE_BDB
    /* Check to see if this bitprint has already been bitprinted */
#ifdef WIN32
    {
       char *dummy;
       if (GetFullPathName(fileName, MAX_PATH, centry.fileName, &dummy) == 0)
           strcpy(centry.fileName, fileName);
    }
#else
    if (realpath(fileName, centry.fileName) == NULL)
        strcpy(centry.fileName, fileName);
#endif

    if (submission->bc->cache &&
        get_cache_entry(submission->bc->cache, &centry))
    {
        if (_stat(submission->fileName, (struct _stat *)&fileInfo) == 0)
        {
            if (fileInfo.st_mtime == centry.lastModDate)
            {
                if (submission->bc->progressCallback &&
                    !submission->bc->preview)
                    submission->bc->progressCallback(0, submission->fileName,
                                   "found in cache, skipped.");
                return true;
            }
        }
        /* Its out of date or something else is wrong. Remove the entry
           from the cache */
        remove_cache_entry(submission->bc->cache, &centry);
    }
#endif

    if (mp3Check)
       mp3Info = malloc(sizeof(mp3_info));

    if (!get_bitprint_data(submission, fileName, bitprint, crc32, md5, ed2kmd4, kzhashhex, firstNHex,
                           mp3Info, methods, &attrList))
    {
       if (mp3Info)
          free(mp3Info);

       return false;
    }

    /* If this is the first bit print, add a header to the attrs */
    if (submission->numBitprints == 0)
    {
       get_agent_string(temp);
       add_attribute(submission, "head.agent", temp);

       sprintf(temp, "S%s", BC_SUBMITSPECVER);
       add_attribute(submission, "head.version", temp);
    }

    /* If this is the second bitprint, convert the single submission
       to a multiple submission */
    if (submission->numBitprints == 1)
       convert_to_multiple_submission(submission);

    add_attribute(submission, "bitprint", bitprint);

    sprintf(temp, "%lu", submission->fileSize);
    add_attribute(submission, "tag.file.length", temp);
    add_attribute(submission, "tag.file.first20", firstNHex);
    add_attribute(submission, "tag.filename.filename", baseFileName);

    if (submission->bc->calculateCRC32)
	{
		add_attribute(submission, "tag.crc32.crc32", crc32);
	}
    if (submission->bc->calculateMD5)
    {
        add_attribute(submission, "tag.md5.md5", md5);
    }

    add_attribute(submission, "tag.ed2k.ed2khash", ed2kmd4);
	add_attribute(submission, "tag.kzhash.kzhash", kzhashhex);

    /* Check to make sure that we carried out the mp3 check, and
       make sure that an audioSha was generated. If not, then the
       mp3 routines deemed that this was not a valid mp3 file
       and we should skip the mp3 tag generation */
    if (mp3Check && mp3Info->samplerate == 0)
    {
       set_warning(submission, WARNING_NOTMP3);
       mp3Check = 0;
    }
    else
    if (mp3Check)
    {
       char audioShaDigest[SHA_BASE32SIZE + 1];
       ID3Info *info;

       bitziEncodeBase32(mp3Info->audioSha, SHA_DIGESTSIZE, audioShaDigest);
       sprintf(temp, "%d", mp3Info->duration);
       add_attribute(submission, "tag.mp3.duration", temp);
       if (mp3Info->bitrate == 0)
       {
          sprintf(temp, "%d", mp3Info->avgBitrate);
          add_attribute(submission, "tag.mp3.bitrate", temp);
          add_attribute(submission, "tag.mp3.vbr", "y");
       }
       else
       {
          sprintf(temp, "%d", mp3Info->bitrate);
          add_attribute(submission, "tag.mp3.bitrate", temp);
       }
       sprintf(temp, "%d", mp3Info->samplerate);
       add_attribute(submission, "tag.mp3.samplerate", temp);
       add_attribute(submission, "tag.mp3.stereo",
                                  mp3Info->stereo ? "y" : "n");
       add_attribute(submission, "tag.mp3.audio_sha1", audioShaDigest);
       free(mp3Info);

       info = read_ID3_tag(fileName);
       if (info)
       {
           if (info->encoder)
               add_attribute(submission, "tag.mp3.encoder", info->encoder);
           if (info->title)
               add_attribute(submission, "tag.audiotrack.title", info->title);
           if (info->artist)
               add_attribute(submission, "tag.audiotrack.artist", info->artist);
           if (info->album)
               add_attribute(submission, "tag.audiotrack.album", info->album);
           if (info->tracknumber)
               add_attribute(submission, "tag.audiotrack.tracknumber",
                             info->tracknumber);
           if (info->genre && atoi(info->genre) >= 0)
               add_attribute(submission, "tag.id3genre.genre", info->genre);
           if (info->year)
               add_attribute(submission, "tag.audiotrack.year", info->year);
           delete_ID3_tag(info);
       }
    }

    /* If a plugin was selected, but no memory analyze functions were
       provided, call the plugin's file analyze methods */
    if (methods && methods->mem_analyze_init == NULL && !submission->bc->exitNow)
       attrList = methods->file_analyze(fileName);

    if (attrList)
    {
       for(attr = attrList; attr->key; attr++)
          add_attribute(submission, attr->key, attr->value);

        methods->free_attributes(attrList);
    }

    /* If we selected a plugin, but the no attributes were returned,
       then check for an error from the plugin */
    if (methods && !attrList)
    {
       /* An error from a plugin should be considered a warning
          since its not fatal to the execution of the bitcollider */
       err = methods->get_error();
       if (err)
           set_warning(submission, err);
    }

    if (submission->bc->progressCallback &&
        !submission->bc->preview &&
        !submission->bc->exitNow)
       submission->bc->progressCallback(100, NULL, "ok.");

    submission->numBitprints++;

#if USE_BDB
    if (_stat(submission->fileName, (struct _stat *)&fileInfo) == 0)
    {
        centry.lastModDate = fileInfo.st_mtime;
        if (submission->bc->cache &&
            !add_cache_entry(submission->bc->cache, &centry, false))
        {
            printf("Failed to insert bitprint into cache.\n");
        }
    }
#endif

    return true;
}

void set_auto_submit(BitcolliderSubmission *tag,
                     b_bool                 autoSubmit)
{
    tag->autoSubmit = autoSubmit;
}

void set_check_as(BitcolliderSubmission *tag,
                  const char            *extension)
{
    if (tag->checkAsExt)
       free(tag->checkAsExt);

    tag->checkAsExt = strdup(extension);
}

void set_progress_callback(Bitcollider *bc,
                           void (*callback)(int, const char *, const char *))
{
    bc->progressCallback = callback;
}

void set_preview(Bitcollider *bc, b_bool preview)
{
    bc->preview = preview;
}

void set_calculateCRC32(Bitcollider *bc, b_bool calculateCRC32)
{
    bc->calculateCRC32 = calculateCRC32;
}

void set_calculateMD5(Bitcollider *bc, b_bool calculateMD5)
{
    bc->calculateMD5 = calculateMD5;
}

void set_exit(Bitcollider *bc, b_bool exitNow)
{
    if (bc->progressCallback)
       bc->progressCallback(-2, NULL, "operation cancelled.");
    bc->exitNow = exitNow;
}

void clear_bitprint_cache(Bitcollider *bc)
{
#if USE_BDB
    if (bc->cache)
    {
        int maxCount;
        char fileName[MAX_PATH];

        strcpy(fileName, bc->cache->fileName);
        maxCount = get_max_count(bc->cache);
        close_cache(bc->cache);
        unlink(fileName);

        bc->cache = init_cache();
        create_cache(bc->cache, fileName, maxCount);
    }
#endif
}


int get_num_bitprints(BitcolliderSubmission *sub)
{
    return sub->numBitprints;
}

char *get_error(Bitcollider *bc)
{
    return bc->error;
}

void set_error(BitcolliderSubmission *sub, const char *newError)
{
    if (sub->bc->error)
        free(sub->bc->error);
    sub->bc->error = strdup(newError);
}

char *get_warning(Bitcollider *bc)
{
    return bc->warning;
}

void set_warning(BitcolliderSubmission *sub, const char *newWarning)
{
    if (sub->bc->warning)
        free(sub->bc->warning);
    sub->bc->warning = strdup(newWarning);
}

void delete_submission(BitcolliderSubmission *submission)
{
    int i;

    for(i = 0; i < submission->numItems; i++)
    {
        free(submission->attrList[i]->key);
        free(submission->attrList[i]->value);
        free(submission->attrList[i]);
    }

    if (submission->fileName)
       free(submission->fileName);
    free(submission->attrList);
    free(submission);
}

void convert_to_multiple_submission(BitcolliderSubmission *submission)
{
    int   i;
    char *temp;

    for(i = 0; i < submission->numItems; i++)
    {
        if (strncmp("head.version", submission->attrList[i]->key, 12) == 0)
        {
            submission->attrList[i]->value[0] = 'M';
            continue;
        }
        if (strncmp("head.", submission->attrList[i]->key, 5) == 0)
            continue;

        temp = malloc(strlen(submission->attrList[i]->key) + 3);
        sprintf(temp, "0.%s", submission->attrList[i]->key);
        free(submission->attrList[i]->key);
        submission->attrList[i]->key = temp;
    }
}

void convert_to_hex(const unsigned char *buffer,
                    int size,
                    char *hexBuffer)
{
    int i;

    for(i = 0; i < size; i++)
    {
        sprintf(hexBuffer + (i * sizeof(char) * 2), "%02X", buffer[i] & 0xFF);
    }
}


void add_attribute(BitcolliderSubmission *submission,
                   const char *key,
                   const char *value)
{
    int   i;
    char *temp = NULL;

    /* Check to see if the attr list already exists */
    if (submission->attrList == NULL)
    {
        submission->attrList = (Attribute **)malloc(sizeof(Attribute *) *
                                                DEFAULT_NUM_ATTRS);
        memset(submission->attrList, 0, sizeof(submission->attrList));
        submission->numItems = 0;
        submission->numAllocated = DEFAULT_NUM_ATTRS;
    }

    /* Do we have enough space for another attr? If not, grow the array */
    if (submission->numItems == submission->numAllocated)
    {
        submission->numAllocated += GROW_NUM_ATTRS;
        submission->attrList = (Attribute **)realloc(submission->attrList,
                                                 sizeof(Attribute *) *
                                                 submission->numAllocated);
        memset(&submission->attrList[submission->numItems],
               0, sizeof(Attribute *) * GROW_NUM_ATTRS);
    }

    /* If this is a multiple tag submission, prepend the sequence number
       to the key */
    if (submission->numBitprints > 0)
    {
        temp = malloc(strlen(key) + 16);
        sprintf(temp, "%d.%s", submission->numBitprints, key);
        key = temp;
    }

    /* Check to make sure we don't already have this attr */
    for(i = 0; i < submission->numItems; i++)
    {
        if (strcmp(key, submission->attrList[i]->key) == 0)
            return;
    }

    submission->attrList[submission->numItems] =
                (Attribute *)malloc(sizeof(Attribute));
    submission->attrList[submission->numItems]->key = strdup(key);
    submission->attrList[submission->numItems]->value = strdup(value);
    submission->numItems++;

    if (temp)
       free(temp);
}

const char *get_attribute(BitcolliderSubmission *submission,
                          const char *key)
{
    int i;

    for(i = 0; i < submission->numItems; i++)
    {
       if (strcmp(submission->attrList[i]->key, key) == 0)
           return submission->attrList[i]->value;
    }

    return NULL;
}

b_bool get_bitprint_data(BitcolliderSubmission *submission,
                         const char            *fileName,
                         char                  *bitprint,
						 char                  *crc32hex,
                         char                  *md5sum,
						 char                  *ed2kmd4,
						 char                  *kzhashhex,
                         unsigned char         *firstHex,
                         mp3_info              *mp3Info,
                         PluginMethods         *methods,
                         Attribute            **attrList)
{
    FILE  *source;
    b_bool   ret;

    source = fopen(fileName, "rb");
    if (source == NULL)
    {
       set_error(submission, ERROR_FILENOTFOUND);
       return false;
    }

    fseek(source, 0, SEEK_END);
    submission->fileSize = ftell(source);
    fseek(source, 0, SEEK_SET);

    ret = calculate_hashes(submission, source, bitprint, crc32hex, md5sum, ed2kmd4, kzhashhex,
                           mp3Info, methods, attrList);
    if (ret)
        ret = generate_first_n_hex(submission, source, FIRST_N_HEX, firstHex);

    fclose(source);
    return ret;
}

b_bool calculate_hashes(BitcolliderSubmission *submission,
                        FILE                  *source,
                        char                  *bitprint,
						char                  *crc32hex,
                        char                  *md5sum,
						char                  *ed2kmd4sum,
						char                  *kzhashsum,
                        mp3_info              *mcontext,
                        PluginMethods         *methods,
                        Attribute            **attrList)
{
    BP_CONTEXT         bcontext;
    unsigned int       crc32 = 0xffffffff;
    struct MD5Context  md5context;
	ED2K_CTX           ed2kmd4context;
	FTUU_CTX           ftuucontext;
	KZTREE_CONTEXT     kztreecontext;
    unsigned char     *buffer, bitprintRaw[BITPRINT_RAW_LEN], md5Digest[16], ed2kDigest[16], kzhash[36];
    int                bytes;
    b_bool             ret = true;
    Context           *context = NULL;

    if (bitziBitprintInit(&bcontext) == -1)
    {
        set_error(submission, ERROR_HASHCHECK);
        return false;
    }

    if (mcontext)
       mp3_init(mcontext);
    if (methods && methods->mem_analyze_init)
       context = methods->mem_analyze_init();
	if (submission->bc->calculateCRC32)
		crc32 = 0xffffffff; //init
    if (submission->bc->calculateMD5)
        MD5Init(&md5context);
    ED2KInit(&ed2kmd4context);
    FTUUInit(&ftuucontext);
	kztree_init(&kztreecontext);

    buffer = (unsigned char*)malloc(BUFFER_LEN);
    if (buffer == NULL)
    {
       set_error(submission, ERROR_MALLOCFAILED);
       return false;
    }

    submission->percentComplete = 0;
    if (submission->bc->progressCallback && !submission->bc->preview)
        submission->bc->progressCallback(0, submission->fileName, NULL);

    fseek(source, 0, SEEK_SET);
    for(;;)
    {
        if (submission->bc->exitNow)
           return false;

        bytes = fread(buffer, 1, BUFFER_LEN, source);
        if (bytes <= 0)
        {
           ret = feof(source);
           break;
        }

        bitziBitprintUpdate(&bcontext, buffer, bytes);
        if (mcontext)
           mp3_update(mcontext, buffer, bytes);
        if (methods && methods->mem_analyze_update)
           methods->mem_analyze_update(context, buffer, bytes);

        if (submission->bc->calculateCRC32)
            crc32 = hashSmallHash(buffer, bytes , crc32); //crc32 by another name
        if (submission->bc->calculateMD5)
            MD5Update(&md5context, buffer, bytes);
        ED2KUpdate(&ed2kmd4context, buffer, bytes);
        FTUUUpdate(&ftuucontext, buffer, bytes);
		kztree_update(&kztreecontext, buffer, bytes);

        if (submission->bc->progressCallback && !submission->bc->preview)
        {
           int          percentComplete;

           percentComplete = (int)(((word64)ftell(source) * (word64)100) /
                                    (word64)submission->fileSize);
           if (percentComplete != submission->percentComplete)
           {
               submission->bc->progressCallback(percentComplete, NULL, NULL);
               submission->percentComplete = percentComplete;
           }
        }
    }
    submission->percentComplete = 100;

    free(buffer);

    bitziBitprintFinal(&bcontext, bitprintRaw);
    bitziBitprintToBase32(bitprintRaw, bitprint);

    if (mcontext)
        mp3_final(mcontext);
    if (methods && methods->mem_analyze_final)
        *attrList = methods->mem_analyze_final(context);

    if (submission->bc->calculateCRC32)
    {
        crc32=~crc32;
        sprintf(crc32hex,"%08X", crc32);
    }

    if (submission->bc->calculateMD5)
    {
        MD5Final(md5Digest, &md5context);
        //bitziEncodeBase32(md5Digest, 16, md5sum);
		sprintf(md5sum,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		    md5Digest[0],md5Digest[1],md5Digest[2],md5Digest[3],
			md5Digest[4],md5Digest[5],md5Digest[6],md5Digest[7],
			md5Digest[8],md5Digest[9],md5Digest[10],md5Digest[11],
			md5Digest[12],md5Digest[13],md5Digest[14],md5Digest[15]);
    }

    ED2KFinal(ed2kDigest, &ed2kmd4context);
    FTUUFinal(kzhash, &ftuucontext);
	kztree_digest(&kztreecontext,kzhash+20);

    sprintf(ed2kmd4sum,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		    ed2kDigest[0],ed2kDigest[1],ed2kDigest[2],ed2kDigest[3],
			ed2kDigest[4],ed2kDigest[5],ed2kDigest[6],ed2kDigest[7],
			ed2kDigest[8],ed2kDigest[9],ed2kDigest[10],ed2kDigest[11],
			ed2kDigest[12],ed2kDigest[13],ed2kDigest[14],ed2kDigest[15]);

	//bitziEncodeBase64(ftuuDigest,20,ftuusum);
	sprintf(kzhashsum,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		    kzhash[0],kzhash[1],kzhash[2],kzhash[3],
			kzhash[4],kzhash[5],kzhash[6],kzhash[7],
			kzhash[8],kzhash[9],kzhash[10],kzhash[11],
			kzhash[12],kzhash[13],kzhash[14],kzhash[15],
			kzhash[16],kzhash[17],kzhash[18],kzhash[19],
			kzhash[20],kzhash[21],kzhash[22],kzhash[23],
			kzhash[24],kzhash[25],kzhash[26],kzhash[27],
			kzhash[28],kzhash[29],kzhash[30],kzhash[31],
			kzhash[32],kzhash[33],kzhash[34],kzhash[35]);

    return ret;
}

b_bool generate_first_n_hex(BitcolliderSubmission *submission,
                            FILE                  *source,
                            int                    n,
                            unsigned char         *bits)
{
    unsigned char *buffer;

    buffer = (unsigned char*)malloc(n);
    if (buffer == NULL)
    {
       set_error(submission, ERROR_MALLOCFAILED);
       return false;
    }

    fseek(source, 0, SEEK_SET);
    n = fread(buffer, sizeof(unsigned char), n, source);
    if (n < 0)
    {
        free(buffer);
        return false;
    }

    bits[0] = 0;
    convert_to_hex(buffer, n, bits);

    free(buffer);

    return true;
}

b_bool submit_submission(BitcolliderSubmission *submission,
                         const char *url,
                         BrowserEnum browser)
{
    FILE *output;
    int   i, last = -1;
    char  tempFile[MAX_PATH], *escaped;
    b_bool  ret;

    if (submission->numBitprints == 0)
    {
        set_error(submission, "The submission contained no bitprints.");
        return false;
    }

#ifdef _WIN32
         GetTempPath(MAX_PATH, tempFile);
    strcat(tempFile, "bitprint.htm");
#else
    strcpy(tempFile, "/tmp/bitprint.html");
#endif
    output = fopen(tempFile, "wb");
    if (output == NULL)
    {
       set_error(submission, ERROR_TEMPFILEERR);
       return false;
    }

    fprintf(output,
       "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n");
    fprintf(output, "<HTML><HEAD><TITLE>");

    if (submission->numBitprints == 1)
       fprintf(output, "Bitprint Submission %s\n", submission->fileName);
    else
       fprintf(output, "Multiple [%d] Bitprint Submission\n", submission->numBitprints);
    fprintf(output, "</TITLE>\n</HEAD>\n");
    if (submission->autoSubmit)
       fprintf(output, "<BODY onLoad=\"document.forms[0].submit()\">\n");
    else
       fprintf(output, "<BODY>\n");

    if (submission->numBitprints == 1)
       fprintf(output, "<h3>Bitprint Submission %s</h3><p>\n",
                        submission->fileName);
    else
       fprintf(output, "<h3>Multiple [%d] Bitprint Submission</h3><p>\n",
                        submission->numBitprints);

    fprintf(output,
      "You are submitting the following bitprint and tag data to the web "
      "location <i>%s</i>. For more information see <a "
      "href=\"http://bitzi.com/bitcollider/websubmit\">the Bitzi website.</a>"
      "<p>\nIf you are submitting more than a handful of files at once, it "
      "may take a while for this page to load and submit.<p>\n"
      "This submission should occur automatically. If it does not, you "
      "may press the \"submit\" button which will appear at the bottom of "
      "the page.<p><HR>\n", (url == NULL) ? SUBMIT_URL : url);

    fprintf(output, "<FORM method=post action=\"%s\">\n",
                     (url == NULL) ? SUBMIT_URL : url);
    fprintf(output, "<PRE>\n");

    for(i = 0; i < submission->numItems; i++)
    {
        if (last != atoi(submission->attrList[i]->key) || i == 2)
        {
            last = atoi(submission->attrList[i]->key);
            fprintf(output, "\n");
        }
        fprintf(output, "%s=<INPUT TYPE=\"hidden\" ",
                submission->attrList[i]->key);
        escaped = escape_form_value(submission->attrList[i]->value);
        fprintf(output, "NAME=\"%s\" VALUE=\"%s\">%s\n",
                submission->attrList[i]->key,
                escaped,
                submission->attrList[i]->value);
        free(escaped);
    }
    fprintf(output, "\n<INPUT TYPE=\"submit\" NAME=\"Submit\" VALUE=\"Submit\">\n");
    fprintf(output, "</PRE>\n</FORM>\n</BODY>\n</HTML>\n");
    fclose(output);

#ifdef _WIN32
    {
        char url[MAX_PATH], *colon;


        colon = strchr(tempFile, ':');
        if (*colon)
           *colon = '|';
        sprintf(url, "file://%s", tempFile);

        //Have Windows launch the default HTML viewer on the tempfile
        ret = (int)ShellExecute(NULL,"open",url,NULL,NULL,SW_SHOWNORMAL);
        ret = ret > 32;
    }
#else
    ret = launch_browser(tempFile, browser);
#endif
    if (!ret)
       set_error(submission, ERROR_LAUNCHBROWSER);

    return ret;
}

char *escape_form_value(char *form_value)
{
    int i, form_value_length, extra_length;
    char *escaped_value, *ptr;

    form_value_length = strlen(form_value);
    for (i = 0, extra_length = 0; i < form_value_length; ++i)
    {
        switch(form_value[i])
        {
            case '"':
                extra_length += 5;
                break;
            case '&':
                extra_length += 4;
                break;
            case '<':
            case '>':
                extra_length += 3;
                break;
        }
    }

    if (extra_length == 0)
    {
        // This is necessary since the caller must free the memory.
        return strdup(form_value);
    }

    escaped_value = malloc(form_value_length + extra_length + 1);
    for (i = 0, ptr = escaped_value; i < form_value_length; ++i)
    {
        switch(form_value[i])
        {
            case '"':
                strcpy(ptr, "&quot;");
                ptr += 6;
                break;
            case '&':
                strcpy(ptr, "&amp;");
                ptr += 5;
                break;
            case '<':
                strcpy(ptr, "&lt;");
                ptr += 4;
                break;
            case '>':
                strcpy(ptr, "&gt;");
                ptr += 4;
                break;
            default:
                *(ptr++) = form_value[i];
        }
    }
    *ptr = 0;

    return escaped_value;
}

BitcolliderSubmission *read_submission_from_file(Bitcollider *bc, char *fileName)
{
    BitcolliderSubmission *submission;
    char                   buf[BUFFER_LEN], last[BUFFER_LEN], temp[BUFFER_LEN], err[255];
    char                  *c, *t;
    int                    line, empty = 1;
    FILE                  *infile;

    if (!strcmp(fileName, "-"))
       infile = stdin;
    else
       infile = fopen(fileName, "rb");

    submission = (BitcolliderSubmission *)malloc(sizeof(BitcolliderSubmission));
    if (submission == NULL)
    {
       fclose(infile);
       return NULL;
    }
    memset(submission, 0, sizeof(BitcolliderSubmission));
    submission->bc = bc;

    if (infile == NULL)
    {
       sprintf(err, "Can't open tag file: %s", strerror(errno));
       set_error(submission, err);
       return submission;
    }

    last[0] = 0;

    for( line = 1; fgets( buf, BUFFER_LEN, infile ) != NULL; ++line )
    {
       if( ! ( ( t = strchr( buf, '\r' ) ) || ( t = strchr( buf, '\n' ) ) ) )
       {
           if( strlen( buf ) == BUFFER_LEN - 1 )
           {
              sprintf(err, "Line %d exceeds length limit", line );
              set_error(submission, err);
              fclose(infile);
              submission->numBitprints = 0;
              return submission;
           }
           else
           {
              sprintf(err, "Line %d is truncated", line );
              set_error(submission, err);
              fclose(infile);
              submission->numBitprints = 0;
              return submission;
           }
       }
       *t = 0;

       for( c = buf; *c == ' ' && *c == '\t'; ++c )
           ; /* <-- Empty for loop */

       if( ! *c || *c == '#' )
           continue;

       if( ! ( t = strchr( c, '=' ) ) )
       {
           sprintf(err, "Line %d does not appear to contain a tag", line );
           set_error(submission, err);
           fclose(infile);
           submission->numBitprints = 0;
           return submission;
       }
       *t = 0;
       if( ! strncmp( c, "head.", 5 ) )
           continue;
       if( isdigit( *c ) )
       {
           if( ! ( t = strchr( c, '.' ) ) )
           {
              sprintf(err, "Line %d does not appear to contain a tag", line);
              set_error(submission, err);
              fclose(infile);
              submission->numBitprints = 0;
              return submission;
           }
           *t = 0;

           strcpy( last, c );
           c = t + 1;
       }
       if (empty)
       {
           empty = 0;
           get_agent_string(temp);
           add_attribute(submission, "head.agent", temp);

           sprintf(temp, "S%s", BC_SUBMITSPECVER);
           add_attribute(submission, "head.version", temp);
       }

       if(strncmp(c, "bitprint", 8) == 0)
       {
           if (submission->numBitprints == 1)
              convert_to_multiple_submission(submission);

           submission->numBitprints++;
       }
       submission->numBitprints--;
       add_attribute( submission, c, c + strlen( c ) + 1 );
       submission->numBitprints++;
    }

    fclose(infile);

    return submission;
}

void print_submission(BitcolliderSubmission *submission)
{
    int i;

    for(i = 0; i < submission->numItems; i++)
    {
        printf("%s=%s\n",
                submission->attrList[i]->key,
                submission->attrList[i]->value);
    }
}

void get_agent_string(char *agentString)
{
    sprintf(agentString, "%s/%s (%s)", BC_AGENTNAME, BC_VERSION,
                                       BC_AGENTBUILD);
}

#ifdef _WIN32
/* We get to write this function becuase win95 does not
   support this function! Thanks uncle Bill!!
*/
void getLongPathName(const char *shortName, int len, char *longName)
{
    WIN32_FIND_DATA  FindFileData;
    HANDLE           findHandle;
    char             drive[10], path[MAX_PATH];

    findHandle = FindFirstFile(shortName, &FindFileData);
    if (findHandle != INVALID_HANDLE_VALUE)
    {
        _splitpath(shortName, drive, path, NULL, NULL);
        _snprintf(longName, len, "%s%s%s", drive, path, FindFileData.cFileName);
        FindClose(findHandle);
    }
    else
    {   /* Hmmm. I guess we're screwed. */
        strncpy(longName, shortName, len);
    }
    longName[len - 1] = 0;
}
#endif

b_bool check_md5_hash(void)
{
    struct MD5Context  md5context;
    unsigned char      md5Digest[16];
    char               md5Hash[33];

    MD5Init(&md5context);
    MD5Final(md5Digest, &md5context);
    bitziEncodeBase32(md5Digest, 16, md5Hash);

    if (strcmp(MD5_SANITY_CHECK_EMPTY, md5Hash))
        return false;

    MD5Init(&md5context);
    MD5Update(&md5context, "01234", 5);
    MD5Final(md5Digest, &md5context);
    bitziEncodeBase32(md5Digest, 16, md5Hash);

    if (strcmp(MD5_SANITY_CHECK_01234, md5Hash))
        return false;

    return true;
}

