/* file "i_integer.h" */

/*
 *  This file contains the interface to the i_integer module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef I_INTEGER_H
#define I_INTEGER_H

#include <stddef.h>
#include "c_foundations/basic.h"


typedef enum i_integer_kind
  {
    IIK_FINITE,
    IIK_POSITIVE_INFINITY,
    IIK_NEGATIVE_INFINITY,
    IIK_UNSIGNED_INFINITY,
    IIK_ZERO_ZERO
  } i_integer_kind;

typedef struct i_integer i_integer;


extern i_integer *ii_create_from_decimal_ascii(size_t digit_count,
        const char *digits, boolean is_negative);
extern i_integer *ii_create_from_hex_ascii(size_t digit_count,
        const char *digits, boolean is_negative);
extern i_integer *ii_create_from_size_t(size_t the_size_t);
extern i_integer *ii_create_from_long_int(long int the_long_int);
extern i_integer *ii_create_from_unsigned_long_int(
        unsigned long int the_unsigned_long_int);
extern i_integer *ii_positive_infinity(void);
extern i_integer *ii_positive_infinity_no_ref(void);
extern i_integer *ii_negative_infinity(void);
extern i_integer *ii_negative_infinity_no_ref(void);
extern i_integer *ii_unsigned_infinity(void);
extern i_integer *ii_unsigned_infinity_no_ref(void);
extern i_integer *ii_zero_zero(void);
extern i_integer *ii_zero_zero_no_ref(void);
extern i_integer *ii_zero(void);
extern i_integer *ii_zero_no_ref(void);
extern i_integer *ii_one(void);
extern i_integer *ii_one_no_ref(void);

extern void ii_add_reference(i_integer *ii);
extern void ii_remove_reference(i_integer *ii);

extern i_integer_kind ii_kind(i_integer *ii);
extern boolean ii_equal(i_integer *ii1, i_integer *ii2);
extern boolean ii_less_than(i_integer *ii1, i_integer *ii2);
extern boolean ii_less_than_size_t(i_integer *ii, size_t the_size_t);
extern boolean ii_is_negative(i_integer *ii);

extern i_integer *ii_negate(i_integer *ii);
extern i_integer *ii_add(i_integer *ii1, i_integer *ii2);
extern i_integer *ii_subtract(i_integer *ii1, i_integer *ii2);
extern i_integer *ii_multiply(i_integer *ii1, i_integer *ii2);
extern i_integer *ii_divide(i_integer *ii1, i_integer *ii2,
                            i_integer **remainder);
extern i_integer *ii_shift_left(i_integer *ii1, i_integer *ii2);
extern i_integer *ii_shift_right(i_integer *ii1, i_integer *ii2);
extern i_integer *ii_bitwise_and(i_integer *ii1, i_integer *ii2);
extern i_integer *ii_bitwise_or(i_integer *ii1, i_integer *ii2);
extern i_integer *ii_bitwise_xor(i_integer *ii1, i_integer *ii2);
extern i_integer *ii_power_of_ten(i_integer *exponent);

extern verdict ii_decimal_digit_count(i_integer *ii, size_t *digit_count);
extern verdict ii_write_decimal_digits(i_integer *ii, char *location);
extern verdict ii_hex_digits(i_integer *ii, size_t *digit_count);
extern verdict ii_write_hex_digits(i_integer *ii, char *location,
                                   boolean uppercase);

extern verdict ii_magnitude_to_size_t(i_integer *ii, size_t *result);
extern verdict ii_to_long_int(i_integer *ii, long int *result);
extern verdict ii_to_unsigned_long_int(i_integer *ii,
                                       unsigned long int *result);

extern int ii_structural_order(i_integer *left, i_integer *right);


#endif /* I_INTEGER_H */
