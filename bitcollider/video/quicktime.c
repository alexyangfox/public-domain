/* QuickTime parsing module for the Bitzi Bitcollider video plugin
 *
 * (PD) 2002 Mark Nelson [delirium] <delirium-bitzi@rufus.d2g.com>
 * Please see file COPYING or http://bitzi.com/publicdomain for more
 * information.
 *
 * The primary reference for the QuickTime file format is:
 *     http://developer.apple.com/techpubs/quicktime/qtdevdocs/QTFF/qtff.html
 */

#include "video.h"

/* QuickTime uses big-endian ordering, and block ("atom") lengths include the
 * entire atom, including the fourcc specifying atom type and the length
 * integer itself.
 */
void parse_quicktime(FILE *file, Data *data)
{
   char fourcc[5];
   unsigned blockLen;
   unsigned subBlockLen;
   unsigned subSubBlockLen;
   unsigned timescale;
   long blockStart;
   long subBlockStart;
   long subSubBlockStart;

   fseek(file, 4L, SEEK_SET);
   fread(fourcc, sizeof(char), 4, file);
   /* If data is first, header's at end of file, so skip to it */
   if(memcmp(fourcc, "mdat", 4)==0)
   {
      fseek(file, 0L, SEEK_SET);
      blockLen = fread_be(file, 4);
      fseek(file, (long) (blockLen + 4), SEEK_SET);
      fread(fourcc, sizeof(char), 4, file);
   }

   if(memcmp(fourcc, "moov", 4)!=0)
      return;
   blockStart = ftell(file);
   blockLen = fread_be(file, 4);	/* mvhd length */
   fread(fourcc, sizeof(char), 4, file);
   if(memcmp(fourcc, "mvhd", 4)!=0)
      return;

   /* Now we're at the start of the movie header */

   /* 20: time scale (time units per second) (4 bytes) */
   fseek(file, blockStart + 20, SEEK_SET);
   timescale = fread_be(file, 4);

   /* 24: duration in time units (4 bytes) */
   data->duration = (unsigned int) round_double((double) fread_be(file, 4)
						/ timescale * 1000);

   /* Skip the rest of the mvhd */
   fseek(file, blockStart + blockLen, SEEK_SET);

   /* Find and parse trak atoms */
   while(!feof(file))
   {
      unsigned int width, height;

      /* Find the next trak atom */
      blockStart = ftell(file);
      blockLen = fread_be(file, 4);	/* trak (or other atom) length */
      fread(fourcc, sizeof(char), 4, file);
      if(memcmp(fourcc, "trak", 4)!=0)	/* If it's not a trak atom, skip it */
      {
	 if(!feof(file))
	    fseek(file, blockStart + blockLen, SEEK_SET);
	 continue;
      }
      
      subBlockStart = ftell(file);
      subBlockLen = fread_be(file, 4);	/* tkhd length */
      fread(fourcc, sizeof(char), 4, file);
      if(memcmp(fourcc, "tkhd", 4)!=0)
	 return;
      
      /* Now in the track header */

      /* 84: width (2 bytes) */
      fseek(file, subBlockStart + 84, SEEK_SET);
      width = fread_be(file, 2);
      
      /* 88: height (2 bytes) */
      fseek(file, subBlockStart + 88, SEEK_SET);
      height = fread_be(file, 2);
      
      /* Note on above: Apple's docs say that width/height are 4-byte integers,
       * but all files I've seen have the data stored in the high-order two
       * bytes, with the low-order two being 0x0000.  Interpreting it the
       * "official" way would make width/height be thousands of pixels each.
       */
      
      /* Skip rest of tkhd */
      fseek(file, subBlockStart + subBlockLen, SEEK_SET);

      /* Find mdia atom for this trak */
      subBlockStart = ftell(file);
      subBlockLen = fread_be(file, 4);
      fread(fourcc, sizeof(char), 4, file);
      while(memcmp(fourcc, "mdia", 4)!=0)
      {
	 fseek(file, subBlockStart + subBlockLen, SEEK_SET);
	 subBlockStart = ftell(file);
	 subBlockLen = fread_be(file, 4);
	 fread(fourcc, sizeof(char), 4, file);
      }

      /* Now we're in the mdia atom; first sub-atom should be mdhd */
      subSubBlockStart = ftell(file);
      subSubBlockLen = fread_be(file, 4);
      fread(fourcc, sizeof(char), 4, file);
      if(memcmp(fourcc, "mdhd", 4)!=0)
	 return;
      /* TODO: extract language from the mdhd?  For now skip to hdlr. */
      fseek(file, subSubBlockStart + subSubBlockLen, SEEK_SET);
      subSubBlockStart = ftell(file);
      subSubBlockLen = fread_be(file, 4);
      fread(fourcc, sizeof(char), 4, file);
      if(memcmp(fourcc, "hdlr", 4)!=0)
	 return;
      /* 12: Component type: "mhlr" or "dhlr"; we only care about mhlr,
       * which should (?) appear first */
      fseek(file, subSubBlockStart + 12, SEEK_SET);
      fread(fourcc, sizeof(char), 4, file);
      if(memcmp(fourcc, "mhlr", 4)!=0)
	 return;
      fread(fourcc, sizeof(char), 4, file);
      if(memcmp(fourcc, "vide", 4)==0)	/* This is a video trak */
      {
	 data->height = height;
	 data->width = width;
      }

      /* Skip rest of the trak */
      fseek(file, blockStart + blockLen, SEEK_SET);
   }
}
