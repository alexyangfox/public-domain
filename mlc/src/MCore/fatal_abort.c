// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Provides error handling capablities.  See error.c for a
                   full description of error handling.
  Assumptions  :
  Comments     : This is not in error.c so that this function can be
                   overwritten to test the fatal_error loop detection.
  Complexity   :
  Enhancements :
  History      : Ronny Kohavi                                       8/19/93
                   Added check for centerline_true() in which case
                   we want to abort() and not exit, so we'll see our place
                   in the stack and can go up to error.
                 Ronny Kohavi                                       7/03/93
                   Initial revision (originally in error.c)
***************************************************************************/

#include <basics.h>
#include <error.h>
#include <string.h> // Note string.h, not MString.h (for strtok())
#include <sysent.h> // Required for unlink. 

const int defaultLineWidth = 80;

int errLineWidth = defaultLineWidth; 

RCSID("MLC++, $RCSfile: fatal_abort.c,v $ $Revision: 1.20 $")
      


/***************************************************************************
  Description : Print string to cerr.  Insert newline on a word that
                   would wrap.
  Comments    : If a word doesn't fit on a line it is printed on a
                   line by itself (and the hardware will do something,
                   usually auto-wrap or truncate).
		We assume that only a period is a bad message (fatal_error
		   appends a period).
***************************************************************************/
static void output_broken_line(char *err_text)
{
   if (errLineWidth == 0 || errLineWidth == SHORT_MAX)
      errLineWidth = defaultLineWidth;

   if (strcmp(err_text, ".") == 0 || strcmp(err_text, " .") == 0)
      cerr << "fatal_abort: no message on err stream." << endl;
   else {
      cerr << ERROR_PREFIX;
      int pos = strlen(ERROR_PREFIX);
   
      char *text = strtok(err_text, WHITESPACE);
      while(text) {
	 pos += strlen(text) + 1;
	 if (pos < errLineWidth)
	    cerr << ' ' << text;
	 else {
	    cerr << endl << WRAP_INDENT << text;
	    pos = strlen(WRAP_INDENT) + strlen(text);
	 }
	 text = strtok(NULL, WHITESPACE); // passing NULL continues on string.
      }
      cerr << endl;
   }
}


/***************************************************************************
  Description : Causes the program to terminate.  Writes messages in err
                  to cerr.  If run in ObjectCenter or if the DUMPCORE
		  environment variable is set, dumps the core; otherwise
		  exits with a bad status.
  Comments    :
***************************************************************************/
ostream& fatal_abort(ostream& err2) // this should be err
{
   // flush any messages.  On Solaris there were cases
   //   where messages  appeared AFTER the fatal error.
   // Do not use regular Mcout because the stream may be bad.
   Mcout.get_fstream() << endl;
   if (err2 != err.get_stream())
      cerr << "fatal_abort: aborting, but stream != err." << endl
           << "  Check why the message triggering fatal_error/fatal_abort\n"
	      "  does not starts with 'err << '." << endl;
   output_broken_line(err_text);
  
#  if defined(__CENTERLINE__)
   // If we are in centerline, this leaves us in the program so we can look 
   //   at the stack, go up levels, etc.
   if (centerline_true())
     abort();
#  endif

   // Dump core if DUMPCORE is yes.
   // Note that you must not have limited coredumpsize (do limit in csh).
   char *dumpcore = getenv("DUMPCORE");
   if (dumpcore && (!strcmp(dumpcore, "yes") || !strcmp(dumpcore, "YES"))) {
      // Attempt to remove old core.  An existing core would not be
      //   overwritten if you do not have write permission to the
      //   file, even if you have write permission to the directory.
      //   Too many times, core files are left with no group write
      //   permission, but the directories are group writable.
      (void)unlink("core"); // ignore status.  May fail if there is no core
                             // or if the directory is not writable   
      abort();
   }
   exit(bad_status); // bad status
   return err2;      // we'll never get here, but just to avoid warnings...
}

