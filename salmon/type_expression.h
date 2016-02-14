/* file "type_expression.h" */

/*
 *  This file contains the interface to the type_expression module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef TYPE_EXPRESSION_H
#define TYPE_EXPRESSION_H

#include "c_foundations/basic.h"


typedef enum
  {
    TEK_CONSTANT,
    TEK_NAME,
    TEK_ENUMERATION,
    TEK_NOT,
    TEK_INTERSECTION,
    TEK_UNION,
    TEK_XOR,
    TEK_EXPRESSION,
    TEK_ARRAY,
    TEK_INTEGER_RANGE,
    TEK_RATIONAL_RANGE,
    TEK_POINTER,
    TEK_TYPE,
    TEK_MAP,
    TEK_ROUTINE,
    TEK_FIELDS,
    TEK_LEPTON,
    TEK_MULTISET,
    TEK_INTERFACE,
    TEK_SEMI_LABELED_VALUE_LIST,
    TEK_REGULAR_EXPRESSION
  } type_expression_kind;

typedef struct type_expression type_expression;


#include "type.h"
#include "expression.h"
#include "regular_expression.h"
#include "source_location.h"


extern type_expression *create_constant_type_expression(type *the_type);
extern type_expression *create_name_type_expression(
        expression *name_expression);
extern type_expression *create_enumeration_type_expression(void);
extern type_expression *create_not_type_expression(type_expression *base);
extern type_expression *create_intersection_type_expression(
        type_expression *left, type_expression *right);
extern type_expression *create_union_type_expression(type_expression *left,
                                                     type_expression *right);
extern type_expression *create_xor_type_expression(type_expression *left,
                                                   type_expression *right);
extern type_expression *create_expression_type_expression(
        expression *the_expression);
extern type_expression *create_array_type_expression(type_expression *base,
        expression *lower_bound, expression *upper_bound);
extern type_expression *create_integer_range_type_expression(
        expression *lower_bound, expression *upper_bound,
        boolean lower_is_inclusive, boolean upper_is_inclusive);
extern type_expression *create_rational_range_type_expression(
        expression *lower_bound, expression *upper_bound,
        boolean lower_is_inclusive, boolean upper_is_inclusive);
extern type_expression *create_pointer_type_expression(type_expression *base,
        boolean read_allowed, boolean write_allowed, boolean null_allowed);
extern type_expression *create_type_type_expression(type_expression *base);
extern type_expression *create_map_type_expression(type_expression *key,
                                                   type_expression *target);
extern type_expression *create_routine_type_expression(
        type_expression *return_type, boolean extra_arguments_allowed);
extern type_expression *create_fields_type_expression(
        boolean extra_fields_allowed);
extern type_expression *create_lepton_type_expression(expression *lepton,
        boolean extra_fields_allowed);
extern type_expression *create_multiset_type_expression(
        boolean extra_fields_allowed);
extern type_expression *create_interface_type_expression(boolean null_allowed);
extern type_expression *create_semi_labeled_value_list_type_expression(
        boolean extra_elements_allowed);
extern type_expression *create_regular_expression_type_expression(
        regular_expression *the_regular_expression);

extern void delete_type_expression(type_expression *the_type_expression);

extern type_expression_kind get_type_expression_kind(
        type_expression *the_type_expression);
extern type *constant_type_expression_type(
        type_expression *constant_type_expression);
extern expression *name_type_expression_name_expression(
        type_expression *name_type_expression);
extern size_t enumeration_type_expression_case_count(
        type_expression *enumeration_type_expression);
extern expression *enumeration_type_expression_case(
        type_expression *enumeration_type_expression, size_t case_num);
extern type_expression *not_type_expression_base(
        type_expression *not_type_expression);
extern type_expression *intersection_type_expression_left(
        type_expression *intersection_type_expression);
extern type_expression *intersection_type_expression_right(
        type_expression *intersection_type_expression);
extern type_expression *union_type_expression_left(
        type_expression *union_type_expression);
extern type_expression *union_type_expression_right(
        type_expression *union_type_expression);
extern type_expression *xor_type_expression_left(
        type_expression *xor_type_expression);
extern type_expression *xor_type_expression_right(
        type_expression *xor_type_expression);
extern expression *expression_type_expression_expression(
        type_expression *expression_type_expression);
extern type_expression *array_type_expression_base(
        type_expression *array_type_expression);
extern expression *array_type_expression_lower_bound(
        type_expression *array_type_expression);
extern expression *array_type_expression_upper_bound(
        type_expression *array_type_expression);
extern expression *integer_range_type_expression_lower_bound(
        type_expression *integer_range_type_expression);
extern expression *integer_range_type_expression_upper_bound(
        type_expression *integer_range_type_expression);
extern boolean integer_range_type_expression_lower_is_inclusive(
        type_expression *integer_range_type_expression);
extern boolean integer_range_type_expression_upper_is_inclusive(
        type_expression *integer_range_type_expression);
extern expression *rational_range_type_expression_lower_bound(
        type_expression *rational_range_type_expression);
extern expression *rational_range_type_expression_upper_bound(
        type_expression *rational_range_type_expression);
extern boolean rational_range_type_expression_lower_is_inclusive(
        type_expression *rational_range_type_expression);
extern boolean rational_range_type_expression_upper_is_inclusive(
        type_expression *rational_range_type_expression);
extern type_expression *pointer_type_expression_base(
        type_expression *pointer_type_expression);
extern boolean pointer_type_expression_read_allowed(
        type_expression *pointer_type_expression);
extern boolean pointer_type_expression_write_allowed(
        type_expression *pointer_type_expression);
extern boolean pointer_type_expression_null_allowed(
        type_expression *pointer_type_expression);
extern type_expression *type_type_expression_base(
        type_expression *type_type_expression);
extern type_expression *map_type_expression_key(
        type_expression *map_type_expression);
extern type_expression *map_type_expression_target(
        type_expression *map_type_expression);
extern type_expression *routine_type_expression_return_type(
        type_expression *routine_type_expression);
extern boolean routine_type_expression_extra_arguments_allowed(
        type_expression *routine_type_expression);
extern boolean routine_type_expression_extra_arguments_unspecified(
        type_expression *routine_type_expression);
extern size_t routine_type_expression_formal_count(
        type_expression *routine_type_expression);
extern type_expression *routine_type_expression_formal_argument_type(
        type_expression *routine_type_expression, size_t formal_num);
extern const char *routine_type_expression_formal_name(
        type_expression *routine_type_expression, size_t formal_num);
extern boolean routine_type_expression_formal_has_default_value(
        type_expression *routine_type_expression, size_t formal_num);
extern boolean fields_type_expression_extra_fields_allowed(
        type_expression *fields_type_expression);
extern size_t fields_type_expression_field_count(
        type_expression *fields_type_expression);
extern type_expression *fields_type_expression_field_type(
        type_expression *fields_type_expression, size_t field_num);
extern const char *fields_type_expression_field_name(
        type_expression *fields_type_expression, size_t field_num);
extern expression *lepton_type_expression_lepton(
        type_expression *lepton_type_expression);
extern boolean lepton_type_expression_extra_fields_allowed(
        type_expression *lepton_type_expression);
extern size_t lepton_type_expression_field_count(
        type_expression *lepton_type_expression);
extern type_expression *lepton_type_expression_field_type(
        type_expression *lepton_type_expression, size_t field_num);
extern const char *lepton_type_expression_field_name(
        type_expression *lepton_type_expression, size_t field_num);
extern boolean multiset_type_expression_extra_fields_allowed(
        type_expression *multiset_type_expression);
extern size_t multiset_type_expression_field_count(
        type_expression *multiset_type_expression);
extern type_expression *multiset_type_expression_field_type(
        type_expression *multiset_type_expression, size_t field_num);
extern const char *multiset_type_expression_field_name(
        type_expression *multiset_type_expression, size_t field_num);
extern boolean interface_type_expression_null_allowed(
        type_expression *interface_type_expression);
extern size_t interface_type_expression_item_count(
        type_expression *interface_type_expression);
extern type_expression *interface_type_expression_item_type(
        type_expression *interface_type_expression, size_t item_num);
extern const char *interface_type_expression_item_name(
        type_expression *interface_type_expression, size_t item_num);
extern boolean interface_type_expression_item_writing_allowed(
        type_expression *interface_type_expression, size_t item_num);
extern boolean semi_labeled_value_list_type_expression_extra_elements_allowed(
        type_expression *semi_labeled_value_list_type_expression);
extern size_t semi_labeled_value_list_type_expression_element_count(
        type_expression *semi_labeled_value_list_type_expression);
extern type_expression *semi_labeled_value_list_type_expression_element_type(
        type_expression *semi_labeled_value_list_type_expression,
        size_t element_num);
extern const char *semi_labeled_value_list_type_expression_element_name(
        type_expression *semi_labeled_value_list_type_expression,
        size_t element_num);
extern regular_expression *
        regular_expression_type_expression_regular_expression(
                type_expression *regular_expression_type_expression);

extern void set_type_expression_start_location(
        type_expression *the_type_expression, const source_location *location);
extern void set_type_expression_end_location(
        type_expression *the_type_expression, const source_location *location);

extern verdict enumeration_type_expression_add_case(
        type_expression *enumeration_type_expression,
        expression *case_expression);
extern verdict routine_type_expression_add_formal(
        type_expression *routine_type_expression,
        type_expression *argument_type, const char *name,
        boolean has_default_value);
extern verdict routine_type_expression_set_extra_arguments_allowed(
        type_expression *routine_type_expression,
        boolean extra_arguments_allowed);
extern verdict routine_type_expression_set_extra_arguments_unspecified(
        type_expression *routine_type_expression,
        boolean extra_arguments_unspecified);
extern verdict fields_type_expression_add_field(
        type_expression *fields_type_expression, type_expression *field_type,
        const char *name);
extern verdict fields_type_expression_set_extra_fields_allowed(
        type_expression *fields_type_expression, boolean extra_fields_allowed);
extern verdict lepton_type_expression_add_field(
        type_expression *lepton_type_expression, type_expression *field_type,
        const char *name);
extern verdict lepton_type_expression_set_extra_fields_allowed(
        type_expression *lepton_type_expression, boolean extra_fields_allowed);
extern verdict multiset_type_expression_add_field(
        type_expression *multiset_type_expression, type_expression *field_type,
        const char *name);
extern verdict multiset_type_expression_set_extra_fields_allowed(
        type_expression *multiset_type_expression,
        boolean extra_fields_allowed);
extern verdict interface_type_expression_add_item(
        type_expression *interface_type_expression, type_expression *item_type,
        const char *name, boolean writing_allowed);
extern verdict semi_labeled_value_list_type_expression_add_element(
        type_expression *semi_labeled_value_list_type_expression,
        type_expression *element_type, const char *name);
extern verdict
        semi_labeled_value_list_type_expression_set_extra_elements_allowed(
                type_expression *semi_labeled_value_list_type_expression,
                boolean extra_elements_allowed);

extern const source_location *get_type_expression_location(
        type_expression *the_type_expression);

extern void type_expression_error(type_expression *the_type_expression,
                                  const char *format, ...);
extern void vtype_expression_error(type_expression *the_type_expression,
                                   const char *format, va_list arg);


#endif /* TYPE_EXPRESSION_H */
