// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the Threshold Categorizer.
  Doesn't test :
  Enhancements :
  History      : Brian Frasca                                       4/12/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <ConstCat.h>
#include <AttrCat.h>
#include <ThresholdCat.h>
#include <InstList.h>
#include <Attribute.h>

RCSID("MLC++, $RCSfile: t_ThresholdCat.c,v $ $Revision: 1.8 $")


// CHECK() checks that ThresholdCategorizer returns cat for categorize() on
//   the instance in the LabelledInstanceBag, pointed to by libPix
// Note that in categorize(li), there is an automatic conversion from
//   LabelledInstance to Instance.
#define CHECK(cat) \
   {InstanceRC instance = bag.get_instance(bagPix); \
    ASSERT(tc.categorize(instance) == FIRST_CATEGORY_VAL + cat); \
    Mcout << tc.categorize(instance).description() << endl;}

// Check that the next instance matches cat.
#define CHECK_NEXT(cat) bag.next(bagPix); CHECK(cat)

main()
{
   Mcout << "t_ThresholdCat executing" << endl;
   
   InstanceList bag("t_ThresholdCat");
   Real val = 4.3;
   ThresholdCategorizer tc(bag.get_schema(), 1, val,
                           "Test1 ThresholdCat");

   tc.display_struct(); // displays to Mcout
   ASSERT(tc.num_categories() == 2);
   ASSERT(tc.description() == "Test1 ThresholdCat");

   Pix bagPix = bag.first();
   CHECK(0);
   CHECK_NEXT(1);
   CHECK_NEXT(1);
   CHECK_NEXT(1);
   CHECK_NEXT(0);
   bag.next(bagPix); ASSERT(bagPix == NULL);

   // Test for operator==()
   InstanceList bag1("t_ThresholdCat");
   Real value = 4.2;
   ThresholdCategorizer tc1(bag1.get_schema(), 1, value,"Test1 ThresholdCat");
   ThresholdCategorizer *tc2 = new 
      ThresholdCategorizer(bag1.get_schema(), 1, value,"Test1 ThresholdCat");
   ASSERT( tc == tc);
   ASSERT(!(tc1 == tc));
   ASSERT( (*(Categorizer*)tc2) == (*(Categorizer *)tc2));

   InstanceList acBag("t_AttrCat");
   AttrCategorizer ac(acBag.get_schema().attr_info(2), 2,
                      "Test2 AttrCat");
   ASSERT(!(tc == ac));

   AugCategory aca(0,"Aug Cat");
   ConstCategorizer cc("Const Categorizer",aca);
   ASSERT(!(tc == cc));

   delete tc2;
   return 0; // return success to shell
}   
