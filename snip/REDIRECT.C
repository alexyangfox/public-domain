/* +++Date last modified: 05-Jul-1997 */

/*
**  REDIRECT.C - Posix-compliant utilities to handle redirection
**
**  written by Benno Sauer, Vienna, 1991
**  released into public domain
*/

#include <stdio.h>
#include <fcntl.h>
#include <sys\stat.h>

#if defined(MSDOS) || defined(__MSDOS__)
 #include "unistd.h"
#else
 #include <unistd.h>
#endif
#include "dirport.h"
#include "snpdosys.h"

/*
**  Use these predefined structures for the 3 primary streams.
**  Define others (e.g. stdprn under DOS) as required.
*/

REDIR_STRUCT redir_stdin, redir_stdout, redir_stderr;



void  start_redirect ( REDIR_STRUCT *s )
{
      if (s->which == fileno(stdin))
            s->what = open ( s->path, O_RDWR, S_IREAD );
      else  s->what = open ( s->path, O_CREAT | O_RDWR, S_IREAD | S_IWRITE );

      s->oldhandle = dup ( s->which );

      dup2  ( s->what, s->which );
      close ( s->what );
      s->flag = 1;
}

void  stop_redirect ( REDIR_STRUCT *s )
{
      if ( s->flag )
      {
            dup2 ( s->oldhandle, s->which );
            close ( s->oldhandle );
            s->flag = 0;
      }
}

#ifdef TEST

#include <string.h>

main()
{
      char line[132];
      int i;

      strcpy(redir_stdin.path, "redirect.c");
      redir_stdin.which = fileno(stdin);

      strcpy(redir_stdout.path, "x-file");
      redir_stdout.which = fileno(stdout);

      strcpy(redir_stderr.path, "file.x");
      redir_stderr.which = fileno(stderr);

      start_redirect(&redir_stdin);
      start_redirect(&redir_stdout);
      start_redirect(&redir_stderr);

      for (i = 1; !feof(stdin); ++i)
      {
            if (fgets(line, 132, stdin))
            {
                  fputs(line, stdout);
                  fprintf(stderr, "Read line #%d\n", i);
            }
      }
      fflush(stdout);
      fflush(stderr);

      stop_redirect(&redir_stdin);
      stop_redirect(&redir_stdout);
      stop_redirect(&redir_stderr);

      fputs("All done!\n", stdout);
      fflush(stdout);
      fputs("Hit Enter to exit\n", stderr);
      getchar();

      return 0;
}

#endif /* TEST */
