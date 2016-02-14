
/*
 * $Id: colors.h,v 1.3 2003/05/01 00:01:22 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See colors.dev for implementation notes.
 *
 */

#ifndef MOTH_COLORS_H
#define MOTH_COLORS_H



#include "data.h"



/*--------------------------------------------------------*\
|                     data structures                      |
\*--------------------------------------------------------*/

struct moth_color_s {
  short unsigned int  red;
  short unsigned int  green;
  short unsigned int  blue;
  short unsigned int  alpha;
};



/*--------------------------------------------------------*\
|                        public API                        |
\*--------------------------------------------------------*/

moth_color_store    moth_color_store_create ();

moth_color          moth_color_store_get (moth_color_store store, const char *name);
/* Creates a color from a string.  Colors can be specified
 * with a name or in #RRGGBB format, where RR, GG, and BB
 * are respectively the red, green, and blue components of
 * the color given in hexadecimal format.  The alpha level
 * can optionally be include, and defaults to 'FF' (opaque)
 * if not given.  Format is #RRGGBBAA for color values,
 * and name#AA for named colors.
 */

void                moth_color_store_list_named ();
/* List to STDOUT all the named colors.  */

moth_color          moth_color_store_get_auto (moth_color_store store);
/* Returns one of the automatically chosen colors, or black
 * if we've run out.
 */

void                moth_color_store_reset_auto (moth_color_store store);
/* Resets the list of automatically chosen colors.  */

void                moth_color_store_destroy (moth_color_store store);


void                moth_color_dump (moth_color color);

void                moth_color_destroy (moth_color color);
/* Destroys a color returned by the store.  */



#endif  /* MOTH_COLORS_H */


