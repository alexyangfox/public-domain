// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the methods of the MEnum class

  Doesn't test :
  Enhancements :
  History      : initial revision                                  10/26/94
                   Dan Sommerfield

***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <MEnum.h>

// statically create an enumerated type, and an MEnum to represent it.
// can you spot the joke in this code??  (Think Monty Python)
enum MyEnumType { zero, one, two, three = 3, four,
		  five = /* three, sir! */ 3 };

MEnum myMEnum = MEnum("zero",zero) << MEnum("one",one) << MEnum("two",two)
                << MEnum("three",three)
                << MEnum("four",four) << MEnum("five",five);

// This used to core dump because of copy constructor problems, so
//   we're leaving it.
MEnum mEnum = myMEnum << MEnum("foo", five);

main()
{

   Mcout << "t_MEnum executing" << endl;
   
   // print Mcout the enumerated type to Mcout.  Then print it fully.
   Mcout << myMEnum << endl;
   Mcout << mEnum << endl;
   myMEnum.display(Mcout, TRUE);
   Mcout << endl;
   
   // test the value_from_name function
   int value;
   value = myMEnum.value_from_name("one");
   ASSERT(value == 1);
   value = myMEnum.value_from_name("not-here");
   ASSERT(value == -1);
   value = myMEnum.value_from_name("five");
   ASSERT(value == 3);
   value = myMEnum.value_from_name("o");
   ASSERT(value == 1);
   value = myMEnum.value_from_name("t");
   ASSERT(value == 2);
   value = myMEnum.value_from_name("thr");
   ASSERT(value == 3);

   // test the name_from_value function
   MString name;
   name = myMEnum.name_from_value(one);
   ASSERT(name == "one");
   name = myMEnum.name_from_value(three);
   ASSERT(name == "three");
   name = myMEnum.name_from_value(17);
   ASSERT(name == "");

   // test the check_value function
   int result;
   result = myMEnum.check_value(0);
   ASSERT(result);
   result = myMEnum.check_value(13);
   ASSERT(result == 0);

   // test errors.  We'll use new/delete here so we can control when
   // the destructor (which may flag errors) gets called.
   #ifndef MEMCHECK
   TEST_ERROR("MEnum::MEnum: name", MEnum dummy("", 0));
   TEST_ERROR("MEnum::MEnum: value", MEnum dummy("test", -1));
   TEST_ERROR("MEnum::OK: name dup has conflicting values 2 and 3",
	      MEnum badEnum = MEnum("dup",2) << MEnum("dup",3) <<
	                MEnum("other",4);
	      );
   #endif

   return 0;
      
}


