/*
 * $Id: pixmap.c,v 1.17 2003/06/20 17:42:22 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 *
 * The drawing library (libart) uses "screen" coordinates
 * (left is zero, top is zero).  moth_point_art() does the
 * conversion.
 *
 * See pixmap.dev for notes and coding issues.
 *
 */



#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <libart_lgpl/libart.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "axes.h"
#include "data.h"
#include "image.h"
#include "options.h"
#include "pixmap.h"



#define MOTH_BYTES_PER_PIXEL 3



/*--------------------------------------------------------*\
|                      datastructures                      |
\*--------------------------------------------------------*/

struct moth_pixmap_s {
  unsigned int      width;
  unsigned int      height;

  /* bitmap stuff */
  art_u8            *buffer;
  unsigned int      rowstride;

  /* font stuff */
  FT_Library        freetype;
  FT_Face           face;
};



struct moth_pixmap_char_s {
  FT_Glyph image;
};



/*--------------------------------------------------------*\
|                    utility functions                     |
\*--------------------------------------------------------*/

INTERNAL  ArtPoint  moth_point_art  (moth_pixmap pixmap, moth_point point);
INTERNAL  void      moth_draw_svp   (moth_pixmap pixmap, ArtSVP *path,
                                        moth_color color);



INTERNAL
art_u32
moth_color_art
(
  moth_color color
)
{
  art_u32 c;
  if (!color) return 0;
  c = (color->red << 24) | (color->green << 16)
    | (color->blue << 8) | (color->alpha);
  return c;
}



INTERNAL
void
moth_pixmap_draw_FT_Bitmap
(
  moth_pixmap     pixmap,
  FT_BitmapGlyph  bit,
  ArtPoint        where,    /* in default coordinates */
  moth_color      color
)
{
  unsigned int  grays;
  unsigned int  row, column;

  /* nothing to do */
  if (bit->bitmap.rows == 0)
    return;

  grays = bit->bitmap.num_grays - 1;
  for (row=0; row < bit->bitmap.rows; ++row) {
    size_t pixmap_offset_y;
    pixmap_offset_y = where.y + 0.5 + row - bit->top;
    /* don't draw off top or bottom of image */
    if (pixmap_offset_y < 0 || pixmap_offset_y >= pixmap->height)
      continue;
    pixmap_offset_y *= pixmap->rowstride;

    for (column=0; column < bit->bitmap.width; ++column) {
      size_t        pixmap_offset_x, pixmap_offset, bitmap_offset;
      unsigned char red, green, blue, new;
      double        old_level, new_level;

      pixmap_offset_x = where.x-0.5 + column + bit->left;
      /* don't draw off left or right of image */
      if (pixmap_offset_x < 0 || pixmap_offset_x >= pixmap->width)
        continue;
      pixmap_offset_x *= MOTH_BYTES_PER_PIXEL;
      pixmap_offset = pixmap_offset_y + pixmap_offset_x;

      /* don't draw outside of allocated space */
      if ( pixmap_offset < 0
           || (pixmap_offset+2) >= (pixmap->height * pixmap->rowstride))
        continue;

      red   = pixmap->buffer[pixmap_offset+0];
      green = pixmap->buffer[pixmap_offset+1];
      blue  = pixmap->buffer[pixmap_offset+2];

      bitmap_offset = row * bit->bitmap.width + column;
      new = *(bit->bitmap.buffer + bitmap_offset);
      /* apply color alpha */
      new *= (double) color->alpha / (double) grays;

      new_level = (double) new / (double) grays;
      old_level = 1.0 - new_level;
      red   = (double)   red * old_level + (double)   color->red * new_level;
      green = (double) green * old_level + (double) color->green * new_level;
      blue  = (double)  blue * old_level + (double)  color->blue * new_level;

      pixmap->buffer[pixmap_offset+0] = red;
      pixmap->buffer[pixmap_offset+1] = green;
      pixmap->buffer[pixmap_offset+2] = blue;
    }
  }
}



INTERNAL
void
moth_pixmap_draw_primitive_circle
(
  moth_pixmap pixmap,
  moth_point  center,
  double      radius,
  int         filled,
  moth_color  color
)
{
  ArtPoint  a_center;
  ArtVpath  *circle;
  ArtSVP    *stroke;

  if (isnan(center.x) || isnan(center.y)) {
    return;
  }

  if (radius <= 0 || isnan(radius))
    return;

  a_center = moth_point_art (pixmap, center);
  circle = art_vpath_new_circle (a_center.x, a_center.y, radius);
  if (filled)
    stroke = art_svp_from_vpath (circle);
  else
    stroke = art_svp_vpath_stroke (circle,
      ART_PATH_STROKE_JOIN_ROUND, ART_PATH_STROKE_CAP_ROUND,
      1.0, 1.0, 1.0);
  moth_draw_svp (pixmap, stroke, color);

  art_free (stroke);
  art_free (circle);
}



INTERNAL
void
moth_pixmap_draw_primitive_line
(
  moth_pixmap pixmap,
  moth_point  start,
  moth_point  end,
  double      width,
  moth_color  color
)
{
  ArtPoint  a_start, a_end;
  ArtVpath  *line;
  ArtSVP    *stroke;

  if (  isnan(start.x) || isnan(start.y)
       || isnan(end.x) || isnan(end.y)   )
  {
    return;
  }

  a_start = moth_point_art (pixmap, start);
  a_end = moth_point_art (pixmap, end);

  line = art_new (ArtVpath, 3);
  line[0].code = ART_MOVETO_OPEN;
  line[0].x = a_start.x;
  line[0].y = a_start.y;
  line[1].code = ART_LINETO;
  line[1].x = a_end.x;
  line[1].y = a_end.y;
  line[2].code = ART_END;
  stroke = art_svp_vpath_stroke (line,
      ART_PATH_STROKE_JOIN_BEVEL, ART_PATH_STROKE_CAP_SQUARE,
      width, width, 1.0);

  moth_draw_svp (pixmap, stroke, color);

  art_free (stroke);
  art_free (line);
}



INTERNAL
void
moth_pixmap_draw_primitive_string
(
  moth_pixmap         pixmap,
  moth_point          where,
  moth_pixmap_string  string,
  moth_color          color,
  int                 center_x,
  int                 center_y
)
{
  ArtPoint            pen;
  moth_pixmap_char    glyph;
  int                 n;

  if (!pixmap) return;
  if (!string) return;

  if (isnan(where.x) || isnan(where.y))
    return;

  pen = moth_point_art (pixmap, where);
  if (center_x)
    pen.x -= trunc (string->width/2.0);
  if (center_y)
    pen.y += trunc (string->height/2.0);

  glyph = string->glyphs;
  for (n=0; n<string->glyph_count; ++n, ++glyph) {
    FT_BitmapGlyph  bit;

    if (!glyph->image)
      continue;
    bit = (FT_BitmapGlyph) glyph->image;
    moth_pixmap_draw_FT_Bitmap (pixmap, bit, pen, color);
  }
}



INTERNAL
void
moth_pixmap_draw_format_text
(
  moth_pixmap       pixmap,
  moth_format_text  text
)
{
  moth_pixmap_string  string;

  if (!pixmap) return;
  if (!text) return;

  string = moth_pixmap_string_create (pixmap, text->string,
      text->flags & MOTH_DRAW_VERTICAL);
  moth_pixmap_draw_primitive_string (pixmap, text->location, string,
      text->color, 
      text->flags & MOTH_CENTER_VERTICAL,
      text->flags & MOTH_CENTER_HORIZONTAL );
  moth_pixmap_string_destroy (string);
}



/* also adjusts for coordinate and viewport differences */
INTERNAL
ArtPoint
moth_point_art
(
  moth_pixmap pixmap,
  moth_point  point
)
{
  ArtPoint ap;
  ap.x = point.x;
  ap.y = point.y;
  ap.y = pixmap->height - ap.y;   /* invert y axis */
  return ap;
}



INTERNAL
void
moth_draw_svp
(
  moth_pixmap pixmap,
  ArtSVP      *path,
  moth_color  color
)
{
  art_rgb_svp_alpha (path, 0, 0,
      pixmap->width, pixmap->height,
      moth_color_art(color),
      pixmap->buffer, pixmap->rowstride, NULL);
}



/*--------------------------------------------------------*\
|                     main pixmap API                      |
\*--------------------------------------------------------*/

moth_pixmap
moth_pixmap_create ()
{
  moth_pixmap pixmap;
  int         error;
  pixmap = (moth_pixmap) malloc (sizeof(struct moth_pixmap_s));
  pixmap->width = 0;
  pixmap->height = 0;
  pixmap->rowstride = 0;
  pixmap->buffer = NULL;

  /* initialize the font service */
  pixmap->freetype = NULL;
  error = FT_Init_FreeType (&(pixmap->freetype));
  if (error)
    die ("couldn't initialize the font library");
  pixmap->face = NULL;
  error = FT_New_Face (pixmap->freetype,
      MOTH_FONT_DEFAULT,
      0, &(pixmap->face));
  if (error)
    die ("couldn't load font \"%s\"", MOTH_FONT_DEFAULT);
  
  error = FT_Set_Char_Size (pixmap->face, 0, MOTH_FONT_SIZE_DEFAULT*64,
      72, 72);
  if (error)
    die ("couldn't set the font size to \"%d\"", MOTH_FONT_SIZE_DEFAULT);

  return pixmap;
}



void
moth_pixmap_get_geometry
(
  moth_pixmap   pixmap,
  unsigned int  *width,
  unsigned int  *height
)
{
  if (!pixmap)
    return;
  if (width)
    *width = pixmap->width;
  if (height)
    *height = pixmap->height;
}



void
moth_pixmap_get_data
(
  moth_pixmap   pixmap,
  unsigned char **data,
  unsigned int  *rowspan,
  unsigned int  *bytes_per_pixel
)
{
  if (!pixmap)
    return;
  if (data)
    *data = pixmap->buffer;
  if (rowspan)
    *rowspan = pixmap->rowstride;
  if (bytes_per_pixel)
    *bytes_per_pixel = MOTH_BYTES_PER_PIXEL;
}



void
moth_pixmap_destroy
(
  moth_pixmap pixmap
)
{
  if (!pixmap)
    return;
  
  if (pixmap->buffer)
    art_free (pixmap->buffer);

  if (pixmap->face)
    FT_Done_Face (pixmap->face);

  if (pixmap->freetype)
    FT_Done_FreeType (pixmap->freetype);

  free (pixmap);
}



/*--------------------------------------------------------*\
|                    vector format API                     |
\*--------------------------------------------------------*/

void
moth_pixmap_start_image
(
  moth_pixmap   pixmap,
  unsigned int  width,
  unsigned int  height,
  moth_color    background,
  moth_color    border_color,
  int unsigned  border_size
)
{
  moth_point    p, q, r, s;

  pixmap->width = width;
  pixmap->height = height;
  pixmap->rowstride = width * MOTH_BYTES_PER_PIXEL;
  pixmap->buffer = art_new (art_u8, height*(pixmap->rowstride));
  /* 
   * Apparently, art_rgb_run_alpha() doesn't initialize all the pixels, so we
   * make sure to.
   */
  bzero (pixmap->buffer, height*(pixmap->rowstride));
  art_rgb_run_alpha (pixmap->buffer, 0xFF, 0xFF, 0xFF, 0xFF, width*height);
  if (background) {
    art_rgb_run_alpha (pixmap->buffer, background->red, background->green,
        background->blue, background->alpha, width*height);
  }

  /* draw border */
  p.x = ((double) border_size) / 2.0;
  p.y = ((double) border_size) / 2.0;
  q.x = p.x;
  q.y = height - p.y;
  r.x = width  - p.x;
  r.y = height - p.y;
  s.x = width  - p.x;
  s.y = p.y;

  moth_pixmap_draw_primitive_line (pixmap, p, q, border_size,
      border_color);
  moth_pixmap_draw_primitive_line (pixmap, q, r, border_size,
      border_color);
  moth_pixmap_draw_primitive_line (pixmap, r, s, border_size,
      border_color);
  moth_pixmap_draw_primitive_line (pixmap, s, p, border_size,
      border_color);
}



void
moth_pixmap_draw_canvas
(
  moth_pixmap pixmap,
  moth_color  color,
  moth_point  bottom_left,
  moth_point  top_right
)
{
  moth_point  top_left, bottom_right;
  ArtPoint    art_bl, art_tl, art_tr, art_br;
  ArtVpath    *box;
  ArtSVP      *stroke;

  top_left.x = bottom_left.x;
  top_left.y = top_right.y;
  bottom_right.x = top_right.x;
  bottom_right.y = bottom_left.y;

  art_bl = moth_point_art (pixmap, bottom_left);
  art_tl = moth_point_art (pixmap, top_left);
  art_tr = moth_point_art (pixmap, top_right);
  art_br = moth_point_art (pixmap, bottom_right);

  box = art_new (ArtVpath, 6);
  box[0].code = ART_MOVETO;
  box[0].x = art_br.x + 0.5;
  box[0].y = art_br.y - 0.5;
  box[1].code = ART_LINETO;
  box[1].x = art_tr.x + 0.5;
  box[1].y = art_tr.y - 0.5;
  box[2].code = ART_LINETO;
  box[2].x = art_tl.x + 0.5;
  box[2].y = art_tl.y - 0.5;
  box[3].code = ART_LINETO;
  box[3].x = art_bl.x + 0.5;
  box[3].y = art_bl.y - 0.5;
  box[4].code = ART_LINETO;
  box[4].x = box[0].x;
  box[4].y = box[0].y;
  box[5].code = ART_END;
  stroke = art_svp_from_vpath (box);
  moth_draw_svp (pixmap, stroke, color);
  art_free (stroke);
  art_free (box);
}



void
moth_pixmap_draw_watermark
(
  moth_pixmap         pixmap,
  moth_format_text    watermark,
  moth_pixmap_string  string
)
{
  moth_pixmap_draw_primitive_string (pixmap, watermark->location, string, 
      watermark->color, 0, 0);
}



void
moth_pixmap_draw_title
(
  moth_pixmap         pixmap,
  moth_format_text    title,
  moth_pixmap_string  string
)
{
  moth_pixmap_draw_primitive_string (pixmap, title->location, string, 
      title->color, 0, 0);
}



void
moth_pixmap_draw_axes
(
  moth_pixmap       pixmap,
  moth_format_axis  axis
)
{
  moth_format_tick  tick;

  if (!pixmap || !axis)
    return;

  moth_pixmap_draw_primitive_line (pixmap, axis->line->location,
      axis->line->next->location, axis->line->size, axis->color);

  for (tick=axis->ticks; tick; tick=tick->next) {
    moth_pixmap_draw_primitive_line (pixmap, tick->mark->location,
        tick->mark->next->location, tick->mark->size, tick->color);
    if (tick->text)
      moth_pixmap_draw_format_text (pixmap, tick->text);
  }

  if (axis->title)
    moth_pixmap_draw_format_text (pixmap, axis->title);

  if (axis->next)
    moth_pixmap_draw_axes (pixmap, axis->next);
}



void
moth_pixmap_draw_area
(
  moth_pixmap             pixmap,
  moth_color              color,
  moth_format_layer_piece areas,
  moth_format_legend      legend
)
{
  moth_format_layer_piece area;

  if (!pixmap || !areas)
    return;

  for ( area = areas;
        area;
        area = area->next )
  {
    size_t              p;
    moth_format_point   point;
    ArtVpath            *box;
    ArtSVP              *stroke, *uncrossed, *rewound;

    if (!area->points)
      continue;

    /* count points */
    p = 0;
    for ( point = area->points;
          point;
          point = point->next )
    {
      ++p;
    }

    /* build libart data structure */
    box = art_new (ArtVpath, p+2);
    p = 0;
    for ( point = area->points;
          point;
          point = point->next )
    {
      ArtPoint art;

      if (isnan(point->location.x) || isnan(point->location.y))
        continue;

      art = moth_point_art (pixmap, point->location);

      if (p == 0)
        box[p].code = ART_MOVETO;
      else
        box[p].code = ART_LINETO;
      box[p].x = art.x;
      box[p].y = art.y;

      ++p;
    }

    /* close path */
    box[p].code = ART_LINETO;
    box[p].x = box[0].x;
    box[p].y = box[0].y;
    ++p;
    box[p].code = ART_END;

    /* draw */
    /* 
     * It's possible for the lines to cross, so we need to handle that
     * explicitely.  Luckily, the libart API has a call to do this.
     *
     * In addition, we want to make sure that all areas have their points given
     * in a counter-clockwise fasion, so that libart will draw them filled.
     */
    stroke = art_svp_from_vpath (box);
    uncrossed = art_svp_uncross (stroke);
    rewound = art_svp_rewind_uncrossed (uncrossed, ART_WIND_RULE_NONZERO);
    moth_draw_svp (pixmap, rewound, color);
    art_free (rewound);
    art_free (uncrossed);
    art_free (stroke);
    art_free (box);
  }

  /* draw legend */
  if (legend) {
    ArtPoint  bottom_left, top_right;
    ArtVpath  *box;
    ArtSVP    *stroke;

    bottom_left = moth_point_art (pixmap, legend->swatch[0]);
    top_right = moth_point_art (pixmap, legend->swatch[1]);

    box = art_new (ArtVpath, 6);
    box[0].code = ART_MOVETO;
    box[0].x = bottom_left.x;
    box[0].y = bottom_left.y;
    box[1].code = ART_LINETO;
    box[1].x = top_right.x;
    box[1].y = bottom_left.y;
    box[2].code = ART_LINETO;
    box[2].x = top_right.x;
    box[2].y = top_right.y;
    box[3].code = ART_LINETO;
    box[3].x = bottom_left.x;
    box[3].y = top_right.y;
    box[4].code = ART_LINETO;
    box[4].x = box[0].x;
    box[4].y = box[0].y;
    box[5].code = ART_END;
    stroke = art_svp_from_vpath (box);
    moth_draw_svp (pixmap, stroke, color);
    art_free (stroke);
    art_free (box);

    if (legend->title)
      moth_pixmap_draw_format_text (pixmap, legend->title);
  }

}



void
moth_pixmap_draw_grid
(
  moth_pixmap             pixmap,
  moth_color              color,
  moth_format_layer_piece lines,
  moth_format_legend      legend
)
{
  moth_format_layer_piece line;

  if (!pixmap || !lines)
    return;

  for ( line = lines;
        line;
        line = line->next )
  {
    if (!line->points || !line->points->next)
      continue;

    moth_pixmap_draw_primitive_line (pixmap, line->points->location,
        line->points->next->location, line->points->size, color);
  }

  /* draw legend */
  /* 
   * We really -shouldn't- get a legend for grids, but we'll draw one if
   * we're given one, just in case that policy changes :) 
   */
  if (legend) {
    moth_point left, right;

    /* draw a centered, horizontal line within the bounding rectangle */
    left.x = legend->swatch[0].x + 0.5;
    left.y = (legend->swatch[0].y + legend->swatch[1].y) / 2.0;
    right.x = legend->swatch[1].x - 0.5;
    right.y = left.y;
    moth_pixmap_draw_primitive_line (pixmap, left, right, 1.0, color);

    if (legend->title)
      moth_pixmap_draw_format_text (pixmap, legend->title);
  }

}



void
moth_pixmap_draw_line
(
  moth_pixmap             pixmap,
  moth_color              color,
  moth_format_layer_piece lines,
  moth_format_legend      legend
)
{
  moth_format_layer_piece line;

  if (!pixmap || !lines)
    return;

  for ( line = lines;
        line;
        line = line->next )
  {
    moth_format_point point, last_point;
    ArtSVP            *stroke;

    stroke = NULL;
    last_point = NULL;
    for ( point = line->points;
          point;
          point = point->next )
    {
      if (last_point) {
        ArtPoint  a_start, a_end;
        ArtVpath  *line_part;
        ArtSVP    *stroke_part;

        if (  isnan(last_point->location.x) || isnan(last_point->location.y)
           || isnan(point->location.x)      || isnan(point->location.y)
           )
        {
          last_point = point;
          continue;
        }
        
        a_start = moth_point_art (pixmap, last_point->location);
        a_end = moth_point_art (pixmap, point->location);

        line_part = art_new (ArtVpath, 3);
        line_part[0].code = ART_MOVETO_OPEN;
        line_part[0].x = a_start.x;
        line_part[0].y = a_start.y;
        line_part[1].code = ART_LINETO;
        line_part[1].x = a_end.x;
        line_part[1].y = a_end.y;
        line_part[2].code = ART_END;
        stroke_part = art_svp_vpath_stroke (line_part,
            ART_PATH_STROKE_JOIN_ROUND, ART_PATH_STROKE_CAP_ROUND,
            point->size, point->size, 1.0);

        if (stroke) {
          ArtSVP *new_stroke;
          new_stroke = art_svp_union (stroke, stroke_part);
          art_free (stroke);
          art_free (stroke_part);
          stroke = new_stroke;
        }
        else {
          stroke = stroke_part;
        }

        art_free (line_part);
      } /* last_point */

      last_point = point;
    } /* foreach point */

    if (stroke) {
      moth_draw_svp (pixmap, stroke, color);
      art_free (stroke);
    }

  } /* foreach line */

  /* draw legend */
  if (legend) {
    moth_point left, right;

    /* draw a centered, horizontal line within the bounding rectangle */
    left.x = legend->swatch[0].x + 0.5;
    left.y = (legend->swatch[0].y + legend->swatch[1].y) / 2.0;
    right.x = legend->swatch[1].x - 0.5;
    right.y = left.y;
    moth_pixmap_draw_primitive_line (pixmap, left, right, 1.0, color);

    if (legend->title)
      moth_pixmap_draw_format_text (pixmap, legend->title);
  }

}



void
moth_pixmap_draw_points
(
  moth_pixmap               pixmap,
  moth_color                color,
  moth_format_layer_points  points,
  moth_format_legend        legend
)
{
  moth_format_point point;

  if (!pixmap || !points)
    return;

  for ( point = points->points;
        point;
        point = point->next )
  {
    moth_pixmap_draw_primitive_circle (pixmap, point->location, point->size,
        points->filled, color);
  }

  /* draw legend */
  if (legend) {
    moth_point  center;
    double      radius;

    center.x = (legend->swatch[0].x + legend->swatch[1].x) / 2.0;
    center.y = (legend->swatch[0].y + legend->swatch[1].y) / 2.0;
    radius = (legend->swatch[1].x - legend->swatch[0].x) / 2.0;
    if (!points->filled) {
      /* keep outside of circle stroke inside the swatch bounding box */
      radius -= 0.5;
    }
    moth_pixmap_draw_primitive_circle (pixmap, center, radius,
        points->filled, color);

    if (legend->title)
      moth_pixmap_draw_format_text (pixmap, legend->title);
  }

}



void
moth_pixmap_draw_segments
(
  moth_pixmap             pixmap,
  moth_color              color,
  moth_format_layer_piece lines,
  moth_format_legend      legend
)
{
  moth_format_layer_piece line;

  if (!pixmap || !lines)
    return;

  for ( line = lines;
        line;
        line = line->next )
  {
    if (!line->points || !line->points->next)
      continue;

    moth_pixmap_draw_primitive_line (pixmap, line->points->location,
        line->points->next->location, line->points->size, color);
  }

  /* draw legend */
  if (legend) {
    moth_point left, right;

    /* draw a centered, horizontal line within the bounding rectangle */
    left.x = legend->swatch[0].x + 0.5;
    left.y = (legend->swatch[0].y + legend->swatch[1].y) / 2.0;
    right.x = legend->swatch[1].x - 0.5;
    right.y = left.y;
    moth_pixmap_draw_primitive_line (pixmap, left, right, 1.0, color);

    if (legend->title)
      moth_pixmap_draw_format_text (pixmap, legend->title);
  }

}



void
moth_pixmap_finish_image
(
  moth_pixmap pixmap
)
{
  /* nothing to do */
}



/*--------------------------------------------------------*\
|                    pixmap string API                     |
\*--------------------------------------------------------*/

moth_pixmap_string
moth_pixmap_string_create
(
  moth_pixmap   pixmap,
  const char    *text,
  int           vertical
)
{
  FT_GlyphSlot      slot = pixmap->face->glyph;
  FT_Bool           use_kerning;
  FT_UInt           previous, glyph_index;
  FT_Matrix         matrix;
  FT_Vector         pen;      /* distance from baseline origin, in 26.6 */
  FT_BBox           bbox;     /* bounding box of string */
  moth_pixmap_string string;
  moth_pixmap_char  glyph;
  unsigned int      n;
  int               error;

  if (!text)
    return NULL;

  /* normalize to zero or one */
  vertical = vertical ? 1 : 0;

  string = (moth_pixmap_string) malloc(sizeof(struct moth_pixmap_string_s));
  string->width = 0;
  string->height = 0;
  string->count = strlen (text);
  string->glyphs = (moth_pixmap_char) calloc (string->count,
      sizeof(struct moth_pixmap_char_s));
  string->glyph_count = 0;

  matrix.xx = (FT_Fixed)( cos(M_PI*(vertical)*0.5)*0x10000);
  matrix.xy = (FT_Fixed)(-sin(M_PI*(vertical)*0.5)*0x10000);
  matrix.yx = (FT_Fixed)( sin(M_PI*(vertical)*0.5)*0x10000);
  matrix.yy = (FT_Fixed)( cos(M_PI*(vertical)*0.5)*0x10000);

  pen.x = 0;
  pen.y = 0;
  bbox.xMin = bbox.yMin = 32000;
  bbox.xMax = bbox.yMax = -32000;

  use_kerning = FT_HAS_KERNING(pixmap->face);
  previous    = 0;
  glyph       = string->glyphs;
  for (n=0; n<string->count; ++n, ++glyph) {
    FT_Vector   glyph_pos;
    FT_BBox     glyph_bbox;

    glyph_index = FT_Get_Char_Index (pixmap->face, text[n]);
    glyph->image = NULL;

    if (use_kerning && previous && glyph_index) {
      FT_Vector kerning;
      FT_Get_Kerning (pixmap->face, previous, glyph_index,
          ft_kerning_default, &kerning);
      pen.x += kerning.x;
      pen.y += kerning.y;
    }

    /* save pen position */
    glyph_pos.x = pen.x;
    glyph_pos.y = pen.y;

    /* load the glyph image (in its native format) */
    /* for now, we take a monochrome glyph bitmap */
    error = FT_Load_Glyph (pixmap->face, glyph_index, FT_LOAD_DEFAULT);
    if (error) {
      warn ("couldn't load glyph");
      continue;
    }
    error = FT_Get_Glyph (slot, &glyph->image);
    if (error) {
      warn ("couldn't get glyph");
      continue;
    }

    pen.x += slot->advance.x;
    pen.y += slot->advance.y;

    /* rotate bitmap */
    FT_Vector_Transform (&glyph_pos, &matrix);
    error = FT_Glyph_Transform (glyph->image, &matrix, &glyph_pos);
    if (error) {
      warn ("couldn't transform glyph");
      continue;
    }

    /* get bitmap of glyph */
    error = FT_Glyph_To_Bitmap (&glyph->image, ft_render_mode_normal, 0, 1);
    if (error) {
      warn ("couldn't convert glyph to bitmap");
      continue;
    }

    FT_Glyph_Get_CBox (glyph->image, ft_glyph_bbox_pixels, &glyph_bbox);
    if (glyph_bbox.xMin < bbox.xMin)    bbox.xMin = glyph_bbox.xMin;
    if (glyph_bbox.xMax > bbox.xMax)    bbox.xMax = glyph_bbox.xMax;
    if (glyph_bbox.yMin < bbox.yMin)    bbox.yMin = glyph_bbox.yMin;
    if (glyph_bbox.yMax > bbox.yMax)    bbox.yMax = glyph_bbox.yMax;

    previous = glyph_index;
    string->glyph_count++;
  }

  /* 
   * Yeah, the "100" below is a magic number.  It works perfectly, but
   * -why- it works perfectly I don't know.  The freetype documentation
   * says that the ascender is given in "font units", but doesn't really
   * clearly say how to find that value.  (Dividing by units_per_EM is
   * not the answer.)
   */
  if (vertical) {
    string->width = pixmap->face->ascender / 100;
    string->height = bbox.yMax - bbox.yMin;
  }
  else {
    string->width = bbox.xMax - bbox.xMin;
    string->height = pixmap->face->ascender / 100;
  }

  return string;
}



void
moth_pixmap_string_destroy
(
  moth_pixmap_string string
)
{
  if (!string) return;
  if (string->glyphs)
    free (string->glyphs);
  free (string);
}



