// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Creates a bunch of interesting and extremely
                   meaningful strings and runs the MString functions
		   and operators on them.
  Doesn't test : Interactive use of get_line is tested by the options
                   functions and their tester.
  Enhancements : 
  History      :   Chia-Hsin Li                                     9/16/94
		   Added test for substring, operator(startPos,
		   endPos), and operator[].
		   Svetlozar Nestorov                               11/27/93
                   Initial revision
*****************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <string.h>
#include <MString.h>
#include <MLCStream.h>

RCSID("MLC++, $RCSfile: t_MString.c,v $ $Revision: 1.20 $")


// test functions to see how precision conversion of reals works w/
// MStrings
void test_precision()
{
   Real x = 3.52;
   Real y = 2.2893128392;
   Real z = 3.0;

   Mcout << "precision test from 0 to 10: " << endl;
   for(int i=0; i<10; i++) {
      MString testx(x, i);
      Mcout << "x(" << i << "): " << testx << endl;
      MString testy(y, i);
      Mcout << "y(" << i << "): " << testy << endl;
      MString testz(z, i);
      Mcout << "z(" << i << "): " << testz << endl;
   }

}


main()
{
   cout << "t_MString executing" << endl;

   // test OK() by cheating
   MString cheat;
   cheat.OK(2);  // tests the line where I check the level
   TEST_ERROR("String is NULL", {
      MString cheat;
      (*((char **)&cheat))= NULL; // sets cheat.str to NULL 
   } );
   
   #ifndef MEMCHECK
   TEST_ERROR("NULL at pos", {
      MString cheat("Two");
      const char *temp = cheat;   // gets pointer to "Two".
      char ss[2];
      strcpy(ss, "1");  
      (*((char **)&cheat))= ss;   // sets cheat.str = ss.
      delete (char *)temp;       
   } );

   TEST_ERROR("Non NULL char", {
      MString cheat("Two");
      const char *temp = cheat;  // gets pointer to "Two".
      char ss[6];
      strcpy(ss, "Three");
      (*((char **)&cheat))= ss;  // sets cheat.str = ss.
      delete (char *)temp;
    } );   
   #endif
   
   // test constructors with different arguments.
   MString empty;
   char *nullPtr = NULL;
   MString empty1(nullPtr);
   ASSERT (empty == empty1); 
   ASSERT (empty.length() == 0);
   ASSERT (strcmp(empty, "") == 0);
   MString s1("Barcelona");
   ASSERT (s1.length() == 9);
   ASSERT (strcmp("Barcelona", s1) == 0);
   MString s2 = "Manchester";
   ASSERT (s2.length() == 10);
   ASSERT (strcmp(s2, "Manchester") == 0);
   MString s3 = s1;
   ASSERT (s3.length() == 9);
   ASSERT (strcmp("Barcelona", s3) == 0);
   MString s4(s1, 4);
   ASSERT (s4.length() == 4);
   ASSERT (strcmp("Barc", s4) == 0);
   MString s5 = 'a';
   ASSERT( s5.length() == 1);
   ASSERT (strcmp(s5, "a") == 0);
   char zeroChar(0);
   MString s6 = zeroChar;
   ASSERT( s6.length() == 0);
   ASSERT( s6 == "");
   TEST_ERROR("Negative first", MString s_error(s1, -1));
   TEST_ERROR("too big", MString s_error(s2, 20));

   // test "conversion" code
   // Longs and Shorts:
   MString dave5((long)333333, 0);
   ASSERT (dave5.long_value() == 333333);
   TEST_ERROR(" overflows a short.", dave5.short_value());
   MString daveInt((int)4453, 8);
   TEST_ERROR("does not fit in the given width",
	      MString daveSmallWidth(3333, 1));
   MString daveShort((short)23, 0);
   ASSERT (daveShort.short_value() == 23);
   ASSERT (daveShort.long_value() == 23);
   MString dave6("cc3");
   TEST_ERROR(" Not all ", dave6.long_value());
   TEST_ERROR(" Not all ", dave6.short_value());
   MString dave7("9999999999999999999999999999999999999999999");
   TEST_ERROR("string value does not fit", dave7.long_value());
   TEST_ERROR("string value does not fit", dave7.short_value());

   // Reals:

   MString dave1(30000.11, 12);
   MString dave2(33.341232, 3, 1000);
   Real daveReal = dave1.real_value();
   Real daveReal2 = dave2.real_value();
   ASSERT (daveReal == 30000.11);
   ASSERT (daveReal2 == 33.341);
   ASSERT (((MString)"0").real_value() == 0);
   ASSERT (((MString)"-2.0").real_value() == -2.0);
   MString dave0("332.erl");
   TEST_ERROR(" characters were ", dave0.real_value());
   TEST_ERROR("does not fit in the given width of", MString
	      davEr(444.3, 8, 2));

   MString bigNum("99999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999.9");
   TEST_ERROR("Overflow", bigNum.real_value());

   MString littleNum("-.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000009");
   TEST_ERROR("Underflow", littleNum.real_value());

   // Chars:
   MString dave3('c');
   char daveChar = dave3.char_value();
   ASSERT(daveChar == 'c');
   MString dave4("cc");
   TEST_ERROR("attempt to convert a string with", dave4.char_value());

   // test equal_ignore_case
   MString foo("hellO");
   MString bar("worlD");
   MString binky("Hello");
   MString winky("HELlo");

   ASSERT(!foo.equal_ignore_case(bar));
   ASSERT(foo.equal_ignore_case(foo));
   ASSERT(foo.equal_ignore_case(binky));
   ASSERT(foo.equal_ignore_case(winky));
      
   // test operator=, +=, +
   s6 = s2;
   ASSERT (s6.length() == 10);
   ASSERT (strcmp(s6, "Manchester") == 0);
   s1 = s1;
   ASSERT (s1.length() == 9);
   ASSERT (strcmp("Barcelona", s1) == 0);
   s2 = s2 + " United";
   ASSERT (s2.length() == 17);
   ASSERT (strcmp(s2, "Manchester United") == 0);
   s4 += s5;
   ASSERT (s4.length() == 5);
   ASSERT (strcmp(s4, "Barca") == 0);
   s5 += s5;
   ASSERT (s5.length() == 2);
   ASSERT (strcmp("aa", s5) == 0);
   s6 = s4 + s4;
   ASSERT (s6.length() == 10);
   ASSERT (strcmp(s6, "BarcaBarca") == 0);
   
   // test the relational operators
   MString a1 = "aaa";
   MString a2 = "aba";
   MString a3 = "aaab";
   MString a4 = "bcdf";
   MString a5 = a4;
   ASSERT(a1 <= a2);
   ASSERT(a1 < a3);
   ASSERT(a2 > a3);
   ASSERT(a4 >= a3);
   ASSERT(a1 != a3);
   ASSERT(a5 == a4);
   ASSERT(a1 < "aaaa");
   ASSERT("aa" < a1);
   ASSERT(a1 == "aaa");
   ASSERT("aaa" == a1);
   ASSERT("aa" <= a1);
   ASSERT(a1 <= "aaaa");
   ASSERT("aaaa" > a1);
   ASSERT(a1 > "aa");
   ASSERT(a2 >= "aab");
   ASSERT("aac" >= a1);
   ASSERT("aas" != a1);
   ASSERT(a1 != "aas");

   
   // test functions for string concatenation
   MString c1 = "ABC";
   MString c2 = 'A';
   MString c3("BC");
   c3.prepend(c2);
   ASSERT(c3 == c1);
   c1.prepend(c1);
   ASSERT(strcmp(c1,"ABCABC") == 0);
   MString c4 = "AB";
   MString c5("CD");
   MString c6 = "";
   cat(c4, c5, c6);
   ASSERT(strcmp(c6,"ABCD") == 0);
   cat(c4, c5, c4);
   ASSERT(c6 == c4);
   cat(c6, c4, "ABCD", c5);
   ASSERT(strcmp(c5,"ABCDABCDABCD") == 0);
   MString c7 = "12";
   cat (c7,c7,"00", c7);
   ASSERT(strcmp(c7, "121200") == 0);

   // test searching functions
   MString ss = "Abracadabra";
   ASSERT(ss.index("a") == 3);
   ASSERT(ss.index("ad") == 5);
   ASSERT(ss.index("a",6) == 7);
   ASSERT(ss.index("a", -1) == 10);
   ASSERT(ss.index("acaadab", -3) == -1);
   ASSERT(ss.index("aaa", 5) == -1);
   ASSERT(ss.index("Ab", 0) == 0);
   ASSERT(ss.index("bra", 4) == 8);
   ASSERT(ss.index("", 6) == 6);
   ASSERT(ss.index("", -3) == 8);
   MString ss1 = "Contention";
   ASSERT(ss1.contains("Con"));
   ASSERT(ss1.contains("tion"));
   ASSERT(!ss1.contains("antion"));
   ASSERT(ss1.contains("tent"));
   ASSERT(ss1.contains(empty));
   MString ss2 = "Bulgaria";
   ASSERT(ss2.contains("Bul", 0));
   ASSERT(ss2.contains("gar", 3));
   ASSERT(ss2.contains("aria", 4));
   ASSERT(!ss2.contains("Bul", 1));
   ASSERT(!ss2.contains("gar", 2));
   ASSERT(!ss2.contains("gart", 4));
   ASSERT(ss2.contains(empty, 5));
   MString ss3 = "World Cup USA 94";
   ASSERT(ss3.matches("USA 94", 10));
   ASSERT(!ss3.matches("World ", 0));
   ASSERT(!ss3.matches("USA 94 ", 10));
   ASSERT(!ss3.matches("USA 94", 0));
   ASSERT(!ss3.matches("Fiasco World Cup USA 94", 0));
   ASSERT(!ss3.matches("France Isreal Finland Austria", 9));
   
   TEST_ERROR("matches: Negative start", ss3.matches(ss1, -2));
   TEST_ERROR("contains: Negative start", ss3.contains(ss2, -1));

   // test operator>>
   MString rs1, rs2, rs3;
   Mcin >> rs1 >> rs2 >> rs3;

   Mcout << rs1 << " " << rs2 << " " << rs3 << endl;

   Mcin >> rs2;
   cout << rs2 << endl;
   TEST_ERROR("has more chars then the max", Mcin >> rs1); 

   // test insert_arg
   MString templateStr("This is the %s test.");
   MString argStr("template");
   MString combination = insert_arg(templateStr,argStr);
   ASSERT(combination == "This is the template test.");
   
   // test substring operations
   MString srcString("This is for substring test");
   MString& sub1=srcString.substring(0,srcString.length());
   ASSERT(sub1==srcString);
   MString& sub2=srcString(0,srcString.length()-1);
   ASSERT(sub2==srcString);

   TEST_ERROR("MString::substring: Negative length of substring",
	      srcString.substring(0,-1));

   TEST_ERROR("MString::substring: Negative or out-of-bound start position",
	      srcString.substring(-1,0));

   TEST_ERROR("MString::substring: Negative or out-of-bound start position",
	      srcString.substring(srcString.length(),0));

   TEST_ERROR("MString::substring: Access out of bound",
	      srcString.substring(0,srcString.length()+1));

   TEST_ERROR("MString::substring: Negative length of substring",
	      srcString(6,5));

   TEST_ERROR("MString::substring: Negative or out-of-bound start position",
	      srcString(srcString.length(),srcString.length()));

   TEST_ERROR("MString::substring: Negative or out-of-bound end position",
	      srcString(0,srcString.length()));
   
   MString& sub9=srcString(5,6);
   ASSERT(sub9=="is");
   MString& sub10=srcString.substring(22,4);
   ASSERT(sub10=="test");

   TEST_ERROR("String::operator[]: Negative or out-of-bound index",
	      char foo = srcString[srcString.length()]);

   ASSERT(srcString[0]=='T');

   // Test line-reading ability:  create a file MString.temp.  Write
   // two lines and try to read them back.  Create a scope to make
   // sure file closes.
   MString test_string = "This is a test";
   {
      MLCOStream out("t_MString.out1");
      out << test_string << endl;
   }

   // now try to read back the line
   {
      MLCIStream in("t_MString.out1");
      MString test2;
      test2.get_line(in);
      ASSERT(test2 == test_string);
   }

   // test uppercase conversion
   MString testUpper = "Convert This to Uppercase";
   MString testUPPER = testUpper.to_upper();
   ASSERT(testUPPER == "CONVERT THIS TO UPPERCASE");

   // test conversion to long/Real WITHOUT abort on error
   int resCode;
   long longResult;
   Real realResult;

   // long: good case
   resCode = MString("123").convert_to_long(longResult);
   cout << "code: " << resCode << "   Result: " << longResult << endl;
   ASSERT(resCode == TRUE && longResult == 123);

   // long: bad characters in conversion
   resCode = MString("abc").convert_to_long(longResult);
   cout << "code: " << resCode << "   Result: " << longResult << endl;
   ASSERT(resCode == FALSE && longResult == 0);

   // long: overflow
   resCode = MString("1239898231823980192840928").convert_to_long(longResult);
   cout << "code: " << resCode << "   Result: " << longResult << endl;
   ASSERT(resCode == FALSE && longResult == -1);

   // real: good case
   resCode = MString("1.5").convert_to_real(realResult);
   cout << "code: " << resCode << "   Result: " << realResult << endl;
   ASSERT(resCode == TRUE && realResult == 1.5);

   // real: bad characters
   resCode = MString("abc").convert_to_real(realResult);
   cout << "code: " << resCode << "   Result: " << realResult << endl;
   ASSERT(resCode == FALSE && realResult == 0.0);

   // real: precision
   test_precision();

   MString name("Ronny");
   name.set_char(0,'r');
   ASSERT(name == "ronny");
   name.set_char(4,'k');
   ASSERT(name == "ronnk");
   name.set_char(1, 'e');
   ASSERT(name == "rennk");
   
   TEST_ERROR("set character to NULL",
	      name.set_char(2, 0));
   
   cout << "Success!!!" << endl;
   return 0; // return success to shell
}   




