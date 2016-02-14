/*
 * $Id: file.h,v 1.2 2003/05/01 00:01:24 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See file.dev for notes and coding issues.
 *
 */



#ifndef MOTH_FILE_H
#define MOTH_FILE_H


#include "data.h"



typedef struct moth_file_private_s *moth_file_private;
struct moth_file_s {
  char *            filename;
  moth_parser       parser;
  moth_column_store columns;

  moth_file_private _p;
};



char *      moth_file_find  (const char *filename);
/* Returns malloc()ed filename, taking into consideration the command-line
 * arguments and the attributes to <moth>.  Returns NULL on error.  */


moth_file  moth_file_create (const char *filename);
void       moth_file_parse (moth_file);
void       moth_file_dump (moth_file);
void       moth_file_process_sources (moth_file);
void       moth_file_draw_images (moth_file);
void       moth_file_destroy (moth_file);

void       moth_file_add_source (moth_file, moth_source);
/* Used by moth_parser to add the source to the file.
 * Takes ownership of the source object.  */

void       moth_file_add_image (moth_file, moth_image);
/* Used by moth_parser to add the image to the file.
 * Takes ownership of the image object.  */



#endif /* MOTH_FILE_H */

