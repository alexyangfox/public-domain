/* MPEG-1/MPEG-2 parsing module for the Bitzi Bitcollider video plugin
 *
 * (PD) 2002 Mark Nelson [delirium] <delirium-bitzi@rufus.d2g.com>
 * Please see file COPYING or http://bitzi.com/publicdomain for more
 * information.
 *
 * The primary references used for the MPEG-1/2 stream formats were:
 *     http://www.andrewduncan.ws/MPEG/MPEG-1_Picts.html and
 *     http://www.andrewduncan.ws/MPEG/MPEG-2_Picts.html
 */

#include "video.h"

/* Returns 1 or 2 to indicate MPEG-1 or MPEG-2
 *
 * Most MPEG data is stored in bits not necessarily aligned on byte boundaries;
 * bits are ordered most-significant first, so big-endian of a sort.
 * Block sizes only count bytes after the block size integer.
 */
int parse_mpeg(FILE *file, Data *data)
{
   int version = 0;			/* MPEG-1/2; our return value */
   uint32 temp;

   /* First check if this is a Program stream (multiplexed audio/video),
    * and handle Pack header if so */
   temp = fread_be(file, 4);
   if(temp == 0x000001BA)
   {
      /* Figure out if this is an MPEG-1 or MPEG-2 program */
      temp = (uint32) fgetc(file);
      if((temp & 0xF0) == 0x20)		/* binary 0010 xxxx */
	 version = 1;
      else if((temp & 0xC0) == 0x40)	/* binary 01xx xxxx */
 	 version = 2;
      else
	 return 0;

      if(version == 1)
      {
	 fseek(file, 4L, SEEK_CUR);
	 data->bitrate = (unsigned int)
	    round_double((double)((fread_be(file, 3) & 0x7FFFFE) >> 1) * 0.4);
      }
      else
      {
	 fseek(file, 5L, SEEK_CUR);
	 data->bitrate = (unsigned int)
	    round_double((double)((fread_be(file, 3) & 0xFFFFFC) >> 2) * 0.4);

	 temp = fgetc(file) & 0x07;	/* stuffing bytes */
	 if(temp != 0)
	    fseek(file, (long) temp, SEEK_CUR);
      }
      /* Skip any other blocks we find until we get to a video stream, which
       * might be within a 2nd PACK */
      temp = fread_be(file, 4);
      while(temp != 0x000001BA && temp != 0x000001E0)
      {
	 if(feof(file))			/* shouldn't happen */
	    return version;
	 if(temp == 0x00000000)		/* Skip past zero padding */
	 {
	    while((temp & 0xFFFFFF00) != 0x00000100)
	    {
	       if(feof(file))
		  return version;	/* shouldn't happen here either */
	       temp <<= 8;
	       temp |= fgetc(file);
	    }
	 }
	 else
	 {
	    temp = fread_be(file, 2);
	    fseek(file, (long) temp, SEEK_CUR);
	    temp = fread_be(file, 4);
	 }
      }

      /* Now read byte by byte until we find the 0x000001B3 instead of actually
       * parsing (due to too many variations).  Theoretically this could mean
       * we find 0x000001B3 as data inside another packet, but that's extremely
       * unlikely, especially since the sequence header should not be far */
      temp = fread_be(file, 4);
      while(temp != 0x000001B3)
      {
	 if(feof(file))			/* No seq. header; shouldn't happen */
	    return version;
	 temp <<= 8;
	 temp |= fgetc(file);
      }
   }
   else					/* video stream only */
      fseek(file, 4L, SEEK_SET);

   /* Now we're just past the video sequence header start code */

   temp = fread_be(file, 3);
   data->width = (temp & 0xFFF000) >> 12;
   data->height = temp & 0x000FFF;

   switch(fgetc(file) & 0x0F)
   {
   case 1:				/* 23.976 fps */
   case 2:				/* 24 fps */
      data->fps = 24;
      break;
   case 3:				/* 25 fps */
      data->fps = 25;
      break;
   case 4:				/* 29.97 fps */
   case 5:				/* 30 fps */
      data->fps = 30;
      break;
   case 6:				/* 50 fps */
      data->fps = 50;
      break;
   case 7:				/* 59.94 fps */
   case 8:				/* 60 fps */
      data->fps = 60;
      break;
   }

   if(data->bitrate == 0)		/* if this is a video-only stream, */
   {					/* get bitrate from here */
      temp = (fread_be(file, 3) & 0xFFFFC0) >> 6;
      if(temp != 0x3FFFF)		/* variable bitrate */
	 data->bitrate = (unsigned int)
	    round_double((double) temp * 0.4);
   }
   else
      fseek(file, 3L, SEEK_CUR);

   /* If MPEG-2 or don't know yet, look for the sequence header extension */
   if(version != 1)
   {
      /* Skip past rest of sequence header and 64-byte matrices (if any) */
      temp = fgetc(file);
      if(temp & 0x02)
      {
	 fseek(file, 63L, SEEK_CUR);
	 temp = fgetc(file);
      }
      if(temp & 0x01)
	 fseek(file, 64L, SEEK_CUR);

      temp = fread_be(file, 4);
      if(temp == 0x000001B5)
      {
	 if(version == 0)
	    version = 2;

	 fseek(file, 1L, SEEK_CUR);
	 /* extensions specify MSBs of width/height */
	 temp = fread_be(file, 2);
	 data->width  |= (temp & 0x0180) << 5;
	 data->height |= (temp & 0x0060) << 7;

	 fseek(file, 2L, SEEK_CUR);
	 /* and a numerator/denominator multiplier for fps */
	 temp = fgetc(file);
	 if((temp & 0x60) && (temp & 0x1F))
	    data->fps = (unsigned int)
	       round_double((double)data->fps * (temp & 0x60)/(temp & 0x1F)); 
      }
      else if(version == 0)
	 version = 1;
   }

   return version;
}
