#!/usr/bin/perl

#
#   This file contains some Perl code that is part of a test suite to test the
#   print_formatting module.
#
#   Written by Chris Wilson.
#
#   This file is hearby placed in the public domain by its author.
#


#
#       Usage
#
#   This program converts a header file containing a set of print formatting
#   tests to a header file containing another set of print formatting tests.
#   All tests in the original set that don't use the %n conversion specifier
#   are converted to use it at the end of the format and confirm that the
#   correct value is written using it.
#
#   This program is rather fragile and sensitive to exactly how the tests are
#   formatted.  It is designed to work with a particular set of tests I am
#   writing and not necessarily any others.  That's because it would take a lot
#   more work to get it to properly parse all possible test cases that are
#   legal in C and it's not worth that effort to me, since I only ever intend
#   to use it with my particular set of tests anyway.
#
#   This program takes exactly one command-line argument, the name of the input
#   file.  It writes its output to standard output.
#
#
#       Requirements
#
#   This program was written using only some of the basic Perl functionality.
#   It was developed using Perl version 5.8.0, but it likely will run under any
#   dialect of Perl 5.  It also uses the ``cat'' basic Linux/Unix utility.  So
#   it is likely to run fine on any Linux system or any modern (1980's or
#   later) flavor of Unix.  It is not likely to run on any version of MS
#   Windows unless Cygwin or some other Unix-like utility package has been
#   installed on that system.
#
#
#       Design Goals
#
#   This is a test program written for a very narrow purpose, to test a
#   particular module that I have written.  So robustness and reusability are
#   not issues for this design.  It is not complex and it doesn't have to be
#   used with many different inputs, so using Perl has few disadvantages and
#   made it easier to do quickly, and, in my opinion, decreased the chances of
#   bugs because the kinds of things this program needs to do fit the set of
#   operations provided by Perl well.
#
#
#       History
#
#   This code was written by me, Chris Wilson, in 2008 and placed in the public
#   domain at that time.  It's entirely new code and isn't based on anything I
#   or anyone else has written in the past.
#
#
#       Legal Issues
#
#   I've written this code from scratch, without using or refering to any other
#   code, on my own equipment and not for hire for anyone else, so I have full
#   legal rights to place it in the public domain.
#
#   I've chosen to put this software in the public domain rather than
#   copyrighting it and using the FSF's GPL or a Berkeley-style ``vanity''
#   license because my personal opinion is that making it public domain
#   maximizes its potential usefulness to others.  Anyone can feel free to use
#   it for any purpose, including with their own proprietary code or with GPL
#   code, without fear of intellectual property issues.  I have no desire to
#   stop anyone else from making money on this code or getting any other
#   advantages they can from it.
#
#   I do request that anyone who finds this software useful let me know about
#   it.  You can drop me e-mail at "Chris Wilson" <chris@chriswilson.info> to
#   let me know how you are using it and what is good and bad about it for you.
#   Bug reports are also appreciated.  Also, if you release a product or
#   software package of some sort that includes this software, I would like you
#   to give me credit in the documentation as appropriate for the importance of
#   my code in your product.  These are requests, not requirements, so you are
#   not legally bound to do them, they're just a nice way to show appreciation.
#
#   Note that even though this software is public domain and there are no
#   copyright laws that limit what you can do with it, other laws may apply.
#   For example, if you lie and claim that you wrote this code when you did
#   not, or you claim that I endorse a product of yours when I do not, that
#   could be fraud and you could be legally liable.
#
#   There is absolutely no warranty for this software!  I am warning you now
#   that it may or may not work.  It may have bugs that cause you a lot of
#   problems.  I disclaim any implied warranties for merchantability or fitness
#   for a particular purpose.  The fact that I have written some documentation
#   on what I intended this software for should not be taken as any kind of
#   warranty that it will actually behave that way.  I am providing this
#   software as-is in the hope that it will be useful.
#
#           Chris Wilson, 2004, 2008
#


if ($ARGV[0] eq "")
  {
    print STDERR "Usage: convert_tests_to_n_suffix.perl <input_file>\n";
    exit 1;
  }

my $contents = `cat $ARGV[0]`;

$contents =~ s/PRINT_TEST\(\(\"([^\"]*)\"((.|\n)*?)\),[ \n]*((\"[^\"]*\"| |\n)+)\)/PRINT_N_TEST\(\(\"$1%n\"$2, &n_var\), $4,\n        int n_var = strlen($4) + 57,\n        (EXPECT(n_var, strlen($4))))/g;

print STDOUT "/* This file was generated automatically\n";
print STDOUT " * by \"convert_tests_to_n_suffix.perl\"\n";
print STDOUT " * from \"$ARGV[0]\". */\n";
print STDOUT "\n";
print STDOUT $contents;

exit 0;
