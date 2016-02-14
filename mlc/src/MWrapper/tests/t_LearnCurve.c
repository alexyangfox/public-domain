// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests learn_curve() with the ID3 Inducer using the
                    lenses data files.
		 The training sizes are set for 10% increments of the
		    size of the read list.  The inducer is trained 5
		    times at each level.
  Doesn't test : 
  Enhancements :
  History      : Richard Long                                       1/14/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <LearnCurve.h>
#include <CtrInstList.h>
#include <ID3Inducer.h>

RCSID("MLC++, $RCSfile: t_LearnCurve.c,v $ $Revision: 1.10 $")

int numIntervals = 10;

// Cleanup some leftovers which may cause ownership problems.
void cleanup()
{
   // Create a file so the asterisk won't fail.
   MLCOStream dummy("t_LearnCurve-A.data");
   dummy.close();

   // Without a shell this doesn't work because the asterisk isn't expanded
   // The input from /dev/null ensures yes to all questions
   system(MString("csh -c \"rm ") + "t_LearnCurve-*.data < /dev/null");
}

main()
{
   Mcout << "t_LearnCurve executing" << endl;
   cleanup();

   CtrInstanceList bag("t_LearnCurve");  // lenses
   ID3Inducer id3Inducer("t_LearnCurve ID3 Inducer");
   Array<LearnCurveInfo> info(1, numIntervals - 1);
   for (int i = 1; i < numIntervals ; i++) {
      info[i].trainSize = (bag.num_instances() * i) / numIntervals;
      info[i].numTrainings = 5;
   }

   MLCOStream out1("t_LearnCurve.out1");

   LearnCurve lc(info);
   lc.set_file_stem("t_LearnCurve");
   lc.init_rand_num_gen(7258789);
   lc.set_log_level(2);
   lc.set_log_stream(out1);
   
   lc.learn_curve(id3Inducer, bag);
   Mcout << lc;
   
#ifndef MEMCHECK
   lc.set_log_level(0);
   info[1].trainSize = -1;
   TEST_ERROR(" must be a positive number", LearnCurve lc2(info));
   info[1].trainSize = bag.num_instances() + 1;
   LearnCurve lc3(info);
   TEST_ERROR(" is >= than the bag size",
	      lc3.learn_curve(id3Inducer, bag));
   info[1].numTrainings = -1;
   info[1].trainSize = 1;
   TEST_ERROR("LearnCurve::LearnCurve: number of trainings (",
	      LearnCurve lc4(info));
   Array<LearnCurveInfo> info0(0);
   TEST_ERROR("LearnCurve::LearnCurve: Info array is of size 0",
	      LearnCurve lc5(info0));
#endif

   cleanup();
   return 0; // return success to shell
}   
