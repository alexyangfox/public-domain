// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Read labor-neg and dump binary and local-encoding conversion
                 of data files to "t_convDisplay.out1" and
		 "t_convDisplay.out2" respectively. *.names files are dumped
		 into t_convDisplay.out3 for binary conversion and
		 t_convDisplay.out4 for local conversion.
  Doesn't test :
  Enhancements :
  History      : Chia-Hsin Li                                       9/27/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <InstList.h>
#include <convDisplay.h>

RCSID("MLC++, $RCSfile: t_convDisplay.c,v $ $Revision: 1.10 $")


main()
{
   Mcout << "t_convDisplay executing" << endl;
//   MString datafile("monk2");
   MString datafile("labor-neg");
   MString emptyFile("t_convDisplay");

   InstanceList file(datafile);
   InstanceList empty(emptyFile);
   
   MLCOStream outfile1("t_convDisplay.out1"),
      outfile2("t_convDisplay.out2"),
      outfile3("t_convDisplay.out3"),
      outfile4("t_convDisplay.out4"),
      outfile5("t_convDisplay.out5"),
      outfile6("t_convDisplay.out6"),
      outfile7("t_convDisplay.out7");
   
   display_list_rep(file, file.get_schema(),
		    outfile1, local, noNormalization, ", ", ", ", ".", NULL );
   display_list_rep(file, file.get_schema(),
		    outfile2, binary, noNormalization, ", ", ", ", ".", NULL );
   display_names_local(file.get_schema(), outfile3, "Bin encoding conversion");
   display_names_bin(file.get_schema(), outfile4,
		       "Local encoding conversion");
   display_list_rep(empty, empty.get_schema(),
		    outfile6, local, noNormalization, ", ", ", ", ".", NULL );
   display_list_rep(empty, empty.get_schema(),
		    outfile6, binary, noNormalization, ", ", ", ", ".", NULL );

   InstanceList list1("", "t_convDisplay.out3", "t_convDisplay.out1");
   outfile5 << list1 << endl;
   InstanceList list2("", "t_convDisplay.out4", "t_convDisplay.out2");
   outfile5 << list2 << endl;

   // now test pebls format.
   display_list_pebls(file, outfile7, "TRAIN");
   display_list_pebls(file, outfile7, "TEST");

   return 0; // return success to shell
}   

