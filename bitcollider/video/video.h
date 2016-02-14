/* Header file for the Bitzi Bitcollider video plugin
 *
 * (PD) 2002 Mark Nelson [delirium] <delirium-bitzi@rufus.d2g.com>
 * Please see file COPYING or http://bitzi.com/publicdomain for more
 * information.
 */

#ifndef VIDEO_H
#define VIDEO_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

#include "plugin.h"

#define PLUGIN_VERSION "0.1.0"
#define PLUGIN_NAME    "Video metadata (AVI, QuickTime, MPEG-1, MPEG-2)"

#define HEAD_BUFFER    12		/* We must be able to determine
					 * file format using this many bytes
					 * from the beginning of the file */

/* 32-bit-specific definitions of portable data types */
typedef unsigned int uint32;

/* The various formats we support. */
typedef enum
{
   UNKNOWN,
   AVI,					/* Microsoft AVI */
   QUICKTIME,				/* Apple QuickTime (MOV) */
   MPEG					/* MPEG 1 or 2 */
} Format;

/* Wrap the metadata we're collecting into a struct for easy passing */
typedef struct
{
   unsigned int width;			/* width in pixels */
   unsigned int height;			/* height in pixels */
   unsigned int fps;			/* frames per second */
   unsigned int duration;		/* duration in milliseconds */
   unsigned int bitrate;		/* bitrate in kbps */
   char *codec;				/* video compression codec */
} Data;

/* local prototypes */

Format find_format(FILE *file);
void parse_avi(FILE *file, Data *data);
void parse_quicktime(FILE *file, Data *data);
int parse_mpeg(FILE *file, Data *data);
double round_double(double num);
unsigned long int fread_le(FILE *file, int bytes);
unsigned long int fread_be(FILE *file, int bytes);

#endif
