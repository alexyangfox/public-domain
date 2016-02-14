/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: vorbis.h,v 1.1 2001/03/16 04:57:19 mayhemchaos Exp $
 */
#ifndef VORBIS_H
#define VORBIS_H

typedef struct _VorbisInfo
{
    unsigned  bitrate;
    unsigned  duration;
    unsigned  samplerate;
    unsigned  channels;
    char     *artist;
    char     *album;
    char     *title;
    char     *genre;
    char     *tracknumber;
    char     *desc;
} VorbisInfo;

#endif
