/*
 * $Id: options.h,v 1.3 2003/05/01 00:01:27 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See options.dev for implementation notes.
 *
 */

#ifndef MOTH_OPTIONS_H
#define MOTH_OPTIONS_H



#include "data.h"



moth_options  moth_options_create ();
void          moth_options_destroy (moth_options options);

int           moth_options_read_argument (moth_options options,
                                            const char *argument);
                                          /* Returns success as a boolean.  */

int           moth_options_set_attribute (moth_options options,
                                            const char *attname,
                                            const char *attvalue);

void          moth_options_dump_attributes (moth_options options);

void          moth_options_reset_attributes (moth_options options);

int           moth_options_get_option (moth_options options,
                                            const char *option);



#endif  /* MOTH_OPTIONS_H */

