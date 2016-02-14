// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the methods of MOption hierarchy and GetOption class

  Doesn't test :
  Enhancements :
  History      : initial revision                                  10/26/94
                   Dan Sommerfield

***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <MEnum.h>
#include <GetOption.h>
#include <stdlib.h>

enum TestEnum1 { t10, t11, t12, t13 };
enum TestEnum2 { t20, t21, t22, t23 };
MEnum testMEnum1 = MEnum("t10", t10) << MEnum("t11", t11) <<
                   MEnum("t12", t12) << MEnum("t13", t13);
MEnum testMEnum2 = MEnum("t20", t20) << MEnum("t21", t21) <<
                   MEnum("t22", t22) << MEnum("t23", t23);

main()
{   
   Mcout << "t_MOption executing" << endl;

   // force prompt-level to nuisance
   MOption::set_prompt_level(MOption::promptAll);

   // get string with default value
   MString astr = get_option_string("test-string1", "DEFAULT");
   Mcout << "value = " << astr << endl;

   // get string without default
   astr = get_option_string("test-string2");
   Mcout << "value = " << astr << endl;
   
   // get integer with default value
   int value = get_option_int("test-option1", 10, "This is a test option",
			      TRUE);
   Mcout << "value = " << value << endl;

   // get integer without default value
   value = get_option_int("test-option2", "This is a test option",
			  FALSE);
   Mcout << "value = " << value << endl;

   // same for integer ranges
   value = get_option_int_range("test-option3", 10, 2, 20,
				    "This is a test option",
			      TRUE);
   Mcout << "value = " << value << endl;

   value = get_option_int_range("test-option4", -20, 33,
				"This is a test option",
			  FALSE);
   Mcout << "value = " << value << endl;

   
   // test real with default value
   Real rvalue = get_option_real("test-option5", 0.5, "This is a test option",
			      TRUE);
   Mcout << "value = " << rvalue << endl;

   // without default value
   rvalue = get_option_real("test-option6", "This is a test option",
			    FALSE);
   Mcout << "value = " << rvalue << endl;

   // range (real) with default and without
   rvalue = get_option_real_range("test-option7", 0.5, 0.1, 1.0,
				 "This is a test option",
				 TRUE);
   Mcout << "value = " << rvalue << endl;

   rvalue = get_option_real_range("test-option8", -0.5, 0.5,
				  "This is a test option", FALSE);
   Mcout << "value = " << rvalue << endl;


   // test a couple of enumerated types
   TestEnum1 te1 = get_option_enum("enum-option1", testMEnum1, t12,
				   "This is an enum option", TRUE);
   Mcout << "value = " << te1 << endl;
   TestEnum2 te2 = get_option_enum(MString("enum-option2"), testMEnum2, t21,
					   "This is an enum option", TRUE);
   Mcout << "value = " << te2 << endl;

   // test other combinations of MString/char *
   te1 = get_option_enum("enum-option3", testMEnum1, t12,
			 MString("This is an enum option"), TRUE);
   Mcout << "value = " << te1 << endl;
   te2 = get_option_enum(MString("enum-option4"), testMEnum2, t22,
				 "This is an enum option", TRUE);

   // Give an option with an improper default
   rvalue = get_option_real_range("bad-option1", -0.5, 0.0, 1.0, "Help", TRUE);
   Mcout << "value = " << rvalue << endl;


   // test prompting levels

   // requiredOnly level:  should prompt options without defaults
   // but no others
   MOption::set_prompt_level(MOption::requiredOnly);
   get_option_int("should-prompt1");
   get_option_int("should-not-prompt1", 10);

   // basic level:  should prompt basic but not nuisance options
   MOption::set_prompt_level(MOption::promptBasic);
   get_option_int("should-prompt2", 10);
   get_option_int("should-not-prompt2", 10, "Help", TRUE);

   // test use of ! at the end of an option value from the shell
   putenv("should-not-prompt3=halt!");
   get_option_string("should-not-prompt3");

   // confirmAdvanced level was tested by everything else
   MOption::set_prompt_level(MOption::promptAll);

   // test get_prompt_level
   ASSERT(MOption::get_prompt_level() == MOption::promptAll);


   // Test reading defaults from the environment
   // User should press return for all prompts in this section.
   putenv("env-option1=10");
   putenv("env-option2=15");
   value = get_option_int("env-option1", 5);
   Mcout << "value = " << value << endl;
   value = get_option_int("env-option2", "Help", TRUE);  // HAS a default!!
   Mcout << "value = " << value << endl;


   // Test using the environment to set a bad enumerated value
   putenv("bad-enum1=abc");
   te1 = get_option_enum("bad-enum1", testMEnum1, t12,
			 MString("This is an enum option"), TRUE);

   // Test force-prompting of option
   putenv("force-prompt1=!");
   putenv("force-prompt2=?");
   MOption::set_prompt_level(MOption::requiredOnly);
   value = get_option_int("force-prompt1", 5);
   Mcout << "value = " << value << endl;
   value = get_option_int("force-prompt2", 5);
   Mcout << "value = " << value << endl;
   MOption::set_prompt_level(MOption::promptAll);

   // Test boolean options with/without default
   Bool bool = get_option_bool("bool-option1", TRUE);
   Mcout << "value = " << (bool ? 'T' : 'F') << endl;
   bool = get_option_bool("bool-option2");
   Mcout << "value = " << (bool ? 'T' : 'F') << endl;

   // Test enum option without default
   get_option_enum("enum-no-default", testMEnum1,
		   MString("This is an enum option"), FALSE, te1);
   Mcout << "value = " << te1 << endl;
   
   // Test some errors
   #ifndef MEMCHECK
   
   // Empty option name
   TEST_ERROR("MOption::OK: empty option name is not allowed",
	      get_option_int("", "Help", FALSE));

   // Advanced with no default
   TEST_ERROR("MOption::get: option bad-option2 is",
	      get_option_int("bad-option2", "This will fail", TRUE));
   
   // Bad integer range
   TEST_ERROR("IntRangeOption::OK: lower bound of",
	      get_option_int_range("bad-option3", 4, 2));

   // Bad real range
   TEST_ERROR("RealRangeOption::OK: lower bound of",
	      get_option_real_range("bad-option4", 3.5, 2.2));

   #endif

   return 0;
}



