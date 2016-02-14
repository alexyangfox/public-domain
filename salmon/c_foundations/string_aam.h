/* file "string_aam.h" */

/*
 *  This file contains the interface to the string_aam type and supporting
 *  declarations.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef STRING_AAM_H
#define STRING_AAM_H

#include "basic.h"
#include "auto_array.h"


/*
 *      Usage
 *
 *  This file contains the interface to the string_aam type and supporting
 *  declarations.  The string_aam type is a dynamically-resizing array of
 *  strings, where the allocation and de-allocation of the space for the member
 *  strings is taken care of by this module, in addition to the allocation and
 *  de-allocation of the array containing the pointers to the strings.
 *
 *  Note that the string_aam is based on the auto_array module, and the
 *  string_aam interface leaves the undering auto_array interface visible while
 *  adding functionality to allocate and deallocate the member strings.  The
 *  auto_array interface deals with the array of pointers to the string but not
 *  with the allocation and deallocation of memory for the strings themselves.
 *
 *  The string_aam type itself is declared with the AUTO_ARRAY() macro, so it
 *  is a structure of the form declared by this macro from the auto_array
 *  module.  What this module adds is two functions, one to deallocate the
 *  array and all the memory used for its constituent strings, and the other to
 *  both allocate space for a string and add it to the array.
 *
 *  The deallocate_string_aam_and_elements() function deallocates all the
 *  memory associated with a string_aam.  This includes the memory for the
 *  array holding the pointers to the strings and also all the array used for
 *  the strings themselves.  This function takes one argument, a pointer to the
 *  strinct_aam object.  Note that this function does not deallocate the memory
 *  for the container structure itself, just the memory pointed to by the
 *  container.  The container structure itself might well be on the stack in
 *  many cases, in the form of a local variable, so it would be wrong to
 *  deallocate it in this function.  If the memory for the container was
 *  allocated on the heap, it must be deallocated as an additional step.
 *
 *  The duplicate_string_and_add_to_string_aam() function allocates space for
 *  the specified string, copies the string into that space, and adds a pointer
 *  to the newly allocated string to the dynamic array.  The first parameter is
 *  a pointer to the string and the second is a pointer to the string_aam to
 *  which a copy of that string is to be added.
 *
 *
 *      Error Handling
 *
 *  The deallocate_string_aam_and_elements() doesn't return any error code
 *  because no errors can happen that it would be possible for this function to
 *  detect and handle.  It doesn't allocate memory, it only deallocates memory,
 *  and deallocation is never supposed to be able to fail, which is why the
 *  free() function in the standard C library has a void return type.
 *
 *  The duplicate_string_and_add_to_string_aam() function returns a ``verdict''
 *  value to indicate whether or not it succeeded.  The only way it can fail is
 *  if some of the memory allocation it is trying to do fails.
 *
 *  Of course, the previous paragraphs don't apply if there are bugs in this
 *  module, if there are bugs in the compiler compiling this module that cause
 *  incorrect code to be generated for it, or if external memory corruption
 *  bugs corrupt the data structures or code used by this module.  In those
 *  cases, all bets are off and there's no way to guarantee what might happen.
 *
 *
 *      Requirements
 *
 *  This requires only an ANSI C compiler and the "basic.h" and "auto_array.h"
 *  header file.  The "basic.h" header file contains some basic definitions I
 *  use in all my C code, such as a boolean type.  The "auto_array.h" header
 *  file contains macros for generating type and function declarations for
 *  automatically-resizing arrays of arbitrary types.  You should be able to
 *  find those two header files in the same place you found this file.
 *
 *
 *      Design Goals
 *
 *  This module doesn't provide a particularly clean, well-encapsulated
 *  interface.  There were other design goals that were more important.
 *
 *  One goal that drove a lot of the design of this module was the efficency of
 *  the code using this module.  That's why the underlying container structure
 *  is visible to client code -- it can freely directly access the members
 *  instead of having to do it indirectly through function calls.
 *
 *  Another factor in the design of this interface was flexibility.  The client
 *  code has access to all the functions to directly manipulate the pointers in
 *  the array that come from the AUTO_ARRAY() macro.  This has the disadvantage
 *  of less safety because the client code can muck around at a lower level,
 *  but the advantage that the client code can mix use of the higher-level
 *  functions in this module and lower-level access, for example by directly
 *  adding a string that it has already allocated somewhere else into the array
 *  in some cases while using the higher-level functions added here to add
 *  other strings for which space was not already allocated and/or to handle
 *  deallocation.
 *
 *  Yet another factor in the design was the ability to re-use code.  Since any
 *  data structure created by this module has the same type as a data structure
 *  created by directly using the underlying AUTO_ARRAY() interface directly,
 *  code that just reads the data structure but doesn't change it can be
 *  shared.  For example, one might write a routine to walk over a list of
 *  strings and print them all out.  The way this module is designed, such a
 *  printing function can be written once to deal both with data structures
 *  created and modified by this module's higher-level functions as well as
 *  data structures using the AUTO_ARRAY() interface to create arrays of
 *  statically-allocated strings.  C's limited type system makes this difficult
 *  without re-using the same type for the case of statically allocated strings
 *  and dynamically allocated strings.
 *
 *  The choice of C as the implementation language was dictated by the fact
 *  that I have a couple of projects that I've chosen to do in C that could use
 *  this functionality.  The reason for using C in those projects is that it is
 *  so widely used and available, not particularly that C itself is
 *  intrinsically particularly suited for the task.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, in 2008 and placed in the public
 *  domain at that time.  I first created the code in this header file within
 *  the "generate_makefile.c" file for use within that file, but with the idea
 *  that if I found it useful I might pull it out into a header file to be used
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


AUTO_ARRAY(string_aam, char *);


extern void deallocate_string_aam_and_elements(string_aam *the_string_aam);
extern verdict duplicate_string_and_add_to_string_aam(const char *to_add,
                                                      string_aam *array);


#endif /* STRING_AAM_H */
