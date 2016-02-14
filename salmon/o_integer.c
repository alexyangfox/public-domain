/* file "o_integer.c" */

/*
 *  This file contains the implementation of the o_integer module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include "c_foundations/basic.h"
#include "o_integer.h"
#include "i_integer.h"


i_integer *oi_long_marker = NULL;
i_integer *oi_no_memory_marker = NULL;
i_integer *oi_positive_infinity_marker = NULL;
i_integer *oi_negative_infinity_marker = NULL;
i_integer *oi_unsigned_infinity_marker = NULL;
i_integer *oi_zero_zero_marker = NULL;

o_integer oi_positive_infinity;
o_integer oi_negative_infinity;
o_integer oi_unsigned_infinity;
o_integer oi_zero_zero;
o_integer oi_zero;
o_integer oi_one;
o_integer oi_null;


extern verdict init_o_integer_module(void)
  {
    oi_long_marker = ii_create_from_size_t(12);
    if (oi_long_marker == NULL)
        goto long_marker_failed;

    oi_no_memory_marker = ii_create_from_size_t(13);
    if (oi_no_memory_marker == NULL)
        goto no_memory_marker_failed;

    oi_positive_infinity_marker = ii_positive_infinity_no_ref();
    if (oi_positive_infinity_marker == NULL)
        goto other_failed;

    oi_negative_infinity_marker = ii_negative_infinity_no_ref();
    if (oi_negative_infinity_marker == NULL)
        goto other_failed;

    oi_unsigned_infinity_marker = ii_unsigned_infinity_no_ref();
    if (oi_unsigned_infinity_marker == NULL)
        goto other_failed;

    oi_zero_zero_marker = ii_zero_zero_no_ref();
    if (oi_zero_zero_marker == NULL)
        goto other_failed;

    oi_positive_infinity.ii = oi_positive_infinity_marker;
    oi_negative_infinity.ii = oi_negative_infinity_marker;
    oi_unsigned_infinity.ii = oi_unsigned_infinity_marker;
    oi_zero_zero.ii = oi_zero_zero_marker;
    oi_zero.the_long = 0;
    oi_zero.ii = oi_long_marker;
    oi_one.the_long = 1;
    oi_one.ii = oi_long_marker;
    oi_null.ii = oi_no_memory_marker;

    return MISSION_ACCOMPLISHED;

  other_failed:
    oi_positive_infinity_marker = NULL;
    oi_negative_infinity_marker = NULL;
    oi_unsigned_infinity_marker = NULL;
    oi_zero_zero_marker = NULL;
    ii_remove_reference(oi_no_memory_marker);
    oi_no_memory_marker = NULL;
  no_memory_marker_failed:
    ii_remove_reference(oi_long_marker);
    oi_long_marker = NULL;
  long_marker_failed:
    return MISSION_FAILED;
  }

extern void cleanup_o_integer_module(void)
  {
    ii_remove_reference(oi_long_marker);
    ii_remove_reference(oi_no_memory_marker);
    oi_long_marker = NULL;
    oi_no_memory_marker = NULL;
    oi_positive_infinity_marker = NULL;
    oi_negative_infinity_marker = NULL;
    oi_unsigned_infinity_marker = NULL;
    oi_zero_zero_marker = NULL;
  }

extern verdict long_decimal_digit_count(long the_long, size_t *digit_count)
  {
    long remainder;
    size_t count;

    remainder = the_long;
    count = 0;

    if (remainder >= 0)
      {
        while (remainder != 0)
          {
            ++count;
            remainder /= 10;
          }
      }
    else
      {
        while (remainder != 0)
          {
            long mod;

            mod = (remainder % 10);
            remainder /= 10;
            if (mod > 0)
              {
                mod -= 10;
                ++remainder;
              }
            ++count;
          }
      }

    *digit_count = count;
    return MISSION_ACCOMPLISHED;
  }

extern verdict long_write_decimal_digits(long the_long, char *location)
  {
    long remainder;
    char *follow;
    char *follow_back;

    remainder = the_long;
    follow = location;

    if (remainder >= 0)
      {
        while (remainder != 0)
          {
            *follow = (remainder % 10) + '0';
            ++follow;
            remainder /= 10;
          }
      }
    else
      {
        while (remainder != 0)
          {
            long mod;

            mod = (remainder % 10);
            remainder /= 10;
            if (mod > 0)
              {
                mod -= 10;
                ++remainder;
              }
            *follow = -mod + '0';
            ++follow;
          }
      }

    --follow;

    follow_back = location;
    while (follow_back < follow)
      {
        char swap;

        swap = *follow_back;
        *follow_back = *follow;
        *follow = swap;
        ++follow_back;
        --follow;
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict long_hex_digits(long the_long, size_t *digit_count)
  {
    long remainder;
    size_t count;

    remainder = the_long;
    count = 0;

    if (remainder >= 0)
      {
        while (remainder != 0)
          {
            ++count;
            remainder >>= 4;
          }
      }
    else
      {
        ++remainder;
        while (remainder != 0)
          {
            long mod;

            mod = (remainder % 16);
            remainder /= 16;
            if (mod > 0)
              {
                mod -= 16;
                ++remainder;
              }
            ++count;
          }
      }

    *digit_count = count;
    return MISSION_ACCOMPLISHED;
  }

extern verdict long_write_hex_digits(long the_long, char *location,
                                     boolean uppercase)
  {
    long remainder;
    char *follow;
    char *follow_back;

    remainder = the_long;
    follow = location;

    if (remainder >= 0)
      {
        while (remainder != 0)
          {
            long mod;

            mod = (remainder & 0xf);
            if (mod < 10)
                *follow = mod + '0';
            else
                *follow = (mod - 0xa) + (uppercase ? 'A' : 'a');
            ++follow;
            remainder >>= 4;
          }
      }
    else
      {
        ++remainder;
        while (remainder != 0)
          {
            long mod;

            mod = (remainder % 16);
            remainder /= 16;
            if (mod > 0)
              {
                mod -= 16;
                ++remainder;
              }
            if (-5 > mod)
                *follow = (0xf + mod) + '0';
            else
                *follow = (5 + mod) + (uppercase ? 'A' : 'a');
            ++follow;
          }
      }

    --follow;

    follow_back = location;
    while (follow_back < follow)
      {
        char swap;

        swap = *follow_back;
        *follow_back = *follow;
        *follow = swap;
        ++follow_back;
        --follow;
      }

    return MISSION_ACCOMPLISHED;
  }
