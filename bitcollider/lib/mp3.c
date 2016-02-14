/* (PD) 2001 The Bitzi Corporation
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * $Id: mp3.c,v 1.5 2001/07/31 18:49:20 mayhemchaos Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mp3.h"

#define DB printf("%s:%d\n", __FILE__, __LINE__);

static int mpeg1Bitrates[] = { 0, 32, 40, 48, 56, 64, 80, 96, 112, 
                               128, 160, 192, 224, 256, 320 };
static int mpeg2Bitrates[] = { 0, 8, 16, 24, 32, 40, 48, 56, 64, 
                               80, 96, 112, 128, 144, 160 };
static int mpeg1SampleRates[] = { 44100, 48000, 32000 };
static int mpeg2SampleRates[] = { 22050, 24000, 16000 };
static int mpegLayer[] = { 0, 3, 2, 1 };

static void update_audio_sha1(mp3_info      *info,
                              unsigned char *buf,
                              unsigned int   bufLen);
#define ID3_TAG_LEN 128

static int bitrate(const char *header)
{
   int id, br;

   id = (header[1] & 0x8) >> 3;
   br = (header[2] & 0xF0) >> 4;

   // TODO: Add range checking 
   return id ? mpeg1Bitrates[br] : mpeg2Bitrates[br];
}

static int samplerate(const char *header)
{
   int id, sr;

   id = (header[1] & 0x8) >> 3;
   sr = (header[2] >> 2) & 0x3;

   // TODO: Add range checking 
   return id ? mpeg1SampleRates[sr] : mpeg2SampleRates[sr];
}

static int stereo(const char *header)
{
   return ((header[3] & 0xc0) >> 6) != 3;
}

static int mpeg_ver(const char *header)
{
   return (!((header[1] & 0x8) >> 3) + 1); 
}

static int mpeg_layer(const char *header)
{
   return mpegLayer[((header[1] & 0x7) >> 1)]; 
}

static int padding(const char *header)
{
   return (header[2] >> 1) & 0x1;
}

int find_mp3_start(mp3_info      *info, 
                   unsigned char *buffer, 
                   unsigned int   len)
{
   unsigned char *max, *ptr;
   int            firstSampleRate, firstLayer, secondSampleRate, secondLayer;
   unsigned int   size, goodFrames = 0;
   int            goodFrameOffset = -1;

   if (info->startBuffer)
   {
      info->startBuffer = realloc(info->startBuffer, info->startBytes + len);
      memcpy(info->startBuffer + info->startBytes, buffer, len);
      info->startBytes += len;

      buffer = info->startBuffer;
      len = info->startBytes;
   }

   /* Loop through the buffer trying to find frames */
   for(ptr = buffer, max = buffer + len; ptr < max;)
   {
      /* Find the frame marker */
      if (*ptr != 0xFF || ((*(ptr + 1) & 0xF0) != 0xF0 &&
                           (*(ptr + 1) & 0xF0) != 0xE0)) 
      {
          ptr++;
          continue;
      }

      /* Extract sample rate and layer from this first frame */
      firstSampleRate = samplerate(ptr);
      firstLayer = mpeg_layer(ptr);

      /* Check for invalid sample rates */
      if (firstSampleRate == 0)
      { 
          ptr++;
          continue;
      }

      /* Calculate the size of the frame from the header components */
      if (mpeg_ver(ptr) == 1)
          size = (144000 * bitrate(ptr)) / samplerate(ptr) + padding(ptr);
      else
          size = (72000 * bitrate(ptr)) / samplerate(ptr) + padding(ptr);
      if (size <= 1 || size > 2048)
      {
          ptr++;
          continue;
      }

      if (ptr + size >= max)
      {
         if (info->startBuffer == NULL)
         {
             info->startBytes = len;
             info->startBuffer = malloc(len);
             memcpy(info->startBuffer, buffer, len);
         }
         return -1; 
      }

      /* now we have what seems to be a valid size. Let's see if there
         is a new frame with the right layer and sample rate right after
         this potential frame */
      secondSampleRate = samplerate(ptr + size);
      secondLayer = mpeg_layer(ptr + size);

      /*
      printf("Size: %d\n", size);
      printf("Offset: %X\n", (int)ptr - (int)buffer);
      printf("Bytes: %02X %02X %02X %02X\n", ptr[0], ptr[1], ptr[2], ptr[3]); 
      printf("fs: %d ss: %d\n", firstSampleRate, secondSampleRate);
      printf("fl: %d sl: %d\n\n", firstLayer, secondLayer);
      */

      if (firstSampleRate == secondSampleRate && firstLayer == secondLayer)
      {
          goodFrames++;
          if (goodFrameOffset < 0)
             goodFrameOffset = (int)ptr - (int)buffer;
          ptr += size;
      }
      else
      {
          goodFrames = 0;

          if (goodFrameOffset >= 0)
             ptr = buffer + goodFrameOffset + 1;
          else
             ptr++;
          goodFrameOffset = -1;
      }

      if (goodFrames == 3)
      {
          return (int)goodFrameOffset;
      }
   }

   return -1;
}

void mp3_init(mp3_info *info)
{
   memset(info, 0, sizeof(mp3_info));
   info->scontext.digest[0] = 0x45;
   sha_init(&info->scontext);
}

void mp3_final(mp3_info *info)
{
   unsigned char *oldShaBuffer;

   if (info->startBuffer)
      free(info->startBuffer);

   /* Save the audiosha buffer... */
   oldShaBuffer = info->audioShaBuffer;

   /* If there are more bad bytes in a file, than there are good bytes,
      assume that the file is not an MP3 file and zero out all the values
      we've collected. Unfortunately there is no good way to detecting
      whether or not a file really is an MP3 file */
   if (info->badBytes > info->goodBytes || info->goodBytes == 0)
   {
       memset(info, 0, sizeof(mp3_info));
   }
   else
   {
       if (info->audioShaBuffer)
       {
          char *tag;
          int   i;

          /* Copy the last three characters from after the last audioSha
             block to the end of the sliding window. Then, look for the
             TAG and skip it if it was found. */
          memcpy(info->audioShaBuffer + ID3_TAG_LEN, info->audioShaExtra, 3);
          for(tag = info->audioShaBuffer, i = 0; i < ID3_TAG_LEN+3; i++, tag++)
          {
             if (strncmp(tag, "TAG", 3) == 0)
                break;
          }

          if (i > ID3_TAG_LEN)
              i = ID3_TAG_LEN;
          sha_update(&info->scontext, info->audioShaBuffer, i);
       }

       sha_final(info->audioSha, &info->scontext);

       if (info->mpegVer == 1)
          info->duration = info->frames * 1152 / (info->samplerate / 1000);
       else
          info->duration = info->frames * 576 / (info->samplerate / 1000);
       info->avgBitrate /= info->frames;
   }

   if (oldShaBuffer)
      free(oldShaBuffer);

   /*
   printf("du: %d\n", info->duration);
   printf("br: %d\n", info->bitrate);
   printf("sr: %d\n", info->samplerate);
   printf("fr: %d\n", info->frames);
   printf("vr: %d\n", info->mpegVer);
   printf("st: %d\n", info->stereo);
   */
}


void mp3_update(mp3_info      *info,
                unsigned char *buffer, 
                unsigned       len)
{
   unsigned       size, bytesLeft;
   unsigned char *ptr, *max;
   unsigned char *temp = NULL;

   /* If this is the first time in the update function, then seek to
      find the actual start of the mp3 and skip over any ID3 tags or garbage
      that might be at the beginning of the file */
   if (info->badBytes == 0 && info->goodBytes == 0)
   {
      int offset;

      offset = find_mp3_start(info, buffer, len);
      if (offset < 0)
         return;

      /* If it took more than one block to determine the start of the mp3
         file, then use the buffer that was created by the find_mp3_start
         routine, rather than the buffer that was passed in. */
      if (info->startBuffer)
      {
         buffer = info->startBuffer;
         len = info->startBytes;
      }

      /* Skip over the crap at the beginning of the file */
      buffer += offset;
      len -= offset;
      size = 0;
   }

   /* If the a header spanned the last block and this block, then
      allocate a larger buffer and copy the last header plus the new
      block into the new buffer and work on it. This shouldn't happen
      very often. */
   if (info->spanningSize > 0)
   {
      temp = malloc(len + info->spanningSize);
      memcpy(temp, info->spanningHeader, info->spanningSize);
      memcpy(temp + info->spanningSize, buffer, len);
      len += info->spanningSize;
      buffer = temp;
   }

   /* Pass the bytes we're skipping through the sha function */
   update_audio_sha1(info, buffer, info->skipSize);

   /* Save the three bytes immediately following the last audio sha
      block for later. These bytes will be used to check for ID3
      tags at the end of truncated audio frames. See mp3_final for
      more details. */
   memcpy(info->audioShaExtra, buffer + info->skipSize, 3);

   /* Loop through the buffer trying to find frames */
   for(ptr = buffer + info->skipSize, max = buffer + len;
       ptr < max;)
   {
      /* printf("%02X%02X\n", ptr[0], ptr[1]); */
      if ((unsigned int)max - (unsigned int)ptr < 4)
      {
         /* If we have a header that spans a block boundary, save
            up to 3 bytes and then return */
         info->spanningSize = (unsigned int)max - (unsigned int)ptr;
         memcpy(info->spanningHeader, ptr, info->spanningSize);
         info->skipSize = 0;

         if (temp)
            free(temp);

         return;
      }
 
      /* Find the frame marker */
      if (*ptr != 0xFF || ((*(ptr + 1) & 0xF0) != 0xF0 &&
                           (*(ptr + 1) & 0xF0) != 0xE0)) 
      {
          info->badBytes ++;
          ptr++;
          continue;
      }

      /* Check for invalid sample rates */
      if (samplerate(ptr) == 0)
      { 
          info->badBytes ++;
          ptr++;
          continue;
      }

      /* Calculate the size of the frame from the header components */
      if (mpeg_ver(ptr) == 1)
          size = (144000 * bitrate(ptr)) / samplerate(ptr) + padding(ptr);
      else
          size = (72000 * bitrate(ptr)) / samplerate(ptr) + padding(ptr);
      if (size <= 1 || size > 2048)
      {
          info->badBytes ++;
          ptr++;
          continue;
      }

      /* If this is the first frame, then tuck away important info */
      if (info->frames == 0)
      {
          info->samplerate = samplerate(ptr);
          info->bitrate = bitrate(ptr);
          info->mpegVer = mpeg_ver(ptr);
          info->stereo = stereo(ptr);
      }
      else
      {
          /* The sample rate inside of a file should never change. If the
             header says it did, then assume that we found a bad header 
             and skip past it. */
          if (info->samplerate != samplerate(ptr))
          {
             info->badBytes ++;
             ptr++;
             continue;
          }

          /* If the bitrate in subsequent frames is different from the
             first frame, then we have a VBR file */
          if (info->bitrate && info->bitrate != bitrate(ptr))
          {
             info->bitrate = 0;
          }
      }

      /*
      printf("%08x: [%04d] %3d %5d %d %d\n", 
              (unsigned int)ptr - (unsigned int)buffer,
              info->frames, bitrate(ptr), 
              samplerate(ptr), size, padding(ptr)); 
      */

      /* Update the sha hash with the data from this frame */
      bytesLeft = (unsigned int)max - (unsigned int)ptr;
      update_audio_sha1(info, ptr, 
                        (size > bytesLeft) ? bytesLeft : size);

      /* save the first three bytes after the audio sha block (see above) */
      memcpy(info->audioShaExtra, 
             ptr + ((size > bytesLeft) ? bytesLeft : size), 3);

      /* Move the memory pointer past the frame */
      info->frames++;
      info->goodBytes += size;
      info->avgBitrate += bitrate(ptr);
      ptr += size;
   }

   /* skipSize defines the number of bytes to skip in the next block,
      so that we're not searching for the frame marker inside of
      a frame, which can lead to false hits. Grrr. 
      Vielen Dank, Karl-Heinz Brandenburg! */
   info->skipSize = (unsigned int)ptr - (unsigned int)max;
   info->spanningSize = 0;
   if (temp)
      free(temp);
}

static void update_audio_sha1(mp3_info *info,
                              unsigned  char *buf,
                              unsigned  int bufLen)
{
    /* Allocate the space for the audiosha sliding window. Allocate three
       extra bytes to allow for the possibility that the ID3 tag spans
       the outer boundary of the audiosha sliding window */
    if (info->audioShaBuffer == NULL)
        info->audioShaBuffer = malloc(ID3_TAG_LEN + 3);

    /* Save the last 128 bytes of the given buffer and audio sha all the
       bytes passed through the sliding window */
    if (bufLen + info->audioShaBytes > ID3_TAG_LEN)
    {
        if (bufLen >= ID3_TAG_LEN)
        {
            sha_update(&info->scontext, info->audioShaBuffer, 
                       info->audioShaBytes);
            sha_update(&info->scontext, buf, bufLen - ID3_TAG_LEN);
            memcpy(info->audioShaBuffer, buf + (bufLen - ID3_TAG_LEN), 
                   ID3_TAG_LEN);
            info->audioShaBytes = ID3_TAG_LEN;
        }
        else
        {
            unsigned bytesToRemove;

            bytesToRemove = info->audioShaBytes + bufLen - ID3_TAG_LEN;
            sha_update(&info->scontext, info->audioShaBuffer, bytesToRemove);
            memmove(info->audioShaBuffer, info->audioShaBuffer + bytesToRemove, 
                    info->audioShaBytes - bytesToRemove);
            memcpy(info->audioShaBuffer + info->audioShaBytes - bytesToRemove, 
                   buf, bufLen);
            info->audioShaBytes = info->audioShaBytes - bytesToRemove + bufLen;
        }
    }
    else
    {
        memcpy(info->audioShaBuffer + info->audioShaBytes, buf, bufLen);
        info->audioShaBytes += bufLen;
    }
}
