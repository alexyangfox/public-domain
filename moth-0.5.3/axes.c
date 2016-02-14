/*
 * $Id: axes.c,v 1.29 2004/03/17 04:10:31 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See axes.dev for notes and coding issues.
 *
 */


#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "axes.h"
#include "columns.h"
#include "data.h"
#include "file.h"
#include "image.h"
#include "parser.h"



INTERNAL void moth_label_dump (moth_label label);



/*--------------------------------------------------------*\
|                      tick functions                      |
\*--------------------------------------------------------*/

typedef enum {
  MOTH_LABEL_TOKEN_UNKNOWN = 0,
  MOTH_LABEL_TOKEN_CONSTANT,
  MOTH_LABEL_TOKEN_REPLACEABLE
} moth_label_token_type;

typedef struct moth_label_token_s *moth_label_token;
struct moth_label_token_s {
  const char            *text;
  size_t                length; /* number of characters */
  moth_label_token_type type;
  moth_label_token      next;   /* for linked list */
};

typedef struct moth_label_tokens_s *moth_label_tokens;
struct moth_label_tokens_s {
  char                  *data;  /* data of tokens */
  moth_label_token      tokens; /* linked list */
  size_t                count;  /* number of tokens */
};



INTERNAL
moth_label_tokens
moth_label_tokens_create
(
  moth_buffer data
)
{
  moth_label_tokens       tokens;
  moth_label_token        token, last_token;
  size_t                  characters, i;
  const char              *c;
  char                    last_c;

  if (!data || 0==moth_buffer_size(data))
    return NULL;

  tokens = (moth_label_tokens)
    malloc (sizeof(struct moth_label_tokens_s));
  tokens->data = NULL;
  tokens->tokens = NULL;
  tokens->count = 0;

  tokens->data = moth_buffer_strdup (data);
  characters = strlen (tokens->data);

  token = last_token = NULL;
  c = NULL;
  last_c = '\0';
  for (i=0; i<characters; ++i) {
    if (c)
      last_c = *c;
    c = tokens->data + i;

    /* 
     * For some cases, the current character will not belong to the last token.
     * In those instances, we complete the token and start a new one.
     */
    if (token) {
      int complete_token;
      complete_token = 0;

      /* 
       * Complete the last token on a constant-to-replaceable boundary.
       */
      if (MOTH_LABEL_TOKEN_CONSTANT == token->type && '%' == *c) {
        complete_token = 1;
      }

      else if (MOTH_LABEL_TOKEN_REPLACEABLE == token->type) {

        /* 
         * Complete last token as constant when an unknown binary unit comes
         * after a 'b' character.
         */
        if ('b' == last_c) {
          /* Assume the worst for now.  */
          token->type = MOTH_LABEL_TOKEN_CONSTANT;
          complete_token = 1;
          switch (*c) {
            case 'Y':
            case 'Z':
            case 'E':
            case 'P':
            case 'T':
            case 'G':
            case 'M':
            case 'k':
              /* Everything is OK.  Token will get completed below.  */
              token->type = MOTH_LABEL_TOKEN_REPLACEABLE;
              complete_token = 0;
              break;
          }
        }

        /*
         * Complete last token as 'decimus' when an unknown datetime unit comes
         * after a 'd' character.
         */
        else if ('d' == last_c) {
          /* Assume the worst for now.  */
          complete_token = 1;
          switch (*c) {
            case 'Y':
            case 'M':
            case 'O':
            case 'W':
            case 'D':
            case 'H':
            case 'I':
            case 'S':
            case 'N':
              /* Everything is OK.  Token will get completed below.  */
              complete_token = 0;
              break;
          }
        }

      } /* replaceable token */

      if (complete_token) {
        if (last_token)
          last_token->next = token;
        else
          tokens->tokens = token;
        last_token = token;
        ++(tokens->count);
        token = NULL;
      }
      
    } /* have an existing token */

    /*
     * The current character always belongs to a token.  If we've completed the
     * token from the last time through the loop, then we need to make a new
     * one.
     */
    if (!token) {
      token = (moth_label_token)
        malloc (sizeof(struct moth_label_token_s));
      token->text = c;
      token->length = 1;        /* the current character */
      token->type = ('%' == *c)
        ? MOTH_LABEL_TOKEN_REPLACEABLE : MOTH_LABEL_TOKEN_CONSTANT;
      token->next = NULL;
      continue;
    }

    /* We should never get here on the first character of a token.  */

    token->length += 1;   /* add the current character */

    if (MOTH_LABEL_TOKEN_REPLACEABLE == token->type) {
      int complete_token;
      complete_token = 0;

      if ('b' == last_c) {
        /*
         * The current character is always a valid binary unit.  We've made
         * sure of that above.
         */
        complete_token = 1;
      }
      else if ('d' == last_c) {
        /*
         * The current character is always a valid datetime unit.  We've made
         * sure of that above.
         */
        complete_token = 1;
      }
      else {
        switch (*c) {
          /* width and precision */
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
          case '.':
            continue;

          /* modifiers */
          case 'b':
          case 'd':
            continue;

          /* decimal units */
          case 'Y':
          case 'Z':
          case 'E':
          case 'P':
          case 'T':
          case 'G':
          case 'M':
          case 'k':
          case 'h':
          case 'D':
          case '#':
/*        case 'd':  latin 'decimus'.  This case is handled specially above. */
          case 'c':
          case 'm':
          case 'u':
          case 'n':
          case 'p':
          case 'f':
          case 'a':
          case 'z':
          case 'y':
            complete_token = 1;
            break;
          case '%':
            token->type = MOTH_LABEL_TOKEN_CONSTANT;
            /* a %% is replaced by a single %  */
            if ('%' == last_c) {
              token->length -= 1;
            }
            complete_token = 1;
            break;
        }
      }

      if (complete_token) {
        if (last_token)
          last_token->next = token;
        else
          tokens->tokens = token;
        last_token = token;
        ++(tokens->count);
        token = NULL;
        continue;
      }
    }

    if (MOTH_LABEL_TOKEN_CONSTANT == token->type) {
      /* Just keep going, nothing special to do.  */
    }

  } /* every character in template */

  /* Add ultimate token, if not already.  */
  if (token) {
    if (last_token)
      last_token->next = token;
    else
      tokens->tokens = token;
    ++(tokens->count);
  }

  return tokens;
}



INTERNAL
void
moth_label_tokens_destroy
(
  moth_label_tokens tokens
)
{
  if (!tokens)
    return;
  if (tokens->data)
    free (tokens->data);
  if (tokens->tokens) {
    moth_label_token t, doomed;
    t = tokens->tokens;
    while (t) {
      doomed = t;
      t = t->next;
      /* DO NOT free data in token.  It doesn't own it.  */
      free (doomed);
    }
  }
  free (tokens);
}



/*
 * This takes a replacement string inside a template, something that starts
 * with '%', and finds the appropriate value for it.  It puts this replacement
 * into the buffer.
 */
INTERNAL
void
moth_label_tick_format_token
(
  moth_buffer       buffer,
  moth_label_token  token,
  moth_axis_scaling scaling,
  double            value       /* scaling already taken into account */
)
{
  char          *tstring;
  int           width, precision;
  char          modifier, unit;
  const char    *prefix;

  char          *value_string;
  int           string_size;

  if (!buffer || !token || !token->length || !token->text)
    return;

  /* 
   * uggg... it would be nice to avoid this, but until we've got an snscanf(),
   * there's not much we can do.
   */
  tstring = (char *) malloc ((token->length+1) * sizeof(char));
  memcpy (tstring, token->text, token->length);
  tstring[token->length] = '\0';

  /* default is single number */
  width = 1;
  precision = 0;
  modifier = unit = '\0';

  if (4==sscanf(tstring,"%%%d.%d%c%c", &width, &precision, &modifier, &unit))
    ;
  else if (3==sscanf(tstring,"%%%d.%d%c", &width, &precision, &unit))
    ;
  else if (3==sscanf(tstring,"%%%d%c%c", &width, &modifier, &unit))
    ;
  else if (2==sscanf(tstring,"%%%d%c", &width, &unit))
    ;
  else if (2==sscanf(tstring,"%%%c%c", &modifier, &unit))
    ;
  else if (1==sscanf(tstring,"%%%c", &unit))
    ;
  else
    die ("unknown template replacement \"%s\" in <label>", tstring);

  prefix = "";
  switch (scaling) {
    case MOTH_LINEAR:
      prefix = "";
      break;
    case MOTH_LOG2:
      prefix = "2^";
      break;
    case MOTH_LOGE:
      prefix = "e^";
      break;
    case MOTH_LOG10:
      prefix = "10e";
      break;
    default:
      die ("unknown scaling in <label>");
      break;
  }

  /* 
   * We handle datetime template replacement here, since it's not handled by
   * convert_value().  Also, we provide some options that aren't available
   * when reading constants (and that don't make sense in that context).
   */
  if ('d' == modifier) {
#ifdef HAVE_LOCALTIME
    time_t      t;
    struct tm   *time_s;
    const char  *name;

    t = value;
    time_s = localtime (&t);

    if (!time_s)
      die ("couldn't find date info for \"%g\" in <label>", value);

    switch (unit) {
      case 'Y': value = time_s->tm_year + 1900; break;
      case 'M': value = time_s->tm_mon + 1; break;
      /* abrev month names */
      case 'O':
        name = NULL;
        switch (time_s->tm_mon) {
          case 0: name = "Jan"; break;
          case 1: name = "Feb"; break;
          case 2: name = "Mar"; break;
          case 3: name = "Apr"; break;
          case 4: name = "May"; break;
          case 5: name = "Jun"; break;
          case 6: name = "Jul"; break;
          case 7: name = "Aug"; break;
          case 8: name = "Sep"; break;
          case 9: name = "Oct"; break;
          case 10: name = "Nov"; break;
          case 11: name = "Dec"; break;
        }
        if (name) {
          moth_buffer_add (buffer, name, 3);
          free (tstring);
          return;
        }
        else
          die ("couldn't find month for \"%g\" in <label>", value);
        break;
      /* abbrev week names */
      case 'W':
        name = NULL;
        switch (time_s->tm_wday) {
          case 0: name = "Sun"; break;
          case 1: name = "Mon"; break;
          case 2: name = "Tue"; break;
          case 3: name = "Wed"; break;
          case 4: name = "Thu"; break;
          case 5: name = "Fri"; break;
          case 6: name = "Sat"; break;
        }
        if (name) {
          moth_buffer_add (buffer, name, 3);
          free (tstring);
          return;
        }
        else
          die ("couldn't find weekday for \"%g\" in <label>", value);
        break;
      case 'D': value = time_s->tm_mday; break;
      case 'H': value = time_s->tm_hour; break;
      case 'I': value = time_s->tm_min; break;
      case 'S': value = time_s->tm_sec; break;
      default:
        warn ("unknown unit \"%c\" in <label>", unit);
        break;
    }
#else
    warn ("couldn't find datetime value for \"%s\" because localtime() is"
        " not available", tstring);
#endif
  } /* datetime modifier */

  /* all other template replacements */
  else {
    value /= convert_value (1.0, modifier, unit);
  }
  free (tstring);

  if (isnan(value))
    return;

  /* 
   * OK, this isn't pretty.  'man sprintf' has some ideas on how to print when
   * you don't know the size of the resulting string.  The example given there
   * is complicated, in part to work around a bug in earlier version of glibc.
   * Also, I couldn't get it to work, so I do this instead.
   */
  string_size = width + 2; /* . and terminator */
  value_string = (char *) malloc (string_size*sizeof(char));
  if (!value_string) {
    warn ("couldn't find memory for <label>");
    return;
  }
  while (1) {
    int value_size;
    value_string[0] = '\0';
    value_size = snprintf (value_string, string_size, "%s%*.*lf",
        prefix, width, precision, value);
    /* value was successfully written and fits in allocated space */
    if (value_size != -1 && (value_size+1) <= string_size)
      break;

    /* the string requires more space */
    string_size += 1;
    value_string = (char *) realloc (value_string, string_size*sizeof(char));
    if (!value_string) {
      warn ("couldn't find memory for <label>");
      return;
    }
  }

  moth_buffer_add (buffer, value_string, strlen(value_string));
  free (value_string);
}



/*
 * This transmogrifies the value according to the template.  Since the
 * template can have many places and formats into which the same value should
 * replaced, we break down the template into tokens, and replace them with the
 * value when and how we need.
 */
INTERNAL
char *
moth_label_tick_format_value
(
  moth_label_tokens tokens,
  moth_axis_scaling scaling,
  double            value
)
{
  moth_buffer       buffer;   /* holds final data */
  moth_label_token  token;
  char              *text;

  buffer = moth_buffer_create ();
  if (!buffer)
    die ("internal error:  couldn't allocate buffer for label value");

  for (token=tokens->tokens; token; token=token->next) {
    if (MOTH_LABEL_TOKEN_REPLACEABLE == token->type) {
      moth_label_tick_format_token (buffer, token, scaling, value);
    }
    else {
      moth_buffer_add (buffer, token->text, token->length);
    }
  }

  text = moth_buffer_strdup (buffer);
  moth_buffer_destroy (buffer);
  return text;
}



INTERNAL
void
moth_label_create_ticks
(
  moth_axis   axis,
  moth_label  label,
  int         max_pixels
)
{
  double          min_unit, max_unit, mod_unit;
  double          v;
  moth_label_tokens tokens;
  moth_label_tick   tick, last_tick;

  /* dependent on scale */
  min_unit = scale_value (axis->scaling, axis->min);
  max_unit = scale_value (axis->scaling, axis->max);
  mod_unit = convert_value (label->modulus_value, label->modulus_modifier,
      label->modulus_unit);

  tokens = moth_label_tokens_create (label->template);

  /* safety code:  if more ticks than pixels, warn and punt */
  if ( ((max_unit-min_unit) / mod_unit) > max_pixels) {
    char *template;
    template = moth_buffer_strdup (label->template);
    warn ("Too many ticks in <label modulus=\"%g%c%c\">%s</label>.\n" 
        "The minimum of the axis is %g and the maximum is %g.\n"
        "Try using a larger modulus.",
        label->modulus_value, label->modulus_modifier, label->modulus_unit,
        template, min_unit, max_unit);
    free (template);
    return;
  }

  /* Step through values (v), creating ticks */
  if ( 'd' == label->modulus_modifier ) {
    v = datetime_modulus (min_unit, label->modulus_unit, label->modulus_value);
  }
  else {
    v = mod_unit * trunc(min_unit/mod_unit);
  }
  if ( 'd' == label->offset_modifier ) {
    v = datetime_add (v, label->offset_unit, label->offset_value);
  }
  else {
    v += convert_value (label->offset_value, label->offset_modifier, 
        label->offset_unit);
  }

  last_tick = NULL;
  for ( ; v <= max_unit; 
            ('d' == label->modulus_modifier)
            ? (v = datetime_add(v,label->modulus_unit,label->modulus_value))
            : (v += mod_unit)
    ) {
    /* 
     * Negative offsets will cause us to start with a tick before the minimum
     * value for the axis.  Skip that tick.
     */
    if (v < min_unit)
      continue;

    tick = (moth_label_tick) malloc (sizeof(struct moth_label_tick_s));
    tick->value = v;
    tick->text = NULL;
    if (tokens) {
      tick->text = moth_label_tick_format_value (tokens, axis->scaling,
          tick->value);
    }
    tick->next = NULL;

    if (last_tick)
      last_tick->next = tick;
    else
      label->ticks = tick;

    last_tick = tick;
  }

  moth_label_tokens_destroy (tokens);
}



void
moth_label_tick_destroy
(
  moth_label_tick tick
)
{
  if (!tick)
    return;
  if (tick->text)
    free (tick->text);
  free (tick);
}



/*--------------------------------------------------------*\
|                      axis functions                      |
\*--------------------------------------------------------*/

moth_axis
moth_axis_create (
  moth_axis_location location
)
{
  moth_axis axis;

  axis = (moth_axis) malloc (sizeof(struct moth_axis_s));
  axis->columns         = moth_column_list_create ();
  axis->location        = location;
  axis->scaling         = MOTH_SCALING_DEFAULT;
  axis->clip_min        = NAN;
  axis->clip_max        = NAN;
  axis->clip_force      = 0;
  axis->snap_to_value   = NAN;
  axis->snap_to_modifier  = ' ';
  axis->snap_to_unit    = ' ';
  axis->labels          = NULL;  /* empty linked list */
  axis->title           = NULL;
  axis->color           = NULL;   /* defaults to black, but that's handled elsewhere */
  axis->configured      = 0;
  axis->min             = NAN;
  axis->max             = NAN;

  return axis;
}



int
moth_axis_configure
(
  moth_axis   axis,
  const char  **attributes
)
{
  int success;
  int a;

  /* Here's the trick:  we need to look at the "location" attribute before all
   * others so that we can do -nothing- if this is the wrong axis */
  for (a=0; attributes[a]; a+=2) {
    const char  *attname = attributes[a];
    const char  *attvalue = attributes[a+1];

    if (0==strcasecmp("location",attname)) {
      moth_axis_location location;
      location = moth_axis_find_location (attvalue);
      if (location != axis->location)
        return -1;
    }
  }

  /* can only configure each axis once */
  if (axis->configured) {
    die ("%s <axis> already exists", moth_axis_find_name(axis->location));
  }

  /* location scaling clip-max clip-min clip-force snap-to */
  success = 1;
  for (a=0; attributes[a]; a+=2) {
    const char  *attname = attributes[a];
    const char  *attvalue = attributes[a+1];

    if (0==strcasecmp("location",attname)) {
      /* already taken care of */
    }
    else if (0==strcasecmp("scaling",attname)) {
      if (0==strcasecmp("linear",attvalue)) {
        axis->scaling = MOTH_LINEAR;
      }
      else if (0==strcasecmp("log2",attvalue)) {
        axis->scaling = MOTH_LOG2;
      }
      else if (0==strcasecmp("loge",attvalue)) {
        axis->scaling = MOTH_LOGE;
      }
      else if (0==strcasecmp("log10",attvalue)) {
        axis->scaling = MOTH_LOG10;
      }
      else if (0==strcasecmp("auto",attvalue)) {
        axis->scaling = MOTH_AUTO;
      }
      else {
        warn ("unknown <axis> attribute %s=\"%s\".  using default",
            attname, attvalue);
      }
    }
    else if (0==strcasecmp("clip-min",attname)) {
      axis->clip_min = read_value (attvalue);
      if (isnan(axis->clip_min))
        die ("couldn't understand <axis> attribute %s=\"%s\"",
            attname, attvalue);
    }
    else if (0==strcasecmp("clip-max",attname)) {
      axis->clip_max = read_value (attvalue);
      if (isnan(axis->clip_max))
        die ("couldn't understand <axis> attribute %s=\"%s\"",
            attname, attvalue);
    }
    else if (0==strcasecmp("clip-force",attname)) {
      if (   0==strcasecmp("on",attvalue)
          || 0==strcasecmp("yes",attvalue)
          || 0==strcasecmp("true",attvalue)
          || 0==strcasecmp("1",attvalue) )
        axis->clip_force = 1;
    }
    else if (0==strcasecmp("snap-to",attname)) {
      if ( ! parse_value(attvalue, &(axis->snap_to_value), 
            &(axis->snap_to_modifier), &(axis->snap_to_unit)))
      {
        axis->snap_to_value = read_value (attvalue);
      }
      if (isnan(axis->snap_to_value))
        die ("couldn't understand <axis> attribute %s=\"%s\"",
            attname, attvalue);
      /* negative numbers don't make sense for a snap-to */
      axis->snap_to_value = fabs (axis->snap_to_value);
    }
    else if (0==strcasecmp("title",attname)) {
      axis->title = strdup (attvalue);
    }
    else if (0==strcasecmp("color",attname)) {
      axis->color = moth_image_get_color (
          moth_parser_current_image(runtime->file->parser),attvalue);
      if (! axis->color)
        die ("couldn't understand <axis> attribute %s=\"%s\"",
            attname, attvalue);
    }
    else {
      warn ("unknown <axis> attribute %s=\"%s\"", attname, attvalue);
      success = 0;
    }
  }

  axis->configured = success;
  return success;
}



void
moth_axis_add_column
(
  moth_axis   axis,
  moth_column column
)
{
  moth_column_list_add (axis->columns, column);

  /* use default configuration if it hasn't explicitely been set */
  axis->configured = 1;
}



void
moth_axis_add_label
(
  moth_axis   axis,
  moth_label  label
)
{
  moth_label l, last;
  
  /* find last label in linked list */
  l = last = axis->labels;
  while (l) {
    last = l;
    l = l->next;
  }
  if (last)
    last->next = label;
  else
    axis->labels = label;
}



void
moth_axis_process
(
  moth_axis axis,
  int       max_pixels
)
{
  moth_label label;

  if (!axis) return;

  /* find min/max */
  if (axis->columns) {
    axis->min = moth_column_list_min (axis->columns, 0);
    axis->max = moth_column_list_max (axis->columns, 0);
  }

  /* expand range, centerd, if there is only one value */
  if (axis->min == axis->max) {
    axis->min -= MOTH_AXIS_SINGLE_VALUE_RANGE / 2.0;
    axis->max += MOTH_AXIS_SINGLE_VALUE_RANGE / 2.0;
  }

  /* clip */
  if (finite(axis->clip_min)) {
    if (axis->clip_force)
      axis->min = axis->clip_min;
    else
      axis->min = (axis->clip_min < axis->min) ? axis->clip_min : axis->min;
  }
  if (finite(axis->clip_max)) {
    if (axis->clip_force)
      axis->max = axis->clip_max;
    else
      axis->max = (axis->clip_max > axis->max) ? axis->clip_max : axis->max;
  }

  if (MOTH_AUTO == axis->scaling)
    axis->scaling = moth_column_list_find_best_scaling (axis->columns);

  /* snap   (this is based on scaling as well) */
  if (finite(axis->snap_to_value)) {
    double min_unit, max_unit, snap_unit;
    double new_min, new_max;

    /* translate to units  (based on scalling) */
    min_unit = scale_value (axis->scaling, axis->min);
    max_unit = scale_value (axis->scaling, axis->max);

    if ( 'd' == axis->snap_to_modifier ) {
      new_min = datetime_modulus (min_unit, axis->snap_to_unit,
          axis->snap_to_value);
      new_max = datetime_modulus (max_unit, axis->snap_to_unit,
          axis->snap_to_value);
      while ( new_max < max_unit ) {
        new_max = datetime_add (new_max, axis->snap_to_unit,
            axis->snap_to_value);
      }
    }
    else {
      snap_unit = convert_value (axis->snap_to_value, axis->snap_to_modifier,
          axis->snap_to_unit);
      new_min = snap_unit * floor(min_unit/snap_unit);
      new_max = snap_unit * ceil(max_unit/snap_unit);
    }

    /* translate from units  (based on scalling) */
    axis->min = unscale_value (axis->scaling, new_min);
    axis->max = unscale_value (axis->scaling, new_max);
  }

  label = axis->labels;
  while (label) {
    moth_label_create_ticks (axis, label, max_pixels);
    label = label->next;
  }
}



void
moth_axis_dump
(
  moth_axis axis
)
{
  const char *scaling;

  /* only show axes that the user has configured */
  if (!axis->configured)
    return;

  fprintf (stdout, "    <axis");
  fprintf (stdout, " location=\"%s\"", moth_axis_find_name(axis->location));
  if (axis->title)
    fprintf (stdout, " title=\"%s\"", axis->title);

  scaling = NULL;
  switch (axis->scaling) {
    case MOTH_LOG2:
      scaling = "log2";
      break;
    case MOTH_LOGE:
      scaling = "loge";
      break;
    case MOTH_LOG10:
      scaling = "log10";
      break;
    case MOTH_LINEAR:
      scaling = "linear";
      break;
    case MOTH_AUTO:
      scaling = "auto";
      break;
    default:
      /* hmmm... we really shouldn't get here */
      break;
  }
  if (scaling)
    fprintf (stdout, "\n        scaling=\"%s\"", scaling);

  if (!isnan(axis->clip_min))
    fprintf (stdout, "\n        clip-min=\"%g\"", axis->clip_min);
  if (!isnan(axis->clip_max))
    fprintf (stdout, "\n        clip-max=\"%g\"", axis->clip_max);
  fprintf (stdout, "\n        clip-force=\"%s\"", axis->clip_force ? "on" : "off");
  if (!isnan(axis->snap_to_value))
    fprintf (stdout, "\n        snap-to=\"%g%c%c\"", axis->snap_to_value,
        axis->snap_to_modifier, axis->snap_to_unit);
  if (axis->color) {
    fprintf (stdout, "\n        color=\"");
    moth_color_dump (axis->color);
    fprintf (stdout, "\"");
  }

  fprintf (stdout, "\n    >\n");

  if (axis->labels) {
    moth_label label;
    label = axis->labels;
    while (label) {
      moth_label_dump (label);
      label = label->next;
    }
  }

  fprintf (stdout, "    </axis>\n");
}



void
moth_axis_destroy
(
  moth_axis axis
)
{
  if (!axis)
    return;
  if (axis->columns)
    moth_column_list_destroy (axis->columns);
  if (axis->labels)
    moth_label_destroy (axis->labels);
  if (axis->title)
    free (axis->title);
  if (axis->color)
    moth_color_destroy (axis->color);
  free (axis);
}



const char *
moth_axis_find_name
(
  moth_axis_location location
)
{
  if (location == MOTH_LEFT)
    return "left";
  else if (location == MOTH_TOP)
    return "top";
  else if (location == MOTH_RIGHT)
    return "right";
  else if (location == MOTH_BOTTOM)
    return "bottom";
  return NULL;
}



moth_axis_location
moth_axis_find_location
(
  const char *name
)
{
  if (0==strcasecmp("left",name))
    return MOTH_LEFT;
  else if (0==strcasecmp("top",name))
    return MOTH_TOP;
  else if (0==strcasecmp("right",name))
    return MOTH_RIGHT;
  else if (0==strcasecmp("bottom",name))
    return MOTH_BOTTOM;
  else
    return MOTH_LOCATION_UNKNOWN;
  return -1;
}



/*--------------------------------------------------------*\
|                     label functions                      |
\*--------------------------------------------------------*/

moth_label
moth_label_create
(
  const char **attributes
)
{
  moth_label  label;
  int         a;

  label = (moth_label) malloc (sizeof(struct moth_label_s));
  label->next = NULL;     /* empty linked list */
  label->color = NULL;    /* defaults to axis color, but that's handled
                           * elsewhere */
  label->modulus_value    = NAN;
  label->modulus_modifier = ' ';
  label->modulus_unit     = ' ';
  label->offset_value     = 0.0;
  label->offset_modifier  = ' ';
  label->offset_unit      = ' ';
  label->template         = moth_buffer_create ();
  label->ticks            = NULL;    /* empty linked list */

  for (a=0; attributes[a]; a+=2) {
    const char *attname = attributes[a];
    const char *attvalue = attributes[a+1];

    if (0==strcasecmp("color",attname)) {
      label->color = moth_image_get_color (
          moth_parser_current_image(runtime->file->parser),attvalue);
      if (! label->color)
        die ("couldn't understand <label> attribute %s=\"%s\"",
            attname, attvalue);
    }
    else if (0==strcasecmp("modulus",attname)) {
      if ( ! parse_value(attvalue, &(label->modulus_value), 
            &(label->modulus_modifier), &(label->modulus_unit)))
      {
        label->modulus_value = read_value (attvalue);
      }
      if (isnan(label->modulus_value))
        die ("couldn't understand <label> attribute %s=\"%s\"",
            attname, attvalue);
      /* negative numbers don't make sense for a modulus */
      label->modulus_value = fabs (label->modulus_value);
    }
    else if (0==strcasecmp("offset",attname)) {
      if ( ! parse_value(attvalue, &(label->offset_value), 
            &(label->offset_modifier), &(label->offset_unit)))
      {
        label->offset_value = read_value (attvalue);
      }
      if (isnan(label->offset_value))
        die ("couldn't understand <label> attribute %s=\"%s\"",
            attname, attvalue);
    }
    else {
      warn ("unknown <label> attribute %s=\"%s\"", attname, attvalue);
    }
  }

  /* required attributes */
  if (isnan(label->modulus_value))
    die ("missing <label> attribute \"modulus\"");

  return label;
}



void
moth_label_add_chunk
(
  moth_label  label,
  const char  *chunk,
  size_t      chunkLen
)
{
  moth_buffer_add (label->template, chunk, chunkLen);
}



INTERNAL
void
moth_label_dump
(
  moth_label label
)
{
  fprintf (stdout, "      <label");
  if (!isnan(label->modulus_value))
    fprintf (stdout, " modulus=\"%g%c%c\"", label->modulus_value,
        label->modulus_modifier, label->modulus_unit);
  if (!isnan(label->offset_value))
    fprintf (stdout, " offset=\"%g%c%c\"", label->offset_value,
        label->offset_modifier, label->offset_unit);
  if (label->color) {
    fprintf (stdout, " color=\"");
    moth_color_dump (label->color);
    fprintf (stdout, "\"");
  }
  fprintf (stdout, ">");

  if (label->template && 0!=moth_buffer_size(label->template)) {
    char *template;
    template = moth_buffer_strdup (label->template);
    fprintf (stdout, "%s", template);
    free (template);
  }

  fprintf (stdout, "</label>\n");
}



void
moth_label_destroy
(
  moth_label label
)
{
  moth_label_tick tick, doomed;
  if (!label)
    return;
  if (label->next)
    moth_label_destroy (label->next);
  if (label->color)
    moth_color_destroy (label->color);
  if (label->template)
    moth_buffer_destroy (label->template);
  tick = label->ticks;
  while (tick) {
    doomed = tick;
    tick = tick->next;
    moth_label_tick_destroy (doomed);
  }
  free (label);
}



