// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : Tests PeblsInducer with various options on monk1-full and
                   crx.
  Comments     :
  Enhancements :
  History      : Yeogirl Yun                                 June 13, 1995
                   Initial revision
***************************************************************************/


#include <basics.h>
#include <PeblsInducer.h>
#include <BagSet.h>
#include <InstList.h>

const MString DATAFILE1 = "monk1-full";
const MString DATAFILE2 = "crx";

// because of randomness in Pebls program, we cannot assert an exact
// accuracy. The followings are mean accuracies of five runs for each
// option used in the tests in main function.
const Real ACC1 = 0.912;
const Real ACC2 = 0.912;
const Real ACC3 = 0.861;
const Real ACC4 = 0.87;
const Real ACC5 = 0.861;
const Real ACC6 = 0.87;
const Real ACC7 = 0.815;
const Real ACC8 = 0.8118;
const Real ACC9 = 0.84;
const Real ACC10 = 0.83;
const Real ACC11 = 0.819;
const Real ACC12 = 0.819;

const Real DEVIATION = 0.05;

Bool warn = FALSE;

void test_pebls(const MString& dataFile,
		int numNeighbors,
		int discLevels,
		PeblsVotingScheme voteScheme,
		Real expAcc)
{
   Real sumAcc = 0;
   for (int i = 0; i < 5; i++) {
      PeblsInducer pebls("Pebls Inducer");

      InstanceList trainSet(dataFile);
      trainSet.remove_inst_with_unknown_attr();
      InstanceList testSet("", dataFile + ".names", dataFile + ".test");
      testSet.remove_inst_with_unknown_attr();
   
      // set options.
      pebls.set_nearest_neighbors(numNeighbors);
      pebls.set_discretization_levels(discLevels);
      pebls.set_voting_scheme(voteScheme);      

      Real acc = pebls.train_and_test(&trainSet, testSet);

      sumAcc += acc;
   }
   Real acc = sumAcc/5;
   Mcout << "DATAFILE : " << dataFile << endl;
   Mcout << "NUM NEIGHBORS : " << numNeighbors << endl;
   Mcout << "DISCRETIZATION LEVELS : " << discLevels << endl;
   Mcout << "VOTING SCHEME : ";
   if (voteScheme == MAJORITY)
      Mcout << "MAJORITY" << endl;
   else if (voteScheme == WEIGHTED_DISTANCE)
      Mcout << "WEIGHTED_DISTANCE" << endl;
   else
      err << "t_PeblsInducer.c:: unknown PeblsVotingScheme : " << voteScheme
	  << fatal_error;

   Mcout << "Expected accuracy : " << expAcc << endl;
   if (acc < expAcc - DEVIATION ||
       acc > expAcc + DEVIATION) {
      warn = TRUE;
      Mcerr << "t_PeblsInducer.c :: the estimated accuracy is much different "
	 "than expected. " << endl <<
	 "The expected value : " << expAcc << endl << 
	 "The actual value : " << acc << endl;
   }
}



int main()
{
   Mcout << "testing t_PeblsInducer.c" << endl;

   test_pebls(DATAFILE1, 1, 10, MAJORITY, ACC1);
   test_pebls(DATAFILE1, 1, 10, WEIGHTED_DISTANCE, ACC2);
   test_pebls(DATAFILE1, 3, 10, MAJORITY, ACC3);
   test_pebls(DATAFILE1, 3, 10, WEIGHTED_DISTANCE, ACC4);
   test_pebls(DATAFILE1, 3, 20, MAJORITY, ACC5);
   test_pebls(DATAFILE1, 3, 20, WEIGHTED_DISTANCE, ACC6);

   test_pebls(DATAFILE2, 1, 10, MAJORITY, ACC7);
   test_pebls(DATAFILE2, 1, 10, WEIGHTED_DISTANCE, ACC8);
   test_pebls(DATAFILE2, 3, 10, MAJORITY, ACC9);
   test_pebls(DATAFILE2, 3, 10, WEIGHTED_DISTANCE, ACC10);
   test_pebls(DATAFILE2, 3, 20, MAJORITY, ACC11);
   test_pebls(DATAFILE2, 3, 20, WEIGHTED_DISTANCE, ACC12);            
   
   if (!warn)
      Mcout << "Success!" << endl;
   return warn;
}

