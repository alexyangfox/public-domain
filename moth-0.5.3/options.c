/*
 * $Id: options.c,v 1.8 2003/06/16 18:25:32 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 * See options.dev for implementation notes.
 *
 */



#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "options.h"



/*--------------------------------------------------------*\
|                      datastructures                      |
\*--------------------------------------------------------*/

typedef const struct moth_option_definition_s *moth_option_definition;
struct moth_option_definition_s {
  const char  *name;
  const int   default_value;
  const char  argument_shortcut;
};
struct moth_option_definition_s moth_option_definitions[] =
{
  {"dump-columns",  0, 'c'},
  {"draw",          1, 'd'},
  {"profile",       0, 'p'},
  {"quiet",         0, 'q'},
  {"xml-relative",  1, 'r'},
  {"dump-xml",      0, 'x'},
  {NULL, 0, '\0'}         /* end marker */
};



typedef struct moth_option_s *moth_option;
struct moth_option_s {
  moth_option_definition  definition;
  int                     argument, attribute;
  moth_option             next;   /* linked list */
};



struct moth_options_s {
  moth_option   options;  /* head of linked list */
};



/*--------------------------------------------------------*\
|                    utility functions                     |
\*--------------------------------------------------------*/

moth_option
moth_option_create
(
  moth_option_definition definition
)
{
  moth_option option;
  option = (moth_option)
    malloc (sizeof(struct moth_option_s));
  option->definition = definition;
  option->argument = -1;    /* AKA undefined */
  option->attribute = -1;   /* AKA undefined */
  option->next = NULL;
  return option;
}



void
moth_option_destroy
(
  moth_option option
)
{
  /* Yup, it's that simple.  The definition is actually statically defined, so
   * we don't want to free that.  */
  free (option);
}



/*
 * Read the string of characters as a string of short options.
 */
int
moth_options_read_short_group
(
  moth_options  options,
  const char    *list
)
{
  size_t    args, a;

  args = strlen (list);
  for (a=0; a<args; ++a) {
    char        argument;
    moth_option option;
    
    argument = list[a];
    option = options->options;
    while (option) {

      /* short match */
      if (argument == option->definition->argument_shortcut) {
        option->argument = 1;
        break;
      }

      /* short negative match */
      else if (argument == toupper(option->definition->argument_shortcut)) {
        option->argument = 0;
        break;
      }

      option = option->next;
    }

    /* option not found */
    if (!option)
      return 0;
  }

  return 1;
}



/*--------------------------------------------------------*\
|                        public API                        |
\*--------------------------------------------------------*/

moth_options
moth_options_create ()
{
  moth_options            options;
  moth_option_definition  definition;
  moth_option             option, last_option;

  options = (moth_options)
    malloc (sizeof(struct moth_options_s));
  options->options = NULL;

  definition = moth_option_definitions;
  option = last_option = NULL;
  while (definition && definition->name) {
    option = moth_option_create (definition);
    if (last_option)
      last_option->next = option;
    else
      options->options = option;

    ++definition;
    last_option = option;
  }

  return options;
}



void
moth_options_destroy
(
  moth_options options
)
{
  moth_option doomed, option;

  option = options->options;
  while (option) {
    doomed = option;
    option = option->next;
    moth_option_destroy (doomed);
  }

  free (options);
}



/* Returns success as boolean.  */
int
moth_options_read_argument
(
  moth_options  options,
  const char    *argument
)
{
  moth_option option;

  /* handle short arguments */
  if (strlen(argument) >= 2
      && '-' == argument[0]
      && '-' != argument[1])
  {
    return moth_options_read_short_group (options, &argument[1]);
  }

  option = options->options;
  while (option) {
    const char  *name;        /* code shorthand */
    size_t      name_length;  /* code shorthand */
    char        *test;
    size_t      test_size;

    name = option->definition->name;
    name_length = strlen (name);
    test = NULL;

    /* long negative match */
    test_size = 5               /* '--no-' */
      + name_length
      + 1;                      /* null-terminating character */
    test = (char *) malloc (test_size * sizeof(char));
    snprintf (test, test_size, "--no-%s", name);
    if (0==strcmp(test,argument)) {
      option->argument = 0;
      free (test);
      return 1;
    }
    free (test);

    /* long match */
    test_size = 2               /* '--' */
      + name_length
      + 1;                      /* null-terminating character */
    test = (char *) malloc (test_size * sizeof(char));
    snprintf (test, test_size, "--%s", name);
    if (0==strcmp(test,argument)) {
      option->argument = 1;
      free (test);
      return 1;
    }
    free (test);

    /* long match with value */
    test_size = 2               /* '--' */
      + name_length
      + 1;                      /* '=' */
    test = (char *) malloc ((test_size+1) * sizeof(char));
    snprintf (test, test_size, "--%s=", name);
    if (0==strncmp(test,argument,test_size-1)) {
      const char *arg_value;
      arg_value = argument + test_size;

      if (   0==strcmp(arg_value,"on")
          || 0==strcmp(arg_value,"yes")
          || 0==strcmp(arg_value,"true")
          || 0==strcmp(arg_value,"1"))
      {
        option->argument = 1;
      }
      else {
        option->argument = 0;
      }

      free (test);
      return 1;
    }
    free (test);

    option = option->next;
  }

  return 0;
}


int
moth_options_set_attribute
(
  moth_options options,
  const char *attname,
  const char *attvalue
)
{
  moth_option option;
  option = options->options;
  while (option) {
    if (0==strcmp(attname,option->definition->name)) {
      if (   0==strcmp(attvalue,"on")
          || 0==strcmp(attvalue,"yes")
          || 0==strcmp(attvalue,"true")
          || 0==strcmp(attvalue,"1"))
        option->attribute = 1;
      else
        option->attribute = 0;
      return 1;
    }

    option = option->next;
  }
  return 0;
}



void
moth_options_dump_attributes
(
  moth_options options
)
{
  moth_option option;
  option = options->options;
  while (option) {
    if (1 == option->attribute) {
      fprintf (stdout, " %s=\"on\"", option->definition->name);
    }
    else if (0 == option->attribute) {
      fprintf (stdout, " %s=\"off\"", option->definition->name);
    }
    option = option->next;
  }
}



void
moth_options_reset_attributes
(
  moth_options options
)
{
  /* walk attributes and set to undef (-1) */
  moth_option option;
  option = options->options;
  while (option) {
    option->attribute = -1;
    option = option->next;
  }
}



int
moth_options_get_option
(
  moth_options  options,
  const char    *option
)
{
  moth_option o;

  o = options->options;
  while (o) {
    if (0==strcmp(option,o->definition->name)) {
      if (-1 != o->argument) {
        return o->argument;
      }
      else if (-1 != o->attribute) {
        return o->attribute;
      }
      else {
        return o->definition->default_value;
      }
    }

    o = o->next;
  }

  return 0;
}



