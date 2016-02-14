/* file "rational.c" */

/*
 *  This file contains the implementation of the rational module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <assert.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "rational.h"
#include "platform_dependent.h"


struct rational
  {
    o_integer numerator;
    o_integer denominator;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
  };


static o_integer get_gcd(o_integer oi1, o_integer oi2);


extern rational *create_rational(o_integer numerator, o_integer denominator)
  {
    o_integer new_numerator;
    o_integer new_denominator;
    rational *result;

    assert(!(oi_out_of_memory(numerator)));
    assert(!(oi_out_of_memory(denominator)));

    if ((oi_kind(numerator) != IIK_FINITE) ||
        (oi_kind(denominator) != IIK_FINITE) || oi_equal(denominator, oi_zero))
      {
        o_integer remainder;

        oi_divide(new_numerator, numerator, denominator, &remainder);
        if (oi_out_of_memory(new_numerator))
            return NULL;

        assert(!(oi_out_of_memory(remainder)));
        oi_remove_reference(remainder);

        new_denominator = oi_one;
        oi_add_reference(oi_one);
      }
    else
      {
        o_integer gcd;
        o_integer remainder;

        gcd = get_gcd(numerator, denominator);
        if (oi_out_of_memory(gcd))
            return NULL;

        assert(oi_kind(gcd) == IIK_FINITE);

        oi_divide(new_numerator, numerator, gcd, &remainder);
        if (oi_out_of_memory(new_numerator))
          {
            oi_remove_reference(gcd);
            return NULL;
          }

        assert(!(oi_out_of_memory(remainder)));
        assert(oi_equal(remainder, oi_zero));
        oi_remove_reference(remainder);

        oi_divide(new_denominator, denominator, gcd, &remainder);
        if (oi_out_of_memory(new_denominator))
          {
            oi_remove_reference(new_numerator);
            oi_remove_reference(gcd);
            return NULL;
          }

        assert(oi_kind(new_denominator) == IIK_FINITE);

        assert(!(oi_out_of_memory(remainder)));
        assert(oi_equal(remainder, oi_zero));
        oi_remove_reference(remainder);

        oi_remove_reference(gcd);

        if (oi_is_negative(new_denominator))
          {
            o_integer neg_numerator;
            o_integer neg_denominator;

            oi_negate(neg_numerator, new_numerator);
            oi_remove_reference(new_numerator);
            if (oi_out_of_memory(neg_numerator))
              {
                oi_remove_reference(new_denominator);
                return NULL;
              }
            new_numerator = neg_numerator;

            oi_negate(neg_denominator, new_denominator);
            oi_remove_reference(new_denominator);
            if (oi_out_of_memory(neg_denominator))
              {
                oi_remove_reference(new_numerator);
                return NULL;
              }
            new_denominator = neg_denominator;
          }

        if (oi_equal(new_denominator, oi_one))
          {
            oi_remove_reference(new_denominator);
            new_denominator = oi_one;
            oi_add_reference(oi_one);
          }
      }

    result = MALLOC_ONE_OBJECT(rational);
    if (result == NULL)
      {
        oi_remove_reference(new_denominator);
        oi_remove_reference(new_numerator);
        return NULL;
      }

    INITIALIZE_SYSTEM_LOCK(result->reference_lock,
            free(result);
            oi_remove_reference(new_denominator);
            oi_remove_reference(new_numerator);
            return NULL);

    result->numerator = new_numerator;
    result->denominator = new_denominator;
    result->reference_count = 1;

    return result;
  }

extern void rational_add_reference(rational *the_rational)
  {
    assert(the_rational != NULL);

    GRAB_SYSTEM_LOCK(the_rational->reference_lock);
    assert(the_rational->reference_count > 0);
    ++(the_rational->reference_count);
    RELEASE_SYSTEM_LOCK(the_rational->reference_lock);
  }

extern void rational_remove_reference(rational *the_rational)
  {
    size_t new_reference_count;

    assert(the_rational != NULL);

    GRAB_SYSTEM_LOCK(the_rational->reference_lock);
    assert(the_rational->reference_count > 0);
    --(the_rational->reference_count);
    new_reference_count = the_rational->reference_count;
    RELEASE_SYSTEM_LOCK(the_rational->reference_lock);

    if (new_reference_count > 0)
        return;

    oi_remove_reference(the_rational->numerator);
    oi_remove_reference(the_rational->denominator);
    DESTROY_SYSTEM_LOCK(the_rational->reference_lock);
    free(the_rational);
  }

extern o_integer rational_numerator(rational *the_rational)
  {
    assert(the_rational != NULL);

    return the_rational->numerator;
  }

extern o_integer rational_denominator(rational *the_rational)
  {
    assert(the_rational != NULL);

    return the_rational->denominator;
  }

extern boolean rational_is_integer(rational *the_rational)
  {
    assert(the_rational != NULL);

    return oi_equal(the_rational->denominator, oi_one);
  }

extern boolean rationals_are_equal(rational *rational1, rational *rational2)
  {
    assert(rational1 != NULL);
    assert(rational2 != NULL);

    if (rational1 == rational2)
        return TRUE;

    return (oi_equal(rational1->numerator, rational2->numerator) &&
            oi_equal(rational1->denominator, rational2->denominator));
  }

extern rational *rational_negate(rational *the_rational)
  {
    o_integer new_numerator;
    rational *result;

    assert(the_rational != NULL);

    oi_negate(new_numerator, the_rational->numerator);
    if (oi_out_of_memory(new_numerator))
        return NULL;

    result = create_rational(new_numerator, the_rational->denominator);
    oi_remove_reference(new_numerator);
    return result;
  }

extern rational *rational_add(rational *rational1, rational *rational2)
  {
    o_integer numerator1;
    o_integer numerator2;
    o_integer denominator1;
    o_integer denominator2;
    o_integer left;
    o_integer right;
    o_integer new_numerator;
    o_integer new_denominator;
    rational *result;

    assert(rational1 != NULL);
    assert(rational2 != NULL);

    numerator1 = rational1->numerator;
    numerator2 = rational2->numerator;
    denominator1 = rational1->denominator;
    denominator2 = rational2->denominator;

    oi_multiply(left, numerator1, denominator2);
    if (oi_out_of_memory(left))
        return NULL;

    oi_multiply(right, numerator2, denominator1);
    if (oi_out_of_memory(right))
      {
        oi_remove_reference(left);
        return NULL;
      }

    oi_add(new_numerator, left, right);
    oi_remove_reference(left);
    oi_remove_reference(right);
    if (oi_out_of_memory(new_numerator))
        return NULL;

    oi_multiply(new_denominator, denominator1, denominator2);
    if (oi_out_of_memory(new_denominator))
      {
        oi_remove_reference(new_numerator);
        return NULL;
      }

    result = create_rational(new_numerator, new_denominator);
    oi_remove_reference(new_numerator);
    oi_remove_reference(new_denominator);

    return result;
  }

extern rational *rational_subtract(rational *rational1, rational *rational2)
  {
    o_integer numerator1;
    o_integer numerator2;
    o_integer denominator1;
    o_integer denominator2;
    o_integer left;
    o_integer right;
    o_integer new_numerator;
    o_integer new_denominator;
    rational *result;

    assert(rational1 != NULL);
    assert(rational2 != NULL);

    numerator1 = rational1->numerator;
    numerator2 = rational2->numerator;
    denominator1 = rational1->denominator;
    denominator2 = rational2->denominator;

    oi_multiply(left, numerator1, denominator2);
    if (oi_out_of_memory(left))
        return NULL;

    oi_multiply(right, numerator2, denominator1);
    if (oi_out_of_memory(right))
      {
        oi_remove_reference(left);
        return NULL;
      }

    oi_subtract(new_numerator, left, right);
    oi_remove_reference(left);
    oi_remove_reference(right);
    if (oi_out_of_memory(new_numerator))
        return NULL;

    oi_multiply(new_denominator, denominator1, denominator2);
    if (oi_out_of_memory(new_denominator))
      {
        oi_remove_reference(new_numerator);
        return NULL;
      }

    result = create_rational(new_numerator, new_denominator);
    oi_remove_reference(new_numerator);
    oi_remove_reference(new_denominator);

    return result;
  }

extern rational *rational_multiply(rational *rational1, rational *rational2)
  {
    o_integer numerator1;
    o_integer numerator2;
    o_integer denominator1;
    o_integer denominator2;
    o_integer new_numerator;
    o_integer new_denominator;
    rational *result;

    assert(rational1 != NULL);
    assert(rational2 != NULL);

    numerator1 = rational1->numerator;
    numerator2 = rational2->numerator;
    denominator1 = rational1->denominator;
    denominator2 = rational2->denominator;

    oi_multiply(new_numerator, numerator1, numerator2);
    if (oi_out_of_memory(new_numerator))
        return NULL;

    oi_multiply(new_denominator, denominator1, denominator2);
    if (oi_out_of_memory(new_denominator))
      {
        oi_remove_reference(new_numerator);
        return NULL;
      }

    result = create_rational(new_numerator, new_denominator);
    oi_remove_reference(new_numerator);
    oi_remove_reference(new_denominator);

    return result;
  }

extern rational *rational_divide(rational *rational1, rational *rational2)
  {
    o_integer numerator1;
    o_integer numerator2;
    o_integer denominator1;
    o_integer denominator2;
    o_integer new_numerator;
    o_integer new_denominator;
    rational *result;

    assert(rational1 != NULL);
    assert(rational2 != NULL);

    numerator1 = rational1->numerator;
    numerator2 = rational2->numerator;
    denominator1 = rational1->denominator;
    denominator2 = rational2->denominator;

    oi_multiply(new_numerator, numerator1, denominator2);
    if (oi_out_of_memory(new_numerator))
        return NULL;

    oi_multiply(new_denominator, denominator1, numerator2);
    if (oi_out_of_memory(new_denominator))
      {
        oi_remove_reference(new_numerator);
        return NULL;
      }

    result = create_rational(new_numerator, new_denominator);
    oi_remove_reference(new_numerator);
    oi_remove_reference(new_denominator);

    return result;
  }

extern boolean rational_less_than(rational *rational1, rational *rational2,
                                  boolean *error)
  {
    o_integer numerator1;
    o_integer numerator2;
    o_integer denominator1;
    o_integer denominator2;
    o_integer left;
    o_integer right;
    boolean result;

    assert(rational1 != NULL);
    assert(rational2 != NULL);
    assert(error != NULL);

    numerator1 = rational1->numerator;
    numerator2 = rational2->numerator;
    denominator1 = rational1->denominator;
    denominator2 = rational2->denominator;

    oi_multiply(left, numerator1, denominator2);
    if (oi_out_of_memory(left))
      {
        *error = TRUE;
        return FALSE;
      }

    oi_multiply(right, numerator2, denominator1);
    if (oi_out_of_memory(right))
      {
        oi_remove_reference(left);
        *error = TRUE;
        return FALSE;
      }

    result = oi_less_than(left, right);
    oi_remove_reference(left);
    oi_remove_reference(right);

    *error = FALSE;
    return result;
  }

extern int rational_structural_order(rational *left, rational *right)
  {
    o_integer left_numerator;
    o_integer right_numerator;
    o_integer left_denominator;
    o_integer right_denominator;
    o_integer left_oi;
    o_integer right_oi;
    int result;

    assert(left != NULL);
    assert(right != NULL);

    left_numerator = left->numerator;
    right_numerator = right->numerator;
    left_denominator = left->denominator;
    right_denominator = right->denominator;

    oi_multiply(left_oi, left_numerator, right_denominator);
    if (oi_out_of_memory(left_oi))
        return -2;

    oi_multiply(right_oi, right_numerator, left_denominator);
    if (oi_out_of_memory(right_oi))
      {
        oi_remove_reference(left_oi);
        return -2;
      }

    result = oi_structural_order(left_oi, right_oi);
    oi_remove_reference(left_oi);
    oi_remove_reference(right_oi);

    return result;
  }

extern void cleanup_rational_module(void)
  {
  }


static o_integer get_gcd(o_integer oi1, o_integer oi2)
  {
    o_integer dividend;
    o_integer remainder;
    o_integer result;

    assert(!(oi_out_of_memory(oi1)));
    assert(!(oi_out_of_memory(oi2)));
    assert(oi_kind(oi1) == IIK_FINITE);
    assert(oi_kind(oi2) == IIK_FINITE);

    if (oi_is_negative(oi1))
      {
        o_integer negated_oi1;
        o_integer result;

        oi_negate(negated_oi1, oi1);
        if (oi_out_of_memory(negated_oi1))
            return oi_null;

        result = get_gcd(negated_oi1, oi2);
        oi_remove_reference(negated_oi1);
        return result;
      }

    if (oi_is_negative(oi2))
        return get_gcd(oi2, oi1);

    if (oi_less_than(oi1, oi2))
        return get_gcd(oi2, oi1);

    if (oi_equal(oi2, oi_zero))
      {
        oi_add_reference(oi1);
        return oi1;
      }

    oi_divide(dividend, oi1, oi2, &remainder);
    if (oi_out_of_memory(dividend))
        return oi_null;

    assert(oi_kind(remainder) == IIK_FINITE);

    oi_remove_reference(dividend);

    result = get_gcd(oi2, remainder);
    oi_remove_reference(remainder);
    return result;
  }
