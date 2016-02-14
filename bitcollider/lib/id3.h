/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: id3.h,v 1.1 2001/02/28 23:29:53 mayhemchaos Exp $
 */
#ifndef ID3_H
#define ID3_H

typedef struct _ID3Info
{
    char *artist;
    char *album;
    char *title;
    char *genre;
    char *year;
    char *encoder;
    char *tracknumber;
} ID3Info;

ID3Info *read_ID3_tag(const char* fileName);
void     delete_ID3_tag(ID3Info *info);

#endif
