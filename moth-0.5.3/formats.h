/*
 * $Id: formats.h,v 1.7 2003/06/06 15:54:56 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See formats.dev for implementation notes.
 *
 */


#ifndef MOTH_FORMATS_H
#define MOTH_FORMATS_H


#include "data.h"



/*--------------------------------------------------------*\
|                      data structures                     |
\*--------------------------------------------------------*/

typedef enum {
  MOTH_CENTER_VERTICAL      = 0x01,
  MOTH_CENTER_HORIZONTAL    = 0x02,
  MOTH_DRAW_VERTICAL        = 0x04
} moth_format_text_flags;
typedef struct moth_format_text_s *moth_format_text;
struct moth_format_text_s {
  moth_point                location;   /* cartesian coordinate system */
  char                      *string;
  moth_color                color;
  moth_format_text_flags    flags;
};

typedef struct moth_format_legend_s *moth_format_legend;
struct moth_format_legend_s {
  moth_point                swatch[2];  /* bottom left, top right */
  moth_format_text          title;
};



/* 
 * Generic point, used by the different layer structures.
 */
typedef struct moth_format_point_s *moth_format_point;
struct moth_format_point_s {
  moth_point                location;   /* cartesion coordinate system */
  double                    size;       /* width or radius, if needed */
  moth_format_point         next;       /* linked list */
};

/* 
 * Generic layer structure.  The different layers will use this as one
 * component of a larger structure, the overall structure of which depends on
 * each layer type.
 *
 * For example, the <segments> layer construction code (see 
 * moth_layer_draw_segments in layers.c) will create a list of these
 * structures, each of which contains exactly two points.
 */
typedef struct moth_format_layer_piece_s *moth_format_layer_piece;
struct moth_format_layer_piece_s {
  moth_format_point         points;
  moth_format_layer_piece   next;       /* linked list */
};

/* 
 * <points>-specific layer structure.  Needed because of the "filled"
 * attribute.
 */
typedef struct moth_format_layer_points_s *moth_format_layer_points;
struct moth_format_layer_points_s {
  moth_format_point         points;
  int                       filled;
};



typedef struct moth_format_tick_s *moth_format_tick;
typedef struct moth_format_axis_s *moth_format_axis;
struct moth_format_axis_s {
  moth_axis_location        location;
  moth_format_point         line;       /* composed of two points */
  moth_format_text          title;
  moth_color                color;
  moth_format_tick          ticks;
  moth_format_axis          next;       /* linked list */
};
struct moth_format_tick_s {
  moth_format_point         mark;       /* composed of two points */
  moth_format_text          text;
  moth_color                color;
  moth_format_tick          next;       /* linked list */
};



/*--------------------------------------------------------*\
|                        common API                        |
\*--------------------------------------------------------*/

moth_format   moth_format_create      (const char *);
/* Creates a linked list of formats from the given string, which is a comma-
 * separated list of filename extensions.  */

unsigned int  moth_format_is_pixmap   (moth_format format);
/* Returns boolean indicating if format uses the global pixmap or not.  */

void          moth_format_dump        (moth_format format);

void          moth_format_destroy     (moth_format format);



/*--------------------------------------------------------*\
|                      pixmap formats                      |
\*--------------------------------------------------------*/

void  moth_format_save_pixmap     (moth_format              format,
                                   const char               *filename,
                                   moth_pixmap              pixmap);
/* Writes the pixmap out to all formats specified in the linked list.  */



/*--------------------------------------------------------*\
|                      vector formats                      |
\*--------------------------------------------------------*/

void  moth_format_start_image     (moth_format              format,
                                   const char               *filename,
                                   unsigned int             width,
                                   unsigned int             height,
                                   moth_color               background,
                                   moth_color               border,
                                   int unsigned             border_size);

void  moth_format_draw_title      (moth_format              format,
                                   moth_format_text         title);

void  moth_format_draw_watermark  (moth_format              format,
                                   moth_format_text         watermark);

void  moth_format_draw_canvas     (moth_format              format,
                                   moth_color               color,
                                   moth_point               bottom_left,
                                   moth_point               top_right);

void  moth_format_draw_axes       (moth_format              format,
                                   moth_format_axis         axes);

void  moth_format_draw_area       (moth_format              format,
                                   moth_color               color,
                                   moth_format_layer_piece  areas,
                                   moth_format_legend       legend);

void  moth_format_draw_grid       (moth_format              format,
                                   moth_color               color,
                                   moth_format_layer_piece  lines,
                                   moth_format_legend       legend);

void  moth_format_draw_line       (moth_format format,
                                   moth_color color,
                                   moth_format_layer_piece  lines,
                                   moth_format_legend       legend);

void  moth_format_draw_points     (moth_format              format,
                                   moth_color               color,
                                   moth_format_layer_points points,
                                   moth_format_legend       legend);

void  moth_format_draw_segments   (moth_format              format,
                                   moth_color               color,
                                   moth_format_layer_piece  lines,
                                   moth_format_legend       legend);

void  moth_format_finish_image    (moth_format              format);



#endif

