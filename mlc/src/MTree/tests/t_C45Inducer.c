// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test C4.5Inducer and interface functions.
  Doesn't test :
  Enhancements : The awk script is a bit awkward.  We might want to parse in
                    C++ to avoid relying on this directory structure.
  History      : Ronny Kohavi                                       8/24/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <InstList.h>
#include <C45Inducer.h>

RCSID("MLC++, $RCSfile: t_C45Inducer.c,v $ $Revision: 1.3 $")


main()
{
   Mcout << "t_C45Inducer executing" << endl;

   InstanceList train("monk1-full");
   InstanceList test("monk1-full", ".names", ".test");

   Real pruneAcc, noPruneAcc, estimateAcc;
   int noPruneSize, pruneSize;

   runC45(C45Inducer::defaultPgmName + C45Inducer::defaultPgmFlags
	  + C45Inducer::defaultPgmSuffix,
	  train, test, pruneAcc, noPruneAcc, estimateAcc, 
	   noPruneSize, pruneSize);

   Mcout << "C4.5 run yields accuracy " << pruneAcc << " (pruned), "
         << noPruneAcc << " (unpruned)\n"
         << "     size " << pruneSize << " (pruned), "
         << noPruneSize << " (unpruned)" << endl;

   C45Inducer ind1("My C4.5 inducer");
   C45Inducer ind2("My C4.5 inducer", C45Inducer::defaultPgmFlags, FALSE);
   Real acc1 = ind1.train_and_test_files("monk1-full");
   Real acc2 = ind2.train_and_test_files("monk1-full");

   ASSERT(pruneAcc = acc1);
   ASSERT(noPruneAcc = acc2);
   
   return 0; // return success to shell
}   
