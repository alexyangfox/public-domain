/*
 * $Id: rrd.c,v 1.7 2003/06/14 18:10:38 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 *
 * This mostly just passes the attributes through as arguments to rrd_fetch()
 * without much interpretation or validity checking.
 */


#include <stdlib.h>
#include <rrd.h>

#include "../sources.h"



typedef struct moth_source_rrd_specifics_s  *moth_source_rrd_specifics;
struct moth_source_rrd_specifics_s {
  char    *filename;
  char    *cf;
  char    *resolution;
  char    *start;
  char    *end;
};



int
moth_source_create_rrd
(
  moth_source source,
  const char  **attributes
)
{
  moth_source_rrd_specifics specs;
  int a;

  specs = (moth_source_rrd_specifics)
    malloc (sizeof(struct moth_source_rrd_specifics_s));
  specs->filename = NULL;
  specs->cf = NULL;
  specs->resolution = NULL;
  specs->start = NULL;
  specs->end = NULL;

  for (a=0; attributes[a]; a+=2) {
    const char *attname = attributes[a];
    const char *attvalue = attributes[a+1];
    if (0==strcasecmp("columns",attname)) {
      /* ignore.  already taken care of */
    }
    else if (0==strcasecmp("filename",attname)) {
      specs->filename = moth_file_find (attvalue);
      if (!specs->filename)
        warn ("unknown filename \"%s\" in <rrd>", attname);
    }
    else if (0==strcasecmp("cf",attname)) {
      specs->cf = strdup (attvalue);
    }
    else if (0==strcasecmp("resolution",attname)) {
      specs->resolution = strdup (attvalue);
    }
    else if (0==strcasecmp("start",attname)) {
      specs->start = strdup (attvalue);
    }
    else if (0==strcasecmp("end",attname)) {
      specs->end = strdup (attvalue);
    }
    else {
      warn ("unknown <rrd> attribute %s=\"%s\"", attname, attvalue);
    }
  }

  /* required / default attributes */
  if (! specs->filename) {
    warn ("missing <rrd> attribute \"filename\"");
    return 0;
  }
  if (! specs->cf) {
    warn ("missing <rrd> attribute \"cf\"");
    return 0;
  }

  source->specifics = specs;
  return 1;
}



INTERNAL
void
moth_source_rrd_tokenize_buffer
(
  moth_buffer   buffer,
  char          ***list,
  size_t        *count
)
{
  size_t  size, i, t;
  char    *data, *c, *token_start;
  int     is_space, last_is_space;

  /* 
   * We manage our own copy of the buffer because we're actually going to
   * modify it.  We'll change the space after each token into a null character
   * so that a pointer to the first character of the token can be treated as a
   * C string.
   */
  data = moth_buffer_strdup (buffer);
  size = strlen (data);

  /* first pass:  count number of tokens */
  last_is_space = 1; /* triggers detection of first token */
  for (i=0; i<size; ++i) {
    c = data + i;
    is_space = isspace (*c);
    if (last_is_space && !is_space)
      ++(*count);
    last_is_space = is_space;
  }

  *list = (char **) calloc (*count, sizeof(char *));

  /* second pass:  assign tokens */
  t = 0;
  token_start = NULL;
  last_is_space = 1; /* triggers detection of first token */
  for (i=0; i<size; ++i) {
    c = data + i;
    is_space = isspace (*c);
    /* beginning of token */
    if (last_is_space && !is_space) {
      token_start = c;
    }
    /* end of token */
    else if (!last_is_space && is_space) {
      *c = '\0';
      (*list)[t] = strdup (token_start);
      token_start = NULL;
      ++t;
    }
    last_is_space = is_space;
  }
  /* handle last token */
  if (token_start) {
    (*list)[t] = strdup (token_start);
  }

  free (data);
}



void
moth_source_dump_specifics_rrd
(
  moth_source source
)
{
  moth_source_rrd_specifics specs;
  specs = (moth_source_rrd_specifics) source->specifics;
  if (specs->filename)
    fprintf (stdout, " filename=\"%s\"", specs->filename);
  if (specs->cf)
    fprintf (stdout, " cf=\"%s\"", specs->cf);
  if (specs->resolution)
    fprintf (stdout, " resolution=\"%s\"", specs->resolution);
  if (specs->start)
    fprintf (stdout, " start=\"%s\"", specs->start);
  if (specs->end)
    fprintf (stdout, " end=\"%s\"", specs->end);
}



void
moth_source_process_rrd
(
  moth_source source
)
{
  moth_source_rrd_specifics specs;
  moth_column   *column_list;
  size_t        column_count;
  char          **source_list;
  int           source_count;

  int           rv;           /* return value.  zero means success */
  int           argc;         /* number of arguments */
  char          **argv;       /* array of arguments */
  time_t        start, end;   /* timeframe used */
  unsigned long step;         /* stepsize */
  unsigned long ds_count;     /* number of data sources in file */
  char          **ds_list;    /* names of data sources */
  rrd_value_t   *rrd_data;    /* two dimensional array containing the data */
  int           *ds_map;      /* int:int map of ds_list index to column_list
                               * index */
  int           old_optind, old_opterr;   /* save and restore globals */

  moth_column   column;
  int           a, b;
  unsigned long timestamp;

  specs = (moth_source_rrd_specifics) source->specifics;

  column_list = NULL;
  column_count = 0;
  source_list = NULL;
  source_count = 0;

  argc = 0;
  argv = NULL;
  start = end = step = ds_count = 0;
  ds_list = NULL;
  rrd_data = NULL;

  /* create list of columns, for easier lookup */
  column_count = moth_column_list_size (source->columns);
  column_list = (moth_column *) calloc (column_count, sizeof(moth_column));
  a = 0;
  moth_column_list_reset (source->columns);
  while (( column = moth_column_list_next(source->columns) )) {
    column_list[a] = column;
    ++a;
  }

  moth_source_rrd_tokenize_buffer (source->buffer, &source_list,
      &source_count);
  if (source_count < (column_count-1))
    die ("not enough RRD data sources specified in <rrd>");
  if (source_count > (column_count-1))
    die ("too many RRD data sources specified in <rrd>");

  /* put attributes into argv array */
  ++argc;           /* first argument has to be the string "fetch" */
  if (specs->filename)    ++argc;
  if (specs->cf)          ++argc;
  if (specs->resolution)  { argc += 2; }
  if (specs->start)       { argc += 2; }
  if (specs->end)         { argc += 2; }
  argv = (char **) calloc (argc, sizeof(char *));
  a = 0;
  argv[a++] = "fetch";
  if (specs->filename) {
    argv[a++] = specs->filename;
  }
  if (specs->cf) {
    argv[a++] = specs->cf;
  }
  if (specs->resolution) {
    argv[a++] = "-r";
    argv[a++] = specs->resolution;
  }
  if (specs->start) {
    argv[a++] = "-s";
    argv[a++] = specs->start;
  }
  if (specs->end) {
    argv[a++] = "-e";
    argv[a++] = specs->end;
  }

  /*
   * Both moth_runtime_read_args() [in moth.c] and rrd_fetch() use the getopt
   * library.  This library uses these globals, so we need to take care to
   * preserve them.
   */
  old_optind = optind;
  old_opterr = opterr;
  optind = opterr = 0;

  /* get data */
  rv = rrd_fetch (argc, argv, &start, &end, &step, &ds_count, &ds_list,
      &rrd_data);

  optind = old_optind;
  opterr = old_opterr;

  if (-1 == rv)
    die ("couldn't read file in <rrd>:  %s", rrd_get_error());
  if (ds_count < source_count)
    die ("not enough data sources in RRD file to fulfill request in <rpn>");

  /* create map of rrd column index to column list index */
  ds_map = (int *) calloc (ds_count, sizeof(int));
  for (a=0; a<ds_count; ++a) {
    ds_map[a] = 0;    /* default:  zero means "don't use this column" */
    for (b=0; b<source_count; ++b) {
      if (0==strcmp(ds_list[a],source_list[b])) {
        /*
         * We add 1 here because the first source column is where we'll place
         * the timestamp for each row of the rrd.
         */
        ds_map[a] = b + 1;
      }
    }
  }

  /* pull data out for desired columns */
  a = 0;
  for (timestamp=start; timestamp<=end; timestamp+=step) {
    moth_column_add (column_list[0], (double) timestamp);
    for (b=0; b<ds_count; ++b) {
      moth_column source_column;    /* column we are FILLING */
      if (0 == ds_map[b])
        continue;                   /* don't use this column */
      source_column = column_list[ds_map[b]];
      moth_column_add (source_column, rrd_data[(a*ds_count)+b]);
    }
    ++a;
  }

  /* rrd_fetch() allocates these, but we need to clean them up.  */
  if (ds_list) {
    for (a=0; a<ds_count; ++a)
      free (ds_list[a]);
    free (ds_list);
  }
  if (rrd_data)
    free (rrd_data);

  if (ds_map)
    free (ds_map);
  if (argv)
    free (argv);
  if (source_list) {
    for (a=0; a<source_count; ++a)
      free (source_list[a]);
    free (source_list);
  }
  if (column_list)
    free (column_list);
}



void
moth_source_destroy_rrd
(
  void *specifics
)
{
  moth_source_rrd_specifics specs;
  specs = (moth_source_rrd_specifics) specifics;
  if (!specs)
    return;
  if (specs->filename)
    free (specs->filename);
  if (specs->cf)
    free (specs->cf);
  if (specs->resolution)
    free (specs->resolution);
  if (specs->start)
    free (specs->start);
  if (specs->end)
    free (specs->end);
  free (specs);
}



