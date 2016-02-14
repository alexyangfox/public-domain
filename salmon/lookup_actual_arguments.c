/* file "lookup_actual_arguments.c" */

/*
 *  This file contains the implementation of the lookup_actual_arguments
 *  module.
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
#include <stdio.h>
#include <string.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "lookup_actual_arguments.h"
#include "value.h"
#include "source_location.h"
#include "execute.h"
#include "driver.h"
#include "validator.h"
#include "reference_cluster.h"


typedef enum
  {
    DS_OK,
    DS_DIMENSION_MISMATCH,
    DS_ALLOCATION_ERROR,
    DS_LOWER_DIMENSION_MISMATCH,
    DS_LOWER_DIMENSION_DOUBT,
    DS_JUMP_BACK,
    DS_STAR_NON_STAR_DIFFERENCE,
    DS_STAR_DIFFERENCE,
    DS_RANGE_NON_RANGE_DIFFERENCE,
    DS_INCOMPATIBLE_RANGES,
    DS_UNDECIDABLE_ORDER,
    DS_FINAL_DIMENSION_MATCH_DOUBT,
    DS_FINAL_NON_INTEGER
  } difference_status;

typedef struct lookup_item
  {
    value *key;
    value *upper_bound;
    type *filter;
  } lookup_item;

struct lookup_actual_arguments
  {
    size_t dimensions;
    lookup_item *array;
    size_t slippery_count;
    validator *validator;
    reference_cluster *reference_cluster;
  };


static value *do_lookup_from(value *base_value,
        lookup_actual_arguments *actuals, value *overload_base,
        const source_location *location, jumper *the_jumper, size_t start);
static value *do_star_lookup_from(value *base_value,
        lookup_actual_arguments *actuals, value *overload_base, type *filter,
        const source_location *location, jumper *the_jumper, size_t start);
static value *set_through_lookup_from(value *base_value,
        lookup_actual_arguments *actuals, value *overload_base,
        value *(*update_function)(void *update_data, value *existing_value,
                value *new_value, const source_location *the_source_location,
                jumper *the_jumper), void *update_data,
        boolean care_about_existing_value, value *new_value,
        const source_location *location, jumper *the_jumper, size_t start);
static value *set_through_star_lookup_from(value *map_value, value *result,
        lookup_actual_arguments *actuals, value *overload_base, type *filter,
        value *(*update_function)(void *update_data, value *existing_value,
                value *new_value, const source_location *the_source_location,
                jumper *the_jumper), void *update_data,
        boolean care_about_existing_value, value *new_value,
        const source_location *location, jumper *the_jumper, size_t start,
        const char *key_kind_name, boolean is_range);
static void find_lookup_types_from(type *base_lower_read,
        type *base_upper_read, type *base_lower_write, type *base_upper_write,
        variable_instance *base_base_variable,
        lookup_actual_arguments *actuals, value *overload_base,
        void (*update_function)(void *update_data, type *in_lower_read,
                type *in_upper_read, type *in_lower_write,
                type *in_upper_write, variable_instance *in_base_variable,
                type **out_lower_read, type **out_upper_read,
                type **out_lower_write, type **out_upper_write,
                variable_instance **out_base_variable), void *update_data,
        boolean skip_overloading, type **result_lower_read,
        type **result_upper_read, type **result_lower_write,
        type **result_upper_write, variable_instance **result_base_variable,
        size_t start, const source_location *location, jumper *the_jumper);
static difference_status find_actuals_difference(lookup_actual_arguments *left,
        lookup_actual_arguments *right, const source_location *location,
        jumper *the_jumper, o_integer *difference_oi, int *order);
static value *set_element_value(value *base_value, value *key_value,
        type *key_type, value *new_element, value *overload_base,
        const source_location *location, jumper *the_jumper);
static value *try_lookup_overload(value *base_value, value *key_value,
        type *key_type, value *upper_bound, value *overload_base,
        value *new_value, const source_location *location, jumper *the_jumper);
static void lookup_overload_type(type *base_lower, type *base_upper,
        value *key, value *upper_bound, type *filter, value *overload_base,
        boolean is_write, type *write_lower_requirement,
        type *write_upper_requirement, boolean one_element,
        boolean *always_hits, boolean *never_hits, type **lower_result,
        type **upper_result, const source_location *location,
        jumper *the_jumper);
static void force_canonical_form(lookup_actual_arguments *actuals,
                                 size_t index, jumper *the_jumper);
static void shift_and_box_map_item_key(type **new_key_type,
        value **new_key_value, value *map_value, size_t item_number,
        o_integer lower, o_integer upper, o_integer shift_amount,
        const source_location *location, jumper *the_jumper);
static void box_map_item_key(type **new_key_type,
        value **new_key_value, value *map_value, size_t item_number,
        o_integer lower, o_integer upper, const source_location *location,
        jumper *the_jumper);
static void box_map_item_key_exclude_items(type **new_key_type,
        value **new_key_value, value *map_value, size_t item_number,
        o_integer lower, o_integer upper, value *value_with_items,
        const source_location *location, jumper *the_jumper);
static void set_through_lookup_from_through_map(value **result,
        lookup_actual_arguments *actuals, value *overload_base,
        value *(*update_function)(void *update_data, value *existing_value,
                value *new_value, const source_location *the_source_location,
                jumper *the_jumper), void *update_data,
        boolean care_about_existing_value, value *new_value, value **map_value,
        value *map_key_value, type *map_key_type,
        const source_location *location, jumper *the_jumper, size_t start);
static boolean key_is_outside_filter(value *sub_key, type *filter,
        const source_location *location, jumper *the_jumper);


extern lookup_actual_arguments *create_lookup_actual_arguments(
        size_t dimensions)
  {
    lookup_actual_arguments *result;
    size_t number;

    assert(dimensions > 0);

    result = MALLOC_ONE_OBJECT(lookup_actual_arguments);
    if (result == NULL)
        return NULL;

    result->dimensions = dimensions;
    result->array = MALLOC_ARRAY(lookup_item, dimensions);
    if (result->array == NULL)
      {
        free(result);
        return NULL;
      }

    for (number = 0; number < dimensions; ++number)
      {
        result->array[number].key = NULL;
        result->array[number].upper_bound = NULL;
        result->array[number].filter = NULL;
      }

    result->slippery_count = 0;

    result->validator = get_trivial_validator();
    assert(result->validator != NULL);

    result->reference_cluster = NULL;

    return result;
  }

extern void delete_lookup_actual_arguments(lookup_actual_arguments *actuals,
                                           jumper *the_jumper)
  {
    size_t number;

    assert(actuals != NULL);
    assert(actuals->array != NULL);

    if (actuals->validator != NULL)
        validator_remove_reference(actuals->validator);

    for (number = 0; number < actuals->dimensions; ++number)
      {
        if (actuals->array[number].key != NULL)
          {
            value_remove_reference_with_reference_cluster(
                    actuals->array[number].key, the_jumper,
                    actuals->reference_cluster);
          }
        if (actuals->array[number].upper_bound != NULL)
          {
            value_remove_reference(actuals->array[number].upper_bound,
                                   the_jumper);
          }
        if (actuals->array[number].filter != NULL)
          {
            type_remove_reference_with_reference_cluster(
                    actuals->array[number].filter, the_jumper,
                    actuals->reference_cluster);
          }
      }

    free(actuals->array);
    free(actuals);
  }

extern void lookup_actual_arguments_set_key(lookup_actual_arguments *actuals,
        size_t index, value *key, jumper *the_jumper)
  {
    value *old_key;

    assert(actuals != NULL);
    assert(key != NULL);
    assert(the_jumper != NULL);

    assert(index < actuals->dimensions);
    assert(actuals->array != NULL);

    assert(actuals->array[index].filter == NULL);

    if (actuals->reference_cluster == NULL)
        actuals->reference_cluster = value_reference_cluster(key);

    value_add_reference_with_reference_cluster(key,
                                               actuals->reference_cluster);

    old_key = actuals->array[index].key;
    if (old_key != NULL)
      {
        actuals->validator = validator_remove_validator(actuals->validator,
                value_validator(old_key));
        if (actuals->validator == NULL)
            jumper_do_abort(the_jumper);

        if (value_is_slippery(old_key))
          {
            assert(actuals->slippery_count > 0);
            --(actuals->slippery_count);
          }
        value_remove_reference_with_reference_cluster(old_key, the_jumper,
                actuals->reference_cluster);
      }

    if (actuals->validator != NULL)
      {
        actuals->validator = validator_add_validator(actuals->validator,
                                                     value_validator(key));
        if (actuals->validator == NULL)
            jumper_do_abort(the_jumper);
      }

    if (value_is_slippery(key))
        ++(actuals->slippery_count);

    actuals->array[index].key = key;

    force_canonical_form(actuals, index, the_jumper);
  }

extern void lookup_actual_arguments_set_upper_bound(
        lookup_actual_arguments *actuals, size_t index, value *upper_bound,
        jumper *the_jumper)
  {
    value *old_upper_bound;

    assert(actuals != NULL);
    assert(upper_bound != NULL);
    assert(the_jumper != NULL);

    assert(index < actuals->dimensions);
    assert(actuals->array != NULL);

    assert(actuals->array[index].filter == NULL);

    assert(get_value_kind(upper_bound) == VK_INTEGER);
    switch (oi_kind(integer_value_data(upper_bound)))
      {
        case IIK_FINITE:
        case IIK_POSITIVE_INFINITY:
        case IIK_NEGATIVE_INFINITY:
            break;
        case IIK_UNSIGNED_INFINITY:
            assert(FALSE);
        case IIK_ZERO_ZERO:
            assert(FALSE);
        default:
            assert(FALSE);
      }

    if (actuals->array[index].key != NULL)
      {
        assert(get_value_kind(actuals->array[index].key) == VK_INTEGER);
        switch (oi_kind(integer_value_data(actuals->array[index].key)))
          {
            case IIK_FINITE:
            case IIK_POSITIVE_INFINITY:
            case IIK_NEGATIVE_INFINITY:
                break;
            case IIK_UNSIGNED_INFINITY:
                assert(FALSE);
            case IIK_ZERO_ZERO:
                assert(FALSE);
            default:
                assert(FALSE);
          }
      }

    value_add_reference(upper_bound);

    old_upper_bound = actuals->array[index].upper_bound;
    if (old_upper_bound != NULL)
      {
        actuals->validator = validator_remove_validator(actuals->validator,
                value_validator(old_upper_bound));
        if (actuals->validator == NULL)
            jumper_do_abort(the_jumper);

        if (value_is_slippery(old_upper_bound))
          {
            assert(actuals->slippery_count > 0);
            --(actuals->slippery_count);
          }
        value_remove_reference(old_upper_bound, the_jumper);
      }

    if (actuals->validator != NULL)
      {
        actuals->validator = validator_add_validator(actuals->validator,
                value_validator(upper_bound));
        if (actuals->validator == NULL)
            jumper_do_abort(the_jumper);
      }

    if (value_is_slippery(upper_bound))
        ++(actuals->slippery_count);

    actuals->array[index].upper_bound = upper_bound;

    force_canonical_form(actuals, index, the_jumper);
  }

extern void lookup_actual_arguments_set_filter(
        lookup_actual_arguments *actuals, size_t index, type *filter,
        jumper *the_jumper)
  {
    type *old_filter;

    assert(actuals != NULL);
    assert(filter != NULL);
    assert(the_jumper != NULL);

    assert(index < actuals->dimensions);
    assert(actuals->array != NULL);

    assert(actuals->array[index].key == NULL);
    assert(actuals->array[index].upper_bound == NULL);

    if (actuals->reference_cluster == NULL)
        actuals->reference_cluster = type_reference_cluster(filter);

    type_add_reference_with_reference_cluster(filter,
                                              actuals->reference_cluster);

    old_filter = actuals->array[index].filter;
    if (old_filter != NULL)
      {
        actuals->validator = validator_remove_validator(actuals->validator,
                type_validator(old_filter));
        if (actuals->validator == NULL)
            jumper_do_abort(the_jumper);

        if (type_is_slippery(old_filter))
          {
            assert(actuals->slippery_count > 0);
            --(actuals->slippery_count);
          }
        type_remove_reference_with_reference_cluster(old_filter, the_jumper,
                actuals->reference_cluster);
      }

    if (actuals->validator != NULL)
      {
        actuals->validator = validator_add_validator(actuals->validator,
                                                     type_validator(filter));
        if (actuals->validator == NULL)
            jumper_do_abort(the_jumper);
      }

    if (type_is_slippery(filter))
        ++(actuals->slippery_count);

    actuals->array[index].filter = filter;
  }

extern value *do_lookup(value *base_value, lookup_actual_arguments *actuals,
        value *overload_base, const source_location *location,
        jumper *the_jumper)
  {
    assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
    return do_lookup_from(base_value, actuals, overload_base, location,
                          the_jumper, 0);
  }

extern value *set_through_lookup(value *base_value,
        lookup_actual_arguments *actuals, value *overload_base,
        value *(*update_function)(void *update_data, value *existing_value,
                value *new_value, const source_location *the_source_location,
                jumper *the_jumper), void *update_data,
        boolean care_about_existing_value, value *new_value,
        const source_location *location, jumper *the_jumper)
  {
    assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
    return set_through_lookup_from(base_value, actuals, overload_base,
            update_function, update_data, care_about_existing_value, new_value,
            location, the_jumper, 0);
  }

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
        const source_location *location, jumper *the_jumper)
  {
    assert(type_is_valid(base_lower_read)); /* VERIFIED */
    assert(type_is_valid(base_upper_read)); /* VERIFIED */
    assert(type_is_valid(base_lower_write)); /* VERIFIED */
    assert(type_is_valid(base_upper_write)); /* VERIFIED */
    assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */

    find_lookup_types_from(base_lower_read, base_upper_read, base_lower_write,
            base_upper_write, base_base_variable, actuals, overload_base,
            update_function, update_data, FALSE, result_lower_read,
            result_upper_read, result_lower_write, result_upper_write,
            result_base_variable, 0, location, the_jumper);
    assert((result_lower_read == NULL) || (*result_lower_read == NULL) ||
           type_is_valid(*result_lower_read)); /* VERIFIED */
    assert((result_upper_read == NULL) || (*result_upper_read == NULL) ||
           type_is_valid(*result_upper_read)); /* VERIFIED */
    assert((result_lower_write == NULL) || (*result_lower_write == NULL) ||
           type_is_valid(*result_lower_write)); /* VERIFIED */
    assert((result_upper_write == NULL) || (*result_upper_write == NULL) ||
           type_is_valid(*result_upper_write)); /* VERIFIED */
  }

extern boolean lookup_actual_arguments_is_valid(
        lookup_actual_arguments *actuals)
  {
    assert(actuals != NULL);

    return validator_is_valid(actuals->validator);
  }

extern void check_lookup_actual_arguments_validity(
        lookup_actual_arguments *actuals, const source_location *location,
        jumper *the_jumper)
  {
    assert(actuals != NULL);
    assert(the_jumper != NULL);

    validator_check_validity(actuals->validator, location, the_jumper);
  }

extern validator *lookup_actual_arguments_validator(
        lookup_actual_arguments *actuals)
  {
    assert(actuals != NULL);

    return actuals->validator;
  }

extern boolean lookup_actual_arguments_are_equal(
        lookup_actual_arguments *actuals1, lookup_actual_arguments *actuals2,
        boolean *doubt, const source_location *location, jumper *the_jumper)
  {
    size_t dimensions;
    size_t number;
    lookup_item *array1;
    lookup_item *array2;

    assert(actuals1 != NULL);
    assert(actuals2 != NULL);
    assert(doubt != NULL);

    assert(lookup_actual_arguments_is_valid(actuals1)); /* VERIFIED */
    assert(lookup_actual_arguments_is_valid(actuals2)); /* VERIFIED */

    *doubt = FALSE;

    dimensions = actuals1->dimensions;
    if (actuals2->dimensions != dimensions)
        return FALSE;

    array1 = actuals1->array;
    array2 = actuals2->array;

    assert(array1 != NULL);
    assert(array2 != NULL);

    for (number = 0; number < dimensions; ++number)
      {
        value *key1;
        value *key2;
        value *upper_bound1;
        value *upper_bound2;
        type *filter1;
        type *filter2;

        key1 = array1[number].key;
        assert((key1 == NULL) || value_is_valid(key1)); /* VERIFIED */
        key2 = array2[number].key;
        assert((key2 == NULL) || value_is_valid(key2)); /* VERIFIED */
        upper_bound1 = array1[number].upper_bound;
        assert((upper_bound1 == NULL) || value_is_valid(upper_bound1));
                /* VERIFIED */
        upper_bound2 = array2[number].upper_bound;
        assert((upper_bound2 == NULL) || value_is_valid(upper_bound2));
                /* VERIFIED */
        filter1 = array1[number].filter;
        assert((filter1 == NULL) || type_is_valid(filter1)); /* VERIFIED */
        filter2 = array2[number].filter;
        assert((filter2 == NULL) || type_is_valid(filter2)); /* VERIFIED */

        if (key1 != key2)
          {
            boolean local_doubt;
            boolean local_equal;

            if ((key1 == NULL) || (key2 == NULL))
                return FALSE;

            assert(value_is_valid(key1)); /* VERIFIED */
            assert(value_is_valid(key2)); /* VERIFIED */
            local_equal = values_are_equal(key1, key2, &local_doubt, location,
                                           the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return FALSE;
            if (local_doubt)
              {
                *doubt = TRUE;
              }
            else if (!local_equal)
              {
                *doubt = FALSE;
                return FALSE;
              }
          }

        if (upper_bound1 != upper_bound2)
          {
            boolean local_doubt;
            boolean local_equal;

            if ((upper_bound1 == NULL) || (upper_bound2 == NULL))
                return FALSE;

            assert(value_is_valid(upper_bound1)); /* VERIFIED */
            assert(value_is_valid(upper_bound2)); /* VERIFIED */
            local_equal = values_are_equal(upper_bound1, upper_bound2,
                                           &local_doubt, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return FALSE;
            if (local_doubt)
              {
                *doubt = TRUE;
              }
            else if (!local_equal)
              {
                *doubt = FALSE;
                return FALSE;
              }
          }

        if (filter1 != filter2)
          {
            boolean local_doubt;
            boolean local_equal;

            if ((filter1 == NULL) || (filter2 == NULL))
                return FALSE;

            assert(type_is_valid(filter1)); /* VERIFIED */
            assert(type_is_valid(filter2)); /* VERIFIED */
            local_equal = types_are_equal(filter1, filter2, &local_doubt,
                                          location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return FALSE;
            if (local_doubt)
              {
                *doubt = TRUE;
              }
            else if (!local_equal)
              {
                *doubt = FALSE;
                return FALSE;
              }
          }
      }

    return TRUE;
  }

extern int lookup_actual_arguments_structural_order(
        lookup_actual_arguments *left, lookup_actual_arguments *right)
  {
    size_t left_dimensions;
    size_t right_dimensions;
    lookup_item *left_array;
    lookup_item *right_array;
    size_t number;

    assert(left != NULL);
    assert(right != NULL);

    assert(lookup_actual_arguments_is_valid(left)); /* VERIFIED */
    assert(lookup_actual_arguments_is_valid(right)); /* VERIFIED */

    left_dimensions = left->dimensions;
    right_dimensions = right->dimensions;

    left_array = left->array;
    right_array = right->array;

    assert(left_array != NULL);
    assert(right_array != NULL);

    number = 0;

    while (TRUE)
      {
        value *left_key;
        value *right_key;
        value *left_bound;
        value *right_bound;
        type *left_filter;
        type *right_filter;

        if (number >= left_dimensions)
          {
            if (number >= right_dimensions)
                return 0;
            else
                return -1;
          }

        if (number >= right_dimensions)
            return 1;

        left_key = left_array[number].key;
        assert((left_key == NULL) || value_is_valid(left_key)); /* VERIFIED */
        right_key = right_array[number].key;
        assert((right_key == NULL) || value_is_valid(right_key));
                /* VERIFIED */
        left_bound = left_array[number].upper_bound;
        assert((left_bound == NULL) || value_is_valid(left_bound));
                /* VERIFIED */
        right_bound = right_array[number].upper_bound;
        assert((right_bound == NULL) || value_is_valid(right_bound));
                /* VERIFIED */
        left_filter = left_array[number].filter;
        assert((left_filter == NULL) || type_is_valid(left_filter));
                /* VERIFIED */
        right_filter = right_array[number].filter;
        assert((right_filter == NULL) || type_is_valid(right_filter));
                /* VERIFIED */

        if (left_key != right_key)
          {
            int key_order;

            if (left_key == NULL)
                return -1;

            if (right_key == NULL)
                return 1;

            assert(value_is_valid(left_key)); /* VERIFIED */
            assert(value_is_valid(right_key)); /* VERIFIED */
            key_order = value_structural_order(left_key, right_key);
            if (key_order != 0)
                return key_order;
          }

        if (left_bound != right_bound)
          {
            int bound_order;

            if (left_bound == NULL)
                return -1;

            if (right_bound == NULL)
                return 1;

            assert(value_is_valid(left_bound)); /* VERIFIED */
            assert(value_is_valid(right_bound)); /* VERIFIED */
            bound_order = value_structural_order(left_bound, right_bound);
            if (bound_order != 0)
                return bound_order;
          }

        if (left_filter != right_filter)
          {
            int filter_order;

            if (left_filter == NULL)
                return -1;

            if (right_filter == NULL)
                return 1;

            assert(type_is_valid(left_filter)); /* VERIFIED */
            assert(type_is_valid(right_filter)); /* VERIFIED */
            filter_order = type_structural_order(left_filter, right_filter);
            if (filter_order != 0)
                return filter_order;
          }

        ++number;
      }
  }

extern boolean lookup_actual_arguments_are_slippery(
        lookup_actual_arguments *actuals)
  {
    assert(actuals != NULL);

    return (actuals->slippery_count > 0);
  }

extern value *lookup_actual_arguments_difference(lookup_actual_arguments *left,
        lookup_actual_arguments *right, const source_location *location,
        jumper *the_jumper)
  {
    difference_status status;
    o_integer difference_oi;

    assert(left != NULL);
    assert(right != NULL);
    assert(the_jumper != NULL);

    assert(lookup_actual_arguments_is_valid(left)); /* VERIFIED */
    assert(lookup_actual_arguments_is_valid(right)); /* VERIFIED */

    status = find_actuals_difference(left, right, location, the_jumper,
                                     &difference_oi, NULL);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    switch (status)
      {
        case DS_OK:
          {
            value *result;

            assert(!(oi_out_of_memory(difference_oi)));
            result = create_integer_value(difference_oi);
            oi_remove_reference(difference_oi);
            return result;
          }
        case DS_DIMENSION_MISMATCH:
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_subtraction_dimension_mismatch),
                    "When evaluating the subtraction of one pointer from "
                    "another, the two pointers were lookups with different "
                    "numbers of dimensions.");
            return NULL;
          }
        case DS_ALLOCATION_ERROR:
          {
            return NULL;
          }
        case DS_LOWER_DIMENSION_MISMATCH:
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(
                            pointer_subtraction_lower_dimension_mismatch),
                    "When evaluating the subtraction of one pointer from "
                    "another, the two pointers were lookups that differed "
                    "before their final dimensions.");
            return NULL;
          }
        case DS_LOWER_DIMENSION_DOUBT:
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_subtraction_lower_dimension_doubt),
                    "When evaluating the subtraction of one pointer from "
                    "another, the two pointers were lookups for which %s could"
                    " not determine whether they differed before their final "
                    "dimensions.", interpreter_name());
            return NULL;
          }
        case DS_STAR_NON_STAR_DIFFERENCE:
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_subtraction_star_non_star),
                    "When evaluating the subtraction of one pointer from "
                    "another, the two pointers were lookups where one had a "
                    "star in the final dimension while the other did not.");
            return NULL;
          }
        case DS_STAR_DIFFERENCE:
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_subtraction_star_difference),
                    "When evaluating the subtraction of one pointer from "
                    "another, the two pointers were lookups with mis-matching "
                    "filters in the final dimension.");
            return NULL;
          }
        case DS_RANGE_NON_RANGE_DIFFERENCE:
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_subtraction_range_non_range),
                    "When evaluating the subtraction of one pointer from "
                    "another, the two pointers were lookups where one had a "
                    "range in the final dimension while the other did not.");
            return NULL;
          }
        case DS_INCOMPATIBLE_RANGES:
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_subtraction_incompatible_ranges),
                    "When evaluating the subtraction of one pointer from "
                    "another, the two pointers were lookups with incompatible "
                    "ranges in the final dimension.");
            return NULL;
          }
        case DS_UNDECIDABLE_ORDER:
          {
            assert(FALSE);
          }
        case DS_FINAL_DIMENSION_MATCH_DOUBT:
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_subtraction_final_match_doubt),
                    "When evaluating the subtraction of one pointer from "
                    "another, the two pointers were lookups for which %s could"
                    " not determine whether they differed in their final "
                    "dimension.", interpreter_name());
            return NULL;
          }
        case DS_FINAL_NON_INTEGER:
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_subtraction_final_non_integer),
                    "When evaluating the subtraction of one pointer from "
                    "another, the two pointers were lookups that had different"
                    " values in their final dimension that weren't both "
                    "integers.");
            return NULL;
          }
        default:
          {
            assert(FALSE);
            return NULL;
          }
      }
  }

extern lookup_actual_arguments *lookup_actual_arguments_add(
        lookup_actual_arguments *actuals, o_integer to_add,
        const source_location *location, jumper *the_jumper)
  {
    size_t dimensions;
    lookup_item *final_item;
    value *final_key_value;
    o_integer final_key_oi;
    value *final_upper_value;
    o_integer final_upper_oi;
    lookup_actual_arguments *result;
    size_t number;
    o_integer new_key_oi;
    value *new_key_value;

    assert(actuals != NULL);
    assert(!(oi_out_of_memory(to_add)));
    assert(the_jumper != NULL);

    switch (oi_kind(to_add))
      {
        case IIK_FINITE:
        case IIK_POSITIVE_INFINITY:
        case IIK_NEGATIVE_INFINITY:
            break;
        case IIK_UNSIGNED_INFINITY:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_integer_addition_unsigned_infinity),
                    "When evaluating the addition of a pointer and an integer,"
                    " the integer was unsigned infinity.");
            return NULL;
        case IIK_ZERO_ZERO:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_integer_addition_zero_zero),
                    "When evaluating the addition of a pointer and an integer,"
                    " the integer was zero-zero.");
            return NULL;
        default:
            assert(FALSE);
      }

    dimensions = actuals->dimensions;
    assert(dimensions > 0);

    final_item = &(actuals->array[dimensions - 1]);

    final_key_value = final_item->key;

    if (final_key_value == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(pointer_integer_addition_star),
                "When evaluating the addition of a pointer and a non-zero "
                "integer, the pointer was a lookup with a filter as the key "
                "for the final dimension.");
        return NULL;
      }

    if (get_value_kind(final_key_value) != VK_INTEGER)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(pointer_integer_addition_non_integer),
                "When evaluating the addition of a pointer and a non-zero "
                "integer, the pointer was a lookup with a non-integer value as"
                " the key for the final dimension.");
        return NULL;
      }

    final_key_oi = integer_value_data(final_key_value);
    assert(!(oi_out_of_memory(final_key_oi)));

    final_upper_value = final_item->upper_bound;

    if (final_upper_value == NULL)
      {
        final_upper_oi = oi_null;
      }
    else
      {
        assert(get_value_kind(final_upper_value) == VK_INTEGER);

        final_upper_oi = integer_value_data(final_upper_value);
        assert(!(oi_out_of_memory(final_upper_oi)));
      }

    result = create_lookup_actual_arguments(dimensions);
    if (result == NULL)
        return NULL;

    for (number = 0; number < (dimensions - 1); ++number)
      {
        lookup_item *this_item;

        this_item = &(actuals->array[number]);

        if (this_item->key != NULL)
          {
            lookup_actual_arguments_set_key(result, number, this_item->key,
                                            the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                delete_lookup_actual_arguments(result, the_jumper);
                return NULL;
              }
          }

        if (this_item->upper_bound != NULL)
          {
            lookup_actual_arguments_set_upper_bound(result, number,
                    this_item->upper_bound, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                delete_lookup_actual_arguments(result, the_jumper);
                return NULL;
              }
          }
      }

    oi_add(new_key_oi, final_key_oi, to_add);
    if (oi_out_of_memory(new_key_oi))
      {
        jumper_do_abort(the_jumper);
        delete_lookup_actual_arguments(result, the_jumper);
        return NULL;
      }

    new_key_value = create_integer_value(new_key_oi);
    oi_remove_reference(new_key_oi);
    if (new_key_value == NULL)
      {
        jumper_do_abort(the_jumper);
        delete_lookup_actual_arguments(result, the_jumper);
        return NULL;
      }

    lookup_actual_arguments_set_key(result, dimensions - 1, new_key_value,
                                    the_jumper);
    value_remove_reference(new_key_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        delete_lookup_actual_arguments(result, the_jumper);
        return NULL;
      }

    if (!(oi_out_of_memory(final_upper_oi)))
      {
        o_integer new_upper_oi;
        value *new_upper_value;

        oi_add(new_upper_oi, final_upper_oi, to_add);
        if (oi_out_of_memory(new_upper_oi))
          {
            jumper_do_abort(the_jumper);
            delete_lookup_actual_arguments(result, the_jumper);
            return NULL;
          }

        new_upper_value = create_integer_value(new_upper_oi);
        oi_remove_reference(new_upper_oi);
        if (new_upper_value == NULL)
          {
            jumper_do_abort(the_jumper);
            delete_lookup_actual_arguments(result, the_jumper);
            return NULL;
          }

        lookup_actual_arguments_set_upper_bound(result, dimensions - 1,
                                                new_upper_value, the_jumper);
        value_remove_reference(new_upper_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            delete_lookup_actual_arguments(result, the_jumper);
            return NULL;
          }
      }

    return result;
  }

extern int lookup_actual_arguments_order(lookup_actual_arguments *left,
        lookup_actual_arguments *right, const source_location *location,
        jumper *the_jumper)
  {
    difference_status status;
    int order;

    assert(left != NULL);
    assert(right != NULL);
    assert(the_jumper != NULL);

    assert(lookup_actual_arguments_is_valid(left)); /* VERIFIED */
    assert(lookup_actual_arguments_is_valid(right)); /* VERIFIED */

    status = find_actuals_difference(left, right, location, the_jumper, NULL,
                                     &order);
    if (!(jumper_flowing_forward(the_jumper)))
        return 0;

    switch (status)
      {
        case DS_OK:
            return order;
        case DS_DIMENSION_MISMATCH:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_comparison_dimension_mismatch),
                    "When evaluating the order comparison of two pointers, the"
                    " two pointers were lookups with different numbers of "
                    "dimensions.");
            return 0;
        case DS_ALLOCATION_ERROR:
            return 0;
        case DS_LOWER_DIMENSION_MISMATCH:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_comparison_lower_dimension_mismatch),
                    "When evaluating the order comparison of two pointers, the"
                    " two pointers were lookups that differed before their "
                    "final dimensions.");
            return 0;
        case DS_LOWER_DIMENSION_DOUBT:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_comparison_lower_dimension_doubt),
                    "When evaluating the order comparison of two pointers, the"
                    " two pointers were lookups for which %s could not "
                    "determine whether they differed before their final "
                    "dimensions.", interpreter_name());
            return 0;
        case DS_STAR_NON_STAR_DIFFERENCE:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_comparison_star_non_star),
                    "When evaluating the order comparison of two pointers, the"
                    " two pointers were lookups where one had a star in the "
                    "final dimension while the other did not.");
            return 0;
        case DS_STAR_DIFFERENCE:
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_comparison_star_difference),
                    "When evaluating the order comparison of two pointers, the"
                    " two pointers were lookups with mis-matching filters in "
                    "the final dimension.");
            return 0;
          }
        case DS_RANGE_NON_RANGE_DIFFERENCE:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_comparison_range_non_range),
                    "When evaluating the order comparison of two pointers, the"
                    " two pointers were lookups where one had a range in the "
                    "final dimension while the other did not.");
            return 0;
        case DS_INCOMPATIBLE_RANGES:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_comparison_incompatible_ranges),
                    "When evaluating the order comparison of two pointers, the"
                    " two pointers were lookups with incompatible ranges in "
                    "the final dimension.");
            return 0;
        case DS_UNDECIDABLE_ORDER:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_comparison_undecidable_order),
                    "When evaluating the order comparison of two pointers, the"
                    " two pointers were lookups whose order was undecidable.");
            return 0;
        case DS_FINAL_DIMENSION_MATCH_DOUBT:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_comparison_final_match_doubt),
                    "When evaluating the order comparison of two pointers, the"
                    " two pointers were lookups for which %s could not "
                    "determine whether they differed in their final "
                    "dimension.", interpreter_name());
            return 0;
        case DS_FINAL_NON_INTEGER:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(pointer_comparison_final_non_integer),
                    "When evaluating the order comparison of two pointers, the"
                    " two pointers were lookups that had different values in "
                    "their final dimension that weren't both integers.");
            return 0;
        default:
            assert(FALSE);
            return 0;
      }
  }

extern reference_cluster *lookup_actual_arguments_reference_cluster(
        lookup_actual_arguments *actuals)
  {
    assert(actuals != NULL);

    return actuals->reference_cluster;
  }

extern void print_lookup_actual_arguments(lookup_actual_arguments *actuals,
        void (*text_printer)(void *data, const char *format, ...), void *data,
        void (*value_printer)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data))
  {
    size_t dimension_count;
    size_t dimension_num;

    assert(actuals != NULL);
    assert(text_printer != NULL);
    assert(value_printer != NULL);

    assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */

    dimension_count = actuals->dimensions;
    for (dimension_num = 0; dimension_num < dimension_count; ++dimension_num)
      {
        lookup_item *item;

        if (dimension_num > 0)
            (*text_printer)(data, ", ");

        item = &(actuals->array[dimension_num]);

        if ((item->key != NULL) && (item->upper_bound == NULL))
          {
            assert(item->filter == NULL);
            assert(value_is_valid(item->key)); /* VERIFIED */
            (*value_printer)(item->key, text_printer, data);
          }
        else if ((item->key != NULL) && (item->upper_bound != NULL))
          {
            assert(item->filter == NULL);
            assert(value_is_valid(item->key)); /* VERIFIED */
            (*value_printer)(item->key, text_printer, data);
            (*text_printer)(data, "...");
            assert(value_is_valid(item->upper_bound)); /* VERIFIED */
            (*value_printer)(item->upper_bound, text_printer, data);
          }
        else if (item->filter != NULL)
          {
            assert(item->key == NULL);
            assert(item->upper_bound == NULL);
            assert(item->filter != NULL);

            (*text_printer)(data, "* : ");
            assert(type_is_valid(item->filter)); /* VERIFIED */
            print_type_with_override(item->filter, text_printer, data,
                                     value_printer, TEPP_TOP);
          }
        else
          {
            assert(item->key == NULL);
            assert(item->upper_bound == NULL);
            assert(item->filter == NULL);

            (*text_printer)(data, "*");
          }
      }
  }


static value *do_lookup_from(value *base_value,
        lookup_actual_arguments *actuals, value *overload_base,
        const source_location *location, jumper *the_jumper, size_t start)
  {
    lookup_item *the_item;
    value *key;
    value *upper_bound;
    type *filter;
    value *result;

    assert(base_value != NULL);
    assert(actuals != NULL);
    assert(location != NULL);
    assert(the_jumper != NULL);

    assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */

    assert(start <= actuals->dimensions);
    if (start == actuals->dimensions)
      {
        value_add_reference(base_value);
        return base_value;
      }

    assert(actuals->array != NULL);
    the_item = &(actuals->array[start]);

    key = the_item->key;
    assert((key == NULL) || value_is_valid(key)); /* VERIFIED */
    upper_bound = the_item->upper_bound;
    assert((upper_bound == NULL) || value_is_valid(upper_bound));
            /* VERIFIED */
    filter = the_item->filter;
    assert((filter == NULL) || type_is_valid(filter)); /* VERIFIED */

    result = try_lookup_overload(base_value, key, filter, upper_bound,
                                 overload_base, NULL, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(result == NULL);
        return NULL;
      }
    if (result != NULL)
      {
        if ((start + 1) == actuals->dimensions)
            return result;

        if ((key != NULL) && (upper_bound == NULL))
          {
            value *next_result;

            assert(filter == NULL);

            assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
            next_result = do_lookup_from(result, actuals, overload_base,
                                         location, the_jumper, start + 1);
            value_remove_reference(result, the_jumper);
            if ((!(jumper_flowing_forward(the_jumper))) &&
                (next_result != NULL))
              {
                value_remove_reference(next_result, the_jumper);
                return NULL;
              }
            return next_result;
          }
        else
          {
            value *next_result;

            assert(((key != NULL) && (upper_bound != NULL)) ||
                   ((key == NULL) && (upper_bound == NULL)));

            switch (get_value_kind(result))
              {
                case VK_MAP:
                case VK_SEMI_LABELED_VALUE_LIST:
                    break;
                default:
                    location_exception(the_jumper, location,
                            ((key != NULL) ?
                             EXCEPTION_TAG(lookup_overloaded_range_bad_value) :
                             EXCEPTION_TAG(lookup_overloaded_star_bad_value)),
                            "An overloaded lookup of a %s returned something "
                            "other than a map or a semi-labeled value list "
                            "when the lookup had more dimensions.",
                            ((key != NULL) ? "range" : "star"));
                    value_remove_reference(result, the_jumper);
                    return NULL;
              }

            assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
            next_result = do_star_lookup_from(result, actuals, overload_base,
                    NULL, location, the_jumper, start);
            value_remove_reference(result, the_jumper);
            if ((!(jumper_flowing_forward(the_jumper))) &&
                (next_result != NULL))
              {
                value_remove_reference(next_result, the_jumper);
                return NULL;
              }
            return next_result;
          }
      }

    if ((key != NULL) && (upper_bound == NULL))
      {
        value *element;

        assert(key != NULL);
        assert(upper_bound == NULL);
        assert(filter == NULL);

        if (get_value_kind(key) != VK_INTEGER)
          {
            boolean doubt;

            assert(actuals->dimensions == 1);

            if (get_value_kind(base_value) != VK_MAP)
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(lookup_non_integer_non_map),
                        "A lookup was evaluated with a non-integer key on a "
                        "non-map base value.");
                return NULL;
              }

            assert(map_value_all_keys_are_valid(base_value));
                    /* VERIFICATION NEEDED */
            assert(value_is_valid(key)); /* VERIFIED */
            element = map_value_lookup(base_value, key, &doubt, location,
                                       the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return NULL;

            if (doubt)
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(lookup_match_indeterminate),
                        "A lookup was evaluated with a key for which %s could "
                        "not determine whether there was a match or not.",
                        interpreter_name());
                return NULL;
              }
          }
        else
          {
            switch (get_value_kind(base_value))
              {
                case VK_MAP:
                  {
                    boolean doubt;

                    assert(map_value_all_keys_are_valid(base_value));
                            /* VERIFICATION NEEDED */
                    assert(value_is_valid(key)); /* VERIFIED */
                    element = map_value_lookup(base_value, key, &doubt,
                                               location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                        return NULL;

                    if (doubt)
                      {
                        location_exception(the_jumper, location,
                                EXCEPTION_TAG(lookup_match_indeterminate),
                                "A lookup was evaluated with a key for which "
                                "%s could not determine whether there was a "
                                "match or not.", interpreter_name());
                        return NULL;
                      }

                    break;
                  }
                case VK_SEMI_LABELED_VALUE_LIST:
                  {
                    o_integer oi;
                    verdict the_verdict;
                    size_t size_t_key;

                    oi = integer_value_data(key);
                    assert(!(oi_out_of_memory(oi)));

                    if ((oi_kind(oi) != IIK_FINITE) || oi_is_negative(oi))
                      {
                        location_exception(the_jumper, location,
                                EXCEPTION_TAG(lookup_undefined),
                                "A lookup referenced an undefined element.");
                        return NULL;
                      }

                    the_verdict = oi_magnitude_to_size_t(oi, &size_t_key);
                    if ((the_verdict != MISSION_ACCOMPLISHED) ||
                        (size_t_key >= value_component_count(base_value)))
                      {
                        location_exception(the_jumper, location,
                                EXCEPTION_TAG(lookup_undefined),
                                "A lookup referenced an undefined element.");
                        return NULL;
                      }

                    element = value_component_value(base_value, size_t_key);

                    break;
                  }
                default:
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(lookup_bad_base),
                            "A lookup was evaluated with a base value that was"
                            " not a map or a semi-labeled value list.");
                    return NULL;
                  }
              }
          }

        if (element == NULL)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(lookup_undefined),
                    "A lookup referenced an undefined element.");
            return NULL;
          }

        assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
        return do_lookup_from(element, actuals, overload_base, location,
                              the_jumper, start + 1);
      }
    else if (key != NULL)
      {
        o_integer lower_oi;
        o_integer upper_oi;

        assert(key != NULL);
        assert(upper_bound != NULL);
        assert(filter == NULL);

        assert((get_value_kind(key) == VK_INTEGER) &&
               (get_value_kind(upper_bound) == VK_INTEGER));

        lower_oi = integer_value_data(key);
        upper_oi = integer_value_data(upper_bound);

        assert(!(oi_out_of_memory(lower_oi)));
        assert(!(oi_out_of_memory(upper_oi)));

        switch (get_value_kind(base_value))
          {
            case VK_MAP:
              {
                value *map_value;
                size_t count;
                size_t number;

                map_value = create_map_value();
                if (map_value == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    return NULL;
                  }

                count = map_value_item_count(base_value);

                for (number = 0; number < count; ++number)
                  {
                    type *new_key_type;
                    value *new_key_value;
                    value *sub_target;
                    value *sub_element;

                    shift_and_box_map_item_key(&new_key_type, &new_key_value,
                            base_value, number, lower_oi, upper_oi, lower_oi,
                            location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }
                    if ((new_key_type == NULL) && (new_key_value == NULL))
                        continue;

                    sub_target = map_value_item_target(base_value, number);
                    assert(sub_target != NULL);

                    assert(lookup_actual_arguments_is_valid(actuals));
                            /* VERIFIED */
                    sub_element = do_lookup_from(sub_target, actuals,
                            overload_base, location, the_jumper, start + 1);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        assert(sub_element == NULL);
                        if (new_key_type != NULL)
                            type_remove_reference(new_key_type, the_jumper);
                        if (new_key_value != NULL)
                            value_remove_reference(new_key_value, the_jumper);
                        value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }

                    assert(sub_element != NULL);

                    assert(map_value_all_keys_are_valid(map_value));
                            /* VERIFIED */
                    if (new_key_type != NULL)
                      {
                        assert(new_key_value == NULL);
                        assert(map_value_all_keys_are_valid(map_value));
                                /* VERIFIED */
                        assert(type_is_valid(new_key_type)); /* VERIFIED */
                        map_value = map_value_set_filter(map_value,
                                new_key_type, sub_element, location,
                                the_jumper);
                        type_remove_reference(new_key_type, the_jumper);
                      }
                    else
                      {
                        assert(new_key_value != NULL);
                        assert(map_value_all_keys_are_valid(map_value));
                                /* VERIFIED */
                        assert(value_is_valid(new_key_value)); /* VERIFIED */
                        map_value = map_value_set(map_value, new_key_value,
                                sub_element, location, the_jumper);
                        value_remove_reference(new_key_value, the_jumper);
                      }
                    value_remove_reference(sub_element, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        if (map_value != NULL)
                            value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }
                    assert(map_value != NULL);
                  }

                return map_value;
              }
            case VK_SEMI_LABELED_VALUE_LIST:
              {
                size_t size_t_upper;
                size_t size_t_lower;
                value *semi_labeled;
                size_t index;

                if (oi_kind(upper_oi) == IIK_POSITIVE_INFINITY)
                  {
                    size_t_upper = value_component_count(base_value);
                  }
                else if ((oi_kind(upper_oi) == IIK_NEGATIVE_INFINITY) ||
                         oi_is_negative(upper_oi))
                  {
                    size_t_upper = 0;
                  }
                else
                  {
                    verdict the_verdict;

                    assert(oi_kind(upper_oi) == IIK_FINITE);
                    the_verdict =
                            oi_magnitude_to_size_t(upper_oi, &size_t_upper);
                    if ((the_verdict != MISSION_ACCOMPLISHED) ||
                        (size_t_upper >= value_component_count(base_value)))
                      {
                        size_t_upper = value_component_count(base_value);
                      }
                    else
                      {
                        ++size_t_upper;
                      }
                  }

                if (oi_kind(lower_oi) == IIK_POSITIVE_INFINITY)
                  {
                    size_t_lower = size_t_upper;
                  }
                else if ((oi_kind(lower_oi) == IIK_NEGATIVE_INFINITY) ||
                         oi_is_negative(lower_oi))
                  {
                    size_t_lower = 0;
                  }
                else
                  {
                    verdict the_verdict;

                    assert(oi_kind(lower_oi) == IIK_FINITE);
                    the_verdict =
                            oi_magnitude_to_size_t(lower_oi, &size_t_lower);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                        size_t_lower = size_t_upper;
                  }

                semi_labeled = create_semi_labeled_value_list_value();
                if (semi_labeled == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    return NULL;
                  }

                for (index = size_t_lower; index < size_t_upper; ++index)
                  {
                    value *sub_element;
                    verdict the_verdict;

                    sub_element = value_component_value(base_value, index);
                    if (sub_element != NULL)
                      {
                        assert(lookup_actual_arguments_is_valid(actuals));
                                /* VERIFIED */
                        sub_element = do_lookup_from(sub_element, actuals,
                                overload_base, location, the_jumper,
                                start + 1);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            assert(sub_element == NULL);
                            value_remove_reference(semi_labeled, the_jumper);
                            return NULL;
                          }
                        assert(sub_element != NULL);
                      }

                    the_verdict = add_field(semi_labeled, NULL, sub_element);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        jumper_do_abort(the_jumper);
                        if (sub_element != NULL)
                            value_remove_reference(sub_element, the_jumper);
                        value_remove_reference(semi_labeled, the_jumper);
                        return NULL;
                      }

                    if (sub_element != NULL)
                        value_remove_reference(sub_element, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(semi_labeled, the_jumper);
                        return NULL;
                      }
                  }

                return semi_labeled;
              }
            default:
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(lookup_bad_base),
                        "A lookup was evaluated with a base value that was"
                        " not a map or a semi-labeled value list.");
                return NULL;
              }
          }
      }
    else
      {
        assert(key == NULL);
        assert(upper_bound == NULL);

        switch (get_value_kind(base_value))
          {
            case VK_MAP:
            case VK_SEMI_LABELED_VALUE_LIST:
                assert(lookup_actual_arguments_is_valid(actuals));
                        /* VERIFIED */
                assert((filter == NULL) || type_is_valid(filter));
                        /* VERIFIED */
                return do_star_lookup_from(base_value, actuals, overload_base,
                        filter, location, the_jumper, start);
            default:
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(lookup_bad_base),
                        "A lookup was evaluated with a base value that was"
                        " not a map or a semi-labeled value list.");
                return NULL;
          }
      }
  }

static value *do_star_lookup_from(value *base_value,
        lookup_actual_arguments *actuals, value *overload_base, type *filter,
        const source_location *location, jumper *the_jumper, size_t start)
  {
    assert(base_value != NULL);
    assert(actuals != NULL);
    assert(location != NULL);
    assert(the_jumper != NULL);

    assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
    assert((filter == NULL) || type_is_valid(filter)); /* VERIFIED */

    assert(start <= actuals->dimensions);

    switch (get_value_kind(base_value))
      {
        case VK_MAP:
          {
            value *map_value;
            size_t count;
            size_t number;

            map_value = create_map_value();
            if (map_value == NULL)
              {
                jumper_do_abort(the_jumper);
                return NULL;
              }

            assert(map_value_all_keys_are_valid(base_value));
                    /* VERIFICATION NEEDED */

            count = map_value_item_count(base_value);

            for (number = 0; number < count; ++number)
              {
                value *sub_target;
                value *sub_element;

                sub_target = map_value_item_target(base_value, number);
                if (sub_target == NULL)
                    continue;

                assert(lookup_actual_arguments_is_valid(actuals));
                        /* VERIFIED */
                sub_element = do_lookup_from(sub_target, actuals,
                        overload_base, location, the_jumper, start + 1);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    assert(sub_element == NULL);
                    value_remove_reference(map_value, the_jumper);
                    return NULL;
                  }

                assert(sub_element != NULL);

                if (map_value_item_is_type(base_value, number))
                  {
                    type *sub_key;
                    type *filtered_key;

                    sub_key = map_value_item_key_type(base_value, number);
                    if (sub_key == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        value_remove_reference(sub_element, the_jumper);
                        value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }

                    if (filter != NULL)
                      {
                        assert(type_is_valid(sub_key)); /* VERIFIED */
                        assert(type_is_valid(filter)); /* VERIFIED */
                        filtered_key = get_intersection_type(sub_key, filter);
                        if (filtered_key == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            value_remove_reference(sub_element, the_jumper);
                            value_remove_reference(map_value, the_jumper);
                            return NULL;
                          }
                      }
                    else
                      {
                        filtered_key = sub_key;
                      }

                    assert(map_value_all_keys_are_valid(map_value));
                            /* VERIFIED */
                    assert(type_is_valid(filtered_key)); /* VERIFIED */
                    map_value = map_value_set_filter(map_value, filtered_key,
                            sub_element, location, the_jumper);
                    if (filter != NULL)
                        type_remove_reference(filtered_key, the_jumper);
                  }
                else
                  {
                    value *sub_key;
                    boolean skip;

                    sub_key = map_value_item_key_value(base_value, number);
                    if (sub_key == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        value_remove_reference(sub_element, the_jumper);
                        value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }

                    assert(value_is_valid(sub_key)); /* VERIFIED */
                    assert((filter == NULL) || type_is_valid(filter));
                            /* VERIFIED */
                    skip = key_is_outside_filter(sub_key, filter, location,
                                                 the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(sub_element, the_jumper);
                        value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }

                    if (!skip)
                      {
                        assert(map_value_all_keys_are_valid(map_value));
                                /* VERIFIED */
                        assert(value_is_valid(sub_key)); /* VERIFIED */
                        map_value = map_value_set(map_value, sub_key,
                                sub_element, location, the_jumper);
                      }
                  }
                value_remove_reference(sub_element, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    if (map_value != NULL)
                        value_remove_reference(map_value, the_jumper);
                    return NULL;
                  }
              }

            return map_value;
          }
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            boolean result_is_map;
            value *result;
            size_t index;

            if (filter == NULL)
              {
                result_is_map = FALSE;
              }
            else
              {
                size_t base_count;

                base_count = value_component_count(base_value);
                if (base_count == 0)
                  {
                    result_is_map = FALSE;
                  }
                else
                  {
                    o_integer upper;
                    type *range;
                    boolean doubt;
                    boolean is_subset;

                    oi_create_from_size_t(upper, base_count);
                    if (oi_out_of_memory(upper))
                      {
                        jumper_do_abort(the_jumper);
                        return NULL;
                      }

                    range = get_integer_range_type(oi_zero, upper, TRUE,
                                                   FALSE);
                    oi_remove_reference(upper);
                    if (range == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        return NULL;
                      }

                    assert(type_is_valid(range)); /* VERIFIED */
                    assert(type_is_valid(filter)); /* VERIFIED */
                    is_subset = type_is_subset(range, filter, &doubt, NULL,
                                               location, the_jumper);
                    type_remove_reference(range, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                        return NULL;
                    result_is_map = ((!is_subset) || doubt);
                  }
              }

            if (result_is_map)
                result = create_map_value();
            else
                result = create_semi_labeled_value_list_value();
            if (result == NULL)
              {
                jumper_do_abort(the_jumper);
                return NULL;
              }

            for (index = 0; index < value_component_count(base_value); ++index)
              {
                value *sub_element;
                verdict the_verdict;

                sub_element = value_component_value(base_value, index);
                if (sub_element != NULL)
                  {
                    assert(lookup_actual_arguments_is_valid(actuals));
                            /* VERIFIED */
                    sub_element = do_lookup_from(sub_element, actuals,
                            overload_base, location, the_jumper, start + 1);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        assert(sub_element == NULL);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }
                    assert(sub_element != NULL);
                  }

                if (result_is_map)
                  {
                    o_integer sub_oi;
                    value *sub_key;
                    boolean skip;

                    assert(filter != NULL);

                    oi_create_from_size_t(sub_oi, index);
                    if (oi_out_of_memory(sub_oi))
                      {
                        jumper_do_abort(the_jumper);
                        if (sub_element != NULL)
                            value_remove_reference(sub_element, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }

                    sub_key = create_integer_value(sub_oi);
                    oi_remove_reference(sub_oi);
                    if (sub_key == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        if (sub_element != NULL)
                            value_remove_reference(sub_element, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }

                    assert(value_is_valid(sub_key)); /* VERIFIED */
                    assert((filter == NULL) || type_is_valid(filter));
                            /* VERIFIED */
                    skip = key_is_outside_filter(sub_key, filter, location,
                                                 the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(sub_key, the_jumper);
                        if (sub_element != NULL)
                            value_remove_reference(sub_element, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }

                    if (!skip)
                      {
                        assert(map_value_all_keys_are_valid(result));
                                /* VERIFIED */
                        assert(value_is_valid(sub_key)); /* VERIFIED */
                        result = map_value_set(result, sub_key, sub_element,
                                               location, the_jumper);
                      }
                    value_remove_reference(sub_key, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        if (sub_element != NULL)
                            value_remove_reference(sub_element, the_jumper);
                        if (result != NULL)
                            value_remove_reference(result, the_jumper);
                        return NULL;
                      }
                  }
                else
                  {
                    the_verdict = add_field(result, NULL, sub_element);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        jumper_do_abort(the_jumper);
                        if (sub_element != NULL)
                            value_remove_reference(sub_element, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }
                  }

                if (sub_element != NULL)
                    value_remove_reference(sub_element, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    value_remove_reference(result, the_jumper);
                    return NULL;
                  }
              }

            return result;
          }
        default:
          {
            assert(FALSE);
            return NULL;
          }
      }
  }

static value *set_through_lookup_from(value *base_value,
        lookup_actual_arguments *actuals, value *overload_base,
        value *(*update_function)(void *update_data, value *existing_value,
                value *new_value, const source_location *the_source_location,
                jumper *the_jumper), void *update_data,
        boolean care_about_existing_value, value *new_value,
        const source_location *location, jumper *the_jumper, size_t start)
  {
    lookup_item *the_item;
    value *key;
    value *upper_bound;
    type *filter;
    value *map_value;
    value *result;

    assert(actuals != NULL);
    assert(update_function != NULL);
    assert(location != NULL);
    assert(the_jumper != NULL);

    assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */

    assert(start <= actuals->dimensions);
    if (start == actuals->dimensions)
      {
        return (*update_function)(update_data, base_value, new_value, location,
                                  the_jumper);
      }

    assert(actuals->array != NULL);
    the_item = &(actuals->array[start]);

    key = the_item->key;
    assert((key == NULL) || value_is_valid(key)); /* VERIFIED */
    upper_bound = the_item->upper_bound;
    assert((upper_bound == NULL) || value_is_valid(upper_bound));
            /* VERIFIED */
    filter = the_item->filter;
    assert((filter == NULL) || type_is_valid(filter)); /* VERIFIED */

    if ((!care_about_existing_value) && ((start + 1) == actuals->dimensions))
      {
        if (new_value != NULL)
          {
            result = try_lookup_overload(base_value, key, filter, upper_bound,
                    overload_base, new_value, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(result == NULL);
                return NULL;
              }
            if (result != NULL)
                return result;
          }

        if (base_value != NULL)
          {
            value_add_reference(base_value);
            result = base_value;
          }
        else
          {
            result = create_map_value();
            if (result == NULL)
              {
                jumper_do_abort(the_jumper);
                return NULL;
              }
          }

        if ((key != NULL) && (upper_bound == NULL))
          {
            assert(key != NULL);
            assert(upper_bound == NULL);
            assert(filter == NULL);

            assert((get_value_kind(key) == VK_INTEGER) ||
                   (actuals->dimensions == 1));

            if (new_value == NULL)
                return result;

            value_add_reference(new_value);
            assert(value_is_valid(key)); /* VERIFIED */
            result = set_element_value(result, key, NULL, new_value,
                                       overload_base, location, the_jumper);
            if (result == NULL)
                return NULL;
          }
        else if (key != NULL)
          {
            o_integer lower_oi;
            o_integer upper_oi;

            assert(key != NULL);
            assert(upper_bound != NULL);
            assert(filter == NULL);

            assert((get_value_kind(key) == VK_INTEGER) &&
                   (get_value_kind(upper_bound) == VK_INTEGER));

            lower_oi = integer_value_data(key);
            upper_oi = integer_value_data(upper_bound);

            assert(!(oi_out_of_memory(lower_oi)));
            assert(!(oi_out_of_memory(upper_oi)));

            if (new_value == NULL)
              {
                /* empty */
              }
            else switch (get_value_kind(new_value))
              {
                case VK_MAP:
                  {
                    o_integer diff_oi;
                    size_t count;
                    size_t number;

                    oi_subtract(diff_oi, upper_oi, lower_oi);
                    if (oi_out_of_memory(diff_oi))
                      {
                        jumper_do_abort(the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }

                    count = map_value_item_count(new_value);

                    for (number = 0; number < count; ++number)
                      {
                        o_integer shift_amount;
                        type *new_key_type;
                        value *new_key_value;
                        value *sub_target;

                        oi_negate(shift_amount, lower_oi);
                        if (oi_out_of_memory(shift_amount))
                          {
                            jumper_do_abort(the_jumper);
                            oi_remove_reference(diff_oi);
                            value_remove_reference(result, the_jumper);
                            return NULL;
                          }

                        shift_and_box_map_item_key(&new_key_type,
                                &new_key_value, new_value, number, oi_zero,
                                diff_oi, shift_amount, location, the_jumper);
                        oi_remove_reference(shift_amount);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            oi_remove_reference(diff_oi);
                            value_remove_reference(result, the_jumper);
                            return NULL;
                          }
                        if ((new_key_type == NULL) && (new_key_value == NULL))
                            continue;

                        sub_target = map_value_item_target(new_value, number);

                        value_add_reference(sub_target);
                        assert((new_key_value == NULL) ||
                               value_is_valid(new_key_value)); /* VERIFIED */
                        assert((new_key_type == NULL) ||
                               type_is_valid(new_key_type)); /* VERIFIED */
                        result = set_element_value(result, new_key_value,
                                new_key_type, sub_target, overload_base,
                                location, the_jumper);
                        if (result == NULL)
                          {
                            if (new_key_value != NULL)
                              {
                                value_remove_reference(new_key_value,
                                                       the_jumper);
                              }
                            if (new_key_type != NULL)
                              {
                                type_remove_reference(new_key_type,
                                                      the_jumper);
                              }
                            oi_remove_reference(diff_oi);
                            return NULL;
                          }

                        if (new_key_value != NULL)
                            value_remove_reference(new_key_value, the_jumper);
                        if (new_key_type != NULL)
                            type_remove_reference(new_key_type, the_jumper);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            oi_remove_reference(diff_oi);
                            value_remove_reference(result, the_jumper);
                            return NULL;
                          }
                      }

                    oi_remove_reference(diff_oi);

                    break;
                  }
                case VK_SEMI_LABELED_VALUE_LIST:
                  {
                    o_integer diff_oi;
                    size_t size_t_upper;
                    size_t index;

                    oi_subtract(diff_oi, upper_oi, lower_oi);
                    if (oi_out_of_memory(diff_oi))
                      {
                        jumper_do_abort(the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }

                    if (oi_kind(diff_oi) == IIK_POSITIVE_INFINITY)
                      {
                        size_t_upper = value_component_count(new_value);
                      }
                    else if ((oi_kind(diff_oi) == IIK_NEGATIVE_INFINITY) ||
                             oi_is_negative(diff_oi))
                      {
                        size_t_upper = 0;
                      }
                    else
                      {
                        verdict the_verdict;

                        assert(oi_kind(diff_oi) == IIK_FINITE);
                        the_verdict =
                                oi_magnitude_to_size_t(diff_oi, &size_t_upper);
                        ++size_t_upper;
                        if ((the_verdict != MISSION_ACCOMPLISHED) ||
                            (size_t_upper > value_component_count(new_value)))
                          {
                            size_t_upper = value_component_count(new_value);
                          }
                      }

                    oi_remove_reference(diff_oi);

                    for (index = 0; index < size_t_upper; ++index)
                      {
                        value *sub_target;
                        o_integer oi;
                        o_integer oi2;
                        value *sub_key;

                        sub_target = value_component_value(new_value, index);

                        oi_create_from_size_t(oi, index);
                        if (oi_out_of_memory(oi))
                          {
                            jumper_do_abort(the_jumper);
                            value_remove_reference(result, the_jumper);
                            return NULL;
                          }

                        oi_add(oi2, lower_oi, oi);
                        oi_remove_reference(oi);
                        if (oi_out_of_memory(oi2))
                          {
                            jumper_do_abort(the_jumper);
                            value_remove_reference(result, the_jumper);
                            return NULL;
                          }

                        sub_key = create_integer_value(oi2);
                        oi_remove_reference(oi2);
                        if (sub_key == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            value_remove_reference(result, the_jumper);
                            return NULL;
                          }

                        value_add_reference(sub_target);
                        assert(value_is_valid(sub_key)); /* VERIFIED */
                        result = set_element_value(result, sub_key, NULL,
                                sub_target, overload_base, location,
                                the_jumper);
                        if (result == NULL)
                          {
                            value_remove_reference(sub_key, the_jumper);
                            return NULL;
                          }

                        value_remove_reference(sub_key, the_jumper);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            value_remove_reference(result, the_jumper);
                            return NULL;
                          }
                      }

                    break;
                  }
                default:
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(lookup_write_range_not_array),
                            "An assignment through a lookup was evaluated with"
                            " a range, but the value being assigned was not a "
                            "map or a semi-labeled value list.");
                    value_remove_reference(result, the_jumper);
                    return NULL;
                  }
              }
          }
        else
          {
            assert(key == NULL);
            assert(upper_bound == NULL);

            assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
            assert((filter == NULL) || type_is_valid(filter)); /* VERIFIED */
            result = set_through_star_lookup_from(NULL, result, actuals,
                    overload_base, filter, update_function, update_data,
                    care_about_existing_value, new_value, location, the_jumper,
                    start, "star", FALSE);
            if (result == NULL)
                return NULL;
          }

        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            return NULL;
          }
        return result;
      }

    result = try_lookup_overload(base_value, key, filter, upper_bound,
                                 overload_base, NULL, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(result == NULL);
        return NULL;
      }
    if (result != NULL)
      {
        if ((key != NULL) && (upper_bound == NULL))
          {
            value *next_result;

            assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
            next_result = set_through_lookup_from(result, actuals,
                    overload_base, update_function, update_data,
                    care_about_existing_value, new_value, location, the_jumper,
                    start + 1);
            value_remove_reference(result, the_jumper);

            assert(value_is_valid(key)); /* VERIFIED */
            result = set_element_value(base_value, key, NULL, next_result,
                                       overload_base, location, the_jumper);
            return result;
          }
        else
          {
            value *map_value;
            value *next_result;

            switch (get_value_kind(result))
              {
                case VK_MAP:
                    value_add_reference(result);
                    map_value = result;
                    break;
                case VK_SEMI_LABELED_VALUE_LIST:
                    map_value = map_value_from_semi_labeled_value_list(result);
                    break;
                default:
                    location_exception(the_jumper, location,
                            ((key != NULL) ?
                             EXCEPTION_TAG(lookup_overloaded_range_bad_value) :
                             EXCEPTION_TAG(lookup_overloaded_star_bad_value)),
                            "An overloaded lookup of a %s returned something "
                            "other than a map or a semi-labeled value list.",
                            ((key != NULL) ? "range" : "star"));
                    value_remove_reference(result, the_jumper);
                    return NULL;
              }

            if (map_value == NULL)
              {
                jumper_do_abort(the_jumper);
                value_remove_reference(result, the_jumper);
                return NULL;
              }

            assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
            assert((filter == NULL) || type_is_valid(filter)); /* VERIFIED */
            next_result = set_through_star_lookup_from(map_value, result,
                    actuals, NULL, filter, update_function, update_data,
                    care_about_existing_value, new_value, location, the_jumper,
                    start, ((key != NULL) ? "range" : "star"), (key != NULL));
            if (!(jumper_flowing_forward(the_jumper)))
              {
                if (next_result != NULL)
                    value_remove_reference(next_result, the_jumper);
                return NULL;
              }

            assert(next_result != NULL);

            result = try_lookup_overload(base_value, key, filter, upper_bound,
                    overload_base, next_result, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(result == NULL);
                value_remove_reference(next_result, the_jumper);
                return NULL;
              }

            if (result != NULL)
              {
                value_remove_reference(next_result, the_jumper);
                return result;
              }

            if (base_value != NULL)
              {
                value_kind kind;

                kind = get_value_kind(base_value);
                if (kind == VK_MAP)
                  {
                    value_add_reference(base_value);
                    map_value = base_value;
                  }
                else if (kind == VK_SEMI_LABELED_VALUE_LIST)
                  {
                    map_value =
                            map_value_from_semi_labeled_value_list(base_value);
                  }
                else
                  {
                    map_value = create_map_value();
                  }
              }
            else
              {
                map_value = create_map_value();
              }

            if (map_value == NULL)
              {
                jumper_do_abort(the_jumper);
                value_remove_reference(next_result, the_jumper);
                return NULL;
              }

            value_add_reference(map_value);
            result = map_value;

            if (key != NULL)
              {
                o_integer lower_oi;
                o_integer upper_oi;

                assert(upper_bound != NULL);
                assert(filter == NULL);

                assert((get_value_kind(key) == VK_INTEGER) &&
                       (get_value_kind(upper_bound) == VK_INTEGER));

                lower_oi = integer_value_data(key);
                upper_oi = integer_value_data(upper_bound);

                assert(!(oi_out_of_memory(lower_oi)));
                assert(!(oi_out_of_memory(upper_oi)));

                switch (get_value_kind(next_result))
                  {
                    case VK_MAP:
                      {
                        o_integer diff_oi;
                        size_t count;
                        size_t number;

                        count = map_value_item_count(map_value);

                        for (number = 0; number < count; ++number)
                          {
                            type *sub_key_type;
                            value *sub_key_value;
                            value *sub_target;

                            box_map_item_key(&sub_key_type, &sub_key_value,
                                    map_value, number, lower_oi, upper_oi,
                                    location, the_jumper);
                            if (!(jumper_flowing_forward(the_jumper)))
                              {
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(result, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }
                            if ((sub_key_type == NULL) &&
                                (sub_key_value == NULL))
                              {
                                continue;
                              }

                            sub_target =
                                    map_value_item_target(map_value, number);

                            value_add_reference(sub_target);
                            assert((sub_key_type == NULL) ||
                                   type_is_valid(sub_key_type)); /* VERIFIED */
                            assert((sub_key_value == NULL) ||
                                   value_is_valid(sub_key_value));
                                    /* VERIFIED */
                            result = set_element_value(result, sub_key_value,
                                    sub_key_type, sub_target, overload_base,
                                    location, the_jumper);
                            if (result == NULL)
                              {
                                if (sub_key_type != NULL)
                                  {
                                    type_remove_reference(sub_key_type,
                                                          the_jumper);
                                  }
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }

                            if (sub_key_type != NULL)
                              {
                                type_remove_reference(sub_key_type,
                                                      the_jumper);
                                if (!(jumper_flowing_forward(the_jumper)))
                                  {
                                    value_remove_reference(map_value,
                                                           the_jumper);
                                    value_remove_reference(result, the_jumper);
                                    value_remove_reference(next_result,
                                                           the_jumper);
                                    return NULL;
                                  }
                              }
                          }

                        oi_subtract(diff_oi, upper_oi, lower_oi);
                        if (oi_out_of_memory(diff_oi))
                          {
                            jumper_do_abort(the_jumper);
                            value_remove_reference(map_value, the_jumper);
                            value_remove_reference(result, the_jumper);
                            value_remove_reference(next_result, the_jumper);
                            return NULL;
                          }

                        count = map_value_item_count(next_result);

                        for (number = 0; number < count; ++number)
                          {
                            o_integer shift_amount;
                            type *new_key_type;
                            value *new_key_value;
                            value *sub_target;

                            oi_negate(shift_amount, lower_oi);
                            if (oi_out_of_memory(shift_amount))
                              {
                                oi_remove_reference(diff_oi);
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(result, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }

                            shift_and_box_map_item_key(&new_key_type,
                                    &new_key_value, next_result, number,
                                    oi_zero, diff_oi, shift_amount, location,
                                    the_jumper);
                            oi_remove_reference(shift_amount);
                            if (!(jumper_flowing_forward(the_jumper)))
                              {
                                oi_remove_reference(diff_oi);
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(result, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }
                            if ((new_key_type == NULL) &&
                                (new_key_value == NULL))
                              {
                                continue;
                              }

                            sub_target =
                                    map_value_item_target(next_result, number);

                            value_add_reference(sub_target);
                            assert((new_key_type == NULL) ||
                                   type_is_valid(new_key_type)); /* VERIFIED */
                            assert((new_key_value == NULL) ||
                                   value_is_valid(new_key_value));
                                    /* VERIFIED */
                            result = set_element_value(result, new_key_value,
                                    new_key_type, sub_target, overload_base,
                                    location, the_jumper);
                            if (result == NULL)
                              {
                                if (new_key_value != NULL)
                                  {
                                    value_remove_reference(new_key_value,
                                                           the_jumper);
                                  }
                                if (new_key_type != NULL)
                                  {
                                    type_remove_reference(new_key_type,
                                                          the_jumper);
                                  }
                                oi_remove_reference(diff_oi);
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }

                            if ((result != map_value) && (sub_target != NULL))
                              {
                                assert(map_value_all_keys_are_valid(
                                               map_value));
                                        /* VERIFICATION NEEDED */
                                if (new_key_value != NULL)
                                  {
                                    assert(new_key_value != NULL);
                                    assert(new_key_type == NULL);
                                    assert(map_value_all_keys_are_valid(
                                                   map_value)); /* VERIFIED */
                                    assert(value_is_valid(new_key_value));
                                            /* VERIFIED */
                                    map_value = map_value_set(map_value,
                                            new_key_value, sub_target,
                                            location, the_jumper);
                                  }
                                else
                                  {
                                    assert(new_key_value == NULL);
                                    assert(new_key_type != NULL);
                                    assert(map_value_all_keys_are_valid(
                                                   map_value)); /* VERIFIED */
                                    assert(type_is_valid(new_key_type));
                                            /* VERIFIED */
                                    map_value = map_value_set_filter(map_value,
                                            new_key_type, sub_target, location,
                                            the_jumper);
                                  }
                              }

                            if (new_key_value != NULL)
                              {
                                value_remove_reference(new_key_value,
                                                       the_jumper);
                              }
                            if (new_key_type != NULL)
                              {
                                type_remove_reference(new_key_type,
                                                      the_jumper);
                              }
                            if (!(jumper_flowing_forward(the_jumper)))
                              {
                                oi_remove_reference(diff_oi);
                                if (map_value != NULL)
                                  {
                                    value_remove_reference(map_value,
                                                           the_jumper);
                                  }
                                value_remove_reference(result, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }
                            assert(map_value != NULL);
                          }

                        oi_remove_reference(diff_oi);

                        break;
                      }
                    case VK_SEMI_LABELED_VALUE_LIST:
                      {
                        o_integer diff_oi;
                        size_t size_t_upper;
                        size_t index;
                        size_t count;
                        size_t number;

                        oi_subtract(diff_oi, upper_oi, lower_oi);
                        if (oi_out_of_memory(diff_oi))
                          {
                            jumper_do_abort(the_jumper);
                            value_remove_reference(map_value, the_jumper);
                            value_remove_reference(result, the_jumper);
                            value_remove_reference(next_result, the_jumper);
                            return NULL;
                          }

                        if (oi_kind(diff_oi) == IIK_POSITIVE_INFINITY)
                          {
                            size_t_upper = value_component_count(next_result);
                          }
                        else if ((oi_kind(diff_oi) == IIK_NEGATIVE_INFINITY) ||
                                 oi_is_negative(diff_oi))
                          {
                            size_t_upper = 0;
                          }
                        else
                          {
                            verdict the_verdict;

                            assert(oi_kind(diff_oi) == IIK_FINITE);
                            the_verdict = oi_magnitude_to_size_t(diff_oi,
                                    &size_t_upper);
                            ++size_t_upper;
                            if ((the_verdict != MISSION_ACCOMPLISHED) ||
                                (size_t_upper >
                                 value_component_count(next_result)))
                              {
                                size_t_upper =
                                        value_component_count(next_result);
                              }
                          }

                        oi_remove_reference(diff_oi);

                        for (index = 0; index < size_t_upper; ++index)
                          {
                            value *sub_target;
                            o_integer oi1;
                            o_integer oi2;
                            value *sub_key;

                            sub_target =
                                    value_component_value(next_result, index);

                            oi_create_from_size_t(oi1, index);
                            if (oi_out_of_memory(oi1))
                              {
                                jumper_do_abort(the_jumper);
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(result, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }

                            oi_add(oi2, lower_oi, oi1);
                            oi_remove_reference(oi1);
                            if (oi_out_of_memory(oi2))
                              {
                                jumper_do_abort(the_jumper);
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(result, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }

                            sub_key = create_integer_value(oi2);
                            oi_remove_reference(oi2);
                            if (sub_key == NULL)
                              {
                                jumper_do_abort(the_jumper);
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(result, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }

                            value_add_reference(sub_target);
                            assert(value_is_valid(sub_key)); /* VERIFIED */
                            result = set_element_value(result, sub_key, NULL,
                                    sub_target, overload_base, location,
                                    the_jumper);
                            if (result == NULL)
                              {
                                value_remove_reference(sub_key, the_jumper);
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }

                            if ((result != map_value) && (sub_target != NULL))
                              {
                                assert(map_value_all_keys_are_valid(
                                               map_value));
                                        /* VERIFICATION NEEDED */
                                assert(value_is_valid(sub_key)); /* VERIFIED */
                                map_value = map_value_set(map_value, sub_key,
                                        sub_target, location, the_jumper);
                              }

                            value_remove_reference(sub_key, the_jumper);
                            if (!(jumper_flowing_forward(the_jumper)))
                              {
                                if (map_value != NULL)
                                  {
                                    value_remove_reference(map_value,
                                                           the_jumper);
                                  }
                                value_remove_reference(result, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }
                            assert(map_value != NULL);
                          }

                        count = map_value_item_count(map_value);

                        for (number = 0; number < count; ++number)
                          {
                            type *sub_key_type;
                            value *sub_key_value;
                            value *sub_target;

                            box_map_item_key_exclude_items(&sub_key_type,
                                    &sub_key_value, map_value, number,
                                    lower_oi, upper_oi, next_result, location,
                                    the_jumper);
                            if (!(jumper_flowing_forward(the_jumper)))
                              {
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(result, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }
                            if ((sub_key_type == NULL) &&
                                (sub_key_value == NULL))
                              {
                                continue;
                              }

                            sub_target =
                                    map_value_item_target(map_value, number);

                            value_add_reference(sub_target);
                            assert((sub_key_type == NULL) ||
                                   type_is_valid(sub_key_type)); /* VERIFIED */
                            assert((sub_key_value == NULL) ||
                                   value_is_valid(sub_key_value));
                                    /* VERIFIED */
                            result = set_element_value(result, sub_key_value,
                                    sub_key_type, sub_target, overload_base,
                                    location, the_jumper);
                            if (result == NULL)
                              {
                                if (sub_key_type != NULL)
                                  {
                                    type_remove_reference(sub_key_type,
                                                          the_jumper);
                                  }
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }

                            if (sub_key_type != NULL)
                              {
                                type_remove_reference(sub_key_type,
                                                      the_jumper);
                                if (!(jumper_flowing_forward(the_jumper)))
                                  {
                                    value_remove_reference(map_value,
                                                           the_jumper);
                                    value_remove_reference(result, the_jumper);
                                    value_remove_reference(next_result,
                                                           the_jumper);
                                    return NULL;
                                  }
                              }
                          }

                        break;
                      }
                    default:
                      {
                        assert(FALSE);
                        break;
                      }
                  }
              }
            else
              {
                assert(key == NULL);
                assert(upper_bound == NULL);

                switch (get_value_kind(next_result))
                  {
                    case VK_MAP:
                      {
                        size_t count;
                        size_t number;

                        assert(map_value_all_keys_are_valid(map_value));
                                /* VERIFICATION NEEDED */

                        count = map_value_item_count(map_value);

                        for (number = 0; number < count; ++number)
                          {
                            type *sub_key_type;
                            value *sub_key_value;
                            value *sub_target;

                            if (map_value_item_is_type(map_value, number))
                              {
                                sub_key_type = map_value_item_key_type(
                                        map_value, number);
                                sub_key_value = NULL;
                                assert(sub_key_type != NULL);
                                assert(type_is_valid(sub_key_type));
                                        /* VERIFIED */

                                if (filter != NULL)
                                  {
                                    assert(type_is_valid(sub_key_type));
                                            /* VERIFIED */
                                    assert(type_is_valid(filter));
                                            /* VERIFIED */
                                    sub_key_type = get_intersection_type(
                                            sub_key_type, filter);
                                    if (sub_key_type == NULL)
                                      {
                                        jumper_do_abort(the_jumper);
                                        value_remove_reference(result,
                                                               the_jumper);
                                        value_remove_reference(map_value,
                                                               the_jumper);
                                        value_remove_reference(next_result,
                                                               the_jumper);
                                        return NULL;
                                      }
                                  }
                              }
                            else
                              {
                                boolean skip;

                                sub_key_type = NULL;
                                sub_key_value = map_value_item_key_value(
                                        map_value, number);
                                assert(sub_key_value != NULL);
                                assert(value_is_valid(sub_key_value));
                                        /* VERIFIED */

                                assert(value_is_valid(sub_key_value));
                                        /* VERIFIED */
                                assert((filter == NULL) ||
                                       type_is_valid(filter)); /* VERIFIED */
                                skip = key_is_outside_filter(sub_key_value,
                                        filter, location, the_jumper);
                                if (!(jumper_flowing_forward(the_jumper)))
                                  {
                                    value_remove_reference(result, the_jumper);
                                    value_remove_reference(map_value,
                                                           the_jumper);
                                    value_remove_reference(next_result,
                                                           the_jumper);
                                    return NULL;
                                  }
                                if (skip)
                                    continue;
                              }

                            sub_target =
                                    map_value_item_target(map_value, number);

                            value_add_reference(sub_target);
                            assert((sub_key_type == NULL) ||
                                   type_is_valid(sub_key_type)); /* VERIFIED */
                            assert((sub_key_value == NULL) ||
                                   value_is_valid(sub_key_value));
                                    /* VERIFIED */
                            result = set_element_value(result, sub_key_value,
                                    sub_key_type, sub_target, overload_base,
                                    location, the_jumper);
                            if (result == NULL)
                              {
                                if ((sub_key_type != NULL) && (filter != NULL))
                                  {
                                    type_remove_reference(sub_key_type,
                                                          the_jumper);
                                  }
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }

                            if ((sub_key_type != NULL) && (filter != NULL))
                              {
                                type_remove_reference(sub_key_type,
                                                      the_jumper);
                              }
                            if (!(jumper_flowing_forward(the_jumper)))
                              {
                                value_remove_reference(result, the_jumper);
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }
                          }

                        assert(map_value_all_keys_are_valid(next_result));
                                /* VERIFICATION NEEDED */

                        count = map_value_item_count(next_result);

                        for (number = 0; number < count; ++number)
                          {
                            type *sub_key_type;
                            value *sub_key_value;
                            value *sub_target;

                            if (map_value_item_is_type(next_result, number))
                              {
                                sub_key_type = map_value_item_key_type(
                                        next_result, number);
                                sub_key_value = NULL;
                                assert(sub_key_type != NULL);
                                assert(type_is_valid(sub_key_type));
                                        /* VERIFIED */

                                if (filter != NULL)
                                  {
                                    assert(type_is_valid(sub_key_type));
                                            /* VERIFIED */
                                    assert(type_is_valid(filter));
                                            /* VERIFIED */
                                    sub_key_type = get_intersection_type(
                                            sub_key_type, filter);
                                    if (sub_key_type == NULL)
                                      {
                                        jumper_do_abort(the_jumper);
                                        value_remove_reference(result,
                                                               the_jumper);
                                        value_remove_reference(map_value,
                                                               the_jumper);
                                        value_remove_reference(next_result,
                                                               the_jumper);
                                        return NULL;
                                      }
                                  }
                              }
                            else
                              {
                                boolean skip;

                                sub_key_type = NULL;
                                sub_key_value = map_value_item_key_value(
                                        next_result, number);
                                assert(sub_key_value != NULL);
                                assert(value_is_valid(sub_key_value));
                                        /* VERIFIED */

                                assert(value_is_valid(sub_key_value));
                                        /* VERIFIED */
                                assert((filter == NULL) ||
                                       type_is_valid(filter)); /* VERIFIED */
                                skip = key_is_outside_filter(sub_key_value,
                                        filter, location, the_jumper);
                                if (!(jumper_flowing_forward(the_jumper)))
                                  {
                                    value_remove_reference(result, the_jumper);
                                    value_remove_reference(map_value,
                                                           the_jumper);
                                    value_remove_reference(next_result,
                                                           the_jumper);
                                    return NULL;
                                  }
                                if (skip)
                                  {
                                    assert(map_value != NULL);
                                    continue;
                                  }
                              }

                            sub_target =
                                    map_value_item_target(next_result, number);

                            value_add_reference(sub_target);
                            assert((sub_key_type == NULL) ||
                                   type_is_valid(sub_key_type)); /* VERIFIED */
                            assert((sub_key_value == NULL) ||
                                   value_is_valid(sub_key_value));
                                    /* VERIFIED */
                            result = set_element_value(result, sub_key_value,
                                    sub_key_type, sub_target, overload_base,
                                    location, the_jumper);
                            if (result == NULL)
                              {
                                if ((sub_key_type != NULL) && (filter != NULL))
                                  {
                                    type_remove_reference(sub_key_type,
                                                          the_jumper);
                                  }
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }

                            if ((result != map_value) && (sub_target != NULL))
                              {
                                assert(map_value_all_keys_are_valid(
                                               map_value));
                                        /* VERIFICATION NEEDED */
                                if (sub_key_type != NULL)
                                  {
                                    assert(sub_key_value == NULL);
                                    assert(map_value_all_keys_are_valid(
                                                   map_value)); /* VERIFIED */
                                    assert(type_is_valid(sub_key_type));
                                            /* VERIFIED */
                                    map_value = map_value_set_filter(map_value,
                                            sub_key_type, sub_target, location,
                                            the_jumper);
                                  }
                                else
                                  {
                                    assert(sub_key_value != NULL);
                                    assert(map_value_all_keys_are_valid(
                                                   map_value)); /* VERIFIED */
                                    assert(value_is_valid(sub_key_value));
                                            /* VERIFIED */
                                    map_value = map_value_set(map_value,
                                            sub_key_value, sub_target,
                                            location, the_jumper);
                                  }
                              }

                            if ((sub_key_type != NULL) && (filter != NULL))
                              {
                                type_remove_reference(sub_key_type,
                                                      the_jumper);
                              }

                            if (!(jumper_flowing_forward(the_jumper)))
                              {
                                value_remove_reference(result, the_jumper);
                                if (map_value != NULL)
                                  {
                                    value_remove_reference(map_value,
                                                           the_jumper);
                                  }
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }
                            assert(map_value != NULL);
                          }

                        break;
                      }
                    case VK_SEMI_LABELED_VALUE_LIST:
                      {
                        size_t count;
                        size_t index;
                        size_t number;

                        count = value_component_count(next_result);

                        for (index = 0; index < count; ++index)
                          {
                            value *sub_target;
                            o_integer oi;
                            value *sub_key;
                            boolean skip;

                            sub_target =
                                    value_component_value(next_result, index);

                            oi_create_from_size_t(oi, index);
                            if (oi_out_of_memory(oi))
                              {
                                jumper_do_abort(the_jumper);
                                value_remove_reference(result, the_jumper);
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }

                            sub_key = create_integer_value(oi);
                            oi_remove_reference(oi);
                            if (sub_key == NULL)
                              {
                                jumper_do_abort(the_jumper);
                                value_remove_reference(result, the_jumper);
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }

                            assert(value_is_valid(sub_key)); /* VERIFIED */
                            assert((filter == NULL) || type_is_valid(filter));
                                    /* VERIFIED */
                            skip = key_is_outside_filter(sub_key, filter,
                                                         location, the_jumper);
                            if (!(jumper_flowing_forward(the_jumper)))
                              {
                                value_remove_reference(sub_key, the_jumper);
                                value_remove_reference(result, the_jumper);
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }
                            if (skip)
                              {
                                value_remove_reference(sub_key, the_jumper);
                                if (!(jumper_flowing_forward(the_jumper)))
                                  {
                                    value_remove_reference(result, the_jumper);
                                    value_remove_reference(map_value,
                                                           the_jumper);
                                    value_remove_reference(next_result,
                                                           the_jumper);
                                    return NULL;
                                  }
                                assert(map_value != NULL);
                                continue;
                              }

                            value_add_reference(sub_target);
                            assert(value_is_valid(sub_key)); /* VERIFIED */
                            result = set_element_value(result, sub_key, NULL,
                                    sub_target, overload_base, location,
                                    the_jumper);
                            if (result == NULL)
                              {
                                value_remove_reference(sub_key, the_jumper);
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }

                            if ((result != map_value) && (sub_target != NULL))
                              {
                                assert(map_value_all_keys_are_valid(
                                               map_value));
                                        /* VERIFICATION NEEDED */
                                assert(value_is_valid(sub_key)); /* VERIFIED */
                                map_value = map_value_set(map_value, sub_key,
                                        sub_target, location, the_jumper);
                              }

                            value_remove_reference(sub_key, the_jumper);
                            if (!(jumper_flowing_forward(the_jumper)))
                              {
                                value_remove_reference(result, the_jumper);
                                if (map_value != NULL)
                                  {
                                    value_remove_reference(map_value,
                                                           the_jumper);
                                  }
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }
                            assert(map_value != NULL);
                          }

                        assert(map_value_all_keys_are_valid(map_value));
                                /* VERIFICATION NEEDED */

                        count = map_value_item_count(map_value);

                        for (number = 0; number < count; ++number)
                          {
                            type *sub_key_type;
                            value *sub_key_value;
                            value *sub_target;

                            if (map_value_item_is_type(map_value, number))
                              {
                                sub_key_type = map_value_item_key_type(
                                        map_value, number);
                                sub_key_value = NULL;
                                assert(sub_key_type != NULL);
                                assert(type_is_valid(sub_key_type));
                                        /* VERIFIED */
                              }
                            else
                              {
                                boolean skip;

                                sub_key_type = NULL;
                                sub_key_value = map_value_item_key_value(
                                        map_value, number);
                                assert(sub_key_value != NULL);
                                assert(value_is_valid(sub_key_value));
                                        /* VERIFIED */

                                assert(value_is_valid(sub_key_value));
                                        /* VERIFIED */
                                assert((filter == NULL) ||
                                       type_is_valid(filter)); /* VERIFIED */
                                skip = key_is_outside_filter(sub_key_value,
                                        filter, location, the_jumper);
                                if (!(jumper_flowing_forward(the_jumper)))
                                  {
                                    value_remove_reference(result, the_jumper);
                                    value_remove_reference(map_value,
                                                           the_jumper);
                                    value_remove_reference(next_result,
                                                           the_jumper);
                                    return NULL;
                                  }
                                if (skip)
                                    continue;
                              }

                            sub_target =
                                    map_value_item_target(map_value, number);

                            value_add_reference(sub_target);
                            assert((sub_key_type == NULL) ||
                                   type_is_valid(sub_key_type)); /* VERIFIED */
                            assert((sub_key_value == NULL) ||
                                   value_is_valid(sub_key_value));
                                    /* VERIFIED */
                            result = set_element_value(result, sub_key_value,
                                    sub_key_type, sub_target, overload_base,
                                    location, the_jumper);
                            if (result == NULL)
                              {
                                value_remove_reference(map_value, the_jumper);
                                value_remove_reference(next_result,
                                                       the_jumper);
                                return NULL;
                              }
                          }

                        break;
                      }
                    default:
                      {
                        assert(FALSE);
                        break;
                      }
                  }
              }

            value_remove_reference(map_value, the_jumper);
            value_remove_reference(next_result, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(result, the_jumper);
                return NULL;
              }
            return result;
          }
      }

    if (base_value != NULL)
      {
        value_kind kind;

        kind = get_value_kind(base_value);
        if (kind == VK_MAP)
          {
            value_add_reference(base_value);
            map_value = base_value;
          }
        else if (kind == VK_SEMI_LABELED_VALUE_LIST)
          {
            map_value = map_value_from_semi_labeled_value_list(base_value);
          }
        else
          {
            map_value = create_map_value();
          }
      }
    else
      {
        map_value = create_map_value();
      }

    if (map_value == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    value_add_reference(map_value);
    result = map_value;

    if ((key != NULL) && (upper_bound == NULL))
      {
        value *element;
        boolean doubt;

        assert(key != NULL);
        assert(upper_bound == NULL);
        assert(filter == NULL);

        assert((get_value_kind(key) == VK_INTEGER) ||
               (actuals->dimensions == 1));

        assert(map_value_all_keys_are_valid(map_value));
                /* VERIFICATION NEEDED */
        assert(value_is_valid(key)); /* VERIFIED */
        element =
                map_value_lookup(map_value, key, &doubt, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(map_value, the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        if (doubt)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(lookup_write_match_indeterminate),
                    "A write through a lookup expression was evaluated with a "
                    "key for which %s could not determine whether there was an"
                    " existing match or not.", interpreter_name());
            value_remove_reference(map_value, the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
        element = set_through_lookup_from(element, actuals, overload_base,
                update_function, update_data, care_about_existing_value,
                new_value, location, the_jumper, start + 1);

        assert(value_is_valid(key)); /* VERIFIED */
        result = set_element_value(result, key, NULL, element, overload_base,
                                   location, the_jumper);
        if (result == NULL)
          {
            value_remove_reference(map_value, the_jumper);
            return NULL;
          }

        value_remove_reference(map_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            return NULL;
          }
      }
    else if (key != NULL)
      {
        o_integer lower_oi;
        o_integer upper_oi;

        assert(key != NULL);
        assert(upper_bound != NULL);
        assert(filter == NULL);

        assert((get_value_kind(key) == VK_INTEGER) &&
               (get_value_kind(upper_bound) == VK_INTEGER));

        lower_oi = integer_value_data(key);
        upper_oi = integer_value_data(upper_bound);

        assert(!(oi_out_of_memory(lower_oi)));
        assert(!(oi_out_of_memory(upper_oi)));

        if (new_value == NULL)
          {
            size_t count;
            size_t number;

            count = map_value_item_count(map_value);

            for (number = 0; number < count; ++number)
              {
                type *sub_key_type;
                value *sub_key_value;
                value *sub_target;
                value *sub_element;

                box_map_item_key(&sub_key_type, &sub_key_value, map_value,
                        number, lower_oi, upper_oi, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    value_remove_reference(map_value, the_jumper);
                    value_remove_reference(result, the_jumper);
                    return NULL;
                  }
                if ((sub_key_type == NULL) && (sub_key_value == NULL))
                    continue;

                sub_target = map_value_item_target(map_value, number);

                assert(lookup_actual_arguments_is_valid(actuals));
                        /* VERIFIED */
                sub_element = set_through_lookup_from(sub_target, actuals,
                        overload_base, update_function, update_data,
                        care_about_existing_value, NULL, location, the_jumper,
                        start + 1);

                assert((sub_key_type == NULL) || type_is_valid(sub_key_type));
                        /* VERIFIED */
                assert((sub_key_value == NULL) ||
                       value_is_valid(sub_key_value)); /* VERIFIED */
                result = set_element_value(result, sub_key_value, sub_key_type,
                        sub_element, overload_base, location, the_jumper);
                if (result == NULL)
                  {
                    if (sub_key_type != NULL)
                        type_remove_reference(sub_key_type, the_jumper);
                    value_remove_reference(map_value, the_jumper);
                    return NULL;
                  }

                if (sub_key_type != NULL)
                  {
                    type_remove_reference(sub_key_type, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(map_value, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }
                  }
              }
          }
        else switch (get_value_kind(new_value))
          {
            case VK_MAP:
              {
                size_t count;
                size_t number;
                o_integer diff_oi;

                count = map_value_item_count(map_value);

                for (number = 0; number < count; ++number)
                  {
                    type *sub_key_type;
                    value *sub_key_value;
                    value *sub_target;
                    value *sub_element;

                    box_map_item_key(&sub_key_type, &sub_key_value, map_value,
                            number, lower_oi, upper_oi, location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(map_value, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }
                    if ((sub_key_type == NULL) && (sub_key_value == NULL))
                        continue;

                    sub_target = map_value_item_target(map_value, number);

                    assert(lookup_actual_arguments_is_valid(actuals));
                            /* VERIFIED */
                    sub_element = set_through_lookup_from(sub_target, actuals,
                            overload_base, update_function, update_data,
                            care_about_existing_value, NULL, location,
                            the_jumper, start + 1);

                    assert((sub_key_type == NULL) ||
                           type_is_valid(sub_key_type)); /* VERIFIED */
                    assert((sub_key_value == NULL) ||
                           value_is_valid(sub_key_value)); /* VERIFIED */
                    result = set_element_value(result, sub_key_value,
                            sub_key_type, sub_element, overload_base, location,
                            the_jumper);
                    if (result == NULL)
                      {
                        if (sub_key_type != NULL)
                            type_remove_reference(sub_key_type, the_jumper);
                        value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }

                    if (sub_key_type != NULL)
                      {
                        type_remove_reference(sub_key_type, the_jumper);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            value_remove_reference(map_value, the_jumper);
                            value_remove_reference(result, the_jumper);
                            return NULL;
                          }
                      }
                  }

                oi_subtract(diff_oi, upper_oi, lower_oi);
                if (oi_out_of_memory(diff_oi))
                  {
                    jumper_do_abort(the_jumper);
                    value_remove_reference(map_value, the_jumper);
                    value_remove_reference(result, the_jumper);
                    return NULL;
                  }

                count = map_value_item_count(new_value);

                for (number = 0; number < count; ++number)
                  {
                    o_integer shift_amount;
                    type *new_key_type;
                    value *new_key_value;
                    value *sub_target;

                    oi_negate(shift_amount, lower_oi);
                    if (oi_out_of_memory(shift_amount))
                      {
                        jumper_do_abort(the_jumper);
                        oi_remove_reference(diff_oi);
                        value_remove_reference(map_value, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }

                    shift_and_box_map_item_key(&new_key_type, &new_key_value,
                            new_value, number, oi_zero, diff_oi, shift_amount,
                            location, the_jumper);
                    oi_remove_reference(shift_amount);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        oi_remove_reference(diff_oi);
                        value_remove_reference(map_value, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }
                    if ((new_key_type == NULL) && (new_key_value == NULL))
                        continue;

                    sub_target = map_value_item_target(new_value, number);

                    assert(lookup_actual_arguments_is_valid(actuals));
                            /* VERIFIED */
                    assert(map_value_all_keys_are_valid(map_value));
                            /* VERIFICATION NEEDED */
                    assert((new_key_type == NULL) ||
                           type_is_valid(new_key_type)); /* VERIFIED */
                    assert((new_key_value == NULL) ||
                           value_is_valid(new_key_value)); /* VERIFIED */
                    set_through_lookup_from_through_map(&result, actuals,
                            overload_base, update_function, update_data,
                            care_about_existing_value, sub_target, &map_value,
                            new_key_value, new_key_type, location, the_jumper,
                            start + 1);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        if (new_key_type != NULL)
                            type_remove_reference(new_key_type, the_jumper);
                        if (new_key_value != NULL)
                            value_remove_reference(new_key_value, the_jumper);
                        oi_remove_reference(diff_oi);
                        if (map_value != NULL)
                            value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }

                    if (new_key_type != NULL)
                        type_remove_reference(new_key_type, the_jumper);
                    if (new_key_value != NULL)
                        value_remove_reference(new_key_value, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        oi_remove_reference(diff_oi);
                        value_remove_reference(map_value, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }
                  }

                oi_remove_reference(diff_oi);

                break;
              }
            case VK_SEMI_LABELED_VALUE_LIST:
              {
                o_integer diff_oi;
                size_t size_t_upper;
                size_t index;
                size_t count;
                size_t number;

                oi_subtract(diff_oi, upper_oi, lower_oi);
                if (oi_out_of_memory(diff_oi))
                  {
                    jumper_do_abort(the_jumper);
                    value_remove_reference(map_value, the_jumper);
                    value_remove_reference(result, the_jumper);
                    return NULL;
                  }

                if (oi_kind(diff_oi) == IIK_POSITIVE_INFINITY)
                  {
                    size_t_upper = value_component_count(new_value);
                  }
                else if ((oi_kind(diff_oi) == IIK_NEGATIVE_INFINITY) ||
                         oi_is_negative(diff_oi))
                  {
                    size_t_upper = 0;
                  }
                else
                  {
                    verdict the_verdict;

                    assert(oi_kind(diff_oi) == IIK_FINITE);
                    the_verdict =
                            oi_magnitude_to_size_t(diff_oi, &size_t_upper);
                    ++size_t_upper;
                    if ((the_verdict != MISSION_ACCOMPLISHED) ||
                        (size_t_upper > value_component_count(new_value)))
                      {
                        size_t_upper = value_component_count(new_value);
                      }
                  }

                oi_remove_reference(diff_oi);

                for (index = 0; index < size_t_upper; ++index)
                  {
                    value *sub_target;
                    o_integer oi1;
                    o_integer oi2;
                    value *sub_key;
                    boolean doubt;
                    value *sub_existing;
                    value *sub_element;

                    sub_target = value_component_value(new_value, index);

                    oi_create_from_size_t(oi1, index);
                    if (oi_out_of_memory(oi1))
                      {
                        jumper_do_abort(the_jumper);
                        value_remove_reference(map_value, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }

                    oi_add(oi2, lower_oi, oi1);
                    oi_remove_reference(oi1);
                    if (oi_out_of_memory(oi2))
                      {
                        jumper_do_abort(the_jumper);
                        value_remove_reference(map_value, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }

                    sub_key = create_integer_value(oi2);
                    oi_remove_reference(oi2);
                    if (sub_key == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        value_remove_reference(map_value, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }

                    assert(map_value_all_keys_are_valid(map_value));
                            /* VERIFICATION NEEDED */
                    assert(value_is_valid(sub_key)); /* VERIFIED */
                    sub_existing = map_value_lookup(map_value, sub_key, &doubt,
                                                    location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        jumper_do_abort(the_jumper);
                        value_remove_reference(sub_key, the_jumper);
                        value_remove_reference(map_value, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }
                    assert(!doubt);

                    assert(lookup_actual_arguments_is_valid(actuals));
                            /* VERIFIED */
                    sub_element = set_through_lookup_from(sub_existing,
                            actuals, overload_base, update_function,
                            update_data, care_about_existing_value, sub_target,
                            location, the_jumper, start + 1);

                    value_add_reference(sub_element);

                    assert(value_is_valid(sub_key)); /* VERIFIED */
                    result = set_element_value(result, sub_key, NULL,
                            sub_element, overload_base, location, the_jumper);
                    if (result == NULL)
                      {
                        value_remove_reference(sub_element, the_jumper);
                        value_remove_reference(sub_key, the_jumper);
                        value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }

                    if ((result != map_value) && (sub_element != NULL))
                      {
                        assert(map_value_all_keys_are_valid(map_value));
                                /* VERIFICATION NEEDED */
                        assert(value_is_valid(sub_key)); /* VERIFIED */
                        map_value = map_value_set(map_value, sub_key,
                                sub_element, location, the_jumper);
                      }

                    value_remove_reference(sub_element, the_jumper);
                    value_remove_reference(sub_key, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        if (map_value != NULL)
                            value_remove_reference(map_value, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }
                    assert(map_value != NULL);
                  }

                count = map_value_item_count(map_value);

                for (number = 0; number < count; ++number)
                  {
                    type *sub_key_type;
                    value *sub_key_value;
                    value *sub_target;
                    value *sub_element;

                    box_map_item_key_exclude_items(&sub_key_type,
                            &sub_key_value, map_value, number, lower_oi,
                            upper_oi, new_value, location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(map_value, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }
                    if ((sub_key_type == NULL) && (sub_key_value == NULL))
                        continue;

                    sub_target = map_value_item_target(map_value, number);

                    assert(lookup_actual_arguments_is_valid(actuals));
                            /* VERIFIED */
                    sub_element = set_through_lookup_from(sub_target, actuals,
                            overload_base, update_function, update_data,
                            care_about_existing_value, NULL,
                            location, the_jumper, start + 1);

                    assert((sub_key_type == NULL) ||
                           type_is_valid(sub_key_type)); /* VERIFIED */
                    assert((sub_key_value == NULL) ||
                           value_is_valid(sub_key_value)); /* VERIFIED */
                    result = set_element_value(result, sub_key_value,
                            sub_key_type, sub_element, overload_base, location,
                            the_jumper);
                    if (result == NULL)
                      {
                        if (sub_key_type != NULL)
                            type_remove_reference(sub_key_type, the_jumper);
                        value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }

                    if (sub_key_type != NULL)
                      {
                        type_remove_reference(sub_key_type, the_jumper);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            value_remove_reference(map_value, the_jumper);
                            value_remove_reference(result, the_jumper);
                            return NULL;
                          }
                      }
                  }

                break;
              }
            default:
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(lookup_write_range_not_array),
                        "An assignment through a lookup was evaluated with a "
                        "range, but the value being assigned was not a map or "
                        "a semi-labeled value list.");
                value_remove_reference(map_value, the_jumper);
                value_remove_reference(result, the_jumper);
                return NULL;
              }
          }

        value_remove_reference(map_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            return NULL;
          }
      }
    else
      {
        assert(key == NULL);
        assert(upper_bound == NULL);

        assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
        assert((filter == NULL) || type_is_valid(filter)); /* VERIFIED */
        result = set_through_star_lookup_from(map_value, result, actuals,
                overload_base, filter, update_function, update_data,
                care_about_existing_value, new_value, location, the_jumper,
                start, "star", FALSE);
        if (result == NULL)
            return NULL;
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            return NULL;
          }
      }

    return result;
  }

static value *set_through_star_lookup_from(value *map_value, value *result,
        lookup_actual_arguments *actuals, value *overload_base, type *filter,
        value *(*update_function)(void *update_data, value *existing_value,
                value *new_value, const source_location *the_source_location,
                jumper *the_jumper), void *update_data,
        boolean care_about_existing_value, value *new_value,
        const source_location *location, jumper *the_jumper, size_t start,
        const char *key_kind_name, boolean is_range)
  {
    assert(actuals != NULL);
    assert(update_function != NULL);
    assert(location != NULL);
    assert(the_jumper != NULL);

    assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
    assert((filter == NULL) || type_is_valid(filter)); /* VERIFIED */

    if (care_about_existing_value || ((start + 1) != actuals->dimensions))
        assert(map_value != NULL);

    if (new_value == NULL)
      {
        if (care_about_existing_value || ((start + 1) != actuals->dimensions))
          {
            size_t count;
            size_t number;

            assert(map_value != NULL);

            assert(map_value_all_keys_are_valid(map_value));
                    /* VERIFICATION NEEDED */

            count = map_value_item_count(map_value);

            for (number = 0; number < count; ++number)
              {
                type *sub_key_type;
                value *sub_key_value;
                value *sub_target;
                value *sub_element;

                if (map_value_item_is_type(map_value, number))
                  {
                    sub_key_type = map_value_item_key_type(map_value, number);
                    sub_key_value = NULL;
                    assert(sub_key_type != NULL);
                    assert(type_is_valid(sub_key_type)); /* VERIFIED */

                    if (filter != NULL)
                      {
                        assert(type_is_valid(sub_key_type)); /* VERIFIED */
                        assert(type_is_valid(filter)); /* VERIFIED */
                        sub_key_type =
                                get_intersection_type(sub_key_type, filter);
                        if (sub_key_type == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            value_remove_reference(result, the_jumper);
                            if (map_value != NULL)
                                value_remove_reference(map_value, the_jumper);
                            return NULL;
                          }
                      }
                  }
                else
                  {
                    boolean skip;

                    sub_key_type = NULL;
                    sub_key_value =
                            map_value_item_key_value(map_value, number);
                    assert(sub_key_value != NULL);
                    assert(value_is_valid(sub_key_value)); /* VERIFIED */

                    assert(value_is_valid(sub_key_value)); /* VERIFIED */
                    assert((filter == NULL) || type_is_valid(filter));
                            /* VERIFIED */
                    skip = key_is_outside_filter(sub_key_value, filter,
                                                 location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(result, the_jumper);
                        if (map_value != NULL)
                            value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }
                    if (skip)
                        continue;
                  }

                sub_target = map_value_item_target(map_value, number);

                assert(lookup_actual_arguments_is_valid(actuals));
                        /* VERIFIED */
                sub_element = set_through_lookup_from(sub_target, actuals,
                        overload_base, update_function, update_data,
                        care_about_existing_value, NULL, location, the_jumper,
                        start + 1);

                assert((sub_key_type == NULL) || type_is_valid(sub_key_type));
                        /* VERIFIED */
                assert((sub_key_value == NULL) ||
                       value_is_valid(sub_key_value)); /* VERIFIED */
                result = set_element_value(result, sub_key_value, sub_key_type,
                        sub_element, overload_base, location, the_jumper);
                if (result == NULL)
                  {
                    if ((sub_key_type != NULL) && (filter != NULL))
                        type_remove_reference(sub_key_type, the_jumper);
                    if (map_value != NULL)
                        value_remove_reference(map_value, the_jumper);
                    return NULL;
                  }
                if ((sub_key_type != NULL) && (filter != NULL))
                    type_remove_reference(sub_key_type, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    value_remove_reference(result, the_jumper);
                    if (map_value != NULL)
                        value_remove_reference(map_value, the_jumper);
                    return NULL;
                  }
              }
          }
      }
    else switch (get_value_kind(new_value))
      {
        case VK_MAP:
          {
            size_t count;
            size_t number;

            assert(map_value_all_keys_are_valid(new_value));
                    /* VERIFICATION NEEDED */

            count = map_value_item_count(new_value);

            for (number = 0; number < count; ++number)
              {
                type *sub_key_type;
                value *sub_key_value;
                value *sub_target;

                if (map_value_item_is_type(new_value, number))
                  {
                    sub_key_type = map_value_item_key_type(new_value, number);
                    sub_key_value = NULL;
                    assert(sub_key_type != NULL);
                    assert(type_is_valid(sub_key_type)); /* VERIFIED */

                    if (filter != NULL)
                      {
                        assert(type_is_valid(sub_key_type)); /* VERIFIED */
                        assert(type_is_valid(filter)); /* VERIFIED */
                        sub_key_type =
                                get_intersection_type(sub_key_type, filter);
                        if (sub_key_type == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            value_remove_reference(result, the_jumper);
                            if (map_value != NULL)
                                value_remove_reference(map_value, the_jumper);
                            return NULL;
                          }
                      }
                  }
                else
                  {
                    boolean skip;

                    sub_key_type = NULL;
                    sub_key_value =
                            map_value_item_key_value(new_value, number);
                    assert(sub_key_value != NULL);
                    assert(value_is_valid(sub_key_value)); /* VERIFIED */

                    assert(value_is_valid(sub_key_value)); /* VERIFIED */
                    assert((filter == NULL) || type_is_valid(filter));
                            /* VERIFIED */
                    skip = key_is_outside_filter(sub_key_value, filter,
                                                 location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(result, the_jumper);
                        if (map_value != NULL)
                            value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }
                    if (skip)
                        continue;
                  }

                sub_target = map_value_item_target(new_value, number);

                assert(lookup_actual_arguments_is_valid(actuals));
                        /* VERIFIED */
                assert((map_value == NULL) ||
                       map_value_all_keys_are_valid(map_value));
                        /* VERIFICATION NEEDED */
                assert((sub_key_type == NULL) || type_is_valid(sub_key_type));
                        /* VERIFIED */
                assert((sub_key_value == NULL) ||
                       value_is_valid(sub_key_value)); /* VERIFIED */
                set_through_lookup_from_through_map(&result, actuals,
                        overload_base, update_function, update_data,
                        care_about_existing_value, sub_target, &map_value,
                        sub_key_value, sub_key_type, location, the_jumper,
                        start + 1);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    if ((sub_key_type != NULL) && (filter != NULL))
                        type_remove_reference(sub_key_type, the_jumper);
                    if (map_value != NULL)
                        value_remove_reference(map_value, the_jumper);
                    return NULL;
                  }

                if ((sub_key_type != NULL) && (filter != NULL))
                    type_remove_reference(sub_key_type, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    value_remove_reference(result, the_jumper);
                    if (map_value != NULL)
                        value_remove_reference(map_value, the_jumper);
                    return NULL;
                  }
              }

            break;
          }
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            size_t count;
            size_t index;

            count = value_component_count(new_value);

            for (index = 0; index < count; ++index)
              {
                value *sub_target;
                o_integer oi;
                value *sub_key;
                boolean skip;
                value *sub_existing;
                value *sub_element;

                sub_target = value_component_value(new_value, index);

                oi_create_from_size_t(oi, index);
                if (oi_out_of_memory(oi))
                  {
                    jumper_do_abort(the_jumper);
                    value_remove_reference(result, the_jumper);
                    if (map_value != NULL)
                        value_remove_reference(map_value, the_jumper);
                    return NULL;
                  }

                sub_key = create_integer_value(oi);
                oi_remove_reference(oi);
                if (sub_key == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    value_remove_reference(result, the_jumper);
                    if (map_value != NULL)
                        value_remove_reference(map_value, the_jumper);
                    return NULL;
                  }

                assert(value_is_valid(sub_key)); /* VERIFIED */
                assert((filter == NULL) || type_is_valid(filter));
                        /* VERIFIED */
                skip = key_is_outside_filter(sub_key, filter, location,
                                             the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    value_remove_reference(sub_key, the_jumper);
                    value_remove_reference(result, the_jumper);
                    if (map_value != NULL)
                        value_remove_reference(map_value, the_jumper);
                    return NULL;
                  }
                if (skip)
                  {
                    value_remove_reference(sub_key, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(result, the_jumper);
                        if (map_value != NULL)
                            value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }
                    continue;
                  }

                if ((!care_about_existing_value) &&
                    ((start + 1) == actuals->dimensions))
                  {
                    sub_existing = NULL;
                  }
                else
                  {
                    boolean doubt;

                    assert(map_value != NULL);
                    assert(map_value_all_keys_are_valid(map_value));
                            /* VERIFICATION NEEDED */
                    assert(value_is_valid(sub_key)); /* VERIFIED */
                    sub_existing = map_value_lookup(map_value, sub_key, &doubt,
                                                    location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(sub_key, the_jumper);
                        value_remove_reference(result, the_jumper);
                        if (map_value != NULL)
                            value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }
                    assert(!doubt);
                  }

                assert(lookup_actual_arguments_is_valid(actuals));
                        /* VERIFIED */
                sub_element = set_through_lookup_from(sub_existing, actuals,
                        overload_base, update_function, update_data,
                        care_about_existing_value, sub_target, location,
                        the_jumper, start + 1);

                value_add_reference(sub_element);

                assert(value_is_valid(sub_key)); /* VERIFIED */
                result = set_element_value(result, sub_key, NULL, sub_element,
                        overload_base, location, the_jumper);
                if (result == NULL)
                  {
                    value_remove_reference(sub_element, the_jumper);
                    value_remove_reference(sub_key, the_jumper);
                    if (map_value != NULL)
                        value_remove_reference(map_value, the_jumper);
                    return NULL;
                  }

                if ((map_value != NULL) && (result != map_value) &&
                    (sub_element != NULL))
                  {
                    assert(map_value_all_keys_are_valid(map_value));
                            /* VERIFICATION NEEDED */
                    assert(value_is_valid(sub_key)); /* VERIFIED */
                    map_value = map_value_set(map_value, sub_key, sub_element,
                                              location, the_jumper);
                  }

                value_remove_reference(sub_element, the_jumper);
                value_remove_reference(sub_key, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    value_remove_reference(result, the_jumper);
                    if (map_value != NULL)
                        value_remove_reference(map_value, the_jumper);
                    return NULL;
                  }
              }

            if (care_about_existing_value ||
                ((start + 1) != actuals->dimensions))
              {
                size_t count;
                size_t number;

                assert(map_value != NULL);

                assert(map_value_all_keys_are_valid(map_value));
                        /* VERIFICATION NEEDED */

                count = map_value_item_count(map_value);

                for (number = 0; number < count; ++number)
                  {
                    type *sub_key_type;
                    value *sub_key_value;
                    value *sub_target;
                    value *sub_element;

                    if (map_value_item_is_type(map_value, number))
                      {
                        sub_key_type =
                                map_value_item_key_type(map_value, number);
                        sub_key_value = NULL;
                        assert(sub_key_type != NULL);
                        assert(type_is_valid(sub_key_type)); /* VERIFIED */
                      }
                    else
                      {
                        boolean skip;

                        sub_key_type = NULL;
                        sub_key_value =
                                map_value_item_key_value(map_value, number);
                        assert(sub_key_value != NULL);
                        assert(value_is_valid(sub_key_value)); /* VERIFIED */

                        assert(value_is_valid(sub_key_value)); /* VERIFIED */
                        assert((filter == NULL) || type_is_valid(filter));
                                /* VERIFIED */
                        skip = key_is_outside_filter(sub_key_value, filter,
                                                     location, the_jumper);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            value_remove_reference(result, the_jumper);
                            if (map_value != NULL)
                                value_remove_reference(map_value, the_jumper);
                            return NULL;
                          }
                        if (skip)
                            continue;
                      }

                    sub_target = map_value_item_target(map_value, number);

                    assert(lookup_actual_arguments_is_valid(actuals));
                            /* VERIFIED */
                    sub_element = set_through_lookup_from(sub_target, actuals,
                            overload_base, update_function, update_data,
                            care_about_existing_value, NULL, location,
                            the_jumper, start + 1);

                    assert((sub_key_type == NULL) ||
                           type_is_valid(sub_key_type)); /* VERIFIED */
                    assert((sub_key_value == NULL) ||
                           value_is_valid(sub_key_value)); /* VERIFIED */
                    result = set_element_value(result, sub_key_value,
                            sub_key_type, sub_element, overload_base, location,
                            the_jumper);
                    if (result == NULL)
                      {
                        if (map_value != NULL)
                            value_remove_reference(map_value, the_jumper);
                        return NULL;
                      }
                  }
              }

            break;
          }
        default:
          {
            location_exception(the_jumper, location,
                    (is_range ? EXCEPTION_TAG(lookup_write_range_not_array) :
                                EXCEPTION_TAG(lookup_write_star_not_array)),
                    "An assignment through a lookup was evaluated with a %s, "
                    "but the value being assigned was not a map or a "
                    "semi-labeled value list.", key_kind_name);
            value_remove_reference(result, the_jumper);
            if (map_value != NULL)
                value_remove_reference(map_value, the_jumper);
            return NULL;
          }
      }

    if (map_value != NULL)
        value_remove_reference(map_value, the_jumper);

    return result;
  }

static void find_lookup_types_from(type *base_lower_read,
        type *base_upper_read, type *base_lower_write, type *base_upper_write,
        variable_instance *base_base_variable,
        lookup_actual_arguments *actuals, value *overload_base,
        void (*update_function)(void *update_data, type *in_lower_read,
                type *in_upper_read, type *in_lower_write,
                type *in_upper_write, variable_instance *in_base_variable,
                type **out_lower_read, type **out_upper_read,
                type **out_lower_write, type **out_upper_write,
                variable_instance **out_base_variable), void *update_data,
        boolean skip_overloading, type **result_lower_read,
        type **result_upper_read, type **result_lower_write,
        type **result_upper_write, variable_instance **result_base_variable,
        size_t start, const source_location *location, jumper *the_jumper)
  {
    lookup_item *the_item;
    value *key;
    value *upper_bound;
    type *filter;
    type *additional_lower_write;
    type *additional_upper_write;
    boolean overload_read_always_hits;
    boolean overload_read_never_hits;
    type *overload_lower_read;
    type *overload_upper_read;
    type *overload_lower_write;
    type *overload_upper_write;
    boolean no_base_variable;
    boolean overload_write_always_hits;
    boolean overload_single_write_always_hits;
    boolean overload_single_write_never_hits;

    assert(base_lower_read != NULL);
    assert(base_upper_read != NULL);
    assert(base_lower_write != NULL);
    assert(base_upper_write != NULL);
    assert(actuals != NULL);
    assert(the_jumper != NULL);

    assert(type_is_valid(base_lower_read)); /* VERIFIED */
    assert(type_is_valid(base_upper_read)); /* VERIFIED */
    assert(type_is_valid(base_lower_write)); /* VERIFIED */
    assert(type_is_valid(base_upper_write)); /* VERIFIED */
    assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */

    assert(start <= actuals->dimensions);
    if (start == actuals->dimensions)
      {
        assert(type_is_valid(base_lower_read)); /* VERIFIED */
        assert(type_is_valid(base_upper_read)); /* VERIFIED */
        assert(type_is_valid(base_lower_write)); /* VERIFIED */
        assert(type_is_valid(base_upper_write)); /* VERIFIED */
        (*update_function)(update_data, base_lower_read, base_upper_read,
                base_lower_write, base_upper_write, base_base_variable,
                result_lower_read, result_upper_read, result_lower_write,
                result_upper_write, result_base_variable);

        assert((result_lower_read == NULL) || (*result_lower_read == NULL) ||
               type_is_valid(*result_lower_read)); /* VERIFIED */
        assert((result_upper_read == NULL) || (*result_upper_read == NULL) ||
               type_is_valid(*result_upper_read)); /* VERIFIED */
        assert((result_lower_write == NULL) || (*result_lower_write == NULL) ||
               type_is_valid(*result_lower_write)); /* VERIFIED */
        assert((result_upper_write == NULL) || (*result_upper_write == NULL) ||
               type_is_valid(*result_upper_write)); /* VERIFIED */
        return;
      }

    if (result_lower_read != NULL)
        *result_lower_read = NULL;
    if (result_upper_read != NULL)
        *result_upper_read = NULL;
    if (result_lower_write != NULL)
        *result_lower_write = NULL;
    if (result_upper_write != NULL)
        *result_upper_write = NULL;
    if (result_base_variable != NULL)
        *result_base_variable = NULL;

    assert((result_lower_read == NULL) || (*result_lower_read == NULL));
            /* VERIFIED */
    assert((result_upper_read == NULL) || (*result_upper_read == NULL));
            /* VERIFIED */
    assert((result_lower_write == NULL) || (*result_lower_write == NULL));
            /* VERIFIED */
    assert((result_upper_write == NULL) || (*result_upper_write == NULL));
            /* VERIFIED */

    assert(actuals->array != NULL);
    the_item = &(actuals->array[start]);

    if (skip_overloading)
      {
        key = NULL;
        upper_bound = NULL;
        filter = NULL;
      }
    else
      {
        key = the_item->key;
        assert((key == NULL) || value_is_valid(key)); /* VERIFIED */
        upper_bound = the_item->upper_bound;
        assert((upper_bound == NULL) || value_is_valid(upper_bound));
                /* VERIFIED */
        filter = the_item->filter;
        assert((filter == NULL) || type_is_valid(filter)); /* VERIFIED */
      }

    assert((key == NULL) || value_is_valid(key)); /* VERIFIED */
    assert((upper_bound == NULL) || value_is_valid(upper_bound));
            /* VERIFIED */
    assert((filter == NULL) || type_is_valid(filter)); /* VERIFIED */

    additional_lower_write = NULL;
    additional_upper_write = NULL;
    overload_lower_write = NULL;
    overload_upper_write = NULL;

    no_base_variable = FALSE;

    if (skip_overloading)
      {
        overload_read_always_hits = FALSE;
        overload_read_never_hits = TRUE;
        overload_lower_read = NULL;
        overload_upper_read = NULL;
        overload_write_always_hits = FALSE;
      }
    else
      {
        type *overload_result_lower_read;
        type *overload_result_upper_read;

        assert(base_lower_read != NULL);
        assert(base_upper_read != NULL);
        assert(type_is_valid(base_lower_read)); /* VERIFIED */
        assert(type_is_valid(base_upper_read)); /* VERIFIED */
        assert((key == NULL) || value_is_valid(key)); /* VERIFIED */
        assert((filter == NULL) || type_is_valid(filter)); /* VERIFIED */
        lookup_overload_type(base_lower_read, base_upper_read, key,
                upper_bound, filter, overload_base, FALSE, NULL, NULL, FALSE,
                &overload_read_always_hits, &overload_read_never_hits,
                &overload_lower_read, &overload_upper_read, location,
                the_jumper);
        assert((overload_lower_read == NULL) ||
               (type_is_valid(overload_lower_read))); /* VERIFIED */
        assert((overload_upper_read == NULL) ||
               (type_is_valid(overload_upper_read))); /* VERIFIED */
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(overload_lower_read == NULL);
            assert(overload_upper_read == NULL);
            assert((result_lower_read == NULL) ||
                   (*result_lower_read == NULL)); /* VERIFIED */
            assert((result_upper_read == NULL) ||
                   (*result_upper_read == NULL)); /* VERIFIED */
            assert((result_lower_write == NULL) ||
                   (*result_lower_write == NULL)); /* VERIFIED */
            assert((result_upper_write == NULL) ||
                   (*result_upper_write == NULL)); /* VERIFIED */
            return;
          }

        assert(overload_lower_read != NULL);
        assert(overload_upper_read != NULL);
        assert(type_is_valid(overload_lower_read)); /* VERIFIED */
        assert(type_is_valid(overload_upper_read)); /* VERIFIED */

        if (overload_read_never_hits)
          {
            assert(!overload_read_always_hits);

            type_remove_reference(overload_lower_read, the_jumper);
            type_remove_reference(overload_upper_read, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert((result_lower_read == NULL) ||
                       (*result_lower_read == NULL)); /* VERIFIED */
                assert((result_upper_read == NULL) ||
                       (*result_upper_read == NULL)); /* VERIFIED */
                assert((result_lower_write == NULL) ||
                       (*result_lower_write == NULL)); /* VERIFIED */
                assert((result_upper_write == NULL) ||
                       (*result_upper_write == NULL)); /* VERIFIED */
                return;
              }
            overload_lower_read = NULL;
            overload_upper_read = NULL;

            overload_write_always_hits = FALSE;

            overload_result_lower_read = NULL;
            overload_result_upper_read = NULL;
          }
        else
          {
            boolean overload_write_never_hits;
            type *overload_lower_write;
            type *overload_upper_write;

            assert(base_lower_read != NULL);
            assert(base_upper_read != NULL);
            assert(type_is_valid(base_lower_read)); /* VERIFIED */
            assert(type_is_valid(base_upper_read)); /* VERIFIED */
            assert((key == NULL) || value_is_valid(key)); /* VERIFIED */
            assert((filter == NULL) || type_is_valid(filter)); /* VERIFIED */
            assert(type_is_valid(base_lower_write)); /* VERIFIED */
            assert(type_is_valid(base_upper_write)); /* VERIFIED */
            lookup_overload_type(base_lower_read, base_upper_read, key,
                    upper_bound, filter, overload_base, TRUE, base_lower_write,
                    base_upper_write, TRUE, &overload_write_always_hits,
                    &overload_write_never_hits, &overload_lower_write,
                    &overload_upper_write, location, the_jumper);
            assert((overload_lower_write == NULL) ||
                   (type_is_valid(overload_lower_write))); /* VERIFIED */
            assert((overload_upper_write == NULL) ||
                   (type_is_valid(overload_upper_write))); /* VERIFIED */
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(overload_lower_write == NULL);
                assert(overload_upper_write == NULL);
                assert(!(jumper_flowing_forward(the_jumper)));
                assert((result_lower_read == NULL) ||
                       (*result_lower_read == NULL)); /* VERIFIED */
                assert((result_upper_read == NULL) ||
                       (*result_upper_read == NULL)); /* VERIFIED */
                assert((result_lower_write == NULL) ||
                       (*result_lower_write == NULL)); /* VERIFIED */
                assert((result_upper_write == NULL) ||
                       (*result_upper_write == NULL)); /* VERIFIED */
                goto cleanup;
              }

            assert(overload_lower_write != NULL);
            assert(overload_upper_write != NULL);
            assert(type_is_valid(overload_lower_write)); /* VERIFIED */
            assert(type_is_valid(overload_upper_write)); /* VERIFIED */

            if (!overload_write_never_hits)
                no_base_variable = TRUE;

            if ((!overload_write_never_hits) &&
                ((key == NULL) || (upper_bound != NULL)))
              {
                assert(type_is_valid(overload_lower_read)); /* VERIFIED */
                assert(type_is_valid(overload_upper_read)); /* VERIFIED */
                assert(type_is_valid(overload_lower_write)); /* VERIFIED */
                assert(type_is_valid(overload_upper_write)); /* VERIFIED */
                assert(lookup_actual_arguments_is_valid(actuals));
                        /* VERIFIED */
                find_lookup_types_from(overload_lower_read,
                        overload_upper_read, overload_lower_write,
                        overload_upper_write, base_base_variable, actuals,
                        overload_base, update_function, update_data, TRUE,
                        &overload_result_lower_read,
                        &overload_result_upper_read, &additional_lower_write,
                        &additional_upper_write, NULL, start, location,
                        the_jumper);
                assert((overload_result_lower_read == NULL) ||
                       type_is_valid(overload_result_lower_read));
                        /* VERIFIED */
                assert((overload_result_upper_read == NULL) ||
                       type_is_valid(overload_result_upper_read));
                        /* VERIFIED */
                assert((additional_lower_write == NULL) ||
                       type_is_valid(additional_lower_write)); /* VERIFIED */
                assert((additional_upper_write == NULL) ||
                       type_is_valid(additional_upper_write)); /* VERIFIED */
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    assert(overload_result_lower_read == NULL);
                    assert(overload_result_upper_read == NULL);
                    assert(additional_lower_write == NULL);
                    assert(additional_upper_write == NULL);
                    type_remove_reference(overload_lower_write, the_jumper);
                    type_remove_reference(overload_upper_write, the_jumper);
                    assert((result_lower_read == NULL) ||
                           (*result_lower_read == NULL)); /* VERIFIED */
                    assert((result_upper_read == NULL) ||
                           (*result_upper_read == NULL)); /* VERIFIED */
                    assert((result_lower_write == NULL) ||
                           (*result_lower_write == NULL)); /* VERIFIED */
                    assert((result_upper_write == NULL) ||
                           (*result_upper_write == NULL)); /* VERIFIED */
                    goto cleanup;
                  }

                assert(overload_result_lower_read != NULL);
                assert(overload_result_upper_read != NULL);
                assert(additional_lower_write != NULL);
                assert(additional_upper_write != NULL);
                assert(type_is_valid(overload_result_lower_read));
                        /* VERIFIED */
                assert(type_is_valid(overload_result_upper_read));
                        /* VERIFIED */
                assert(type_is_valid(additional_lower_write)); /* VERIFIED */
                assert(type_is_valid(additional_upper_write)); /* VERIFIED */
              }
            else
              {
                overload_result_lower_read = NULL;
                overload_result_upper_read = NULL;
              }

            assert((result_lower_read == NULL) ||
                   (*result_lower_read == NULL)); /* VERIFIED */
            assert((result_upper_read == NULL) ||
                   (*result_upper_read == NULL)); /* VERIFIED */
            assert((result_lower_write == NULL) ||
                   (*result_lower_write == NULL)); /* VERIFIED */
            assert((result_upper_write == NULL) ||
                   (*result_upper_write == NULL)); /* VERIFIED */

            type_remove_reference(overload_lower_write, the_jumper);
            type_remove_reference(overload_upper_write, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
              cleanup_plus:
                if (overload_result_lower_read != NULL)
                  {
                    type_remove_reference(overload_result_lower_read,
                                          the_jumper);
                  }
                if (overload_result_upper_read != NULL)
                  {
                    type_remove_reference(overload_result_upper_read,
                                          the_jumper);
                  }
                goto cleanup;
              }
          }

        assert((result_lower_read == NULL) || (*result_lower_read == NULL));
                /* VERIFIED */
        assert((result_upper_read == NULL) || (*result_upper_read == NULL));
                /* VERIFIED */
        assert((result_lower_write == NULL) || (*result_lower_write == NULL));
                /* VERIFIED */
        assert((result_upper_write == NULL) || (*result_upper_write == NULL));
                /* VERIFIED */

        assert(base_lower_read != NULL);
        assert(base_upper_read != NULL);
        assert(type_is_valid(base_lower_read)); /* VERIFIED */
        assert(type_is_valid(base_upper_read)); /* VERIFIED */
        assert((key == NULL) || value_is_valid(key)); /* VERIFIED */
        assert((filter == NULL) || type_is_valid(filter)); /* VERIFIED */
        assert(type_is_valid(base_lower_write)); /* VERIFIED */
        assert(type_is_valid(base_upper_write)); /* VERIFIED */
        lookup_overload_type(base_lower_read, base_upper_read, key,
                upper_bound, filter, overload_base, TRUE, base_lower_write,
                base_upper_write, FALSE, &overload_single_write_always_hits,
                &overload_single_write_never_hits, &overload_lower_write,
                &overload_upper_write, location, the_jumper);
        assert((overload_lower_write == NULL) ||
               (type_is_valid(overload_lower_write))); /* VERIFIED */
        assert((overload_upper_write == NULL) ||
               (type_is_valid(overload_upper_write))); /* VERIFIED */
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(overload_lower_write == NULL);
            assert(overload_upper_write == NULL);
            goto cleanup_plus;
          }

        assert(overload_lower_write != NULL);
        assert(overload_upper_write != NULL);
        assert(type_is_valid(overload_lower_write)); /* VERIFIED */
        assert(type_is_valid(overload_upper_write)); /* VERIFIED */

        if (!overload_single_write_never_hits)
            no_base_variable = TRUE;

        if (overload_single_write_never_hits)
          {
            assert(!overload_single_write_always_hits);

            type_remove_reference(overload_lower_write, the_jumper);
            type_remove_reference(overload_upper_write, the_jumper);
            overload_lower_write = NULL;
            overload_upper_write = NULL;
            if (!(jumper_flowing_forward(the_jumper)))
                goto cleanup_plus;
          }
        else if ((key == NULL) || (upper_bound != NULL))
          {
            if (overload_single_write_always_hits)
              {
                if (additional_lower_write != NULL)
                  {
                    type_remove_reference(additional_lower_write, the_jumper);
                    additional_lower_write = NULL;
                    if (!(jumper_flowing_forward(the_jumper)))
                        goto cleanup_plus;
                  }
                assert(type_is_valid(overload_lower_write)); /* VERIFIED */
                type_add_reference(overload_lower_write);
                additional_lower_write = overload_lower_write;
                assert(type_is_valid(additional_lower_write)); /* VERIFIED */

                if (additional_upper_write != NULL)
                  {
                    type_remove_reference(additional_upper_write, the_jumper);
                    additional_upper_write = NULL;
                    if (!(jumper_flowing_forward(the_jumper)))
                        goto cleanup_plus;
                  }
                assert(type_is_valid(overload_upper_write)); /* VERIFIED */
                type_add_reference(overload_upper_write);
                additional_upper_write = overload_upper_write;
                assert(type_is_valid(additional_upper_write)); /* VERIFIED */
              }
            else
              {
                if (additional_lower_write == NULL)
                  {
                    assert(type_is_valid(overload_lower_write)); /* VERIFIED */
                    type_add_reference(overload_lower_write);
                    additional_lower_write = overload_lower_write;
                    assert(type_is_valid(additional_lower_write));
                            /* VERIFIED */
                  }
                else
                  {
                    type *new_type;

                    assert(type_is_valid(additional_lower_write));
                            /* VERIFIED */
                    assert(type_is_valid(overload_lower_write)); /* VERIFIED */
                    new_type = get_union_type(additional_lower_write,
                                              overload_lower_write);
                    assert((new_type == NULL) || type_is_valid(new_type));
                            /* VERIFIED */
                    if (new_type == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        goto cleanup_plus;
                      }
                    assert(type_is_valid(new_type)); /* VERIFIED */
                    type_remove_reference(additional_lower_write, the_jumper);
                    additional_lower_write = new_type;
                    assert(type_is_valid(additional_lower_write));
                            /* VERIFIED */
                    if (!(jumper_flowing_forward(the_jumper)))
                        goto cleanup_plus;
                  }

                if (additional_upper_write == NULL)
                  {
                    assert(type_is_valid(overload_upper_write)); /* VERIFIED */
                    type_add_reference(overload_upper_write);
                    additional_upper_write = overload_upper_write;
                    assert(type_is_valid(additional_upper_write));
                            /* VERIFIED */
                  }
                else
                  {
                    type *new_type;

                    assert(type_is_valid(additional_upper_write));
                            /* VERIFIED */
                    assert(type_is_valid(overload_upper_write)); /* VERIFIED */
                    new_type = get_union_type(additional_upper_write,
                                              overload_upper_write);
                    assert((new_type == NULL) || type_is_valid(new_type));
                            /* VERIFIED */
                    if (new_type == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        goto cleanup_plus;
                      }
                    assert(type_is_valid(new_type)); /* VERIFIED */
                    type_remove_reference(additional_upper_write, the_jumper);
                    additional_upper_write = new_type;
                    assert(type_is_valid(additional_upper_write));
                            /* VERIFIED */
                    if (!(jumper_flowing_forward(the_jumper)))
                        goto cleanup_plus;
                  }
              }

            assert((result_lower_read == NULL) ||
                   (*result_lower_read == NULL)); /* VERIFIED */
            assert((result_upper_read == NULL) ||
                   (*result_upper_read == NULL)); /* VERIFIED */
            assert((result_lower_write == NULL) ||
                   (*result_lower_write == NULL)); /* VERIFIED */
            assert((result_upper_write == NULL) ||
                   (*result_upper_write == NULL)); /* VERIFIED */

            type_remove_reference(overload_lower_write, the_jumper);
            type_remove_reference(overload_upper_write, the_jumper);
            overload_lower_write = NULL;
            overload_upper_write = NULL;
            if (!(jumper_flowing_forward(the_jumper)))
                goto cleanup_plus;
          }

        if (((key == NULL) || (upper_bound != NULL)) &&
            overload_read_always_hits &&
            (overload_write_always_hits || overload_single_write_always_hits))
          {
            assert(overload_result_lower_read != NULL);
            assert(overload_result_upper_read != NULL);
            assert(type_is_valid(overload_result_lower_read)); /* VERIFIED */
            assert(type_is_valid(overload_result_upper_read)); /* VERIFIED */

            if (result_lower_read != NULL)
                *result_lower_read = overload_result_lower_read;
            else
                type_remove_reference(overload_result_lower_read, the_jumper);
            if (result_upper_read != NULL)
                *result_upper_read = overload_result_upper_read;
            else
                type_remove_reference(overload_result_upper_read, the_jumper);
            if (result_lower_write != NULL)
              {
                assert(type_is_valid(additional_lower_write)); /* VERIFIED */
                *result_lower_write = additional_lower_write;
                assert(type_is_valid(*result_lower_write)); /* VERIFIED */
              }
            else
              {
                type_remove_reference(additional_lower_write, the_jumper);
              }
            additional_lower_write = NULL;
            if (result_upper_write != NULL)
              {
                assert(type_is_valid(additional_upper_write)); /* VERIFIED */
                *result_upper_write = additional_upper_write;
                assert(type_is_valid(*result_upper_write)); /* VERIFIED */
              }
            else
              {
                type_remove_reference(additional_upper_write, the_jumper);
              }
            additional_upper_write = NULL;
            assert((result_lower_read == NULL) ||
                   type_is_valid(*result_lower_read)); /* VERIFIED */
            assert((result_upper_read == NULL) ||
                   type_is_valid(*result_upper_read)); /* VERIFIED */
            assert((result_lower_write == NULL) ||
                   type_is_valid(*result_lower_write)); /* VERIFIED */
            assert((result_upper_write == NULL) ||
                   type_is_valid(*result_upper_write)); /* VERIFIED */
            type_remove_reference(overload_lower_read, the_jumper);
            type_remove_reference(overload_upper_read, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                goto cleanup;
            assert((result_lower_read == NULL) ||
                   type_is_valid(*result_lower_read)); /* VERIFIED */
            assert((result_upper_read == NULL) ||
                   type_is_valid(*result_upper_read)); /* VERIFIED */
            assert((result_lower_write == NULL) ||
                   type_is_valid(*result_lower_write)); /* VERIFIED */
            assert((result_upper_write == NULL) ||
                   type_is_valid(*result_upper_write)); /* VERIFIED */
            if (result_base_variable != NULL)
                *result_base_variable = NULL;
            return;
          }

        if (overload_result_lower_read != NULL)
            type_remove_reference(overload_result_lower_read, the_jumper);
        if (overload_result_upper_read != NULL)
            type_remove_reference(overload_result_upper_read, the_jumper);
      }

    assert((result_lower_read == NULL) || (*result_lower_read == NULL));
            /* VERIFIED */
    assert((result_upper_read == NULL) || (*result_upper_read == NULL));
            /* VERIFIED */
    assert((result_lower_write == NULL) || (*result_lower_write == NULL));
            /* VERIFIED */
    assert((result_upper_write == NULL) || (*result_upper_write == NULL));
            /* VERIFIED */

    if ((key != NULL) && (upper_bound == NULL))
      {
        type *element_lower_read;
        type *element_upper_read;
        type *element_lower_write;
        type *element_upper_write;

        assert(key != NULL);
        assert(upper_bound == NULL);
        assert(filter == NULL);

        assert(type_is_valid(base_lower_read)); /* VERIFIED */
        assert(value_is_valid(key)); /* VERIFIED */
        element_lower_read = map_target_type(base_lower_read, key, LOU_LOWER,
                                             location, the_jumper);
        assert((element_lower_read == NULL) ||
               type_is_valid(element_lower_read)); /* VERIFIED */
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(element_lower_read == NULL);
            goto cleanup;
          }
        assert(element_lower_read != NULL);
        assert(type_is_valid(element_lower_read)); /* VERIFIED */

        assert(type_is_valid(base_upper_read)); /* VERIFIED */
        assert(value_is_valid(key)); /* VERIFIED */
        element_upper_read = map_target_type(base_upper_read, key, LOU_UPPER,
                                             location, the_jumper);
        assert((element_upper_read == NULL) ||
               type_is_valid(element_upper_read)); /* VERIFIED */
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(element_upper_read == NULL);
            type_remove_reference(element_lower_read, the_jumper);
            goto cleanup;
          }
        assert(element_upper_read != NULL);
        assert(type_is_valid(element_upper_read)); /* VERIFIED */

        assert(type_is_valid(base_lower_write)); /* VERIFIED */
        assert(value_is_valid(key)); /* VERIFIED */
        element_lower_write = map_target_type(base_lower_write, key, LOU_LOWER,
                                              location, the_jumper);
        assert((element_lower_write == NULL) ||
               type_is_valid(element_lower_write)); /* VERIFIED */
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(element_lower_write == NULL);
            type_remove_reference(element_lower_read, the_jumper);
            type_remove_reference(element_upper_read, the_jumper);
            goto cleanup;
          }
        assert(element_lower_write != NULL);
        assert(type_is_valid(element_lower_write)); /* VERIFIED */

        assert(type_is_valid(base_upper_write)); /* VERIFIED */
        assert(value_is_valid(key)); /* VERIFIED */
        element_upper_write = map_target_type(base_upper_write, key, LOU_UPPER,
                                              location, the_jumper);
        assert((element_upper_write == NULL) ||
               type_is_valid(element_upper_write)); /* VERIFIED */
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(element_upper_write == NULL);
            type_remove_reference(element_lower_read, the_jumper);
            type_remove_reference(element_upper_read, the_jumper);
            type_remove_reference(element_lower_write, the_jumper);
            goto cleanup;
          }
        assert(element_upper_write != NULL);
        assert(type_is_valid(element_upper_write)); /* VERIFIED */

        assert(element_lower_read != NULL);
        assert(element_upper_read != NULL);
        assert(element_lower_write != NULL);
        assert(element_upper_write != NULL);
        assert(type_is_valid(element_lower_read)); /* VERIFIED */
        assert(type_is_valid(element_upper_read)); /* VERIFIED */
        assert(type_is_valid(element_lower_write)); /* VERIFIED */
        assert(type_is_valid(element_upper_write)); /* VERIFIED */

        if (overload_lower_read != NULL)
          {
            if (overload_read_always_hits)
              {
                assert(type_is_valid(overload_lower_read)); /* VERIFIED */
                type_add_reference(overload_lower_read);
                type_remove_reference(element_lower_read, the_jumper);
                element_lower_read = overload_lower_read;
                assert(type_is_valid(element_lower_read)); /* VERIFIED */
              }
            else
              {
                type *new_type;

                assert(type_is_valid(element_lower_read)); /* VERIFIED */
                assert(type_is_valid(overload_lower_read)); /* VERIFIED */
                new_type = get_union_type(element_lower_read,
                                          overload_lower_read);
                assert((new_type == NULL) || type_is_valid(new_type));
                        /* VERIFIED */
                if (new_type == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    type_remove_reference(element_lower_read, the_jumper);
                    type_remove_reference(element_upper_read, the_jumper);
                    type_remove_reference(element_lower_write, the_jumper);
                    type_remove_reference(element_upper_write, the_jumper);
                    goto cleanup;
                  }
                assert(type_is_valid(new_type)); /* VERIFIED */
                type_remove_reference(element_lower_read, the_jumper);
                element_lower_read = new_type;
                assert(type_is_valid(element_lower_read)); /* VERIFIED */
              }
            assert(type_is_valid(element_lower_read)); /* VERIFIED */

            if (!(jumper_flowing_forward(the_jumper)))
              {
                type_remove_reference(element_lower_read, the_jumper);
                type_remove_reference(element_upper_read, the_jumper);
                type_remove_reference(element_lower_write, the_jumper);
                type_remove_reference(element_upper_write, the_jumper);
                goto cleanup;
              }
          }

        assert(type_is_valid(element_lower_read)); /* VERIFIED */

        if (overload_upper_read != NULL)
          {
            if (overload_read_always_hits)
              {
                assert(type_is_valid(overload_upper_read)); /* VERIFIED */
                type_add_reference(overload_upper_read);
                type_remove_reference(element_upper_read, the_jumper);
                element_upper_read = overload_upper_read;
                assert(type_is_valid(element_upper_read)); /* VERIFIED */
              }
            else
              {
                type *new_type;

                assert(type_is_valid(element_upper_read)); /* VERIFIED */
                assert(type_is_valid(overload_upper_read)); /* VERIFIED */
                new_type = get_union_type(element_upper_read,
                                          overload_upper_read);
                assert((new_type == NULL) || type_is_valid(new_type));
                        /* VERIFIED */
                if (new_type == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    type_remove_reference(element_lower_read, the_jumper);
                    type_remove_reference(element_upper_read, the_jumper);
                    type_remove_reference(element_lower_write, the_jumper);
                    type_remove_reference(element_upper_write, the_jumper);
                    goto cleanup;
                  }
                assert(type_is_valid(new_type)); /* VERIFIED */
                type_remove_reference(element_upper_read, the_jumper);
                element_upper_read = new_type;
                assert(type_is_valid(element_upper_read)); /* VERIFIED */
              }
            assert(type_is_valid(element_upper_read)); /* VERIFIED */

            if (!(jumper_flowing_forward(the_jumper)))
              {
                type_remove_reference(element_lower_read, the_jumper);
                type_remove_reference(element_upper_read, the_jumper);
                type_remove_reference(element_lower_write, the_jumper);
                type_remove_reference(element_upper_write, the_jumper);
                goto cleanup;
              }
          }

        assert(type_is_valid(element_upper_read)); /* VERIFIED */

        if (overload_lower_write != NULL)
          {
            if (overload_single_write_always_hits)
              {
                assert(type_is_valid(overload_lower_write)); /* VERIFIED */
                type_add_reference(overload_lower_write);
                type_remove_reference(element_lower_write, the_jumper);
                element_lower_write = overload_lower_write;
                assert(type_is_valid(element_lower_write)); /* VERIFIED */
              }
            else
              {
                type *new_type;

                assert(type_is_valid(element_lower_write)); /* VERIFIED */
                assert(type_is_valid(overload_lower_write)); /* VERIFIED */
                new_type = get_union_type(element_lower_write,
                                          overload_lower_write);
                assert((new_type == NULL) || type_is_valid(new_type));
                        /* VERIFIED */
                if (new_type == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    type_remove_reference(element_lower_read, the_jumper);
                    type_remove_reference(element_upper_read, the_jumper);
                    type_remove_reference(element_lower_write, the_jumper);
                    type_remove_reference(element_upper_write, the_jumper);
                    goto cleanup;
                  }
                assert(type_is_valid(new_type)); /* VERIFIED */
                type_remove_reference(element_lower_write, the_jumper);
                element_lower_write = new_type;
                assert(type_is_valid(element_lower_write)); /* VERIFIED */
              }
            assert(type_is_valid(element_lower_write)); /* VERIFIED */

            if (!(jumper_flowing_forward(the_jumper)))
              {
                type_remove_reference(element_lower_read, the_jumper);
                type_remove_reference(element_upper_read, the_jumper);
                type_remove_reference(element_lower_write, the_jumper);
                type_remove_reference(element_upper_write, the_jumper);
                goto cleanup;
              }
          }

        assert(type_is_valid(element_lower_write)); /* VERIFIED */

        if (overload_upper_write != NULL)
          {
            if (overload_single_write_always_hits)
              {
                assert(type_is_valid(overload_upper_write)); /* VERIFIED */
                type_add_reference(overload_upper_write);
                type_remove_reference(element_upper_write, the_jumper);
                element_upper_write = overload_upper_write;
                assert(type_is_valid(element_upper_write)); /* VERIFIED */
              }
            else
              {
                type *new_type;

                assert(type_is_valid(element_upper_write)); /* VERIFIED */
                assert(type_is_valid(overload_upper_write)); /* VERIFIED */
                new_type = get_union_type(element_upper_write,
                                          overload_upper_write);
                assert((new_type == NULL) || type_is_valid(new_type));
                        /* VERIFIED */
                if (new_type == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    type_remove_reference(element_lower_read, the_jumper);
                    type_remove_reference(element_upper_read, the_jumper);
                    type_remove_reference(element_lower_write, the_jumper);
                    type_remove_reference(element_upper_write, the_jumper);
                    goto cleanup;
                  }
                assert(type_is_valid(new_type)); /* VERIFIED */
                type_remove_reference(element_upper_write, the_jumper);
                element_upper_write = new_type;
                assert(type_is_valid(element_upper_write)); /* VERIFIED */
              }
            assert(type_is_valid(element_upper_write)); /* VERIFIED */

            if (!(jumper_flowing_forward(the_jumper)))
              {
                type_remove_reference(element_lower_read, the_jumper);
                type_remove_reference(element_upper_read, the_jumper);
                type_remove_reference(element_lower_write, the_jumper);
                type_remove_reference(element_upper_write, the_jumper);
                goto cleanup;
              }
          }

        assert(type_is_valid(element_upper_write)); /* VERIFIED */

        assert((result_lower_read == NULL) || (*result_lower_read == NULL));
                /* VERIFIED */
        assert((result_upper_read == NULL) || (*result_upper_read == NULL));
                /* VERIFIED */
        assert((result_lower_write == NULL) || (*result_lower_write == NULL));
                /* VERIFIED */
        assert((result_upper_write == NULL) || (*result_upper_write == NULL));
                /* VERIFIED */
        assert(type_is_valid(element_lower_read)); /* VERIFIED */
        assert(type_is_valid(element_upper_read)); /* VERIFIED */
        assert(type_is_valid(element_lower_write)); /* VERIFIED */
        assert(type_is_valid(element_upper_write)); /* VERIFIED */
        assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
        find_lookup_types_from(element_lower_read, element_upper_read,
                element_lower_write, element_upper_write, base_base_variable,
                actuals, overload_base, update_function, update_data, FALSE,
                result_lower_read, result_upper_read, result_lower_write,
                result_upper_write, result_base_variable, start + 1, location,
                the_jumper);
        assert((result_lower_read == NULL) || (*result_lower_read == NULL) ||
               type_is_valid(*result_lower_read)); /* VERIFIED */
        assert((result_upper_read == NULL) || (*result_upper_read == NULL) ||
               type_is_valid(*result_upper_read)); /* VERIFIED */
        assert((result_lower_write == NULL) || (*result_lower_write == NULL) ||
               type_is_valid(*result_lower_write)); /* VERIFIED */
        assert((result_upper_write == NULL) || (*result_upper_write == NULL) ||
               type_is_valid(*result_upper_write)); /* VERIFIED */
        type_remove_reference(element_lower_read, the_jumper);
        type_remove_reference(element_upper_read, the_jumper);
        type_remove_reference(element_lower_write, the_jumper);
        type_remove_reference(element_upper_write, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            goto cleanup;
      }
    else
      {
        type *internal_key_lower_read;
        type *internal_key_upper_read;
        type *internal_key_lower_write;
        type *internal_key_upper_write;
        type *external_key_lower_read;
        type *external_key_upper_read;
        type *external_key_lower_write;
        type *external_key_upper_write;
        type *element_lower_read;
        type *element_upper_read;
        type *element_lower_write;
        type *element_upper_write;
        type *target_lower_read;
        type *target_upper_read;
        type *target_lower_write;
        type *target_upper_write;

        if (key != NULL)
          {
            o_integer lower_oi;
            o_integer upper_oi;
            o_integer diff_oi;

            assert(key != NULL);
            assert(upper_bound != NULL);
            assert(filter == NULL);

            assert((get_value_kind(key) == VK_INTEGER) &&
                   (get_value_kind(upper_bound) == VK_INTEGER));

            lower_oi = integer_value_data(key);
            upper_oi = integer_value_data(upper_bound);

            assert(!(oi_out_of_memory(lower_oi)));
            assert(!(oi_out_of_memory(upper_oi)));

            internal_key_lower_read =
                    get_integer_range_type(lower_oi, upper_oi, TRUE, TRUE);
            assert((internal_key_lower_read == NULL) ||
                   type_is_valid(internal_key_lower_read)); /* VERIFIED */
            if (internal_key_lower_read == NULL)
              {
                jumper_do_abort(the_jumper);
                goto cleanup;
              }
            assert(type_is_valid(internal_key_lower_read)); /* VERIFIED */

            oi_subtract(diff_oi, upper_oi, lower_oi);
            if (oi_out_of_memory(diff_oi))
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(internal_key_lower_read, the_jumper);
                goto cleanup;
              }

            external_key_lower_read =
                    get_integer_range_type(oi_zero, diff_oi, TRUE, TRUE);
            oi_remove_reference(diff_oi);
            if (external_key_lower_read == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(internal_key_lower_read, the_jumper);
                goto cleanup;
              }
            assert(type_is_valid(external_key_lower_read)); /* VERIFIED */

            assert(type_is_valid(internal_key_lower_read)); /* VERIFIED */
            type_add_reference(internal_key_lower_read);
            internal_key_upper_read = internal_key_lower_read;
            assert(type_is_valid(internal_key_upper_read)); /* VERIFIED */
            type_add_reference(internal_key_lower_read);
            internal_key_lower_write = internal_key_lower_read;
            assert(type_is_valid(internal_key_lower_write)); /* VERIFIED */
            type_add_reference(internal_key_lower_read);
            internal_key_upper_write = internal_key_lower_read;
            assert(type_is_valid(internal_key_upper_write)); /* VERIFIED */

            type_add_reference(external_key_lower_read);
            external_key_upper_read = external_key_lower_read;
            assert(type_is_valid(external_key_upper_read)); /* VERIFIED */
            type_add_reference(external_key_lower_read);
            external_key_lower_write = external_key_lower_read;
            assert(type_is_valid(external_key_lower_write)); /* VERIFIED */
            type_add_reference(external_key_lower_read);
            external_key_upper_write = external_key_lower_read;
            assert(type_is_valid(external_key_upper_write)); /* VERIFIED */
          }
        else
          {
            assert(key == NULL);
            assert(upper_bound == NULL);

            assert(type_is_valid(base_lower_read)); /* VERIFIED */
            internal_key_lower_read =
                    get_map_key_type(base_lower_read, LOU_LOWER, the_jumper);
            assert((internal_key_lower_read == NULL) ||
                   type_is_valid(internal_key_lower_read)); /* VERIFIED */
            if (internal_key_lower_read == NULL)
              {
                assert(!(jumper_flowing_forward(the_jumper)));
                goto cleanup;
              }
            assert(jumper_flowing_forward(the_jumper));
            assert(type_is_valid(internal_key_lower_read)); /* VERIFIED */

            assert(type_is_valid(base_upper_read)); /* VERIFIED */
            internal_key_upper_read =
                    get_map_key_type(base_upper_read, LOU_UPPER, the_jumper);
            assert((internal_key_upper_read == NULL) ||
                   type_is_valid(internal_key_upper_read)); /* VERIFIED */
            if (internal_key_upper_read == NULL)
              {
                assert(!(jumper_flowing_forward(the_jumper)));
                type_remove_reference(internal_key_lower_read, the_jumper);
                goto cleanup;
              }
            assert(jumper_flowing_forward(the_jumper));
            assert(type_is_valid(internal_key_upper_read)); /* VERIFIED */

            assert(type_is_valid(base_lower_write)); /* VERIFIED */
            internal_key_lower_write =
                    get_map_key_type(base_lower_write, LOU_LOWER, the_jumper);
            assert((internal_key_lower_write == NULL) ||
                   type_is_valid(internal_key_lower_write)); /* VERIFIED */
            if (internal_key_lower_write == NULL)
              {
                assert(!(jumper_flowing_forward(the_jumper)));
                type_remove_reference(internal_key_lower_read, the_jumper);
                type_remove_reference(internal_key_upper_read, the_jumper);
                goto cleanup;
              }
            assert(jumper_flowing_forward(the_jumper));
            assert(type_is_valid(internal_key_lower_write)); /* VERIFIED */

            assert(type_is_valid(base_upper_write)); /* VERIFIED */
            internal_key_upper_write =
                    get_map_key_type(base_upper_write, LOU_UPPER, the_jumper);
            assert((internal_key_upper_write == NULL) ||
                   type_is_valid(internal_key_upper_write)); /* VERIFIED */
            if (internal_key_upper_write == NULL)
              {
                assert(!(jumper_flowing_forward(the_jumper)));
                type_remove_reference(internal_key_lower_read, the_jumper);
                type_remove_reference(internal_key_upper_read, the_jumper);
                type_remove_reference(internal_key_lower_write, the_jumper);
                goto cleanup;
              }
            assert(jumper_flowing_forward(the_jumper));
            assert(type_is_valid(internal_key_upper_write)); /* VERIFIED */

            if (filter != NULL)
              {
                type *new_type;

                assert(type_is_valid(internal_key_lower_read)); /* VERIFIED */
                assert(type_is_valid(filter)); /* VERIFIED */
                new_type =
                        get_intersection_type(internal_key_lower_read, filter);
                if (new_type == NULL)
                  {
                    type_remove_reference(internal_key_lower_read, the_jumper);
                    type_remove_reference(internal_key_upper_read, the_jumper);
                    type_remove_reference(internal_key_lower_write,
                                          the_jumper);
                    type_remove_reference(internal_key_upper_write,
                                          the_jumper);
                    goto cleanup;
                  }
                type_remove_reference(internal_key_lower_read, the_jumper);
                internal_key_lower_read = new_type;
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    type_remove_reference(internal_key_lower_read, the_jumper);
                    type_remove_reference(internal_key_upper_read, the_jumper);
                    type_remove_reference(internal_key_lower_write,
                                          the_jumper);
                    type_remove_reference(internal_key_upper_write,
                                          the_jumper);
                    goto cleanup;
                  }

                assert(type_is_valid(internal_key_upper_read)); /* VERIFIED */
                assert(type_is_valid(filter)); /* VERIFIED */
                new_type =
                        get_intersection_type(internal_key_upper_read, filter);
                if (new_type == NULL)
                  {
                    type_remove_reference(internal_key_lower_read, the_jumper);
                    type_remove_reference(internal_key_upper_read, the_jumper);
                    type_remove_reference(internal_key_lower_write,
                                          the_jumper);
                    type_remove_reference(internal_key_upper_write,
                                          the_jumper);
                    goto cleanup;
                  }
                type_remove_reference(internal_key_upper_read, the_jumper);
                internal_key_upper_read = new_type;
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    type_remove_reference(internal_key_lower_read, the_jumper);
                    type_remove_reference(internal_key_upper_read, the_jumper);
                    type_remove_reference(internal_key_lower_write,
                                          the_jumper);
                    type_remove_reference(internal_key_upper_write,
                                          the_jumper);
                    goto cleanup;
                  }

                assert(type_is_valid(internal_key_lower_write)); /* VERIFIED */
                assert(type_is_valid(filter)); /* VERIFIED */
                new_type = get_intersection_type(internal_key_lower_write,
                                                 filter);
                if (new_type == NULL)
                  {
                    type_remove_reference(internal_key_lower_read, the_jumper);
                    type_remove_reference(internal_key_upper_read, the_jumper);
                    type_remove_reference(internal_key_lower_write,
                                          the_jumper);
                    type_remove_reference(internal_key_upper_write,
                                          the_jumper);
                    goto cleanup;
                  }
                type_remove_reference(internal_key_lower_write, the_jumper);
                internal_key_lower_write = new_type;
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    type_remove_reference(internal_key_lower_read, the_jumper);
                    type_remove_reference(internal_key_upper_read, the_jumper);
                    type_remove_reference(internal_key_lower_write,
                                          the_jumper);
                    type_remove_reference(internal_key_upper_write,
                                          the_jumper);
                    goto cleanup;
                  }

                assert(type_is_valid(internal_key_upper_write)); /* VERIFIED */
                assert(type_is_valid(filter)); /* VERIFIED */
                new_type = get_intersection_type(internal_key_upper_write,
                                                 filter);
                if (new_type == NULL)
                  {
                    type_remove_reference(internal_key_lower_read, the_jumper);
                    type_remove_reference(internal_key_upper_read, the_jumper);
                    type_remove_reference(internal_key_lower_write,
                                          the_jumper);
                    type_remove_reference(internal_key_upper_write,
                                          the_jumper);
                    goto cleanup;
                  }
                type_remove_reference(internal_key_upper_write, the_jumper);
                internal_key_upper_write = new_type;
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    type_remove_reference(internal_key_lower_read, the_jumper);
                    type_remove_reference(internal_key_upper_read, the_jumper);
                    type_remove_reference(internal_key_lower_write,
                                          the_jumper);
                    type_remove_reference(internal_key_upper_write,
                                          the_jumper);
                    goto cleanup;
                  }
              }

            type_add_reference(internal_key_lower_read);
            external_key_lower_read = internal_key_lower_read;
            assert(type_is_valid(external_key_lower_read)); /* VERIFIED */

            type_add_reference(internal_key_upper_read);
            external_key_upper_read = internal_key_upper_read;
            assert(type_is_valid(external_key_upper_read)); /* VERIFIED */

            type_add_reference(internal_key_lower_write);
            external_key_lower_write = internal_key_lower_write;
            assert(type_is_valid(external_key_lower_write)); /* VERIFIED */

            type_add_reference(internal_key_upper_write);
            external_key_upper_write = internal_key_upper_write;
            assert(type_is_valid(external_key_upper_write)); /* VERIFIED */
          }

        assert((result_lower_read == NULL) || (*result_lower_read == NULL));
                /* VERIFIED */
        assert((result_upper_read == NULL) || (*result_upper_read == NULL));
                /* VERIFIED */
        assert((result_lower_write == NULL) || (*result_lower_write == NULL));
                /* VERIFIED */
        assert((result_upper_write == NULL) || (*result_upper_write == NULL));
                /* VERIFIED */

        assert(type_is_valid(base_lower_read)); /* VERIFIED */
        assert(type_is_valid(internal_key_lower_read)); /* VERIFIED */
        element_lower_read = map_target_type_from_key_type(base_lower_read,
                internal_key_lower_read, LOU_LOWER, location, the_jumper);
        assert((element_lower_read == NULL) ||
               type_is_valid(element_lower_read)); /* VERIFIED */
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(element_lower_read == NULL);
            type_remove_reference(internal_key_lower_read, the_jumper);
            type_remove_reference(internal_key_upper_read, the_jumper);
            type_remove_reference(internal_key_lower_write, the_jumper);
            type_remove_reference(internal_key_upper_write, the_jumper);
            type_remove_reference(external_key_lower_read, the_jumper);
            type_remove_reference(external_key_upper_read, the_jumper);
            type_remove_reference(external_key_lower_write, the_jumper);
            type_remove_reference(external_key_upper_write, the_jumper);
            goto cleanup;
          }

        assert(element_lower_read != NULL);
        assert(type_is_valid(element_lower_read)); /* VERIFIED */

        if (overload_lower_read != NULL)
          {
            type *key_type;
            type *additional_element;

            key_type = get_anything_type();
            if (key_type == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(element_lower_read, the_jumper);
                type_remove_reference(internal_key_lower_read, the_jumper);
                type_remove_reference(internal_key_upper_read, the_jumper);
                type_remove_reference(internal_key_lower_write, the_jumper);
                type_remove_reference(internal_key_upper_write, the_jumper);
                type_remove_reference(external_key_lower_read, the_jumper);
                type_remove_reference(external_key_upper_read, the_jumper);
                type_remove_reference(external_key_lower_write, the_jumper);
                type_remove_reference(external_key_upper_write, the_jumper);
                goto cleanup;
              }

            assert(type_is_valid(overload_lower_read)); /* VERIFIED */
            assert(type_is_valid(key_type)); /* VERIFIED */
            additional_element = map_target_type_from_key_type(
                    overload_lower_read, key_type, LOU_LOWER, location,
                    the_jumper);
            assert((additional_element == NULL) ||
                   type_is_valid(additional_element)); /* VERIFIED */
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(additional_element == NULL);
                type_remove_reference(element_lower_read, the_jumper);
                type_remove_reference(internal_key_lower_read, the_jumper);
                type_remove_reference(internal_key_upper_read, the_jumper);
                type_remove_reference(internal_key_lower_write, the_jumper);
                type_remove_reference(internal_key_upper_write, the_jumper);
                type_remove_reference(external_key_lower_read, the_jumper);
                type_remove_reference(external_key_upper_read, the_jumper);
                type_remove_reference(external_key_lower_write, the_jumper);
                type_remove_reference(external_key_upper_write, the_jumper);
                goto cleanup;
              }

            assert(additional_element != NULL);
            assert(type_is_valid(additional_element)); /* VERIFIED */

            if (overload_read_always_hits)
              {
                assert(type_is_valid(additional_element)); /* VERIFIED */
                type_remove_reference(element_lower_read, the_jumper);
                element_lower_read = additional_element;
                assert(type_is_valid(element_lower_read)); /* VERIFIED */
              }
            else
              {
                type *new_type;

                assert(type_is_valid(element_lower_read)); /* VERIFIED */
                assert(type_is_valid(additional_element)); /* VERIFIED */
                new_type =
                        get_union_type(element_lower_read, additional_element);
                assert((new_type == NULL) || type_is_valid(new_type));
                        /* VERIFIED */
                if (new_type == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    type_remove_reference(additional_element, the_jumper);
                    type_remove_reference(element_lower_read, the_jumper);
                    type_remove_reference(internal_key_lower_read, the_jumper);
                    type_remove_reference(internal_key_upper_read, the_jumper);
                    type_remove_reference(internal_key_lower_write,
                                          the_jumper);
                    type_remove_reference(internal_key_upper_write,
                                          the_jumper);
                    type_remove_reference(external_key_lower_read, the_jumper);
                    type_remove_reference(external_key_upper_read, the_jumper);
                    type_remove_reference(external_key_lower_write,
                                          the_jumper);
                    type_remove_reference(external_key_upper_write,
                                          the_jumper);
                    goto cleanup;
                  }
                assert(type_is_valid(new_type)); /* VERIFIED */
                type_remove_reference(additional_element, the_jumper);
                type_remove_reference(element_lower_read, the_jumper);
                element_lower_read = new_type;
                assert(type_is_valid(element_lower_read)); /* VERIFIED */
              }

            if (!(jumper_flowing_forward(the_jumper)))
              {
                type_remove_reference(element_lower_read, the_jumper);
                type_remove_reference(internal_key_lower_read, the_jumper);
                type_remove_reference(internal_key_upper_read, the_jumper);
                type_remove_reference(internal_key_lower_write, the_jumper);
                type_remove_reference(internal_key_upper_write, the_jumper);
                type_remove_reference(external_key_lower_read, the_jumper);
                type_remove_reference(external_key_upper_read, the_jumper);
                type_remove_reference(external_key_lower_write, the_jumper);
                type_remove_reference(external_key_upper_write, the_jumper);
                goto cleanup;
              }
          }

        assert(type_is_valid(base_upper_read)); /* VERIFIED */
        assert(type_is_valid(internal_key_upper_read)); /* VERIFIED */
        element_upper_read = map_target_type_from_key_type(base_upper_read,
                internal_key_upper_read, LOU_UPPER, location, the_jumper);
        assert((element_upper_read == NULL) ||
               type_is_valid(element_upper_read)); /* VERIFIED */
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(element_upper_read == NULL);
            type_remove_reference(element_lower_read, the_jumper);
            type_remove_reference(internal_key_lower_read, the_jumper);
            type_remove_reference(internal_key_upper_read, the_jumper);
            type_remove_reference(internal_key_lower_write, the_jumper);
            type_remove_reference(internal_key_upper_write, the_jumper);
            type_remove_reference(external_key_lower_read, the_jumper);
            type_remove_reference(external_key_upper_read, the_jumper);
            type_remove_reference(external_key_lower_write, the_jumper);
            type_remove_reference(external_key_upper_write, the_jumper);
            goto cleanup;
          }

        assert(element_upper_read != NULL);
        assert(type_is_valid(element_upper_read)); /* VERIFIED */

        if (overload_upper_read != NULL)
          {
            type *key_type;
            type *additional_element;

            key_type = get_anything_type();
            if (key_type == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(element_lower_read, the_jumper);
                type_remove_reference(element_upper_read, the_jumper);
                type_remove_reference(internal_key_lower_read, the_jumper);
                type_remove_reference(internal_key_upper_read, the_jumper);
                type_remove_reference(internal_key_lower_write, the_jumper);
                type_remove_reference(internal_key_upper_write, the_jumper);
                type_remove_reference(external_key_lower_read, the_jumper);
                type_remove_reference(external_key_upper_read, the_jumper);
                type_remove_reference(external_key_lower_write, the_jumper);
                type_remove_reference(external_key_upper_write, the_jumper);
                goto cleanup;
              }

            assert(type_is_valid(overload_upper_read)); /* VERIFIED */
            assert(type_is_valid(key_type)); /* VERIFIED */
            additional_element = map_target_type_from_key_type(
                    overload_upper_read, key_type, LOU_UPPER, location,
                    the_jumper);
            assert((additional_element == NULL) ||
                   type_is_valid(additional_element)); /* VERIFIED */
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(additional_element == NULL);
                type_remove_reference(element_lower_read, the_jumper);
                type_remove_reference(element_upper_read, the_jumper);
                type_remove_reference(internal_key_lower_read, the_jumper);
                type_remove_reference(internal_key_upper_read, the_jumper);
                type_remove_reference(internal_key_lower_write, the_jumper);
                type_remove_reference(internal_key_upper_write, the_jumper);
                type_remove_reference(external_key_lower_read, the_jumper);
                type_remove_reference(external_key_upper_read, the_jumper);
                type_remove_reference(external_key_lower_write, the_jumper);
                type_remove_reference(external_key_upper_write, the_jumper);
                goto cleanup;
              }

            assert(additional_element != NULL);
            assert(type_is_valid(additional_element)); /* VERIFIED */

            if (overload_read_always_hits)
              {
                type_remove_reference(element_upper_read, the_jumper);
                element_upper_read = additional_element;
                assert(type_is_valid(element_upper_read)); /* VERIFIED */
              }
            else
              {
                type *new_type;

                assert(type_is_valid(element_upper_read)); /* VERIFIED */
                assert(type_is_valid(additional_element)); /* VERIFIED */
                new_type =
                        get_union_type(element_upper_read, additional_element);
                assert((new_type == NULL) || type_is_valid(new_type));
                        /* VERIFIED */
                if (new_type == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    type_remove_reference(additional_element, the_jumper);
                    type_remove_reference(element_lower_read, the_jumper);
                    type_remove_reference(element_upper_read, the_jumper);
                    type_remove_reference(internal_key_lower_read, the_jumper);
                    type_remove_reference(internal_key_upper_read, the_jumper);
                    type_remove_reference(internal_key_lower_write,
                                          the_jumper);
                    type_remove_reference(internal_key_upper_write,
                                          the_jumper);
                    type_remove_reference(external_key_lower_read, the_jumper);
                    type_remove_reference(external_key_upper_read, the_jumper);
                    type_remove_reference(external_key_lower_write,
                                          the_jumper);
                    type_remove_reference(external_key_upper_write,
                                          the_jumper);
                    goto cleanup;
                  }
                assert(type_is_valid(new_type)); /* VERIFIED */
                type_remove_reference(additional_element, the_jumper);
                type_remove_reference(element_upper_read, the_jumper);
                element_upper_read = new_type;
                assert(type_is_valid(element_upper_read)); /* VERIFIED */
              }

            if (!(jumper_flowing_forward(the_jumper)))
              {
                type_remove_reference(element_lower_read, the_jumper);
                type_remove_reference(element_upper_read, the_jumper);
                type_remove_reference(internal_key_lower_read, the_jumper);
                type_remove_reference(internal_key_upper_read, the_jumper);
                type_remove_reference(internal_key_lower_write, the_jumper);
                type_remove_reference(internal_key_upper_write, the_jumper);
                type_remove_reference(external_key_lower_read, the_jumper);
                type_remove_reference(external_key_upper_read, the_jumper);
                type_remove_reference(external_key_lower_write, the_jumper);
                type_remove_reference(external_key_upper_write, the_jumper);
                goto cleanup;
              }
          }

        assert(type_is_valid(base_lower_write)); /* VERIFIED */
        assert(type_is_valid(internal_key_lower_write)); /* VERIFIED */
        element_lower_write = map_target_type_from_key_type(base_lower_write,
                internal_key_lower_write, LOU_LOWER, location, the_jumper);
        assert((element_lower_write == NULL) ||
               type_is_valid(element_lower_write)); /* VERIFIED */
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(element_lower_write == NULL);
            type_remove_reference(element_lower_read, the_jumper);
            type_remove_reference(element_upper_read, the_jumper);
            type_remove_reference(internal_key_lower_read, the_jumper);
            type_remove_reference(internal_key_upper_read, the_jumper);
            type_remove_reference(internal_key_lower_write, the_jumper);
            type_remove_reference(internal_key_upper_write, the_jumper);
            type_remove_reference(external_key_lower_read, the_jumper);
            type_remove_reference(external_key_upper_read, the_jumper);
            type_remove_reference(external_key_lower_write, the_jumper);
            type_remove_reference(external_key_upper_write, the_jumper);
            goto cleanup;
          }

        assert(element_lower_write != NULL);
        assert(type_is_valid(element_lower_write)); /* VERIFIED */

        assert(type_is_valid(base_upper_write)); /* VERIFIED */
        assert(type_is_valid(internal_key_upper_write)); /* VERIFIED */
        element_upper_write = map_target_type_from_key_type(base_upper_write,
                internal_key_upper_write, LOU_UPPER, location, the_jumper);
        assert((element_upper_write == NULL) ||
               type_is_valid(element_upper_write)); /* VERIFIED */
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(element_upper_write == NULL);
            type_remove_reference(element_lower_read, the_jumper);
            type_remove_reference(element_upper_read, the_jumper);
            type_remove_reference(element_lower_write, the_jumper);
            type_remove_reference(internal_key_lower_read, the_jumper);
            type_remove_reference(internal_key_upper_read, the_jumper);
            type_remove_reference(internal_key_lower_write, the_jumper);
            type_remove_reference(internal_key_upper_write, the_jumper);
            type_remove_reference(external_key_lower_read, the_jumper);
            type_remove_reference(external_key_upper_read, the_jumper);
            type_remove_reference(external_key_lower_write, the_jumper);
            type_remove_reference(external_key_upper_write, the_jumper);
            goto cleanup;
          }

        assert(element_upper_write != NULL);
        assert(type_is_valid(element_upper_write)); /* VERIFIED */

        assert(type_is_valid(element_lower_read)); /* VERIFIED */
        assert(type_is_valid(element_upper_read)); /* VERIFIED */
        assert(type_is_valid(element_lower_write)); /* VERIFIED */
        assert(type_is_valid(element_upper_write)); /* VERIFIED */
        assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
        find_lookup_types_from(element_lower_read, element_upper_read,
                element_lower_write, element_upper_write, base_base_variable,
                actuals, overload_base, update_function, update_data, FALSE,
                &target_lower_read, &target_upper_read, &target_lower_write,
                &target_upper_write, result_base_variable, start + 1, location,
                the_jumper);
        assert((target_lower_read == NULL) ||
               type_is_valid(target_lower_read)); /* VERIFIED */
        assert((target_upper_read == NULL) ||
               type_is_valid(target_upper_read)); /* VERIFIED */
        assert((target_lower_write == NULL) ||
               type_is_valid(target_lower_write)); /* VERIFIED */
        assert((target_upper_write == NULL) ||
               type_is_valid(target_upper_write)); /* VERIFIED */
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(target_lower_read == NULL);
            assert(target_upper_read == NULL);
            assert(target_lower_write == NULL);
            assert(target_upper_write == NULL);

            type_remove_reference(element_lower_read, the_jumper);
            type_remove_reference(element_upper_read, the_jumper);
            type_remove_reference(element_lower_write, the_jumper);
            type_remove_reference(element_upper_write, the_jumper);
            type_remove_reference(internal_key_lower_read, the_jumper);
            type_remove_reference(internal_key_upper_read, the_jumper);
            type_remove_reference(internal_key_lower_write, the_jumper);
            type_remove_reference(internal_key_upper_write, the_jumper);
            type_remove_reference(external_key_lower_read, the_jumper);
            type_remove_reference(external_key_upper_read, the_jumper);
            type_remove_reference(external_key_lower_write, the_jumper);
            type_remove_reference(external_key_upper_write, the_jumper);
            goto cleanup;
          }

        assert(target_lower_read != NULL);
        assert(target_upper_read != NULL);
        assert(target_lower_write != NULL);
        assert(target_upper_write != NULL);

        type_remove_reference(internal_key_lower_read, the_jumper);
        type_remove_reference(internal_key_upper_read, the_jumper);
        type_remove_reference(internal_key_lower_write, the_jumper);
        type_remove_reference(internal_key_upper_write, the_jumper);
        type_remove_reference(element_lower_read, the_jumper);
        type_remove_reference(element_upper_read, the_jumper);
        type_remove_reference(element_lower_write, the_jumper);
        type_remove_reference(element_upper_write, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(target_lower_read, the_jumper);
            type_remove_reference(target_upper_read, the_jumper);
            type_remove_reference(target_lower_write, the_jumper);
            type_remove_reference(target_upper_write, the_jumper);
            type_remove_reference(external_key_lower_read, the_jumper);
            type_remove_reference(external_key_upper_read, the_jumper);
            type_remove_reference(external_key_lower_write, the_jumper);
            type_remove_reference(external_key_upper_write, the_jumper);
            goto cleanup;
          }

        if (result_lower_read != NULL)
          {
            assert(*result_lower_read == NULL); /* VERIFIED */
            assert(type_is_valid(external_key_lower_read)); /* VERIFIED */
            assert(type_is_valid(target_lower_read)); /* VERIFIED */
            *result_lower_read =
                    get_map_type(external_key_lower_read, target_lower_read);
            assert((*result_lower_read == NULL) ||
                   type_is_valid(*result_lower_read)); /* VERIFIED */
            if (*result_lower_read == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(target_lower_read, the_jumper);
                type_remove_reference(target_upper_read, the_jumper);
                type_remove_reference(target_lower_write, the_jumper);
                type_remove_reference(target_upper_write, the_jumper);
                type_remove_reference(external_key_lower_read, the_jumper);
                type_remove_reference(external_key_upper_read, the_jumper);
                type_remove_reference(external_key_lower_write, the_jumper);
                type_remove_reference(external_key_upper_write, the_jumper);
                goto cleanup;
              }
            assert(type_is_valid(*result_lower_read)); /* VERIFIED */
          }
        assert((result_lower_read == NULL) ||
               type_is_valid(*result_lower_read)); /* VERIFIED */

        type_remove_reference(external_key_lower_read, the_jumper);
        type_remove_reference(target_lower_read, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(target_upper_read, the_jumper);
            type_remove_reference(target_lower_write, the_jumper);
            type_remove_reference(target_upper_write, the_jumper);
            type_remove_reference(external_key_upper_read, the_jumper);
            type_remove_reference(external_key_lower_write, the_jumper);
            type_remove_reference(external_key_upper_write, the_jumper);
            goto cleanup;
          }

        if (result_upper_read != NULL)
          {
            assert(*result_upper_read == NULL); /* VERIFIED */
            assert(type_is_valid(external_key_upper_read)); /* VERIFIED */
            assert(type_is_valid(target_upper_read)); /* VERIFIED */
            *result_upper_read =
                    get_map_type(external_key_upper_read, target_upper_read);
            assert((*result_upper_read == NULL) ||
                   type_is_valid(*result_upper_read)); /* VERIFIED */
            if (*result_upper_read == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(target_upper_read, the_jumper);
                type_remove_reference(target_lower_write, the_jumper);
                type_remove_reference(target_upper_write, the_jumper);
                type_remove_reference(external_key_upper_read, the_jumper);
                type_remove_reference(external_key_lower_write, the_jumper);
                type_remove_reference(external_key_upper_write, the_jumper);
                goto cleanup;
              }
            assert(type_is_valid(*result_upper_read)); /* VERIFIED */
          }
        assert((result_upper_read == NULL) ||
               type_is_valid(*result_upper_read)); /* VERIFIED */

        type_remove_reference(external_key_upper_read, the_jumper);
        type_remove_reference(target_upper_read, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(target_lower_write, the_jumper);
            type_remove_reference(target_upper_write, the_jumper);
            type_remove_reference(external_key_lower_write, the_jumper);
            type_remove_reference(external_key_upper_write, the_jumper);
            goto cleanup;
          }

        if (result_lower_write != NULL)
          {
            assert(*result_lower_write == NULL); /* VERIFIED */
            assert(type_is_valid(external_key_lower_write)); /* VERIFIED */
            assert(type_is_valid(target_lower_write)); /* VERIFIED */
            *result_lower_write =
                    get_map_type(external_key_lower_write, target_lower_write);
            assert((*result_lower_write == NULL) ||
                   type_is_valid(*result_lower_write)); /* VERIFIED */
            if (*result_lower_write == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(target_lower_write, the_jumper);
                type_remove_reference(target_upper_write, the_jumper);
                type_remove_reference(external_key_lower_write, the_jumper);
                type_remove_reference(external_key_upper_write, the_jumper);
                goto cleanup;
              }
            assert(type_is_valid(*result_lower_write)); /* VERIFIED */
          }
        assert((result_lower_write == NULL) ||
               type_is_valid(*result_lower_write)); /* VERIFIED */

        type_remove_reference(external_key_lower_write, the_jumper);
        type_remove_reference(target_lower_write, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(target_upper_write, the_jumper);
            type_remove_reference(external_key_upper_write, the_jumper);
            goto cleanup;
          }

        if (result_upper_write != NULL)
          {
            assert(*result_upper_write == NULL); /* VERIFIED */
            assert(type_is_valid(external_key_upper_write)); /* VERIFIED */
            assert(type_is_valid(target_upper_write)); /* VERIFIED */
            *result_upper_write =
                    get_map_type(external_key_upper_write, target_upper_write);
            assert((*result_upper_write == NULL) ||
                   type_is_valid(*result_upper_write)); /* VERIFIED */
            if (*result_upper_write == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(target_upper_write, the_jumper);
                type_remove_reference(external_key_upper_write, the_jumper);
                goto cleanup;
              }
            assert(type_is_valid(*result_upper_write)); /* VERIFIED */
          }
        assert((result_upper_write == NULL) ||
               type_is_valid(*result_upper_write)); /* VERIFIED */

        type_remove_reference(external_key_upper_write, the_jumper);
        type_remove_reference(target_upper_write, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            goto cleanup;

        assert((result_lower_read == NULL) ||
               type_is_valid(*result_lower_read)); /* VERIFIED */
        assert((result_upper_read == NULL) ||
               type_is_valid(*result_upper_read)); /* VERIFIED */
        assert((result_lower_write == NULL) ||
               type_is_valid(*result_lower_write)); /* VERIFIED */
        assert((result_upper_write == NULL) ||
               type_is_valid(*result_upper_write)); /* VERIFIED */
      }

    assert((result_lower_read == NULL) || (*result_lower_read == NULL) ||
           type_is_valid(*result_lower_read)); /* VERIFIED */
    assert((result_upper_read == NULL) || (*result_upper_read == NULL) ||
           type_is_valid(*result_upper_read)); /* VERIFIED */
    assert((result_lower_write == NULL) || (*result_lower_write == NULL) ||
           type_is_valid(*result_lower_write)); /* VERIFIED */
    assert((result_upper_write == NULL) || (*result_upper_write == NULL) ||
           type_is_valid(*result_upper_write)); /* VERIFIED */

    if ((additional_lower_write != NULL) && (result_lower_write != NULL))
      {
        if (overload_write_always_hits)
          {
            assert(type_is_valid(additional_lower_write)); /* VERIFIED */
            type_add_reference(additional_lower_write);
            type_remove_reference(*result_lower_write, the_jumper);
            *result_lower_write = additional_lower_write;
            assert(type_is_valid(*result_lower_write)); /* VERIFIED */
          }
        else
          {
            type *new_result_type;

            assert(type_is_valid(*result_lower_write)); /* VERIFIED */
            assert(type_is_valid(additional_lower_write)); /* VERIFIED */
            new_result_type = get_union_type(*result_lower_write,
                                             additional_lower_write);
            assert((new_result_type == NULL) ||
                   type_is_valid(new_result_type)); /* VERIFIED */
            if (new_result_type == NULL)
              {
                jumper_do_abort(the_jumper);
                goto cleanup;
              }
            assert(type_is_valid(new_result_type)); /* VERIFIED */
            type_remove_reference(*result_lower_write, the_jumper);
            *result_lower_write = new_result_type;
            assert(type_is_valid(*result_lower_write)); /* VERIFIED */
          }
        assert(type_is_valid(*result_lower_write)); /* VERIFIED */
      }
    assert((result_lower_write == NULL) || (*result_lower_write == NULL) ||
           type_is_valid(*result_lower_write)); /* VERIFIED */

    if ((additional_upper_write != NULL) && (result_upper_write != NULL))
      {
        if (overload_write_always_hits)
          {
            assert(type_is_valid(additional_upper_write)); /* VERIFIED */
            type_add_reference(additional_upper_write);
            type_remove_reference(*result_upper_write, the_jumper);
            *result_upper_write = additional_upper_write;
            assert(type_is_valid(*result_upper_write)); /* VERIFIED */
          }
        else
          {
            type *new_result_type;

            assert(type_is_valid(*result_upper_write)); /* VERIFIED */
            assert(type_is_valid(additional_upper_write)); /* VERIFIED */
            new_result_type = get_union_type(*result_upper_write,
                                             additional_upper_write);
            assert((new_result_type != NULL) ||
                   type_is_valid(new_result_type)); /* VERIFIED */
            if (new_result_type == NULL)
              {
                jumper_do_abort(the_jumper);
                goto cleanup;
              }
            assert(type_is_valid(new_result_type)); /* VERIFIED */
            type_remove_reference(*result_upper_write, the_jumper);
            *result_upper_write = new_result_type;
            assert(type_is_valid(*result_upper_write)); /* VERIFIED */
          }
        assert(type_is_valid(*result_upper_write)); /* VERIFIED */
      }
    assert((result_upper_write == NULL) || (*result_upper_write == NULL) ||
           type_is_valid(*result_upper_write)); /* VERIFIED */

    if (no_base_variable && (result_base_variable != NULL))
        *result_base_variable = NULL;

    assert((result_lower_read == NULL) || (*result_lower_read == NULL) ||
           type_is_valid(*result_lower_read)); /* VERIFIED */
    assert((result_upper_read == NULL) || (*result_upper_read == NULL) ||
           type_is_valid(*result_upper_read)); /* VERIFIED */
    assert((result_lower_write == NULL) || (*result_lower_write == NULL) ||
           type_is_valid(*result_lower_write)); /* VERIFIED */
    assert((result_upper_write == NULL) || (*result_upper_write == NULL) ||
           type_is_valid(*result_upper_write)); /* VERIFIED */

  cleanup:
    assert((result_lower_read == NULL) || (*result_lower_read == NULL) ||
           type_is_valid(*result_lower_read)); /* VERIFIED */
    assert((result_upper_read == NULL) || (*result_upper_read == NULL) ||
           type_is_valid(*result_upper_read)); /* VERIFIED */
    assert((result_lower_write == NULL) || (*result_lower_write == NULL) ||
           type_is_valid(*result_lower_write)); /* VERIFIED */
    assert((result_upper_write == NULL) || (*result_upper_write == NULL) ||
           type_is_valid(*result_upper_write)); /* VERIFIED */
    if (overload_lower_read != NULL)
        type_remove_reference(overload_lower_read, the_jumper);
    if (overload_upper_read != NULL)
        type_remove_reference(overload_upper_read, the_jumper);
    if (overload_lower_write != NULL)
        type_remove_reference(overload_lower_write, the_jumper);
    if (overload_upper_write != NULL)
        type_remove_reference(overload_upper_write, the_jumper);
    if (additional_lower_write != NULL)
        type_remove_reference(additional_lower_write, the_jumper);
    if (additional_upper_write != NULL)
        type_remove_reference(additional_upper_write, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if ((result_lower_read != NULL) && (*result_lower_read != NULL))
          {
            type_remove_reference(*result_lower_read, the_jumper);
            *result_lower_read = NULL;
          }
        if ((result_upper_read != NULL) && (*result_upper_read != NULL))
          {
            type_remove_reference(*result_upper_read, the_jumper);
            *result_upper_read = NULL;
          }
        if ((result_lower_write != NULL) && (*result_lower_write != NULL))
          {
            type_remove_reference(*result_lower_write, the_jumper);
            *result_lower_write = NULL;
          }
        if ((result_upper_write != NULL) && (*result_upper_write != NULL))
          {
            type_remove_reference(*result_upper_write, the_jumper);
            *result_upper_write = NULL;
          }
        if ((result_base_variable != NULL) && (*result_base_variable != NULL))
            *result_base_variable = NULL;
      }

    assert((result_lower_read == NULL) || (*result_lower_read == NULL) ||
           type_is_valid(*result_lower_read)); /* VERIFIED */
    assert((result_upper_read == NULL) || (*result_upper_read == NULL) ||
           type_is_valid(*result_upper_read)); /* VERIFIED */
    assert((result_lower_write == NULL) || (*result_lower_write == NULL) ||
           type_is_valid(*result_lower_write)); /* VERIFIED */
    assert((result_upper_write == NULL) || (*result_upper_write == NULL) ||
           type_is_valid(*result_upper_write)); /* VERIFIED */
  }

static difference_status find_actuals_difference(lookup_actual_arguments *left,
        lookup_actual_arguments *right, const source_location *location,
        jumper *the_jumper, o_integer *difference_oi, int *order)
  {
    size_t dimensions;
    size_t number;
    lookup_item *left_final_item;
    lookup_item *right_final_item;
    value *left_final_key;
    value *right_final_key;
    boolean final_key_doubt;
    boolean final_key_match;
    o_integer left_final_oi;
    o_integer right_final_oi;

    assert(left != NULL);
    assert(right != NULL);
    assert(the_jumper != NULL);

    assert(lookup_actual_arguments_is_valid(left)); /* VERIFIED */
    assert(lookup_actual_arguments_is_valid(right)); /* VERIFIED */

    dimensions = left->dimensions;
    if (dimensions != right->dimensions)
        return DS_DIMENSION_MISMATCH;

    if (dimensions == 0)
      {
      return_same:
        if (difference_oi != NULL)
          {
            oi_add_reference(oi_zero);
            *difference_oi = oi_zero;
            assert(!(oi_out_of_memory(*difference_oi)));
          }

        if (order != NULL)
            *order = 0;

        return DS_OK;
      }

    for (number = 0; number < (dimensions - 1); ++number)
      {
        lookup_item *left_item;
        lookup_item *right_item;

        left_item = &(left->array[number]);
        right_item = &(right->array[number]);

        if ((left_item->key != NULL) != (right_item->key != NULL))
            return DS_LOWER_DIMENSION_MISMATCH;

        if (left_item->key != NULL)
          {
            boolean doubt;
            boolean match;

            assert(value_is_valid(left_item->key)); /* VERIFIED */
            assert(value_is_valid(right_item->key)); /* VERIFIED */
            match = values_are_equal(left_item->key, right_item->key, &doubt,
                                     location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return DS_JUMP_BACK;
            if (doubt)
                return DS_LOWER_DIMENSION_DOUBT;
            if (!match)
                return DS_LOWER_DIMENSION_MISMATCH;
          }

        if ((left_item->upper_bound != NULL) !=
            (right_item->upper_bound != NULL))
          {
            return DS_LOWER_DIMENSION_MISMATCH;
          }

        if (left_item->upper_bound != NULL)
          {
            boolean doubt;
            boolean match;

            assert(value_is_valid(left_item->upper_bound)); /* VERIFIED */
            assert(value_is_valid(right_item->upper_bound)); /* VERIFIED */
            match = values_are_equal(left_item->upper_bound,
                    right_item->upper_bound, &doubt, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return DS_JUMP_BACK;
            if (doubt)
                return DS_LOWER_DIMENSION_DOUBT;
            if (!match)
                return DS_LOWER_DIMENSION_MISMATCH;
          }

        if ((left_item->filter != NULL) != (right_item->filter != NULL))
            return DS_LOWER_DIMENSION_MISMATCH;

        if (left_item->filter != NULL)
          {
            boolean doubt;
            boolean match;

            assert(type_is_valid(left_item->filter)); /* VERIFIED */
            assert(type_is_valid(right_item->filter)); /* VERIFIED */
            match = types_are_equal(left_item->filter, right_item->filter,
                                    &doubt, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return DS_JUMP_BACK;
            if (doubt)
                return DS_LOWER_DIMENSION_DOUBT;
            if (!match)
                return DS_LOWER_DIMENSION_MISMATCH;
          }
      }

    left_final_item = &(left->array[dimensions - 1]);
    right_final_item = &(right->array[dimensions - 1]);

    if ((left_final_item->filter != NULL) !=
        (right_final_item->filter != NULL))
      {
        return DS_STAR_DIFFERENCE;
      }

    if (left_final_item->filter != NULL)
      {
        boolean doubt;
        boolean match;

        assert(type_is_valid(left_final_item->filter)); /* VERIFIED */
        assert(type_is_valid(right_final_item->filter)); /* VERIFIED */
        match = types_are_equal(left_final_item->filter,
                right_final_item->filter, &doubt, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return DS_JUMP_BACK;
        if (doubt)
            return DS_FINAL_DIMENSION_MATCH_DOUBT;
        if (!match)
            return DS_STAR_DIFFERENCE;
      }

    left_final_key = left_final_item->key;
    assert((left_final_key == NULL) || value_is_valid(left_final_key));
            /* VERIFIED */
    right_final_key = right_final_item->key;
    assert((right_final_key == NULL) || value_is_valid(right_final_key));
            /* VERIFIED */

    if ((left_final_key != NULL) != (right_final_key != NULL))
        return DS_STAR_NON_STAR_DIFFERENCE;

    if (left_final_key == NULL)
        goto return_same;

    if ((left_final_item->upper_bound != NULL) !=
        (right_final_item->upper_bound != NULL))
      {
        return DS_RANGE_NON_RANGE_DIFFERENCE;
      }

    if (left_final_item->upper_bound != NULL)
      {
        o_integer left_lower_oi;
        o_integer right_lower_oi;
        o_integer left_upper_oi;
        o_integer right_upper_oi;
        o_integer lower_diff;
        o_integer upper_diff;
        int diff_order;

        assert(left_final_key != NULL);
        assert(right_final_key != NULL);
        assert(left_final_item->upper_bound != NULL);
        assert(right_final_item->upper_bound != NULL);

        assert(get_value_kind(left_final_key) == VK_INTEGER);
        assert(get_value_kind(right_final_key) == VK_INTEGER);
        assert(get_value_kind(left_final_item->upper_bound) == VK_INTEGER);
        assert(get_value_kind(right_final_item->upper_bound) == VK_INTEGER);

        left_lower_oi = integer_value_data(left_final_key);
        right_lower_oi = integer_value_data(right_final_key);
        left_upper_oi = integer_value_data(left_final_item->upper_bound);
        right_upper_oi = integer_value_data(right_final_item->upper_bound);

        assert(!(oi_out_of_memory(left_lower_oi)));
        assert(!(oi_out_of_memory(right_lower_oi)));
        assert(!(oi_out_of_memory(left_upper_oi)));
        assert(!(oi_out_of_memory(right_upper_oi)));

        oi_subtract(lower_diff, left_lower_oi, right_lower_oi);
        if (oi_out_of_memory(lower_diff))
            return DS_ALLOCATION_ERROR;

        oi_subtract(upper_diff, left_upper_oi, right_upper_oi);
        if (oi_out_of_memory(upper_diff))
          {
            oi_remove_reference(lower_diff);
            return DS_ALLOCATION_ERROR;
          }

        diff_order = oi_structural_order(lower_diff, upper_diff);
        oi_remove_reference(upper_diff);
        if (diff_order == -2)
          {
            oi_remove_reference(lower_diff);
            return DS_ALLOCATION_ERROR;
          }

        if (diff_order != 0)
          {
            oi_remove_reference(lower_diff);
            return DS_INCOMPATIBLE_RANGES;
          }

        if (order != NULL)
          {
            switch (oi_kind(lower_diff))
              {
                case IIK_FINITE:
                    if (oi_equal(left_lower_oi, right_lower_oi))
                        *order = 0;
                    else if (oi_less_than(left_lower_oi, right_lower_oi))
                        *order = -1;
                    else
                        *order = 1;
                    break;
                case IIK_POSITIVE_INFINITY:
                    *order = 1;
                    break;
                case IIK_NEGATIVE_INFINITY:
                    *order = -1;
                    break;
                case IIK_UNSIGNED_INFINITY:
                    return DS_UNDECIDABLE_ORDER;
                case IIK_ZERO_ZERO:
                    return DS_UNDECIDABLE_ORDER;
                default:
                    assert(FALSE);
              }
          }

        if (difference_oi != NULL)
            *difference_oi = lower_diff;
        else
            oi_remove_reference(lower_diff);

        return DS_OK;
      }

    assert(value_is_valid(left_final_key)); /* VERIFIED */
    assert(value_is_valid(right_final_key)); /* VERIFIED */
    final_key_match = values_are_equal(left_final_key, right_final_key,
                                       &final_key_doubt, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return DS_JUMP_BACK;
    if (final_key_doubt)
        return DS_FINAL_DIMENSION_MATCH_DOUBT;

    if (final_key_match)
        goto return_same;

    if ((get_value_kind(left_final_key) != VK_INTEGER) ||
        (get_value_kind(right_final_key) != VK_INTEGER))
      {
        return DS_FINAL_NON_INTEGER;
      }

    left_final_oi = integer_value_data(left_final_key);
    right_final_oi = integer_value_data(right_final_key);

    assert(!(oi_out_of_memory(left_final_oi)));
    assert(!(oi_out_of_memory(right_final_oi)));

    if (order != NULL)
      {
        if ((oi_kind(left_final_oi) == IIK_UNSIGNED_INFINITY) ||
            (oi_kind(left_final_oi) == IIK_ZERO_ZERO) ||
            (oi_kind(right_final_oi) == IIK_UNSIGNED_INFINITY) ||
            (oi_kind(right_final_oi) == IIK_ZERO_ZERO))
          {
            return DS_UNDECIDABLE_ORDER;
          }

        if (oi_equal(left_final_oi, right_final_oi))
            *order = 0;
        else if (oi_less_than(left_final_oi, right_final_oi))
            *order = -1;
        else
            *order = 1;
      }

    if (difference_oi != NULL)
      {
        oi_subtract(*difference_oi, left_final_oi, right_final_oi);
        if (oi_out_of_memory(*difference_oi))
            return DS_ALLOCATION_ERROR;
      }

    return DS_OK;
  }

static value *set_element_value(value *base_value, value *key_value,
        type *key_type, value *new_element, value *overload_base,
        const source_location *location, jumper *the_jumper)
  {
    value *result;
    value *map_value;

    assert(base_value != NULL);
    assert((key_value != NULL) || (key_type != NULL));
    assert((key_value == NULL) || (key_type == NULL));
    assert(location != NULL);
    assert(the_jumper != NULL);

    assert(value_is_valid_except_map_targets(base_value));
            /* VERIFICATION NEEDED */
    assert((key_value == NULL) || value_is_valid(key_value)); /* VERIFIED */
    assert((key_type == NULL) || type_is_valid(key_type)); /* VERIFIED */

    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (new_element != NULL)
            value_remove_reference(new_element, the_jumper);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    if (new_element == NULL)
        return base_value;

    result = try_lookup_overload(base_value, key_value, key_type, NULL,
            overload_base, new_element, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(result == NULL);
        value_remove_reference(new_element, the_jumper);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }
    if (result != NULL)
      {
        value_remove_reference(new_element, the_jumper);
        value_remove_reference(base_value, the_jumper);
        return result;
      }

    assert(value_is_valid_except_map_targets(base_value)); /* VERIFIED */

    switch (get_value_kind(base_value))
      {
        case VK_MAP:
          {
            assert(map_value_all_keys_are_valid(base_value));
                    /* VERIFIED */
            value_add_reference(base_value);
            map_value = base_value;
            assert((map_value == NULL) ||
                   map_value_all_keys_are_valid(map_value)); /* VERIFIED */
            break;
          }
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            map_value = map_value_from_semi_labeled_value_list(base_value);
            assert((map_value == NULL) ||
                   map_value_all_keys_are_valid(map_value)); /* VERIFIED */
            break;
          }
        default:
          {
            map_value = create_map_value();
            assert((map_value == NULL) ||
                   map_value_all_keys_are_valid(map_value)); /* VERIFIED */
          }
      }

    assert((map_value == NULL) || map_value_all_keys_are_valid(map_value));
            /* VERIFIED */

    if (map_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(new_element, the_jumper);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    value_remove_reference(base_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(new_element, the_jumper);
        return NULL;
      }

    assert(map_value_all_keys_are_valid(map_value)); /* VERIFIED */
    if (key_value != NULL)
      {
        assert(key_type == NULL);
        assert(map_value_all_keys_are_valid(map_value)); /* VERIFIED */
        assert(value_is_valid(key_value)); /* VERIFIED */
        map_value = map_value_set(map_value, key_value, new_element, location,
                                  the_jumper);
      }
    else
      {
        assert(key_type != NULL);
        assert(map_value_all_keys_are_valid(map_value)); /* VERIFIED */
        assert(type_is_valid(key_type)); /* VERIFIED */
        map_value = map_value_set_filter(map_value, key_type, new_element,
                                         location, the_jumper);
      }
    value_remove_reference(new_element, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (map_value != NULL)
            value_remove_reference(map_value, the_jumper);
        return NULL;
      }

    return map_value;
  }

static value *try_lookup_overload(value *base_value, value *key_value,
        type *key_type, value *upper_bound, value *overload_base,
        value *new_value, const source_location *location, jumper *the_jumper)
  {
    value *cleanup_needed_value;
    size_t argument_count;
    value *arguments[4];
    const char *argument_names[4];
    value *result;
    verdict the_verdict;

    assert(the_jumper != NULL);

    if (base_value == NULL)
        return NULL;

    if ((get_value_kind(base_value) == VK_MAP) && (overload_base == NULL))
        return NULL;

    if (key_type != NULL)
      {
        assert(key_type != NULL);
        assert(key_value == NULL);
        assert(upper_bound == NULL);

        cleanup_needed_value = create_type_value(key_type);
        if (cleanup_needed_value == NULL)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        argument_count = 1;
        arguments[1] = cleanup_needed_value;
        argument_names[1] = "filter";
      }
    else if ((key_value != NULL) && (upper_bound == NULL))
      {
        assert(key_type == NULL);
        assert(key_value != NULL);
        assert(upper_bound == NULL);

        cleanup_needed_value = NULL;
        argument_count = 1;
        arguments[1] = key_value;
        argument_names[1] = "key";
      }
    else if (key_value != NULL)
      {
        assert(key_type == NULL);
        assert(key_value != NULL);
        assert(upper_bound != NULL);

        cleanup_needed_value = NULL;
        argument_count = 2;
        arguments[1] = key_value;
        argument_names[1] = "lower";
        arguments[2] = upper_bound;
        argument_names[2] = "upper";
      }
    else
      {
        assert(key_type == NULL);
        assert(key_value == NULL);
        assert(upper_bound == NULL);

        cleanup_needed_value = create_string_value("*");
        if (cleanup_needed_value == NULL)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        argument_count = 1;
        arguments[1] = cleanup_needed_value;
        argument_names[1] = "star";
      }

    if (new_value != NULL)
      {
        ++argument_count;
        arguments[argument_count] = new_value;
        argument_names[argument_count] = NULL;
      }

    if (overload_base != NULL)
      {
        verdict the_verdict;

        arguments[0] = base_value;
        argument_names[0] = NULL;
        the_verdict = try_overloading_from_call_base(overload_base, &result,
                argument_count + 1, &(arguments[0]), &(argument_names[0]),
                the_jumper, location);
        if ((!(jumper_flowing_forward(the_jumper))) ||
            (the_verdict == MISSION_ACCOMPLISHED))
          {
            if (cleanup_needed_value != NULL)
                value_remove_reference(cleanup_needed_value, the_jumper);
            return result;
          }
      }

    the_verdict = try_overloading(base_value, "operator[]", &result,
            argument_count, &(arguments[1]), &(argument_names[1]), the_jumper,
            location);
    if (cleanup_needed_value != NULL)
        value_remove_reference(cleanup_needed_value, the_jumper);
    if (the_verdict == MISSION_ACCOMPLISHED)
        return result;

    return NULL;
  }

static void lookup_overload_type(type *base_lower, type *base_upper,
        value *key, value *upper_bound, type *filter, value *overload_base,
        boolean is_write, type *write_lower_requirement,
        type *write_upper_requirement, boolean one_element,
        boolean *always_hits, boolean *never_hits, type **lower_result,
        type **upper_result, const source_location *location,
        jumper *the_jumper)
  {
    value *star_value;
    type *key_lower;
    size_t argument_count;
    parameter_pattern_kind parameter_pattern_kinds[4];
    const char *parameter_names[4];
    value *exact_parameters[4];
    type *parameter_lower_types[4];
    type *parameter_upper_types[4];
    size_t map_key_argument_number;
    type *operator_lower;
    type *operator_upper;
    type *overload_lower;
    type *overload_upper;
    boolean local_always_hits;
    boolean local_never_hits;
    type *new_lower;
    type *new_upper;

    assert(base_lower != NULL);
    assert(base_upper != NULL);
    assert(lower_result != NULL);
    assert(upper_result != NULL);

    assert(type_is_valid(base_lower)); /* VERIFIED */
    assert(type_is_valid(base_upper)); /* VERIFIED */
    assert((key == NULL) || value_is_valid(key)); /* VERIFIED */
    assert((filter == NULL) || type_is_valid(filter)); /* VERIFIED */
    assert((write_lower_requirement == NULL) ||
           type_is_valid(write_lower_requirement)); /* VERIFIED */
    assert((write_upper_requirement == NULL) ||
           type_is_valid(write_upper_requirement)); /* VERIFIED */

    *lower_result = NULL;
    *upper_result = NULL;

    *always_hits = FALSE;
    *never_hits = TRUE;

    if (filter != NULL)
      {
        assert(key == NULL);
        assert(upper_bound == NULL);

        if (one_element)
          {
            star_value = NULL;
            key_lower = NULL;

            argument_count = 1;
            parameter_pattern_kinds[1] = PPK_TYPE;
            parameter_names[1] = "key";
            exact_parameters[1] = NULL;
            parameter_lower_types[1] = filter;
            parameter_upper_types[1] = filter;
            map_key_argument_number = 0;
          }
        else
          {
            key_lower = NULL;

            star_value = create_type_value(filter);
            if (star_value == NULL)
              {
                jumper_do_abort(the_jumper);
                assert(*lower_result == NULL); /* VERIFIED */
                assert(*upper_result == NULL); /* VERIFIED */
                return;
              }

            argument_count = 1;
            parameter_pattern_kinds[1] = PPK_EXACT;
            parameter_names[1] = "filter";
            assert(value_is_valid(star_value)); /* VERIFIED */
            exact_parameters[1] = star_value;
            parameter_lower_types[1] = NULL;
            parameter_upper_types[1] = NULL;
            map_key_argument_number = argument_count;
          }
      }
    else if ((key != NULL) && (upper_bound == NULL))
      {
        star_value = NULL;
        key_lower = NULL;
        argument_count = 1;
        parameter_pattern_kinds[1] = PPK_EXACT;
        parameter_names[1] = "key";
        assert(value_is_valid(key)); /* VERIFIED */
        exact_parameters[1] = key;
        parameter_lower_types[1] = NULL;
        parameter_upper_types[1] = NULL;
        map_key_argument_number = argument_count;
      }
    else if (key != NULL)
      {
        assert(key != NULL);
        assert(upper_bound != NULL);
        assert(filter == NULL);

        star_value = NULL;

        if (one_element)
          {
            o_integer lower_oi;
            o_integer upper_oi;

            assert((get_value_kind(key) == VK_INTEGER) &&
                   (get_value_kind(upper_bound) == VK_INTEGER));

            lower_oi = integer_value_data(key);
            upper_oi = integer_value_data(upper_bound);

            assert(!(oi_out_of_memory(lower_oi)));
            assert(!(oi_out_of_memory(upper_oi)));

            key_lower = get_integer_range_type(lower_oi, upper_oi, TRUE, TRUE);
            if (key_lower == NULL)
              {
                jumper_do_abort(the_jumper);
                assert(*lower_result == NULL); /* VERIFIED */
                assert(*upper_result == NULL); /* VERIFIED */
                return;
              }

            argument_count = 1;
            parameter_pattern_kinds[1] = PPK_TYPE;
            parameter_names[1] = "key";
            exact_parameters[1] = NULL;
            parameter_lower_types[1] = key_lower;
            parameter_upper_types[1] = key_lower;
            map_key_argument_number = 0;
          }
        else
          {
            key_lower = NULL;

            argument_count = 2;
            parameter_pattern_kinds[1] = PPK_EXACT;
            parameter_names[1] = "lower";
            assert(value_is_valid(key)); /* VERIFIED */
            exact_parameters[1] = key;
            parameter_lower_types[1] = NULL;
            parameter_upper_types[1] = NULL;
            parameter_pattern_kinds[2] = PPK_EXACT;
            parameter_names[2] = "upper";
            assert(value_is_valid(upper_bound)); /* VERIFIED */
            exact_parameters[2] = upper_bound;
            parameter_lower_types[2] = NULL;
            parameter_upper_types[2] = NULL;
            map_key_argument_number = argument_count;
          }
      }
    else
      {
        assert(key == NULL);
        assert(upper_bound == NULL);
        assert(filter == NULL);

        if (one_element)
          {
            star_value = NULL;
            key_lower = NULL;

            argument_count = 1;
            parameter_pattern_kinds[1] = PPK_ANY;
            parameter_names[1] = "key";
            exact_parameters[1] = NULL;
            parameter_lower_types[1] = NULL;
            parameter_upper_types[1] = NULL;
            map_key_argument_number = 0;
          }
        else
          {
            key_lower = NULL;

            star_value = create_string_value("*");
            if (star_value == NULL)
              {
                jumper_do_abort(the_jumper);
                assert(*lower_result == NULL); /* VERIFIED */
                assert(*upper_result == NULL); /* VERIFIED */
                return;
              }

            argument_count = 1;
            parameter_pattern_kinds[1] = PPK_EXACT;
            parameter_names[1] = "star";
            assert(value_is_valid(star_value)); /* VERIFIED */
            exact_parameters[1] = star_value;
            parameter_lower_types[1] = NULL;
            parameter_upper_types[1] = NULL;
            map_key_argument_number = argument_count;
          }
      }

    assert(*lower_result == NULL);
    assert(*upper_result == NULL);

    *lower_result = get_nothing_type();
    if (*lower_result == NULL)
      {
        jumper_do_abort(the_jumper);
        if (star_value != NULL)
            value_remove_reference(star_value, the_jumper);
        if (key_lower != NULL)
            type_remove_reference(key_lower, the_jumper);
        assert(*lower_result == NULL); /* VERIFIED */
        assert(*upper_result == NULL); /* VERIFIED */
        return;
      }
    type_add_reference(*lower_result);
    type_add_reference(*lower_result);
    *upper_result = *lower_result;
    assert(type_is_valid(*lower_result)); /* VERIFIED */
    assert(type_is_valid(*upper_result)); /* VERIFIED */

    if (overload_base != NULL)
      {
        type *overload_lower;
        type *overload_upper;
        boolean local_never_hits;
        type *new_lower;
        type *new_upper;

        parameter_pattern_kinds[0] = PPK_TYPE;
        parameter_names[0] = NULL;
        exact_parameters[0] = NULL;
        parameter_lower_types[0] = base_lower;
        parameter_upper_types[0] = base_upper;

        find_overload_type_with_possible_map_result(overload_base,
                argument_count + 1, &(parameter_pattern_kinds[0]),
                &(parameter_names[0]), &(exact_parameters[0]),
                &(parameter_lower_types[0]), &(parameter_upper_types[0]),
                is_write, write_lower_requirement, write_upper_requirement,
                map_key_argument_number + 1, &overload_lower, &overload_upper,
                always_hits, &local_never_hits, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(overload_lower == NULL);
            assert(overload_upper == NULL);
            type_remove_reference(*lower_result, the_jumper);
            type_remove_reference(*upper_result, the_jumper);
            *lower_result = NULL;
            *upper_result = NULL;
            if (star_value != NULL)
                value_remove_reference(star_value, the_jumper);
            if (key_lower != NULL)
                type_remove_reference(key_lower, the_jumper);
            assert(*lower_result == NULL); /* VERIFIED */
            assert(*upper_result == NULL); /* VERIFIED */
            return;
          }

        assert(overload_lower != NULL);
        assert(overload_upper != NULL);
        assert(type_is_valid(overload_lower)); /* VERIFIED */
        assert(type_is_valid(overload_upper)); /* VERIFIED */

        if (!local_never_hits)
            *never_hits = FALSE;

        if (*always_hits)
          {
            new_lower = overload_lower;
            new_upper = overload_upper;
            assert(type_is_valid(new_lower)); /* VERIFIED */
            assert(type_is_valid(new_upper)); /* VERIFIED */
          }
        else
          {
            assert(type_is_valid(*lower_result)); /* VERIFIED */
            assert(type_is_valid(overload_lower)); /* VERIFIED */
            new_lower = get_union_type(*lower_result, overload_lower);
            if (new_lower == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(overload_lower, the_jumper);
                type_remove_reference(overload_upper, the_jumper);
                type_remove_reference(*lower_result, the_jumper);
                type_remove_reference(*upper_result, the_jumper);
                *lower_result = NULL;
                *upper_result = NULL;
                if (star_value != NULL)
                    value_remove_reference(star_value, the_jumper);
                if (key_lower != NULL)
                    type_remove_reference(key_lower, the_jumper);
                assert(*lower_result == NULL); /* VERIFIED */
                assert(*upper_result == NULL); /* VERIFIED */
                return;
              }
            assert(type_is_valid(new_lower)); /* VERIFIED */
            type_remove_reference(overload_lower, the_jumper);

            assert(type_is_valid(*upper_result)); /* VERIFIED */
            assert(type_is_valid(overload_upper)); /* VERIFIED */
            new_upper = get_union_type(*upper_result, overload_upper);
            if (new_upper == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(overload_upper, the_jumper);
                type_remove_reference(*lower_result, the_jumper);
                type_remove_reference(*upper_result, the_jumper);
                *lower_result = NULL;
                *upper_result = NULL;
                if (star_value != NULL)
                    value_remove_reference(star_value, the_jumper);
                if (key_lower != NULL)
                    type_remove_reference(key_lower, the_jumper);
                assert(*lower_result == NULL); /* VERIFIED */
                assert(*upper_result == NULL); /* VERIFIED */
                return;
              }
            assert(type_is_valid(new_upper)); /* VERIFIED */
            type_remove_reference(overload_upper, the_jumper);
            assert(type_is_valid(new_lower)); /* VERIFIED */
            assert(type_is_valid(new_upper)); /* VERIFIED */
          }

        assert(type_is_valid(new_lower)); /* VERIFIED */
        assert(type_is_valid(new_upper)); /* VERIFIED */
        type_remove_reference(*lower_result, the_jumper);
        type_remove_reference(*upper_result, the_jumper);
        *lower_result = new_lower;
        *upper_result = new_upper;
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(*lower_result, the_jumper);
            type_remove_reference(*upper_result, the_jumper);
            *lower_result = NULL;
            *upper_result = NULL;
            if (star_value != NULL)
                value_remove_reference(star_value, the_jumper);
            if (key_lower != NULL)
                type_remove_reference(key_lower, the_jumper);
            assert(*lower_result == NULL); /* VERIFIED */
            assert(*upper_result == NULL); /* VERIFIED */
            return;
          }

        assert(type_is_valid(*lower_result)); /* VERIFIED */
        assert(type_is_valid(*upper_result)); /* VERIFIED */

        if (*always_hits)
          {
            if (star_value != NULL)
                value_remove_reference(star_value, the_jumper);
            if (key_lower != NULL)
                type_remove_reference(key_lower, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                type_remove_reference(*lower_result, the_jumper);
                type_remove_reference(*upper_result, the_jumper);
                *lower_result = NULL;
                *upper_result = NULL;
              }
            assert((*lower_result == NULL) || (type_is_valid(*lower_result)));
                    /* VERIFIED */
            assert((*upper_result == NULL) || (type_is_valid(*upper_result)));
                    /* VERIFIED */
            return;
          }
      }

    assert(type_is_valid(*lower_result)); /* VERIFIED */
    assert(type_is_valid(*upper_result)); /* VERIFIED */

    assert(type_is_valid(base_lower)); /* VERIFIED */
    operator_lower = type_field(base_lower, "operator[]", LOU_LOWER);
    if (operator_lower == NULL)
      {
        jumper_do_abort(the_jumper);
        type_remove_reference(*lower_result, the_jumper);
        type_remove_reference(*upper_result, the_jumper);
        *lower_result = NULL;
        *upper_result = NULL;
        if (star_value != NULL)
            value_remove_reference(star_value, the_jumper);
        if (key_lower != NULL)
            type_remove_reference(key_lower, the_jumper);
        assert(*lower_result == NULL); /* VERIFIED */
        assert(*upper_result == NULL); /* VERIFIED */
        return;
      }

    assert(type_is_valid(operator_lower)); /* VERIFIED */

    assert(type_is_valid(base_upper)); /* VERIFIED */
    operator_upper = type_field(base_upper, "operator[]", LOU_UPPER);
    if (operator_upper == NULL)
      {
        jumper_do_abort(the_jumper);
        type_remove_reference(operator_lower, the_jumper);
        type_remove_reference(*lower_result, the_jumper);
        type_remove_reference(*upper_result, the_jumper);
        *lower_result = NULL;
        *upper_result = NULL;
        if (star_value != NULL)
            value_remove_reference(star_value, the_jumper);
        if (key_lower != NULL)
            type_remove_reference(key_lower, the_jumper);
        assert(*lower_result == NULL); /* VERIFIED */
        assert(*upper_result == NULL); /* VERIFIED */
        return;
      }

    assert(type_is_valid(operator_upper)); /* VERIFIED */

    assert(type_is_valid(operator_lower)); /* VERIFIED */
    assert((write_lower_requirement == NULL) ||
           type_is_valid(write_lower_requirement)); /* VERIFIED */
    assert((write_upper_requirement == NULL) ||
           type_is_valid(write_upper_requirement)); /* VERIFIED */
    find_type_overload_type_with_possible_map_result(operator_lower,
            argument_count, &(parameter_pattern_kinds[1]),
            &(parameter_names[1]), &(exact_parameters[1]),
            &(parameter_lower_types[1]), &(parameter_upper_types[1]), is_write,
            write_lower_requirement, write_upper_requirement,
            map_key_argument_number, &overload_lower, &overload_upper,
            &local_always_hits, &local_never_hits, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(overload_lower == NULL);
        assert(overload_upper == NULL);
        type_remove_reference(operator_lower, the_jumper);
        type_remove_reference(operator_upper, the_jumper);
        type_remove_reference(*lower_result, the_jumper);
        type_remove_reference(*upper_result, the_jumper);
        *lower_result = NULL;
        *upper_result = NULL;
        if (star_value != NULL)
            value_remove_reference(star_value, the_jumper);
        if (key_lower != NULL)
            type_remove_reference(key_lower, the_jumper);
        assert(*lower_result == NULL); /* VERIFIED */
        assert(*upper_result == NULL); /* VERIFIED */
        return;
      }

    assert(overload_lower != NULL);
    assert(overload_upper != NULL);
    assert(type_is_valid(overload_lower)); /* VERIFIED */
    assert(type_is_valid(overload_upper)); /* VERIFIED */

    type_remove_reference(operator_lower, the_jumper);
    type_remove_reference(overload_upper, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(overload_lower, the_jumper);
        type_remove_reference(operator_upper, the_jumper);
        type_remove_reference(*lower_result, the_jumper);
        type_remove_reference(*upper_result, the_jumper);
        *lower_result = NULL;
        *upper_result = NULL;
        if (star_value != NULL)
            value_remove_reference(star_value, the_jumper);
        if (key_lower != NULL)
            type_remove_reference(key_lower, the_jumper);
        assert(*lower_result == NULL); /* VERIFIED */
        assert(*upper_result == NULL); /* VERIFIED */
        return;
      }

    if (!local_never_hits)
        *never_hits = FALSE;

    if (local_always_hits)
      {
        assert(type_is_valid(overload_lower)); /* VERIFIED */
        new_lower = overload_lower;
        assert(type_is_valid(new_lower)); /* VERIFIED */
      }
    else
      {
        assert(type_is_valid(*lower_result)); /* VERIFIED */
        assert(type_is_valid(overload_lower)); /* VERIFIED */
        new_lower = get_union_type(*lower_result, overload_lower);
        assert((new_lower == NULL) || type_is_valid(new_lower)); /* VERIFIED */
        if (new_lower == NULL)
          {
            jumper_do_abort(the_jumper);
            type_remove_reference(overload_lower, the_jumper);
            type_remove_reference(operator_upper, the_jumper);
            type_remove_reference(*lower_result, the_jumper);
            type_remove_reference(*upper_result, the_jumper);
            *lower_result = NULL;
            *upper_result = NULL;
            if (star_value != NULL)
                value_remove_reference(star_value, the_jumper);
            if (key_lower != NULL)
                type_remove_reference(key_lower, the_jumper);
            assert(*lower_result == NULL); /* VERIFIED */
            assert(*upper_result == NULL); /* VERIFIED */
            return;
          }
        assert(type_is_valid(new_lower)); /* VERIFIED */
        type_remove_reference(overload_lower, the_jumper);
      }

    assert(type_is_valid(new_lower)); /* VERIFIED */
    type_remove_reference(*lower_result, the_jumper);
    *lower_result = new_lower;
    assert(type_is_valid(*lower_result)); /* VERIFIED */
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(operator_upper, the_jumper);
        type_remove_reference(*lower_result, the_jumper);
        type_remove_reference(*upper_result, the_jumper);
        *lower_result = NULL;
        *upper_result = NULL;
        if (star_value != NULL)
            value_remove_reference(star_value, the_jumper);
        if (key_lower != NULL)
            type_remove_reference(key_lower, the_jumper);
        assert(*lower_result == NULL); /* VERIFIED */
        assert(*upper_result == NULL); /* VERIFIED */
        return;
      }

    if (local_always_hits)
      {
        *always_hits = TRUE;
        type_remove_reference(operator_upper, the_jumper);
        if (star_value != NULL)
            value_remove_reference(star_value, the_jumper);
        if (key_lower != NULL)
            type_remove_reference(key_lower, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(*lower_result, the_jumper);
            type_remove_reference(*upper_result, the_jumper);
            *lower_result = NULL;
            *upper_result = NULL;
          }
        assert((*lower_result == NULL) || (type_is_valid(*lower_result)));
                /* VERIFIED */
        assert((*upper_result == NULL) || (type_is_valid(*upper_result)));
                /* VERIFIED */
        return;
      }

    assert(type_is_valid(operator_upper)); /* VERIFIED */
    assert((write_lower_requirement == NULL) ||
           type_is_valid(write_lower_requirement)); /* VERIFIED */
    assert((write_upper_requirement == NULL) ||
           type_is_valid(write_upper_requirement)); /* VERIFIED */
    find_type_overload_type_with_possible_map_result(operator_upper,
            argument_count, &(parameter_pattern_kinds[1]),
            &(parameter_names[1]), &(exact_parameters[1]),
            &(parameter_lower_types[1]), &(parameter_upper_types[1]), is_write,
            write_lower_requirement, write_upper_requirement,
            map_key_argument_number, &overload_lower, &overload_upper,
            &local_always_hits, &local_never_hits, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(overload_lower == NULL);
        assert(overload_upper == NULL);
        type_remove_reference(operator_upper, the_jumper);
        type_remove_reference(*lower_result, the_jumper);
        type_remove_reference(*upper_result, the_jumper);
        *lower_result = NULL;
        *upper_result = NULL;
        if (star_value != NULL)
            value_remove_reference(star_value, the_jumper);
        if (key_lower != NULL)
            type_remove_reference(key_lower, the_jumper);
        assert(*lower_result == NULL); /* VERIFIED */
        assert(*upper_result == NULL); /* VERIFIED */
        return;
      }

    assert(overload_lower != NULL);
    assert(overload_upper != NULL);
    assert(type_is_valid(overload_lower)); /* VERIFIED */
    assert(type_is_valid(overload_upper)); /* VERIFIED */

    type_remove_reference(operator_upper, the_jumper);
    type_remove_reference(overload_lower, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(overload_upper, the_jumper);
        type_remove_reference(*lower_result, the_jumper);
        type_remove_reference(*upper_result, the_jumper);
        *lower_result = NULL;
        *upper_result = NULL;
        if (star_value != NULL)
            value_remove_reference(star_value, the_jumper);
        if (key_lower != NULL)
            type_remove_reference(key_lower, the_jumper);
        assert(*lower_result == NULL); /* VERIFIED */
        assert(*upper_result == NULL); /* VERIFIED */
        return;
      }

    if (!local_never_hits)
        *never_hits = FALSE;

    if (local_always_hits)
      {
        assert(type_is_valid(overload_upper)); /* VERIFIED */
        new_upper = overload_upper;
        assert(type_is_valid(new_upper)); /* VERIFIED */
      }
    else
      {
        assert(type_is_valid(*upper_result)); /* VERIFIED */
        assert(type_is_valid(overload_upper)); /* VERIFIED */
        new_upper = get_union_type(*upper_result, overload_upper);
        assert((new_upper == NULL) || type_is_valid(new_upper)); /* VERIFIED */
        if (new_upper == NULL)
          {
            jumper_do_abort(the_jumper);
            type_remove_reference(overload_upper, the_jumper);
            type_remove_reference(*lower_result, the_jumper);
            type_remove_reference(*upper_result, the_jumper);
            *lower_result = NULL;
            *upper_result = NULL;
            if (star_value != NULL)
                value_remove_reference(star_value, the_jumper);
            if (key_lower != NULL)
                type_remove_reference(key_lower, the_jumper);
            assert(*lower_result == NULL); /* VERIFIED */
            assert(*upper_result == NULL); /* VERIFIED */
            return;
          }
        assert(type_is_valid(new_upper)); /* VERIFIED */
        type_remove_reference(overload_upper, the_jumper);
      }

    assert(type_is_valid(new_upper)); /* VERIFIED */
    type_remove_reference(*upper_result, the_jumper);
    *upper_result = new_upper;
    assert(type_is_valid(*upper_result)); /* VERIFIED */
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(*lower_result, the_jumper);
        type_remove_reference(*upper_result, the_jumper);
        *lower_result = NULL;
        *upper_result = NULL;
        if (star_value != NULL)
            value_remove_reference(star_value, the_jumper);
        if (key_lower != NULL)
            type_remove_reference(key_lower, the_jumper);
        assert(*lower_result == NULL); /* VERIFIED */
        assert(*upper_result == NULL); /* VERIFIED */
        return;
      }

    if (star_value != NULL)
        value_remove_reference(star_value, the_jumper);
    if (key_lower != NULL)
        type_remove_reference(key_lower, the_jumper);
    assert(type_is_valid(*lower_result)); /* VERIFIED */
    assert(type_is_valid(*upper_result)); /* VERIFIED */
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(*lower_result, the_jumper);
        type_remove_reference(*upper_result, the_jumper);
        *lower_result = NULL;
        *upper_result = NULL;
      }

    assert((*lower_result == NULL) || (type_is_valid(*lower_result)));
            /* VERIFIED */
    assert((*upper_result == NULL) || (type_is_valid(*upper_result)));
            /* VERIFIED */
    return;
  }

static void force_canonical_form(lookup_actual_arguments *actuals,
                                 size_t index, jumper *the_jumper)
  {
    value *key;
    value *upper_bound;
    o_integer lower_oi;
    o_integer upper_oi;

    assert(actuals != NULL);
    assert(the_jumper != NULL);

    assert(index < actuals->dimensions);
    assert(actuals->array != NULL);

    key = actuals->array[index].key;
    upper_bound = actuals->array[index].upper_bound;

    if ((key == NULL) || (upper_bound == NULL))
        return;

    assert((get_value_kind(key) == VK_INTEGER) &&
           (get_value_kind(upper_bound) == VK_INTEGER));

    lower_oi = integer_value_data(key);
    upper_oi = integer_value_data(upper_bound);

    assert(!(oi_out_of_memory(lower_oi)));
    assert(!(oi_out_of_memory(upper_oi)));

    if (!(oi_less_than(upper_oi, lower_oi)))
        return;

    value_remove_reference(key, the_jumper);
    value_remove_reference(upper_bound, the_jumper);

    key = create_integer_value(oi_one);
    actuals->array[index].key = key;
    if (key == NULL)
      {
        jumper_do_abort(the_jumper);
        actuals->array[index].upper_bound = NULL;
        return;
      }

    upper_bound = create_integer_value(oi_zero);
    actuals->array[index].upper_bound = upper_bound;
    if (key == NULL)
      {
        jumper_do_abort(the_jumper);
        return;
      }
  }

static void shift_and_box_map_item_key(type **new_key_type,
        value **new_key_value, value *map_value, size_t item_number,
        o_integer lower, o_integer upper, o_integer shift_amount,
        const source_location *location, jumper *the_jumper)
  {
    assert(new_key_type != NULL);
    assert(new_key_value != NULL);
    assert(map_value != NULL);
    assert(!(oi_out_of_memory(lower)));
    assert(!(oi_out_of_memory(upper)));
    assert(!(oi_out_of_memory(shift_amount)));
    assert(the_jumper != NULL);

    assert(get_value_kind(map_value) == VK_MAP);

    if (map_value_item_is_type(map_value, item_number))
      {
        type *filter_type;
        o_integer base_oi;
        o_integer diff_oi;

        *new_key_value = NULL;

        filter_type = map_value_item_key_type(map_value, item_number);
        assert(filter_type != NULL);
        assert(type_is_valid(filter_type)); /* VERIFICATION NEEDED */

        oi_subtract(base_oi, lower, shift_amount);
        if (oi_out_of_memory(base_oi))
          {
            jumper_do_abort(the_jumper);
            return;
          }

        oi_subtract(diff_oi, upper, lower);
        if (oi_out_of_memory(diff_oi))
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(base_oi);
            return;
          }

        assert(type_is_valid(filter_type)); /* VERIFIED */
        *new_key_type = shift_and_box_type(filter_type, shift_amount, base_oi,
                                           diff_oi, location, the_jumper);
        oi_remove_reference(base_oi);
        oi_remove_reference(diff_oi);
        if (*new_key_type == NULL)
          {
            assert(!(jumper_flowing_forward(the_jumper)));
            return;
          }
        assert(type_is_valid(*new_key_type)); /* VERIFICATION NEEDED */
      }
    else
      {
        value *sub_key;
        o_integer sub_oi;
        o_integer new_oi;

        *new_key_type = NULL;

        sub_key = map_value_item_key_value(map_value, item_number);
        assert(sub_key != NULL);

        if (get_value_kind(sub_key) != VK_INTEGER)
          {
            *new_key_value = NULL;
            return;
          }

        sub_oi = integer_value_data(sub_key);
        if ((oi_kind(sub_oi) == IIK_UNSIGNED_INFINITY) ||
            (oi_kind(sub_oi) == IIK_ZERO_ZERO) ||
            oi_less_than(sub_oi, lower) || oi_less_than(upper, sub_oi))
          {
            *new_key_value = NULL;
            return;
          }

        oi_subtract(new_oi, sub_oi, shift_amount);
        if (oi_out_of_memory(new_oi))
          {
            jumper_do_abort(the_jumper);
            return;
          }

        *new_key_value = create_integer_value(new_oi);
        oi_remove_reference(new_oi);
        if (*new_key_value == NULL)
          {
            jumper_do_abort(the_jumper);
            return;
          }
      }
  }

static void box_map_item_key(type **new_key_type,
        value **new_key_value, value *map_value, size_t item_number,
        o_integer lower, o_integer upper, const source_location *location,
        jumper *the_jumper)
  {
    assert(new_key_type != NULL);
    assert(new_key_value != NULL);
    assert(map_value != NULL);
    assert(!(oi_out_of_memory(lower)));
    assert(!(oi_out_of_memory(upper)));
    assert(the_jumper != NULL);

    assert(get_value_kind(map_value) == VK_MAP);

    if (map_value_item_is_type(map_value, item_number))
      {
        type *filter_type;
        o_integer diff_oi;

        *new_key_value = NULL;

        filter_type = map_value_item_key_type(map_value, item_number);
        assert(filter_type != NULL);
        assert(type_is_valid(filter_type)); /* VERIFICATION NEEDED */

        oi_subtract(diff_oi, upper, lower);
        if (oi_out_of_memory(diff_oi))
          {
            jumper_do_abort(the_jumper);
            return;
          }

        assert(type_is_valid(filter_type)); /* VERIFIED */
        *new_key_type = shift_and_box_type(filter_type, oi_zero,
                lower, diff_oi, location, the_jumper);
        oi_remove_reference(diff_oi);
        if (*new_key_type == NULL)
          {
            assert(!(jumper_flowing_forward(the_jumper)));
            return;
          }
        assert(type_is_valid(*new_key_type)); /* VERIFICATION NEEDED */
      }
    else
      {
        value *sub_key;
        o_integer sub_oi;

        *new_key_type = NULL;

        sub_key = map_value_item_key_value(map_value, item_number);
        assert(sub_key != NULL);

        if (get_value_kind(sub_key) != VK_INTEGER)
          {
            *new_key_value = NULL;
            return;
          }

        sub_oi = integer_value_data(sub_key);
        if ((oi_kind(sub_oi) == IIK_UNSIGNED_INFINITY) ||
            (oi_kind(sub_oi) == IIK_ZERO_ZERO) ||
            oi_less_than(sub_oi, lower) || oi_less_than(upper, sub_oi))
          {
            *new_key_value = NULL;
            return;
          }

        *new_key_value = sub_key;
      }
  }

static void box_map_item_key_exclude_items(type **new_key_type,
        value **new_key_value, value *map_value, size_t item_number,
        o_integer lower, o_integer upper, value *value_with_items,
        const source_location *location, jumper *the_jumper)
  {
    assert(new_key_type != NULL);
    assert(new_key_value != NULL);
    assert(map_value != NULL);
    assert(!(oi_out_of_memory(lower)));
    assert(!(oi_out_of_memory(upper)));
    assert(value_with_items != NULL);
    assert(the_jumper != NULL);

    box_map_item_key(new_key_type, new_key_value, map_value, item_number,
                     lower, upper, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return;
    if ((*new_key_type == NULL) && (*new_key_value == NULL))
        return;

    if (*new_key_type != NULL)
      {
        o_integer max_oi;
        type *range;
        type *not;
        type *fixed;

        assert(*new_key_type != NULL);
        assert(*new_key_value == NULL);

        oi_create_from_size_t(max_oi, value_component_count(value_with_items));
        if (oi_out_of_memory(max_oi))
          {
            jumper_do_abort(the_jumper);
            type_remove_reference(*new_key_type, the_jumper);
            return;
          }

        range = get_integer_range_type(oi_zero, max_oi, TRUE, FALSE);
        oi_remove_reference(max_oi);
        if (range == NULL)
          {
            jumper_do_abort(the_jumper);
            type_remove_reference(*new_key_type, the_jumper);
            return;
          }

        not = get_not_type(range);
        if (not == NULL)
          {
            jumper_do_abort(the_jumper);
            type_remove_reference(range, the_jumper);
            type_remove_reference(*new_key_type, the_jumper);
            return;
          }

        type_remove_reference(range, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(not, the_jumper);
            type_remove_reference(*new_key_type, the_jumper);
            return;
          }

        fixed = get_intersection_type(*new_key_type, not);
        if (fixed == NULL)
          {
            jumper_do_abort(the_jumper);
            type_remove_reference(not, the_jumper);
            type_remove_reference(*new_key_type, the_jumper);
            return;
          }

        type_remove_reference(not, the_jumper);
        type_remove_reference(*new_key_type, the_jumper);
        *new_key_type = fixed;
        if (!(jumper_flowing_forward(the_jumper)))
            type_remove_reference(fixed, the_jumper);
      }
    else
      {
        o_integer new_oi;

        assert(*new_key_type == NULL);
        assert(*new_key_value != NULL);

        assert(get_value_kind(*new_key_value) == VK_INTEGER);
        new_oi = integer_value_data(*new_key_value);

        if ((oi_kind(new_oi) == IIK_FINITE) && !(oi_is_negative(new_oi)))
          {
            verdict the_verdict;
            size_t new_size_t;

            the_verdict = oi_magnitude_to_size_t(new_oi, &new_size_t);
            if ((the_verdict == MISSION_ACCOMPLISHED) &&
                (new_size_t < value_component_count(value_with_items)))
              {
                *new_key_value = NULL;
              }
          }
      }
  }

static void set_through_lookup_from_through_map(value **result,
        lookup_actual_arguments *actuals, value *overload_base,
        value *(*update_function)(void *update_data, value *existing_value,
                value *new_value, const source_location *the_source_location,
                jumper *the_jumper), void *update_data,
        boolean care_about_existing_value, value *new_value, value **map_value,
        value *map_key_value, type *map_key_type,
        const source_location *location, jumper *the_jumper, size_t start)
  {
    assert(result != NULL);
    assert(*result != NULL);
    assert(actuals != NULL);
    assert(update_function != NULL);
    assert(map_value != NULL);
    assert(location != NULL);
    assert((map_key_value == NULL) != (map_key_type == NULL));
    assert(the_jumper != NULL);

    assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
    assert((*map_value == NULL) || map_value_all_keys_are_valid(*map_value));
            /* VERIFIED */
    assert((map_key_value == NULL) || value_is_valid(map_key_value));
            /* VERIFIED */
    assert((map_key_type == NULL) || type_is_valid(map_key_type));
            /* VERIFIED */

    if ((!care_about_existing_value) && (start == actuals->dimensions))
      {
        value *sub_element;

        sub_element = set_through_lookup_from(NULL, actuals, overload_base,
                update_function, update_data, care_about_existing_value,
                new_value, location, the_jumper, start);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(*result, the_jumper);
            return;
          }

        value_add_reference(sub_element);

        assert((map_key_type == NULL) || type_is_valid(map_key_type));
                /* VERIFIED */
        assert((map_key_value == NULL) || value_is_valid(map_key_value));
                /* VERIFIED */
        *result = set_element_value(*result, map_key_value, map_key_type,
                sub_element, overload_base, location, the_jumper);
        if (*result == NULL)
          {
            value_remove_reference(sub_element, the_jumper);
            return;
          }

        if ((*map_value != NULL) && (*result != *map_value) &&
            (sub_element != NULL))
          {
            assert(map_value_all_keys_are_valid(*map_value));
                    /* VERIFICATION NEEDED */
            if (map_key_type != NULL)
              {
                assert(map_key_value == NULL);
                assert(map_value_all_keys_are_valid(*map_value));
                        /* VERIFIED */
                assert(type_is_valid(map_key_type)); /* VERIFIED */
                *map_value = map_value_set_filter(*map_value, map_key_type,
                        sub_element, location, the_jumper);
              }
            else
              {
                assert(map_key_value != NULL);
                assert(map_value_all_keys_are_valid(*map_value));
                        /* VERIFIED */
                assert(value_is_valid(map_key_value)); /* VERIFIED */
                *map_value = map_value_set(*map_value, map_key_value,
                                           sub_element, location, the_jumper);
              }
          }

        value_remove_reference(sub_element, the_jumper);

        return;
      }

    assert(*map_value != NULL);

    if (map_key_value != NULL)
      {
        boolean doubt;
        value *sub_existing;
        value *sub_element;

        assert(map_key_type == NULL);

        assert(map_value_all_keys_are_valid(*map_value)); /* VERIFIED */
        assert(value_is_valid(map_key_value)); /* VERIFIED */
        sub_existing = map_value_lookup(*map_value, map_key_value, &doubt,
                                        location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(*result, the_jumper);
            return;
          }
        if (doubt)
          {
            value *values[1];
            type *singleton_type;

            values[0] = map_key_value;
            singleton_type = get_enumeration_type(1, &(values[0]));
            if (singleton_type == NULL)
              {
                jumper_do_abort(the_jumper);
                value_remove_reference(*result, the_jumper);
                return;
              }

            set_through_lookup_from_through_map(result, actuals, overload_base,
                    update_function, update_data, care_about_existing_value,
                    new_value, map_value, NULL, singleton_type, location,
                    the_jumper, start);

            type_remove_reference(singleton_type, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(*result, the_jumper);
                return;
              }

            return;
          }

        sub_element = set_through_lookup_from(sub_existing, actuals,
                overload_base, update_function, update_data,
                care_about_existing_value, new_value, location, the_jumper,
                start);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(*result, the_jumper);
            return;
          }

        value_add_reference(sub_element);

        assert(map_key_type == NULL); /* VERIFIED */
        assert((map_key_value != NULL) && value_is_valid(map_key_value));
                /* VERIFIED */
        *result = set_element_value(*result, map_key_value, map_key_type,
                sub_element, overload_base, location, the_jumper);
        if (*result == NULL)
          {
            value_remove_reference(sub_element, the_jumper);
            return;
          }

        if ((*map_value != NULL) && (*result != *map_value) &&
            (sub_element != NULL))
          {
            assert(map_value_all_keys_are_valid(*map_value));
                    /* VERIFICATION NEEDED */
            assert(map_key_value != NULL);
            assert(value_is_valid(map_key_value)); /* VERIFIED */
            *map_value = map_value_set(*map_value, map_key_value, sub_element,
                                       location, the_jumper);
          }

        value_remove_reference(sub_element, the_jumper);

        return;
      }
    else
      {
        type *remainder;
        size_t count;
        size_t number;
        value *final_sub_element;

        assert(map_key_type != NULL);

        remainder = map_key_type;
        assert(remainder != NULL);
        type_add_reference(remainder);

        count = map_value_item_count(*map_value);

        for (number = 0; number < count; ++number)
          {
            type *write_key_type;
            value *write_key_value;
            type *not_type;
            type *new_remainder;
            value *sub_existing;
            value *sub_element;

            if (map_value_item_is_type(*map_value, number))
              {
                type *local_type;

                local_type = map_value_item_key_type(*map_value, number);
                assert(local_type != NULL);

                write_key_type =
                        get_intersection_type(map_key_type, local_type);
                if (write_key_type == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    type_remove_reference(remainder, the_jumper);
                    value_remove_reference(*result, the_jumper);
                    return;
                  }

                if (get_type_kind(write_key_type) == TK_NOTHING)
                  {
                    type_remove_reference(write_key_type, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        type_remove_reference(remainder, the_jumper);
                        value_remove_reference(*result, the_jumper);
                        return;
                      }
                    continue;
                  }

                write_key_value = NULL;
              }
            else
              {
                value *local_value;
                boolean doubt;
                boolean is_in;

                local_value = map_value_item_key_value(*map_value, number);
                assert(local_value != NULL);

                assert(type_is_valid(map_key_type)); /* VERIFIED */
                is_in = value_is_in_type(local_value, map_key_type, &doubt,
                                         NULL, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    type_remove_reference(remainder, the_jumper);
                    value_remove_reference(*result, the_jumper);
                    return;
                  }

                if (!doubt)
                  {
                    if (!is_in)
                        continue;

                    write_key_type = NULL;
                    write_key_value = local_value;
                  }
                else
                  {
                    value *values[1];
                    type *singleton_type;

                    values[0] = local_value;
                    singleton_type = get_enumeration_type(1, &(values[0]));
                    if (singleton_type == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        type_remove_reference(remainder, the_jumper);
                        value_remove_reference(*result, the_jumper);
                        return;
                      }

                    write_key_type = get_intersection_type(map_key_type,
                                                           singleton_type);
                    type_remove_reference(singleton_type, the_jumper);
                    if (write_key_type == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        type_remove_reference(remainder, the_jumper);
                        value_remove_reference(*result, the_jumper);
                        return;
                      }
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        type_remove_reference(write_key_type, the_jumper);
                        type_remove_reference(remainder, the_jumper);
                        value_remove_reference(*result, the_jumper);
                        return;
                      }

                    write_key_value = NULL;
                  }
              }

            if (write_key_type != NULL)
              {
                assert(write_key_value == NULL);

                not_type = get_not_type(write_key_type);
                if (not_type == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    type_remove_reference(write_key_type, the_jumper);
                    type_remove_reference(remainder, the_jumper);
                    value_remove_reference(*result, the_jumper);
                    return;
                  }
              }
            else
              {
                value *values[1];
                type *singleton_type;

                assert(write_key_value != NULL);

                values[0] = write_key_value;
                singleton_type = get_enumeration_type(1, &(values[0]));
                if (singleton_type == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    type_remove_reference(remainder, the_jumper);
                    value_remove_reference(*result, the_jumper);
                    return;
                  }

                not_type = get_not_type(singleton_type);
                type_remove_reference(singleton_type, the_jumper);
                if (not_type == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    type_remove_reference(remainder, the_jumper);
                    value_remove_reference(*result, the_jumper);
                    return;
                  }
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    type_remove_reference(not_type, the_jumper);
                    type_remove_reference(remainder, the_jumper);
                    value_remove_reference(*result, the_jumper);
                    return;
                  }
              }

            new_remainder = get_intersection_type(remainder, not_type);
            if (new_remainder == NULL)
              {
                type_remove_reference(not_type, the_jumper);
                if (write_key_type != NULL)
                    type_remove_reference(write_key_type, the_jumper);
                type_remove_reference(remainder, the_jumper);
                value_remove_reference(*result, the_jumper);
                return;
              }

            type_remove_reference(not_type, the_jumper);
            type_remove_reference(remainder, the_jumper);
            remainder = new_remainder;
            if (!(jumper_flowing_forward(the_jumper)))
              {
                if (write_key_type != NULL)
                    type_remove_reference(write_key_type, the_jumper);
                type_remove_reference(remainder, the_jumper);
                value_remove_reference(*result, the_jumper);
                return;
              }

            sub_existing = map_value_item_target(*map_value, number);
            assert(sub_existing != NULL);

            sub_element = set_through_lookup_from(sub_existing, actuals,
                    overload_base, update_function, update_data,
                    care_about_existing_value, new_value, location, the_jumper,
                    start);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                if (write_key_type != NULL)
                    type_remove_reference(write_key_type, the_jumper);
                type_remove_reference(remainder, the_jumper);
                value_remove_reference(*result, the_jumper);
                return;
              }

            value_add_reference(sub_element);

            assert((write_key_type == NULL) || type_is_valid(write_key_type));
                    /* VERIFICATION NEEDED */
            assert((write_key_value == NULL) ||
                   value_is_valid(write_key_value)); /* VERIFICATION NEEDED */
            *result = set_element_value(*result, write_key_value,
                    write_key_type, sub_element, overload_base, location,
                    the_jumper);
            if (*result == NULL)
              {
                if (write_key_type != NULL)
                    type_remove_reference(write_key_type, the_jumper);
                type_remove_reference(remainder, the_jumper);
                value_remove_reference(sub_element, the_jumper);
                return;
              }

            if ((*map_value != NULL) && (*result != *map_value) &&
                (sub_element != NULL))
              {
                assert(map_value_all_keys_are_valid(*map_value));
                        /* VERIFICATION NEEDED */
                if (write_key_type != NULL)
                  {
                    assert(write_key_value == NULL);
                    assert(map_value_all_keys_are_valid(*map_value));
                            /* VERIFIED */
                    assert(type_is_valid(write_key_type));
                            /* VERIFICATION NEEDED */
                    *map_value = map_value_set_filter(*map_value,
                            write_key_type, sub_element, location, the_jumper);
                  }
                else
                  {
                    assert(write_key_value != NULL);
                    assert(map_value_all_keys_are_valid(*map_value));
                            /* VERIFIED */
                    assert(value_is_valid(write_key_value));
                            /* VERIFICATION NEEDED */
                    *map_value = map_value_set(*map_value, write_key_value,
                            sub_element, location, the_jumper);
                  }
              }

            value_remove_reference(sub_element, the_jumper);
            if (write_key_type != NULL)
                type_remove_reference(write_key_type, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                type_remove_reference(remainder, the_jumper);
                value_remove_reference(*result, the_jumper);
                return;
              }
          }

        final_sub_element = set_through_lookup_from(NULL, actuals,
                overload_base, update_function, update_data,
                care_about_existing_value, new_value, location, the_jumper,
                start);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(remainder, the_jumper);
            value_remove_reference(*result, the_jumper);
            return;
          }

        value_add_reference(final_sub_element);

        assert(type_is_valid(remainder)); /* VERIFICATION NEEDED */
        *result = set_element_value(*result, NULL, remainder,
                final_sub_element, overload_base, location, the_jumper);
        if (*result == NULL)
          {
            type_remove_reference(remainder, the_jumper);
            value_remove_reference(final_sub_element, the_jumper);
            return;
          }

        if ((*map_value != NULL) && (*result != *map_value))
          {
            assert(map_value_all_keys_are_valid(*map_value));
                    /* VERIFICATION NEEDED */
            assert(type_is_valid(remainder)); /* VERIFICATION NEEDED */
            *map_value = map_value_set_filter(*map_value, remainder,
                    final_sub_element, location, the_jumper);
          }

        value_remove_reference(final_sub_element, the_jumper);

        type_remove_reference(remainder, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(*result, the_jumper);
            return;
          }
      }
  }

static boolean key_is_outside_filter(value *sub_key, type *filter,
        const source_location *location, jumper *the_jumper)
  {
    boolean doubt;
    boolean is_in;

    assert(sub_key != NULL);
    assert(location != NULL);
    assert(the_jumper != NULL);

    assert(value_is_valid(sub_key)); /* VERIFIED */
    assert((filter == NULL) || type_is_valid(filter)); /* VERIFIED */

    if (filter == NULL)
        return FALSE;

    assert(value_is_valid(sub_key)); /* VERIFIED */
    assert(type_is_valid(filter)); /* VERIFIED */
    is_in = value_is_in_type(sub_key, filter, &doubt, NULL, location,
                             the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return FALSE;
    if (doubt)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(lookup_filter_indeterminate),
                "A lookup was evaluated with a filter dimension and a key for "
                "which %s was unable to determine whether that key was in the "
                "filter type.", interpreter_name());
        return FALSE;
      }
    return !is_in;
  }
