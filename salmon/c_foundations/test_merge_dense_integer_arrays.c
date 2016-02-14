/* file "test_merge_dense_integer_arrays.c" */

/*
 *  This file contains the implementation of code to test
 *  merge_dense_integer_arrays.c.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include "merge_dense_integer_arrays.h"
#include "memory_allocation_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 *      Implementation
 *
 *  This code includes eight test cases.  Each case is a set of input data and
 *  an expected output array.  For each test case, the
 *  merge_dense_integer_arrays() function is called on the input data.  The
 *  result array is checked to be sure that it matches the expected result, and
 *  then the output array is de-allocated.  Next, this function is called on
 *  the same input data several more times with the memory allocation testing
 *  framework set up to simulate the possible out-of-memory conditions.  The
 *  memory allocation testing framework has a counter that can be set to cause
 *  a simulated out-of-memory error after the given number of allocations.  The
 *  function being tested is called first with the counter set to zero (so the
 *  first memory allocation the function tries will fail), then with the
 *  counter set to one, then two, and so on.  The result is checked each time.
 *  As long as the result is NULL, the counter is incremented and the function
 *  is tested again.  As soon as the result is non-NULL that means that the
 *  function didn't do enough allocations to hit the limit so all the possible
 *  memory failure cases for that set of input data have been tested, so at
 *  that point the testing stops for that set of input data.
 *
 *  This code starts with five variable declarations.  The variables so
 *  declared are all static globals, so they are local to this file.  Since
 *  this file includes the main program, which is single-threaded, there are no
 *  re-entrance issues to worry about, so using some globals is fine.
 *
 *  The first of these globals is failed_test_count.  This is incremented every
 *  time a test fails so that at the end a report can be printed stating how
 *  many tests failed.  It's type is size_t -- this is expected to be
 *  sufficient because size_t has to be big enough to count all objects that
 *  can be allocated, and is generally greater than or equal to the size of the
 *  address space of the system running the program.  And this variable can be
 *  incremented at most once for each test, with each test having a separate
 *  piece of code to implement it.  So the number of tests should be no larger
 *  than the address space.  Of course, technically, the program address space
 *  could be larger than the data address space and the size_t only has to
 *  cover what can be allocated in a single array, which doesn't even have to
 *  be the entire data address space, but practically there are only nine cases
 *  that can cause test failures and size_t is going to be large enough to
 *  handle that in any likely case.
 *
 *  The other four globals -- num_arrays, array_space, array_sizes, and
 *  ``arrays'' -- together form the data structure that contains the input data
 *  for the test that will next be run.  The individual arrays to be merged are
 *  specified by statically allocated arrays, but they are combined dynamically
 *  into different sets of arrays and these four globals handle that dynamic
 *  combination.  The num_arrays variable specifies how many arrays are
 *  currently in the input set.  The array_space variable specifies how many
 *  arrays space has been allocated for.  array_space always has to be greater
 *  than or equal to num_arrays.  The array_sizes global is a pointer to an
 *  array of sizes of the component arrays and the ``arrays'' global is a
 *  pointer to an array of pointers to the component arrays themselves.  The
 *  arrays pointed to by these variables are resized as needed when new arrays
 *  are added.
 *
 *  The ADD_ARRAY() and DO_TEST() macros are used to set up the arrays for the
 *  tests and to run the tests.  They call the add_array_to_test() and
 *  test_merge() functions respectively.  The macros are used instead of using
 *  the functions directly to avoid having to manually specify the sizes of the
 *  various arrays.
 *
 *  The test_code_point() function isn't actually called directly within this
 *  file.  Instead, when the code being tested is built, code_point() is
 *  changed to test_code_point() through the pre-processor, so this function is
 *  called for each code point in the code under test.  This causes a line to
 *  be printed for each code point executed, giving a trace of code points
 *  executed.  This trace of code points is used to verify the coverage of the
 *  test cases.
 *
 *  The main() function starts with the declarations of all the arrays to be
 *  merged and all the expected result arrays in all the test cases.  Then come
 *  the calls to ADD_ARRAY() and DO_TEST() to specify all the test cases to
 *  run.  After that, there is a check of unfreed_blocks(), which is a function
 *  from the memory allocation test library that tells how many blocks that
 *  were allocated through the memory allocation test interface were not yet
 *  freed.  Checking that it's zero makes sure there are no memory leaks after
 *  all the tests have run.  The main() routine then prints out a one-line
 *  summary of the results -- how many tests failed or that all passed -- and
 *  returns 0 for success or non-zero for failure.
 *
 *  Next is the add_array_to_test() function.  This function adds the specified
 *  arrays to the list of arrays to merge in the next test as maintained in the
 *  four globals mentioned above.  It first checks to see if there is space in
 *  the arrays and if not it allocates more space, copies the necessary
 *  information, and de-allocates the old arrays.  Then, it adds the new array
 *  to be merged into the data structures.
 *
 *  The final function is the test_merge() function.  This is what actually
 *  calls merge_dense_integer_arrays(), which is the function under test.  It
 *  calls it first without any memory allocation failures being simulated and
 *  checks that the result is non-NULL and all the elements of the result are
 *  as expected.  It then frees the result and re-runs the test with
 *  successively higher limits on the number of allocations until the test
 *  succeeds again, just to make sure that all the error handling cases have
 *  been exercised.  The purpose of doing this is to make sure that there are
 *  no crashes or memory leaks when out-of-memory conditions are encountered.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, in 2005 and placed in the public
 *  domain at that time.  It's entirely new code and isn't based on anything I
 *  or anyone else has written in the past.
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
 *          Chris Wilson, 2005
 */


static size_t failed_test_count = 0;
static size_t num_arrays = 0;
static size_t array_space = 0;
static size_t *array_sizes = NULL;
static const size_t **arrays = NULL;


static void add_array_to_test(const size_t *array, size_t element_count);
static void test_merge(const size_t *merge_result, size_t element_count,
                       const char *test_specifier);


#define ADD_ARRAY(to_add) \
        add_array_to_test(to_add, sizeof(to_add) / sizeof(size_t))
#define DO_TEST(expected_result, test_specifier) \
        test_merge(expected_result, sizeof(expected_result) / sizeof(size_t), \
                   test_specifier)


extern void test_code_point(const char *point)
  {
    fprintf(stderr, "Code point %s reached.\n", point);
  }

extern int main(int argc, char *argv[])
  {
    size_t array_1[] = { 0 };
    size_t array_2[] = { 1 };
    size_t array_3[] = { 0, 1 };
    size_t array_4[] = { 1, 0 };
    size_t array_5[] = { 0, 1, 2, 3, 4, 5 };
    size_t array_6[] = { 4, 2 };
    size_t array_7[] = { 0, 1, 4, 5, 2, 3 };
    size_t array_8[] = { 0, 1, 2, 6, 3, 5 };
    size_t array_9[] = { 0, 1, 2, 6, 3, 4, 5 };
    size_t array_10[] = { 0, 1, 4, 3, 2, 5 };

    ADD_ARRAY(array_1);
    DO_TEST(array_1, "Test 1");

    ADD_ARRAY(array_1);
    ADD_ARRAY(array_1);
    DO_TEST(array_1, "Test 2");

    ADD_ARRAY(array_1);
    ADD_ARRAY(array_2);
    DO_TEST(array_3, "Test 3");

    ADD_ARRAY(array_3);
    ADD_ARRAY(array_4);
    DO_TEST(array_3, "Test 4");

    ADD_ARRAY(array_5);
    ADD_ARRAY(array_6);
    ADD_ARRAY(array_6);
    DO_TEST(array_7, "Test 5");

    ADD_ARRAY(array_5);
    ADD_ARRAY(array_8);
    DO_TEST(array_9, "Test 6");

    ADD_ARRAY(array_5);
    ADD_ARRAY(array_5);
    ADD_ARRAY(array_10);
    DO_TEST(array_5, "Test 7");

    ADD_ARRAY(array_5);
    ADD_ARRAY(array_10);
    ADD_ARRAY(array_10);
    DO_TEST(array_10, "Test 8");

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


static void add_array_to_test(const size_t *array, size_t element_count)
  {
    if (array_space <= num_arrays)
      {
        size_t new_space;
        size_t *new_sizes;
        const size_t **new_arrays;

        new_space = ((array_space * 2) + 3);
        new_sizes = (size_t *)(malloc(new_space * sizeof(size_t)));
        if (new_sizes == NULL)
          {
            fprintf(stderr, "Error: Out of memory in test framework.\n");
            exit(-1);
          }
        new_arrays =
                (const size_t **)(malloc(new_space * sizeof(const size_t *)));
        if (new_arrays == NULL)
          {
            fprintf(stderr, "Error: Out of memory in test framework.\n");
            exit(-1);
          }
        memcpy(new_sizes, array_sizes, array_space * sizeof(size_t));
        memcpy(new_arrays, arrays, array_space * sizeof(const size_t *));
        free(array_sizes);
        free(arrays);
        array_sizes = new_sizes;
        arrays = new_arrays;
        array_space = new_space;
      }

    array_sizes[num_arrays] = element_count;
    arrays[num_arrays] = array;

    ++num_arrays;
  }

static void test_merge(const size_t *merge_result, size_t element_count,
                       const char *test_specifier)
  {
    size_t *test_result;
    size_t elem_num;
    size_t allocation_count;

    test_result = merge_dense_integer_arrays(element_count, num_arrays,
                                             array_sizes, arrays);
    if (test_result == NULL)
      {
        fprintf(stderr, "%s failed: The return value was NULL.\n",
                test_specifier);
        if (failed_test_count < ~(size_t)0)
            ++failed_test_count;
        num_arrays = 0;
        return;
      }
    for (elem_num = 0; elem_num < element_count; ++elem_num)
      {
        if (test_result[elem_num] != merge_result[elem_num])
          {
            fprintf(stderr,
                    "%s failed: The value of element %lu in the result was %lu"
                    " when it should have been %lu.\n", test_specifier,
                    (unsigned long)elem_num,
                    (unsigned long)(test_result[elem_num]),
                    (unsigned long)(merge_result[elem_num]));
            if (failed_test_count < ~(size_t)0)
                ++failed_test_count;
            test_free(test_result);
            num_arrays = 0;
            return;
          }
      }
    test_free(test_result);

    /* Test out-of-memory behavior. */
    allocation_count = 0;
    while (TRUE)
      {
        set_memory_allocation_limit(allocation_count);
        test_result = merge_dense_integer_arrays(element_count, num_arrays,
                                                 array_sizes, arrays);
        if (test_result != NULL)
          {
            test_free(test_result);
            break;
          }
        ++allocation_count;
      }
    clear_memory_allocation_limit();

    num_arrays = 0;
  }
