/*
 * $Id: formats.c,v 1.9 2003/06/05 20:33:05 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See formats.dev for implementation notes.
 *
 */


#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "file.h"
#include "formats.h"
#include "formats/definitions.h"



/*--------------------------------------------------------*\
|                      data structures                     |
\*--------------------------------------------------------*/

struct moth_format_s {
  char                    *name;
  moth_format_definition  definition;
  void                    *specs;       /* format specifics */
  moth_format             next;         /* linked list */
};



/*--------------------------------------------------------*\
|                         formats                          |
\*--------------------------------------------------------*/

#if defined(HAVE_JPEGLIB_H) && defined(HAVE_LIBJPEG)
  #define MOTH_HAVE_JPEG
  #include "formats/jpeg.c"
#endif

#if defined(HAVE_PNG_H) && defined(HAVE_LIBPNG)
  #define MOTH_HAVE_PNG
  #include "formats/png.c"
#endif



/*--------------------------------------------------------*\
|                    utility functions                     |
\*--------------------------------------------------------*/

/* returns malloc'd filename */
INTERNAL
char *
moth_format_find_file
(
  moth_format format,
  const char  *base
)
{
  size_t  size;
  char    *fullname;
  char    *found_file;

  /* append suffix to filename */
  size = strlen(base) + 1/* period */ + strlen(format->name);
  fullname = (char *) malloc (size+1);
  snprintf (fullname, size+1, "%s.%s", base, format->name);

  found_file = moth_file_find (fullname);
  if (!found_file)
    die ("couldn't find file \"%s\"", base);

  free (fullname);
  return found_file;
}



/*--------------------------------------------------------*\
|                        common API                        |
\*--------------------------------------------------------*/

moth_format
moth_format_create
(
  const char *name
)
{
  moth_format format;
  const char  *comma;

  format = (moth_format) malloc (sizeof(struct moth_format_s));
  format->name = NULL;
  format->definition = NULL;
  format->specs = NULL;
  format->next = NULL;

  if (( comma = strchr(name, MOTH_LIST_SEPARATOR) )) {
    size_t  size;
    size = comma - name;
    format->name = (char *) malloc (size+1);
    memcpy (format->name, name, size);
    format->name[size] = '\0';
    format->next = moth_format_create (comma+1);
  }
  else {
    format->name = strdup (name);
  }

  /* set definition */
  format->definition = NULL;
  if (0==strcasecmp("png",format->name)) {
#ifdef MOTH_HAVE_PNG
    format->definition = &moth_format_definition_png;
#else
    warn ("moth not compiled with support for png");
#endif
  }
  else if ( 0==strcasecmp("jpeg",format->name)
            || 0==strcasecmp("jpg",format->name))
  {
#ifdef MOTH_HAVE_JPEG
    format->definition = &moth_format_definition_jpeg;
#else
    warn ("moth not compiled with support for jpeg");
#endif
  }
  else {
    warn ("unknown file format \"%s\"", format->name);
  }

  return format;
}



unsigned int
moth_format_is_pixmap
(
  moth_format format
)
{
  if (!format->definition)
    return 0;
  if (format->definition->is_pixmap)
    return 1;
  if (format->next)
    return moth_format_is_pixmap (format->next);
  return 0;
}



void
moth_format_dump
(
  moth_format format
)
{
  int first;
  first = 1;
  while (format) {
    if (!first)
      fprintf (stdout, "%c", MOTH_LIST_SEPARATOR);
    fprintf (stdout, "%s", format->name);
    format = format->next;
  }
}



void
moth_format_destroy
(
  moth_format format
)
{
  if (format->next)
    moth_format_destroy (format->next);
  if (format->name)
    free (format->name);
  free (format);
}



/*--------------------------------------------------------*\
|                      pixmap formats                      |
\*--------------------------------------------------------*/

void
moth_format_save_pixmap
(
  moth_format format,
  const char  *filename,
  moth_pixmap pixmap
)
{
  if (!format)
    return;

  if (format->definition && format->definition->save_pixmap) {
    char    *found_file;
    found_file = moth_format_find_file (format, filename);
    format->definition->save_pixmap (format, found_file, pixmap);
    free (found_file);
  }

  if (format->next)
    moth_format_save_pixmap (format->next, filename , pixmap);
}



/*--------------------------------------------------------*\
|                      vector formats                      |
\*--------------------------------------------------------*/

void
moth_format_start_image
(
  moth_format   format,
  const char    *filename,
  unsigned int  width,
  unsigned int  height,
  moth_color    background,
  moth_color    border,
  int unsigned  border_size
)
{
  if (!format)
    return;

  if (format->definition && format->definition->start_image) {
    char    *found_file;
    found_file = moth_format_find_file (format, filename);
    format->definition->start_image (format, found_file, width, height,
        background, border, border_size);
    free (found_file);
  }

  if (format->next)
    moth_format_start_image (format->next, filename, width, height, background,
        border, border_size);
}



void
moth_format_draw_title
(
  moth_format       format,
  moth_format_text  title
)
{
  if (format->definition && format->definition->draw_title)
    format->definition->draw_title (format, title);

  if (format->next)
    moth_format_draw_title (format->next, title);
}



void
moth_format_draw_watermark
(
  moth_format       format,
  moth_format_text  watermark
)
{
  if (format->definition && format->definition->draw_watermark)
    format->definition->draw_watermark (format, watermark);

  if (format->next)
    moth_format_draw_watermark (format->next, watermark);
}



void
moth_format_draw_canvas
(
  moth_format format,
  moth_color  color,
  moth_point  bottom_left,
  moth_point  top_right
)
{
  if (format->definition && format->definition->draw_canvas)
    format->definition->draw_canvas (format, color, bottom_left, top_right);

  if (format->next)
    moth_format_draw_canvas (format->next, color, bottom_left, top_right);
}



void
moth_format_draw_axes
(
  moth_format       format,
  moth_format_axis  axes
)
{
  if (format->definition && format->definition->draw_axes)
    format->definition->draw_axes (format, axes);

  if (format->next)
    moth_format_draw_axes (format->next, axes);
}



void
moth_format_draw_area
(
  moth_format             format,
  moth_color              color,
  moth_format_layer_piece areas,
  moth_format_legend      legend
)
{
  if (format->definition && format->definition->draw_area)
    format->definition->draw_area (format, color, areas, legend);

  if (format->next)
    moth_format_draw_area (format->next, color, areas, legend);
}



void
moth_format_draw_grid
(
  moth_format             format,
  moth_color              color,
  moth_format_layer_piece lines,
  moth_format_legend      legend
)
{
  if (format->definition && format->definition->draw_grid)
    format->definition->draw_grid (format, color, lines, legend);

  if (format->next)
    moth_format_draw_grid (format->next, color, lines, legend);
}



void
moth_format_draw_line
(
  moth_format             format,
  moth_color              color,
  moth_format_layer_piece lines,
  moth_format_legend      legend
)
{
  if (format->definition && format->definition->draw_line)
    format->definition->draw_line (format, color, lines, legend);

  if (format->next)
    moth_format_draw_line (format->next, color, lines, legend);
}



void
moth_format_draw_points
(
  moth_format               format,
  moth_color                color,
  moth_format_layer_points  points,
  moth_format_legend        legend
)
{
  if (format->definition && format->definition->draw_points)
    format->definition->draw_points (format, color, points, legend);

  if (format->next)
    moth_format_draw_points (format->next, color, points, legend);
}



void
moth_format_draw_segments
(
  moth_format             format,
  moth_color              color,
  moth_format_layer_piece lines,
  moth_format_legend      legend
)
{
  if (format->definition && format->definition->draw_segments)
    format->definition->draw_segments (format, color, lines, legend);

  if (format->next)
    moth_format_draw_segments (format->next, color, lines, legend);
}



void
moth_format_finish_image
(
  moth_format format
)
{
  if (format->definition && format->definition->finish_image)
    format->definition->finish_image (format);

  if (format->next)
    moth_format_finish_image (format->next);
}



