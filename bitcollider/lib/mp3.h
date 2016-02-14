/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: mp3.h,v 1.4 2001/07/31 18:49:20 mayhemchaos Exp $
 */
#ifndef MP3_H
#define MP3_H

#include "sha1.h"

typedef struct _mp3_info
{
    int            bitrate;
    int            samplerate;
    int            stereo;
    int            duration;
    unsigned char  audioSha[SHA_DIGESTSIZE];
    int            frames;
    int            mpegVer;
    int            avgBitrate;

    // Private information -- do not use
    unsigned int   skipSize;
    unsigned char  spanningHeader[3];
    unsigned int   spanningSize;
    SHA_INFO       scontext;
    unsigned int   goodBytes, badBytes;
    unsigned char *startBuffer;
    unsigned int   startBytes;
    unsigned char *audioShaBuffer, audioShaExtra[3];
    unsigned int   audioShaBytes;
} mp3_info;

#define MP3_HEADER_SIZE 4

void mp3_init(mp3_info *info);
void mp3_final(mp3_info *info);
void mp3_update(mp3_info      *info,
                unsigned char *buffer, 
                unsigned       len);

#endif
