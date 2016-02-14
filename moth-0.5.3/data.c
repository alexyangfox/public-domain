/*
 * $Id: data.c,v 1.29 2004/03/17 04:14:06 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See data.dev for implementation notes.
 *
 */



#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "data.h"
#include "file.h"
#include "options.h"
#include "parser.h"



/*--------------------------------------------------------*\
|                      data structures                     |
\*--------------------------------------------------------*/

typedef struct moth_buffer_chunk_s *moth_buffer_chunk;
struct moth_buffer_chunk_s {
  size_t              size;
  void                *data;  /* NOT null terminated */
  moth_buffer_chunk   next;
};
struct moth_buffer_s {
  size_t              size;
  moth_buffer_chunk   chunks; /* linked list */
  void                *data;  /* pointer returned by _data()
                               * NOT null terminated */
  size_t              datasize;
};



/*--------------------------------------------------------*\
|                    utility functions                     |
\*--------------------------------------------------------*/

void
die
(
  const char *format,
  ...
)
{
  va_list ap;
  fprintf (stderr, "ERROR");

  if (runtime && runtime->file && runtime->file->parser) {
    int line, column;
    line = 0;
    column = 0;
    moth_parser_streamstats (runtime->file->parser, &line, &column);
    fprintf (stderr, " at line %d column %d", line, column);
  }

  if (runtime && runtime->file && runtime->file->filename) {
    fprintf (stderr, " in %s", runtime->file->filename);
  }

  va_start (ap, format);
  fprintf (stderr, ":  ");
  vfprintf (stderr, format, ap);
  fprintf (stderr, "\n");
  va_end (ap);
  exit (1);
}



void
warn
(
  const char *format,
  ...
)
{
  va_list ap;

  if (runtime && runtime->options
      && moth_options_get_option(runtime->options,"quiet"))
  {
    return;
  }

  fprintf (stderr, "WARNING");

  if (runtime && runtime->file && runtime->file->parser) {
    int line, column;
    line = 0;
    column = 0;
    moth_parser_streamstats (runtime->file->parser, &line, &column);
    fprintf (stderr, " at line %d column %d", line, column);
  }

  if (runtime && runtime->file && runtime->file->filename) {
    fprintf (stderr, " in %s", runtime->file->filename);
  }

  va_start (ap, format);
  fprintf (stderr, ":  ");
  vfprintf (stderr, format, ap);
  fprintf (stderr, "\n");
  va_end (ap);
}



/*--------------------------------------------------------*\
|                      math functions                      |
\*--------------------------------------------------------*/

double
scale_value
(
  moth_axis_scaling scaling,
  double            value
)
{
  switch (scaling) {
    case MOTH_LOG2:
      value = log(value) / log(2.0);
      break;
    case MOTH_LOGE:
      value = log (value);
      break;
    case MOTH_LOG10:
      value = log10 (value);
      break;
    case MOTH_LINEAR:
    default:
      /* no conversion necessary */
      break;
  }
  return value;
}



double
unscale_value
(
  moth_axis_scaling scaling,
  double            value
)
{
  switch (scaling) {
    case MOTH_LOG2:
      value = pow (2.0, value);
      break;
    case MOTH_LOGE:
      value = pow (M_E, value);
      break;
    case MOTH_LOG10:
      value = pow (10.0, value);
      break;
    case MOTH_LINEAR:
    default:
      /* no conversion necessary */
      break;
  }
  return value;
}



double
convert_value
(
  double  value,
  char    modifier,
  char    unit
)
{
  if ('b' == modifier) {
    switch (unit) {
      case 'Y': value *= pow(1024.0,8.0); break;
      case 'Z': value *= pow(1024.0,7.0); break;
      case 'E': value *= pow(1024.0,6.0); break;
      case 'P': value *= pow(1024.0,5.0); break;
      case 'T': value *= pow(1024.0,4.0); break;
      case 'G': value *= pow(1024.0,3.0); break;
      case 'M': value *= pow(1024.0,2.0); break;
      case 'k': value *= 1024; break;
      default: return NAN;
    }
  }
  else if ('d' == modifier) {
    switch (unit) {
      case 'Y': value *= 31556926; break;
      case 'M':
      case 'O': value *= 2629743.8; break;
      case 'W': value *= 604800; break;
      case 'D': value *= 86400; break;
      case 'H': value *= 3600; break;
      case 'I': value *= 60; break;
      case 'S': /* nothing to do */ break;
      case 'N': value = time(NULL); break;
      default: return NAN;
    }
  }
  else {
    switch (unit) {
      case 'Y': value *= 1e24; break;
      case 'Z': value *= 1e21; break;
      case 'E': value *= 1e18; break;
      case 'P': value *= 1e15; break;
      case 'T': value *= 1e12; break;
      case 'G': value *= 1e9; break;
      case 'M': value *= 1e6; break;
      case 'k': value *= 1e3; break;
      case 'h': value *= 1e2; break;
      case 'D': value *= 1e1; break;
      case '#': /* nothing to do */ break;
      case 'd': value /= 1e1; break;
      case 'c': value /= 1e2; break;
      case 'm': value /= 1e3; break;
      case 'u': value /= 1e6; break;
      case 'n': value /= 1e9; break;
      case 'p': value /= 1e12; break;
      case 'f': value /= 1e15; break;
      case 'a': value /= 1e18; break;
      case 'z': value /= 1e21; break;
      case 'y': value /= 1e24; break;
      default: return value;
    }
  }
  return value;
}



/* Parses a string looking for modifier and unit specifiers. 
 * Returns success as boolean.  */
int
parse_value
(
  const char  *string,
  double      *value,
  char        *modifier,
  char        *unit
)
{
  char  *v, *first, *second;
  int   matches, consumed, found;

  if ( !value ) return 0;

  found = 0;
  v = (char *) malloc (20*sizeof(char));
  v[19] = '\0';
  first = (char *) malloc (2*sizeof(char));
  first[0] = '\0';
  first[1] = '\0';
  second = (char *) malloc (2*sizeof(char));
  second[0] = '\0';
  second[1] = '\0';

  /* 
   * Because the "%lf" sscanf template will eat our decimal unit 'E' (exa,
   * 1e18), we need to destinguish between the two ourselves.  We do this by
   * explicitely testing for the exponent format ([-]d.ddde±dd).
   *
   * This is related to bug #670924.
   */
  consumed = 0;
  sscanf (string, "%*d.%*d%*[eE]%*[-+]%*2d%n", &consumed);
  if (consumed >= 7) {
    sscanf (string, "%lf", value);
    if ( modifier ) *modifier = '\0';
    if ( unit )     *unit     = '\0';
    free (v);
    free (first);
    free (second);
    return 1;
  }

  matches = sscanf (string, "%19[0-9.-]%1[a-zA-Z#]%1[a-zA-Z#]", v, first,
      second);
  if (matches >= 1 && 1 == sscanf(v,"%lf",value)) {
    found = 1;
  }
  if (found && 2 == matches) {
    if ( unit ) *unit = first[0];
    found = 1;
  }
  if (found && 3 == matches) {
    if ( modifier ) *modifier = first[0];
    if ( unit )     *unit     = second[0];
    found = 1;
  }

  free (v);
  free (first);
  free (second);
  return found;
}



double
read_value
(
  const char  *string
)
{
  double        value;
  unsigned int  year, month, day, hours, minutes, seconds;
  int           matches;
  char          modifier, unit;
  value = NAN;

  /* datetime format */
  matches = sscanf (string, "%04u-%02u-%02u %02u:%02u:%02u",
      &year, &month, &day, &hours, &minutes, &seconds);
  if (matches >= 3) {
    struct tm time;
    time.tm_isdst = -1;  /* don't know */
    time.tm_year = year - 1900;
    time.tm_mon = month - 1;
    time.tm_mday = day;
    time.tm_hour = (matches>=4) ? hours : 0;
    time.tm_min = (matches>=5) ? minutes : 0;
    time.tm_sec = (matches>=6) ? seconds : 0;
    value = mktime (&time);
    if (value < 0)
      return NAN;
    return value;
  }

  modifier = unit = ' ';
  if ( parse_value(string, &value, &modifier, &unit) ) {
    return convert_value(value, modifier, unit);
  }
  return NAN;
}



int
double_compare
(
  const void *a,
  const void *b
)
{
  if (*((double *) a) < *((double *) b)) return -1;
  if (*((double *) a) > *((double *) b)) return 1;
  return 0;
}



/*--------------------------------------------------------*\
|                    datetime functions                    |
\*--------------------------------------------------------*/

double
datetime_modulus
(
  double    value,
  char      unit,
  double    count
)
{
    time_t      time;
    struct tm   *time_s;
    time    = value;
    time_s  = localtime (&time);

    switch (unit) {
      case 'Y':
        time_s->tm_sec  = 0;
        time_s->tm_min  = 0;
        time_s->tm_hour = 0;
        time_s->tm_mday = 1;
        time_s->tm_mon  = 0;
        time_s->tm_year -= fmod(time_s->tm_year, count);
        break;
      case 'M':
      case 'O':
        time_s->tm_sec  = 0;
        time_s->tm_min  = 0;
        time_s->tm_hour = 0;
        time_s->tm_mday = 1;
        time_s->tm_mon  -= fmod(time_s->tm_mon, count);
        break;
      case 'W':
        time_s->tm_sec  = 0;
        time_s->tm_min  = 0;
        time_s->tm_hour = 0;

        /* adjust to week boundary (Sunday) */
        time_s->tm_mday -= time_s->tm_wday;
        time_s->tm_yday -= time_s->tm_wday;

        /* step back to our modulus boundary, based on the week
         * number (calculated as the week in the year) */
        time_s->tm_mday -= 7.0 * fmod(
                (time_s->tm_yday / 7),  /* week number in year */
                count);
        break;
      case 'D':
        time_s->tm_sec  = 0;
        time_s->tm_min  = 0;
        time_s->tm_hour = 0;
        time_s->tm_mday -= fmod(time_s->tm_mday, count);
        break;
      case 'H':
        time_s->tm_sec  = 0;
        time_s->tm_min  = 0;
        time_s->tm_hour -= fmod(time_s->tm_hour, count);
        break;
      case 'I':
        time_s->tm_sec  = 0;
        time_s->tm_min  -= fmod(time_s->tm_min, count);
        break;
      case 'S':
        time_s->tm_sec  -= fmod(time_s->tm_sec, count);
        break;
      case 'N':
      default:
        return NAN;
    }

    time = mktime (time_s);
    return time;
}



double
datetime_add
(
  double    value,
  char      unit,
  double    count
)
{
    time_t      time;
    struct tm   *time_s;
    time = value;
    time_s = localtime (&time);

    switch (unit) {
      case 'Y':
        time_s->tm_year += count;
        break;
      case 'M':
      case 'O':
        time_s->tm_mon  += count;
        break;
      case 'W':
        time_s->tm_mday += 7.0 * count;
        break;
      case 'D':
        time_s->tm_mday += count;
        break;
      case 'H':
        time_s->tm_hour += count;
        break;
      case 'I':
        time_s->tm_min  += count;
        break;
      case 'S':
        time_s->tm_sec  += count;
        break;
      case 'N':
      default:
        return NAN;
    }

    time = mktime (time_s);
    return time;
}



/*--------------------------------------------------------*\
|                          buffer                          |
\*--------------------------------------------------------*/

moth_buffer
moth_buffer_create (void)
{
  moth_buffer buffer;
  buffer = (moth_buffer) malloc (sizeof(struct moth_buffer_s));
  buffer->size = 0;
  buffer->chunks = NULL;
  buffer->datasize = 0;
  buffer->data = NULL;
  return buffer;
}



void
moth_buffer_add
(
  moth_buffer buffer,
  const void  *chunk,
  size_t      chunksize
)
{
  moth_buffer_chunk c, last, new;

  c = last = buffer->chunks;
  while (c) {
    last = c;
    c = c->next;
  }

  /* make new chunk */
  new = (moth_buffer_chunk) malloc (sizeof(struct moth_buffer_chunk_s));
  new->size = chunksize;
  new->data = (void *) malloc (new->size);
  new->next = NULL;
  memcpy (new->data, chunk, new->size);

  if (last)
    last->next = new;
  else
    buffer->chunks = new;
  buffer->size += new->size;
}



size_t
moth_buffer_size
(
  moth_buffer buffer
)
{
  return buffer->size;
}



const void *
moth_buffer_data
(
  moth_buffer buffer
)
{
  void *old, *p;
  moth_buffer_chunk c, doomed;

  if (!buffer) return NULL;
  if (!buffer->chunks)
    return buffer->data;

  old = buffer->data;
  buffer->data = (void *) malloc (buffer->size);
  if (old) {
    memcpy (buffer->data, old, buffer->datasize);
  }

  p = buffer->data + buffer->datasize;
  c = buffer->chunks;
  while (c) {
    doomed = c;
    memcpy (p, c->data, c->size);
    p += c->size;
    c = c->next;
    if (doomed->data)
      free (doomed->data);
    free (doomed);
  }
  buffer->datasize = buffer->size;
  buffer->chunks = NULL;

  if (old)
    free (old);
  return buffer->data;
}



char *
moth_buffer_strdup
(
  moth_buffer buffer
)
{
  size_t      size;
  const void  *data;
  char        *string;
  size = moth_buffer_size (buffer);
  if (!size)
    return NULL;
  data = moth_buffer_data (buffer);
  if (!data)
    return NULL;
  string = (char *) malloc (size+1);
  if (!string)
    return NULL;
  memcpy (string, data, size);
  string[size] = '\0';
  return string;
}



void
moth_buffer_destroy
(
  moth_buffer buffer
)
{
  moth_buffer_chunk c, doomed;
  c = buffer->chunks;
  while (c) {
    doomed = c;
    c = c->next;
    free (doomed->data);
    free (doomed);
  }
  if (buffer->data)
    free (buffer->data);
  free (buffer);
}



