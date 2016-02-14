/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * This code is based on vorbis metadata plugin from FreeAmp. EMusic.com
 * has released this code into the Public Domain. 
 * (Thanks goes to Brett Thomas, VP Engineering Emusic.com)
 *
 * $Id: vorbis.c,v 1.10 2004/02/03 01:11:07 mayhemchaos Exp $
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <assert.h>

#include "vorbis/vorbisfile.h"
#include "plugin.h"
#include "vorbis.h"

/*-------------------------------------------------------------------------*/

#ifndef _WIN32
#define init_plugin vorbis_init_plugin
#endif

PluginMethods           *init_plugin(void);
static void              vorbis_shutdown_plugin(void);
static SupportedFormat  *vorbis_get_supported_formats(void);
static const char       *vorbis_get_name(void);
static const char       *vorbis_get_version(void);
static Attribute        *vorbis_file_analyze(const char *fileName);
static void              vorbis_free_attributes(Attribute *attrList);
static const char       *vorbis_get_error(void);

/* Static internal functions */
static char *      convertToISO(const char *utf8);
static VorbisInfo *read_vorbis_tag(const char *fileName);
static void        delete_vorbis_tag(VorbisInfo *info);

/*-------------------------------------------------------------------------*/

#define CHUNK          4096
#define PLUGIN_VERSION "1.0.0"
#define PLUGIN_NAME    "Ogg/Vorbis Metadata"
#define MAX_ATTRS      16

/*-------------------------------------------------------------------------*/

static SupportedFormat formats[] =
{ 
     { ".ogg", "Ogg/Vorbis" },
     { NULL,   NULL }
};

/*-------------------------------------------------------------------------*/

static PluginMethods methods = 
{
    vorbis_shutdown_plugin,
    vorbis_get_version,
    vorbis_get_name,
    vorbis_get_supported_formats,
    vorbis_file_analyze,
    NULL, /* mem_ functions are not supported in this plugin */
    NULL,
    NULL,
    vorbis_free_attributes,
    vorbis_get_error
};

static char *errorString = NULL;

/*-------------------------------------------------------------------------*/

PluginMethods *init_plugin(void)
{
    return &methods;
}

static void vorbis_shutdown_plugin(void)
{
    if (errorString)
       free(errorString);
}

static const char *vorbis_get_version(void)
{
    return PLUGIN_VERSION;
}

static const char *vorbis_get_name(void)
{
    return PLUGIN_NAME;
}

static SupportedFormat *vorbis_get_supported_formats(void)
{
    return formats;
}

static Attribute *vorbis_file_analyze(const char *fileName)
{
    VorbisInfo *info;
    Attribute  *attrList;
    int         i;
    char        temp[1024];

    info = read_vorbis_tag(fileName);
    if (info == NULL)
       return NULL;

    attrList = malloc(sizeof(Attribute) * MAX_ATTRS);
    memset(attrList, 0, sizeof(Attribute) * MAX_ATTRS);

    i = 0;
    sprintf(temp, "%d", info->bitrate);
    attrList[i].key = strdup("tag.vorbis.bitrate");
    attrList[i++].value = strdup(temp);

    sprintf(temp, "%d", info->duration);
    attrList[i].key = strdup("tag.vorbis.duration");
    attrList[i++].value = strdup(temp);

    sprintf(temp, "%d", info->samplerate);
    attrList[i].key = strdup("tag.vorbis.samplerate");
    attrList[i++].value = strdup(temp);

    sprintf(temp, "%d", info->channels);
    attrList[i].key = strdup("tag.vorbis.channels");
    attrList[i++].value = strdup(temp);

    if (info->title)
    {
        attrList[i].key = strdup("tag.audiotrack.title");
        attrList[i++].value = strdup(info->title);
    }

    if (info->artist)
    {
        attrList[i].key = strdup("tag.audiotrack.artist");
        attrList[i++].value = strdup(info->artist);
    }

    if (info->album)
    {
        attrList[i].key = strdup("tag.audiotrack.album");
        attrList[i++].value = strdup(info->album);
    }

    if (info->tracknumber)
    {
        attrList[i].key = strdup("tag.audiotrack.tracknumber");
        attrList[i++].value = strdup(info->tracknumber);
    }

    if (info->desc)
    {
        attrList[i].key = strdup("tag.objective.description");
        attrList[i++].value = strdup(info->desc);
    }

    if (info->genre)
    {
        attrList[i].key = strdup("tag.id3genre.genre");
        attrList[i++].value = strdup(info->genre);
    }

    delete_vorbis_tag(info);

    return attrList;
}

static void vorbis_free_attributes(Attribute *attrList)
{
    int i;

    for(i = 0; i < MAX_ATTRS; i++)
    {
       if (attrList[i].key)
          free(attrList[i].key);
       if (attrList[i].value)
          free(attrList[i].value);
    }

    free(attrList);
}

static const char *vorbis_get_error(void)
{
    return errorString;
}

static void delete_vorbis_tag(VorbisInfo *info)
{
    if (!info)
       return;

    if (info->artist)
       free(info->artist);
    if (info->album)
       free(info->album);
    if (info->title)
       free(info->title);
    if (info->genre)
       free(info->genre);
    if (info->tracknumber)
       free(info->tracknumber);
    if (info->desc)
       free(info->desc);

    free(info);
}

static VorbisInfo *read_vorbis_tag(const char *fileName)
{
    char           *temp;
    FILE           *file;
    OggVorbis_File  vf;
    vorbis_comment *comment; 
    vorbis_info    *vi; 
    VorbisInfo     *info;
    int             ret;
    ov_callbacks    callbacks;

    file = fopen(fileName, "rb");
    if (file == NULL)
       return NULL;

    callbacks.read_func = fread;
    callbacks.seek_func = fseek;
    callbacks.close_func = fclose;
    callbacks.tell_func = ftell;
    
    memset(&vf, 0, sizeof(vf));
    ret = ov_open_callbacks(file, &vf, NULL, 0, callbacks);
    if (ret < 0)
    {
       switch(ret)
       {
          case OV_EREAD: 
             errorString = strdup("A read from media returned an error.");
             break;
          case OV_ENOTVORBIS:
             errorString = strdup("Bitstream is not Vorbis data.");
             break;
          case OV_EVERSION:
             errorString = strdup("Vorbis version mismatch.");
             break;
          case OV_EBADHEADER:
             errorString = strdup("Invalid Vorbis bitstream header.");
             break;
          case OV_EFAULT:
             errorString = strdup("Internal logic fault; indicates a bug "
                                  "or heap/stack corruption. ");
             break;
          default:
             errorString = strdup("Unknown error."); 
             break;
       }
       fclose(file);
       return NULL;
    }

    info = malloc(sizeof(VorbisInfo));
    memset(info, 0, sizeof(VorbisInfo));

    info->bitrate = ov_bitrate(&vf, -1) / 1000;
    info->duration = (int)(ov_time_total(&vf, 0) * 1000);
    vi = ov_info(&vf, -1);
    info->channels = vi->channels;
    info->samplerate = vi->rate;

    comment = ov_comment(&vf, -1); 
    if (comment)
    {
        temp = vorbis_comment_query(comment, "title", 0);
        if (temp)
            info->title = convertToISO(temp);

        temp = vorbis_comment_query(comment, "artist", 0);
        if (temp)
            info->artist = convertToISO(temp);

        temp = vorbis_comment_query(comment, "album", 0);
        if (temp)
            info->album = convertToISO(temp);

        temp = vorbis_comment_query(comment, "tracknumber", 0);
        if (temp)
            info->tracknumber = convertToISO(temp);

        temp = vorbis_comment_query(comment, "genre", 0);
        if (temp)
            info->genre = convertToISO(temp);

        temp = vorbis_comment_query(comment, "description", 0);
        if (temp)
            info->desc = convertToISO(temp);
    }
    ov_clear(&vf);   

    return info;
}

static char *convertToISO(const char *utf8)
{
   unsigned char *in, *buf;
   unsigned char *out, *end;

   in = (unsigned char *)utf8;
   buf = out = malloc(strlen(utf8) + 1);
   end = in + strlen(utf8);
   for(;*in != 0x00 && in <= end; in++, out++)
   {
       if (*in < 0x80)
       {  /* lower 7-bits unchanged */
          *out = *in;
       }
       else
       if (*in > 0xC3)
       { /* discard anything above 0xFF */
          *out = '?';
       }
       else
       if (*in & 0xC0)
       { /* parse upper 7-bits */
          if (in >= end)
            *out = 0;
          else
          {
            *out = (((*in) & 0x1F) << 6) | (0x3F & (*(++in)));
          }
       }  
       else
       {
          *out = '?';  /* this should never happen */
       }
   }
   *out = 0x00; /* append null */

   return buf;
}
