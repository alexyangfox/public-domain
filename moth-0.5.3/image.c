/*
 * $Id: image.c,v 1.26 2003/06/25 18:09:34 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See image.dev for implementation notes.
 *
 */


#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "axes.h"
#include "colors.h"
#include "data.h"
#include "formats.h"
#include "image.h"
#include "layers.h"
#include "pixmap.h"



/*--------------------------------------------------------*\
|                      datastructures                      |
\*--------------------------------------------------------*/

typedef unsigned int  moth_pixel;



typedef enum {
  MOTH_SIZE_TYPE_IMAGE = 0,
  MOTH_SIZE_TYPE_CANVAS
} moth_size_type;



typedef enum {
  MOTH_HORIZONTAL = 0,
  MOTH_VERTICAL = 1
} moth_image_layout_orientation;



struct moth_image_s {
  char                *filename;
  unsigned int        width_attribute;
  unsigned int        height_attribute;
  unsigned int        width;
  unsigned int        height;
  moth_size_type      size_type;
  char                *title;

  moth_format         formats;  /* linked list */
  moth_axis           axes[4];
  moth_layer          layers;   /* linked list */

  moth_color          background;
  moth_color          canvas;
  moth_color          border_color;
  unsigned int        border_size;

  moth_color_store    colors;

  /* viewport settings */
  unsigned int        x_right, x_width;
  moth_axis_scaling   x_scaling;
  double              x_min, x_max;
  unsigned int        y_up, y_height;
  moth_axis_scaling   y_scaling;
  double              y_min, y_max;

  /* pixmap handling */
  moth_pixmap         pixmap;
  unsigned int        draw_pixmap;
};



/* region */
typedef struct moth_image_layout_region_s moth_image_layout_region;
struct moth_image_layout_region_s {
  moth_pixel      x, y;
  moth_pixel      width, height;
};

/* string */
typedef struct moth_image_layout_string_s moth_image_layout_string;
struct moth_image_layout_string_s {
  moth_image_layout_region  region;
  moth_pixmap_string        string;
};

/* axis */
typedef struct moth_image_layout_axis_s moth_image_layout_axis;
struct moth_image_layout_axis_s {
  moth_image_layout_region      region;
  moth_axis_location            location;
  moth_image_layout_orientation orientation;
  moth_image_layout_string      title;
  moth_image_layout_region      tick_values; /* bounding box */
  moth_image_layout_region      tick_marks;  /* bounding box */
  moth_image_layout_region      axis;        /* line */
};

/* canvas */
typedef struct moth_image_layout_canvas_s moth_image_layout_canvas;
struct moth_image_layout_canvas_s {
  moth_image_layout_region region;
};

/* legend */
typedef struct moth_image_layout_legend_s moth_image_layout_legend;
struct moth_image_layout_legend_s {
  moth_image_layout_region  region;
  size_t                    layer_count;
  moth_image_layout_string  *layers;
};


/* image */
typedef struct moth_image_layout_s *moth_image_layout;
struct moth_image_layout_s {
  moth_pixel                width, height;
  moth_image_layout_string  title;
  moth_image_layout_axis    axes[4];
  moth_image_layout_canvas  canvas;
  moth_image_layout_legend  legend;
};



/*--------------------------------------------------------*\
|                    viewport functions                    |
\*--------------------------------------------------------*/

/* 
 * Transmogrify x and y based on the viewport settings.
 */
void
moth_image_get_viewport_value
(
  moth_image  image,
  double      *x,
  double      *y
)
{

  if (x) {
    *x = scale_value (image->x_scaling, *x);
    /* clip to min/max  (mainly when they are forced) */
    /* (min and max already scaled */
    if (*x < image->x_min) *x = image->x_min;
    if (*x > image->x_max) *x = image->x_max;
    *x = 
      image->x_right +         /* start here */
      (image->x_width *        /* size of drawable area */
      ((*x-image->x_min)/(image->x_max-image->x_min))
                                /* proportion of drawable area */
      )
      + 0.5;                    /* whole values at center of pixel */
  }

  if (y) {
    *y = scale_value (image->y_scaling, *y);
    /* clip to min/max  (mainly when they are forced) */
    /* (min and max already scaled */
    if (*y < image->y_min) *y = image->y_min;
    if (*y > image->y_max) *y = image->y_max;
    *y = 
      image->y_up +             /* start here */
      (image->y_height *        /* size of drawable area */
      ((*y-image->y_min)/(image->y_max-image->y_min))
                                /* proportion of drawable area */
      )
      + 0.5;                    /* whole values at center of pixel */
  }
}



INTERNAL
void
moth_image_set_x_viewport
(
  moth_image        image,
  unsigned int      right,
  unsigned int      width,
  moth_axis_scaling scaling,
  double            min,
  double            max
)
{
  if (!image)
    return;
  image->x_right = right;
  image->x_width = width;
  image->x_scaling = scaling;
  /* scale min/max now for ease later */
  image->x_min = scale_value (scaling, min);
  image->x_max = scale_value (scaling, max);
}



INTERNAL
void
moth_image_set_y_viewport
(
  moth_image        image,
  unsigned int      up,
  unsigned int      height,
  moth_axis_scaling scaling,
  double            min,
  double            max
)
{
  if (!image)
    return;
  image->y_up = up;
  image->y_height = height;
  image->y_scaling = scaling;
  /* scale min/max now for ease later */
  image->y_min = scale_value (scaling, min);
  image->y_max = scale_value (scaling, max);
}



INTERNAL
void
moth_image_reset_viewports
(
  moth_image image
)
{
  moth_image_set_x_viewport (image, 0, image->width, MOTH_LINEAR, 0.0,
      image->width);
  moth_image_set_y_viewport (image, 0, image->height, MOTH_LINEAR, 0.0,
      image->height);
}





/*--------------------------------------------------------*\
|                     layout functions                     |
\*--------------------------------------------------------*/

INTERNAL
void
moth_image_layout_region_initialize
(
  moth_image_layout_region *region
)
{
  region->x = 0;
  region->y = 0;
  region->width = 0;
  region->height = 0;
}



INTERNAL
void
moth_image_layout_string_initialize
(
  moth_image_layout_string      *layout,
  moth_pixmap                   pixmap,
  const char                    *string,
  moth_image_layout_orientation orientation
)
{
  moth_image_layout_region_initialize (&layout->region);
  layout->string = moth_pixmap_string_create (pixmap, string, orientation);
  if (layout->string) {
    layout->region.width = layout->string->width;
    layout->region.height = layout->string->height;
  }
}



INTERNAL
void
moth_image_layout_string_destroy
(
  moth_image_layout_string *string
)
{
  if (string->string)
    moth_pixmap_string_destroy (string->string);
}



INTERNAL
void
moth_image_layout_axis_initialize
(
  moth_image_layout_axis  *layout,
  moth_axis_location      location,
  moth_image              image
)
{
  moth_axis   axis;

  moth_image_layout_region_initialize (&layout->region);
  layout->location = location;

  switch (layout->location) {
    case MOTH_LEFT:
    case MOTH_RIGHT:
      layout->orientation = MOTH_VERTICAL;
      break;
    default:
      layout->orientation = MOTH_HORIZONTAL;
      break;
  }
  
  axis = moth_image_axis (image, location);
  moth_image_layout_string_initialize (&layout->title, image->pixmap,
      axis->title, layout->orientation);

  moth_image_layout_region_initialize (&layout->tick_values);
  moth_image_layout_region_initialize (&layout->tick_marks);
  moth_image_layout_region_initialize (&layout->axis);
}



/* 
 * For the left and right axes, their width and internal layout on the x axis
 * are independant of the rest of the image.
 * For the top and bottom axes, their height and internal layout on the y axis
 * are independant of the rest of the image.
 */
INTERNAL
void
moth_image_layout_axis_calc_independant
(
  moth_image_layout_axis  *layout,
  moth_image              image
)
{
  moth_axis     axis;
  moth_pixel    width, height;
  moth_label    label;

  axis = moth_image_axis (image, layout->location);

  /*
   * We'll go ahead and calculate the greatest bounding box for all the ticks,
   * as if they're drawn on top of each other.  We'll actually stretch out the
   * bounding box later, to match the canvas.
   */
  width = height = 0;
  for (label=axis->labels; label; label=label->next) {
    moth_label_tick tick;
    for (tick=label->ticks; tick; tick=tick->next) {
      moth_pixmap_string string;
      string = moth_pixmap_string_create (image->pixmap, tick->text, 0);
      if (!string)
        continue;
      if (string->width > width)
        width = string->width;
      if (string->height > height)
        height = string->height;
      moth_pixmap_string_destroy (string);
    }
  }
  layout->tick_values.width = width;
  layout->tick_values.height = height;

  if (MOTH_VERTICAL == layout->orientation) {
    /* set tick and axis line widths */
    if (axis->configured) {
      layout->tick_marks.width = MOTH_IMAGE_TICK_LENGTH_DEFAULT;
      layout->axis.width = MOTH_IMAGE_AXIS_WIDTH_DEFAULT;
    }
  }
  else {
    /* set tick and axis line heights */
    if (axis->configured) {
      layout->tick_marks.height = MOTH_IMAGE_TICK_LENGTH_DEFAULT;
      layout->axis.height = MOTH_IMAGE_AXIS_WIDTH_DEFAULT;
    }
  }

  switch (layout->location) {
    case MOTH_LEFT:
      width = 0;
      layout->title.region.x = width;
      width += layout->title.region.width;
      if (layout->title.region.width)
        width += MOTH_IMAGE_SMALL_SPACER;
      layout->tick_values.x = width;
      width += layout->tick_values.width;
      if (layout->tick_values.width)
        width += MOTH_IMAGE_SMALL_SPACER;
      layout->tick_marks.x = width;
      /* don't show marks unless we have values */
      if (layout->tick_values.width)
        width += layout->tick_marks.width;
      layout->axis.x = width;
      width += layout->axis.width;
      layout->region.width = width;
      break;

    case MOTH_TOP:
      height = 0;
      layout->axis.y = height;
      height += layout->axis.height;
      layout->tick_marks.y = height;
      if (layout->tick_values.height) {
        /* don't show marks unless we have values */
        height += layout->tick_marks.height;
        height += MOTH_IMAGE_SMALL_SPACER;
      }
      layout->tick_values.y = height;
      height += layout->tick_values.height;
      if (layout->title.region.height)
        height += MOTH_IMAGE_SMALL_SPACER;
      layout->title.region.y = height;
      height += layout->title.region.height;
      layout->region.height = height;
      break;

    case MOTH_RIGHT:
      width = 0;
      layout->axis.x = width;
      width += layout->axis.width;
      layout->tick_marks.x = width;
      if (layout->tick_values.width) {
        /* don't show marks unless we have values */
        width += layout->tick_marks.width;
        width += MOTH_IMAGE_SMALL_SPACER;
      }
      layout->tick_values.x = width;
      width += layout->tick_values.width;
      if (layout->title.region.width)
        width += MOTH_IMAGE_SMALL_SPACER;
      layout->title.region.x = width;
      width += layout->title.region.width;
      layout->region.width = width;
      break;

    case MOTH_BOTTOM:
      height = 0;
      layout->title.region.y = height;
      height += layout->title.region.height;
      if (layout->title.region.height)
        height += MOTH_IMAGE_SMALL_SPACER;
      layout->tick_values.y = height;
      height += layout->tick_values.height;
      if (layout->tick_values.height)
        height += MOTH_IMAGE_SMALL_SPACER;
      layout->tick_marks.y = height;
      /* don't show marks unless we have values */
      if (layout->tick_values.height)
        height += layout->tick_marks.height;
      layout->axis.y = height;
      height += layout->axis.height;
      layout->region.height = height;
      break;

    default:
      /* we shouldn't really ever get here */
      die ("unknown <axis> type %d", layout->location);
      break;
  }

}



INTERNAL
void
moth_image_layout_axis_calc_dependent
(
  moth_image_layout_axis    *layout,
  moth_image_layout_region  *requisite
)
{
  switch (layout->orientation) {
    case MOTH_VERTICAL:
      layout->region.height = requisite->height;
      layout->tick_values.height = requisite->height;
      layout->tick_marks.height = requisite->height;
      layout->axis.height = requisite->height;

      layout->title.region.y = floor (
          (layout->region.height - layout->title.region.height) / 2.0);
      layout->tick_values.y = 0;
      layout->tick_marks.y = 0;
      layout->axis.y = 0;
      break;

    case MOTH_HORIZONTAL:
      layout->region.width = requisite->width;
      layout->tick_values.width = requisite->width;
      layout->tick_marks.width = requisite->width;
      layout->axis.width = requisite->width;

      layout->title.region.x = floor (
          (layout->region.width - layout->title.region.width) / 2.0);
      layout->tick_values.x = 0;
      layout->tick_marks.x = 0;
      layout->axis.x = 0;
      break;
  }
}



INTERNAL
void
moth_image_layout_draw_axes
(
  moth_image_layout layout,
  moth_image        image
)
{
  moth_axis_location  location;
  moth_format_axis    axes, last_axis, axis;
  moth_color          default_color;
  moth_label          label;

  default_color = moth_image_get_color (image, "black");

  axes = last_axis = NULL;

  for (location=MOTH_LEFT; location<=MOTH_BOTTOM; location++) {
    moth_axis               axis;
    moth_image_layout_axis  *axis_layout;
    moth_format_axis        axis_format;
    moth_format_tick        last_tick;

    axis = moth_image_axis (image, location);
    if (!axis || !axis->configured)
      continue;

    axis_layout = &(layout->axes[location-MOTH_LEFT]);

    axis_format = (moth_format_axis)
      malloc (sizeof(struct moth_format_axis_s));
    axis_format->location = location;
    axis_format->line = NULL;
    axis_format->title = NULL;
    axis_format->color = NULL;
    axis_format->ticks = NULL;
    axis_format->next = NULL;

    /* set color */
    if (axis->color)
      axis_format->color = axis->color;
    else
      axis_format->color = default_color;

    /* build line */
    axis_format->line = (moth_format_point)
      malloc (sizeof(struct moth_format_point_s));
    axis_format->line->next = (moth_format_point)
      malloc (sizeof(struct moth_format_point_s));
    axis_format->line->next->next = NULL;

    if (axis_layout->orientation == MOTH_VERTICAL) {
      axis_format->line->location.x = axis_layout->region.x
        + axis_layout->axis.x + 0.5;
      axis_format->line->location.y = axis_layout->region.y
        + axis_layout->axis.y + 0.5;
      axis_format->line->size = axis_layout->axis.width;

      axis_format->line->next->location.x = axis_format->line->location.x;
      axis_format->line->next->location.y = axis_format->line->location.y
        + axis_layout->axis.height - 1.0;
      axis_format->line->next->size = axis_format->line->size;
    }
    else {
      axis_format->line->location.x = axis_layout->region.x
        + axis_layout->axis.x + 0.5;
      axis_format->line->location.y = axis_layout->region.y
        + axis_layout->axis.y + 0.5;
      axis_format->line->size = axis_layout->axis.height;

      axis_format->line->next->location.x = axis_format->line->location.x
        + axis_layout->axis.width - 1.0;
      axis_format->line->next->location.y = axis_format->line->location.y;
      axis_format->line->next->size = axis_format->line->size;
    }

    /* set title */
    if (axis_layout->title.string) {
      axis_format->title = (moth_format_text)
        malloc (sizeof(struct moth_format_text_s));
      axis_format->title->location.x = axis_layout->region.x
        + axis_layout->title.region.x + 0.5;
      axis_format->title->location.y = axis_layout->region.y
        + axis_layout->title.region.y + 0.5;
      axis_format->title->string = axis->title;
      axis_format->title->color = axis_format->color;
      axis_format->title->flags = 0;

      if (MOTH_VERTICAL == axis_layout->orientation) {
        axis_format->title->flags |= MOTH_DRAW_VERTICAL;
        /* _draw_string() takes a point on the baseline of the string */
        axis_format->title->location.x += axis_layout->title.region.width;
      }
    }

    /* build ticks */
    /* 
     * The tick values are ALREADY scaled, so we need to setup
     * the viewport to display them linearly, within the
     * appropriate (scaled) bounds.
     */
    switch (axis_layout->orientation) {
      case MOTH_HORIZONTAL:
        moth_image_set_x_viewport (image,
            axis_layout->region.x, axis_layout->region.width-1,
            MOTH_LINEAR,
            scale_value(axis->scaling,axis->min),
            scale_value(axis->scaling,axis->max));
        break;
      case MOTH_VERTICAL:
        moth_image_set_y_viewport (image,
            axis_layout->region.y, axis_layout->region.height-1,
            MOTH_LINEAR,
            scale_value(axis->scaling,axis->min),
            scale_value(axis->scaling,axis->max));
        break;
    }

    last_tick = NULL;
    for (label=axis->labels; label; label=label->next) {
      moth_label_tick   tick;
      moth_format_tick  tick_format;
      moth_color        color;

      if (label->color)
        color = label->color;
      else
        color = axis_format->color;

      for (tick=label->ticks; tick; tick=tick->next) {

        tick_format = (moth_format_tick)
          malloc (sizeof(struct moth_format_tick_s));
        tick_format->mark = NULL;
        tick_format->text = NULL;
        tick_format->color = color;
        tick_format->next = NULL;

        tick_format->mark = (moth_format_point)
          malloc (sizeof(struct moth_format_point_s));
        tick_format->mark->size = MOTH_IMAGE_TICK_WIDTH_DEFAULT;
        tick_format->mark->next = NULL;

        tick_format->mark->next = (moth_format_point)
          malloc (sizeof(struct moth_format_point_s));
        tick_format->mark->next->size = MOTH_IMAGE_TICK_WIDTH_DEFAULT;
        tick_format->mark->next->next = NULL;

        switch (axis_layout->orientation) {
          case MOTH_VERTICAL:
            tick_format->mark->location.x = axis_layout->region.x
              + axis_layout->tick_marks.x;
            tick_format->mark->location.y = tick->value;
            tick_format->mark->next->location.x = tick_format->mark->location.x
              + axis_layout->tick_marks.width - 1.0;
            tick_format->mark->next->location.y = tick->value;
            break;

          case MOTH_HORIZONTAL:
            tick_format->mark->location.x = tick->value;
            tick_format->mark->location.y = axis_layout->region.y
              + axis_layout->tick_marks.y;
            tick_format->mark->next->location.x = tick->value;
            tick_format->mark->next->location.y = tick_format->mark->location.y
              + axis_layout->tick_marks.height - 1.0;
            break;

          default:
            die ("unknown <axis> orientation %d", axis_layout->orientation);
            break;
        }
        moth_image_get_viewport_value (image, 
            &(tick_format->mark->location.x),
            &(tick_format->mark->location.y) );
        moth_image_get_viewport_value (image, 
            &(tick_format->mark->next->location.x),
            &(tick_format->mark->next->location.y) );

        if (tick->text) {
          moth_pixmap_string string;
          string = moth_pixmap_string_create (image->pixmap, tick->text, 0);
          if (!string) {
            warn ("couldn't create tick value string");
            continue;
          }

          tick_format->text = (moth_format_text)
            malloc (sizeof(struct moth_format_text_s));
          tick_format->text->string = tick->text;
          tick_format->text->color = color;
          tick_format->text->flags = 0;

          switch (axis_layout->orientation) {
            case MOTH_VERTICAL:
              /* right justified */
              tick_format->text->location.x = axis_layout->region.x
                + axis_layout->tick_values.x
                + axis_layout->tick_values.width
                - string->width;
              tick_format->text->location.y = tick->value;
              tick_format->text->flags |= MOTH_CENTER_HORIZONTAL;
              break;

            case MOTH_HORIZONTAL:
              /* bottom justified */
              tick_format->text->location.x = tick->value;
              tick_format->text->location.y = axis_layout->region.y
                + axis_layout->tick_values.y;
              tick_format->text->flags |= MOTH_CENTER_VERTICAL;
              break;

            default:
              die ("unknown <axis> orientation %d", axis_layout->orientation);
              break;
          }
          moth_image_get_viewport_value (image, 
              &(tick_format->text->location.x),
              &(tick_format->text->location.y) );

          moth_pixmap_string_destroy (string);
        }

        if (last_tick)
          last_tick->next = tick_format;
        else
          axis_format->ticks = tick_format;
        last_tick = tick_format;
      }
    }
    moth_image_reset_viewports (image);

    if (last_axis)
      last_axis->next = axis_format;
    else
      axes = axis_format;
    last_axis = axis_format;
  }

  /* draw */
  moth_format_draw_axes (image->formats, axes);
  if (image->draw_pixmap)
    moth_pixmap_draw_axes (image->pixmap, axes);

  /* delete data structures */
  axis = axes;
  while (axis) {
    moth_format_axis  doomed_axis;
    moth_format_tick  tick;
    doomed_axis = axis;
    axis = axis->next;
    
    free (doomed_axis->line->next);
    free (doomed_axis->line);
    if (doomed_axis->title)
      free (doomed_axis->title);

    tick = doomed_axis->ticks;
    while (tick) {
      moth_format_tick doomed_tick;
      doomed_tick = tick;
      tick = tick->next;

      free (doomed_tick->mark->next);
      free (doomed_tick->mark);
      if (doomed_tick->text)
        free (doomed_tick->text);
      free (doomed_tick);
    }

    free (doomed_axis);
  }

  moth_color_destroy (default_color);
}



INTERNAL
void
moth_image_layout_axis_destroy
(
  moth_image_layout_axis *axis
)
{
  moth_image_layout_string_destroy (&axis->title);
}



INTERNAL
void
moth_image_layout_canvas_initialize
(
  moth_image_layout_canvas *canvas
)
{
  moth_image_layout_region_initialize (&canvas->region);
}



INTERNAL
void
moth_image_layout_draw_canvas
(
  moth_image_layout_canvas  *canvas,
  moth_image                image
)
{
  moth_point p, q;
  p.x = canvas->region.x - 0.5;
  p.y = canvas->region.y - 0.5;
  q.x = p.x + canvas->region.width;
  q.y = p.y + canvas->region.height;
  moth_pixmap_draw_canvas (image->pixmap, image->canvas, p, q);
}



INTERNAL
void
moth_image_layout_canvas_destroy
(
  moth_image_layout_canvas *canvas
)
{
  /* nothing to do */
}



INTERNAL
void
moth_image_layout_draw_watermark
(
  moth_image_layout layout,
  moth_image        image
)
{
  moth_format_text    watermark;
  moth_pixmap_string  string;

  string = moth_pixmap_string_create (image->pixmap, PACKAGE_STRING, 0);
  if (!string)
    die ("internal error:  couldn't create watermark pixmap string");

  watermark = (moth_format_text) malloc (sizeof(struct moth_format_text_s));
  if (!watermark)
    die ("internal error:  couldn't malloc watermark structure");
  watermark->location.x = layout->width - string->width - image->border_size;
  watermark->location.y = 1.0 + image->border_size;
  watermark->string = PACKAGE_STRING;
  watermark->color = moth_image_get_color (image, MOTH_IMAGE_WATERMARK_COLOR);
  watermark->flags = 0;

  moth_format_draw_watermark (image->formats, watermark);
  if (image->draw_pixmap)
    moth_pixmap_draw_watermark (image->pixmap, watermark, string);

  moth_pixmap_string_destroy (string);
  moth_color_destroy (watermark->color);
  free (watermark);
}



INTERNAL
void
moth_image_layout_draw_title
(
  moth_image_layout layout,
  moth_image        image
)
{
  moth_format_text title;

  title = (moth_format_text) malloc (sizeof(struct moth_format_text_s));
  if (!title)
    die ("internal error:  couldn't malloc title structure");

  title->location.x = layout->title.region.x + 0.5;
  title->location.y = layout->title.region.y + 0.5;
  title->string = image->title;
  title->color = moth_image_get_color (image, "black");
  title->flags = 0;

  moth_format_draw_title (image->formats, title);
  if (image->draw_pixmap)
    moth_pixmap_draw_title (image->pixmap, title, layout->title.string);

  moth_color_destroy (title->color);
  free (title);
}



INTERNAL
void
moth_image_layout_legend_initialize
(
  moth_image_layout_legend  *legend,
  moth_image                image
)
{
  moth_layer    layer;
  int           i;

  moth_image_layout_region_initialize (&legend->region);

  legend->layer_count = 0;
  for (layer=image->layers; layer; layer=layer->next) {
    if (!layer->title) continue;
    ++legend->layer_count;
  }
  legend->layers = (moth_image_layout_string *)
    calloc (legend->layer_count, sizeof(struct moth_image_layout_string_s));

  i = 0;
  for (layer=image->layers; layer; layer=layer->next) {
    if (!layer->title) continue;
    moth_image_layout_string_initialize (&(legend->layers[i]), image->pixmap,
        layer->title, MOTH_HORIZONTAL);
    ++i;
  }
}



INTERNAL
void
moth_image_layout_legend_calc_geometry
(
  moth_image_layout_legend  *legend,
  moth_pixel                width
)
{
  size_t        l, rows;
  moth_pixel    row_height, x, y;
  
  legend->region.width = width;

  /* find maximum row height */
  row_height = 0;
  for (l=0; l<legend->layer_count; ++l) {
    if (legend->layers[l].region.height > row_height)
      row_height = legend->layers[l].region.height;
  }

  rows = 1;
  x = y = 0;
  for (l=0; l<legend->layer_count; ++l) {
    moth_pixel width;
    width = MOTH_IMAGE_LEGEND_SWATCHSIZE + MOTH_IMAGE_SMALL_SPACER
      + legend->layers[l].region.width;

    /* switch to new row if not enough room 
     * (and not the first item of the row)  */
    if ( ((x+width) > legend->region.width)
        && (0 != x))
    {
      ++rows;
      x = 0;
      y += row_height + MOTH_IMAGE_SMALL_SPACER;
    }

    legend->layers[l].region.x = x;
    legend->layers[l].region.y = y;  /* for now.  invert later */
    x += width + 2*MOTH_IMAGE_SPACER;
  }

  legend->region.height = rows*row_height + ((rows-1)*MOTH_IMAGE_SMALL_SPACER);

  /* invert vertical location of layers */
  for (l=0; l<legend->layer_count; ++l) {
    legend->layers[l].region.y = legend->region.height
      - legend->layers[l].region.y - legend->layers[l].region.height;
  }

}



INTERNAL
void
moth_image_layout_legend_destroy
(
  moth_image_layout_legend *legend
)
{
  int i;
  for (i=0; i<legend->layer_count; ++i) {
    moth_image_layout_string_destroy (&(legend->layers[i]));
  }
  free (legend->layers);
}



INTERNAL
void
moth_image_layout_layout_horizontal
(
  moth_image_layout layout,
  moth_image        image
)
{
  moth_pixel x;
  x = 0;

  x += image->border_size;
  x += MOTH_IMAGE_MARGIN;
  layout->axes[MOTH_LEFT-MOTH_LEFT].region.x = x;
  x += layout->axes[MOTH_LEFT-MOTH_LEFT].region.width;
  layout->canvas.region.x = x;
  x += layout->canvas.region.width;
  layout->axes[MOTH_RIGHT-MOTH_LEFT].region.x = x;

  layout->axes[MOTH_TOP-MOTH_LEFT].region.x = layout->canvas.region.x;
  layout->axes[MOTH_BOTTOM-MOTH_LEFT].region.x = layout->canvas.region.x;
  layout->legend.region.x = layout->canvas.region.x;

  layout->title.region.x
    = floor ((layout->width - layout->title.region.width) / 2.0);
}



INTERNAL
void
moth_image_layout_layout_vertical
(
  moth_image_layout layout,
  moth_image        image
)
{
  moth_pixel y;
  y = 0;

  y += image->border_size;
  y += MOTH_IMAGE_MARGIN;
  layout->legend.region.y = y;
  y += layout->legend.region.height;
  if (layout->legend.region.height)
    y += MOTH_IMAGE_SPACER;
  layout->axes[MOTH_BOTTOM-MOTH_LEFT].region.y = y;
  y += layout->axes[MOTH_BOTTOM-MOTH_LEFT].region.height;
  layout->canvas.region.y = y;
  y += layout->canvas.region.height;
  layout->axes[MOTH_TOP-MOTH_LEFT].region.y = y;
  y += layout->axes[MOTH_TOP-MOTH_LEFT].region.height;
  if (layout->title.region.height) {
    y += MOTH_IMAGE_SPACER;
    layout->title.region.y = y;
  }

  layout->axes[MOTH_LEFT-MOTH_LEFT].region.y = layout->canvas.region.y;
  layout->axes[MOTH_RIGHT-MOTH_LEFT].region.y = layout->canvas.region.y;
}



/* 
 * ugg... complicated algorithm because of the interdependancies
 *
 *  - the width of the canvas depends on the width and location of
 *    the left and right axes, as well as the image attributes
 *
 *  - the height of the legend depends on the width of the canvas
 *    (since the legend swatches may wrap onto more than one line)
 *
 *  - the height of the canvas depends on the height and location
 *    of the top and bottom axes, as well as the image attributes
 *
 */
INTERNAL
void
moth_image_layout_layout
(
  moth_image_layout layout,
  moth_image        image
)
{
  moth_pixel  width, height;
  int         i;
  width = height = 0;

  for (i=MOTH_LEFT; i<=MOTH_BOTTOM; ++i) {
    moth_image_layout_axis_calc_independant (&(layout->axes[i-MOTH_LEFT]),
        image);
  }

  /* calc width of canvas */
  if (MOTH_SIZE_TYPE_IMAGE == image->size_type) {
    width = image->width_attribute;
    layout->width = width;

    width -= image->border_size;
    width -= MOTH_IMAGE_MARGIN;
    width -= layout->axes[MOTH_LEFT-MOTH_LEFT].region.width;
    width -= layout->axes[MOTH_RIGHT-MOTH_LEFT].region.width;
    width -= MOTH_IMAGE_MARGIN;
    width -= image->border_size;
    layout->canvas.region.width = width;
  }
  else {
    layout->canvas.region.width = image->width_attribute;

    width = 0;
    width += image->border_size;
    width += MOTH_IMAGE_MARGIN;
    width += layout->axes[MOTH_LEFT-MOTH_LEFT].region.width;
    width += layout->canvas.region.width;
    width += layout->axes[MOTH_RIGHT-MOTH_LEFT].region.width;
    width += MOTH_IMAGE_MARGIN;
    width += image->border_size;
    layout->width = width;
  }

  moth_image_layout_legend_calc_geometry (&layout->legend,
      layout->canvas.region.width);

  /* calc height of canvas */
  if (MOTH_SIZE_TYPE_IMAGE == image->size_type) {
    height = image->height_attribute;
    layout->height = height;

    height -= image->border_size;
    height -= MOTH_IMAGE_MARGIN;
    height -= layout->legend.region.height;
    if (layout->legend.region.height)
      height -= MOTH_IMAGE_SPACER;
    height -= layout->axes[MOTH_BOTTOM-MOTH_LEFT].region.height;
    height -= layout->axes[MOTH_TOP-MOTH_LEFT].region.height;
    if (layout->title.region.height) {
      height -= MOTH_IMAGE_SPACER;
      height -= layout->title.region.height;
    }
    height -= MOTH_IMAGE_MARGIN;
    height -= image->border_size;
    layout->canvas.region.height = height;
  }
  else {
    layout->canvas.region.height = image->height_attribute;

    height = 0;
    height += image->border_size;
    height += MOTH_IMAGE_MARGIN;
    height += layout->legend.region.height;
    if (layout->legend.region.height)
      height += MOTH_IMAGE_SPACER;
    height += layout->axes[MOTH_BOTTOM-MOTH_LEFT].region.height;
    height += layout->canvas.region.height;
    height += layout->axes[MOTH_TOP-MOTH_LEFT].region.height;
    if (layout->title.region.height) {
      height += MOTH_IMAGE_SPACER;
      height += layout->title.region.height;
    }
    height += MOTH_IMAGE_MARGIN;
    height += image->border_size;
    layout->height = height;
  }

  for (i=MOTH_LEFT; i<=MOTH_BOTTOM; ++i) {
    moth_image_layout_axis_calc_dependent (&(layout->axes[i-MOTH_LEFT]),
        &layout->canvas.region);
  }

  image->width = layout->width;
  image->height = layout->height;
  moth_image_layout_layout_horizontal (layout, image);
  moth_image_layout_layout_vertical (layout, image);
}



INTERNAL
moth_image_layout
moth_image_layout_create
(
  moth_image image
)
{
  moth_image_layout layout;
  int               i;

  layout = (moth_image_layout)
    malloc (sizeof(struct moth_image_layout_s));
  if (!layout)
    die ("couldn't find memory to arrange <image>");

  layout->width = 0;
  layout->height = 0;
  moth_image_layout_string_initialize (&layout->title, image->pixmap,
      image->title, MOTH_HORIZONTAL);

  for (i=MOTH_LEFT; i<=MOTH_BOTTOM; ++i) {
    moth_axis axis;
    axis = moth_image_axis (image, i);
    moth_image_layout_axis_initialize (&(layout->axes[i-MOTH_LEFT]), i, image);
  }

  moth_image_layout_canvas_initialize (&layout->canvas);
  moth_image_layout_legend_initialize (&layout->legend, image);

  moth_image_layout_layout (layout, image);
  return layout;
}



INTERNAL
void
moth_image_layout_draw
(
  moth_image_layout layout,
  moth_image        image
)
{
  moth_layer          layer;
  size_t              l;
  moth_color          black;

  black = moth_image_get_color (image, "black");

  /* canvas background */
  moth_image_layout_draw_canvas (&layout->canvas, image);

  /* version watermark */
  moth_image_layout_draw_watermark (layout, image);

  /* title */
  if (layout->title.string)
    moth_image_layout_draw_title (layout, image);

  /* axes */
  moth_image_layout_draw_axes (layout, image);

  /* layers */
  l = 0;
  for (layer=image->layers; layer; layer=layer->next) {
    moth_axis           x_axis, y_axis;
    moth_format_legend  legend;

    /* set viewports */
    x_axis = moth_image_axis (image, layer->x_axis);
    y_axis = moth_image_axis (image, layer->y_axis);
    if (!x_axis || !y_axis)
      die ("missing x or y axis for <%s>", moth_layer_find_name(layer->type));

    moth_image_set_x_viewport (image, 
        layout->canvas.region.x, layout->canvas.region.width - 1.0,
        x_axis->scaling, x_axis->min, x_axis->max);
    moth_image_set_y_viewport (image, 
        layout->canvas.region.y, layout->canvas.region.height - 1.0,
        y_axis->scaling, y_axis->min, y_axis->max);

    /* build legend */
    legend = NULL;
    if (layer->title) {
      legend = (moth_format_legend)
        malloc (sizeof(struct moth_format_legend_s));
      legend->swatch[0].x = layout->legend.region.x
        + layout->legend.layers[l].region.x;
      legend->swatch[0].y = layout->legend.region.y
        + layout->legend.layers[l].region.y;
      legend->swatch[1].x = legend->swatch[0].x + MOTH_IMAGE_LEGEND_SWATCHSIZE;
      legend->swatch[1].y = legend->swatch[0].y + MOTH_IMAGE_LEGEND_SWATCHSIZE;

      legend->title = (moth_format_text)
        malloc (sizeof(struct moth_format_text_s));
      legend->title->location.x = layout->legend.region.x
        + layout->legend.layers[l].region.x
        + MOTH_IMAGE_LEGEND_SWATCHSIZE + MOTH_IMAGE_SPACER - 1.0;
      legend->title->location.y = layout->legend.region.y
        + layout->legend.layers[l].region.y;
      legend->title->string = layer->title;
      legend->title->color = black;
      legend->title->flags = 0;

      ++l;
    }

    moth_layer_draw (layer, image, legend, image->formats, image->pixmap);

    if (legend) {
      free (legend->title);
      free (legend);
    }
  }

  moth_color_destroy (black);
}



INTERNAL
void
moth_image_layout_destroy
(
  moth_image_layout layout
)
{
  int i;

  moth_image_layout_string_destroy (&layout->title);
  for (i=MOTH_LEFT; i<=MOTH_BOTTOM; ++i) {
    moth_image_layout_axis_destroy (&(layout->axes[i-MOTH_LEFT]));
  }
  moth_image_layout_canvas_destroy (&layout->canvas);
  moth_image_layout_legend_destroy (&layout->legend);

  free (layout);
}



/*--------------------------------------------------------*\
|                     image functions                      |
\*--------------------------------------------------------*/

moth_image
moth_image_create
(
  const char **attributes
)
{
  moth_image image;
  int got_width, got_height;
  int a;

  image = (moth_image) malloc (sizeof(struct moth_image_s));
  if (!image)
    return NULL;
  image->filename = NULL;
  image->width_attribute = 0;
  image->height_attribute = 0;
  image->width = 0;
  image->height = 0;
  image->size_type = MOTH_SIZE_TYPE_CANVAS;
  image->title = NULL;
  image->formats = NULL;
  image->axes[  MOTH_LEFT-MOTH_LEFT] = moth_axis_create (MOTH_LEFT);
  image->axes[   MOTH_TOP-MOTH_LEFT] = moth_axis_create (MOTH_TOP);
  image->axes[ MOTH_RIGHT-MOTH_LEFT] = moth_axis_create (MOTH_RIGHT);
  image->axes[MOTH_BOTTOM-MOTH_LEFT] = moth_axis_create (MOTH_BOTTOM);
  image->layers = NULL;
  image->background = NULL;
  image->canvas = NULL;
  image->border_color = NULL;
  image->border_size = MOTH_IMAGE_DEFAULT_BORDERSIZE;
  image->colors = moth_color_store_create ();

  moth_image_reset_viewports (image);
  image->pixmap = NULL;
  image->draw_pixmap = 0;

  got_width = got_height = 0;
  for (a=0; attributes[a]; a+=2) {
    const char  *attname = attributes[a];
    const char  *attvalue = attributes[a+1];

    if (0==strcasecmp("filename",attname)) {
      image->filename = strdup (attvalue);
    }
    else if (0==strcasecmp("fileformat",attname)) {
      image->formats = moth_format_create (attvalue);
    }
    else if (0==strcasecmp("width",attname)) {
      if (1!=sscanf(attvalue,"%u",&(image->width_attribute)))
        die ("couldn't understand <moth> attribute %s=\"%s\"", attname,
            attvalue);
      got_width = 1;
    }
    else if (0==strcasecmp("height",attname)) {
      if (1!=sscanf(attvalue,"%u",&(image->height_attribute)))
        die ("couldn't understand <moth> attribute %s=\"%s\"", attname,
            attvalue);
      got_height = 1;
    }
    else if (0==strcasecmp("size-type",attname)) {
      if (0==strcasecmp("image",attvalue)) {
        image->size_type = MOTH_SIZE_TYPE_IMAGE;
      }
      else if (0==strcasecmp("canvas",attvalue)) {
        image->size_type = MOTH_SIZE_TYPE_CANVAS;
      }
      else
        die ("couldn't understand <moth> attribute %s=\"%s\"", attname,
            attvalue);
    }
    else if (0==strcasecmp("title",attname)) {
      image->title = strdup (attvalue);
    }
    else if (0==strcasecmp("background",attname)) {
      image->background = moth_image_get_color (image, attvalue);
    }
    else if (0==strcasecmp("canvas",attname)) {
      image->canvas = moth_image_get_color (image, attvalue);
    }
    else if (0==strcasecmp("border-color",attname)) {
      image->border_color = moth_image_get_color (image, attvalue);
    }
    else if (0==strcasecmp("border-size",attname)) {
      if (1!=sscanf(attvalue,"%u",&(image->border_size)))
        die ("couldn't understand <moth> attribute %s=\"%s\"", attname,
            attvalue);
    }
    else {
      warn ("unknown <moth> attribute %s=\"%s\"", attname, attvalue);
    }
  }

  if (! image->filename)
    die ("missing <moth> attribute \"filename\"");
  if (! got_width)
    die ("missing <moth> attribute \"width\"");
  if (! got_height)
    die ("missing <moth> attribute \"height\"");
  if (! image->background)
    image->background = moth_image_get_color (image,
        MOTH_IMAGE_BACKGROUND_COLOR_DEFAULT);
  if (! image->canvas)
    image->canvas = moth_image_get_color (image,
        MOTH_IMAGE_BACKGROUND_COLOR_DEFAULT);
  if (! image->border_color)
    image->border_color = moth_image_get_color (image,
        MOTH_IMAGE_BORDER_COLOR_DEFAULT);

  /* auto-detect format from filename extension */
  if (! image->formats) {
    char *period;
    period = strrchr (image->filename, '.');
    if (period) {
      moth_format format;
      char        *oldname;
      size_t      name_length;

      format = moth_format_create (period+1);
      if (!format)
        die ("unknown file format \"%s\"", period+1);
      image->formats = format;

      /* normalize filename to version w/out suffix */
      oldname = image->filename;
      name_length = period - oldname;
      image->filename = (char *) malloc (name_length+1);
      strncpy (image->filename, oldname, name_length);
      image->filename[name_length] = '\0';
      free (oldname);
    }
    else {
      die ("missing <moth> attribute \"fileformat\", and can't auto-detect");
    }
  }

  /* query formats for need of pixmap */
  image->draw_pixmap = moth_format_is_pixmap (image->formats);

  return image;
}



/* write image to files */
INTERNAL
void
moth_image_write
(
  moth_image image
)
{
  /* run all file formats */
  moth_format_save_pixmap (image->formats, image->filename, image->pixmap);
}



moth_axis
moth_image_axis
(
  moth_image          image,
  moth_axis_location  axis
)
{
  if (!image)
    return NULL;
  if (MOTH_LOCATION_UNKNOWN == axis) {
    warn ("unknown <axis> location");
    return NULL;
  }
  return image->axes[axis-MOTH_LEFT];
}



void
moth_image_add_layer
(
  moth_image  image,
  moth_layer  layer
)
{
  if (image->layers)
    moth_layer_add_layer (image->layers, layer);
  else
    image->layers = layer;
}



moth_color
moth_image_get_color
(
  moth_image  image,
  const char  *name
)
{
  return moth_color_store_get (image->colors, name);
}



moth_color
moth_image_get_autocolor
(
  moth_image image
)
{
  return moth_color_store_get_auto (image->colors);
}



void
moth_image_dump
(
  moth_image        image
)
{
  moth_axis_location  a;
  moth_layer          layer;

  fprintf (stdout, "  <image");
  if (image->filename)
    fprintf (stdout, " filename=\"%s\"", image->filename);
  if (image->formats) {
    fprintf (stdout, " fileformat=\"");
    moth_format_dump (image->formats);
    fprintf (stdout, "\"");
  }
  if (image->width_attribute)
    fprintf (stdout, "\n    width=\"%u\"", image->width_attribute);
  if (image->height_attribute)
    fprintf (stdout, "\n    height=\"%u\"", image->height_attribute);
  if (MOTH_SIZE_TYPE_IMAGE == image->size_type)
    fprintf (stdout, "\n    size-type=\"image\"");
  else
    fprintf (stdout, "\n    size-type=\"canvas\"");
  if (image->title)
    fprintf (stdout, "\n    title=\"%s\"", image->title);

  if (image->background) {
    fprintf (stdout, "\n    background=\"");
    moth_color_dump (image->background);
    fprintf (stdout, "\"");
  }
  if (image->canvas) {
    fprintf (stdout, "\n    canvas=\"");
    moth_color_dump (image->canvas);
    fprintf (stdout, "\"");
  }
  if (image->border_color) {
    fprintf (stdout, "\n    border-color=\"");
    moth_color_dump (image->border_color);
    fprintf (stdout, "\"");
  }
  fprintf (stdout, "\n    border-size=\"%u\"", image->border_size);
  fprintf (stdout, "\n  >\n");

  /* axes */
  for (a=MOTH_LEFT; a<=MOTH_BOTTOM; a++) {
    moth_axis axis;
    axis = moth_image_axis (image, a);
    moth_axis_dump (axis);
  }

  /* layers */
  for (layer=image->layers; layer; layer=layer->next) {
    moth_layer_dump (layer);
  }

  fprintf (stdout, "  </image>\n");
}



void
moth_image_process
(
  moth_image        image
)
{
  moth_axis_location  a;
  moth_image_layout   layout;

  for (a=MOTH_LEFT; a<=MOTH_BOTTOM; a++) {
    moth_axis axis;
    int       max_pixels;

    axis = moth_image_axis (image, a);
    if (!axis || !(axis->configured))
      continue;

    /* pass along size of axis in pixels.  (ok, it's just a guess.)  */
    switch (a) {
      case MOTH_LEFT:
      case MOTH_RIGHT:
        max_pixels = image->height_attribute;
        break;
      case MOTH_TOP:
      case MOTH_BOTTOM:
        max_pixels = image->width_attribute;
        break;
      default:
        max_pixels = 0;
        break;
    }
    moth_axis_process (axis, max_pixels);
  }

  /* 
   * Always create a pixmap, since we'll need to use it to find the 
   * geometry of various strings.
   */
  image->pixmap = moth_pixmap_create ();
  layout = moth_image_layout_create (image);

  moth_pixmap_start_image (image->pixmap, layout->width, layout->height,
      image->background, image->border_color, image->border_size);

  moth_format_start_image (image->formats, image->filename, layout->width,
      layout->height, image->background, image->border_color,
      image->border_size);

  moth_image_reset_viewports (image);
  moth_image_layout_draw (layout, image);
  moth_image_write (image);

  moth_format_finish_image (image->formats);
  moth_pixmap_finish_image (image->pixmap);
  moth_pixmap_destroy (image->pixmap);
  moth_image_layout_destroy (layout);
}



void
moth_image_destroy
(
  moth_image image
)
{
  moth_axis_location axis;

  if (image->filename)
    free (image->filename);
  if (image->title)
    free (image->title);
  if (image->formats)
    moth_format_destroy (image->formats);
  for (axis=MOTH_LEFT; axis<=MOTH_BOTTOM; axis++) {
    moth_axis_destroy (moth_image_axis(image,axis));
  }
  if (image->layers)
    moth_layer_destroy (image->layers);
  if (image->background)
    moth_color_destroy (image->background);
  if (image->canvas)
    moth_color_destroy (image->canvas);
  if (image->border_color)
    moth_color_destroy (image->border_color);

  if (image->colors)
    moth_color_store_destroy (image->colors);

  free (image);
}



