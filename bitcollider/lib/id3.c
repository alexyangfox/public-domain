/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * This code is based on id3v1.cpp and id3v2.cpp from FreeAmp. EMusic.com
 * has released this code into the Public Domain. 
 * (Thanks goes to Brett Thomas, VP Engineering Emusic.com)
 *
 * $Id: id3.c,v 1.8 2001/08/01 21:21:56 mayhemchaos Exp $
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


#include "id3.h"
#include "bitcollider.h"

#define DB printf("%s:%d\n", __FILE__, __LINE__);

/* This version 2.2 handling is not completely up to spec -- this code
   was added to handle parsing of Apples iTunes id3v2.2 tags, which
   are not id3v2.2 compliant. I haven't seen any other programs use
   the old id3v2.2 stuff, so this code is tweaked to make sure that
   iTunes files can be parsed. Yuck. 

   The 2.3 support is up to snuff and should work on all 2.2 compliant
   id3v2 tags.
*/
const int supportedVersion_v2_2 = 2;
const int supportedVersion_v2_3 = 3;
const unsigned frameHeaderSize_v2_3 = 10;
const unsigned frameHeaderSize_v2_2 = 6;

ID3Info *read_ID3v1_tag(const char* fileName, ID3Info *info);
ID3Info *read_ID3v2_tag(const char* fileName);

typedef struct _ID3Header
{
   char          tag[3];
   unsigned char versionMajor;
   unsigned char versionRevision;
   unsigned char flags;
   unsigned char size[4];
} ID3Header;
typedef struct _FrameHeader_v2_3
{
   char           tag[4];
   unsigned int   size;
   unsigned short flags;
} FrameHeader_v2_3;
typedef struct _FrameHeader_v2_2
{
   char           tag[3];
   unsigned char  size[3];
} FrameHeader_v2_2;

typedef struct id3v1_0 
{
    char id[3];
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    char comment[30];
    unsigned char genre;

} id3v1_0;

typedef struct id3v1_1 
{
    char id[3];
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    char comment[28];
    char zero;
    char track;
    unsigned char genre;
} id3v1_1;

typedef struct id3v1 {
    union {
        struct id3v1_0 v1_0;
        struct id3v1_1 v1_1;
    } id3;
} id3v1;

#define v1_0 id3.v1_0
#define v1_1 id3.v1_1

static char *genreList[] =
{
     "Blues",
     "Classic Rock",
     "Country",
     "Dance",
     "Disco",
     "Funk",
     "Grunge",
     "Hip-Hop",
     "Jazz",
     "Metal",
     "New Age",
     "Oldies",
     "Other",
     "Pop",
     "R&B",
     "Rap",
     "Reggae",
     "Rock",
     "Techno",
     "Industrial",
     "Alternative",
     "Ska",
     "Death Metal",
     "Pranks",
     "Soundtrack",
     "Euro-Techno",
     "Ambient",
     "Trip-Hop",
     "Vocal",
     "Jazz+Funk",
     "Fusion",
     "Trance",
     "Classical",
     "Instrumental",
     "Acid",
     "House",
     "Game",
     "Sound Clip",
     "Gospel",
     "Noise",
     "AlternRock",
     "Bass",
     "Soul",
     "Punk",
     "Space",
     "Meditative",
     "Instrumental Pop",
     "Instrumental Rock",
     "Ethnic",
     "Gothic",
     "Darkwave",
     "Techno-Industrial",
     "Electronic",
     "Pop-Folk",
     "Eurodance",
     "Dream",
     "Southern Rock",
     "Comedy",
     "Cult",
     "Gangsta",
     "Top 40",
     "Christian Rap",
     "Pop/Funk",
     "Jungle",
     "Native American",
     "Cabaret",
     "New Wave",
     "Psychadelic",
     "Rave",
     "Showtunes",
     "Trailer",
     "Lo-Fi",
     "Tribal",
     "Acid Punk",
     "Acid Jazz",
     "Polka",
     "Retro",
     "Musical",
     "Rock & Roll",
     "Hard Rock",
     "Folk",
     "Folk-Rock",
     "National Folk",
     "Swing",
     "Fast Fusion",
     "Bebob",
     "Latin",
     "Revival",
     "Celtic",
     "Bluegrass",
     "Avantgarde",
     "Gothic Rock",
     "Progressive Rock",
     "Psychedelic Rock",
     "Symphonic Rock",
     "Slow Rock",
     "Big Band",
     "Chorus",
     "Easy Listening",
     "Acoustic",
     "Humour",
     "Speech",
     "Chanson",
     "Opera",
     "Chamber Music",
     "Sonata",
     "Symphony",
     "Booty Bass",
     "Primus",
     "Porn Groove",
     "Satire",
     "Slow Jam",
     "Club",
     "Tango",
     "Samba",
     "Folklore",
     "Ballad",
     "Power Ballad",
     "Rhythmic Soul",
     "Freestyle",
     "Duet",
     "Punk Rock",
     "Drum Solo",
     "Acapella",
     "Euro-House",
     "Dance Hall",
     "Goa",
     "Drum & Bass",
     "Club-House",
     "Hardcore",
     "Terror",
     "Indie",
     "BritPop",
     "Negerpunk",
     "Polsk Punk",
     "Beat",
     "Christian Gangsta",
     "Heavy Metal",
     "Black Metal",
     "Crossover",
     "Contemporary C",
     "Christian Rock",
     "Merengue",
     "Salsa",
     "Thrash Metal",
     "Anime",
     "JPop",
     "SynthPop",
     "\0"
};

void delete_ID3_tag(ID3Info *info)
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
    if (info->year)
       free(info->year);
    if (info->encoder)
       free(info->encoder);
    if (info->tracknumber)
       free(info->tracknumber);

    free(info);
}

void handle_frame_v2_3(char *tag, char *frameData, ID3Info *info)
{
    char tagName[5];

    if (frameData == NULL || strlen(frameData) == 0)
        return;

    strncpy(tagName, tag, 4);
    tagName[4] = 0;

    if (strcmp(tagName, "TIT2") == 0)
        info->title = strdup(frameData);

    if (strcmp(tagName, "TALB") == 0)
        info->album = strdup(frameData);

    if (strcmp(tagName, "TPE1") == 0)
        info->artist = strdup(frameData);

    if (strcmp(tagName, "TYER") == 0)
        info->year = strdup(frameData);

    if (strcmp(tagName, "TCON") == 0)
    {
        int i;

        for(i = 0;; i++)
        {
           if (*genreList[i] == 0)
              break;

           if (strcasecmp(genreList[i], frameData) == 0)
           {
              info->genre = malloc(10);
              sprintf(info->genre, "%d", i);
           }
        }
    }

    if (strcmp(tagName, "TRCK") == 0)
        info->tracknumber = strdup(frameData);

    if (strcmp(tagName, "TSSE") == 0)
        info->encoder = strdup(frameData);
}

void handle_frame_v2_2(char *tag, char *frameData, ID3Info *info)
{
    char tagName[5];

    if (frameData == NULL || strlen(frameData) == 0)
        return;

    strncpy(tagName, tag, 3);
    tagName[3] = 0;

    if (strcmp(tagName, "TT2") == 0)
        info->title = strdup(frameData);

    if (strcmp(tagName, "TAL") == 0)
        info->album = strdup(frameData);

    if (strcmp(tagName, "TP1") == 0)
        info->artist = strdup(frameData);

    if (strcmp(tagName, "TYE") == 0)
        info->year = strdup(frameData);

    if (strcmp(tagName, "TSI") == 0)
        info->genre = strdup(frameData);

    if (strcmp(tagName, "TRK") == 0)
    {
        info->tracknumber = strdup(frameData);
        sscanf(frameData, "%[0-9]", info->tracknumber);
    }

    if (strcmp(tagName, "TSS") == 0)
        info->encoder = strdup(frameData);
}

ID3Info *read_ID3_tag(const char *fileName)
{
    return read_ID3v1_tag(fileName, read_ID3v2_tag(fileName));
}

ID3Info *read_ID3v2_tag(const char* fileName)
{
    FILE            *inFile;
    char             buffer[1024], *frameData;
    ID3Header        head;
    FrameHeader_v2_3 frame_v2_3;
    FrameHeader_v2_2 frame_v2_2;
    ID3Info         *info = NULL;
    int              ret;
    unsigned int     size, frameSize = 0, fileSize = 0;

    inFile = fopen(fileName, "rb");
    if (inFile == NULL)
        return NULL;

	ret = fseek(inFile, 0, SEEK_END);
	fileSize = ftell(inFile);
	fseek(inFile, 0, SEEK_SET);

    ret = fread(&head, 1, sizeof(ID3Header), inFile);
    if (ret != sizeof(ID3Header))
    {
        fclose(inFile);
        return NULL;
    }

    if (strncmp(head.tag, "ID3", 3))
    {
        fclose(inFile);
        return NULL;
    }

    if (head.versionMajor != supportedVersion_v2_2 &&
        head.versionMajor != supportedVersion_v2_3)
    {
        fclose(inFile);
        return NULL;
    }
    size = ( head.size[3] & 0x7F       ) |
           ((head.size[2] & 0x7F) << 7 ) |
           ((head.size[1] & 0x7F) << 14) |
           ((head.size[0] & 0x7F) << 21);

	// Check to make sure that the size we calculate are sane!
	if (size > fileSize)
	{
        fclose(inFile);
        return NULL;
	}

    if (head.flags & (1 << 6))
    {
        unsigned extHeaderSize;

        if (fread(&extHeaderSize, 1, sizeof(int), inFile) != sizeof(int))
        {
            fclose(inFile);
            return NULL;
        }
        if (fread(buffer, 1, extHeaderSize, inFile) != extHeaderSize)
        {
            fclose(inFile);
            return NULL;
        }
    }

    info = malloc(sizeof(ID3Info));
    memset(info, 0, sizeof(ID3Info));
    for(; size > 0;)
    {
        if (head.versionMajor == supportedVersion_v2_2)
        {
            if (fread(&frame_v2_2, 1, frameHeaderSize_v2_2, inFile) != 
                frameHeaderSize_v2_2)
            {
                free(info);
                fclose(inFile);
                return NULL;
            }
            ((unsigned char *)&frameSize)[0] = 0;
            ((unsigned char *)&frameSize)[1] = frame_v2_2.size[0];
            ((unsigned char *)&frameSize)[2] = frame_v2_2.size[1];
            ((unsigned char *)&frameSize)[3] = frame_v2_2.size[2];
            frameSize = ntohl(frameSize);
        }
        if (head.versionMajor == supportedVersion_v2_3)
        {
            if (fread(&frame_v2_3, 1, frameHeaderSize_v2_3, inFile) != 
                frameHeaderSize_v2_3)
            {
                free(info);
                fclose(inFile);
                return NULL;
            }
            frameSize = ntohl(frame_v2_3.size);
        }

		// If the frame size is funky, skip it and move on
        if (frameSize == 0 || frameSize > fileSize)
            break;

        frameData = malloc(frameSize + 1);
        if (fread(frameData, 1, frameSize, inFile) != frameSize)
        {
            free(info);
            free(frameData);
            fclose(inFile);
            return NULL;
        }
        frameData[frameSize] = 0;
        if (head.versionMajor == supportedVersion_v2_2)
            handle_frame_v2_2(frame_v2_2.tag, &frameData[1], info);
        else
            handle_frame_v2_3(frame_v2_3.tag, &frameData[1], info);

        free(frameData);
        size -= (head.versionMajor == supportedVersion_v2_3 ? 
                frameHeaderSize_v2_3 : frameHeaderSize_v2_2) + frameSize;
    }

    fclose(inFile);

    return info;
}

void remove_trailing_spaces(char* string)
{
	char* cp = &(string[strlen(string)]);

	do 
    {
	    *cp = '\0';
	    cp--;
	}while ((*cp == ' ') && (cp >= string));
}

ID3Info *read_ID3v1_tag(const char* fileName, ID3Info *info)
{
    id3v1  id3;
    FILE  *fp;
    char   buffer[31];

    fp = fopen(fileName, "rb");
    if (fp == NULL)
        return info;

    if (fseek(fp, -128, SEEK_END))
    {
        fclose(fp);
        return info;
    }
                    
    if (fread(&id3, 1, 128, fp) != 128)
    {
        fclose(fp);
        return info;
    }

    if(strncmp(id3.v1_0.id, "TAG", 3))
    {
        fclose(fp);
        return info;
    }
         
    if (info == NULL)
    {
        info = malloc(sizeof(ID3Info));
        memset(info, 0, sizeof(ID3Info));
    }

    strncpy(buffer, id3.v1_0.artist, 30);
    buffer[30] = 0;
    remove_trailing_spaces(buffer);
    if (strlen(buffer) && info->artist == NULL)
        info->artist = strdup(buffer);

    strncpy(buffer, id3.v1_0.album, 30);
    buffer[30] = 0;
    remove_trailing_spaces(buffer);
    if (strlen(buffer) && info->album == NULL)
        info->album = strdup(buffer);

    strncpy(buffer, id3.v1_0.title, 30);
    buffer[30] = 0;
    remove_trailing_spaces(buffer);
    if (strlen(buffer) && info->title == NULL)
        info->title = strdup(buffer);

    strncpy(buffer, id3.v1_0.year,4);
    buffer[4] = 0;
    remove_trailing_spaces(buffer);
    if (strlen(buffer) && info->year == NULL)
    {
        int check;

        if (sscanf(buffer, "%d", &check) == 1 && check >= 1000 && check < 3000)
           info->year = strdup(buffer);
    }

    if( id3.v1_1.zero == 0x00 && id3.v1_1.track != 0x00)
    {
        sprintf(buffer, "%d", id3.v1_1.track);
        if (strlen(buffer) && info->tracknumber == NULL)
            info->tracknumber = strdup(buffer);
    }

    if (id3.v1_0.genre != 255)
    {
        sprintf(buffer, "%u", (unsigned int)id3.v1_0.genre);
        if (strlen(buffer) && info->genre == NULL)
           info->genre = strdup(buffer);
    }

    fclose(fp);
    return info;
}

