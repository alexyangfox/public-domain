// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the methods of the CrossValidator class
                   and AccEstimator
                 The final C45 Inducer can be verified by running
		    C4.5 on t_SCValidator-0-0, and -0-1 and averaging.
  Doesn't test :
  Enhancements :
  History      : Ron Kohavi                                          10/2/94
                   Re-engineered the class
		   Richard Long                                       1/04/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <math.h>
#include <errorUnless.h>
#include <DblLinkList.h>
#include <Attribute.h>
#include <StratifiedCV.h>
#include <CtrInstList.h>
#include <ID3Inducer.h>
#include <C45Inducer.h>
#include <CatTestResult.h>

RCSID("MLC++, $RCSfile: t_StratifiedCV.c,v $ $Revision: 1.9 $")

const MString fileStem  = "/tmp/t_lensesSCV";
const MString namesFile = "t_SCValidator.names";
const MString dataFile  = "t_SCValidator.data";

const int NUM_FOLDS = 9;
const int NUM_TIMES = 2;
const int RAND_SEED = 7258789;

// Cleanup some leftovers which may cause ownership problems.
void cleanup()
{
   // Create a file so the asterisk won't fail.
   MLCOStream dummy(fileStem + "-dummy.names");
   dummy.close();

   // Without a shell this doesn't work because the asterisk isn't expanded
   // The input from /dev/null ensures yes to all questions
   system("csh -c \"rm " + fileStem + "-*.{names,data,test}\" < /dev/null");

   MLCOStream dummy1("t_SCValidator-0-X.names");
   dummy1.close();

   system("csh -c \"rm t_SCValidator-0-*.{names,data,test,tree,unpruned}\""
	  "< /dev/null");
}


main()
{
   Mcout << "t_StratifiedCV executing" << endl;
   cleanup();
   CtrInstanceList bag("", namesFile, dataFile);


   // Prepare Cross validation with logging going to .out1
   MLCOStream out1("t_StratifiedCV.out1");
   StratifiedCV crossValidator;
   crossValidator.set_folds(NUM_FOLDS);
   crossValidator.set_times(NUM_TIMES);
   crossValidator.set_log_level(2);
   crossValidator.set_log_stream(out1);
   crossValidator.init_rand_num_gen(RAND_SEED);

   TEST_ERROR("AccEstimator::check_acc_data: Must be called",
	      crossValidator.accuracy());

   // Dump the folds to the fileStem
   crossValidator.dump_files(bag, fileStem);
   out1 << "Finished dump" << endl;

   // Do the cross validation from the dumped files.
   StratifiedCV cv2;
   cv2.set_folds(NUM_FOLDS);
   cv2.set_times(NUM_TIMES);
   cv2.set_log_level(2);
   cv2.set_log_stream(out1);
   ID3Inducer id3Inducer("t_SCValidator id3 inducer");
   id3Inducer.set_log_level(max(globalLogLevel - 2, 0));
   cv2.init_rand_num_gen(RAND_SEED);
   Real acc2 = cv2.estimate_accuracy(id3Inducer, fileStem);
   ASSERT(acc2 == cv2.accuracy());
   Real trimAcc2 = cv2.accuracy(0.2);

   out1  << "ID3 categorizer cross validation from " << fileStem << " is "
         << cv2 << endl;
   Mcout << "ID3 categorizer cross validation from " << fileStem << " is "
         << cv2 << endl;

   // Now do the cross validation in memory.
   StratifiedCV cv(NUM_FOLDS, NUM_TIMES);
   cv.set_log_level(3);
   cv.set_log_stream(out1);
   cv.rand_num_gen().init(RAND_SEED);
   Real accCV = cv.estimate_accuracy(id3Inducer, bag);
   out1  << "ID3 cv in memory " << cv << endl;
   Mcout << "ID3 cv in memory " << cv << endl;   

   // Verify that the results from memory match the results from the dumped
   // files. 
   ASSERT(accCV == cv.accuracy());
   Real trimAccCV = cv.accuracy(0.2);
   ASSERT(trimAccCV == trimAcc2);
   ASSERT(cv.get_acc_data() == cv2.get_acc_data());
   out1 << "ID3 categorizer cross validation from bag: "
         << cv.get_acc_data() << endl;

   // Test for errors.
   cv.set_folds(bag.num_instances() + 1);
   TEST_ERROR("invalid for data with" ,
	      cv.estimate_accuracy(id3Inducer, bag));
   TEST_ERROR("AccEstimator::dump_files: empty filestem",
	      cv2.dump_files(bag, ""));
   cv.set_folds(-1000);
   TEST_ERROR("leave no training", cv.estimate_accuracy(id3Inducer, bag));

   InstanceList emptyList(bag.get_schema());
   cv.set_folds(2);
   TEST_ERROR("CrossValidator::estimate_accuracy: 0 instances in dataList",
	      cv.estimate_accuracy(id3Inducer, emptyList));
   TEST_ERROR("set_times: num times", cv.set_times(0));
   TEST_ERROR("set_folds: num folds", cv.set_folds(0));

   // Test the leave-one-out method
   cv.set_folds(-1);
   cv.set_times(1); 
   cv.estimate_accuracy(id3Inducer, bag);
   Mcout << "leave one out is " << cv << endl;


   // Try everything with a C45 inducer, so we know there's no problem with
   // external inducers.
   C45Inducer c45Inducer("my c45 inducer");
   StratifiedCV C45CV;
   C45CV.set_folds(2);
   C45CV.set_times(1);
   C45CV.set_log_level(3);
   C45CV.init_rand_num_gen(RAND_SEED);
   Real cvacc = Mround(C45CV.estimate_accuracy(c45Inducer, bag), 4);
   Mcout << "c4.5 accuracy is " << cvacc << endl;

   // Dump files.
   C45CV.init_rand_num_gen(RAND_SEED);
   C45CV.set_log_level(0);
   C45CV.dump_files(bag, "t_SCValidator");

   // Run C4.5 "manually," i.e., without using CValidator
   InstanceList train0("t_SCValidator-0-0");
   InstanceList test0 ("", "t_SCValidator-0-0.names","t_SCValidator-0-0.test");
   InstanceList train1("t_SCValidator-0-1");
   InstanceList test1 ("", "t_SCValidator-0-1.names","t_SCValidator-0-1.test");
   
   Real pruneAcc, noPruneAcc, estimateAcc;
   int noPruneSize, pruneSize;
   
   MString c45Pgm = C45Inducer::defaultPgmName + C45Inducer::defaultPgmFlags
	  + C45Inducer::defaultPgmSuffix;

   runC45(c45Pgm, train0, test0, pruneAcc, noPruneAcc, estimateAcc, 
	   noPruneSize, pruneSize);
   Real acc0 = pruneAcc;

   runC45(c45Pgm, train1, test1, pruneAcc, noPruneAcc, estimateAcc, 
	   noPruneSize, pruneSize);
   Real acc1 = pruneAcc;
   
   Real avgAcc = Mround((acc0 + acc1)/2,4);
   Mcout << "Running C4.5 manually yields " << acc0 << " and " << acc1 <<
      " with the average " << avgAcc << endl;

   ASSERT(fabs(avgAcc - cvacc) < REAL_EPSILON);

   cleanup();
   
   return 0; // return success to shell
}   
