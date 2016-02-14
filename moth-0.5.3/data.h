/*
 * $Id: data.h,v 1.26 2004/03/17 04:14:06 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See data.dev for implementation notes.
 *
 */

#ifndef MOTH_DATA_H
#define MOTH_DATA_H

#include <stdarg.h>
#include <stdio.h>



/*--------------------------------------------------------*\
|                    constants / macros                    |
\*--------------------------------------------------------*/

typedef enum {
  MOTH_LOCATION_UNKNOWN = 0,
  MOTH_LEFT,
  MOTH_TOP,
  MOTH_RIGHT,
  MOTH_BOTTOM
} moth_axis_location;

typedef enum {
  MOTH_LINEAR = 0,
  MOTH_LOG2,
  MOTH_LOGE,
  MOTH_LOG10,
  /* all non-auto enums should appear before this one */
  MOTH_AUTO           
} moth_axis_scaling;

/* I can never remember which is which */
#define STRCMP_MATCH(a) (0==(a))
#define STRCMP_BEFORE(a) (0>(a))
#define STRCMP_AFTER(a) (0<(a))

/* because in C, what it's called is not the same as what it does */
#define INTERNAL static


#define MOTH_LIST_SEPARATOR             ','
#define MOTH_IMAGE_WATERMARK_COLOR      "#00000022"

/* When we don't care if two numbers are -exactly- the same, how close is close
 * enough. */
#define MOTH_DOUBLE_EPSILON             0.0000001

/* size of graphics elements and spacers */
#define MOTH_IMAGE_LEGEND_SWATCHSIZE    7
#define MOTH_IMAGE_MARGIN               7
#define MOTH_IMAGE_SPACER               4
#define MOTH_IMAGE_SMALL_SPACER         2

/* When there is only one value on the axis, how much to stretch the axis. */
#define MOTH_AXIS_SINGLE_VALUE_RANGE    1.0

/* Which colors to pick automatically for layers, if they're not specified. */
#define MOTH_COLOR_AUTO_LIST \
  "#0000FF" "#00FF00" "#FF0000" \
  "#00FFFF" "#FFFF00" "#FF00FF" \
  "#000099" "#009900" "#990000" \
  "#009999" "#999900" "#990099"

/* 
 * When scoring the different scaling methods to automatically choose one
 * (<axis scaling="auto">), how much better than linear another scaling method
 * has to be (as a percentage).
 */
#define MOTH_COLUMN_LIST_LINEAR_PREFERENCE 0.05



/*--------------------------------------------------------*\
|                         defaults                         |
\*--------------------------------------------------------*/

#define MOTH_LINE_WIDTH_DEFAULT         1.0
#define MOTH_RADIUS_DEFAULT             2.0
#define MOTH_SCALING_DEFAULT            MOTH_LINEAR

#define MOTH_FONT_DEFAULT "/usr/share/fonts/default/Type1/n019043l.pfb"
#define MOTH_FONT_SIZE_DEFAULT          12

#define MOTH_IMAGE_DEFAULT_BORDERSIZE   MOTH_LINE_WIDTH_DEFAULT
#define MOTH_IMAGE_AXIS_WIDTH_DEFAULT   MOTH_LINE_WIDTH_DEFAULT
#define MOTH_IMAGE_TICK_WIDTH_DEFAULT   MOTH_LINE_WIDTH_DEFAULT
#define MOTH_IMAGE_TICK_LENGTH_DEFAULT  3.0

#define MOTH_IMAGE_BACKGROUND_COLOR_DEFAULT   "#FFFFFF"
#define MOTH_IMAGE_BORDER_COLOR_DEFAULT       "#FFFFFF"



/*--------------------------------------------------------*\
|                      data structures                     |
\*--------------------------------------------------------*/

/* primitives */
typedef struct moth_buffer_s      *moth_buffer;         /* data.c     */
typedef struct moth_color_s       *moth_color;          /* colors.h   */
typedef struct moth_column_s      *moth_column;         /* columns.c  */
typedef struct moth_column_list_s *moth_column_list;    /* columns.c  */

/* processing tools */
typedef struct moth_runtime_s       *moth_runtime;
typedef struct moth_options_s       *moth_options;      /* options.c  */
typedef struct moth_file_s          *moth_file;         /* file.h     */
typedef struct moth_color_store_s   *moth_color_store;  /* colors.c   */
typedef struct moth_parser_s        *moth_parser;       /* parser.c   */
typedef struct moth_column_store_s  *moth_column_store; /* columns.c  */

/* graph elements */
typedef struct moth_axis_s        *moth_axis;           /* axes.h     */
typedef struct moth_format_s      *moth_format;         /* formats.h  */
typedef struct moth_image_s       *moth_image;          /* image.c    */
typedef struct moth_label_s       *moth_label;          /* axes.h     */
typedef struct moth_label_tick_s  *moth_label_tick;     /* axes.h     */
typedef struct moth_layer_s       *moth_layer;          /* layer.h    */
typedef struct moth_point_s        moth_point;
typedef struct moth_pixmap_s      *moth_pixmap;         /* pixmap.c   */
typedef struct moth_source_s      *moth_source;         /* sources.h  */


struct moth_runtime_s {
  /* per-execution state */
  moth_options      options;

  /* per-file state */
  moth_file         file;
};


struct moth_point_s {
  double  x;
  double  y;
};



/*--------------------------------------------------------*\
|                         globals                          |
\*--------------------------------------------------------*/

extern moth_runtime   runtime;



/*--------------------------------------------------------*\
|                    utility functions                     |
\*--------------------------------------------------------*/

void      warn            (const char *format, ...);
/* Prints a warning, and allows execution to continue.  */

void      die             (const char *format, ...);
/* Prints an error, and stops program execution.  */




/*--------------------------------------------------------*\
|                      math functions                      |
\*--------------------------------------------------------*/

double    scale_value     (moth_axis_scaling scaling, double value);
/* Transforms value according to the given scaling.  */

double    unscale_value   (moth_axis_scaling scaling, double value);
/* Performs the inverse of scale_value().  */

double    convert_value   (double value, char modifier, char unit);
/* Interprets value in terms of units.  */
/* See also docs/UNITS.  */

int       parse_value     (const char *string, double *value, char *modifier, char *unit);
/* Tries to breaks a string up into parts as specified in docs/UNITS.
 * If successful, returns true and sets the value, modifier, and unit pointed
 * to by the last three arguments.  Returns false otherwise.  */

double    read_value      (const char *string);
/* Reads value from string, also handling unit specifiers as specified in
 * docs/UNITS.  It also understands datetimes in the form 
 * "YYYY-MM-DD HH:II:SS".  */

int       double_compare  (const void *a, const void *b);
/* For use with qsort() */


/*--------------------------------------------------------*\
|                    datetime functions                    |
\*--------------------------------------------------------*/

double  datetime_modulus  (double value, char unit, double count);
/* Returns the value rounded down to the nearest count units.  
 * Unit is a character as understood by read_value().
 * See also docs/UNITS.  */

double  datetime_add      (double value, char unit, double amount);
/* Add count units to the current value.  Unit is a character as
 * understood by read_value().  See also docs/UNITS.  */


/*--------------------------------------------------------*\
|                          buffer                          |
\*--------------------------------------------------------*/

moth_buffer   moth_buffer_create  (void);

void          moth_buffer_add     (moth_buffer, const void *, size_t);
/* Invalidates pointer returned by moth_buffer_data(). */

size_t        moth_buffer_size    (moth_buffer);
/* Returns the number of bytes of data in the buffer.  */

const void *  moth_buffer_data    (moth_buffer);
/* Returns a pointer to the data of the buffer.  This pointer is invalidated
 * when the buffer is changed through moth_buffer_add().  */

char *        moth_buffer_strdup  (moth_buffer);
/* Returns a malloc'd and null-terminated version of the buffer.  */

void          moth_buffer_destroy (moth_buffer);



#endif  /* MOTH_DATA_H */

