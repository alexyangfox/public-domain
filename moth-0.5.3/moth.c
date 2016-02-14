/*
 * $Id: moth.c,v 1.20 2003/06/25 18:07:52 drewfish Exp $
 *
 * Placed in the public domain by Drew Folta, 2002.  Share and enjoy!
 *
 */


#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#include "colors.h"
#include "data.h"
#include "file.h"
#include "image.h"
#include "options.h"
#include "parser.h"
#include "sources.h"



/*--------------------------------------------------------*\
|                         runtime                          |
\*--------------------------------------------------------*/

moth_runtime  runtime = NULL;

INTERNAL moth_runtime  moth_runtime_create       (int, char **);
INTERNAL void          moth_runtime_read_args    (moth_runtime, int, char*[]);
INTERNAL void          moth_runtime_destroy      (moth_runtime);



/*--------------------------------------------------------*\
|                    utility functions                     |
\*--------------------------------------------------------*/

INTERNAL
void
moth_option_help
(
  const char *program_name
)
{
  printf ("usage:  %s [options] [files]\n", program_name);
  printf ("\n");
  printf ("options:\n");
  printf ("  -h, --help             show this message and quit\n");
  printf ("  -l, --list-colors      list named colors and quit\n");
  printf ("  -v, --version          report version number\n");
  printf ("\n");
  printf ("  -d, --draw             draw images\n");
  printf ("  -D, --no-draw          don't draw images\n");
  printf ("  -c, --dump-columns     print column values from sources\n");
  printf ("  -C, --no-dump-columns  don't print column values\n");
  printf ("  -p, --profile          report how long sources take to process\n");
  printf ("  -P, --no-profile       don't report on sources\n");
  printf ("  -q, --quiet            don't show warnings\n");
  printf ("  -Q, --no-quiet         show warnings\n");
  printf ("  -r, --xml-relative     relative filenames are given in relation to the XML file\n");
  printf ("  -R, --no-xml-relative  relative filenames are given in relation to the cwd\n");
  printf ("  -x, --dump-xml         print XML as interpretted\n");
  printf ("  -X, --no-dump-xml      don't print XML\n");
  printf ("\n");
  printf ("defaults are:\n");
  printf ("  --draw\n");
  printf ("  --no-dump-columns --no-profile --no-quiet\n");
  printf ("  --no-xml-relative --no-dump-xml\n");
  printf ("\n");
}



void
moth_option_version ()
{
  printf ("moth, a 2D graphing application\n");
  printf ("  version %s\n", PACKAGE_VERSION);
  printf ("  please send bug reports to %s\n", PACKAGE_BUGREPORT);
}



/*--------------------------------------------------------*\
|                      execution start                     |
\*--------------------------------------------------------*/

int
main (int argc, char * argv [])
{
  runtime = moth_runtime_create (argc, argv);
  if (!runtime) {
    fprintf (stderr, "ERROR:  couldn't determine runtime\n");
    exit (-1);
  }

  moth_runtime_read_args (runtime, argc, argv);
  moth_runtime_destroy (runtime);
  return 0;
}



/*--------------------------------------------------------*\
|                    runtime functions                     |
\*--------------------------------------------------------*/

INTERNAL
moth_runtime
moth_runtime_create
(
  int   argc,
  char  **argv
)
{
  struct timeval  t;

  runtime = (moth_runtime) malloc (sizeof(struct moth_runtime_s));
  if (!runtime)
    return NULL;

  runtime->options = moth_options_create ();
  runtime->file = NULL;

  /* for isalpha(), isspace(), and friends */
  setlocale (LC_CTYPE, "POSIX");

  /* seed random numbers */
  /* (mainly for <rpn> function RANDOM) */
  t.tv_usec = 0;
#ifdef HAVE_GETTIMEOFDAY
  gettimeofday (&t, NULL);
  srandom ((long int) t.tv_usec);
#endif

  return runtime;
}



INTERNAL
void
moth_runtime_read_args
(
  moth_runtime  runtime,
  int           argc,
  char          *argv[]
)
{
  int i;
  for (i=1; i<argc; ++i) {
    char  *argument;
    argument = argv[i];

    if ('-' == argument[0]) {
      if (! moth_options_read_argument(runtime->options, argument)) {
        if (   0==strcmp("-h",argument)
            || 0==strcmp("--help",argument))
        {
          moth_option_help (argv[0]);
          exit (0);
        }
        else if (0==strcmp("-l",argument)
              || 0==strcmp("--list-colors",argument))
        {
          moth_color_store_list_named ();
          exit (0);
        }
        else if (0==strcmp("-v",argument)
              || 0==strcmp("--version",argument))
        {
          moth_option_version ();
          exit (0);
        }
        warn ("unknown argument %s", argument);
      }
    }
    else {
      runtime->file = moth_file_create (argument);

      moth_file_parse (runtime->file);
      if (moth_options_get_option(runtime->options, "dump-xml"))
        moth_file_dump (runtime->file);
      moth_file_process_sources (runtime->file);
      moth_file_draw_images (runtime->file);

      moth_file_destroy (runtime->file);
      runtime->file = NULL;
      moth_options_reset_attributes (runtime->options);
    }
  }
}



INTERNAL
void
moth_runtime_destroy
(
  moth_runtime runtime
)
{
  if (!runtime)
    return;

  if (runtime->options) {
    moth_options_destroy (runtime->options);
    runtime->options = NULL;
  }

  if (runtime->file) {
    moth_file_destroy (runtime->file);
    runtime->file = NULL;
  }

  free (runtime);
}



