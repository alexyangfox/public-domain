/* file "code_point.h" */

/*
 *  This file contains some pre-processor directives to define a code_point()
 *  macro.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef CODE_POINT_H
#define CODE_POINT_H


/*
 *      Usage
 *
 *  This file contains some pre-processor directives to define a code_point()
 *  macro.  This macro is used to mark basic blocks.  It takes one argument,
 *  which is expected to be a decimal integer unique to that basic block.  The
 *  idea is to have C files that use this macro mark every basic block with one
 *  use of the code_point() macro and have each one use a different integer
 *  value as the argument.  In addition to one per basic block, it's useful to
 *  add ``else'' blocks just for the code_point macros to ``if'' statements
 *  where the ``else'' would not otherwise be necessary, so that if the ``if''
 *  body is not executed, a code_point() invocation will be hit that would not
 *  be hit if the body was executed.  Also, where different code is compiled
 *  depending on a ``#if*'', it's useful to put a code_point() in each version.
 *
 *  There are two major reasons for using the code_point() macro.  The first
 *  reason is to aid in developing unit tests for the module using the macro.
 *  By building with a definition of the macro that prints the argument the
 *  first time it is executed, one can get a list of all the blocks executed
 *  and make sure the tests cover all the blocks, and know where tests need to
 *  be added to cover additional code if there are some blocks that are not
 *  covered.  This is why it's useful to add the ``else'' blocks everywhere, to
 *  be sure that if all code points are covered, every way an ``if'' test could
 *  go has been tested.  Similarly, putting code_point() invocations in each
 *  case of a ``#if*'' allows one to know whether all the versions of the code
 *  are tested, including those that need to be compiled differently.
 *
 *  The second main reason for the code_point() macro is to aid in
 *  documentation.  In the documentation, one can refer to ``code point 72''
 *  and mean the unique place where code_point(72) appears in that file.
 *  Without this it would be awkward in many cases to try to clearly and
 *  specifically identify part of the code when discussing the internals of the
 *  implementation.  One could use line numbers in the documentation, but then
 *  adding or removing code in the middle would change the line numbers of all
 *  subsequent code and require updating lots of line number in the
 *  documentation to keep it correct.
 *
 *  The code in this file consist of a ``#ifndef code_point'' block.  This
 *  block defines the code_point() macro to evaluate to nothing if the user
 *  hasn't defined it with ``-D'' on the compilation line.  This is the usual
 *  way it should be compiled for production use.  If the user does define
 *  code_point(), though, the code evaluates CODE_POINT_DECLARATION to allow a
 *  user to use another ``-D'' to provide a declaration or declarations to be
 *  used by the code_point() macro, such as an ``extern'' declaration of a
 *  function to call to do tracing.
 *
 *
 *      Error Handling
 *
 *  There are no sorts of error conditions that the code_point macro can detect
 *  and communicate back to the invoking.  In the normal production case,
 *  executing code_point() has absolutely no effect anyway, so no errors can
 *  happen.  In the case that it is compiled so that it makes some kind of
 *  record or trace of code points executed, if the code executed for each code
 *  point encounters some sort of error, it can print out an error message and
 *  then either terminate the program or let it continue.  Since this error
 *  can't impact the rest of the program execution, there's no reason to try to
 *  communicate back to the calling code that there was such an error.
 *
 *
 *      Requirements
 *
 *  This requires only an ANSI C compiler.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, on November 2, 2003 and placed
 *  in the public domain at that time.  I first created this macro definition
 *  within the "string_index.c" file for use within that file.  Over time, I
 *  copied this same code to define the macro into several other C files in the
 *  c_foundations module.  Then, on November 10, 2008, I pulled this macro
 *  definition block out to create this "code_point.h" header file and changed
 *  code that had copies of this block to instead include this header file.
 *
 *
 *      Legal Issues
 *
 *  I've written this code from scratch, without using or refering to any other
 *  code.  I've written it on my own equipment and not for hire for anyone
 *  else, so I have full legal rights to place it in the public domain.
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
 *          Chris Wilson, 2003-2004, 2008
 */


#ifndef code_point
#define code_point(x)
#else
CODE_POINT_DECLARATION
#endif


#endif /* CODE_POINT_H */
