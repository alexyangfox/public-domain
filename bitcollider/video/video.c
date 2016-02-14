/* Bitzi Bitcollider video plugin
 * 
 * (PD) 2002 Mark Nelson [delirium] <delirium-bitzi@rufus.d2g.com>
 * Please see file COPYING or http://bitzi.com/publicdomain 
 * for more info.
 *
 * Currently supported: AVI, QuickTime, MPEG-1, MPEG-2
 *  (types are detected by headers, not by extension, since many files on
 *   the internet have the wrong extension)
 *
 * Revision history:
 * v0.0.1 - 12 Jan 2002 - Initial version; preliminary AVI support.
 * v0.1.0 - 19 Aug 2002 - Complete rewrite, now supports AVI/QT/MPEG-1/MPEG-2
 */

#include "video.h"

/* --- Prototypes for externally-called plugin functions --- */

#ifndef _WIN32
#define init_plugin video_init_plugin
#endif

PluginMethods           *video_init_plugin(void);
static void              video_shutdown_plugin(void);
static void              video_free_attributes(Attribute *attrList);
static SupportedFormat  *video_get_supported_formats(void);
static const char       *video_get_name(void);
static const char       *video_get_version(void);
static char             *video_get_error(void);
static Attribute        *video_file_analyze(const char *fileName);

/* --- Initialize plugin information --- */

static SupportedFormat formats[] =
{ 
   { ".AVI",  "Microsoft AVI"   },
   { ".MOV",  "Apple QuickTime" },
   { ".QT",  "Apple QuickTime"  },
   { ".MPG",  "MPEG-1"          },
   { ".MPEG", "MPEG-1"          },
   { ".M2V",  "MPEG-2"          },
   { NULL,    NULL              }
};
static char *errorString = NULL;

static PluginMethods methods = 
{
   video_shutdown_plugin,
   video_get_version,
   video_get_name,
   video_get_supported_formats,
   video_file_analyze,
   NULL,				/* no init/update/final */
   NULL,
   NULL,
   video_free_attributes,
   video_get_error
};

/* --- Externally-called Plugin Functions --- */

PluginMethods *init_plugin(void)
{
   return &methods;
}

static void video_shutdown_plugin(void)
{
   if (errorString)
      free(errorString);
}

static const char *video_get_version(void)
{
   return PLUGIN_VERSION;
}

static const char *video_get_name(void)
{
   return PLUGIN_NAME;
}

static SupportedFormat *video_get_supported_formats(void)
{
   return formats;
}

static Attribute *video_file_analyze(const char *fileName)
{
   FILE *file;
   Attribute *attrList;
   int numAttrs;
   int attr;
   int version;
   char temp[100];		     /* Used for sprintf()'ing values to */
   char fmt[10] = "";
   Format format;

   Data data;
   data.width = 0;
   data.height = 0;
   data.fps = 0;
   data.duration = 0;
   data.bitrate = 0;
   data.codec = NULL;
   
   file = fopen(fileName, "rb");
   if(file == NULL)
      return NULL;

   format = find_format(file);

   switch(format)
   {
   case AVI:
      parse_avi(file, &data);
      strcpy(fmt, "AVI");
      break;
   case QUICKTIME:
      parse_quicktime(file, &data);
      strcpy(fmt, "QuickTime");
      break;
   case MPEG:
      version = parse_mpeg(file, &data);
      if(version == 1)
	 strcpy(fmt, "MPEG-1");
      else if(version == 2)
	 strcpy(fmt, "MPEG-2");
      break;
   case UNKNOWN:
      /* this is only here to quiet compiler warnings */
	   ;
   }

   /* If necessary, use filesize to estimate bitrate from duration
    * or vice versa */
   if(data.bitrate == 0 && data.duration != 0)
   {
      fseek(file, 0L, SEEK_END);
      data.bitrate = (unsigned int)
	 round_double((double) ftell(file) / data.duration * 8);
   }
   else if(data.duration == 0 && data.bitrate != 0)
   {
      fseek(file, 0L, SEEK_END);
      data.duration = (unsigned int)
	 round_double((double) ftell(file) / data.bitrate * 8);
   }
   
   fclose(file);

   /* Figure out how many attributes we collected (everything not 0/NULL/"" */
   numAttrs = (strcmp(fmt, "") != 0) + (data.width != 0) + (data.height != 0)
      + (data.fps != 0) + (data.duration != 0) + (data.bitrate != 0) + 
      (data.codec != NULL);

   if(numAttrs == 0)
      return NULL;

   /* Allocate space for all the attributes plus a NULL/NULL sentinel pair */
   attrList = malloc(sizeof(Attribute) * (numAttrs + 1));

   /* Return the attributes we collected */
   attr = 0;
   if(strcmp(fmt, "") != 0)
   {
      attrList[attr].key = strdup("tag.video.format");
      attrList[attr].value = strdup(fmt);
      attr++;
   }
   if(data.width != 0)
   {
      sprintf(temp, "%u", data.width);
      attrList[attr].key = strdup("tag.video.width");
      attrList[attr].value = strdup(temp);
      attr++;
   }
   if(data.height != 0)
   {
      sprintf(temp, "%u", data.height);
      attrList[attr].key = strdup("tag.video.height");
      attrList[attr].value = strdup(temp);
      attr++;
   }
   if(data.fps != 0)
   {
      sprintf(temp, "%u", data.fps);
      attrList[attr].key = strdup("tag.video.fps");
      attrList[attr].value = strdup(temp);
      attr++;
   }
   if(data.duration != 0)
   {
      sprintf(temp, "%u", data.duration);
      attrList[attr].key = strdup("tag.video.duration");
      attrList[attr].value = strdup(temp);
      attr++;
   }
   if(data.bitrate != 0)
   {
      sprintf(temp, "%u", data.bitrate);
      attrList[attr].key = strdup("tag.video.bitrate");
      attrList[attr].value = strdup(temp);
      attr++;
   }
   if(data.codec != NULL)
   {
      attrList[attr].key = strdup("tag.video.codec");
      attrList[attr].value = data.codec;
      attr++;
   }
   attrList[attr].key = NULL;
   attrList[attr].value = NULL;

   return attrList;
}

static void video_free_attributes(Attribute *attrList)
{
   int i = 0;

   while(attrList[i].key != NULL)
   {
      free(attrList[i].key);
      free(attrList[i].value);
      i++;
   }
   
   free(attrList);
}

static char *video_get_error(void)
{
   return errorString;
}

/* --- Classify file format --- */

Format find_format(FILE *file)
{
   unsigned char buffer[HEAD_BUFFER];

   /* Read start of the file into a buffer, so we don't have to
    * keep re-reading for each file-format check. */
   if(fread(buffer, sizeof(char), HEAD_BUFFER, file) != HEAD_BUFFER)
      return UNKNOWN;

   rewind(file);

   /* AVI signature: "RIFF____AVI " */
   if(memcmp(buffer, "RIFF", 4)==0 && memcmp(buffer+8, "AVI ", 4)==0)
      return AVI;

   /* QuickTime signature: "____moov" or "____mdat" */
   else if(memcmp(buffer+4, "moov", 4)==0 || memcmp(buffer+4, "mdat", 4)==0)
      return QUICKTIME;

   /* MPEG signature: 0x000001B3 or 0x000001BA */
   else if(buffer[0]==0x00 && buffer[1]==0x00 && buffer[2]==0x01 &&
	   (buffer[3]==0xB3 || buffer[3]==0xBA))
      return MPEG;

   return UNKNOWN;
}

/* --- Utility functions --- */

/* We implement our own rounding function, because the availability of
 * C99's round(), nearbyint(), rint(), etc. seems to be spotty, whereas
 * floor() is available in math.h on all C compilers.
 */
double round_double(double num)
{
   return floor(num + 0.5);
}

/* Read the specified number of bytes as a little-endian (least
 * significant byte first) integer.
 * Note: bytes must be less than the byte width of "unsigned long int"
 * on your platform (e.g. 8 for 32-bit systems). */
unsigned long int fread_le(FILE *file, int bytes)
{
   int x;
   unsigned long int result = 0;

   for(x=0; x < bytes; x++)
      result |= getc(file) << (x*8);

   return result;
}

/* Same as above, but big-endian (most significant byte first) ordering */
unsigned long int fread_be(FILE *file, int bytes)
{
   int x;
   unsigned long int result = 0;

   for(x=bytes-1; x >= 0; x--)
      result |= getc(file) << (x*8);

   return result;
}
