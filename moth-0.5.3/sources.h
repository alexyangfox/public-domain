/*
 * $Id: sources.h,v 1.4 2003/05/01 00:01:28 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See sources.dev for notes and coding issues.
 *
 */

#ifndef MOTH_SOURCES_H
#define MOTH_SOURCES_H

#include "data.h"



typedef enum {
  MOTH_SOURCE_UNKNOWN = 0,
  MOTH_MYSQL,
  MOTH_RPN,
  MOTH_RRD,
  MOTH_TEXTFILE
} moth_source_type;



moth_source_type  moth_source_find_type   (const char *name);
const char *      moth_source_find_name   (moth_source_type type);


moth_source       moth_source_create      (const char *name,
                                            moth_column_list columns,
                                            const char **attributes);
                                          /* 
                                           * Takes possession of the column
                                           * list.
                                           */

void              moth_source_add_chunk   (moth_source source,
                                            const char *chunk,
                                            size_t chunkLen);

void              moth_source_dump        (moth_source source);
void              moth_source_process     (moth_source source);
void              moth_source_destroy     (moth_source source);



struct moth_source_s {
  moth_source_type  type;
  moth_column_list  columns;
  moth_buffer       buffer;
  void              *specifics;   /* type-specific */
};



#endif

