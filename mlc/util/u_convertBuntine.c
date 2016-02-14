// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a utility for MLC++
// It is not part of the MLC++ library itself, but instead uses the
//   library to provide a useful function.

/***************************************************************************
  Description  : This converts a data and test files from Quinlan/Irvine
                    format to Buntine format.
  Usage        : Buntine format is used for Oliver's "dgraph".
  Enhancements :
  History      : Richard Long                                       1/24/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <InstList.h>
#include <LogOptions.h>
#include <FileNames.h>
#include <GetOption.h>

RCSID("MLC++, $RCSfile: u_convertBuntine.c,v $ $Revision: 1.7 $")

const MString buntineFileHelp =
  "This option specifies the file into which to output the Buntine "
  "format version of the data file.";

main()
{
   FileNames files;
   MString dataFile = files.data_file();
   MString namesFile = files.names_file();
   MString testFile = files.test_file();
   
   MString rootname;
   if(dataFile.contains("."))
      rootname = MString(dataFile, dataFile.index("."));
   else
      rootname = dataFile;
   
   MString buntineFile =
      get_option_string("BUNTINEFILE", rootname + ".dta",
			buntineFileHelp, TRUE);
  
   InstanceList il("", namesFile, dataFile);

   MLCOStream out1(buntineFile);
   
   il.buntine_display(out1);
   il.read_data(testFile);
   il.buntine_display(out1);
   
   return 0; // return success to shell
}   
