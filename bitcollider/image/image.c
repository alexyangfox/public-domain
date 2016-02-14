/* (PD) 2001 Mark Nelson [delirium] -> delirium4u@theoffspring.net
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * Revision history:
 * v0.1.0 - 30 May 2001 - Initial version, supports BMP (Bitmaps)
 * v0.1.1 - 01 Jun 2001 - Added endian-safe portable input functions
 * v0.2.0 - 04 Jun 2001 - Added support for GIF (CompuServe Graphics
 *                        Interchange Format)
 * v0.3.0 - 13 Jun 2001 - Added support for JPEG/JFIF
 * v0.4.0 - 12 Jan 2002 - Added support for PNG (Portable Network Graphics)
 * v0.4.1 - 14 Jan 2002 - Now returns a tag.image.format tag.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include "plugin.h"


/*--32-bit Specific Definitions of Portable Data Types---------------------*/

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned        uint32;

/*--Prototypes-------------------------------------------------------------*/

#ifndef _WIN32
#define init_plugin image_init_plugin
#endif

/* external plugin functions */
PluginMethods           *init_plugin(void);
static void              image_shutdown_plugin(void);
static void              image_free_attributes(Attribute *attrList);
static SupportedFormat  *image_get_supported_formats(void);
static const char       *image_get_name(void);
static const char       *image_get_version(void);
static char             *image_get_error(void);
static Attribute        *image_file_analyze(const char *fileName);

/* datafile parsing functions */
int parse_bmp(FILE *file, uint32 *width, uint32 *height, uint16 *bpp);
int parse_gif(FILE *file, uint32 *width, uint32 *height, uint16 *bpp);
int parse_jpg(FILE *file, uint32 *width, uint32 *height, uint16 *bpp);
int parse_png(FILE *file, uint32 *width, uint32 *height, uint16 *bpp);

/* endian-safe input functions */
uint8   read_8(FILE *file);
uint16  read_16_big_endian(FILE *file);
uint16  read_16_little_endian(FILE *file);
uint32  read_32_big_endian(FILE *file);
uint32  read_32_little_endian(FILE *file);

/*--Plugin Parameters------------------------------------------------------*/

#define PLUGIN_VERSION "0.4.1"
#define PLUGIN_NAME    "Image metadata (BMP, GIF, JPEG, PNG)"

/* NUM_ATTRS must be _one_more_ than the number of attribute we're
 * returning, to allow for a sentinel NULL/NULL pair.
 */
#define NUM_ATTRS      5

/*--Cross Platform Foo-----------------------------------------------------*/

#ifdef _WIN32
   #define strcasecmp stricmp
#else
   #include "../config.h"
#endif

/*--Plugin Info------------------------------------------------------------*/

static SupportedFormat formats[] =
{ 
   { ".bmp" , "Windows Bitmap image" },
   { ".gif" , "CompuServe Graphics Interchange Format (GIF) image" },
   { ".jpg" , "JPEG/JFIF compressed image" },
   { ".jpeg", "JPEG/JFIF compressed image" },
   { ".png" , "Portable Network Graphics (PNG) image" },
   { NULL   , NULL }
};

static char *errorString = NULL;

static PluginMethods methods = 
{
   image_shutdown_plugin,
   image_get_version,
   image_get_name,
   image_get_supported_formats,
   image_file_analyze,
   NULL,           /* no init/update/final */
   NULL,
   NULL,
   image_free_attributes,
   image_get_error
};

/*--Externally-called Plugin Functions-------------------------------------*/

PluginMethods *init_plugin(void)
{
   return &methods;
}

static void image_shutdown_plugin(void)
{
   if (errorString)
      free(errorString);
}

static const char *image_get_version(void)
{
   return PLUGIN_VERSION;
}

static const char *image_get_name(void)
{
   return PLUGIN_NAME;
}

static SupportedFormat *image_get_supported_formats(void)
{
   return formats;
}

static Attribute *image_file_analyze(const char *fileName)
{
   FILE        *file;
   Attribute   *attrList;
   char         temp[100];              /* Used for sprintf()'ing values to */
   char         format[5];              /* "BMP", "GIF", "JPEG", or "PNG" */
   char        *ext;
   int          errorcode = 1;
   
   uint32 width;
   uint32 height;
   uint16 bpp;
   
   file = fopen(fileName, "rb");
   if(file == NULL)
      return NULL;

   /* Find the filename extension (return if there is none) */
   ext = strrchr(fileName, '.');
   if(ext == NULL)
      return NULL;

   /* Call the appropriate parsing routine based on the extension */
   if(strcasecmp(ext, ".bmp") == 0)
   {
      strcpy(format, "BMP");
      errorcode = parse_bmp(file, &width, &height, &bpp);
   }
   else if(strcasecmp(ext, ".gif") == 0)
   {
      strcpy(format, "GIF");
      errorcode = parse_gif(file, &width, &height, &bpp);
   }
   else if(strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)
   {
      strcpy(format, "JPEG");
      errorcode = parse_jpg(file, &width, &height, &bpp);
   }
   else if(strcasecmp(ext, ".png") == 0)
   {
      strcpy(format, "PNG");
      errorcode = parse_png(file, &width, &height, &bpp);
   }
   
   fclose(file);

   /* Check for errors and do a sanity check on values. */
   if(errorcode || width==0 || height==0 || bpp==0)
      return NULL;
   
   /* by this point we should have valid info, so return it */
   attrList = malloc(sizeof(Attribute) * NUM_ATTRS);
   
   sprintf(temp, "%d", width);
   attrList[0].key = strdup("tag.image.width");
   attrList[0].value = strdup(temp);
   
   sprintf(temp, "%d", height);
   attrList[1].key = strdup("tag.image.height");
   attrList[1].value = strdup(temp);
   
   sprintf(temp, "%d", bpp);
   attrList[2].key = strdup("tag.image.bpp");
   attrList[2].value = strdup(temp);

   attrList[3].key = strdup("tag.image.format");
   attrList[3].value = strdup(format);

   /* Set a sentinel value so the bitcollider knows it's done */
   attrList[4].key = NULL;
   attrList[4].value = NULL;
   
   return attrList;
}

static void image_free_attributes(Attribute *attrList)
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

static char *image_get_error(void)
{
   return errorString;
}

/*--Functions to Parse Files for Width/Height/BPP info---------------------*/

/* All functions return 0 on success and 1 if file is not a valid file of that type */

int parse_bmp(FILE *file, uint32 *width, uint32 *height, uint16 *bpp)
{
   /* File must start with "BM" */
   if(read_8(file) != 'B' || read_8(file) != 'M')
      return 1;
   
   fseek(file, 16L, SEEK_CUR);
   
   *width = read_32_little_endian(file);
   *height = read_32_little_endian(file);
   fseek(file, 2L, SEEK_CUR);
   *bpp = read_16_little_endian(file);
   
   return 0;
}

int parse_gif(FILE *file, uint32 *width, uint32 *height, uint16 *bpp)
{
   unsigned char packed;
   uint16 bpp1, bpp2;
   
   /* File must start with "GIF" */
   if(read_8(file) != 'G' || read_8(file) != 'I' || read_8(file) != 'F')
      return 1;
   
   fseek(file, 3L, SEEK_CUR);
   *width = (uint32) read_16_little_endian(file);
   *height = (uint32) read_16_little_endian(file);
   
   /* packed byte: 
    * Bits 8 and 5 are flags we don't need to worry about; 
    * bits 6-8 and 1-3 are 3-bit descriptions of the number of bits, 
    * minus 1, of "color resolution" and "bits per pixel" respectively.
    * Usually these values are the same, but if they're not, take the
    * larger of the two to be "bpp," since this is what standard 
    * image editing programs seem to do.  I don't know why.
    */
   packed = read_8(file);
   bpp1 = ((packed & 0x70) >> 4) + 1;
   bpp2 = (packed & 0x07) + 1;
   
   if(bpp1 > bpp2)
      *bpp = bpp1;
   else
      *bpp = bpp2;
   
   return 0;
}

int parse_jpg(FILE *file, uint32 *width, uint32 *height, uint16 *bpp)
{
   uint16 bytes_left=0;
   
   /* File must start with 0xFFD8FFE0, <2 byte field length>, "JFIF", 0x00 */
   if(read_8(file) != 0xFF || read_8(file) != 0xD8 || 
      read_8(file) != 0xFF || read_8(file) != 0xE0)
      return 1;
   
   bytes_left = read_16_big_endian(file);
   bytes_left -= 2; /* 2 bytes of the field length indicator itself count */

   if(read_8(file) != 'J' || read_8(file) != 'F' || read_8(file) != 'I' || 
      read_8(file) != 'F' || read_8(file) != 0x00)
      return 1;
   
   bytes_left -= 5;
   
   /* skip past the rest of the JFIF indicator field */
   fseek(file, bytes_left, SEEK_CUR);
   
   /* now we parse the file for the image information field.  JPEG fields
    * have the general structure of: 0xFF, <1 byte field type>, <2 byte field
    * length>, <x byte field data>
    */
   while(!feof(file))
   {
      uint8 type, samples, bits_per_sample;
      
      /* if there's no 0xFF marker, JPEG file is malformed */
      if(read_8(file) != 0xFF)
         return 1;
      /* JPEG files are sometimes padded with sequential 0xFF bytes */
      do
      {
         type = read_8(file);

	 /* skip JPEG metadata extraction when file truncated here */
         if(feof(file))
            return 1;
      } while (type == 0xFF);
      
      switch (type) {
      /* image information fields (for various types of compression) */
      case 0xC0:
      case 0xC1:
      case 0xC2:
      case 0xC3:
      case 0xC5:
      case 0xC6:
      case 0xC7:
      case 0xC9:
      case 0xCA:
      case 0xCB:
      case 0xCD:
      case 0xCE:
      case 0xCF:
         fseek(file, 2L, SEEK_CUR);     /* skip the field length */
         bits_per_sample = read_8(file);
         *height = (uint32) read_16_big_endian(file);
         *width = (uint32) read_16_big_endian(file);
         samples = read_8(file);
         *bpp = (uint16) samples * bits_per_sample;
         return 0;
         
      case 0xD9:                        /* if end of image, */
      case 0xDA:                        /* or beginning of compressed data, */
         return 1;              /* there was no image info (or we missed it) */
         
         /* if any other field, we don't care, so skip past it */
      default:
         bytes_left = read_16_big_endian(file);
         /* since the length takes 2 bytes, length must be >= 2 */
         if (bytes_left < 2)
            return 1;
         bytes_left -= 2;
         
         /* skip the rest of the field and go on the next one */
         fseek(file, bytes_left, SEEK_CUR);
         break;
      }
   }
   return 1;
}

int parse_png(FILE *file, uint32 *width, uint32 *height, uint16 *bpp)
{
   uint8 bit_depth = 0, color_type = 0;

   /* File must start with 0x89, "PNG", 0x0D0A1A0A */
   if(read_8(file) != 0x89 || read_8(file) != 'P' || read_8(file) != 'N' ||
      read_8(file) != 'G' || read_8(file) != 0x0D || read_8(file) != 0x0A ||
      read_8(file) != 0x1A || read_8(file) != 0x0A)
      return 1;

   /* Skip IHDR chunk length (since we know its structure already) */
   fseek(file, 4L, SEEK_CUR);

   /* Make sure this really is an IHDR chunk (the first chunk must be an
    * IHDR chunk in a valid PNG file) */
   if(read_8(file) != 'I' || read_8(file) != 'H' || read_8(file) != 'D' ||
      read_8(file) != 'R')
      return 1;

   /* Read in our data */
   *width = read_32_big_endian(file);
   *height = read_32_big_endian(file);

   /* bpp depends on bit_depth and color_type. */
   bit_depth = read_8(file);
   color_type = read_8(file);

   switch(color_type)
   {
   case 0:                              /* grayscale */
   case 3:                              /* pixels are palette indices */
      *bpp = bit_depth;
      break;
   case 2:                              /* pixels are RGB triples */
      *bpp = bit_depth * 3;
      break;
   case 4:
      *bpp = bit_depth * 2;             /* pixels are grayscale + alpha */
      break;
   case 6:
      *bpp = bit_depth * 4;             /* pixels are RGB triples + alpha */
      break;
   default:
      return 1;                         /* invalid color_type */
   }

   return 0;
}

/*--Endian-Safe Input Functions--------------------------------------------*/

uint8 read_8(FILE *file)
{
    uint8 a;

    a=getc(file);

    return a;
}

uint16 read_16_big_endian(FILE *file)
{
    uint16 a,b;

    a=getc(file);
        b=getc(file);

    return ((a<<8) + b);
}

uint16 read_16_little_endian(FILE *file)
{
    uint16 a,b;

    a=getc(file);
        b=getc(file);

    return ((b<<8) + a);
}

uint32 read_32_big_endian(FILE *file)
{
    uint32 a,b,c,d;

    a=getc(file);
        b=getc(file);
        c=getc(file);
        d=getc(file);

    return ((a<<24) + (b<<16) + (c<<8) + d);
}

uint32 read_32_little_endian(FILE *file)
{
    uint32 a,b,c,d;

    a=getc(file);
        b=getc(file);
        c=getc(file);
        d=getc(file);

    return ((d<<24) + (c<<16) + (b<<8) + a);
}
