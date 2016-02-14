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
#   This program is the driver for a suite of tests for the print_formatting
#   module.  Before it can be used, the executable that calls the C code tests
#   must be built, using the Makefile and several C source and header files.
#   The path to this executable must be provided to this program as its first
#   command-line argument.  This program then runs that executable multiple
#   times to run the various tests and compares the results with the expected
#   results.
#
#   This program takes exactly two command-line arguments -- the path to the
#   executable and the name of a log file.  A coverage trace from all the tests
#   that are run is written to the log file.  This program doesn't look at the
#   log file, but it is expected that after this program is done, the log file
#   will be used to verify the coverage of the tests.
#
#
#       Requirements
#
#   This program was written using only some of the basic Perl functionality.
#   It was developed using Perl version 5.8.0, but it likely will run under any
#   dialect of Perl 5.  It also uses the ``rm'' basic Linux/Unix utility and
#   sh/bash-style shell piping syntax.  So it is likely to run fine on any
#   Linux system or any modern (1980's or later) flavor of Unix.  It is not
#   likely to run on any version of MS Windows unless Cygwin or some other
#   Unix-like utility package has been installed on that system.
#
#   Also, as mentioned above, this program requires that the executable
#   specified as the first parameter on the command line has been built.
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
#   This code was written by me, Chris Wilson, in 2003, 2004, and 2008 and
#   placed in the public domain at that time.  It's entirely new code and isn't
#   based on anything I or anyone else has written in the past.
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


sub run_test; # ($executable_name, $test_name, $log_file) -> $stderr_result
sub run_all_tests; # ($executable_name, $log_file) -> void


my @test_table =
  (
    "1", 0,
        "^",
    "2", 0,
        "^",
    "3", 0,
        "^",
  );


if ($ARGV[1] eq "")
  {
    print STDERR "Usage: run_test_print_formatting.perl <executable> " .
                 "<logfile>\n";
    exit 1;
  }
`rm -f $ARGV[1]`;
run_all_tests($ARGV[0], $ARGV[1]);
print STDERR "All tests of the print_formatting module passed.\n";
exit 0;


sub run_test # ($executable_name, $test_name, $log_file) -> $stderr_result
  {
    my $executable_name = $_[0];
    my $test_name = $_[1];
    my $log_file = $_[2];

    return `$executable_name $test_name 2>&1 >> $log_file`;
  }

sub run_all_tests # ($executable_name, $log_file) -> void
  {
    my $executable_name = $_[0];
    my $log_file = $_[1];

    my @remaining_tests = @test_table;

    while ($#remaining_tests >= 0)
      {
        if ($#remaining_tests < 2)
          {
            print STDERR "ERROR: Bad test_table in test framework -- its " .
                         "element count is not a multiple of three.\n";
            exit(1);
          }
        my $test_name = shift(@remaining_tests);
        my $expected_return_code = shift(@remaining_tests);
        my $expected_stderr_pattern = shift(@remaining_tests);
        my $stderr_result = run_test($executable_name, $test_name, $log_file);
        if ($expected_return_code != $?)
          {
            print STDERR "ERROR: When running test `$test_name', the return " .
                         "code was $? when $expected_return_code was " .
                         "expected.\n";
            exit(1);
          }
        if (!($stderr_result =~ m/$expected_stderr_pattern/))
          {
            print STDERR "ERROR: When running test `$test_name', the output " .
                         "was `$stderr_result', which does not match the " .
                         "expected pattern `$expected_stderr_pattern'.\n";
            exit(1);
          }
      }
  }
