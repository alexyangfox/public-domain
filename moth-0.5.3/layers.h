/*
 * $Id: layers.h,v 1.11 2003/06/06 15:54:56 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See layers.dev for implementation notes.
 *
 */



#ifndef MOTH_LAYERS_H
#define MOTH_LAYERS_H



#include "colors.h"
#include "data.h"
#include "formats.h"



typedef enum {
  MOTH_LAYER_UNKNOWN = 0,
  MOTH_AREA,
  MOTH_GRID,
  MOTH_LINE,
  MOTH_POINTS,
  MOTH_SEGMENTS
} moth_layer_type;



struct moth_layer_s {
  moth_layer            next;
  moth_layer_type       type;
  char                  *title;
  moth_color            color;
  moth_axis_location    x_axis;
  moth_axis_location    y_axis;
  moth_column           x;  /* x0 for areas and segments */
  moth_column           y;  /* y0 for areas and segments */
  moth_column           x1; /* for areas and segments */
  moth_column           y1; /* for areas and segments */
  void                  *specifics;
};



const char *      moth_layer_find_name  (moth_layer_type type);
moth_layer_type   moth_layer_find_type  (const char *name);


moth_layer    moth_layer_create         (const char* name,
                                            const char **attributes);

void          moth_layer_dump           (moth_layer layer);
                                        /* Prints layer as XML.  */

void          moth_layer_add_layer      (moth_layer list,
                                            moth_layer new_layer);
                                        /* 
                                         * Adds the new layer to the list of
                                         * layers represented by the first
                                         * argument.
                                         */
                                         

void          moth_layer_draw           (moth_layer         layer,
                                         moth_image         image,
                                         moth_format_legend legend,
                                         moth_format        formats,
                                         moth_pixmap        pixmap);
                                        /*
                                         * Draws layer into formats and
                                         * optionally the pixmap.
                                         */


void          moth_layer_destroy        (moth_layer layer);



#endif

