#!/usr/bin/perl

#
#   This file contains some Perl code to determine which of a set of object
#   files a given object file depends upon.
#
#   Written by Chris Wilson.
#
#   This file is hearby placed in the public domain by its author.
#


#
#       Usage
#
#   This program determines which object files from a list of object files
#   contain symbols that are referenced by a given object file.  This is
#   intended to help automate generation and maintenance of makefiles.
#
#   This program is not intended to be particularly robust or portable, because
#   it would take more effort to make it robust and portable, and it's not
#   currently worth the effort to me.  Since it's only used in the generation
#   and maintenance of makefiles, this script doesn't need to be run to build
#   any code, just to generate and maintain the makefiles.  If I can use it
#   successfully on my own platform, I can generate and maintain the makefiles
#   and then distribute those makefiles with the code when I distribute it.
#   Anyone else who gets the code and makes substantial enough changes to
#   affect the dependences between object files will have to either manually
#   edit the makefiles or get this script or a similar script working in that
#   person's own development environment.
#
#   The primary reason that this program is not portable is that it uses
#   ``objdump'' and counts on the format of its output.  This program expects
#   GNU objdump, and a version whose output is of the form as the version I
#   developed this with.  Other versions of ``objdump'' may break this program.
#
#   On the other hand, GNU ``objdump'' has been around for a while and there
#   are probably many scripts around the world that count on its output format,
#   so it's likely that future versions will keep the same output format.  And
#   the GNU version is likely to be the one used on any Linux system.  Even on
#   non-Linux systems, its not unlikely that GNU ``objdump'' will be available
#   one way or another.
#
#   Another portability issue is that on systems with different object file
#   formats, the output of even GNU ``objdump'' may be different.  So while
#   this script is likely to work on Linux systems, it may not on other systems
#   even with a similar version of GNU ``objdump''.
#
#   Note also that this script is hard-coded with lists of certain symbols from
#   libc (the standard C library) and libm (the standard C math library) that
#   are used by the code this script is intended to be used on.  They're not
#   comprehensive lists, so if this code is modified or this script is used on
#   new code, the lists of symbols for those two libraries might have to be
#   modified for this script to continue functioning properly.  And the symbols
#   might be entirely different on non-Linux systems -- for example, the symbol
#   "__assert_fail" is in this list, and that symbol is used by the assert()
#   macro with gcc on Linux, but on other systems a different symbol might well
#   be used by the assert() macro.
#
#   Finally, note that this code is in Perl, a language that does not lend
#   itself to robust code.  So I consider this code more likely to have hidden
#   bugs than most code I write in other languages.  Also, the fact that it has
#   to make system-dependent calls to ``objdump'' makes it less likely to be
#   robust.
#
#   So, this code should be considered to be special-purpose code created and
#   used for a specific, narrow set of circumstances, not code that can be
#   counted on to be re-used for other purposes.
#
#
#       Requirements
#
#   This program was written using only some of the basic Perl functionality.
#   It was developed using Perl version 5.8.0, but it likely will run under any
#   dialect of Perl 5.  It also uses GNU ``objdump''.  It was developed and
#   tested using version 2.13 of GNU ``objdump'' on Linux and may or may not
#   work properly on other versions of ``objdump'' or other operating systems.
#   It is not likely to run on any version of MS Windows unless Cygwin or some
#   other Unix-like utility package has been installed on that system.
#
#
#       Design Goals
#
#   Using Perl to write this program made it much faster to implement, and
#   that's why I wrote it in Perl.  Since it involves running another program
#   (``objdump'') and doing various string manipulations on the output from
#   several calls to that program, Perl lent itself well to this program.
#
#
#       History
#
#   This code was written by me, Chris Wilson, in 2008 and placed in the public
#   domain at that time.  It was not based on anything else that I or anyone
#   else wrote in any form.  All of it is original to me.
#
#
#       Legal Issues
#
#   I wrote all of this code from scratch without using or refering to any
#   other code.  It was produced for my own use on my own equipment, not for
#   hire for anyone else, so I have full legal rights to place this code in the
#   public domain.
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


sub objdump ($);
sub find_defined_symbols ($);
sub find_undefined_symbols ($);


my @c_lib_symbols =
  (
    "__assert_fail",
    "__ctype_b_loc",
    "__errno_location",
    "exit",
    "fclose",
    "feof",
    "ferror",
    "fopen",
    "fprintf",
    "fread",
    "free",
    "malloc",
    "memcmp",
    "memcpy",
    "memset",
    "printf",
    "sprintf",
    "stderr",
    "stdout",
    "strcmp",
    "strcpy",
    "strerror",
    "strlen",
    "strncmp",
    "vfprintf",
    "vsprintf"
  );

my @m_lib_symbols =
  (
    "floor",
    "log10",
    "pow"
  );


if ($#ARGV < 1)
  {
    print STDERR "Usage: find_object_dependences.perl <primary-object-file> " .
            "{ <other-object-file> }* { -e { <expected-object-file> }* }?\n";
    exit 1;
  }

my $have_expected = 0;
my @other_object_files;
my @expected_object_files = @ARGV;
my $primary_object_file = shift(@expected_object_files);
while ($#expected_object_files >= 0)
  {
    my $argument = shift(@expected_object_files);

    if ($argument eq "-e")
      {
        $have_expected = 1;
        last;
      }

    push(@other_object_files, $argument);
  }

my %symbol_table;

for $other_object_file (@other_object_files)
  {
    my @defined_symbols = find_defined_symbols($other_object_file);
    for $symbol_name (@defined_symbols)
      {
        $symbol_table{$symbol_name} = $other_object_file;
      }
  }

for $symbol_name (@c_lib_symbols)
  {
    $symbol_table{$symbol_name} = "-lc";
  }

for $symbol_name (@m_lib_symbols)
  {
    $symbol_table{$symbol_name} = "-lm";
  }

my @undefined_symbols = find_undefined_symbols($primary_object_file);
my @unresolved_symbols;
my @used_file_array;
my %used_file_index;

for $symbol (@undefined_symbols)
  {
    my $lookup = $symbol_table{$symbol};
    if (length($lookup) <= 0)
      {
        push(@unresolved_symbols, $symbol);
      }
    elsif ($used_file_index{$lookup} ne "yes")
      {
        push(@used_file_array, $lookup);
        $used_file_index{$lookup} = "yes";
      }
  }

my $return_code = 0;

if ($have_expected == 0)
  {
    for $file (@used_file_array)
      {
        print STDOUT $file . "\n";
      }
  }
else
  {
    my %used_index;
    my %expected_index;

    for $file (@used_file_array)
      {
        $used_index{$file} = "yes";
      }
    for $file (@expected_object_files)
      {
        $expected_index{$file} = "yes";
      }

    for $file (@used_file_array)
      {
        if ($expected_index{$file} ne "yes")
          {
            print STDERR "ERROR: $file is used but not expected.\n";
            $return_code = 1;
          }
      }
    for $file (@expected_object_files)
      {
        if ($used_index{$file} ne "yes")
          {
            print STDERR "ERROR: $file is expected but not used.\n";
            $return_code = 1;
          }
      }
  }

if ($#unresolved_symbols != -1)
  {
    print STDERR "ERROR: Unresolved symbols found:\n";
    for $symbol (@unresolved_symbols)
      {
        print STDERR "    $symbol\n";
      }
    $return_code = 1;
  }

exit $return_code;


sub objdump ($)
  {
    my $object_file_name = @_[0];

    return `objdump --syms $object_file_name`;
  }

sub find_defined_symbols ($)
  {
    my $object_file_name = @_[0];
    my @result;

    my @objdump_lines = objdump($object_file_name);
    for $line (@objdump_lines)
      {
        $line =~ s/\n//g;
        my @words = split(/[\ \t][\ \t]*/, $line);
        if ($#words != 5)
          {
            next;
          }
        push(@result, $words[5]);
      }

    return @result;
  }

sub find_undefined_symbols ($)
  {
    my $object_file_name = @_[0];
    my @result;

    my @objdump_lines = objdump($object_file_name);
    for $line (@objdump_lines)
      {
        $line =~ s/\n//g;
        my @words = split(/[\ \t][\ \t]*/, $line);
        if ($#words != 3)
          {
            next;
          }
        if ($words[1] ne "*UND*")
          {
            next;
          }
        push(@result, $words[3]);
      }

    return @result;
  }
