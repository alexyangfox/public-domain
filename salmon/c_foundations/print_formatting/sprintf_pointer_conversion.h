/* file "sprintf_pointer_conversion.h" */

/*
 *  This file contains the interface to one particular algorithm to do pointer
 *  conversion to ASCII.  This algorithm uses the sprintf() function to do the
 *  conversion itself, and provides the conversion through the interface used
 *  by the print formatting module.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef SPRINTF_POINTER_CONVERSION_H
#define SPRINTF_POINTER_CONVERSION_H

#include <stddef.h>
#include "../basic.h"
#include "pointer_plug_in.h"


/*
 *      Usage
 *
 *  This interface provides exactly two function declarations.  Between them,
 *  they provide a pointer conversion algorithm that converts a pointer value
 *  to ASCII characters using the sprintf() function.  The interface to this
 *  module is designed to allow it to be used as a plug-in with the interface
 *  provided by the "pointer_plug_in.h" header file.
 *
 *  The do_sprintf_pointer_conversion() function is the one that does the
 *  actual conversion.  It has the type pointer_plug_in_function_type and its
 *  interface is as specified in the "pointer_plug_in.h" header file. This
 *  function expects a NULL pointer as its first parameter because it doesn't
 *  use any data of its own.
 *
 *  The sprintf_pointer_conversion_max_output_characters() function returns the
 *  maximum number of characters in the output from the
 *  do_sprintf_pointer_conversion() function.
 *
 *  So, to set this module as the plug-in to use,
 *  set_pointer_conversion_plug_in() should be called with
 *  do_sprintf_pointer_conversion() as the first parameter; NULL as the second
 *  parameter; and the result of calling
 *  sprintf_pointer_conversion_max_output_characters() as the third parameter.
 *
 *
 *      Error Handling
 *
 *  As specified in the "pointer_plug_in.h" header, the
 *  do_sprintf_pointer_conversion() function will handle any error condition by
 *  returning MISSION_FAILED.  The
 *  sprintf_pointer_conversion_max_output_characters() function cannot
 *  encounter any error conditions.
 *
 *  Of course, the rest of this section doesn't apply if there are bugs in this
 *  module, if there are bugs in the compiler compiling this module that cause
 *  incorrect code to be generated for it, if external memory corruption bugs
 *  corrupt the data structures or code used by this module, or if pointer
 *  parameters passed to this module are corrupt.  In those cases, all bets are
 *  off and there's no way to guarantee what might happen.
 *
 *
 *      Requirements
 *
 *  This requires only an ANSI C compiler.
 *
 *
 *      Design Goals
 *
 *  The choice of C as the implementation language was dictated by the fact
 *  that I have a number of modules that I've chosen to do in C that could use
 *  this functionality.  The reason for using C in those projects is that it is
 *  so widely used and available, not particularly that C itself is
 *  intrinsically particularly suited for the task.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, in July of 2008 and earlier and
 *  placed in the public domain at that time.  A version of this code was first
 *  developed by me in a different file, "print_formatting.c" and then moved
 *  into this separate header file as a part of breaking up what was originally
 *  a single module.  All of it was developed by me for my own use on my own
 *  time and on hardware that I own.
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
 *          Chris Wilson, 2003, 2007, 2008
 */


extern verdict do_sprintf_pointer_conversion(void *plug_in_data,
        char *output_buffer, void *value_data, size_t *output_byte_count);
extern size_t sprintf_pointer_conversion_max_output_characters(void);


#endif /* SPRINTF_POINTER_CONVERSION_H */
