/* file "test_memory_allocation_test.c" */

/*
 *  This file contains the implementation of code to test
 *  memory_allocation_test.c.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include "memory_allocation_test.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "basic.h"


/*
 *      Implementation
 *
 *  This code implements several different test cases, all testing the
 *  memory_allocation_test module.  A command-line argument is used to
 *  determine which test case is to be run.  Each time this program is run,
 *  only one test case is run.  The test cases are listed in the test_table
 *  array.  For each test case, this array has the name (as specified on the
 *  command line), a pointer to the function that implements that case, and the
 *  expected number of memory allocations done by the test case.  The main()
 *  function simply looks through that table to figure out which test case to
 *  run and runs it.  If the test case runs without errors and returns, the
 *  number of memory allocations done by the test case is checked against the
 *  expected result.  Note that many of the test cases are designed to exit
 *  with errors, so no expected number of memory allocations apply to these
 *  cases.
 *
 *  The code is structured this way, as a set of separate tests with only one
 *  run per invocation of this program, because the module being tested is
 *  mostly about error handling.  So the tests need to cause various sorts of
 *  errors and make sure that the memory_allocation_test module catches and
 *  handles them as it is supposed to.  Since each error is handled by printing
 *  an error message and exiting the program, multiple calls to the program are
 *  necessary to test all the error cases.  Another program,
 *  run_test_memory_allocation_test.perl, invokes this program with the right
 *  arguments and checks the results.  So it is this program combined with the
 *  run_test_memory_allocation_test.perl program that together form the
 *  complete test suite for the memory_allocation_test module.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, in 2003, 2004, and 2008 and
 *  placed in the public domain at that time.  It's entirely new code and isn't
 *  based on anything I or anyone else has written in the past.
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
 *          Chris Wilson, 2004, 2008
 */


#define ASSERT_UNFREED_COUNT(x)  assert_unfreed_count_with_line(x, __LINE__)


static void no_errors_test(void);
static void leak_test(void);
static void write_after_free_test(void);
static void free_without_alloc_test(void);
static void free_static_test(void);
static void free_on_stack_test(void);
static void double_free_test(void);
static void double_free2_test(void);
static void write_before_test(void);
static void write_after_test(void);
static void write_before2_test(void);
static void write_after2_test(void);
static void allocate_zero_test(void);
static void allocate_huge_test(void);
static void bad_free_after_good(void);
static void assert_zero_size(void);
static void assert_bad_exact_size(void);
static void assert_bad_limit_size(void);
static void assert_null_limit_size(void);
static void assert_null_exact_size(void);
static void assert_bogus_limit_size(void);
static void assert_bogus_exact_size(void);
static void assert_freed_limit_size(void);
static void assert_freed_exact_size(void);


static const struct
  {
    const char *name;
    void (*function)(void);
    size_t expected_allocation_count;
  }
        test_table[] =
  {
    { "no-errors", no_errors_test, 26 },
    { "leak", leak_test, 3 },
    { "write-after-free", write_after_free_test, 3 },
    { "free-without-alloc", free_without_alloc_test },
    { "free-static", free_static_test },
    { "free-on-stack", free_on_stack_test },
    { "double-free", double_free_test },
    { "double-free2", double_free2_test },
    { "write-before", write_before_test },
    { "write-after", write_after_test },
    { "write-before2", write_before2_test },
    { "write-after2", write_after2_test },
    { "allocate-zero", allocate_zero_test },
    { "allocate-huge", allocate_huge_test, 1 },
    { "bad-free-after-good", bad_free_after_good },
    { "assert-zero-size", assert_zero_size },
    { "assert-bad-exact-size", assert_bad_exact_size },
    { "assert-bad-limit-size", assert_bad_limit_size },
    { "assert-null-limit-size", assert_null_limit_size },
    { "assert-null-exact-size", assert_null_exact_size },
    { "assert-bogus-limit-size", assert_bogus_limit_size },
    { "assert-bogus-exact-size", assert_bogus_exact_size },
    { "assert-freed-limit-size", assert_freed_limit_size },
    { "assert-freed-exact-size", assert_freed_exact_size },
    { NULL, NULL, 0 }
  };


static void assert_unfreed_count_with_line(size_t count,
                                           unsigned long line_num)
  {
    if (unfreed_blocks() != count)
      {
        fprintf(stderr,
                "ERROR: At line %lu of %s, the unfreed block count should have"
                " been %lu, but it was %lu.\n", line_num, __FILE__,
                (unsigned long)count, (unsigned long)(unfreed_blocks()));
        exit(1);
      }
  }

static void print_usage(const char *program_name)
  {
    size_t test_num;

    fprintf(stderr, "Usage: %s <test-name>\n", program_name);
    fprintf(stderr, "    Where <test-name> is one of:\n");
    for (test_num = 0; test_table[test_num].name != NULL; ++test_num)
        fprintf(stderr, "        %s\n", test_table[test_num].name);
  }

static void no_errors_test(void)
  {
    void *pointer1;
    void *pointer2;
    void *pointer3;

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    pointer2 = test_malloc(1);
    ASSERT_UNFREED_COUNT(2);
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(1);
    test_free(pointer2);
    ASSERT_UNFREED_COUNT(0);

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    pointer2 = test_malloc(1);
    ASSERT_UNFREED_COUNT(2);
    test_free(pointer2);
    ASSERT_UNFREED_COUNT(1);
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    *(unsigned char *)pointer1 = 57;
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    ((unsigned char *)pointer1)[0] = 57;
    ((unsigned char *)pointer1)[9] = 23;
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);

    set_memory_allocation_limit(0);
    clear_memory_allocation_limit();
    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    ((unsigned char *)pointer1)[0] = 57;
    ((unsigned char *)pointer1)[9] = 23;
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);

    set_memory_allocation_limit(1);
    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    ((unsigned char *)pointer1)[0] = 57;
    ((unsigned char *)pointer1)[9] = 23;
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);
    clear_memory_allocation_limit();

    set_memory_allocation_limit(1);
    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    ((unsigned char *)pointer1)[0] = 57;
    ((unsigned char *)pointer1)[9] = 23;
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);
    set_memory_allocation_limit(1);
    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    ((unsigned char *)pointer1)[0] = 57;
    ((unsigned char *)pointer1)[9] = 23;
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);
    set_memory_allocation_limit(1);
    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    ((unsigned char *)pointer1)[0] = 57;
    ((unsigned char *)pointer1)[9] = 23;
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);
    clear_memory_allocation_limit();

    set_memory_allocation_limit(3);
    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    ((unsigned char *)pointer1)[0] = 57;
    ((unsigned char *)pointer1)[9] = 23;
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    ((unsigned char *)pointer1)[0] = 57;
    ((unsigned char *)pointer1)[9] = 23;
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    ((unsigned char *)pointer1)[0] = 57;
    ((unsigned char *)pointer1)[9] = 23;
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);
    clear_memory_allocation_limit();

    set_memory_allocation_limit(0);
    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    assert(pointer1 == NULL);
    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(200);
    assert(pointer1 == NULL);
    ASSERT_UNFREED_COUNT(0);
    clear_memory_allocation_limit();

    set_memory_allocation_limit(1);
    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    ((unsigned char *)pointer1)[0] = 57;
    ((unsigned char *)pointer1)[9] = 23;
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    assert(pointer1 == NULL);
    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(200);
    assert(pointer1 == NULL);
    ASSERT_UNFREED_COUNT(0);
    clear_memory_allocation_limit();

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    assert_is_malloced_block(pointer1);
    assert_is_malloced_block_with_minimum_size(pointer1, 10);
    assert_is_malloced_block_with_minimum_size(pointer1, 8);
    assert_is_malloced_block_with_minimum_size(pointer1, 0);
    assert_is_malloced_block_with_exact_size(pointer1, 10);
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);

    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    pointer2 = test_malloc(1);
    ASSERT_UNFREED_COUNT(2);
    pointer3 = test_malloc(1);
    ASSERT_UNFREED_COUNT(3);
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(2);
    test_free(pointer2);
    ASSERT_UNFREED_COUNT(1);
    test_free(pointer3);
    ASSERT_UNFREED_COUNT(0);

    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    pointer2 = test_malloc(1);
    ASSERT_UNFREED_COUNT(2);
    pointer3 = test_malloc(1);
    ASSERT_UNFREED_COUNT(3);
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(2);
    test_free(pointer2);
    ASSERT_UNFREED_COUNT(1);
    test_free(pointer3);
    ASSERT_UNFREED_COUNT(0);

    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    pointer2 = test_malloc(1);
    ASSERT_UNFREED_COUNT(2);
    pointer3 = test_malloc(1);
    ASSERT_UNFREED_COUNT(3);
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(2);
    test_free(pointer2);
    ASSERT_UNFREED_COUNT(1);
    test_free(pointer3);
    ASSERT_UNFREED_COUNT(0);

    check_blocks();
    describe_unfreed_blocks();
  }

static void leak_test(void)
  {
    void *pointer1;
    void *pointer2;
    void *pointer3;

    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    pointer2 = test_malloc(2);
    ASSERT_UNFREED_COUNT(2);
    pointer3 = test_malloc(3);
    ASSERT_UNFREED_COUNT(3);

    check_blocks();
    describe_unfreed_blocks();

    ASSERT_UNFREED_COUNT(3);
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(2);
    test_free(pointer2);
    ASSERT_UNFREED_COUNT(1);
    test_free(pointer3);
    ASSERT_UNFREED_COUNT(0);

    check_blocks();
    describe_unfreed_blocks();
  }

static void write_after_free_test(void)
  {
    void *pointer1;

    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);
    *(unsigned char *)pointer1 = 5;

    check_blocks();
  }

static void free_without_alloc_test(void)
  {
    test_free(NULL);
  }

static void free_static_test(void)
  {
    static int tester;

    test_free(&tester);
  }

static void free_on_stack_test(void)
  {
    int tester;

    test_free(&tester);
  }

static void double_free_test(void)
  {
    void *pointer1;

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);
    test_free(pointer1);
  }

static void double_free2_test(void)
  {
    void *pointer1;
    void *pointer2;

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    pointer2 = test_malloc(1);
    ASSERT_UNFREED_COUNT(2);
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(1);
    test_free(pointer1);
  }

static void write_before_test(void)
  {
    void *pointer1;

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    ((unsigned char *)pointer1)[-1] = 57;
    test_free(pointer1);
  }

static void write_after_test(void)
  {
    void *pointer1;

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    ((unsigned char *)pointer1)[10] = 23;
    test_free(pointer1);
  }

static void write_before2_test(void)
  {
    void *pointer1;

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    ((unsigned char *)pointer1)[-1] = 57;
    assert_is_malloced_block_with_minimum_size(pointer1, 1);
  }

static void write_after2_test(void)
  {
    void *pointer1;

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    ((unsigned char *)pointer1)[10] = 23;
    assert_is_malloced_block_with_exact_size(pointer1, 1);
  }

static void allocate_zero_test(void)
  {
    test_malloc(0);
    assert(FALSE);
  }

static void allocate_huge_test(void)
  {
    void *pointer1;

    pointer1 = test_malloc((~(size_t)0) - 3);
    assert(pointer1 == NULL);
  }

static void bad_free_after_good(void)
  {
    void *pointer1;
    void *pointer2;
    int tester;

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    pointer2 = test_malloc(1);
    ASSERT_UNFREED_COUNT(2);
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(1);
    test_free(pointer2);
    test_free(&tester);
  }

static void assert_zero_size(void)
  {
    void *pointer1;

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    assert_is_malloced_block(pointer1);
    assert_is_malloced_block_with_minimum_size(pointer1, 10);
    assert_is_malloced_block_with_minimum_size(pointer1, 8);
    assert_is_malloced_block_with_minimum_size(pointer1, 0);
    assert_is_malloced_block_with_exact_size(pointer1, 0);
  }

static void assert_bad_exact_size(void)
  {
    void *pointer1;

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    assert_is_malloced_block(pointer1);
    assert_is_malloced_block_with_minimum_size(pointer1, 10);
    assert_is_malloced_block_with_minimum_size(pointer1, 8);
    assert_is_malloced_block_with_minimum_size(pointer1, 0);
    assert_is_malloced_block_with_exact_size(pointer1, 20);
  }

static void assert_bad_limit_size(void)
  {
    void *pointer1;

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(10);
    ASSERT_UNFREED_COUNT(1);
    assert_is_malloced_block(pointer1);
    assert_is_malloced_block_with_minimum_size(pointer1, 10);
    assert_is_malloced_block_with_minimum_size(pointer1, 8);
    assert_is_malloced_block_with_minimum_size(pointer1, 0);
    assert_is_malloced_block_with_minimum_size(pointer1, 11);
  }

static void assert_null_limit_size(void)
  {
    assert_is_malloced_block_with_minimum_size(NULL, 10);
  }

static void assert_null_exact_size(void)
  {
    assert_is_malloced_block_with_exact_size(NULL, 10);
  }

static void assert_bogus_limit_size(void)
  {
    int tester;

    assert_is_malloced_block_with_minimum_size(&tester, 10);
  }

static void assert_bogus_exact_size(void)
  {
    void *pointer1;
    int tester;

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    *(unsigned char *)pointer1 = 57;
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);

    assert_is_malloced_block_with_exact_size(&tester, 10);
  }

static void assert_freed_limit_size(void)
  {
    void *pointer1;

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    *(unsigned char *)pointer1 = 57;
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(0);

    assert_is_malloced_block_with_minimum_size(pointer1, 10);
  }

static void assert_freed_exact_size(void)
  {
    void *pointer1;
    void *pointer2;
    void *pointer3;

    ASSERT_UNFREED_COUNT(0);
    pointer1 = test_malloc(1);
    ASSERT_UNFREED_COUNT(1);
    pointer2 = test_malloc(1);
    ASSERT_UNFREED_COUNT(2);
    pointer3 = test_malloc(1);
    ASSERT_UNFREED_COUNT(3);
    *(unsigned char *)pointer1 = 57;
    test_free(pointer1);
    ASSERT_UNFREED_COUNT(2);
    test_free(pointer2);
    ASSERT_UNFREED_COUNT(1);

    assert_is_malloced_block_with_exact_size(pointer2, 10);
  }

extern void test_code_point(const char *point)
  {
    fprintf(stdout, "Code point %s reached.\n", point);
  }

extern int main(int argc, char *argv[])
  {
    size_t test_num;

    if (argc != 2)
      {
        print_usage(argv[0]);
        return 1;
      }

    for (test_num = 0; test_table[test_num].name != NULL; ++test_num)
      {
        if (strcmp(test_table[test_num].name, argv[1]) == 0)
          {
            size_t start_count;
            size_t end_count;
            size_t expected_count;

            start_count = get_memory_allocation_count();
            (test_table[test_num].function)();
            end_count = get_memory_allocation_count();
            end_count -= start_count;
            expected_count = test_table[test_num].expected_allocation_count;
            if (end_count != expected_count)
              {
                fprintf(stderr,
                        "Error: For test `%s', we expected %lu allocations but"
                        " found %lu allocations.\n", argv[1],
                        (unsigned long)expected_count,
                        (unsigned long)end_count);
                return 1;
              }
            return 0;
          }
      }

    fprintf(stderr,
            "Error: Test name `%s' was not recognized as a valid test name.\n",
            argv[1]);
    print_usage(argv[0]);
    return 1;
  }
