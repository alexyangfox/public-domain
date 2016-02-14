/*
 * $Id: axes.h,v 1.13 2004/03/17 04:10:31 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See axes.dev for notes and coding issues.
 *
 */


#ifndef MOTH_AXES_H
#define MOTH_AXES_H

#include "colors.h"
#include "data.h"



struct moth_axis_s {
  moth_column_list      columns;
  moth_axis_location    location;
  moth_axis_scaling     scaling;
  double                clip_min;
  double                clip_max;
  int                   clip_force;
  double                snap_to_value;
  char                  snap_to_modifier;
  char                  snap_to_unit;
  moth_label            labels;     /* linked list */
  char                  *title;
  moth_color            color;      /* line and title color */
  int                   configured; /* boolean */

  /* not valid until moth_axis_process() has been called */
  double                min;
  double                max;
};



struct moth_label_s {
  moth_label            next;   /* points to next label in linked list */
  moth_color            color;
  double                modulus_value;
  char                  modulus_modifier;
  char                  modulus_unit;
  double                offset_value;
  char                  offset_modifier;
  char                  offset_unit;
  moth_buffer           template;

  /* not valid until moth_axis_process() has been called */
  moth_label_tick       ticks;    /* linked list */
};



struct moth_label_tick_s {
  double                value;  /* power, if logarithmic scale */
  char                  *text;
  moth_label_tick       next;   /* points to next tick in linked list */
};



const char *          moth_axis_find_name (moth_axis_location location);
moth_axis_location    moth_axis_find_location (const char *name);


moth_axis             moth_axis_create    (moth_axis_location location);

int                   moth_axis_configure (moth_axis axis,
                          const char **attributes);
                      /* returns 1 on success, 0 on failure, and -1 if the
                       * configuration information refers to the wrong
                       * axis */

void                  moth_axis_add_column (moth_axis axis,
                          moth_column column);
                      /* associates a column with the axis */

void                  moth_axis_add_label (moth_axis axis, moth_label label);

void                  moth_axis_dump      (moth_axis axis);
                      /* prints axis as XML */

void                  moth_axis_process   (moth_axis axis, int max_pixels);
                      /* 
                       * Finds min and max and creates the ticks.
                       * max_pixels is an estimate of the number of pixels
                       * on which the axis is drawn.  It is used to determine
                       * if the the label modulus will cause an absurd number
                       * (more * than one per pixel) of ticks to be generated.
                       */

void                  moth_axis_destroy   (moth_axis axis);



moth_label    moth_label_create     (const char **attributes);
void          moth_label_add_chunk  (moth_label label, const char *chunk,
                                        size_t chunkLen);
void          moth_label_destroy    (moth_label label);



#endif /* MOTH_AXES_H */

