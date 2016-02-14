/* file "rational.h" */

/*
 *  This file contains the interface to the rational module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef RATIONAL_H
#define RATIONAL_H

#include "c_foundations/basic.h"


typedef struct rational rational;


#include "o_integer.h"


extern rational *create_rational(o_integer numerator, o_integer denominator);

extern void rational_add_reference(rational *the_rational);
extern void rational_remove_reference(rational *the_rational);

extern o_integer rational_numerator(rational *the_rational);
extern o_integer rational_denominator(rational *the_rational);

extern boolean rational_is_integer(rational *the_rational);

extern boolean rationals_are_equal(rational *rational1, rational *rational2);

extern rational *rational_negate(rational *the_rational);
extern rational *rational_add(rational *rational1, rational *rational2);
extern rational *rational_subtract(rational *rational1, rational *rational2);
extern rational *rational_multiply(rational *rational1, rational *rational2);
extern rational *rational_divide(rational *rational1, rational *rational2);

extern boolean rational_less_than(rational *rational1, rational *rational2,
                                  boolean *error);

extern int rational_structural_order(rational *left, rational *right);

extern void cleanup_rational_module(void);


#endif /* RATIONAL_H */
