/* file "o_integer.h" */

/*
 *  This file contains the interface to the o_integer module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef O_INTEGER_H
#define O_INTEGER_H

#include <stddef.h>
#include <limits.h>
#include <assert.h>
#include "c_foundations/basic.h"
#include "i_integer.h"


typedef struct
  {
    long the_long;
    i_integer *ii;
  } o_integer;


extern i_integer *oi_long_marker;
extern i_integer *oi_no_memory_marker;
extern i_integer *oi_positive_infinity_marker;
extern i_integer *oi_negative_infinity_marker;
extern i_integer *oi_unsigned_infinity_marker;
extern i_integer *oi_zero_zero_marker;

extern o_integer oi_positive_infinity;
extern o_integer oi_negative_infinity;
extern o_integer oi_unsigned_infinity;
extern o_integer oi_zero_zero;
extern o_integer oi_zero;
extern o_integer oi_one;
extern o_integer oi_null;


extern verdict init_o_integer_module(void);
extern void cleanup_o_integer_module(void);

extern verdict long_decimal_digit_count(long the_long, size_t *digit_count);
extern verdict long_write_decimal_digits(long the_long, char *location);
extern verdict long_hex_digits(long the_long, size_t *digit_count);
extern verdict long_write_hex_digits(long the_long, char *location,
                                     boolean uppercase);


#define negative_long_is_invertable(the_long) \
    (((LONG_MAX + LONG_MIN) < -1) ? \
     (the_long >= (((LONG_MAX + LONG_MIN) < -1) ? ((-LONG_MAX) - 1) : 0)) : \
     TRUE)
#define positive_long_is_invertable(the_long) \
    (((LONG_MAX + LONG_MIN) > -1) ? \
     (the_long <= (((LONG_MAX + LONG_MIN) > -1) ? (-(LONG_MIN + 1)) : 0)) : \
     TRUE)
#define invert_long(the_long)  ((-1) - (the_long))
#define negative_div_ceiling(numerator, denominator) \
    (((numerator % denominator) < 0) ? (numerator / denominator) : \
     ((numerator / denominator) + 1))
#define negative_div_floor(numerator, denominator) \
    (((numerator % denominator) < 0) ? (numerator / denominator) : \
     ((numerator / denominator) - 1))


#define oi_create_from_ii(result, the_ii) \
  { \
    assert((the_ii) != NULL); \
    if (ii_kind(the_ii) == IIK_FINITE) \
      { \
        verdict the_verdict; \
 \
        the_verdict = ii_to_long_int((the_ii), &((result).the_long)); \
        if (the_verdict == MISSION_ACCOMPLISHED) \
            (result).ii = oi_long_marker; \
        else \
            (result).ii = (the_ii); \
      } \
    else \
      { \
        (result).ii = (the_ii); \
      } \
  }

#define oi_create_from_ii_capture_reference(result, the_ii) \
  { \
    if ((the_ii) == NULL) \
      { \
        (result).ii = oi_no_memory_marker; \
      } \
    else \
      { \
        if (ii_kind(the_ii) == IIK_FINITE) \
          { \
            verdict the_verdict; \
 \
            the_verdict = ii_to_long_int((the_ii), &((result).the_long)); \
            if (the_verdict == MISSION_ACCOMPLISHED) \
              { \
                (result).ii = oi_long_marker; \
                ii_remove_reference(the_ii); \
              } \
            else \
              { \
                (result).ii = (the_ii); \
              } \
          } \
        else \
          { \
            (result).ii = (the_ii); \
          } \
      } \
  }

#define oi_create_from_decimal_ascii(result, digit_count, digits, \
                                     is_negative) \
  { \
    long oi_long_result; \
    const char *oi_end; \
    const char *oi_position; \
 \
    oi_long_result = 0; \
    oi_end = &(digits[digit_count]); \
    oi_position = digits; \
 \
    if (!is_negative) \
      { \
        while (TRUE) \
          { \
            long to_add; \
 \
            if (oi_position == oi_end) \
              { \
                (result).the_long = oi_long_result; \
                (result).ii = oi_long_marker; \
                break; \
              } \
 \
            if (oi_long_result > (LONG_MAX / 10)) \
              { \
                (result).ii = ii_create_from_decimal_ascii(digit_count, \
                        digits, is_negative); \
                if ((result).ii == NULL) \
                    (result).ii = oi_no_memory_marker; \
                break; \
              } \
            oi_long_result *= 10; \
 \
            assert(((*oi_position) >= '0') && ((*oi_position) <= '9')); \
            to_add = (*oi_position) - '0'; \
            if (oi_long_result > (LONG_MAX - to_add)) \
              { \
                (result).ii = ii_create_from_decimal_ascii(digit_count, \
                        digits, is_negative); \
                if ((result).ii == NULL) \
                    (result).ii = oi_no_memory_marker; \
                break; \
              } \
            oi_long_result += to_add; \
 \
            ++oi_position; \
          } \
      } \
    else \
      { \
        while (TRUE) \
          { \
            long to_subtract; \
 \
            if (oi_position == oi_end) \
              { \
                (result).the_long = oi_long_result; \
                (result).ii = oi_long_marker; \
                break; \
              } \
 \
            if (oi_long_result < negative_div_ceiling(LONG_MIN, 10)) \
              { \
                (result).ii = ii_create_from_decimal_ascii(digit_count, \
                        digits, is_negative); \
                if ((result).ii == NULL) \
                    (result).ii = oi_no_memory_marker; \
                break; \
              } \
            oi_long_result *= 10; \
 \
            assert(((*oi_position) >= '0') && ((*oi_position) <= '9')); \
            to_subtract = (*oi_position) - '0'; \
            if (oi_long_result < (LONG_MIN + to_subtract)) \
              { \
                (result).ii = ii_create_from_decimal_ascii(digit_count, \
                        digits, is_negative); \
                if ((result).ii == NULL) \
                    (result).ii = oi_no_memory_marker; \
                break; \
              } \
            oi_long_result -= to_subtract; \
 \
            ++oi_position; \
          } \
      } \
  }

#define oi_create_from_hex_ascii(result, digit_count, digits, is_negative) \
  { \
    long oi_long_result; \
    const char *oi_end; \
    const char *oi_position; \
 \
    oi_long_result = 0; \
    oi_end = &(digits[digit_count]); \
    oi_position = digits; \
 \
    if (!is_negative) \
      { \
        while (TRUE) \
          { \
            char new_char; \
            long to_add; \
 \
            if (oi_position == oi_end) \
              { \
                (result).the_long = oi_long_result; \
                (result).ii = oi_long_marker; \
                break; \
              } \
 \
            if (oi_long_result > (LONG_MAX / 16)) \
              { \
                (result).ii = ii_create_from_hex_ascii(digit_count, digits, \
                                                       is_negative); \
                if ((result).ii == NULL) \
                    (result).ii = oi_no_memory_marker; \
                break; \
              } \
            oi_long_result *= 16; \
 \
            new_char = *oi_position; \
            if ((new_char >= '0') && (new_char <= '9')) \
                to_add = new_char - '0'; \
            else if ((new_char >= 'a') && (new_char <= 'f')) \
                to_add = (new_char - 'a') + 0xa; \
            else if ((new_char >= 'A') && (new_char <= 'F')) \
                to_add = (new_char - 'A') + 0xa; \
            else \
                assert(FALSE); \
 \
            if (oi_long_result > (LONG_MAX - to_add)) \
              { \
                (result).ii = ii_create_from_hex_ascii(digit_count, digits, \
                                                       is_negative); \
                if ((result).ii == NULL) \
                    (result).ii = oi_no_memory_marker; \
                break; \
              } \
            oi_long_result += to_add; \
 \
            ++oi_position; \
          } \
      } \
    else \
      { \
        while (TRUE) \
          { \
            char new_char; \
            long to_subtract; \
 \
            if (oi_position == oi_end) \
              { \
                (result).the_long = oi_long_result; \
                (result).ii = oi_long_marker; \
                break; \
              } \
 \
            if (oi_long_result < negative_div_ceiling(LONG_MIN, 16)) \
              { \
                (result).ii = ii_create_from_hex_ascii(digit_count, digits, \
                                                       is_negative); \
                if ((result).ii == NULL) \
                    (result).ii = oi_no_memory_marker; \
                break; \
              } \
            oi_long_result *= 16; \
 \
            new_char = *oi_position; \
            if ((new_char >= '0') && (new_char <= '9')) \
                to_subtract = new_char - '0'; \
            else if ((new_char >= 'a') && (new_char <= 'f')) \
                to_subtract = (new_char - 'a') + 0xa; \
            else if ((new_char >= 'A') && (new_char <= 'F')) \
                to_subtract = (new_char - 'A') + 0xa; \
            else \
                assert(FALSE); \
 \
            if (oi_long_result < (LONG_MIN + to_subtract)) \
              { \
                (result).ii = ii_create_from_hex_ascii(digit_count, digits, \
                                                       is_negative); \
                if ((result).ii == NULL) \
                    (result).ii = oi_no_memory_marker; \
                break; \
              } \
            oi_long_result -= to_subtract; \
 \
            ++oi_position; \
          } \
      } \
  }

#define oi_create_from_size_t(result, the_size_t) \
  { \
    if ((the_size_t) <= LONG_MAX) \
      { \
        (result).the_long = (long)(the_size_t); \
        (result).ii = oi_long_marker; \
      } \
    else \
      { \
        (result).ii = ii_create_from_size_t(the_size_t); \
        if ((result).ii == NULL) \
            (result).ii = oi_no_memory_marker; \
      } \
  }

#define oi_create_from_long_int(result, the_long_int) \
  { \
    (result).the_long = the_long_int; \
    (result).ii = oi_long_marker; \
  }

#define oi_create_from_unsigned_long_int(result, the_long_int) \
  { \
    if ((the_long_int) <= LONG_MAX) \
      { \
        (result).the_long = (long)(the_long_int); \
        (result).ii = oi_long_marker; \
      } \
    else \
      { \
        (result).ii = ii_create_from_unsigned_long_int(the_long_int); \
        if ((result).ii == NULL) \
            (result).ii = oi_no_memory_marker; \
      } \
  }

#define ii_create_from_oi(the_oi) \
    (((the_oi).ii != oi_long_marker) ? \
     (ii_add_reference((the_oi).ii), (the_oi).ii) : \
     ii_create_from_long_int((the_oi).the_long))

#define oi_add_reference(the_oi) \
  { \
    if ((the_oi).ii != oi_long_marker) \
        ii_add_reference((the_oi).ii); \
  }

#define oi_remove_reference(the_oi) \
  { \
    if ((the_oi).ii != oi_long_marker) \
        ii_remove_reference((the_oi).ii); \
  }

#define oi_out_of_memory(the_oi)  ((the_oi).ii == oi_no_memory_marker)

#define oi_kind(the_oi) \
    (((the_oi).ii == oi_long_marker) ? IIK_FINITE : ii_kind((the_oi).ii))

#define oi_equal(oi1, oi2) \
    (((oi1).ii == oi_long_marker) ? \
     (((oi2).ii == oi_long_marker) ? ((oi1).the_long == (oi2).the_long) : \
                                     FALSE) : \
     (((oi2).ii == oi_long_marker) ? FALSE : ii_equal((oi1).ii, (oi2).ii)))

#define oi_less_than(oi1, oi2) \
    (((oi1).ii == oi_long_marker) ? \
     (((oi2).ii == oi_long_marker) ? ((oi1).the_long < (oi2).the_long) : \
      ((ii_kind((oi2).ii) == IIK_FINITE) ? !(ii_is_negative((oi2).ii)) : \
       (ii_kind((oi2).ii) == IIK_POSITIVE_INFINITY))) : \
     (((oi2).ii == oi_long_marker) ? \
      ((ii_kind((oi1).ii) == IIK_FINITE) ? ii_is_negative((oi1).ii) : \
       (ii_kind((oi1).ii) == IIK_NEGATIVE_INFINITY)) : \
      ii_less_than((oi1).ii, (oi2).ii)))

#define oi_less_than_size_t(the_oi, the_size_t) \
    (((the_oi).ii == oi_long_marker) ? \
     (((the_oi).the_long < 0) ? TRUE : \
      (((~(size_t)0) >= LONG_MAX) ? \
       (((size_t)((the_oi).the_long)) < (the_size_t)) : \
       ((the_oi).the_long < ((long)(the_size_t))))) : \
     ii_less_than_size_t((the_oi).ii, the_size_t))

#define oi_is_negative(the_oi) \
    (((the_oi).ii == oi_long_marker) ? ((the_oi).the_long < 0) : \
                                       ii_is_negative((the_oi).ii))

#define oi_negate(result, the_oi) \
  { \
    if ((the_oi).ii == oi_long_marker) \
      { \
        if (((long)LONG_MAX) + ((long)LONG_MIN) == 0) \
          { \
            (result).the_long = -((the_oi).the_long); \
            (result).ii = oi_long_marker; \
          } \
        else if (((long)LONG_MAX) + ((long)LONG_MIN) > 0) \
          { \
            if ((the_oi).the_long <= \
                -((((long)LONG_MAX) + ((long)LONG_MIN) > 0) ? \
                  (long)LONG_MIN : 0)) \
              { \
                (result).the_long = -((the_oi).the_long); \
                (result).ii = oi_long_marker; \
              } \
            else \
              { \
                i_integer *base_ii; \
                i_integer *result_ii; \
 \
                base_ii = ii_create_from_long_int((the_oi).the_long); \
                if (base_ii == NULL) \
                  { \
                    (result).ii = oi_no_memory_marker; \
                  } \
                else \
                  { \
                    result_ii = ii_negate(base_ii); \
                    ii_remove_reference(base_ii); \
                    oi_create_from_ii_capture_reference(result, result_ii); \
                  } \
              } \
          } \
        else \
          { \
            if ((the_oi).the_long >= \
                -((((long)LONG_MAX) + ((long)LONG_MIN) > 0) ? 0 : \
                  (long)LONG_MAX)) \
              { \
                (result).the_long = -((the_oi).the_long); \
                (result).ii = oi_long_marker; \
              } \
            else \
              { \
                i_integer *base_ii; \
                i_integer *result_ii; \
 \
                base_ii = ii_create_from_long_int((the_oi).the_long); \
                if (base_ii == NULL) \
                  { \
                    (result).ii = oi_no_memory_marker; \
                  } \
                else \
                  { \
                    result_ii = ii_negate(base_ii); \
                    ii_remove_reference(base_ii); \
                    oi_create_from_ii_capture_reference(result, result_ii); \
                  } \
              } \
          } \
      } \
    else \
      { \
        i_integer *the_ii; \
 \
        the_ii = ii_negate((the_oi).ii); \
        oi_create_from_ii_capture_reference(result, the_ii); \
      } \
  }

#define oi_add(result, oi1, oi2) \
  { \
    if (((oi1).ii == oi_long_marker) && ((oi2).ii == oi_long_marker) && \
        (((oi2).the_long >= 0) ? \
         ((oi1).the_long <= (LONG_MAX - (oi2).the_long)) : \
         ((oi1).the_long >= (LONG_MIN - (oi2).the_long)))) \
      { \
        (result).the_long = (((oi1).the_long) + ((oi2).the_long)); \
        (result).ii = oi_long_marker; \
      } \
    else \
      { \
        i_integer *ii1; \
        i_integer *ii2; \
        i_integer *result_ii; \
 \
        if ((oi1).ii == oi_long_marker) \
            ii1 = ii_create_from_long_int((oi1).the_long); \
        else \
            ii1 = (oi1).ii; \
 \
        if ((oi2).ii == oi_long_marker) \
            ii2 = ii_create_from_long_int((oi2).the_long); \
        else \
            ii2 = (oi2).ii; \
 \
        if ((ii1 != NULL) && (ii2 != NULL)) \
            result_ii = ii_add(ii1, ii2); \
        else \
            result_ii = NULL; \
        if (((oi1).ii == oi_long_marker) && (ii1 != NULL)) \
            ii_remove_reference(ii1); \
        if (((oi2).ii == oi_long_marker) && (ii2 != NULL)) \
            ii_remove_reference(ii2); \
        oi_create_from_ii_capture_reference(result, result_ii); \
      } \
  }

#define oi_subtract(result, oi1, oi2) \
  { \
    if (((oi1).ii == oi_long_marker) && ((oi2).ii == oi_long_marker) && \
        (((oi2).the_long >= 0) ? \
         ((oi1).the_long >= (LONG_MIN + (oi2).the_long)) : \
         ((oi1).the_long <= (LONG_MAX + (oi2).the_long)))) \
      { \
        (result).the_long = (((oi1).the_long) - ((oi2).the_long)); \
        (result).ii = oi_long_marker; \
      } \
    else \
      { \
        i_integer *ii1; \
        i_integer *ii2; \
        i_integer *result_ii; \
 \
        if ((oi1).ii == oi_long_marker) \
            ii1 = ii_create_from_long_int((oi1).the_long); \
        else \
            ii1 = (oi1).ii; \
 \
        if ((oi2).ii == oi_long_marker) \
            ii2 = ii_create_from_long_int((oi2).the_long); \
        else \
            ii2 = (oi2).ii; \
 \
        if ((ii1 != NULL) && (ii2 != NULL)) \
            result_ii = ii_subtract(ii1, ii2); \
        else \
            result_ii = NULL; \
        if (((oi1).ii == oi_long_marker) && (ii1 != NULL)) \
            ii_remove_reference(ii1); \
        if (((oi2).ii == oi_long_marker) && (ii2 != NULL)) \
            ii_remove_reference(ii2); \
        oi_create_from_ii_capture_reference(result, result_ii); \
      } \
  }

#define oi_multiply(result, oi1, oi2) \
  { \
    boolean do_extended_arithmetic; \
 \
    (result).ii = oi_long_marker; \
 \
    if (((oi1).ii == oi_long_marker) && ((oi2).ii == oi_long_marker)) \
      { \
        long long1; \
        long long2; \
 \
        long1 = (oi1).the_long; \
        long2 = (oi2).the_long; \
 \
        if (((long1 == 0) || (long2 == 0))) \
          { \
            (result).the_long = 0; \
            do_extended_arithmetic = FALSE; \
          } \
        else \
          { \
            if (long1 >= 0) \
              { \
                if (long2 >= 0) \
                  { \
                    do_extended_arithmetic = (long1 > (LONG_MAX / long2)); \
                  } \
                else \
                  { \
                    do_extended_arithmetic = \
                            (long2 < negative_div_ceiling(LONG_MIN, long1)); \
                  } \
              } \
            else \
              { \
                if (long2 >= 0) \
                  { \
                    do_extended_arithmetic = \
                            (long1 < negative_div_ceiling(LONG_MIN, long2)); \
                  } \
                else \
                  { \
                    do_extended_arithmetic = \
                            (long1 < negative_div_floor(LONG_MAX, long2)); \
                  } \
              } \
            if (!do_extended_arithmetic) \
              { \
                (result).the_long = (long1 * long2); \
              } \
          } \
      } \
    else \
      { \
        do_extended_arithmetic = TRUE; \
      } \
 \
    if (do_extended_arithmetic) \
      { \
        i_integer *ii1; \
        i_integer *ii2; \
        i_integer *result_ii; \
 \
        if ((oi1).ii == oi_long_marker) \
            ii1 = ii_create_from_long_int((oi1).the_long); \
        else \
            ii1 = (oi1).ii; \
 \
        if ((oi2).ii == oi_long_marker) \
            ii2 = ii_create_from_long_int((oi2).the_long); \
        else \
            ii2 = (oi2).ii; \
 \
        if ((ii1 != NULL) && (ii2 != NULL)) \
            result_ii = ii_multiply(ii1, ii2); \
        else \
            result_ii = NULL; \
        if (((oi1).ii == oi_long_marker) && (ii1 != NULL)) \
            ii_remove_reference(ii1); \
        if (((oi2).ii == oi_long_marker) && (ii2 != NULL)) \
            ii_remove_reference(ii2); \
        oi_create_from_ii_capture_reference(result, result_ii); \
      } \
  }

#define oi_divide(result, oi1, oi2, remainder) \
  { \
    assert((remainder) != NULL); \
    if (((oi1).ii == oi_long_marker) && ((oi2).ii == oi_long_marker)) \
      { \
        if ((oi2).the_long == 0) \
          { \
            (remainder)->ii = oi_zero_zero_marker; \
            ii_add_reference(oi_zero_zero_marker); \
            if ((oi1).the_long == 0) \
              { \
                (result).ii = oi_zero_zero_marker; \
                ii_add_reference(oi_zero_zero_marker); \
              } \
            else \
              { \
                (result).ii = oi_unsigned_infinity_marker; \
                ii_add_reference(oi_unsigned_infinity_marker); \
              } \
          } \
        else \
          { \
            (result).the_long = (((oi1).the_long) / ((oi2).the_long)); \
            (result).ii = oi_long_marker; \
            (remainder)->the_long = (((oi1).the_long) % ((oi2).the_long)); \
            (remainder)->ii = oi_long_marker; \
            if (((result).the_long < 0) && ((remainder)->the_long > 0)) \
              { \
                if ((oi1).the_long > 0) \
                  { \
                    (remainder)->the_long -= (oi2).the_long; \
                    ++(result).the_long; \
                  } \
                else \
                  { \
                    (remainder)->the_long += (oi2).the_long; \
                    --(result).the_long; \
                  } \
              } \
          } \
      } \
    else \
      { \
        i_integer *ii1; \
        i_integer *ii2; \
        i_integer *result_ii; \
        i_integer *remainder_ii; \
 \
        if ((oi1).ii == oi_long_marker) \
            ii1 = ii_create_from_long_int((oi1).the_long); \
        else \
            ii1 = (oi1).ii; \
 \
        if ((oi2).ii == oi_long_marker) \
            ii2 = ii_create_from_long_int((oi2).the_long); \
        else \
            ii2 = (oi2).ii; \
 \
        if ((ii1 != NULL) && (ii2 != NULL)) \
          { \
            result_ii = ii_divide(ii1, ii2, &remainder_ii); \
          } \
        else \
          { \
            result_ii = NULL; \
            remainder_ii = NULL; \
          } \
        if (((oi1).ii == oi_long_marker) && (ii1 != NULL)) \
            ii_remove_reference(ii1); \
        if (((oi2).ii == oi_long_marker) && (ii2 != NULL)) \
            ii_remove_reference(ii2); \
        oi_create_from_ii_capture_reference(result, result_ii); \
        oi_create_from_ii_capture_reference(*remainder, remainder_ii); \
      } \
  }

#define oi_shift_left(result, oi1, oi2) \
  { \
    boolean do_extended_arithmetic; \
 \
    (result).ii = oi_long_marker; \
 \
    if ((oi2).ii == oi_long_marker) \
      { \
        long long2; \
 \
        long2 = (oi2).the_long; \
 \
        if (long2 == 0) \
          { \
            if ((oi1).ii == oi_long_marker) \
              { \
                (result).the_long = (oi1).the_long; \
              } \
            else \
              { \
                (result).ii = (oi1).ii; \
                ii_add_reference((oi1).ii); \
              } \
            do_extended_arithmetic = FALSE; \
          } \
        else if ((oi1).ii == oi_long_marker) \
          { \
            long long1; \
 \
            long1 = (oi1).the_long; \
 \
            if (long1 == 0) \
              { \
                (result).the_long = 0; \
                do_extended_arithmetic = FALSE; \
              } \
            else if (long2 > 0) \
              { \
                if (long2 >= sizeof(long) * 8) \
                  { \
                    do_extended_arithmetic = TRUE; \
                  } \
                else \
                  { \
                    if (long1 > 0) \
                      { \
                        if (long1 <= (LONG_MAX >> long2)) \
                          { \
                            (result).the_long = long1 << long2; \
                            do_extended_arithmetic = FALSE; \
                          } \
                        else \
                          { \
                            do_extended_arithmetic = TRUE; \
                          } \
                      } \
                    else \
                      { \
                        if ((LONG_MIN + LONG_MAX <= 0) ? \
                            (long1 >= \
                             -((LONG_MIN + LONG_MAX <= 0) ? LONG_MAX : 0)) : \
                            TRUE) \
                          { \
                            long1 = -long1; \
                            if (long1 <= (LONG_MAX >> long2)) \
                              { \
                                long negated_result; \
 \
                                negated_result = long1 << long2; \
                                if ((LONG_MIN + LONG_MAX <= 0) ? \
                                    (negated_result >= \
                                     -((LONG_MIN + LONG_MAX <= 0) ? \
                                       LONG_MAX : 0)) : TRUE) \
                                  { \
                                    (result).the_long = -negated_result; \
                                    do_extended_arithmetic = FALSE; \
                                  } \
                                else \
                                  { \
                                    do_extended_arithmetic = TRUE; \
                                  } \
                              } \
                            else \
                              { \
                                do_extended_arithmetic = TRUE; \
                              } \
                          } \
                        else \
                          { \
                            do_extended_arithmetic = TRUE; \
                          } \
                      } \
                  } \
              } \
            else \
              { \
                if (long2 > -sizeof(long) * 8) \
                  { \
                    if (long1 > 0) \
                      { \
                        (result).the_long = long1 >> -long2; \
                        do_extended_arithmetic = FALSE; \
                      } \
                    else \
                      { \
                        if (negative_long_is_invertable(long1)) \
                          { \
                            (result).the_long = invert_long( \
                                    invert_long(long1) >> -long2); \
                            do_extended_arithmetic = FALSE; \
                          } \
                        else \
                          { \
                            do_extended_arithmetic = TRUE; \
                          } \
                      } \
                  } \
                else if (long1 > 0) \
                  { \
                    (result).the_long = 0; \
                    do_extended_arithmetic = FALSE; \
                  } \
                else \
                  { \
                    (result).the_long = -1; \
                    do_extended_arithmetic = FALSE; \
                  } \
              } \
          } \
        else \
          { \
            do_extended_arithmetic = TRUE; \
          } \
      } \
    else \
      { \
        if (((oi1).ii == oi_long_marker) && ((oi1).the_long == 0)) \
          { \
            switch (ii_kind((oi2).ii)) \
              { \
                case IIK_FINITE: \
                    (result).the_long = 0; \
                    break; \
                case IIK_POSITIVE_INFINITY: \
                    (result).ii = oi_zero_zero_marker; \
                    ii_add_reference(oi_zero_zero_marker); \
                    break; \
                case IIK_NEGATIVE_INFINITY: \
                    (result).the_long = 0; \
                    break; \
                case IIK_UNSIGNED_INFINITY: \
                    (result).ii = oi_zero_zero_marker; \
                    ii_add_reference(oi_zero_zero_marker); \
                    break; \
                case IIK_ZERO_ZERO: \
                    (result).ii = oi_zero_zero_marker; \
                    ii_add_reference(oi_zero_zero_marker); \
                    break; \
                default: \
                    assert(FALSE); \
              } \
            do_extended_arithmetic = FALSE; \
          } \
        else \
          { \
            do_extended_arithmetic = TRUE; \
          } \
      } \
 \
    if (do_extended_arithmetic) \
      { \
        i_integer *ii1; \
        i_integer *ii2; \
        i_integer *result_ii; \
 \
        if ((oi1).ii == oi_long_marker) \
            ii1 = ii_create_from_long_int((oi1).the_long); \
        else \
            ii1 = (oi1).ii; \
 \
        if ((oi2).ii == oi_long_marker) \
            ii2 = ii_create_from_long_int((oi2).the_long); \
        else \
            ii2 = (oi2).ii; \
 \
        if ((ii1 != NULL) && (ii2 != NULL)) \
            result_ii = ii_shift_left(ii1, ii2); \
        else \
            result_ii = NULL; \
        if (((oi1).ii == oi_long_marker) && (ii1 != NULL)) \
            ii_remove_reference(ii1); \
        if (((oi2).ii == oi_long_marker) && (ii2 != NULL)) \
            ii_remove_reference(ii2); \
        oi_create_from_ii_capture_reference(result, result_ii); \
      } \
  }

#define oi_shift_right(result, oi1, oi2) \
  { \
    boolean do_extended_arithmetic; \
 \
    (result).ii = oi_long_marker; \
 \
    if ((oi2).ii == oi_long_marker) \
      { \
        long long2; \
 \
        long2 = (oi2).the_long; \
 \
        if (long2 == 0) \
          { \
            if ((oi1).ii == oi_long_marker) \
              { \
                (result).the_long = (oi1).the_long; \
              } \
            else \
              { \
                (result).ii = (oi1).ii; \
                ii_add_reference((oi1).ii); \
              } \
            do_extended_arithmetic = FALSE; \
          } \
        else if ((oi1).ii == oi_long_marker) \
          { \
            long long1; \
 \
            long1 = (oi1).the_long; \
 \
            if (long1 == 0) \
              { \
                (result).the_long = 0; \
                do_extended_arithmetic = FALSE; \
              } \
            else if (long2 < 0) \
              { \
                if (long2 <= -sizeof(long) * 8) \
                  { \
                    do_extended_arithmetic = TRUE; \
                  } \
                else \
                  { \
                    if (long1 > 0) \
                      { \
                        if (long1 <= (LONG_MAX >> -long2)) \
                          { \
                            (result).the_long = long1 << -long2; \
                            do_extended_arithmetic = FALSE; \
                          } \
                        else \
                          { \
                            do_extended_arithmetic = TRUE; \
                          } \
                      } \
                    else \
                      { \
                        if ((LONG_MIN + LONG_MAX <= 0) ? \
                            (long1 >= \
                             -((LONG_MIN + LONG_MAX <= 0) ? LONG_MAX : 0)) : \
                            TRUE) \
                          { \
                            long1 = -long1; \
                            if (long1 <= (LONG_MAX >> -long2)) \
                              { \
                                long negated_result; \
 \
                                negated_result = long1 << -long2; \
                                if ((LONG_MIN + LONG_MAX <= 0) ? \
                                    (negated_result >= \
                                     -((LONG_MIN + LONG_MAX <= 0) ? \
                                       LONG_MAX : 0)) : TRUE) \
                                  { \
                                    (result).the_long = -negated_result; \
                                    do_extended_arithmetic = FALSE; \
                                  } \
                                else \
                                  { \
                                    do_extended_arithmetic = TRUE; \
                                  } \
                              } \
                            else \
                              { \
                                do_extended_arithmetic = TRUE; \
                              } \
                          } \
                        else \
                          { \
                            do_extended_arithmetic = TRUE; \
                          } \
                      } \
                  } \
              } \
            else \
              { \
                if (long2 < sizeof(long) * 8) \
                  { \
                    if (long1 > 0) \
                      { \
                        (result).the_long = long1 >> long2; \
                        do_extended_arithmetic = FALSE; \
                      } \
                    else \
                      { \
                        if (negative_long_is_invertable(long1)) \
                          { \
                            (result).the_long = \
                                    invert_long(invert_long(long1) >> long2); \
                            do_extended_arithmetic = FALSE; \
                          } \
                        else \
                          { \
                            do_extended_arithmetic = TRUE; \
                          } \
                      } \
                  } \
                else if (long1 > 0) \
                  { \
                    (result).the_long = 0; \
                    do_extended_arithmetic = FALSE; \
                  } \
                else \
                  { \
                    (result).the_long = -1; \
                    do_extended_arithmetic = FALSE; \
                  } \
              } \
          } \
        else \
          { \
            do_extended_arithmetic = TRUE; \
          } \
      } \
    else \
      { \
        if (((oi1).ii == oi_long_marker) && ((oi1).the_long == 0)) \
          { \
            switch (ii_kind((oi2).ii)) \
              { \
                case IIK_FINITE: \
                    (result).the_long = 0; \
                    break; \
                case IIK_POSITIVE_INFINITY: \
                    (result).the_long = 0; \
                    break; \
                case IIK_NEGATIVE_INFINITY: \
                    (result).ii = oi_zero_zero_marker; \
                    ii_add_reference(oi_zero_zero_marker); \
                    break; \
                case IIK_UNSIGNED_INFINITY: \
                    (result).ii = oi_zero_zero_marker; \
                    ii_add_reference(oi_zero_zero_marker); \
                    break; \
                case IIK_ZERO_ZERO: \
                    (result).ii = oi_zero_zero_marker; \
                    ii_add_reference(oi_zero_zero_marker); \
                    break; \
                default: \
                    assert(FALSE); \
              } \
            do_extended_arithmetic = FALSE; \
          } \
        else \
          { \
            do_extended_arithmetic = TRUE; \
          } \
      } \
 \
    if (do_extended_arithmetic) \
      { \
        i_integer *ii1; \
        i_integer *ii2; \
        i_integer *result_ii; \
 \
        if ((oi1).ii == oi_long_marker) \
            ii1 = ii_create_from_long_int((oi1).the_long); \
        else \
            ii1 = (oi1).ii; \
 \
        if ((oi2).ii == oi_long_marker) \
            ii2 = ii_create_from_long_int((oi2).the_long); \
        else \
            ii2 = (oi2).ii; \
 \
        if ((ii1 != NULL) && (ii2 != NULL)) \
            result_ii = ii_shift_right(ii1, ii2); \
        else \
            result_ii = NULL; \
        if (((oi1).ii == oi_long_marker) && (ii1 != NULL)) \
            ii_remove_reference(ii1); \
        if (((oi2).ii == oi_long_marker) && (ii2 != NULL)) \
            ii_remove_reference(ii2); \
        oi_create_from_ii_capture_reference(result, result_ii); \
      } \
  }

#define oi_bitwise_and(result, oi1, oi2) \
  { \
    boolean do_extended_arithmetic; \
 \
    (result).ii = oi_long_marker; \
 \
    if (((oi1).ii == oi_long_marker) && ((oi2).ii == oi_long_marker)) \
      { \
        long long1; \
        long long2; \
 \
        long1 = (oi1).the_long; \
        long2 = (oi2).the_long; \
 \
        if (long1 >= 0) \
          { \
            if (long2 >= 0) \
              { \
                (result).the_long = (long1 & long2); \
                do_extended_arithmetic = FALSE; \
              } \
            else \
              { \
                if (negative_long_is_invertable(long2)) \
                  { \
                    (result).the_long = \
                            ((long1 ^ invert_long(long2)) & long1); \
                    do_extended_arithmetic = FALSE; \
                 } \
                else \
                  { \
                    do_extended_arithmetic = TRUE; \
                  } \
              } \
          } \
        else \
          { \
            if (long2 >= 0) \
              { \
                if (negative_long_is_invertable(long1)) \
                  { \
                    (result).the_long = \
                            ((long2 ^ invert_long(long1)) & long2); \
                    do_extended_arithmetic = FALSE; \
                 } \
                else \
                  { \
                    do_extended_arithmetic = TRUE; \
                  } \
              } \
            else \
              { \
                if (negative_long_is_invertable(long1) && \
                    negative_long_is_invertable(long2)) \
                  { \
                    long inverted_result; \
 \
                    inverted_result = \
                            invert_long(long1) | invert_long(long2); \
                    if (positive_long_is_invertable(inverted_result)) \
                      { \
                        (result).the_long = invert_long(inverted_result); \
                        do_extended_arithmetic = FALSE; \
                      } \
                    else \
                      { \
                        do_extended_arithmetic = TRUE; \
                      } \
                  } \
                else \
                  { \
                    do_extended_arithmetic = TRUE; \
                  } \
              } \
          } \
      } \
    else \
      { \
        do_extended_arithmetic = TRUE; \
      } \
 \
    if (do_extended_arithmetic) \
      { \
        i_integer *ii1; \
        i_integer *ii2; \
        i_integer *result_ii; \
 \
        if ((oi1).ii == oi_long_marker) \
            ii1 = ii_create_from_long_int((oi1).the_long); \
        else \
            ii1 = (oi1).ii; \
 \
        if ((oi2).ii == oi_long_marker) \
            ii2 = ii_create_from_long_int((oi2).the_long); \
        else \
            ii2 = (oi2).ii; \
 \
        if ((ii1 != NULL) && (ii2 != NULL)) \
            result_ii = ii_bitwise_and(ii1, ii2); \
        else \
            result_ii = NULL; \
        if (((oi1).ii == oi_long_marker) && (ii1 != NULL)) \
            ii_remove_reference(ii1); \
        if (((oi2).ii == oi_long_marker) && (ii2 != NULL)) \
            ii_remove_reference(ii2); \
        oi_create_from_ii_capture_reference(result, result_ii); \
      } \
  }

#define oi_bitwise_or(result, oi1, oi2) \
  { \
    boolean do_extended_arithmetic; \
 \
    (result).ii = oi_long_marker; \
 \
    if (((oi1).ii == oi_long_marker) && ((oi2).ii == oi_long_marker)) \
      { \
        long long1; \
        long long2; \
 \
        long1 = (oi1).the_long; \
        long2 = (oi2).the_long; \
 \
        if (long1 >= 0) \
          { \
            if (long2 >= 0) \
              { \
                (result).the_long = (long1 | long2); \
                do_extended_arithmetic = FALSE; \
              } \
            else \
              { \
                if (negative_long_is_invertable(long2)) \
                  { \
                    (result).the_long = \
                            (~((long1 ^ invert_long(long2))) | long1); \
                    do_extended_arithmetic = FALSE; \
                 } \
                else \
                  { \
                    do_extended_arithmetic = TRUE; \
                  } \
              } \
          } \
        else \
          { \
            if (long2 >= 0) \
              { \
                if (negative_long_is_invertable(long1)) \
                  { \
                    (result).the_long = \
                            (~((long2 ^ invert_long(long1))) | long2); \
                    do_extended_arithmetic = FALSE; \
                 } \
                else \
                  { \
                    do_extended_arithmetic = TRUE; \
                  } \
              } \
            else \
              { \
                if (negative_long_is_invertable(long1) && \
                    negative_long_is_invertable(long2)) \
                  { \
                    long inverted_result; \
 \
                    inverted_result = \
                            invert_long(long1) & invert_long(long2); \
                    assert(positive_long_is_invertable(inverted_result)); \
                    (result).the_long = invert_long(inverted_result); \
                    do_extended_arithmetic = FALSE; \
                  } \
                else \
                  { \
                    do_extended_arithmetic = TRUE; \
                  } \
              } \
          } \
      } \
    else \
      { \
        do_extended_arithmetic = TRUE; \
      } \
 \
    if (do_extended_arithmetic) \
      { \
        i_integer *ii1; \
        i_integer *ii2; \
        i_integer *result_ii; \
 \
        if ((oi1).ii == oi_long_marker) \
            ii1 = ii_create_from_long_int((oi1).the_long); \
        else \
            ii1 = (oi1).ii; \
 \
        if ((oi2).ii == oi_long_marker) \
            ii2 = ii_create_from_long_int((oi2).the_long); \
        else \
            ii2 = (oi2).ii; \
 \
        if ((ii1 != NULL) && (ii2 != NULL)) \
            result_ii = ii_bitwise_or(ii1, ii2); \
        else \
            result_ii = NULL; \
        if (((oi1).ii == oi_long_marker) && (ii1 != NULL)) \
            ii_remove_reference(ii1); \
        if (((oi2).ii == oi_long_marker) && (ii2 != NULL)) \
            ii_remove_reference(ii2); \
        oi_create_from_ii_capture_reference(result, result_ii); \
      } \
  }

#define oi_bitwise_xor(result, oi1, oi2) \
  { \
    boolean do_extended_arithmetic; \
 \
    (result).ii = oi_long_marker; \
 \
    if (((oi1).ii == oi_long_marker) && ((oi2).ii == oi_long_marker)) \
      { \
        long long1; \
        long long2; \
 \
        long1 = (oi1).the_long; \
        long2 = (oi2).the_long; \
 \
        if (long1 >= 0) \
          { \
            if (long2 >= 0) \
              { \
                (result).the_long = (long1 ^ long2); \
                do_extended_arithmetic = FALSE; \
              } \
            else \
              { \
                if (negative_long_is_invertable(long2)) \
                  { \
                    long inverted2; \
 \
                    inverted2 = invert_long(long2); \
                    (result).the_long = \
                            ((long1 & inverted2) | ~(long1 | inverted2)); \
                    do_extended_arithmetic = FALSE; \
                 } \
                else \
                  { \
                    do_extended_arithmetic = TRUE; \
                  } \
              } \
          } \
        else \
          { \
            if (long2 >= 0) \
              { \
                if (negative_long_is_invertable(long1)) \
                  { \
                    long inverted1; \
 \
                    inverted1 = invert_long(long1); \
                    (result).the_long = \
                            ((long2 & inverted1) | ~(long2 | inverted1)); \
                    do_extended_arithmetic = FALSE; \
                 } \
                else \
                  { \
                    do_extended_arithmetic = TRUE; \
                  } \
              } \
            else \
              { \
                if (negative_long_is_invertable(long1) && \
                    negative_long_is_invertable(long2)) \
                  { \
                    (result).the_long = \
                            invert_long(long1) ^ invert_long(long2); \
                    do_extended_arithmetic = FALSE; \
                  } \
                else \
                  { \
                    do_extended_arithmetic = TRUE; \
                  } \
              } \
          } \
      } \
    else \
      { \
        do_extended_arithmetic = TRUE; \
      } \
 \
    if (do_extended_arithmetic) \
      { \
        i_integer *ii1; \
        i_integer *ii2; \
        i_integer *result_ii; \
 \
        if ((oi1).ii == oi_long_marker) \
            ii1 = ii_create_from_long_int((oi1).the_long); \
        else \
            ii1 = (oi1).ii; \
 \
        if ((oi2).ii == oi_long_marker) \
            ii2 = ii_create_from_long_int((oi2).the_long); \
        else \
            ii2 = (oi2).ii; \
 \
        if ((ii1 != NULL) && (ii2 != NULL)) \
            result_ii = ii_bitwise_xor(ii1, ii2); \
        else \
            result_ii = NULL; \
        if (((oi1).ii == oi_long_marker) && (ii1 != NULL)) \
            ii_remove_reference(ii1); \
        if (((oi2).ii == oi_long_marker) && (ii2 != NULL)) \
            ii_remove_reference(ii2); \
        oi_create_from_ii_capture_reference(result, result_ii); \
      } \
  }

#define oi_power_of_ten(result, exponent) \
  { \
    boolean do_extended_arithmetic; \
 \
    (result).ii = oi_long_marker; \
 \
    if ((exponent).ii == oi_long_marker) \
      { \
        long remainder; \
        long power; \
        long long_result; \
\
        remainder = (exponent).the_long; \
        assert(remainder >= 0); \
        power = 10; \
        long_result = 1; \
        while (TRUE) \
          { \
            assert(remainder >= 0); \
            if (remainder == 0) \
              { \
                (result).the_long = long_result; \
                do_extended_arithmetic = FALSE; \
                break; \
              } \
 \
            if (remainder & 0x1) \
              { \
                if (long_result > (LONG_MAX / power)) \
                  { \
                    do_extended_arithmetic = TRUE; \
                    break; \
                  } \
                long_result *= power; \
              } \
 \
            remainder >>= 1; \
 \
            if (remainder != 0) \
              { \
                if (power > (LONG_MAX / power)) \
                  { \
                    do_extended_arithmetic = TRUE; \
                    break; \
                  } \
                power *= power; \
              } \
          } \
      } \
    else \
      { \
        do_extended_arithmetic = TRUE; \
      } \
 \
    if (do_extended_arithmetic) \
      { \
        i_integer *exponent_ii; \
        i_integer *result_ii; \
 \
        if ((exponent).ii == oi_long_marker) \
            exponent_ii = ii_create_from_long_int((exponent).the_long); \
        else \
            exponent_ii = (exponent).ii; \
 \
        if (exponent_ii != NULL) \
            result_ii = ii_power_of_ten(exponent_ii); \
        else \
            result_ii = NULL; \
        if (((exponent).ii == oi_long_marker) && (exponent_ii != NULL)) \
            ii_remove_reference(exponent_ii); \
        oi_create_from_ii_capture_reference(result, result_ii); \
      } \
  }

#define oi_decimal_digit_count(the_oi, digit_count) \
    (((the_oi).ii == oi_long_marker) ? \
     long_decimal_digit_count((the_oi).the_long, digit_count) : \
     ii_decimal_digit_count((the_oi).ii, digit_count))

#define oi_write_decimal_digits(the_oi, location) \
    (((the_oi).ii == oi_long_marker) ? \
     long_write_decimal_digits((the_oi).the_long, location) : \
     ii_write_decimal_digits((the_oi).ii, location))

#define oi_hex_digits(the_oi, digit_count) \
    (((the_oi).ii == oi_long_marker) ? \
     long_hex_digits((the_oi).the_long, digit_count) : \
     ii_hex_digits((the_oi).ii, digit_count))

#define oi_write_hex_digits(the_oi, location, uppercase) \
    (((the_oi).ii == oi_long_marker) ? \
     long_write_hex_digits((the_oi).the_long, location, uppercase) : \
     ii_write_hex_digits((the_oi).ii, location, uppercase))

#define oi_magnitude_to_size_t(the_oi, result) \
    (((the_oi).ii == oi_long_marker) ? \
     ((((the_oi).the_long >= 0) ? \
       (((~(size_t)0) >= LONG_MAX) ? TRUE : \
        ((the_oi).the_long <= (~(size_t)0))) : \
       (((~(size_t)0) + LONG_MIN <= 0) ? TRUE : \
        (-((the_oi).the_long) <= (~(size_t)0)))) ? \
      (*result = (((the_oi).the_long >= 0) ? (size_t)((the_oi).the_long) : \
                  (size_t)-((the_oi).the_long)), MISSION_ACCOMPLISHED) : \
      MISSION_FAILED) : ii_magnitude_to_size_t((the_oi).ii, result))

#define oi_to_long_int(the_oi, result) \
    (((the_oi).ii == oi_long_marker) ? \
     (*result = (the_oi).the_long, MISSION_ACCOMPLISHED) : \
     ii_to_long_int((the_oi).ii, result))

#define oi_to_unsigned_long_int(the_oi, result) \
    (((the_oi).ii == oi_long_marker) ? \
     (*result = (unsigned long)((the_oi).the_long), MISSION_ACCOMPLISHED) : \
     ii_to_unsigned_long_int((the_oi).ii, result))

#define oi_structural_order(left, right) \
    (((left).ii == oi_long_marker) ? \
     (((right).ii == oi_long_marker) ? \
      (((left).the_long < (right).the_long) ? -1 : \
       (((left).the_long == (right).the_long) ? 0 : 1)) : \
      ((ii_kind((right).ii) == IIK_FINITE) ? \
       ((ii_is_negative((right).ii)) ? 1 : -1) : \
       ((ii_kind((right).ii) == IIK_NEGATIVE_INFINITY) ? 1 : -1))) : \
     (((right).ii == oi_long_marker) ? \
      ((ii_kind((left).ii) == IIK_FINITE) ? \
       ((ii_is_negative((left).ii)) ? -1 : 1) : \
       ((ii_kind((left).ii) == IIK_NEGATIVE_INFINITY) ? -1 : 1)) : \
      ii_structural_order((left).ii, (right).ii)))


#endif /* O_INTEGER_H */
