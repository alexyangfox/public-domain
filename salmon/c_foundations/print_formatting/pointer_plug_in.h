/* file "pointer_plug_in.h" */

/*
 *  This file contains an interface to allow different modules to be plugged in
 *  to do pointer value conversion to ASCII for formatted printing.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef POINTER_PLUG_IN_H
#define POINTER_PLUG_IN_H

#include <stddef.h>
#include "../basic.h"


/*
 *      Usage
 *
 *  This interface allows different modules to be plugged in to do conversion
 *  from pointer values to ASCII characters for the sake of code to do
 *  formatted printing.
 *
 *  This interface exists to allow print formatting code to have a modular
 *  interface to the code that does the conversion between pointer values and
 *  ASCII characters.  The desirable representation of pointers as ASCII
 *  characters can vary widely from one machine architecture to another.  On
 *  many machines, there is a flat address space, so pointers are typically
 *  represented in ASCII with a fixed number of hexidecimal digits.  But other
 *  machines use segmented pointers that are conventionally represented in
 *  ASCII differently.  To allow the main print formatting code to be system-
 *  independent, it is useful to have a modular interface so that the rest of
 *  the print formatting code can be written once and not changed for different
 *  algorithms for pointer conversion to ASCII.  The generic print formatting
 *  code calls a system-specific function to do the conversion of a pointer
 *  value to ASCII characters.  The interface in this file allows the
 *  conversion algorithm to be specified and changed.
 *
 *  The heart of this interface is the type definition of the
 *  pointer_plug_in_function_type function type.  Each conversion algorithm
 *  should have this type.
 *
 *  The pointer_plug_in_function_type function type has ``verdict'' as its
 *  return type because each conversion algorithm should return a value of this
 *  type to indicate the success or failure of its attempted conversion.
 *
 *  The first argument of the pointer_plug_in_function_type function type is a
 *  void pointer called plug_in_data.  This parameter is used to pass to the
 *  function a pointer to data that is registered along with the plug in
 *  function itself.  This makes the plug-in interface object-oriented -- not
 *  just a function, but data to go along with that function, is registered,
 *  and then both can be used together to do the conversion.  It is up to the
 *  creator of the algorithm whether to use this data and what to use it for.
 *  The core print formatting code never looks at this pointer, it simply
 *  passes it to the conversion function whenever the function is called.  The
 *  pointer may be NULL or non-NULL.
 *
 *  The second argument is the output_buffer parameter.  This argument tells
 *  the function where to write its results.  It points to an array into which
 *  the ASCII characters representing the function should be written, in the
 *  order in which they should be printed out.
 *
 *  Next comes the value_data parameter.  This is a void pointer that is the
 *  pointer value to be converted.  Note that the function should never try to
 *  follow this pointer.  Instead, it uses the value in the parameter itself in
 *  the conversion.
 *
 *  The final parameter is the output_byte_count parameter.  This is a pointer
 *  to a location into which the function should write the number of ASCII
 *  characters used to represent the pointer value.  It is this number of
 *  characters that the function wrote into the array pointed to by the
 *  output_buffer parameter.  The output_byte_count pointer must never be NULL
 *  -- the caller is responsible for setting it to a location that will end up
 *  containing the character count.
 *
 *  The remainder of this interface is two functions.  One function sets a
 *  particular plug-in to use for pointer conversions and the other gets the
 *  currently set plug-in.  Note that each plug-in consists of a pointer to the
 *  function (of type pointer_plug_in_function_type), a void pointer for any
 *  private data for that function, and a size_t value specifying the maximum
 *  number of ASCII characters used to represent a pointer by that plug-in.
 *  The caller to the plug-in function is responsible for making sure the
 *  output_buffer parameter points to a buffer with at least as many elements
 *  as this specified maximum number of ASCII characters.
 *
 *  The set_pointer_conversion_plug_in() function is the one that sets the
 *  conversion function to be used.  It takes a pointer to the function, a
 *  pointer to its data, and a size_t specifying the maximum number of output
 *  characters.  It returns a verdict value specifying whether or not the
 *  registering of the conversion function succeeded.
 *
 *  The get_pointer_conversion_plug_in() function is the one that fetches the
 *  currently set plug-in data.  It takes three pointers as parameters.  The
 *  locations these pointers point to are filled with the values for the
 *  current plug-in -- the function pointer, the plug-in data pointer, and the
 *  size_t specifying the maximum number of output characters, respectively.
 *  This function returns a verdict value specifying whether or not it was
 *  successful in gathering and returning that information.
 *
 *
 *      Error Handling
 *
 *  An error by either of the two functions specified by this interface is
 *  handled by returning MISSION_FAILED as its return value.  An error by the
 *  function being registered, if and when it is called, should be signalled by
 *  returning MISSION_FAILED to its caller as its return value.
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
 *  placed in the public domain at that time.  A version of some of this code
 *  was first developed by me in a different file, "print_formatting.c" and
 *  then moved into this separate header file as a part of breaking up what was
 *  originally a single module.  Also, I used the file
 *  "floating_point_plug_in.h" -- another file I developed on my own -- as a
 *  template for developing this file.  All of it was developed by me for my
 *  own use on my own time and on hardware that I own.
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


typedef verdict pointer_plug_in_function_type(void *plug_in_data,
        char *output_buffer, void *value_data, size_t *output_byte_count);


extern verdict set_pointer_conversion_plug_in(
        pointer_plug_in_function_type *plug_in_function, void *plug_in_data,
        size_t max_output_characters);
extern verdict get_pointer_conversion_plug_in(
        pointer_plug_in_function_type **plug_in_function, void **plug_in_data,
        size_t *max_output_characters);


#endif /* POINTER_PLUG_IN_H */
