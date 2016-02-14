// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the methods of the Bootstrap class

  Doesn't test :
  Enhancements :
  History      : Developed from t_XValidator.c                    10/6/94
                   Dan Sommerfield

***************************************************************************/
#include <basics.h>
#include <math.h>
#include <errorUnless.h>
#include <DblLinkList.h>
#include <Attribute.h>
#include <Bootstrap.h>
#include <CtrInstList.h>
#include <ID3Inducer.h>
#include <C45Inducer.h>
#include <CatTestResult.h>

RCSID("MLC++, $RCSfile: t_Bootstrap.c,v $ $Revision: 1.10 $")

const MString fileStem  = "/tmp/t_lensesBS";
const MString namesFile = "t_Bootstrap.names";
const MString dataFile  = "t_Bootstrap.data";
const MString bsStem = "t_Bootstrap";

const int NUM_TIMES = 20;
const int RAND_SEED = 7258789;

// this is the cleanup function from t_XValidator.c.  It cleans up old files
// to avoid file ownership problems.
void cleanup()
{
   // Create a file so the asterisk won't fail.
   MLCOStream dummy(bsStem + "-dummy.names");
   dummy.close();

   // Without a shell this doesn't work because the asterisk isn't expanded
   // The input from /dev/null ensures yes to all questions
   // Note that this call DOES expand to -A-*.names, etc as well, because
   // -* can expand to -A-*.
   system("csh -c \"rm " + bsStem + "-*.{names,data,test}\" < /dev/null");

   // repeat the cleanup with the other filestem
   MLCOStream dummy2(fileStem + "-dummy.names");
   dummy2.close();
   system("csh -c \"rm " + fileStem + "-*.{names,data,test}\" < /dev/null");
   
}


main()
{
  Mcout << "t_Bootstrap executing." << endl;
  cleanup();

  const int num_bs_types = 3;
  for(int i=0; i<num_bs_types; i++) {
     Real bsFract;
    
    // based on the loop index, select a bootstrap type.
    Bootstrap::BootstrapType bsType;
    switch(i) {
    case 0:
      bsType = Bootstrap::simple;
      bsFract = 0.632; break;
    case 1: 
      bsType = Bootstrap::fractional;
      bsFract = 0.632; break;
    case 2: 
      bsType = Bootstrap::fractional;
      bsFract = 0.5; break;
    default:
       ASSERT(FALSE);
    }
    Mcout << "t_Bootstrap: type = " << bsType << endl;

    CtrInstanceList bag("", namesFile, dataFile);
    MString outName = "t_Bootstrap.out" + MString(i+1, 1);
    MLCOStream out1(outName);
    Bootstrap bootstrap;

    #ifndef MEMCHECK
    TEST_ERROR("Bootstrap::set_times: illegal number",
	       bootstrap.set_times(-1));
    TEST_ERROR("Bootstrap::set_type: illegal type",
	       bootstrap.set_type((Bootstrap::BootstrapType)666));
    #endif

    bootstrap.set_type(bsType);
    bootstrap.set_times(NUM_TIMES);
    if(bsType == Bootstrap::fractional)
       bootstrap.set_fraction(bsFract);

    bootstrap.set_log_level(2);
    bootstrap.set_log_stream(out1);
    bootstrap.init_rand_num_gen(RAND_SEED);

    #ifndef MEMCHECK
    TEST_ERROR("AccEstimator::check_acc_data: Must be called",
	       bootstrap.accuracy());
    TEST_ERROR("AccEstimator::check_acc_data: Must be called",
	       bootstrap.accuracy_std_dev());
    #endif
    
    Mcout << bootstrap << endl;
    out1 << bootstrap << endl;

    bootstrap.dump_files(bag, fileStem);
    out1 << "Finished dump" << endl;

    Bootstrap bs2(NUM_TIMES, bsType, bsFract);
    bs2.set_log_level(2);
    bs2.set_log_stream(out1);
    ID3Inducer id3Inducer("t_Bootstrap id3 inducer");
    bs2.init_rand_num_gen(RAND_SEED);
    Real acc2 = bs2.estimate_accuracy(id3Inducer, fileStem);
    ASSERT(acc2 == bs2.accuracy());

    out1  << "ID3 categorizer bootstrap from " << fileStem;
    bs2.display_acc_data(out1);
    out1 << endl;
    Mcout << "ID3 categorizer bootstrap from "  << fileStem;
    bs2.display_acc_data(Mcout);
    Mcout << endl;

    Bootstrap bs(NUM_TIMES, bsType, bsFract);
    bs.set_log_level(3);
    bs.set_log_stream(out1);
    bs.rand_num_gen().init(RAND_SEED);
    Real accBS = bs.estimate_accuracy(id3Inducer, bag);
    out1  << "ID3 bs in memory " << bs << endl;
    Mcout << "ID3 bs in memory " << bs << endl;   

    ASSERT(accBS == bs.accuracy());
    ASSERT(accBS == acc2);

    out1 << "ID3 categorizer bootstrap from bag: " << endl << bs << endl;

  }

   // Extra bootstrap tests.  These will only be done on the default
   // (.632) bootstrap.
    Mcout << "Extra tests:" << endl;
    CtrInstanceList bag("", namesFile, dataFile);
    MLCOStream out1("t_Bootstrap.out");
    
    Bootstrap bs(NUM_TIMES, Bootstrap::fractional);
    bs.set_log_level(3);
    bs.set_log_stream(out1);
    bs.rand_num_gen().init(RAND_SEED);

    ID3Inducer id3Inducer("t_Bootstrap id3 inducer");

    // Test for errors.
    bs.set_times(1);
    bs.estimate_accuracy(id3Inducer, bag);
    bs.set_fraction(0.5);
  
    #ifndef MEMCHECK
    TEST_ERROR("Bootstrap::set_fraction: Illegal value",
	       bs.set_fraction(-0.5));
    TEST_ERROR("Bootstrap::set_fraction: Illegal value",
	       bs.set_fraction(1.5));
    #endif

    // Try everything with a C45 inducer, so we know there's no problem with
    // external inducers.
    cleanup();
    C45Inducer c45Inducer("my c45 inducer");
    Bootstrap C45BS;
    C45BS.set_times(2);
    C45BS.set_log_level(3);
    C45BS.init_rand_num_gen(RAND_SEED);
    Real bsacc = Mround(C45BS.estimate_accuracy(c45Inducer, bag), 4);
    Mcout << "c4.5 accuracy is " << bsacc << endl;

    // Dump files.
    C45BS.init_rand_num_gen(RAND_SEED);
    C45BS.set_log_level(0);
    C45BS.dump_files(bag, "t_Bootstrap");

    // Run C4.5 "manually," i.e., without using Bootstrap
    InstanceList train0("t_Bootstrap-0");
    InstanceList test0 ("", "t_Bootstrap-0.names", "t_Bootstrap-0.test");
    InstanceList train1("t_Bootstrap-1");
    InstanceList test1 ("", "t_Bootstrap-1.names", "t_Bootstrap-1.test");
    InstanceList train2("t_Bootstrap-A");
    InstanceList test2 ("", "t_Bootstrap-A.names", "t_Bootstrap-A.test");
   
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
   
    runC45(c45Pgm, train2, test2, pruneAcc, noPruneAcc, estimateAcc, 
	   noPruneSize, pruneSize);
    Real accApp = pruneAcc;

    Real avgAcc = Mround((acc0 + acc1) / 2 * .632 + accApp*.368, 4);
    Mcout << "Running C4.5 manually yields " << acc0 << ", " << acc1 <<
       " and apparent " << accApp << " with the average " << avgAcc << endl;

    ASSERT(fabs(avgAcc - bsacc) < REAL_EPSILON);
      
    return 0; // return success to shell
  }   




    


