/* file "test_sprintf_driver.c" */

/*
 *  This file contains the implementation of a driver to run the tests in
 *  "print_test_basic.h" on the sprintf() function.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "diagnostic.h"
#include "memory_allocation.h"


/*
 *      Implementation
 *
 *  The tests used by this driver were created first for testing the
 *  print_formatting module, but in a way that they could be re-used in tests
 *  of other printf()-based code.  This driver runs the tests on the sprintf()
 *  function, which allows both testing of the tests against a sprintf()
 *  function that is widely used or testing of a new sprintf() function against
 *  the tests.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, in 2008 and placed in the public
 *  domain at that time.  It is partly based on code in
 *  "test_print_formatting_driver.c", which I also wrote in its entirity, and
 *  isn't based on anything else I or anyone else has written in the past.
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


static char *current_expected_result = NULL;
static size_t error_count = 0;


static void do_tests(void);
static void do_one_test(const char *format, ...);
static void test_n_expectation(const char *variable_name, long actual_value,
                               long expected_value);


extern int main(int argc, char **argv)
  {
    do_tests();

    if (error_count == 0)
      {
        basic_notice("All sprintf() tests passed.");
        return 0;
      }
    else
      {
        return 1;
      }
  }


static void do_tests(void)
  {
#define PRINT_TEST(args, expected_result) \
      { \
        current_expected_result = expected_result; \
        do_one_test args; \
      }

#define PRINT_N_TEST(args, expected_result_constant, declarations, \
                     expectations) \
      { \
        declarations; \
 \
        PRINT_TEST(args, expected_result_constant); \
 \
        expectations; \
      }

#define EXPECT(variable, value) \
    test_n_expectation(#variable, (long)variable, (long)value)

#include "print_test_basic.h"
#include "print_test_basic_n.h"

  }

static void do_one_test(const char *format, ...)
  {
    va_list ap;
    size_t expected_character_count;
    char *test_buffer;
    int actual_character_count;

    assert(current_expected_result != NULL);

    va_start(ap, format);

    expected_character_count = strlen(current_expected_result);

    test_buffer = MALLOC_ARRAY(char, ((expected_character_count * 3) + 200));
    if (test_buffer == NULL)
      {
        ++error_count;
        va_end(ap);
        return;
      }

    actual_character_count = vsprintf(test_buffer, format, ap);

    va_end(ap);

    if ((actual_character_count != expected_character_count) ||
        (strcmp(current_expected_result, test_buffer) != 0))
      {
        basic_error("Mismatch: expected `%s', found `%s'.",
                    current_expected_result, test_buffer);
        ++error_count;
      }

    free(test_buffer);
  }

static void test_n_expectation(const char *variable_name, long actual_value,
                               long expected_value)
  {
    if (actual_value != expected_value)
      {
        basic_error(
                "Mismatch: variable `%s' was expected to end up with the value"
                " %ld, but it actually ended up with the value %ld.",
                variable_name, expected_value, actual_value);
        ++error_count;
      }
  }
