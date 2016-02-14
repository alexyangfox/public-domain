// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a utility for MLC++
// It is not part of the MLC++ library itself, but instead uses the
//   library to provide a useful function.

/***************************************************************************
  Description  : Uses a Discretizor to discretize the Instances in a bag
                   with Real Attribute values. Outputs the datafiles to
		   a user specified DUMPSTEM.
		   
  Usage        : Environment variables to set: 
                   DATAFILE -source datafile to be discretized
		   DUMPSTEM - stem of output file, followed by {.test,.names,
		              .data}
		   DISC_TYPE -type of discretization method to be used.
		              valid choices are "1r" or "1R" for OneR
			      discretization "bin" or "Bin" for
			      equal-interval binning discretization, or
			      "entropy" for entropy discretization.
		   MIN_INST -If the discretizor type (DISC_TYPE) is set to
		             "1r" or "1R" designates the minimum number of
			     instances per label.

		   MIN_SPLIT -If the discretizor type (DISC_TYPE) is set to
		              "entropy" designates the minimum split.

		   INITIAL_VAL -Designates the maximum number of bins the
		              discretization method should strive for. If
			      this option is set to zero, entropy and binning
			      will perform automatic discretization based
			      upon the criteria described below.
			      
		  If the .test file corresponding to the DATAFILE stem is
		    non-existent, a warning will be generated indicating
		    that this is the case.
		  Entropy uses the MDL criterion as a heuristic for the number
		    of bins when the initial value of 0
		  Binning uses k log N as a heuristic for the number of bins
		    when the initial value is zero.
			     
  Enhancements :   * Add other discretization methods as they become available.
                   * Allow the user to input a discretization vector.

  History      :  Ronny Kohavi                                    13/3/95
                    Allow .all files, project option
                  James Dougherty
		    Initial Revision                              11/21/94
***************************************************************************/

#include <basics.h>
#include <mlcIO.h>
#include <time.h>
#include <MLCStream.h>
#include <InstList.h>
#include <LogOptions.h>
#include <OneR.h>
#include <EntropyDisc.h>
#include <BinningDisc.h>
#include <GetOption.h>
#include <FileNames.h>
#include <DiscDispatch.h>


RCSID("MLC++, $RCSfile: u_discretize.c,v $ $Revision: 1.16 $")


/***************************************************************************
  Description : Dump training set, test set (if exists).
  Comments    : If there's no test set, we dump the data with a .all suffix.
***************************************************************************/

void dump_data_test(const MString& dumpStem, const MString& descr,
		  const InstanceBag* dataBag, const InstanceBag* testBag)
{
   ASSERT(dataBag != NULL);

   MString datafile;
   if (testBag != NULL)
      datafile = dumpStem + ".data";
   else
      datafile = dumpStem + ".all";
   Mcout << "Dumping data file " << datafile << endl;
   MLCOStream trainStream(datafile);
   trainStream << "|" << descr << endl << *dataBag << endl;

   
   //output testing data, if it exists (the test bag is optional)
   if (testBag) { 
      Mcout << "Dumping test file " << dumpStem << ".test" << endl;
      MLCOStream testStream(dumpStem + ".test");
      testStream << "|" << descr << endl << *testBag << endl;
   }
}


/***************************************************************************
  Description : Outputs the results of the discretization to the appropriately
                  named files. 
  Comments    :
***************************************************************************/
void dump_results(const MString& dumpStem, const MString& descr,
		  const Array<RealDiscretizor*>& discretizors,
		  const InstanceBag* discDataBag,
		  const InstanceBag* discTestBag )
{
   //Set up names stream
   MLCOStream namesStream(dumpStem + ".names");
   namesStream << "|" << descr << endl;
   MStreamOptions outputOptions = namesStream.get_options();
   namesStream.set_width(Mcout.wrap_width()); // set to Mcout's width
   namesStream.set_newline_prefix("|");
   namesStream.set_wrap_prefix("|");

   //output discretizors
   for(int i = 0; i < discretizors.size(); i++)
      if ( NULL != discretizors.index(i) )
	 namesStream << *discretizors.index(i);

   //output names
   Mcout << "Outputting names file " << dumpStem << ".names" << endl;
   namesStream.set_options(outputOptions);
   discDataBag->display_names(namesStream, TRUE, "MLC++ discretize utility");

   dump_data_test(dumpStem, descr, discDataBag, discTestBag);
}


/***************************************************************************
  Description : Performs discretization on the datafiles using the
                  dispatcher setup in main.
  Comments    :
***************************************************************************/
void discretize(const MString& dumpStem, FileNames& files, DiscDispatch& dp);

void disc_project(const MString& dumpStem, FileNames& files, DiscDispatch& dp,
		Bool projectOnly)
{
   if (!projectOnly)
      discretize(dumpStem, files, dp);
   else {
      // If initial val is not zero, only if there is one value
      //    will this do anything!
      if ( dp.get_initial_val() )
	 Mcout << "Warning: initial val set to "
	       << dp.get_initial_val() << "." << endl;

      // Create databag and discretizors
      InstanceList dataBag("", files.names_file(), files.data_file());
      dp.create_discretizors(dataBag);
      ASSERT(dp.discretizors() != NULL);
      BoolArray attrMask(0, dp.disc_vect()->size(), TRUE);

      for (int i = 0; i < attrMask.size(); i++) {
         if (dp.discretizors()->index(i))
	    attrMask[i] = dp.discretizors()->index(i)->
	       num_intervals_chosen() > 1;
      }

      Mcout << "Projection vector is: " << attrMask << endl;
      Mcout << "Projecting data file: " << files.data_file() << endl;
      InstanceBag* projDataBag = dataBag.project(attrMask);

      MLCOStream namesFile(dumpStem + ".names");
      projDataBag->display_names(namesFile, TRUE, "Projected from " +
          files.data_file() + " using discretize utility");

      InstanceBag* projTestBag = NULL;
      if (files.test_file(FALSE) != "" ){
	 InstanceList testBag("", files.names_file(), files.test_file());   
	 Mcout << "Projecting test file: " << files.test_file() << endl;
	 projTestBag = testBag.project(attrMask);
      }

   MString descr = "Projection from discretize util";
   dump_data_test(dumpStem, descr, projDataBag, projTestBag);
   
   delete projDataBag;
   delete projTestBag;
   }
}

void discretize(const MString& dumpStem, FileNames& files, DiscDispatch& dp)
{
   if ( dp.get_initial_val() )
      Mcout << "Performing using initial value of "
	    << dp.get_initial_val() << "." << endl;
   else
      Mcout << "Performing automatic discretization. " << endl;

   // Create databag and discretizors
   InstanceList dataBag("", files.names_file(), files.data_file());
   dp.create_discretizors(dataBag);

   //discretize the training set
   Mcout << "Discretizing data file: " << files.data_file() << endl;
   InstanceBag* discDataBag = dp.discretize_bag(dataBag);

   //discretize the test set, if it exists
   InstanceBag* discTestBag = NULL;

   if (files.test_file(FALSE) != "" ){
      InstanceList testBag("", files.names_file(), files.test_file());   
      Mcout << "Discretizing test file: " << files.test_file() << endl;
      discTestBag = dp.discretize_bag(testBag);
   }

   MString descr = "Discretization into bins of size "
                   + MString(dp.get_initial_val(),0);

   // Get our discretizors
   PtrArray<RealDiscretizor*>* brDisc = dp.disc_copy();

   // output the results   
   dump_results(dumpStem, descr, *brDisc, discDataBag, discTestBag);
   
   delete discDataBag;
   delete discTestBag;
   delete brDisc;
}


/***************************************************************************
  Description : Gets options from the user or environment, invokes a
                  discretization method through the dispatcher. 
  Comments    :
***************************************************************************/
main()
{
   Mcout << "MLC++ Discretize utility" << endl;
   FileNames files;   
   DiscDispatch dp;
   MString dumpStem = files.dump_stem();
   dp.set_user_options("");
   
   Bool projectOnly = 
      get_option_bool("PROJECT_ONLY", FALSE, "Only project out attributes "
		      "whose discretization vector is one.", TRUE);

   // Discretize using 1-Rule 
   if (dp.get_disc_type() == DiscDispatch::oneR){ 
      Mcout << "1-Rule discretization selected." << endl;
      if(dp.get_min_inst())
	 Mcout << "Using " << dp.get_min_inst()
	       << " minimum number of instances per label." << endl;
      disc_project(dumpStem, files, dp, projectOnly);
   } // Discretize via Binning
   else if(dp.get_disc_type() == DiscDispatch::binning) {
      Mcout << "Binning discretization selected." << endl;
      disc_project(dumpStem, files, dp, projectOnly);
   } // Discretize via entropy 
   else if(dp.get_disc_type() == DiscDispatch::entropy) {
      Mcout << "Entropy discretization selected." << endl;
      Mcout << "Using minsplit of " << dp.get_min_split() << endl;
      disc_project(dumpStem, files, dp, projectOnly);
   } // C4.5 discretization
   else if (dp.get_disc_type() == DiscDispatch::c45Disc) {
      Mcout << "C4.5 discretization selected." << endl;
      disc_project(dumpStem, files, dp, projectOnly);
   } // T2 discretization
   else if (dp.get_disc_type() == DiscDispatch::t2Disc) {
      Mcout << "T2 discretization selected." << endl;
      disc_project(dumpStem, files, dp, projectOnly);
   } else
     ASSERT(FALSE); //internal error

   Mcout << "Process complete. " << endl;
   return 0; // return success to shell
}   











