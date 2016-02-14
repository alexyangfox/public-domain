/*
 * $Id: image.h,v 1.8 2003/06/05 20:33:05 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See image.dev for implementation notes.
 *
 */


#ifndef MOTH_IMAGE_H
#define MOTH_IMAGE_H



#include "colors.h"
#include "data.h"



moth_image      moth_image_create     (const char **attributes);
void            moth_image_add_layer  (moth_image image, moth_layer layer);
moth_axis       moth_image_axis       (moth_image image,
                                          moth_axis_location axis);
moth_color      moth_image_get_color  (moth_image image, const char *name);
moth_color      moth_image_get_autocolor  (moth_image image);
void            moth_image_dump       (moth_image image);
void            moth_image_process    (moth_image image);
void            moth_image_destroy    (moth_image image);

/* viewport info */
void            moth_image_get_viewport_value (moth_image image,
                                               double *x, double *y);

/* canvas info */
double          moth_image_get_min    (moth_image image);
double          moth_image_get_max    (moth_image image);


#endif

