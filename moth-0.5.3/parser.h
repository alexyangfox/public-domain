/*
 * $Id: parser.h,v 1.6 2003/04/21 00:38:18 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See parser.dev for notes and coding issues.
 *
 */

#ifndef MOTH_PARSER_H
#define MOTH_PARSER_H



moth_parser   moth_parser_create      ();

void          moth_parser_read        (moth_parser parser, FILE *file);

void          moth_parser_streamstats (moth_parser parser, 
                                          int *line, int *column);

moth_image    moth_parser_current_image (moth_parser parser);
/* Returns the image that we are currently creating.  */

void          moth_parser_destroy     (moth_parser parser);



#endif

