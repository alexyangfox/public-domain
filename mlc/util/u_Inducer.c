// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a utility for MLC++
// It is not part of the MLC++ library itself, but instead uses the
//   library to provide a useful function.

/***************************************************************************
  Description  : Run a given inducer.
  Usage        : Environment variables to set:
                   DATAFILE
                   INDUCER
  Enhancements :
  History      : Ronny Kohavi                                       5/29/94
***************************************************************************/

#include <basics.h>
#include <Inducer.h>
#include <env_inducer.h>
#include <CatTestResult.h>
#include <CtrInstList.h>
#include <math.h>
#include <GetOption.h>
#include <FileNames.h>

static MString dispStructHelp("Display the resulting tree using dotty");
enum DisplayTypeEnum {noDisp, asciiDisp, dotDisp, dottyDisp, treeViz};
static MEnum displayType = 
  MEnum("none", noDisp) << 
  MEnum("ASCII", asciiDisp) <<
  MEnum("dot", dotDisp) <<
  MEnum("dotty", dottyDisp) <<
  MEnum("TreeViz", treeViz); // @@ should support treeviz only for trees.

RCSID("MLC++, $RCSfile: u_Inducer.c,v $ $Revision: 1.17 $")

const int ACC_PRECISION = 2;

main()
{
   BaseInducer *baseInducer = env_inducer();
   FileNames files;

   if (baseInducer->can_cast_to_inducer()) {
      Inducer& inducer = baseInducer->cast_to_inducer();
      inducer.read_data("", files.names_file(), files.data_file());
      inducer.train();
      CatTestResult result(inducer.get_categorizer(),
			   inducer.instance_bag(), "", files.test_file());
      Mcout << result << endl;

      DisplayTypeEnum disp = get_option_enum("DISPLAY_STRUCT", 
					     displayType, noDisp, 
					     dispStructHelp, TRUE);
      if (disp == asciiDisp)
	 inducer.display_struct();
      else if (disp == dotDisp || disp == dottyDisp) {
         MString outputDotfile = get_option_string("INDUCER_DOT",
            "Inducer.dot", "Dot file to dump Inducer's structure into",TRUE);
         MLCOStream out(outputDotfile);
	 DotGraphPref pref;
	 inducer.display_struct(out,pref);
	 out.close();  // when killing dotty, this file isn't closed properly.
         if (disp == dottyDisp) {
	    MLCOStream out1(XStream);
	    inducer.display_struct(out1,pref);
	 }
      } else if (disp == treeViz) {
         MString outputTVizfile = get_option_string("INDUCER_TREEVIZ",
            "Inducer", "File name to dump TreeViz config file", TRUE);
         outputTVizfile += ".treeviz";
         MLCOStream out(outputTVizfile);
	 TreeVizPref pref;
	 inducer.display_struct(out, pref);
	 MLCOStream out1(XStream);
	 inducer.display_struct(out1, pref);
      } else if (disp != noDisp)
	 err << "u_Inducer.c: Unexpected display type" << fatal_error;
   } else {
      CtrInstanceList trainSet("", files.names_file(), files.data_file());
      if (files.test_file() == "")
	 err << "Inducer: must have a test file" << fatal_error;
      
      CtrInstanceList testSet("", files.names_file(), files.test_file());

      // Don't Mcout "accuracy " << train_and_test because when log level is
      //   on, you'll see the word accuracy a year before the number.
      Real acc = baseInducer->train_and_test(&trainSet, testSet);
      Real confLow, confHigh;
      CatTestResult::confidence(confLow, confHigh, acc,
				testSet.num_instances());
      Mcout << "Accuracy: " << MString(acc*100,ACC_PRECISION) << "% +- "
	    << MString(CatTestResult::theoretical_std_dev(
	       acc, testSet.num_instances())*100, ACC_PRECISION) << "% ["
	    << MString(confLow*100, ACC_PRECISION) << "% - " <<
	       MString(confHigh*100, ACC_PRECISION) << "%]" << endl;
      
   }

   delete baseInducer;
   return 0; // return success to shell
}   
