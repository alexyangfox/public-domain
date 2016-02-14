/* file "print_test_non_standard.h" */

/*
 *  This file contains a set of tests of printf()-based formatting
 *  functionality that are non-standard.  That is, the tests may not behave the
 *  same way on all implementations that conform to the ANSI C standard.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


/*
 *      Details
 *
 *  This file contains a set of tests of printf()-style print formatting.  The
 *  tests are all designed to test features that are beyond the range of the
 *  ANSI/ISO 9899:1990 C programming language standard's specifications for
 *  format handling for the printf(), fprintf(), sprintf(), vprintf(),
 *  vfprintf(), and vsprintf() functions.  Specifically, these tests rely on
 *  the handling of floating-point values beyond the range the ANSI standard
 *  guarantees an implementation will handle.  This includes the implementation
 *  of special values to represent positive and negative infinity and ``not a
 *  number'', which are often used to handle overflow situations in floating-
 *  point implementations.  It also includes the handling of non-overflow
 *  floating-point values beyond the range that the ANSI standard dictates must
 *  be handled.
 *
 *  The format of the tests is as calls to one of two macros, PRINT_TEST() or
 *  PRINT_N_TEST().  These two macros are not defined here and should be
 *  defined appropriately by the code that includes this file.  The
 *  PRINT_TEST() macro is for tests not involving the %n format specifier and
 *  the PRINT_N_TEST() macro is for tests involving the %n format specifier.
 *
 *  The first argument to either macro is an argument list in printf() style,
 *  starting with a format string and optionally continuing with other
 *  arguments.  The second argument to either macro is a string literal
 *  specifying the expected result of formatting the string as specified by the
 *  argument list of the first argument.  These are the only two arguments for
 *  the PRINT_TEST() macro.
 *
 *  The PRINT_N_TEST() macro takes two additional arugments.  Its third
 *  argument is a list of semicolon-separated variable declarations.  These
 *  should be variables that are set by %n conversion specifiers.  The fourth
 *  and final argument is a specification of the expected final values of each
 *  of the variables set by the %n specifiers.  The format of this list is as a
 *  set of variable-value pairs, with commas between the pairs and a pair of
 *  parentheses enclosing the list of pairs.  Each pair should be specified
 *  with a call to the EXPECT() macro with the variable as the first argument
 *  and the expected value as the second argument.
 *
 *  These tests are not exhaustive, but they are designed to test most of the
 *  basic features that are usually found in floating-point implementations but
 *  that are not specifed by the standard.  To try to generate an overflow
 *  value, these tests multiply LDBL_MAX by itself.  This will certainly not be
 *  a representable value in any of the long double, double, or float types, so
 *  if an implementation handles floating-point overflow with special values,
 *  these tests are likely to obtain this value that way.  But some
 *  implementations might cause a program fault and terminate the test program
 *  in the case of floating-point overflow, or otherwise handle it in some way
 *  other than returning a special overflow value.  Similarly, this code uses
 *  the negation of LDBL_MAX times LDBL_MAX to try to obtain a negative
 *  overflow value and zero divided by zero to obtain a ``not a number'' value.
 *  If such values are supported in a given floating-point implementation,
 *  these methods are likely to obtain these values.
 *
 *  Beyond being implementation-dependent in assuming these three special
 *  values, this code is further implementation-dependent in the way it assume
 *  these values are printed.  These implementation-dependent assumptions are
 *  consistent with the standard sprintf() implementation on Linux and with the
 *  implementation in the print_formatting module these tests are primarily
 *  designed to test.  Here are the specific assumptions:
 *
 *    * The positive overflow value is printed "inf".
 *    * The negative overflow value is printed "-inf".
 *    * The ``not a number'' value is printed "nan".
 *    * For purposes of the plus and space flags in a conversion specification,
 *      "inf" and "nan" are considered non-negative values and "-inf" is
 *      considered a negative value with a minus sign on the front.
 *    * The zero flag is ignored, so even if it is present, any necessary
 *      padding will be done with spaces on the left instead of zeros after the
 *      sign character, if any.
 *    * The minus flag and width specification will be handled as for any other
 *      floating-point value, causing space padding on either the left- or the
 *      right-hand side.
 *    * The hash flag and precision specification will have no effect.
 *
 *  In addition to the testing of these special non-numeric floating-point
 *  values, these tests include a floating-point value with a particularly
 *  large exponent magnitude.  This test is designed to validate some code in
 *  my print_formatting module that handles large exponent magnitude values
 *  differently from small exponent magnitude values.
 *
 *  The tests aren't designed particularly systematically -- rather, they're an
 *  ad-hoc set of tests I came up with to cover the functionality implemented
 *  in my print_formatting module.
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


PRINT_TEST(("%.3Lf", LDBL_MAX * LDBL_MAX), "inf")
PRINT_TEST(("%-.3Lf", LDBL_MAX * LDBL_MAX), "inf")
PRINT_TEST(("%+.3Lf", LDBL_MAX * LDBL_MAX), "+inf")
PRINT_TEST(("% .3Lf", LDBL_MAX * LDBL_MAX), " inf")
PRINT_TEST(("%0.3Lf", LDBL_MAX * LDBL_MAX), "inf")
PRINT_TEST(("%10.3Lf", LDBL_MAX * LDBL_MAX), "       inf")
PRINT_TEST(("%-10.3Lf", LDBL_MAX * LDBL_MAX), "inf       ")
PRINT_TEST(("%+10.3Lf", LDBL_MAX * LDBL_MAX), "      +inf")
PRINT_TEST(("% 10.3Lf", LDBL_MAX * LDBL_MAX), "       inf")
PRINT_TEST(("%010.3Lf", LDBL_MAX * LDBL_MAX), "       inf")

PRINT_TEST(("%.3Lf", -LDBL_MAX * LDBL_MAX), "-inf")
PRINT_TEST(("%-.3Lf", -LDBL_MAX * LDBL_MAX), "-inf")
PRINT_TEST(("%+.3Lf", -LDBL_MAX * LDBL_MAX), "-inf")
PRINT_TEST(("% .3Lf", -LDBL_MAX * LDBL_MAX), "-inf")
PRINT_TEST(("%0.3Lf", -LDBL_MAX * LDBL_MAX), "-inf")
PRINT_TEST(("%10.3Lf", -LDBL_MAX * LDBL_MAX), "      -inf")
PRINT_TEST(("%-10.3Lf", -LDBL_MAX * LDBL_MAX), "-inf      ")
PRINT_TEST(("%+10.3Lf", -LDBL_MAX * LDBL_MAX), "      -inf")
PRINT_TEST(("% 10.3Lf", -LDBL_MAX * LDBL_MAX), "      -inf")
PRINT_TEST(("%010.3Lf", -LDBL_MAX * LDBL_MAX), "      -inf")

PRINT_TEST(("%.3Lf", (0.0L) / (0.0L)), "nan")
PRINT_TEST(("%-.3Lf", (0.0L) / (0.0L)), "nan")
PRINT_TEST(("%+.3Lf", (0.0L) / (0.0L)), "+nan")
PRINT_TEST(("% .3Lf", (0.0L) / (0.0L)), " nan")
PRINT_TEST(("%0.3Lf", (0.0L) / (0.0L)), "nan")
PRINT_TEST(("%10.3Lf", (0.0L) / (0.0L)), "       nan")
PRINT_TEST(("%-10.3Lf", (0.0L) / (0.0L)), "nan       ")
PRINT_TEST(("%+10.3Lf", (0.0L) / (0.0L)), "      +nan")
PRINT_TEST(("% 10.3Lf", (0.0L) / (0.0L)), "       nan")
PRINT_TEST(("%010.3Lf", (0.0L) / (0.0L)), "       nan")

PRINT_TEST(("%.3Lg", 37.5e3057L), "3.75e+3058")
