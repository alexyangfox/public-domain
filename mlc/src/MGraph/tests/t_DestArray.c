// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test DestArray
  Doesn't test :
  Enhancements :
  History      : Ronny Kohavi                                       1/09/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <DestArray.h>
#include <MLCStream.h>

RCSID("MLC++, $RCSfile: t_DestArray.c,v $ $Revision: 1.8 $")

// Show a,b, 2 newlines
#define SHOW Mcout << "a:" << a << endl << "b:" << b << '\n' << endl;

main()
{
   cout << "t_DestArray executing" << endl;

   Array<NominalVal> a(UNKNOWN_CATEGORY_VAL, 3, UNKNOWN_CATEGORY_VAL);
   Array<NominalVal> b(UNKNOWN_CATEGORY_VAL, 3, UNKNOWN_CATEGORY_VAL);
   Array<NominalVal> c(UNKNOWN_CATEGORY_VAL, 3, UNKNOWN_CATEGORY_VAL);
   Array<Real> ac(UNKNOWN_CATEGORY_VAL, 3, 0);
   Array<Real> bc(UNKNOWN_CATEGORY_VAL, 3, 0);
   Array<Real> cc(UNKNOWN_CATEGORY_VAL, 3, 0);
   SHOW;

   // Both have all destinations as UNKNOWN

   ASSERT(DestArray::num_different_dests(a) == 0);
   ASSERT(DestArray::num_diff_dests_after_merge(a,b) == 0);
   ASSERT(DestArray::consistent_dests(a,b));
   ASSERT(DestArray::consistent_dests(b,a));
   ASSERT(DestArray::included_in_dest(a,b));   
   ASSERT(DestArray::included_in_dest(b,a));   
   ASSERT(a == b);
   ASSERT(DestArray::num_new_dests_from_merge(a,b) == 0);

   a.index(1) = FIRST_CATEGORY_VAL + 1;
   ac.index(1) = 1;
   ASSERT(DestArray::num_diff_dests_after_merge(a,b) == 1);
   ASSERT(ac.index(0) == 0);
   ASSERT(ac.index(1) == 1);
   ASSERT(ac.index(2) == 0);

   b.index(1) = FIRST_CATEGORY_VAL + 2;
   bc.index(1) = 1;
   ASSERT(DestArray::num_dests(a) == 1);
   ASSERT(DestArray::num_dests(b) == 1);
   ASSERT(DestArray::same_dest(a) == FIRST_CATEGORY_VAL + 1);
   ASSERT(DestArray::same_dest(b) == FIRST_CATEGORY_VAL + 2);

   ASSERT(DestArray::num_different_dests(a) == 1);
   // Now they conflict.
   ASSERT(!DestArray::consistent_dests(a,b));
   ASSERT(!DestArray::consistent_dests(b,a));
   ASSERT(!DestArray::included_in_dest(a,b));   
   ASSERT(!DestArray::included_in_dest(b,a));   

   b.index(1) = FIRST_CATEGORY_VAL + 1;
   // Now they are the same again.
   ASSERT(DestArray::num_diff_dests_after_merge(a,b) == 1);
   ASSERT(ac.index(0) == 0);
   ASSERT(ac.index(1) == 1);
   ASSERT(ac.index(2) == 0);

   ASSERT(DestArray::consistent_dests(a,b));
   ASSERT(DestArray::included_in_dest(a,b));   
   ASSERT(DestArray::merge_dests(a,ac,b,bc) == 0);
   ASSERT(ac.index(0) == 0);
   ASSERT(ac.index(1) == 2);
   ASSERT(ac.index(2) == 0);
   ASSERT(a == b);
   ASSERT(DestArray::num_new_dests_from_merge(a,b) == 0);
   ASSERT(ac.index(0) == 0);
   ASSERT(ac.index(1) == 2);
   ASSERT(ac.index(2) == 0);


   a.index(2) = FIRST_CATEGORY_VAL + 2;
   ac.index(2) = 1;
   ASSERT(DestArray::num_diff_dests_after_merge(a,b) == 2);
   ASSERT(DestArray::num_dests(a) == 2);
   ASSERT(DestArray::same_dest(a) == UNKNOWN_CATEGORY_VAL);
   ASSERT(DestArray::num_different_dests(a) == 2);
   SHOW;
   ASSERT(DestArray::consistent_dests(a,b));
   ASSERT(DestArray::included_in_dest(b,a));   
   ASSERT(DestArray::num_new_dests_from_merge(a,b) == 0);
   ASSERT(DestArray::num_new_dests_from_merge(b,a) == 1);
   c = a;
   ASSERT(DestArray::merge_dests(a,ac,b,bc) == 0);
   ASSERT(a == c);
   ASSERT(DestArray::merge_dests(b,bc,a,ac) == 1);
   ASSERT(b == c);
   
   a.index(0) = FIRST_CATEGORY_VAL + 2;
   ASSERT(DestArray::num_diff_dests_after_merge(a,b) == 2);
   ASSERT(DestArray::num_different_dests(a) == 2);
   SHOW;
   ASSERT(!DestArray::included_in_dest(a,b));      

   Array<NominalVal> d(UNKNOWN_CATEGORY_VAL, 4, UNKNOWN_CATEGORY_VAL);
   Array<NominalVal> e(UNKNOWN_CATEGORY_VAL, 4, UNKNOWN_CATEGORY_VAL);

   d.index(1) = FIRST_CATEGORY_VAL;
   d.index(2) = FIRST_CATEGORY_VAL + 1;
   e.index(3) = FIRST_CATEGORY_VAL + 2;

   ASSERT(DestArray::num_dests(d) == 2);
   ASSERT(DestArray::num_dests(e) == 1);
   ASSERT(DestArray::num_new_dests_from_merge(d,e) == 1);
   ASSERT(d.index(1) == 0);
   ASSERT(d.index(2) == 1);
   ASSERT(d.index(3) == UNKNOWN_CATEGORY_VAL);
   return 0; // return success to shell
}   
