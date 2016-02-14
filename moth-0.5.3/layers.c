/*
 * $Id: layers.c,v 1.32 2004/03/17 04:15:09 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See layers.dev for implementation notes.
 *
 */


#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "axes.h"
#include "columns.h"
#include "file.h"
#include "image.h"
#include "layers.h"
#include "options.h"
#include "parser.h"
#include "pixmap.h"



/*--------------------------------------------------------*\
|                      data structures                     |
\*--------------------------------------------------------*/

typedef struct moth_layer_grid_specs_s  *moth_layer_grid_specs;
struct moth_layer_grid_specs_s {
  double    x_modulus_value;
  char      x_modulus_modifier;
  char      x_modulus_unit;
  double    x_offset_value;
  char      x_offset_modifier;
  char      x_offset_unit;
  double    y_modulus_value;
  char      y_modulus_modifier;
  char      y_modulus_unit;
  double    y_offset_value;
  char      y_offset_modifier;
  char      y_offset_unit;
};



typedef struct moth_layer_line_specs_s  *moth_layer_line_specs;
struct moth_layer_line_specs_s {
  moth_column width;
};



typedef struct moth_layer_points_specs_s  *moth_layer_points_specs;
struct moth_layer_points_specs_s {
  moth_column radius;
  int         filled;
};



typedef struct moth_layer_segments_specs_s  *moth_layer_segments_specs;
struct moth_layer_segments_specs_s {
  moth_column width;
};



/*--------------------------------------------------------*\
|                      area functions                      |
\*--------------------------------------------------------*/

INTERNAL
void
moth_layer_create_area
(
  moth_layer        layer,
  const char        **attributes
)
{
  int a;

  /*
   * We don't need to assign anything to the specifics, since the generic
   * attributes are sufficient to describe an <area>.
   */
  layer->specifics = NULL;

  /* ignore common attributes */
  for (a=0; attributes[a]; a+=2) {
    const char *attname = attributes[a];
    const char *attvalue = attributes[a+1];

    if ( 0==strcasecmp("title",attname)
      || 0==strcasecmp("color",attname) 
      || 0==strcasecmp("x0",attname)
      || 0==strcasecmp("y0",attname)
      || 0==strcasecmp("x1",attname)
      || 0==strcasecmp("y1",attname)
      || 0==strcasecmp("x-axis",attname)
      || 0==strcasecmp("y-axis",attname) ) {
      /* do nothing.  they are already taken care of */
    }
    else {
      warn ("unknown <area> attribute %s=\"%s\"", attname, attvalue);
    }
  }

  /* required and default attributes */
  if (! layer->x) {
    layer->x = moth_column_store_get (runtime->file->columns, "x0");
    if (! layer->x)
      layer->x = moth_column_store_get (runtime->file->columns, "x");
    if (! layer->x)
      die ("missing <area> attribute \"x0\"");
  }
  if (! layer->y) {
    layer->y = moth_column_store_get (runtime->file->columns, "y0");
    if (! layer->y)
      layer->y = moth_column_store_get (runtime->file->columns, "y");
    if (! layer->y)
      die ("missing <area> attribute \"y0\"");
  }
  if (! layer->x1) {
    layer->x1 = moth_column_store_get (runtime->file->columns, "x1");
    if (! layer->x1)
      layer->x1 = moth_column_store_get (runtime->file->columns, "x");
    if (! layer->x1)
      die ("missing <area> attribute \"x1\"");
  }
  if (! layer->y1) {
    layer->y1 = moth_column_store_get (runtime->file->columns, "y1");
    if (! layer->y1)
      layer->y1 = moth_column_store_get (runtime->file->columns, "y");
    if (! layer->y1)
      die ("missing <area> attribute \"y1\"");
  }
  if (MOTH_LOCATION_UNKNOWN == layer->x_axis)
    die ("missing <area> attribute \"x-axis\"");
  if (MOTH_LOCATION_UNKNOWN == layer->y_axis)
    die ("missing <area> attribute \"y-axis\"");
}



/*
 * Once we know the area is closed, we need to draw the points on the
 * bottom of the area, in reverse order.
 */
INTERNAL
void
moth_layer_draw_area_back_stroke
(
  moth_image        image,
  moth_format_point last_point,
  moth_column       x_column,
  moth_column       y_column,
  int               current_row,
  int               start_row
)
{
  int row;

  for (row=current_row; row>=start_row; --row) {
    moth_format_point new_point;

    new_point = (moth_format_point)
      malloc (sizeof(struct moth_format_point_s));
    new_point->location.x = moth_column_value (x_column, row);
    new_point->location.y = moth_column_value (y_column, row);
    new_point->size = NAN;
    new_point->next = NULL;
    moth_image_get_viewport_value (image, &new_point->location.x,
        &new_point->location.y);

    last_point->next = new_point;
    last_point = new_point;
  }
}



INTERNAL
void
moth_layer_draw_area
(
  moth_layer          layer,
  moth_image          image,
  moth_format_legend  legend,
  moth_format         formats,
  moth_pixmap         pixmap
)
{
  int           x0_count, x1_count, y0_count, y1_count;
  int           row, current_area_start;

  moth_point                p0, p1, last_p0, last_p1;
  moth_format_layer_piece   areas, current_area, last_area;
  moth_format_point         last_point;

  x0_count = moth_column_size (layer->x);
  x1_count = moth_column_size (layer->x1);
  y0_count = moth_column_size (layer->y);
  y1_count = moth_column_size (layer->y1);

  areas = last_area = current_area = NULL;
  current_area_start = 0;
  last_point = NULL;

  last_p0.x = last_p0.y = NAN;
  last_p1.x = last_p1.y = NAN;
  for ( row = 0;
        row < x0_count || row < x1_count ||
        row < y0_count || row < y1_count ;
        ++row, last_p0 = p0, last_p1 = p1
      )
  {
    moth_format_point new_point;

    p0.x = moth_column_value (layer->x, row);
    p0.y = moth_column_value (layer->y, row);
    p1.x = moth_column_value (layer->x1, row);
    p1.y = moth_column_value (layer->y1, row);

    /* 
     * Skip the current points.  Once we know we have two good concurrent sets
     * of points, we'll start building things in ernest (and take care of the
     * current set of points we're skipping right now).
     */
    if (  isnan(last_p0.x) || isnan(last_p0.y)
       || isnan(last_p1.x) || isnan(last_p1.y)
       )
    {
      continue;
    }

    if (isnan(p0.x) || isnan(p0.y) || isnan(p1.x) || isnan(p1.y)) {
      /* close current area */
      if (current_area) {
        moth_layer_draw_area_back_stroke (image, last_point, layer->x,
            layer->y, row-1, current_area_start);
        current_area = NULL;
        last_point = NULL;
      }
      continue;
    }

    /* create a new area */
    if (!current_area) {
      /* We subtract 1 because we know the last row was not NaN.  */
      current_area_start = row - 1;
      current_area = (moth_format_layer_piece)
        malloc (sizeof(struct moth_format_layer_piece_s));
      current_area->points = NULL;
      current_area->next = NULL;

      /* add to list of areas */
      if (last_area)
        last_area->next = current_area;
      else
        areas = current_area;
      last_area = current_area;

      /* add last point to new area */
      moth_image_get_viewport_value (image, &last_p1.x, &last_p1.y);
      new_point = (moth_format_point)
        malloc (sizeof(struct moth_format_point_s));
      new_point->location.x = last_p1.x;
      new_point->location.y = last_p1.y;
      new_point->size = NAN;
      new_point->next = NULL;
      current_area->points = new_point;
      last_point = new_point;
    }

    /* add point to current area */
    moth_image_get_viewport_value (image, &p1.x, &p1.y);
    new_point = (moth_format_point)
      malloc (sizeof(struct moth_format_point_s));
    new_point->location.x = p1.x;
    new_point->location.y = p1.y;
    new_point->size = NAN;
    new_point->next = NULL;
    last_point->next = new_point;
    last_point = new_point;

  } /* foreach row */

  if (current_area) {
    --row;    /* we went one too many, so adjust */
    moth_layer_draw_area_back_stroke (image, last_point, layer->x, layer->y,
        row, current_area_start);
    current_area = NULL;
    last_point = NULL;
  }

  /* nothing to do */
  if (!areas)
    return;

  /* draw */
  moth_format_draw_area (formats, layer->color, areas, legend);
  if (pixmap)
    moth_pixmap_draw_area (pixmap, layer->color, areas, legend);

  /* free data structure */
  current_area = areas;
  while (current_area) {
    moth_format_layer_piece doomed_area;
    moth_format_point       point;
    doomed_area = current_area;
    current_area = current_area->next;

    point = doomed_area->points;
    while (point) {
      moth_format_point doomed_point;
      doomed_point = point;
      point = point->next;
      free (doomed_point);
    }

    free (doomed_area);
  }
}



INTERNAL
void
moth_layer_dump_specifics_area
(
  moth_layer layer
)
{
  /* nothing to do, since areas don't have specifics */
}



/* 
 * Destroy just the specifics, everything else is taken care of.
 */
INTERNAL
void
moth_layer_destroy_area
(
  void  *specifics
)
{
  if (!specifics)
    return;
  free (specifics);
}



/*--------------------------------------------------------*\
|                      grid functions                      |
\*--------------------------------------------------------*/

INTERNAL
void
moth_layer_create_grid
(
  moth_layer        layer,
  const char        **attributes
)
{
  moth_layer_grid_specs grid;
  int a;

  grid = (moth_layer_grid_specs)
    malloc (sizeof(struct moth_layer_grid_specs_s));
  grid->x_modulus_value     = NAN;
  grid->x_modulus_modifier  = ' ';
  grid->x_modulus_unit      = ' ';
  grid->x_offset_value      = 0.0;
  grid->x_offset_modifier   = ' ';
  grid->x_offset_unit       = ' ';
  grid->y_modulus_value     = NAN;
  grid->y_modulus_modifier  = ' ';
  grid->y_modulus_unit      = ' ';
  grid->y_offset_value      = 0.0;
  grid->y_offset_modifier   = ' ';
  grid->y_offset_unit       = ' ';
  layer->specifics = grid;

  /* read attributes */
  for (a=0; attributes[a]; a+=2) {
    const char *attname = attributes[a];
    const char *attvalue = attributes[a+1];

    if (0==strcasecmp("x-modulus",attname)) {
      if ( ! parse_value(attvalue, &(grid->x_modulus_value),
            &(grid->x_modulus_modifier), &(grid->x_modulus_unit)))
      {
        grid->x_modulus_value = read_value (attvalue);
      }
      if (isnan(grid->x_modulus_value))
        die ("couldn't understand <grid> attribute %s=\"%s\"", attname,
            attvalue);
      /* negative numbers don't make sense for a modulus */
      grid->x_modulus_value = fabs (grid->x_modulus_value);
    }
    else if (0==strcasecmp("x-offset",attname)) {
      if ( ! parse_value(attvalue, &(grid->x_offset_value),
            &(grid->x_offset_modifier), &(grid->x_offset_unit)))
      {
        grid->x_offset_value = read_value (attvalue);
      }
      if (isnan(grid->x_offset_value))
        die ("couldn't understand <grid> attribute %s=\"%s\"", attname,
            attvalue);
    }
    else if (0==strcasecmp("y-modulus",attname)) {
      if ( ! parse_value(attvalue, &(grid->y_modulus_value),
            &(grid->y_modulus_modifier), &(grid->y_modulus_unit)))
      {
        grid->y_modulus_value = read_value (attvalue);
      }
      if (isnan(grid->y_modulus_value))
        die ("couldn't understand <grid> attribute %s=\"%s\"", attname,
            attvalue);
      /* negative numbers don't make sense for a modulus */
      grid->y_modulus_value = fabs (grid->y_modulus_value);
    }
    else if (0==strcasecmp("y-offset",attname)) {
      if ( ! parse_value(attvalue, &(grid->y_offset_value),
            &(grid->y_offset_modifier), &(grid->y_offset_unit)))
      {
        grid->y_offset_value = read_value (attvalue);
      }
      if (isnan(grid->y_offset_value))
        die ("couldn't understand <grid> attribute %s=\"%s\"", attname,
            attvalue);
    }

    /* ignore common attributes */
    else if ( 0==strcasecmp("color",attname) 
           || 0==strcasecmp("x-axis",attname)
           || 0==strcasecmp("y-axis",attname) ) {
      /* do nothing.  they are already taken care of */
    }
    else {
      warn ("unknown <grid> attribute %s=\"%s\"", attname, attvalue);
    }
  }

  /* required and default attributes */
  if (MOTH_LOCATION_UNKNOWN == layer->x_axis)
    die ("missing <grid> attribute \"x-axis\"");
  if (MOTH_LOCATION_UNKNOWN == layer->y_axis)
    die ("missing <grid> attribute \"y-axis\"");
  if (isnan(grid->x_modulus_value) && isnan(grid->y_modulus_value))
    die ("missing <grid> attribute \"x-modulus\" or \"y-modulus\"");
}



INTERNAL
void
moth_layer_draw_grid
(
  moth_layer          layer,
  moth_image          image,
  moth_format_legend  legend,
  moth_format         formats,
  moth_pixmap         pixmap
)
{
  moth_layer_grid_specs   specs;
  moth_axis               x_axis, y_axis;
  double                  value, x_mod_value, y_mod_value;
  moth_format_layer_piece lines, last_line, line;

  specs = (moth_layer_grid_specs) layer->specifics;
  x_axis = moth_image_axis (image, layer->x_axis);
  y_axis = moth_image_axis (image, layer->y_axis);
  lines = last_line = NULL;

  x_mod_value = NAN;
  if ( 'd' == specs->x_modulus_modifier ) {
    value = datetime_modulus (x_axis->min, specs->x_modulus_unit,
        specs->x_modulus_value);
  }
  else {
    x_mod_value = convert_value (specs->x_modulus_value,
        specs->x_modulus_modifier, specs->x_modulus_unit);
    value = x_mod_value * trunc(x_axis->min/x_mod_value);
  }
  if ( 'd' == specs->x_offset_modifier ) {
    value = datetime_add (value, specs->x_offset_unit, specs->x_offset_value);
  }
  else {
    value += convert_value (specs->x_offset_value, specs->x_offset_modifier,
        specs->x_offset_unit);
  }

  while (value <= x_axis->max) {
    moth_format_layer_piece new_line;

    /* 
     * A negative offset can cause us to draw one or more lines off the left
     * side of the canvas.  Skip those lines.
     */
    if (value < x_axis->min) {
      if ( 'd' == specs->x_modulus_modifier ) {
        value = datetime_add(value, specs->x_modulus_unit,
            specs->x_modulus_value);
      }
      else {
        value += x_mod_value;
      }
      continue;
    }

    new_line = (moth_format_layer_piece)
      malloc (sizeof(struct moth_format_layer_piece_s));
    new_line->points = NULL;
    new_line->next = NULL;

    new_line->points = (moth_format_point)
      malloc (sizeof(struct moth_format_point_s));
    new_line->points->location.x = value;
    new_line->points->location.y = y_axis->min;
    new_line->points->size = MOTH_LINE_WIDTH_DEFAULT;
    new_line->points->next = NULL;

    new_line->points->next = (moth_format_point)
      malloc (sizeof(struct moth_format_point_s));
    new_line->points->next->location.x = value;
    new_line->points->next->location.y = y_axis->max;
    new_line->points->next->size = MOTH_LINE_WIDTH_DEFAULT;
    new_line->points->next->next = NULL;

    moth_image_get_viewport_value (image,
        &new_line->points->location.x,
        &new_line->points->location.y);
    moth_image_get_viewport_value (image,
        &new_line->points->next->location.x,
        &new_line->points->next->location.y);

    if (last_line)
      last_line->next = new_line;
    else
      lines = new_line;

    last_line = new_line;
    if ( 'd' == specs->x_modulus_modifier ) {
      value = datetime_add(value, specs->x_modulus_unit,
          specs->x_modulus_value);
    }
    else {
      value += x_mod_value;
    }
  }

  y_mod_value = NAN;
  if ( 'd' == specs->y_modulus_modifier ) {
    value = datetime_modulus (y_axis->min, specs->y_modulus_unit,
        specs->y_modulus_value);
  }
  else {
    y_mod_value = convert_value (specs->y_modulus_value,
        specs->y_modulus_modifier, specs->y_modulus_unit);
    value = y_mod_value * trunc(y_axis->min/y_mod_value);
  }
  if ( 'd' == specs->y_offset_modifier ) {
    value = datetime_add (value, specs->y_offset_unit, specs->y_offset_value);
  }
  else {
    value += convert_value (specs->y_offset_value, specs->y_offset_modifier,
        specs->y_offset_unit);
  }

  while (value <= y_axis->max) {
    moth_format_layer_piece new_line;

    /* 
     * A negative offset can cause us to draw one or more lines off the bottom
     * of the canvas.  Skip those lines.
     */
    if (value < y_axis->min) {
      if ( 'd' == specs->y_modulus_modifier ) {
        value = datetime_add(value, specs->y_modulus_unit,
            specs->y_modulus_value);
      }
      else {
        value += y_mod_value;
      }
      continue;
    }

    new_line = (moth_format_layer_piece)
      malloc (sizeof(struct moth_format_layer_piece_s));
    new_line->points = NULL;
    new_line->next = NULL;

    new_line->points = (moth_format_point)
      malloc (sizeof(struct moth_format_point_s));
    new_line->points->location.x = x_axis->min;
    new_line->points->location.y = value;
    new_line->points->size = MOTH_LINE_WIDTH_DEFAULT;
    new_line->points->next = NULL;

    new_line->points->next = (moth_format_point)
      malloc (sizeof(struct moth_format_point_s));
    new_line->points->next->location.x = x_axis->max;
    new_line->points->next->location.y = value;
    new_line->points->next->size = MOTH_LINE_WIDTH_DEFAULT;
    new_line->points->next->next = NULL;

    moth_image_get_viewport_value (image,
        &new_line->points->location.x,
        &new_line->points->location.y);
    moth_image_get_viewport_value (image,
        &new_line->points->next->location.x,
        &new_line->points->next->location.y);

    if (last_line)
      last_line->next = new_line;
    else
      lines = new_line;

    last_line = new_line;
    if ( 'd' == specs->y_modulus_modifier ) {
      value = datetime_add(value, specs->y_modulus_unit,
          specs->y_modulus_value);
    }
    else {
      value += y_mod_value;
    }
  }

  /* draw */
  moth_format_draw_grid (formats, layer->color, lines, legend);
  if (pixmap)
    moth_pixmap_draw_grid (pixmap, layer->color, lines, legend);

  /* free data structures */
  line = lines;
  while (line) {
    moth_format_layer_piece doomed;
    doomed = line;
    line = line->next;

    free (doomed->points->next);
    free (doomed->points);
    free (doomed);
  }

}



INTERNAL
void
moth_layer_dump_specifics_grid
(
  moth_layer layer
)
{
  moth_layer_grid_specs specs;
  specs = (moth_layer_grid_specs) layer->specifics;

  if (!isnan(specs->x_modulus_value))
    fprintf (stdout, "\n        x-modulus=\"%g%c%c\"", specs->x_modulus_value,
        specs->x_modulus_modifier, specs->x_modulus_unit);
  if (!isnan(specs->x_offset_value))
    fprintf (stdout, "\n        x-offset=\"%g%c%c\"", specs->x_offset_value,
        specs->x_offset_modifier, specs->x_offset_unit);
  if (!isnan(specs->y_modulus_value))
    fprintf (stdout, "\n        y-modulus=\"%g%c%c\"", specs->y_modulus_value,
        specs->y_modulus_modifier, specs->y_modulus_unit);
  if (!isnan(specs->y_offset_value))
    fprintf (stdout, "\n        y-offset=\"%g%c%c\"", specs->y_offset_value,
        specs->y_offset_modifier, specs->y_offset_unit);
}



/* 
 * Destroy just the specifics, everything else is taken care of.
 */
INTERNAL
void
moth_layer_destroy_grid
(
  void  *specifics
)
{
  moth_layer_grid_specs grid;
  grid = (moth_layer_grid_specs) specifics;

  if (!grid)
    return;
  free (grid);
}



/*--------------------------------------------------------*\
|                      line functions                      |
\*--------------------------------------------------------*/

INTERNAL
void
moth_layer_create_line
(
  moth_layer        layer,
  const char        **attributes
)
{
  moth_layer_line_specs line;
  int a;

  line = (moth_layer_line_specs)
    malloc (sizeof(struct moth_layer_line_specs_s));
  line->width = NULL;
  layer->specifics = line;

  /* read attributes */
  for (a=0; attributes[a]; a+=2) {
    const char *attname = attributes[a];
    const char *attvalue = attributes[a+1];

    if (0==strcasecmp("width",attname)) {
      line->width = moth_column_store_get (runtime->file->columns, attvalue);
      if (!line->width)
        die ("couldn't understand <line> attribute %s=\"%s\"", attname,
            attvalue);
    }
    /* ignore common attributes */
    else if ( 0==strcasecmp("title",attname)
           || 0==strcasecmp("color",attname) 
           || 0==strcasecmp("x",attname)
           || 0==strcasecmp("y",attname)
           || 0==strcasecmp("x-axis",attname)
           || 0==strcasecmp("y-axis",attname) ) {
      /* do nothing.  they are already taken care of */
    }
    else {
      warn ("unknown <line> attribute %s=\"%s\"", attname, attvalue);
    }
  }

  /* required and default attributes */
  if (! layer->x) {
    layer->x = moth_column_store_get (runtime->file->columns, "x");
    if (! layer->x)
      die ("missing <line> attribute \"x\"");
  }
  if (! layer->y) {
    layer->y = moth_column_store_get (runtime->file->columns, "y");
    if (! layer->y)
      die ("missing <line> attribute \"y\"");
  }
  if (MOTH_LOCATION_UNKNOWN == layer->x_axis)
    die ("missing <line> attribute \"x-axis\"");
  if (MOTH_LOCATION_UNKNOWN == layer->y_axis)
    die ("missing <line> attribute \"y-axis\"");
}



INTERNAL
void
moth_layer_draw_line
(
  moth_layer          layer,
  moth_image          image,
  moth_format_legend  legend,
  moth_format         formats,
  moth_pixmap         pixmap
)
{
  moth_layer_line_specs specs;
  size_t        x_count, y_count;
  int           row;

  moth_format_layer_piece lines, current_piece, last_piece;
  moth_format_point       last_point;

  specs = (moth_layer_line_specs) layer->specifics;
  x_count = moth_column_size (layer->x);
  y_count = moth_column_size (layer->y);

  lines = current_piece = NULL;
  last_piece = NULL;
  last_point = NULL;
  for ( row = 0;
        row < x_count || row < y_count;
        ++row )
  {
    moth_format_point new_point;
    double            x, y, width;

    x = moth_column_value (layer->x, row);
    y = moth_column_value (layer->y, row);

    if (isnan(x) || isnan(y)) {
      /* close current piece, if there is one */
      if (current_piece) {
        current_piece = NULL;
        last_point = NULL;
      }
      continue;
    }

    if (!current_piece) {
      current_piece = (moth_format_layer_piece)
        malloc (sizeof(struct moth_format_layer_piece_s));
      current_piece->points = NULL;
      current_piece->next = NULL;

      /* add to list of pieces */
      if (last_piece)
        last_piece->next = current_piece;
      else
        lines = current_piece;
      last_piece = current_piece;
    }

    moth_image_get_viewport_value (image, &x, &y);
    width = moth_column_value (specs->width, row);
    if (isnan(width))
      width = MOTH_LINE_WIDTH_DEFAULT;

    /* add point to current piece */
    new_point = (moth_format_point)
      malloc (sizeof(struct moth_format_point_s));
    new_point->location.x = x;
    new_point->location.y = y;
    new_point->size = width;
    new_point->next = NULL;
    if (last_point)
      last_point->next = new_point;
    else
      current_piece->points = new_point;
    last_point = new_point;
  }

  /* nothing to do */
  if (!lines)
    return;

  moth_format_draw_line (formats, layer->color, lines, legend);
  if (pixmap)
    moth_pixmap_draw_line (pixmap, layer->color, lines, legend);

  /* delete data structures */
  current_piece = lines;
  while (current_piece) {
    moth_format_layer_piece doomed_piece;
    moth_format_point       point;
    doomed_piece = current_piece;
    current_piece = current_piece->next;

    point = doomed_piece->points;
    while (point) {
      moth_format_point doomed_point;
      doomed_point = point;
      point = point->next;
      free (doomed_point);
    }

    free (doomed_piece);
  }

}



INTERNAL
void
moth_layer_dump_specifics_line
(
  moth_layer layer
)
{
  moth_layer_line_specs specs;
  specs = (moth_layer_line_specs) layer->specifics;
  if (specs->width)
    fprintf (stdout, "\n        width=\"%s\"", moth_column_name(specs->width));
}



/* 
 * Destroy just the specifics, everything else is taken care of.
 */
INTERNAL
void
moth_layer_destroy_line
(
  void  *specifics
)
{
  if (!specifics)
    return;
  free (specifics);
}



/*--------------------------------------------------------*\
|                     points functions                     |
\*--------------------------------------------------------*/

INTERNAL
void
moth_layer_create_points
(
  moth_layer        layer,
  const char        **attributes
)
{
  moth_layer_points_specs points;
  int a;

  points = (moth_layer_points_specs)
    malloc (sizeof(struct moth_layer_points_specs_s));
  points->radius = NULL;
  points->filled = 0;
  layer->specifics = points;

  /* read attributes */
  for (a=0; attributes[a]; a+=2) {
    const char *attname = attributes[a];
    const char *attvalue = attributes[a+1];
    if (0==strcasecmp("radius",attname)) {
      points->radius = moth_column_store_get (runtime->file->columns,
                          attvalue);
      if (!points->radius)
        die ("couldn't understand <points> attribute %s=\"%s\"", attname,
            attvalue);
    }
    else if (0==strcasecmp("filled",attname)) {
      if (   0==strcasecmp("on",attvalue)
          || 0==strcasecmp("yes",attvalue)
          || 0==strcasecmp("true",attvalue)
          || 0==strcasecmp("1",attvalue) )
        points->filled = 1;
    }
    /* ignore common attributes */
    else if ( 0==strcasecmp("title",attname)
           || 0==strcasecmp("color",attname) 
           || 0==strcasecmp("x",attname)
           || 0==strcasecmp("y",attname)
           || 0==strcasecmp("x-axis",attname)
           || 0==strcasecmp("y-axis",attname) ) {
      /* do nothing.  they are already taken care of */
    }
    else {
      warn ("unknown <points> attribute %s=\"%s\"", attname, attvalue);
    }
  }

  /* required and default attributes */
  if (! layer->x) {
    layer->x = moth_column_store_get (runtime->file->columns, "x");
    if (! layer->x)
      die ("missing <points> attribute \"x\"");
  }
  if (! layer->y) {
    layer->y = moth_column_store_get (runtime->file->columns, "y");
    if (! layer->y)
      die ("missing <points> attribute \"y\"");
  }
  if (MOTH_LOCATION_UNKNOWN == layer->x_axis)
    die ("missing <points> attribute \"x-axis\"");
  if (MOTH_LOCATION_UNKNOWN == layer->y_axis)
    die ("missing <points> attribute \"y-axis\"");
}



INTERNAL
void
moth_layer_draw_points
(
  moth_layer          layer,
  moth_image          image,
  moth_format_legend  legend,
  moth_format         formats,
  moth_pixmap         pixmap
)
{
  moth_layer_points_specs specs;
  size_t        x_count, y_count;
  int           row;

  moth_format_layer_points  points;
  moth_format_point         point, last_point;

  specs = (moth_layer_points_specs) layer->specifics;
  x_count  = moth_column_size (layer->x);
  y_count  = moth_column_size (layer->y);

  /* create data structure */
  points = (moth_format_layer_points)
    malloc (sizeof(struct moth_format_layer_points_s));
  points->points = NULL;
  points->filled = specs->filled;

  last_point = NULL;
  for ( row = 0;
        row < x_count || row < y_count;
        ++row )
  {
    moth_format_point new;
    double            x, y, radius;

    x = moth_column_value (layer->x, row);
    y = moth_column_value (layer->y, row);

    if (isnan(x) || isnan(y))
      continue;
    moth_image_get_viewport_value (image, &x, &y);

    radius = moth_column_value (specs->radius, row);
    if (isnan(radius))
      radius = MOTH_RADIUS_DEFAULT;

    /* add */
    new = (moth_format_point) malloc (sizeof(struct moth_format_point_s));
    new->location.x = x;
    new->location.y = y;
    new->size = radius;
    new->next = NULL;

    if (last_point)
      last_point->next = new;
    else
      points->points = new;
    last_point = new;
  }

  moth_format_draw_points (formats, layer->color, points, legend);
  if (pixmap)
    moth_pixmap_draw_points (pixmap, layer->color, points, legend);

  /* delete data structures */
  point = points->points;
  while (point) {
    moth_format_point doomed;
    doomed = point;
    point = point->next;
    free (doomed);
  }
  free (points);
}



INTERNAL
void
moth_layer_dump_specifics_points
(
  moth_layer layer
)
{
  moth_layer_points_specs specs;
  specs = (moth_layer_points_specs) layer->specifics;
  if (specs->radius)
    fprintf (stdout, "\n        radius=\"%s\"", moth_column_name(specs->radius));
  fprintf (stdout, "\n        filled=\"%s\"", specs->filled ? "on": "off");
}



/* 
 * Destroy just the specifics, everything else is taken care of.
 */
INTERNAL
void
moth_layer_destroy_points
(
  void  *specifics
)
{
  if (!specifics)
    return;
  free (specifics);
}



/*--------------------------------------------------------*\
|                    segments functions                    |
\*--------------------------------------------------------*/

INTERNAL
void
moth_layer_create_segments
(
  moth_layer        layer,
  const char        **attributes
)
{
  moth_layer_segments_specs segments;
  int a;

  segments = (moth_layer_segments_specs)
    malloc (sizeof(struct moth_layer_segments_specs_s));
  segments->width = NULL;
  layer->specifics = segments;

  /* read attributes */
  for (a=0; attributes[a]; a+=2) {
    const char *attname = attributes[a];
    const char *attvalue = attributes[a+1];

    if (0==strcasecmp("width",attname)) {
      segments->width = moth_column_store_get (runtime->file->columns,
                            attvalue);
      if (!segments->width)
        die ("couldn't understand <segments> attribute %s=\"%s\"", attname,
            attvalue);
    }
    /* ignore common attributes */
    else if ( 0==strcasecmp("title",attname)
           || 0==strcasecmp("color",attname) 
           || 0==strcasecmp("x0",attname)
           || 0==strcasecmp("y0",attname)
           || 0==strcasecmp("x1",attname)
           || 0==strcasecmp("y1",attname)
           || 0==strcasecmp("x-axis",attname)
           || 0==strcasecmp("y-axis",attname) ) {
      /* do nothing.  they are already taken care of */
    }
    else {
      warn ("unknown <segments> attribute %s=\"%s\"", attname, attvalue);
    }
  }

  /* required and default attributes */
  if (! layer->x) {
    layer->x = moth_column_store_get (runtime->file->columns, "x0");
    if (! layer->x)
      layer->x = moth_column_store_get (runtime->file->columns, "x");
    if (! layer->x)
      die ("missing <segments> attribute \"x0\"");
  }
  if (! layer->y) {
    layer->y = moth_column_store_get (runtime->file->columns, "y0");
    if (! layer->y)
      layer->y = moth_column_store_get (runtime->file->columns, "y");
    if (! layer->y)
      die ("missing <segments> attribute \"y0\"");
  }
  if (! layer->x1) {
    layer->x1 = moth_column_store_get (runtime->file->columns, "x1");
    if (! layer->x1)
      layer->x1 = moth_column_store_get (runtime->file->columns, "x");
    if (! layer->x1)
      die ("missing <segments> attribute \"x1\"");
  }
  if (! layer->y1) {
    layer->y1 = moth_column_store_get (runtime->file->columns, "y1");
    if (! layer->y1)
      layer->y1 = moth_column_store_get (runtime->file->columns, "y");
    if (! layer->y1)
      die ("missing <segments> attribute \"y1\"");
  }
  if (MOTH_LOCATION_UNKNOWN == layer->x_axis)
    die ("missing <segments> attribute \"x-axis\"");
  if (MOTH_LOCATION_UNKNOWN == layer->y_axis)
    die ("missing <segments> attribute \"y-axis\"");
}



INTERNAL
void
moth_layer_draw_segments
(
  moth_layer          layer,
  moth_image          image,
  moth_format_legend  legend,
  moth_format         formats,
  moth_pixmap         pixmap
)
{
  moth_layer_segments_specs specs;
  size_t                    x0_count, x1_count, y0_count, y1_count;
  moth_format_layer_piece   lines, last_line, line;
  int                       row;

  specs = (moth_layer_segments_specs) layer->specifics;
  x0_count  = moth_column_size (layer->x);
  x1_count  = moth_column_size (layer->x1);
  y0_count  = moth_column_size (layer->y);
  y1_count  = moth_column_size (layer->y1);

  lines = last_line = NULL;
  for ( row = 0;
        row < x0_count || row < x1_count ||
        row < y0_count || row < y1_count ;
        row++
      )
  {
    double width;
    width = moth_column_value (specs->width, row);
    if (isnan(width))
      width = MOTH_LINE_WIDTH_DEFAULT;

    line = (moth_format_layer_piece)
      malloc (sizeof(struct moth_format_layer_piece_s));
    line->points = NULL;
    line->next = NULL;

    line->points = (moth_format_point)
      malloc (sizeof(struct moth_format_point_s));
    line->points->location.x = moth_column_value (layer->x, row);
    line->points->location.y = moth_column_value (layer->y, row);
    line->points->size = width;
    line->points->next = NULL;

    line->points->next = (moth_format_point)
      malloc (sizeof(struct moth_format_point_s));
    line->points->next->location.x = moth_column_value (layer->x1, row);
    line->points->next->location.y = moth_column_value (layer->y1, row);
    line->points->next->size = width;
    line->points->next->next = NULL;

    moth_image_get_viewport_value (image,
        &line->points->location.x,
        &line->points->location.y);
    moth_image_get_viewport_value (image,
        &line->points->next->location.x,
        &line->points->next->location.y);

    if (last_line)
      last_line->next = line;
    else
      lines = line;
    last_line = line;
  }

  /* draw */
  moth_format_draw_segments (formats, layer->color, lines, legend);
  if (pixmap)
    moth_pixmap_draw_segments (pixmap, layer->color, lines, legend);

  /* free data structures */
  line = lines;
  while (line) {
    moth_format_layer_piece doomed;
    doomed = line;
    line = line->next;

    free (doomed->points->next);
    free (doomed->points);
    free (doomed);
  }
}



INTERNAL
void
moth_layer_dump_specifics_segments
(
  moth_layer layer
)
{
  moth_layer_segments_specs specs;
  specs = (moth_layer_segments_specs) layer->specifics;
  if (specs->width)
    fprintf (stdout, "\n        width=\"%s\"", moth_column_name(specs->width));
}



/* 
 * Destroy just the specifics, everything else is taken care of.
 */
INTERNAL
void
moth_layer_destroy_segments
(
  void  *specifics
)
{
  if (!specifics)
    return;
  free (specifics);
}



/*--------------------------------------------------------*\
|                     public functions                     |
\*--------------------------------------------------------*/

const char *
moth_layer_find_name
(
  moth_layer_type type
)
{
  switch (type) {
    case MOTH_AREA:
      return "area";
    case MOTH_GRID:
      return "grid";
    case MOTH_LINE:
      return "line";
    case MOTH_POINTS:
      return "points";
    case MOTH_SEGMENTS:
      return "segments";
    default:
      return NULL;
  }
}



moth_layer_type
moth_layer_find_type
(
  const char *name
)
{
  if (0==strcasecmp("area",name)) {
    return MOTH_AREA;
  }
  else if (0==strcasecmp("grid",name)) {
    return MOTH_GRID;
  }
  else if (0==strcasecmp("line",name)) {
    return MOTH_LINE;
  }
  else if (0==strcasecmp("points",name)) {
    return MOTH_POINTS;
  }
  else if (0==strcasecmp("segments",name)) {
    return MOTH_SEGMENTS;
  }
  else {
    return MOTH_LAYER_UNKNOWN;
  }
}



moth_layer
moth_layer_create
(
  const char        *name,
  const char        **attributes
)
{
  moth_layer layer;
  int a;

  layer = (moth_layer) malloc (sizeof(struct moth_layer_s));
  if (!layer)
    die ("couldn't find space for <%s> object", name);
  layer->next = NULL;
  layer->type = moth_layer_find_type (name);
  layer->title = NULL;
  layer->color = NULL;
  layer->x_axis = MOTH_BOTTOM;
  layer->y_axis = MOTH_LEFT;
  layer->x = NULL;
  layer->y = NULL;
  layer->x1 = NULL;
  layer->y1 = NULL;
  layer->specifics = NULL;

  /* common attributes */
  for (a=0; attributes[a]; a+=2) {
    const char *attname = attributes[a];
    const char *attvalue = attributes[a+1];

    if (0==strcasecmp("title",attname)) {
      layer->title = strdup (attvalue);
      if (!layer->title)
        die ("couldn't find enough space for the title of <%s>", name);
    }
    else if (0==strcasecmp("color",attname)) {
      layer->color = moth_image_get_color (
          moth_parser_current_image(runtime->file->parser), attvalue);
      if (! layer->color)
        warn ("can't understand attribute %s=\"%s\"", attname, attvalue);
    }
    else if (0==strcasecmp("x-axis",attname)) {
      layer->x_axis = moth_axis_find_location (attvalue);
      if (MOTH_LOCATION_UNKNOWN == layer->x_axis)
        warn ("can't understand attribute %s=\"%s\"", attname, attvalue);
    }
    else if (0==strcasecmp("y-axis",attname)) {
      layer->y_axis = moth_axis_find_location (attvalue);
      if (MOTH_LOCATION_UNKNOWN == layer->y_axis)
        warn ("can't understand attribute %s=\"%s\"", attname, attvalue);
    }
    else if ( (0==strcasecmp("x",attname))
           || (0==strcasecmp("x0",attname)) )
    {
      layer->x = moth_column_store_get (runtime->file->columns, attvalue);
      if (! layer->x)
        warn ("can't understand column \"%s\" for attribute \"%s\"",
            attvalue, attname);
    }
    else if ( (0==strcasecmp("y",attname))
           || (0==strcasecmp("y0",attname)) )
    {
      layer->y = moth_column_store_get (runtime->file->columns, attvalue);
      if (! layer->y)
        warn ("can't understand column \"%s\" for attribute \"%s\"",
            attvalue, attname);
    }
    else if (0==strcasecmp("x1",attname)) {
      layer->x1 = moth_column_store_get (runtime->file->columns, attvalue);
      if (! layer->x1)
        warn ("can't understand column \"%s\" for attribute \"%s\"",
            attvalue, attname);
    }
    else if (0==strcasecmp("y1",attname)) {
      layer->y1 = moth_column_store_get (runtime->file->columns, attvalue);
      if (! layer->y1)
        warn ("can't understand column \"%s\" for attribute \"%s\"",
            attvalue, attname);
    }
  }

  /* required common attributes and default values */
  if (! layer->color) {
    layer->color = moth_image_get_autocolor (
          moth_parser_current_image(runtime->file->parser));
    /* <grid>s should be more subtle */
    if (MOTH_GRID == layer->type)
      layer->color->alpha = 64;
  }

  /* make specifics for type */
  switch (layer->type) {
    case MOTH_AREA:
      moth_layer_create_area (layer, attributes);
      break;
    case MOTH_GRID:
      moth_layer_create_grid (layer, attributes);
      break;
    case MOTH_LINE:
      moth_layer_create_line (layer, attributes);
      break;
    case MOTH_POINTS:
      moth_layer_create_points (layer, attributes);
      break;
    case MOTH_SEGMENTS:
      moth_layer_create_segments (layer, attributes);
      break;
    default:
      warn ("unknown layer type \"%s\"", name);
      free (layer);
      return NULL;
  }

  return layer;
}



void
moth_layer_dump
(
  moth_layer layer
)
{
  char  *x_name, *y_name;

  fprintf (stdout, "    <%s", moth_layer_find_name(layer->type));
  if (layer->title)
    fprintf (stdout, " title=\"%s\"", layer->title);
  if (layer->color) {
    fprintf (stdout, " color=\"");
    moth_color_dump (layer->color);
    fprintf (stdout, "\"");
  }

  x_name = "x";
  y_name = "y";
  if (MOTH_AREA == layer->type || MOTH_SEGMENTS == layer->type) {
    x_name = "x0";
    y_name = "y0";
  }

  if (layer->x || layer->y) {
    fprintf (stdout, "\n        ");
    fprintf (stdout, "%s=\"%s\"", x_name, moth_column_name(layer->x));
    fprintf (stdout, " %s=\"%s\"", y_name, moth_column_name(layer->y));
  }
  if (layer->x1 || layer->y1) {
    fprintf (stdout, "\n        ");
    fprintf (stdout, "x1=\"%s\"", moth_column_name(layer->x1));
    fprintf (stdout, " y1=\"%s\"", moth_column_name(layer->y1));
  }
  
  fprintf (stdout, "\n        x-axis=\"%s\"", moth_axis_find_name(layer->x_axis));
  fprintf (stdout, "\n        y-axis=\"%s\"", moth_axis_find_name(layer->y_axis));

  switch (layer->type) {
    case MOTH_AREA:
      moth_layer_dump_specifics_area (layer);
      break;
    case MOTH_GRID:
      moth_layer_dump_specifics_grid (layer);
      break;
    case MOTH_LINE:
      moth_layer_dump_specifics_line (layer);
      break;
    case MOTH_POINTS:
      moth_layer_dump_specifics_points (layer);
      break;
    case MOTH_SEGMENTS:
      moth_layer_dump_specifics_segments (layer);
      break;
    default:
      /* hmmm... we really shouldn't get here */
      break;
  }

  fprintf (stdout, "\n    />\n");
}



void
moth_layer_add_layer
(
  moth_layer  list,
  moth_layer  new_layer
)
{
  moth_layer l, last;
  l = last = list->next;
  while (l) {
    last = l;
    l = l->next;
  }
  if (last)
    last->next = new_layer;
  else
    list->next = new_layer;
}



void
moth_layer_draw
(
  moth_layer          layer,
  moth_image          image,
  moth_format_legend  legend,
  moth_format         formats,
  moth_pixmap         pixmap
)
{
  switch (layer->type) {
    case MOTH_AREA:
      moth_layer_draw_area (layer, image, legend, formats, pixmap);
      break;
    case MOTH_GRID:
      moth_layer_draw_grid (layer, image, legend, formats, pixmap);
      break;
    case MOTH_LINE:
      moth_layer_draw_line (layer, image, legend, formats, pixmap);
      break;
    case MOTH_POINTS:
      moth_layer_draw_points (layer, image, legend, formats, pixmap);
      break;
    case MOTH_SEGMENTS:
      moth_layer_draw_segments (layer, image, legend, formats, pixmap);
      break;
    default:
      die ("can't draw unknown <%s>", moth_layer_find_name(layer->type));
      break;
  }
}



void
moth_layer_destroy
(
  moth_layer layer
)
{
  if (layer->next)
    moth_layer_destroy (layer->next);

  if (layer->title)
    free (layer->title);

  if (layer->color)
    moth_color_destroy (layer->color);

  switch (layer->type) {
    case MOTH_AREA:
      moth_layer_destroy_area (layer->specifics);
      break;
    case MOTH_GRID:
      moth_layer_destroy_grid (layer->specifics);
      break;
    case MOTH_LINE:
      moth_layer_destroy_line (layer->specifics);
      break;
    case MOTH_POINTS:
      moth_layer_destroy_points (layer->specifics);
      break;
    case MOTH_SEGMENTS:
      moth_layer_destroy_segments (layer->specifics);
      break;
    default:
      die ("can't destroy unknown <%s>", moth_layer_find_name(layer->type));
      break;
  }

  free (layer);
}




