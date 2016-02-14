/*
 * $Id: colors.c,v 1.5 2003/06/06 16:51:45 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See colors.dev for implementation notes.
 *
 */



#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



#include "colors.h"
#include "data.h"
#include "rgb.c"



/*--------------------------------------------------------*\
|                     data structures                      |
\*--------------------------------------------------------*/
typedef struct moth_color_store_entry_s  *moth_color_store_entry;
struct moth_color_store_entry_s {
  moth_color              color;
  moth_color_store_entry  next;   /* linked list */
};


struct moth_color_store_s {
  moth_color_store_entry  auto_list;
  moth_color_store_entry  auto_iterator;  /* next available color */
};



/*--------------------------------------------------------*\
|                    utility functions                     |
\*--------------------------------------------------------*/

INTERNAL
moth_color
moth_color_create ()
{
  moth_color color;
  color = (moth_color) malloc (sizeof(struct moth_color_s));
  color->red = 0;
  color->green = 0;
  color->blue = 0;
  color->alpha = 0;
  return color;
}



INTERNAL
moth_color
moth_color_store_find_name
(
  moth_color_store  store,
  const char        *string
)
{
  moth_color_entry  c;
  c = moth_color_predefined;
  while (c->name) {
    if (0==strcasecmp(string,c->name))
      return &(c->color);
    ++c;
  }
  return NULL;
}





/*--------------------------------------------------------*\
|                        public API                        |
\*--------------------------------------------------------*/

moth_color_store
moth_color_store_create ()
{
  moth_color_store store;
  const char *auto_text;
  short unsigned int red, green, blue;
  moth_color_store_entry last_entry;

  store = (moth_color_store)
    malloc (sizeof(struct moth_color_store_s));
  store->auto_list = NULL;
  store->auto_iterator = NULL;

  auto_text = MOTH_COLOR_AUTO_LIST;
  red = green = blue = 0;
  last_entry = NULL;
  while (3==sscanf(auto_text, "#%02hX%02hX%02hX", &red, &green, &blue)) {
    moth_color_store_entry new_entry;

    new_entry = (moth_color_store_entry)
      malloc (sizeof(struct moth_color_store_entry_s));
    new_entry->color = moth_color_create ();
    new_entry->color->red = red;
    new_entry->color->green = green;
    new_entry->color->blue = blue;
    new_entry->color->alpha = 255;
    new_entry->next = NULL;

    if (last_entry)
      last_entry->next = new_entry;
    else
      store->auto_list = new_entry;

    last_entry = new_entry;
    auto_text += 7;
  }

  moth_color_store_reset_auto (store);
  return store;
}



void
moth_color_store_list_named ()
{
  moth_color_entry entry;
  entry = moth_color_predefined;
  printf ("NAME\t\t#RRGGBBAA\n");
  while (entry->name) {
    printf ("%s\t\t#%02X%02X%02X%02X\n",
        entry->name, entry->color.red, entry->color.green, 
        entry->color.blue, entry->color.alpha);
    ++entry;
  }
}



moth_color
moth_color_store_get
(
  moth_color_store  store,
  const char        *name
)
{
  moth_color          color, found;
  char                namedcolor[30];
  short unsigned int  alpha;

  color = moth_color_create ();
  found = NULL;
  
  /* #RRGGBBAA */
  if (4==sscanf(name, "#%02hx%02hx%02hx%02hx",
              &(color->red), &(color->green),
              &(color->blue), &(color->alpha)) )
  {
    /* nothing to do, sscanf() did all the work */
  }

  /* #RRGGBB   */
  else if (3==sscanf(name, "#%02hx%02hx%02hx",
              &(color->red), &(color->green),
              &(color->blue)) )
  {
    color->alpha = 255;
  }

  /* color#AA  */
  else if (2==sscanf(name, "%29[^#]#%02hx", namedcolor, &alpha))
  {
    found = moth_color_store_find_name (store, namedcolor);
    if (found) {
      color->red = found->red;
      color->green = found->green;
      color->blue = found->blue;
      color->alpha = alpha;
    }
  }

  /* color     */
  else if (1==sscanf(name, "%29[^#]", namedcolor))
  {
    found = moth_color_store_find_name (store, namedcolor);
    if (found) {
      color->red = found->red;
      color->green = found->green;
      color->blue = found->blue;
      color->alpha = found->alpha;
    }
  }

  /* failure */
  else {
    free (color);
    return NULL;
  }

  /* FUTURE
   *    Remove any auto-color that is very similar to the returned color.
   */
  return color;
}



moth_color
moth_color_store_get_auto
(
  moth_color_store store
)
{
  moth_color_store_entry entry;  /* available auto color */
  moth_color             color;

  if (!store) {
    color = moth_color_create ();
    color->red    = 0;
    color->green  = 0;
    color->blue   = 0;
    color->alpha  = 255;
    return color;
  }

  entry = store->auto_iterator;
  if (entry) {
    color = moth_color_create ();
    color->red = entry->color->red;
    color->green = entry->color->green;
    color->blue = entry->color->blue;
    color->alpha = entry->color->alpha;

    store->auto_iterator = entry->next;
  }
  else {
    color = moth_color_store_get (store, "black");
  }

  return color;
}



void
moth_color_store_reset_auto
(
  moth_color_store store
)
{
  if (!store)
    return;
  store->auto_iterator = store->auto_list;
}



void
moth_color_store_destroy
(
  moth_color_store store
)
{
  moth_color_store_entry doomed, t;

  if (!store)
    return;

  t = store->auto_list;
  while (t) {
    doomed = t;
    t = t->next;
    moth_color_destroy (doomed->color);
    free (doomed);
  }

  free (store);
}



void
moth_color_dump
(
  moth_color color
)
{
  fprintf (stdout, "#%02X%02X%02X%02X", color->red, color->green, color->blue, color->alpha);
}



void
moth_color_destroy
(
  moth_color color
)
{
  free (color);
}



