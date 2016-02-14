// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test mlcIO
  Doesn't test : Doesn't test much.  This is a temporary version
  Enhancements :
  History      : Ronny Kohavi                                       9/2/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <mlcIO.h>

RCSID("MLC++, $RCSfile: t_mlcIO.c,v $ $Revision: 1.9 $")


main()
{
   Mcout << "t_mlcIO executing" << endl;

   ASSERT(file_exists("/etc/passwd") != "");
   ASSERT(file_exists("/foo/bar/etc/passwd") == "");

   MLCOStream stream("t_mlcIO.out1");
   stream << "This is a long test that goes on for a while" << endl;
   stream.get_stream().seekp(MString("This is a ").length());
   stream << "short test" << endl;
   truncate_file(stream);

   // test bad path
   putenv("MLCPATH=.:~sommda/code/basic:/u/mlc");
   TEST_ERROR("mlcIO::file_exists", file_exists("monk1.names"));

   MString file1 = get_temp_file_name();
   MString file2 = get_temp_file_name();
   ASSERT(file1 != file2);

   MString tmpFile = get_temp_file_name();
   if (tmpFile == "" || tmpFile == ".MLC")
      err << "Empty file name.  get_temp_file_name failed" << fatal_error;

   MLCOStream tmpStream(tmpFile);
   tmpStream << "test";
   tmpStream.close();
   remove_file(tmpFile);

   if (file_exists(tmpFile, FALSE) != "")
      err << "File " << tmpFile << " still exists after remove" << fatal_error;

   // Test that removes really gives a warning.
   remove_file("/tmp/nonexistent-file.MLC");

   return 0; // return success to shell
}   

