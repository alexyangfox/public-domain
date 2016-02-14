/*
 * $Id: textfile.c,v 1.12 2003/06/06 16:51:46 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 */



#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "../sources.h"
#include "file.h"



#define MOTH_SOURCE_TEXTFILE_BUFFERSIZE 1024
#define MOTH_SOURCE_TEXTFILE_DELIMITER_DEFAULT "\t"
#define MOTH_SOURCE_TEXTFILE_LINETERMINATOR_DEFAULT '\n'



typedef struct moth_source_textfile_specifics_s 
  *moth_source_textfile_specifics;
struct moth_source_textfile_specifics_s {
  char    *filename;
  size_t  skip;
  char    *delimiter;
};



int
moth_source_create_textfile
(
  moth_source source,
  const char  **attributes
)
{
  moth_source_textfile_specifics specs;
  int a;

  specs = (moth_source_textfile_specifics)
    malloc (sizeof(struct moth_source_textfile_specifics_s));
  if (!specs)
    return 0;
  specs->filename = NULL;
  specs->skip = 0;
  specs->delimiter = NULL;

  for (a=0; attributes[a]; a+=2) {
    const char *attname = attributes[a];
    const char *attvalue = attributes[a+1];

    if (0==strcasecmp("columns",attname)) {
      /* ignore.  already been taken care of.  */
    }
    else if (0==strcasecmp("filename",attname)) {
      specs->filename = moth_file_find (attvalue);
      if (!specs->filename)
        warn ("unknown filename \"%s\" in <textfile>", attname);
    }
    else if (0==strcasecmp("skip",attname)) {
      if (1!=sscanf(attvalue,"%u",&(specs->skip))) {
        die ("couldn't understand <textfile> attribute %s=\"%s\"", attname,
            attvalue);
      }
    }
    else if (0==strcasecmp("delimiter",attname)) {
      specs->delimiter = strdup (attvalue);
    }
    else {
      warn ("unknown <textfile> attribute %s=\"%s\"", attname, attvalue);
    }
  }

  /* required / default attributes */
  if (! specs->filename) {
    warn ("missing <textfile> attribute \"filename\"");
    return 0;
  }
  if (! specs->delimiter) {
    /*
     * Since this is going to get free()ed later, we'll just make a copy.
     * Another way to do it is to keep track of whether we malloc()ed or
     * assigned the value, but this way isn't too expensive.
     */
    specs->delimiter = strdup (MOTH_SOURCE_TEXTFILE_DELIMITER_DEFAULT);
  }

  source->specifics = specs;
  return 1;
}



void
moth_source_dump_specifics_textfile
(
  moth_source source
)
{
  moth_source_textfile_specifics specs;
  specs = (moth_source_textfile_specifics) source->specifics;
  if (specs->filename)
    fprintf (stdout, " filename=\"%s\"", specs->filename);
  if (specs->skip)
    fprintf (stdout, " skip=\"%u\"", specs->skip);
  if (specs->delimiter)
    fprintf (stdout, " delimiter=\"%s\"", specs->delimiter);
}



void
moth_source_process_textfile
(
  moth_source source
)
{
  moth_source_textfile_specifics specs;
  FILE          *file;
  char          *buffer, *leftover;
  size_t        num_columns, i;
  size_t        line_number;
  moth_column   *columns;
  size_t        delim_size;

  specs = (moth_source_textfile_specifics) source->specifics;

  file = fopen (specs->filename, "r");
  if (!file)
    die ("couldn't open \"%s\" in <textfile>:  %s", specs->filename,
        strerror(errno));

  buffer = (char *) malloc (MOTH_SOURCE_TEXTFILE_BUFFERSIZE+1);
  if (!buffer)
    die ("couldn't allocate buffer for <textfile>");
  leftover = NULL;
  
  /* make integer-indexed list of columns, for easier reference */
  num_columns = moth_column_list_size (source->columns);
  i = 0;
  columns = (moth_column *) calloc (num_columns, sizeof(moth_column));
  moth_column_list_reset (source->columns);
  while (i<num_columns) {
    columns[i] = moth_column_list_next (source->columns);
    ++i;
  }

  delim_size = strlen (specs->delimiter);
  line_number = 0;
  for (;;) {
    size_t bytes_read, bytes_leftover;
    const char  *line_start, *newline;

    bytes_read = fread (buffer, 1, MOTH_SOURCE_TEXTFILE_BUFFERSIZE, file);
    if (bytes_read == 0) {
      if (feof(file)) {
        break;
      }
      else {
        /* This causes a loop back until there is data,
         * basically polling the file, so we throttle it. */
        sleep (1);
        continue;
      }
    }
    /* since we'll be treating like a string (using strchr() and such) 
     * we'ld better terminate it like a string */
    buffer[bytes_read] = '\0';

    /* prefix leftover to buffer */
    if (leftover) {
      char *new_buffer, *doomed;

      bytes_leftover = strlen (leftover);
      new_buffer = (char *) calloc (
            bytes_leftover + MOTH_SOURCE_TEXTFILE_BUFFERSIZE + 1,
            sizeof(char));
      strcpy (new_buffer, leftover);
      strcpy (new_buffer + bytes_leftover, buffer);
      doomed = buffer;
      buffer = new_buffer;
      free (doomed);
      free (leftover);
      leftover = NULL;
    }

    /* find lines in data */
    line_start = buffer;
    while (( newline
          = strchr(line_start,MOTH_SOURCE_TEXTFILE_LINETERMINATOR_DEFAULT) )) {
      char          *line;
      size_t        line_size;
      const char    *field_start, *delim;
      unsigned int  column;

      /* skip top lines */
      ++line_number;
      if (line_number <= specs->skip) {
        line_start = newline + 1;
        continue;
      }

      line_size = newline - line_start;
      line = (char *) calloc (line_size+1, sizeof(char));
      if (!line)
        die ("couldn't allocate space for line in <textfile>");

      /* null-terminate line so that strstr() doesn't go too far */
      strncpy (line, line_start, line_size);
      line[line_size] = '\0';

      /* find fields in line */
      column = 0;
      field_start = line;
      while (( delim = strstr(field_start,specs->delimiter) )) {
        char      *field;
        size_t    field_size;

        field_size = delim - field_start;
        field = (char *) calloc (field_size+1, sizeof(char));
        if (!field)
          die ("couldn't allocate space for field in <textfile>:  %d", errno);
        strncpy (field, field_start, field_size);
        field[field_size] = '\0';

        /* cram into columns */
        if (column < num_columns)
          moth_column_add (columns[column], read_value(field));

        free (field);
        ++column;
        field_start = delim + delim_size;
      }
      /* handle last column (starting at "field_start") */
      if (column < num_columns)
        moth_column_add (columns[column], read_value(field_start));

      free (line);
      line_start = newline + 1;
    }

    /* save the last line of the buffer for the next iteration */
    bytes_leftover = strlen (line_start);
    if (bytes_leftover) {
      leftover = (char *) calloc (bytes_leftover+1, sizeof(char));
      strcpy (leftover, line_start);
    }
  }

  if (leftover) {
    /* NOTE   Lines MUST end in newline.  To change this, add line-handling
     *        code here, using leftover.
     */
    free (leftover);
  }

  free (buffer);
  free (columns);
  fclose (file);
}



void
moth_source_destroy_textfile
(
  void *specifics
)
{
  moth_source_textfile_specifics specs;
  specs = (moth_source_textfile_specifics) specifics;
  if (!specs)
    return;
  if (specs->filename)
    free (specs->filename);
  if (specs->delimiter)
    free (specs->delimiter);
  free (specs);
}



