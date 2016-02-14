/* file "floating_point_output_caller.h" */

/*
 *  This file contains an interface to a module to do floating-point output
 *  formatting.  This is the interface for use by the caller of the floating-
 *  point output formatting functionality, as opposed to that used by plug-in
 *  converter modules used by this module, which is in a different file,
 *  "floating_point_output_conversion.h".
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef FLOATING_POINT_OUTPUT_CALLER_H
#define FLOATING_POINT_OUTPUT_CALLER_H

#include <stddef.h>
#include "../basic.h"


/*
 *      Usage
 *
 *  This is an interface to be used to call a function to format and output
 *  floating-point values as human-readable ASCII strings.  It is one of two
 *  interfaces to the floating_point_output module.  The other interface to
 *  this module -- "floating_point_output_conversion.h" -- is used by plug-in
 *  converter code that is called by this module and must make calls back to
 *  this module.  Code that needs to get a floating-point value converted to
 *  ASCII and has an existing conversion plug-in to use only needs this
 *  interface, not that other interface.
 *
 *  This interface consists of the declaration of two types and two functions.
 *
 *  The first type is the floating_point_output_control structure type.  It is
 *  declared here only because it is passed as a parameter by this module to
 *  the plug-in conversion function, and a pointer to the plug-in conversion
 *  function is passed to this module when it is invoked.  But the all objects
 *  of this type are strictly for use by this module and by the plug-in -- the
 *  code using this interface need not be concerned with it.
 *
 *  Note that the typedef of the floating_point_output_control structure is
 *  protected by a #ifndef block.  This is because the same type is also
 *  defined in the "floating_point_output_conversion.h" header file, and if
 *  both header files are included by the same code, there will be a conflict
 *  unless there is a mechanism to avoid it, because C doesn't allow two
 *  typedefs of the same name, even if they are identical typedef statements.
 *  The #ifndef block, in conjunction with an identical #ifndef block in the
 *  other header file, ensures that the typedef is used if and only if it
 *  hasn't appeared already.
 *
 *  The second type is the padding_kind enumerated type.  This is used to
 *  specify what kind of padding is to be done when sending the output
 *  characters.  The PK_NO_PADDING enumeration value implies that no padding is
 *  to be done to meet a minimum field width requirement.  The
 *  PK_LEFT_SPACE_PADDING enumeration value says that if the number of
 *  characters would otherwise be less than the specified minimum width, spaces
 *  should be added to the left side to pad the field out to the minimum width.
 *  The PK_LEFT_ZERO_PADDING enumeration value also says that padding should be
 *  used to meet the mimimum width specified, but in this case the padding
 *  should be done with zeros to the left of the mantissa digits but to the
 *  right of the sign character, if any.  If the value is a non-numeric value,
 *  the PK_LEFT_ZERO_PADDING enumeration constant will be treated as if it were
 *  the PK_LEFT_SPACE_PADDING, because in this case there is no mantissa onto
 *  which it makes sense to add zeros.  Finally, the PK_RIGHT_SPACE_PADDING
 *  enumeration value implies that the field should be padded with spaces to
 *  meet the minimum width requirement, if necessary, but that the spaces
 *  should go on the right (that is, after all other characters in the field).
 *
 *  The do_floating_point_output() function is the primary function of this
 *  interface.  It is called to convert one floating-point value to ASCII and
 *  send the ASCII characters to a specified output function.
 *
 *  The first parameter of the do_floating_point_output() function is called
 *  conversion_function.  It is a pointer to the plug-in function to do
 *  floating-point conversion.  The plug-in function is responsible for giving
 *  this module the decimal digits for the mantissa, the decimal digits for the
 *  exponent, and the sign of the exponent -- or telling this module that the
 *  value is a special non-numeric value and giving it the ASCII for that
 *  special value.  This module then takes that raw data and decides whether to
 *  use exponent notation or not; whether to use a decimal point or not; where
 *  to put the decimal point, if used; where to add zeros, if needed; where to
 *  round, if the plug-in doesn't do the rounding itself; what trailing zeros,
 *  if any, to truncate; whether to use a sign character and if so what that
 *  sign character should be; and whether to do padding and if so what that
 *  padding should be.  This allows the formatting code in this module to be
 *  re-used for different conversion algorithms from floating-point values to
 *  ASCII decimal digits.
 *
 *  The conversion_function parameter's type is a pointer to a function of the
 *  type required for the plug-in converter.  The converter's first parameter
 *  is a void pointer to data specific to that plug-in but not to the
 *  particular value.  This essentially makes the plug-in object-oriented, in
 *  that it consists of a function and a pointer to local data for that
 *  function, and the function has access to that associated data when it is
 *  called.  The next parameter of the plug-in is an object of type
 *  floating_point_output_control -- see the comments in
 *  "floating_point_output_conversion.h" for the meaning of that pointer.  The
 *  following parameter for the plug-in is requested_mantissa_digit_count.
 *  That specifies how many decimal mantissa digits the floating_point_output
 *  module requires from the plug-in.  The care_about_trailing_zero_count
 *  parameter is a hint to the plug-in about whether the floating_point_output
 *  module cares about trailing zeros.  If it is TRUE, the plug-in might go to
 *  a bit more effort to inform the floating_point_output module about the
 *  number of trailing zeros early, but it is not required to do so.  The
 *  value_data parameter is a void pointer that is used to pass along a handle
 *  on the floating-point value to be converted.  Note that the
 *  floating_point_output module is entirely agnostic about the type of
 *  floating-point data being used.  It would work perfectly well with
 *  non-standard ways of specifying floating-point values.  The caller of the
 *  do_floating_point_output() function and the plug-in just have to agree on
 *  the meaning of the value_data pointer.  Finally, the mantissa_is_negative
 *  parameter simply passes on the information that the floating_point_output
 *  module already has about whether or not the mantissa of the value being
 *  converted is negative (the floating_point_output module needs to know this
 *  to handle the output of the sign bit, but nothing else directly about the
 *  floating-point value, so it gets this bit of information as a separate
 *  boolean value, and since it has this information already, it passes it
 *  along to the plug-in, even though the plug-in should be able to determine
 *  that for itself from the information pointed to by value_data).
 *
 *  That completes the discussion of the conversion_function parameter to
 *  do_floating_point_output().  The next parameter to
 *  do_floating_point_output() is the function_data void pointer.  This is the
 *  data that is to be sent to the plug-in function as its first argument --
 *  the function and this data pointer together constitute the plug-in.  This
 *  pointer is in no other way used by the floating_point_output module.
 *
 *  The next parameter to do_floating_point_output() is value_data.  This void
 *  pointer is also only passed to the plug-in and not otherwise used by the
 *  floating_point_output module, but instead of being part of the plug-in
 *  itself, tells the plug-in the floating-point value to be converted.  It is
 *  passed to the plug-in function as the parameter of the same name in the
 *  interface to the plug-in function.
 *
 *  The mantissa_is_negative parameter to do_floating_point_output() informs
 *  this function whether or not the mantissa is negative.  In addition to
 *  being passed along to the plug-in, this value is used by the
 *  floating_point_output module to figure out the correct sign character, if
 *  any, to use in the output, as mentioned above.
 *
 *  The requested_mantissa_digit_count parameter specifies to the
 *  floating_point_output module how many mantissa digits to use.  It doesn't
 *  always use exactly this many mantissa digits, though.  If the
 *  suppress_trailing_zeros parameter is set to TRUE, trailing zero suppression
 *  might diminish the number of mantissa digits used.  If exponent notation is
 *  not used and the exponent is positive, more digits might have to be used on
 *  the right to reach the decimal point (or where it would be if it were
 *  printed.  Also, if exponent notation is not used and the exponent is
 *  negative and the fixed_number_of_digits_after_decimal_point parameter is
 *  FALSE, then additional zeros will be put on the left to retain
 *  requested_mantissa_digit_count significant digits.  Finally, zero-padding,
 *  if it is done, could be considered another form of adding more mantissa
 *  digits.
 *
 *  The ``precision'' parameter to do_floating_point_output() specifies the
 *  precision field specified by the format that generated this call.  Note
 *  that it is not directly used by the floating_point_output module, because
 *  the caller of this module is supposed to interpret it and use it to set the
 *  other parameters, such as the requested_mantissa_digit_count parameter.  It
 *  is passed separately into this function only for the sake of diagnostic
 *  messages -- this module puts the precision into certain diagnostic messages
 *  to make it easier for the user to understand the format that caused a
 *  problem.
 *
 *  The print_space_if_positive parameter of do_floating_point_output() is a
 *  boolean that if TRUE tells the floating_point_output module to print a
 *  space as the sign character if the value is non-negative.  Similarly, the
 *  print_plus_sign_if_positive parameter says to printe a plus sign as the
 *  sign character if the value is non-negative, if that parameter is TRUE.  It
 *  is illegal to make both of these parameters TRUE in the same call.  If
 *  neither is true and the value is non-negative, no sign character at all
 *  will be printed.  If the value is negative, these two parameters don't
 *  matter and a minus sign is used as the sign character.
 *
 *  The exponent_marker_character parameter specifies which character to use as
 *  the exponent marker if exponential notation is being used.  It is normally
 *  either `e' or `E' (as in "1.0e+10" or "1.0E+10").
 *
 *  The suppress_trailing_zeros parameter to do_floating_point_output(), if
 *  TRUE, says to suppress trailing mantissa zeros that come after the decimal
 *  point.  If it is FALSE, trailing mantissa zeros are to be printed as if
 *  they were any other mantissa digits.
 *
 *  The fixed_number_of_digits_after_decimal_point parameter to the
 *  do_floating_point_output() function specifies whether or not the number of
 *  digits after the decimal point is to be fixed.  If FALSE, the
 *  requested_mantissa_digit_count parameter specifies the number of
 *  significant digits; if TRUE, the requested_mantissa_digit_count parameter
 *  specifies the number of digits after the decimal point plus one.  Note that
 *  it only makes a difference when not using exponent notation, because when
 *  using exponential notation, the decimal point is always normalized to come
 *  after the first significant mantissa digit.
 *
 *  The next two parameters of do_floating_point_output() --
 *  decimal_point_use_decided and print_decimal_point -- go together.  If
 *  decimal_point_use_decided is TRUE, then print_decimal_point specifies
 *  whether or not to print a decimal point.  Otherwise, print_decimal_point is
 *  ignored and instead the decimal point is printed if and only if it is
 *  followed by at least one digit.
 *
 *  The next three parameters of do_floating_point_output() --
 *  exponent_notation_use_decided,
 *  negative_exponent_limit_for_exponent_notation, and use_exponent_notation --
 *  also go together.  Together, these three parameters determine whether or
 *  not exponent notation is used for the output.  If
 *  exponent_notation_use_decided is TRUE, then
 *  negative_exponent_limit_for_exponent_notation is ignored and
 *  use_exponent_notation specifies whether or not exponent notation is used.
 *  If exponent_notation_use_decided is FALSE, then use_exponent_notation is
 *  ignored and the value of negative_exponent_limit_for_exponent_notation
 *  becomes relevant.  In this case, exponent notation is used if and only if
 *  the exponent value is either greater than or equal to
 *  negative_exponent_limit_for_exponent_notation or less than or equal to -5.
 *  Note that this interface is quirky -- the positive limit is parameterized
 *  while the negative limit is fixed at -5.  That's because that fits what's
 *  needed to support the ANSI C ``g'' and ``G'' conversion specifiers, where
 *  -5 is hard-coded as the negative limit and the positive limit is specified
 *  by the precision.
 *  @@@
 *  Note also that the use of the word ``negative'' in the name of the
 *  negative_exponent_limit_for_exponent_notation parameter is wrong -- it's
 *  actually the positive limit, not the negative limit.  The name of this
 *  parameter really should be changed to fix that.
 *  @@@
 *
 *  The conversion_type_specification_character parameter is similar to the
 *  ``precision'' parameter -- it is used by this module only for certain
 *  diagnostic messages, to make the messages easier for the user to understand
 *  which conversion specifier was the source of the problem.  This parameter
 *  should contain the ASCII character that is the conversion specifier for
 *  this floating-point conversion operation.  Usually, this should be `f',
 *  `e', `E', `g', or `G'.  But this module doesn't care what it is, and
 *  doesn't interpret it -- it's up to the code calling this module to
 *  interpret this character and use that information to set the various other
 *  parameters to this function.
 *
 *  The next two parameters to do_floating_point_output() -- minimum_width and
 *  padding_specification -- together specify what padding, if any, this module
 *  should do.  See the discussion above of the padding_kind enumerated type
 *  for the meaning of the padding_specification parameter.  In that
 *  discussion, the minimum width is used -- that minimum width is specified by
 *  the minimum_width parameter.
 *
 *  Next is the character_output_function parameter.  This parameter to
 *  do_floating_point_output() is a pointer to a function.  The function it
 *  points to is the function that is to be called to send out output
 *  characters from this module.  It should be called once for each output
 *  character, in order from left to right.  The first parameter to the
 *  function is ``data'', a void pointer which makes this function object-
 *  oriented.  The do_floating_point_output() function will simply pass the
 *  value in its own character_output_data parameter as the first parameter to
 *  the character function.  The other parameter to the character function is
 *  the output_character parameter.  This parameter contains the ASCII value of
 *  the character being sent to the output.  By using a character function for
 *  output, this module can be used for printing to a file, to a terminal, to a
 *  string buffer, or to another layer of software that does anything at all
 *  with the output from this module.
 *
 *  The character_output_data parameter to do_floating_point_output() is used
 *  in conjuction with the character_output_function parameter to make
 *  character output use an object-oriented interface.  As mentioned above, the
 *  character_output_data parameter's value is passed to the character function
 *  each time it is called by this module.  That is the only use this module
 *  makes of the value in the character_output_data parameter.
 *
 *  The rounding_done_early parameter of the do_floating_point_output()
 *  function specifies whether the value conversion plug-in does rounding
 *  itself.  If rounding_done_early is TRUE, then the plug-in does rounding.
 *  That is, if the next mantissa digit after the digits requested would have
 *  been 5 or greater, the plug-in rounds up, incrementing the digits it does
 *  pass to this module.  If rounding_done_early is FALSE, then the plug-in
 *  just truncates the mantissa to the number of digits requested by this
 *  module.  In that case, this module will have to request an extra mantissa
 *  digit so that it can do rounding itself.
 *
 *  Finally, there is the output_character_count parameter.  This parameter is
 *  a pointer to a location that can hold a size_t value.  The
 *  do_floating_point_output() function will write the number of output
 *  characters it sent into this location.  The pointer value must not be NULL.
 *  The value is only written in the case that the do_floating_point_output()
 *  function finished without an error and returned MISSION_ACCOMPLISHED.  In
 *  case of an error, no value will be written to this location by this
 *  function.
 *
 *  That completes the interface to the do_floating_point_output() function.
 *
 *  The final part of this interface is the
 *  too_many_floating_point_mantissa_digits_by_details() function.  This
 *  function is for use by the code that would call this module.  In the case
 *  that the number of mantissa digits required would be too big -- for
 *  example, if it didn't fit in a size_t value and thus couldn't be passed in
 *  the requested_mantissa_digit_count parameter to do_floating_point_output()
 *  -- this function can be called.  This function simply prints a diagnostic
 *  message.  The reason for including this function in the interface to this
 *  module instead of having the caller simply print its own diagnostic message
 *  is so that the same diagnostic message can be used when the number of
 *  digits required is too high whether that is determined by this module or by
 *  the code that would call this module.
 *
 *  The too_many_floating_point_mantissa_digits_by_details() function takes two
 *  parameters.  The first is the ``precision'' and the second is the
 *  conversion_type_specification_character.  These parameters have meanings
 *  identical to those of the parameters of the same name to the
 *  do_floating_point_output() function.  Just as in the
 *  do_floating_point_output() function, in the
 *  too_many_floating_point_mantissa_digits_by_details() function these two
 *  values are used in generating a diagnostic message that gives information
 *  that is meaningful to the user.
 *
 *
 *      Error Handling
 *
 *  For the do_floating_point_output() function, any error will result in a
 *  return value of MISSION_FAILED and a lack of error will result in a return
 *  value of MISSION_ACCOMPLISHED.  The caller should check the return value
 *  and return an error code itself if the called function had an error.  The
 *  too_many_floating_point_mantissa_digits_by_details() function is a special
 *  case since it should be called only in an error case -- since it's an error
 *  anyway, the caller is expected to return an error code, so the
 *  too_many_floating_point_mantissa_digits_by_details() function doesn't need
 *  to bother with returning a code indicating error or success.
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

typedef enum padding_kind
  {
    PK_NO_PADDING,
    PK_LEFT_SPACE_PADDING,
    PK_LEFT_ZERO_PADDING,
    PK_RIGHT_SPACE_PADDING
  } padding_kind;


extern verdict do_floating_point_output(
        verdict (*conversion_function)(void *function_data,
                floating_point_output_control *output_control,
                size_t requested_mantissa_digit_count,
                boolean care_about_trailing_zero_count, void *value_data,
                boolean mantissa_is_negative), void *function_data,
        void *value_data, boolean mantissa_is_negative,
        size_t requested_mantissa_digit_count, size_t precision,
        boolean print_space_if_positive, boolean print_plus_sign_if_positive,
        char exponent_marker_character, boolean suppress_trailing_zeros,
        boolean fixed_number_of_digits_after_decimal_point,
        boolean decimal_point_use_decided, boolean print_decimal_point,
        boolean exponent_notation_use_decided,
        size_t negative_exponent_limit_for_exponent_notation,
        boolean use_exponent_notation,
        char conversion_type_specification_character, size_t minimum_width,
        padding_kind padding_specification,
        verdict (*character_output_function)(void *data,
                                             char output_character),
        void *character_output_data, boolean rounding_done_early,
        size_t *output_character_count);
extern void too_many_floating_point_mantissa_digits_by_details(
        size_t precision, char conversion_type_specification_character);


#endif /* FLOATING_POINT_OUTPUT_CALLER_H */
