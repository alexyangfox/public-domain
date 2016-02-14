/* AVI parsing module for the Bitzi Bitcollider video plugin
 *
 * (PD) 2002 Mark Nelson [delirium] <delirium-bitzi@rufus.d2g.com>
 * Please see file COPYING or http://bitzi.com/publicdomain for more
 * information.
 *
 * The primary reference used for the AVI file format was the file format
 * section of John McGowan's AVI Overview:
 *     http://www.jmcgowan.com/avitech.html#Format
 * The full AVI Overview is available at:
 *     http://www.jmcgowan.com/avi.html
 */

#include "video.h"

/* AVI uses little-endian ordering, and block sizes count only bytes after
 * the block size integer.
 */
void parse_avi(FILE *file, Data *data)
{
   char fourcc[5];			/* Buffer in which to store fourccs */
   unsigned blockLen;			/* Length of the current block */

   fseek(file, 12L, SEEK_SET);		/* We've already checked signature */

   /* Verify existence of and read length of AVI header:
    * "LIST____hdrlavih____"
    * where the first ____ is the length of the LIST block
    */
   fread(fourcc, sizeof(char), 4, file);
   if(memcmp(fourcc, "LIST", 4)!=0)
      return;
   fseek(file, 4L, SEEK_CUR);
   fread(fourcc, sizeof(char), 4, file);
   if(memcmp(fourcc, "hdrl", 4)!=0)
      return;
   fread(fourcc, sizeof(char), 4, file);
   if(memcmp(fourcc, "avih", 4)!=0)
      return;
   blockLen = fread_le(file, 4);

   /* Now we're at the start of the AVI header */

   /* 0: microseconds per frame (4 bytes) */
   data->fps = (unsigned int) round_double((double) 1.0e6 / fread_le(file, 4));

   fseek(file, 12L, SEEK_CUR);

   /* 16: total frames (4 bytes) */
   data->duration = (unsigned int) round_double((double) fread_le(file, 4)
						* 1000 / data->fps);

   fseek(file, 12L, SEEK_CUR);

   /* 32: width (4 bytes) */
   data->width = (unsigned int) fread_le(file, 4);

   /* 36: height (4 bytes) */
   data->height = (unsigned int) fread_le(file, 4);

   /* Skip rest of avi header */
   fseek(file, (long) blockLen - 40, SEEK_CUR);

   /* Verify existence of and read length of video stream header:
    * "LIST____strlstrh____vids"
    */
   fread(fourcc, sizeof(char), 4, file);
   if(memcmp(fourcc, "LIST", 4)!=0)
      return;
   blockLen = fread_le(file, 4);
   fread(fourcc, sizeof(char), 4, file);
   if(memcmp(fourcc, "strl", 4)!=0)
      return;
   fread(fourcc, sizeof(char), 4, file);
   if(memcmp(fourcc, "strh", 4)!=0)
      return;
   fseek(file, 4L, SEEK_CUR);
   fread(fourcc, sizeof(char), 4, file);
   if(memcmp(fourcc, "vids", 4)!=0)
      return;

   /* Now we're in the video stream header */

   /* 16: FOURCC of video codec (4 bytes)*/
   fread(fourcc, sizeof(char), 4, file);
   fourcc[4] = '\0';
   data->codec = strdup(fourcc);

   /* Skip rest of video stream header */
   fseek(file, (long) blockLen - 20, SEEK_CUR);

   /* Verify existence of audio stream header:
    * "LIST____strlstrh____auds"
    * Note: audio stream header is optional
    */

   /* This is commented out since we're not reading audio data anyway (yet) */
/*
   fread(fourcc, sizeof(char), 4, file);
   if(memcmp(fourcc, "LIST", 4)!=0)
      return;
   fseek(file, 4L, SEEK_CUR);
   fread(fourcc, sizeof(char), 4, file);
   if(memcmp(fourcc, "strl", 4)!=0)
      return;
   fread(fourcc, sizeof(char), 4, file);
   if(memcmp(fourcc, "strh", 4)!=0)
      return;
   fseek(file, 4L, SEEK_CUR);
   fread(fourcc, sizeof(char), 4, file);
   if(memcmp(fourcc, "auds", 4)!=0)
      return;
*/

   /* Now we're in the audio stream header */

   /* TODO: extract some information about audio stream encoding */

}
