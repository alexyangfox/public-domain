/*
 * $Id: file.c,v 1.4 2003/06/10 16:26:06 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See file.dev for notes and coding issues.
 *
 */



#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "columns.h"
#include "file.h"
#include "image.h"
#include "options.h"
#include "parser.h"
#include "sources.h"



/*--------------------------------------------------------*\
|                      datastructures                      |
\*--------------------------------------------------------*/

struct moth_file_private_s {
  size_t            n_sources;
  moth_source       *sources;
  size_t            n_images;
  moth_image        *images;
};



/*--------------------------------------------------------*\
|                 static (non object) API                  |
\*--------------------------------------------------------*/

char *
moth_file_find
(
  const char *filename
)
{
  char  *interpretted_filename;

  if (!filename || !strlen(filename))
    return NULL;

  if (moth_options_get_option(runtime->options,"xml-relative")) {
    /* File is specified with an absolute path.  */
    if ('/' == filename[0]) {
      /* just copy, so that we can always free() below.  */
      interpretted_filename = strdup (filename);
    }

    /* Determine file using current working directory and path to XML file.  */
    else {
      /* find path to xml file */
      char    *xml_filename, *xml_dirname;
      size_t  interpretted_size, written_size;
      xml_filename = strdup (runtime->file->filename);
      xml_dirname = dirname (xml_filename);
      interpretted_size = strlen(xml_dirname) 
        + 1                   /* slash ('/') character */
        + strlen(filename) 
        + 1;                  /* null-terminating character */
      interpretted_filename = (char *) malloc (interpretted_size*sizeof(char));
      written_size = snprintf (interpretted_filename, interpretted_size,
          "%s/%s", xml_dirname, filename);
      if ((written_size+1) != interpretted_size) {
        warn ("couldn't calculate filename while using --xml-relative");
        return NULL;
      }
      free (xml_filename);
    }
  }

  /* Use current working directory (which happens by default).  */
  else {
    /* just copy, so that we can always free() */
    interpretted_filename = strdup (filename);
  }

  return interpretted_filename;
}



/*--------------------------------------------------------*\
|                        public API                        |
\*--------------------------------------------------------*/

moth_file
moth_file_create
(
  const char *filename
)
{
  moth_file file;

  file = (moth_file) malloc (sizeof(struct moth_file_s));
  if (!file) return NULL;

  file->filename = strdup (filename);
  file->parser = NULL;
  file->columns = moth_column_store_create ();
  file->_p = (moth_file_private) malloc (sizeof(struct moth_file_private_s));
  file->_p->n_sources = 0;
  file->_p->sources = NULL;
  file->_p->n_images = 0;
  file->_p->images = NULL;

  if (!file->filename || !file->columns || !file->_p) {
    moth_file_destroy (file);
    fprintf (stderr, "ERROR:  couldn't allocate memory for file object\n");
    exit (-1);
  }

  return file;
}



void
moth_file_parse
(
  moth_file file
)
{
  FILE  *inFile;
  int   openned;

  if (0==strcmp("-",file->filename)) {
    inFile = stdin;
    openned = 0;
  }
  else {
    inFile = fopen (file->filename, "rb");
    if (!inFile) {
      perror (file->filename);
      return;
    }
    openned = 1;
  }

  file->parser = moth_parser_create ();
  if (!file->parser)
    die ("couldn't create parser");
  moth_parser_read (file->parser, inFile);
  moth_parser_destroy (file->parser);
  file->parser = NULL;

  if (openned)
    fclose (inFile);
}



void
moth_file_dump
(
  moth_file file
)
{
  size_t i;

  fprintf (stdout, "<moth");
  moth_options_dump_attributes (runtime->options);
  fprintf (stdout, ">\n");

  for (i=0; i<file->_p->n_sources; ++i) {
    fprintf (stdout, "\n");
    moth_source_dump (file->_p->sources[i]);
  }
  for (i=0; i<file->_p->n_images; ++i) {
    fprintf (stdout, "\n");
    moth_image_dump (file->_p->images[i]);
  }

  fprintf (stdout, "</moth>\n");
}



void
moth_file_process_sources
(
  moth_file file
)
{
  size_t i;
  for (i=0; i<file->_p->n_sources; ++i) {
    moth_source_process (file->_p->sources[i]);
  }
}



void
moth_file_draw_images
(
  moth_file file
)
{
  size_t i;
  if (! moth_options_get_option(runtime->options,"draw")) {
    return;
  }
  for (i=0; i<file->_p->n_images; ++i) {
    moth_image_process (file->_p->images[i]);
  }
}



void
moth_file_destroy
(
  moth_file file
)
{
  if (file->filename) free (file->filename);
  if (file->parser)   moth_parser_destroy (file->parser);
  if (file->columns)  moth_column_store_destroy (file->columns);
  if (file->_p) {
    size_t i;

    if (file->_p->sources) {
      for (i=0; i<file->_p->n_sources; ++i) {
        moth_source_destroy (file->_p->sources[i]);
      }
      free (file->_p->sources);
    }

    if (file->_p->images) {
      for (i=0; i<file->_p->n_images; ++i) {
        moth_image_destroy (file->_p->images[i]);
      }
      free (file->_p->images);
    }

    free (file->_p);
  }

  free (file);
}



void
moth_file_add_source
(
  moth_file     file,
  moth_source   source
)
{
  size_t      n_old;
  moth_source *old;

  n_old = file->_p->n_sources;
  old = file->_p->sources;

  /* make new */
  file->_p->n_sources = n_old + 1;
  file->_p->sources = (moth_source *)
    calloc (file->_p->n_sources, sizeof(moth_source));

  /* copy old (and delete) */
  if (old) {
    size_t i;
    for (i=0; i<n_old; ++i) {
      file->_p->sources[i] = old[i];
    }
    free (old);
  }

  /* add new source */
  file->_p->sources[file->_p->n_sources-1] = source;
}



void
moth_file_add_image
(
  moth_file   file,
  moth_image  image
)
{
  size_t      n_old;
  moth_image  *old;

  n_old = file->_p->n_images;
  old = file->_p->images;

  /* make new */
  file->_p->n_images = n_old + 1;
  file->_p->images = (moth_image *)
    calloc (file->_p->n_images, sizeof(moth_image));

  /* copy old (and delete) */
  if (old) {
    size_t i;
    for (i=0; i<n_old; ++i) {
      file->_p->images[i] = old[i];
    }
    free (old);
  }

  /* add new image */
  file->_p->images[file->_p->n_images-1] = image;
}



