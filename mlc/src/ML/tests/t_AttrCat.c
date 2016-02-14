// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test the Attribute Categorizer.
  Doesn't test : Non-nominal attributes aren't tested (should abort).
  Enhancements :
  History      : Ronny Kohavi                                       8/17/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <ConstCat.h>
#include <AttrCat.h>
#include <ThresholdCat.h>
#include <InstList.h>

RCSID("MLC++, $RCSfile: t_AttrCat.c,v $ $Revision: 1.26 $")

// CHECK() checks that AttrCategorizer returns cat for categorize() on 
//   the instance in the InstanceBag, pointed to by libPix
#define CHECK(cat) \
   {InstanceRC instance = bag.get_instance(bagPix); \
    ASSERT(ac.categorize(instance) == FIRST_CATEGORY_VAL + cat);}

// Check that the next instance matches cat.
#define CHECK_NEXT(cat) bag.next(bagPix); CHECK(cat)

static void check1(InstanceList& bag)
{
   AttrCategorizer ac(bag.get_schema(), 1, "Test1 AttrCat");
   MLCOStream out1("t_AttrCat.out1");
   ac.display_struct(out1);
   ASSERT(ac.num_categories() == 3);
   ASSERT(ac.description() == "Test1 AttrCat");

   Pix bagPix = bag.first();

   CHECK(0);
   CHECK_NEXT(1);
   CHECK_NEXT(2);   
   CHECK_NEXT(2);   
   CHECK_NEXT(2);   
   CHECK_NEXT(1);
   bag.next(bagPix); ASSERT(bagPix == NULL);
}

static void check2(InstanceList& bag)
{
   AttrCategorizer ac(bag.get_schema(), 0, "Test0 AttrCat");
   MLCOStream out1("t_AttrCat.out1", FileStream, TRUE);
   ac.display_struct(out1);
   ASSERT(ac.num_categories() == 3);
   ASSERT(ac.description() == "Test0 AttrCat");

   Pix bagPix = bag.first();

   CHECK(0);
   CHECK_NEXT(1);
   CHECK_NEXT(2);   
   CHECK_NEXT(2);   
   CHECK_NEXT(2);   
   CHECK_NEXT(0);
   bag.next(bagPix); ASSERT(bagPix == NULL);
}

static void check3(InstanceList& bag)
{

   // test the second constructor
   AttrCategorizer ac(bag.get_schema().attr_info(2), 2,
                      "Test2 AttrCat");
   MLCOStream out1("t_AttrCat.out1", FileStream, TRUE);
   ac.display_struct(out1);

   MLCOStream o(XStream);
   DotGraphPref dgp;
   TEST_ERROR("supported", ac.display_struct(o));
   TEST_ERROR("supported", ac.display_struct(out1, dgp));

   ASSERT(ac.num_categories() == 5);
   ASSERT(ac.description() == "Test2 AttrCat");

   Pix bagPix = bag.first();

   CHECK(0);
   CHECK_NEXT(1);
   CHECK_NEXT(2);   
   CHECK_NEXT(UNKNOWN_CATEGORY_VAL-FIRST_CATEGORY_VAL);   
   CHECK_NEXT(4);
   CHECK_NEXT(2);   
   bag.next(bagPix); ASSERT(bagPix == NULL);
}

static void test_info_mismatch(InstanceList& bag)
{
   InstanceList bag2("", "t_AttrCat2.names", "t_AttrCat.data");

   // build ac2 with bag (different than bag2)
   AttrCategorizer ac2(bag.get_schema(), 1, "kuku");

   Pix bagPix = bag2.first();
   InstanceRC instance = bag2.get_instance(bagPix);

   TEST_ERROR("NominalAttrInfo::equal: this had 3 values; info had 4",
              ac2.categorize(instance));// categorize a bag2 instance with bag.
}

main()
{
   cout << "t_AttrCat executing" << endl;
   
   InstanceList bag("t_AttrCat");
   check1(bag);
   check2(bag);
   check3(bag);

#ifndef MEMCHECK
   TEST_ERROR("AttrCat.c::num_nominal_attr: Bad attribute number: -5",
              AttrCategorizer ac(bag.get_schema(), -5, "kuku"));

   test_info_mismatch(bag);
#endif

   // Test for operator==()
   AttrCategorizer ac1(bag.get_schema().attr_info(2), 2,
                      "Test2 AttrCat");
   AttrCategorizer ac2(bag.get_schema().attr_info(2), 5,
                      "Test2 AttrCat");
   ASSERT(ac1 == ac1);
   ASSERT(!(ac1 == ac2));
   ASSERT( (*(Categorizer*)&ac1) == (*(Categorizer*)&ac1));

   InstanceList bag1("t_ThresholdCat");
   Real value = 4.2;
   ThresholdCategorizer tc(bag1.get_schema(), 1, value,"Test1 ThresholdCat");
   ASSERT(!(ac1 == tc));
   
   AugCategory aca(0,"Aug Cat");
   ConstCategorizer cc("Const Categorizer",aca);
   ASSERT(!(ac1 == cc));

   return 0; // return success to shell
}   
