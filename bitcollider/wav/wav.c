/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: wav.c,v 1.10 2004/02/03 01:11:07 mayhemchaos Exp $
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#ifdef WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#include <sys/param.h>
#endif

#include "plugin.h"
#include "sha1.h"
#include "bitcollider.h"

/*-------------------------------------------------------------------------*/

#ifndef _WIN32
#define init_plugin wav_init_plugin
#endif

PluginMethods           *init_plugin(void);
static void              wav_shutdown_plugin(void);
static void              wav_free_attributes(Attribute *attrList);
static SupportedFormat  *wav_get_supported_formats(void);
static const char       *wav_get_name(void);
static const char       *wav_get_version(void);
static Context          *wav_analyze_init(void);
static void              wav_analyze_update(Context             *context, 
                                            const unsigned char *buf,
                                            unsigned             bufLen);
static Attribute        *wav_analyze_final(Context *context);
static char             *wav_get_error(void);

static long toLittleEndian32(long in);
static short toLittleEndian16(short in);

/*-------------------------------------------------------------------------*/

#define CHUNK          4096
#define PLUGIN_VERSION "1.0.0"
#define PLUGIN_NAME    "WAV file information"
#define NUM_ATTRS      6 

/*-------------------------------------------------------------------------*/

typedef int            int32;
typedef unsigned       uint32;
typedef short          int16;
typedef unsigned short uint16;
typedef struct _WavContext

{
   SHA_INFO audioSha1;
   b_bool   stereo;
   uint32   sampleRate, channels, sampleSize;
   uint32   dataLen;
   uint32   samples;
   int      bytesProcessed;
} WavContext;

/*-------------------------------------------------------------------------*/

static SupportedFormat formats[] =
{ 
     { ".wav", "WAV Audio format" },
     { NULL,   NULL               }
};
static char *errorString = NULL;

static PluginMethods methods = 
{
    wav_shutdown_plugin,
    wav_get_version,
    wav_get_name,
    wav_get_supported_formats,
    NULL, /* the file functions is not supported in this plugin */
    wav_analyze_init, 
    wav_analyze_update,
    wav_analyze_final,
    wav_free_attributes,
    wav_get_error
};

/*-------------------------------------------------------------------------*/

PluginMethods *init_plugin(void)
{
    return &methods;
}

static void wav_shutdown_plugin(void)
{
    if (errorString)
       free(errorString);
}

static const char *wav_get_version(void)
{
    return PLUGIN_VERSION;
}

static const char *wav_get_name(void)
{
    return PLUGIN_NAME;
}

static SupportedFormat *wav_get_supported_formats(void)
{
    return formats;
}

static Context *wav_analyze_init(void)
{
    WavContext *context;

    context = malloc(sizeof(WavContext));
    memset(context, 0, sizeof(WavContext));
    sha_init(&context->audioSha1);

    return (Context *)context;
}

/*
  16      4 bytes  0x00000010     // Length of the fmt data (16 bytes)
  20      2 bytes  0x0001         // Format tag: 1 = PCM
  22      2 bytes  <channels>     // Channels: 1 = mono, 2 = stereo
  24      4 bytes  <sample rate>  // Samples per second: e.g., 44100
*/
static void wav_analyze_update(Context             *contextArg, 
                               const unsigned char *buf,
                               unsigned             bufLen)
{
    WavContext *context = (WavContext *)contextArg;

    /* if bytes processed is set to -1, then this is not a wav file
       and we should not continue to process it */
    if (context->bytesProcessed == -1)
       return;

    if (context->bytesProcessed == 0)
    {
        /* Check to make sure this is a WAV file */
        if (buf[0] != 'R' || buf[1] != 'I' || 
            buf[2] != 'F' || buf[3] != 'F' ||
            buf[8] != 'W' || buf[9] != 'A' || 
            buf[10] != 'V' || buf[11] != 'E' ||
            buf[12] != 'f' || buf[13] != 'm' || 
            buf[14] != 't' || buf[15] != ' ')
        {
           errorString = strdup("File is not in WAV format.");
           context->bytesProcessed = -1;
           return;
        }

        /* We're going to assume that we have the entire header in the
           first block */
        assert(bufLen >= 44);
       
        /* TODO: convert these to or from network order */
        context->channels = *((int16 *)&buf[22]); 
        context->sampleRate = *((int32 *)&buf[24]); 
        context->sampleSize = *((int16 *)&buf[34]); 
        context->dataLen = *((int32 *)&buf[40]); 

#if WORDS_BIGENDIAN
        context->channels = toLittleEndian16(context->channels);
        context->sampleSize = toLittleEndian16(context->sampleSize);
        context->sampleRate = toLittleEndian32(context->sampleRate);
        context->dataLen = toLittleEndian32(context->dataLen);
#endif

        /*
        printf(" samplesize: %d\n", context->sampleSize);
        printf("sample rate: %d\n", context->sampleRate);
        printf("   channels: %d\n", context->channels);
        printf("    datalen: %d\n", context->dataLen);
        printf("    samples: %u\n", context->samples);
        */

        if (context->sampleSize != 8 && context->sampleSize != 16)
        {
           context->bytesProcessed = -1;
           errorString = strdup("Invalid sample size found in wav file.");
           return;
        }

        context->samples = context->dataLen / 
                           (context->channels * (context->sampleSize >> 3));
       

        sha_update(&context->audioSha1, (unsigned char *)buf + 44, bufLen - 44);
        context->bytesProcessed += bufLen - 44;
    }
    else
    {
        sha_update(&context->audioSha1, (unsigned char *)buf, bufLen);
        context->bytesProcessed += bufLen;
    }
}

static Attribute *wav_analyze_final(Context *contextArg)
{
    WavContext    *context = (WavContext *)contextArg;
    unsigned char  hash[SHA_DIGESTSIZE];
    char           temp[100];
    Attribute     *attrList;

    /* If we determined that this file was illegal, just return */
    if (context->bytesProcessed == -1)
       return NULL;

    sha_final(hash, &context->audioSha1);

    attrList = malloc(sizeof(Attribute) * NUM_ATTRS);
    memset(attrList, 0, sizeof(Attribute) * NUM_ATTRS);

    /* Do a quick check to see which integer math calc is appropriate */
    if (context->samples < context->sampleRate)
       sprintf(temp, "%d", (context->samples * 1000 / context->sampleRate));
    else
       sprintf(temp, "%d", (context->samples / context->sampleRate) * 1000);
       
    attrList[0].key = strdup("tag.wav.duration");
    attrList[0].value = strdup(temp);

    sprintf(temp, "%d", context->sampleRate);
    attrList[1].key = strdup("tag.wav.samplerate");
    attrList[1].value = strdup(temp);

    sprintf(temp, "%d", context->channels);
    attrList[2].key = strdup("tag.wav.channels");
    attrList[2].value = strdup(temp); 

    sprintf(temp, "%d", context->sampleSize);
    attrList[3].key = strdup("tag.wav.samplesize");
    attrList[3].value = strdup(temp);

    convert_to_hex(hash, SHA_DIGESTSIZE, temp);
    attrList[4].key = strdup("tag.wav.audio_sha1");
    attrList[4].value = strdup(temp);

    return attrList;
}

static void wav_free_attributes(Attribute *attrList)
{
    int i;

    for(i = 0; i < NUM_ATTRS; i++)
    {
       if (attrList[i].key)
          free(attrList[i].key);
       if (attrList[i].value)
          free(attrList[i].value);
    }

    free(attrList);
}

static char *wav_get_error(void)
{
    return errorString;
}

static short toLittleEndian16(short in)
{
    char *ptr = (char *)&in;

    return ((ptr[1] & 0xFF) << 8) | (ptr[0] & 0xFF); 
}

static long toLittleEndian32(long in)
{
    char *ptr = (char *)&in;

    return ((ptr[3] & 0xFF) << 24) | ((ptr[2] & 0xFF) << 16) | ((ptr[1] & 0xFF) << 8) | (ptr[0] & 0xFF); 
}
