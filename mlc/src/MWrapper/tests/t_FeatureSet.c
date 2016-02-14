// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test FeatureSet
  Doesn't test : The unsorted error from OK().  This should never be able
                   to happen using only public members.
  Enhancements :
  History      : Dan Sommerfield                                    4/22/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <FeatureSet.h>

RCSID("MLC++, $RCSfile: t_FeatureSet.c,v $ $Revision: 1.2 $")


// macro to help set up integer arrays for quick use in testing
#define MAKE_INT_ARRAY(mlc_array, c_array) \
  Array<int> mlc_array(sizeof(c_array)/sizeof(int));  \
{ \
     for(int i=0; i<mlc_array.size(); i++) \
	mlc_array[i] = c_array[i]; \
}

int t1[] = {1,3,5,7,9,12};
int t2[] = {3,5,7,9};
int t3[] = {1,3,5,6,7};
int t4[] = {1,12};

int b1[] = {3,3};
int b2[] = {-2, -1, 3, 6};
int b3[] = {3, 8, 2};

void test_add_feature()
{
   MAKE_INT_ARRAY(test1, t1);
   FeatureSet fSet(test1);
   Mcout << fSet;

   fSet.add_feature(6);
   fSet.add_feature(13);
   fSet.add_feature(0);
   Mcout << test1 << endl;
   fSet.OK();
}

void test_contains()
{
   MAKE_INT_ARRAY(test1, t1);
   MAKE_INT_ARRAY(test2, t2);
   MAKE_INT_ARRAY(test3, t3);
   FeatureSet fs1(test1);
   FeatureSet fs2(test2);
   FeatureSet fs3(test3);

   ASSERT(fs1.contains(5));
   ASSERT(!fs1.contains(22));
   ASSERT(!fs1.contains(2));
   ASSERT(fs1.contains(fs2));
   ASSERT(!fs1.contains(fs3));
}

void test_difference()
{
   MAKE_INT_ARRAY(test1,t1);
   MAKE_INT_ARRAY(test2,t2);
   MAKE_INT_ARRAY(test4,t4);

   FeatureSet fs1(test1);
   FeatureSet fs2(test2);
   FeatureSet fs4(test4);
   FeatureSet fsDiff;

   fs1.difference(fs2, fsDiff);
   Mcout << fs1 << " - " << fs2 << " = " << fsDiff << endl;
   Mcout << "expected: " << fs4 << endl;
   ASSERT(fsDiff == fs4);
}


int p1[] = {1,2,5,7,9};
int i1[] = {3,4,6,8};
int a1[] = {2,5};
void test_project_down()
{
   MAKE_INT_ARRAY(proj1, p1);
   MAKE_INT_ARRAY(inst1, i1);
   MAKE_INT_ARRAY(delta1, a1);

   Mcout << "proj1: " << proj1 << endl;
   Mcout << "inst1: " << inst1 << endl;
   Mcout << "delta1: " << delta1 << endl;

   FeatureSet oldProj(proj1);
   FeatureSet oldInst(inst1);
   FeatureSet delta(delta1);
   FeatureSet newProj;
   FeatureSet newInst;
   delta.project_down(oldProj, oldInst, newProj, newInst);
   
   Mcout << "new proj: " << newProj << endl;
   Mcout << "new inst: " << newInst << endl;
}


void test_assignment()
{
   MAKE_INT_ARRAY(proj1, t1);
   MAKE_INT_ARRAY(proj2, t2);
   FeatureSet f1(proj1);
   FeatureSet f2(proj1);
   FeatureSet f3(proj2);

   ASSERT(f3 != f1);
   f3 = f1;
   ASSERT(f3 == f1);
   ASSERT(f2 == f3);

   FeatureSet f4(f3, ctorDummy);
   ASSERT(f4 == f3);
}
   


void test_instance()
{
   InstanceList instList("monk1-full");
   FeatureSet fullSet(instList.get_schema());
   fullSet.display_names(instList.get_schema());
   Mcout << endl;

   FeatureSet smallSet;
   smallSet.add_feature(1);
   smallSet.add_feature(2);
   smallSet.display_names(instList.get_schema());
   Mcout << ": " << endl;
   for(Pix p=instList.first(); p; instList.next(p)) {
      Mcout << "  ";
      smallSet.display_instance(instList.get_instance(p));
      Mcout << endl;
   }

   FeatureSet diffSet;
   fullSet.difference(smallSet, diffSet);
   diffSet.display_names(instList.get_schema());
   Mcout << ": " << endl;
   for(p = instList.first(); p; instList.next(p)) {
      Mcout << "  ";
      diffSet.display_instance(instList.get_instance(p));
      Mcout << endl;
   }

   ASSERT(instList.num_instances() > 1);
   p = instList.first();
   const InstanceRC& inst1 = instList.get_instance(p);
   instList.next(p);
   const InstanceRC& inst2 = instList.get_instance(p);
   ASSERT(smallSet.instances_equal(inst1, inst2));
   ASSERT(!diffSet.instances_equal(inst1, inst2));

   FeatureSet nullSet;
   nullSet.display_instance(inst1);
}

void test_errors()
{
   MAKE_INT_ARRAY(test1, t1);
   FeatureSet f1(test1);

   // test bad attempts to use add_feature
   TEST_ERROR("FeatureSet::add_feature:", f1.add_feature(12));
   TEST_ERROR("FeatureSet::add_feature:", f1.add_feature(-1));
   
   MAKE_INT_ARRAY(test3, t3);
   FeatureSet f3(test3);

   // test illegal uses of difference
   FeatureSet diff;
   TEST_ERROR("FeatureSet::difference", f3.difference(f1, diff));
   DBG(TEST_ERROR("FeatureSet::difference", f1.difference(f3, diff)));

   // test bad construction
   //@@ does TEST_ERROR catch constructor errors??
   #ifdef foo
   DBG(
      MAKE_INT_ARRAY(bad1, b1);
      TEST_ERROR("FeatureSet::OK", FeatureSet fb1(bad1));
      MAKE_INT_ARRAY(bad2, b2);
      TEST_ERROR("FeatureSet::OK", FeatureSet fb2(bad2));
   );
   #endif

   // bad3 is just unsorted, so it should actually work
   MAKE_INT_ARRAY(bad3, b3);
   FeatureSet fb3(bad3);
   Mcout << fb3 << endl;
}


main()
{
   test_add_feature();
   test_contains();
   test_difference();
   test_project_down();
   test_assignment();
   test_instance();
   test_errors();
   
   return 0;
}

