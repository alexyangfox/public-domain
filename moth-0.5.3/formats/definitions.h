/*
 * $Id: definitions.h,v 1.1 2003/06/05 20:33:07 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2003.  Share and enjoy!
 *
 * See ../formats.dev for implementation notes.
 *
 */


#ifndef MOTH_FORMATS_DEFINITIONS_H
#define MOTH_FORMATS_DEFINITIONS_H


typedef void (*moth_format_save_pixmap_func)
  (
    moth_format format,
    const char  *filename,
    moth_pixmap pixmap
  );

typedef void (*moth_format_start_image_func)
  (
    moth_format   format,
    const char    *filename,
    unsigned int  width,
    unsigned int  height,
    moth_color    background,
    moth_color    border,
    int unsigned  border_size
  );

typedef void (*moth_format_draw_title_func)
  (
    moth_format       format,
    moth_format_text  title
  );

typedef void (*moth_format_draw_watermark_func)
  (
    moth_format       format,
    moth_format_text  watermark
  );

typedef void (*moth_format_draw_canvas_func)
  (
    moth_format format,
    moth_color  color,
    moth_point  bottom_left,
    moth_point  top_right
  );

typedef void (*moth_format_draw_axes_func)
  (
    moth_format       format,
    moth_format_axis  axes
  );

typedef void (*moth_format_draw_area_func)
  (
    moth_format             format,
    moth_color              color,
    moth_format_layer_piece areas,
    moth_format_legend      legend
  );

typedef void (*moth_format_draw_grid_func)
  (
    moth_format             format,
    moth_color              color,
    moth_format_layer_piece lines,
    moth_format_legend      legend
  );

typedef void (*moth_format_draw_line_func)
  (
    moth_format             format,
    moth_color              color,
    moth_format_layer_piece lines,
    moth_format_legend      legend
  );

typedef void (*moth_format_draw_points_func)
  (
    moth_format               format,
    moth_color                color,
    moth_format_layer_points  points,
    moth_format_legend        legend
  );

typedef void (*moth_format_draw_segments_func)
  (
    moth_format             format,
    moth_color              color,
    moth_format_layer_piece lines,
    moth_format_legend      legend
  );

typedef void (*moth_format_finish_image_func)
  (
    moth_format format
  );


typedef struct moth_format_definition_s *moth_format_definition;
struct moth_format_definition_s {
  /* pixmap interface */
  unsigned int                    is_pixmap;
  moth_format_save_pixmap_func    save_pixmap;

  /* vector interface */
  moth_format_start_image_func    start_image;
  moth_format_draw_title_func     draw_title;
  moth_format_draw_watermark_func draw_watermark;
  moth_format_draw_canvas_func    draw_canvas;
  moth_format_draw_axes_func      draw_axes;
  moth_format_draw_area_func      draw_area;
  moth_format_draw_grid_func      draw_grid;
  moth_format_draw_line_func      draw_line;
  moth_format_draw_points_func    draw_points;
  moth_format_draw_segments_func  draw_segments;
  moth_format_finish_image_func   finish_image;
};



#endif

