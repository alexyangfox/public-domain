/* file "floating_point_output_conversion.h" */

/*
 *  This file contains an interface for use by code to do floating-point
 *  conversion to decimal for formatted printing.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef FLOATING_POINT_OUTPUT_CONVERSION_H
#define FLOATING_POINT_OUTPUT_CONVERSION_H

#include <stddef.h>
#include "../basic.h"


/*
 *      Usage
 *
 *  This interface is intended for only a very narrow purpose -- use by modules
 *  that do the conversion from floating-point values to decimal digits for the
 *  use of code to do formatted printing.
 *
 *  This interface exists to allow print formatting code to have a modular
 *  interface to the code that does the conversion between a floating-point
 *  value and decimal digits.  Converting between floating-point values and
 *  decimal representations of them is complex.  Different machine
 *  architectures can have different implementations of floating-point, and
 *  even for a given implementation, different algorithms can be used for
 *  conversion, with different strengths and weaknesses.  Since the actual
 *  conversion algorithms themselves are more complex and can be system-
 *  dependent, it makes sense to make a modular interface so that the rest of
 *  the print formatting code can be written once and not changed for different
 *  algorithms for floating-point conversion to digits.  The generic print
 *  formatting code calls an algorithm-specific function to do the conversion
 *  of a floating-point value to decimal digits and provides the interface in
 *  this file for the algorithm to use to communicate information back to the
 *  generic print formatting code as it goes along.
 *
 *  This interface consists of the declaration of one type and eight functions.
 *
 *  The type is the floating_point_output_control structure type.  A pointer to
 *  an object of this type is passed to the conversion algorithm when it is
 *  invoked and that same pointer is expected to be passed to each of the
 *  functions in this interface by the conversion algorithm if and when they
 *  are called.  The conversion algorithm should never look inside this object,
 *  and that is why the type is only declared, not defined.  It exists for
 *  communication between the code that invoked the conversion algorithm and
 *  the functions declared in this interface.  That is why each of the
 *  functions declared here takes as its first argument a pointer to an object
 *  of this type with the name output_control.
 *
 *  Note that the typedef of the floating_point_output_control structure is
 *  protected by a #ifndef block.  This is because the same type is also
 *  defined in the "floating_point_output_caller.h" header file, and if both
 *  header files are included by the same code, there will be a conflict unless
 *  there is a mechanism to avoid it, because C doesn't allow two typedefs of
 *  the same name, even if they are identical typedef statements.  The #ifndef
 *  block, in conjunction with an identical #ifndef block in the other header
 *  file, insures that the typedef is used if and only if it hasn't appeared
 *  already.
 *
 *  Assuming the value to be converted is a valid finite floating-point numeric
 *  value, the conversion algorithm is expected to first call one or the other
 *  of the two early exponent functions (early_exponent_by_size_t() or
 *  early_exponent_by_digits()) but not both, and not more than once.  Then, it
 *  is expected to call handle_mantissa_digit() once for each decimal digit of
 *  the mantissa, from the most to the least significant.  Then, it is expected
 *  to call one or the other of the two late exponent functions
 *  (late_exponent_by_size_t() or late_exponent_by_digits()) but not both, and
 *  not more than once.  If the algorithm runs into problems because the number
 *  of mantissa digits requested is too large for it to handle (for example, if
 *  it needs to allocate a buffer but can't allocate a buffer large enough), it
 *  should call too_many_floating_point_mantissa_digits() and then return.  As
 *  a short cut, it may -- but need not -- call
 *  notify_of_mantissa_trailing_zero_count() at any time between the early
 *  exponent and late exponent function calls.  This is to notify the generic
 *  code how many trailing zeros there are.  If this function is called,
 *  handle_mantissa_digit() should not be called for any of the trailing zeros
 *  specified by the call to notify_of_mantissa_trailing_zero_count().
 *
 *  That's all assuming the value to be converted is a valid finite floating-
 *  point numeric value.  In some floating-point implementations, other values
 *  -- for example, positive infinity, negative infinity, and ``not a number''
 *  -- might be representable in the floating-point system.  If the conversion
 *  algorithm detects such a case, it should make only one call, to
 *  special_non_numeric_value().  It should pass to this function an ASCII
 *  string representation of the value.  This ASCII string will be printed as
 *  the formatted version of the value.  If the conversion algorithm calls
 *  special_non_numeric_value(), it shouldn't call any of the other functions
 *  in this interface and it should return directly after calling that
 *  function.
 *
 *  Note that there are two versions each of the early and late exponent
 *  functions.  That's so that the algorithm can specify the exponent's
 *  magnitude either with a size_t integer value or with an ASCII decimal
 *  digits string.  The size_t option is more efficient, but is limited to
 *  exponent magnitudes of a particular size.  The ASCII string version is less
 *  efficient but unlimited.  The algorithm is free to use either form.
 *  Obviously, if the exponent magnitude won't fit in a size_t, it must use the
 *  other version, but otherwise it can use either one, even using the string
 *  form when the value would fit in a size_t.
 *
 *  All but one of the functions has ``verdict'' as its return value.  As is
 *  standard for functions with that return type, each of these functions
 *  returns MISSION_ACCOMPLISHED if it ran without error and MISSION_FAILED on
 *  error.  On error, the calling algorithm should abort and return an error
 *  code to its caller.  The only exception is the
 *  too_many_floating_point_mantissa_digits() function, which has a void return
 *  type, because that function does not need to return an error code -- it is
 *  only called in case of an error anyway, and it is expected that after it is
 *  called, an error code will be returned to the caller of the algorithm in
 *  any case.
 *
 *  The interface to the handle_mantissa_digit() function is fairly
 *  straightforward.  In addition to the output_control parameter, it takes a
 *  ``digit'' parameter of type char.  This parameter should be the ASCII
 *  version of the appropriate decimal mantissa digit -- for example, the
 *  character '3' instead of the raw value 3.
 *
 *  The early_exponent_by_size_t() function takes -- in addition to the usual
 *  output_control parameter -- a boolean mantissa_is_negative to tell whether
 *  or not the mantissa is negative; a boolean exponent_is_negative to tell the
 *  same thing about the exponent; and a size_t exponent_magnitude to specify
 *  the magnitude of the decimal exponent.  In addition, it is expected to pass
 *  a pointer in the new_requested_mantissa_digit_count parameter to a size_t
 *  location of its own into which the early_exponent_by_size_t() function will
 *  place an updated number of mantissa digits that will be required.  When the
 *  algorithm was first invoked, it was given a number of mantissa digits
 *  required, but based on the exponent information this might be modified,
 *  which is why the new_requested_mantissa_digit_count parameter exists.
 *
 *  The early_exponent_by_digits() function is identical to the
 *  early_exponent_by_size_t() function except that the exponent_magnitude
 *  size_t parameter is replaced by two parameters, the size_t
 *  exponent_digit_count and the character string pointer exponent_digits.  The
 *  first of these parameters specifies the number of decimal digits in the
 *  decimal exponent and the second is a pointer to a string containing them.
 *  The string should have the digits in the form of ASCII digits in order from
 *  the most to the least significant.
 *
 *  The late_exponent_by_size_t() and late_exponent_by_digits() functions are
 *  identical to their early_*() counterparts except that the
 *  mantissa_is_negative and new_requested_mantissa_digit_count parameters are
 *  omitted.  The mantissa_is_negative parameter is omitted because the
 *  mantissa has already been dealt with an is no longer relevant.  The
 *  new_requested_mantissa_digit_count parameter is omitted because the
 *  mantissa digit count is not going to be further modified at this point.
 *
 *  Note that the algorithm is expected to provide the same information in the
 *  early and late exponent calls.  The information is given both before and
 *  after the mantissa digits just so that the exponent information doesn't
 *  have to be buffered by the code called by this interface.  If the exponent
 *  is being printed, it is printed after the mantissa, which is why the late
 *  exponent calls exist, but the exponent value can in many cases affect how
 *  the mantissa itself is printed (where the decimal place occurs, how many
 *  digits are printed, etc.), which is why the early exponent calls exist.
 *
 *  The notify_of_mantissa_trailing_zero_count() function is optional -- the
 *  algorithm can either choose to use it or not use it.  There are two
 *  advantages to using it in some cases.  The first is to remove the need for
 *  buffering in some cases -- if there might be left padding and trailing
 *  zeros are to be dropped, the padding cannot be determined until the last
 *  mantissa digit is known, which requires buffering the mantissa digits,
 *  unless this function is called.  The second advantage is to remove multiple
 *  calls to handle_mantissa_digit(), one for each of the zeros.  The interface
 *  to this function is pretty straightforward -- in addition to the usual
 *  output_control parameter, it takes a size_t specifying the number of
 *  trailing zeros.
 *
 *  The special_non_numeric_value() function takes the usual output_control
 *  parameter plus one other: output_string, which should point to a zero-
 *  terminated ASCII string specifying the printed form of the special value
 *  being converted.
 *
 *  Finally, the interface to the too_many_floating_point_mantissa_digits() is
 *  the simplest of all.  It takes nothing more than the output_control
 *  parameter.
 *
 *
 *      Error Handling
 *
 *  For any but the too_many_floating_point_mantissa_digits() function, any
 *  error will result in a return value of MISSION_FAILED and a lack of error
 *  will result in a return value of MISSION_ACCOMPLISHED.  The caller should
 *  check the return value and return an error code itself if the called
 *  function had an error.  The too_many_floating_point_mantissa_digits()
 *  function is a special case since it should be called only in an error case
 *  -- since it's an error anyway, the caller is expected to return an error
 *  code, so the too_many_floating_point_mantissa_digits() function doesn't
 *  need to bother with returning a code indicating error or success.
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
 *  This code was written by me, Chris Wilson, in October of 2008 and earlier
 *  and placed in the public domain at that time.  A version of this code was
 *  first developed by me in a different file, "print_formatting.c" and then
 *  moved into this separate header file as a part of breaking up what was
 *  originally a single module.  All of it was developed by me for my own use
 *  in my own time and on hardware that I own.
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

#ifndef FLOATING_POINT_OUTPUT_CONTROL
#define FLOATING_POINT_OUTPUT_CONTROL
typedef struct floating_point_output_control floating_point_output_control;
#endif /* FLOATING_POINT_OUTPUT_CONTROL */


extern verdict handle_mantissa_digit(
        floating_point_output_control *output_control, char digit);
extern verdict early_exponent_by_size_t(
        floating_point_output_control *output_control,
        boolean mantissa_is_negative, boolean exponent_is_negative,
        size_t exponent_magnitude, size_t *new_requested_mantissa_digit_count);
extern verdict early_exponent_by_digits(
        floating_point_output_control *output_control,
        boolean mantissa_is_negative, boolean exponent_is_negative,
        size_t exponent_digit_count, const char *exponent_digits,
        size_t *new_requested_mantissa_digit_count);
extern verdict late_exponent_by_size_t(
        floating_point_output_control *output_control,
        boolean exponent_is_negative, size_t exponent_magnitude);
extern verdict late_exponent_by_digits(
        floating_point_output_control *output_control,
        boolean exponent_is_negative, size_t exponent_digit_count,
        const char *exponent_digits);
extern verdict notify_of_mantissa_trailing_zero_count(
        floating_point_output_control *output_control,
        size_t trailing_zero_count);
extern verdict special_non_numeric_value(
        floating_point_output_control *output_control,
        const char *output_string);
extern void too_many_floating_point_mantissa_digits(
        floating_point_output_control *output_control);


#endif /* FLOATING_POINT_OUTPUT_CONVERSION_H */
