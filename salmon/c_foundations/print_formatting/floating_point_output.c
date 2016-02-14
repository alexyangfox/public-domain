/* file "floating_point_output.c" */

/*
 *  This file contains the implementation of an interface for use by code to do
 *  floating-point conversion to decimal for formatted printing.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <limits.h>
#include <float.h>
#include "basic.h"
#include "diagnostic.h"
#include "memory_allocation.h"
#include "print_formatting/floating_point_output_caller.h"
#include "print_formatting/floating_point_output_conversion.h"
#include "code_point.h"


/*
 *      Data Structures
 *
 *  @@@
 *  @@@
 *
 *  Here are what the fields of the floating_point_output_control structure
 *  mean:
 *
 *  verdict (*character_output_function)(void *data, char output_character);
 *      This field contains a pointer to the function that is to be called by
 *      this module to send a character out, after all processing by this
 *      module.
 *
 *  void *character_output_data;
 *      This field contains a data pointer that is to be sent along in all
 *      calls to the function pointed to by the character_output_function
 *      field.  So really these two fields together implement an object-
 *      oriented call interface.
 *
 *  boolean rounding_done_early;
 *      This field indicates whether or not this module has to concern itself
 *      with rounding.  If it is TRUE, then the rounding is being done before
 *      the digits are sent to this module and this module doesn't have to do
 *      any rounding.  If it is FALSE, then no rounding is being done before
 *      the digits are sent to this module and this module must do proper
 *      rounding.
 *
 *  verdict verdict_so_far;
 *      This field is used to indicate whether or not the character output
 *      function pointed to by the character_output_function field has returned
 *      an error indication through any of the calls to it by the current
 *      invocation of this module.  A value of MISSION_ACCOMPLISHED indicates
 *      no errors returned and any other value indicates an error was returned.
 *      Note that the value of this field is only used in assertions, to make
 *      sure that certain code isn't executed if we've received an error from
 *      the back end.
 *
 *  boolean value_is_special_non_numeric;
 *      This field is used to indicate whether the value being handled is a
 *      special non-numeric value such a ``infinity'' or ``not a number''
 *      instead of a finite rational value.
 *
 *  boolean did_early_exponent;
 *  boolean did_late_exponent;
 *      These two fields are used to catch phase ordering errors.  The
 *      did_early_exponent field is set to TRUE if and only if the
 *      digit-generating code has called either early_exponent_by_size_t() or
 *      early_exponent_by_digits().  The did_late_exponent field is set to TRUE
 *      if and only if the digit-generating code has called either
 *      late_exponent_by_size_t() or late_exponent_by_digits().  The values of
 *      both fields are only used in assertions, to make sure the
 *      digit-generating code calls things in the proper order.
 *
 *  boolean possible_left_padding;
 *  boolean possible_right_padding;
 *  boolean possible_zero_padding;
 *  size_t minimum_width;
 *      These four fields collectively specify what padding, if any, this
 *      module is required to do to the output.  The first three fields
 *      indicate the sort of padding.  At most one of the three may have the
 *      value TRUE.  All three may be FALSE.  If all three are FALSE, no
 *      padding is done and the minimum_width field is ignored.  If one of the
 *      three is TRUE, padding may or may not be done, depending on the value
 *      of the mimimum_width field.  If the number of characters that are to be
 *      output is greater than or equal to that specified by the minimum_width
 *      field, no padding is done.  Otherwise, if padding is enabled and the
 *      number of characters that would otherwise be send out is less than the
 *      mimimum_width field's value, then characters are added to the output to
 *      make the total number of characters reach the value of the
 *      minimum_width field.  What characters are added and where they are
 *      added depends on which of the first three of these fields has the value
 *      TRUE.  If the possible_left_padding field is TRUE, then the added
 *      characters are spaces and they are added to the left (i.e. before) the
 *      other characters.  If the possible_right_padding field is TRUE, then
 *      the added characters are spaces and they are added to thr right (i.e.
 *      after) the other characters.  If the possible_zero_padding field is
 *      TRUE, then the added characters are zeros and they are added
 *      immediately after the mantissa sign character, if any, or before all
 *      other characters if there is no mantissa sign character.
 *
 *  boolean exponent_notation_use_decided;
 *      This field indicates whether or not it has been determined whether
 *      exponent notation will be used in the output.  A value of TRUE
 *      indicates that it has been decided and a value of FALSE indicates that
 *      it has not yet been decided.
 *
 *  size_t negative_exponent_limit_for_exponent_notation;
 *      If the exponent_notation_use_decided field has the value FALSE, then
 *      this field contains information that specifies how to decide whether or
 *      not exponent notation will be used in the output: if the exponent is
 *      greater than or equal to the value of this field or less than or equal
 *      to -5, then exponent notation will be used, anotherwise it will not.
 *
 *  boolean use_exponent_notation;
 *      If the exponent_notation_use_decided field has the value TRUE, this
 *      field indicates whether or not exponent notation is used.  If the
 *      exponent_notation_use_decided field as the value FALSE, the value in
 *      the use_exponent_notation field is not used.
 *
 *  boolean decimal_point_use_decided;
 *      This field specifies whether or not the decision about whether to print
 *      a decimal point for the mantissa has been made.  If it has the value
 *      TRUE, the decision has been made, and otherwise it has not.
 *
 *  boolean print_decimal_point;
 *      This field is only used if the decimal_point_use_decided field has the
 *      value TRUE.  In that case, this field specifies whether or not the
 *      decimal point is to be printed.  It has the value TRUE if the decimal
 *      point is to be printed and FALSE if it is not to be printed.
 *
 *  char exponent_marker_character;
 *      This field specifies which character to is to be used to separate the
 *      mantissa from the exponent in the output.  Though this module doesn't
 *      care what it is, typically it would be `e' or `E'.
 *
 *  boolean print_plus_sign_if_positive;
 *      This field specifies whether or not to print a plus sign for the
 *      mantissa if the mantissa is positive or zero.  If the mantissa turns
 *      out to be negative, this flag has no effect (a minus sign is always
 *      printed out in the case that the mantissa is negative).
 *
 *  boolean print_space_if_positive;
 *      If this flag is TRUE and the print_plus_sign_if_positive flag is FALSE
 *      and the mantissa turns out to be non-negative, then a space will be
 *      printed where the sign character would go.  If the
 *      print_plus_sign_if_positive flag is TRUE or if the mantissa turns out
 *      to be negative, this flag has no effect.
 *
 *  boolean suppress_trailing_zeros;
 *      If this field is set to TRUE, then trailing zeros in the mantissa after
 *      the decimal point are to be supressed.
 *
 *  boolean fixed_number_of_digits_after_decimal_point;
 *      This field is set to TRUE to indicate that the number of significant
 *      mantissa digits after the decimal point is fixed and FALSE to indicate
 *      that the number of significant mantissa digits does not change
 *      depending on where the decimal point is.  This field has no effect in
 *      the case that exponent notation is used, but in the case that exponent
 *      notation is not being used, if this field is TRUE then if the exponent
 *      is not zero then the number of significant mantissa digits can go up or
 *      down based on the exponent.
 *
 *  exponent_rollover_effect the_exponent_rollover_effect;
 *      This field is a structure that specifies what effects the rolling-over
 *      of the exponent value will have, in case the early exponent value rolls
 *      over to a larger magnitude because of rounding.
 *
 *  size_t field_characters;
 *      This field is set to the total number of characters that would be sent
 *      out as output from this module if padding were not included.  It is
 *      used to figure out what padding, if any, needs to be done.  This field
 *      only has a valid value after enough information is available to compute
 *      it.  The final exponent value needs to be known before this field can
 *      be calculated.  Also, if trailing zeros are to be supressed, the
 *      trailing zero count must be known before this field's value can be
 *      computed.
 *
 *  boolean mantissa_is_negative;
 *      This field indicates whether or not the mantissa's value is negative.
 *      It is set to TRUE if and only if the mantissa is negative.  It is set
 *      when one of the early_exponent_*() functions is called.
 *
 *  size_t stuffed_zeros;
 *      This field contains a count of the number of zeros that are to be
 *      inserted before mantissa digits provided by the conversion routine.
 *      These zeros are inserted in the case that exponent notation is not used
 *      and the exponent is negative, so that there is a zero before the
 *      decimal point and enough zeros after the decimal point to put the
 *      non-zero mantissa digits in the right place.
 *
 *  boolean have_pending_mantissa_digit;
 *  char pending_mantissa_digit;
 *  size_t pending_mantissa_nine_count;
 *      Together, these three fields are used to implement a system to cache
 *      output mantissa digits in case of possible rounding.  Digits aren't
 *      actually sent on to the next phase until it is certain that they won't
 *      be changed by rounding.
 *
 *      The cache formed by these three fields can hold up to one non-nine
 *      digit and any number of nine digits following it.  This is all the
 *      cache ever needs to hold because if any digit is followed by a non-nine
 *      digit in a less-significant place, then rounding cannot affect the
 *      more-significant digit, since the carrying from a rounding operation
 *      will stop at the first non-nine digit.
 *
 *      The have_pending_mantissa_digit field indicates whether or not there is
 *      a non-nine digit in this cache.  If it has the value TRUE, then the
 *      pending_mantissa_digit contains the ASCII value of the pending digit
 *      ('0' through '8').  If the have_pending_mantissa_digit field has the
 *      value FALSE, then the value of the pending_mantissa_digit field is
 *      unusued and there is no non-nine digit in the cache.  The
 *      pending_mantissa_nine_count field contains a count of the number of
 *      nine digits in the cache.  If there are both non-nine and nine digits
 *      in the cache, the non-nine digit is the most significant.  So a value
 *      of FALSE for the have_pending_mantissa_digit field and a value of zero
 *      for the pending_mantissa_nine_count field indicates an empty cache.
 *      Note that the pending_mantissa_nine_count value can be zero or non-zero
 *      for either case of the have_pending_mantissa_digit field -- TRUE of
 *      FALSE.
 *
 *      It is also worth mentioning that if the pending_mantissa_nine_count
 *      field is non-zero while the have_pending_mantissa_digit field is FALSE
 *      then all the digits in the mantissa seen so far are nines, so if
 *      rounding up happens while in this state, then the exponent value will
 *      have to go up by one and a new first mantissa digit of one will have to
 *      be added, followed by zeros.
 *
 *  size_t pending_mantissa_zero_count;
 *      This field is used to cache zero digits.  Note that this cache is
 *      logically after the cache implemented by the previous three fields --
 *      after digits are passed out of that cache, they go through this zero
 *      cache.  In this cache, all non-zero digits are immediately passed
 *      through, and a non-zero digit also causes the cache of zero digits to
 *      be emptied before the non-zero digit is passed on.  Zero digits,
 *      however, all stick in the cache.  The pending_mantissa_zero_count field
 *      is a count of the number of zero digits in the cache.  Since the cache
 *      only holds zero digits, this count of zeros is the only data that needs
 *      to be stored to implement this cache.
 *
 *  boolean increment_exponent;
 *      This flag is set to indicate that the exponent value provided by the
 *      conversion function has to be increment by one.  It is set if and only
 *      if this module is doing rounding and rounding up results in the
 *      mantissa growing by one digit -- i.e. the case where 9.999... rolls
 *      over to 10.000... when exponent notation is being used, requiring a
 *      change to 1.000... and an incremented exponent.
 *
 *  size_t digits_before_decimal_point;
 *      This field is used to indicate where the decimal point should be
 *      printed.  It specifies how many mantissa digits should come before the
 *      decimal point.
 *
 *  size_t output_mantissa_digit_count;
 *      This field contains the total number of mantissa digits that are to be
 *      output by this module.  Note that this can change as new information
 *      becomes avaiable because of rounding or trailing zero truncation.
 *
 *  size_t requested_mantissa_digit_count;
 *      This field indicates how many mantissa digits were requested from the
 *      conversion function by this module.
 *
 *  size_t received_mantissa_digit_count;
 *      This field records how many mantissa digits have been recieved from the
 *      conversion function by this module so far.
 *
 *  size_t buffered_mantissa_digit_count;
 *      If the do_received_mantissa_digit_buffering field is TRUE, this field
 *      indicates how many mantissa digits are currently buffered by this
 *      module.  Mantissa digits are buffered when they are recieved by this
 *      module from the conversion function but this module can't print them
 *      out right away because of possible left padding that hasn't been
 *      determined yet.  The buffered mantissa digits are kept in memory
 *      pointed to by the received_mantissa_digit_buffer field.  If the
 *      do_received_mantissa_digit_buffering field is FALSE, this field is not
 *      used.
 *
 *  size_t sent_mantissa_digit_count;
 *      This field contains a count of the number of mantissa digits to reach
 *      the send_mantissa_digit() function.
 *
 *  boolean prefix_done;
 *      The prefix_done field indicates whether or not the prefix has already
 *      been printed.  The prefix includes:
 *        * left space padding, if necessary
 *        * the sign character, if any
 *        * zero padding, if necessary
 *        * zeros stuffed in to handle negative exponents when not using
 *          exponent notation, and an associated decimal point
 *      In other words, the prefix is everything before the non-stuffed
 *      mantissa digits start.
 *
 *  size_t digits_before_rounding;
 *      The digits_before_rounding field indicates how many digits remain to be
 *      recieved from the conversion function by this module before the digit
 *      that will have five added to it to implement rounding up.  A zero
 *      indicates that either no rounding is to be done or it has already been
 *      done.  A 1 in this field indicates that the next digit received from
 *      the conversion function will be the one that five is added to.
 *
 *  boolean do_received_mantissa_digit_buffering;
 *      This flag indicates whether digits received from the conversion
 *      function are currently being buffered.  If it is TRUE, then the
 *      buffered_mantissa_digit_count and received_mantissa_digit_buffer fields
 *      will be used to implement the buffering; if it is FALSE, those two
 *      fields will not be used.
 *
 *  char *received_mantissa_digit_buffer;
 *      If the do_received_mantissa_digit_buffering flag is TRUE, then this
 *      field points to the memory being used as a mantissa digit buffer and
 *      the buffered_mantissa_digit_count field indicates how many elements of
 *      this array are valid.  If the do_received_mantissa_digit_buffering flag
 *      is FALSE, this field is not used.
 *
 *  char *stack_mantissa_buffer;
 *      The do_floating_point_output() function always allocates a fixed-size
 *      space on the stack for mantissa digit buffering.  If mantissa digit
 *      buffering is needed and the amount of space needed is less than or
 *      equal to the size of this fixed-size buffer, then this stack buffer is
 *      used.  Otherwise, a larger buffer is allocated on the heap.  This is
 *      done for efficency, to avoid heap allocation and deallocation when in
 *      most cases it's not necessary.  Since the fixed-size buffer is
 *      allocated by declaring a local variable in the
 *      do_floating_point_output() function, and that variable is only visible
 *      within that function, for other functions called by
 *      do_floating_point_output() to access this buffer requires sending them
 *      a pointer to it.  That's what this field is for -- it contains a
 *      pointer to that fixed-size buffer on the stack, in case that buffer is
 *      needed.
 *
 *  boolean have_trailing_zero_count;
 *      This field is used to indicate whether or not this module already knows
 *      the trailing zero count.
 *
 *  size_t character_count;
 *      This field records how many characters have been sent out by this
 *      module to the back end.
 *
 *  boolean exponent_digit_buffer_is_populated;
 *  size_t exponent_magnitude_for_buffer;
 *  char exponent_digit_buffer[MAX_EXPONENT_DIGITS];
 *      These three fields together implement a buffer for exponent digits.
 *      This buffer is only used in the case that a size_t value is used by the
 *      conversion function to communicate the exponent value to this module.
 *      In that case, this module needs to convert the size_t exponent
 *      magnitude to decimal digits, both to get a count of the number of
 *      digits the exponent will take and also to send the exponent digits out.
 *      This buffer is used for that purpose.
 *
 *      The exponent_digit_buffer_is_populated field specifies whether or not
 *      the buffer currently contains valid data.  If it does, the
 *      exponent_magnitude_for_buffer field specifies the size_t value
 *      corresponding to the digits in the buffer (so that if the exponent
 *      value changes, we'll know to re-generate the digits) and the
 *      exponent_digit_buffer field contains the ASCII decimal digits
 *      themselves.
 *
 *  boolean exponent_is_negative;
 *      This field is set to TRUE if the exponent is negative and FALSE if it
 *      is zero or positive.  Note that the exponent can change with mantissa
 *      rounding, so this value can change because of rounding.
 *
 *  size_t exponent_digit_count;
 *      This field indicates how many decimal digits are in the exponent
 *      magnitude.
 *
 *  boolean exponent_digit_count_will_change_if_exponent_increments;
 *      This field indicates whether or not incrementing the exponent will
 *      change the number of decimal digits it has.  For example, it will be
 *      TRUE for exponent values of +99 and -100.  The exponent can be
 *      incremented by rounding, but by at most one, so that's why this flag is
 *      useful.  If we're not suppressing trailing zeros but are rounding, then
 *      we can do left space and zero padding before getting any mantissa
 *      digits from the conversion function if and only if this flag is FALSE.
 *
 *  size_t provisional_exponent_magnitude_size_t;
 *  size_t provisional_exponent_remaining_digit_count;
 *  const char *provisional_exponent_remaining_digits;
 *      These three fields are used only when the conversion function supplies
 *      the exponent magnitude as decimal digits instead of as a size_t.  In
 *      that case, we sometimes need to compare the value of the exponent
 *      magnitude to a size_t, and if it is less then get the value of the
 *      exponent as a size_t.  These three fields allow us to cache the work
 *      done in such comparisons so we don't have to redo it.
 *
 *      When comparing a size_t value to the exponent magnitude in terms of
 *      digits, we start with the most significant digit and convert it to a
 *      size_t value.  Then, we keep going from most to least significant digit
 *      and for each digit we multiply our value so far by ten and then add the
 *      value of the new digit.  If at any point the value so far exceeds the
 *      value we are comparing against, we know the answer.  Note that this
 *      works even if the value of the exponent is so large it won't fit in a
 *      size_t because we'll stop at the point where the value so far would
 *      exceed the value of the size_t we're comparing against.  The three
 *      fields here allow us to cache our progress so that next time we have to
 *      compare against a size_t we don't have to start over, we can
 *      immediately use the value so far.  The
 *      provisional_exponent_magnitude_size_t field contains the value so far.
 *      The provisional_exponent_remaining_digit_count field tells how many
 *      digits remain in the exponent and the
 *      provisional_exponent_remaining_digits field points to those remaining
 *      digits.  Note also that if the value of the exponent was less than the
 *      limit we were comparing against then we will reach the end of the
 *      exponent digits and still have a value so far that is less than the
 *      limit.  So in that case, after we've finished the comparison, the
 *      provisional_exponent_magnitude_size_t field will contain the size_t
 *      value of the exponent magnitude, and we can use that.
 *
 *  size_t precision;
 *  char conversion_type_specification_character;
 *      The ``precision'' and ``conversion_type_specification_character''
 *      fields are kept here just to be able to send additional information to
 *      the user in diagnostic messages.  These fields are not otherwise used
 *      in this module.  The ``precision'' field specifies the precision that
 *      the format string specified.  This value will be used by the client of
 *      this module to compute other values sent to this module, so this module
 *      doesn't have to use it except in diagnostic messages.  The
 *      conversion_type_specification_character field specifies the character
 *      in the format that specified the conversion specification.  For
 *      example, for a "%10lu" format, the
 *      conversion_type_specification_character field will have the value `u'.
 *
 *  @@@
 *  Before we get into the details of the data structures, I'll introduce a bit
 *  of terminology.  The set of integers from zero through num_elements-1
 *  inclusive is important for this module.  It is an ordering of all these
 *  integers that will form the final result.  Each of the lists to be merged
 *  consists of some or all of these integers.  So it's useful in this
 *  discussion to have a term for them.  For the purposes of this discussion,
 *  they will be called the ``core element''.  That makes it easier to
 *  distinguish between the core element, which can occur in several lists, and
 *  a particular instance of that core element as an element of one of the
 *  lists to be merged.
 *
 *  Aside from the input and output arrays specified by the interface, there
 *  are two primary data structures used in this module.
 *
 *  The first data structure is an array of element_global_info objects, with
 *  one for each core element.  This structure is dynamically allocated and a
 *  pointer to it is passed around to various functions, with each pointer to
 *  it named element_info.  This data structure provides a place for any
 *  information for which there is at most one copy per core element.
 *
 *  The second major data structure consists of a set of arrays of objects of
 *  type link_info.  These structures are dynamically allocated so that there
 *  is one such array for each of the lists to be merged and the number of
 *  elements in each array is equal to the number of elements in the
 *  corresponding one of the lists to be merged.  While they are allocated this
 *  way, though, they are used to form linked lists, where each element in each
 *  array as also an element in some linked list.  The linked lists are set up
 *  so that there is one list per core element, and the list for each core
 *  element lists the successors of that element in all the arrays to be merged
 *  of which it is a part.  That is, the arrays to be merged can be thought of
 *  as a way of encoding a graph, where one element coming after another in an
 *  array corresponds to the second element having an edge in the graph coming
 *  from the first element.  The link_info objects form a linked list for each
 *  node where the linked list shows all the successors of that node.  The fact
 *  that the link_info objects are allocated in arrays instead of individually
 *  as in most linked lists is a convenience for allocation and deallocation
 *  that works because of where this graph comes from in the first place, but
 *  aside from allocation and deallocation, the link_info objects are treated
 *  as a linked list, not as array elements.  As the algorithm progresses,
 *  elements can be removed from these linked lists, but elements are not
 *  added, and thus no additional allocation is needed.
 *
 *  So, one of the things that the element_global_info and link_info objects do
 *  is represent the graph structure of the arrays to be merged in a different
 *  format, a format which makes it easier to find all the successors of any
 *  given element.  Note that for purposes of this algorithm, we only care
 *  about this graph structure, not about additional information that is lost
 *  in the conversion to graph form.  For example, if the input lists are
 *  {0, 1, 2} and {2, 3}, they would form the same graph structure as if the
 *  input lists had been {0, 1} and {1, 2, 3}, but for the purposes of this
 *  algorithm those differences don't matter.  So the graph structure we
 *  construct represents the information we do care about while removing some
 *  information we don't care about.  Also, it represents it in a form that
 *  makes it easy to drop any particular edge, which, as we shall see, is going
 *  to be useful in the algorithm.
 *
 *  So now we get to the actual C code for the data structures.  After the
 *  includes and comments, there is a block of pre-processing directives before
 *  the data structures are introduced.  This section begins with
 *  ``#ifndef ASSERT_MALLOCED_SIZES''.  If the user doesn't explicitly define
 *  the ASSERT_MALLOCED_SIZES macro, the
 *  assert_is_malloced_block_with_exact_size() macro evaluates to nothing, but
 *  otherwise it includes "memory_allocation_test.h", which defines this as an
 *  external function.  This is called to assert properties of pointers to
 *  internal data structures.  For production, this call evaluates to nothing,
 *  but when ``-DASSERT_MALLOCED_SIZES'' is used, it instead calls code in an
 *  external memory checking module to do the requested checks.  Note that to
 *  use this feature requires memory_allocation_test.h and
 *  memory_allocation_test.c, which are both written by me and which are both
 *  in the public domain.
 *
 *  Now we get to the first real code, which is the definition of the
 *  element_state enumeration.  Each core element will have one field of this
 *  type, so each core element will at any given time be in one of the three
 *  states specified in this enumeration.  The algorithm will at one point be
 *  walking the tree of core elements in depth-first fashion, so during this
 *  time there will be a stack of core elements.  Any elements not yet visited
 *  will be in the ES_UNVISITED state.  Any elements currently on the stack
 *  (visiting them has begun, but they are still being processed) will be in
 *  the ES_BEING_VISITED state.  And any elements that have already been
 *  visited in the tree walk will be in the ES_FINISHED state.
 *
 *  Next comes the definition of the element_global_info structure.  As
 *  mentioned above, there will be one object of this type for each core
 *  element.
 *
 *  The ``state'' field is of type element_state, and reflects the state of the
 *  core element (as described above) during the tree walk (to be described
 *  soon).  The incoming_link_count field is kept up-to-date with the number of
 *  remaining incoming links to this core element in the graph.  For the
 *  algorithm, we will not need to be able to follow those links back.  All we
 *  really need to know is whether there are any left, so we can do that by
 *  keeping a count and decrementing it when a link to it is removed.
 *
 *  The first_link field of the element_global_info structure contains the head
 *  of the linked list of edges in the graph coming out of that core element.
 *  The first_unsearched_link field always points to a link pointer in the
 *  linked list that is headed by first_link.  That is, it contains either the
 *  location of first_link or the location of the next pointer of one of the
 *  elements of the list pointed to by first_link.  It serves as a bookmark of
 *  how far through the successor list we've searched already in case we need
 *  to come back to it and continue the search, and don't want to have to
 *  re-examine the links that have already been handled.  Initially, it is the
 *  location of first_link, because none of the outgoing edges has been handled
 *  yet.  It is a pointer to the link rather than a pointer to the element that
 *  link points to because the element might have to be removed from the list
 *  if a cycle is found, so the pointer to it might have to be updated.
 *
 *  The final type definition is that of the link_info structure.  As described
 *  above, objects of this type are used to construct the outgoing edge linked
 *  lists of the graph.  The two fields implement the linked list of edges: the
 *  target_element field specifies which core element the edge points to and
 *  the next_link field determines the next object in the linked list.
 *
 *  And that completes the detailed explanation of the data structures used
 *  here.
 *
 *
 *      Algorithm
 *
 *  Here's an outline of how the algorithm works.  First, the graph structure
 *  is put into the form of element_global_info and link_info objects.  Then,
 *  the whole graph is walked looking for cycles.  Any cycles that are found
 *  are removed.  That is, every edge in the cycle is removed.  Any nodes that
 *  are left with no incoming edges are put on the start list.  This is
 *  repeated until no cycles are left.  Then, the remaining graph is walked,
 *  with the edges being removed as they are traversed.  Any node that has
 *  other incoming edges is ignored.  Only when the last edge into a node is
 *  walked is the node put in the output list and the edges from that node
 *  walked.
 *
 *  The key step here is the removal of cycles.  Each cycles represent
 *  conflicts among the different input arrays about the ordering of the
 *  elements in that cycle.  As long as there are cycles, there is no way to
 *  order all the elements in such a way that every edges goes from an earlier
 *  element to a later element in the final ordering.  As we find cycles, we
 *  remove every element in each cycle.  So if an element tends to follow
 *  another in most cases but in a few its the other way around, the removal of
 *  cycles will tend to remove as many edges each direction, making it likely
 *  that the element that usually follows ends up following in the result.
 *
 *      Functions
 *
 *  If I've been successful, I've marked every basic block with one use of the
 *  code_point() macro and each one uses a different integer value as the
 *  argument.  In addition to one per basic block, I've added ``else'' blocks
 *  to ``if'' statements where they would not otherwise be necessary just for
 *  the code_point() macros.  This is to make it easier to check that the tests
 *  cover all the code in this file.
 *
 *      merge_dense_integer_arrays()
 *
 *  The merge_dense_integer_arrays() function is the one function in this
 *  module that is externally accessable.  It is the top-level function that
 *  carries out the merge operation.
 *
 *  The first thing it does, at code point 1, is assert some checks on its
 *  parameters.  Then it calls create_internal_structures(), which creates both
 *  the array of element_global_info objects and the array of arrays of
 *  link_info objects.  That function returns a pointer to the array of
 *  element_global_info objects and sets the location pointed to by its
 *  link_arrays_location parameter to point to the array of arrays of link_info
 *  objects.  It also fills in all of these data structures with the starting
 *  information based on the arrays to be merged, which includes setting up the
 *  linked lists of successors for each core element.  In addition, it creates
 *  one more linked list, the head list, which is also a linked list of
 *  link_info objects but which specifies all the head elements of all the
 *  lists.  This head list is also composed of elements in the array of arrays
 *  of link_info objects.  The location pointed to by the head_list parameter
 *  is set to point to this list and the head_list_tail is set to point to the
 *  last pointer in the list (the location of the NULL that terminates the
 *  list).  This head_list_tail allows elements to be appended to the list
 *  efficiently.  If the create_internal_structures() function fails (it can
 *  fail if malloc() fails), it returns NULL and sets the link_info array of
 *  arrays pointer to NULL also.  Then the ``if'' statement right after the
 *  call to create_internal_structures() tests for this failure condition.  If
 *  element_info is NULL (code point 2), then the code asserts that link_arrays
 *  is also NULL and returns NULL to indicate a failure.
 *
 *  Once create_internal_structures() has been called successfully (code point
 *  4), some assertions are made about the data structures set up by that
 *  function.  Then it walks through the head element list and for each element
 *  (code point 5) it calls remove_cycles().  The remove_cycles() function is
 *  the heart of the algorithm.  It recursively walks the graph looking for
 *  cycles.  Calling it for each element in the head list insures that it walks
 *  the entire graph.  As the name implies, this function removes all the
 *  cycles it finds, so when the loop is done and all the calls to
 *  remove_cycles() have been made, all cycles in the graph are gone.  It
 *  asserts that the result returned by remove_cycles() is TRUE because a
 *  result of FALSE is used to indicate the incoming edge is part of a cycle,
 *  which can't be because this is being entered from the head list, not from
 *  another node.
 *
 *  Next, at code point 6, the code calls order_elements() to walk the graph
 *  and create the result array.  If order_elements() fails (because malloc()
 *  failed), it returns NULL, so right after the call to order_elements() is a
 *  check to see if the result is NULL.  If it is not NULL (code point 7), the
 *  result is asserted to be of the correct size.  Note that if it is NULL
 *  (code point 8), nothing special is done because this NULL result will
 *  simply be returned by this function anyway, after the internal data
 *  structures have been deallocated.
 *
 *  Finally, at code point 9, delete_internal_structures() is called to
 *  deallocate all the internal data structures that were allocated.  Note that
 *  this does not include the result array since that will be passed back as
 *  the result.  Then, the result array is returned.
 *
 *      create_internal_structures()
 *
 *  Next is the create_internal_structures() function.  This is what allocates
 *  the internal data structures and fills them in, translating from the array
 *  representation passed in to the graph structure used internally.
 *
 *  To start with, at code point 10, this function makes some assertions about
 *  its input parameters.  Then, malloc() is called to allocate element_info,
 *  the array of element_global_info objects.  If the malloc() fails (code
 *  point 11), the appropriate message is printed to standard error and NULL is
 *  both returned and set in the location pointed to by link_arrays_location,
 *  to indicate failure.
 *
 *  Next (code point 13), the array of pointers to link_info objects is
 *  allocated.  There is one element in this array for each of the input arrays
 *  to be merged.  If the allocation fails (code point 14) what element_info
 *  points to is freed and again an error message is printed to standard error
 *  and NULL is both returned and set in the location pointed to by
 *  link_arrays_location.
 *
 *  After that, at code point 16, it's time to set up the initial state of the
 *  elements in the array pointed to by element_info.  For each element (code
 *  point 17), the state is set to ES_UNVISITED, the incoming link count is set
 *  to zero, and the first_link field is set to NULL.  That is, the graph
 *  structure is set to have no edges.  The first_unsearched_link field is set
 *  to point to the first_link field to indicate that none of the links has
 *  been walked looking for cycles yet.  Later in this function we will walk
 *  the input arrays and add edges to the graph as they are discovered.
 *
 *  Next (code point 18), comes the allocation of all the arrays of link_info
 *  objects and the building of the edge arrays.  The head_list is also built
 *  at this time.  The array_num variable is the index for the loop through the
 *  input arrays.  Note that we loop through them backwards, starting from the
 *  highest number and ending with the first array.  This is because edges are
 *  added to the front of the head and successor linked lists, so going through
 *  the arrays in reverse order means these linked lists will end up with the
 *  edges from the lower-numbered arrays before those of the higher numbered
 *  arrays.  This will make the elements of the earlier arrays tend to come
 *  before those of the later arrays all else being equal, which is desirable.
 *
 *  For each of the input arrays (code point 19) we first have the decrement of
 *  the index variable of the loop and then array_size and this_array are set
 *  to grab the elements of array_sizes and ``arrays'' respectively that
 *  correspond to this input array.  Assertions make sure that these have
 *  reasonable values.  Then malloc() is called to allocate the array of
 *  link_info objects for this one of the input arrays to be merged.
 *
 *  If the malloc() call fails (code point 20) an error is printed to standard
 *  error.  Then, all the memory that was allocated so far has to be cleaned
 *  up.  In general, at this point some but not all of the link_info arrays
 *  have been allocated, so only those that have already been allocated need to
 *  be freed.  This is done by first incrementing array_num (because the
 *  allocation that was supposed to be for the old value of array_num failed
 *  and shouldn't be cleaned up) and then deallocating the array for each
 *  element of link_arrays from there up to but not including num_arrays.
 *  Before each array is freed (code point 21) the code asserts that it is
 *  actually an allocated array of the expected size.  Then (code point 22) the
 *  top-level link_arrays array is freed as is the element_info array.  This
 *  completes the cleanup and then the location pointed to by
 *  link_arrays_location is set to NULL and NULL is returned to indicate the
 *  failure of this function.
 *
 *  If the malloc() succeeded (code point 24), the input array is walked and
 *  the array of link_info objects is used to add to all the appropriate linked
 *  lists.  Note that the first element in the input array needs to be added to
 *  the head list while every other element needs to be added to the successor
 *  list of the element that comes before it in the input array.  This is done
 *  by using a single loop but making sure that the source_list variable points
 *  to the head list on the first iteration and to the successor list of the
 *  previous element on every other iteration.  At the start of each iteration
 *  (code point 25), the number of the core element of this position of the
 *  input array is read into the target_element local variable.  Then an
 *  assertion checks that the element is in the proper range.
 *
 *  The next two lines implement a check that no single element appears more
 *  than once in the same input array.  The ``state'' field is used for this
 *  purpose since it's not being otherwise used in the
 *  create_internal_structures() function.  The state is set to
 *  ES_BEING_VISITED for each element as it is seen in the array.  Before it is
 *  set, though, an assertion checks that the value of the state was
 *  ES_UNVISITED.  If the same element appears in the input array more than
 *  once, this assertion will trigger the second time the element is seen in
 *  the list.  Later, after the input array has been walked all the way
 *  through (code point 29), the array will be walked again to set all the
 *  states back to ES_UNVISITED to make sure that if the element is seen in
 *  another list, the assertion isn't triggered.
 *
 *  The next line increments the incoming link count for this core element.
 *  Note that the incoming link count will therefor count both incoming links
 *  that are edges from other core elements and also incoming links from the
 *  head list.  Then the corresponding element of the link_info array has its
 *  fields set.  The target_element field is set to the present core element
 *  and the next_link field is set to whatever used to be the head of what
 *  source_list points to.  Then comes special code for handling the
 *  head_list_tail pointer.  If the head list was empty and this element is
 *  being added to the head list, the head_list_tail pointer must be updated to
 *  point to the next field of this link element.  This case is detected by
 *  checking to see if the source_list is equal to what the head_list_tail
 *  points to.  This can only happen if the source_list is the head list and if
 *  the tail of the head list is the start of the list.  When this is the case
 *  (code point 26), the head_list_tail is set to point to the next pointer of
 *  the link_info object being added to the list.  Since successive objects are
 *  added to the start of the list, once the head_list_tail has been set to the
 *  next pointer of an object, it doesn't need to be changed in this function.
 *
 *  After that, at code point 28, what's pointed to by the source_list pointer
 *  is updated to point to the new element and then the source_list pointer is
 *  set to the location of the successor link array for the present element, so
 *  that on the next iteration, the successor element will be added to the
 *  correct list.
 *
 *  Once every element of the input array has been handled (code point 29),
 *  it's time to set all the state fields back to ES_UNVISITED so we're ready
 *  for the next input array (and so that all the states will be ES_UNVISITED
 *  when this function is done).  Since link_num is already at the end of the
 *  input array, it's convenient to simply step it backwards through all the
 *  elements of the input array.  For each such element (code point 30) a
 *  couple of assertions sanity check the value of the element and the state of
 *  that core element then set the state back to ES_UNVISITED.  When that is
 *  done (code point 31) the appropriate element of link_arrays is set to the
 *  newly allocated and set up array of link_info objects.
 *
 *  When all the input arrays have been walked and the linked lists filled in
 *  (code point 32), all that's left is to assert that each core element's
 *  incoming link count is greater than zero (code point 33).  This is
 *  equivalent to checking that each core element appears in at least one of
 *  the input arrays, because the incoming link count is initially (before
 *  cycles are removed) equal to the number of times that core element appears
 *  in all of the input arrays to be merged.
 *
 *  When that's done (code point 34), all the data structures are set up, so
 *  they are returned to the caller.
 *
 *      remove_cycles()
 *
 *  As its name implies, the remove_cycles() function removes cycles from the
 *  graph until there are no cycles left.  For each cycle that is found, every
 *  edge in the cycle is removed, not just one of them.  It returns TRUE if
 *  this element was not found to be a part of a cycle that includes the edge
 *  going into it and FALSE if this element is part of a cycle that includes
 *  the edge going into it.
 *
 *  This function starts by making some assertions about its inputs.  Then it
 *  examines the start_element.  It sets the element_record local to point to
 *  the element_global_info record for this element.  Then it switches on the
 *  state of the element.
 *
 *  If the state is ES_UNVISITED (code point 36), then this element hasn't yet
 *  been visited in the walk, or has only been visited on walks during which it
 *  was found to be part of a cycle and for which the incoming edge was
 *  deleted.  In this case, the state is set to ES_BEING_VISITED.  Then, each
 *  of the link coming out of this element is walked until a cycle is found or
 *  all the links from this object have been walked.  Note that if a cycle is
 *  found, the link that is part of that cycle will be removed, so the
 *  iteration through the list out outgoing links uses a local variable of type
 *  ``link_info **'', not a variable of type ``link_info *''.  At each
 *  iteration it is set to point to the pointer in the list that points to the
 *  current link_info object, which is the pointer that needs to be updated to
 *  remove that element from the list.  That's why the link_pointer variable is
 *  set initially to the first_unsearched_link field of the core element.
 *  During each iteration, it is either set to the location of the next_link
 *  field of the element, unless the current element was just removed and we're
 *  continuing through the loop, in which case it doesn't have to be changed at
 *  all because the thing that's next after this has changed.
 *
 *  Inside the loop, the this_link local is set to point to the current element
 *  of the linked list.  The target_element local is used to store the number
 *  of the core element pointed to by the current element of the edge linked
 *  list.  Then remove_cycles() is called recursively on the target.  If the
 *  recursive call returns FALSE then a cycle has been found that includes this
 *  edge.  Further, when such a cycle is found, the ``state'' field for the
 *  element that was found to complete the cycle is set to ES_FINISHED.  This
 *  is to indicate how far back up the call chain edges must be removed.
 *
 *  When the code finds that the recursive call indicated this edge is part of
 *  a cycle (code pointe 38), it first stashes away the pointer to the next
 *  element in the linked list in the next_link variable.  Since the next field
 *  may change before the next_link is used, it is critical to store this
 *  before messing with the linked list.  Then the incoming link count for the
 *  target element is read.  Since this link points to that target, the
 *  incoming link count must be at least one, so the code here asserts that.
 *  But whether it is exactly one or not is important to what happens next.  If
 *  it is exactly one (code point 39), then when this link is removed, there
 *  will be no links coming into the target node.  So in this case the target
 *  node must be added to the head list.  Since we are traversing the head list
 *  already, it must be added to the end of the head list to make sure the walk
 *  actually reaches it again.  So the code at code point 39 sets the tail of
 *  the head list to point to the element being removed from the successor
 *  list, updates the pointer to the tail of the head list to point to the next
 *  pointer of this element, and sets that next field to NULL.  Note that in
 *  this case the incoming link count is unchanged because the link from
 *  another node is replaced by a link from the head list.  If the incoming
 *  link count was greater than one (code point 40), then all that needs to be
 *  done is to decrement the incoming link count.  The old link_info element
 *  doesn't have to be put in another list since it will be dropped and not be
 *  part of any list after this.  The code at code point 41 does the actual
 *  removing of the link_info element from the successor list by setting what's
 *  pointed to by the link_pointer to point to what next_link points to.
 *
 *  What happens next depends on the ``state'' field of the start element.
 *  Recall that before beginning the walk through the successors of the start
 *  element the state was set to ES_BEING_VISITED.  When a cycle is found, the
 *  state of the element that closes the cycle is changed to ES_FINISHED to
 *  indicate that this is where the loop is closed.  So the code here checks
 *  the state.  If it is ES_FINISHED (code point 42), then the link we just
 *  removed is the last one that needs to be removed to completely remove the
 *  discovered cycle.  So in this case the state is set back to
 *  ES_BEING_VISITED and the walk through the successor nodes can continue.  If
 *  the state wasn't set to ES_FINISHED (code point 43), then this isn't the
 *  last edge that needs to be removed.  So we set the first_unsearched_link
 *  field to store how far through the list of successors we got, so that if
 *  and when we revisit this node we don't have to go through paths we've
 *  already been through.  Then we set the state to ES_UNVISITED because now if
 *  we reach it again we shouldn't consider it part of a cycle because it's not
 *  on the stack of elements indicating our current path, and it's not finished
 *  being handled.  Then the function returns FALSE to indicate that the edge
 *  coming into this call needs to be removed also.
 *
 *  The next code is at code point 45, which is the ``else'' case for the test
 *  to see if the current link needs to be removed.  In this case, it does not
 *  need to be removed, so we simply update link_pointer to point to the next
 *  link in the list.
 *
 *  After that, the next code comes at code point 47, which follows the
 *  completion of the loop through the successors of this element.  If this
 *  point is reached, then we've made it through all the successors without
 *  finding a cycle that includes the edge coming into the start node for this
 *  call, so we set the state to ES_FINISHED because we know that we've walked
 *  everything reachable from this node so if we ever reach it again in this
 *  walk, we know that we don't have to look, we will find no cycles through
 *  this node.  Then we return TRUE to indicate that the edge that was followed
 *  to reach this node does not have to be removed.
 *
 *  The next case is the case that the state of the element when we enter this
 *  function is ES_BEING_VISITED.  In this case, we've just found the node that
 *  closes a cycle.  So we set the state of that node to ES_FINISHED so that
 *  when we're removing edges as we go back up the call chain we don't go past
 *  this element.  Then we return FALSE to indicate that the edge followed to
 *  reach this node is part of a cycle and needs to be removed.
 *
 *  The final possible case is that the state of the element when we enter this
 *  function is ES_FINISHED.  That means we've already walked everything
 *  reachable from this node and not found a cycle, so we can safely assume we
 *  don't have to walk it again.  So we simply return TRUE to indicate no
 *  cycles including the edge along which this node was reached.
 *
 *  Finally, there is a default case that simply asserts FALSE.  This should
 *  never be reachable unless the data structure is corrupt.  It has to return
 *  something at this point to make the compiler happy, so it returns FALSE (on
 *  the theory that TRUE means everything is fine, though it doesn't matter
 *  much what it returns -- either way, things are messed up and not likely to
 *  end up with the correct answer).
 *
 *      order_elements()
 *
 *  The order_elements() function is used to walk an acyclic graph and produce
 *  an ordering that is consistent with the graph.  Since the graph is acyclic,
 *  there must be such an ordering.
 *
 *  The first thing this function does (code point 50) is some assertions on
 *  its parameters.  Then, it tries to allocate the array for the final result.
 *  The final result always has to have num_elements elements.  If the malloc()
 *  attempt fails (code point 51), an appropriate error message is printed and
 *  NULL is returned to indicate failure.
 *
 *  Next, at code point 53, this code walks the graph and fills in the result
 *  array.  It walks through the head list and for each element in the head
 *  list, recursively walks starting from that element, using the
 *  walk_and_fill() function to do the walking (code point 54).  In this walk,
 *  the last time an element is reached, it is put in the output and all
 *  successor nodes are walked.  Successor nodes aren't walked until the last
 *  time it is reached.  This insures that before a core element goes into the
 *  final output array, all predecessors have been put there.  When the walk is
 *  done (code point 55), all that's left to do is assert that every element
 *  has been filled in in the output array and then return that output array.
 *
 *      walk_and_fill()
 *
 *  As mentioned above, the walk_and_fill() function does the final walk of the
 *  graph and puts the core elements in the result array.  Since it needs to
 *  handle each element only the last time it is reached and nothing else will
 *  use the graph after it's done, it simply decrements the count of incoming
 *  links and ignores the element until that incoming link count reaches zero.
 *
 *  The first thing this function does (code point 56) is some assertions about
 *  its parameters.  Then it sets the local element_record local to point to
 *  the element_global_info record for the core element in question.  For good
 *  measure, it checks to see that the state is set to ES_FINISHED -- there's
 *  no reason it wouldn't be and the state no longer matters, but this
 *  assertion might catch certain kinds of data structure corruption.  Then the
 *  incoming link count is read and decremented.  The original value is
 *  asserted to be at least one.  If it is more than that (code point 57), this
 *  function simply returns -- it will wait to deal with the code until the
 *  link count is one, indicating the last link into this core element has just
 *  been followed.
 *
 *  If we reach code point 59, then it is time to handle this element.  First,
 *  we assert that there is room left in the result array for it.  Then this
 *  core element is placed in the output array and the pointer to the next
 *  location in the output array is incremented.  After that, all the outgoing
 *  edges from this node are followed and this function is called recursively
 *  on the target of each such edge (code point 60).  When this is done, the
 *  function exits (code point 61).
 *
 *      delete_internal_structures()
 *
 *  The final function is the delete_internal_structures() function.  This
 *  function simply deletes the internal data structures that have been
 *  allocated.  It assumes that the complete set of internal data structures
 *  has been set up.
 *
 *  The first thing it does (code point 62) is make some assertions about the
 *  data structures it is about to deallocate.  Then it frees element_info
 *  (this is a simple array).  After that, it deallocates each of the arrays
 *  pointed to by the elements of the array pointed to by the link_arrays
 *  parameter (code point 63).  Finally, it frees the array pointed to by
 *  link_arrays itself (code point 64).
 *
 *  And that's the last of the code in this module.
 *
 *
 *      History
 *
 *  See the history discussion in merge_dense_integer_arrays.h for the history
 *  of this code.
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
 *  @@@
 *
 *          Chris Wilson, 2005, 2007
 */


#define STACK_BUFFER_SIZE 100

/*
 *  The maximum number of exponent digits we might encounter is the greater of
 *  the number of decimal digits in LDBL_MAX_10_EXP and the number of decimal
 *  digits in (LDBL_MIN_10_EXP - 1).  We can be conservative about the number
 *  of decimal digits in either of these two values by taking the number of
 *  bytes needed to represent each of the constants, multiplying that by 3
 *  (since each byte adds a factor of 256, which is less than 10^3) and adding
 *  2 (one for the first digit, which may be a zero, and one in case
 *  subtracting one bumps up the digit count).  And adding these two values
 *  gives a conservative bound on the maximum of them.
 */
#define MAX_EXPONENT_DIGITS \
        (((sizeof(LDBL_MAX_10_EXP) + sizeof(LDBL_MIN_10_EXP)) * 3) + 4)


typedef struct exponent_rollover_effect
  {
    boolean change_output_mantissa_digit_count;
    size_t new_output_mantissa_digit_count;
    boolean change_digits_before_decimal_point;
    size_t new_digits_before_decimal_point;
    boolean change_print_decimal_point;
    boolean new_print_decimal_point;
    boolean change_use_exponent_notation;
    boolean new_use_exponent_notation;
    boolean change_stuffed_zeros;
    size_t new_stuffed_zeros;
  } exponent_rollover_effect;

struct floating_point_output_control
  {
    verdict (*character_output_function)(void *data, char output_character);
    void *character_output_data;
    boolean rounding_done_early;
    verdict verdict_so_far;
    boolean value_is_special_non_numeric;
    boolean did_early_exponent;
    boolean did_late_exponent;
    boolean possible_left_padding;
    boolean possible_right_padding;
    boolean possible_zero_padding;
    size_t minimum_width;
    boolean exponent_notation_use_decided;
    size_t negative_exponent_limit_for_exponent_notation;
    boolean use_exponent_notation;
    boolean decimal_point_use_decided;
    boolean print_decimal_point;
    char exponent_marker_character;
    boolean print_plus_sign_if_positive;
    boolean print_space_if_positive;
    boolean suppress_trailing_zeros;
    boolean fixed_number_of_digits_after_decimal_point;
    exponent_rollover_effect the_exponent_rollover_effect;
    size_t field_characters;
    boolean mantissa_is_negative;
    size_t stuffed_zeros;
    boolean have_pending_mantissa_digit;
    char pending_mantissa_digit;
    size_t pending_mantissa_nine_count;
    size_t pending_mantissa_zero_count;
    boolean increment_exponent;
    size_t digits_before_decimal_point;
    size_t output_mantissa_digit_count;
    size_t requested_mantissa_digit_count;
    size_t received_mantissa_digit_count;
    size_t buffered_mantissa_digit_count;
    size_t sent_mantissa_digit_count;
    boolean prefix_done;
    size_t digits_before_rounding;
    boolean do_received_mantissa_digit_buffering;
    char *received_mantissa_digit_buffer;
    char *stack_mantissa_buffer;
    boolean have_trailing_zero_count;
    unsigned long character_count;
    boolean exponent_digit_buffer_is_populated;
    size_t exponent_magnitude_for_buffer;
    char exponent_digit_buffer[MAX_EXPONENT_DIGITS];
    boolean exponent_is_negative;
    size_t exponent_digit_count;
    boolean exponent_digit_count_will_change_if_exponent_increments;
    size_t provisional_exponent_magnitude_size_t;
    size_t provisional_exponent_remaining_digit_count;
    const char *provisional_exponent_remaining_digits;
    size_t precision;
    char conversion_type_specification_character;
  };


static verdict complete_received_mantissa_digit_buffering(
        floating_point_output_control *output_control);
static verdict end_received_mantissa_digit_buffering(
        floating_point_output_control *output_control);
static verdict handle_carries_for_mantissa_digit(
        floating_point_output_control *output_control, char digit);
static verdict do_carry_expansion(
        floating_point_output_control *output_control);
static verdict handle_pending_mantissa_digits(
        floating_point_output_control *output_control);
static verdict handle_zeros_for_mantissa_digit(
        floating_point_output_control *output_control, char digit);
static verdict handle_mantissa_digit_with_buffering(
        floating_point_output_control *output_control, char digit);
static verdict send_mantissa_digit(
        floating_point_output_control *output_control, char digit);
static verdict start_late_exponent(
        floating_point_output_control *output_control);
static verdict finish_late_exponent(
        floating_point_output_control *output_control);
static verdict do_common_early_exponent(
        floating_point_output_control *output_control,
        boolean exponent_is_negative,
        size_t *new_requested_mantissa_digit_count);
static verdict do_floating_point_prefix(
        floating_point_output_control *output_control);
static verdict send_floating_point_character(
        floating_point_output_control *output_control, char to_send);
static void populate_exponent_buffer(
        floating_point_output_control *output_control,
        boolean exponent_is_negative, size_t exponent_magnitude);
static verdict do_floating_point_padding(
        floating_point_output_control *output_control, char pad_character);
static void mark_exponent_within_non_exponent_notation_limit(
        floating_point_output_control *output_control,
        size_t exponent_magnitude, boolean exponent_is_negative);
static void mark_exponent_outside_non_exponent_notation_limit(
        floating_point_output_control *output_control);
static size_t find_exponent_limit_for_exponent_notation(
        floating_point_output_control *output_control,
        boolean exponent_is_negative);
static void set_up_exponent_rollover_effect_for_no_format_change(
        floating_point_output_control *output_control,
        boolean use_exponent_notation);
static void set_up_exponent_rollover_effect_for_switch_to_exponent_notation(
        floating_point_output_control *output_control,
        size_t new_exponent_magnitude);
static void set_up_exponent_rollover_effect_for_switch_from_exponent_notation(
        floating_point_output_control *output_control,
        size_t new_exponent_magnitude);
static void calculate_field_characters(
        floating_point_output_control *output_control);
static boolean exponent_magnitude_is_greater_than(
        floating_point_output_control *output_control,
        size_t comparison_value);


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
    verdict (*character_output_function)(void *data, char output_character),
    void *character_output_data, boolean rounding_done_early,
    size_t *output_character_count)
  {
    floating_point_output_control output_control;
    char stack_mantissa_buffer[STACK_BUFFER_SIZE];
    verdict the_verdict;

    code_point(1);
    output_control.character_output_function = character_output_function;
    output_control.character_output_data = character_output_data;
    output_control.rounding_done_early = rounding_done_early;
    output_control.verdict_so_far = MISSION_ACCOMPLISHED;
    output_control.value_is_special_non_numeric = FALSE;
    output_control.did_early_exponent = FALSE;
    output_control.did_late_exponent = FALSE;
    output_control.pending_mantissa_nine_count = 0;
    output_control.pending_mantissa_zero_count = 0;
    output_control.increment_exponent = FALSE;

    switch (padding_specification)
      {
        case PK_NO_PADDING:
            code_point(2);
            output_control.possible_left_padding = FALSE;
            output_control.possible_zero_padding = FALSE;
            output_control.possible_right_padding = FALSE;
            break;
        case PK_LEFT_SPACE_PADDING:
            code_point(3);
            output_control.possible_left_padding = TRUE;
            output_control.possible_zero_padding = FALSE;
            output_control.possible_right_padding = FALSE;
            break;
        case PK_LEFT_ZERO_PADDING:
            code_point(4);
            output_control.possible_left_padding = FALSE;
            output_control.possible_zero_padding = TRUE;
            output_control.possible_right_padding = FALSE;
            break;
        case PK_RIGHT_SPACE_PADDING:
            code_point(5);
            output_control.possible_left_padding = FALSE;
            output_control.possible_zero_padding = FALSE;
            output_control.possible_right_padding = TRUE;
            break;
        default:
            assert(FALSE);
      }

    code_point(6);
    output_control.minimum_width = minimum_width;
    output_control.conversion_type_specification_character =
            conversion_type_specification_character;
    output_control.exponent_notation_use_decided =
            exponent_notation_use_decided;
    output_control.negative_exponent_limit_for_exponent_notation =
            negative_exponent_limit_for_exponent_notation;
    output_control.use_exponent_notation = use_exponent_notation;
    output_control.fixed_number_of_digits_after_decimal_point =
            fixed_number_of_digits_after_decimal_point;
    output_control.decimal_point_use_decided = decimal_point_use_decided;
    output_control.print_decimal_point = print_decimal_point;
    output_control.suppress_trailing_zeros = suppress_trailing_zeros;
    output_control.exponent_marker_character = exponent_marker_character;
    output_control.print_plus_sign_if_positive = print_plus_sign_if_positive;

    output_control.print_space_if_positive = print_space_if_positive;
    output_control.exponent_digit_buffer_is_populated = FALSE;

    if (!output_control.suppress_trailing_zeros)
      {
        code_point(7);
        if (!(output_control.decimal_point_use_decided))
          {
            code_point(8);
            output_control.print_decimal_point = TRUE;
          }
        else
          {
            code_point(9);
          }
        code_point(10);
      }
    else
      {
        code_point(11);
      }

    code_point(12);
    output_control.stuffed_zeros = 0;
    output_control.have_pending_mantissa_digit = FALSE;

    output_control.precision = precision;
    output_control.output_mantissa_digit_count =
            requested_mantissa_digit_count;

    if ((!(output_control.rounding_done_early)) &&
        (requested_mantissa_digit_count < ~(size_t)0))
      {
        code_point(13);
        ++requested_mantissa_digit_count;
      }
    else
      {
        code_point(14);
      }

    code_point(15);
    output_control.requested_mantissa_digit_count =
            requested_mantissa_digit_count;

    output_control.character_count = 0;

    output_control.received_mantissa_digit_count = 0;
    output_control.buffered_mantissa_digit_count = 0;
    output_control.sent_mantissa_digit_count = 0;
    output_control.prefix_done = FALSE;
    output_control.digits_before_rounding = 0;
    output_control.have_trailing_zero_count = FALSE;
    output_control.do_received_mantissa_digit_buffering =
            (output_control.suppress_trailing_zeros &&
             (output_control.possible_left_padding ||
              output_control.possible_zero_padding));
    output_control.received_mantissa_digit_buffer = NULL;
    output_control.stack_mantissa_buffer = &(stack_mantissa_buffer[0]);

    the_verdict = (*conversion_function)(function_data, &output_control,
            output_control.requested_mantissa_digit_count,
            output_control.suppress_trailing_zeros, value_data,
            mantissa_is_negative);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        code_point(16);
        return the_verdict;
      }

    code_point(17);
    *output_character_count = output_control.character_count;

    if (output_control.value_is_special_non_numeric)
      {
        code_point(451);
        assert(!(output_control.did_early_exponent));
        assert(!(output_control.did_late_exponent));
      }
    else
      {
        code_point(452);
        assert(output_control.did_early_exponent);
        assert(output_control.did_late_exponent);
      }

    code_point(453);
    return MISSION_ACCOMPLISHED;
  }

extern verdict handle_mantissa_digit(
        floating_point_output_control *output_control, char digit)
  {
    code_point(18);
    assert(output_control != NULL);
    assert(!(output_control->value_is_special_non_numeric));
    assert((digit >= '0') && (digit <= ('9' + 1)));

    ++(output_control->received_mantissa_digit_count);

    return handle_carries_for_mantissa_digit(output_control, digit);
  }

static verdict complete_received_mantissa_digit_buffering(
        floating_point_output_control *output_control)
  {
    size_t digit_num;

    code_point(19);
    assert(output_control != NULL);
    assert(output_control->do_received_mantissa_digit_buffering);

    digit_num = output_control->received_mantissa_digit_count;
    while ((digit_num > 1) &&
           (output_control->received_mantissa_digit_buffer[digit_num - 1] ==
            '0'))
      {
        code_point(20);
        --digit_num;
      }
    code_point(21);
    return notify_of_mantissa_trailing_zero_count(output_control,
            (output_control->received_mantissa_digit_count - digit_num));
  }

static verdict end_received_mantissa_digit_buffering(
        floating_point_output_control *output_control)
  {
    size_t digit_num;

    code_point(22);
    assert(output_control != NULL);
    assert(output_control->do_received_mantissa_digit_buffering);

    for (digit_num = 0;
         digit_num < output_control->buffered_mantissa_digit_count;
         ++digit_num)
      {
        verdict the_verdict;

        code_point(23);
        assert(output_control->received_mantissa_digit_buffer != NULL);
        the_verdict = send_mantissa_digit(output_control,
                output_control->received_mantissa_digit_buffer[digit_num]);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(24);
            return the_verdict;
          }
        code_point(25);
      }

    code_point(26);
    output_control->do_received_mantissa_digit_buffering = FALSE;

    if ((output_control->received_mantissa_digit_buffer != NULL) &&
        (output_control->received_mantissa_digit_buffer !=
         output_control->stack_mantissa_buffer))
      {
        code_point(27);
        free(output_control->received_mantissa_digit_buffer);
        output_control->received_mantissa_digit_buffer = NULL;
      }
    else
      {
        code_point(28);
      }

    code_point(29);
    return MISSION_ACCOMPLISHED;
  }

static verdict handle_carries_for_mantissa_digit(
        floating_point_output_control *output_control, char digit)
  {
    verdict the_verdict;

    code_point(30);
    assert(output_control != NULL);
    assert(output_control->did_early_exponent);
    assert(!(output_control->did_late_exponent));
    assert(output_control->verdict_so_far == MISSION_ACCOMPLISHED);
    assert((digit >= '0') && (digit <= ('9' + 1)));

    if (output_control->digits_before_rounding > 0)
      {
        code_point(31);
        --(output_control->digits_before_rounding);
        if (output_control->digits_before_rounding == 0)
          {
            code_point(32);
            if (digit <= '5')
              {
                code_point(33);
                digit += 5;
              }
            else
              {
                code_point(34);
                digit = ('9' + 1);
              }
            code_point(35);
          }
        else
          {
            code_point(36);
          }
        code_point(37);
      }
    else
      {
        code_point(38);
      }

    code_point(39);
    if (digit == ('9' + 1))
      {
        size_t nine_count;

        code_point(40);
        nine_count = output_control->pending_mantissa_nine_count;

        if (output_control->have_pending_mantissa_digit)
          {
            char pending_digit;

            code_point(41);
            pending_digit = output_control->pending_mantissa_digit;
            assert((pending_digit >= '0') && (pending_digit <= '8'));

            ++pending_digit;
            assert((pending_digit >= '1') && (pending_digit <= '9'));

            the_verdict = handle_zeros_for_mantissa_digit(output_control,
                                                          pending_digit);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                code_point(42);
                return the_verdict;
              }
            code_point(43);
          }
        else if (output_control->stuffed_zeros > 0)
          {
            code_point(44);
            --output_control->stuffed_zeros;

            the_verdict = handle_zeros_for_mantissa_digit(output_control,
                                                          '1');
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                code_point(45);
                return the_verdict;
              }
            code_point(46);
          }
        else
          {
            code_point(47);
            the_verdict = do_carry_expansion(output_control);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                code_point(48);
                return the_verdict;
              }
            code_point(49);
          }

        code_point(50);
        while (nine_count > 0)
          {
            code_point(51);
            the_verdict = handle_zeros_for_mantissa_digit(output_control, '0');
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                code_point(52);
                return the_verdict;
              }
            code_point(53);
            --nine_count;
          }

        code_point(54);
        output_control->pending_mantissa_nine_count = 0;
        output_control->have_pending_mantissa_digit = TRUE;
        output_control->pending_mantissa_digit = '0';
      }
    else if (digit == '9')
      {
        code_point(55);
        ++(output_control->pending_mantissa_nine_count);
        the_verdict = MISSION_ACCOMPLISHED;
      }
    else
      {
        code_point(56);
        the_verdict = handle_pending_mantissa_digits(output_control);
        output_control->have_pending_mantissa_digit = TRUE;
        output_control->pending_mantissa_digit = digit;
      }

    code_point(57);
    return the_verdict;
  }

static verdict do_carry_expansion(
        floating_point_output_control *output_control)
  {
    verdict the_verdict;

    code_point(58);
    assert(output_control != NULL);

    if (output_control->the_exponent_rollover_effect.
                change_output_mantissa_digit_count)
      {
        code_point(59);
        output_control->output_mantissa_digit_count =
                output_control->the_exponent_rollover_effect.
                        new_output_mantissa_digit_count;
      }
    else
      {
        code_point(60);
      }

    code_point(61);
    if (output_control->the_exponent_rollover_effect.
                change_digits_before_decimal_point)
      {
        code_point(62);
        output_control->digits_before_decimal_point =
                output_control->the_exponent_rollover_effect.
                        new_digits_before_decimal_point;
      }
    else
      {
        code_point(63);
      }

    code_point(64);
    if (output_control->the_exponent_rollover_effect.
                change_print_decimal_point)
      {
        code_point(65);
        output_control->print_decimal_point =
                output_control->the_exponent_rollover_effect.
                        new_print_decimal_point;
      }
    else
      {
        code_point(66);
      }

    code_point(67);
    if (output_control->the_exponent_rollover_effect.
                change_use_exponent_notation)
      {
        code_point(68);
        output_control->use_exponent_notation =
                output_control->the_exponent_rollover_effect.
                        new_use_exponent_notation;
      }
    else
      {
        code_point(69);
      }

    code_point(70);
    if (output_control->the_exponent_rollover_effect.
                change_stuffed_zeros)
      {
        code_point(71);
        output_control->stuffed_zeros =
                output_control->the_exponent_rollover_effect.
                        new_stuffed_zeros;
      }
    else
      {
        code_point(72);
      }

    code_point(73);
    output_control->increment_exponent = TRUE;
    output_control->exponent_digit_buffer_is_populated = FALSE;

    if (output_control->
                exponent_digit_count_will_change_if_exponent_increments)
      {
        code_point(74);
        if (output_control->exponent_is_negative)
          {
            code_point(75);
            assert(output_control->exponent_digit_count > 0);
            --(output_control->exponent_digit_count);
          }
        else
          {
            code_point(76);
            ++(output_control->exponent_digit_count);
          }
        code_point(77);
      }
    else
      {
        code_point(78);
      }

    code_point(79);
    the_verdict = handle_zeros_for_mantissa_digit(output_control, '1');
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        code_point(80);
        return the_verdict;
      }

    code_point(81);
    return MISSION_ACCOMPLISHED;
  }

static verdict handle_pending_mantissa_digits(
        floating_point_output_control *output_control)
  {
    size_t nine_count;
    verdict the_verdict;

    code_point(82);
    nine_count = output_control->pending_mantissa_nine_count;

    if (output_control->have_pending_mantissa_digit)
      {
        code_point(83);
        the_verdict = handle_zeros_for_mantissa_digit(output_control,
                output_control->pending_mantissa_digit);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(84);
            return the_verdict;
          }
        code_point(85);
      }
    else
      {
        code_point(86);
      }

    code_point(87);
    while (nine_count > 0)
      {
        code_point(88);
        the_verdict = handle_zeros_for_mantissa_digit(output_control, '9');
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(89);
            return the_verdict;
          }
        code_point(90);
        --nine_count;
      }

    code_point(91);
    output_control->pending_mantissa_nine_count = 0;

    return MISSION_ACCOMPLISHED;
  }

static verdict handle_zeros_for_mantissa_digit(
        floating_point_output_control *output_control, char digit)
  {
    code_point(92);
    assert(output_control != NULL);
    assert((digit >= '0') && (digit <= '9'));

    if ((output_control->sent_mantissa_digit_count +
         output_control->pending_mantissa_zero_count) >=
        output_control->output_mantissa_digit_count)
      {
        code_point(93);
        return MISSION_ACCOMPLISHED;
      }

    code_point(94);
    if (digit == '0')
      {
        code_point(95);
        ++(output_control->pending_mantissa_zero_count);
      }
    else
      {
        size_t zero_count;
        verdict the_verdict;

        code_point(96);
        zero_count = output_control->pending_mantissa_zero_count;
        while (zero_count > 0)
          {
            verdict the_verdict;

            code_point(97);
            the_verdict =
                    handle_mantissa_digit_with_buffering(output_control, '0');
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                code_point(98);
                return the_verdict;
              }

            code_point(99);
            --zero_count;
          }
        code_point(100);
        output_control->pending_mantissa_zero_count = 0;

        the_verdict =
                handle_mantissa_digit_with_buffering(output_control, digit);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(101);
            return the_verdict;
          }
        code_point(102);
      }

    code_point(103);
    return MISSION_ACCOMPLISHED;
  }

static verdict handle_mantissa_digit_with_buffering(
        floating_point_output_control *output_control, char digit)
  {
    code_point(104);
    assert(output_control != NULL);
    assert((digit >= '0') && (digit <= '9'));

    if (output_control->do_received_mantissa_digit_buffering)
      {
        size_t digit_num;
        size_t next_digit_num;

        code_point(105);
        digit_num = output_control->buffered_mantissa_digit_count;
        next_digit_num = digit_num + 1;
        output_control->buffered_mantissa_digit_count = next_digit_num;

        if (output_control->received_mantissa_digit_buffer == NULL)
          {
            size_t buffer_characters;

            code_point(106);
            assert(output_control->requested_mantissa_digit_count > 0);
            buffer_characters = (output_control->output_mantissa_digit_count -
                                 output_control->stuffed_zeros);

            if (buffer_characters > STACK_BUFFER_SIZE)
              {
                code_point(107);
                output_control->received_mantissa_digit_buffer =
                        MALLOC_ARRAY(char, buffer_characters);
                if (output_control->received_mantissa_digit_buffer == NULL)
                  {
                    code_point(108);
                    return MISSION_FAILED;
                  }
                code_point(109);
              }
            else
              {
                code_point(110);
                output_control->received_mantissa_digit_buffer =
                        output_control->stack_mantissa_buffer;
              }
            code_point(111);
          }
        else
          {
            code_point(112);
          }

        code_point(113);
        assert(output_control->received_mantissa_digit_buffer != NULL);
        output_control->received_mantissa_digit_buffer[digit_num] = digit;

        if (next_digit_num ==
            (output_control->output_mantissa_digit_count -
             output_control->stuffed_zeros))
          {
            verdict the_verdict;

            code_point(114);
            the_verdict =
                    complete_received_mantissa_digit_buffering(output_control);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                code_point(115);
                return the_verdict;
              }
            code_point(116);
          }
        else
          {
            code_point(117);
          }

        code_point(118);
        return MISSION_ACCOMPLISHED;
      }
    else
      {
        code_point(119);
        return send_mantissa_digit(output_control, digit);
      }
  }

static verdict send_mantissa_digit(
        floating_point_output_control *output_control, char digit)
  {
    verdict the_verdict;
    size_t digit_num;

    code_point(120);
    if (!(output_control->prefix_done))
      {
        verdict the_verdict;

        code_point(121);
        the_verdict = do_floating_point_prefix(output_control);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(122);
            return the_verdict;
          }
        code_point(123);
      }
    else
      {
        code_point(124);
      }

    code_point(125);
    digit_num = output_control->sent_mantissa_digit_count;
    assert(digit_num < output_control->output_mantissa_digit_count);

    if (digit_num == output_control->digits_before_decimal_point)
      {
        code_point(126);
        the_verdict = send_floating_point_character(output_control, '.');
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(127);
            return the_verdict;
          }
        code_point(128);
      }
    else
      {
        code_point(129);
      }

    code_point(130);
    ++digit_num;
    output_control->sent_mantissa_digit_count = digit_num;

    the_verdict = send_floating_point_character(output_control, digit);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        code_point(131);
        return the_verdict;
      }

    code_point(132);
    return MISSION_ACCOMPLISHED;
  }

extern verdict early_exponent_by_size_t(
        floating_point_output_control *output_control,
        boolean mantissa_is_negative, boolean exponent_is_negative,
        size_t exponent_magnitude, size_t *new_requested_mantissa_digit_count)
  {
    code_point(133);
    assert(output_control != NULL);
    assert(!(output_control->value_is_special_non_numeric));
    assert(!(output_control->did_early_exponent));
    assert(!(output_control->did_late_exponent));
    assert(output_control->verdict_so_far == MISSION_ACCOMPLISHED);

    output_control->mantissa_is_negative = mantissa_is_negative;

    output_control->provisional_exponent_magnitude_size_t = exponent_magnitude;
    output_control->provisional_exponent_remaining_digit_count = 0;
    output_control->provisional_exponent_remaining_digits = NULL;

    if (output_control->use_exponent_notation)
      {
        code_point(134);
        if ((!(output_control->exponent_digit_buffer_is_populated)) ||
            (output_control->exponent_magnitude_for_buffer !=
             exponent_magnitude))
          {
            code_point(135);
            populate_exponent_buffer(output_control, exponent_is_negative,
                                     exponent_magnitude);
          }
        else
          {
            code_point(136);
          }
        code_point(137);
        assert(output_control->exponent_digit_buffer_is_populated);
        assert(output_control->exponent_magnitude_for_buffer ==
               exponent_magnitude);
        assert(output_control->exponent_digit_count >= 0);
      }
    else
      {
        code_point(138);
      }

    code_point(139);
    return do_common_early_exponent(output_control, exponent_is_negative,
                                    new_requested_mantissa_digit_count);
  }

extern verdict early_exponent_by_digits(
        floating_point_output_control *output_control,
        boolean mantissa_is_negative, boolean exponent_is_negative,
        size_t exponent_digit_count, const char *exponent_digits,
        size_t *new_requested_mantissa_digit_count)
  {
    code_point(140);
    assert(output_control != NULL);
    assert(!(output_control->value_is_special_non_numeric));
    assert(!(output_control->did_early_exponent));
    assert(!(output_control->did_late_exponent));
    assert(output_control->verdict_so_far == MISSION_ACCOMPLISHED);
    assert(new_requested_mantissa_digit_count != NULL);

    output_control->mantissa_is_negative = mantissa_is_negative;

    output_control->provisional_exponent_magnitude_size_t = 0;
    output_control->provisional_exponent_remaining_digit_count =
            exponent_digit_count;
    output_control->provisional_exponent_remaining_digits = exponent_digits;

    if (output_control->use_exponent_notation)
      {
        code_point(141);
        output_control->exponent_is_negative = exponent_is_negative;
        output_control->exponent_digit_count = exponent_digit_count;

        if (exponent_is_negative)
          {
            code_point(142);
            assert(exponent_digit_count > 0);
            if (exponent_digits[0] != '1')
              {
                code_point(143);
                output_control->
                        exponent_digit_count_will_change_if_exponent_increments
                                = FALSE;
              }
            else
              {
                size_t digit_num;

                code_point(144);
                digit_num = 1;
                while (TRUE)
                  {
                    code_point(145);
                    if (digit_num == exponent_digit_count)
                      {
                        code_point(146);
                        output_control->
                        exponent_digit_count_will_change_if_exponent_increments
                                = TRUE;
                        break;
                      }
                    code_point(147);
                    if (exponent_digits[digit_num] != '0')
                      {
                        code_point(148);
                        output_control->
                        exponent_digit_count_will_change_if_exponent_increments
                                = FALSE;
                        break;
                      }
                    code_point(149);
                    ++digit_num;
                  }
                code_point(150);
              }
            code_point(151);
          }
        else
          {
            code_point(152);
            if (exponent_digit_count == 0)
              {
                code_point(153);
                output_control->
                        exponent_digit_count_will_change_if_exponent_increments
                                = TRUE;
              }
            else
              {
                size_t digit_num;

                code_point(154);
                digit_num = 0;
                while (TRUE)
                  {
                    code_point(155);
                    if (digit_num == exponent_digit_count)
                      {
                        code_point(156);
                        output_control->
                        exponent_digit_count_will_change_if_exponent_increments
                                = TRUE;
                        break;
                      }
                    code_point(157);
                    if (exponent_digits[digit_num] != '9')
                      {
                        code_point(158);
                        output_control->
                        exponent_digit_count_will_change_if_exponent_increments
                                = FALSE;
                        break;
                      }
                    code_point(159);
                    ++digit_num;
                  }
                code_point(160);
              }
            code_point(161);
          }
        code_point(162);
      }
    else
      {
        code_point(163);
      }

    code_point(164);
    return do_common_early_exponent(output_control, exponent_is_negative,
                                    new_requested_mantissa_digit_count);
  }

extern verdict late_exponent_by_size_t(
        floating_point_output_control *output_control,
        boolean exponent_is_negative, size_t exponent_magnitude)
  {
    verdict the_verdict;

    code_point(165);
    assert(!(exponent_is_negative && (exponent_magnitude == 0)));

    the_verdict = start_late_exponent(output_control);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        code_point(166);
        return the_verdict;
      }

    code_point(167);
    if (output_control->use_exponent_notation)
      {
        verdict the_verdict;
        size_t digit_num;

        code_point(168);
        the_verdict = send_floating_point_character(output_control,
                output_control->exponent_marker_character);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(169);
            return the_verdict;
          }

        code_point(170);
        if (output_control->increment_exponent && exponent_is_negative)
          {
            code_point(171);
            assert(exponent_magnitude > 0);
            --exponent_magnitude;
            if (exponent_magnitude == 0)
              {
                code_point(172);
                exponent_is_negative = FALSE;
              }
            else
              {
                code_point(173);
              }
            code_point(174);
            output_control->increment_exponent = FALSE;
          }
        else
          {
            code_point(175);
          }
        code_point(176);
        the_verdict = send_floating_point_character(output_control,
                (exponent_is_negative ? '-' : '+'));
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(177);
            return the_verdict;
          }

        code_point(178);
        if ((!(output_control->exponent_digit_buffer_is_populated)) ||
            (output_control->exponent_magnitude_for_buffer !=
             exponent_magnitude))
          {
            code_point(179);
            populate_exponent_buffer(output_control, exponent_is_negative,
                                     exponent_magnitude);
          }
        else
          {
            code_point(180);
          }
        code_point(181);
        assert(output_control->exponent_digit_buffer_is_populated);
        assert(output_control->exponent_magnitude_for_buffer ==
               exponent_magnitude);
        assert(output_control->exponent_digit_count >= 0);

        if (output_control->exponent_digit_count < 2)
          {
            code_point(182);
            the_verdict = send_floating_point_character(output_control, '0');
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                code_point(183);
                return the_verdict;
              }
            code_point(184);
          }
        else
          {
            code_point(185);
          }
        code_point(186);
        if (output_control->exponent_digit_count < 1)
          {
            code_point(187);
            the_verdict = send_floating_point_character(output_control, '0');
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                code_point(188);
                return the_verdict;
              }
            code_point(189);
          }
        else
          {
            code_point(190);
          }

        code_point(191);
        for (digit_num = output_control->exponent_digit_count; digit_num > 0;
             --digit_num)
          {
            code_point(192);
            the_verdict = send_floating_point_character(output_control,
                    output_control->exponent_digit_buffer[digit_num - 1]);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                code_point(193);
                return the_verdict;
              }
            code_point(194);
          }
        code_point(195);
      }
    else
      {
        code_point(196);
      }

    code_point(197);
    return finish_late_exponent(output_control);
  }

extern verdict late_exponent_by_digits(
        floating_point_output_control *output_control,
        boolean exponent_is_negative, size_t exponent_digit_count,
        const char *exponent_digits)
  {
    verdict the_verdict;

    code_point(198);
    the_verdict = start_late_exponent(output_control);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        code_point(199);
        return the_verdict;
      }

    code_point(200);
    if (output_control->use_exponent_notation)
      {
        verdict the_verdict;
        boolean increment_exponent;
        boolean do_force;
        char pending_digit;
        size_t possible_carry_count;
        size_t digit_num;

        code_point(201);
        the_verdict = send_floating_point_character(output_control,
                output_control->exponent_marker_character);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(202);
            return the_verdict;
          }

        code_point(203);
        increment_exponent = output_control->increment_exponent;
        if (increment_exponent &&
            output_control->
                    exponent_digit_count_will_change_if_exponent_increments)
          {
            code_point(204);
            do_force = TRUE;
            if (exponent_is_negative)
              {
                code_point(205);
                assert(exponent_digit_count > 0);
                assert(exponent_digits[0] == '1');
                --exponent_digit_count;
                if (exponent_digit_count == 0)
                  {
                    code_point(206);
                    exponent_is_negative = FALSE;
                  }
                else
                  {
                    code_point(207);
                  }
                code_point(208);
              }
            else
              {
                code_point(209);
                ++exponent_digit_count;
              }
            code_point(210);
          }
        else
          {
            code_point(211);
            do_force = FALSE;
          }

        code_point(212);
        the_verdict = send_floating_point_character(output_control,
                (exponent_is_negative ? '-' : '+'));
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(213);
            return the_verdict;
          }

        code_point(214);
        if (exponent_digit_count < 2)
          {
            code_point(215);
            the_verdict = send_floating_point_character(output_control, '0');
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                code_point(216);
                return the_verdict;
              }
            code_point(217);
          }
        else
          {
            code_point(218);
          }
        code_point(219);
        if (exponent_digit_count < 1)
          {
            code_point(220);
            the_verdict = send_floating_point_character(output_control, '0');
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                code_point(221);
                return the_verdict;
              }
            code_point(222);
          }
        else
          {
            code_point(223);
          }

        code_point(224);
        possible_carry_count = 0;
        for (digit_num = 0; digit_num < exponent_digit_count; ++digit_num)
          {
            char to_send;

            code_point(225);
            if (increment_exponent)
              {
                code_point(226);
                if (do_force)
                  {
                    code_point(227);
                    if (exponent_is_negative)
                      {
                        code_point(228);
                        assert(exponent_digits[digit_num + 1] == '0');
                        to_send = '9';
                      }
                    else
                      {
                        code_point(229);
                        if (digit_num == 0)
                          {
                            code_point(230);
                            to_send = '1';
                          }
                        else
                          {
                            code_point(231);
                            assert(exponent_digits[digit_num - 1] == '9');
                            to_send = '0';
                          }
                        code_point(232);
                      }
                    code_point(233);
                  }
                else
                  {
                    char next_char;

                    code_point(234);
                    next_char = exponent_digits[digit_num];
                    if (next_char == (exponent_is_negative ? '0' : '9'))
                      {
                        code_point(235);
                        ++possible_carry_count;
                        continue;
                      }

                    code_point(236);
                    while (possible_carry_count > 0)
                      {
                        code_point(237);
                        the_verdict = send_floating_point_character(
                                output_control,
                                (exponent_is_negative ? '0' : '9'));
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            code_point(238);
                            return the_verdict;
                          }
                        code_point(239);
                        --possible_carry_count;
                      }

                    code_point(240);
                    to_send = pending_digit;
                    pending_digit = next_char;

                    if (digit_num == 0)
                      {
                        code_point(241);
                        continue;
                      }
                    code_point(242);
                  }
                code_point(243);
              }
            else
              {
                code_point(244);
                to_send = exponent_digits[digit_num];
              }
            code_point(245);
            the_verdict =
                    send_floating_point_character(output_control, to_send);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                code_point(246);
                return the_verdict;
              }
            code_point(247);
          }

        code_point(248);
        if (increment_exponent && !do_force)
          {
            code_point(249);
            if (exponent_is_negative)
              {
                code_point(250);
                assert(pending_digit > '0');
                --pending_digit;
              }
            else
              {
                code_point(251);
                assert(pending_digit < '9');
                ++pending_digit;
              }
            code_point(252);
            the_verdict = send_floating_point_character(output_control,
                                                        pending_digit);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                code_point(253);
                return the_verdict;
              }

            code_point(254);
            while (possible_carry_count > 0)
              {
                code_point(255);
                the_verdict = send_floating_point_character(output_control,
                        (exponent_is_negative ? '9' : '0'));
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    code_point(256);
                    return the_verdict;
                  }
                code_point(257);
                --possible_carry_count;
              }
            code_point(258);
          }
        else
          {
            code_point(259);
          }
        code_point(260);
      }
    else
      {
        code_point(261);
      }

    code_point(262);
    return finish_late_exponent(output_control);
  }

extern verdict notify_of_mantissa_trailing_zero_count(
        floating_point_output_control *output_control,
        size_t trailing_zero_count)
  {
    size_t mantissa_digit_count;

    code_point(263);
    assert(output_control != NULL);
    assert(!(output_control->value_is_special_non_numeric));
    assert((output_control->did_early_exponent));
    assert(!(output_control->did_late_exponent));
    assert(trailing_zero_count <= output_control->output_mantissa_digit_count);
    assert(trailing_zero_count <=
           output_control->requested_mantissa_digit_count);

    assert(!(output_control->have_trailing_zero_count));
    output_control->have_trailing_zero_count = TRUE;

    if (!output_control->suppress_trailing_zeros)
      {
        code_point(264);
        return MISSION_ACCOMPLISHED;
      }

    code_point(265);
    mantissa_digit_count =
            ((output_control->requested_mantissa_digit_count +
              output_control->stuffed_zeros) - trailing_zero_count);
    assert(output_control->stuffed_zeros <= mantissa_digit_count);

    if (mantissa_digit_count > output_control->output_mantissa_digit_count)
      {
        code_point(266);
        mantissa_digit_count = output_control->output_mantissa_digit_count;
      }
    else
      {
        code_point(267);
      }

    code_point(268);
    assert(output_control->digits_before_decimal_point > 0);
    if (mantissa_digit_count < output_control->digits_before_decimal_point)
      {
        code_point(269);
        mantissa_digit_count = output_control->digits_before_decimal_point;
      }
    else
      {
        code_point(270);
      }
    code_point(271);
    assert(output_control->stuffed_zeros <= mantissa_digit_count);

    assert((output_control->requested_mantissa_digit_count +
            output_control->stuffed_zeros) >= mantissa_digit_count);
    if (mantissa_digit_count < output_control->output_mantissa_digit_count)
      {
        code_point(272);
        output_control->output_mantissa_digit_count = mantissa_digit_count;
      }
    else
      {
        code_point(273);
      }

    code_point(274);
    if (!(output_control->decimal_point_use_decided))
      {
        code_point(275);
        if (mantissa_digit_count > output_control->digits_before_decimal_point)
          {
            code_point(276);
            output_control->print_decimal_point = TRUE;
          }
        else
          {
            code_point(277);
            output_control->print_decimal_point = FALSE;
          }
        code_point(278);
      }
    else
      {
        code_point(279);
      }

    code_point(280);
    if (output_control->do_received_mantissa_digit_buffering)
      {
        code_point(281);
        return end_received_mantissa_digit_buffering(output_control);
      }
    else
      {
        code_point(282);
        return MISSION_ACCOMPLISHED;
      }
  }

static verdict start_late_exponent(
        floating_point_output_control *output_control)
  {
    verdict the_verdict;
    size_t zero_count;

    code_point(283);
    assert(output_control != NULL);
    assert(!(output_control->value_is_special_non_numeric));
    assert(output_control->did_early_exponent);
    assert(!(output_control->did_late_exponent));
    assert(output_control->verdict_so_far == MISSION_ACCOMPLISHED);
    assert(output_control->received_mantissa_digit_count <=
           output_control->requested_mantissa_digit_count);

    zero_count = output_control->pending_mantissa_zero_count;

    if (output_control->suppress_trailing_zeros)
      {
        code_point(284);
        assert(output_control->output_mantissa_digit_count >=
               output_control->digits_before_decimal_point);
        if ((output_control->output_mantissa_digit_count - zero_count) <
            output_control->digits_before_decimal_point)
          {
            code_point(285);
            output_control->pending_mantissa_zero_count =
                    (output_control->output_mantissa_digit_count -
                     output_control->digits_before_decimal_point);
            output_control->output_mantissa_digit_count =
                    output_control->digits_before_decimal_point;
          }
        else
          {
            code_point(286);
            output_control->output_mantissa_digit_count =
                    (output_control->output_mantissa_digit_count - zero_count);
            output_control->pending_mantissa_zero_count = 0;
          }
        code_point(287);
        assert(output_control->output_mantissa_digit_count >=
               output_control->digits_before_decimal_point);
      }
    else
      {
        code_point(288);
      }

    code_point(289);
    if (output_control->do_received_mantissa_digit_buffering)
      {
        verdict the_verdict;

        code_point(290);
        the_verdict =
                complete_received_mantissa_digit_buffering(output_control);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(291);
            return the_verdict;
          }
        code_point(292);
      }
    else
      {
        code_point(293);
      }

    code_point(294);
    assert(!(output_control->do_received_mantissa_digit_buffering));

    if (!(output_control->prefix_done))
      {
        verdict the_verdict;

        code_point(295);
        the_verdict = do_floating_point_prefix(output_control);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(296);
            return the_verdict;
          }
        code_point(297);
      }
    else
      {
        code_point(298);
      }

    code_point(299);
    the_verdict = handle_pending_mantissa_digits(output_control);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        code_point(300);
        return the_verdict;
      }

    code_point(301);
    if (!(output_control->have_trailing_zero_count))
      {
        code_point(302);
        assert((output_control->sent_mantissa_digit_count +
                output_control->pending_mantissa_zero_count) >=
               output_control->output_mantissa_digit_count);
      }
    else
      {
        code_point(303);
        if ((output_control->sent_mantissa_digit_count +
             output_control->pending_mantissa_zero_count) <
            output_control->output_mantissa_digit_count)
          {
            code_point(304);
            output_control->pending_mantissa_zero_count =
                    (output_control->output_mantissa_digit_count -
                     output_control->sent_mantissa_digit_count);
          }
        else
          {
            code_point(305);
          }
        code_point(306);
      }

    code_point(307);
    assert((output_control->sent_mantissa_digit_count +
            output_control->pending_mantissa_zero_count) >=
           output_control->output_mantissa_digit_count);

    output_control->did_late_exponent = TRUE;

    zero_count = output_control->pending_mantissa_zero_count;

    while (zero_count > 0)
      {
        verdict the_verdict;

        code_point(308);
        the_verdict = send_mantissa_digit(output_control, '0');
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(309);
            return the_verdict;
          }

        code_point(310);
        --zero_count;
      }
    code_point(311);
    output_control->pending_mantissa_zero_count = 0;

    if (output_control->suppress_trailing_zeros)
      {
        code_point(312);
        if (!(output_control->decimal_point_use_decided))
          {
            code_point(313);
            if (output_control->output_mantissa_digit_count >
                output_control->digits_before_decimal_point)
              {
                code_point(314);
                output_control->print_decimal_point = TRUE;
              }
            else
              {
                code_point(315);
                output_control->print_decimal_point = FALSE;
              }
            code_point(316);
          }
        else
          {
            code_point(317);
          }
        code_point(318);
      }
    else
      {
        code_point(319);
      }

    code_point(320);
    assert(output_control->output_mantissa_digit_count >=
           output_control->digits_before_decimal_point);
    if ((output_control->output_mantissa_digit_count ==
         output_control->digits_before_decimal_point) &&
        output_control->decimal_point_use_decided &&
        output_control->print_decimal_point)
      {
        verdict the_verdict;

        code_point(321);
        the_verdict = send_floating_point_character(output_control, '.');
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(322);
            return the_verdict;
          }
        code_point(323);
      }
    else
      {
        code_point(324);
      }

    code_point(325);
    return MISSION_ACCOMPLISHED;
  }

static verdict finish_late_exponent(
        floating_point_output_control *output_control)
  {
    assert(output_control != NULL);

    code_point(326);
    if (output_control->possible_right_padding)
      {
        code_point(327);
        calculate_field_characters(output_control);
        return do_floating_point_padding(output_control, ' ');
      }

    code_point(328);
    return MISSION_ACCOMPLISHED;
  }

static verdict do_common_early_exponent(
        floating_point_output_control *output_control,
        boolean exponent_is_negative,
        size_t *new_requested_mantissa_digit_count)
  {
    code_point(329);
    assert(output_control != NULL);
    assert(!(output_control->did_early_exponent));
    assert(!(output_control->did_late_exponent));
    assert(output_control->verdict_so_far == MISSION_ACCOMPLISHED);

    output_control->did_early_exponent = TRUE;

    if (output_control->exponent_notation_use_decided &&
        (!output_control->use_exponent_notation))
      {
        code_point(330);
        if (!(exponent_is_negative))
          {
            size_t exponent_magnitude_size_t;

            code_point(331);
            if (exponent_magnitude_is_greater_than(output_control,
                        ((~(size_t)0) -
                         (*new_requested_mantissa_digit_count))))
              {
                code_point(332);
                basic_error(
                        "The exponent of the value for an `f' conversion "
                        "specification plus the precision was so huge that not"
                        " enough memory could be allocated to deal with it.");
                return MISSION_FAILED;
              }

            code_point(333);
            assert(output_control->provisional_exponent_remaining_digit_count
                   == 0);
            exponent_magnitude_size_t =
                    output_control->provisional_exponent_magnitude_size_t;
            output_control->requested_mantissa_digit_count +=
                    exponent_magnitude_size_t;
            output_control->output_mantissa_digit_count +=
                    exponent_magnitude_size_t;
            output_control->digits_before_decimal_point =
                    (1 + exponent_magnitude_size_t);
          }
        else
          {
            size_t original_requested_mantissa_digit_count;

            code_point(334);
            original_requested_mantissa_digit_count =
                    output_control->requested_mantissa_digit_count;
            if (exponent_magnitude_is_greater_than(output_control,
                        original_requested_mantissa_digit_count))
              {
                code_point(335);
                output_control->requested_mantissa_digit_count = 0;
                output_control->stuffed_zeros =
                        original_requested_mantissa_digit_count;
              }
            else
              {
                size_t exponent_magnitude_size_t;

                code_point(336);
                assert(output_control->
                               provisional_exponent_remaining_digit_count ==
                       0);
                exponent_magnitude_size_t =
                        output_control->provisional_exponent_magnitude_size_t;
                output_control->requested_mantissa_digit_count =
                        (original_requested_mantissa_digit_count -
                         exponent_magnitude_size_t);
                output_control->stuffed_zeros = exponent_magnitude_size_t;
              }

            code_point(337);
            output_control->digits_before_decimal_point = 1;
          }
        code_point(338);
        *new_requested_mantissa_digit_count =
                output_control->requested_mantissa_digit_count;
      }
    else
      {
        code_point(339);
        output_control->digits_before_decimal_point = 1;
      }

    code_point(340);
    if (!(output_control->exponent_notation_use_decided))
      {
        size_t exponent_limit;

        code_point(341);
        exponent_limit = find_exponent_limit_for_exponent_notation(
                output_control, exponent_is_negative);

        if ((exponent_limit == 0) ||
            exponent_magnitude_is_greater_than(output_control,
                                               (exponent_limit - 1)))
          {
            code_point(342);
            mark_exponent_outside_non_exponent_notation_limit(output_control);
            if (exponent_is_negative &&
                !exponent_magnitude_is_greater_than(output_control,
                                                    exponent_limit))
              {
                code_point(343);
            set_up_exponent_rollover_effect_for_switch_from_exponent_notation(
                    output_control, (exponent_limit - 1));
              }
            else
              {
                code_point(344);
                set_up_exponent_rollover_effect_for_no_format_change(
                        output_control, output_control->use_exponent_notation);
              }
            code_point(345);
          }
        else
          {
            size_t exponent_magnitude_size_t;

            code_point(346);
            assert(output_control->provisional_exponent_remaining_digit_count
                   == 0);
            exponent_magnitude_size_t =
                    output_control->provisional_exponent_magnitude_size_t;
            mark_exponent_within_non_exponent_notation_limit(output_control,
                    exponent_magnitude_size_t, exponent_is_negative);
            output_control->exponent_notation_use_decided = TRUE;
            if ((!exponent_is_negative) &&
                ((exponent_magnitude_size_t + 1) == exponent_limit))
              {
                code_point(347);
            set_up_exponent_rollover_effect_for_switch_to_exponent_notation(
                    output_control, exponent_limit);
              }
            else
              {
                code_point(348);
                set_up_exponent_rollover_effect_for_no_format_change(
                        output_control, output_control->use_exponent_notation);
              }
            code_point(349);
          }
        code_point(350);
      }
    else
      {
        code_point(351);
        set_up_exponent_rollover_effect_for_no_format_change(output_control,
                output_control->use_exponent_notation);
      }

    code_point(352);
    if (!(output_control->rounding_done_early))
      {
        code_point(353);
        output_control->digits_before_rounding =
                ((output_control->output_mantissa_digit_count + 1) -
                 output_control->stuffed_zeros);
      }
    else
      {
        code_point(354);
      }

    code_point(355);
    *new_requested_mantissa_digit_count =
            output_control->requested_mantissa_digit_count;
    return MISSION_ACCOMPLISHED;
  }

static verdict do_floating_point_prefix(
        floating_point_output_control *output_control)
  {
    verdict the_verdict;
    size_t digit_num;

    code_point(356);
    assert(output_control != NULL);
    assert(!(output_control->prefix_done));

    output_control->prefix_done = TRUE;

    calculate_field_characters(output_control);

    if (output_control->possible_left_padding)
      {
        code_point(357);
        the_verdict = do_floating_point_padding(output_control, ' ');
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(358);
            return the_verdict;
          }
        code_point(359);
      }
    else
      {
        code_point(360);
      }

    code_point(361);
    if (output_control->mantissa_is_negative)
      {
        code_point(362);
        the_verdict = send_floating_point_character(output_control, '-');
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(363);
            return the_verdict;
          }
        code_point(364);
      }
    else if (output_control->print_plus_sign_if_positive)
      {
        code_point(365);
        the_verdict = send_floating_point_character(output_control, '+');
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(366);
            return the_verdict;
          }
        code_point(367);
      }
    else if (output_control->print_space_if_positive)
      {
        code_point(368);
        the_verdict = send_floating_point_character(output_control, ' ');
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(369);
            return the_verdict;
          }
        code_point(370);
      }
    else
      {
        code_point(371);
      }

    code_point(372);
    if (output_control->possible_zero_padding)
      {
        code_point(373);
        the_verdict = do_floating_point_padding(output_control, '0');
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(374);
            return the_verdict;
          }
        code_point(375);
      }
    else
      {
        code_point(376);
      }

    code_point(377);
    for (digit_num = 0; digit_num < output_control->stuffed_zeros; ++digit_num)
      {
        code_point(378);
        the_verdict = send_mantissa_digit(output_control, '0');
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(379);
            return the_verdict;
          }
        code_point(380);
      }

    code_point(381);
    return MISSION_ACCOMPLISHED;
  }

static verdict send_floating_point_character(
        floating_point_output_control *output_control, char to_send)
  {
    verdict the_verdict;

    code_point(382);
    assert(output_control != NULL);
    assert(output_control->verdict_so_far == MISSION_ACCOMPLISHED);

    the_verdict = (*(output_control->character_output_function))(
            output_control->character_output_data, to_send);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        code_point(383);
        output_control->verdict_so_far = the_verdict;
        return the_verdict;
      }

    code_point(384);
    if (output_control->character_count < (unsigned long)UINT_MAX)
      {
        code_point(385);
        ++(output_control->character_count);
      }
    else
      {
        code_point(386);
      }

    code_point(387);
    return MISSION_ACCOMPLISHED;
  }

static void populate_exponent_buffer(
        floating_point_output_control *output_control,
        boolean exponent_is_negative, size_t exponent_magnitude)
  {
    size_t remainder;
    size_t position;
    boolean will_change;

    code_point(388);
    assert(output_control != NULL);
    assert((!(output_control->exponent_digit_buffer_is_populated)) ||
           (output_control->exponent_magnitude_for_buffer !=
            exponent_magnitude));

    remainder = exponent_magnitude;
    position = 0;
    will_change = TRUE;
    while (remainder > 0)
      {
        char new_digit;

        code_point(389);
        assert(position < MAX_EXPONENT_DIGITS);
        new_digit = (((char)(remainder % 10)) + '0');
        remainder /= 10;
        assert((new_digit >= '0') && (new_digit <= '9'));
        if ((position == 0) && output_control->increment_exponent)
          {
            code_point(390);
            if (new_digit == '9')
              {
                code_point(391);
                new_digit = '0';
                ++remainder;
              }
            else
              {
                code_point(392);
                assert((new_digit >= '0') && (new_digit <= '8'));
                ++new_digit;
              }
            code_point(393);
          }
        else
          {
            code_point(394);
          }
        code_point(395);
        assert((new_digit >= '0') && (new_digit <= '9'));
        output_control->exponent_digit_buffer[position] = new_digit;
        if (will_change &&
            (exponent_is_negative ?
             ((remainder == 0) ? (new_digit != '1') : (new_digit != '0')) :
             (new_digit != '9')))
          {
            code_point(396);
            will_change = FALSE;
          }
        else
          {
            code_point(397);
          }
        code_point(398);
        ++position;
      }
    code_point(399);
    output_control->exponent_is_negative = exponent_is_negative;
    output_control->exponent_digit_count = position;
    output_control->exponent_digit_count_will_change_if_exponent_increments =
            will_change;

    output_control->exponent_magnitude_for_buffer = exponent_magnitude;
    output_control->exponent_digit_buffer_is_populated = TRUE;
  }

extern verdict special_non_numeric_value(
        floating_point_output_control *output_control,
        const char *output_string)
  {
    const char *follow;
    size_t field_characters;
    boolean print_sign_space;

    code_point(477);
    assert(output_control != NULL);
    assert(output_string != NULL);
    assert(!(output_control->value_is_special_non_numeric));
    assert(!(output_control->did_early_exponent));
    assert(!(output_control->did_late_exponent));
    assert(output_control->verdict_so_far == MISSION_ACCOMPLISHED);

    output_control->value_is_special_non_numeric = TRUE;

    follow = output_string;
    while (*follow != 0)
      {
        code_point(478);
        ++follow;
      }
    code_point(479);
    field_characters = (follow - output_string);

    follow = output_string;
    if ((*follow == '+') && (!(output_control->print_plus_sign_if_positive)))
      {
        code_point(480);
        ++follow;
        if (output_control->print_space_if_positive)
          {
            code_point(481);
            print_sign_space = TRUE;
          }
        else
          {
            code_point(482);
            assert(field_characters > 0);
            --field_characters;
            print_sign_space = FALSE;
          }
        code_point(483);
      }
    else
      {
        code_point(484);
        print_sign_space = FALSE;
      }

    code_point(485);
    output_control->field_characters = field_characters;

    if ((output_control->possible_left_padding) ||
        (output_control->possible_zero_padding))
      {
        verdict the_verdict;

        code_point(486);
        the_verdict = do_floating_point_padding(output_control, ' ');
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(487);
            return the_verdict;
          }
        code_point(488);
      }
    else
      {
        code_point(489);
      }

    code_point(490);
    if (print_sign_space)
      {
        verdict the_verdict;

        code_point(491);
        the_verdict = send_floating_point_character(output_control, ' ');
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(492);
            return the_verdict;
          }
        code_point(493);
      }
    else
      {
        code_point(494);
      }

    code_point(495);
    while (*follow != 0)
      {
        verdict the_verdict;

        code_point(496);
        the_verdict = send_floating_point_character(output_control, *follow);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(497);
            return the_verdict;
          }
        code_point(498);
        ++follow;
      }

    code_point(499);
    if (output_control->possible_right_padding)
      {
        verdict the_verdict;

        code_point(500);
        the_verdict = do_floating_point_padding(output_control, ' ');
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(501);
            return the_verdict;
          }
        code_point(502);
      }
    else
      {
        code_point(503);
      }

    code_point(504);
    return MISSION_ACCOMPLISHED;
  }

extern void too_many_floating_point_mantissa_digits(
        floating_point_output_control *output_control)
  {
    code_point(400);
    assert(output_control != NULL);
    assert(!(output_control->value_is_special_non_numeric));

    too_many_floating_point_mantissa_digits_by_details(
            output_control->precision,
            output_control->conversion_type_specification_character);
  }

extern void too_many_floating_point_mantissa_digits_by_details(
        size_t precision, char conversion_type_specification_character)
  {
    code_point(401);
    basic_error(
            "The precision of `%lu' for a `%c' conversion specification was so"
            " huge that not enough memory could be allocated to deal with it.",
            (unsigned long)precision, conversion_type_specification_character);
  }


static verdict do_floating_point_padding(
        floating_point_output_control *output_control, char pad_character)
  {
    size_t pad_count;

    code_point(402);
    assert(output_control != NULL);

    if (output_control->minimum_width <= output_control->field_characters)
      {
        code_point(403);
        return MISSION_ACCOMPLISHED;
      }

    code_point(404);
    pad_count =
            (output_control->minimum_width - output_control->field_characters);

    while (pad_count > 0)
      {
        verdict the_verdict;

        code_point(405);
        the_verdict =
                send_floating_point_character(output_control, pad_character);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            code_point(406);
            return the_verdict;
          }
        code_point(407);
        --pad_count;
      }

    code_point(408);
    return MISSION_ACCOMPLISHED;
  }

static void mark_exponent_within_non_exponent_notation_limit(
        floating_point_output_control *output_control,
        size_t exponent_magnitude, boolean exponent_is_negative)
  {
    code_point(409);
    assert(output_control != NULL);
    assert(!(output_control->exponent_notation_use_decided));

    output_control->use_exponent_notation = FALSE;
    if (exponent_is_negative)
      {
        code_point(410);
        assert(output_control->stuffed_zeros == 0);
        output_control->stuffed_zeros = exponent_magnitude;
        output_control->requested_mantissa_digit_count -= exponent_magnitude;
      }
    else
      {
        code_point(411);
        output_control->digits_before_decimal_point += exponent_magnitude;
        assert(output_control->digits_before_decimal_point <=
               output_control->output_mantissa_digit_count);
      }
    code_point(412);
  }

static void mark_exponent_outside_non_exponent_notation_limit(
        floating_point_output_control *output_control)
  {
    code_point(413);
    assert(output_control != NULL);
    assert(!(output_control->exponent_notation_use_decided));

    output_control->use_exponent_notation = TRUE;
  }

static size_t find_exponent_limit_for_exponent_notation(
        floating_point_output_control *output_control,
        boolean exponent_is_negative)
  {
    code_point(414);
    assert(output_control != NULL);

    if (exponent_is_negative)
      {
        code_point(415);
        return 5;
      }
    else
      {
        code_point(416);
        return output_control->negative_exponent_limit_for_exponent_notation;
      }
  }

static void set_up_exponent_rollover_effect_for_no_format_change(
        floating_point_output_control *output_control,
        boolean use_exponent_notation)
  {
    code_point(417);
    output_control->the_exponent_rollover_effect.change_use_exponent_notation =
            FALSE;
    if (use_exponent_notation)
      {
        code_point(418);
        output_control->the_exponent_rollover_effect.
                change_output_mantissa_digit_count = FALSE;
        output_control->the_exponent_rollover_effect.
                change_digits_before_decimal_point = FALSE;
        output_control->the_exponent_rollover_effect.change_stuffed_zeros =
                FALSE;
      }
    else
      {
        code_point(419);
        output_control->the_exponent_rollover_effect.
                change_output_mantissa_digit_count = TRUE;
        output_control->the_exponent_rollover_effect.
                new_output_mantissa_digit_count =
                        output_control->output_mantissa_digit_count + 1;
        output_control->the_exponent_rollover_effect.
                change_digits_before_decimal_point = TRUE;
        output_control->the_exponent_rollover_effect.
                new_digits_before_decimal_point =
                        output_control->digits_before_decimal_point + 1;
        output_control->the_exponent_rollover_effect.change_stuffed_zeros =
                FALSE;
      }
    code_point(420);
    output_control->the_exponent_rollover_effect.change_print_decimal_point =
            FALSE;
  }

static void set_up_exponent_rollover_effect_for_switch_to_exponent_notation(
        floating_point_output_control *output_control,
        size_t new_exponent_magnitude)
  {
    code_point(421);
    output_control->the_exponent_rollover_effect.change_use_exponent_notation =
            TRUE;
    output_control->the_exponent_rollover_effect.new_use_exponent_notation =
            TRUE;
    output_control->the_exponent_rollover_effect.
            change_output_mantissa_digit_count = FALSE;
    output_control->the_exponent_rollover_effect.
            change_digits_before_decimal_point = TRUE;
    output_control->the_exponent_rollover_effect.
            new_digits_before_decimal_point =
                    (output_control->digits_before_decimal_point -
                     (new_exponent_magnitude - 1));
    output_control->the_exponent_rollover_effect.change_print_decimal_point =
            FALSE;
    output_control->the_exponent_rollover_effect.change_stuffed_zeros = FALSE;
  }

static void set_up_exponent_rollover_effect_for_switch_from_exponent_notation(
        floating_point_output_control *output_control,
        size_t new_exponent_magnitude)
  {
    code_point(422);
    output_control->the_exponent_rollover_effect.change_use_exponent_notation =
            TRUE;
    output_control->the_exponent_rollover_effect.new_use_exponent_notation =
            FALSE;
    output_control->the_exponent_rollover_effect.
            change_output_mantissa_digit_count = FALSE;
    output_control->the_exponent_rollover_effect.
            change_digits_before_decimal_point = FALSE;
    output_control->the_exponent_rollover_effect.change_print_decimal_point =
            FALSE;
    output_control->the_exponent_rollover_effect.change_stuffed_zeros = TRUE;
    output_control->the_exponent_rollover_effect.new_stuffed_zeros =
            new_exponent_magnitude;
  }

static void calculate_field_characters(
        floating_point_output_control *output_control)
  {
    boolean has_sign_character;
    size_t field_characters;

    code_point(423);
    if (output_control->mantissa_is_negative)
      {
        code_point(424);
        has_sign_character = TRUE;
      }
    else if (output_control->print_plus_sign_if_positive)
      {
        code_point(425);
        has_sign_character = TRUE;
      }
    else if (output_control->print_space_if_positive)
      {
        code_point(426);
        has_sign_character = TRUE;
      }
    else
      {
        code_point(427);
        has_sign_character = FALSE;
      }

    code_point(428);
    field_characters = output_control->output_mantissa_digit_count;

    if (has_sign_character)
      {
        code_point(429);
        ++field_characters;
      }
    else
      {
        code_point(430);
      }
    code_point(431);
    if (output_control->print_decimal_point)
      {
        code_point(432);
        ++field_characters;
      }
    else
      {
        code_point(433);
      }

    code_point(434);
    if (output_control->use_exponent_notation)
      {
        code_point(435);
        field_characters += (((output_control->exponent_digit_count < 2) ? 2 :
                              output_control->exponent_digit_count) + 2);
      }
    else
      {
        code_point(436);
      }

    code_point(437);
    output_control->field_characters = field_characters;
  }

static boolean exponent_magnitude_is_greater_than(
        floating_point_output_control *output_control, size_t comparison_value)
  {
    size_t magnitude_size_t;
    size_t remaining_digit_count;
    const char *remaining_digits;
    boolean result;

    code_point(438);
    assert(output_control != NULL);

    magnitude_size_t = output_control->provisional_exponent_magnitude_size_t;

    if (magnitude_size_t > comparison_value)
      {
        code_point(439);
        return TRUE;
      }

    code_point(440);
    remaining_digit_count =
            output_control->provisional_exponent_remaining_digit_count;
    if (remaining_digit_count == 0)
      {
        code_point(441);
        return FALSE;
      }

    code_point(442);
    remaining_digits = output_control->provisional_exponent_remaining_digits;

    assert(remaining_digits != NULL);

    while (TRUE)
      {
        char this_ascii_digit;
        size_t this_digit_value;

        code_point(443);
        this_ascii_digit = *remaining_digits;

        this_digit_value = (size_t)(this_ascii_digit - '0');

        if (magnitude_size_t > ((((size_t)~0) - this_digit_value) / 10))
          {
            code_point(444);
            result = TRUE;
            break;
          }

        code_point(445);
        magnitude_size_t = ((magnitude_size_t * 10) + this_digit_value);
        ++remaining_digits;
        --remaining_digit_count;

        if (magnitude_size_t > comparison_value)
          {
            code_point(446);
            result = TRUE;
            break;
          }

        code_point(447);
        if (remaining_digit_count == 0)
          {
            code_point(448);
            result = FALSE;
            break;
          }
        code_point(449);
      }

    code_point(450);
    output_control->provisional_exponent_magnitude_size_t = magnitude_size_t;
    output_control->provisional_exponent_remaining_digit_count =
            remaining_digit_count;
    output_control->provisional_exponent_remaining_digits = remaining_digits;

    return result;
  }
