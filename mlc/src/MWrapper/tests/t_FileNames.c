// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test FileNames
  Doesn't test :
  Enhancements :
  History      : Ronny Kohavi                                       12/16/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <FileNames.h>

RCSID("MLC++, $RCSfile: t_FileNames.c,v $ $Revision: 1.9 $")

#define PUTENV(str) ASSERT(putenv(str) == 0)

main()
{
   Mcout << "t_FileNames executing" << endl;

   {
      PUTENV("DATAFILE=monk1-full");
      PUTENV("DUMPSTEM=foofoo");
      FileNames files;
      ASSERT(files.data_file() == "monk1-full.data");
      ASSERT(files.names_file() == "monk1-full.names");
      ASSERT(files.test_file() == "monk1-full.test");
      ASSERT(files.dump_stem() == "foofoo");
   }


   {
      PUTENV("DATAFILE=monk1-full.data");
      PUTENV("DUMPSTEM=bahbah");
      FileNames files;
      ASSERT(files.data_file() == "monk1-full.data");
      ASSERT(files.names_file() == "monk1-full.names");
      ASSERT(files.test_file() == "monk1-full.test");
      ASSERT(files.dump_stem() == "bahbah");
   }

   {
      PUTENV("DATAFILE=monk1-full");
      PUTENV("NAMESFILE=monk1-full");
      PUTENV("TESTFILE=monk1-full");
      FileNames files;
      ASSERT(files.data_file() == "monk1-full.data");
      ASSERT(files.names_file() == "monk1-full.names");
      ASSERT(files.test_file() == "monk1-full.test");
   }

   {
      PUTENV("DATAFILE=monk1-full");
      PUTENV("NAMESFILE=monk1-full.foo");
      PUTENV("TESTFILE=monk1-full.bar");
      FileNames files;
      ASSERT(files.data_file() == "monk1-full.data");
      ASSERT(files.names_file() == "monk1-full.foo");
      ASSERT(files.test_file() == "monk1-full.bar");
   }

   {
      PUTENV("DATAFILE=monk1-full.all");
      PUTENV("TESTFILE=");
      FileNames files;
      ASSERT(files.data_file() == "monk1-full.all");
      ASSERT(files.names_file() == "monk1-full.foo");
      ASSERT(files.test_file(FALSE) == "");
   }

   {
      PUTENV("DATAFILE=monk1-full.all");
      PUTENV("TESTFILE=monk1-full.test");
      FileNames files;
      ASSERT(files.data_file() == "monk1-full.all");
      ASSERT(files.names_file() == "monk1-full.foo");
      ASSERT(files.test_file() == "monk1-full.test");
   }

   {
      PUTENV("DATAFILE=monk1-foo");
      FileNames files;
      ASSERT(files.data_file() == "monk1-foo.data");
      ASSERT(files.test_file(FALSE) == "monk1-full.test");
   }

   return 0; // return success to shell
}   
