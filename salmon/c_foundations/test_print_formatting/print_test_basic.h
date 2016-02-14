/* file "print_test_basic.h" */

/*
 *  This file contains a basic set of tests of printf()-based formatting
 *  functionality.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


/*
 *      Details
 *
 *  This file contains a basic set of tests of printf()-style print formatting.
 *  The tests are all designed to match the ANSI/ISO 9899:1990 C programming
 *  language standard's specifications for format handling for the printf(),
 *  fprintf(), sprintf(), vprintf(), vfprintf(), and vsprintf() functions.  Any
 *  implementation of those functions that follows the standard should give
 *  results for these tests that match the specified target results for the
 *  tests.
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
 *  core features specifed by the standard.  They purposely don't include
 *  anything that can vary from implementation to implementation, such as
 *  printing pointers through the %p conversion specifier.  So these tests
 *  should produce the same results on all conforming implementations.
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


PRINT_TEST(("Hello, World!\n"), "Hello, World!\n")

PRINT_TEST((""), "")
PRINT_TEST(("a"), "a")
PRINT_TEST(("b"), "b")
PRINT_TEST(("ccc"), "ccc")

PRINT_TEST(("%s", ""), "")
PRINT_TEST(("%s", "a"), "a")
PRINT_TEST(("%s", "abc"), "abc")
PRINT_TEST(("%s %s", "abc", "xyz"), "abc xyz")

PRINT_TEST(("%d", 13), "13")
PRINT_TEST(("%d", 54321), "54321")

PRINT_TEST(("%d", 13), "13")
PRINT_TEST(("%i", 13), "13")
PRINT_TEST(("%o", (unsigned)13), "15")
PRINT_TEST(("%u", (unsigned)13), "13")
PRINT_TEST(("%x", (unsigned)13), "d")
PRINT_TEST(("%X", (unsigned)13), "D")

PRINT_TEST(("%2s", "abc"), "abc")
PRINT_TEST(("%20s", "abc"), "                 abc")
PRINT_TEST(("%-20s", "abc"), "abc                 ")

PRINT_TEST(("%.2s", "abc"), "ab")
PRINT_TEST(("%2.2s", "abc"), "ab")
PRINT_TEST(("%-2.2s", "abc"), "ab")

PRINT_TEST(("%+d", 13), "+13")
PRINT_TEST(("% d", 13), " 13")
PRINT_TEST(("%5d", 13), "   13")
PRINT_TEST(("%05d", 13), "00013")

PRINT_TEST(("%#o", (unsigned)13), "015")
PRINT_TEST(("%#o", (unsigned)1), "01")
PRINT_TEST(("%#o", (unsigned)0), "0")

PRINT_TEST(("%#x", (unsigned)13), "0xd")
PRINT_TEST(("%#X", (unsigned)13), "0XD")

PRINT_TEST(("%*s", 2, "abc"), "abc")
PRINT_TEST(("%*s", 20, "abc"), "                 abc")
PRINT_TEST(("%*s", -20, "abc"), "abc                 ")

PRINT_TEST(("%.*s", 2, "abc"), "ab")
PRINT_TEST(("%*.*s", 2, 2, "abc"), "ab")
PRINT_TEST(("%*.*s", -2, 2, "abc"), "ab")

PRINT_TEST(("%*.*s", -2, -2, "abc"), "abc")

PRINT_TEST(("%ld", (long)13), "13")
PRINT_TEST(("%li", (long)13), "13")
PRINT_TEST(("%lo", (unsigned long)13), "15")
PRINT_TEST(("%lu", (unsigned long)13), "13")
PRINT_TEST(("%lx", (unsigned long)13), "d")
PRINT_TEST(("%lX", (unsigned long)13), "D")

/* Test some combinations of multiple conversion specifications in the same
 * format string. */
PRINT_TEST(("`%d', `%d'\n", (int)53, (int)54), "`53', `54'\n");
PRINT_TEST(("`%lu', `%lu'\n", (unsigned long)53, (unsigned long)54),
           "`53', `54'\n");
PRINT_TEST(("`%c', `%c', `%lu', `%s'\n", 'a', 'b', (unsigned long)53,
            "my string"), "`a', `b', `53', `my string'\n");

PRINT_TEST(("%e", 1.3), "1.300000e+00")
PRINT_TEST(("%E", 1.3), "1.300000E+00")
PRINT_TEST(("%f", 1.3), "1.300000")
PRINT_TEST(("%g", 1.3), "1.3")
PRINT_TEST(("%.0g", 1.3), "1")
PRINT_TEST(("%G", 1.3), "1.3")
PRINT_TEST(("%G", 1.3333), "1.3333")
PRINT_TEST(("%g", 1.2345678912345), "1.23457")
PRINT_TEST(("%.0g", 1.2345678912345), "1")
PRINT_TEST(("%.1g", 1.2345678912345), "1")
PRINT_TEST(("%.2g", 1.2345678912345), "1.2")
PRINT_TEST(("%.3g", 1.2345678912345), "1.23")
PRINT_TEST(("%g", 1.2345678912345), "1.23457")
PRINT_TEST(("%g", 12.345678912345), "12.3457")
PRINT_TEST(("%g", 123.45678912345), "123.457")
PRINT_TEST(("%g", 1234.5678912345), "1234.57")
PRINT_TEST(("%g", 12345.678912345), "12345.7")
PRINT_TEST(("%g", 123456.78912345), "123457")
PRINT_TEST(("%g", 1234567.8912345), "1.23457e+06")
PRINT_TEST(("%g", 12345678.912345), "1.23457e+07")
PRINT_TEST(("%g", 123456789.12345), "1.23457e+08")

PRINT_TEST(("%Le", (long double)1.3), "1.300000e+00")
PRINT_TEST(("%LE", (long double)1.3), "1.300000E+00")
PRINT_TEST(("%Lf", (long double)1.3), "1.300000")
PRINT_TEST(("%Lg", (long double)1.3), "1.3")
PRINT_TEST(("%.0Lg", (long double)1.3), "1")
PRINT_TEST(("%LG", (long double)1.3), "1.3")
PRINT_TEST(("%LG", (long double)1.3333), "1.3333")
PRINT_TEST(("%Lg", (long double)1.2345678912345), "1.23457")
PRINT_TEST(("%.0Lg", (long double)1.2345678912345), "1")
PRINT_TEST(("%.1Lg", (long double)1.2345678912345), "1")
PRINT_TEST(("%.2Lg", (long double)1.2345678912345), "1.2")
PRINT_TEST(("%.3Lg", (long double)1.2345678912345), "1.23")
PRINT_TEST(("%Lg", (long double)1.2345678912345), "1.23457")
PRINT_TEST(("%Lg", (long double)12.345678912345), "12.3457")
PRINT_TEST(("%Lg", (long double)123.45678912345), "123.457")
PRINT_TEST(("%Lg", (long double)1234.5678912345), "1234.57")
PRINT_TEST(("%Lg", (long double)12345.678912345), "12345.7")
PRINT_TEST(("%Lg", (long double)123456.78912345), "123457")
PRINT_TEST(("%Lg", (long double)1234567.8912345), "1.23457e+06")
PRINT_TEST(("%Lg", (long double)12345678.912345), "1.23457e+07")
PRINT_TEST(("%Lg", (long double)123456789.12345), "1.23457e+08")

PRINT_TEST(("%hd", 13), "13")
PRINT_TEST(("%hi", 13), "13")
PRINT_TEST(("%ho", (unsigned)13), "15")
PRINT_TEST(("%hu", (unsigned)13), "13")
PRINT_TEST(("%hx", (unsigned)13), "d")
PRINT_TEST(("%hX", (unsigned)13), "D")

PRINT_TEST(("%Le", (long double)1.3), "1.300000e+00")
PRINT_TEST(("%LE", (long double)1.3), "1.300000E+00")
PRINT_TEST(("%Lf", (long double)1.3), "1.300000")
PRINT_TEST(("%Lg", (long double)1.3), "1.3")
PRINT_TEST(("%.0Lg", (long double)1.3), "1")
PRINT_TEST(("%LG", (long double)1.3), "1.3")
PRINT_TEST(("%LG", (long double)1.3333), "1.3333")

PRINT_TEST(("%#e", 1.3), "1.300000e+00")
PRINT_TEST(("%#E", 1.3), "1.300000E+00")
PRINT_TEST(("%#f", 1.3), "1.300000")
PRINT_TEST(("%#g", 1.3), "1.30000")
PRINT_TEST(("%#.0g", 1.3), "1.")
PRINT_TEST(("%#G", 1.3), "1.30000")
PRINT_TEST(("%#G", 1.3333), "1.33330")
PRINT_TEST(("%#g", 1.2345678912345), "1.23457")
PRINT_TEST(("%#.0g", 1.2345678912345), "1.")
PRINT_TEST(("%#.1g", 1.2345678912345), "1.")
PRINT_TEST(("%#.2g", 1.2345678912345), "1.2")
PRINT_TEST(("%#.3g", 1.2345678912345), "1.23")
PRINT_TEST(("%#g", 1.2345678912345), "1.23457")
PRINT_TEST(("%#g", 12.345678912345), "12.3457")
PRINT_TEST(("%#g", 123.45678912345), "123.457")
PRINT_TEST(("%#g", 1234.5678912345), "1234.57")
PRINT_TEST(("%#g", 12345.678912345), "12345.7")
PRINT_TEST(("%#g", 123456.78912345), "123457.")
PRINT_TEST(("%#g", 1234567.8912345), "1.23457e+06")
PRINT_TEST(("%#g", 12345678.912345), "1.23457e+07")
PRINT_TEST(("%#g", 123456789.12345), "1.23457e+08")

PRINT_TEST(("%#e", 1.3e23), "1.300000e+23")
PRINT_TEST(("%#E", 1.3e23), "1.300000E+23")
PRINT_TEST(("%#g", 1.3e23), "1.30000e+23")
PRINT_TEST(("%#.0g", 1.3e23), "1.e+23")
PRINT_TEST(("%#G", 1.3e23), "1.30000E+23")
PRINT_TEST(("%#G", 1.3333e23), "1.33330E+23")
PRINT_TEST(("%#g", 1.2345678912345e23), "1.23457e+23")

PRINT_TEST(("%e", 1.3e-23), "1.300000e-23")
PRINT_TEST(("%E", 1.3e-23), "1.300000E-23")
PRINT_TEST(("%g", 1.3e-23), "1.3e-23")
PRINT_TEST(("%.0g", 1.3e-23), "1e-23")
PRINT_TEST(("%G", 1.3e-23), "1.3E-23")
PRINT_TEST(("%G", 1.3333e-23), "1.3333E-23")
PRINT_TEST(("%g", 1.2345678912345e-23), "1.23457e-23")

PRINT_TEST(("%c", 'a'), "a")
PRINT_TEST(("%*c", 0, 'a'), "a")
PRINT_TEST(("%1c", 'a'), "a")
PRINT_TEST(("%2c", 'a'), " a")
PRINT_TEST(("%-2c", 'a'), "a ")

PRINT_TEST(("%d", (int)0x80000000), "-2147483648")

PRINT_TEST(("%.200f", 1.0),
        "1.0000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000")

PRINT_TEST(("%205.200g", 1.0),
        "                                                                     "
        "                                                                     "
        "                                                                  1")

PRINT_TEST(("%10.2f", 9.9999), "     10.00")
PRINT_TEST(("%10.2f", 0.0099999), "      0.01")
PRINT_TEST(("%10.2f", 0.099999), "      0.10")
PRINT_TEST(("%10.4f", 0.0033333), "    0.0033")
PRINT_TEST(("%10.4g", 0.0033333), "  0.003333")

PRINT_TEST(("%10.4g", 0.000001), "     1e-06")
PRINT_TEST(("%10.4g", 0.00001), "     1e-05")
PRINT_TEST(("%10.4g", 0.0001), "    0.0001")
PRINT_TEST(("%10.4g", 0.001), "     0.001")
PRINT_TEST(("%10.4g", 0.01), "      0.01")
PRINT_TEST(("%10.4g", 0.1), "       0.1")
PRINT_TEST(("%10.4g", 1.0), "         1")
PRINT_TEST(("%10.4g", 10.0), "        10")
PRINT_TEST(("%10.4g", 100.0), "       100")
PRINT_TEST(("%10.4g", 1000.0), "      1000")
PRINT_TEST(("%10.4g", 10000.0), "     1e+04")
PRINT_TEST(("%10.4g", 100000.0), "     1e+05")

PRINT_TEST(("%10.4g", 0.000002), "     2e-06")
PRINT_TEST(("%10.4g", 0.00002), "     2e-05")
PRINT_TEST(("%10.4g", 0.0002), "    0.0002")
PRINT_TEST(("%10.4g", 0.002), "     0.002")
PRINT_TEST(("%10.4g", 0.02), "      0.02")
PRINT_TEST(("%10.4g", 0.2), "       0.2")
PRINT_TEST(("%10.4g", 2.0), "         2")
PRINT_TEST(("%10.4g", 20.0), "        20")
PRINT_TEST(("%10.4g", 200.0), "       200")
PRINT_TEST(("%10.4g", 2000.0), "      2000")
PRINT_TEST(("%10.4g", 20000.0), "     2e+04")
PRINT_TEST(("%10.4g", 200000.0), "     2e+05")

PRINT_TEST(("%10.4g", 0.00000099996), "     1e-06")
PRINT_TEST(("%10.4g", 0.0000099996), "     1e-05")
PRINT_TEST(("%10.4g", 0.000099996), "    0.0001")
PRINT_TEST(("%10.4g", 0.00099996), "     0.001")
PRINT_TEST(("%10.4g", 0.0099996), "      0.01")
PRINT_TEST(("%10.4g", 0.099996), "       0.1")
PRINT_TEST(("%10.4g", 0.99996), "         1")
PRINT_TEST(("%10.4g", 9.9996), "        10")
PRINT_TEST(("%10.4g", 99.996), "       100")
PRINT_TEST(("%10.4g", 999.96), "      1000")
PRINT_TEST(("%10.4g", 9999.6), "     1e+04")
PRINT_TEST(("%10.4g", 99996.0), "     1e+05")

PRINT_TEST(("%10.4g", 0.99996e5), "     1e+05")
PRINT_TEST(("%10.4g", 0.99996e9), "     1e+09")
PRINT_TEST(("%10.4g", 0.99996e10), "     1e+10")
PRINT_TEST(("%10.4g", 0.99996e11), "     1e+11")
PRINT_TEST(("%10.4g", 0.99996e99), "     1e+99")
PRINT_TEST(("%10.4g", 0.99996e100), "    1e+100")
PRINT_TEST(("%10.4g", 0.99996e101), "    1e+101")
PRINT_TEST(("%10.4g", 0.99996e-9), "     1e-09")
PRINT_TEST(("%10.4g", 0.99996e-10), "     1e-10")
PRINT_TEST(("%10.4g", 0.99996e-11), "     1e-11")
PRINT_TEST(("%10.4g", 0.99996e-99), "     1e-99")
PRINT_TEST(("%10.4g", 0.99996e-100), "    1e-100")
PRINT_TEST(("%10.4g", 0.99996e-101), "    1e-101")

PRINT_TEST(("%5d", -13), "  -13")
PRINT_TEST(("%+5d", 13), "  +13")
PRINT_TEST(("% 5d", 13), "   13")
PRINT_TEST(("% 1d", 13), " 13")
PRINT_TEST(("%1d", 13), "13")
PRINT_TEST(("%5u", 13), "   13")
PRINT_TEST(("%1u", 13), "13")
PRINT_TEST(("%#5o", 13), "  015")
PRINT_TEST(("%#5x", 13), "  0xd")
PRINT_TEST(("%#5X", 13), "  0XD")
PRINT_TEST(("%#5.0o", 13), "  015")
PRINT_TEST(("%#5.1o", 13), "  015")
PRINT_TEST(("%#5.2o", 13), "  015")
PRINT_TEST(("%#5.3o", 13), "  015")
PRINT_TEST(("%#5.4o", 13), " 0015")
PRINT_TEST(("%#5.5o", 13), "00015")
PRINT_TEST(("%#5.6o", 13), "000015")

PRINT_TEST(("%+e", 1.0), "+1.000000e+00")
PRINT_TEST(("%+E", 1.0), "+1.000000E+00")
PRINT_TEST(("%+f", 1.0), "+1.000000")
PRINT_TEST(("%+g", 1.0), "+1")
PRINT_TEST(("%+G", 1.0), "+1")

PRINT_TEST(("% e", 1.0), " 1.000000e+00")
PRINT_TEST(("% E", 1.0), " 1.000000E+00")
PRINT_TEST(("% f", 1.0), " 1.000000")
PRINT_TEST(("% g", 1.0), " 1")
PRINT_TEST(("% G", 1.0), " 1")

PRINT_TEST(("% e", -1.0), "-1.000000e+00")
PRINT_TEST(("% E", -1.0), "-1.000000E+00")
PRINT_TEST(("% f", -1.0), "-1.000000")
PRINT_TEST(("% g", -1.0), "-1")
PRINT_TEST(("% G", -1.0), "-1")

PRINT_TEST(("%-20e", 1.0), "1.000000e+00        ")
PRINT_TEST(("%-20E", 1.0), "1.000000E+00        ")
PRINT_TEST(("%-20f", 1.0), "1.000000            ")
PRINT_TEST(("%-20g", 1.0), "1                   ")
PRINT_TEST(("%-20G", 1.0), "1                   ")

PRINT_TEST(("%020e", 1.0), "000000001.000000e+00")
PRINT_TEST(("%020E", 1.0), "000000001.000000E+00")
PRINT_TEST(("%020f", 1.0), "0000000000001.000000")
PRINT_TEST(("%020g", 1.0), "00000000000000000001")
PRINT_TEST(("%020G", 1.0), "00000000000000000001")

PRINT_TEST(("%e", 1.3), "1.300000e+00")
PRINT_TEST(("%.0e", 1.3), "1e+00")
PRINT_TEST(("%#.0e", 1.3), "1.e+00")
PRINT_TEST(("%.1e", 1.3), "1.3e+00")
PRINT_TEST(("%.2e", 1.3), "1.30e+00")
PRINT_TEST(("%.3e", 1.3), "1.300e+00")
PRINT_TEST(("%.4e", 1.3), "1.3000e+00")
PRINT_TEST(("%.5e", 1.3), "1.30000e+00")
PRINT_TEST(("%.6e", 1.3), "1.300000e+00")
PRINT_TEST(("%.7e", 1.3), "1.3000000e+00")

PRINT_TEST(("%E", 1.3), "1.300000E+00")
PRINT_TEST(("%.0E", 1.3), "1E+00")
PRINT_TEST(("%#.0E", 1.3), "1.E+00")
PRINT_TEST(("%.1E", 1.3), "1.3E+00")
PRINT_TEST(("%.2E", 1.3), "1.30E+00")
PRINT_TEST(("%.3E", 1.3), "1.300E+00")
PRINT_TEST(("%.4E", 1.3), "1.3000E+00")
PRINT_TEST(("%.5E", 1.3), "1.30000E+00")
PRINT_TEST(("%.6E", 1.3), "1.300000E+00")
PRINT_TEST(("%.7E", 1.3), "1.3000000E+00")

PRINT_TEST(("%f", 0.00013), "0.000130")
PRINT_TEST(("%g", 0.00013), "0.00013")
PRINT_TEST(("%.8g", 0.00013), "0.00013")

PRINT_TEST(("%f", 512.0), "512.000000")
PRINT_TEST(("%.0f", 512.0), "512")
PRINT_TEST(("%.1f", 512.0), "512.0")
PRINT_TEST(("%.2f", 512.0), "512.00")
PRINT_TEST(("%.3f", 512.0), "512.000")
PRINT_TEST(("%.4f", 512.0), "512.0000")
PRINT_TEST(("%.5f", 512.0), "512.00000")
PRINT_TEST(("%.6f", 512.0), "512.000000")
PRINT_TEST(("%.7f", 512.0), "512.0000000")
PRINT_TEST(("%.8f", 512.0), "512.00000000")
PRINT_TEST(("%.9f", 512.0), "512.000000000")
PRINT_TEST(("%.10f", 512.0), "512.0000000000")
PRINT_TEST(("%.11f", 512.0), "512.00000000000")
PRINT_TEST(("%.12f", 512.0), "512.000000000000")

PRINT_TEST(("%.3f", 1.999), "1.999")
PRINT_TEST(("%.3f", 1.899), "1.899")
PRINT_TEST(("%.3f", 1.889), "1.889")
PRINT_TEST(("%.3f", 1.888), "1.888")
PRINT_TEST(("%g", 1.008), "1.008")
PRINT_TEST(("%.3e", 1.0), "1.000e+00")
PRINT_TEST(("%.3e", 0.99999), "1.000e+00")
PRINT_TEST(("%.3e", 0.99999e199), "1.000e+199")
PRINT_TEST(("%.3e", 0.99999e200), "1.000e+200")
PRINT_TEST(("%.3e", 0.99999e201), "1.000e+201")
PRINT_TEST(("%.3e", 0.99999e202), "1.000e+202")
PRINT_TEST(("%.3f", 0.00000001), "0.000")
PRINT_TEST(("%.3f", 0.0000001), "0.000")
PRINT_TEST(("%.3f", 0.000001), "0.000")
PRINT_TEST(("%.3f", 0.00001), "0.000")
PRINT_TEST(("%.3f", 0.0001), "0.000")
PRINT_TEST(("%.3f", 0.001), "0.001")
PRINT_TEST(("%.3f", 0.01), "0.010")
PRINT_TEST(("%.3f", 0.1), "0.100")
PRINT_TEST(("%.3f", 1.0), "1.000")
PRINT_TEST(("%.3f", 10.0), "10.000")

PRINT_TEST(("%.3e", 1.0), "1.000e+00")
PRINT_TEST(("%1.3e", 1.0), "1.000e+00")
PRINT_TEST(("%2.3e", 1.0), "1.000e+00")
PRINT_TEST(("%3.3e", 1.0), "1.000e+00")
PRINT_TEST(("%4.3e", 1.0), "1.000e+00")
PRINT_TEST(("%5.3e", 1.0), "1.000e+00")
PRINT_TEST(("%6.3e", 1.0), "1.000e+00")
PRINT_TEST(("%7.3e", 1.0), "1.000e+00")
PRINT_TEST(("%8.3e", 1.0), "1.000e+00")
PRINT_TEST(("%9.3e", 1.0), "1.000e+00")
PRINT_TEST(("%10.3e", 1.0), " 1.000e+00")

PRINT_TEST(("%.3e", -1.0), "-1.000e+00")
PRINT_TEST(("%1.3e", -1.0), "-1.000e+00")
PRINT_TEST(("%2.3e", -1.0), "-1.000e+00")
PRINT_TEST(("%3.3e", -1.0), "-1.000e+00")
PRINT_TEST(("%4.3e", -1.0), "-1.000e+00")
PRINT_TEST(("%5.3e", -1.0), "-1.000e+00")
PRINT_TEST(("%6.3e", -1.0), "-1.000e+00")
PRINT_TEST(("%7.3e", -1.0), "-1.000e+00")
PRINT_TEST(("%8.3e", -1.0), "-1.000e+00")
PRINT_TEST(("%9.3e", -1.0), "-1.000e+00")
PRINT_TEST(("%10.3e", -1.0), "-1.000e+00")
PRINT_TEST(("%11.3e", -1.0), " -1.000e+00")

PRINT_TEST(("%%"), "%")

PRINT_TEST(("a%sb", "ccc"), "acccb");

PRINT_N_TEST(("%na%hn%s%lnb%n", &n1, &n2, "ccc", &n3, &n4), "acccb",
        int n1 = 57; short n2 = 57; long n3 = 57; int n4 = 57,
        (EXPECT(n1, 0), EXPECT(n2, 1), EXPECT(n3, 4), EXPECT(n4, 5)));
