/* file "print_test_error.h" */

/*
 *  This file contains a set of tests of printf()-based formatting error case
 *  handling.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


/*
 *      Details
 *
 *  This file contains some tests of error cases for printf()-style print
 *  formatting.  The tests are all designed to have errors that violate the
 *  ANSI/ISO 9899:1990 C programming language standard's specifications for
 *  format handling for the printf(), fprintf(), sprintf(), vprintf(),
 *  vfprintf(), and vsprintf() functions.  The intention is to use these tests
 *  to see how an implementation of print formatting handles various error
 *  cases.
 *
 *  The format of the tests is as calls to the macro PRINT_TEST().  This macro
 *  is not defined here and should be defined appropriately by the code that
 *  includes this file.  The first argument to the macro is an argument list in
 *  printf() style, starting with a format string and optionally continuing
 *  with other arguments.  The second argument is a string literal specifying
 *  the expected result of formatting the string as specified by the argument
 *  list of the first argument.
 *
 *  These tests are not exhaustive.  They are simply intended to test a
 *  sampling of the possible error cases a print formatting implementation may
 *  encounter.  They purposely don't include anything where the correct
 *  behavior can vary from implementation to implementation, such as printing
 *  pointers through the %p conversion specifier.
 *
 *  The tests aren't designed particularly systematically -- rather, they're an
 *  ad-hoc set of tests I came up with from reading the ANSI standard.
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


PRINT_TEST(("%-s", "a"), "a")
PRINT_TEST(("%+s", "a"), "a")
PRINT_TEST(("% s", "a"), "a")
PRINT_TEST(("%.#s", "a"), "")
PRINT_TEST(("%.-s", "a"), "")
PRINT_TEST(("%--2s", "a"), "")
PRINT_TEST(("%.+s", "a"), "")
PRINT_TEST(("%33+s", "a"), "")
PRINT_TEST(("%++s", "a"), "")
PRINT_TEST(("%33 s", "a"), "")
PRINT_TEST(("%  s", "a"), "")
PRINT_TEST(("%##s", "a"), "")
PRINT_TEST(("%h0d", 1), "")
PRINT_TEST(("%003.3d", 1), "")
PRINT_TEST(("%3.3*d", 1), "")
PRINT_TEST(("%-*d", -1, 1), "")
PRINT_TEST(("%h5d", 1), "")
PRINT_TEST(("%5500000000000000000000000000000.55555555555555555555555#s", "a"),
           "")
PRINT_TEST(("%3.3.3s", "a"), "")
PRINT_TEST(("%3.-3s", "a"), "  a")

PRINT_TEST(("%4.-17f", 1.0), "1.000000")
PRINT_TEST(("%.3-s", "a"), "")
PRINT_TEST(("%3.-3376890798061235653s", "a"), "  a")
PRINT_TEST(("%.+5s", "a"), "a")

PRINT_TEST(("%hhd", 1), "")
PRINT_TEST(("% +d", 1), "+1")
PRINT_TEST(("%+ d", 1), "+1")
PRINT_TEST(("%0 d", 1), " 1")
PRINT_TEST(("%-0d", 1), "1")
PRINT_TEST(("%- d", 1), " 1")
PRINT_TEST(("%0.3d", 1), "001")

PRINT_TEST(("%Lc", 'a'), "a")
PRINT_TEST(("%Ld", 1), "")
PRINT_TEST(("%Li", 1), "")
PRINT_TEST(("%Lo", 1), "")
PRINT_TEST(("%Lu", 1), "")
PRINT_TEST(("%Lx", 1), "")
PRINT_TEST(("%LX", 1), "")

PRINT_TEST(("%#d", 1), "1")
PRINT_TEST(("%#i", 1), "1")
PRINT_TEST(("%#u", 1), "1")

PRINT_TEST(("%#3d", 1), "  1")
PRINT_TEST(("%#3i", 1), "  1")
PRINT_TEST(("%#3u", 1), "  1")

PRINT_TEST(("% +e", 1.0), "+1.000000e+00")
PRINT_TEST(("% +E", 1.0), "+1.000000E+00")
PRINT_TEST(("% +f", 1.0), "+1.000000")
PRINT_TEST(("% +g", 1.0), "+1")
PRINT_TEST(("% +G", 1.0), "+1")

PRINT_TEST(("%he", 1.0), "")
PRINT_TEST(("%hE", 1.0), "")
PRINT_TEST(("%hf", 1.0), "")
PRINT_TEST(("%hg", 1.0), "")
PRINT_TEST(("%hG", 1.0), "")

PRINT_TEST(("%le", 1.0), "")
PRINT_TEST(("%lE", 1.0), "")
PRINT_TEST(("%lf", 1.0), "")
PRINT_TEST(("%lg", 1.0), "")
PRINT_TEST(("%lG", 1.0), "")

PRINT_TEST(("%0-20e", 1.0), "1.000000e+00        ")
PRINT_TEST(("%0-20E", 1.0), "1.000000E+00        ")
PRINT_TEST(("%0-20f", 1.0), "1.000000            ")
PRINT_TEST(("%0-20g", 1.0), "1                   ")
PRINT_TEST(("%0-20G", 1.0), "1                   ")

PRINT_TEST(("%.33378899999999999999999999f", 1.0), "")
PRINT_TEST(("%.33378899999999999999999999e", 1.0), "")
PRINT_TEST(("%.33378899999999999999999999E", 1.0), "")
PRINT_TEST(("%.4294967290f", 1.0e10), "")

PRINT_TEST(("%.1c", 'a'), "a")

PRINT_TEST(("%hs", "abc"), "abc")
PRINT_TEST(("%ls", "abc"), "abc")
PRINT_TEST(("%Ls", "abc"), "abc")

PRINT_TEST(("%;"), "")

PRINT_N_TEST(("%-n", &n1), "", int n1 = 57, (EXPECT(n1, 0)))
PRINT_N_TEST(("%+n", &n1), "", int n1 = 57, (EXPECT(n1, 0)))
PRINT_N_TEST(("% n", &n1), "", int n1 = 57, (EXPECT(n1, 0)))
PRINT_N_TEST(("%#n", &n1), "", int n1 = 57, (EXPECT(n1, 0)))
PRINT_N_TEST(("%0n", &n1), "", int n1 = 57, (EXPECT(n1, 0)))
PRINT_N_TEST(("%5n", &n1), "", int n1 = 57, (EXPECT(n1, 0)))
PRINT_N_TEST(("%.5n", &n1), "", int n1 = 57, (EXPECT(n1, 0)))
PRINT_N_TEST(("%Ln", &n1), "", int n1 = 57, (EXPECT(n1, 57)))
