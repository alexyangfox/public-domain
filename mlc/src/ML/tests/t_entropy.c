// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test the entropy functions.
  Doesn't test :
  Enhancements :
  History      : Ronny Kohavi                                       9/05/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <entropy.h>
#include <CtrInstList.h>
#include <AttrEqCat.h>
#include <math.h>

RCSID("MLC++, $RCSfile: t_entropy.c,v $ $Revision: 1.16 $")

// approx aborts if val1 and val2 aren't close approximations (3 digits
//   after the decimal point should be the same).
// Quinlan gives 3 digits in his C4.5 book so that's what we use.

static void approx(double val1, double val2)
{
   if (fabs(val1 - val2) > 0.001)
      err << "t_entropy::approx failed.  val1=" << val1 << " val2=" <<
         val2 << fatal_error;
}   

static void check_weighted_entropy(LogOptions opt,
				   const CtrInstanceList& ctrList)
{
   // Weighted by itself, the bag should give log_k = log_2 = 1 for our data
   // set. 
   Real ent = weighted_entropy(opt, ctrList, ctrList);
   approx(ent, 1); 

   Real gini = weighted_gini(opt, ctrList, ctrList);
   approx(gini, 0.5);  // 1-.5^2-.5^2


   ent = entropy(opt, ctrList);
   Array<int> lblCounts(ctrList.counters().label_counts().low(), 
			ctrList.counters().label_counts().size(), 1);
   Real ent2 = weighted_entropy(opt, ctrList.counters().label_counts(),
			   lblCounts, ctrList.num_instances(),
			   lblCounts.size());
   approx(ent, ent2); // should be equal since we weighted by 1.
}

static void check_pessimistic_entropy(LogOptions opt,
				      const CtrInstanceList& bag)

{
   AttrEqCategorizer ac(bag.get_schema(), 0, FIRST_NOMINAL_VAL,
			"Attribute Equal Categorizer", FALSE);
   CtrBagPtrArray& bagArray = *(bag.ctr_split(ac));
   const CtrInstanceBag& subBag = *bagArray[FIRST_CATEGORY_VAL];

   Mcout << "Bag is " << subBag << endl;
   Real ent = weighted_entropy(opt, subBag, bag, 0);
   Real ent1 = weighted_entropy(opt, subBag, bag, 0.1);
   Real ent2 = weighted_entropy(opt, subBag, bag, .25);
   Real ent3 = weighted_entropy(opt, subBag, bag, 1.96);

   Mcout << "Entropies are " << ent << ", " << ent1 << ", " << ent2
         << ", " << ent3 << endl;

   ASSERT(ent <= ent1);
   ASSERT(ent1 <= ent2);
   ASSERT(ent2 <= ent3);
   delete &bagArray;
}
      
   


/*****************************************************************************
  Description : Returns 2-way conditional entropy using simple method.
  Comments    :
*****************************************************************************/

Real slow_way_cond_entropy(const LogOptions& opt,
		       const CtrInstanceBag& bag,
		       int attrNum,
		       NominalVal val)
{
   // calculate mutualInfoRate.
   AttrEqCategorizer ac(bag.get_schema(), attrNum, val,
			"Attribute Equal Categorizer", FALSE);
   CtrBagPtrArray& bagArray = *(bag.ctr_split(ac));
   const CtrInstanceBag& subBag1 = *bagArray[FIRST_CATEGORY_VAL];
   const CtrInstanceBag& subBag2 = *bagArray[FIRST_CATEGORY_VAL + 1];

   Real ent1 = 0;
   if (!subBag1.no_instances())
      ent1 = entropy(opt, subBag1) * (Real)subBag1.num_instances();
   Real ent2 = 0;
   if (!subBag2.no_instances())
      ent2 = entropy(opt, subBag2) * (Real)subBag2.num_instances();

   delete &bagArray; 

   return (ent1 + ent2) / bag.num_instances();

}



void test_2way_cond_entropy(const LogOptions& opt,
			    const CtrInstanceBag& bag)
{
   for (int i = bag.get_schema().num_attr(); --i >= 0; ) {
      const NominalAttrInfo& nai = bag.attr_info(i).cast_to_nominal();
      for (int j = UNKNOWN_NOMINAL_VAL; j < nai.num_values(); j++) {
         Real twce  = cond_entropy(opt, bag, i);
         Real twce1 = slow_way_cond_entropy(opt, bag, i, j);
         Real twce2 = two_way_cond_entropy(opt, bag, i, j);
	 if (i == 3 && j != UNKNOWN_NOMINAL_VAL) {
	    ASSERT(fabs(twce  - twce1) < REAL_EPSILON);
	    ASSERT(fabs(twce  - twce2) < REAL_EPSILON);
	 }
	 ASSERT(fabs(twce1 - twce2) < REAL_EPSILON);
      }
   }
}




main()
{
   Mcout << "t_entropy executing" << endl;

   CtrInstanceList ctrList("t_entropy"); // Golf database.
   const LogOptions opt;
   opt.set_log_level(0);

   test_2way_cond_entropy(opt, ctrList);


   approx(entropy(opt, ctrList), 0.940);
   approx(entropy(opt, ctrList.counters().label_counts(),
                  ctrList.num_instances()),
          0.940);
   approx(cond_entropy(opt, ctrList, 0), 0.694);
   approx(cond_entropy(opt, *ctrList.counters().value_counts()[0],
                       *ctrList.counters().attr_counts()[0],
                       ctrList.num_instances()),
          0.694);
   approx(mutual_info(opt, ctrList, 0), 0.246);

   // Check j-measure, which should equal sum of entropy
   Real j = 0;
   for (NominalVal x = FIRST_NOMINAL_VAL; x < FIRST_NOMINAL_VAL+3; x++)
      j += j_measure(opt, ctrList, 0, x);
   approx(j,0.246); 

   approx(cond_entropy(opt, ctrList, 3), 0.892);
   approx(cond_entropy(opt, *ctrList.counters().value_counts()[3],
                       *ctrList.counters().attr_counts()[3],
                       ctrList.num_instances()), 0.892);
   approx(mutual_info(opt, ctrList, 3), 0.048);
   
   j = 0;
   for (x = FIRST_NOMINAL_VAL; x < FIRST_NOMINAL_VAL+2; x++)
      j += j_measure(opt, ctrList, 3, x);
   approx(j, 0.048); 


   // Test real_mutual_information().
   CtrInstanceList ctrList2("t_entropy-real"); // database with real attr
   Real threshold, mutualInfo;

   int minSplit ;

   minSplit = min_split(opt, ctrList2, 1, 25, 0);


   ASSERT(real_cond_entropy(opt,
			    ctrList2, 0, threshold, mutualInfo, minSplit));
   approx(threshold, 6.5);
   approx(mutualInfo, 0.570759);
   
   ASSERT(real_cond_entropy(opt,
			    ctrList2, 1, threshold, mutualInfo, minSplit));
   approx(threshold, 2.5);
   approx(mutualInfo, 0.345084);

   ASSERT(real_cond_entropy(opt,
			    ctrList2, 2, threshold, mutualInfo, minSplit));
   approx(threshold, 2.5);
   approx(mutualInfo, 0.364977);

   ASSERT(real_cond_entropy(opt,
			    ctrList2, 3, threshold, mutualInfo, minSplit));
   approx(threshold, 3.5);
   approx(mutualInfo, 0.558121);

   ASSERT(real_cond_entropy(opt,
			    ctrList2, 4, threshold, mutualInfo, minSplit));
   approx(threshold, 2.5);
   approx(mutualInfo, 0.439816);

   ASSERT(real_cond_entropy(opt,
			    ctrList2, 5, threshold, mutualInfo, minSplit));
   approx(threshold, 2.5);
   approx(mutualInfo, 0.413765);
   
   ASSERT(real_cond_entropy(opt,
			    ctrList2, 6, threshold, mutualInfo, minSplit));
   approx(threshold, 3.5);
   approx(mutualInfo, 0.443692);
   
   ASSERT(real_cond_entropy(opt,
			    ctrList2, 7, threshold, mutualInfo, minSplit));
   approx(threshold, 2.5);
   approx(mutualInfo, 0.476764);
   
   ASSERT(real_cond_entropy(opt,
			    ctrList2, 8, threshold, mutualInfo, minSplit));
   approx(threshold, 1.5);
   approx(mutualInfo, 0.734604);

   check_weighted_entropy(opt, ctrList);
   check_pessimistic_entropy(opt, ctrList);

   return 0; // return success to shell
}   

