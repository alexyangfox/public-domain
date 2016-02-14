/*
 * $Id: parser.c,v 1.25 2003/04/21 00:38:18 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See parser.dev for notes and coding issues.
 *
 */


#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#include <expat.h>

#include "axes.h"
#include "columns.h"
#include "file.h"
#include "image.h"
#include "layers.h"
#include "options.h"
#include "parser.h"
#include "sources.h"



#define MOTH_PARSER_BUFFER_SIZE 1024



/*--------------------------------------------------------*\
|                      datastructures                      |
\*--------------------------------------------------------*/

/* 
 * The reason that we define this separate structure is so that the fact that
 * we are using expat is hidden from the rest of the program.
 */

struct moth_parser_s {
  XML_Parser        expat;
  unsigned int      got_moth;

  moth_image        current_image;
  moth_source       current_source;
  moth_axis         current_axis;
  moth_label        current_label;
};



/*--------------------------------------------------------*\
|                       XML handling                       |
\*--------------------------------------------------------*/

/* <moth> encountered */
/* In reality, this function will only be called once for each file.  This
 * is because of expat, which requires an single opening and closing tag
 * pair to contain the whole XML structure.
 */
INTERNAL
void
moth_parser_start
(
  moth_parser parser,
  const char  **attributes
)
{
  int a;

  if (parser->got_moth)
    die ("<moth> cannot be nested");
  parser->got_moth = 1;

  /* process attributes */
  for (a=0; attributes[a]; a+=2) {
    const char *attname = attributes[a];
    const char *attvalue = attributes[a+1];

    if (! moth_options_set_attribute(runtime->options,attname,attvalue)) {
      warn ("unknown attribute %s=\"%s\"", attname, attvalue);
    }
  }
}



/* </moth> encountered */
INTERNAL
void
moth_parser_finish
(
  moth_parser parser
)
{
  parser->got_moth = 0;
  if (parser->current_image)
    die ("matching </image> never found");
  if (parser->current_source)
    die ("matching </%s> never found", 
        moth_source_find_name(parser->current_source->type));
  if (parser->current_axis)
    die ("matching </axis> never found");
  if (parser->current_label)
    die ("matching </label> never found");
}



/* <axis> encountered */
/* Configure the axis object from the attributes.  We keep a pointer to this
 * object around so that we know to which axis any upcoming labels belong.
 */
INTERNAL
void
moth_parser_axis_start
(
  moth_parser parser,
  const char  **attributes
)
{
  moth_axis_location location;
  moth_axis axis;

  if (!parser->got_moth)
    die ("<axis> cannot be specified outside <image>");
  if (!parser->current_image)
    die ("<axis> must be specified inside <image>");
  if (parser->current_source)
    die ("<axis> must not be specified inside <%s>",
        moth_source_find_name(parser->current_source->type));
  if (parser->current_axis)
    die ("<axis> must not be nested");
  if (parser->current_label)
    die ("<axis> must not be specified inside <label>");

  /* iterate over axes, trying to configure.  if none succeed, trouble */
  for (location=MOTH_LEFT; location<=MOTH_BOTTOM; location++) {
    int res;
    axis = moth_image_axis (parser->current_image, location);
    res = moth_axis_configure (axis, attributes);
    switch (res) {
      case -1:
        continue;

      case 1:
        parser->current_axis = axis;
        return;

      default:
      case 0:
        die ("couldn't configure %s <axis>", moth_axis_find_name(location));
        break;
    }
  }

  die ("couldn't configure %s <axis>", moth_axis_find_name(location));
}



/* </axis> or <axis ... /> encountered */
INTERNAL
void
moth_parser_axis_finish
(
  moth_parser parser
)
{
  parser->current_axis = NULL;
}



/* <label> encountered */
/* Labels are stored temporarily before attaching.  This is because, for most
 * cases, they can't be constructed strictly from attributes.
 */
INTERNAL
void
moth_parser_label_start
(
  moth_parser parser,
  const char  **attributes
)
{
  moth_label label;

  if (!parser->got_moth)
    die ("<axis> cannot be specified outside <moth>");
  if (!parser->current_image || !parser->current_axis)
    die ("<label> must be specified inside <axis>");
  if (parser->current_source)
    die ("<label> cannot be specified inside <%s>",
        moth_source_find_name(parser->current_source->type));
  if (parser->current_label)
    die ("<label> must not be nested");

  label = moth_label_create (attributes);
  if (!label)
    die ("couldn't create <label>");

  parser->current_label = label;
}



/* </label> encountered */
INTERNAL
void
moth_parser_label_finish
(
  moth_parser parser
)
{
  if (! parser->current_label)
    die ("<label> must end in \"/>\"");

  moth_axis_add_label (parser->current_axis, parser->current_label);
  parser->current_label = NULL;
}



/* <image> encountered */
INTERNAL
void
moth_parser_image_start
(
  moth_parser parser,
  const char  **attributes
)
{
  if (!parser->got_moth)
    die ("<image> must be specified inside <moth>");
  if (parser->current_image)
    die ("<image> cannot be nested");
  if (parser->current_image)
    die ("<image> cannot be nested");
  parser->current_image = moth_image_create (attributes);
}



/* </image> encountered */
INTERNAL
void
moth_parser_image_finish
(
  moth_parser parser
)
{
  if (parser->current_source)
    die ("matching </%s> never found",
        moth_source_find_name(parser->current_source->type));
  if (parser->current_axis)
    die ("matching </axis> never found");
  if (parser->current_label)
    die ("matching </label> never found");
  if (!parser->current_image)
    die ("openning <image> never found");

  /* The file takes ownership of the image.  */
  moth_file_add_image (runtime->file, parser->current_image);
  parser->current_image = NULL;
}



/* <mysql>, <rpn>, <rrd>, <textfile>, or other source encountered */
INTERNAL
void
moth_parser_source_start
(
  moth_parser parser,
  const char  *name,
  const char  **attributes
)
{
  moth_column_list columns;
  int a;
  columns = NULL;

  if (!parser->got_moth)
    die ("<%s> cannot be specified outside <moth>", name);
  if (parser->current_image)
    die ("<%s> must not be specified inside <image>", name);
  if (parser->current_source)
    die ("<%s> must not be specified inside <%s>", name,
        moth_source_find_name(parser->current_source->type));
  if (parser->current_axis)
    die ("<%s> must not be specified inside <axis>", name);
  if (parser->current_label)
    die ("<%s> must not be specified inside <label>", name);

  /* Build columns from "columns" attribute. This is common for all sources. */
  for (a=0; attributes[a]; a+=2) {
    const XML_Char  *attname = attributes[a];
    const XML_Char  *attvalue = attributes[a+1];
    if (0==strcasecmp("columns",attname)) {
      columns = moth_column_store_get_list (runtime->file->columns, attvalue);
    }
  }
  if (!columns)
    die ("couldn't understand column list in <%s>", name);
  if (0 == moth_column_list_size(columns))
    die ("<%s> requires a comma-separated list of columns",
        name);
  
  /* The source needs to know its columns, because it's going to put data
   * in them.  */
  /* NOTE:  source takes possession of the column list.  It will destroy it. */
  parser->current_source = moth_source_create (name, columns, attributes);
  if (! parser->current_source)
    die ("couldn't create <%s>", name);
}



/* </mysql>, </rpn>, </rrd>, </textfile>, or other source encountered */
INTERNAL
void
moth_parser_source_finish
(
  moth_parser parser,
  const char  *name
)
{
  if (! parser->current_source)
    die ("</%s> must follow a <%s>", name, name);

  /* The file takes ownership of the source.  */
  moth_file_add_source (runtime->file, parser->current_source);
  parser->current_source = NULL;
}



/* <area>, <grid>, <line>, <points>, or other layer encountered */
INTERNAL
void
moth_parser_layer_start
(
  moth_parser parser,
  const char  *name,
  const char  **attributes
)
{
  moth_layer  layer;
  moth_axis   axis;

  if (!parser->got_moth)
    die ("<%s> cannot be specified outside <moth>", name);
  if (!parser->current_image)
    die ("<%s> must be specified inside <moth>", name);
  if (parser->current_source)
    die ("<%s> cannot be specified inside <%s>",
        name, moth_source_find_name(parser->current_source->type));
  if (parser->current_axis)
    die ("<%s> cannot be specified inside <axis>", name);
  if (parser->current_label)
    die ("<%s> cannot be specified inside <label>", name);

  layer = moth_layer_create (name, attributes);
  if (!layer)
    die ("couldn't create <layer>");

  /* associate axis with column, according to the layer attributes */
  if (layer->x_axis) {
    axis = moth_image_axis (parser->current_image, layer->x_axis);
    if (!axis)
      die ("can't add <%s> to <axis> \"%s\"",
          moth_layer_find_name(layer->type),
          moth_axis_find_name(layer->x_axis));
    if (layer->x)
      moth_axis_add_column (axis, layer->x);
    if (layer->x1)
      moth_axis_add_column (axis, layer->x1);
  }
  if (layer->y_axis) {
    axis = moth_image_axis (parser->current_image, layer->y_axis);
    if (!axis)
      die ("can't add <%s> to <axis> \"%s\"",
          moth_layer_find_name(layer->type),
          moth_axis_find_name(layer->y_axis));
    if (layer->y)
      moth_axis_add_column (axis, layer->y);
    if (layer->y1)
      moth_axis_add_column (axis, layer->y1);
  }

  moth_image_add_layer (parser->current_image, layer);
}



/* XML start tag encountered */
/* Here we check to make sure that tags don't occur in inappropriate places.
 * The expat library handles many generic XML cases, such as unbalanced open
 * and close tags.  */
INTERNAL
void
moth_parser_startHandler
(
  void        *context,
  const char  *name,
  const char  **attributes
)
{
  moth_parser parser;
  parser = (moth_parser) context;

  /* moth */
  if (0==strcasecmp("moth",name)) {
    moth_parser_start (parser, attributes);
  }

  /* image */
  else if (0==strcasecmp("image",name)) {
    moth_parser_image_start (parser, attributes);
  }

  /* sources */
  else if (MOTH_SOURCE_UNKNOWN != moth_source_find_type(name)) {
    moth_parser_source_start (parser, name, attributes);
  }

  /* axes */
  else if (0==strcasecmp("axis",name)) {
    moth_parser_axis_start (parser, attributes);
  }
  else if (0==strcasecmp("label",name)) {
    moth_parser_label_start (parser, attributes);
  }

  /* layers */
  else if (MOTH_LAYER_UNKNOWN != moth_layer_find_type(name)) {
    moth_parser_layer_start (parser, name, attributes);
  }

  /* default */
  else {
    warn ("unknown tag <%s>", name);
  }
}



/* contents between start end end tag encountered */
/* Few elements have content that we care about.  */
INTERNAL
void
moth_parser_contentHandler
(
  void        *context,
  const char  *chunk,
  int         chunkLen
)
{
  moth_parser parser;
  parser = (moth_parser) context;

  /* add chunk to current source */
  if (parser->current_source) {
    moth_source_add_chunk (parser->current_source, chunk, (size_t)chunkLen);
  }

  /* add chunk to current label */
  else if (parser->current_label) {
    moth_label_add_chunk (parser->current_label, chunk, (size_t)chunkLen);
  }
  
  /* else, ignore.  most likely just whitespace */
}



/* XML end tag encountered */
/* This is also called for XML tags that end in "/>", which is cool.  That
 * means that some of our tags can be used with or without content without
 * having to code for it explicitly.  An example is <axis>, which can be
 * called like :
 *   <axis ... />
 * .. for axes without labels.
 */
INTERNAL
void
moth_parser_endHandler
(
  void        *context,
  const char  *name
)
{
  moth_parser parser;
  parser = (moth_parser) context;

  /* moth */
  if (0==strcasecmp("moth",name)) {
    moth_parser_finish (parser);
  }

  /* image */
  else if (0==strcasecmp("image",name)) {
    moth_parser_image_finish (parser);
  }

  /* sources */
  else if (MOTH_SOURCE_UNKNOWN != moth_source_find_type(name)) {
    moth_parser_source_finish (parser, name);
  }

  /* axes */
  else if (0==strcasecmp("axis",name)) {
    moth_parser_axis_finish (parser);
  }
  else if (0==strcasecmp("label",name)) {
    moth_parser_label_finish (parser);
  }

  /* layers */
  else if (MOTH_LAYER_UNKNOWN != moth_layer_find_type(name)) {
    /* nothing to do.  all of the layers can be completely constructed
     * from attributes.  */
  }

  /* default */
  else {
    warn ("unknown tag </%s>", name);
  }
}



/*--------------------------------------------------------*\
|                        public API                        |
\*--------------------------------------------------------*/

moth_parser
moth_parser_create ()
{
  moth_parser parser;
  parser = (moth_parser) malloc (sizeof(struct moth_parser_s));
  parser->expat = NULL;
  parser->got_moth = 0;
  parser->current_image = NULL;
  parser->current_source = NULL;
  parser->current_axis = NULL;
  parser->current_label = NULL;
  return parser;
}



void
moth_parser_read
(
  moth_parser parser,
  FILE        *file
)
{
  parser->expat = XML_ParserCreate (NULL);
  if (!parser->expat)
    exit (-1);

  /* register XML callbacks */
  XML_SetUserData (parser->expat, (void *) parser);
  XML_SetElementHandler (parser->expat, &moth_parser_startHandler,
      &moth_parser_endHandler);
  XML_SetCharacterDataHandler (parser->expat, &moth_parser_contentHandler);

  /* process by chunks */
  for (;;) {
    size_t bytes_read;
    void *buffer = XML_GetBuffer (parser->expat, MOTH_PARSER_BUFFER_SIZE);
    if (buffer == NULL) {
      die ("couldn't allocate buffer");
    }

    bytes_read = fread (buffer, 1, MOTH_PARSER_BUFFER_SIZE, file);
    if (bytes_read == 0) {
      if (feof(file))
        break;
      else {
        /* This loops until there is data, so we throttle it to keep it from
         * sucking up -too- much of the system resources.  Most common cases
         * of reaching here are during some kind of filesystem timeout, such
         * as when trying to read from a network filesystem, such as NFS or
         * samba.  */
        sleep (1);
        continue;
      }
    }

    /* During the execution of this function, the expat library will call the
     * functions we specified above.  */
    if (!XML_ParseBuffer(parser->expat, bytes_read, feof(file))) {
      die ("couldn't understand XML");
    }
  }  

  XML_ParserFree (parser->expat);
  parser->expat = NULL;
}



void
moth_parser_streamstats
(
  moth_parser   parser,
  int           *line,
  int           *column
)
{
  if (line)
    *line = XML_GetCurrentLineNumber (parser->expat);
  if (column)
    *column = XML_GetCurrentColumnNumber (parser->expat);
}



moth_image
moth_parser_current_image
(
  moth_parser parser
)
{
  return parser->current_image;
}



void
moth_parser_destroy
(
  moth_parser parser
)
{
  if (!parser) return;

  if (parser->expat)
    XML_ParserFree (parser->expat);

  if (parser->got_moth) {
    warn ("</moth> never specified");
  }
  if (parser->current_image) {
    warn ("leftover <image>");
    moth_image_destroy (parser->current_image);
  }
  if (parser->current_source) {
    warn ("leftover <%s>",
        moth_source_find_name(parser->current_source->type));
    moth_source_destroy (parser->current_source);
  }
  if (parser->current_axis) {
    warn ("leftover <axis>");
    moth_axis_destroy (parser->current_axis);
  }
  if (parser->current_label) {
    warn ("leftover <label>");
    moth_label_destroy (parser->current_label);
  }

  free (parser);
}



