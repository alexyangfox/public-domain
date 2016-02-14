/* file "value.h" */

/*
 *  This file contains the interface to the value module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef VALUE_H
#define VALUE_H


typedef enum value_kind
  {
    VK_TRUE,
    VK_FALSE,
    VK_INTEGER,
    VK_RATIONAL,
    VK_STRING,
    VK_CHARACTER,
    VK_REGULAR_EXPRESSION,
    VK_SEMI_LABELED_VALUE_LIST,
    VK_SEMI_LABELED_MULTI_SET,
    VK_MAP,
    VK_QUARK,
    VK_LEPTON,
    VK_LEPTON_KEY,
    VK_SLOT_LOCATION,
    VK_NULL,
    VK_JUMP_TARGET,
    VK_ROUTINE,
    VK_ROUTINE_CHAIN,
    VK_TYPE,
    VK_OBJECT,
    VK_TAGALONG_KEY,
    VK_LOCK
  } value_kind;

typedef struct value value;
typedef struct value_tagalong_handle value_tagalong_handle;


#include "o_integer.h"
#include "rational.h"
#include "regular_expression.h"
#include "quark.h"
#include "lepton_key_instance.h"
#include "slot_location.h"
#include "jump_target.h"
#include "routine_instance.h"
#include "routine_instance_chain.h"
#include "context.h"
#include "type.h"
#include "object.h"
#include "tagalong_key.h"
#include "lock_instance.h"
#include "validator.h"
#include "reference_cluster.h"


extern value *create_true_value(void);
extern value *create_false_value(void);
extern value *create_integer_value(o_integer the_integer);
extern value *create_rational_value(rational *the_rational);
extern value *create_string_value(const char *string_data);
extern value *create_character_value(const char *character_data);
extern value *create_regular_expression_value(
        regular_expression *the_regular_expression);
extern value *create_semi_labeled_value_list_value(void);
extern value *create_semi_labeled_multi_set_value(void);
extern value *create_map_value(void);
extern value *create_quark_value(quark *the_quark);
extern value *create_lepton_value(lepton_key_instance *key);
extern value *create_lepton_key_value(lepton_key_instance *key);
extern value *create_slot_location_value(slot_location *the_slot_location);
extern value *create_null_value(void);
extern value *create_jump_target_value(jump_target *target);
extern value *create_routine_value(routine_instance *the_routine_instance);
extern value *create_routine_chain_value(routine_instance_chain *chain);
extern value *create_type_value(type *the_type);
extern value *create_object_value(object *the_object);
extern value *create_tagalong_key_value(tagalong_key *key);
extern value *create_lock_value(lock_instance *the_lock_instance);

extern value_kind get_value_kind(value *the_value);
extern o_integer integer_value_data(value *the_value);
extern rational *rational_value_data(value *the_value);
extern const char *string_value_data(value *the_value);
extern const char *character_value_data(value *the_value);
extern regular_expression *regular_expression_value_data(value *the_value);
extern size_t value_component_count(value *the_value);
extern const char *value_component_label(value *the_value,
                                         size_t component_number);
extern value *value_component_value(value *the_value, size_t component_number);
extern value *value_get_field(const char *label, value *the_value);
extern size_t value_get_field_index(const char *label, value *the_value);
extern size_t map_value_item_count(value *map_value);
extern boolean map_value_item_is_type(value *map_value, size_t item_num);
extern type *map_value_item_key_type(value *map_value, size_t item_num);
extern value *map_value_item_key_value(value *map_value, size_t item_num);
extern value *map_value_item_target(value *map_value, size_t item_num);
extern boolean map_value_item_is_definitely_touchable(value *map_value,
        size_t item_num, const source_location *location, jumper *the_jumper);
extern boolean map_value_is_array(value *map_value, boolean *doubt,
        const source_location *location, jumper *the_jumper);
extern void map_value_integer_key_bounds(value *map_value, o_integer *minimum,
        o_integer *maximum, boolean *min_doubt, boolean *max_doubt,
        const source_location *location, jumper *the_jumper);
extern value *map_value_lookup(value *map_value, value *key, boolean *doubt,
        const source_location *location, jumper *the_jumper);
extern quark *value_quark(value *the_value);
extern lepton_key_instance *value_lepton_key(value *the_value);
extern slot_location *slot_location_value_data(value *the_value);
extern jump_target *jump_target_value_data(value *the_value);
extern routine_instance *routine_value_data(value *the_value);
extern routine_instance_chain *routine_chain_value_data(value *the_value);
extern type *type_value_data(value *the_value);
extern object *object_value_data(value *the_value);
extern tagalong_key *tagalong_key_value_data(value *the_value);
extern lock_instance *lock_value_data(value *the_value);

extern verdict add_field(value *base_value, const char *label,
                         value *field_value);
extern void set_field(value *base_value, const char *label, value *field_value,
                      jumper *the_jumper);
extern value *map_value_set(value *map_value, value *key, value *target,
        const source_location *location, jumper *the_jumper);
extern value *map_value_set_filter(value *map_value, type *filter,
        value *target, const source_location *location, jumper *the_jumper);
extern void convert_semi_labeled_value_list_value_to_multi_set(
        value *the_value);

extern boolean value_has_only_one_reference(value *the_value);

extern value *copy_value(value *the_value);

extern value *map_value_from_semi_labeled_value_list(value *source);

extern boolean value_has_only_named_fields(value *the_value);

extern void value_add_reference(value *the_value);
extern void value_remove_reference(value *the_value, jumper *the_jumper);
extern void value_add_reference_with_reference_cluster(value *the_value,
        reference_cluster *cluster);
extern void value_remove_reference_with_reference_cluster(value *the_value,
        jumper *the_jumper, reference_cluster *cluster);
extern reference_cluster *value_reference_cluster(value *the_value);

extern boolean value_is_valid(value *the_value);
extern void check_value_validity(value *the_value,
        const source_location *location, jumper *the_jumper);
extern void check_value_validity_except_map_targets(value *the_value,
        const source_location *location, jumper *the_jumper);
extern validator *value_validator(value *the_value);
extern boolean map_value_all_keys_are_valid(value *map_value);
extern validator *map_value_all_keys_validator(value *map_value);
extern boolean value_is_valid_except_map_targets(value *the_value);

extern value *value_string_concatenate(value *left, value *right);

extern boolean values_are_equal(value *value1, value *value2, boolean *doubt,
        const source_location *location, jumper *the_jumper);

/*
 *  The value_structural_order() function provides an ordering of values.  It
 *  has the following properties:
 *
 *    * value_structural_order() returns one of the values -2, -1, 0, or 1.  -2
 *      indicates an error while trying to determine the structurual ordering
 *      -- and the only kind of error that can happen during this process is a
 *      memory allocation error.  -1 indicates that the value pointed to by
 *      ``left'' is less than the value pointed to by ``right'' in the
 *      structural ordering.  0 indicates the values are equivalent in the
 *      structural ordering.  1 indicates value pointed to by ``left'' is
 *      greater than the value pointed to by ``right'' in the structural
 *      ordering.  The following properties apply only in the non-error case.
 *    * value_structural_order(a, b) == -value_structural_order(b, a) for all a
 *      and b.
 *    * If value_structural_order(a, b) is zero, then values_are_equal() will
 *      indicate they are equal without doubt, unless there is an error during
 *      evaluation of values_are_equal().  Note that the converse is not true
 *      -- if value_structural_order(a, b) is non-zero it is still possible
 *      that values_are_equal() will say they are equal without doubt.
 *    * The ordering is transitive -- value_structural_order(a, b) <= 0 and
 *      value_structural_order(b, c) <= 0 implies value_structural_order(a, c)
 *      <= 0.  Also, value_structural_order(a, b) < 0 and
 *      value_structural_order(b, c) < 0 implies value_structural_order(a, c) <
 *      0.
 */
extern int value_structural_order(value *left, value *right);

/*
 *  The value_is_slippery() function provides some information about a value
 *  that relates to the value_structural_order() function.  In particular, if
 *  !value_is_slippery(a) and !value_is_slippery(b), then
 *  value_structural_order(a, b) != 0 implies that values_are_equal() will
 *  always say that the values are unequal without doubt, unless there is an
 *  error during the execution of values_are_equal.
 */
extern boolean value_is_slippery(value *the_value);

extern void print_value(value *the_value,
        void (*printer)(void *data, const char *format, ...), void *data);
extern void print_value_with_override(value *the_value,
        void (*printer)(void *data, const char *format, ...), void *data,
        void (*override)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data));
extern void print_oi(o_integer oi,
        void (*printer)(void *data, const char *format, ...), void *data);
extern void fp_printer(void *data, const char *format, ...);

extern value *lookup_tagalong(value *base_value, tagalong_key *key,
        boolean is_for_write, const source_location *location,
        jumper *the_jumper);
extern void set_tagalong(value *base_value, tagalong_key *key,
        value *new_value, const source_location *location, jumper *the_jumper);

extern void kill_value_tagalong(value_tagalong_handle *handle,
                                jumper *the_jumper);

extern void assert_is_live_value(value *the_value);

DEFINE_EXCEPTION_TAG(object_tagalong_read_non_object);
DEFINE_EXCEPTION_TAG(object_tagalong_write_non_object);


#endif /* VALUE_H */
