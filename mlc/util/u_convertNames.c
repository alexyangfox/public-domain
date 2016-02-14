// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a utility for MLC++
// It is not part of the MLC++ library itself, but instead uses the
//   library to provide a useful function.

/***************************************************************************
  Description  : Given two name files N1 and N2 and one datafile D, the utility
                   reads in the InstanceList D using N1 and writes them
                   out using the schema N2.  If the two schemas are compatible,
		   i.e., attributes types and number of values for nominals,
		   the conversion will be successful, but the names in N2
		   will be written out.
		 This is useful for converting obscure values in nominal
		   attributes to comprehensible values.
		 If DATAFILE + ".test" exists, it is converted too.
  Usage        : Requires the following environment variables to be set:
                   DATAFILE - source datafile (must not contain period).
                   DUMPSTEM  - stem for output file.  Must not contain
                               a period.  We assume the namesfile exists.
  Enhancements :
  History      : Ronny Kohavi                                       12/14/94
                   Made safer (test DUMPSTEM is different), fixed
		   the output files which were wrong.
                 James Dougherty                                    10/26/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <LogOptions.h>
#include <mlcIO.h>
#include <MLCStream.h>
#include <MString.h>
#include <InstList.h>
#include <FileNames.h>

RCSID("MLC++, $RCSfile: u_convertNames.c,v $ $Revision: 1.5 $")

main()
{
   FileNames files;
   MString dataFile = files.data_file();
   MString dumpStem = files.dump_stem();	 
   MString namesFile = files.names_file();

   Mcout << "Converting..." << endl;

   InstanceList sourceBag("", namesFile, dataFile); 
   // Read in the names file, no data.
   InstanceList namesBag(dumpStem ,argDummy); 

   // Change the schema of the source bag to the new schema.
   sourceBag.set_schema(namesBag.get_schema());
   
   MLCOStream outFile(dumpStem + ".data");
   outFile << sourceBag;
   outFile.close();

   if(files.test_file() != ""){
      InstanceList sourceTestBag("", namesFile, files.test_file());
      sourceTestBag.set_schema(namesBag.get_schema());

      MLCOStream outFile(dumpStem + ".test");
      outFile << sourceTestBag;
      outFile.close();
   }

   Mcout << "Done." << endl;

   return 0; // return success to shell
}   



