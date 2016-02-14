// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a utility for MLC++
// It is not part of the MLC++ library itself, but instead uses the
//   library to provide a useful function.

/***************************************************************************
  Description  : Checks that number of instances in .test and .data files
  		   adds up the number of instances in the .all if it exists.
		 Reports basic statistics.
  Usage        : Environment variables to set:
                   DATAFILE
  Enhancements :
  History      : YeoGirl Yun                                       12/31/94
                 Brian Frasca                                       6/13/94
***************************************************************************/

#include <basics.h>
#include <CtrInstList.h>
#include <LogOptions.h>
#include <BagSet.h>
#include <CtrBag.h>
#include <mlcIO.h>
#include <InstanceRC.h>
#include <FileNames.h>
#include <GetOption.h>

RCSID("MLC++, $RCSfile: u_info.c,v $ $Revision: 1.21 $")

void attribute_info(const InstanceList& dataBag, const InstanceList* allBag)
{
   CtrInstanceBag bag(dataBag.get_schema());

   if (allBag)
      for (Pix pix = allBag->first(); pix; allBag->next(pix))
	 bag.add_instance(allBag->get_instance(pix));
   else
      for (Pix pix = dataBag.first(); pix; dataBag.next(pix))
	 bag.add_instance(dataBag.get_instance(pix));      

   if (allBag)
      Mcout << "Information about .all file : " << endl;
   else
      Mcout << "Information about .data file : " << endl;
   
   for (int i = 0; i < dataBag.num_attr(); i++) {
      int numValues = 0;
      if (dataBag.attr_info(i).can_cast_to_nominal())  // nominal attributes
	 numValues = bag.counters().attr_num_vals(i);
      else { // continous attributes
	 DynamicArray<Real> realVals(dataBag.num_attr());
	 int c = realVals.low();
	 for (Pix pix = bag.first(); pix; bag.next(pix)) {
	    const InstanceRC& instance = bag.get_instance(pix);
	    if (!bag.attr_info(i).is_unknown(instance[i]))
	       realVals[c++] =
		  bag.attr_info(i).get_real_val(instance[i]);
	 }
	 realVals.truncate(c);
	 realVals.sort();
	 c = realVals.low();
	 Real val = realVals[c];
	 numValues = 1;
	 for (; c <= realVals.high(); c++)
	    if (val != realVals[c]) {
	       numValues++;
	       val = realVals[c];
	    }
      }
      const MString& attrName = bag.attr_info(i).name();
      Mcout << setw(4) <<  numValues << " distinct values for attribute #"
	 << i <<" (" << attrName << ")";
      if (bag.attr_info(i).can_cast_to_nominal())
	 Mcout << " nominal" << endl;
      else if (bag.attr_info(i).can_cast_to_real())
	 Mcout << " continuous" << endl;
      else
	 Mcout << " unknown type" << endl;
   }      
}



void class_probability(const MString& heading, const CtrInstanceList& bag,
		       int num)
{
   for (int i = 0; i < bag.get_schema().num_label_values(); i++) {
	 const SchemaRC& schema = bag.get_schema();
	 const MString& label = schema.
	    category_to_label_string(i + FIRST_CATEGORY_VAL);
	 Real rate =  (Real)bag.counters().
	    label_count(schema.label_info().cast_to_nominal().
			nominal_to_int(label))/(Real)num;
	 Mcout << heading << " - Class probability for the label '" <<
	    label << "' : " << MString(rate*100,2)  << "%" << endl;
   }
}


main()
{
   FileNames files;
   MString dataFile = files.data_file();
   MString rootName;
   
   if (dataFile.contains("."))
      rootName = MString(dataFile, dataFile.index("."));
   else {
      rootName = dataFile;
      dataFile += ".data";
   }

   MString namesFile(rootName + ".names");

   GLOBLOG(1, "DataFile is " << dataFile << ". LOGLEVEL is " <<
           globalLogLevel << endl);
   
   // Read in original train/test bags (these contain all the features) and
   // remove all instances with unknown attributes.
   CtrInstanceList dataBag("", namesFile, dataFile);
   CtrInstanceList orgDataBag(dataBag, ctorDummy);
   CtrInstanceList testBag(rootName, ".names", ".test");
   CtrInstanceList orgTestBag(testBag, ctorDummy);

   int numAll = -1;
   int numUniqAll = -1;
   CtrInstanceList *allBag = NULL;
   CtrInstanceList *orgAllBag = NULL;
   if (file_exists(rootName + ".all") != "") {
      allBag = new CtrInstanceList(rootName, ".names", ".all");
      numAll = allBag->num_instances();
      orgAllBag = new CtrInstanceList(*allBag, ctorDummy);

      allBag->remove_conflicting_instances();
      numUniqAll = allBag->num_instances();
   }

   int numData = dataBag.num_instances();
   int numUniqData = (dataBag.remove_conflicting_instances(),
		      dataBag.num_instances());
   int numTest = testBag.num_instances();
   int numUniqTest = (testBag.remove_conflicting_instances(),
		      testBag.num_instances());
   if (numAll != -1) {
      if (numAll == numData + numTest)
	 Mcout << "Data + Test == All" << endl;
      else
	 Mcout << "*** Warning: Data + Test != All" << endl;
      Mcout << "Number of instances in .all = " << numAll <<
	 " (duplicate or conflicting instances : " << numAll - numUniqAll
	    << ")" << endl;
   }
      

   Mcout << "Number of instances in " << dataFile << " = "  << numData <<
      " (duplicate or conflicting instances : " << numData - numUniqData
	 << ")" << endl;
   Mcout << "Number of instances in .test = " << numTest <<
      " (duplicate or conflicting instances : " << numTest - numUniqTest
	 << ")" << endl;

   if (allBag)
      class_probability(".all file", *orgAllBag, numAll);
   else
      class_probability(".test file", orgTestBag, numTest);

   int cont = 0;
   int nomi = 0;
   for (int i = 0; i < dataBag.num_attr(); i++) {
      if (dataBag.attr_info(i).can_cast_to_nominal())
	 nomi++;
      else
	 cont++;
   }
   Mcout << "Number of attributes = " << dataBag.num_attr() <<
      " (continuous : " << cont << " nominal : " << nomi << ")"<< endl;

   if (get_option_bool("SHOW_ATTR_INFO", FALSE,
       "Iff SHOW_ATTR_INFO is on, it prints out attribute infos"))
       attribute_info(dataBag, allBag);
   return 0; // return success to shell
}   

