/*
 * $Id: pixmap.h,v 1.8 2003/06/05 20:33:06 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See pixmap.dev for notes and coding issues.
 *
 */



#ifndef MOTH_RENDERER_H
#define MOTH_RENDERER_H

#include "colors.h"
#include "data.h"
#include "formats.h"



typedef struct moth_pixmap_char_s *moth_pixmap_char;
typedef struct moth_pixmap_string_s *moth_pixmap_string;
struct moth_pixmap_string_s {
  unsigned int      width;
  unsigned int      height;
  /* INTERNAL */
  size_t            count;
  moth_pixmap_char  glyphs;
  size_t            glyph_count;
};



/*--------------------------------------------------------*\
|                     main pixmap API                      |
\*--------------------------------------------------------*/

moth_pixmap   moth_pixmap_create (void);

void          moth_pixmap_get_geometry (moth_pixmap pixmap,
                                          unsigned int *width,
                                          unsigned int *height);

void          moth_pixmap_get_data (moth_pixmap pixmap,
                                          unsigned char **data,
                                          unsigned int *rowspan,
                                          unsigned int *bytes_per_pixel);

void          moth_pixmap_destroy (moth_pixmap pixmap);



/*--------------------------------------------------------*\
|                    vector format API                     |
\*--------------------------------------------------------*/

void  moth_pixmap_start_image     (moth_pixmap              pixmap,
                                   unsigned int             width,
                                   unsigned int             height,
                                   moth_color               background,
                                   moth_color               border_color,
                                   int unsigned             border_size);

void  moth_pixmap_draw_canvas     (moth_pixmap              pixmap,
                                   moth_color               color,
                                   moth_point               bottom_left,
                                   moth_point               top_right);

void  moth_pixmap_draw_watermark  (moth_pixmap              pixmap,
                                   moth_format_text         watermark,
                                   moth_pixmap_string       string);

void  moth_pixmap_draw_title      (moth_pixmap              pixmap,
                                   moth_format_text         title,
                                   moth_pixmap_string       string);

void  moth_pixmap_draw_axes       (moth_pixmap              pixmap,
                                   moth_format_axis         axes);

void  moth_pixmap_draw_area       (moth_pixmap              pixmap,
                                   moth_color               color,
                                   moth_format_layer_piece  areas,
                                   moth_format_legend       legend);

void  moth_pixmap_draw_grid       (moth_pixmap              pixmap,
                                   moth_color               color,
                                   moth_format_layer_piece  lines,
                                   moth_format_legend       legend);

void  moth_pixmap_draw_line       (moth_pixmap              pixmap,
                                   moth_color               color,
                                   moth_format_layer_piece  lines,
                                   moth_format_legend       legend);

void  moth_pixmap_draw_points     (moth_pixmap              pixmap,
                                   moth_color               color,
                                   moth_format_layer_points points,
                                   moth_format_legend       legend);

void  moth_pixmap_draw_segments   (moth_pixmap              pixmap,
                                   moth_color               color,
                                   moth_format_layer_piece  lines,
                                   moth_format_legend       legend);

void  moth_pixmap_finish_image    (moth_pixmap              pixmap);



/*--------------------------------------------------------*\
|                    pixmap string API                     |
\*--------------------------------------------------------*/

moth_pixmap_string  moth_pixmap_string_create (moth_pixmap pixmap,
                                          const char *text,
                                          int vertical);

void                moth_pixmap_string_destroy (moth_pixmap_string string);



#endif

