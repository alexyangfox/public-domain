/*
 * $Id: sources.c,v 1.13 2003/06/25 18:09:34 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See sources.dev for notes and coding issues.
 *
 */



#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>

#include "data.h"
#include "columns.h"
#include "options.h"
#include "sources.h"

#include "sources/textfile.c"
#include "sources/rpn.c"

#if defined(HAVE_MYSQL_MYSQL_H) && defined(HAVE_LIBMYSQLCLIENT)
#define MOTH_HAVE_MYSQL
#include "sources/mysql.c"
#endif
#if defined(HAVE_LIBRRD) && defined(HAVE_RRD_H)
#define MOTH_HAVE_RRDTOOL
#include "sources/rrd.c"
#endif



moth_source_type
moth_source_find_type
(
  const char *name
)
{
  if (0==strcasecmp("textfile",name)) {
    return MOTH_TEXTFILE;
  }
  else if (0==strcasecmp("rpn",name)) {
    return MOTH_RPN;
  }
  else if (0==strcasecmp("mysql",name)) {
#ifdef MOTH_HAVE_MYSQL
    return MOTH_MYSQL;
#else
    warn ("moth not compiled with <mysql> support");
    return MOTH_SOURCE_UNKNOWN;
#endif
  }
  else if (0==strcasecmp("rrd",name)) {
#ifdef MOTH_HAVE_RRDTOOL
    return MOTH_RRD;
#else
    warn ("moth not compiled with <rrd> support");
    return MOTH_SOURCE_UNKNOWN;
#endif
  }
  return MOTH_SOURCE_UNKNOWN;
}



const char *
moth_source_find_name
(
  moth_source_type type
)
{
  switch (type) {
    case MOTH_TEXTFILE:
      return "textfile";
    case MOTH_RPN:
      return "rpn";
#ifdef MOTH_HAVE_MYSQL
    case MOTH_MYSQL:
      return "mysql";
#endif
#ifdef MOTH_HAVE_RRDTOOL
    case MOTH_RRD:
      return "rrd";
#endif
    default:
      return NULL;
  }
}



moth_source
moth_source_create
(
  const char        *name,
  moth_column_list  columns,
  const char        **attributes
)
{
  moth_source source;
  int         success;

  source = (moth_source) malloc (sizeof(struct moth_source_s));
  source->type = moth_source_find_type (name);
  source->columns = columns;
  source->buffer = moth_buffer_create ();
  source->specifics = NULL;

  switch (source->type) {
    case MOTH_TEXTFILE:
      success = moth_source_create_textfile (source, attributes);
      break;
    case MOTH_RPN:
      success = moth_source_create_rpn (source, attributes);
      break;
#ifdef MOTH_HAVE_MYSQL
    case MOTH_MYSQL:
      success = moth_source_create_mysql (source, attributes);
      break;
#endif
#ifdef MOTH_HAVE_RRDTOOL
    case MOTH_RRD:
      success = moth_source_create_rrd (source, attributes);
      break;
#endif
    default:
      success = 0;
      break;
  }

  if (success)
    return source;

  moth_column_list_destroy (source->columns);
  moth_buffer_destroy (source->buffer);
  free (source);
  return NULL;
}



void
moth_source_add_chunk
(
  moth_source source,
  const char  *chunk,
  size_t      chunkLen
)
{
  moth_buffer_add (source->buffer, chunk, chunkLen);
}



void
moth_source_dump
(
  moth_source source
)
{
  char *buffer;

  fprintf (stdout, "  <%s", moth_source_find_name(source->type));

  /* dump columns */
  fprintf (stdout, " columns=\"");
  moth_column_list_dump (source->columns);
  fprintf (stdout, "\"");

  /* dump special attributes */
  switch (source->type) {
    case MOTH_TEXTFILE:
      moth_source_dump_specifics_textfile (source);
      break;
    case MOTH_RPN:
      moth_source_dump_specifics_rpn (source);
      break;
#ifdef MOTH_HAVE_MYSQL
    case MOTH_MYSQL:
      moth_source_dump_specifics_mysql (source);
      break;
#endif
#ifdef MOTH_HAVE_RRDTOOL
    case MOTH_RRD:
      moth_source_dump_specifics_rrd (source);
      break;
#endif
    default:
      die ("EXCEPTION:  can't process unknown source type");
      break;
  }

  fprintf (stdout, ">");

  buffer = NULL;
  buffer = moth_buffer_strdup (source->buffer);
  if (buffer) {
    fprintf (stdout, "%s", buffer);
    free (buffer);
  }
  fprintf (stdout, "</%s>\n", moth_source_find_name(source->type));
}



void
moth_source_process
(
  moth_source source
)
{
  clock_t     start, stop;
  struct tms  start_s, stop_s;
  int         profile_sources;
  if (!source) return;

  start = stop = 0;
  profile_sources = moth_options_get_option (runtime->options, "profile");
  if (profile_sources) {
    start = times (&start_s);
  }

  switch (source->type) {
    case MOTH_TEXTFILE:
      moth_source_process_textfile (source);
      break;
    case MOTH_RPN:
      moth_source_process_rpn (source);
      break;
#ifdef MOTH_HAVE_MYSQL
    case MOTH_MYSQL:
      moth_source_process_mysql (source);
      break;
#endif
#ifdef MOTH_HAVE_RRDTOOL
    case MOTH_RRD:
      moth_source_process_rrd (source);
      break;
#endif
    default:
      die ("EXCEPTION:  can't process unknown source type");
      break;
  }

  if ((start != -1) && profile_sources) {
    double ticks_per_second;
    stop = times (&stop_s);
    ticks_per_second = (double) sysconf (_SC_CLK_TCK);
    printf ("PROFILE\t%s\t%lf\t%lf\t%lf\n",
        moth_source_find_name(source->type),
        (stop-start)/ticks_per_second,
        (stop_s.tms_utime-start_s.tms_utime)/ticks_per_second,
        (stop_s.tms_stime-start_s.tms_stime)/ticks_per_second);
  }

  if (moth_options_get_option(runtime->options,"dump-columns")) {
    moth_column column;
    moth_column_list_reset (source->columns);
    while (( column = moth_column_list_next(source->columns) )) {
      size_t rows, r;
      rows = moth_column_size (column);
      printf ("COLUMN\t%s\t%s\t%d\n", moth_column_name(column), 
          moth_source_find_name(source->type), rows);
      for (r=0; r<rows; ++r) {

        printf ("\t%g\n", moth_column_value(column, r));
      } /* rows */
    } /* columns */
  }

}



void
moth_source_destroy
(
  moth_source source
)
{
  if (!source) return;

  switch (source->type) {
    case MOTH_TEXTFILE:
      moth_source_destroy_textfile (source->specifics);
      break;
    case MOTH_RPN:
      moth_source_destroy_rpn (source->specifics);
      break;
#ifdef MOTH_HAVE_MYSQL
    case MOTH_MYSQL:
      moth_source_destroy_mysql (source->specifics);
      break;
#endif
#ifdef MOTH_HAVE_RRDTOOL
    case MOTH_RRD:
      moth_source_destroy_rrd (source->specifics);
      break;
#endif
    default:
      /* no compiler warnings here :) */
      break;
  }

  moth_column_list_destroy (source->columns);
  moth_buffer_destroy (source->buffer);
  free (source);
}



