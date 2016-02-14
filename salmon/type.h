/* file "type.h" */

/*
 *  This file contains the interface to the type module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef TYPE_H
#define TYPE_H

#include "c_foundations/basic.h"


typedef enum type_kind
  {
    TK_ANYTHING,
    TK_NOTHING,
    TK_INTEGER,
    TK_RATIONAL,
    TK_ANY_REGULAR_EXPRESSION,
    TK_ANY_QUARK,
    TK_ANY_LEPTON,
    TK_LEPTON_KEY,
    TK_JUMP_TARGET,
    TK_ANY_CLASS,
    TK_OBJECT,
    TK_TAGALONG_KEY,
    TK_LOCK,
    TK_STRING,
    TK_CHARACTER,
    TK_ENUMERATION,
    TK_NOT,
    TK_INTERSECTION,
    TK_UNION,
    TK_XOR,
    TK_ARRAY,
    TK_INTEGER_RANGES,
    TK_RATIONAL_RANGES,
    TK_POINTER,
    TK_TYPE,
    TK_MAP,
    TK_ROUTINE,
    TK_FIELDS,
    TK_LEPTON,
    TK_MULTISET,
    TK_INTERFACE,
    TK_SEMI_LABELED_VALUE_LIST,
    TK_REGULAR_EXPRESSION,
    TK_CLASS,
    TK_TEST_ROUTINE,
    TK_TEST_ROUTINE_CHAIN,
    TK_SEPARATOR
  } type_kind;

typedef enum { LOU_LOWER, LOU_UPPER } lower_or_upper;

typedef struct type type;


#include "value.h"
#include "regular_expression.h"
#include "lepton_key_instance.h"
#include "routine_instance.h"
#include "routine_instance_chain.h"
#include "context.h"
#include "object.h"
#include "precedence.h"
#include "validator.h"
#include "reference_cluster.h"
#include "variable_instance.h"


extern type *get_anything_type(void);
extern type *get_nothing_type(void);
extern type *get_integer_type(void);
extern type *get_finite_integer_type(void);
extern type *get_rational_type(void);
extern type *get_any_regular_expression_type(void);
extern type *get_any_quark_type(void);
extern type *get_any_lepton_type(void);
extern type *get_lepton_key_type(void);
extern type *get_jump_target_type(void);
extern type *get_any_class_type(void);
extern type *get_object_type(void);
extern type *get_tagalong_key_type(void);
extern type *get_lock_type(void);
extern type *get_string_type(void);
extern type *get_character_type(void);
extern type *get_boolean_type(void);
extern type *get_enumeration_type(size_t value_count, value **values);
extern type *get_not_type(type *base);
extern type *get_intersection_type(type *left, type *right);
extern type *get_union_type(type *left, type *right);
extern type *get_xor_type(type *left, type *right);
extern type *get_array_type(type *base, o_integer lower_bound,
                            o_integer upper_bound);
extern type *get_integer_range_type(o_integer lower_bound,
        o_integer upper_bound, boolean lower_is_inclusive,
        boolean upper_is_inclusive);
extern type *get_rational_range_type(rational *lower_bound,
        rational *upper_bound, boolean lower_is_inclusive,
        boolean upper_is_inclusive);
extern type *get_pointer_type(type *base, boolean read_allowed,
                              boolean write_allowed, boolean null_allowed);
extern type *get_type_type(type *base);
extern type *get_map_type(type *key, type *target);
extern type *get_routine_type(type *return_type, size_t argument_count,
        type **argument_types, const char **argument_names,
        boolean *argument_has_defaults, boolean extra_arguments_allowed,
        boolean extra_arguments_unspecified);
extern type *get_fields_type(size_t field_count, type **field_types,
        const char **field_names, boolean extra_fields_allowed);
extern type *get_lepton_type(lepton_key_instance *key, size_t field_count,
        type **field_types, const char **field_names,
        boolean extra_fields_allowed);
extern type *get_multiset_type(size_t field_count, type **field_types,
        const char **field_names, boolean extra_fields_allowed);
extern type *get_interface_type(size_t item_count, type **item_types,
        const char **item_names, boolean *item_writing_alloweds,
        boolean null_allowed);
extern type *get_semi_labeled_value_list_type(size_t element_count,
        type **element_types, const char **element_names,
        boolean extra_elements_allowed);
extern type *get_regular_expression_type(
        regular_expression *the_regular_expression);
extern type *get_class_type(routine_instance *class_routine);
extern type *get_test_routine_type(routine_instance *test_routine);
extern type *get_test_routine_chain_type(
        routine_instance_chain *instance_chain);

extern void type_add_reference(type *the_type);
extern void type_remove_reference(type *the_type, jumper *the_jumper);
extern void type_add_reference_with_reference_cluster(type *the_type,
        reference_cluster *cluster);
extern void type_remove_reference_with_reference_cluster(type *the_type,
        jumper *the_jumper, reference_cluster *cluster);
extern reference_cluster *type_reference_cluster(type *the_type);

extern type_kind get_type_kind(type *the_type);

extern boolean value_is_in_type(value *the_value, type *the_type,
        boolean *doubt, char **why_not, const source_location *location,
        jumper *the_jumper);

extern boolean type_is_valid(type *the_type);
extern void check_type_validity(type *the_type,
        const source_location *location, jumper *the_jumper);
extern validator *type_validator(type *the_type);

extern boolean types_are_equal(type *type1, type *type2, boolean *doubt,
        const source_location *location, jumper *the_jumper);
extern int type_structural_order(type *left, type *right);
extern boolean type_is_slippery(type *the_type);

extern void print_type(type *the_type,
        void (*printer)(void *data, const char *format, ...), void *data,
        type_expression_parsing_precedence precedence);
extern void print_type_with_override(type *the_type,
        void (*printer)(void *data, const char *format, ...), void *data,
        void (*override)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data), type_expression_parsing_precedence precedence);

extern boolean type_is_subset(type *small, type *big, boolean *doubt,
        char **why_not, const source_location *location, jumper *the_jumper);
extern type *map_target_type(type *base_type, value *key_value,
        lower_or_upper bound_direction, const source_location *location,
        jumper *the_jumper);
extern type *map_target_type_from_key_type(type *base_type, type *key_type,
        lower_or_upper bound_direction, const source_location *location,
        jumper *the_jumper);
extern type *get_map_key_type(type *base_type, lower_or_upper bound_direction,
                              jumper *the_jumper);
extern type *type_field(type *base_type, const char *field_name,
                        lower_or_upper bound_direction);
extern type *augment_write_type_from_read_type(type *write_type,
        type *read_type, const char *field_name,
        lower_or_upper bound_direction, variable_instance **base_variable);
extern boolean intersection_empty(type *type1, type *type2, boolean *doubt,
        const source_location *location, jumper *the_jumper);
extern value *force_value_to_type(value *to_force, type *the_type,
        const source_location *location, jumper *the_jumper);
extern boolean type_always_forceable_to(type *source, type *target,
        boolean *doubt, const source_location *location, jumper *the_jumper);
extern void find_type_overload_type(type *base_type, size_t argument_count,
        parameter_pattern_kind *parameter_pattern_kinds,
        const char **parameter_names, value **exact_parameters,
        type **parameter_lower_types, type **parameter_upper_types,
        boolean is_write, type *write_lower_requirement,
        type *write_upper_requirement, type **result_lower,
        type **result_upper, boolean *always_hits, boolean *never_hits,
        const source_location *location, jumper *the_jumper);
extern void find_type_overload_type_with_possible_map_result(type *base_type,
        size_t argument_count, parameter_pattern_kind *parameter_pattern_kinds,
        const char **parameter_names, value **exact_parameters,
        type **parameter_lower_types, type **parameter_upper_types,
        boolean is_write, type *write_lower_requirement,
        type *write_upper_requirement, size_t map_key_argument_number,
        type **result_lower, type **result_upper, boolean *always_hits,
        boolean *never_hits, const source_location *location,
        jumper *the_jumper);
extern void find_type_integer_bounds(type *the_type, o_integer *minimum,
        o_integer *maximum, boolean *min_doubt, boolean *max_doubt,
        const source_location *location, jumper *the_jumper);
/*
 *  The shift_and_box_type() returns a type which is the intersection of the
 *  range [box_lower...box_upper] with the type of all integers x for which
 *  x + shift_amount is in base.
 */
extern type *shift_and_box_type(type *base, o_integer shift_amount,
        o_integer box_lower, o_integer box_upper,
        const source_location *location, jumper *the_jumper);
extern boolean type_has_finite_element_count(type *base, boolean *doubt);

extern void type_message_deallocate(char *message);

extern verdict init_type_module(void);
extern void cleanup_type_module_instances(void);
extern void cleanup_type_module(void);

DEFINE_EXCEPTION_TAG(force_match_indeterminate);
DEFINE_EXCEPTION_TAG(type_for_invalid_nothing_indeterminate);
DEFINE_EXCEPTION_TAG(type_for_invalid_everything_indeterminate);


#endif /* TYPE_H */
