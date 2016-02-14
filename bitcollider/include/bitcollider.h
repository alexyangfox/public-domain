/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: bitcollider.h,v 1.14 2003/10/27 03:09:35 gojomo Exp $
 */
#ifndef BITCOLLIDER_H
#define BITCOLLIDER_H

#ifndef WIN32
   #include "config.h"
#endif
#include "cache.h"
#include "plugin.h"
#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SUBMIT_URL "http://bitzi.com/lookup/"
#define MAX_PLUGINS 256

typedef struct _PluginInfo
{
   PluginMethods   *methods;
   SupportedFormat *formats;
#ifdef WIN32
   HANDLE           handle;
#else
   void            *handle;
#endif
   char            *file;
} PluginInfo;

typedef struct _Bitcollider
{
    PluginInfo  plugins[MAX_PLUGINS];
    int         numPluginsLoaded;
    char       *warning;
    char       *error; 
    void      (*progressCallback)(int, const char *, const char *);
    b_bool      preview;
    b_bool      calculateMD5;
    b_bool      calculateCRC32;
    b_bool      exitNow;
#if USE_BDB
    cache_info *cache;
    int         maxCacheSize;
#endif
} Bitcollider;

typedef struct _BitcolliderSubmission
{
    Bitcollider   *bc; 
    Attribute    **attrList;     
    int            numBitprints;
    int            numItems;
    int            numAllocated;
    char          *fileName;          // The current filename and filesize
    unsigned long  fileSize;
    b_bool         autoSubmit;
    char          *checkAsExt;
    int            percentComplete;
} BitcolliderSubmission;

typedef enum
{
   eBrowserNetscape = 0,
   eBrowserMozilla,
   eBrowserKonqueror,
   eBrowserOpera,
   eBrowserLynx
} BrowserEnum;

Bitcollider           *bitcollider_init    (b_bool printDebugInfo);
void                   bitcollider_shutdown(Bitcollider *bc);

char                  *get_error           (Bitcollider *bc);
char                  *get_warning         (Bitcollider *bc);
void                  set_progress_callback(Bitcollider          *bc,
                                            void (*progress_func)(int,
                                                                 const char *, 
                                                                 const char *));
void                   set_preview         (Bitcollider          *bc,
                                            b_bool                enable);
void                   set_calculateMD5    (Bitcollider          *bc,
                                            b_bool                enable);
void                   set_calculateCRC32  (Bitcollider          *bc,
                                            b_bool                enable);
void                   set_exit            (Bitcollider          *bc,
                                            b_bool                enable);
void                   clear_bitprint_cache(Bitcollider          *bc);


BitcolliderSubmission *create_submission   (Bitcollider *bc);
void                   delete_submission   (BitcolliderSubmission *tag);
void                   set_auto_submit     (BitcolliderSubmission *tag,
                                            b_bool                 autoSubmit);
void                   set_check_as        (BitcolliderSubmission *tag,
                                            const char            *extension);
b_bool                 analyze_file        (BitcolliderSubmission *tag,
                                            const char *fileName,
                                            b_bool      matchingExtsOnly);
b_bool                 submit_submission   (BitcolliderSubmission *tag, 
                                            const char *url,
                                            BrowserEnum browser);
BitcolliderSubmission *read_submission_from_file(Bitcollider *bc, 
                                            char *tagFile);
int                    get_num_bitprints   (BitcolliderSubmission *sub);
void                   print_submission    (BitcolliderSubmission *tag);
void                   convert_to_hex      (const unsigned char *buffer, 
                                            int size, 
                                             char *hexBuffer);
void                   add_attribute       (BitcolliderSubmission *tag,
                                            const char *key, 
                                            const char *value);
const char            *get_attribute       (BitcolliderSubmission *tag,
                                            const char *key);
void                   get_agent_string    (char *agentString);

/* For debugging */
void                   set_plugin_debug    (BitcolliderSubmission *tag,
                                            b_bool                 debug);

/* Misc Win32 Support stuff */
#ifdef _WIN32
void getLongPathName(const char *shortName, int len, char *longName);
#endif

#ifdef __cplusplus
}
#endif

#endif
