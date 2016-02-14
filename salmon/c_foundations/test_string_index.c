/* file "test_string_index.c" */

/*
 *  This file contains the implementation of code to test string_index.c.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include "string_index.h"
#include "memory_allocation_test.h"
#include <stdio.h>
#include <string.h>


/*
 *      Implementation
 *
 *  This code is basically one long sequence of calls into the string_index
 *  module to test the functionality of that module.  It uses the
 *  test_code_point() function as a replacement for the code_point() macro in
 *  the string_index.c code so it can trace the coverage this testing achieves.
 *  The fail_test() function allows multiple test failures to be recorded and
 *  printed out, and then for the program to return success or failure based on
 *  whether there were failures, as well as printing out a message at the end
 *  saying how many failures there were.
 *
 *  There is nothing particularly special about the particular way the calls
 *  are made to test the code.  I just made up various things that seemed to me
 *  like they might catch bugs in the code (and in a few cases they did catch
 *  bugs that I then fixed).  I used the coverage results to see where I was
 *  failing to cover all the code and added more tests based on that feedback
 *  until I was covering everything I reasonably could with these tests.  Note
 *  that these tests use the memory_allocation_test module to make sure that
 *  allocations and deallocations are done properly, and to simulate
 *  out-of-memory cases.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, in 2003 and 2004 and placed in
 *  the public domain at that time.  It's entirely new code and isn't based on
 *  anything I or anyone else has written in the past.
 *
 *
 *      Legal Issues
 *
 *  I've written this code from scratch, without using or refering to any other
 *  code, on my own equipment and not for hire for anyone else, so I have full
 *  legal rights to place it in the public domain.
 *
 *  I've chosen to put this software in the public domain rather than
 *  copyrighting it and using the FSF's GPL or a Berkeley-style ``vanity''
 *  license because my personal opinion is that making it public domain
 *  maximizes its potential usefulness to others.  Anyone can feel free to use
 *  it for any purpose, including with their own proprietary code or with GPL
 *  code, without fear of intellectual property issues.  I have no desire to
 *  stop anyone else from making money on this code or getting any other
 *  advantages they can from it.
 *
 *  I do request that anyone who finds this software useful let me know about
 *  it.  You can drop me e-mail at "Chris Wilson" <chris@chriswilson.info> to
 *  let me know how you are using it and what is good and bad about it for you.
 *  Bug reports are also appreciated.  Also, if you release a product or
 *  software package of some sort that includes this software, I would like you
 *  to give me credit in the documentation as appropriate for the importance of
 *  my code in your product.  These are requests, not requirements, so you are
 *  not legally bound to do them, they're just a nice way to show appreciation.
 *
 *  Note that even though this software is public domain and there are no
 *  copyright laws that limit what you can do with it, other laws may apply.
 *  For example, if you lie and claim that you wrote this code when you did
 *  not, or you claim that I endorse a product of yours when I do not, that
 *  could be fraud and you could be legally liable.
 *
 *  There is absolutely no warranty for this software!  I am warning you now
 *  that it may or may not work.  It may have bugs that cause you a lot of
 *  problems.  I disclaim any implied warranties for merchantability or fitness
 *  for a particular purpose.  The fact that I have written some documentation
 *  on what I intended this software for should not be taken as any kind of
 *  warranty that it will actually behave that way.  I am providing this
 *  software as-is in the hope that it will be useful.
 *
 *          Chris Wilson, 2004
 */


static size_t failed_test_count = 0;

extern void test_code_point(const char *point)
  {
    fprintf(stderr, "Code point %s reached.\n", point);
  }

extern void fail_test(const char *test_specifier)
  {
    fprintf(stderr, "%s failed!\n", test_specifier);
    if (failed_test_count < ~(size_t)0)
        ++failed_test_count;
  }

extern int main(int argc, char *argv[])
  {
    string_index *index1;
    string_index *index2;
    string_index *index3;
    string_index *index4;
    string_index *index5;
    string_index *index6;
    string_index *index7;
    string_index *index8;
    string_index *index9;
    string_index *index10;
    string_index *index11;
    string_index *index12;
    string_index *index13;

    index1 = create_string_index();
    check_string_index_integrity(index1);
    index2 = create_string_index();
    check_string_index_integrity(index2);
    index3 = create_string_index();
    check_string_index_integrity(index3);
    index4 = create_string_index();
    check_string_index_integrity(index4);
    index5 = create_string_index();
    check_string_index_integrity(index5);
    index6 = create_string_index();
    check_string_index_integrity(index6);
    index7 = create_string_index();
    check_string_index_integrity(index7);
    index8 = create_string_index();
    check_string_index_integrity(index8);
    index9 = create_string_index();
    check_string_index_integrity(index9);
    index10 = create_string_index();
    check_string_index_integrity(index10);
    index11 = create_string_index();
    check_string_index_integrity(index11);

    if (lookup_in_string_index(index1, "") != NULL)
        fail_test("Test 1");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, " ") != NULL)
        fail_test("Test 2");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, "a") != NULL)
        fail_test("Test 3");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, "aaaa") != NULL)
        fail_test("Test 4");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, ""))
        fail_test("Test 5");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, " "))
        fail_test("Test 6");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, "a"))
        fail_test("Test 7");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, "aaaa"))
        fail_test("Test 8");
    check_string_index_integrity(index1);

    enter_into_string_index(index1, "", "Hello!");
    check_string_index_integrity(index1);
    if (strcmp(lookup_in_string_index(index1, ""), "Hello!") != 0)
        fail_test("Test 9");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, " ") != NULL)
        fail_test("Test 10");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, "a") != NULL)
        fail_test("Test 11");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, "aaaa") != NULL)
        fail_test("Test 12");
    check_string_index_integrity(index1);
    if (!exists_in_string_index(index1, ""))
        fail_test("Test 13");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, " "))
        fail_test("Test 14");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, "a"))
        fail_test("Test 15");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, "aaaa"))
        fail_test("Test 16");
    check_string_index_integrity(index1);

    remove_from_string_index(index1, " ");
    check_string_index_integrity(index1);
    if (strcmp(lookup_in_string_index(index1, ""), "Hello!") != 0)
        fail_test("Test 17");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, " ") != NULL)
        fail_test("Test 18");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, "a") != NULL)
        fail_test("Test 19");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, "aaaa") != NULL)
        fail_test("Test 20");
    check_string_index_integrity(index1);
    if (!exists_in_string_index(index1, ""))
        fail_test("Test 21");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, " "))
        fail_test("Test 22");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, "a"))
        fail_test("Test 23");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, "aaaa"))
        fail_test("Test 24");
    check_string_index_integrity(index1);

    remove_from_string_index(index1, "");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, "") != NULL)
        fail_test("Test 25");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, " ") != NULL)
        fail_test("Test 26");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, "a") != NULL)
        fail_test("Test 27");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, "aaaa") != NULL)
        fail_test("Test 28");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, ""))
        fail_test("Test 29");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, " "))
        fail_test("Test 30");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, "a"))
        fail_test("Test 31");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, "aaaa"))
        fail_test("Test 32");
    check_string_index_integrity(index1);

    enter_into_string_index(index1, "aaaaa", "five");
    check_string_index_integrity(index1);
    enter_into_string_index(index1, "aaaa", "four");
    check_string_index_integrity(index1);
    enter_into_string_index(index1, "aaa", "three");
    check_string_index_integrity(index1);
    if (strcmp(lookup_in_string_index(index1, "aaa"), "three") != 0)
        fail_test("Test 33");
    check_string_index_integrity(index1);
    if (strcmp(lookup_in_string_index(index1, "aaaa"), "four") != 0)
        fail_test("Test 34");
    check_string_index_integrity(index1);
    if (strcmp(lookup_in_string_index(index1, "aaaaa"), "five") != 0)
        fail_test("Test 35");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, "aaaaaa") != NULL)
        fail_test("Test 36");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, "a"))
        fail_test("Test 37");
    check_string_index_integrity(index1);
    if (!exists_in_string_index(index1, "aaaa"))
        fail_test("Test 38");
    check_string_index_integrity(index1);

    remove_from_string_index(index1, "aaaa");
    check_string_index_integrity(index1);
    if (strcmp(lookup_in_string_index(index1, "aaa"), "three") != 0)
        fail_test("Test 39");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, "aaaa") != NULL)
        fail_test("Test 40");
    check_string_index_integrity(index1);
    if (strcmp(lookup_in_string_index(index1, "aaaaa"), "five") != 0)
        fail_test("Test 41");
    check_string_index_integrity(index1);
    if (lookup_in_string_index(index1, "aaaaaa") != NULL)
        fail_test("Test 42");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, "a"))
        fail_test("Test 43");
    check_string_index_integrity(index1);
    if (exists_in_string_index(index1, "aaaa"))
        fail_test("Test 44");
    check_string_index_integrity(index1);

    enter_into_string_index(index2, "able", "elba");
    check_string_index_integrity(index2);
    enter_into_string_index(index2, "Able", "Elba");
    check_string_index_integrity(index2);
    enter_into_string_index(index2, "ale", "beer");
    check_string_index_integrity(index2);
    enter_into_string_index(index2, "ability", "math");
    check_string_index_integrity(index2);
    enter_into_string_index(index2, "abilityA", "writing");
    check_string_index_integrity(index2);
    enter_into_string_index(index2, "abilityB", "driving");
    check_string_index_integrity(index2);
    enter_into_string_index(index2, "abilityC", "singing");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "able"), "elba") != 0)
        fail_test("Test 45");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "Able"), "Elba") != 0)
        fail_test("Test 46");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "ale"), "beer") != 0)
        fail_test("Test 47");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "ability"), "math") != 0)
        fail_test("Test 48");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "abilityA"), "writing") != 0)
        fail_test("Test 49");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "abilityB"), "driving") != 0)
        fail_test("Test 50");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "abilityC"), "singing") != 0)
        fail_test("Test 51");
    check_string_index_integrity(index2);

    enter_into_string_index(index2, "abilityC", "running");
    check_string_index_integrity(index2);
    enter_into_string_index(index2, "abilityD", "laughing");
    check_string_index_integrity(index2);
    enter_into_string_index(index2, "abilityE", "swimming");
    check_string_index_integrity(index2);
    enter_into_string_index(index2, "abilityF", "teaching");
    check_string_index_integrity(index2);
    enter_into_string_index(index2, "ability;", "teaching;");
    check_string_index_integrity(index2);
    enter_into_string_index(index2, "abilityEdgar", "believing");
    check_string_index_integrity(index2);
    enter_into_string_index(index2, "ability ", NULL);
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "able"), "elba") != 0)
        fail_test("Test 52");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "Able"), "Elba") != 0)
        fail_test("Test 53");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "ale"), "beer") != 0)
        fail_test("Test 54");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "ability"), "math") != 0)
        fail_test("Test 55");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "abilityA"), "writing") != 0)
        fail_test("Test 56");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "abilityB"), "driving") != 0)
        fail_test("Test 57");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "abilityC"), "running") != 0)
        fail_test("Test 58");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "abilityD"), "laughing") != 0)
        fail_test("Test 59");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "abilityE"), "swimming") != 0)
        fail_test("Test 60");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "abilityF"), "teaching") != 0)
        fail_test("Test 61");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "ability;"), "teaching;") != 0)
        fail_test("Test 62");
    check_string_index_integrity(index2);
    if (strcmp(lookup_in_string_index(index2, "abilityEdgar"), "believing") !=
        0)
      {
        fail_test("Test 63");
      }
    check_string_index_integrity(index2);
    if (lookup_in_string_index(index2, "ability ") != NULL)
        fail_test("Test 64");
    check_string_index_integrity(index2);
    if (!exists_in_string_index(index2, "ability"))
        fail_test("Test 65");
    check_string_index_integrity(index2);
    if (!exists_in_string_index(index2, "abilityEdgar"))
        fail_test("Test 66");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "abilityEdgar2"))
        fail_test("Test 67");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "abilityG"))
        fail_test("Test 68");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index1, "abilityEdgar"))
        fail_test("Test 69");
    check_string_index_integrity(index1);

    remove_from_string_index(index2, "able");
    check_string_index_integrity(index2);
    remove_from_string_index(index2, "Able");
    check_string_index_integrity(index2);
    remove_from_string_index(index2, "ale");
    check_string_index_integrity(index2);
    remove_from_string_index(index2, "ability");
    check_string_index_integrity(index2);
    remove_from_string_index(index2, "abilityA");
    check_string_index_integrity(index2);
    remove_from_string_index(index2, "abilityB");
    check_string_index_integrity(index2);
    remove_from_string_index(index2, "abilityC");
    check_string_index_integrity(index2);
    remove_from_string_index(index2, "abilityD");
    check_string_index_integrity(index2);
    remove_from_string_index(index2, "abilityE");
    check_string_index_integrity(index2);
    remove_from_string_index(index2, "abilityF");
    check_string_index_integrity(index2);
    remove_from_string_index(index2, "ability;");
    check_string_index_integrity(index2);
    remove_from_string_index(index2, "abilityEdgar");
    check_string_index_integrity(index2);
    remove_from_string_index(index2, "ability ");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "able"))
        fail_test("Test 70");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "Able"))
        fail_test("Test 71");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "ale"))
        fail_test("Test 72");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "ability"))
        fail_test("Test 73");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "abilityA"))
        fail_test("Test 74");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "abilityB"))
        fail_test("Test 75");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "abilityC"))
        fail_test("Test 76");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "abilityC"))
        fail_test("Test 77");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "abilityD"))
        fail_test("Test 78");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "abilityE"))
        fail_test("Test 79");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "abilityF"))
        fail_test("Test 80");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "ability;"))
        fail_test("Test 81");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "abilityEdgar"))
        fail_test("Test 82");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, "ability "))
        fail_test("Test 83");
    check_string_index_integrity(index2);
    if (exists_in_string_index(index2, ""))
        fail_test("Test 84");
    check_string_index_integrity(index2);

    enter_into_string_index(index3, "a", "Alpha");
    check_string_index_integrity(index3);
    enter_into_string_index(index3, "b", "Bravo");
    check_string_index_integrity(index3);
    enter_into_string_index(index3, "c", "Charlie");
    check_string_index_integrity(index3);
    enter_into_string_index(index3, "d", "Delta");
    check_string_index_integrity(index3);
    if (strcmp(lookup_in_string_index(index3, "a"), "Alpha") != 0)
        fail_test("Test 85");
    check_string_index_integrity(index3);
    if (strcmp(lookup_in_string_index(index3, "b"), "Bravo") != 0)
        fail_test("Test 86");
    check_string_index_integrity(index3);
    if (strcmp(lookup_in_string_index(index3, "c"), "Charlie") != 0)
        fail_test("Test 87");
    check_string_index_integrity(index3);
    if (strcmp(lookup_in_string_index(index3, "d"), "Delta") != 0)
        fail_test("Test 88");
    check_string_index_integrity(index3);

    remove_from_string_index(index3, "b");
    check_string_index_integrity(index3);
    if (strcmp(lookup_in_string_index(index3, "a"), "Alpha") != 0)
        fail_test("Test 89");
    check_string_index_integrity(index3);
    if (lookup_in_string_index(index3, "b") != NULL)
        fail_test("Test 90");
    check_string_index_integrity(index3);
    if (strcmp(lookup_in_string_index(index3, "c"), "Charlie") != 0)
        fail_test("Test 91");
    check_string_index_integrity(index3);
    if (strcmp(lookup_in_string_index(index3, "d"), "Delta") != 0)
        fail_test("Test 92");
    check_string_index_integrity(index3);

    remove_from_string_index(index3, "a");
    check_string_index_integrity(index3);
    if (lookup_in_string_index(index3, "a") != NULL)
        fail_test("Test 93");
    check_string_index_integrity(index3);
    if (lookup_in_string_index(index3, "b") != NULL)
        fail_test("Test 94");
    check_string_index_integrity(index3);
    if (strcmp(lookup_in_string_index(index3, "c"), "Charlie") != 0)
        fail_test("Test 95");
    check_string_index_integrity(index3);
    if (strcmp(lookup_in_string_index(index3, "d"), "Delta") != 0)
        fail_test("Test 96");
    check_string_index_integrity(index3);

    remove_from_string_index(index3, "c");
    check_string_index_integrity(index3);
    remove_from_string_index(index3, "d");
    check_string_index_integrity(index3);
    if (lookup_in_string_index(index3, "a") != NULL)
        fail_test("Test 97");
    check_string_index_integrity(index3);
    if (lookup_in_string_index(index3, "b") != NULL)
        fail_test("Test 98");
    check_string_index_integrity(index3);
    if (lookup_in_string_index(index3, "c") != NULL)
        fail_test("Test 99");
    check_string_index_integrity(index3);
    if (lookup_in_string_index(index3, "d") != NULL)
        fail_test("Test 100");
    check_string_index_integrity(index3);

    enter_into_string_index(index3, "a", "Alpha");
    check_string_index_integrity(index3);
    enter_into_string_index(index3, "b", "Bravo");
    check_string_index_integrity(index3);
    enter_into_string_index(index3, "c", "Charlie");
    check_string_index_integrity(index3);
    enter_into_string_index(index3, "d", "Delta");
    check_string_index_integrity(index3);
    if (strcmp(lookup_in_string_index(index3, "a"), "Alpha") != 0)
        fail_test("Test 101");
    check_string_index_integrity(index3);
    if (strcmp(lookup_in_string_index(index3, "b"), "Bravo") != 0)
        fail_test("Test 102");
    check_string_index_integrity(index3);
    if (strcmp(lookup_in_string_index(index3, "c"), "Charlie") != 0)
        fail_test("Test 103");
    check_string_index_integrity(index3);
    if (strcmp(lookup_in_string_index(index3, "d"), "Delta") != 0)
        fail_test("Test 104");
    check_string_index_integrity(index3);

    enter_into_string_index(index3, "d1", "Delta One");
    check_string_index_integrity(index3);
    enter_into_string_index(index3, "d2", "Delta Two");
    check_string_index_integrity(index3);
    enter_into_string_index(index3, "d3", "Delta Three");
    check_string_index_integrity(index3);
    enter_into_string_index(index3, "d4", "Delta Four");
    check_string_index_integrity(index3);
    enter_into_string_index(index3, "d5", "Delta Five");
    check_string_index_integrity(index3);
    enter_into_string_index(index3, "d6", "Delta Six");
    check_string_index_integrity(index3);
    enter_into_string_index(index3, "d7", "Delta Seven");
    check_string_index_integrity(index3);
    enter_into_string_index(index3, "d8", "Delta Eight");
    check_string_index_integrity(index3);
    enter_into_string_index(index3, "dd", "Delta Delta");
    check_string_index_integrity(index3);

    enter_into_string_index(index4, "aaabbb", "B");
    check_string_index_integrity(index4);
    enter_into_string_index(index4, "aaaccc", "C");
    check_string_index_integrity(index4);
    remove_from_string_index(index4, "aaabbb");
    check_string_index_integrity(index4);
    remove_from_string_index(index4, "aaaccc");
    check_string_index_integrity(index4);
    enter_into_string_index(index4, "dd", "D");
    check_string_index_integrity(index4);
    if (lookup_in_string_index(index4, "aaabbb") != NULL)
        fail_test("Test 105");
    check_string_index_integrity(index4);
    if (lookup_in_string_index(index4, "aaaccc") != NULL)
        fail_test("Test 106");
    check_string_index_integrity(index4);
    if (strcmp(lookup_in_string_index(index4, "dd"), "D") != 0)
        fail_test("Test 107");
    check_string_index_integrity(index4);

    enter_into_string_index(index5, "e1", "Echo One");
    check_string_index_integrity(index5);
    enter_into_string_index(index5, "e2", "Echo Two");
    check_string_index_integrity(index5);
    enter_into_string_index(index5, "e3", "Echo Three");
    check_string_index_integrity(index5);
    enter_into_string_index(index5, "e4", "Echo Four");
    check_string_index_integrity(index5);
    if (strcmp(lookup_in_string_index(index5, "e1"), "Echo One") != 0)
        fail_test("Test 108");
    check_string_index_integrity(index5);
    if (strcmp(lookup_in_string_index(index5, "e2"), "Echo Two") != 0)
        fail_test("Test 109");
    check_string_index_integrity(index5);
    if (strcmp(lookup_in_string_index(index5, "e3"), "Echo Three") != 0)
        fail_test("Test 110");
    check_string_index_integrity(index5);
    if (strcmp(lookup_in_string_index(index5, "e4"), "Echo Four") != 0)
        fail_test("Test 111");
    check_string_index_integrity(index5);
    if (lookup_in_string_index(index5, "e5") != NULL)
        fail_test("Test 112");
    check_string_index_integrity(index5);

    enter_into_string_index(index6, "e1", "Echo One");
    check_string_index_integrity(index6);
    enter_into_string_index(index6, "e2", "Echo Two");
    check_string_index_integrity(index6);
    enter_into_string_index(index6, "e3", "Echo Three");
    check_string_index_integrity(index6);
    if (strcmp(lookup_in_string_index(index6, "e1"), "Echo One") != 0)
        fail_test("Test 113");
    check_string_index_integrity(index6);
    if (strcmp(lookup_in_string_index(index6, "e2"), "Echo Two") != 0)
        fail_test("Test 114");
    check_string_index_integrity(index6);
    if (strcmp(lookup_in_string_index(index6, "e3"), "Echo Three") != 0)
        fail_test("Test 115");
    check_string_index_integrity(index6);
    if (lookup_in_string_index(index6, "e4") != NULL)
        fail_test("Test 116");
    check_string_index_integrity(index6);
    if (lookup_in_string_index(index6, "e5") != NULL)
        fail_test("Test 117");
    check_string_index_integrity(index6);

    enter_into_string_index(index7, "bbb", "B");
    check_string_index_integrity(index7);
    enter_into_string_index(index7, "ccc", "C");
    check_string_index_integrity(index7);
    remove_from_string_index(index7, "bbb");
    check_string_index_integrity(index7);
    remove_from_string_index(index7, "ccc");
    check_string_index_integrity(index7);
    enter_into_string_index(index7, "dd", "D");
    check_string_index_integrity(index7);
    if (lookup_in_string_index(index7, "bbb") != NULL)
        fail_test("Test 118");
    check_string_index_integrity(index7);
    if (lookup_in_string_index(index7, "ccc") != NULL)
        fail_test("Test 119");
    check_string_index_integrity(index7);
    if (strcmp(lookup_in_string_index(index7, "dd"), "D") != 0)
        fail_test("Test 120");
    check_string_index_integrity(index7);

    enter_into_string_index(index8, "bbb", "B");
    check_string_index_integrity(index8);
    enter_into_string_index(index8, "ccc", "C");
    check_string_index_integrity(index8);
    remove_from_string_index(index8, "bbb");
    check_string_index_integrity(index8);
    remove_from_string_index(index8, "ccc");
    check_string_index_integrity(index8);
    enter_into_string_index(index8, "", "D");
    check_string_index_integrity(index8);
    if (lookup_in_string_index(index8, "bbb") != NULL)
        fail_test("Test 121");
    check_string_index_integrity(index8);
    if (lookup_in_string_index(index8, "ccc") != NULL)
        fail_test("Test 122");
    check_string_index_integrity(index8);
    if (strcmp(lookup_in_string_index(index8, ""), "D") != 0)
        fail_test("Test 123");
    check_string_index_integrity(index8);

    enter_into_string_index(index9, "aaabbb", "B");
    check_string_index_integrity(index9);
    enter_into_string_index(index9, "aaaccc", "C");
    check_string_index_integrity(index9);
    remove_from_string_index(index9, "aaabbb");
    check_string_index_integrity(index9);
    remove_from_string_index(index9, "aaaccc");
    check_string_index_integrity(index9);
    enter_into_string_index(index9, "aaa", "D");
    check_string_index_integrity(index9);
    if (lookup_in_string_index(index9, "aaabbb") != NULL)
        fail_test("Test 124");
    check_string_index_integrity(index9);
    if (lookup_in_string_index(index9, "aaaccc") != NULL)
        fail_test("Test 125");
    check_string_index_integrity(index9);
    if (strcmp(lookup_in_string_index(index9, "aaa"), "D") != 0)
        fail_test("Test 126");
    check_string_index_integrity(index9);

    enter_into_string_index(index10, "aaabbb", "B");
    check_string_index_integrity(index10);
    enter_into_string_index(index10, "aaaccc", "C");
    check_string_index_integrity(index10);
    remove_from_string_index(index10, "aaabbb");
    check_string_index_integrity(index10);
    remove_from_string_index(index10, "aaacc");
    check_string_index_integrity(index10);
    remove_from_string_index(index10, "aaa");
    check_string_index_integrity(index10);
    remove_from_string_index(index10, "aaaccc");
    check_string_index_integrity(index10);
    enter_into_string_index(index10, "aaaddddd", "D");
    check_string_index_integrity(index10);
    if (lookup_in_string_index(index10, "aaabbb") != NULL)
        fail_test("Test 127");
    check_string_index_integrity(index10);
    if (lookup_in_string_index(index10, "aaaccc") != NULL)
        fail_test("Test 128");
    check_string_index_integrity(index10);
    if (strcmp(lookup_in_string_index(index10, "aaaddddd"), "D") != 0)
        fail_test("Test 129");
    check_string_index_integrity(index10);

    enter_into_string_index(index11, "ddd1", "Delta One");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "ddd2", "Delta Two");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "ddd3", "Delta Three");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "ddd4", "Delta Four");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "ddd5", "Delta Five");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "ddd6", "Delta Six");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "ddd7", "Delta Seven");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "ddd8", "Delta Eight");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "dddd", "Delta Delta");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "dd", "Short Delta");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "ddd1"), "Delta One") != 0)
        fail_test("Test 130");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "ddd2"), "Delta Two") != 0)
        fail_test("Test 131");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "ddd3"), "Delta Three") != 0)
        fail_test("Test 132");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "ddd4"), "Delta Four") != 0)
        fail_test("Test 133");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "ddd5"), "Delta Five") != 0)
        fail_test("Test 134");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "ddd6"), "Delta Six") != 0)
        fail_test("Test 135");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "ddd7"), "Delta Seven") != 0)
        fail_test("Test 136");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "ddd8"), "Delta Eight") != 0)
        fail_test("Test 137");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "dddd"), "Delta Delta") != 0)
        fail_test("Test 138");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "dd"), "Short Delta") != 0)
        fail_test("Test 139");
    check_string_index_integrity(index11);

    enter_into_string_index(index11, "eee8", "Echo Eight");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "eee1", "Echo One");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "eee2", "Echo Two");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "eee3", "Echo Three");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "eee4", "Echo Four");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "eee5", "Echo Five");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "eee6", "Echo Six");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "eee7", "Echo Seven");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "eeed", "Echo Delta");
    check_string_index_integrity(index11);
    enter_into_string_index(index11, "ee", "Short Echo");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "eee1"), "Echo One") != 0)
        fail_test("Test 140");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "eee2"), "Echo Two") != 0)
        fail_test("Test 141");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "eee3"), "Echo Three") != 0)
        fail_test("Test 142");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "eee4"), "Echo Four") != 0)
        fail_test("Test 143");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "eee5"), "Echo Five") != 0)
        fail_test("Test 144");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "eee6"), "Echo Six") != 0)
        fail_test("Test 145");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "eee7"), "Echo Seven") != 0)
        fail_test("Test 146");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "eee8"), "Echo Eight") != 0)
        fail_test("Test 147");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "eeed"), "Echo Delta") != 0)
        fail_test("Test 148");
    check_string_index_integrity(index11);
    if (strcmp(lookup_in_string_index(index11, "ee"), "Short Echo") != 0)
        fail_test("Test 149");
    check_string_index_integrity(index11);

    /* Test out-of-memory behavior. */
    set_memory_allocation_limit(0);
    index12 = create_string_index();
    if (index12 != NULL)
        fail_test("Test 150");
    clear_memory_allocation_limit();
    index12 = create_string_index();
    if (index12 == NULL)
        fail_test("Test 151");
    check_string_index_integrity(index12);

    set_memory_allocation_limit(0);
    enter_into_string_index(index12, "a", "Alpha");
    check_string_index_integrity(index12);
    if (exists_in_string_index(index12, "a"))
        fail_test("Test 152");
    check_string_index_integrity(index12);
    clear_memory_allocation_limit();
    enter_into_string_index(index12, "a", "Alpha");
    check_string_index_integrity(index12);
    if (!exists_in_string_index(index12, "a"))
        fail_test("Test 153");
    check_string_index_integrity(index12);

    set_memory_allocation_limit(0);
    enter_into_string_index(index12, "b", "Beta");
    check_string_index_integrity(index12);
    if (exists_in_string_index(index12, "b"))
        fail_test("Test 154");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(1);
    enter_into_string_index(index12, "b", "Beta");
    check_string_index_integrity(index12);
    clear_memory_allocation_limit();
    enter_into_string_index(index12, "b", "Beta");
    check_string_index_integrity(index12);
    if (!exists_in_string_index(index12, "b"))
        fail_test("Test 155");
    check_string_index_integrity(index12);

    enter_into_string_index(index12, "aa", "Alpha Alpha");
    check_string_index_integrity(index12);
    if (!exists_in_string_index(index12, "aa"))
        fail_test("Test 156");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(0);
    enter_into_string_index(index12, "ab", "Alpha Beta");
    check_string_index_integrity(index12);
    if (exists_in_string_index(index12, "ab"))
        fail_test("Test 157");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(1);
    enter_into_string_index(index12, "ab", "Alpha Beta");
    check_string_index_integrity(index12);
    clear_memory_allocation_limit();
    enter_into_string_index(index12, "ab", "Alpha Beta");
    check_string_index_integrity(index12);
    if (!exists_in_string_index(index12, "ab"))
        fail_test("Test 158");
    check_string_index_integrity(index12);

    enter_into_string_index(index12, "ba", "Bravo Alpha");
    check_string_index_integrity(index12);
    if (!exists_in_string_index(index12, "ba"))
        fail_test("Test 159");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(0);
    enter_into_string_index(index12, "bbb", "Three Bravo");
    check_string_index_integrity(index12);
    if (exists_in_string_index(index12, "bbb"))
        fail_test("Test 160");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(1);
    enter_into_string_index(index12, "bbb", "Three Bravo");
    check_string_index_integrity(index12);
    clear_memory_allocation_limit();
    enter_into_string_index(index12, "bbb", "Three Bravo");
    check_string_index_integrity(index12);
    if (!exists_in_string_index(index12, "bbb"))
        fail_test("Test 161");
    check_string_index_integrity(index12);

    enter_into_string_index(index12, "cccaa", "3c2a");
    check_string_index_integrity(index12);
    if (!exists_in_string_index(index12, "cccaa"))
        fail_test("Test 162");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(0);
    enter_into_string_index(index12, "cccbb", "3c2b");
    check_string_index_integrity(index12);
    if (exists_in_string_index(index12, "cccbb"))
        fail_test("Test 163");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(1);
    enter_into_string_index(index12, "cccbb", "3c2b");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(2);
    enter_into_string_index(index12, "cccbb", "3c2b");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(3);
    enter_into_string_index(index12, "cccbb", "3c2b");
    check_string_index_integrity(index12);
    clear_memory_allocation_limit();
    enter_into_string_index(index12, "cccbb", "3c2b");
    check_string_index_integrity(index12);
    if (!exists_in_string_index(index12, "cccbb"))
        fail_test("Test 164");
    check_string_index_integrity(index12);
    if (!exists_in_string_index(index12, "cccaa"))
        fail_test("Test 165");
    check_string_index_integrity(index12);

    enter_into_string_index(index12, "eee1", "Echo One");
    check_string_index_integrity(index12);
    enter_into_string_index(index12, "eee2", "Echo Two");
    check_string_index_integrity(index12);
    enter_into_string_index(index12, "eee3", "Echo Three");
    check_string_index_integrity(index12);
    enter_into_string_index(index12, "eee4", "Echo Four");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(0);
    enter_into_string_index(index12, "eee5", "Echo Five");
    check_string_index_integrity(index12);
    if (exists_in_string_index(index12, "eee5"))
        fail_test("Test 166");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(1);
    enter_into_string_index(index12, "eee5", "Echo Five");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(2);
    enter_into_string_index(index12, "eee5", "Echo Five");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(3);
    enter_into_string_index(index12, "eee5", "Echo Five");
    check_string_index_integrity(index12);
    clear_memory_allocation_limit();
    enter_into_string_index(index12, "eee5", "Echo Five");
    check_string_index_integrity(index12);
    remove_from_string_index(index12, "eee0");
    check_string_index_integrity(index12);
    if (lookup_in_string_index(index12, "eee0") != NULL)
        fail_test("Test 167");
    check_string_index_integrity(index12);
    if (strcmp(lookup_in_string_index(index12, "eee1"), "Echo One") != 0)
        fail_test("Test 168");
    check_string_index_integrity(index12);
    if (strcmp(lookup_in_string_index(index12, "eee2"), "Echo Two") != 0)
        fail_test("Test 169");
    check_string_index_integrity(index12);
    if (strcmp(lookup_in_string_index(index12, "eee3"), "Echo Three") != 0)
        fail_test("Test 170");
    check_string_index_integrity(index12);
    if (strcmp(lookup_in_string_index(index12, "eee4"), "Echo Four") != 0)
        fail_test("Test 171");
    check_string_index_integrity(index12);
    if (strcmp(lookup_in_string_index(index12, "eee5"), "Echo Five") != 0)
        fail_test("Test 172");
    check_string_index_integrity(index12);

    set_memory_allocation_limit(0);
    enter_into_string_index(index12, "eee0", "Echo Zero");
    check_string_index_integrity(index12);
    if (exists_in_string_index(index12, "eee0"))
        fail_test("Test 173");
    check_string_index_integrity(index12);
    if (lookup_in_string_index(index12, "eee0") != NULL)
        fail_test("Test 174");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(1);
    enter_into_string_index(index12, "eee0", "Echo Zero");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(2);
    enter_into_string_index(index12, "eee0", "Echo Zero");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(3);
    enter_into_string_index(index12, "eee0", "Echo Zero");
    check_string_index_integrity(index12);
    clear_memory_allocation_limit();
    enter_into_string_index(index12, "eee0", "Echo Zero");
    check_string_index_integrity(index12);
    if (strcmp(lookup_in_string_index(index12, "eee0"), "Echo Zero") != 0)
        fail_test("Test 175");
    check_string_index_integrity(index12);

    set_memory_allocation_limit(0);
    enter_into_string_index(index12, "eeezz", "Echo Zulu");
    check_string_index_integrity(index12);
    if (exists_in_string_index(index12, "eeezz"))
        fail_test("Test 176");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(1);
    enter_into_string_index(index12, "eeezz", "Echo Zulu");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(2);
    enter_into_string_index(index12, "eeezz", "Echo Zulu");
    check_string_index_integrity(index12);
    set_memory_allocation_limit(3);
    enter_into_string_index(index12, "eeezz", "Echo Zulu");
    check_string_index_integrity(index12);
    clear_memory_allocation_limit();
    enter_into_string_index(index12, "eeezz", "Echo Zulu");
    check_string_index_integrity(index12);
    if (strcmp(lookup_in_string_index(index12, "eeezz"), "Echo Zulu") != 0)
        fail_test("Test 177");
    check_string_index_integrity(index12);

    enter_into_string_index(index12, "ffff", "Four Foxtrots");
    check_string_index_integrity(index12);
    enter_into_string_index(index12, "ff", "Two Foxtrots");
    check_string_index_integrity(index12);
    remove_from_string_index(index12, "ffff");
    check_string_index_integrity(index12);
    if (lookup_in_string_index(index12, "ffff") != NULL)
        fail_test("Test 178");
    check_string_index_integrity(index12);
    if (strcmp(lookup_in_string_index(index12, "ff"), "Two Foxtrots") != 0)
        fail_test("Test 179");
    check_string_index_integrity(index12);

    enter_into_string_index(index12, "ff1", "Foxtrots One");
    check_string_index_integrity(index12);
    enter_into_string_index(index12, "ff2", "Foxtrots Two");
    check_string_index_integrity(index12);
    enter_into_string_index(index12, "ff3", "Foxtrots Three");
    check_string_index_integrity(index12);
    enter_into_string_index(index12, "ff4", "Foxtrots Four");
    check_string_index_integrity(index12);
    remove_from_string_index(index12, "ffff");
    check_string_index_integrity(index12);
    if (lookup_in_string_index(index12, "ffff") != NULL)
        fail_test("Test 180");
    check_string_index_integrity(index12);
    if (strcmp(lookup_in_string_index(index12, "ff1"), "Foxtrots One") != 0)
        fail_test("Test 181");
    check_string_index_integrity(index12);
    if (strcmp(lookup_in_string_index(index12, "ff2"), "Foxtrots Two") != 0)
        fail_test("Test 182");
    check_string_index_integrity(index12);
    if (strcmp(lookup_in_string_index(index12, "ff3"), "Foxtrots Three") != 0)
        fail_test("Test 183");
    check_string_index_integrity(index12);
    if (strcmp(lookup_in_string_index(index12, "ff4"), "Foxtrots Four") != 0)
        fail_test("Test 184");
    check_string_index_integrity(index12);

    index13 = create_string_index();
    check_string_index_integrity(index13);
    enter_into_string_index(index13, "", "Empty");
    check_string_index_integrity(index13);
    enter_into_string_index(index13, "1", "One");
    check_string_index_integrity(index13);
    enter_into_string_index(index13, "2", "Two");
    check_string_index_integrity(index13);
    enter_into_string_index(index13, "3", "Three");
    check_string_index_integrity(index13);
    enter_into_string_index(index13, "4", "Four");
    check_string_index_integrity(index13);
    enter_into_string_index(index13, "5", "Five");
    check_string_index_integrity(index13);
    if (strcmp(lookup_in_string_index(index13, ""), "Empty") != 0)
        fail_test("Test 184");
    check_string_index_integrity(index13);
    if (strcmp(lookup_in_string_index(index13, "1"), "One") != 0)
        fail_test("Test 185");
    check_string_index_integrity(index13);
    if (strcmp(lookup_in_string_index(index13, "2"), "Two") != 0)
        fail_test("Test 186");
    check_string_index_integrity(index13);
    if (strcmp(lookup_in_string_index(index13, "3"), "Three") != 0)
        fail_test("Test 187");
    check_string_index_integrity(index13);
    if (strcmp(lookup_in_string_index(index13, "4"), "Four") != 0)
        fail_test("Test 188");
    check_string_index_integrity(index13);
    if (strcmp(lookup_in_string_index(index13, "5"), "Five") != 0)
        fail_test("Test 189");
    check_string_index_integrity(index13);
    remove_from_string_index(index13, "1");
    check_string_index_integrity(index13);
    remove_from_string_index(index13, "2");
    check_string_index_integrity(index13);
    remove_from_string_index(index13, "3");
    check_string_index_integrity(index13);
    remove_from_string_index(index13, "4");
    check_string_index_integrity(index13);
    remove_from_string_index(index13, "5");
    check_string_index_integrity(index13);
    if (strcmp(lookup_in_string_index(index13, ""), "Empty") != 0)
        fail_test("Test 190");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "1") != NULL)
        fail_test("Test 191");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "2") != NULL)
        fail_test("Test 192");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "3") != NULL)
        fail_test("Test 193");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "4") != NULL)
        fail_test("Test 194");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "5") != NULL)
        fail_test("Test 195");
    check_string_index_integrity(index13);
    remove_from_string_index(index13, "");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "") != NULL)
        fail_test("Test 196");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "1") != NULL)
        fail_test("Test 197");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "2") != NULL)
        fail_test("Test 198");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "3") != NULL)
        fail_test("Test 199");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "4") != NULL)
        fail_test("Test 200");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "5") != NULL)
        fail_test("Test 201");
    check_string_index_integrity(index13);
    enter_into_string_index(index13, "", "Empty");
    check_string_index_integrity(index13);
    enter_into_string_index(index13, "1", "One");
    check_string_index_integrity(index13);
    remove_from_string_index(index13, "");
    check_string_index_integrity(index13);
    remove_from_string_index(index13, "1");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "") != NULL)
        fail_test("Test 202");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "1") != NULL)
        fail_test("Test 203");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "2") != NULL)
        fail_test("Test 204");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "3") != NULL)
        fail_test("Test 205");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "4") != NULL)
        fail_test("Test 206");
    check_string_index_integrity(index13);
    if (lookup_in_string_index(index13, "5") != NULL)
        fail_test("Test 207");
    check_string_index_integrity(index13);

    printf("Before deallocating all string indexes, there are %lu blocks still"
           " in use.\n", (unsigned long)(unfreed_blocks()));

    check_string_index_integrity(index1);
    destroy_string_index(index1);
    check_string_index_integrity(index2);
    destroy_string_index(index2);
    check_string_index_integrity(index3);
    destroy_string_index(index3);
    check_string_index_integrity(index4);
    destroy_string_index(index4);
    check_string_index_integrity(index5);
    destroy_string_index(index5);
    check_string_index_integrity(index6);
    destroy_string_index(index6);
    check_string_index_integrity(index7);
    destroy_string_index(index7);
    check_string_index_integrity(index8);
    destroy_string_index(index8);
    check_string_index_integrity(index9);
    destroy_string_index(index9);
    check_string_index_integrity(index10);
    destroy_string_index(index10);
    check_string_index_integrity(index11);
    destroy_string_index(index11);
    check_string_index_integrity(index12);
    destroy_string_index(index12);
    check_string_index_integrity(index13);
    destroy_string_index(index13);

    if (unfreed_blocks() != 0)
      {
        fprintf(stderr,
                "Error: Memory leak -- %lu blocks are still in use after all "
                "should have been deallocated.\n",
                (unsigned long)(unfreed_blocks()));
        if (failed_test_count < ~(size_t)0)
            ++failed_test_count;
      }

    if (failed_test_count > 0)
      {
        fprintf(stderr, "%lu tests failed.\n",
                (unsigned long)failed_test_count);
        return 1;
      }

    fprintf(stderr, "All tests passed.\n");
    return 0;
  }
