// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a utility for MLC++
// It is not part of the MLC++ library itself, but instead uses the
//   library to provide a useful function.

/***************************************************************************
  Description  : Generate a learning curve for use with mathematica.
  Usage        : Options to set:
                   DATAFILE sets the datafile name
                   NAMESFILE (optional)
                   INDUCER  (see env_inducer)
		   NUM_INTERVALS (optional, 10)
                   MIN_TEST_SIZE (optional, num-inst/num_intervals)
		   NUM_REPEATS   (optional, 5)
                   INDUCERNAME   (optional, INDUCER)
		   SEED          (optional, 7258789)
                   DUMPSTEM      (optional)
		   MATH_DISPLAY  (optional, no) output mathematica style info
  Enhancements :
  History      : Ronny Kohavi                                       1/17/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <env_inducer.h>
#include <LearnCurve.h>
#include <CtrInstList.h>
#include <StatData.h>
#include <GetOption.h>
#include <FileNames.h>

RCSID("MLC++, $RCSfile: u_LearnCurve.c,v $ $Revision: 1.15 $")

const MString numIntervalsHelp =
  "This option specifies the number of intervals (sample points) "
  "to display in the generated learning curve.";

const MString numRepeatsHelp =
  "This option specifies the number of times to run the inducer at "
  "each sample point in the learning curve.";

const MString minTestSizeHelp =
  "This option specifies the size of the test set to be used "
  "for the last sample point.";

const MString seedHelp =
  "This option specifies the random number generator seed used for "
  "all runs used to generate this learning curve.";

const MString mathDisplayHelp =
  "This option specifies whether or not to generate mathematica "
  "code to display the learning curve (use LearnCurve.m).";


main()
{
   FileNames files;

   MString dataFile = files.data_file();
   MString namesFile = files.names_file();
   MString dumpStem = files.dump_stem();

   int numIntervals = get_option_int("NUM_INTERVALS", 10,
				     numIntervalsHelp, FALSE);
   int numRepeats   = get_option_int("NUM_REPEATS", 5,
				     numRepeatsHelp, FALSE);
   int seed         = get_option_int("SEED", 7258789,
				     seedHelp, TRUE);
   int minTestSize  = get_option_int("MIN_TEST_SIZE", 0,
				     minTestSizeHelp, TRUE);

   // Don't show too much in Inducer
   int saveLogLevel = globalLogLevel;
   globalLogLevel = max(globalLogLevel-2, 0);
   BaseInducer *inducer = env_inducer();

   CtrInstanceList data("", namesFile, dataFile);
   int numInstances = data.num_instances();
   if (minTestSize == 0)
      minTestSize = int(Real(numInstances) / numIntervals + 0.5);
   
   Mcout << "Inducer: " << inducer->description()
         << ".  Intervals: " << numIntervals
         << ", Repeats: " << numRepeats
         << ", Min test size: " << minTestSize
         << ".  Seed: " << seed << endl;


   Mcout << "DATAFILE: " << dataFile << " (size=" << numInstances << ')'
         << endl;

   if (minTestSize >= numInstances)
      err << "MIN_TEST_SIZE : " << minTestSize << " >= number of instances "
	  << numInstances << fatal_error;

   numInstances -= minTestSize;
   Array<LearnCurveInfo> info(1, numIntervals-1);
   for (int i = 1; i <= info.high(); i++) {
      info[i].trainSize = (int)((Real(numInstances) * i) / 
                                                      (numIntervals-1) + 0.5);
      info[i].numTrainings = numRepeats;
   }
   LearnCurve lc(info, dumpStem);
   lc.set_log_level(saveLogLevel); // LC has higher level.


   lc.init_rand_num_gen(seed);
   lc.learn_curve(*inducer, data);

   if(get_option_bool("MATH_DISPLAY", FALSE, mathDisplayHelp, TRUE)) {
      MStreamOptions oldOpt = Mcout.get_options();
      Mcout          <<        "data={";
      Mcout.set_newline_prefix("      "); // same as data=
      Mcout.set_wrap_prefix(Mcout.get_newline_prefix() + "  ");
      for (i = info.low(); i <= info.high(); i++) {
	 Mcout << '{' << info[i].trainSize;
	 for (int j = 0; j < info[i].numTrainings; j++)
	    Mcout << ", " << lc.get_accuracy(i, j);
	 if (i < info.high())
	    Mcout << "}," << endl;
	 else
	    Mcout << '}';
      }
      Mcout << "}\n" << endl;
      Mcout.set_options(oldOpt);
   }

   Mcout << "Size,  Acc, std-dev of mean" << endl;
   Mcout.get_stream().precision(2);
   Mcout.get_stream().setf(ios::fixed);
   for (i = info.low(); i <= info.high(); i++) {
      StatData stats;
      for (int j = 0; j < info[i].numTrainings; j++)
	 stats.insert(lc.get_accuracy(i,j));

      Mcout << setw(6) << info[i].trainSize << ", " 
            << stats.mean()*100 << "% +- " << stats.std_dev_of_mean()*100 
	    << '%' << endl;
   }

   delete inducer;
   return 0; // return success to shell
}   
