/* file "floating_point_plug_in.h" */

/*
 *  This file contains an interface to allow different modules to be plugged in
 *  to do floating-point value conversion to decimal for formatted printing.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef FLOATING_POINT_PLUG_IN_H
#define FLOATING_POINT_PLUG_IN_H

#include <stddef.h>
#include "../basic.h"
#include "floating_point_output_conversion.h"


/*
 *      Usage
 *
 *  This interface allows different modules to be plugged in to do conversion
 *  from floating-point values to decimal digits for the sake of code to do
 *  formatted printing.
 *
 *  This interface exists to allow print formatting code to have a modular
 *  interface to the code that does the conversion between floating-point
 *  values and decimal digits.  Converting between floating-point values and
 *  decimal representations of them is complex.  Different machine
 *  architectures can have different implementations of floating-point, and
 *  even for a given implementation, different algorithms can be used for
 *  conversion, with different strengths and weaknesses.  Since the actual
 *  conversion algorithms themselves are more complex and can be system-
 *  dependent, it makes sense to make a modular interface so that the rest of
 *  the print formatting code can be written once and not changed for different
 *  algorithms for floating-point conversion to digits.  The generic print
 *  formatting code calls an algorithm-specific function to do the conversion
 *  of a floating-point value to decimal digits.  The interface in this file
 *  allows the conversion algorithm to be specified and changed.
 *
 *  The heart of this interface is the type definition of the
 *  floating_point_plug_in_function_type function type.  Each conversion
 *  algorithm should have this type.
 *
 *  The floating_point_plug_in_function_type function type has ``verdict'' as
 *  its return type because each conversion algorithm should return a value of
 *  this type to indicate the success or failure of its attempted conversion.
 *
 *  The first argument of the floating_point_plug_in_function_type function
 *  type is a void pointer called plug_in_data.  This parameter is used to pass
 *  to the function a pointer to data that is registered along with the plug in
 *  function itself.  This makes the plug-in interface object-oriented -- not
 *  just a function, but data to go along with that function, is registered,
 *  and then both can be used together to do the conversion.  It is up to the
 *  creator of the algorithm whether to use this data and what to use it for.
 *  The core print formatting code never looks at this pointer, it simply
 *  passes it to the conversion function whenever the function is called.  The
 *  pointer may be NULL or non-NULL.
 *
 *  The second argument is the output_control parameter.  This is a handle that
 *  the conversion algorithm should use to communicate its results.  The
 *  "floating_point_output_conversion.h" header file provides the interface
 *  that the conversion algorithm should use to communicate its results, and
 *  the output_control parameter will be needed for all the functions in that
 *  interface.
 *
 *  The next argument is the third argument, which is the
 *  requested_mantissa_digit_count parameter.  As the name implies, this
 *  specifies to the conversion algorithm how many decimal digits of mantissa
 *  data are required by the print formatting code.  The conversion algorithm
 *  is required to provide this many decimal digits of mantissa data.
 *
 *  Next is the care_about_trailing_zero_count argument.  This argument is a
 *  boolean that specifies whether or not the print formatting code cares about
 *  trailing zeros.  It is optional whether or not the conversion algorithm
 *  uses this hint, but if it wants to, it can use this hint to be more
 *  efficient.  If the print formatting code does care about the trailing zero
 *  count, and the conversion algorithm can efficiently compute that and tell
 *  the print formatting code early, the print formatting code can avoid some
 *  buffering in some cases.  But if the print formatting code doesn't care
 *  about the trailing zero count, it can be more efficient for the conversion
 *  algorithm to skip computing the trailing zero count.
 *
 *  The next parameter is the type_kind parameter.  Its type is
 *  floating_point_type_kind, which is an enumerated type defined in this
 *  interface.  The enumeration lists the three standard C floating-point
 *  types.  The parameter specifies which of those three standard C
 *  floating-point types the value to be converted has.
 *
 *  Next comes the value_data parameter.  This is a void pointer that points to
 *  the floating-point value to be converted.  The type of value it points to
 *  is specified by the type_kind parameter.
 *
 *  The final parameter is the mantissa_is_negative parameter.  This is a
 *  boolean that, as its name implies, specifies whether or not the mantissa is
 *  negative.
 *
 *  The remainder of this interface is two functions.  One function sets a
 *  particular plug-in to use for floating-point conversions and the other gets
 *  the currently set plug-in.  Note that each plug-in consists of a pointer to
 *  the function (of type floating_point_plug_in_function_type), a void pointer
 *  for any private data for that function, and a boolean specifying whether or
 *  not that function does rounding itself.  If the function does rounding
 *  itself, the generic framework doesn't have to do rounding, but if the
 *  function itself does not do rounding, the generic framework will ask the
 *  conversion function for an extra digit of mantissa data so it can do the
 *  rounding itself.
 *
 *  The set_floating_point_conversion_plug_in() function is the one that sets
 *  the conversion function to be used.  It takes a pointer to the function, a
 *  pointer to its data, and a boolean specifying whether or not the conversion
 *  function does rounding.  It returns a verdict value specifying whether or
 *  not the registering of the conversion function succeeded.
 *
 *  The get_floating_point_conversion_plug_in() function is the one that
 *  fetches the currently set plug-in data.  It takes three pointers as
 *  parameters.  The locations these pointers point to are filled with the
 *  values for the current plug-in -- the function pointer, the plug-in data
 *  pointer, and the boolean specifying whether or not the currently registered
 *  function does rounding, respectively.  This function returns a verdict
 *  value specifying whether or not it was successful in gathering and
 *  returning that information.
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
 *  This code was written by me, Chris Wilson, in April and May of 2008 and
 *  earlier and placed in the public domain at that time.  A version of some of
 *  this code was first developed by me in a different file,
 *  "print_formatting.c" and then moved into this separate header file as a
 *  part of breaking up what was originally a single module.  All of it was
 *  developed by me for my own use on my own time and on hardware that I own.
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


typedef enum
  {
    FPTK_FLOAT, FPTK_DOUBLE, FPTK_LONG_DOUBLE
  } floating_point_type_kind;

typedef verdict floating_point_plug_in_function_type(void *plug_in_data,
        floating_point_output_control *output_control,
        size_t requested_mantissa_digit_count,
        boolean care_about_trailing_zero_count,
        floating_point_type_kind type_kind, void *value_data,
        boolean mantissa_is_negative);


extern verdict set_floating_point_conversion_plug_in(
        floating_point_plug_in_function_type *plug_in_function,
        void *plug_in_data, boolean rounding_done_in_plug_in);
extern verdict get_floating_point_conversion_plug_in(
        floating_point_plug_in_function_type **plug_in_function,
        void **plug_in_data, boolean *rounding_done_in_plug_in);


#endif /* FLOATING_POINT_PLUG_IN_H */
