/* file "lookup_actual_arguments.h" */

/*
 *  This file contains the interface to the lookup_actual_arguments module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef LOOKUP_ACTUAL_ARGUMENTS_H
#define LOOKUP_ACTUAL_ARGUMENTS_H

#include <stddef.h>
#include "c_foundations/basic.h"


typedef struct lookup_actual_arguments lookup_actual_arguments;


#include "value.h"
#include "source_location.h"
#include "jumper.h"
#include "validator.h"
#include "variable_instance.h"
#include "reference_cluster.h"


extern lookup_actual_arguments *create_lookup_actual_arguments(
        size_t dimensions);

extern void delete_lookup_actual_arguments(lookup_actual_arguments *actuals,
                                           jumper *the_jumper);

extern void lookup_actual_arguments_set_key(lookup_actual_arguments *actuals,
        size_t index, value *key, jumper *the_jumper);
extern void lookup_actual_arguments_set_upper_bound(
        lookup_actual_arguments *actuals, size_t index, value *upper_bound,
        jumper *the_jumper);
extern void lookup_actual_arguments_set_filter(
        lookup_actual_arguments *actuals, size_t index, type *filter,
        jumper *the_jumper);

extern value *do_lookup(value *base_value, lookup_actual_arguments *actuals,
        value *overload_base, const source_location *location,
        jumper *the_jumper);
extern value *set_through_lookup(value *base_value,
        lookup_actual_arguments *actuals, value *overload_base,
        value *(*update_function)(void *update_data, value *existing_value,
                value *new_value, const source_location *the_source_location,
                jumper *the_jumper), void *update_data,
        boolean care_about_existing_value, value *new_value,
        const source_location *location, jumper *the_jumper);
extern void find_lookup_types(type *base_lower_read, type *base_upper_read,
        type *base_lower_write, type *base_upper_write,
        variable_instance *base_base_variable,
        lookup_actual_arguments *actuals, value *overload_base,
        void (*update_function)(void *update_data, type *in_lower_read,
                type *in_upper_read, type *in_lower_write,
                type *in_upper_write, variable_instance *in_base_variable,
                type **out_lower_read, type **out_upper_read,
                type **out_lower_write, type **out_upper_write,
                variable_instance **out_base_variable), void *update_data,
        type **result_lower_read, type **result_upper_read,
        type **result_lower_write, type **result_upper_write,
        variable_instance **result_base_variable,
        const source_location *location, jumper *the_jumper);

extern boolean lookup_actual_arguments_is_valid(
        lookup_actual_arguments *actuals);
extern void check_lookup_actual_arguments_validity(
        lookup_actual_arguments *actuals, const source_location *location,
        jumper *the_jumper);
extern validator *lookup_actual_arguments_validator(
        lookup_actual_arguments *actuals);

extern boolean lookup_actual_arguments_are_equal(
        lookup_actual_arguments *actuals1, lookup_actual_arguments *actuals2,
        boolean *doubt, const source_location *location, jumper *the_jumper);
extern int lookup_actual_arguments_structural_order(
        lookup_actual_arguments *left, lookup_actual_arguments *right);
extern boolean lookup_actual_arguments_are_slippery(
        lookup_actual_arguments *actuals);
extern value *lookup_actual_arguments_difference(lookup_actual_arguments *left,
        lookup_actual_arguments *right, const source_location *location,
        jumper *the_jumper);
extern lookup_actual_arguments *lookup_actual_arguments_add(
        lookup_actual_arguments *actuals, o_integer to_add,
        const source_location *location, jumper *the_jumper);
extern int lookup_actual_arguments_order(lookup_actual_arguments *left,
        lookup_actual_arguments *right, const source_location *location,
        jumper *the_jumper);
extern reference_cluster *lookup_actual_arguments_reference_cluster(
        lookup_actual_arguments *actuals);

extern void print_lookup_actual_arguments(lookup_actual_arguments *actuals,
        void (*text_printer)(void *data, const char *format, ...), void *data,
        void (*value_printer)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data));

DEFINE_EXCEPTION_TAG(pointer_subtraction_dimension_mismatch);
DEFINE_EXCEPTION_TAG(pointer_subtraction_lower_dimension_mismatch);
DEFINE_EXCEPTION_TAG(pointer_subtraction_lower_dimension_doubt);
DEFINE_EXCEPTION_TAG(pointer_subtraction_star_non_star);
DEFINE_EXCEPTION_TAG(pointer_subtraction_star_difference);
DEFINE_EXCEPTION_TAG(pointer_subtraction_range_non_range);
DEFINE_EXCEPTION_TAG(pointer_subtraction_incompatible_ranges);
DEFINE_EXCEPTION_TAG(pointer_subtraction_final_match_doubt);
DEFINE_EXCEPTION_TAG(pointer_subtraction_final_non_integer);
DEFINE_EXCEPTION_TAG(pointer_integer_addition_unsigned_infinity);
DEFINE_EXCEPTION_TAG(pointer_integer_addition_zero_zero);
DEFINE_EXCEPTION_TAG(pointer_integer_addition_star);
DEFINE_EXCEPTION_TAG(pointer_integer_addition_non_integer);
DEFINE_EXCEPTION_TAG(pointer_comparison_dimension_mismatch);
DEFINE_EXCEPTION_TAG(pointer_comparison_lower_dimension_mismatch);
DEFINE_EXCEPTION_TAG(pointer_comparison_lower_dimension_doubt);
DEFINE_EXCEPTION_TAG(pointer_comparison_star_non_star);
DEFINE_EXCEPTION_TAG(pointer_comparison_star_difference);
DEFINE_EXCEPTION_TAG(pointer_comparison_range_non_range);
DEFINE_EXCEPTION_TAG(pointer_comparison_incompatible_ranges);
DEFINE_EXCEPTION_TAG(pointer_comparison_undecidable_order);
DEFINE_EXCEPTION_TAG(pointer_comparison_final_match_doubt);
DEFINE_EXCEPTION_TAG(pointer_comparison_final_non_integer);
DEFINE_EXCEPTION_TAG(lookup_overloaded_range_bad_value);
DEFINE_EXCEPTION_TAG(lookup_overloaded_star_bad_value);
DEFINE_EXCEPTION_TAG(lookup_non_integer_non_map);
DEFINE_EXCEPTION_TAG(lookup_match_indeterminate);
DEFINE_EXCEPTION_TAG(lookup_filter_indeterminate);
DEFINE_EXCEPTION_TAG(lookup_undefined);
DEFINE_EXCEPTION_TAG(lookup_bad_base);
DEFINE_EXCEPTION_TAG(lookup_write_match_indeterminate);
DEFINE_EXCEPTION_TAG(lookup_write_range_not_array);
DEFINE_EXCEPTION_TAG(lookup_write_star_not_array);


#endif /* LOOKUP_ACTUAL_ARGUMENTS_H */
