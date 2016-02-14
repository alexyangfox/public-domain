/* file "print_test_fp_precision.h" */

/*
 *  This file contains a set of tests of printf()-based formatting
 *  functionality.  This set of tests requires high-precision conversions of
 *  floating-point values to decimal.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


/*
 *      Details
 *
 *  This file contains a set of tests of printf()-style print formatting.  The
 *  tests are all designed to match the ANSI/ISO 9899:1990 C programming
 *  language standard's specifications for format handling for the printf(),
 *  fprintf(), sprintf(), vprintf(), vfprintf(), and vsprintf() functions.  Any
 *  implementation of those functions that follows the standard should give
 *  results for these tests that match the specified target results for the
 *  tests.  However, the tests here require that the conversion from floating-
 *  point to decimal be done with high precision.  Since some implementations
 *  don't get all the lower-order digits correct when dealing with high-
 *  precision floating-point conversions, these tests are separated from the
 *  rest of the tests so that they can be omitted when testing such
 *  implementations.
 *
 *  The format of the tests is as calls to the macro PRINT_TEST().  This macro
 *  is not defined here and should be defined appropriately by the code that
 *  includes this file.  The first argument to the macro is an argument list in
 *  printf() style, starting with a format string and optionally continuing
 *  with other arguments.  The second argument is a string literal specifying
 *  the expected result of formatting the string as specified by the argument
 *  list of the first argument.
 *
 *  These tests are not exhaustive, but they are designed to test a selection
 *  of high-precision floating-point conversions.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, in 2008 and placed in the public
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
 *          Chris Wilson, 2004, 2008
 */


PRINT_TEST(("%.200f", 512.0),
        "512.00000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000")
PRINT_TEST(("%.200e", 512.0),
        "5.1200000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000e+02")
