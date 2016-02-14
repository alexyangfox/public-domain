// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a utility for MLC++
// It is not part of the MLC++ library itself, but instead uses the
//   library to provide a useful function.

/***************************************************************************
  Description  : Project the namesfile/datafile/testfile on a subset of the
                   attributes.  Useful for feature subset selection
		   experiments, GLDs, etc.
  Usage        : Environment variable DATAFILE determines the soure stem.
                 DUMPSTEM contains the destination stem.
  Enhancements :
  History      : Ronny Kohavi                                       6/12/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <InstList.h>
#include <GetOption.h>
#include <FileNames.h>

RCSID("MLC++, $RCSfile: u_project.c,v $ $Revision: 1.10 $")

main()
{
   FileNames files;

   InstanceList trainSet("", files.names_file(), files.data_file());
   MString fileStem = files.dump_stem();

   int totalNumAttrs = trainSet.num_attr();
   
   BoolArray attr(0, totalNumAttrs, FALSE);

   Mcout << "Enter attribute.  End with -1" << endl;
   int attrNum;
   do {
      Mcin >> attrNum;
      if (attrNum >= 0  && attrNum < totalNumAttrs) {
	 MString name(trainSet.attr_info(attrNum).name());
	 Mcout << "Attribute " << attrNum << " is " << name << endl;
	 attr[attrNum] = TRUE;
      } else if (attrNum != -1)
	 Mcout << "Invalid attribute number.  Must be 0 to "
	       << totalNumAttrs - 1 << endl;
   } while (attrNum != -1);
      
   InstanceBag *projectedData = trainSet.project(attr);
   MString testFile = files.test_file(FALSE);
   MString dataFile;
   if (testFile != "")
      dataFile = fileStem + ".data";
   else
      dataFile = fileStem + ".all";
   
   MLCOStream ProjDataFile(dataFile);
   ProjDataFile << *projectedData << endl;
   MLCOStream namesFile(fileStem + ".names");
   projectedData->display_names(namesFile, TRUE, "Projected from " +
      files.data_file());
   delete projectedData;

   // project a test set if one is given
   if(testFile != "") {
      InstanceList testSet("", files.names_file(),
			   files.test_file());
      InstanceBag *projectedTest = testSet.project(attr);   
      MLCOStream testFile(fileStem + ".test");
      testFile << *projectedTest << endl;

      delete projectedTest;
   }

   return 0;
   
}


