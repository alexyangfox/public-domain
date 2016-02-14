/* file "try.h" */

/*
 *  This file contains some macros to help handling error conditions.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef TRY_H
#define TRY_H

#include "basic.h"


/*
 *      Usage
 *
 *  This file provides several macros to make checking for and handling error
 *  conditions slightly easier in C.
 *
 *  The DO_ABORT() macro takes one argument, which should be an expression.  It
 *  declares a label called ``abort'' and after that label creates a return
 *  statement returning the value specified by its parameter.  The idea is that
 *  then this ``abort'' target can be jumped to whenever an error condition is
 *  detected and the argument expression will be a return value indicating an
 *  error.  For example DO_ABORT(MISSION_FAILED) might be useful in a function
 *  returning a ``verdict'' value and DO_ABORT(NULL) might be useful in a
 *  function returning a pointer value.
 *
 *  The CLEANUP_ISLAND() macro takes three arguments and generates code to
 *  handle some error condition.  The first argument is a name of a label that
 *  this macro will generate at the start of the error handling code.  The
 *  second argument should be the name of a label that should be jumped to
 *  after the cleanup is complete.  And the third argument is the code to
 *  actually do the error handling.  The word ``island'' is used in the name
 *  because if any code reaches the macro without jumping to its label, it will
 *  skip the cleanup and do nothing.  So it's not really connected to the code
 *  immediately before or after it -- it's an island.
 *
 *  The TRY() macro takes two arguments and generates code to call a function,
 *  check its return value, and jump to error handling code if and only if the
 *  call return value indicates an error.  The first argument is the label to
 *  be jumped to in the case that the call fails.  The second argument is the
 *  call expression itself.  The call is assumed to return a value of type
 *  ``verdict''.
 *
 *  Here's a complete example using all three of these macros:
 *
 *      TRY(abort, init_a());
 *      CLEANUP_ISLAND(cleanup_a, abort, deinit_a());
 *
 *      TRY(abort, init_b());
 *      CLEANUP_ISLAND(cleanup_b, cleanup_a, deinit_b());
 *
 *      TRY(abort, init_c());
 *      CLEANUP_ISLAND(cleanup_c, cleanup_b, deinit_c());
 *
 *      TRY(abort, init_d());
 *
 *      return MISSION_ACCOMPLISHED;
 *
 *      DO_ABORT(MISSION_FAILED);
 *
 *  In this example, we want to call init_*() for each of ``a'', ``b'', ``c'',
 *  and ``d'' in this order.  But if any fails, we want to clean up the
 *  initialization we did for the previous ones by calling deinit_*().  The use
 *  of these macros allows us to cluser the calls to init_*() and deinit_*()
 *  code for each letter in one place instead.  Then, if we want to insert
 *  another one in between two existing ones, or delete an existing one, it's
 *  easy to do and completely local.
 *
 *
 *      Error Handling
 *
 *  These macros are all about error handling, and how errors are handled is
 *  what the macros are all about, and is therefor completely defined in the
 *  ``Usage'' section.
 *
 *
 *      Requirements
 *
 *  This requires only an ANSI C compiler and the "basic.h" header file.  The
 *  "basic.h" header file contains some basic definitions I use in all my C
 *  code, such as a boolean type.  You should be able to find "basic.h" in the
 *  same place you found this file.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, in 2008 and placed in the public
 *  domain at that time.  I first created the macros here within the
 *  "generate_makefile.c" file for use within that file, but with the idea that
 *  if I found them useful I might pull them out into a header file to be used
 *  elsewhere in the future, and that's exactly what happened.
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
 *          Chris Wilson, 2003, 2008
 */


#define DO_ABORT(result) \
    abort: \
        return result;

#define CLEANUP_ISLAND(entry_label, next_label, action) \
        goto skip_ ## entry_label; \
    entry_label: \
        action; \
        goto next_label; \
    skip_ ## entry_label:

#define TRY(fail_label, call) \
      { \
        verdict the_verdict; \
 \
        the_verdict = call; \
        if (the_verdict != MISSION_ACCOMPLISHED) \
            goto fail_label; \
      }


#endif /* TRY_H */
