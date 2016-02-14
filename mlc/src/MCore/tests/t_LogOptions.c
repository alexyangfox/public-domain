// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test LogOptions
  Doesn't test :
  Enhancements :
  History      : Chia-Hsin Li                                     12/27/94
                   Added test for FLOG.
                 Ronny Kohavi                                     10/11/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <LogOptions.h>

RCSID("MLC++, $RCSfile: t_LogOptions.c,v $ $Revision: 1.3 $")

class Test{
   LOG_OPTIONS;
public:
   void output_text();
};

void Test::output_text()
{
   // The messages are defined here and not in the LOG statement
   //   because some compilers give the line number as the last line
   //   of the statement (SGI's CC) and some give the first line
   //   ObjectCenter's CFfront.
   
   MString msg1("This is a test of the log options which should wrap over more"
          " than one line to see that everything is working.");
   
   MString msg2("This is executed when log level is set to 2 only.\n"
          "It is longer than the previous statement and should start to wrap"
          " sometimes now 12345 67890");

   Mcout << "In output_text()" << endl;
   LOG(1, msg1 << endl);
   LOG(2, msg2 << endl);
}

void test_flog(const LogOptions& logOptions)
{
   Mcout << "In test_flog()" << endl;
   FLOG(1, "This is a test of the function log options and executed when log "
           "level is set to 1." << endl);
   FLOG(2, "This is excuted when loglevel is set to 2 only." << endl);
}

main()
{
   Mcout << "t_LogOptions executing" << endl;

   Test t;
   t.output_text();
   t.set_log_level(1);
   t.output_text();
   test_flog(t.get_log_options());
   t.set_log_level(2);
   t.output_text();
   test_flog(t.get_log_options());
   putenv("SHOW_LOG_ORIGIN=yes");
   t.output_text();


   return 0; // return success to shell
}





