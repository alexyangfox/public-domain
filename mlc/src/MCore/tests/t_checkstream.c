// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test the check_stream routines.
                 We basically close files and see that output fails,
                   or try to read passed EOF to see that input fails.
  Doesn't test :
  Enhancements : Test other problems, not just closed files, but this
                   is really hard to do. 
  History      : Ronny Kohavi                                       7/28/93
                   Initial revision

***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <string.h> // note string, not MString
#include <checkstream.h>
#include <stream.h>
#include <MLCStream.h>

RCSID("MLC++, $RCSfile: t_checkstream.c,v $ $Revision: 1.15 $")


const MString fileName = "t_checkstream.out1";

main()
{
   Mcout << "t_checkstream executing" << endl;

   ofstream file(fileName);
   if (!file) err << "Can't open " << fileName << fatal_error;

   // Write "test" to the file.  Close it, then make sure we can't write
   file << "test" << endl;
   check_ostream(file,fileName);
   file.close();
   TEST_ERROR("Unable to write to file " + fileName,
              file << "this should abort"; check_ostream(file,fileName));
   // Test the direct function call method
   file << "this should abort";  // remember, error cleared already
   TEST_ERROR("Unable to write to file " + fileName,
              check_ostream(file, fileName));


   // Now let's read the "test" string we wrote.
   ifstream ifile(fileName);
   char line[50];
   ifile >> line;
   check_istream(ifile, fileName);
   ASSERT(strcmp(line, "test") == 0);


   // Since there's only one line, we should bump on EOF.
   TEST_ERROR("Attempted read after EOF on file " + fileName,
              ifile >> line; check_istream(ifile, fileName));


   // Make sure check_cout compares to see that it's getting cout
   TEST_ERROR("stream != cout", check_cout(cerr));


   // Make sure check_cin compares to see that it's getting cin.
   TEST_ERROR("stream != cin", ifile >> line; check_cin(ifile));
   istream_withassign cin_save;
   cin_save = cin;
   cin = ifile;
   // Mess up the input and check that things don't work
   TEST_ERROR("Bad state for cin (state=3)", cin >> line; check_cin(cin));
   cin = cin_save;
   
   // mess up cout and restore.
   ostream_withassign cout_save;
   cout_save = cout; // Note this doesn't work in the declaration
                    // (complains about private members).  
   cout = file; // closed file, so it's an error
   TEST_ERROR("Bad state for cout",
              cout << "shouldn't print" << endl; check_cout(cout));
   cout = cout_save;
   
   // Note that cout must be restored before exit() because its
   //   destructor is being called AFTER the file constructor and we
   //   get a run-time error otherwise.
   return 0; // return success to shell
}   




