/* file "i_integer.c" */

/*
 *  This file contains the implementation of the i_integer module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/diagnostic.h"
#include "i_integer.h"
#include "platform_dependent.h"


struct i_integer
  {
    i_integer_kind kind;
    boolean is_negative;
    DECLARE_SYSTEM_LOCK(decimal_digit_lock);
    size_t decimal_digit_count;
    char *decimal_digits;
    DECLARE_SYSTEM_LOCK(bit_block_lock);
    size_t bit_block_count;
    unsigned long *bit_blocks;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
  };


/*
 *  The following is a tuning parameter.  Its optimal value may be different on
 *  different machines.  The value used here was chosen based on
 *  experimentation on a particular Linux x86 system using the
 *  "examples/factorial_block.salm" program.  On the test system, the
 *  performance curve was fairly flat for cut-off values from 10 to 50, so even
 *  if the value specified here isn't optimal for some systems, there's a good
 *  chance the resulting performance isn't that much different than it would be
 *  for the optimal value.
 */
#ifndef TOOM_2_DECIMAL_CUTOFF_DIGITS
#define TOOM_2_DECIMAL_CUTOFF_DIGITS 20
#endif /* TOOM_2_DECIMAL_CUTOFF_DIGITS */

#define DECIMAL_STORAGE_ZERO_CHAR 0


static boolean statics_initialized = FALSE;
static i_integer static_positive_infinity;
static i_integer static_negative_infinity;
static i_integer static_unsigned_infinity;
static i_integer static_zero_zero;
static i_integer static_zero;
static i_integer static_one;
static i_integer static_minus_one;


static void initialize(void);
static void initialize_one(i_integer *to_initialize, i_integer_kind kind);
static void initialize_one_base(i_integer *to_initialize, i_integer_kind kind);
static i_integer *create_empty_ii(i_integer_kind kind);
static verdict make_compatible(i_integer *ii1, i_integer *ii2);
static char *force_decimal_digits(i_integer *ii);
static unsigned long *force_bit_blocks(i_integer *ii);
static boolean is_zero(i_integer *ii);
static i_integer *ii_minus_one(void);
static i_integer *shift_up(i_integer *ii, size_t magnitude);
static i_integer *shift_down(i_integer *ii, size_t magnitude);
static size_t ii_to_size_t(i_integer *ii);
static void add_decimal_magnitudes(char *digits1, char *digits2, size_t count1,
        size_t count2, char *result_digits, size_t *result_count);
static char *add_decimal_magnitudes_with_allocation(char *digits1,
        char *digits2, size_t count1, size_t count2, size_t *result_count);
static void subtract_decimal_magnitudes(char *digits1, char *digits2,
        size_t count1, size_t count2, char *result_digits,
        size_t *result_count);
static char *subtract_decimal_magnitudes_with_allocation(char *digits1,
        char *digits2, size_t count1, size_t count2, size_t *result_count);
static void zero_decimal_magnitude(char *digits, size_t count);
static boolean decimal_magnitude_is_less(char *digits1, char *digits2,
        size_t count1, size_t count2, boolean *equal);
static verdict multiply_decimal_magnitudes(char *digits1, char *digits2,
        size_t count1, size_t count2, char *result_digits);
static void multiply_decimal_magnitudes_simple(char *digits1, char *digits2,
        size_t count1, size_t count2, char *result_digits);
static verdict multiply_decimal_magnitudes_toom_2(char *digits1, char *digits2,
        size_t count1, size_t count2, char *result_digits);
static size_t safe_shift_right_size_t(size_t to_shift, size_t amount);
static size_t safe_shift_left_size_t(size_t to_shift, size_t amount);
static long safe_shift_right_long(long to_shift, size_t amount);
static long safe_shift_left_long(long to_shift, size_t amount);


extern i_integer *ii_create_from_decimal_ascii(size_t digit_count,
        const char *digits, boolean is_negative)
  {
    i_integer *result;
    char *copy;
    size_t digit_num;

    assert(digits != NULL);

    result = create_empty_ii(IIK_FINITE);
    if (result == NULL)
        return NULL;

    copy = MALLOC_ARRAY(char, ((digit_count == 0) ? 1 : digit_count));
    if (copy == NULL)
      {
        ii_remove_reference(result);
        return NULL;
      }

    for (digit_num = 0; digit_num < digit_count; ++digit_num)
      {
        copy[digit_num] =
                ((digits[digit_count - (digit_num + 1)] - '0') +
                 DECIMAL_STORAGE_ZERO_CHAR);
      }

    while ((digit_count > 0) &&
           (copy[digit_count - 1] == DECIMAL_STORAGE_ZERO_CHAR))
      {
        --digit_count;
      }

    result->is_negative = ((digit_count == 0) ? FALSE : is_negative);
    result->decimal_digit_count = digit_count;
    result->decimal_digits = copy;

    return result;
  }

extern i_integer *ii_create_from_hex_ascii(size_t digit_count,
        const char *digits, boolean is_negative)
  {
    i_integer *result;
    size_t digits_per_block;
    size_t block_count;
    unsigned long *blocks;
    size_t block_num;
    size_t digit_num;

    assert(digits != NULL);

    result = create_empty_ii(IIK_FINITE);
    if (result == NULL)
        return NULL;

    digits_per_block = (sizeof(unsigned long) * 2);
    block_count =
            ((digit_count == 0) ? 1 :
             ((digit_count + (digits_per_block - 1)) / digits_per_block));
    blocks = MALLOC_ARRAY(unsigned long, block_count);
    if (blocks == NULL)
      {
        ii_remove_reference(result);
        return NULL;
      }

    for (block_num = 0; block_num < block_count; ++block_num)
        blocks[block_num] = 0;

    for (digit_num = 0; digit_num < digit_count; ++digit_num)
      {
        size_t block_num;
        char digit;
        unsigned long bits;

        block_num = (digit_num / digits_per_block);
        assert(block_num < block_count);

        digit = digits[digit_count - (digit_num + 1)];

        if ((digit >= '0') && (digit <= '9'))
          {
            bits = digit - '0';
          }
        else if ((digit >= 'a') && (digit <= 'f'))
          {
            bits = (digit - 'a') + 0xa;
          }
        else if ((digit >= 'A') && (digit <= 'F'))
          {
            bits = (digit - 'a') + 0xa;
          }
        else
          {
            assert(FALSE);
            bits = 0;
          }

        blocks[block_num] |= (bits << (4 * (digit_num % digits_per_block)));
      }

    if (is_negative)
      {
        for (; digit_num < (block_count * digits_per_block); ++digit_num)
          {
            size_t block_num;

            block_num = (digit_num / digits_per_block);
            assert(block_num < block_count);

            blocks[block_num] |= (0xf << (4 * (digit_num % digits_per_block)));
          }
      }

    while ((block_count > 0) &&
           (blocks[block_count - 1] == (is_negative ? ~(unsigned long)0 : 0)))
      {
        --block_count;
      }

    result->is_negative = ((block_count == 0) ? FALSE : is_negative);
    result->bit_block_count = block_count;
    result->bit_blocks = blocks;

    return result;
  }

extern i_integer *ii_create_from_size_t(size_t the_size_t)
  {
    char buffer[(sizeof(unsigned long) * 3) + 2];

    sprintf(buffer, "%lu", (unsigned long)the_size_t);
    return ii_create_from_decimal_ascii(strlen(buffer), buffer, FALSE);
  }

extern i_integer *ii_create_from_long_int(long int the_long_int)
  {
    char buffer[(sizeof(long int) * 3) + 3];
    boolean is_negative;
    const char *digits;

    sprintf(buffer, "%ld", the_long_int);

    if (buffer[0] == '-')
      {
        digits = &(buffer[1]);
        is_negative = TRUE;
      }
    else
      {
        digits = buffer;
        is_negative = FALSE;
      }

    return ii_create_from_decimal_ascii(strlen(digits), digits, is_negative);
  }

extern i_integer *ii_create_from_unsigned_long_int(
        unsigned long int the_unsigned_long_int)
  {
    char buffer[(sizeof(unsigned long int) * 3) + 3];
    boolean is_negative;
    const char *digits;

    sprintf(buffer, "%lu", the_unsigned_long_int);

    if (buffer[0] == '-')
      {
        digits = &(buffer[1]);
        is_negative = TRUE;
      }
    else
      {
        digits = buffer;
        is_negative = FALSE;
      }

    return ii_create_from_decimal_ascii(strlen(digits), digits, is_negative);
  }

extern i_integer *ii_positive_infinity(void)
  {
    initialize();
    GRAB_SYSTEM_LOCK(static_positive_infinity.reference_lock);
    ++(static_positive_infinity.reference_count);
    RELEASE_SYSTEM_LOCK(static_positive_infinity.reference_lock);
    return &static_positive_infinity;
  }

extern i_integer *ii_positive_infinity_no_ref(void)
  {
    initialize();
    return &static_positive_infinity;
  }

extern i_integer *ii_negative_infinity(void)
  {
    initialize();
    GRAB_SYSTEM_LOCK(static_negative_infinity.reference_lock);
    ++(static_negative_infinity.reference_count);
    RELEASE_SYSTEM_LOCK(static_negative_infinity.reference_lock);
    return &static_negative_infinity;
  }

extern i_integer *ii_negative_infinity_no_ref(void)
  {
    initialize();
    return &static_negative_infinity;
  }

extern i_integer *ii_unsigned_infinity(void)
  {
    initialize();
    GRAB_SYSTEM_LOCK(static_unsigned_infinity.reference_lock);
    ++(static_unsigned_infinity.reference_count);
    RELEASE_SYSTEM_LOCK(static_unsigned_infinity.reference_lock);
    return &static_unsigned_infinity;
  }

extern i_integer *ii_unsigned_infinity_no_ref(void)
  {
    initialize();
    return &static_unsigned_infinity;
  }

extern i_integer *ii_zero_zero(void)
  {
    initialize();
    GRAB_SYSTEM_LOCK(static_zero_zero.reference_lock);
    ++(static_zero_zero.reference_count);
    RELEASE_SYSTEM_LOCK(static_zero_zero.reference_lock);
    return &static_zero_zero;
  }

extern i_integer *ii_zero_zero_no_ref(void)
  {
    initialize();
    return &static_zero_zero;
  }

extern i_integer *ii_zero(void)
  {
    initialize();
    GRAB_SYSTEM_LOCK(static_zero.reference_lock);
    ++(static_zero.reference_count);
    RELEASE_SYSTEM_LOCK(static_zero.reference_lock);
    return &static_zero;
  }

extern i_integer *ii_zero_no_ref(void)
  {
    initialize();
    return &static_zero;
  }

extern i_integer *ii_one(void)
  {
    initialize();
    GRAB_SYSTEM_LOCK(static_one.reference_lock);
    ++(static_one.reference_count);
    RELEASE_SYSTEM_LOCK(static_one.reference_lock);
    return &static_one;
  }

extern i_integer *ii_one_no_ref(void)
  {
    initialize();
    return &static_one;
  }

extern void ii_add_reference(i_integer *ii)
  {
    assert(ii != NULL);

    GRAB_SYSTEM_LOCK(ii->reference_lock);
    ++(ii->reference_count);
    RELEASE_SYSTEM_LOCK(ii->reference_lock);
  }

extern void ii_remove_reference(i_integer *ii)
  {
    size_t new_reference_count;

    assert(ii != NULL);

    GRAB_SYSTEM_LOCK(ii->reference_lock);
    assert(ii->reference_count > 0);
    --(ii->reference_count);
    new_reference_count = ii->reference_count;
    RELEASE_SYSTEM_LOCK(ii->reference_lock);

    if (new_reference_count > 0)
        return;

    assert(ii->kind == IIK_FINITE);
    if (ii->decimal_digits != NULL)
        free(ii->decimal_digits);
    if (ii->bit_blocks != NULL)
        free(ii->bit_blocks);

    DESTROY_SYSTEM_LOCK(ii->decimal_digit_lock);
    DESTROY_SYSTEM_LOCK(ii->bit_block_lock);
    DESTROY_SYSTEM_LOCK(ii->reference_lock);

    free(ii);
  }

extern i_integer_kind ii_kind(i_integer *ii)
  {
    assert(ii != NULL);

    return ii->kind;
  }

extern boolean ii_equal(i_integer *ii1, i_integer *ii2)
  {
    boolean is_negative;
    verdict the_verdict;

    assert(ii1 != NULL);
    assert(ii2 != NULL);

    switch (ii1->kind)
      {
        case IIK_FINITE:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    break;
                case IIK_POSITIVE_INFINITY:
                case IIK_NEGATIVE_INFINITY:
                case IIK_UNSIGNED_INFINITY:
                    return FALSE;
                case IIK_ZERO_ZERO:
                    return FALSE;
                default:
                    assert(FALSE);
              }
            break;
        case IIK_POSITIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    return FALSE;
                case IIK_POSITIVE_INFINITY:
                    return TRUE;
                case IIK_NEGATIVE_INFINITY:
                case IIK_UNSIGNED_INFINITY:
                    return FALSE;
                case IIK_ZERO_ZERO:
                    return FALSE;
                default:
                    assert(FALSE);
              }
        case IIK_NEGATIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    return FALSE;
                case IIK_POSITIVE_INFINITY:
                    return FALSE;
                case IIK_NEGATIVE_INFINITY:
                    return TRUE;
                case IIK_UNSIGNED_INFINITY:
                    return FALSE;
                case IIK_ZERO_ZERO:
                    return FALSE;
                default:
                    assert(FALSE);
              }
        case IIK_UNSIGNED_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    return FALSE;
                case IIK_POSITIVE_INFINITY:
                case IIK_NEGATIVE_INFINITY:
                    return FALSE;
                case IIK_UNSIGNED_INFINITY:
                    return TRUE;
                case IIK_ZERO_ZERO:
                    return FALSE;
                default:
                    assert(FALSE);
              }
        case IIK_ZERO_ZERO:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    return FALSE;
                case IIK_POSITIVE_INFINITY:
                case IIK_NEGATIVE_INFINITY:
                    return FALSE;
                case IIK_UNSIGNED_INFINITY:
                    return FALSE;
                case IIK_ZERO_ZERO:
                    return TRUE;
                default:
                    assert(FALSE);
              }
        default:
            assert(FALSE);
      }

    assert(ii1->kind == IIK_FINITE);
    assert(ii2->kind == IIK_FINITE);

    is_negative = ii1->is_negative;
    if (is_negative != ii2->is_negative)
        return FALSE;

    the_verdict = make_compatible(ii1, ii2);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return FALSE;

    if ((ii1->decimal_digits != NULL) && (ii2->decimal_digits != NULL))
      {
        size_t digit_count;
        char *digits1;
        char *digits2;
        size_t digit_num;

        digit_count = ii1->decimal_digit_count;
        if (digit_count != ii2->decimal_digit_count)
            return FALSE;

        digits1 = ii1->decimal_digits;
        digits2 = ii2->decimal_digits;

        for (digit_num = 0; digit_num < digit_count; ++digit_num)
          {
            if (digits1[digit_num] != digits2[digit_num])
                return FALSE;
          }

        return TRUE;
      }
    else
      {
        unsigned long *blocks1;
        unsigned long *blocks2;
        size_t count1;
        size_t count2;
        size_t combo_count;
        size_t block_num;

        blocks1 = ii1->bit_blocks;
        assert(blocks1 != NULL);
        blocks2 = ii2->bit_blocks;
        assert(blocks2 != NULL);

        count1 = ii1->bit_block_count;
        count2 = ii2->bit_block_count;

        combo_count = ((count1 >= count2) ? count1 : count2);
        for (block_num = 0; block_num < combo_count; ++block_num)
          {
            unsigned long block1;
            unsigned long block2;

            block1 = ((block_num < count1) ? blocks1[block_num] :
                      (is_negative ? ~(unsigned long)0 : 0));
            block2 = ((block_num < count2) ? blocks2[block_num] :
                      (is_negative ? ~(unsigned long)0 : 0));
            if (block1 != block2)
                return FALSE;
          }

        return TRUE;
      }
  }

extern boolean ii_less_than(i_integer *ii1, i_integer *ii2)
  {
    boolean is_negative;
    verdict the_verdict;

    assert(ii1 != NULL);
    assert(ii2 != NULL);

    switch (ii1->kind)
      {
        case IIK_FINITE:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    break;
                case IIK_POSITIVE_INFINITY:
                    return TRUE;
                case IIK_NEGATIVE_INFINITY:
                    return FALSE;
                case IIK_UNSIGNED_INFINITY:
                    return FALSE;
                case IIK_ZERO_ZERO:
                    return FALSE;
                default:
                    assert(FALSE);
              }
            break;
        case IIK_POSITIVE_INFINITY:
            return FALSE;
        case IIK_NEGATIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    return TRUE;
                case IIK_POSITIVE_INFINITY:
                    return TRUE;
                case IIK_NEGATIVE_INFINITY:
                    return FALSE;
                case IIK_UNSIGNED_INFINITY:
                    return FALSE;
                case IIK_ZERO_ZERO:
                    return FALSE;
                default:
                    assert(FALSE);
              }
        case IIK_UNSIGNED_INFINITY:
            return FALSE;
        case IIK_ZERO_ZERO:
            return FALSE;
        default:
            assert(FALSE);
      }

    assert(ii1->kind == IIK_FINITE);
    assert(ii2->kind == IIK_FINITE);

    is_negative = ii1->is_negative;
    if (is_negative != ii2->is_negative)
        return is_negative;

    the_verdict = make_compatible(ii1, ii2);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return FALSE;

    if ((ii1->decimal_digits != NULL) && (ii2->decimal_digits != NULL))
      {
        boolean equal;
        boolean magnitude_is_less;

        magnitude_is_less = decimal_magnitude_is_less(ii1->decimal_digits,
                ii2->decimal_digits, ii1->decimal_digit_count,
                ii2->decimal_digit_count, &equal);
        if (equal)
            return FALSE;
        return (magnitude_is_less ? !is_negative : is_negative);
      }
    else
      {
        unsigned long *blocks1;
        unsigned long *blocks2;
        size_t count1;
        size_t count2;
        size_t combo_count;
        size_t block_num;

        blocks1 = ii1->bit_blocks;
        assert(blocks1 != NULL);
        blocks2 = ii2->bit_blocks;
        assert(blocks2 != NULL);

        count1 = ii1->bit_block_count;
        count2 = ii2->bit_block_count;

        combo_count = ((count1 >= count2) ? count1 : count2);
        for (block_num = combo_count; block_num > 0; --block_num)
          {
            unsigned long block1;
            unsigned long block2;

            block1 = ((block_num <= count1) ? blocks1[block_num - 1] :
                      (is_negative ? ~(unsigned long)0 : 0));
            block2 = ((block_num <= count2) ? blocks2[block_num - 1] :
                      (is_negative ? ~(unsigned long)0 : 0));
            if (block1 != block2)
                return ((block1 < block2) ? !is_negative : is_negative);
          }

        return FALSE;
      }
  }

extern boolean ii_less_than_size_t(i_integer *ii, size_t the_size_t)
  {
    assert(ii != NULL);

    switch (ii->kind)
      {
        case IIK_FINITE:
            break;
        case IIK_POSITIVE_INFINITY:
            return FALSE;
        case IIK_NEGATIVE_INFINITY:
            return TRUE;
        case IIK_UNSIGNED_INFINITY:
            return FALSE;
        case IIK_ZERO_ZERO:
            return FALSE;
        default:
            assert(FALSE);
      }

    assert(ii->kind == IIK_FINITE);

    if (ii->is_negative)
        return TRUE;

    if (ii->decimal_digits != NULL)
      {
        char buffer[(sizeof(unsigned long) * 3) + 2];
        size_t size_t_digit_count;
        size_t half_count;
        size_t digit_num;
        boolean equal;
        boolean magnitude_is_less;

        sprintf(buffer, "%lu", (unsigned long)the_size_t);
        size_t_digit_count = strlen(buffer);
        if ((size_t_digit_count == 1) && (buffer[0] == '0'))
            size_t_digit_count = 0;
        half_count = (size_t_digit_count + 1) / 2;
        for (digit_num = 0; digit_num < half_count; ++digit_num)
          {
            char save;

            save = ((buffer[digit_num] - '0') + DECIMAL_STORAGE_ZERO_CHAR);
            buffer[digit_num] =
                    ((buffer[size_t_digit_count - (digit_num + 1)] - '0') +
                     DECIMAL_STORAGE_ZERO_CHAR);
            buffer[size_t_digit_count - (digit_num + 1)] = save;
          }
        magnitude_is_less = decimal_magnitude_is_less(ii->decimal_digits,
                buffer, ii->decimal_digit_count, size_t_digit_count, &equal);
        if (equal)
            return FALSE;
        return magnitude_is_less;
      }
    else
      {
        unsigned long *blocks1;
        size_t count1;
        size_t count2;
        size_t remainder;
        unsigned long blocks2[sizeof(unsigned long)];
        size_t combo_count;
        size_t block_num;

        blocks1 = ii->bit_blocks;
        assert(blocks1 != NULL);

        count1 = ii->bit_block_count;

        count2 = 0;
        remainder = the_size_t;
        while (remainder > 0)
          {
            assert(count2 < sizeof(unsigned long));
            if (remainder <= ULONG_MAX)
              {
                blocks2[count2] = (long)remainder;
                remainder = 0;
              }
            else
              {
                size_t divisor;

                divisor = (((size_t)(((~(size_t)0) <= ULONG_MAX) ? 0 :
                                     ULONG_MAX)) + 1);
                blocks2[count2] = remainder % divisor;
                remainder = remainder / divisor;
              }
            ++count2;
          }
        assert(count2 <= sizeof(unsigned long));

        combo_count = ((count1 >= count2) ? count1 : count2);
        for (block_num = combo_count; block_num > 0; --block_num)
          {
            unsigned long block1;
            unsigned long block2;

            block1 = ((block_num <= count1) ? blocks1[block_num - 1] : 0);
            block2 = ((block_num <= count2) ? blocks2[block_num - 1] : 0);
            if (block1 != block2)
                return (block1 < block2);
          }

        return FALSE;
      }
  }

extern boolean ii_is_negative(i_integer *ii)
  {
    assert(ii != NULL);

    assert(ii->kind == IIK_FINITE);
    return ii->is_negative;
  }

extern i_integer *ii_negate(i_integer *ii)
  {
    char *decimal;
    i_integer *result;

    assert(ii != NULL);

    switch (ii->kind)
      {
        case IIK_FINITE:
            break;
        case IIK_POSITIVE_INFINITY:
            return ii_negative_infinity();
        case IIK_NEGATIVE_INFINITY:
            return ii_positive_infinity();
        case IIK_UNSIGNED_INFINITY:
            ii_add_reference(ii);
            return ii;
        case IIK_ZERO_ZERO:
            ii_add_reference(ii);
            return ii;
        default:
            assert(FALSE);
      }

    assert(ii->kind == IIK_FINITE);

    decimal = force_decimal_digits(ii);
    if (decimal == NULL)
        return NULL;

    result = create_empty_ii(IIK_FINITE);
    if (result == NULL)
        return NULL;

    result->is_negative = !(ii->is_negative);

    assert((ii->decimal_digits != NULL) || (ii->bit_blocks != NULL));

    if (ii->decimal_digits != NULL)
      {
        size_t count;
        char *copy;

        count = ii->decimal_digit_count;
        copy = MALLOC_ARRAY(char, ((count == 0) ? 1 : count));
        if (copy == NULL)
          {
            ii_remove_reference(result);
            return NULL;
          }

        if (count > 0)
            memcpy(copy, ii->decimal_digits, count);
        else
            result->is_negative = FALSE;

        result->decimal_digit_count = count;
        result->decimal_digits = copy;
      }

    return result;
  }

extern i_integer *ii_add(i_integer *ii1, i_integer *ii2)
  {
    verdict the_verdict;
    char *decimal;
    i_integer *result;

    assert(ii1 != NULL);
    assert(ii2 != NULL);

    switch (ii1->kind)
      {
        case IIK_FINITE:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    break;
                case IIK_POSITIVE_INFINITY:
                case IIK_NEGATIVE_INFINITY:
                case IIK_UNSIGNED_INFINITY:
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
            break;
        case IIK_POSITIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_NEGATIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_NEGATIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_UNSIGNED_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_ZERO_ZERO:
            ii_add_reference(ii1);
            return ii1;
        default:
            assert(FALSE);
      }

    assert(ii1->kind == IIK_FINITE);
    assert(ii2->kind == IIK_FINITE);

    the_verdict = make_compatible(ii1, ii2);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    decimal = force_decimal_digits(ii1);
    if (decimal == NULL)
        return NULL;

    decimal = force_decimal_digits(ii2);
    if (decimal == NULL)
        return NULL;

    result = create_empty_ii(IIK_FINITE);
    if (result == NULL)
        return NULL;

    if ((ii1->decimal_digits != NULL) && (ii2->decimal_digits != NULL))
      {
        boolean is_negative1;
        boolean is_negative2;
        char *new_digits;
        size_t new_count;

        is_negative1 = ii1->is_negative;
        is_negative2 = ii2->is_negative;

        if (is_negative1 == is_negative2)
          {
            result->is_negative = is_negative1;

            new_digits = add_decimal_magnitudes_with_allocation(
                    ii1->decimal_digits, ii2->decimal_digits,
                    ii1->decimal_digit_count, ii2->decimal_digit_count,
                    &new_count);
          }
        else
          {
            char *digits1;
            char *digits2;
            size_t count1;
            size_t count2;
            boolean equal;

            digits1 = ii1->decimal_digits;
            digits2 = ii2->decimal_digits;
            count1 = ii1->decimal_digit_count;
            count2 = ii2->decimal_digit_count;

            if (decimal_magnitude_is_less(digits1, digits2, count1, count2,
                                          &equal))
              {
                char *temp_digits;
                size_t temp_count;

                result->is_negative = is_negative2;

                temp_digits = digits1;
                digits1 = digits2;
                digits2 = temp_digits;

                temp_count = count1;
                count1 = count2;
                count2 = temp_count;
              }
            else
              {
                result->is_negative = is_negative1;
              }

            new_digits = subtract_decimal_magnitudes_with_allocation(digits1,
                    digits2, count1, count2, &new_count);
          }

        if (new_digits == NULL)
          {
            ii_remove_reference(result);
            return NULL;
          }

        if (new_count == 0)
            result->is_negative = FALSE;

        result->decimal_digit_count = new_count;
        result->decimal_digits = new_digits;
      }

    return result;
  }

extern i_integer *ii_subtract(i_integer *ii1, i_integer *ii2)
  {
    verdict the_verdict;
    char *decimal;
    i_integer *result;

    assert(ii1 != NULL);
    assert(ii2 != NULL);

    switch (ii1->kind)
      {
        case IIK_FINITE:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    break;
                case IIK_POSITIVE_INFINITY:
                    return ii_negative_infinity();
                case IIK_NEGATIVE_INFINITY:
                    return ii_positive_infinity();
                case IIK_UNSIGNED_INFINITY:
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
            break;
        case IIK_POSITIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_NEGATIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_NEGATIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_UNSIGNED_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_ZERO_ZERO:
            ii_add_reference(ii1);
            return ii1;
        default:
            assert(FALSE);
      }

    assert(ii1->kind == IIK_FINITE);
    assert(ii2->kind == IIK_FINITE);

    the_verdict = make_compatible(ii1, ii2);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    decimal = force_decimal_digits(ii1);
    if (decimal == NULL)
        return NULL;

    decimal = force_decimal_digits(ii2);
    if (decimal == NULL)
        return NULL;

    result = create_empty_ii(IIK_FINITE);
    if (result == NULL)
        return NULL;

    if ((ii1->decimal_digits != NULL) && (ii2->decimal_digits != NULL))
      {
        boolean is_negative1;
        boolean is_negative2;
        char *new_digits;
        size_t new_count;

        is_negative1 = ii1->is_negative;
        is_negative2 = !(ii2->is_negative);

        if (is_negative1 == is_negative2)
          {
            result->is_negative = is_negative1;

            new_digits = add_decimal_magnitudes_with_allocation(
                    ii1->decimal_digits, ii2->decimal_digits,
                    ii1->decimal_digit_count, ii2->decimal_digit_count,
                    &new_count);
          }
        else
          {
            char *digits1;
            char *digits2;
            size_t count1;
            size_t count2;
            boolean equal;

            digits1 = ii1->decimal_digits;
            digits2 = ii2->decimal_digits;
            count1 = ii1->decimal_digit_count;
            count2 = ii2->decimal_digit_count;

            if (decimal_magnitude_is_less(digits1, digits2, count1, count2,
                                          &equal))
              {
                char *temp_digits;
                size_t temp_count;

                result->is_negative = is_negative2;

                temp_digits = digits1;
                digits1 = digits2;
                digits2 = temp_digits;

                temp_count = count1;
                count1 = count2;
                count2 = temp_count;
              }
            else
              {
                result->is_negative = is_negative1;
              }

            new_digits = subtract_decimal_magnitudes_with_allocation(digits1,
                    digits2, count1, count2, &new_count);
          }

        if (new_digits == NULL)
          {
            ii_remove_reference(result);
            return NULL;
          }

        if (new_count == 0)
            result->is_negative = FALSE;

        result->decimal_digit_count = new_count;
        result->decimal_digits = new_digits;
      }

    return result;
  }

extern i_integer *ii_multiply(i_integer *ii1, i_integer *ii2)
  {
    verdict the_verdict;
    char *decimal;
    i_integer *result;

    assert(ii1 != NULL);
    assert(ii2 != NULL);

    switch (ii1->kind)
      {
        case IIK_FINITE:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    break;
                case IIK_POSITIVE_INFINITY:
                    if (is_zero(ii1))
                        return ii_zero_zero();
                    if (ii1->is_negative)
                        return ii_negative_infinity();
                    ii_add_reference(ii2);
                    return ii2;
                case IIK_NEGATIVE_INFINITY:
                    if (is_zero(ii1))
                        return ii_zero_zero();
                    if (ii1->is_negative)
                        return ii_positive_infinity();
                    ii_add_reference(ii2);
                    return ii2;
                case IIK_UNSIGNED_INFINITY:
                    if (is_zero(ii1))
                        return ii_zero_zero();
                    ii_add_reference(ii2);
                    return ii2;
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
            break;
        case IIK_POSITIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    if (is_zero(ii2))
                        return ii_zero_zero();
                    if (ii2->is_negative)
                        return ii_negative_infinity();
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_NEGATIVE_INFINITY:
                    ii_add_reference(ii2);
                    return ii2;
                case IIK_UNSIGNED_INFINITY:
                    ii_add_reference(ii2);
                    return ii2;
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_NEGATIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    if (is_zero(ii2))
                        return ii_zero_zero();
                    if (ii2->is_negative)
                        return ii_positive_infinity();
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_NEGATIVE_INFINITY:
                    return ii_positive_infinity();
                case IIK_UNSIGNED_INFINITY:
                    ii_add_reference(ii2);
                    return ii2;
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_UNSIGNED_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    if (is_zero(ii2))
                        return ii_zero_zero();
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_NEGATIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_UNSIGNED_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_ZERO_ZERO:
            ii_add_reference(ii1);
            return ii1;
        default:
            assert(FALSE);
      }

    assert(ii1->kind == IIK_FINITE);
    assert(ii2->kind == IIK_FINITE);

    the_verdict = make_compatible(ii1, ii2);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    decimal = force_decimal_digits(ii1);
    if (decimal == NULL)
        return NULL;

    decimal = force_decimal_digits(ii2);
    if (decimal == NULL)
        return NULL;

    result = create_empty_ii(IIK_FINITE);
    if (result == NULL)
        return NULL;

    if (ii1->is_negative)
        result->is_negative = !(ii2->is_negative);
    else
        result->is_negative = ii2->is_negative;

    if ((ii1->decimal_digits != NULL) && (ii2->decimal_digits != NULL))
      {
        size_t result_count;
        char *result_digits;
        verdict the_verdict;

        result_count = ii1->decimal_digit_count + ii2->decimal_digit_count;

        result_digits =
                MALLOC_ARRAY(char, ((result_count == 0) ? 1 : result_count));
        if (result_digits == NULL)
          {
            ii_remove_reference(result);
            return NULL;
          }

        the_verdict = multiply_decimal_magnitudes(ii1->decimal_digits,
                ii2->decimal_digits, ii1->decimal_digit_count,
                ii2->decimal_digit_count, result_digits);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            free(result_digits);
            ii_remove_reference(result);
            return NULL;
          }

        while ((result_count > 0) &&
               (result_digits[result_count - 1] == DECIMAL_STORAGE_ZERO_CHAR))
          {
            --result_count;
          }

        if (result_count == 0)
            result->is_negative = FALSE;

        result->decimal_digit_count = result_count;
        result->decimal_digits = result_digits;
      }

    return result;
  }

extern i_integer *ii_divide(i_integer *ii1, i_integer *ii2,
                            i_integer **remainder)
  {
    verdict the_verdict;
    char *decimal;
    i_integer *result;

    assert(ii1 != NULL);
    assert(ii2 != NULL);

    switch (ii1->kind)
      {
        case IIK_FINITE:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    break;
                case IIK_POSITIVE_INFINITY:
                    ii_add_reference(ii1);
                    *remainder = ii1;
                    return ii_zero();
                case IIK_NEGATIVE_INFINITY:
                    ii_add_reference(ii1);
                    *remainder = ii1;
                    return ii_zero();
                case IIK_UNSIGNED_INFINITY:
                    ii_add_reference(ii1);
                    *remainder = ii1;
                    return ii_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    *remainder = ii2;
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
            break;
        case IIK_POSITIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    *remainder = ii_zero_zero();
                    if (is_zero(ii2))
                        return ii_unsigned_infinity();
                    if (ii2->is_negative)
                        return ii_negative_infinity();
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    *remainder = ii_zero_zero();
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    *remainder = ii_zero_zero();
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    *remainder = ii_zero_zero();
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    *remainder = ii2;
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_NEGATIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    *remainder = ii_zero_zero();
                    if (is_zero(ii2))
                        return ii_unsigned_infinity();
                    if (ii2->is_negative)
                        return ii_positive_infinity();
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    *remainder = ii_zero_zero();
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    *remainder = ii_zero_zero();
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    *remainder = ii_zero_zero();
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    *remainder = ii2;
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_UNSIGNED_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    *remainder = ii_zero_zero();
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    *remainder = ii_zero_zero();
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    *remainder = ii_zero_zero();
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    *remainder = ii_zero_zero();
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    *remainder = ii2;
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_ZERO_ZERO:
            ii_add_reference(ii1);
            *remainder = ii1;
            ii_add_reference(ii1);
            return ii1;
        default:
            assert(FALSE);
      }

    assert(ii1->kind == IIK_FINITE);
    assert(ii2->kind == IIK_FINITE);

    if (is_zero(ii2))
      {
        *remainder = ii_zero_zero();
        if (is_zero(ii1))
            return ii_zero_zero();
        else
            return ii_unsigned_infinity();
      }

    if (is_zero(ii1))
      {
        ii_add_reference(ii1);
        *remainder = ii1;
        ii_add_reference(ii1);
        return ii1;
      }

    the_verdict = make_compatible(ii1, ii2);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        *remainder = NULL;
        return NULL;
      }

    decimal = force_decimal_digits(ii1);
    if (decimal == NULL)
      {
        *remainder = NULL;
        return NULL;
      }

    decimal = force_decimal_digits(ii2);
    if (decimal == NULL)
      {
        *remainder = NULL;
        return NULL;
      }

    result = create_empty_ii(IIK_FINITE);
    if (result == NULL)
      {
        *remainder = NULL;
        return NULL;
      }

    if (ii1->is_negative)
        result->is_negative = !(ii2->is_negative);
    else
        result->is_negative = ii2->is_negative;

    if ((ii1->decimal_digits != NULL) && (ii2->decimal_digits != NULL))
      {
        size_t count1;
        size_t count2;
        char *digits1;
        char *digits2;
        size_t result_count;
        char *result_digits;
        char *extended_2_digits;
        size_t digit_num;
        size_t remainder_count;
        char *remainder_digits;

        count1 = ii1->decimal_digit_count;
        count2 = ii2->decimal_digit_count;

        digits1 = ii1->decimal_digits;
        digits2 = ii2->decimal_digits;

        if (count2 > count1)
          {
            ii_add_reference(ii1);
            *remainder = ii1;
            ii_remove_reference(result);
            return ii_zero();
          }

        result_count = (count1 - count2) + 1;
        assert(result_count > 0);

        result_digits = MALLOC_ARRAY(char, result_count);
        if (result_digits == NULL)
          {
            ii_remove_reference(result);
            *remainder = NULL;
            return NULL;
          }

        extended_2_digits = MALLOC_ARRAY(char, count2 + result_count);
        if (extended_2_digits == NULL)
          {
            free(result_digits);
            ii_remove_reference(result);
            *remainder = NULL;
            return NULL;
          }

        for (digit_num = 0; digit_num < (result_count - 1); ++digit_num)
            extended_2_digits[digit_num] = DECIMAL_STORAGE_ZERO_CHAR;

        for (digit_num = 0; digit_num < count2; ++digit_num)
          {
            extended_2_digits[digit_num + (result_count - 1)] =
                    digits2[digit_num];
          }

        remainder_count = count1;
        remainder_digits = MALLOC_ARRAY(char, ((count1 == 0) ? 1 : count1));
        if (remainder_digits == NULL)
          {
            free(extended_2_digits);
            free(result_digits);
            ii_remove_reference(result);
            *remainder = NULL;
            return NULL;
          }

        for (digit_num = 0; digit_num < count1; ++digit_num)
            remainder_digits[digit_num] = digits1[digit_num];

        for (digit_num = 0; digit_num < result_count; ++digit_num)
          {
            size_t repetitions;

            repetitions = 0;
            while (TRUE)
              {
                boolean equal;
                char *new_remainder_digits;
                size_t new_remainder_count;

                if (decimal_magnitude_is_less(remainder_digits,
                            extended_2_digits + digit_num, remainder_count,
                            count2 + ((result_count - 1) - digit_num), &equal))
                  {
                    break;
                  }

                ++repetitions;

                new_remainder_digits =
                        subtract_decimal_magnitudes_with_allocation(
                                remainder_digits,
                                extended_2_digits + digit_num, remainder_count,
                                count2 + ((result_count - 1) - digit_num),
                                &new_remainder_count);
                free(remainder_digits);
                if (new_remainder_digits == NULL)
                  {
                    free(extended_2_digits);
                    free(result_digits);
                    ii_remove_reference(result);
                    *remainder = NULL;
                    return NULL;
                  }

                remainder_digits = new_remainder_digits;
                remainder_count = new_remainder_count;
              }

            assert(repetitions < 10);
            result_digits[result_count - (digit_num + 1)] =
                    (repetitions + DECIMAL_STORAGE_ZERO_CHAR);
          }

        free(extended_2_digits);

        while ((result_count > 0) &&
               (result_digits[result_count - 1] == DECIMAL_STORAGE_ZERO_CHAR))
          {
            --result_count;
          }

        if (result_count == 0)
            result->is_negative = FALSE;

        result->decimal_digit_count = result_count;
        result->decimal_digits = result_digits;

        while ((remainder_count > 0) &&
               (remainder_digits[remainder_count - 1] ==
                DECIMAL_STORAGE_ZERO_CHAR))
          {
            --remainder_count;
          }

        *remainder = create_empty_ii(IIK_FINITE);
        if (*remainder == NULL)
          {
            free(remainder_digits);
            ii_remove_reference(result);
            return NULL;
          }

        (*remainder)->is_negative =
                ((remainder_count == 0) ? FALSE : ii1->is_negative);
        (*remainder)->decimal_digit_count = remainder_count;
        (*remainder)->decimal_digits = remainder_digits;
      }
    else
      {
        *remainder = ii_zero();
      }

    return result;
  }

extern i_integer *ii_shift_left(i_integer *ii1, i_integer *ii2)
  {
    size_t magnitude;

    assert(ii1 != NULL);
    assert(ii2 != NULL);

    switch (ii1->kind)
      {
        case IIK_FINITE:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    break;
                case IIK_POSITIVE_INFINITY:
                    if (is_zero(ii1))
                        return ii_zero_zero();
                    if (ii1->is_negative)
                        return ii_negative_infinity();
                    ii_add_reference(ii2);
                    return ii2;
                case IIK_NEGATIVE_INFINITY:
                    if (ii1->is_negative)
                        return ii_minus_one();
                    return ii_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
            break;
        case IIK_POSITIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_NEGATIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_NEGATIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_NEGATIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_UNSIGNED_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_NEGATIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_ZERO_ZERO:
            ii_add_reference(ii1);
            return ii1;
        default:
            assert(FALSE);
      }

    assert(ii1->kind == IIK_FINITE);
    assert(ii2->kind == IIK_FINITE);

    magnitude = ii_to_size_t(ii2);

    if (!(ii2->is_negative))
        return shift_up(ii1, magnitude);
    else
        return shift_down(ii1, magnitude);
  }

extern i_integer *ii_shift_right(i_integer *ii1, i_integer *ii2)
  {
    size_t magnitude;

    assert(ii1 != NULL);
    assert(ii2 != NULL);

    switch (ii1->kind)
      {
        case IIK_FINITE:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    break;
                case IIK_POSITIVE_INFINITY:
                    if (ii1->is_negative)
                        return ii_minus_one();
                    return ii_zero();
                case IIK_NEGATIVE_INFINITY:
                    if (is_zero(ii1))
                        return ii_zero_zero();
                    if (ii1->is_negative)
                      {
                        ii_add_reference(ii2);
                        return ii2;
                      }
                    return ii_positive_infinity();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
            break;
        case IIK_POSITIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_NEGATIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_UNSIGNED_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_ZERO_ZERO:
            ii_add_reference(ii1);
            return ii1;
        default:
            assert(FALSE);
      }

    assert(ii1->kind == IIK_FINITE);
    assert(ii2->kind == IIK_FINITE);

    magnitude = ii_to_size_t(ii2);

    if (ii2->is_negative)
        return shift_up(ii1, magnitude);
    else
        return shift_down(ii1, magnitude);
  }

extern i_integer *ii_bitwise_and(i_integer *ii1, i_integer *ii2)
  {
    unsigned long *block1;
    unsigned long *block2;
    i_integer *result;
    boolean is_negative1;
    boolean is_negative2;
    size_t count1;
    size_t count2;
    size_t combo_count;
    unsigned long *result_blocks;
    size_t block_num;
    boolean result_is_negative;

    assert(ii1 != NULL);
    assert(ii2 != NULL);

    if (is_zero(ii1))
      {
        ii_add_reference(ii1);
        return ii1;
      }

    if (is_zero(ii2))
      {
        ii_add_reference(ii2);
        return ii2;
      }

    switch (ii1->kind)
      {
        case IIK_FINITE:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    break;
                case IIK_POSITIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
            break;
        case IIK_POSITIVE_INFINITY:
            return ii_zero_zero();
        case IIK_NEGATIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    return ii_zero_zero();
                case IIK_POSITIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_UNSIGNED_INFINITY:
            return ii_zero_zero();
        case IIK_ZERO_ZERO:
            ii_add_reference(ii1);
            return ii1;
        default:
            assert(FALSE);
      }

    assert(ii1->kind == IIK_FINITE);
    assert(ii2->kind == IIK_FINITE);

    block1 = force_bit_blocks(ii1);
    if (block1 == NULL)
        return NULL;

    block2 = force_bit_blocks(ii2);
    if (block2 == NULL)
        return NULL;

    result = create_empty_ii(IIK_FINITE);
    if (result == NULL)
        return NULL;

    is_negative1 = ii1->is_negative;
    is_negative2 = ii2->is_negative;

    count1 = ii1->bit_block_count;
    count2 = ii2->bit_block_count;

    combo_count = ((count1 >= count2) ? count1 : count2);

    result_blocks = MALLOC_ARRAY(unsigned long,
                                 ((combo_count == 0) ? 1 : combo_count));
    if (result_blocks == NULL)
      {
        ii_remove_reference(result);
        return NULL;
      }

    for (block_num = 0; block_num < combo_count; ++block_num)
      {
        unsigned long bits1;
        unsigned long bits2;

        if (block_num < count1)
            bits1 = block1[block_num];
        else
            bits1 = (is_negative1 ? ~(unsigned long)0 : 0);

        if (block_num < count2)
            bits2 = block2[block_num];
        else
            bits2 = (is_negative2 ? ~(unsigned long)0 : 0);

        result_blocks[block_num] = (bits1 & bits2);
      }

    result_is_negative = (is_negative1 && is_negative2);

    while ((combo_count > 0) &&
           (result_blocks[combo_count - 1] ==
            (result_is_negative ? ~(unsigned long)0 : 0)))
      {
        --combo_count;
      }

    result->is_negative = result_is_negative;
    result->bit_block_count = combo_count;
    result->bit_blocks = result_blocks;

    return result;
  }

extern i_integer *ii_bitwise_or(i_integer *ii1, i_integer *ii2)
  {
    unsigned long *block1;
    unsigned long *block2;
    i_integer *result;
    boolean is_negative1;
    boolean is_negative2;
    size_t count1;
    size_t count2;
    size_t combo_count;
    unsigned long *result_blocks;
    size_t block_num;
    boolean result_is_negative;

    assert(ii1 != NULL);
    assert(ii2 != NULL);

    switch (ii1->kind)
      {
        case IIK_FINITE:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    break;
                case IIK_POSITIVE_INFINITY:
                    ii_add_reference(ii2);
                    return ii2;
                case IIK_NEGATIVE_INFINITY:
                    ii_add_reference(ii2);
                    return ii2;
                case IIK_UNSIGNED_INFINITY:
                    ii_add_reference(ii2);
                    return ii2;
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
            break;
        case IIK_POSITIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_NEGATIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_NEGATIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_UNSIGNED_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_ZERO_ZERO:
            ii_add_reference(ii1);
            return ii1;
        default:
            assert(FALSE);
      }

    assert(ii1->kind == IIK_FINITE);
    assert(ii2->kind == IIK_FINITE);

    block1 = force_bit_blocks(ii1);
    if (block1 == NULL)
        return NULL;

    block2 = force_bit_blocks(ii2);
    if (block2 == NULL)
        return NULL;

    result = create_empty_ii(IIK_FINITE);
    if (result == NULL)
        return NULL;

    is_negative1 = ii1->is_negative;
    is_negative2 = ii2->is_negative;

    count1 = ii1->bit_block_count;
    count2 = ii2->bit_block_count;

    combo_count = ((count1 >= count2) ? count1 : count2);

    result_blocks = MALLOC_ARRAY(unsigned long,
                                 ((combo_count == 0) ? 1 : combo_count));
    if (result_blocks == NULL)
      {
        ii_remove_reference(result);
        return NULL;
      }

    for (block_num = 0; block_num < combo_count; ++block_num)
      {
        unsigned long bits1;
        unsigned long bits2;

        if (block_num < count1)
            bits1 = block1[block_num];
        else
            bits1 = (is_negative1 ? ~(unsigned long)0 : 0);

        if (block_num < count2)
            bits2 = block2[block_num];
        else
            bits2 = (is_negative2 ? ~(unsigned long)0 : 0);

        result_blocks[block_num] = (bits1 | bits2);
      }

    result_is_negative = (is_negative1 || is_negative2);

    while ((combo_count > 0) &&
           (result_blocks[combo_count - 1] ==
            (result_is_negative ? ~(unsigned long)0 : 0)))
      {
        --combo_count;
      }

    result->is_negative = result_is_negative;
    result->bit_block_count = combo_count;
    result->bit_blocks = result_blocks;

    return result;
  }

extern i_integer *ii_bitwise_xor(i_integer *ii1, i_integer *ii2)
  {
    unsigned long *block1;
    unsigned long *block2;
    i_integer *result;
    boolean is_negative1;
    boolean is_negative2;
    size_t count1;
    size_t count2;
    size_t combo_count;
    unsigned long *result_blocks;
    size_t block_num;
    boolean result_is_negative;

    assert(ii1 != NULL);
    assert(ii2 != NULL);

    switch (ii1->kind)
      {
        case IIK_FINITE:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    break;
                case IIK_POSITIVE_INFINITY:
                    ii_add_reference(ii2);
                    return ii2;
                case IIK_NEGATIVE_INFINITY:
                    ii_add_reference(ii2);
                    return ii2;
                case IIK_UNSIGNED_INFINITY:
                    ii_add_reference(ii2);
                    return ii2;
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
            break;
        case IIK_POSITIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_NEGATIVE_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_UNSIGNED_INFINITY:
            switch (ii2->kind)
              {
                case IIK_FINITE:
                    ii_add_reference(ii1);
                    return ii1;
                case IIK_POSITIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_NEGATIVE_INFINITY:
                    return ii_zero_zero();
                case IIK_UNSIGNED_INFINITY:
                    return ii_zero_zero();
                case IIK_ZERO_ZERO:
                    ii_add_reference(ii2);
                    return ii2;
                default:
                    assert(FALSE);
              }
        case IIK_ZERO_ZERO:
            ii_add_reference(ii1);
            return ii1;
        default:
            assert(FALSE);
      }

    assert(ii1->kind == IIK_FINITE);
    assert(ii2->kind == IIK_FINITE);

    block1 = force_bit_blocks(ii1);
    if (block1 == NULL)
        return NULL;

    block2 = force_bit_blocks(ii2);
    if (block2 == NULL)
        return NULL;

    result = create_empty_ii(IIK_FINITE);
    if (result == NULL)
        return NULL;

    is_negative1 = ii1->is_negative;
    is_negative2 = ii2->is_negative;

    count1 = ii1->bit_block_count;
    count2 = ii2->bit_block_count;

    combo_count = ((count1 >= count2) ? count1 : count2);

    result_blocks = MALLOC_ARRAY(unsigned long,
                                 ((combo_count == 0) ? 1 : combo_count));
    if (result_blocks == NULL)
      {
        ii_remove_reference(result);
        return NULL;
      }

    for (block_num = 0; block_num < combo_count; ++block_num)
      {
        unsigned long bits1;
        unsigned long bits2;

        if (block_num < count1)
            bits1 = block1[block_num];
        else
            bits1 = (is_negative1 ? ~(unsigned long)0 : 0);

        if (block_num < count2)
            bits2 = block2[block_num];
        else
            bits2 = (is_negative2 ? ~(unsigned long)0 : 0);

        result_blocks[block_num] = (bits1 ^ bits2);
      }

    result_is_negative = ((is_negative1 && !is_negative2) ||
                          ((!is_negative1) && is_negative2));

    while ((combo_count > 0) &&
           (result_blocks[combo_count - 1] ==
            (result_is_negative ? ~(unsigned long)0 : 0)))
      {
        --combo_count;
      }

    result->is_negative = result_is_negative;
    result->bit_block_count = combo_count;
    result->bit_blocks = result_blocks;

    return result;
  }

extern i_integer *ii_power_of_ten(i_integer *exponent)
  {
    size_t size_t_exponent;
    verdict the_verdict;
    char *buffer;
    size_t zero_num;
    i_integer *result;

    assert(exponent != NULL);

    switch (exponent->kind)
      {
        case IIK_FINITE:
            break;
        case IIK_POSITIVE_INFINITY:
            ii_add_reference(exponent);
            return exponent;
        case IIK_NEGATIVE_INFINITY:
            assert(FALSE);
            return NULL;
        case IIK_UNSIGNED_INFINITY:
            assert(FALSE);
            return NULL;
        case IIK_ZERO_ZERO:
            ii_add_reference(exponent);
            return exponent;
        default:
            assert(FALSE);
      }

    assert(exponent->kind == IIK_FINITE);

    the_verdict = ii_magnitude_to_size_t(exponent, &size_t_exponent);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    buffer = MALLOC_ARRAY(char, size_t_exponent + 1);
    if (buffer == NULL)
        return NULL;

    buffer[0] = '1';

    for (zero_num = 0; zero_num < size_t_exponent; ++zero_num)
        buffer[zero_num + 1] = '0';

    result = ii_create_from_decimal_ascii(size_t_exponent + 1, buffer, FALSE);
    free(buffer);
    return result;
  }

extern verdict ii_decimal_digit_count(i_integer *ii, size_t *digit_count)
  {
    char *decimal;

    assert(ii != NULL);
    assert(digit_count != NULL);

    assert(ii->kind == IIK_FINITE);

    decimal = force_decimal_digits(ii);
    if (decimal == NULL)
        return MISSION_FAILED;

    *digit_count = ii->decimal_digit_count;
    return MISSION_ACCOMPLISHED;
  }

extern verdict ii_write_decimal_digits(i_integer *ii, char *location)
  {
    char *decimal;
    size_t digit_count;
    size_t digit_num;

    assert(ii != NULL);
    assert(location != NULL);

    assert(ii->kind == IIK_FINITE);

    decimal = force_decimal_digits(ii);
    if (decimal == NULL)
        return MISSION_FAILED;

    digit_count = ii->decimal_digit_count;
    for (digit_num = 0; digit_num < digit_count; ++digit_num)
      {
        location[digit_num] = ((decimal[digit_count - (digit_num + 1)] -
                                DECIMAL_STORAGE_ZERO_CHAR) + '0');
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict ii_hex_digits(i_integer *ii, size_t *digit_count)
  {
    unsigned long *blocks;
    size_t digits_per_block;
    size_t result;
    unsigned long drop_pattern;

    assert(ii != NULL);
    assert(digit_count != NULL);

    assert(ii->kind == IIK_FINITE);

    blocks = force_bit_blocks(ii);
    if (blocks == NULL)
        return MISSION_FAILED;

    digits_per_block = (sizeof(unsigned long) * 2);
    result = (ii->bit_block_count * digits_per_block);
    drop_pattern = (ii->is_negative ? 0xf : 0);
    while ((result > 0) &&
           (((blocks[(result - 1) / digits_per_block] >>
              (((result - 1) % digits_per_block) * 4)) & 0xf) == drop_pattern))
      {
        --result;
      }
    *digit_count = result;

    return MISSION_ACCOMPLISHED;
  }

extern verdict ii_write_hex_digits(i_integer *ii, char *location,
                                   boolean uppercase)
  {
    size_t digit_count;
    verdict the_verdict;
    unsigned long *blocks;
    size_t digit_num;

    assert(ii != NULL);
    assert(location != NULL);

    assert(ii->kind == IIK_FINITE);

    the_verdict = ii_hex_digits(ii, &digit_count);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    blocks = ii->bit_blocks;
    assert(blocks != NULL);

    for (digit_num = 0; digit_num < digit_count; ++digit_num)
      {
        size_t digits_per_block;
        unsigned long bits;
        char digit;

        digits_per_block = (sizeof(unsigned long) * 2);
        bits = ((blocks[digit_num / digits_per_block] >>
                 ((digit_num % digits_per_block) * 4)) & 0xf);

        if (bits <= 9)
          {
            digit = bits + '0';
          }
        else
          {
            digit = (bits - 0xa) + (uppercase ? 'A': 'a');
          }

        location[digit_count - (digit_num + 1)] = digit;
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict ii_magnitude_to_size_t(i_integer *ii, size_t *result)
  {
    char *digits;
    size_t so_far;
    size_t digit_num;

    assert(ii != NULL);
    assert(result != NULL);

    assert(ii->kind == IIK_FINITE);

    digits = force_decimal_digits(ii);
    if (digits == NULL)
      {
        boolean is_negative;
        size_t block_count;
        unsigned long *blocks;
        size_t so_far;
        size_t block_num;

        is_negative = ii->is_negative;
        block_count = ii->bit_block_count;
        blocks = ii->bit_blocks;
        assert(blocks != NULL);

        so_far = 0;

        for (block_num = 0; block_num < block_count; ++block_num)
          {
            unsigned long bits;
            size_t limit;

            bits = blocks[block_num - (block_num + 1)];
            if (is_negative)
                bits = ~bits;

            if (bits > ~(size_t)0)
                return MISSION_FAILED;

            limit = safe_shift_right_size_t(((~(size_t)0) - (size_t)bits),
                                            (sizeof(unsigned long) * 8));
            if (so_far > limit)
                return MISSION_FAILED;

            so_far = safe_shift_left_size_t(so_far,
                                            (sizeof(unsigned long) * 8));
            so_far += bits;
          }

        *result = so_far;
        return MISSION_ACCOMPLISHED;
      }

    so_far = 0;

    for (digit_num = ii->decimal_digit_count; digit_num > 0; --digit_num)
      {
        char this_digit;

        this_digit = digits[digit_num - 1];
        assert((this_digit >= DECIMAL_STORAGE_ZERO_CHAR) &&
               (this_digit <= DECIMAL_STORAGE_ZERO_CHAR + 9));

        if (so_far >
            (((~(size_t)0) - (this_digit - DECIMAL_STORAGE_ZERO_CHAR)) / 10))
          {
            return MISSION_FAILED;
          }

        so_far = ((so_far * 10) + (this_digit - DECIMAL_STORAGE_ZERO_CHAR));
      }

    *result = so_far;
    return MISSION_ACCOMPLISHED;
  }

extern verdict ii_to_long_int(i_integer *ii, long int *result)
  {
    char *digits;
    long int so_far;
    size_t digit_num;

    assert(ii != NULL);
    assert(result != NULL);

    assert(ii->kind == IIK_FINITE);

    digits = force_decimal_digits(ii);
    if (digits == NULL)
      {
        boolean is_negative;
        size_t block_count;
        unsigned long *blocks;
        long int so_far;
        size_t block_num;

        is_negative = ii->is_negative;
        block_count = ii->bit_block_count;
        blocks = ii->bit_blocks;
        assert(blocks != NULL);

        if (is_negative)
            so_far = -1;
        else
            so_far = 0;

        for (block_num = 0; block_num < block_count; ++block_num)
          {
            unsigned long bits;
            long local_long;
            long limit;

            bits = blocks[block_num - (block_num + 1)];

            if (is_negative)
              {
                unsigned long converter;

                converter = (unsigned long)(ULONG_MAX + LONG_MIN + 1);
                if (bits < converter)
                    return MISSION_FAILED;
                local_long = (((long)(bits - converter)) + LONG_MIN);
                assert(local_long < 0);
              }
            else
              {
                if (bits > LONG_MAX)
                    return MISSION_FAILED;
                local_long = bits;
                assert(local_long >= 0);
              }

            limit = safe_shift_right_long((LONG_MAX - local_long),
                                          (sizeof(unsigned long) * 8));
            if (local_long >= 0)
              {
                if (so_far > limit)
                    return MISSION_FAILED;
              }
            else
              {
                if (so_far < limit)
                    return MISSION_FAILED;
              }

            so_far = safe_shift_left_long(so_far, (sizeof(unsigned long) * 8));
            so_far += local_long;
          }

        *result = so_far;
        return MISSION_ACCOMPLISHED;
      }

    so_far = 0;

    for (digit_num = ii->decimal_digit_count; digit_num > 0; --digit_num)
      {
        char this_digit;
        long int to_add;

        this_digit = digits[digit_num - 1];
        assert((this_digit >= DECIMAL_STORAGE_ZERO_CHAR) &&
               (this_digit <= (DECIMAL_STORAGE_ZERO_CHAR + 9)));

        to_add = (this_digit - DECIMAL_STORAGE_ZERO_CHAR);
        if (ii->is_negative)
          {
            to_add = -to_add;

            if (so_far < (((LONG_MIN) - to_add) / 10))
                return MISSION_FAILED;
          }
        else
          {
            if (so_far > (((LONG_MAX) - to_add) / 10))
                return MISSION_FAILED;
          }

        so_far = ((so_far * 10) + to_add);
      }

    *result = so_far;
    return MISSION_ACCOMPLISHED;
  }

extern verdict ii_to_unsigned_long_int(i_integer *ii,
                                       unsigned long int *result)
  {
    char *digits;
    unsigned long int so_far;
    size_t digit_num;

    assert(ii != NULL);
    assert(result != NULL);

    assert(ii->kind == IIK_FINITE);

    if (ii->is_negative)
        return MISSION_FAILED;

    digits = force_decimal_digits(ii);
    if (digits == NULL)
      {
        size_t block_count;
        unsigned long *blocks;
        unsigned long int so_far;
        size_t block_num;

        block_count = ii->bit_block_count;
        blocks = ii->bit_blocks;
        assert(blocks != NULL);

        so_far = 0;

        for (block_num = 0; block_num < block_count; ++block_num)
          {
            unsigned long bits;

            bits = blocks[block_num - (block_num + 1)];

            if (so_far > 0)
                return MISSION_FAILED;

            so_far += bits;
          }

        *result = so_far;
        return MISSION_ACCOMPLISHED;
      }

    so_far = 0;

    for (digit_num = ii->decimal_digit_count; digit_num > 0; --digit_num)
      {
        char this_digit;
        unsigned long int to_add;

        this_digit = digits[digit_num - 1];
        assert((this_digit >= DECIMAL_STORAGE_ZERO_CHAR) &&
               (this_digit <= (DECIMAL_STORAGE_ZERO_CHAR + 9)));

        to_add = (this_digit - DECIMAL_STORAGE_ZERO_CHAR);
        if (so_far > (((ULONG_MAX) - to_add) / 10))
            return MISSION_FAILED;

        so_far = ((so_far * 10) + to_add);
      }

    *result = so_far;
    return MISSION_ACCOMPLISHED;
  }

extern int ii_structural_order(i_integer *left, i_integer *right)
  {
    assert(left != NULL);
    assert(right != NULL);

    if (left->kind == right->kind)
      {
        switch (left->kind)
          {
            case IIK_FINITE:
                if (ii_equal(left, right))
                    return 0;
                else if (ii_less_than(left, right))
                    return -1;
                else
                    return 1;
            case IIK_POSITIVE_INFINITY:
            case IIK_NEGATIVE_INFINITY:
            case IIK_UNSIGNED_INFINITY:
            case IIK_ZERO_ZERO:
                return 0;
            default:
                assert(FALSE);
          }
      }

    if (left->kind == IIK_NEGATIVE_INFINITY)
        return -1;
    if (right->kind == IIK_NEGATIVE_INFINITY)
        return 1;

    if (left->kind == IIK_FINITE)
        return -1;
    if (right->kind == IIK_FINITE)
        return 1;

    if (left->kind == IIK_POSITIVE_INFINITY)
        return -1;
    if (right->kind == IIK_POSITIVE_INFINITY)
        return 1;

    if (left->kind == IIK_UNSIGNED_INFINITY)
        return -1;

    assert(left->kind == IIK_ZERO_ZERO);
    assert(right->kind == IIK_UNSIGNED_INFINITY);
    return 1;
  }


static void initialize(void)
  {
    static char zero_digits[1];
    static char one_digits[1] = {DECIMAL_STORAGE_ZERO_CHAR + 1};
    static unsigned long zero_blocks[1];
    static unsigned long one_blocks[1] = {1};

    if (statics_initialized)
        return;

    initialize_one(&static_positive_infinity, IIK_POSITIVE_INFINITY);
    initialize_one(&static_negative_infinity, IIK_NEGATIVE_INFINITY);
    initialize_one(&static_unsigned_infinity, IIK_UNSIGNED_INFINITY);
    initialize_one(&static_zero_zero, IIK_ZERO_ZERO);

    initialize_one(&static_zero, IIK_FINITE);
    static_zero.decimal_digit_count = 0;
    static_zero.decimal_digits = &(zero_digits[0]);
    static_zero.bit_block_count = 0;
    static_zero.bit_blocks = &(zero_blocks[0]);

    initialize_one(&static_one, IIK_FINITE);
    static_one.decimal_digit_count = 1;
    static_one.decimal_digits = &(one_digits[0]);
    static_one.bit_block_count = 1;
    static_one.bit_blocks = &(one_blocks[0]);

    initialize_one(&static_minus_one, IIK_FINITE);
    static_minus_one.decimal_digit_count = 1;
    static_minus_one.decimal_digits = &(one_digits[0]);
    static_minus_one.bit_block_count = 0;
    static_minus_one.bit_blocks = &(zero_blocks[0]);

    statics_initialized = TRUE;
  }

static void initialize_one(i_integer *to_initialize, i_integer_kind kind)
  {
    INITIALIZE_SYSTEM_LOCK(to_initialize->decimal_digit_lock, assert(FALSE));
    INITIALIZE_SYSTEM_LOCK(to_initialize->bit_block_lock, assert(FALSE));
    INITIALIZE_SYSTEM_LOCK(to_initialize->reference_lock, assert(FALSE));

    initialize_one_base(to_initialize, kind);
  }

static void initialize_one_base(i_integer *to_initialize, i_integer_kind kind)
  {
    assert(to_initialize != NULL);

    to_initialize->kind = kind;
    to_initialize->is_negative = FALSE;
    to_initialize->decimal_digit_count = 0;
    to_initialize->decimal_digits = NULL;
    to_initialize->bit_block_count = 0;
    to_initialize->bit_blocks = NULL;
    to_initialize->reference_count = 1;
  }

static i_integer *create_empty_ii(i_integer_kind kind)
  {
    i_integer *result;

    result = MALLOC_ONE_OBJECT(i_integer);
    if (result == NULL)
         return NULL;

    INITIALIZE_SYSTEM_LOCK(result->decimal_digit_lock,
            free(result);
            return NULL);

    INITIALIZE_SYSTEM_LOCK(result->bit_block_lock,
            DESTROY_SYSTEM_LOCK(result->decimal_digit_lock);
            free(result);
            return NULL);

    INITIALIZE_SYSTEM_LOCK(result->reference_lock,
            DESTROY_SYSTEM_LOCK(result->bit_block_lock);
            DESTROY_SYSTEM_LOCK(result->decimal_digit_lock);
            free(result);
            return NULL);

    initialize_one_base(result, kind);

    return result;
  }

static verdict make_compatible(i_integer *ii1, i_integer *ii2)
  {
    char *digits;

    assert(ii1 != NULL);
    assert(ii2 != NULL);

    assert(ii1->kind == IIK_FINITE);
    assert(ii2->kind == IIK_FINITE);

    if ((ii1->decimal_digits != NULL) && (ii2->decimal_digits != NULL))
        return MISSION_ACCOMPLISHED;
    if ((ii1->bit_blocks != NULL) && (ii2->bit_blocks != NULL))
        return MISSION_ACCOMPLISHED;

    digits = force_decimal_digits(ii1);
    if (digits == NULL)
        return MISSION_FAILED;

    digits = force_decimal_digits(ii2);
    if (digits == NULL)
        return MISSION_FAILED;

    return MISSION_ACCOMPLISHED;
  }

static char *force_decimal_digits(i_integer *ii)
  {
    char *result;
    size_t block_count;
    size_t result_count;
    size_t digit_num;
    boolean is_negative;
    char *power;
    unsigned long *blocks;
    size_t block_num;

    assert(ii != NULL);

    assert(ii->kind == IIK_FINITE);

    GRAB_SYSTEM_LOCK(ii->decimal_digit_lock);

    result = ii->decimal_digits;
    if (result != NULL)
      {
        RELEASE_SYSTEM_LOCK(ii->decimal_digit_lock);
        return result;
      }

    block_count = ii->bit_block_count;
    result_count = (block_count * sizeof(unsigned long) * 3);
    if (result_count == 0)
        result_count = 1;

    result = MALLOC_ARRAY(char, result_count);
    if (result == NULL)
      {
        RELEASE_SYSTEM_LOCK(ii->decimal_digit_lock);
        return NULL;
      }

    for (digit_num = 0; digit_num < result_count; ++digit_num)
        result[digit_num] = DECIMAL_STORAGE_ZERO_CHAR;

    is_negative = ii->is_negative;
    if (is_negative)
        result[0] = DECIMAL_STORAGE_ZERO_CHAR + 1;

    power = MALLOC_ARRAY(char, result_count);
    if (power == NULL)
      {
        RELEASE_SYSTEM_LOCK(ii->decimal_digit_lock);
        free(result);
        return NULL;
      }

    power[0] = DECIMAL_STORAGE_ZERO_CHAR + 1;
    for (digit_num = 1; digit_num < result_count; ++digit_num)
        power[digit_num] = DECIMAL_STORAGE_ZERO_CHAR;

    blocks = ii->bit_blocks;
    assert(blocks != NULL);

    for (block_num = 0; block_num < block_count; ++block_num)
      {
        unsigned long bits;
        size_t bit_num;

        bits = blocks[block_num];
        if (is_negative)
            bits = ~bits;

        for (bit_num = 0; bit_num < (sizeof(unsigned long) * 8); ++bit_num)
          {
            unsigned carry;
            size_t digit_num;

            if ((bits & 0x1) != 0)
              {
                unsigned carry;
                size_t digit_num;

                carry = 0;
                for (digit_num = 0; digit_num < result_count; ++digit_num)
                  {
                    char digit1;
                    char digit2;

                    digit1 = result[digit_num];
                    digit2 = power[digit_num];

                    assert((digit1 >= DECIMAL_STORAGE_ZERO_CHAR) &&
                           (digit1 <= DECIMAL_STORAGE_ZERO_CHAR + 9));
                    assert((digit2 >= DECIMAL_STORAGE_ZERO_CHAR) &&
                           (digit2 <= DECIMAL_STORAGE_ZERO_CHAR + 9));

                    carry += (digit1 - DECIMAL_STORAGE_ZERO_CHAR) +
                             (digit2 - DECIMAL_STORAGE_ZERO_CHAR);
                    assert(carry <= 19);

                    result[digit_num] =
                            ((carry % 10) + DECIMAL_STORAGE_ZERO_CHAR);
                    carry /= 10;
                  }
                assert(carry == 0);
              }

            carry = 0;
            for (digit_num = 0; digit_num < result_count; ++digit_num)
              {
                char digit;

                digit = power[digit_num];

                assert((digit >= DECIMAL_STORAGE_ZERO_CHAR) &&
                       (digit <= DECIMAL_STORAGE_ZERO_CHAR + 9));

                carry += ((digit - DECIMAL_STORAGE_ZERO_CHAR) * 2);
                assert(carry <= 19);

                power[digit_num] = ((carry % 10) + DECIMAL_STORAGE_ZERO_CHAR);
                carry /= 10;
              }
            assert(carry == 0);

            bits >>= 1;
          }
        assert(bits == 0);
      }

    free(power);

    while ((result_count > 0) &&
           (result[result_count - 1] == DECIMAL_STORAGE_ZERO_CHAR))
      {
        --result_count;
      }

    ii->decimal_digit_count = result_count;
    ii->decimal_digits = result;

    RELEASE_SYSTEM_LOCK(ii->decimal_digit_lock);

    return result;
  }

static unsigned long *force_bit_blocks(i_integer *ii)
  {
    unsigned long *blocks;
    size_t digit_count;
    size_t block_count;
    size_t block_num;
    char *remainder;

    assert(ii != NULL);

    assert(ii->kind == IIK_FINITE);

    GRAB_SYSTEM_LOCK(ii->bit_block_lock);

    blocks = ii->bit_blocks;
    if (blocks != NULL)
      {
        RELEASE_SYSTEM_LOCK(ii->bit_block_lock);
        return blocks;
      }

    assert(ii->decimal_digits != NULL);
    digit_count = ii->decimal_digit_count;
    block_count = ((digit_count + ((sizeof(unsigned long) * 2) - 1)) /
                   (sizeof(unsigned long) * 2));
    if (block_count == 0)
        block_count = 1;

    blocks = MALLOC_ARRAY(unsigned long, block_count);
    if (blocks == NULL)
      {
        RELEASE_SYSTEM_LOCK(ii->bit_block_lock);
        return NULL;
      }

    remainder = MALLOC_ARRAY(char, ((digit_count == 0) ? 1 : digit_count));
    if (remainder == NULL)
      {
        RELEASE_SYSTEM_LOCK(ii->bit_block_lock);
        free(blocks);
        return NULL;
      }

    if (digit_count > 0)
        memcpy(remainder, ii->decimal_digits, digit_count);

    if (ii->is_negative)
      {
        char *follow;

        assert(digit_count > 0);

        follow = remainder;
        while (TRUE)
          {
            assert((follow - remainder) < digit_count);
            assert((*follow >= DECIMAL_STORAGE_ZERO_CHAR) &&
                   (*follow <= DECIMAL_STORAGE_ZERO_CHAR + 9));
            if (*follow > DECIMAL_STORAGE_ZERO_CHAR)
              {
                --(*follow);
                break;
              }

            assert(*follow == DECIMAL_STORAGE_ZERO_CHAR);
            *follow = DECIMAL_STORAGE_ZERO_CHAR + 9;
            ++follow;
          }
      }

    for (block_num = 0; block_num < block_count; ++block_num)
      {
        unsigned long bits;
        size_t bit_num;

        bits = 0;

        for (bit_num = 0; bit_num < (sizeof(unsigned long) * 8); ++bit_num)
          {
            unsigned carry;
            unsigned long this_bit;
            size_t digit_num;

            carry = 0;
            for (digit_num = 0; digit_num < digit_count; ++digit_num)
              {
                char digit;

                digit = remainder[digit_count - (digit_num + 1)];
                assert((digit >= DECIMAL_STORAGE_ZERO_CHAR) &&
                       (digit <= DECIMAL_STORAGE_ZERO_CHAR + 9));

                carry = ((carry * 10) +
                         ((digit - DECIMAL_STORAGE_ZERO_CHAR) * 5));
                assert(carry <= 95);

                remainder[digit_count - (digit_num + 1)] =
                        ((carry / 10) + DECIMAL_STORAGE_ZERO_CHAR);
                carry %= 10;
              }

            if (carry == 5)
              {
                this_bit = 0x1;
              }
            else
              {
                assert(carry == 0);
                this_bit = 0x0;
              }

            bits |= (this_bit << bit_num);
          }

        if (ii->is_negative)
            bits = ~bits;
        blocks[block_num] = bits;
      }

    free(remainder);

    while ((block_count > 0) &&
           (blocks[block_count - 1] ==
            ((ii->is_negative) ? ~(unsigned long)0 : 0)))
      {
        --block_count;
      }

    ii->bit_block_count = block_count;
    ii->bit_blocks = blocks;

    RELEASE_SYSTEM_LOCK(ii->bit_block_lock);

    return blocks;
  }

static boolean is_zero(i_integer *ii)
  {
    assert(ii != NULL);

    switch (ii->kind)
      {
        case IIK_FINITE:
            if (ii->decimal_digits != NULL)
              {
                return (ii->decimal_digit_count == 0);
              }
            else
              {
                if (ii->is_negative)
                    return FALSE;
                assert(ii->bit_blocks != NULL);
                return (ii->bit_block_count == 0);
              }
        case IIK_POSITIVE_INFINITY:
        case IIK_NEGATIVE_INFINITY:
        case IIK_UNSIGNED_INFINITY:
            return FALSE;
        case IIK_ZERO_ZERO:
            return TRUE;
        default:
            assert(FALSE);
            return FALSE;
      }
  }

static i_integer *ii_minus_one(void)
  {
    initialize();
    GRAB_SYSTEM_LOCK(static_minus_one.reference_lock);
    ++(static_minus_one.reference_count);
    RELEASE_SYSTEM_LOCK(static_minus_one.reference_lock);
    return &static_minus_one;
  }

static i_integer *shift_up(i_integer *ii, size_t magnitude)
  {
    unsigned long *old_blocks;
    size_t old_count;
    size_t new_count;
    unsigned long *new_blocks;
    size_t block_num;
    boolean is_negative;
    size_t bit_num;
    i_integer *result;

    assert(ii != NULL);

    assert(ii->kind == IIK_FINITE);

    if (magnitude == 0)
      {
        ii_add_reference(ii);
        return ii;
      }

    old_blocks = force_bit_blocks(ii);
    if (old_blocks == NULL)
        return NULL;

    old_count = ii->bit_block_count;

    if (((sizeof(unsigned long) * 8) - 1) > ((~(size_t)0) - magnitude))
      {
        basic_error(
                "Out of memory while trying to allocate a number of bytes "
                "beyond the addressable size of the system.");
        return NULL;
      }

    new_count = ((magnitude + ((sizeof(unsigned long) * 8) - 1)) /
                 (sizeof(unsigned long) * 8));

    if (new_count > ((~(size_t)0) - old_count))
      {
        basic_error(
                "Out of memory while trying to allocate a number of bytes "
                "beyond the addressable size of the system.");
        return NULL;
      }

    new_count += old_count;
    assert(new_count > 0);

    new_blocks = MALLOC_ARRAY(unsigned long, new_count);
    if (new_blocks == NULL)
        return NULL;

    for (block_num = 0; block_num < new_count; ++block_num)
        new_blocks[block_num] = 0x0;

    is_negative = ii->is_negative;

    for (bit_num = 0; bit_num < (old_count * sizeof(unsigned long) * 8);
         ++bit_num)
      {
        size_t old_block_num;
        unsigned long the_bit;

        old_block_num = (bit_num / (sizeof(unsigned long) * 8));

        assert(old_block_num < old_count);
        the_bit = ((old_blocks[old_block_num] >>
                    (bit_num % (sizeof(unsigned long) * 8))) & 0x1);

        new_blocks[(bit_num + magnitude) / (sizeof(unsigned long) * 8)] |=
                (the_bit <<
                 ((bit_num + magnitude) % (sizeof(unsigned long) * 8)));
      }

    bit_num += magnitude;
    while (bit_num < (new_count * sizeof(unsigned long) * 8))
      {
        unsigned long the_bit;

        the_bit = (is_negative ? 0x1 : 0x0);

        new_blocks[bit_num / (sizeof(unsigned long) * 8)] |=
                (the_bit << (bit_num % (sizeof(unsigned long) * 8)));

        ++bit_num;
      }

    result = create_empty_ii(IIK_FINITE);
    if (result == NULL)
      {
        free(new_blocks);
        return NULL;
      }

    while ((new_count > 0) &&
           (new_blocks[new_count - 1] ==
            (is_negative ? ~(unsigned long)0 : 0)))
      {
        --new_count;
      }

    result->is_negative = is_negative;
    result->bit_block_count = new_count;
    result->bit_blocks = new_blocks;

    return result;
  }

static i_integer *shift_down(i_integer *ii, size_t magnitude)
  {
    unsigned long *old_blocks;
    size_t old_count;
    size_t new_count;
    unsigned long *new_blocks;
    size_t block_num;
    boolean is_negative;
    size_t bit_num;
    i_integer *result;

    assert(ii != NULL);

    assert(ii->kind == IIK_FINITE);

    if (magnitude == 0)
      {
        ii_add_reference(ii);
        return ii;
      }

    old_blocks = force_bit_blocks(ii);
    if (old_blocks == NULL)
        return NULL;

    old_count = ii->bit_block_count;

    if ((old_count * sizeof(unsigned long) * 8) < magnitude)
      {
        if (ii->is_negative)
            return ii_minus_one();
        else
            return ii_zero();
      }

    new_count = old_count - (magnitude / (sizeof(unsigned long) * 8));

    new_blocks =
            MALLOC_ARRAY(unsigned long, ((new_count == 0) ? 1 : new_count));
    if (new_blocks == NULL)
        return NULL;

    for (block_num = 0; block_num < new_count; ++block_num)
        new_blocks[block_num] = 0x0;

    is_negative = ii->is_negative;

    for (bit_num = 0; bit_num < (new_count * sizeof(unsigned long) * 8);
         ++bit_num)
      {
        size_t old_block_num;
        unsigned long the_bit;

        old_block_num = (bit_num + magnitude) / (sizeof(unsigned long) * 8);

        if (old_block_num < old_count)
          {
            the_bit = ((old_blocks[old_block_num] >>
                        ((bit_num + magnitude) %
                         (sizeof(unsigned long) * 8))) & 0x1);
          }
        else
          {
            the_bit = (is_negative ? 0x1 : 0x0);
          }

        new_blocks[bit_num / (sizeof(unsigned long) * 8)] |=
                (the_bit << (bit_num % (sizeof(unsigned long) * 8)));
      }

    result = create_empty_ii(IIK_FINITE);
    if (result == NULL)
      {
        free(new_blocks);
        return NULL;
      }

    while ((new_count > 0) &&
           (new_blocks[new_count - 1] ==
            (is_negative ? ~(unsigned long)0 : 0)))
      {
        --new_count;
      }

    result->is_negative = is_negative;
    result->bit_block_count = new_count;
    result->bit_blocks = new_blocks;

    return result;
  }

static size_t ii_to_size_t(i_integer *ii)
  {
    assert(ii != NULL);

    assert(ii->kind == IIK_FINITE);

    if (ii->decimal_digits != NULL)
      {
        size_t result;
        char *digits;
        size_t count;
        size_t digit_num;

        result = 0;
        digits = ii->decimal_digits;

        count = ii->decimal_digit_count;
        for (digit_num = 0; digit_num < count; ++digit_num)
          {
            char digit;

            digit = digits[count - (digit_num + 1)];
            assert((digit >= DECIMAL_STORAGE_ZERO_CHAR) &&
                   (digit <= DECIMAL_STORAGE_ZERO_CHAR + 9));

            if (result >
                (((~(size_t)0) - (digit - DECIMAL_STORAGE_ZERO_CHAR)) / 10))
              {
                return ~(size_t)0;
              }
            result = (result * 10) + (digit - DECIMAL_STORAGE_ZERO_CHAR);
          }

        return result;
      }
    else
      {
        unsigned long *blocks;
        size_t result;
        size_t count;
        size_t block_num;

        blocks = ii->bit_blocks;
        assert(blocks != NULL);

        result = 0;

        count = ii->bit_block_count;
        for (block_num = 0; block_num < count; ++block_num)
          {
            unsigned long bits;
            size_t mask;

            bits = blocks[count - (block_num + 1)];
            if (ii->is_negative)
                bits = ~bits;

            if ((sizeof(unsigned long) > sizeof(size_t)) &&
                (bits > (unsigned long)~(size_t)0))
              {
                return ~(size_t)0;
              }

            mask = ~(safe_shift_right_size_t(~(size_t)0,
                                             (sizeof(unsigned long) * 8)));

            if ((result & mask) != 0)
                return ~(size_t)0;
            result = safe_shift_left_size_t(result,
                                            (sizeof(unsigned long) * 8));

            result |= bits;
          }

        if ((ii->is_negative) && (result < ~(size_t)0))
            ++result;

        return result;
      }
  }

static void add_decimal_magnitudes(char *digits1, char *digits2, size_t count1,
        size_t count2, char *result_digits, size_t *result_count)
  {
    size_t output_count;
    size_t carry;
    size_t digit_num;

    assert((digits1 != NULL) || (count1 == 0));
    assert((digits2 != NULL) || (count2 == 0));
    assert(result_digits != NULL);
    assert(result_count != NULL);

    if (count1 > count2)
        output_count = (count1 + 1);
    else
        output_count = (count2 + 1);

    carry = 0;

    for (digit_num = 0; digit_num < output_count; ++digit_num)
      {
        size_t digit1;
        size_t digit2;

        digit1 = ((digit_num < count1) ? digits1[digit_num] :
                                         DECIMAL_STORAGE_ZERO_CHAR);
        digit2 = ((digit_num < count2) ? digits2[digit_num] :
                                         DECIMAL_STORAGE_ZERO_CHAR);

        assert((digit1 >= DECIMAL_STORAGE_ZERO_CHAR) &&
               (digit1 <= DECIMAL_STORAGE_ZERO_CHAR + 9));
        assert((digit2 >= DECIMAL_STORAGE_ZERO_CHAR) &&
               (digit2 <= DECIMAL_STORAGE_ZERO_CHAR + 9));

        carry += ((digit1 - DECIMAL_STORAGE_ZERO_CHAR) +
                  (digit2 - DECIMAL_STORAGE_ZERO_CHAR));
        assert(carry <= 19);

        if (((digit_num + 1) < output_count) || (carry != 0))
          {
            result_digits[digit_num] =
                    ((carry % 10) + DECIMAL_STORAGE_ZERO_CHAR);
          }
        else
          {
            --output_count;
            break;
          }
        carry /= 10;
      }

    assert(carry == 0);

    while ((output_count > 0) &&
           (result_digits[output_count - 1] == DECIMAL_STORAGE_ZERO_CHAR))
      {
        --output_count;
      }

    *result_count = output_count;
  }

static char *add_decimal_magnitudes_with_allocation(char *digits1,
        char *digits2, size_t count1, size_t count2, size_t *result_count)
  {
    char *result;

    assert(digits1 != NULL);
    assert(digits2 != NULL);
    assert(result_count != NULL);

    result = MALLOC_ARRAY(char, (((count1 > count2) ? count1 : count2) + 1));
    if (result == NULL)
        return NULL;

    add_decimal_magnitudes(digits1, digits2, count1, count2, result,
                           result_count);

    return result;
  }

static void subtract_decimal_magnitudes(char *digits1, char *digits2,
        size_t count1, size_t count2, char *result_digits,
        size_t *result_count)
  {
    size_t carry;
    size_t digit_num;
    size_t output_count;

    assert((digits2 != NULL) || (count2 == 0));

    if (count1 == 0)
      {
        for (digit_num = 0; digit_num < count2; ++digit_num)
            assert(digits2[digit_num] == DECIMAL_STORAGE_ZERO_CHAR);
        *result_count = 0;
        return;
      }

    assert(digits1 != NULL);
    assert(result_digits != NULL);
    assert(result_count != NULL);

    carry = 0;

    for (digit_num = 0; digit_num < count1; ++digit_num)
      {
        size_t digit1;
        size_t digit2;

        digit1 = digits1[digit_num];
        digit2 = ((digit_num < count2) ? digits2[digit_num] :
                                         DECIMAL_STORAGE_ZERO_CHAR);

        assert((digit1 >= DECIMAL_STORAGE_ZERO_CHAR) &&
               (digit1 <= DECIMAL_STORAGE_ZERO_CHAR + 9));
        assert((digit2 >= DECIMAL_STORAGE_ZERO_CHAR) &&
               (digit2 <= DECIMAL_STORAGE_ZERO_CHAR + 9));

        carry += (digit2 - DECIMAL_STORAGE_ZERO_CHAR);
        assert(carry <= 10);

        if (carry > (digit1 - DECIMAL_STORAGE_ZERO_CHAR))
          {
            result_digits[digit_num] =
                    ((digit1 - DECIMAL_STORAGE_ZERO_CHAR) + (10 - carry)) +
                    DECIMAL_STORAGE_ZERO_CHAR;
            carry = 1;
          }
        else
          {
            result_digits[digit_num] =
                    (((digit1 - DECIMAL_STORAGE_ZERO_CHAR) - carry) +
                     DECIMAL_STORAGE_ZERO_CHAR);
            carry = 0;
          }
      }

    assert(carry == 0);

    for (; digit_num < count2; ++digit_num)
        assert(digits2[digit_num] == DECIMAL_STORAGE_ZERO_CHAR);

    output_count = count1;

    while ((output_count > 0) &&
           (result_digits[output_count - 1] == DECIMAL_STORAGE_ZERO_CHAR))
      {
        --output_count;
      }

    *result_count = output_count;
  }

static char *subtract_decimal_magnitudes_with_allocation(char *digits1,
        char *digits2, size_t count1, size_t count2, size_t *result_count)
  {
    char *result;

    assert(digits1 != NULL);
    assert(digits2 != NULL);
    assert(result_count != NULL);

    result = MALLOC_ARRAY(char, ((count1 > count2) ? count1 : count2));
    if (result == NULL)
        return NULL;

    subtract_decimal_magnitudes(digits1, digits2, count1, count2, result,
                                result_count);

    return result;
  }

static void zero_decimal_magnitude(char *digits, size_t count)
  {
    size_t num;

    assert(digits != NULL);

    for (num = 0; num < count; ++num)
        digits[num] = DECIMAL_STORAGE_ZERO_CHAR;
  }

static boolean decimal_magnitude_is_less(char *digits1, char *digits2,
        size_t count1, size_t count2, boolean *equal)
  {
    size_t digit_num;

    *equal = FALSE;

    if (count1 != count2)
        return (count1 < count2);

    for (digit_num = 0; digit_num < count1; ++digit_num)
      {
        char digit1;
        char digit2;

        digit1 = digits1[count1 - (digit_num + 1)];
        digit2 = digits2[count1 - (digit_num + 1)];
        if (digit1 != digit2)
            return (digit1 < digit2);
      }

    *equal = TRUE;
    return FALSE;
  }

static verdict multiply_decimal_magnitudes(char *digits1, char *digits2,
        size_t count1, size_t count2, char *result_digits)
  {
    assert(digits1 != NULL);
    assert(digits2 != NULL);
    assert(result_digits != NULL);

    if ((count1 > 0) && (count2 > 0) &&
        ((count1 > TOOM_2_DECIMAL_CUTOFF_DIGITS) &&
         (count2 > TOOM_2_DECIMAL_CUTOFF_DIGITS)))
      {
        return multiply_decimal_magnitudes_toom_2(digits1, digits2, count1,
                                                  count2, result_digits);
      }
    else
      {
        multiply_decimal_magnitudes_simple(digits1, digits2, count1, count2,
                                           result_digits);
        return MISSION_ACCOMPLISHED;
      }
  }

static void multiply_decimal_magnitudes_simple(char *digits1, char *digits2,
        size_t count1, size_t count2, char *result_digits)
  {
    size_t result_count;
    size_t digit_num;
    size_t index1;

    result_count = count1 + count2;

    for (digit_num = 0; digit_num < result_count; ++digit_num)
        result_digits[digit_num] = DECIMAL_STORAGE_ZERO_CHAR;

    for (index1 = 0; index1 < count1; ++index1)
      {
        unsigned carry;
        size_t index2;

        carry = 0;
        for (index2 = 0; index2 < count2; ++index2)
          {
            char digit1;
            char digit2;
            char old_result_digit;

            assert((index1 + index2) < result_count);

            digit1 = digits1[index1];
            digit2 = digits2[index2];
            old_result_digit = result_digits[index1 + index2];

            assert((digit1 >= DECIMAL_STORAGE_ZERO_CHAR) &&
                   (digit1 <= DECIMAL_STORAGE_ZERO_CHAR + 9));
            assert((digit2 >= DECIMAL_STORAGE_ZERO_CHAR) &&
                   (digit2 <= DECIMAL_STORAGE_ZERO_CHAR + 9));
            assert((old_result_digit >= DECIMAL_STORAGE_ZERO_CHAR) &&
                   (old_result_digit <= DECIMAL_STORAGE_ZERO_CHAR + 9));

            carry += ((old_result_digit - DECIMAL_STORAGE_ZERO_CHAR) +
                      ((digit1 - DECIMAL_STORAGE_ZERO_CHAR) *
                       (digit2 - DECIMAL_STORAGE_ZERO_CHAR)));
            assert(carry <= 99);

            result_digits[index1 + index2] =
                    (carry % 10) + DECIMAL_STORAGE_ZERO_CHAR;
            carry /= 10;
          }

        while (carry > 0)
          {
            char old_result_digit;

            assert((index1 + index2) < result_count);

            old_result_digit = result_digits[index1 + index2];

            assert((old_result_digit >= DECIMAL_STORAGE_ZERO_CHAR) &&
                   (old_result_digit <= DECIMAL_STORAGE_ZERO_CHAR + 9));

            carry += (old_result_digit - DECIMAL_STORAGE_ZERO_CHAR);
            assert(carry <= 18);

            result_digits[index1 + index2] =
                    (carry % 10) + DECIMAL_STORAGE_ZERO_CHAR;
            carry /= 10;

            ++index2;
          }
      }
  }

#define MULTIPLY_TOOM_2(function_name, unit_type, recurse, add, subtract, \
                        zero) \
static verdict function_name(unit_type *digits1, unit_type *digits2, \
        size_t count1, size_t count2, unit_type *result_digits) \
  { \
    size_t low_count; \
    size_t high1_count; \
    size_t low1_count; \
    size_t high2_count; \
    size_t low2_count; \
    unit_type *mixed1; \
    unit_type *mixed2; \
    size_t mixed1_count; \
    size_t mixed2_count; \
    unit_type *middle; \
    size_t low_result_count; \
    size_t high_result_count; \
    unit_type *high_result; \
    unit_type *low_result; \
    size_t middle_count; \
 \
    assert(digits1 != NULL); \
    assert(digits2 != NULL); \
    assert(count1 > 0); \
    assert(count2 > 0); \
    assert((count1 > 1) || (count2 > 1)); \
    assert(result_digits != NULL); \
 \
    low_count = \
            ((count1 >= count2) ? ((count1 + 1) / 2) : ((count2 + 1) / 2)); \
    high1_count = ((low_count <= count1) ? (count1 - low_count) : 0); \
    low1_count = ((low_count <= count1) ? low_count : count1); \
    high2_count = ((low_count <= count2) ? (count2 - low_count) : 0); \
    low2_count = ((low_count <= count2) ? low_count : count2); \
 \
    assert((high1_count + low1_count) == count1); \
    assert((high2_count + low2_count) == count2); \
    assert((low_count * 2) >= count1); \
    assert((low_count * 2) >= count2); \
    assert(low1_count >= high1_count); \
    assert(low2_count >= high2_count); \
 \
    mixed1 = result_digits; \
    mixed2 = result_digits + low1_count + ((count1 == low1_count) ? 0 : 1); \
 \
    add(digits1, digits1 + low1_count, low1_count, count1 - low1_count, \
        mixed1, &mixed1_count); \
 \
    add(digits2, digits2 + low2_count, low2_count, count2 - low2_count, \
        mixed2, &mixed2_count); \
 \
    low_result_count = (low1_count + low2_count); \
    assert(low_result_count > 0); \
 \
    middle_count = (mixed1_count + mixed2_count); \
    if (middle_count < (low_result_count - low_count)) \
        middle_count = (low_result_count - low_count); \
    middle_count += 2; \
 \
    middle = MALLOC_ARRAY(unit_type, middle_count); \
    if ((mixed1_count > 0) || (mixed2_count > 0)) \
      { \
        recurse(mixed1, mixed2, mixed1_count, mixed2_count, middle); \
      } \
 \
    if ((count1 > low_count) && (count2 > low_count)) \
      { \
        high_result_count = (count1 - low1_count) + (count2 - low2_count); \
        assert(high_result_count > 0); \
        high_result = result_digits + (2 * low_count); \
        recurse(digits1 + low1_count, digits2 + low2_count, \
                count1 - low1_count, count2 - low2_count, high_result); \
      } \
    else \
      { \
        high_result_count = 0; \
      } \
 \
    low_result = result_digits; \
    recurse(digits1, digits2, low1_count, low2_count, low_result); \
 \
    if (high_result_count > 0) \
      { \
        subtract(middle, high_result, mixed1_count + mixed2_count, \
                 high_result_count, middle, &middle_count); \
      } \
    else \
      { \
        middle_count = mixed1_count + mixed2_count; \
      } \
 \
    subtract(middle, low_result, middle_count, low_result_count, middle, \
             &middle_count); \
 \
    assert(low_result_count > low_count); \
    assert(low_count <= count1 + count2); \
 \
    add(low_result + low_count, middle, (low_result_count - low_count), \
        middle_count, middle, &middle_count); \
 \
    if (high_result_count > 0) \
      { \
        if ((low_count + middle_count) <= (low_count * 2)) \
          { \
            if (middle_count + low_count > count1 + count2) \
                middle_count = ((count1 + count2) - low_count); \
            memcpy(result_digits + low_count, middle, \
                   middle_count * sizeof(unit_type)); \
            if ((low_count * 2) >= (count1 + count2)) \
              { \
                if ((low_count + middle_count) < (count1 + count2)) \
                  { \
                    zero(result_digits + (low_count + middle_count), \
                         ((count1 + count2) - (low_count + middle_count))); \
                  } \
              } \
            else \
              { \
                if ((low_count + middle_count) < (low_count * 2)) \
                  { \
                    zero(result_digits + low_count + middle_count, \
                         ((low_count * 2) - (low_count + middle_count))); \
                  } \
                assert(((low_count * 2) + high_result_count) <= \
                       (count1 + count2)); \
 \
                if (((low_count * 2) + high_result_count) < \
                    (count1 + count2)) \
                  { \
                    zero(result_digits + (low_count * 2) + high_result_count, \
                         ((count1 + count2) - \
                          ((low_count * 2) + high_result_count))); \
                  } \
              } \
          } \
        else \
          { \
            size_t top_count; \
 \
            memcpy(result_digits + low_count, middle, \
                   low_count * sizeof(unit_type)); \
 \
            add(high_result, middle + low_count, high_result_count, \
                middle_count - low_count, high_result, &top_count); \
 \
            if (((2 * low_count) + top_count) < (count1 + count2)) \
              { \
                zero(result_digits + ((2 * low_count) + top_count), \
                     ((count1 + count2) - ((2 * low_count) + top_count))); \
              } \
          } \
      } \
    else \
      { \
        assert((low_count + middle_count) <= (count1 + count2)); \
        memcpy(result_digits + low_count, middle, \
               middle_count * sizeof(unit_type)); \
        if ((low_count + middle_count) < (count1 + count2)) \
          { \
            zero(result_digits + low_count + middle_count, \
                 ((count1 + count2) - (low_count + middle_count))); \
          } \
      } \
 \
    free(middle); \
 \
    return MISSION_ACCOMPLISHED; \
  }

MULTIPLY_TOOM_2(multiply_decimal_magnitudes_toom_2, char,
        multiply_decimal_magnitudes, add_decimal_magnitudes,
        subtract_decimal_magnitudes, zero_decimal_magnitude);

static size_t safe_shift_right_size_t(size_t to_shift, size_t amount)
  {
    if (amount < (sizeof(size_t) * 8))
        return (to_shift >> amount);
    else
        return 0;
  }

static size_t safe_shift_left_size_t(size_t to_shift, size_t amount)
  {
    if (amount < (sizeof(size_t) * 8))
        return (to_shift << amount);
    else
        return 0;
  }

static long safe_shift_right_long(long to_shift, size_t amount)
  {
    if (to_shift >= 0)
      {
        if (amount < (sizeof(long) * 8))
            return (to_shift >> amount);
        else
            return ((to_shift < 0) ? -1 : 0);
      }
    else
      {
        size_t bits_left;
        long result;

        if (amount >= (sizeof(long) * 8))
            return ((to_shift < 0) ? -1 : 0);

        bits_left = amount;
        result = to_shift;

        while (bits_left > 0)
          {
            size_t local_bits;
            long mod;

            local_bits = bits_left;
            while ((LONG_MAX >> local_bits) == 0)
                --local_bits;

            mod = (result % (1 << local_bits));
            result = (result / (1 << local_bits));
            if (mod < 0)
                --result;

            bits_left -= local_bits;
          }

        return result;
      }
  }

static long safe_shift_left_long(long to_shift, size_t amount)
  {
    if (amount < (sizeof(long) * 8))
        return (to_shift << amount);
    else
        return 0;
  }
