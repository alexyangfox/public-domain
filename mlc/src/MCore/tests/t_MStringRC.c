// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : There are two main tests. One is to do the checks of
                   MStringRC interfaces to MString. The other is to see
		   whether reference count mechanism works well without
		   memory leaks. To check memory leaks, run mlcproof.
  Doesn't test : 
  Enhancements : 
  History      : YeoGirl Yun                                      10/3/94
                   Initial Revision
*****************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <string.h>
#include <MRandom.h>
#include <MStringRC.h>
#include <MLCStream.h>

RCSID("MLC++, $RCSfile: t_MStringRC.c,v $ $Revision: 1.5 $")

#ifndef __OBJECTCENTER__
const int NUM_STRING =  100;
const int DATA_SIZE = 100;
const int INNER_LOOP = 10;
const int OUTER_LOOP = 100;
#else
const int NUM_STRING = 10;
const int DATA_SIZE = 10;
const int INNER_LOOP = 10;
const int OUTER_LOOP = 5;
#endif

const int RANDOM_SEED = 24123;

/*****************************************************************************
   Description : Returns the pointer to MStringRC of length i.
   Comments    :
*****************************************************************************/
MStringRC* make_string_rc(int i)
{
   MRandom randString(0);
   char buf[NUM_STRING+5];
   for (int j = 0; j < i; j++)
      buf[j] = randString.integer('z' - 'a') + 'a';
   buf[j] = NULL;
   MStringRC *ptr = new MStringRC(buf);
   return ptr;
}


/*****************************************************************************
  Description  : Tests basic reference count operations.
  Comments     : To make sure there's no memory leak, it should be mlcproofed.
*****************************************************************************/
void basic_reference_count_tests()
{
   MStringRC* srcArray[NUM_STRING];
 
   // make a bunch of MStringRC's
   // each string has i+1 length and
   for(int i = 0; i < NUM_STRING; i++)  
      srcArray[i] = make_string_rc(i+1);

   // now delete all and see it doesn't leak
   for( i = 0; i < NUM_STRING; i++)
      delete srcArray[i];

   // read two separate MStringRC's and delete both
   MStringRC* srcArray2[NUM_STRING];
   for( i = 0; i < NUM_STRING; i++) {
      srcArray[i] = make_string_rc(i+1);
      srcArray2[i] = make_string_rc(i+1);
      delete srcArray[i];
      delete srcArray2[i];
   }

   // read two separate MStringRC's and copy the first
   // to the second.
   for( i = 0; i < NUM_STRING; i++) {
      srcArray[i] = make_string_rc(i+1);
      srcArray2[i] = make_string_rc(i+1);

      // copy the first to the second.
      *srcArray2[i] = *srcArray[i];
      ASSERT(*srcArray2[i] == *srcArray[i]);
      
      delete srcArray[i];
      delete srcArray2[i];
   }

   // Imagine that there are two strings: A and B.
   // Make copies A1 and A2 of string A.  Make copies B1 and B2 of B.
   // Delete A, A1, and B.
   MStringRC* srcArray3[NUM_STRING];
   MStringRC* srcArray4[NUM_STRING];
   MStringRC* srcArray5[NUM_STRING];
   MStringRC* srcArray6[NUM_STRING];   

   for( i = 0; i < NUM_STRING; i++) {
      srcArray[i] = make_string_rc(i+1);
      srcArray2[i] = make_string_rc(i+1);
      srcArray3[i] = make_string_rc(i+1);
      srcArray4[i] = make_string_rc(i+1);
      srcArray5[i] = make_string_rc(i+1);
      srcArray6[i] = make_string_rc(i+1);
      *srcArray2[i] = *srcArray[i];
      ASSERT(*srcArray2[i] == *srcArray[i]);
      *srcArray3[i] = *srcArray[i];
      ASSERT(*srcArray3[i] == *srcArray[i]);
      *srcArray5[i] = *srcArray4[i];
      ASSERT(*srcArray5[i] == *srcArray4[i]);
      *srcArray6[i] = *srcArray5[i];
      ASSERT(*srcArray6[i] == *srcArray5[i]);

      delete srcArray[i];
      delete srcArray2[i];
      delete srcArray4[i];
      delete srcArray3[i];
      delete srcArray5[i];
      delete srcArray6[i];
   }
   Mcout << "Basic reference count tests OK" << endl;
}



/*****************************************************************************
  Description  : Tests thoroughly the operations of MStringRC class by
                    performing random operations.
  Comments     :
*****************************************************************************/
void intensive_reference_count_tests()
{
   Mcout << "Intensive reference count tests" << endl;
   MRandom randGen(RANDOM_SEED);

   const int L1 = 5;
   const int L2 = 10;
   const int L3 = 15;

   // Initialize an array of DATA_SIZE MStringRC's 
   // Also initialize integer array which is parallel to MStringRC array
   MStringRC* srcArray[DATA_SIZE];
   int intArray[DATA_SIZE];
   for( int i=0; i < DATA_SIZE; i++) {
      if( i%3 == 0 ) {
	 srcArray[i] = make_string_rc(L1);
	 intArray[i] = L1;
      }
      else if(i%3 == 1) {
	 srcArray[i] = make_string_rc(L2);
	 intArray[i] = L2;
      }
      else {
	 srcArray[i] = make_string_rc(L3);
	 intArray[i] = L3;
      }
   }
   
   for( i =0; i < OUTER_LOOP; i++) {
      // pick the two arbitrary numbers.
      int num1 = randGen.integer(DATA_SIZE);
      int num2 = randGen.integer(DATA_SIZE);
      *srcArray[num1] = *srcArray[num2];
      intArray[num1] = intArray[num2];

      for(int j=0; j < INNER_LOOP; j++) {
	 int num3 = randGen.integer(DATA_SIZE);
	 // operation to cause a copy-on-write.
	 if( srcArray[num3]->length() == L2) {
	    *srcArray[num3] += "*";
	    intArray[num3]++;
	 }
      }
   }

   for( i = 5; i <= 15; i+=5 ) {
      MStringRC* src=NULL;
      for(int j = 0; j < DATA_SIZE; j++) {
	 if(intArray[j] == i )
	    if (src == NULL)
	       src = srcArray[j];
            else
	       ASSERT(src->length() == srcArray[j]->length());
	 else if (intArray[j] == i + 1)
	    ASSERT(srcArray[j]->contains("*"));
      }
   }

   // now delete srcArray
   for( i = 0; i < DATA_SIZE; i++)
      delete srcArray[i];
   
   Mcout << "OK." << endl;
}



/*****************************************************************************
   Description : Does basic MString tests using MStringRC.
   Comments    :
*****************************************************************************/
void basic_cntr_tests()
{
   // test OK() by cheating
   MStringRC cheat;
   cheat.OK(2);  // tests the line where I check the level


   // test constructors with different arguments.
   MStringRC empty;
   char *nullPtr = NULL;
   MStringRC empty1(nullPtr);
   ASSERT (empty == empty1); 
   ASSERT (empty.length() == 0);
   ASSERT (strcmp(empty, "") == 0);
   MStringRC s1("Barcelona");
   ASSERT (s1.length() == 9);
   ASSERT (strcmp("Barcelona", s1) == 0);
   MStringRC s2 = "Manchester";
   ASSERT (s2.length() == 10);
   ASSERT (strcmp(s2, "Manchester") == 0);
   MStringRC s3 = s1;
   ASSERT (s3.length() == 9);
   ASSERT (strcmp("Barcelona", s3) == 0);
   MStringRC s4(s1, 4);
   ASSERT (s4.length() == 4);
   ASSERT (strcmp("Barc", s4) == 0);
   MStringRC s5 = 'a';
   ASSERT( s5.length() == 1);
   ASSERT (strcmp(s5, "a") == 0);
   char zeroChar(0);
   MStringRC s6 = zeroChar;
   ASSERT( s6.length() == 0);
   ASSERT( s6 == "");
   #ifndef MEMCHECK
   TEST_ERROR("Negative first", MStringRC s_error(s1, -1));
   TEST_ERROR("too big", MStringRC s_error(s2, 20));
   #endif

}

void basic_cnvr_tests()
{
   // test "conversion" code
   // Longs and Shorts:
   MStringRC dave5((long)333333, 0);
   ASSERT (dave5.long_value() == 333333);
   TEST_ERROR(" overflows a short.", dave5.short_value());
   MStringRC daveInt((int)4453, 8);
   #ifndef MEMCHECK
   TEST_ERROR("does not fit in the given width",
	      MStringRC daveSmallWidth(3333, 1));
   #endif
   MStringRC daveShort((short)23, 0);
   ASSERT (daveShort.short_value() == 23);
   ASSERT (daveShort.long_value() == 23);
   MStringRC dave6("cc3");
   TEST_ERROR(" Not all ", dave6.long_value());
   TEST_ERROR(" Not all ", dave6.short_value());
   MStringRC dave7("9999999999999999999999999999999999999999999");
   TEST_ERROR("string value does not fit", dave7.long_value());
   TEST_ERROR("string value does not fit", dave7.short_value());

   // Reals:

   MStringRC dave1(30000.11, 12);
   MStringRC dave2(33.341232, 3, 1000);
   Real daveReal = dave1.real_value();
   Real daveReal2 = dave2.real_value();
   ASSERT (daveReal == 30000.11);
   ASSERT (daveReal2 == 33.341);
   ASSERT (((MStringRC)"0").real_value() == 0);
   ASSERT (((MStringRC)"-2.0").real_value() == -2.0);
   MStringRC dave0("332.erl");
   TEST_ERROR(" characters were ", dave0.real_value());
   #ifndef MEMCHECK
   TEST_ERROR("does not fit in the given width of", MStringRC
	      davEr(444.3, 8, 2));
   #endif

   MStringRC bigNum("99999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999.9");
   TEST_ERROR("Overflow", bigNum.real_value());

   MStringRC littleNum("-.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000009");
   TEST_ERROR("Underflow", littleNum.real_value());

   // Chars:
   MStringRC dave3('c');
   char daveChar = dave3.char_value();
   ASSERT(daveChar == 'c');
   MStringRC dave4("cc");
   TEST_ERROR("attempt to convert a string with", dave4.char_value());
}


void basic_case_tests()
{
   // test equal_ignore_case
   MStringRC foo("hellO");
   MStringRC bar("worlD");
   MStringRC binky("Hello");
   MStringRC winky("HELlo");

   ASSERT(!foo.equal_ignore_case(bar));
   ASSERT(foo.equal_ignore_case(foo));
   ASSERT(foo.equal_ignore_case(binky));
   ASSERT(foo.equal_ignore_case(winky));
}

void basic_oper_tests()
{
   MStringRC s2 = "Manchester";
   MStringRC s1("Barcelona");
   MStringRC s4(s1, 4);
   MStringRC s5 = 'a';


   // test operator=, +=, +
   MStringRC s6 = s2;
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
   MStringRC a1 = "aaa";
   MStringRC a2 = "aba";
   MStringRC a3 = "aaab";
   MStringRC a4 = "bcdf";
   MStringRC a5 = a4;
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
}

void basic_str_tests()
{
   MStringRC empty;

   // test functions for string concatenation
   MStringRC c1 = "ABC";
   MStringRC c2 = 'A';
   MStringRC c3("BC");
   c3.prepend(c2);
   ASSERT(c3 == c1);
   c1.prepend(c1);
   ASSERT(strcmp(c1,"ABCABC") == 0);
   MStringRC c4 = "AB";
   MStringRC c5("CD");
   MStringRC c6 = "";
   cat(c4, c5, c6);
   ASSERT(strcmp(c6,"ABCD") == 0);
   cat(c4, c5, c4);
   ASSERT(c6 == c4);
   cat(c6, c4, "ABCD", c5);
   ASSERT(strcmp(c5,"ABCDABCDABCD") == 0);
   MStringRC c7 = "12";
   cat (c7,c7,"00", c7);
   ASSERT(strcmp(c7, "121200") == 0);
}

void basic_search_tests1()
{
   // test searching functions

   MStringRC empty;
   MStringRC ss = "Abracadabra";
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
   MStringRC ss1 = "Contention";
   ASSERT(ss1.contains("Con"));
   ASSERT(ss1.contains("tion"));
   ASSERT(!ss1.contains("antion"));
   ASSERT(ss1.contains("tent"));
   ASSERT(ss1.contains(empty));
   MStringRC ss2 = "Bulgaria";
   ASSERT(ss2.contains("Bul", 0));
   ASSERT(ss2.contains("gar", 3));
   ASSERT(ss2.contains("aria", 4));
   ASSERT(!ss2.contains("Bul", 1));
   ASSERT(!ss2.contains("gar", 2));
   ASSERT(!ss2.contains("gart", 4));
   ASSERT(ss2.contains(empty, 5));
}

void basic_search_tests2()
{
   MStringRC ss1 = "Contention";
   MStringRC ss2 = "Bulgaria";
   MStringRC ss3 = "World Cup USA 94";

   ASSERT(ss3.matches("USA 94", 10));
   ASSERT(!ss3.matches("World ", 0));
   ASSERT(!ss3.matches("USA 94 ", 10));
   ASSERT(!ss3.matches("USA 94", 0));
   ASSERT(!ss3.matches("Fiasco World Cup USA 94", 0));
   ASSERT(!ss3.matches("France Isreal Finland Austria", 9));
   
   TEST_ERROR("matches: Negative start", ss3.matches(ss1, -2));
   TEST_ERROR("contains: Negative start", ss3.contains(ss2, -1));
}

void basic_io_tests()
{
   // test operator>>
   MStringRC rs1, rs2, rs3;
   Mcin >> rs1 >> rs2 >> rs3;

   Mcout << rs1 << " " << rs2 << " " << rs3 << endl;

   Mcin >> rs2;
   Mcout << rs2 << endl;
   TEST_ERROR("has more chars then the max", Mcin >> rs1);


   // test insert_arg
   MStringRC templateStr("This is the %s test.");
   MStringRC argStr("template");
   MStringRC combination = insert_arg(templateStr,argStr);
   ASSERT(combination == "This is the template test.");
}

void basic_substr_tests()
{
   // test substring operations
   MStringRC srcString("This is for substring test");
   MStringRC& sub1=srcString.substring(0,srcString.length());
   ASSERT(sub1==srcString);
   Mcout << srcString(0,srcString.length() -1 ) << endl;
   MStringRC& sub2=srcString(0,srcString.length()-1);
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

   MStringRC& sub9=srcString(5,6);
   ASSERT(sub9=="is");
   MStringRC& sub10=srcString.substring(22,4);
   ASSERT(sub10=="test");


   TEST_ERROR("String::operator[]: Negative or out-of-bound index",
	      char foo = srcString[srcString.length()]);

   ASSERT(srcString[0]=='T');
   Mcout << "OK" << endl;
}   


main()
{
   Mcout << "t_MStringRC executing" << endl;   
   basic_cntr_tests();
   basic_cnvr_tests();
   basic_case_tests();
   basic_oper_tests();
   basic_str_tests();
   basic_search_tests1();
   basic_search_tests2();
   basic_io_tests();
   basic_substr_tests();
   basic_reference_count_tests();
   intensive_reference_count_tests();
   Mcout << "Success!!" << endl;

   MString a("Ronny");
   MStringRC b(a); // try constructor from MString
   ASSERT(b == "Ronny");

   return 0;
}



