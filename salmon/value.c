/* file "value.c" */

/*
 *  This file contains the implementation of the value module.
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
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/auto_array.h"
#include "c_foundations/string_index.h"
#include "c_foundations/auto_array_implementation.h"
#include "value.h"
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
#include "unicode.h"
#include "driver.h"
#include "validator.h"
#include "reference_cluster.h"
#include "platform_dependent.h"


AUTO_ARRAY(mstring_aa, char *);
AUTO_ARRAY(value_aa, value *);
AUTO_ARRAY(type_aa, type *);
AUTO_ARRAY(size_t_aa, size_t);

typedef struct
  {
    size_t the_size_t;
    void *pointer;
  } size_t_and_pointer;

typedef struct
  {
    size_t index;
    value *new_target;
  } map_target_replacement;

AUTO_ARRAY(map_target_replacement_aa, map_target_replacement);

typedef struct
  {
    value_aa value_key_keys;
    value_aa value_key_targets;
    type_aa type_key_keys;
    value_aa type_key_targets;
    size_t item_count;
    string_index *string_key_index;
    string_index *integer_key_index;
    size_t non_integer_key_count;
    size_t slippery_key_count;
    size_t slippery_count;
    validator *all_keys_validator;
    boolean is_extension;
    struct
      {
        value *base;
        size_t_aa overlaps;
        string_index *overlap_index;
        boolean overlaps_sorted;
        DECLARE_SYSTEM_LOCK(overlap_sorting_lock);
        map_target_replacement_aa replacements;
        string_index *replacement_index;
        boolean replacements_sorted;
        DECLARE_SYSTEM_LOCK(replacement_lock);
        boolean is_uncompressable;
      } extension;
    DECLARE_SYSTEM_LOCK(lock);
    reference_cluster *reference_cluster;
    size_t cluster_use_count;
    size_t negated_cluster_use_count;
    size_t reference_count;
  } map_info;

struct value
  {
    value_kind kind;
    union
      {
        char *string_data;
        char character_data[5];
        o_integer integer;
        rational *rational;
        regular_expression *regular_expression;
        quark *quark;
        struct
          {
            lepton_key_instance *lepton_key;
            validator *key_validator;
            mstring_aa labels;
            value_aa values;
            string_index *index;
            size_t labeled_element_count;
            size_t_and_pointer *ordered_labeled_elements;
            size_t unlabeled_element_count;
            size_t_and_pointer *ordered_unlabeled_elements;
            value *map_equivalent;
            size_t slippery_count;
            DECLARE_SYSTEM_LOCK(lock);
          } lepton;
        struct
          {
            map_info *map_info;
            value *first_extension;
            value *next_extension;
            value *previous_extension;
            DECLARE_SYSTEM_LOCK(lock);
          } map;
        slot_location *slot_location;
        jump_target *jump_target;
        routine_instance *routine;
        routine_instance_chain *routine_instance_chain;
        type *type;
        object *object;
        tagalong_key *tagalong_key;
        lock_instance *lock_instance;
      } u;
    DECLARE_SYSTEM_LOCK(tagalong_lock);
    value_tagalong_handle *tagalong_chain;
    reference_cluster *reference_cluster;
    size_t cluster_use_count;
    validator *validator;
    DECLARE_SYSTEM_LOCK(lock);
    size_t reference_count;
    size_t no_cluster_reference_count;
    boolean destructing;
  };

struct value_tagalong_handle
  {
    tagalong_key *key;
    value *field_value;
    value *parent;
    value_tagalong_handle *value_previous;
    value_tagalong_handle *value_next;
    value_tagalong_handle *key_previous;
    value_tagalong_handle *key_next;
  };


#define EXTENDED_MAP_COPY_CONVERT_SIZE 2
#define UNEXTENDED_MAP_COPY_EXTEND_SIZE 2


AUTO_ARRAY_IMPLEMENTATION(size_t_aa, size_t, 0);
AUTO_ARRAY_IMPLEMENTATION(map_target_replacement_aa, map_target_replacement,
                          0);


static void delete_value(value *the_value, jumper *the_jumper);
static void delete_value_common(value *the_value);
static value *map_value_lookup_with_index(value *map_value, value *key,
        boolean *doubt, size_t *index, const source_location *location,
        jumper *the_jumper);
static verdict map_value_insert(value *map_value, value *key, value *target,
        const source_location *location, jumper *the_jumper);
static value *create_empty_value(value_kind kind);
static verdict initialize_lepton_components(value *the_value);
static verdict initialize_map_components(value *the_value,
        size_t initial_value_key_space, size_t initial_type_key_space);
static void deallocate_lepton_components(value *the_value, jumper *the_jumper);
static verdict copy_lepton_components(value *target, value *source);
static boolean semi_labeled_value_lists_are_equal(value *value1, value *value2,
        boolean *doubt, const source_location *location, jumper *the_jumper);
static int semi_labeled_value_list_structural_order(value *left, value *right);
static boolean semi_labeled_value_list_is_slippery(value *the_value);
static boolean semi_labeled_multi_sets_are_equal(value *value1, value *value2,
        boolean *doubt, const source_location *location, jumper *the_jumper);
static int semi_labeled_multi_set_structural_order(value *left, value *right);
static boolean semi_labeled_multi_set_is_slippery(value *the_value);
static boolean maps_are_equal(value *value1, value *value2, boolean *doubt,
        const source_location *location, jumper *the_jumper);
static boolean map_all_value_keys_match_second_map(value *to_test,
        value *other, boolean *doubt, const source_location *location,
        jumper *the_jumper);
static size_t *map_filter_items(value *map_value, size_t *filter_count,
                                jumper *the_jumper);
static type **map_precise_key_types(value *map_value, size_t filter_count,
        size_t *filters, type **all_keys_type, const source_location *location,
        jumper *the_jumper);
static int map_structural_order(value *left, value *right);
static boolean map_is_slippery(value *the_value);
static boolean map_and_semi_labeled_value_list_are_equal(value *map_value,
        value *value2, boolean *doubt, const source_location *location,
        jumper *the_jumper);
static size_t map_info_local_index_for_key(map_info *info, value *key,
        boolean *found, boolean *doubt,
        boolean care_about_position_if_not_found,
        const source_location *location, jumper *the_jumper);
static size_t map_value_global_index_for_key(value *map_value, value *key,
        boolean *found, boolean *doubt,
        boolean care_about_position_if_not_found,
        const source_location *location, jumper *the_jumper);
static size_t map_info_find_index_for_key(map_info *info, value *key,
        boolean *found, boolean *doubt, boolean *error,
        boolean *match_possible, boolean care_about_position_if_not_possible);
static char *key_string_for_integer(value *integer_value,
                                    boolean *deallocation_needed);
static verdict map_info_append_value_key_item(map_info *info, value *key,
                                              value *target);
static verdict map_info_append_type_key_item(map_info *info, type *key,
                                             value *target);
static verdict update_map_indexes(map_info *info, value *key,
                                  size_t target_position);
static boolean semi_labeled_value_list_value_has_no_labels(value *the_value);
static verdict generate_lepton_field_order_information(value *the_value);
static int compare_size_t_and_string(const void *left, const void *right);
static int compare_size_t_and_value(const void *left, const void *right);
static int compare_size_t(const void *left, const void *right);
static int compare_map_target_replacement(const void *left, const void *right);
static void print_component_value(value *the_value,
        void (*printer)(void *data, const char *format, ...), void *data,
        void (*override)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data));
static verdict map_value_consolidate(value *map_value);
static void clear_map_fields(value *the_value, jumper *the_jumper);
static void delete_tagalongs(value *the_value, jumper *the_jumper);
static verdict make_map_copy(value *old_value, value *new_value);
static verdict enter_into_map_overlap_index(map_info *info, size_t item_num);
static verdict enter_replacement(map_info *info, size_t item_num,
                                 value *new_target, jumper *the_jumper);
static verdict enter_into_map_replacement_index(map_info *info,
        size_t base_item_num, size_t replacement_list_position);
static verdict set_up_map_extension(value *old_value, value *new_value);
static size_t map_item_num_for_extension_base(map_info *info, size_t item_num);
static value *map_base_target(map_info *info, size_t item_num);
static value *map_base_replacement(map_info *info, size_t item_num);
static void merge_extension(value *the_extension, value *the_value,
                            jumper *the_jumper);
static void replace_value_key_target(map_info *info, size_t item_num,
        value *new_target, value *key, validator **the_validator,
        const source_location *location, jumper *the_jumper);
static void update_info_for_value_key_target(map_info *info, value *old_target,
        value *new_target, value *key, validator **the_validator,
        jumper *the_jumper);
static map_info *create_map_info(size_t initial_value_key_space,
                                 size_t initial_type_key_space);
static void map_info_add_reference(map_info *info);
static void map_info_remove_reference(map_info *info, jumper *the_jumper);
static verdict map_info_bubble_value_keys(map_info *info, size_t position,
        size_t to_move, value *key, value *target);
static verdict initialize_extension_info(map_info *info,
        size_t initial_overlap_space, size_t initial_replacement_space);
static boolean might_hit_map_type_key(map_info *info, value *test_key,
                                      jumper *the_jumper);
static void add_intervalue_reference(value *from, value *to);
static void add_map_info_to_value_reference(map_info *from, value *to);
static void remove_intervalue_reference(value *from, value *to,
                                        jumper *the_jumper);
static void remove_map_info_to_value_reference(map_info *from, value *to,
                                               jumper *the_jumper);
static void add_map_info_to_type_reference(map_info *from, type *to);
static void remove_map_info_to_type_reference(map_info *from, type *to,
                                              jumper *the_jumper);
static void value_remove_reference_skip_cluster(value *the_value,
                                                jumper *the_jumper);
static void update_value_cluster_for_map_info(value *the_value,
        reference_cluster *cluster, jumper *the_jumper);
static void map_info_set_extension_base(map_info *info, value *new_base);


extern value *create_true_value(void)
  {
    return create_empty_value(VK_TRUE);
  }

extern value *create_false_value(void)
  {
    return create_empty_value(VK_FALSE);
  }

extern value *create_integer_value(o_integer the_integer)
  {
    value *result;

    assert(!(oi_out_of_memory(the_integer)));

    result = create_empty_value(VK_INTEGER);
    if (result == NULL)
        return NULL;

    oi_add_reference(the_integer);
    result->u.integer = the_integer;

    return result;
  }

extern value *create_rational_value(rational *the_rational)
  {
    value *result;

    assert(the_rational != NULL);

    result = create_empty_value(VK_RATIONAL);
    if (result == NULL)
        return NULL;

    rational_add_reference(the_rational);
    result->u.rational = the_rational;

    return result;
  }

extern value *create_string_value(const char *string_data)
  {
    const char *follow;
    value *result;
    char *copy;

    assert(string_data != NULL);

    follow = string_data;
    while (*follow != 0)
      {
        int byte_count;

        byte_count = validate_utf8_character(follow);
        assert(byte_count > 0);
        follow += byte_count;
      }

    result = create_empty_value(VK_STRING);
    if (result == NULL)
        return NULL;

    copy = MALLOC_ARRAY(char, strlen(string_data) + 1);
    if (copy == NULL)
      {
        delete_value_common(result);
        return NULL;
      }
    strcpy(copy, string_data);
    result->u.string_data = copy;

    return result;
  }

extern value *create_character_value(const char *character_data)
  {
    value *result;

    assert(character_data != NULL);
    assert(strlen(character_data) <= 4);

    result = create_empty_value(VK_CHARACTER);
    if (result == NULL)
        return NULL;

    strcpy(&(result->u.character_data[0]), character_data);

    return result;
  }

extern value *create_regular_expression_value(
        regular_expression *the_regular_expression)
  {
    value *result;

    assert(the_regular_expression != NULL);

    result = create_empty_value(VK_REGULAR_EXPRESSION);
    if (result == NULL)
        return NULL;

    regular_expression_add_reference(the_regular_expression);
    result->u.regular_expression = the_regular_expression;

    return result;
  }

extern value *create_semi_labeled_value_list_value(void)
  {
    value *result;
    verdict the_verdict;

    result = create_empty_value(VK_SEMI_LABELED_VALUE_LIST);
    if (result == NULL)
        return NULL;

    the_verdict = initialize_lepton_components(result);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_value_common(result);
        return NULL;
      }

    result->u.lepton.lepton_key = NULL;
    result->u.lepton.key_validator = NULL;

    return result;
  }

extern value *create_semi_labeled_multi_set_value(void)
  {
    value *result;
    verdict the_verdict;

    result = create_empty_value(VK_SEMI_LABELED_MULTI_SET);
    if (result == NULL)
        return NULL;

    the_verdict = initialize_lepton_components(result);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_value_common(result);
        return NULL;
      }

    result->u.lepton.lepton_key = NULL;
    result->u.lepton.key_validator = NULL;

    return result;
  }

extern value *create_map_value(void)
  {
    verdict the_verdict;
    value *result;

    result = create_empty_value(VK_MAP);
    if (result == NULL)
        return NULL;

    the_verdict = initialize_map_components(result, 10, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_value_common(result);
        return NULL;
      }

    result->u.map.first_extension = NULL;

    return result;
  }

extern value *create_quark_value(quark *the_quark)
  {
    value *result;

    assert(the_quark != NULL);

    result = create_empty_value(VK_QUARK);
    if (result == NULL)
        return NULL;

    assert(result->reference_cluster == NULL);
    result->reference_cluster = quark_reference_cluster(the_quark);
    if (result->reference_cluster != NULL)
      {
        reference_cluster_add_reference(result->reference_cluster);
        result->cluster_use_count = 1;
      }
    quark_add_reference_with_cluster(the_quark, result->reference_cluster);
    result->u.quark = the_quark;

    result->validator = validator_add_instance(result->validator,
            quark_instance_instance(the_quark));
    if (result->validator == NULL)
      {
        delete_value(result, NULL);
        return NULL;
      }

    return result;
  }

extern value *create_lepton_value(lepton_key_instance *key)
  {
    value *result;
    verdict the_verdict;

    assert(key != NULL);

    result = create_empty_value(VK_LEPTON);
    if (result == NULL)
        return NULL;

    the_verdict = initialize_lepton_components(result);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_value_common(result);
        return NULL;
      }

    assert(result->reference_cluster == NULL);
    result->reference_cluster = lepton_key_instance_reference_cluster(key);
    if (result->reference_cluster != NULL)
      {
        reference_cluster_add_reference(result->reference_cluster);
        result->cluster_use_count = 1;
      }
    lepton_key_instance_add_reference_with_cluster(key,
                                                   result->reference_cluster);
    result->u.lepton.lepton_key = key;

    result->u.lepton.key_validator = get_trivial_validator();
    assert(result->u.lepton.key_validator != NULL);

    result->validator = validator_add_instance(result->validator,
            lepton_key_instance_instance(key));
    if (result->validator == NULL)
      {
        delete_value(result, NULL);
        return NULL;
      }

    result->u.lepton.key_validator = validator_add_instance(
            result->u.lepton.key_validator, lepton_key_instance_instance(key));
    if (result->u.lepton.key_validator == NULL)
      {
        delete_value(result, NULL);
        return NULL;
      }

    return result;
  }

extern value *create_lepton_key_value(lepton_key_instance *key)
  {
    value *result;

    assert(key != NULL);

    result = create_empty_value(VK_LEPTON_KEY);
    if (result == NULL)
        return NULL;

    assert(result->reference_cluster == NULL);
    result->reference_cluster = lepton_key_instance_reference_cluster(key);
    if (result->reference_cluster != NULL)
      {
        reference_cluster_add_reference(result->reference_cluster);
        result->cluster_use_count = 1;
      }
    lepton_key_instance_add_reference_with_cluster(key,
                                                   result->reference_cluster);
    result->u.lepton.lepton_key = key;

    result->validator = validator_add_instance(result->validator,
            lepton_key_instance_instance(key));
    if (result->validator == NULL)
      {
        delete_value(result, NULL);
        return NULL;
      }

    result->u.lepton.key_validator = NULL;

    return result;
  }

extern value *create_slot_location_value(slot_location *the_slot_location)
  {
    value *result;

    assert(the_slot_location != NULL);

    result = create_empty_value(VK_SLOT_LOCATION);
    if (result == NULL)
        return NULL;

    assert(result->reference_cluster == NULL);
    result->reference_cluster =
            slot_location_reference_cluster(the_slot_location);
    if (result->reference_cluster != NULL)
      {
        reference_cluster_add_reference(result->reference_cluster);
        result->cluster_use_count = 1;
      }
    slot_location_add_reference_with_cluster(the_slot_location,
                                             result->reference_cluster);
    result->u.slot_location = the_slot_location;

    result->validator = validator_add_validator(result->validator,
            slot_location_validator(the_slot_location));
    if (result->validator == NULL)
      {
        delete_value(result, NULL);
        return NULL;
      }

    return result;
  }

extern value *create_null_value(void)
  {
    return create_empty_value(VK_NULL);
  }

extern value *create_jump_target_value(jump_target *target)
  {
    value *result;

    assert(target != NULL);

    result = create_empty_value(VK_JUMP_TARGET);
    if (result == NULL)
        return NULL;

    jump_target_add_reference(target);
    result->u.jump_target = target;

    result->validator = validator_add_jump_target(result->validator, target);
    if (result->validator == NULL)
      {
        delete_value(result, NULL);
        return NULL;
      }

    return result;
  }

extern value *create_routine_value(routine_instance *the_routine_instance)
  {
    value *result;

    assert(the_routine_instance != NULL);

    result = create_empty_value(VK_ROUTINE);
    if (result == NULL)
        return NULL;

    assert(result->reference_cluster == NULL);
    result->reference_cluster =
            routine_instance_reference_cluster(the_routine_instance);
    if (result->reference_cluster != NULL)
      {
        reference_cluster_add_reference(result->reference_cluster);
        result->cluster_use_count = 1;
      }
    routine_instance_add_reference_with_cluster(the_routine_instance,
                                                result->reference_cluster);
    result->u.routine = the_routine_instance;

    result->validator = validator_add_instance(result->validator,
            routine_instance_instance(the_routine_instance));
    if (result->validator == NULL)
      {
        delete_value(result, NULL);
        return NULL;
      }

    return result;
  }

extern value *create_routine_chain_value(routine_instance_chain *chain)
  {
    value *result;

    assert(chain != NULL);

    result = create_empty_value(VK_ROUTINE_CHAIN);
    if (result == NULL)
        return NULL;

    assert(result->reference_cluster == NULL);
    result->reference_cluster =
            routine_instance_chain_reference_cluster(chain);
    if (result->reference_cluster != NULL)
      {
        reference_cluster_add_reference(result->reference_cluster);
        result->cluster_use_count = 1;
      }
    routine_instance_chain_add_reference_with_cluster(chain,
            result->reference_cluster);

    result->u.routine_instance_chain = chain;

    result->validator = validator_add_validator(result->validator,
            routine_instance_chain_validator(chain));
    if (result->validator == NULL)
      {
        delete_value(result, NULL);
        return NULL;
      }

    return result;
  }

extern value *create_type_value(type *the_type)
  {
    value *result;

    assert(the_type != NULL);

    result = create_empty_value(VK_TYPE);
    if (result == NULL)
        return NULL;

    assert(result->reference_cluster == NULL);
    result->reference_cluster = type_reference_cluster(the_type);
    if (result->reference_cluster != NULL)
      {
        reference_cluster_add_reference(result->reference_cluster);
        result->cluster_use_count = 1;
      }
    type_add_reference_with_reference_cluster(the_type,
                                              result->reference_cluster);
    result->u.type = the_type;

    result->validator = validator_add_validator(result->validator,
                                                type_validator(the_type));
    if (result->validator == NULL)
      {
        delete_value(result, NULL);
        return NULL;
      }

    return result;
  }

extern value *create_object_value(object *the_object)
  {
    value *result;

    assert(the_object != NULL);

    result = create_empty_value(VK_OBJECT);
    if (result == NULL)
        return NULL;

    result->u.object = the_object;

    assert(result->reference_cluster == NULL);
    result->reference_cluster = object_reference_cluster(the_object);
    assert(result->reference_cluster != NULL);
    reference_cluster_add_reference(result->reference_cluster);
    result->cluster_use_count = 1;
    object_add_reference_with_cluster(the_object, result->reference_cluster);

    result->validator = validator_add_object(result->validator, the_object);
    if (result->validator == NULL)
      {
        delete_value(result, NULL);
        return NULL;
      }

    return result;
  }

extern value *create_tagalong_key_value(tagalong_key *key)
  {
    value *result;

    assert(key != NULL);

    result = create_empty_value(VK_TAGALONG_KEY);
    if (result == NULL)
        return NULL;

    assert(result->reference_cluster == NULL);
    result->reference_cluster = tagalong_key_reference_cluster(key);
    if (result->reference_cluster != NULL)
      {
        reference_cluster_add_reference(result->reference_cluster);
        result->cluster_use_count = 1;
      }
    tagalong_key_add_reference_with_cluster(key, result->reference_cluster);
    result->u.tagalong_key = key;

    result->validator = validator_add_instance(result->validator,
                                               tagalong_key_instance(key));
    if (result->validator == NULL)
      {
        delete_value(result, NULL);
        return NULL;
      }

    return result;
  }

extern value *create_lock_value(lock_instance *the_lock_instance)
  {
    value *result;

    assert(the_lock_instance != NULL);

    result = create_empty_value(VK_LOCK);
    if (result == NULL)
        return NULL;

    assert(result->reference_cluster == NULL);
    result->reference_cluster =
            lock_instance_reference_cluster(the_lock_instance);
    if (result->reference_cluster != NULL)
      {
        reference_cluster_add_reference(result->reference_cluster);
        result->cluster_use_count = 1;
      }
    lock_instance_add_reference_with_cluster(the_lock_instance,
                                             result->reference_cluster);
    result->u.lock_instance = the_lock_instance;

    result->validator = validator_add_instance(result->validator,
            lock_instance_instance(the_lock_instance));
    if (result->validator == NULL)
      {
        delete_value(result, NULL);
        return NULL;
      }

    return result;
  }

extern value_kind get_value_kind(value *the_value)
  {
    assert(the_value != NULL);

    return the_value->kind;
  }

extern o_integer integer_value_data(value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_INTEGER);
    assert(!(oi_out_of_memory(the_value->u.integer)));
    return the_value->u.integer;
  }

extern rational *rational_value_data(value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_RATIONAL);
    assert(the_value->u.rational != NULL);
    return the_value->u.rational;
  }

extern const char *string_value_data(value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_STRING);
    assert(the_value->u.string_data != NULL);
    return the_value->u.string_data;
  }

extern const char *character_value_data(value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_CHARACTER);
    return &(the_value->u.character_data[0]);
  }

extern regular_expression *regular_expression_value_data(value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_REGULAR_EXPRESSION);
    return the_value->u.regular_expression;
  }

extern size_t value_component_count(value *the_value)
  {
    size_t count;

    assert(the_value != NULL);

    assert((the_value->kind == VK_SEMI_LABELED_VALUE_LIST) ||
           (the_value->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (the_value->kind == VK_LEPTON));

    count = the_value->u.lepton.labels.element_count;
    assert(count == the_value->u.lepton.values.element_count);

    return count;
  }

extern const char *value_component_label(value *the_value,
                                         size_t component_number)
  {
    size_t count;

    assert(the_value != NULL);

    assert((the_value->kind == VK_SEMI_LABELED_VALUE_LIST) ||
           (the_value->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (the_value->kind == VK_LEPTON));

    count = the_value->u.lepton.labels.element_count;
    assert(count == the_value->u.lepton.values.element_count);
    assert(component_number < count);

    return the_value->u.lepton.labels.array[component_number];
  }

extern value *value_component_value(value *the_value, size_t component_number)
  {
    size_t count;

    assert(the_value != NULL);

    assert((the_value->kind == VK_SEMI_LABELED_VALUE_LIST) ||
           (the_value->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (the_value->kind == VK_LEPTON));

    count = the_value->u.lepton.labels.element_count;
    assert(count == the_value->u.lepton.values.element_count);
    assert(component_number < count);

    return the_value->u.lepton.values.array[component_number];
  }

extern value *value_get_field(const char *label, value *the_value)
  {
    assert(label != NULL);
    assert(the_value != NULL);

    assert((the_value->kind == VK_SEMI_LABELED_VALUE_LIST) ||
           (the_value->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (the_value->kind == VK_LEPTON));

    return (value *)(lookup_in_string_index(the_value->u.lepton.index, label));
  }

extern size_t value_get_field_index(const char *label, value *the_value)
  {
    size_t count;
    size_t index;

    assert(label != NULL);
    assert(the_value != NULL);

    assert((the_value->kind == VK_SEMI_LABELED_VALUE_LIST) ||
           (the_value->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (the_value->kind == VK_LEPTON));

    count = value_component_count(the_value);

    for (index = 0; index < count; ++index)
      {
        const char *test;

        test = value_component_label(the_value, index);
        if ((test != NULL) && (strcmp(label, test) == 0))
            return index;
      }

    return index;
  }

extern size_t map_value_item_count(value *map_value)
  {
    map_info *info;
    size_t result;

    assert(map_value != NULL);

    assert(map_value->kind == VK_MAP);

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);

    info = map_value->u.map.map_info;

    assert(info->value_key_keys.element_count ==
           info->value_key_targets.element_count);
    assert(info->type_key_keys.element_count ==
           info->type_key_targets.element_count);
    if (info->is_extension)
      {
#ifndef NDEBUG
        size_t base_count;

        base_count = map_value_item_count(info->extension.base);
        assert(info->extension.base != NULL);
        assert(info->extension.overlaps.element_count <= base_count);
        assert((((base_count - info->extension.overlaps.element_count) +
                 info->value_key_keys.element_count) +
                info->type_key_keys.element_count) == info->item_count);
#endif /* !NDEBUG */
      }
    else
      {
        assert((info->value_key_keys.element_count +
                info->type_key_keys.element_count) == info->item_count);
      }
    result = info->item_count;

    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    return result;
  }

extern boolean map_value_item_is_type(value *map_value, size_t item_num)
  {
    map_info *info;
    size_t remainder;

    assert(map_value != NULL);

    assert(map_value->kind == VK_MAP);
    assert(item_num < map_value_item_count(map_value));

    remainder = item_num;

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);

    info = map_value->u.map.map_info;

    if (info->is_extension)
      {
        size_t extension_count;

        extension_count =
                (map_value_item_count(info->extension.base) -
                 info->extension.overlaps.element_count);
        if (remainder < extension_count)
          {
            boolean result;

            result = map_value_item_is_type(info->extension.base,
                    map_item_num_for_extension_base(info, remainder));

            RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

            return result;
          }
        remainder -= extension_count;
      }

    if (remainder < info->type_key_keys.element_count)
      {
        RELEASE_SYSTEM_LOCK(map_value->u.map.lock);
        return TRUE;
      }
    remainder -= info->type_key_keys.element_count;

    assert(remainder < info->value_key_targets.element_count);

    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    return FALSE;
  }

extern type *map_value_item_key_type(value *map_value, size_t item_num)
  {
    size_t remainder;
    map_info *info;
    type *result;

    assert(map_value != NULL);

    assert(map_value->kind == VK_MAP);
    assert(item_num < map_value_item_count(map_value));

    remainder = item_num;

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);

    info = map_value->u.map.map_info;

    if (info->is_extension)
      {
        size_t extension_count;

        extension_count =
                (map_value_item_count(info->extension.base) -
                 info->extension.overlaps.element_count);
        if (remainder < extension_count)
          {
            type *result;

            result = map_value_item_key_type(info->extension.base,
                    map_item_num_for_extension_base(info, remainder));

            RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

            return result;
          }
        remainder -= extension_count;
      }

    assert(info->type_key_keys.element_count ==
           info->type_key_targets.element_count);
    assert(remainder < info->type_key_keys.element_count);
    result = info->type_key_keys.array[remainder];

    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    return result;
  }

extern value *map_value_item_key_value(value *map_value, size_t item_num)
  {
    size_t remainder;
    map_info *info;
    value *result;

    assert(map_value != NULL);

    assert(map_value->kind == VK_MAP);
    assert(item_num < map_value_item_count(map_value));

    remainder = item_num;

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);

    info = map_value->u.map.map_info;

    if (info->is_extension)
      {
        size_t extension_count;

        extension_count =
                (map_value_item_count(info->extension.base) -
                 info->extension.overlaps.element_count);
        if (remainder < extension_count)
          {
            value *result;

            result = map_value_item_key_value(info->extension.base,
                    map_item_num_for_extension_base(info, remainder));

            RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

            return result;
          }
        remainder -= extension_count;
      }

    assert(info->type_key_keys.element_count ==
           info->type_key_targets.element_count);
    assert(remainder >= info->type_key_keys.element_count);
    remainder -= info->type_key_keys.element_count;

    result = info->value_key_keys.array[remainder];

    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    return result;
  }

extern value *map_value_item_target(value *map_value, size_t item_num)
  {
    size_t remainder;
    map_info *info;
    value *result;

    assert(map_value != NULL);

    assert(map_value->kind == VK_MAP);
    assert(item_num < map_value_item_count(map_value));

    remainder = item_num;

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);

    info = map_value->u.map.map_info;

    if (info->is_extension)
      {
        size_t extension_count;

        extension_count =
                (map_value_item_count(info->extension.base) -
                 info->extension.overlaps.element_count);
        if (remainder < extension_count)
          {
            value *result;

            result = map_base_target(info,
                    map_item_num_for_extension_base(info, remainder));

            RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

            return result;
          }
        remainder -= extension_count;
      }

    assert(info->type_key_keys.element_count ==
           info->type_key_targets.element_count);
    if (remainder < info->type_key_keys.element_count)
      {
        value *result;

        result = info->type_key_targets.array[remainder];

        RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

        return result;
      }
    remainder -= info->type_key_keys.element_count;

    result = info->value_key_targets.array[remainder];

    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    return result;
  }

extern boolean map_value_item_is_definitely_touchable(value *map_value,
        size_t item_num, const source_location *location, jumper *the_jumper)
  {
    size_t remainder;
    map_info *info;
    boolean doubt;
    boolean is_finite;
    size_t other_num;

    assert(map_value != NULL);

    assert(map_value->kind == VK_MAP);
    assert(item_num < map_value_item_count(map_value));

    if (item_num + 1 == map_value_item_count(map_value))
        return TRUE;

    remainder = item_num;

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);
    info = map_value->u.map.map_info;
    map_info_add_reference(info);
    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    if (info->is_extension)
      {
        value *base_map;
        size_t extension_count;

        base_map = info->extension.base;
        extension_count =
                (map_value_item_count(base_map) -
                 info->extension.overlaps.element_count);
        if (remainder < extension_count)
          {
            size_t base_item_num;
            boolean base_touchable;

            base_item_num = map_item_num_for_extension_base(info, remainder);
            base_touchable = map_value_item_is_definitely_touchable(base_map,
                    base_item_num, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                map_info_remove_reference(info, the_jumper);
                return FALSE;
              }
            if (!base_touchable)
              {
                map_info_remove_reference(info, the_jumper);
                return FALSE;
              }

            if (map_value_item_is_type(base_map, base_item_num))
              {
                type *key;
                boolean doubt;
                boolean is_finite;
                size_t filter_count;
                size_t filter_num;

                key = map_value_item_key_type(base_map, base_item_num);

                is_finite = type_has_finite_element_count(key, &doubt);
                if (doubt || is_finite)
                  {
                    map_info_remove_reference(info, the_jumper);
                    return FALSE;
                  }

                filter_count = info->type_key_keys.element_count;
                for (filter_num = 0; filter_num < filter_count; ++filter_num)
                  {
                    boolean doubt;
                    boolean is_finite;

                    is_finite = type_has_finite_element_count(
                            info->type_key_keys.array[filter_num], &doubt);
                    if (doubt || !is_finite)
                      {
                        map_info_remove_reference(info, the_jumper);
                        return FALSE;
                      }
                  }

                map_info_remove_reference(info, the_jumper);
                return TRUE;
              }
            else
              {
                value *key;
                boolean found;
                boolean doubt;
                size_t position;
                size_t filter_count;
                size_t filter_num;

                key = map_value_item_key_value(base_map, base_item_num);

                position = map_info_local_index_for_key(info, key, &found,
                        &doubt, FALSE, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    map_info_remove_reference(info, the_jumper);
                    return FALSE;
                  }

                if (found || doubt)
                  {
                    map_info_remove_reference(info, the_jumper);
                    return FALSE;
                  }

                filter_count = info->type_key_keys.element_count;
                for (filter_num = 0; filter_num < filter_count; ++filter_num)
                  {
                    boolean doubt;
                    boolean is_in;

                    is_in = value_is_in_type(key,
                            info->type_key_keys.array[filter_num], &doubt,
                            NULL, location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        map_info_remove_reference(info, the_jumper);
                        return FALSE;
                      }
                    if (doubt || is_in)
                      {
                        map_info_remove_reference(info, the_jumper);
                        return FALSE;
                      }
                  }

                map_info_remove_reference(info, the_jumper);
                return TRUE;
              }
          }
        remainder -= extension_count;
      }

    assert(info->type_key_keys.element_count ==
           info->type_key_targets.element_count);
    if (remainder >= info->type_key_keys.element_count)
      {
        map_info_remove_reference(info, the_jumper);
        return TRUE;
      }

    is_finite = type_has_finite_element_count(
            info->type_key_keys.array[remainder], &doubt);
    if (doubt || is_finite)
      {
        map_info_remove_reference(info, the_jumper);
        return FALSE;
      }

    for (other_num = remainder + 1;
         other_num < info->type_key_keys.element_count; ++other_num)
      {
        boolean doubt;
        boolean is_finite;

        is_finite = type_has_finite_element_count(
                info->type_key_keys.array[other_num], &doubt);
        if (doubt || !is_finite)
          {
            map_info_remove_reference(info, the_jumper);
            return FALSE;
          }
      }

    map_info_remove_reference(info, the_jumper);
    return TRUE;
  }

extern boolean map_value_is_array(value *map_value, boolean *doubt,
        const source_location *location, jumper *the_jumper)
  {
    map_info *info;
    type *integer_type;
    type **types;
    size_t type_count;
    size_t type_num;
    boolean base_result;
    boolean base_doubt;

    assert(map_value != NULL);
    assert(doubt != NULL);
    assert(the_jumper != NULL);

    assert(map_value->kind == VK_MAP);
    assert(value_is_valid_except_map_targets(map_value)); /* VERIFIED */

    *doubt = FALSE;

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);
    info = map_value->u.map.map_info;
    map_info_add_reference(info);
    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    if (info->non_integer_key_count > 0)
      {
        map_info_remove_reference(info, the_jumper);
        return FALSE;
      }

    integer_type = get_integer_type();
    if (integer_type == NULL)
      {
        map_info_remove_reference(info, the_jumper);
        *doubt = TRUE;
        return FALSE;
      }

    types = info->type_key_keys.array;
    type_count = info->type_key_keys.element_count;

    for (type_num = 0; type_num < type_count; ++type_num)
      {
        boolean local_doubt;
        boolean local_fits;

        /*
         * All calls to map_value_is_array() other than the one at the end of
         * this function have it verified that all keys and key types are
         * valid.  So, if this was a call from anywhere but the end of this
         * function, type_is_valid(types[type_num]) will be TRUE.  But if it's
         * a call from the end of this function and this key type is not valid,
         * then the key type must have been entirely covered by keys or key
         * types farther back on the call stack because otherwise the top-level
         * call wouldn't have all key types valid.  Since it's entirely covered
         * by keys or key types farther back on the call stack, we can safely
         * ignore any key type that isn't valid here.
         */
        if (!(type_is_valid(types[type_num])))
            continue;

        assert(type_is_valid(types[type_num])); /* VERIFIED */
        assert(type_is_valid(integer_type)); /* VERIFIED */
        local_fits = type_is_subset(types[type_num], integer_type,
                                    &local_doubt, NULL, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            map_info_remove_reference(info, the_jumper);
            return FALSE;
          }

        if (local_doubt)
          {
            *doubt = TRUE;
          }
        else if (!local_fits)
          {
            map_info_remove_reference(info, the_jumper);
            *doubt = FALSE;
            return FALSE;
          }
      }

    if (!(info->is_extension))
      {
        map_info_remove_reference(info, the_jumper);
        return TRUE;
      }

    assert(info->extension.base != NULL);
    base_result = map_value_is_array(info->extension.base, &base_doubt,
                                     location, the_jumper);
    map_info_remove_reference(info, the_jumper);
    if (base_doubt)
        *doubt = TRUE;
    return base_result;
  }

extern void map_value_integer_key_bounds(value *map_value, o_integer *minimum,
        o_integer *maximum, boolean *min_doubt, boolean *max_doubt,
        const source_location *location, jumper *the_jumper)
  {
    size_t entry_count;
    o_integer result_min;
    o_integer result_max;
    boolean result_min_doubt;
    boolean result_max_doubt;
    size_t entry_num;

    assert(map_value != NULL);
    assert(minimum != NULL);
    assert(maximum != NULL);
    assert(min_doubt != NULL);
    assert(max_doubt != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(map_value) == VK_MAP);

    entry_count = map_value_item_count(map_value);

    if (entry_count == 0)
      {
        *minimum = oi_positive_infinity;
        oi_add_reference(*minimum);
        *maximum = oi_negative_infinity;
        oi_add_reference(*maximum);
        *min_doubt = FALSE;
        *max_doubt = FALSE;
        return;
      }

    result_min = oi_null;
    result_max = oi_null;
    result_min_doubt = FALSE;
    result_max_doubt = FALSE;

    for (entry_num = 0; entry_num < entry_count; ++entry_num)
      {
        if (map_value_item_is_type(map_value, entry_num))
          {
            type *key_type;
            o_integer local_min;
            o_integer local_max;
            boolean local_min_doubt;
            boolean local_max_doubt;

            key_type = map_value_item_key_type(map_value, entry_num);
            assert(key_type != NULL);

            find_type_integer_bounds(key_type, &local_min, &local_max,
                    &local_min_doubt, &local_max_doubt, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                if (!(oi_out_of_memory(result_max)))
                    oi_remove_reference(result_max);
                if (!(oi_out_of_memory(result_min)))
                    oi_remove_reference(result_min);
                return;
              }

            if (local_min_doubt)
              {
                if (!(oi_out_of_memory(result_min)))
                  {
                    oi_remove_reference(result_min);
                    result_min = oi_null;
                  }
                *minimum = oi_null;
                *min_doubt = TRUE;
                if (result_max_doubt)
                  {
                    assert(oi_out_of_memory(result_max));
                    assert(oi_out_of_memory(local_min));
                    if (!(oi_out_of_memory(local_max)))
                        oi_remove_reference(local_max);
                    return;
                  }
                result_min_doubt = TRUE;
              }
            if (local_max_doubt)
              {
                if (!(oi_out_of_memory(result_max)))
                  {
                    oi_remove_reference(result_max);
                    result_max = oi_null;
                  }
                *maximum = oi_null;
                *max_doubt = TRUE;
                if (result_min_doubt)
                  {
                    assert(oi_out_of_memory(result_min));
                    assert(oi_out_of_memory(local_max));
                    if (!(oi_out_of_memory(local_min)))
                        oi_remove_reference(local_min);
                    return;
                  }
                result_max_doubt = TRUE;
              }

            if (oi_out_of_memory(local_min))
              {
                assert(oi_out_of_memory(local_max));
              }
            else
              {
                assert(!(oi_out_of_memory(local_max)));
                if (oi_out_of_memory(result_max))
                  {
                    assert(oi_out_of_memory(result_min));
                    result_max = local_max;
                    result_min = local_min;
                  }
                else
                  {
                    assert(!(oi_out_of_memory(result_min)));
                    if (oi_less_than(result_max, local_max))
                      {
                        oi_remove_reference(result_max);
                        result_max = local_max;
                      }
                    else
                      {
                        oi_remove_reference(local_max);
                      }
                    if (oi_less_than(local_min, result_min))
                      {
                        oi_remove_reference(result_min);
                        result_min = local_min;
                      }
                    else
                      {
                        oi_remove_reference(local_min);
                      }
                  }
              }
          }
        else
          {
            value *key_value;
            o_integer key_oi;

            key_value = map_value_item_key_value(map_value, entry_num);
            assert(key_value != NULL);

            assert(get_value_kind(key_value) == VK_INTEGER);
            key_oi = integer_value_data(key_value);
            assert(!(oi_out_of_memory(key_oi)));

            if (oi_out_of_memory(result_max))
              {
                assert(oi_out_of_memory(result_min));
                oi_add_reference(key_oi);
                result_max = key_oi;
                oi_add_reference(key_oi);
                result_min = key_oi;
              }
            else
              {
                assert(!(oi_out_of_memory(result_min)));
                if (oi_less_than(result_max, key_oi))
                  {
                    oi_add_reference(key_oi);
                    oi_remove_reference(result_max);
                    result_max = key_oi;
                  }
                if (oi_less_than(key_oi, result_min))
                  {
                    oi_add_reference(key_oi);
                    oi_remove_reference(result_min);
                    result_min = key_oi;
                  }
              }
          }
      }

    *minimum = result_min;
    *maximum = result_max;
    *min_doubt = result_min_doubt;
    *max_doubt = result_max_doubt;
    return;
  }

extern value *map_value_lookup(value *map_value, value *key, boolean *doubt,
        const source_location *location, jumper *the_jumper)
  {
    return map_value_lookup_with_index(map_value, key, doubt, NULL, location,
                                       the_jumper);
  }

extern quark *value_quark(value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_QUARK);
    assert(the_value->u.quark != NULL);
    return the_value->u.quark;
  }

extern lepton_key_instance *value_lepton_key(value *the_value)
  {
    assert(the_value != NULL);

    assert((the_value->kind == VK_LEPTON) ||
           (the_value->kind == VK_LEPTON_KEY));
    assert(the_value->u.lepton.lepton_key != NULL);
    return the_value->u.lepton.lepton_key;
  }

extern slot_location *slot_location_value_data(value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_SLOT_LOCATION);
    assert(the_value->u.slot_location != NULL);
    return the_value->u.slot_location;
  }

extern jump_target *jump_target_value_data(value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_JUMP_TARGET);
    assert(the_value->u.jump_target != NULL);
    return the_value->u.jump_target;
  }

extern routine_instance *routine_value_data(value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_ROUTINE);
    assert(the_value->u.routine != NULL);
    return the_value->u.routine;
  }

extern routine_instance_chain *routine_chain_value_data(value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_ROUTINE_CHAIN);
    assert(the_value->u.routine_instance_chain != NULL);
    return the_value->u.routine_instance_chain;
  }

extern type *type_value_data(value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_TYPE);
    assert(the_value->u.type != NULL);
    return the_value->u.type;
  }

extern object *object_value_data(value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_OBJECT);
    assert(the_value->u.object != NULL);
    return the_value->u.object;
  }

extern tagalong_key *tagalong_key_value_data(value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_TAGALONG_KEY);
    assert(the_value->u.tagalong_key != NULL);
    return the_value->u.tagalong_key;
  }

extern lock_instance *lock_value_data(value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_LOCK);
    assert(the_value->u.lock_instance != NULL);
    return the_value->u.lock_instance;
  }

extern verdict add_field(value *base_value, const char *label,
                         value *field_value)
  {
    char *new_label;
    verdict the_verdict;
    value *map_equivalent;

    assert(base_value != NULL);
    assert(field_value != NULL);

    assert((base_value->kind == VK_SEMI_LABELED_VALUE_LIST) ||
           (base_value->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (base_value->kind == VK_LEPTON));

    assert(base_value->reference_count == 1);

    GRAB_SYSTEM_LOCK(base_value->u.lepton.lock);

    assert((label == NULL) ||
           (lookup_in_string_index(base_value->u.lepton.index, label) ==
            NULL));

    if (label == NULL)
      {
        new_label = NULL;
      }
    else
      {
        new_label = MALLOC_ARRAY(char, strlen(label) + 1);
        if (new_label == NULL)
          {
            RELEASE_SYSTEM_LOCK(base_value->u.lepton.lock);
            return MISSION_FAILED;
          }
        strcpy(new_label, label);
      }

    the_verdict = mstring_aa_append(&(base_value->u.lepton.labels), new_label);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        RELEASE_SYSTEM_LOCK(base_value->u.lepton.lock);
        if (new_label != NULL)
            free(new_label);
        return the_verdict;
      }

    the_verdict = value_aa_append(&(base_value->u.lepton.values), field_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(base_value->u.lepton.labels.element_count > 0);
        --(base_value->u.lepton.labels.element_count);
        RELEASE_SYSTEM_LOCK(base_value->u.lepton.lock);
        if (new_label != NULL)
            free(new_label);
        return the_verdict;
      }

    if (new_label != NULL)
      {
        verdict the_verdict;

        the_verdict = enter_into_string_index(base_value->u.lepton.index,
                                              new_label, field_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            assert(base_value->u.lepton.values.element_count > 0);
            --(base_value->u.lepton.values.element_count);
            assert(base_value->u.lepton.labels.element_count > 0);
            --(base_value->u.lepton.labels.element_count);
            RELEASE_SYSTEM_LOCK(base_value->u.lepton.lock);
            if (new_label != NULL)
                free(new_label);
            return the_verdict;
          }
      }

    base_value->validator = validator_add_validator(base_value->validator,
            value_validator(field_value));
    if (base_value->validator == NULL)
      {
        enter_into_string_index(base_value->u.lepton.index, new_label, NULL);
        assert(base_value->u.lepton.values.element_count > 0);
        --(base_value->u.lepton.values.element_count);
        assert(base_value->u.lepton.labels.element_count > 0);
        --(base_value->u.lepton.labels.element_count);
        RELEASE_SYSTEM_LOCK(base_value->u.lepton.lock);
        if (new_label != NULL)
            free(new_label);
        return MISSION_FAILED;
      }

    add_intervalue_reference(base_value, field_value);

    base_value->u.lepton.labeled_element_count = 0;
    if (base_value->u.lepton.ordered_labeled_elements != NULL)
      {
        free(base_value->u.lepton.ordered_labeled_elements);
        base_value->u.lepton.ordered_labeled_elements = NULL;
      }
    base_value->u.lepton.unlabeled_element_count = 0;
    if (base_value->u.lepton.ordered_unlabeled_elements != NULL)
      {
        free(base_value->u.lepton.ordered_unlabeled_elements);
        base_value->u.lepton.ordered_unlabeled_elements = NULL;
      }

    map_equivalent = base_value->u.lepton.map_equivalent;
    if (map_equivalent != NULL)
        base_value->u.lepton.map_equivalent = NULL;

    if (value_is_slippery(field_value))
        ++(base_value->u.lepton.slippery_count);

    RELEASE_SYSTEM_LOCK(base_value->u.lepton.lock);

    if (map_equivalent != NULL)
      {
        value_remove_reference_with_reference_cluster(map_equivalent, NULL,
                base_value->reference_cluster);
      }

    return MISSION_ACCOMPLISHED;
  }

extern void set_field(value *base_value, const char *label, value *field_value,
                      jumper *the_jumper)
  {
    value *old_value;
    size_t count;
    char **string_array;
    value **value_array;
    size_t index;

    assert(label != NULL);
    assert(base_value != NULL);
    assert(field_value != NULL);

    assert((base_value->kind == VK_SEMI_LABELED_VALUE_LIST) ||
           (base_value->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (base_value->kind == VK_LEPTON));

    assert(base_value->reference_count == 1);

    old_value = (value *)(lookup_in_string_index(base_value->u.lepton.index,
                                                 label));
    if (old_value == NULL)
      {
        verdict the_verdict;

        the_verdict = add_field(base_value, label, field_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
            jumper_do_abort(the_jumper);
        return;
      }

    count = base_value->u.lepton.labels.element_count;
    assert(count == base_value->u.lepton.values.element_count);

    string_array = base_value->u.lepton.labels.array;
    value_array = base_value->u.lepton.values.array;
    assert(string_array != NULL);
    assert(value_array != NULL);

    for (index = 0; index < count; ++index)
      {
        if ((string_array[index] != NULL) &&
            (strcmp(label, string_array[index]) == 0))
          {
            verdict the_verdict;
            value *map_equivalent;

            assert(value_array[index] == old_value);

            base_value->validator = validator_remove_validator(
                    base_value->validator, value_validator(old_value));
            if (base_value->validator == NULL)
              {
                jumper_do_abort(the_jumper);
                return;
              }

            base_value->validator = validator_add_validator(
                    base_value->validator, value_validator(field_value));
            if (base_value->validator == NULL)
              {
                jumper_do_abort(the_jumper);
                return;
              }

            the_verdict = enter_into_string_index(base_value->u.lepton.index,
                                                  label, field_value);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                return;
              }

            if (value_is_slippery(old_value))
              {
                assert(base_value->u.lepton.slippery_count > 0);
                --(base_value->u.lepton.slippery_count);
              }
            add_intervalue_reference(base_value, field_value);
            remove_intervalue_reference(base_value, old_value, the_jumper);
            value_array[index] = field_value;

            GRAB_SYSTEM_LOCK(base_value->u.lepton.lock);

            map_equivalent = base_value->u.lepton.map_equivalent;
            if (map_equivalent != NULL)
                base_value->u.lepton.map_equivalent = NULL;

            RELEASE_SYSTEM_LOCK(base_value->u.lepton.lock);

            if (map_equivalent != NULL)
              {
                value_remove_reference_with_reference_cluster(map_equivalent,
                        the_jumper, base_value->reference_cluster);
              }

            if (value_is_slippery(field_value))
                ++(base_value->u.lepton.slippery_count);

            return;
          }
      }

    assert(FALSE);
  }

extern value *map_value_set(value *map_value, value *key, value *target,
        const source_location *location, jumper *the_jumper)
  {
    size_t reference_count;
    size_t no_cluster_reference_count;
    map_info *source_info;
    verdict the_verdict;
    value *result;
    map_info *result_info;

    assert(map_value != NULL);
    assert(key != NULL);
    assert(target != NULL);
    assert(the_jumper != NULL);

    assert(map_value->kind == VK_MAP);

    GRAB_SYSTEM_LOCK(map_value->lock);
    reference_count = map_value->reference_count;
    no_cluster_reference_count = map_value->no_cluster_reference_count;
    RELEASE_SYSTEM_LOCK(map_value->lock);

    if (reference_count - no_cluster_reference_count > 1)
      {
        value *copy;

        copy = copy_value(map_value);
        if (copy == NULL)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        value_remove_reference(map_value, the_jumper);
        assert(jumper_flowing_forward(the_jumper));

        return map_value_set(copy, key, target, location, the_jumper);
      }

    assert(reference_count - no_cluster_reference_count == 1);

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);
    source_info = map_value->u.map.map_info;
    map_info_add_reference(source_info);
    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    assert(validator_is_valid(source_info->all_keys_validator)); /* VERIFIED */
    assert(value_is_valid(key)); /* VERIFIED */

    the_verdict =
            map_value_insert(map_value, key, target, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        map_info_remove_reference(source_info, the_jumper);
        value_remove_reference(map_value, the_jumper);
        return NULL;
      }
    assert(validator_is_valid(source_info->all_keys_validator)); /* VERIFIED */

    if (the_verdict == MISSION_ACCOMPLISHED)
      {
        assert(validator_is_valid(source_info->all_keys_validator));
                /* VERIFIED */
        map_info_remove_reference(source_info, the_jumper);
        return map_value;
      }

    result = create_empty_value(VK_MAP);
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        map_info_remove_reference(source_info, the_jumper);
        value_remove_reference(map_value, the_jumper);
        return NULL;
      }

    assert(result->reference_count == 1);
    assert(result->reference_cluster == NULL);
    assert(result->cluster_use_count == 0);

    the_verdict = set_up_map_extension(map_value, result);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        map_info_remove_reference(source_info, the_jumper);
        value_remove_reference(map_value, the_jumper);
        return NULL;
      }

    result_info = result->u.map.map_info;
    assert(result_info != NULL);

    result_info->extension.is_uncompressable = TRUE;

    map_info_remove_reference(source_info, the_jumper);
    value_remove_reference(map_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        update_value_cluster_for_map_info(result, NULL, the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    the_verdict = map_info_append_value_key_item(result_info, key, target);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        update_value_cluster_for_map_info(result, NULL, the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    result->validator =
            validator_add_validator(result->validator, value_validator(key));
    if (result->validator == NULL)
      {
        jumper_do_abort(the_jumper);
        update_value_cluster_for_map_info(result, NULL, the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    result->validator = validator_add_validator(result->validator,
                                                value_validator(target));
    if (result->validator == NULL)
      {
        jumper_do_abort(the_jumper);
        update_value_cluster_for_map_info(result, NULL, the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    result_info->all_keys_validator = validator_add_validator(
            result_info->all_keys_validator, value_validator(key));
    if (result_info->all_keys_validator == NULL)
      {
        jumper_do_abort(the_jumper);
        update_value_cluster_for_map_info(result, NULL, the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }
    assert(validator_is_valid(result_info->all_keys_validator)); /* VERIFIED */

    ++(result_info->item_count);

    update_value_cluster_for_map_info(result, NULL, the_jumper);

    assert(validator_is_valid(result_info->all_keys_validator)); /* VERIFIED */
    return result;
  }

extern value *map_value_set_filter(value *map_value, type *filter,
        value *target, const source_location *location, jumper *the_jumper)
  {
    size_t reference_count;
    size_t no_cluster_reference_count;
    map_info *source_info;
    size_t old_key_count;
    value **value_key_keys;
    value **value_key_targets;
    size_t slippery_key_count;
    size_t slippery_count;
    boolean blocked;
    size_t new_key_num;
    size_t old_key_num;
    size_t old_filter_count;
    type **type_key_keys;
    value **type_key_targets;
    size_t new_filter_num;
    size_t old_filter_num;
    value *result;
    map_info *result_info;
    verdict the_verdict;

    assert(map_value != NULL);
    assert(filter != NULL);
    assert(target != NULL);
    assert(the_jumper != NULL);

    assert(map_value->kind == VK_MAP);

    GRAB_SYSTEM_LOCK(map_value->lock);
    reference_count = map_value->reference_count;
    no_cluster_reference_count = map_value->no_cluster_reference_count;
    RELEASE_SYSTEM_LOCK(map_value->lock);

    if (reference_count - no_cluster_reference_count > 1)
      {
        value *copy;

        copy = copy_value(map_value);
        if (copy == NULL)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        value_remove_reference(map_value, the_jumper);
        assert(jumper_flowing_forward(the_jumper));

        return map_value_set_filter(copy, filter, target, location,
                                    the_jumper);
      }

    assert(reference_count - no_cluster_reference_count == 1);

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);
    source_info = map_value->u.map.map_info;
    map_info_add_reference(source_info);
    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    assert(validator_is_valid(source_info->all_keys_validator)); /* VERIFIED */
    assert(type_is_valid(filter)); /* VERIFIED */

    if (get_type_kind(filter) == TK_NOTHING)
      {
        assert(validator_is_valid(source_info->all_keys_validator));
                /* VERIFIED */
        map_info_remove_reference(source_info, the_jumper);
        return map_value;
      }

    old_key_count = source_info->value_key_keys.element_count;
    assert(old_key_count == source_info->value_key_targets.element_count);
    value_key_keys = source_info->value_key_keys.array;
    value_key_targets = source_info->value_key_targets.array;

    slippery_key_count = 0;
    slippery_count = 0;

    blocked = FALSE;
    new_key_num = 0;

    for (old_key_num = 0; old_key_num < old_key_count; ++old_key_num)
      {
        value *this_key;
        value *this_target;
        boolean is_in;
        boolean doubt;

        this_key = value_key_keys[old_key_num];
        this_target = value_key_targets[old_key_num];
        assert(value_is_valid(this_key)); /* VERIFIED */
        assert(type_is_valid(filter)); /* VERIFIED */
        is_in = value_is_in_type(this_key, filter, &doubt, NULL, location,
                                 the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
          error:
            if (new_key_num != old_key_num)
              {
                for (; old_key_num < old_key_count; ++old_key_num)
                  {
                    value_key_keys[new_key_num] = value_key_keys[old_key_num];
                    value_key_targets[new_key_num] =
                            value_key_targets[old_key_num];
                    ++new_key_num;
                  }
              }
            map_info_remove_reference(source_info, the_jumper);
            update_value_cluster_for_map_info(map_value, NULL, the_jumper);
            value_remove_reference(map_value, the_jumper);
            return NULL;
          }

        if (doubt)
            blocked = TRUE;

        if (is_in && !doubt)
          {
            value_kind this_key_kind;

            assert(source_info->item_count > 0);
            --(source_info->item_count);

            this_key_kind = get_value_kind(this_key);

            if (this_key_kind == VK_STRING)
              {
                remove_from_string_index(source_info->string_key_index,
                                         this_key->u.string_data);
              }

            if (this_key_kind == VK_INTEGER)
              {
                char *string;
                boolean deallocation_needed;

                string =
                        key_string_for_integer(this_key, &deallocation_needed);
                if (string == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    goto error;
                  }

                remove_from_string_index(source_info->integer_key_index,
                                         string);

                if (deallocation_needed)
                    free(string);
              }

            if ((this_key_kind != VK_INTEGER) ||
                (oi_kind(this_key->u.integer) != IIK_FINITE))
              {
                assert(source_info->non_integer_key_count > 0);
                --(source_info->non_integer_key_count);
              }

            map_value->validator = validator_remove_validator(
                    map_value->validator, value_validator(this_key));
            if (map_value->validator == NULL)
              {
                jumper_do_abort(the_jumper);
                goto error;
              }

            source_info->all_keys_validator = validator_remove_validator(
                    source_info->all_keys_validator,
                    value_validator(this_key));
            if (source_info->all_keys_validator == NULL)
              {
                jumper_do_abort(the_jumper);
                goto error;
              }

            map_value->validator = validator_remove_validator(
                    map_value->validator, value_validator(this_target));
            if (map_value->validator == NULL)
              {
                jumper_do_abort(the_jumper);
                goto error;
              }

            remove_map_info_to_value_reference(source_info, this_key,
                                               the_jumper);
            remove_map_info_to_value_reference(source_info, this_target,
                                               the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                ++old_key_num;
                goto error;
              }
          }
        else
          {
            if (value_is_slippery(this_key))
              {
                ++slippery_key_count;
                ++slippery_count;
              }
            if (value_is_slippery(value_key_targets[old_key_num]))
                ++slippery_count;
            if (new_key_num != old_key_num)
              {
                value_key_keys[new_key_num] = value_key_keys[old_key_num];
                value_key_targets[new_key_num] =
                        value_key_targets[old_key_num];
              }
            ++new_key_num;
          }
      }

    source_info->value_key_keys.element_count = new_key_num;
    source_info->value_key_targets.element_count = new_key_num;

    if (source_info->is_extension)
      {
        value *base_map;
        size_t_aa *overlaps;
        size_t original_overlap_count;
        size_t overlap_num;
        size_t base_item_count;
        size_t base_item_num;

        base_map = source_info->extension.base;

        overlaps = &(source_info->extension.overlaps);
        original_overlap_count = overlaps->element_count;

        GRAB_SYSTEM_LOCK(source_info->extension.overlap_sorting_lock);

        if (!(source_info->extension.overlaps_sorted))
          {
            if (original_overlap_count > 0)
              {
                qsort(overlaps->array, original_overlap_count, sizeof(size_t),
                      &compare_size_t);
              }
            source_info->extension.overlaps_sorted = TRUE;
          }

        RELEASE_SYSTEM_LOCK(source_info->extension.overlap_sorting_lock);

        overlap_num = 0;

        base_item_count = map_value_item_count(base_map);

        for (base_item_num = 0; base_item_num < base_item_count;
             ++base_item_num)
          {
            boolean doubt;
            boolean new_overlap;

            if ((overlap_num < original_overlap_count) &&
                (overlaps->array[overlap_num] == base_item_num))
              {
                ++overlap_num;
                continue;
              }

            if (map_value_item_is_type(base_map, base_item_num))
              {
                type *item_key;

                item_key = map_value_item_key_type(base_map, base_item_num);
                assert(type_is_valid(item_key)); /* VERIFIED */
                assert(type_is_valid(filter)); /* VERIFIED */
                new_overlap = type_is_subset(item_key, filter, &doubt, NULL,
                                             location, the_jumper);
              }
            else
              {
                value *item_key;

                item_key = map_value_item_key_value(base_map, base_item_num);
                assert(value_is_valid(item_key)); /* VERIFIED */
                assert(type_is_valid(filter)); /* VERIFIED */
                new_overlap = value_is_in_type(item_key, filter, &doubt, NULL,
                                               location, the_jumper);
              }
            if (!(jumper_flowing_forward(the_jumper)))
              {
                map_info_remove_reference(source_info, the_jumper);
                update_value_cluster_for_map_info(map_value, NULL, the_jumper);
                value_remove_reference(map_value, the_jumper);
                return NULL;
              }

            if (doubt)
                source_info->extension.is_uncompressable = TRUE;

            if (new_overlap && !doubt)
              {
                verdict the_verdict;

                the_verdict = size_t_aa_append(overlaps, base_item_num);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    map_info_remove_reference(source_info, the_jumper);
                    update_value_cluster_for_map_info(map_value, NULL,
                                                      the_jumper);
                    value_remove_reference(map_value, the_jumper);
                    return NULL;
                  }

                --(source_info->item_count);

                the_verdict = enter_into_map_overlap_index(source_info,
                                                           base_item_num);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    map_info_remove_reference(source_info, the_jumper);
                    update_value_cluster_for_map_info(map_value, NULL,
                                                      the_jumper);
                    value_remove_reference(map_value, the_jumper);
                    return NULL;
                  }
              }
            else
              {
                if (map_value_item_is_type(base_map, base_item_num))
                  {
                    if (type_is_slippery(map_value_item_key_type(base_map,
                                                 base_item_num)))
                      {
                        ++slippery_count;
                      }
                  }
                else
                  {
                    if (value_is_slippery(map_value_item_key_value(base_map,
                                                  base_item_num)))
                      {
                        ++slippery_key_count;
                        ++slippery_count;
                      }
                  }

                if (value_is_slippery(map_base_target(source_info,
                                                      base_item_num)))
                  {
                    ++slippery_count;
                  }
              }
          }

        assert(overlap_num == original_overlap_count);
        if ((overlaps->element_count > original_overlap_count) &&
            (overlaps->element_count > 1))
          {
            source_info->extension.overlaps_sorted = FALSE;
          }
      }

    old_filter_count = source_info->type_key_keys.element_count;
    assert(old_filter_count == source_info->type_key_targets.element_count);
    type_key_keys = source_info->type_key_keys.array;
    type_key_targets = source_info->type_key_targets.array;

    new_filter_num = 0;

    for (old_filter_num = 0; old_filter_num < old_filter_count;
         ++old_filter_num)
      {
        type *old_key_type;
        value *old_target;
        boolean doubt;
        boolean is_subset;

        old_key_type = type_key_keys[old_filter_num];
        old_target = type_key_targets[old_filter_num];

        check_type_validity(filter, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            goto filter_error;

        assert(type_is_valid(old_key_type)); /* VERIFIED */
        assert(type_is_valid(filter)); /* VERIFIED */
        is_subset = type_is_subset(old_key_type, filter, &doubt, NULL,
                                   location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
          filter_error:
            if (new_filter_num != old_filter_num)
              {
                for (; old_filter_num < old_filter_count; ++old_filter_num)
                  {
                    type_key_keys[new_filter_num] =
                            type_key_keys[old_filter_num];
                    type_key_targets[new_filter_num] =
                            type_key_targets[old_filter_num];
                    ++new_filter_num;
                  }
              }
            source_info->type_key_keys.element_count = new_filter_num;
            source_info->type_key_targets.element_count = new_filter_num;
            map_info_remove_reference(source_info, the_jumper);
            update_value_cluster_for_map_info(map_value, NULL, the_jumper);
            value_remove_reference(map_value, the_jumper);
            return NULL;
          }

        if (is_subset && !doubt)
          {
            assert(source_info->item_count > 0);
            --(source_info->item_count);

            map_value->validator = validator_remove_validator(
                    map_value->validator, type_validator(old_key_type));
            if (map_value->validator == NULL)
              {
                jumper_do_abort(the_jumper);
                goto filter_error;
              }

            source_info->all_keys_validator = validator_remove_validator(
                    source_info->all_keys_validator,
                    type_validator(old_key_type));
            if (source_info->all_keys_validator == NULL)
              {
                jumper_do_abort(the_jumper);
                goto filter_error;
              }

            map_value->validator = validator_remove_validator(
                    map_value->validator, value_validator(old_target));
            if (map_value->validator == NULL)
              {
                jumper_do_abort(the_jumper);
                goto filter_error;
              }

            remove_map_info_to_type_reference(source_info, old_key_type,
                                              the_jumper);
            remove_map_info_to_value_reference(source_info, old_target,
                                               the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                ++old_filter_num;
                goto filter_error;
              }
          }
        else
          {
            if (type_is_slippery(old_key_type))
              {
                ++slippery_key_count;
                ++slippery_count;
              }
            if (value_is_slippery(old_target))
                ++slippery_count;
            if (new_filter_num != old_filter_num)
              {
                type_key_keys[new_filter_num] = old_key_type;
                type_key_targets[new_filter_num] = old_target;
              }
            ++new_filter_num;
          }
      }

    source_info->type_key_keys.element_count = new_filter_num;
    source_info->type_key_targets.element_count = new_filter_num;

    source_info->slippery_key_count = slippery_key_count;
    source_info->slippery_count = slippery_count;

    if (blocked)
      {
        verdict the_verdict;

        result = create_empty_value(VK_MAP);
        if (result == NULL)
          {
            jumper_do_abort(the_jumper);
            map_info_remove_reference(source_info, the_jumper);
            update_value_cluster_for_map_info(map_value, NULL, the_jumper);
            value_remove_reference(map_value, the_jumper);
            return NULL;
          }

        update_value_cluster_for_map_info(map_value, NULL, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            map_info_remove_reference(source_info, the_jumper);
            value_remove_reference(map_value, the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        check_type_validity(filter, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            map_info_remove_reference(source_info, the_jumper);
            value_remove_reference(map_value, the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        assert(result->reference_count == 1);
        assert(result->reference_cluster == NULL);
        assert(result->cluster_use_count == 0);

        the_verdict = set_up_map_extension(map_value, result);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            map_info_remove_reference(source_info, the_jumper);
            value_remove_reference(map_value, the_jumper);
            return NULL;
          }

        result_info = result->u.map.map_info;

        assert(result_info->is_extension);
        result_info->extension.is_uncompressable = TRUE;

        map_info_add_reference(result_info);
        map_info_remove_reference(source_info, the_jumper);
        value_remove_reference(map_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            map_info_remove_reference(result_info, the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }
      }
    else
      {
        result = map_value;
        result_info = source_info;
      }

#ifndef NDEBUG
    GRAB_SYSTEM_LOCK(result->lock);
    assert(result->reference_count - result->no_cluster_reference_count == 1);
    RELEASE_SYSTEM_LOCK(result->lock);
#endif /* !NDEBUG */

    the_verdict = map_info_append_type_key_item(result_info, filter, target);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        map_info_remove_reference(result_info, the_jumper);
        update_value_cluster_for_map_info(result, NULL, the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    result->validator =
            validator_add_validator(result->validator, type_validator(filter));
    if (result->validator == NULL)
      {
        jumper_do_abort(the_jumper);
        map_info_remove_reference(result_info, the_jumper);
        update_value_cluster_for_map_info(result, NULL, the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    result->validator = validator_add_validator(result->validator,
                                                value_validator(target));
    if (result->validator == NULL)
      {
        jumper_do_abort(the_jumper);
        map_info_remove_reference(result_info, the_jumper);
        update_value_cluster_for_map_info(result, NULL, the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    result_info->all_keys_validator = validator_add_validator(
            result_info->all_keys_validator, type_validator(filter));
    if (result_info->all_keys_validator == NULL)
      {
        jumper_do_abort(the_jumper);
        map_info_remove_reference(result_info, the_jumper);
        update_value_cluster_for_map_info(result, NULL, the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    ++(result_info->item_count);

    update_value_cluster_for_map_info(result, NULL, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        map_info_remove_reference(result_info, the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    check_type_validity(filter, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        map_info_remove_reference(result_info, the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    map_info_remove_reference(result_info, the_jumper);
    return result;
  }

extern void convert_semi_labeled_value_list_value_to_multi_set(
        value *the_value)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_SEMI_LABELED_VALUE_LIST);
    the_value->kind = VK_SEMI_LABELED_MULTI_SET;
  }

extern boolean value_has_only_one_reference(value *the_value)
  {
    boolean result;

    assert(the_value != NULL);

    GRAB_SYSTEM_LOCK(the_value->lock);
    assert(the_value->reference_count > 0);
    result = (the_value->reference_count == 1);
    RELEASE_SYSTEM_LOCK(the_value->lock);
    return result;
  }

extern value *copy_value(value *the_value)
  {
    value *result;
    value_tagalong_handle **tagalong_tail;
    value_tagalong_handle *previous;
    value_tagalong_handle *follow;

    assert(the_value != NULL);

    result = create_empty_value(the_value->kind);
    if (result == NULL)
        return NULL;

    assert(result->reference_cluster == NULL);
    assert(result->cluster_use_count == 0);

    if (the_value->reference_cluster != NULL)
        result->reference_cluster = the_value->reference_cluster;

    switch (the_value->kind)
      {
        case VK_TRUE:
          {
            assert(result->cluster_use_count == 0);
            break;
          }
        case VK_FALSE:
          {
            assert(result->cluster_use_count == 0);
            break;
          }
        case VK_INTEGER:
          {
            assert(result->cluster_use_count == 0);
            assert(!(oi_out_of_memory(the_value->u.integer)));
            oi_add_reference(the_value->u.integer);
            result->u.integer = the_value->u.integer;

            break;
          }
        case VK_RATIONAL:
          {
            assert(result->cluster_use_count == 0);
            assert(the_value->u.rational != NULL);
            rational_add_reference(the_value->u.rational);
            result->u.rational = the_value->u.rational;

            break;
          }
        case VK_STRING:
          {
            char *copy;

            assert(result->cluster_use_count == 0);

            assert(the_value->u.string_data != NULL);
            copy = MALLOC_ARRAY(char, strlen(the_value->u.string_data) + 1);
            if (copy == NULL)
              {
                delete_value_common(result);
                return NULL;
              }
            strcpy(copy, the_value->u.string_data);
            result->u.string_data = copy;

            break;
          }
        case VK_CHARACTER:
          {
            assert(result->cluster_use_count == 0);
            assert(strlen(&(the_value->u.character_data[0])) <= 4);
            strcpy(&(result->u.character_data[0]),
                   &(the_value->u.character_data[0]));

            break;
          }
        case VK_REGULAR_EXPRESSION:
          {
            assert(result->cluster_use_count == 0);
            assert(the_value->u.regular_expression != NULL);
            regular_expression_add_reference(the_value->u.regular_expression);
            result->u.regular_expression = the_value->u.regular_expression;
            break;
          }
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            verdict the_verdict;

            assert(result->cluster_use_count == 0);

            the_verdict = copy_lepton_components(result, the_value);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_value_common(result);
                return NULL;
              }

            result->u.lepton.lepton_key = NULL;
            result->u.lepton.key_validator = NULL;

            break;
          }
        case VK_SEMI_LABELED_MULTI_SET:
          {
            verdict the_verdict;

            assert(result->cluster_use_count == 0);

            the_verdict = copy_lepton_components(result, the_value);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_value_common(result);
                return NULL;
              }

            result->u.lepton.lepton_key = NULL;
            result->u.lepton.key_validator = NULL;

            break;
          }
        case VK_MAP:
          {
            verdict the_verdict;

            assert(result->cluster_use_count == 0);

            the_verdict = make_map_copy(the_value, result);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return NULL;

            break;
          }
        case VK_QUARK:
          {
            if ((result->reference_cluster != NULL) &&
                (result->reference_cluster ==
                 quark_reference_cluster(the_value->u.quark)))
              {
                assert(result->cluster_use_count == 0);
                result->cluster_use_count = 1;
                reference_cluster_add_reference(result->reference_cluster);
              }
            quark_add_reference_with_cluster(the_value->u.quark,
                                             result->reference_cluster);
            result->u.quark = the_value->u.quark;

            break;
          }
        case VK_LEPTON:
          {
            verdict the_verdict;
            lepton_key_instance *key;

            assert(result->cluster_use_count == 0);

            the_verdict = copy_lepton_components(result, the_value);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_value_common(result);
                return NULL;
              }

            key = the_value->u.lepton.lepton_key;
            assert(key != NULL);

            if ((result->reference_cluster != NULL) &&
                (result->reference_cluster ==
                 lepton_key_instance_reference_cluster(key)))
              {
                ++(result->cluster_use_count);
                if (result->cluster_use_count == 1)
                  {
                    reference_cluster_add_reference(result->reference_cluster);
                  }
              }

            lepton_key_instance_add_reference_with_cluster(key,
                    result->reference_cluster);
            result->u.lepton.lepton_key = key;

            if (the_value->u.lepton.key_validator != NULL)
                validator_add_reference(the_value->u.lepton.key_validator);
            result->u.lepton.key_validator = the_value->u.lepton.key_validator;

            break;
          }
        case VK_LEPTON_KEY:
          {
            lepton_key_instance *key;

            assert(result->cluster_use_count == 0);

            key = the_value->u.lepton.lepton_key;
            assert(key != NULL);

            if ((result->reference_cluster != NULL) &&
                (result->reference_cluster ==
                 lepton_key_instance_reference_cluster(key)))
              {
                assert(result->cluster_use_count == 0);
                result->cluster_use_count = 1;
                reference_cluster_add_reference(result->reference_cluster);
              }

            lepton_key_instance_add_reference_with_cluster(key,
                    result->reference_cluster);
            result->u.lepton.lepton_key = key;
            result->u.lepton.key_validator = NULL;

            break;
          }
        case VK_SLOT_LOCATION:
          {
            assert(result->cluster_use_count == 0);

            if ((result->reference_cluster != NULL) &&
                (result->reference_cluster ==
                 slot_location_reference_cluster(the_value->u.slot_location)))
              {
                assert(result->cluster_use_count == 0);
                result->cluster_use_count = 1;
                reference_cluster_add_reference(result->reference_cluster);
              }

            slot_location_add_reference_with_cluster(
                    the_value->u.slot_location, result->reference_cluster);
            result->u.slot_location = the_value->u.slot_location;

            break;
          }
        case VK_NULL:
          {
            assert(result->cluster_use_count == 0);
            break;
          }
        case VK_JUMP_TARGET:
          {
            assert(result->cluster_use_count == 0);
            jump_target_add_reference(the_value->u.jump_target);
            result->u.jump_target = the_value->u.jump_target;

            break;
          }
        case VK_ROUTINE:
          {
            assert(result->cluster_use_count == 0);

            if ((result->reference_cluster != NULL) &&
                (result->reference_cluster ==
                 routine_instance_reference_cluster(the_value->u.routine)))
              {
                assert(result->cluster_use_count == 0);
                result->cluster_use_count = 1;
                reference_cluster_add_reference(result->reference_cluster);
              }

            routine_instance_add_reference_with_cluster(the_value->u.routine,
                    result->reference_cluster);
            result->u.routine = the_value->u.routine;

            break;
          }
        case VK_ROUTINE_CHAIN:
          {
            assert(result->cluster_use_count == 0);

            if ((result->reference_cluster != NULL) &&
                (result->reference_cluster ==
                 routine_instance_chain_reference_cluster(
                         the_value->u.routine_instance_chain)))
              {
                assert(result->cluster_use_count == 0);
                result->cluster_use_count = 1;
                reference_cluster_add_reference(result->reference_cluster);
              }

            routine_instance_chain_add_reference_with_cluster(
                    the_value->u.routine_instance_chain,
                    result->reference_cluster);
            result->u.routine_instance_chain =
                    the_value->u.routine_instance_chain;

            break;
          }
        case VK_TYPE:
          {
            assert(result->cluster_use_count == 0);

            if ((result->reference_cluster != NULL) &&
                (result->reference_cluster ==
                 type_reference_cluster(the_value->u.type)))
              {
                result->reference_cluster = the_value->reference_cluster;
                result->cluster_use_count = 1;
                reference_cluster_add_reference(result->reference_cluster);
              }

            type_add_reference_with_reference_cluster(the_value->u.type,
                    result->reference_cluster);
            result->u.type = the_value->u.type;

            break;
          }
        case VK_OBJECT:
          {
            assert(result->cluster_use_count == 0);

            if ((result->reference_cluster != NULL) &&
                (result->reference_cluster ==
                 object_reference_cluster(the_value->u.object)))
              {
                assert(result->cluster_use_count == 0);
                result->cluster_use_count = 1;
                reference_cluster_add_reference(result->reference_cluster);
              }

            object_add_reference_with_cluster(the_value->u.object,
                                              result->reference_cluster);
            result->u.object = the_value->u.object;

            break;
          }
        case VK_TAGALONG_KEY:
          {
            assert(result->cluster_use_count == 0);

            if ((result->reference_cluster != NULL) &&
                (result->reference_cluster ==
                 tagalong_key_reference_cluster(the_value->u.tagalong_key)))
              {
                assert(result->cluster_use_count == 0);
                result->cluster_use_count = 1;
                reference_cluster_add_reference(result->reference_cluster);
              }

            tagalong_key_add_reference_with_cluster(the_value->u.tagalong_key,
                                                    result->reference_cluster);
            result->u.tagalong_key = the_value->u.tagalong_key;

            break;
          }
        case VK_LOCK:
          {
            assert(result->cluster_use_count == 0);

            if ((result->reference_cluster != NULL) &&
                (result->reference_cluster ==
                 lock_instance_reference_cluster(the_value->u.lock_instance)))
              {
                assert(result->cluster_use_count == 0);
                result->cluster_use_count = 1;
                reference_cluster_add_reference(result->reference_cluster);
              }

            lock_instance_add_reference_with_cluster(
                    the_value->u.lock_instance, result->reference_cluster);
            result->u.lock_instance = the_value->u.lock_instance;

            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    tagalong_tail = &(result->tagalong_chain);
    previous = NULL;

    GRAB_SYSTEM_LOCK(the_value->tagalong_lock);

    follow = the_value->tagalong_chain;
    while (follow != NULL)
      {
        value_tagalong_handle *new;

        new = MALLOC_ONE_OBJECT(value_tagalong_handle);
        if (new == NULL)
          {
            *tagalong_tail = NULL;
            RELEASE_SYSTEM_LOCK(the_value->tagalong_lock);
            delete_value(result, NULL);
            return NULL;
          }

        new->key = follow->key;
        new->parent = result;
        new->value_previous = previous;

        assert(follow->field_value != NULL);
        value_add_reference(follow->field_value);
        new->field_value = follow->field_value;

        *tagalong_tail = new;
        tagalong_tail = &(new->value_next);
        previous = new;

        new->key_previous = follow;
        new->key_next = follow->key_next;
        follow->key_next->key_previous = new;
        follow->key_next = new;

        follow = follow->value_next;
      }

    RELEASE_SYSTEM_LOCK(the_value->tagalong_lock);

    *tagalong_tail = NULL;

    validator_remove_reference(result->validator);
    result->validator = the_value->validator;
    validator_add_reference(the_value->validator);

    return result;
  }

extern value *map_value_from_semi_labeled_value_list(value *source)
  {
    value *result;
    map_info *result_info;
    o_integer oi_index;
    size_t count;
    size_t size_t_index;

    assert(source != NULL);

    assert(source->kind == VK_SEMI_LABELED_VALUE_LIST);

    GRAB_SYSTEM_LOCK(source->u.lepton.lock);
    result = source->u.lepton.map_equivalent;
    RELEASE_SYSTEM_LOCK(source->u.lepton.lock);

    if (result != NULL)
      {
        value_add_reference(result);
        return result;
      }

    result = create_map_value();
    if (result == NULL)
        return NULL;

    result_info = result->u.map.map_info;
    assert(result_info != NULL);

    oi_index = oi_zero;
    assert(!(oi_out_of_memory(oi_index)));
    oi_add_reference(oi_index);

    count = value_component_count(source);
    for (size_t_index = 0; size_t_index < count; ++size_t_index)
      {
        value *key_value;
        value *target_value;
        verdict the_verdict;
        o_integer new_oi;

        key_value = create_integer_value(oi_index);
        if (key_value == NULL)
          {
            oi_remove_reference(oi_index);
            update_value_cluster_for_map_info(result, NULL, NULL);
            value_remove_reference(result, NULL);
            return NULL;
          }

        target_value = value_component_value(source, size_t_index);
        the_verdict = map_info_append_value_key_item(result_info, key_value,
                                                     target_value);
        value_remove_reference(key_value, NULL);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            oi_remove_reference(oi_index);
            update_value_cluster_for_map_info(result, NULL, NULL);
            value_remove_reference(result, NULL);
            return NULL;
          }

        if (target_value != NULL)
          {
            result->validator = validator_add_validator(result->validator,
                    value_validator(target_value));
            if (result->validator == NULL)
              {
                oi_remove_reference(oi_index);
                update_value_cluster_for_map_info(result, NULL, NULL);
                value_remove_reference(result, NULL);
                return NULL;
              }
          }

        oi_add(new_oi, oi_index, oi_one);
        oi_remove_reference(oi_index);
        if (oi_out_of_memory(new_oi))
          {
            update_value_cluster_for_map_info(result, NULL, NULL);
            value_remove_reference(result, NULL);
            return NULL;
          }

        oi_index = new_oi;
      }

    oi_remove_reference(oi_index);

    result_info->item_count = count;

    update_value_cluster_for_map_info(result, NULL, NULL);

    GRAB_SYSTEM_LOCK(source->u.lepton.lock);

    if (source->u.lepton.map_equivalent == NULL)
      {
        value_add_reference_with_reference_cluster(result,
                                                   source->reference_cluster);
        source->u.lepton.map_equivalent = result;
      }

    RELEASE_SYSTEM_LOCK(source->u.lepton.lock);

    return result;
  }

extern boolean value_has_only_named_fields(value *the_value)
  {
    size_t component_count;
    size_t component_num;

    assert(the_value != NULL);

    switch (get_value_kind(the_value))
      {
        case VK_SEMI_LABELED_VALUE_LIST:
        case VK_SEMI_LABELED_MULTI_SET:
            break;
        default:
            return FALSE;
      }

    component_count = value_component_count(the_value);

    for (component_num = 0; component_num < component_count; ++component_num)
      {
        if (value_component_label(the_value, component_num) == NULL)
            return FALSE;
      }

    return TRUE;
  }

extern void value_add_reference(value *the_value)
  {
    reference_cluster *value_cluster;
    size_t cluster_use_count;

    assert(the_value != NULL);
    assert_is_malloced_block_with_exact_size(the_value, sizeof(value));

    GRAB_SYSTEM_LOCK(the_value->lock);
    assert(the_value->reference_count > 0);
    ++(the_value->reference_count);
    assert(the_value->reference_count >=
           the_value->no_cluster_reference_count);
    value_cluster = the_value->reference_cluster;
    cluster_use_count = the_value->cluster_use_count;
    RELEASE_SYSTEM_LOCK(the_value->lock);

    if ((value_cluster != NULL) && (cluster_use_count > 0))
        reference_cluster_add_reference(value_cluster);
  }

extern void value_remove_reference(value *the_value, jumper *the_jumper)
  {
    value *the_extension;
    size_t new_reference_count;
    reference_cluster *value_cluster;
    size_t cluster_use_count;

    assert(the_value != NULL);
    assert_is_malloced_block_with_exact_size(the_value, sizeof(value));

    if (the_value->kind == VK_MAP)
      {
        GRAB_SYSTEM_LOCK(the_value->u.map.lock);
        the_extension = the_value->u.map.first_extension;
        if (the_extension != NULL)
          {
            GRAB_SYSTEM_LOCK(the_extension->lock);
            if (the_extension->destructing)
              {
                RELEASE_SYSTEM_LOCK(the_extension->lock);
                the_extension = NULL;
              }
            else
              {
                assert(the_extension->reference_count > 0);
                ++(the_extension->reference_count);
                ++(the_extension->no_cluster_reference_count);
                assert(the_extension->reference_count >=
                       the_extension->no_cluster_reference_count);
                RELEASE_SYSTEM_LOCK(the_extension->lock);
              }
          }
        RELEASE_SYSTEM_LOCK(the_value->u.map.lock);
      }
    else
      {
        the_extension = NULL;
      }

    GRAB_SYSTEM_LOCK(the_value->lock);
    assert(the_value->reference_count > 0);
    --(the_value->reference_count);
    new_reference_count = the_value->reference_count;
    if (new_reference_count == 0)
        the_value->destructing = TRUE;
    assert(the_value->reference_count >=
           the_value->no_cluster_reference_count);
    value_cluster = the_value->reference_cluster;
    cluster_use_count = the_value->cluster_use_count;
    RELEASE_SYSTEM_LOCK(the_value->lock);

    if ((value_cluster != NULL) && (cluster_use_count > 0))
        reference_cluster_remove_reference(value_cluster, the_jumper);

    if ((new_reference_count == 1) && (the_extension != NULL) &&
        validator_is_valid(the_value->u.map.map_info->all_keys_validator) &&
        (the_jumper != NULL))
      {
        GRAB_SYSTEM_LOCK(the_extension->u.map.lock);
        if (the_extension->u.map.map_info->reference_count == 1)
            merge_extension(the_extension, the_value, the_jumper);
        RELEASE_SYSTEM_LOCK(the_extension->u.map.lock);
      }

    if (the_extension != NULL)
        value_remove_reference_skip_cluster(the_extension, the_jumper);

    if (new_reference_count == 0)
      {
        ++(the_value->reference_count);
        delete_value(the_value, the_jumper);
      }
  }

extern void value_add_reference_with_reference_cluster(value *the_value,
        reference_cluster *cluster)
  {
    reference_cluster *value_cluster;
    size_t cluster_use_count;

    assert(the_value != NULL);
    assert_is_malloced_block_with_exact_size(the_value, sizeof(value));

    GRAB_SYSTEM_LOCK(the_value->lock);
    assert(the_value->reference_count > 0);
    ++(the_value->reference_count);
    assert(the_value->reference_count >=
           the_value->no_cluster_reference_count);
    value_cluster = the_value->reference_cluster;
    cluster_use_count = the_value->cluster_use_count;
    RELEASE_SYSTEM_LOCK(the_value->lock);

    if ((value_cluster != NULL) && (cluster_use_count > 0) &&
        (value_cluster != cluster))
      {
        reference_cluster_add_reference(value_cluster);
      }
  }

extern void value_remove_reference_with_reference_cluster(value *the_value,
        jumper *the_jumper, reference_cluster *cluster)
  {
    value *the_extension;
    size_t new_reference_count;
    reference_cluster *value_cluster;
    size_t cluster_use_count;

    assert(the_value != NULL);
    assert_is_malloced_block_with_exact_size(the_value, sizeof(value));

    if (the_value->kind == VK_MAP)
      {
        GRAB_SYSTEM_LOCK(the_value->u.map.lock);
        the_extension = the_value->u.map.first_extension;
        if (the_extension != NULL)
          {
            assert_is_malloced_block_with_exact_size(the_extension,
                                                     sizeof(value));
            GRAB_SYSTEM_LOCK(the_extension->lock);
            if (the_extension->destructing)
              {
                RELEASE_SYSTEM_LOCK(the_extension->lock);
                the_extension = NULL;
              }
            else
              {
                assert(the_extension->reference_count > 0);
                ++(the_extension->reference_count);
                ++(the_extension->no_cluster_reference_count);
                assert(the_extension->reference_count >=
                       the_extension->no_cluster_reference_count);
                RELEASE_SYSTEM_LOCK(the_extension->lock);
              }
          }
        RELEASE_SYSTEM_LOCK(the_value->u.map.lock);
      }
    else
      {
        the_extension = NULL;
      }

    GRAB_SYSTEM_LOCK(the_value->lock);
    assert(the_value->reference_count > 0);
    --(the_value->reference_count);
    new_reference_count = the_value->reference_count;
    if (new_reference_count == 0)
        the_value->destructing = TRUE;
    assert(the_value->reference_count >=
           the_value->no_cluster_reference_count);
    value_cluster = the_value->reference_cluster;
    cluster_use_count = the_value->cluster_use_count;
    RELEASE_SYSTEM_LOCK(the_value->lock);

    if ((value_cluster != NULL) && (cluster_use_count > 0) &&
        (value_cluster != cluster))
      {
        reference_cluster_remove_reference(value_cluster, the_jumper);
      }

    if ((new_reference_count == 1) && (the_extension != NULL) &&
        validator_is_valid(the_value->u.map.map_info->all_keys_validator) &&
        (the_jumper != NULL))
      {
        GRAB_SYSTEM_LOCK(the_extension->u.map.lock);
        if (the_extension->u.map.map_info->reference_count == 1)
            merge_extension(the_extension, the_value, the_jumper);
        RELEASE_SYSTEM_LOCK(the_extension->u.map.lock);
      }

    if (the_extension != NULL)
        value_remove_reference_skip_cluster(the_extension, the_jumper);

    if (new_reference_count == 0)
      {
        ++(the_value->reference_count);
        delete_value(the_value, the_jumper);
      }
  }

extern reference_cluster *value_reference_cluster(value *the_value)
  {
    assert(the_value != NULL);

    if (the_value->cluster_use_count == 0)
        return NULL;
    else
        return the_value->reference_cluster;
  }

extern boolean value_is_valid(value *the_value)
  {
    assert(the_value != NULL);

    return validator_is_valid(the_value->validator);
  }

extern void check_value_validity(value *the_value,
        const source_location *location, jumper *the_jumper)
  {
    assert(the_value != NULL);
    assert(the_jumper != NULL);

    validator_check_validity(the_value->validator, location, the_jumper);
  }

extern void check_value_validity_except_map_targets(value *the_value,
        const source_location *location, jumper *the_jumper)
  {
    assert(the_value != NULL);
    assert(the_jumper != NULL);

    switch (the_value->kind)
      {
        case VK_MAP:
          {
            map_info *info;

            GRAB_SYSTEM_LOCK(the_value->u.map.lock);
            info = the_value->u.map.map_info;
            map_info_add_reference(info);
            RELEASE_SYSTEM_LOCK(the_value->u.map.lock);

            validator_check_validity(info->all_keys_validator, location,
                                     the_jumper);

            map_info_remove_reference(info, the_jumper);

            break;
          }
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            break;
          }
        case VK_SEMI_LABELED_MULTI_SET:
          {
            break;
          }
        case VK_LEPTON:
          {
            validator_check_validity(the_value->u.lepton.key_validator,
                                     location, the_jumper);
            break;
          }
        default:
          {
            validator_check_validity(the_value->validator, location,
                                     the_jumper);
            break;
          }
      }
  }

extern validator *value_validator(value *the_value)
  {
    assert(the_value != NULL);

    return the_value->validator;
  }

extern boolean map_value_all_keys_are_valid(value *map_value)
  {
    validator *all_keys_validator;
    boolean result;

    assert(map_value != NULL);

    assert(map_value->kind == VK_MAP);

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);
    all_keys_validator = map_value->u.map.map_info->all_keys_validator;
    validator_add_reference(all_keys_validator);
    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    result = validator_is_valid(all_keys_validator);

    validator_remove_reference(all_keys_validator);

    return result;
  }

extern validator *map_value_all_keys_validator(value *map_value)
  {
    assert(map_value != NULL);

    assert(map_value->kind == VK_MAP);

    return map_value->u.map.map_info->all_keys_validator;
  }

extern boolean value_is_valid_except_map_targets(value *the_value)
  {
    assert(the_value != NULL);

    switch (the_value->kind)
      {
        case VK_MAP:
          {
            validator *all_keys_validator;
            boolean result;

            GRAB_SYSTEM_LOCK(the_value->u.map.lock);
            all_keys_validator = the_value->u.map.map_info->all_keys_validator;
            validator_add_reference(all_keys_validator);
            RELEASE_SYSTEM_LOCK(the_value->u.map.lock);

            result = validator_is_valid(all_keys_validator);

            validator_remove_reference(all_keys_validator);

            return result;
          }
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            return TRUE;
          }
        case VK_SEMI_LABELED_MULTI_SET:
          {
            return TRUE;
          }
        case VK_LEPTON:
          {
            return validator_is_valid(the_value->u.lepton.key_validator);
          }
        default:
          {
            return validator_is_valid(the_value->validator);
          }
      }
  }

extern value *value_string_concatenate(value *left, value *right)
  {
    const char *string1;
    const char *string2;
    char *result_string;
    value *result_value;

    assert(left != NULL);
    assert(right != NULL);

    assert((left->kind == VK_STRING) || (left->kind == VK_CHARACTER));
    assert((right->kind == VK_STRING) || (right->kind == VK_CHARACTER));

    if (left->kind == VK_STRING)
        string1 = left->u.string_data;
    else
        string1 = &(left->u.character_data[0]);
    assert(string1 != NULL);

    if (right->kind == VK_STRING)
        string2 = right->u.string_data;
    else
        string2 = &(right->u.character_data[0]);
    assert(string2 != NULL);

    result_string = MALLOC_ARRAY(char,
            strlen(string1) + strlen(string2) + 1);
    if (result_string == NULL)
        return NULL;

    strcpy(result_string, string1);
    strcpy(result_string + strlen(string1), string2);

    result_value = create_empty_value(VK_STRING);
    if (result_value == NULL)
      {
        free(result_string);
        return NULL;
      }

    result_value->u.string_data = result_string;

    return result_value;
  }

extern boolean values_are_equal(value *value1, value *value2, boolean *doubt,
        const source_location *location, jumper *the_jumper)
  {
    assert(value1 != NULL);
    assert(value2 != NULL);
    assert(doubt != NULL);
    assert(the_jumper != NULL);

    assert(value_is_valid(value1)); /* VERIFIED */
    assert(value_is_valid(value2)); /* VERIFIED */

    *doubt = FALSE;

    if (value1 == value2)
        return TRUE;

    if (value1->kind != value2->kind)
      {
        if ((value1->kind == VK_MAP) &&
            (value2->kind == VK_SEMI_LABELED_VALUE_LIST))
          {
            assert(value_is_valid(value1)); /* VERIFIED */
            assert(value_is_valid(value2)); /* VERIFIED */
            return map_and_semi_labeled_value_list_are_equal(value1, value2,
                    doubt, location, the_jumper);
          }

        if ((value1->kind == VK_SEMI_LABELED_VALUE_LIST) &&
            (value2->kind == VK_MAP))
          {
            assert(value_is_valid(value2)); /* VERIFIED */
            assert(value_is_valid(value1)); /* VERIFIED */
            return map_and_semi_labeled_value_list_are_equal(value2, value1,
                    doubt, location, the_jumper);
          }

        return FALSE;
      }

    switch (value1->kind)
      {
        case VK_TRUE:
            return TRUE;
        case VK_FALSE:
            return TRUE;
        case VK_INTEGER:
            return oi_equal(value1->u.integer, value2->u.integer);
        case VK_RATIONAL:
            return rationals_are_equal(value1->u.rational, value2->u.rational);
        case VK_STRING:
            assert(value1->u.string_data != NULL);
            assert(value2->u.string_data != NULL);
            return (strcmp(value1->u.string_data, value2->u.string_data) == 0);
        case VK_CHARACTER:
            return (strcmp(&(value1->u.character_data[0]),
                           &(value2->u.character_data[0])) == 0);
        case VK_REGULAR_EXPRESSION:
            return (regular_expression_structural_order(
                            value1->u.regular_expression,
                            value2->u.regular_expression) == 0);
        case VK_SEMI_LABELED_VALUE_LIST:
            assert(value_is_valid(value1)); /* VERIFIED */
            assert(value_is_valid(value2)); /* VERIFIED */
            return semi_labeled_value_lists_are_equal(value1, value2, doubt,
                                                      location, the_jumper);
        case VK_SEMI_LABELED_MULTI_SET:
            assert(value_is_valid(value1)); /* VERIFIED */
            assert(value_is_valid(value2)); /* VERIFIED */
            return semi_labeled_multi_sets_are_equal(value1, value2, doubt,
                                                     location, the_jumper);
        case VK_MAP:
            assert(value_is_valid(value1)); /* VERIFIED */
            assert(value_is_valid(value2)); /* VERIFIED */
            return maps_are_equal(value1, value2, doubt, location, the_jumper);
        case VK_QUARK:
            assert(quark_is_instantiated(value1->u.quark)); /* VERIFIED */
            assert(quark_is_instantiated(value2->u.quark)); /* VERIFIED */
            assert(!(quark_scope_exited(value1->u.quark))); /* VERIFIED */
            assert(!(quark_scope_exited(value2->u.quark))); /* VERIFIED */
            return quarks_are_equal(value1->u.quark, value2->u.quark);
        case VK_LEPTON:
            assert(lepton_key_instance_is_instantiated(
                           value1->u.lepton.lepton_key)); /* VERIFIED */
            assert(lepton_key_instance_is_instantiated(
                           value2->u.lepton.lepton_key)); /* VERIFIED */
            assert(!(lepton_key_instance_scope_exited(
                             value1->u.lepton.lepton_key))); /* VERIFIED */
            assert(!(lepton_key_instance_scope_exited(
                             value2->u.lepton.lepton_key))); /* VERIFIED */
            if (!(lepton_key_instances_are_equal(value1->u.lepton.lepton_key,
                                                 value2->u.lepton.lepton_key)))
              {
                return FALSE;
              }
            assert(value_is_valid(value1)); /* VERIFIED */
            assert(value_is_valid(value2)); /* VERIFIED */
            return semi_labeled_multi_sets_are_equal(value1, value2, doubt,
                                                     location, the_jumper);
        case VK_LEPTON_KEY:
            assert(lepton_key_instance_is_instantiated(
                           value1->u.lepton.lepton_key)); /* VERIFIED */
            assert(lepton_key_instance_is_instantiated(
                           value2->u.lepton.lepton_key)); /* VERIFIED */
            assert(!(lepton_key_instance_scope_exited(
                             value1->u.lepton.lepton_key))); /* VERIFIED */
            assert(!(lepton_key_instance_scope_exited(
                             value2->u.lepton.lepton_key))); /* VERIFIED */
            return lepton_key_instances_are_equal(value1->u.lepton.lepton_key,
                                                  value2->u.lepton.lepton_key);
        case VK_SLOT_LOCATION:
            assert(slot_location_is_valid(value1->u.slot_location));
                    /* VERIFIED */
            assert(slot_location_is_valid(value2->u.slot_location));
                    /* VERIFIED */
            return slot_locations_are_equal(value1->u.slot_location,
                    value2->u.slot_location, doubt, location, the_jumper);
        case VK_NULL:
            return TRUE;
        case VK_JUMP_TARGET:
            assert(!(jump_target_scope_exited(value1->u.jump_target)));
                    /* VERIFIED */
            assert(!(jump_target_scope_exited(value2->u.jump_target)));
                    /* VERIFIED */
            return jump_targets_are_equal(value1->u.jump_target,
                                          value2->u.jump_target);
        case VK_ROUTINE:
            assert(routine_instance_is_instantiated(value1->u.routine));
                    /* VERIFIED */
            assert(routine_instance_is_instantiated(value2->u.routine));
                    /* VERIFIED */
            assert(!(routine_instance_scope_exited(value1->u.routine)));
                    /* VERIFIED */
            assert(!(routine_instance_scope_exited(value2->u.routine)));
                    /* VERIFIED */
            return routine_instances_are_equal(value1->u.routine,
                                               value2->u.routine);
        case VK_ROUTINE_CHAIN:
            assert(routine_instance_chain_is_valid(
                           value1->u.routine_instance_chain)); /* VERIFIED */
            assert(routine_instance_chain_is_valid(
                           value2->u.routine_instance_chain)); /* VERIFIED */
            return routine_instance_chains_are_equal(
                    value1->u.routine_instance_chain,
                    value2->u.routine_instance_chain);
        case VK_TYPE:
            assert(type_is_valid(value1->u.type)); /* VERIFIED */
            assert(type_is_valid(value2->u.type)); /* VERIFIED */
            return types_are_equal(value1->u.type, value2->u.type, doubt,
                                   location, the_jumper);
        case VK_OBJECT:
            assert(!(object_is_closed(value1->u.object))); /* VERIFIED */
            assert(!(object_is_closed(value2->u.object))); /* VERIFIED */
            return objects_are_equal(value1->u.object, value2->u.object);
        case VK_TAGALONG_KEY:
            assert(tagalong_key_is_instantiated(value1->u.tagalong_key));
                    /* VERIFIED */
            assert(tagalong_key_is_instantiated(value2->u.tagalong_key));
                    /* VERIFIED */
            assert(!(tagalong_key_scope_exited(value1->u.tagalong_key)));
                    /* VERIFIED */
            assert(!(tagalong_key_scope_exited(value2->u.tagalong_key)));
                    /* VERIFIED */
            return tagalong_keys_are_equal(value1->u.tagalong_key,
                                           value2->u.tagalong_key);
        case VK_LOCK:
            assert(lock_instance_is_instantiated(value1->u.lock_instance));
                    /* VERIFIED */
            assert(lock_instance_is_instantiated(value2->u.lock_instance));
                    /* VERIFIED */
            assert(!(lock_instance_scope_exited(value1->u.lock_instance)));
                    /* VERIFIED */
            assert(!(lock_instance_scope_exited(value2->u.lock_instance)));
                    /* VERIFIED */
            return lock_instances_are_equal(value1->u.lock_instance,
                                            value2->u.lock_instance);
        default:
            assert(FALSE);
            return FALSE;
      }
  }

extern int value_structural_order(value *left, value *right)
  {
    assert(left != NULL);
    assert(right != NULL);

    assert(value_is_valid(left)); /* VERIFIED */
    assert(value_is_valid(right)); /* VERIFIED */

    if (left == right)
        return 0;

    if ((left->kind == VK_SEMI_LABELED_VALUE_LIST) &&
        (semi_labeled_value_list_value_has_no_labels(left)))
      {
        value *left_map;
        int result;

        left_map = map_value_from_semi_labeled_value_list(left);
        if (left_map == NULL)
            return -2;

        assert(value_is_valid(left_map)); /* VERIFIED */
        assert(value_is_valid(right)); /* VERIFIED */
        result = value_structural_order(left_map, right);
        value_remove_reference(left_map, NULL);
        return result;
      }

    if ((right->kind == VK_SEMI_LABELED_VALUE_LIST) &&
        (semi_labeled_value_list_value_has_no_labels(right)))
      {
        value *right_map;
        int result;

        right_map = map_value_from_semi_labeled_value_list(right);
        if (right_map == NULL)
            return -2;

        assert(value_is_valid(left)); /* VERIFIED */
        assert(value_is_valid(right_map)); /* VERIFIED */
        result = value_structural_order(left, right_map);
        value_remove_reference(right_map, NULL);
        return result;
      }

    if (left->kind != right->kind)
        return ((left->kind < right->kind) ? -1 : 1);

    switch (left->kind)
      {
        case VK_TRUE:
          {
            return 0;
          }
        case VK_FALSE:
          {
            return 0;
          }
        case VK_INTEGER:
          {
            return oi_structural_order(left->u.integer, right->u.integer);
          }
        case VK_RATIONAL:
          {
            return rational_structural_order(left->u.rational,
                                             right->u.rational);
          }
        case VK_STRING:
          {
            assert(left->u.string_data != NULL);
            assert(right->u.string_data != NULL);
            return utf8_string_lexicographical_order_by_code_point(
                    left->u.string_data, right->u.string_data);
          }
        case VK_CHARACTER:
          {
            return utf8_string_lexicographical_order_by_code_point(
                    &(left->u.character_data[0]),
                    &(right->u.character_data[0]));
          }
        case VK_REGULAR_EXPRESSION:
          {
            return regular_expression_structural_order(
                    left->u.regular_expression, right->u.regular_expression);
          }
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            assert(value_is_valid(left)); /* VERIFIED */
            assert(value_is_valid(right)); /* VERIFIED */
            return semi_labeled_value_list_structural_order(left, right);
          }
        case VK_SEMI_LABELED_MULTI_SET:
          {
            assert(value_is_valid(left)); /* VERIFIED */
            assert(value_is_valid(right)); /* VERIFIED */
            return semi_labeled_multi_set_structural_order(left, right);
          }
        case VK_MAP:
          {
            assert(value_is_valid(left)); /* VERIFIED */
            assert(value_is_valid(right)); /* VERIFIED */
            return map_structural_order(left, right);
          }
        case VK_QUARK:
          {
            assert(quark_is_instantiated(left->u.quark)); /* VERIFIED */
            assert(quark_is_instantiated(right->u.quark)); /* VERIFIED */
            assert(!(quark_scope_exited(left->u.quark))); /* VERIFIED */
            assert(!(quark_scope_exited(right->u.quark))); /* VERIFIED */
            return quark_structural_order(left->u.quark, right->u.quark);
          }
        case VK_LEPTON:
          {
            int key_order;

            assert(lepton_key_instance_is_instantiated(
                           left->u.lepton.lepton_key)); /* VERIFIED */
            assert(lepton_key_instance_is_instantiated(
                           right->u.lepton.lepton_key)); /* VERIFIED */
            assert(!(lepton_key_instance_scope_exited(
                             left->u.lepton.lepton_key))); /* VERIFIED */
            assert(!(lepton_key_instance_scope_exited(
                             right->u.lepton.lepton_key))); /* VERIFIED */
            key_order = lepton_key_instance_structural_order(
                    left->u.lepton.lepton_key, right->u.lepton.lepton_key);
            if (key_order != 0)
                return key_order;
            assert(value_is_valid(left)); /* VERIFIED */
            assert(value_is_valid(right)); /* VERIFIED */
            return semi_labeled_multi_set_structural_order(left, right);
          }
        case VK_LEPTON_KEY:
          {
            assert(lepton_key_instance_is_instantiated(
                           left->u.lepton.lepton_key)); /* VERIFIED */
            assert(lepton_key_instance_is_instantiated(
                           right->u.lepton.lepton_key)); /* VERIFIED */
            assert(!(lepton_key_instance_scope_exited(
                             left->u.lepton.lepton_key))); /* VERIFIED */
            assert(!(lepton_key_instance_scope_exited(
                             right->u.lepton.lepton_key))); /* VERIFIED */
            return lepton_key_instance_structural_order(
                    left->u.lepton.lepton_key, right->u.lepton.lepton_key);
          }
        case VK_SLOT_LOCATION:
          {
            assert(slot_location_is_valid(left->u.slot_location));
                    /* VERIFIED */
            assert(slot_location_is_valid(right->u.slot_location));
                    /* VERIFIED */
            return slot_location_structural_order(left->u.slot_location,
                                                  right->u.slot_location);
          }
        case VK_NULL:
          {
            return 0;
          }
        case VK_JUMP_TARGET:
          {
            assert(!(jump_target_scope_exited(left->u.jump_target)));
                    /* VERIFIED */
            assert(!(jump_target_scope_exited(right->u.jump_target)));
                    /* VERIFIED */
            return jump_target_structural_order(left->u.jump_target,
                                                right->u.jump_target);
          }
        case VK_ROUTINE:
          {
            assert(routine_instance_is_instantiated(left->u.routine));
                    /* VERIFIED */
            assert(routine_instance_is_instantiated(right->u.routine));
                    /* VERIFIED */
            assert(!(routine_instance_scope_exited(left->u.routine)));
                    /* VERIFIED */
            assert(!(routine_instance_scope_exited(right->u.routine)));
                    /* VERIFIED */
            return routine_instance_structural_order(left->u.routine,
                                                     right->u.routine);
          }
        case VK_ROUTINE_CHAIN:
          {
            assert(routine_instance_chain_is_valid(
                           left->u.routine_instance_chain)); /* VERIFIED */
            assert(routine_instance_chain_is_valid(
                           right->u.routine_instance_chain)); /* VERIFIED */
            return routine_instance_chain_structural_order(
                    left->u.routine_instance_chain,
                    right->u.routine_instance_chain);
          }
        case VK_TYPE:
          {
            assert(type_is_valid(left->u.type)); /* VERIFIED */
            assert(type_is_valid(right->u.type)); /* VERIFIED */
            return type_structural_order(left->u.type, right->u.type);
          }
        case VK_OBJECT:
          {
            assert(!(object_is_closed(left->u.object))); /* VERIFIED */
            assert(!(object_is_closed(right->u.object))); /* VERIFIED */
            return object_structural_order(left->u.object, right->u.object);
          }
        case VK_TAGALONG_KEY:
          {
            assert(tagalong_key_is_instantiated(left->u.tagalong_key));
                    /* VERIFIED */
            assert(tagalong_key_is_instantiated(right->u.tagalong_key));
                    /* VERIFIED */
            assert(!(tagalong_key_scope_exited(left->u.tagalong_key)));
                    /* VERIFIED */
            assert(!(tagalong_key_scope_exited(right->u.tagalong_key)));
                    /* VERIFIED */
            return tagalong_key_structural_order(left->u.tagalong_key,
                                                 right->u.tagalong_key);
          }
        case VK_LOCK:
          {
            assert(lock_instance_is_instantiated(left->u.lock_instance));
                    /* VERIFIED */
            assert(lock_instance_is_instantiated(right->u.lock_instance));
                    /* VERIFIED */
            assert(!(lock_instance_scope_exited(left->u.lock_instance)));
                    /* VERIFIED */
            assert(!(lock_instance_scope_exited(right->u.lock_instance)));
                    /* VERIFIED */
            return lock_instance_structural_order(left->u.lock_instance,
                                                  right->u.lock_instance);
          }
        default:
          {
            assert(FALSE);
            return 0;
          }
      }
  }

extern boolean value_is_slippery(value *the_value)
  {
    assert(the_value != NULL);

    switch (the_value->kind)
      {
        case VK_TRUE:
        case VK_FALSE:
        case VK_STRING:
        case VK_CHARACTER:
        case VK_REGULAR_EXPRESSION:
        case VK_QUARK:
        case VK_LEPTON_KEY:
        case VK_NULL:
        case VK_JUMP_TARGET:
        case VK_ROUTINE:
        case VK_ROUTINE_CHAIN:
        case VK_OBJECT:
        case VK_TAGALONG_KEY:
        case VK_LOCK:
          {
            return FALSE;
          }
        case VK_INTEGER:
          {
            return ((oi_kind(the_value->u.integer) == IIK_UNSIGNED_INFINITY) ||
                    (oi_kind(the_value->u.integer) == IIK_ZERO_ZERO));
          }
        case VK_RATIONAL:
          {
            o_integer numerator;

            numerator = rational_numerator(the_value->u.rational);
            assert(!(oi_out_of_memory(numerator)));
            return ((oi_kind(numerator) == IIK_UNSIGNED_INFINITY) ||
                    (oi_kind(numerator) == IIK_ZERO_ZERO));
          }
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            return semi_labeled_value_list_is_slippery(the_value);
          }
        case VK_SEMI_LABELED_MULTI_SET:
        case VK_LEPTON:
          {
            return semi_labeled_multi_set_is_slippery(the_value);
          }
        case VK_MAP:
          {
            return map_is_slippery(the_value);
          }
        case VK_SLOT_LOCATION:
          {
            return slot_location_is_slippery(the_value->u.slot_location);
          }
        case VK_TYPE:
          {
            return type_is_slippery(the_value->u.type);
          }
        default:
          {
            assert(FALSE);
            return TRUE;
          }
      }
  }

extern void print_value(value *the_value,
        void (*printer)(void *data, const char *format, ...), void *data)
  {
    print_value_with_override(the_value, printer, data, &print_value);
  }

extern void print_value_with_override(value *the_value,
        void (*printer)(void *data, const char *format, ...), void *data,
        void (*override)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data))
  {
    assert(the_value != NULL);
    assert(printer != NULL);

    switch (get_value_kind(the_value))
      {
        case VK_TRUE:
          {
            (*printer)(data, "true");
            break;
          }
        case VK_FALSE:
          {
            (*printer)(data, "false");
            break;
          }
        case VK_INTEGER:
          {
            print_oi(integer_value_data(the_value), printer, data);
            break;
          }
        case VK_RATIONAL:
          {
            rational *the_rational;

            the_rational = rational_value_data(the_value);
            assert(the_rational != NULL);
            print_oi(rational_numerator(the_rational), printer, data);
            (*printer)(data, "/");
            print_oi(rational_denominator(the_rational), printer, data);

            break;
          }
        case VK_STRING:
        case VK_CHARACTER:
          {
            const char *chars;
            boolean last_was_hex;
            const char *follow;

            if (get_value_kind(the_value) == VK_STRING)
              {
                chars = string_value_data(the_value);
                (*printer)(data, "\"");
              }
            else
              {
                chars = character_value_data(the_value);
                (*printer)(data, "\'");
              }
            assert(chars != NULL);

            last_was_hex = FALSE;
            follow = chars;
            while (*follow != 0)
              {
                if (last_was_hex &&
                    (((*follow >= '0') && (*follow <= '9')) ||
                     ((*follow >= 'a') && (*follow <= 'f')) ||
                     ((*follow >= 'A') && (*follow <= 'F'))))
                  {
                    (*printer)(data, "\\%03o",
                               (unsigned long)(unsigned char)(*follow));
                    ++follow;
                    last_was_hex = FALSE;
                    continue;
                  }
                last_was_hex = FALSE;
                if (((*follow >= '0') && (*follow <= '9')) ||
                    ((*follow >= 'a') && (*follow <= 'z')) ||
                    ((*follow >= 'A') && (*follow <= 'Z')) ||
                    (*follow == ' ') || (*follow == '!') || (*follow == '#') ||
                    (*follow == '$') || (*follow == '%') || (*follow == '&') ||
                    (*follow == '(') || (*follow == ')') || (*follow == '*') ||
                    (*follow == '+') || (*follow == ',') || (*follow == '-') ||
                    (*follow == '.') || (*follow == '/') || (*follow == ':') ||
                    (*follow == ';') || (*follow == '<') || (*follow == '=') ||
                    (*follow == '>') || (*follow == '?') || (*follow == '@') ||
                    (*follow == '[') || (*follow == ']') || (*follow == '^') ||
                    (*follow == '_') || (*follow == '`') || (*follow == '{') ||
                    (*follow == '|') || (*follow == '}') || (*follow == '~'))
                  {
                    (*printer)(data, "%c", *follow);
                  }
                else if ((*follow == '\'') || (*follow == '"') ||
                         (*follow == '\\'))
                  {
                    (*printer)(data, "\\%c", *follow);
                  }
                else if (*follow == '\a')
                  {
                    (*printer)(data, "\\a");
                  }
                else if (*follow == '\b')
                  {
                    (*printer)(data, "\\b");
                  }
                else if (*follow == '\f')
                  {
                    (*printer)(data, "\\f");
                  }
                else if (*follow == '\n')
                  {
                    (*printer)(data, "\\n");
                  }
                else if (*follow == '\r')
                  {
                    (*printer)(data, "\\r");
                  }
                else if (*follow == '\t')
                  {
                    (*printer)(data, "\\t");
                  }
                else if (*follow == '\v')
                  {
                    (*printer)(data, "\\v");
                  }
                else
                  {
                    int byte_count;

                    byte_count = validate_utf8_character(follow);
                    assert(byte_count > 0);
                    (*printer)(data, "\\x%0lx", utf8_to_code_point(follow));
                    follow += (byte_count - 1);
                    last_was_hex = TRUE;
                  }
                ++follow;
              }

            if (get_value_kind(the_value) == VK_STRING)
                (*printer)(data, "\"");
            else
                (*printer)(data, "\'");

            break;
          }
        case VK_REGULAR_EXPRESSION:
          {
            (*printer)(data, "@%s@",
                    regular_expression_pattern(
                            regular_expression_value_data(the_value)));
            break;
          }
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            print_component_value(the_value, printer, data, override);
            break;
          }
        case VK_SEMI_LABELED_MULTI_SET:
          {
            print_component_value(the_value, printer, data, override);
            break;
          }
        case VK_MAP:
          {
            size_t count;
            size_t item_num;

            (*printer)(data, "<<");

            count = map_value_item_count(the_value);

            for (item_num = 0; item_num < count; ++item_num)
              {
                value *target;

                if (item_num > 0)
                    (*printer)(data, ", ");

                (*printer)(data, "(");

                if (map_value_item_is_type(the_value, item_num))
                  {
                    type *key_type;

                    (*printer)(data, "*");

                    key_type = map_value_item_key_type(the_value, item_num);
                    if (get_type_kind(key_type) != TK_ANYTHING)
                      {
                        (*printer)(data, ": ");
                        print_type_with_override(key_type, printer, data,
                                                 override, TEPP_TOP);
                      }
                  }
                else
                  {
                    value *key;

                    key = map_value_item_key_value(the_value, item_num);
                    (*override)(key, printer, data);
                  }

                (*printer)(data, " --> ");

                target = map_value_item_target(the_value, item_num);
                (*override)(target, printer, data);

                (*printer)(data, ")");
              }

            (*printer)(data, ">>");

            break;
          }
        case VK_QUARK:
          {
            print_instance(printer, data,
                           quark_instance_instance(value_quark(the_value)));
            break;
          }
        case VK_LEPTON:
          {
            print_instance(printer, data,
                    lepton_key_instance_instance(value_lepton_key(the_value)));
            print_component_value(the_value, printer, data, override);
            break;
          }
        case VK_LEPTON_KEY:
          {
            print_instance(printer, data,
                    lepton_key_instance_instance(value_lepton_key(the_value)));
            break;
          }
        case VK_SLOT_LOCATION:
          {
            (*printer)(data, "&");
            print_slot_location(slot_location_value_data(the_value), printer,
                                data, override, EPP_UNARY);
            break;
          }
        case VK_NULL:
          {
            (*printer)(data, "null");
            break;
          }
        case VK_JUMP_TARGET:
          {
            jump_target *target;

            target = jump_target_value_data(the_value);
            assert(target != NULL);

            switch (get_jump_target_kind(target))
              {
                case JTK_LABEL:
                    (*printer)(data, "%s",
                            label_statement_name(
                                    label_jump_target_label_statement(
                                            target)));
                    break;
                case JTK_ROUTINE_RETURN:
                case JTK_TOP_LEVEL_RETURN:
                case JTK_LOOP_CONTINUE:
                case JTK_LOOP_BREAK:
                case JTK_BLOCK_EXPRESSION_RETURN:
                case JTK_TRY_CATCH_CATCH:
                    (*printer)(data, "jump_target_%p", target);
                    break;
                default:
                    assert(FALSE);
              }

            break;
          }
        case VK_ROUTINE:
          {
            print_instance(printer, data,
                    routine_instance_instance(routine_value_data(the_value)));
            break;
          }
        case VK_ROUTINE_CHAIN:
          {
            routine_instance *first_instance;
            routine_declaration *declaration;

            first_instance = routine_instance_chain_instance(
                    routine_chain_value_data(the_value));
            assert(first_instance != NULL);

            declaration = routine_instance_declaration(first_instance);
            assert(declaration != NULL);

            if (routine_declaration_name(declaration) != NULL)
              {
                (*printer)(data, "%s", routine_declaration_name(declaration));
              }
            else
              {
                (*printer)(data, "routine_%p",
                           routine_chain_value_data(the_value));
              }

            break;
          }
        case VK_TYPE:
          {
            (*printer)(data, "type ");
            print_type_with_override(type_value_data(the_value), printer, data,
                                     override, TEPP_TOP);
            (*printer)(data, "");
            break;
          }
        case VK_OBJECT:
          {
            (*printer)(data, "object(%p)", object_value_data(the_value));
            break;
          }
        case VK_TAGALONG_KEY:
          {
            print_instance(printer, data,
                    tagalong_key_instance(tagalong_key_value_data(the_value)));
            break;
          }
        case VK_LOCK:
          {
            print_instance(printer, data,
                           lock_instance_instance(lock_value_data(the_value)));
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }
  }

extern void print_oi(o_integer oi,
        void (*printer)(void *data, const char *format, ...), void *data)
  {
    size_t digit_count;
    verdict the_verdict;
    char *digit_buffer;

    assert(!(oi_out_of_memory(oi)));
    assert(printer != NULL);

    if (oi_kind(oi) != IIK_FINITE)
      {
        switch (oi_kind(oi))
          {
            case IIK_POSITIVE_INFINITY:
                (*printer)(data, "+oo");
                return;
            case IIK_NEGATIVE_INFINITY:
                (*printer)(data, "-oo");
                return;
            case IIK_UNSIGNED_INFINITY:
                (*printer)(data, "1/0");
                return;
            case IIK_ZERO_ZERO:
                (*printer)(data, "0/0");
                return;
            default:
                assert(FALSE);
                return;
          }
      }

    the_verdict = oi_decimal_digit_count(oi, &digit_count);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return;

    digit_buffer = MALLOC_ARRAY(char, digit_count + 1);
    if (digit_buffer == NULL)
        return;

    the_verdict = oi_write_decimal_digits(oi, digit_buffer);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(digit_buffer);
        return;
      }

    digit_buffer[digit_count] = 0;

    (*printer)(data, "%s%s%s", (oi_is_negative(oi) ? "-" : ""),
               ((digit_count == 0) ? "0" : ""), digit_buffer);

    free(digit_buffer);
  }

extern void fp_printer(void *data, const char *format, ...)
  {
    FILE *fp;
    va_list ap;

    assert(data != NULL);

    fp = (FILE *)data;

    va_start(ap, format);
    vfprintf(fp, format, ap);
    va_end(ap);
  }

extern value *lookup_tagalong(value *base_value, tagalong_key *key,
        boolean is_for_write, const source_location *location,
        jumper *the_jumper)
  {
    tagalong_declaration *the_tagalong_declaration;
    value_tagalong_handle *follow;

    assert(base_value != NULL);
    assert(key != NULL);

    assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
    assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */

    the_tagalong_declaration = tagalong_key_declaration(key);
    if (tagalong_declaration_is_object(the_tagalong_declaration))
      {
        if (base_value->kind != VK_OBJECT)
          {
            location_exception(the_jumper, location,
                    (is_for_write ?
                     EXCEPTION_TAG(object_tagalong_write_non_object) :
                     EXCEPTION_TAG(object_tagalong_read_non_object)),
                    "An object tagalong key (%a) was used to try to %s a "
                    "tagalong on a non-object value.",
                    tagalong_declaration_declaration(the_tagalong_declaration),
                    (is_for_write ? "set" : "read"));
            return NULL;
          }

        return object_lookup_tagalong(base_value->u.object, key);
      }

    GRAB_SYSTEM_LOCK(base_value->tagalong_lock);

    follow = base_value->tagalong_chain;
    while (follow != NULL)
      {
        assert(follow->parent == base_value);
        if (follow->key == key)
          {
            RELEASE_SYSTEM_LOCK(base_value->tagalong_lock);
            return follow->field_value;
          }

        follow = follow->value_next;
      }

    RELEASE_SYSTEM_LOCK(base_value->tagalong_lock);

    assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
    assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */
    return tagalong_key_default_value(key);
  }

extern void set_tagalong(value *base_value, tagalong_key *key,
        value *new_value, const source_location *location, jumper *the_jumper)
  {
    tagalong_declaration *the_tagalong_declaration;
    value_tagalong_handle *follow;
    value_tagalong_handle *new;
    value_tagalong_handle *old;

    assert(base_value != NULL);
    assert(key != NULL);
    assert(new_value != NULL);

    assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
    assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */

    the_tagalong_declaration = tagalong_key_declaration(key);
    if (tagalong_declaration_is_object(the_tagalong_declaration))
      {
        if (base_value->kind != VK_OBJECT)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(object_tagalong_write_non_object),
                    "An object tagalong key (%a) was used to try to set a "
                    "tagalong on a non-object value.",
                    tagalong_declaration_declaration(
                            the_tagalong_declaration));
            return;
          }

        object_set_tagalong(base_value->u.object, key, new_value, the_jumper);
        return;
      }

    GRAB_SYSTEM_LOCK(base_value->tagalong_lock);

    follow = base_value->tagalong_chain;
    while (follow != NULL)
      {
        assert(follow->parent == base_value);
        if (follow->key == key)
          {
            value *old_value;

            assert(follow->field_value != NULL);
            value_add_reference(new_value);
            old_value = follow->field_value;
            follow->field_value = new_value;
            RELEASE_SYSTEM_LOCK(base_value->tagalong_lock);
            value_remove_reference(old_value, the_jumper);
            return;
          }

        follow = follow->value_next;
      }

    new = MALLOC_ONE_OBJECT(value_tagalong_handle);
    if (new == NULL)
      {
        RELEASE_SYSTEM_LOCK(base_value->tagalong_lock);
        jumper_do_abort(the_jumper);
        return;
      }

    new->key = key;
    new->parent = base_value;
    new->value_next = base_value->tagalong_chain;
    new->value_previous = NULL;

    grab_tagalong_key_value_tagalong_lock(key);

    old = get_value_tagalong_handle(key);
    if (old == NULL)
      {
        new->key_next = new;
        new->key_previous = new;
        set_value_tagalong_handle(key, new, the_jumper);
        assert((jumper_flowing_forward(the_jumper)));
      }
    else
      {
        new->key_previous = old;
        new->key_next = old->key_next;
        old->key_next->key_previous = new;
        old->key_next = new;
        release_tagalong_key_value_tagalong_lock(key);
      }

    value_add_reference(new_value);
    new->field_value = new_value;

    base_value->tagalong_chain = new;
    if (new->value_next != NULL)
        new->value_next->value_previous = new;

    RELEASE_SYSTEM_LOCK(base_value->tagalong_lock);
  }

extern void kill_value_tagalong(value_tagalong_handle *handle,
                                jumper *the_jumper)
  {
    value *parent;
    value_tagalong_handle *value_previous;
    value_tagalong_handle *value_next;
    value_tagalong_handle *key_previous;
    value_tagalong_handle *key_next;

    assert(handle != NULL);

    assert(handle->field_value != NULL);

    parent = handle->parent;
    assert(parent != NULL);

    GRAB_SYSTEM_LOCK(parent->tagalong_lock);

    value_previous = handle->value_previous;
    value_next = handle->value_next;

    if (value_previous != NULL)
      {
        value_previous->value_next = value_next;
      }
    else
      {
        assert(parent->tagalong_chain == handle);
        parent->tagalong_chain = value_next;
      }

    if (value_next != NULL)
        value_next->value_previous = value_previous;

    key_previous = handle->key_previous;
    key_next = handle->key_next;

    grab_tagalong_key_value_tagalong_lock(handle->key);

    if (key_next == handle)
      {
        assert(key_previous == handle);
        assert(get_value_tagalong_handle(handle->key) == handle);
        set_value_tagalong_handle(handle->key, NULL, the_jumper);
      }
    else
      {
        assert(key_previous != handle);
        key_previous->key_next = key_next;
        key_next->key_previous = key_previous;
        if (get_value_tagalong_handle(handle->key) == handle)
            set_value_tagalong_handle(handle->key, key_next, the_jumper);
        else
            release_tagalong_key_value_tagalong_lock(handle->key);
      }

    RELEASE_SYSTEM_LOCK(parent->tagalong_lock);

    value_remove_reference(handle->field_value, the_jumper);

    free(handle);
  }

extern void assert_is_live_value(value *the_value)
  {
    assert(the_value != NULL);
    assert_is_malloced_block_with_exact_size(the_value, sizeof(value));
    assert(the_value->reference_count > 0);
  }


static void delete_value(value *the_value, jumper *the_jumper)
  {
    assert(the_value != NULL);

    assert(the_value->reference_count == 1);
    assert(the_value->destructing);

    if (the_value->validator != NULL)
        validator_remove_reference(the_value->validator);

    switch (the_value->kind)
      {
        case VK_TRUE:
          {
            break;
          }
        case VK_FALSE:
          {
            break;
          }
        case VK_INTEGER:
          {
            oi_remove_reference(the_value->u.integer);
            break;
          }
        case VK_RATIONAL:
          {
            rational_remove_reference(the_value->u.rational);
            break;
          }
        case VK_STRING:
          {
            assert(the_value->u.string_data != NULL);
            free(the_value->u.string_data);
            break;
          }
        case VK_CHARACTER:
          {
            break;
          }
        case VK_REGULAR_EXPRESSION:
          {
            regular_expression_remove_reference(
                    the_value->u.regular_expression);
            break;
          }
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            deallocate_lepton_components(the_value, the_jumper);
            break;
          }
        case VK_SEMI_LABELED_MULTI_SET:
          {
            deallocate_lepton_components(the_value, the_jumper);
            break;
          }
        case VK_MAP:
          {
            clear_map_fields(the_value, the_jumper);
            break;
          }
        case VK_QUARK:
          {
            quark_remove_reference_with_cluster(the_value->u.quark, the_jumper,
                                                the_value->reference_cluster);
            break;
          }
        case VK_LEPTON:
          {
            if (the_value->u.lepton.key_validator != NULL)
                validator_remove_reference(the_value->u.lepton.key_validator);
            lepton_key_instance_remove_reference_with_cluster(
                    the_value->u.lepton.lepton_key, the_jumper,
                    the_value->reference_cluster);
            deallocate_lepton_components(the_value, the_jumper);
            break;
          }
        case VK_LEPTON_KEY:
          {
            lepton_key_instance_remove_reference_with_cluster(
                    the_value->u.lepton.lepton_key, the_jumper,
                    the_value->reference_cluster);
            break;
          }
        case VK_SLOT_LOCATION:
          {
            slot_location_remove_reference_with_cluster(
                    the_value->u.slot_location, the_value->reference_cluster,
                    the_jumper);
            break;
          }
        case VK_NULL:
          {
            break;
          }
        case VK_JUMP_TARGET:
          {
            jump_target_remove_reference(the_value->u.jump_target);
            break;
          }
        case VK_ROUTINE:
          {
            routine_instance_remove_reference_with_cluster(
                    the_value->u.routine, the_value->reference_cluster,
                    the_jumper);
            break;
          }
        case VK_ROUTINE_CHAIN:
          {
            routine_instance_chain_remove_reference_with_cluster(
                    the_value->u.routine_instance_chain,
                    the_value->reference_cluster, the_jumper);
            break;
          }
        case VK_TYPE:
          {
            type_remove_reference_with_reference_cluster(the_value->u.type,
                    the_jumper, the_value->reference_cluster);
            break;
          }
        case VK_OBJECT:
          {
            object_remove_reference_with_cluster(the_value->u.object,
                    the_value->reference_cluster, the_jumper);
            break;
          }
        case VK_TAGALONG_KEY:
          {
            tagalong_key_remove_reference_with_cluster(
                    the_value->u.tagalong_key, the_jumper,
                    the_value->reference_cluster);
            break;
          }
        case VK_LOCK:
          {
            lock_instance_remove_reference_with_cluster(
                    the_value->u.lock_instance, the_jumper,
                    the_value->reference_cluster);
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    delete_tagalongs(the_value, the_jumper);

    DESTROY_SYSTEM_LOCK(the_value->tagalong_lock);
    DESTROY_SYSTEM_LOCK(the_value->lock);
    the_value->reference_count = 0;

    free(the_value);
  }

static void delete_value_common(value *the_value)
  {
    assert(the_value != NULL);

    the_value->kind = VK_TRUE;
    delete_value(the_value, NULL);
  }

static value *map_value_lookup_with_index(value *map_value, value *key,
        boolean *doubt, size_t *index, const source_location *location,
        jumper *the_jumper)
  {
    map_info *info;
    boolean found;
    size_t position;
    size_t filter_count;
    size_t base_count;
    boolean are_equal;
    value *result;

    assert(map_value != NULL);
    assert(key != NULL);
    assert(doubt != NULL);
    assert(the_jumper != NULL);

    assert(map_value->kind == VK_MAP);

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);
    info = map_value->u.map.map_info;
    map_info_add_reference(info);
    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    assert(validator_is_valid(info->all_keys_validator)); /* VERIFIED */
    assert(value_is_valid(key)); /* VERIFIED */

    position = map_info_local_index_for_key(info, key, &found, doubt, FALSE,
                                            location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        map_info_remove_reference(info, the_jumper);
        return NULL;
      }

    if (*doubt)
      {
        map_info_remove_reference(info, the_jumper);
        return NULL;
      }

    filter_count = info->type_key_keys.element_count;
    assert(filter_count == info->type_key_targets.element_count);

    base_count = (info->is_extension ?
                  map_value_item_count(info->extension.base) : 0);

    if (!found)
      {
        type **keys;
        value **targets;
        size_t filter_num;

        keys = info->type_key_keys.array;
        targets = info->type_key_targets.array;

        filter_num = filter_count;
        while (filter_num > 0)
          {
            boolean is_in;

            --filter_num;

            assert(value_is_valid(key)); /* VERIFIED */
            assert(type_is_valid(keys[filter_num])); /* VERIFIED */
            is_in = value_is_in_type(key, keys[filter_num], doubt, NULL,
                                     location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                map_info_remove_reference(info, the_jumper);
                return NULL;
              }

            if (*doubt)
              {
                map_info_remove_reference(info, the_jumper);
                return NULL;
              }

            if (is_in)
              {
                value *result;

                result = targets[filter_num];
                if (index != NULL)
                    *index = base_count + filter_num;
                map_info_remove_reference(info, the_jumper);
                return result;
              }
          }

        if (info->is_extension)
          {
            value *base;
            size_t base_index;
            value *result;

            base = info->extension.base;
            assert(base != NULL);
            assert(base->kind == VK_MAP);
            GRAB_SYSTEM_LOCK(base->u.map.lock);
            assert(validator_is_valid(
                           base->u.map.map_info->all_keys_validator));
                    /* VERIFIED */
            RELEASE_SYSTEM_LOCK(base->u.map.lock);
            assert(value_is_valid(key)); /* VERIFIED */

            result = map_value_lookup_with_index(base, key, doubt, &base_index,
                                                 location, the_jumper);

            if (result != NULL)
              {
                value *replacement;

                replacement = map_base_replacement(info, base_index);
                if (replacement != NULL)
                    result = replacement;
              }

            map_info_remove_reference(info, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return NULL;

            if (index != NULL)
                *index = base_index;

            return result;
          }
        else
          {
            map_info_remove_reference(info, the_jumper);
            return NULL;
          }
      }

    assert(position > 0);
    --position;

    assert(info->value_key_keys.element_count ==
           info->value_key_targets.element_count);
    assert(position < info->value_key_keys.element_count);
    assert(value_is_valid(key)); /* VERIFIED */
    assert(value_is_valid(info->value_key_keys.array[position]));
            /* VERIFIED */
    are_equal = values_are_equal(key, info->value_key_keys.array[position],
                                 doubt, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        map_info_remove_reference(info, the_jumper);
        return NULL;
      }

    if (*doubt)
      {
        map_info_remove_reference(info, the_jumper);
        return NULL;
      }

    assert(are_equal);

    result = info->value_key_targets.array[position];

    if (index != NULL)
        *index = base_count + filter_count + position;

    map_info_remove_reference(info, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    return result;
  }

static verdict map_value_insert(value *map_value, value *key, value *target,
        const source_location *location, jumper *the_jumper)
  {
    map_info *info;
    boolean found;
    size_t position;
    boolean doubt;
    value *updated_key;
    size_t to_move;
    verdict the_verdict;

    assert(map_value != NULL);
    assert(key != NULL);
    assert(target != NULL);
    assert(the_jumper != NULL);

    assert(map_value->kind == VK_MAP);
#ifndef NDEBUG
    GRAB_SYSTEM_LOCK(map_value->lock);
    assert(map_value->reference_count - map_value->no_cluster_reference_count
           == 1);
    RELEASE_SYSTEM_LOCK(map_value->lock);
#endif /* !NDEBUG */

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);
    info = map_value->u.map.map_info;
    map_info_add_reference(info);
    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    assert(validator_is_valid(info->all_keys_validator)); /* VERIFIED */
    assert(value_is_valid(key)); /* VERIFIED */

    position = map_info_local_index_for_key(info, key, &found, &doubt, TRUE,
                                            location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        map_info_remove_reference(info, the_jumper);
        update_value_cluster_for_map_info(map_value, NULL, the_jumper);
        return MISSION_FAILED;
      }

    if (doubt)
      {
        assert(validator_is_valid(info->all_keys_validator)); /* VERIFIED */
        map_info_remove_reference(info, the_jumper);
        update_value_cluster_for_map_info(map_value, NULL, the_jumper);
        return MISSION_FAILED;
      }

    assert(position > 0);

    if (found)
      {
        assert(position > 0);

        --position;

        replace_value_key_target(info, position, target, key,
                &(map_value->validator), location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            map_info_remove_reference(info, the_jumper);
            update_value_cluster_for_map_info(map_value, NULL, the_jumper);
            return MISSION_FAILED;
          }

        assert(validator_is_valid(info->all_keys_validator)); /* VERIFIED */
        map_info_remove_reference(info, the_jumper);
        update_value_cluster_for_map_info(map_value, NULL, the_jumper);
        return MISSION_ACCOMPLISHED;
      }

    updated_key = key;

    if (info->is_extension)
      {
        value *base_map;
        boolean base_found;
        boolean base_doubt;
        size_t base_position;
        boolean does_overlap;
        boolean might_hit;

        base_map = info->extension.base;
        assert(base_map != NULL);
        assert(base_map->kind == VK_MAP);
        assert(validator_is_valid(info->all_keys_validator)); /* VERIFIED */
        assert(value_is_valid(key)); /* VERIFIED */
        base_position = map_value_global_index_for_key(base_map, key,
                &base_found, &base_doubt, FALSE, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            map_info_remove_reference(info, the_jumper);
            update_value_cluster_for_map_info(map_value, NULL, the_jumper);
            return MISSION_FAILED;
          }

        does_overlap = ((!base_doubt) && base_found);

        if (base_doubt)
            info->extension.is_uncompressable = TRUE;

        might_hit = might_hit_map_type_key(info, key, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            map_info_remove_reference(info, the_jumper);
            update_value_cluster_for_map_info(map_value, NULL, the_jumper);
            return MISSION_FAILED;
          }

        if (does_overlap)
          {
            char buffer[(sizeof(size_t) * 3) + 2];

            assert(base_position > 0);

            sprintf(&(buffer[0]), "%lu", (unsigned long)(base_position - 1));

            if (!(exists_in_string_index(info->extension.overlap_index,
                                         &(buffer[0]))))
              {
                value *base_target;

                base_target = map_base_target(info, base_position - 1);
                assert(base_target != NULL);
                value_add_reference(base_target);

                if (might_hit)
                  {
                    verdict the_verdict;

                    updated_key = map_value_item_key_value(base_map,
                                                           base_position - 1);

                    map_value->validator = validator_remove_validator(
                            map_value->validator,
                            value_validator(base_target));
                    if (map_value->validator == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        value_remove_reference(base_target, the_jumper);
                        map_info_remove_reference(info, the_jumper);
                        update_value_cluster_for_map_info(map_value, NULL,
                                                          the_jumper);
                        return MISSION_FAILED;
                      }

                    if (value_is_slippery(base_target))
                        --(info->slippery_count);

                    value_remove_reference(base_target, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        map_info_remove_reference(info, the_jumper);
                        update_value_cluster_for_map_info(map_value, NULL,
                                                          the_jumper);
                        return MISSION_FAILED;
                      }

                    if (value_is_slippery(key))
                        --(info->slippery_count);

                    the_verdict = size_t_aa_append(&(info->extension.overlaps),
                                                   base_position - 1);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        jumper_do_abort(the_jumper);
                        map_info_remove_reference(info, the_jumper);
                        update_value_cluster_for_map_info(map_value, NULL,
                                                          the_jumper);
                        return MISSION_FAILED;
                      }

                    --(info->item_count);

                    the_verdict = enter_into_string_index(
                            info->extension.overlap_index, &(buffer[0]),
                            (void *)(base_position - 1));

                    if (info->extension.overlaps.element_count > 1)
                        info->extension.overlaps_sorted = FALSE;
                  }
                else
                  {
                    verdict the_verdict;

                    if (target != NULL)
                      {
                        verdict the_verdict;

                        the_verdict = enter_replacement(info,
                                base_position - 1, target, the_jumper);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            value_remove_reference(base_target, the_jumper);
                            map_info_remove_reference(info, the_jumper);
                            update_value_cluster_for_map_info(map_value, NULL,
                                                              the_jumper);
                            return MISSION_FAILED;
                          }

                        assert(base_position > 0);

                        update_info_for_value_key_target(info, base_target,
                                target, key, &(map_value->validator),
                                the_jumper);
                        value_remove_reference(base_target, the_jumper);
                        map_info_remove_reference(info, the_jumper);
                        update_value_cluster_for_map_info(map_value, NULL,
                                                          the_jumper);
                        if (!(jumper_flowing_forward(the_jumper)))
                            return MISSION_FAILED;

                        return MISSION_ACCOMPLISHED;
                      }

                    updated_key = map_value_item_key_value(base_map,
                                                           base_position - 1);

                    if (base_target != NULL)
                      {
                        map_value->validator = validator_remove_validator(
                                map_value->validator,
                                value_validator(base_target));
                        if (map_value->validator == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            value_remove_reference(base_target, the_jumper);
                            map_info_remove_reference(info, the_jumper);
                            update_value_cluster_for_map_info(map_value, NULL,
                                                              the_jumper);
                            return MISSION_FAILED;
                          }

                        if (value_is_slippery(base_target))
                            --(info->slippery_count);

                        value_remove_reference(base_target, the_jumper);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            map_info_remove_reference(info, the_jumper);
                            update_value_cluster_for_map_info(map_value, NULL,
                                                              the_jumper);
                            return MISSION_FAILED;
                          }
                      }

                    if (value_is_slippery(key))
                        --(info->slippery_count);

                    the_verdict = size_t_aa_append(&(info->extension.overlaps),
                                                   base_position - 1);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        jumper_do_abort(the_jumper);
                        map_info_remove_reference(info, the_jumper);
                        update_value_cluster_for_map_info(map_value, NULL,
                                                          the_jumper);
                        return MISSION_FAILED;
                      }

                    --(info->item_count);

                    the_verdict = enter_into_string_index(
                            info->extension.overlap_index, &(buffer[0]),
                            (void *)(base_position - 1));

                    if (info->extension.overlaps.element_count > 1)
                        info->extension.overlaps_sorted = FALSE;
                  }
              }
          }
      }

    to_move = info->value_key_keys.element_count;

    the_verdict = map_info_append_value_key_item(info, updated_key, target);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        map_info_remove_reference(info, the_jumper);
        update_value_cluster_for_map_info(map_value, NULL, the_jumper);
        return MISSION_FAILED;
      }

    ++(info->item_count);

    map_value->validator = validator_add_validator(map_value->validator,
            value_validator(updated_key));
    if (map_value->validator == NULL)
      {
        jumper_do_abort(the_jumper);
        map_info_remove_reference(info, the_jumper);
        update_value_cluster_for_map_info(map_value, NULL, the_jumper);
        return MISSION_FAILED;
      }
    else
      {
        if (target != NULL)
          {
            map_value->validator = validator_add_validator(
                    map_value->validator, value_validator(target));
          }
        if ((target != NULL) && (map_value->validator == NULL))
          {
            jumper_do_abort(the_jumper);
            map_info_remove_reference(info, the_jumper);
            update_value_cluster_for_map_info(map_value, NULL, the_jumper);
            return MISSION_FAILED;
          }
        else
          {
            info->all_keys_validator = validator_add_validator(
                    info->all_keys_validator, value_validator(updated_key));
            if (info->all_keys_validator == NULL)
              {
                jumper_do_abort(the_jumper);
                map_info_remove_reference(info, the_jumper);
                update_value_cluster_for_map_info(map_value, NULL, the_jumper);
                return MISSION_FAILED;
              }
            assert(validator_is_valid(info->all_keys_validator));
                    /* VERIFIED */
          }
      }

    the_verdict = map_info_bubble_value_keys(info, position, to_move,
                                             updated_key, target);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        map_info_remove_reference(info, the_jumper);
        update_value_cluster_for_map_info(map_value, NULL, the_jumper);
        return the_verdict;
      }

    assert(validator_is_valid(info->all_keys_validator)); /* VERIFIED */
    map_info_remove_reference(info, the_jumper);
    update_value_cluster_for_map_info(map_value, NULL, the_jumper);
    return MISSION_ACCOMPLISHED;
  }

static value *create_empty_value(value_kind kind)
  {
    value *result;

    result = MALLOC_ONE_OBJECT(value);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->tagalong_lock, free(result); return NULL);

    INITIALIZE_SYSTEM_LOCK(result->lock,
            DESTROY_SYSTEM_LOCK(result->tagalong_lock);
            free(result);
            return NULL);

    result->kind = kind;
    result->reference_cluster = NULL;
    result->cluster_use_count = 0;
    result->tagalong_chain = NULL;
    result->validator = get_trivial_validator();
    assert(result->validator != NULL);
    result->reference_count = 1;
    result->no_cluster_reference_count = 0;
    result->destructing = FALSE;

    return result;
  }

static verdict initialize_lepton_components(value *the_value)
  {
    verdict the_verdict;
    string_index *new_index;

    assert(the_value != NULL);

    assert((the_value->kind == VK_SEMI_LABELED_VALUE_LIST) ||
           (the_value->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (the_value->kind == VK_LEPTON));

    the_verdict = mstring_aa_init(&the_value->u.lepton.labels, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_verdict = value_aa_init(&the_value->u.lepton.values, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(the_value->u.lepton.labels.array);
        return the_verdict;
      }

    new_index = create_string_index();
    if (new_index == NULL)
      {
        free(the_value->u.lepton.values.array);
        free(the_value->u.lepton.labels.array);
        return MISSION_FAILED;
      }

    INITIALIZE_SYSTEM_LOCK(the_value->u.lepton.lock,
            destroy_string_index(new_index);
            free(the_value->u.lepton.values.array);
            free(the_value->u.lepton.labels.array);
            return MISSION_FAILED);

    the_value->u.lepton.index = new_index;
    the_value->u.lepton.labeled_element_count = 0;
    the_value->u.lepton.ordered_labeled_elements = NULL;
    the_value->u.lepton.unlabeled_element_count = 0;
    the_value->u.lepton.ordered_unlabeled_elements = NULL;
    the_value->u.lepton.map_equivalent = NULL;
    the_value->u.lepton.slippery_count = 0;

    return MISSION_ACCOMPLISHED;
  }

static verdict initialize_map_components(value *the_value,
        size_t initial_value_key_space, size_t initial_type_key_space)
  {
    assert(the_value != NULL);

    assert(the_value->kind == VK_MAP);

    INITIALIZE_SYSTEM_LOCK(the_value->u.map.lock, return MISSION_FAILED);

    the_value->u.map.map_info =
            create_map_info(initial_value_key_space, initial_type_key_space);
    if (the_value->u.map.map_info == NULL)
      {
        DESTROY_SYSTEM_LOCK(the_value->u.map.lock);
        return MISSION_FAILED;
      }

    return MISSION_ACCOMPLISHED;
  }

static void deallocate_lepton_components(value *the_value, jumper *the_jumper)
  {
    size_t count;
    char **string_array;
    value **value_array;
    size_t index;

    assert(the_value != NULL);

    assert((the_value->kind == VK_SEMI_LABELED_VALUE_LIST) ||
           (the_value->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (the_value->kind == VK_LEPTON));

    count = the_value->u.lepton.labels.element_count;
    assert(count == the_value->u.lepton.values.element_count);

    string_array = the_value->u.lepton.labels.array;
    value_array = the_value->u.lepton.values.array;
    assert(string_array != NULL);
    assert(value_array != NULL);

    for (index = 0; index < count; ++index)
      {
        if (string_array[index] != NULL)
            free(string_array[index]);
        value_remove_reference_with_reference_cluster(value_array[index],
                the_jumper, the_value->reference_cluster);
      }

    free(string_array);
    free(value_array);

    destroy_string_index(the_value->u.lepton.index);

    DESTROY_SYSTEM_LOCK(the_value->u.lepton.lock);

    if (the_value->u.lepton.ordered_labeled_elements != NULL)
        free(the_value->u.lepton.ordered_labeled_elements);
    if (the_value->u.lepton.ordered_unlabeled_elements != NULL)
        free(the_value->u.lepton.ordered_unlabeled_elements);
    if (the_value->u.lepton.map_equivalent != NULL)
      {
        value_remove_reference_with_reference_cluster(
                the_value->u.lepton.map_equivalent, the_jumper,
                the_value->reference_cluster);
      }
  }

static verdict copy_lepton_components(value *target, value *source)
  {
    verdict the_verdict;
    string_index *new_index;
    size_t count;
    char **string_array;
    value **value_array;
    size_t index;
    size_t labeled_element_count;
    size_t unlabeled_element_count;
    value *map_equivalent;

    assert(target != NULL);
    assert(source != NULL);

    assert((target->kind == VK_SEMI_LABELED_VALUE_LIST) ||
           (target->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (target->kind == VK_LEPTON));
    assert(target->kind == source->kind);

    the_verdict = initialize_lepton_components(target);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    new_index = target->u.lepton.index;
    assert(new_index != NULL);

    count = source->u.lepton.labels.element_count;
    assert(count == source->u.lepton.values.element_count);

    string_array = source->u.lepton.labels.array;
    value_array = source->u.lepton.values.array;
    assert(string_array != NULL);
    assert(value_array != NULL);

    for (index = 0; index < count; ++index)
      {
        char *old_label;
        char *new_label;
        verdict the_verdict;
        value *field_value;

        old_label = string_array[index];
        if (old_label == NULL)
          {
            new_label = NULL;
          }
        else
          {
            new_label = MALLOC_ARRAY(char, strlen(old_label) + 1);
            if (new_label == NULL)
              {
                deallocate_lepton_components(target, NULL);
                return MISSION_FAILED;
              }

            strcpy(new_label, old_label);
          }

        the_verdict = mstring_aa_append(&(target->u.lepton.labels), new_label);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            if (new_label != NULL)
                free(new_label);
            deallocate_lepton_components(target, NULL);
            return MISSION_FAILED;
          }

        field_value = value_array[index];
        assert(field_value != NULL);

        the_verdict = value_aa_append(&target->u.lepton.values, field_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            assert(target->u.lepton.labels.element_count > 0);
            --(target->u.lepton.labels.element_count);
            if (new_label != NULL)
                free(new_label);
            deallocate_lepton_components(target, NULL);
            return MISSION_FAILED;
          }

        add_intervalue_reference(target, field_value);

        if (new_label != NULL)
          {
            verdict the_verdict;

            the_verdict =
                    enter_into_string_index(new_index, new_label, field_value);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                deallocate_lepton_components(target, NULL);
                return the_verdict;
              }
          }
      }

    GRAB_SYSTEM_LOCK(source->u.lepton.lock);

    labeled_element_count = source->u.lepton.labeled_element_count;
    unlabeled_element_count = source->u.lepton.unlabeled_element_count;

    if (labeled_element_count + unlabeled_element_count == count)
      {
        size_t_and_pointer *new_ordered_labeled_elements;
        size_t_and_pointer *new_ordered_unlabeled_elements;

        if (labeled_element_count == 0)
          {
            new_ordered_labeled_elements = NULL;
          }
        else
          {
            size_t element_num;
            size_t_and_pointer *old_ordered_labeled_elements;

            new_ordered_labeled_elements =
                    MALLOC_ARRAY(size_t_and_pointer, labeled_element_count);
            if (new_ordered_labeled_elements == NULL)
              {
                RELEASE_SYSTEM_LOCK(source->u.lepton.lock);
                deallocate_lepton_components(target, NULL);
                return MISSION_FAILED;
              }

            old_ordered_labeled_elements =
                    source->u.lepton.ordered_labeled_elements;
            assert(old_ordered_labeled_elements != NULL);

            for (element_num = 0; element_num < labeled_element_count;
                 ++element_num)
              {
                new_ordered_labeled_elements[element_num] =
                        old_ordered_labeled_elements[element_num];
              }
          }

        if (unlabeled_element_count == 0)
          {
            new_ordered_unlabeled_elements = NULL;
          }
        else
          {
            size_t element_num;
            size_t_and_pointer *old_ordered_unlabeled_elements;

            new_ordered_unlabeled_elements =
                    MALLOC_ARRAY(size_t_and_pointer, unlabeled_element_count);
            if (new_ordered_unlabeled_elements == NULL)
              {
                RELEASE_SYSTEM_LOCK(source->u.lepton.lock);
                if (new_ordered_labeled_elements != NULL)
                    free(new_ordered_labeled_elements);
                deallocate_lepton_components(target, NULL);
                return MISSION_FAILED;
              }

            old_ordered_unlabeled_elements =
                    source->u.lepton.ordered_unlabeled_elements;
            assert(old_ordered_unlabeled_elements != NULL);

            for (element_num = 0; element_num < unlabeled_element_count;
                 ++element_num)
              {
                new_ordered_unlabeled_elements[element_num] =
                        old_ordered_unlabeled_elements[element_num];
              }
          }

        target->u.lepton.labeled_element_count = labeled_element_count;
        target->u.lepton.ordered_labeled_elements =
                new_ordered_labeled_elements;
        target->u.lepton.unlabeled_element_count = unlabeled_element_count;
        target->u.lepton.ordered_unlabeled_elements =
                new_ordered_unlabeled_elements;
      }

    map_equivalent = source->u.lepton.map_equivalent;
    if (map_equivalent != NULL)
      {
        value_add_reference_with_reference_cluster(map_equivalent,
                                                   target->reference_cluster);
      }
    target->u.lepton.map_equivalent = map_equivalent;
    target->u.lepton.slippery_count = source->u.lepton.slippery_count;

    RELEASE_SYSTEM_LOCK(source->u.lepton.lock);

    return MISSION_ACCOMPLISHED;
  }

static boolean semi_labeled_value_lists_are_equal(value *value1, value *value2,
        boolean *doubt, const source_location *location, jumper *the_jumper)
  {
    size_t count;
    char **string_array1;
    char **string_array2;
    value **value_array1;
    value **value_array2;
    size_t index;

    assert(value1 != NULL);
    assert(value2 != NULL);
    assert(doubt != NULL);
    assert(the_jumper != NULL);

    assert(value1->kind == VK_SEMI_LABELED_VALUE_LIST);
    assert(value2->kind == VK_SEMI_LABELED_VALUE_LIST);

    assert(value_is_valid(value1)); /* VERIFIED */
    assert(value_is_valid(value2)); /* VERIFIED */

    *doubt = FALSE;

    count = value1->u.lepton.labels.element_count;
    assert(count == value1->u.lepton.values.element_count);

    if (count != value2->u.lepton.labels.element_count)
        return FALSE;

    assert(count == value2->u.lepton.values.element_count);

    string_array1 = value1->u.lepton.labels.array;
    string_array2 = value2->u.lepton.labels.array;
    value_array1 = value1->u.lepton.values.array;
    value_array2 = value2->u.lepton.values.array;
    assert(string_array1 != NULL);
    assert(string_array2 != NULL);
    assert(value_array1 != NULL);
    assert(value_array2 != NULL);

    for (index = 0; index < count; ++index)
      {
        if (string_array1[index] == NULL)
          {
            if (string_array2[index] != NULL)
                return FALSE;
          }
        else
          {
            if (string_array2[index] == NULL)
                return FALSE;
            if (strcmp(string_array1[index], string_array2[index]) != 0)
                return FALSE;
          }

        assert(value_is_valid(value_array1[index])); /* VERIFIED */
        assert(value_is_valid(value_array2[index])); /* VERIFIED */
        if (!values_are_equal(value_array1[index], value_array2[index], doubt,
                              location, the_jumper))
            return FALSE;
        if (!(jumper_flowing_forward(the_jumper)))
            return FALSE;
        if (*doubt)
            return FALSE;
      }

    return TRUE;
  }

static int semi_labeled_value_list_structural_order(value *left, value *right)
  {
    size_t left_count;
    size_t right_count;
    char **left_labels;
    char **right_labels;
    value **left_values;
    value **right_values;
    size_t index;

    assert(left != NULL);
    assert(right != NULL);

    assert(left->kind == VK_SEMI_LABELED_VALUE_LIST);
    assert(right->kind == VK_SEMI_LABELED_VALUE_LIST);

    assert(value_is_valid(left)); /* VERIFIED */
    assert(value_is_valid(right)); /* VERIFIED */

    left_count = left->u.lepton.labels.element_count;
    assert(left_count == left->u.lepton.values.element_count);

    right_count = right->u.lepton.labels.element_count;
    assert(right_count == right->u.lepton.values.element_count);

    left_labels = left->u.lepton.labels.array;
    right_labels = right->u.lepton.labels.array;
    left_values = left->u.lepton.values.array;
    right_values = right->u.lepton.values.array;
    assert(left_labels != NULL);
    assert(right_labels != NULL);
    assert(left_values != NULL);
    assert(right_values != NULL);

    index = 0;

    while (TRUE)
      {
        int value_order;

        if (index >= left_count)
          {
            if (left_count != right_count)
                return -1;
            return 0;
          }

        if (index >= right_count)
          {
            assert(left_count > right_count);
            return 1;
          }

        if (left_labels[index] == NULL)
          {
            if (right_labels[index] != NULL)
                return -1;
          }
        else
          {
            int name_order;

            if (right_labels[index] == NULL)
                return 1;

            name_order = utf8_string_lexicographical_order_by_code_point(
                    left_labels[index], right_labels[index]);
            if (name_order != 0)
                return name_order;
          }

        assert(value_is_valid(left_values[index])); /* VERIFIED */
        assert(value_is_valid(right_values[index])); /* VERIFIED */
        value_order = value_structural_order(left_values[index],
                                             right_values[index]);
        if (value_order != 0)
            return value_order;

        ++index;
      }
  }

static boolean semi_labeled_value_list_is_slippery(value *the_value)
  {
    assert(the_value != NULL);
    assert(the_value->kind == VK_SEMI_LABELED_VALUE_LIST);

    return (the_value->u.lepton.slippery_count > 0);
  }

static boolean semi_labeled_multi_sets_are_equal(value *value1, value *value2,
        boolean *doubt, const source_location *location, jumper *the_jumper)
  {
    size_t count;
    char **string_array1;
    char **string_array2;
    value **value_array1;
    value **value_array2;
    string_index *lookup;
    boolean *used;
    size_t index;

    assert(value1 != NULL);
    assert(value2 != NULL);
    assert(doubt != NULL);
    assert(the_jumper != NULL);

    assert((value1->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (value1->kind == VK_LEPTON));
    assert((value2->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (value2->kind == VK_LEPTON));

    assert(value_is_valid(value1)); /* VERIFIED */
    assert(value_is_valid(value2)); /* VERIFIED */

    *doubt = FALSE;

    count = value1->u.lepton.labels.element_count;
    assert(count == value1->u.lepton.values.element_count);

    if (count != value2->u.lepton.labels.element_count)
        return FALSE;

    assert(count == value2->u.lepton.values.element_count);

    string_array1 = value1->u.lepton.labels.array;
    string_array2 = value2->u.lepton.labels.array;
    value_array1 = value1->u.lepton.values.array;
    value_array2 = value2->u.lepton.values.array;
    assert(string_array1 != NULL);
    assert(string_array2 != NULL);
    assert(value_array1 != NULL);
    assert(value_array2 != NULL);

    lookup = value2->u.lepton.index;
    assert(lookup != NULL);

    if (count == 0)
        return TRUE;

    used = MALLOC_ARRAY(boolean, count);
    if (used == NULL)
      {
        jumper_do_abort(the_jumper);
        return FALSE;
      }

    for (index = 0; index < count; ++index)
        used[index] = FALSE;

    for (index = 0; index < count; ++index)
      {
        if (string_array1[index] == NULL)
          {
            size_t index2;

            for (index2 = 0; index2 < count; ++index2)
              {
                boolean local_equal;
                boolean local_doubt;

                if (string_array2[index2] != NULL)
                    continue;

                if (used[index2])
                    continue;

                assert(value_is_valid(value_array1[index])); /* VERIFIED */
                assert(value_is_valid(value_array2[index2])); /* VERIFIED */
                local_equal = values_are_equal(value_array1[index],
                        value_array2[index2], &local_doubt, location,
                        the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    free(used);
                    return FALSE;
                  }
                if (local_equal && (!local_doubt))
                  {
                    used[index2] = TRUE;
                    break;
                  }

                if (local_doubt)
                    *doubt = TRUE;
              }
            if (index2 >= count)
              {
                free(used);
                return FALSE;
              }
          }
        else
          {
            value *value2;
            boolean local_doubt;

            value2 = (value *)(lookup_in_string_index(lookup,
                                                      string_array1[index]));
            if (value2 == NULL)
              {
                free(used);
                return FALSE;
              }

            assert(value_is_valid(value_array1[index])); /* VERIFIED */
            assert(value_is_valid(value2)); /* VERIFIED */
            if (!(values_are_equal(value_array1[index], value2, &local_doubt,
                                   location, the_jumper)))
              {
                if (local_doubt)
                    *doubt = TRUE;
                free(used);
                return FALSE;
              }

            if (local_doubt)
                *doubt = TRUE;

            if (local_doubt || (!(jumper_flowing_forward(the_jumper))))
              {
                free(used);
                return FALSE;
              }
          }
      }

    for (index = 0; index < count; ++index)
      {
        if (string_array2[index] == NULL)
            assert(used[index]);
        else
            assert(!(used[index]));
      }

    free(used);

    *doubt = FALSE;

    return TRUE;
  }

static int semi_labeled_multi_set_structural_order(value *left, value *right)
  {
    verdict the_verdict;
    size_t left_labeled_count;
    size_t right_labeled_count;
    size_t_and_pointer *left_ordered_labeled_elements;
    size_t_and_pointer *right_ordered_labeled_elements;
    size_t element_num;
    size_t left_unlabeled_count;
    size_t right_unlabeled_count;
    size_t_and_pointer *left_ordered_unlabeled_elements;
    size_t_and_pointer *right_ordered_unlabeled_elements;

    assert(left != NULL);
    assert(right != NULL);

    assert((left->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (left->kind == VK_LEPTON));
    assert((right->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (right->kind == VK_LEPTON));

    assert(value_is_valid(left)); /* VERIFIED */
    assert(value_is_valid(right)); /* VERIFIED */

    the_verdict = generate_lepton_field_order_information(left);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return -2;

    the_verdict = generate_lepton_field_order_information(right);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return -2;

    left_labeled_count = left->u.lepton.labeled_element_count;
    right_labeled_count = right->u.lepton.labeled_element_count;

    left_ordered_labeled_elements = left->u.lepton.ordered_labeled_elements;
    right_ordered_labeled_elements = right->u.lepton.ordered_labeled_elements;

    element_num = 0;

    while (TRUE)
      {
        size_t left_element_num;
        size_t right_element_num;
        int label_order;
        int value_order;

        if (element_num >= left_labeled_count)
          {
            if (left_labeled_count != right_labeled_count)
                return -1;
            break;
          }

        if (element_num >= right_labeled_count)
            return 1;

        assert(left_ordered_labeled_elements != NULL);
        left_element_num =
                left_ordered_labeled_elements[element_num].the_size_t;
        assert(left_element_num < left->u.lepton.labels.element_count);

        assert(right_ordered_labeled_elements != NULL);
        right_element_num =
                right_ordered_labeled_elements[element_num].the_size_t;
        assert(right_element_num < right->u.lepton.labels.element_count);

        label_order = utf8_string_lexicographical_order_by_code_point(
                left->u.lepton.labels.array[left_element_num],
                right->u.lepton.labels.array[right_element_num]);
        if (label_order != 0)
            return label_order;

        assert(value_is_valid(left->u.lepton.values.array[left_element_num]));
                /* VERIFIED */
        assert(value_is_valid(
                       right->u.lepton.values.array[right_element_num]));
                /* VERIFIED */
        value_order = value_structural_order(
                left->u.lepton.values.array[left_element_num],
                right->u.lepton.values.array[right_element_num]);
        if (value_order != 0)
            return value_order;

        ++element_num;
      }

    left_unlabeled_count = left->u.lepton.unlabeled_element_count;
    right_unlabeled_count = right->u.lepton.unlabeled_element_count;

    left_ordered_unlabeled_elements =
            left->u.lepton.ordered_unlabeled_elements;
    right_ordered_unlabeled_elements =
            right->u.lepton.ordered_unlabeled_elements;

    element_num = 0;

    while (TRUE)
      {
        size_t left_element_num;
        size_t right_element_num;
        int value_order;

        if (element_num >= left_unlabeled_count)
          {
            if (left_unlabeled_count != right_unlabeled_count)
                return -1;
            return 0;
          }

        if (element_num >= right_unlabeled_count)
            return 1;

        assert(left_ordered_unlabeled_elements != NULL);
        left_element_num =
                left_ordered_unlabeled_elements[element_num].the_size_t;
        assert(left_element_num < left->u.lepton.labels.element_count);

        assert(right_ordered_unlabeled_elements != NULL);
        right_element_num =
                right_ordered_unlabeled_elements[element_num].the_size_t;
        assert(right_element_num < right->u.lepton.labels.element_count);

        assert(left->u.lepton.labels.array[left_element_num] == NULL);
        assert(right->u.lepton.labels.array[right_element_num] == NULL);

        assert(value_is_valid(left->u.lepton.values.array[left_element_num]));
                /* VERIFIED */
        assert(value_is_valid(
                       right->u.lepton.values.array[right_element_num]));
                /* VERIFIED */
        value_order = value_structural_order(
                left->u.lepton.values.array[left_element_num],
                right->u.lepton.values.array[right_element_num]);
        if (value_order != 0)
            return value_order;

        ++element_num;
      }
  }

static boolean semi_labeled_multi_set_is_slippery(value *the_value)
  {
    assert(the_value != NULL);
    assert((the_value->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (the_value->kind == VK_LEPTON));

    return (the_value->u.lepton.slippery_count > 0);
  }

static boolean maps_are_equal(value *value1, value *value2, boolean *doubt,
        const source_location *location, jumper *the_jumper)
  {
    boolean local_doubt;
    boolean local_match;
    size_t filter_count1;
    size_t *filters1;
    size_t filter_count2;
    size_t *filters2;
    type *all_keys1;
    type **precise1;
    type *all_keys2;
    type **precise2;
    size_t filter_num1;
    boolean footprint_doubt;
    boolean footprints_equal;

    assert(value1 != NULL);
    assert(value2 != NULL);
    assert(doubt != NULL);
    assert(the_jumper != NULL);

    assert(value1->kind == VK_MAP);
    assert(value2->kind == VK_MAP);

    assert(value_is_valid(value1)); /* VERIFIED */
    assert(value_is_valid(value2)); /* VERIFIED */

    *doubt = FALSE;

    assert(value_is_valid(value1)); /* VERIFIED */
    assert(value_is_valid(value2)); /* VERIFIED */
    local_match = map_all_value_keys_match_second_map(value1, value2,
            &local_doubt, location, the_jumper);
    if (local_doubt)
        *doubt = TRUE;
    else if (!local_match)
        return FALSE;

    assert(value_is_valid(value2)); /* VERIFIED */
    assert(value_is_valid(value1)); /* VERIFIED */
    local_match = map_all_value_keys_match_second_map(value2, value1,
            &local_doubt, location, the_jumper);
    if (local_doubt)
        *doubt = TRUE;
    else if (!local_match)
        return FALSE;

    filters1 = map_filter_items(value1, &filter_count1, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return FALSE;

    filters2 = map_filter_items(value2, &filter_count2, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (filter_count1 > 0)
            free(filters1);
        return FALSE;
      }

    if ((filter_count1 == 0) && (filter_count2 == 0))
      {
        if (filter_count1 > 0)
            free(filters1);
        if (filter_count2 > 0)
            free(filters2);
        return TRUE;
      }

    if (filter_count1 == filter_count2)
      {
        size_t filter_num;

        filter_num = 0;
        while (TRUE)
          {
            type *filter1;
            type *filter2;
            boolean equal_doubt;
            boolean is_equal;
            value *target1;
            value *target2;

            if (filter_num >= filter_count1)
              {
                if (filter_count1 > 0)
                    free(filters1);
                if (filter_count2 > 0)
                    free(filters2);
                return TRUE;
              }

            assert(value_is_valid(value1)); /* VERIFIED */
            assert(value_is_valid(value2)); /* VERIFIED */
            filter1 = map_value_item_key_type(value1, filters1[filter_num]);
            filter2 = map_value_item_key_type(value2, filters2[filter_num]);
            assert(type_is_valid(filter1)); /* VERIFIED */
            assert(type_is_valid(filter2)); /* VERIFIED */
            is_equal = types_are_equal(filter1, filter2, &equal_doubt,
                                       location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                if (filter_count1 > 0)
                    free(filters1);
                if (filter_count2 > 0)
                    free(filters2);
                return FALSE;
              }

            if (equal_doubt || !is_equal)
                break;

            assert(value_is_valid(value1)); /* VERIFIED */
            assert(value_is_valid(value2)); /* VERIFIED */
            target1 = map_value_item_target(value1, filters1[filter_num]);
            target2 = map_value_item_target(value2, filters2[filter_num]);
            assert(value_is_valid(target1)); /* VERIFIED */
            assert(value_is_valid(target2)); /* VERIFIED */
            is_equal = values_are_equal(target1, target2, &equal_doubt,
                                        location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                if (filter_count1 > 0)
                    free(filters1);
                if (filter_count2 > 0)
                    free(filters2);
                return FALSE;
              }

            if (equal_doubt || !is_equal)
                break;
          }
      }

    precise1 = map_precise_key_types(value1, filter_count1, filters1,
                                     &all_keys1, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (filter_count1 > 0)
            free(filters1);
        if (filter_count2 > 0)
            free(filters2);
        return FALSE;
      }

    precise2 = map_precise_key_types(value2, filter_count2, filters2,
                                     &all_keys2, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        size_t filter_num;

        type_remove_reference(all_keys1, the_jumper);
        for (filter_num = 0; filter_num < filter_count1; ++filter_num)
            type_remove_reference(precise1[filter_num], the_jumper);
        if (filter_count1 > 0)
            free(precise1);
        if (filter_count1 > 0)
            free(filters1);
        if (filter_count2 > 0)
            free(filters2);
        return FALSE;
      }

    for (filter_num1 = 0; filter_num1 < filter_count1; ++filter_num1)
      {
        size_t filter_num2;

        for (filter_num2 = 0; filter_num2 < filter_count2; ++filter_num2)
          {
            boolean empty_doubt;
            boolean empty;
            value *target1;
            value *target2;
            boolean equal_doubt;
            boolean equal;

            assert(type_is_valid(precise1[filter_num1])); /* VERIFIED */
            assert(type_is_valid(precise2[filter_num2])); /* VERIFIED */
            empty = intersection_empty(precise1[filter_num1],
                    precise2[filter_num2], &empty_doubt, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                size_t filter_num;

                type_remove_reference(all_keys1, the_jumper);
                for (filter_num = 0; filter_num < filter_count1; ++filter_num)
                    type_remove_reference(precise1[filter_num], the_jumper);
                free(precise1);
                type_remove_reference(all_keys2, the_jumper);
                for (filter_num = 0; filter_num < filter_count2; ++filter_num)
                    type_remove_reference(precise2[filter_num], the_jumper);
                free(precise2);
                if (filter_count1 > 0)
                    free(filters1);
                if (filter_count2 > 0)
                    free(filters2);
                return FALSE;
              }

            if (empty && !empty_doubt)
                continue;

            assert(value_is_valid(value1)); /* VERIFIED */
            target1 = map_value_item_target(value1, filters1[filter_num1]);
            assert(target1 != NULL);
            assert(value_is_valid(target1)); /* VERIFIED */

            assert(value_is_valid(value2)); /* VERIFIED */
            target2 = map_value_item_target(value2, filters2[filter_num2]);
            assert(target2 != NULL);
            assert(value_is_valid(target2)); /* VERIFIED */

            assert(value_is_valid(target1)); /* VERIFIED */
            assert(value_is_valid(target2)); /* VERIFIED */
            equal = values_are_equal(target1, target2, &equal_doubt, location,
                                     the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                size_t filter_num;

                type_remove_reference(all_keys1, the_jumper);
                for (filter_num = 0; filter_num < filter_count1; ++filter_num)
                    type_remove_reference(precise1[filter_num], the_jumper);
                free(precise1);
                type_remove_reference(all_keys2, the_jumper);
                for (filter_num = 0; filter_num < filter_count2; ++filter_num)
                    type_remove_reference(precise2[filter_num], the_jumper);
                free(precise2);
                if (filter_count1 > 0)
                    free(filters1);
                if (filter_count2 > 0)
                    free(filters2);
                return FALSE;
              }

            if (!equal_doubt && equal)
                continue;

            if (!empty_doubt && !equal_doubt)
              {
                size_t filter_num;

                type_remove_reference(all_keys1, the_jumper);
                for (filter_num = 0; filter_num < filter_count1; ++filter_num)
                    type_remove_reference(precise1[filter_num], the_jumper);
                free(precise1);
                type_remove_reference(all_keys2, the_jumper);
                for (filter_num = 0; filter_num < filter_count2; ++filter_num)
                    type_remove_reference(precise2[filter_num], the_jumper);
                free(precise2);
                *doubt = FALSE;
                if (filter_count1 > 0)
                    free(filters1);
                if (filter_count2 > 0)
                    free(filters2);
                return FALSE;
              }

            *doubt = TRUE;
          }
      }

    for (filter_num1 = 0; filter_num1 < filter_count1; ++filter_num1)
        type_remove_reference(precise1[filter_num1], the_jumper);
    if (filter_count1 > 0)
        free(precise1);
    for (filter_num1 = 0; filter_num1 < filter_count2; ++filter_num1)
        type_remove_reference(precise2[filter_num1], the_jumper);
    if (filter_count2 > 0)
        free(precise2);
    if (filter_count1 > 0)
        free(filters1);
    if (filter_count2 > 0)
        free(filters2);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(all_keys1, the_jumper);
        type_remove_reference(all_keys2, the_jumper);
        return FALSE;
      }

    assert(type_is_valid(all_keys1)); /* VERIFIED */
    assert(type_is_valid(all_keys2)); /* VERIFIED */
    footprints_equal = types_are_equal(all_keys1, all_keys2, &footprint_doubt,
                                       location, the_jumper);
    type_remove_reference(all_keys1, the_jumper);
    type_remove_reference(all_keys2, the_jumper);

    if (footprint_doubt)
      {
        *doubt = TRUE;
        return FALSE;
      }
    else if (!footprints_equal)
      {
        *doubt = FALSE;
        return FALSE;
      }
    else
      {
        return TRUE;
      }
  }

static boolean map_all_value_keys_match_second_map(value *to_test,
        value *other, boolean *doubt, const source_location *location,
        jumper *the_jumper)
  {
    size_t count;
    size_t number;

    assert(to_test != NULL);
    assert(other != NULL);
    assert(doubt != NULL);
    assert(the_jumper != NULL);

    assert(to_test->kind == VK_MAP);
    assert(other->kind == VK_MAP);

    assert(value_is_valid(to_test)); /* VERIFIED */
    assert(value_is_valid(other)); /* VERIFIED */

    *doubt = FALSE;

    count = map_value_item_count(to_test);

    for (number = 0; number < count; ++number)
      {
        value *this_target;
        value *key;
        boolean local_doubt;
        value *other_target;

        if (map_value_item_is_type(to_test, number))
            continue;

        key = map_value_item_key_value(to_test, number);
        assert(key != NULL);
        assert(value_is_valid(key)); /* VERIFIED */

        assert(to_test->kind == VK_MAP);
        assert(value_is_valid_except_map_targets(to_test)); /* VERIFIED */
        assert(value_is_valid(key)); /* VERIFIED */
        this_target = map_value_lookup(to_test, key, &local_doubt, location,
                                       the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return FALSE;
        if (local_doubt)
          {
            *doubt = TRUE;
            continue;
          }

        assert(other->kind == VK_MAP);
        assert(value_is_valid_except_map_targets(other)); /* VERIFIED */
        assert(value_is_valid(key)); /* VERIFIED */
        other_target = map_value_lookup(other, key, &local_doubt, location,
                                        the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return FALSE;
        if (local_doubt)
          {
            *doubt = TRUE;
            continue;
          }

        if (this_target == NULL)
          {
            if (other_target != NULL)
              {
                *doubt = FALSE;
                return FALSE;
              }
          }
        else
          {
            boolean equal_targets;

            if (other_target == NULL)
              {
                *doubt = FALSE;
                return FALSE;
              }

            assert(value_is_valid(this_target)); /* VERIFIED */
            assert(value_is_valid(other_target)); /* VERIFIED */
            equal_targets = values_are_equal(this_target, other_target,
                    &local_doubt, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return FALSE;
            if (local_doubt)
              {
                *doubt = TRUE;
                continue;
              }
            if (!equal_targets)
              {
                *doubt = FALSE;
                return FALSE;
              }
          }
      }

    return TRUE;
  }

static size_t *map_filter_items(value *map_value, size_t *filter_count,
                                jumper *the_jumper)
  {
    size_t item_count;
    size_t local_filter_count;
    size_t item_num;
    size_t *result;
    size_t result_num;

    assert(map_value != NULL);
    assert(filter_count != NULL);
    assert(the_jumper != NULL);

    assert(map_value->kind == VK_MAP);

    assert(value_is_valid(map_value)); /* VERIFIED */

    item_count = map_value_item_count(map_value);

    local_filter_count = 0;

    for (item_num = 0; item_num < item_count; ++item_num)
      {
        if (map_value_item_is_type(map_value, item_num))
            ++local_filter_count;
      }

    *filter_count = local_filter_count;

    if (local_filter_count == 0)
        return NULL;

    result = MALLOC_ARRAY(size_t, local_filter_count);
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result_num = local_filter_count;

    for (item_num = item_count; item_num > 0; --item_num)
      {
        if (map_value_item_is_type(map_value, item_num - 1))
          {
            assert(result_num > 0);
            --result_num;
            assert(result_num >= 0);
            result[result_num] = item_num - 1;
          }
      }

    return result;
  }

static type **map_precise_key_types(value *map_value, size_t filter_count,
        size_t *filters, type **all_keys_type, const source_location *location,
        jumper *the_jumper)
  {
    size_t item_count;
    size_t item_num;
    type **result;
    size_t result_num;
    size_t last_type_position;
    size_t max_gap;
    size_t final_gap;
    value **gap_keys;
    type *shadow;

    assert(map_value != NULL);
    assert((filter_count == 0) || (filters != NULL));
    assert(all_keys_type != NULL);
    assert(the_jumper != NULL);

    assert(map_value->kind == VK_MAP);

    assert(value_is_valid(map_value)); /* VERIFIED */

    if (filter_count == 0)
        return NULL;

    item_count = map_value_item_count(map_value);

    result = MALLOC_ARRAY(type *, filter_count);
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result_num = filter_count;
    last_type_position = filter_count;
    max_gap = 0;

    for (item_num = item_count; item_num > 0; --item_num)
      {
        if (map_value_item_is_type(map_value, item_num - 1))
          {
            size_t new_gap;

            assert(result_num > 0);
            --result_num;
            assert(result_num >= 0);
            new_gap = last_type_position - item_num;
            if (new_gap > max_gap)
                max_gap = new_gap;
            last_type_position = item_num - 1;
          }
      }

    final_gap = last_type_position;
    if (final_gap > max_gap)
        max_gap = final_gap;

    gap_keys = MALLOC_ARRAY(value *, ((max_gap > 0) ? max_gap : 1));
    if (gap_keys == NULL)
      {
        jumper_do_abort(the_jumper);
        free(result);
        return NULL;
      }

    shadow = get_anything_type();
    if (shadow == NULL)
      {
        jumper_do_abort(the_jumper);
        free(gap_keys);
        free(result);
        return NULL;
      }

    type_add_reference(shadow);
    assert(type_is_valid(shadow)); /* VERIFIED */

    last_type_position = filter_count;

    for (result_num = filter_count; result_num > 0;)
      {
        size_t current_position;
        type *base_local;
        type *not_shadow;
        type *new_shadow;

        --result_num;

        current_position = filters[result_num];

        assert(last_type_position > current_position);
        if (last_type_position > (current_position + 1))
          {
            size_t gap_count;
            size_t gap_num;
            type *enum_type;
            type *new_shadow;

            gap_count = (last_type_position - (current_position + 1));
            assert(gap_count > 0);
            assert(gap_count <= max_gap);

            for (gap_num = 0; gap_num < gap_count; ++gap_num)
              {
                size_t item_num;

                item_num = current_position + 1 + gap_num;
                assert(!(map_value_item_is_type(map_value, item_num)));
                assert(value_is_valid(map_value)); /* VERIFIED */
                gap_keys[gap_num] =
                        map_value_item_key_value(map_value, item_num);
                assert(value_is_valid(gap_keys[gap_num])); /* VERIFIED */
              }

            enum_type = get_enumeration_type(gap_count, gap_keys);
            if (enum_type == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(shadow, the_jumper);
                ++result_num;
                while (result_num < filter_count)
                  {
                    type_remove_reference(result[result_num], the_jumper);
                    ++result_num;
                  }
                free(result);
                return NULL;
              }

            assert(type_is_valid(enum_type)); /* VERIFIED */

            assert(type_is_valid(shadow)); /* VERIFIED */
            assert(type_is_valid(enum_type)); /* VERIFIED */
            new_shadow = get_union_type(shadow, enum_type);
            if (new_shadow == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(enum_type, the_jumper);
                type_remove_reference(shadow, the_jumper);
                free(gap_keys);
                ++result_num;
                while (result_num < filter_count)
                  {
                    type_remove_reference(result[result_num], the_jumper);
                    ++result_num;
                  }
                free(result);
                return NULL;
              }

            assert(type_is_valid(new_shadow)); /* VERIFIED */

            type_remove_reference(enum_type, the_jumper);
            type_remove_reference(shadow, the_jumper);
            shadow = new_shadow;
            if (!(jumper_flowing_forward(the_jumper)))
              {
                type_remove_reference(shadow, the_jumper);
                free(gap_keys);
                ++result_num;
                while (result_num < filter_count)
                  {
                    type_remove_reference(result[result_num], the_jumper);
                    ++result_num;
                  }
                free(result);
                return NULL;
              }
            assert(type_is_valid(shadow)); /* VERIFIED */
          }

        last_type_position = current_position;

        assert(value_is_valid(map_value)); /* VERIFIED */
        base_local = map_value_item_key_type(map_value, current_position);
        assert(base_local != NULL);
        assert(type_is_valid(base_local)); /* VERIFIED */

        assert(type_is_valid(shadow)); /* VERIFIED */
        not_shadow = get_not_type(shadow);
        if (not_shadow == NULL)
          {
            jumper_do_abort(the_jumper);
            type_remove_reference(shadow, the_jumper);
            free(gap_keys);
            ++result_num;
            while (result_num < filter_count)
              {
                type_remove_reference(result[result_num], the_jumper);
                ++result_num;
              }
            free(result);
            return NULL;
          }

        assert(type_is_valid(not_shadow)); /* VERIFIED */

        assert(type_is_valid(base_local)); /* VERIFIED */
        assert(type_is_valid(not_shadow)); /* VERIFIED */
        result[result_num] = get_intersection_type(base_local, not_shadow);
        if (result[result_num] == NULL)
          {
            jumper_do_abort(the_jumper);
            type_remove_reference(not_shadow, the_jumper);
            type_remove_reference(shadow, the_jumper);
            free(gap_keys);
            ++result_num;
            while (result_num < filter_count)
              {
                type_remove_reference(result[result_num], the_jumper);
                ++result_num;
              }
            free(result);
            return NULL;
          }

        assert(type_is_valid(result[result_num])); /* VERIFIED */

        type_remove_reference(not_shadow, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            jumper_do_abort(the_jumper);
            type_remove_reference(shadow, the_jumper);
            free(gap_keys);
            while (result_num < filter_count)
              {
                type_remove_reference(result[result_num], the_jumper);
                ++result_num;
              }
            free(result);
            return NULL;
          }

        assert(type_is_valid(shadow)); /* VERIFIED */
        assert(type_is_valid(base_local)); /* VERIFIED */
        new_shadow = get_union_type(shadow, base_local);
        if (new_shadow == NULL)
          {
            jumper_do_abort(the_jumper);
            type_remove_reference(shadow, the_jumper);
            free(gap_keys);
            while (result_num < filter_count)
              {
                type_remove_reference(result[result_num], the_jumper);
                ++result_num;
              }
            free(result);
            return NULL;
          }

        assert(type_is_valid(new_shadow)); /* VERIFIED */

        type_remove_reference(shadow, the_jumper);
        shadow = new_shadow;
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(shadow, the_jumper);
            free(gap_keys);
            while (result_num < filter_count)
              {
                type_remove_reference(result[result_num], the_jumper);
                ++result_num;
              }
            free(result);
            return NULL;
          }
        assert(type_is_valid(shadow)); /* VERIFIED */
      }

    assert(last_type_position >= 0);
    if (last_type_position > 0)
      {
        size_t gap_count;
        size_t gap_num;
        type *enum_type;
        type *new_shadow;

        gap_count = last_type_position;
        assert(gap_count > 0);
        assert(gap_count <= max_gap);

        for (gap_num = 0; gap_num < gap_count; ++gap_num)
          {
            assert(!(map_value_item_is_type(map_value, gap_num)));
            assert(value_is_valid(map_value)); /* VERIFIED */
            gap_keys[gap_num] = map_value_item_key_value(map_value, gap_num);
            assert(value_is_valid(gap_keys[gap_num])); /* VERIFIED */
          }

        enum_type = get_enumeration_type(gap_count, gap_keys);
        if (enum_type == NULL)
          {
            size_t result_num;

            jumper_do_abort(the_jumper);
            type_remove_reference(shadow, the_jumper);
            for (result_num = 0; result_num < filter_count; ++result_num)
                type_remove_reference(result[result_num], the_jumper);
            free(result);
            return NULL;
          }

        assert(type_is_valid(enum_type)); /* VERIFIED */

        assert(type_is_valid(shadow)); /* VERIFIED */
        assert(type_is_valid(enum_type)); /* VERIFIED */
        new_shadow = get_union_type(shadow, enum_type);
        if (new_shadow == NULL)
          {
            size_t result_num;

            jumper_do_abort(the_jumper);
            type_remove_reference(enum_type, the_jumper);
            type_remove_reference(shadow, the_jumper);
            free(gap_keys);
            for (result_num = 0; result_num < filter_count; ++result_num)
                type_remove_reference(result[result_num], the_jumper);
            free(result);
            return NULL;
          }

        assert(type_is_valid(new_shadow)); /* VERIFIED */

        type_remove_reference(enum_type, the_jumper);
        type_remove_reference(shadow, the_jumper);
        shadow = new_shadow;
        if (!(jumper_flowing_forward(the_jumper)))
          {
            size_t result_num;

            type_remove_reference(shadow, the_jumper);
            free(gap_keys);
            for (result_num = 0; result_num < filter_count; ++result_num)
                type_remove_reference(result[result_num], the_jumper);
            free(result);
            return NULL;
          }
        assert(type_is_valid(shadow)); /* VERIFIED */
      }

    *all_keys_type = shadow;
    assert(type_is_valid(*all_keys_type)); /* VERIFIED */

    free(gap_keys);

    return result;
  }

static int map_structural_order(value *left, value *right)
  {
    size_t left_count;
    size_t right_count;
    size_t left_pair_num;
    size_t right_pair_num;

    assert(left != NULL);
    assert(right != NULL);

    assert(left->kind == VK_MAP);
    assert(right->kind == VK_MAP);

    assert(value_is_valid(left)); /* VERIFIED */
    assert(value_is_valid(right)); /* VERIFIED */

    left_count = map_value_item_count(left);
    right_count = map_value_item_count(right);

    left_pair_num = 0;
    right_pair_num = 0;

    while (TRUE)
      {
        value *left_target;
        value *right_target;
        int local_result;

        if (left_pair_num < left_count)
          {
            left_target = map_value_item_target(left, left_pair_num);
            if (left_target == NULL)
              {
                ++left_pair_num;
                continue;
              }
          }
        else
          {
            left_target = NULL;
          }

        if (right_pair_num < right_count)
          {
            right_target = map_value_item_target(right, right_pair_num);
            if (right_target == NULL)
              {
                ++right_pair_num;
                continue;
              }
          }
        else
          {
            right_target = NULL;
          }

        if (left_target == NULL)
          {
            if (right_target != NULL)
                return -1;
            else
                return 0;
          }

        if (right_target == NULL)
            return 1;

        if (map_value_item_is_type(left, left_pair_num))
          {
            type *left_key_type;
            type *right_key_type;

            if (!(map_value_item_is_type(right, right_pair_num)))
                return 1;

            left_key_type = map_value_item_key_type(left, left_pair_num);
            right_key_type = map_value_item_key_type(right, right_pair_num);
            assert(type_is_valid(left_key_type)); /* VERIFIED */
            assert(type_is_valid(right_key_type)); /* VERIFIED */
            local_result =
                    type_structural_order(left_key_type, right_key_type);
          }
        else
          {
            value *left_key;
            value *right_key;

            if (map_value_item_is_type(right, right_pair_num))
                return -1;

            left_key = map_value_item_key_value(left, left_pair_num);
            right_key = map_value_item_key_value(right, right_pair_num);
            assert(value_is_valid(left_key)); /* VERIFIED */
            assert(value_is_valid(right_key)); /* VERIFIED */
            local_result = value_structural_order(left_key, right_key);
          }

        if (local_result != 0)
            return local_result;

        assert(value_is_valid(left_target)); /* VERIFIED */
        assert(value_is_valid(right_target)); /* VERIFIED */
        local_result = value_structural_order(left_target, right_target);
        if (local_result != 0)
            return local_result;

        ++left_pair_num;
        ++right_pair_num;
      }
  }

static boolean map_is_slippery(value *the_value)
  {
    boolean result;

    assert(the_value != NULL);
    assert(the_value->kind == VK_MAP);

    GRAB_SYSTEM_LOCK(the_value->u.map.lock);
    result = (the_value->u.map.map_info->slippery_count > 0);
    RELEASE_SYSTEM_LOCK(the_value->u.map.lock);
    return result;
  }

static boolean map_and_semi_labeled_value_list_are_equal(value *map_value,
        value *value2, boolean *doubt, const source_location *location,
        jumper *the_jumper)
  {
    value *map_value2;
    boolean result;

    assert(map_value != NULL);
    assert(value2 != NULL);
    assert(doubt != NULL);
    assert(the_jumper != NULL);

    assert(map_value->kind == VK_MAP);
    assert(value2->kind == VK_SEMI_LABELED_VALUE_LIST);

    assert(value_is_valid(map_value)); /* VERIFIED */
    assert(value_is_valid(value2)); /* VERIFIED */

    if (!(semi_labeled_value_list_value_has_no_labels(value2)))
      {
        *doubt = FALSE;
        return FALSE;
      }

    map_value2 = map_value_from_semi_labeled_value_list(value2);
    if (map_value2 == NULL)
      {
        jumper_do_abort(the_jumper);
        *doubt = TRUE;
        return FALSE;
      }

    assert(value_is_valid(map_value)); /* VERIFIED */
    assert(value_is_valid(map_value2)); /* VERIFIED */
    result =
            maps_are_equal(map_value, map_value2, doubt, location, the_jumper);

    value_remove_reference(map_value2, the_jumper);

    return result;
  }

static size_t map_info_local_index_for_key(map_info *info, value *key,
        boolean *found, boolean *doubt,
        boolean care_about_position_if_not_found,
        const source_location *location, jumper *the_jumper)
  {
    boolean error;
    boolean match_possible;
    size_t result;

    assert(info != NULL);
    assert(key != NULL);
    assert(found != NULL);
    assert(doubt != NULL);
    assert(the_jumper != NULL);

    assert(validator_is_valid(info->all_keys_validator)); /* VERIFIED */
    assert(value_is_valid(key)); /* VERIFIED */

    result = map_info_find_index_for_key(info, key, found, doubt, &error,
            &match_possible, care_about_position_if_not_found);
    if (error)
      {
        jumper_do_abort(the_jumper);
        return result;
      }
    if (*found)
        return result;

    if (match_possible &&
        ((info->slippery_key_count > 0) || value_is_slippery(key)))
      {
        size_t count;
        size_t number;

        count = info->value_key_keys.element_count;

        for (number = 0; number < count; ++number)
          {
            assert(value_is_valid(key)); /* VERIFIED */
            assert(value_is_valid(info->value_key_keys.array[number]));
                    /* VERIFIED */
            if (values_are_equal(key, info->value_key_keys.array[number],
                                 doubt, location, the_jumper))
              {
                *found = TRUE;
                return (number + 1);
              }
            if ((*doubt) || (!(jumper_flowing_forward(the_jumper))))
              {
                *found = FALSE;
                return result;
              }
          }
      }

    return result;
  }

static size_t map_value_global_index_for_key(value *map_value, value *key,
        boolean *found, boolean *doubt,
        boolean care_about_position_if_not_found,
        const source_location *location, jumper *the_jumper)
  {
    map_info *info;
    size_t local_result;
    size_t final_result;

    assert(map_value != NULL);
    assert(key != NULL);
    assert(found != NULL);
    assert(doubt != NULL);
    assert(the_jumper != NULL);

    assert(map_value->kind == VK_MAP);

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);
    info = map_value->u.map.map_info;
    map_info_add_reference(info);
    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    assert(validator_is_valid(info->all_keys_validator)); /* VERIFIED */
    assert(value_is_valid(key)); /* VERIFIED */

    local_result = map_info_local_index_for_key(info, key, found, doubt,
            care_about_position_if_not_found, location, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) || (*doubt))
      {
        map_info_remove_reference(info, the_jumper);
        return local_result;
      }

    if (!(info->is_extension))
      {
        size_t final_result;

        final_result = local_result + info->type_key_keys.element_count;
        map_info_remove_reference(info, the_jumper);
        return final_result;
      }

    if (*found)
      {
        size_t final_result;

        final_result =
                local_result + info->type_key_keys.element_count +
                (map_value_item_count(info->extension.base) -
                 info->extension.overlaps.element_count);
        map_info_remove_reference(info, the_jumper);
        return final_result;
      }

    final_result = map_value_global_index_for_key(info->extension.base, key,
            found, doubt, care_about_position_if_not_found, location,
            the_jumper);
    map_info_remove_reference(info, the_jumper);
    return final_result;
  }

static size_t map_info_find_index_for_key(map_info *info, value *key,
        boolean *found, boolean *doubt, boolean *error,
        boolean *match_possible, boolean care_about_position_if_not_possible)
  {
    size_t count;
    size_t lower;
    size_t upper;

    assert(info != NULL);
    assert(key != NULL);
    assert(found != NULL);
    assert(doubt != NULL);
    assert(error != NULL);
    assert(match_possible != NULL);

    assert(validator_is_valid(info->all_keys_validator)); /* VERIFIED */
    assert(value_is_valid(key)); /* VERIFIED */

    *doubt = FALSE;
    *error = FALSE;

    switch (key->kind)
      {
        case VK_STRING:
          {
            size_t result;

            result = (size_t)(lookup_in_string_index(info->string_key_index,
                                                     key->u.string_data));
            if (result > 0)
              {
                *found = TRUE;
                return result;
              }

            *match_possible = FALSE;

            if (!care_about_position_if_not_possible)
              {
                *found = FALSE;
                return 1;
              }

            break;
          }
        case VK_INTEGER:
          {
            char *string;
            boolean deallocation_needed;
            size_t position;

            string = key_string_for_integer(key, &deallocation_needed);
            if (string == NULL)
              {
                *error = TRUE;
                return 0;
              }

            position = (size_t)(lookup_in_string_index(info->integer_key_index,
                                                       string));

            if (deallocation_needed)
                free(string);

            if (position > 0)
              {
                *found = TRUE;
                return position;
              }

            *match_possible = FALSE;

            if (!care_about_position_if_not_possible)
              {
                *found = FALSE;
                return 1;
              }

            break;
          }
        default:
          {
            *match_possible = TRUE;
            break;
          }
      }

    count = info->value_key_keys.element_count;
    assert(count == info->value_key_targets.element_count);

    lower = 0;
    upper = count;

    while (lower < upper)
      {
        size_t index_to_test;
        int pair_order;

        index_to_test = (lower + ((upper - lower) / 2));

        assert(value_is_valid(info->value_key_keys.array[index_to_test]));
                /* VERIFIED */
        assert(value_is_valid(key)); /* VERIFIED */
        pair_order = value_structural_order(
                info->value_key_keys.array[index_to_test], key);
        if (pair_order == -2)
          {
            *error = TRUE;
            return 0;
          }

        if (pair_order == 0)
          {
            assert(*match_possible);
            *found = TRUE;
            return index_to_test + 1;
          }
        else if (pair_order < 0)
          {
            lower = index_to_test + 1;
          }
        else
          {
            upper = index_to_test;
          }
      }

    assert(lower == upper);

    *found = FALSE;
    return lower + 1;
  }

static char *key_string_for_integer(value *integer_value,
                                    boolean *deallocation_needed)
  {
    o_integer oi;

    assert(integer_value != NULL);
    assert(deallocation_needed != NULL);

    assert(integer_value->kind == VK_INTEGER);

    oi = integer_value->u.integer;
    assert(!(oi_out_of_memory(oi)));

    *deallocation_needed = (oi_kind(oi) == IIK_FINITE);

    switch (oi_kind(oi))
      {
        case IIK_FINITE:
          {
            verdict the_verdict;
            size_t length;
            char *string;

            the_verdict = oi_decimal_digit_count(oi, &length);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return NULL;

            string = MALLOC_ARRAY(char, length + 3);
            if (string == NULL)
                return NULL;

            string[0] = (oi_is_negative(oi) ? '-' : '+');

            the_verdict = oi_write_decimal_digits(oi, &(string[1]));
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                free(string);
                return NULL;
              }

            if (length == 0)
              {
                ++length;
                string[1] = '0';
              }
            string[length + 1] = 0;

            return string;
          }
        case IIK_POSITIVE_INFINITY:
          {
            return "+infinity";
          }
        case IIK_NEGATIVE_INFINITY:
          {
            return "-infinity";
          }
        case IIK_UNSIGNED_INFINITY:
          {
            return "infinity";
          }
        case IIK_ZERO_ZERO:
          {
            return "zero-zero";
          }
        default:
          {
            assert(FALSE);
            return NULL;
          }
      }
  }

static verdict map_info_append_value_key_item(map_info *info, value *key,
                                              value *target)
  {
    verdict the_verdict;
    size_t position;

    assert(info != NULL);
    assert(key != NULL);

    the_verdict = value_aa_append(&(info->value_key_keys), key);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_verdict = value_aa_append(&(info->value_key_targets), target);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(info->value_key_keys.element_count > 0);
        --(info->value_key_keys.element_count);
        return the_verdict;
      }

    position = info->value_key_keys.element_count;
    assert(position == info->value_key_targets.element_count);
    assert(position > 0);

    the_verdict = update_map_indexes(info, key, position);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(info->value_key_keys.element_count > 0);
        --(info->value_key_keys.element_count);
        assert(info->value_key_targets.element_count > 0);
        --(info->value_key_targets.element_count);
        return the_verdict;
      }

    add_map_info_to_value_reference(info, key);
    if (target != NULL)
        add_map_info_to_value_reference(info, target);

    if ((key->kind != VK_INTEGER) || (oi_kind(key->u.integer) != IIK_FINITE))
      {
        if (target != NULL)
            ++(info->non_integer_key_count);
      }

    if (value_is_slippery(key))
      {
        ++(info->slippery_key_count);
        ++(info->slippery_count);
      }

    if ((target != NULL) && value_is_slippery(target))
        ++(info->slippery_count);

    return MISSION_ACCOMPLISHED;
  }

static verdict map_info_append_type_key_item(map_info *info, type *key,
                                             value *target)
  {
    verdict the_verdict;

    assert(info != NULL);
    assert(key != NULL);

    the_verdict = type_aa_append(&(info->type_key_keys), key);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_verdict = value_aa_append(&(info->type_key_targets), target);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(info->type_key_keys.element_count > 0);
        --(info->type_key_keys.element_count);
        return the_verdict;
      }

    add_map_info_to_type_reference(info, key);
    if (target != NULL)
        add_map_info_to_value_reference(info, target);

    if (type_is_slippery(key))
        ++(info->slippery_count);

    if ((target != NULL) && value_is_slippery(target))
        ++(info->slippery_count);

    return MISSION_ACCOMPLISHED;
  }

static verdict update_map_indexes(map_info *info, value *key,
                                  size_t target_position)
  {
    assert(key != NULL);

    switch (key->kind)
      {
        case VK_STRING:
          {
            return enter_into_string_index(info->string_key_index,
                    key->u.string_data, (void *)target_position);
          }
        case VK_INTEGER:
          {
            char *string;
            boolean deallocation_needed;
            verdict the_verdict;

            string = key_string_for_integer(key, &deallocation_needed);
            if (string == NULL)
                return MISSION_FAILED;

            the_verdict = enter_into_string_index(info->integer_key_index,
                    string, (void *)target_position);

            if (deallocation_needed)
                free(string);

            return the_verdict;
          }
        default:
          {
            return MISSION_ACCOMPLISHED;
          }
      }
  }

static boolean semi_labeled_value_list_value_has_no_labels(value *the_value)
  {
    size_t component_count;
    size_t component_num;

    assert(the_value != NULL);
    assert(the_value->kind == VK_SEMI_LABELED_VALUE_LIST);

    component_count = value_component_count(the_value);

    for (component_num = 0; component_num < component_count; ++component_num)
      {
        if (value_component_label(the_value, component_num) != NULL)
            return FALSE;
      }

    return TRUE;
  }

static verdict generate_lepton_field_order_information(value *the_value)
  {
    size_t total_count;
    char **labels;
    value **values;
    size_t labeled_element_count;
    size_t unlabeled_element_count;
    size_t element_num;
    size_t_and_pointer *ordered_labeled_elements;
    size_t_and_pointer *ordered_unlabeled_elements;

    assert(the_value != NULL);
    assert((the_value->kind == VK_SEMI_LABELED_MULTI_SET) ||
           (the_value->kind == VK_LEPTON));

    assert(value_is_valid(the_value)); /* VERIFIED */

    total_count = the_value->u.lepton.labels.element_count;
    assert(total_count == the_value->u.lepton.values.element_count);

    GRAB_SYSTEM_LOCK(the_value->u.lepton.lock);

    if ((the_value->u.lepton.labeled_element_count +
         the_value->u.lepton.unlabeled_element_count) == total_count)
      {
        RELEASE_SYSTEM_LOCK(the_value->u.lepton.lock);
        return MISSION_ACCOMPLISHED;
      }

    if (the_value->u.lepton.ordered_labeled_elements != NULL)
      {
        free(the_value->u.lepton.ordered_labeled_elements);
        the_value->u.lepton.ordered_labeled_elements = NULL;
      }
    if (the_value->u.lepton.ordered_unlabeled_elements != NULL)
      {
        free(the_value->u.lepton.ordered_unlabeled_elements);
        the_value->u.lepton.ordered_unlabeled_elements = NULL;
      }

    RELEASE_SYSTEM_LOCK(the_value->u.lepton.lock);

    labels = the_value->u.lepton.labels.array;
    assert(labels != NULL);

    values = the_value->u.lepton.values.array;
    assert(values != NULL);

    labeled_element_count = 0;
    unlabeled_element_count = 0;

    for (element_num = 0; element_num < total_count; ++element_num)
      {
        if (labels[element_num] == NULL)
            ++unlabeled_element_count;
        else
            ++labeled_element_count;
      }

    assert(labeled_element_count + unlabeled_element_count == total_count);

    if (labeled_element_count == 0)
      {
        ordered_labeled_elements = NULL;
      }
    else
      {
        ordered_labeled_elements =
                MALLOC_ARRAY(size_t_and_pointer, labeled_element_count);
        if (ordered_labeled_elements == NULL)
            return MISSION_FAILED;
      }

    if (unlabeled_element_count == 0)
      {
        ordered_unlabeled_elements = NULL;
      }
    else
      {
        ordered_unlabeled_elements =
                MALLOC_ARRAY(size_t_and_pointer, unlabeled_element_count);
        if (ordered_unlabeled_elements == NULL)
          {
            if (ordered_labeled_elements != NULL)
                free(ordered_labeled_elements);
            return MISSION_FAILED;
          }
      }

    labeled_element_count = 0;
    unlabeled_element_count = 0;

    for (element_num = 0; element_num < total_count; ++element_num)
      {
        if (labels[element_num] == NULL)
          {
            assert(ordered_unlabeled_elements != NULL);
            ordered_unlabeled_elements[unlabeled_element_count].the_size_t =
                    element_num;
            assert(value_is_valid(values[element_num])); /* VERIFIED */
            ordered_unlabeled_elements[unlabeled_element_count].pointer =
                    values[element_num];
            ++unlabeled_element_count;
          }
        else
          {
            assert(ordered_labeled_elements != NULL);
            ordered_labeled_elements[labeled_element_count].the_size_t =
                    element_num;
            ordered_labeled_elements[labeled_element_count].pointer =
                    labels[element_num];
            ++labeled_element_count;
          }
      }

    assert(labeled_element_count + unlabeled_element_count == total_count);

    if (labeled_element_count > 0)
      {
        assert(ordered_labeled_elements != NULL);
        qsort(ordered_labeled_elements, labeled_element_count,
              sizeof(size_t_and_pointer), &compare_size_t_and_string);
      }

    if (unlabeled_element_count > 0)
      {
        assert(ordered_unlabeled_elements != NULL);
        qsort(ordered_unlabeled_elements, unlabeled_element_count,
              sizeof(size_t_and_pointer), &compare_size_t_and_value);
      }

    GRAB_SYSTEM_LOCK(the_value->u.lepton.lock);

    if (the_value->u.lepton.ordered_labeled_elements == NULL)
      {
        the_value->u.lepton.labeled_element_count = labeled_element_count;
        the_value->u.lepton.ordered_labeled_elements =
                ordered_labeled_elements;
      }
    else
      {
        if (ordered_labeled_elements != NULL)
            free(ordered_labeled_elements);
      }

    if (the_value->u.lepton.ordered_unlabeled_elements == NULL)
      {
        the_value->u.lepton.unlabeled_element_count = unlabeled_element_count;
        the_value->u.lepton.ordered_unlabeled_elements =
                ordered_unlabeled_elements;
      }
    else
      {
        if (ordered_unlabeled_elements != NULL)
            free(ordered_unlabeled_elements);
      }

    RELEASE_SYSTEM_LOCK(the_value->u.lepton.lock);

    return MISSION_ACCOMPLISHED;
  }

static int compare_size_t_and_string(const void *left, const void *right)
  {
    size_t_and_pointer *left_data;
    size_t_and_pointer *right_data;

    assert(left != NULL);
    assert(right != NULL);

    left_data = (size_t_and_pointer *)left;
    right_data = (size_t_and_pointer *)right;

    return utf8_string_lexicographical_order_by_code_point(
            (const char *)(left_data->pointer),
            (const char *)(right_data->pointer));
  }

static int compare_size_t_and_value(const void *left, const void *right)
  {
    size_t_and_pointer *left_data;
    size_t_and_pointer *right_data;
    int result;

    assert(left != NULL);
    assert(right != NULL);

    left_data = (size_t_and_pointer *)left;
    right_data = (size_t_and_pointer *)right;

    assert(value_is_valid((value *)(left_data->pointer))); /* VERIFIED */
    assert(value_is_valid((value *)(right_data->pointer))); /* VERIFIED */
    result = value_structural_order((value *)(left_data->pointer),
                                    (value *)(right_data->pointer));
    if (result == -2)
      {
        return ((left_data->the_size_t < right_data->the_size_t) ? -1 :
                (left_data->the_size_t > right_data->the_size_t) ? 1 : 0);
      }
    return result;
  }

static int compare_size_t(const void *left, const void *right)
  {
    size_t *left_size_t;
    size_t *right_size_t;

    assert(left != NULL);
    assert(right != NULL);

    left_size_t = (size_t *)left;
    right_size_t = (size_t *)right;

    if (*left_size_t < *right_size_t)
        return -1;
    if (*left_size_t > *right_size_t)
        return 1;
    return 0;
  }

static int compare_map_target_replacement(const void *left, const void *right)
  {
    map_target_replacement *left_replacement;
    map_target_replacement *right_replacement;

    assert(left != NULL);
    assert(right != NULL);

    left_replacement = (map_target_replacement *)left;
    right_replacement = (map_target_replacement *)right;

    if (left_replacement->index < right_replacement->index)
        return -1;
    if (left_replacement->index > right_replacement->index)
        return 1;
    return 0;
  }

static void print_component_value(value *the_value,
        void (*printer)(void *data, const char *format, ...), void *data,
        void (*override)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data))
  {
    size_t count;
    size_t component_num;

    assert(the_value != NULL);
    assert(printer != NULL);

    (*printer)(data, "[");

    count = value_component_count(the_value);

    for (component_num = 0; component_num < count; ++component_num)
      {
        const char *label;
        value *component_value;

        if (component_num > 0)
            (*printer)(data, ", ");

        label = value_component_label(the_value, component_num);
        if (label != NULL)
            (*printer)(data, "%s := ", label);

        component_value = value_component_value(the_value, component_num);
        if (component_value != NULL)
            (*override)(component_value, printer, data);
      }

    (*printer)(data, "]");
  }

static verdict map_value_consolidate(value *map_value)
  {
    map_info *top_info;
    value *base_value;
    map_info *base_info;
    size_t_aa *overlaps;
    size_t overlap_count;
    size_t *overlaps_array;
    map_target_replacement_aa *replacements;
    size_t replacement_count;
    map_target_replacement *replacements_array;
    size_t base_base_count;
    size_t overlap_num;
    size_t base_base_overlap_count;
    size_t replacement_num;
    size_t base_base_replacement_count;
    size_t base_filter_count;
    type **base_filters;
    value **base_filter_targets;
    size_t extension_filter_count;
    size_t base_count;
    size_t extension_count;
    map_info *new_info;
    size_t base_filter_num;
    size_t base_num;
    type **extension_filters;
    value **extension_filter_targets;
    size_t extension_filter_num;
    size_t extension_num;
    validator *all_keys_validator;

    assert(map_value != NULL);

    assert(map_value->kind == VK_MAP);

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);
    top_info = map_value->u.map.map_info;
    assert(top_info != NULL);
    map_info_add_reference(top_info);
    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    assert(top_info->is_extension);
    assert(!(top_info->extension.is_uncompressable));

    assert(validator_is_valid(top_info->all_keys_validator)); /* VERIFIED */

    base_value = top_info->extension.base;
    assert(base_value != NULL);
    assert(base_value->kind == VK_MAP);

    GRAB_SYSTEM_LOCK(base_value->u.map.lock);
    base_info = base_value->u.map.map_info;
    assert(base_info != NULL);
    map_info_add_reference(base_info);
    RELEASE_SYSTEM_LOCK(base_value->u.map.lock);

    overlaps = &(top_info->extension.overlaps);
    overlap_count = overlaps->element_count;
    overlaps_array = overlaps->array;

    GRAB_SYSTEM_LOCK(top_info->extension.overlap_sorting_lock);

    if (!(top_info->extension.overlaps_sorted))
      {
        if (overlap_count > 0)
          {
            qsort(overlaps_array, overlap_count, sizeof(size_t),
                  &compare_size_t);
          }
        top_info->extension.overlaps_sorted = TRUE;
      }

    RELEASE_SYSTEM_LOCK(top_info->extension.overlap_sorting_lock);

    replacements = &(top_info->extension.replacements);
    replacement_count = replacements->element_count;
    replacements_array = replacements->array;

    if (replacement_count > 0)
      {
        GRAB_SYSTEM_LOCK(top_info->extension.replacement_lock);

        if (!(top_info->extension.replacements_sorted))
          {
            size_t replacement_num;

            qsort(replacements_array, replacement_count,
                  sizeof(map_target_replacement),
                  &compare_map_target_replacement);

            top_info->extension.replacements_sorted = TRUE;

            for (replacement_num = 0; replacement_num < replacement_count;
                 ++replacement_num)
              {
                verdict the_verdict;

                the_verdict = enter_into_map_replacement_index(top_info,
                        replacements_array[replacement_num].index,
                        replacement_num);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    RELEASE_SYSTEM_LOCK(top_info->extension.replacement_lock);
                    map_info_remove_reference(base_info, NULL);
                    map_info_remove_reference(top_info, NULL);
                    return the_verdict;
                  }
              }
          }

        RELEASE_SYSTEM_LOCK(top_info->extension.replacement_lock);
      }

    if (!(base_info->is_extension))
      {
        base_base_count = 0;
      }
    else
      {
        base_base_count =
                (map_value_item_count(base_info->extension.base) -
                 base_info->extension.overlaps.element_count);
      }

    overlap_num = 0;

    while ((overlap_num < overlap_count) &&
           (overlaps_array[overlap_num] < base_base_count))
      {
        ++overlap_num;
      }

    base_base_overlap_count = overlap_num;

    replacement_num = 0;

    while ((replacement_num < replacement_count) &&
           (replacements_array[replacement_num].index < base_base_count))
      {
        ++replacement_num;
      }

    base_base_replacement_count = replacement_num;

    base_filter_count = base_info->type_key_keys.element_count;
    assert(base_filter_count == base_info->type_key_targets.element_count);
    base_filters = base_info->type_key_keys.array;
    base_filter_targets = base_info->type_key_targets.array;

    extension_filter_count = top_info->type_key_keys.element_count;
    assert(extension_filter_count == top_info->type_key_targets.element_count);

    base_count = base_info->value_key_keys.element_count;
    assert(base_count == base_info->value_key_targets.element_count);

    extension_count = top_info->value_key_keys.element_count;
    assert(extension_count == top_info->value_key_targets.element_count);

    new_info = create_map_info(base_count + extension_count + 10,
            base_filter_count + extension_filter_count + 10);
    if (new_info == NULL)
      {
        map_info_remove_reference(base_info, NULL);
        map_info_remove_reference(top_info, NULL);
        return MISSION_FAILED;
      }

    assert(new_info->reference_cluster == NULL);
    assert(new_info->cluster_use_count == 0);
    assert(new_info->negated_cluster_use_count == 0);
    new_info->reference_cluster = map_value->reference_cluster;

    assert(!(new_info->is_extension));

    for (base_filter_num = 0; base_filter_num < base_filter_count;
         ++base_filter_num)
      {
        value *target;
        verdict the_verdict;

        if (overlap_num < overlap_count)
          {
            size_t element;

            element = overlaps_array[overlap_num];
            assert(element >= base_base_count + base_filter_num);
            if (element == base_base_count + base_filter_num)
              {
                ++overlap_num;
                continue;
              }
          }

        target = base_filter_targets[base_filter_num];
        while (replacement_num < replacement_count)
          {
            map_target_replacement *element;

            element = &(replacements_array[replacement_num]);
            assert(element->index >= base_base_count + base_filter_num);
            if (element->index > base_base_count + base_filter_num)
                break;

            target = element->new_target;
            ++replacement_num;
          }

        the_verdict = map_info_append_type_key_item(new_info,
                base_filters[base_filter_num], target);
        assert(the_verdict == MISSION_ACCOMPLISHED);
      }

    for (base_num = 0; base_num < base_count; ++base_num)
      {
        value *key;
        value *target;
        verdict the_verdict;

        key = base_info->value_key_keys.array[base_num];
        target = base_info->value_key_targets.array[base_num];

        if (overlap_num < overlap_count)
          {
            size_t element;

            element = overlaps_array[overlap_num];
            assert(element >= base_base_count + base_filter_count + base_num);
            if (element == base_base_count + base_filter_count + base_num)
              {
                ++overlap_num;
                continue;
              }
          }

        while (replacement_num < replacement_count)
          {
            map_target_replacement *element;

            element = &(replacements_array[replacement_num]);
            assert(element->index >=
                   base_base_count + base_filter_count + base_num);
            if (element->index >
                base_base_count + base_filter_count + base_num)
              {
                break;
              }

            target = element->new_target;
            ++replacement_num;
          }

        the_verdict = map_info_append_value_key_item(new_info, key, target);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            map_info_remove_reference(new_info, NULL);
            map_info_remove_reference(base_info, NULL);
            map_info_remove_reference(top_info, NULL);
            return the_verdict;
          }
      }

    assert(overlap_num == overlap_count);
    assert(replacement_num == replacement_count);

    new_info->is_extension = base_info->is_extension;

    if (new_info->is_extension)
      {
        verdict the_verdict;
        size_t overlap_num;
        size_t replacement_num;

        the_verdict = initialize_extension_info(new_info,
                base_base_overlap_count +
                base_info->extension.overlaps.element_count + 10,
                base_base_replacement_count +
                base_info->extension.replacements.element_count + 10);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            new_info->is_extension = FALSE;
            map_info_remove_reference(new_info, NULL);
            map_info_remove_reference(base_info, NULL);
            map_info_remove_reference(top_info, NULL);
            return the_verdict;
          }

        map_info_set_extension_base(new_info, base_info->extension.base);

        GRAB_SYSTEM_LOCK(base_info->extension.overlap_sorting_lock);

        for (overlap_num = 0;
             overlap_num < base_info->extension.overlaps.element_count;
             ++overlap_num)
          {
            size_t item;
            verdict the_verdict;

            item = base_info->extension.overlaps.array[overlap_num];
            the_verdict =
                    size_t_aa_append(&(new_info->extension.overlaps), item);
            assert(the_verdict == MISSION_ACCOMPLISHED);

            the_verdict = enter_into_map_overlap_index(new_info, item);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                RELEASE_SYSTEM_LOCK(base_info->extension.overlap_sorting_lock);
                map_info_remove_reference(new_info, NULL);
                map_info_remove_reference(base_info, NULL);
                map_info_remove_reference(top_info, NULL);
                return the_verdict;
              }
          }

        if (base_info->extension.overlaps.element_count == 0)
          {
            new_info->extension.overlaps_sorted = TRUE;
          }
        else
          {
            new_info->extension.overlaps_sorted =
                    base_info->extension.overlaps_sorted;
          }

        RELEASE_SYSTEM_LOCK(base_info->extension.overlap_sorting_lock);

        GRAB_SYSTEM_LOCK(base_info->extension.replacement_lock);

        for (replacement_num = 0;
             replacement_num < base_info->extension.replacements.element_count;
             ++replacement_num)
          {
            map_target_replacement item;
            verdict the_verdict;

            item = base_info->extension.replacements.array[replacement_num];

            the_verdict = enter_replacement(new_info, item.index,
                                            item.new_target, NULL);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                RELEASE_SYSTEM_LOCK(base_info->extension.replacement_lock);
                map_info_remove_reference(new_info, NULL);
                map_info_remove_reference(base_info, NULL);
                map_info_remove_reference(top_info, NULL);
                return the_verdict;
              }

            add_map_info_to_value_reference(new_info, item.new_target);
          }

        RELEASE_SYSTEM_LOCK(base_info->extension.replacement_lock);

        new_info->extension.is_uncompressable =
                base_info->extension.is_uncompressable;
      }

    extension_filters = top_info->type_key_keys.array;
    extension_filter_targets = top_info->type_key_targets.array;

    for (extension_filter_num = 0;
         extension_filter_num < extension_filter_count; ++extension_filter_num)
      {
        verdict the_verdict;

        the_verdict = map_info_append_type_key_item(new_info,
                extension_filters[extension_filter_num],
                extension_filter_targets[extension_filter_num]);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            map_info_remove_reference(new_info, NULL);
            map_info_remove_reference(base_info, NULL);
            map_info_remove_reference(top_info, NULL);
            return the_verdict;
          }
      }

    for (extension_num = 0; extension_num < extension_count; ++extension_num)
      {
        value *key;
        value *target;
        boolean found;
        boolean doubt;
        boolean error;
        boolean match_possible;
        size_t position;
        size_t to_move;
        verdict the_verdict;

        key = top_info->value_key_keys.array[extension_num];
        assert(key != NULL);
        assert(value_is_valid(key)); /* VERIFIED */

        target = top_info->value_key_targets.array[extension_num];
        assert(target != NULL);

        assert(validator_is_valid(new_info->all_keys_validator));
                /* VERIFIED */
        assert(value_is_valid(key)); /* VERIFIED */
        position = map_info_find_index_for_key(new_info, key, &found, &doubt,
                                               &error, &match_possible, TRUE);
        if (error)
          {
            map_info_remove_reference(new_info, NULL);
            map_info_remove_reference(base_info, NULL);
            map_info_remove_reference(top_info, NULL);
            return MISSION_FAILED;
          }

        assert(!doubt);
        assert(!found);
        assert(position > 0);

        to_move = new_info->value_key_keys.element_count;

        the_verdict = map_info_append_value_key_item(new_info, key, target);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            map_info_remove_reference(new_info, NULL);
            map_info_remove_reference(base_info, NULL);
            map_info_remove_reference(top_info, NULL);
            return the_verdict;
          }

        the_verdict = map_info_bubble_value_keys(new_info, position, to_move,
                                                 key, target);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            map_info_remove_reference(new_info, NULL);
            map_info_remove_reference(base_info, NULL);
            map_info_remove_reference(top_info, NULL);
            return the_verdict;
          }
      }

    if (new_info->is_extension)
      {
        size_t replacement_num;
        size_t overlap_num;

        for (replacement_num = 0;
             replacement_num < base_base_replacement_count; ++replacement_num)
          {
            map_target_replacement item;
            verdict the_verdict;

            item = replacements_array[replacement_num];

            the_verdict = enter_replacement(new_info,
                    map_item_num_for_extension_base(base_info, item.index),
                    item.new_target, NULL);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                map_info_remove_reference(new_info, NULL);
                map_info_remove_reference(base_info, NULL);
                map_info_remove_reference(top_info, NULL);
                return the_verdict;
              }

            add_map_info_to_value_reference(new_info, item.new_target);
          }

        if ((new_info->extension.overlaps.element_count != 0) &&
            (base_base_overlap_count != 0))
          {
            new_info->extension.overlaps_sorted = FALSE;
          }

        for (overlap_num = 0; overlap_num < base_base_overlap_count;
             ++overlap_num)
          {
            size_t new_index;
            verdict the_verdict;

            new_index = map_item_num_for_extension_base(base_info,
                    overlaps_array[overlap_num]);
            the_verdict = size_t_aa_append(&(new_info->extension.overlaps),
                                           new_index);
            assert(the_verdict == MISSION_ACCOMPLISHED);

            the_verdict = enter_into_map_overlap_index(new_info, new_index);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                map_info_remove_reference(new_info, NULL);
                map_info_remove_reference(base_info, NULL);
                map_info_remove_reference(top_info, NULL);
                return the_verdict;
              }
          }
      }

    new_info->item_count = top_info->item_count;

    GRAB_SYSTEM_LOCK(map_value->u.map.lock);

    all_keys_validator = top_info->all_keys_validator;
    top_info->all_keys_validator = new_info->all_keys_validator;
    new_info->all_keys_validator = all_keys_validator;

    if (map_value->u.map.map_info->is_extension)
      {
        value *end_base_value;
        value *next;
        value *previous;

        end_base_value = map_value->u.map.map_info->extension.base;
        assert(end_base_value != NULL);

        GRAB_SYSTEM_LOCK(end_base_value->u.map.lock);

        next = map_value->u.map.next_extension;
        previous = map_value->u.map.previous_extension;

        assert(end_base_value != NULL);
        assert(end_base_value->kind == VK_MAP);

        if (next != NULL)
          {
            assert(next->kind == VK_MAP);
            assert(next->u.map.map_info->is_extension);
            assert(next->u.map.map_info->extension.base == end_base_value);
            assert(next->u.map.previous_extension == map_value);
            next->u.map.previous_extension = previous;
          }

        if (previous != NULL)
          {
            assert(previous->kind == VK_MAP);
            assert(previous->u.map.map_info->is_extension);
            assert(previous->u.map.map_info->extension.base == end_base_value);
            assert(previous->u.map.next_extension == map_value);
            previous->u.map.next_extension = next;
          }
        else
          {
            assert(end_base_value->u.map.first_extension == map_value);
            end_base_value->u.map.first_extension = next;
          }

        RELEASE_SYSTEM_LOCK(end_base_value->u.map.lock);
      }

    if (new_info->is_extension)
      {
        value *base_base;
        value *first_extension;

        base_base = base_info->extension.base;
        assert(base_base != NULL);

        GRAB_SYSTEM_LOCK(base_base->u.map.lock);

        first_extension = base_base->u.map.first_extension;
        assert(first_extension != NULL);
        assert(first_extension->u.map.previous_extension == NULL);
        first_extension->u.map.previous_extension = map_value;
        map_value->u.map.next_extension = first_extension;
        map_value->u.map.previous_extension = NULL;
        base_base->u.map.first_extension = map_value;
      }

    map_info_remove_reference(map_value->u.map.map_info, NULL);
    map_value->u.map.map_info = new_info;

    GRAB_SYSTEM_LOCK(map_value->lock);
    assert(map_value->reference_cluster == new_info->reference_cluster);
    assert(map_value->cluster_use_count ==
           (new_info->cluster_use_count -
            new_info->negated_cluster_use_count));
    RELEASE_SYSTEM_LOCK(map_value->lock);

    if (new_info->is_extension)
      {
        RELEASE_SYSTEM_LOCK(base_info->extension.base->u.map.lock);
      }

    RELEASE_SYSTEM_LOCK(map_value->u.map.lock);

    map_info_remove_reference(base_info, NULL);
    map_info_remove_reference(top_info, NULL);

    return MISSION_ACCOMPLISHED;
  }

static void clear_map_fields(value *the_value, jumper *the_jumper)
  {
    map_info *info;

    assert(the_value != NULL);

    assert(the_value->kind == VK_MAP);
    assert(the_value->u.map.first_extension == NULL);

    info = the_value->u.map.map_info;

    if (info->is_extension)
      {
        value *base;
        value *next;
        value *previous;

        base = info->extension.base;

        GRAB_SYSTEM_LOCK(base->u.map.lock);

        next = the_value->u.map.next_extension;
        previous = the_value->u.map.previous_extension;

        assert(base != NULL);
        assert(base->kind == VK_MAP);

        if (next != NULL)
          {
            assert(next->kind == VK_MAP);
            assert(next->u.map.map_info->is_extension);
            assert(next->u.map.map_info->extension.base == base);
            assert(next->u.map.previous_extension == the_value);
            next->u.map.previous_extension = previous;
          }

        if (previous != NULL)
          {
            assert(previous->kind == VK_MAP);
            assert(previous->u.map.map_info->is_extension);
            assert(previous->u.map.map_info->extension.base == base);
            assert(previous->u.map.next_extension == the_value);
            previous->u.map.next_extension = next;
          }
        else
          {
            assert(base->u.map.first_extension == the_value);
            base->u.map.first_extension = next;
          }

        RELEASE_SYSTEM_LOCK(base->u.map.lock);
      }

    map_info_remove_reference(info, the_jumper);

    DESTROY_SYSTEM_LOCK(the_value->u.map.lock);
  }

static void delete_tagalongs(value *the_value, jumper *the_jumper)
  {
    assert(the_value != NULL);

    while (the_value->tagalong_chain != NULL)
      {
        value_tagalong_handle *follow;

        follow = the_value->tagalong_chain;
        assert(follow->parent == the_value);

        grab_tagalong_key_value_tagalong_lock(follow->key);

        if (follow->key_next == follow)
          {
            assert(follow->key_previous == follow);
            assert(get_value_tagalong_handle(follow->key) == follow);
            set_value_tagalong_handle(follow->key, NULL, the_jumper);
          }
        else
          {
            assert(follow->key_previous != follow);
            follow->key_previous->key_next = follow->key_next;
            follow->key_next->key_previous = follow->key_previous;
            if (get_value_tagalong_handle(follow->key) == follow)
              {
                set_value_tagalong_handle(follow->key, follow->key_next,
                                          the_jumper);
              }
            else
              {
                release_tagalong_key_value_tagalong_lock(follow->key);
              }
          }

        the_value->tagalong_chain = follow->value_next;

        value_remove_reference(follow->field_value, the_jumper);

        free(follow);
      }
  }

static verdict make_map_copy(value *old_value, value *new_value)
  {
    map_info *old_info;
    size_t value_key_count;
    size_t type_key_count;
    verdict the_verdict;
    map_info *new_info;
    size_t type_key_num;
    size_t value_key_num;
    size_t overlap_count;
    size_t replacement_count;
    value *base;
    value *next;
    size_t *overlaps_array;
    size_t overlap_num;
    map_target_replacement *replacement_array;
    size_t replacement_num;

    assert(old_value != NULL);
    assert(new_value != NULL);

    assert(old_value->kind == VK_MAP);
    assert(new_value->kind == VK_MAP);

    assert(new_value->reference_count == 1);
    assert(new_value->cluster_use_count == 0);

    GRAB_SYSTEM_LOCK(old_value->u.map.lock);
    old_info = old_value->u.map.map_info;
    assert(old_info != NULL);
    map_info_add_reference(old_info);
    RELEASE_SYSTEM_LOCK(old_value->u.map.lock);

    if (old_info->is_extension && (!(old_info->extension.is_uncompressable)) &&
        (old_info->value_key_keys.element_count >=
         EXTENDED_MAP_COPY_CONVERT_SIZE) &&
        (validator_is_valid(old_info->all_keys_validator)))
      {
        verdict the_verdict;

        map_info_remove_reference(old_info, NULL);

        assert(validator_is_valid(old_info->all_keys_validator));
                /* VERIFIED */
        the_verdict = map_value_consolidate(old_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_value_common(new_value);
            return the_verdict;
          }

        GRAB_SYSTEM_LOCK(old_value->u.map.lock);
        old_info = old_value->u.map.map_info;
        assert(old_info != NULL);
        map_info_add_reference(old_info);
        RELEASE_SYSTEM_LOCK(old_value->u.map.lock);
      }

    if (old_info->value_key_keys.element_count >=
        UNEXTENDED_MAP_COPY_EXTEND_SIZE)
      {
        verdict the_verdict;

        map_info_remove_reference(old_info, NULL);

        assert(new_value->reference_count == 1);
        assert(new_value->cluster_use_count == 0);

        the_verdict = set_up_map_extension(old_value, new_value);
        if (the_verdict == MISSION_ACCOMPLISHED)
            new_value->u.map.map_info->extension.is_uncompressable = FALSE;
        return the_verdict;
      }

    new_value->validator = validator_add_validator(new_value->validator,
                                                   old_value->validator);

    value_key_count = old_info->value_key_keys.element_count;
    assert(value_key_count == old_info->value_key_targets.element_count);

    type_key_count = old_info->type_key_keys.element_count;
    assert(type_key_count == old_info->type_key_targets.element_count);

    the_verdict = initialize_map_components(new_value, value_key_count + 10,
                                            type_key_count + 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        map_info_remove_reference(old_info, NULL);
        delete_value_common(new_value);
        return the_verdict;
      }

    new_info = new_value->u.map.map_info;
    assert(new_info != NULL);

    new_value->u.map.first_extension = NULL;

    new_value->tagalong_chain = NULL;

    for (type_key_num = 0; type_key_num < type_key_count; ++type_key_num)
      {
        the_verdict = map_info_append_type_key_item(new_info,
                old_info->type_key_keys.array[type_key_num],
                old_info->type_key_targets.array[type_key_num]);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            map_info_remove_reference(old_info, NULL);
            update_value_cluster_for_map_info(new_value, NULL, NULL);
            delete_value(new_value, NULL);
            return the_verdict;
          }
      }

    for (value_key_num = 0; value_key_num < value_key_count; ++value_key_num)
      {
        the_verdict = map_info_append_value_key_item(new_info,
                old_info->value_key_keys.array[value_key_num],
                old_info->value_key_targets.array[value_key_num]);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            map_info_remove_reference(old_info, NULL);
            update_value_cluster_for_map_info(new_value, NULL, NULL);
            delete_value(new_value, NULL);
            return the_verdict;
          }
      }

    new_info->slippery_count = old_info->slippery_count;
    new_info->item_count = old_info->item_count;

    if (!(old_info->is_extension))
      {
        map_info_remove_reference(old_info, NULL);
        update_value_cluster_for_map_info(new_value, NULL, NULL);
        return MISSION_ACCOMPLISHED;
      }

    assert(!(new_info->is_extension));

    overlap_count = old_info->extension.overlaps.element_count;
    replacement_count = old_info->extension.replacements.element_count;
    the_verdict = initialize_extension_info(new_info, overlap_count + 10,
                                            replacement_count + 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        new_value->u.map.first_extension = NULL;
        map_info_remove_reference(old_info, NULL);
        update_value_cluster_for_map_info(new_value, NULL, NULL);
        delete_value(new_value, NULL);
        return the_verdict;
      }

    new_info->is_extension = TRUE;

    GRAB_SYSTEM_LOCK(old_info->extension.overlap_sorting_lock);

    the_verdict = size_t_aa_append_array(&(new_info->extension.overlaps),
            overlap_count, old_info->extension.overlaps.array);
    assert(the_verdict == MISSION_ACCOMPLISHED);

    new_info->extension.overlaps_sorted = old_info->extension.overlaps_sorted;

    RELEASE_SYSTEM_LOCK(old_info->extension.overlap_sorting_lock);

    base = old_info->extension.base;
    assert(base != NULL);
    assert(base->kind == VK_MAP);
    map_info_set_extension_base(new_info, base);

    new_info->all_keys_validator = validator_add_validator(
            new_info->all_keys_validator, old_info->all_keys_validator);

    GRAB_SYSTEM_LOCK(base->u.map.lock);

    next = base->u.map.first_extension;
    new_value->u.map.next_extension = next;
    base->u.map.first_extension = new_value;
    new_value->u.map.previous_extension = NULL;
    if (next != NULL)
      {
        assert(next->u.map.previous_extension == NULL);
        next->u.map.previous_extension = new_value;
      }

    RELEASE_SYSTEM_LOCK(base->u.map.lock);

    new_info->extension.is_uncompressable =
            old_info->extension.is_uncompressable;

    overlaps_array = new_info->extension.overlaps.array;

    for (overlap_num = 0; overlap_num < overlap_count; ++overlap_num)
      {
        the_verdict = enter_into_map_overlap_index(new_info,
                overlaps_array[overlap_num]);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            map_info_remove_reference(old_info, NULL);
            update_value_cluster_for_map_info(new_value, NULL, NULL);
            delete_value(new_value, NULL);
            return the_verdict;
          }
      }

    GRAB_SYSTEM_LOCK(old_info->extension.replacement_lock);

    the_verdict = map_target_replacement_aa_append_array(
            &(new_info->extension.replacements), replacement_count,
            old_info->extension.replacements.array);
    assert(the_verdict == MISSION_ACCOMPLISHED);

    RELEASE_SYSTEM_LOCK(old_info->extension.replacement_lock);

    replacement_array = new_info->extension.replacements.array;

    for (replacement_num = 0; replacement_num < replacement_count;
         ++replacement_num)
      {
        value *new_target;

        new_target = replacement_array[replacement_num].new_target;
        add_map_info_to_value_reference(new_info, new_target);
        if ((new_info->reference_cluster != NULL) &&
            (value_reference_cluster(map_value_item_target(
                     new_info->extension.base,
                     replacement_array[replacement_num].index)) ==
             new_info->reference_cluster))
          {
            ++(new_info->negated_cluster_use_count);
          }
      }

    for (replacement_num = 0; replacement_num < replacement_count;
         ++replacement_num)
      {
        the_verdict = enter_into_map_replacement_index(new_info,
                replacement_array[replacement_num].index, replacement_num);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            map_info_remove_reference(old_info, NULL);
            update_value_cluster_for_map_info(new_value, NULL, NULL);
            delete_value(new_value, NULL);
            return the_verdict;
          }
      }

    map_info_remove_reference(old_info, NULL);
    update_value_cluster_for_map_info(new_value, NULL, NULL);
    return MISSION_ACCOMPLISHED;
  }

static verdict enter_into_map_overlap_index(map_info *info, size_t item_num)
  {
    char buffer[(sizeof(size_t) * 3) + 2];

    assert(info != NULL);

    assert(info->is_extension);

    sprintf(&(buffer[0]), "%lu", (unsigned long)item_num);
    return enter_into_string_index(info->extension.overlap_index, &(buffer[0]),
                                   (void *)item_num);
  }

static verdict enter_replacement(map_info *info, size_t item_num,
                                 value *new_target, jumper *the_jumper)
  {
    char buffer[(sizeof(size_t) * 3) + 2];
    verdict the_verdict;
    map_target_replacement new_item;

    assert(info != NULL);

    assert(info->is_extension);

    sprintf(&(buffer[0]), "%lu", (unsigned long)item_num);

    if (exists_in_string_index(info->extension.replacement_index, buffer))
      {
        size_t existing_position;
        map_target_replacement *existing_item;
        value *old_target;

        existing_position = (size_t)(lookup_in_string_index(
                info->extension.replacement_index, buffer));
        existing_item =
                &(info->extension.replacements.array[existing_position]);
        assert(existing_item->index == item_num);

        old_target = existing_item->new_target;
        existing_item->new_target = new_target;

        remove_map_info_to_value_reference(info, old_target, the_jumper);

        if ((the_jumper != NULL) && !(jumper_flowing_forward(the_jumper)))
            return MISSION_FAILED;

        return MISSION_ACCOMPLISHED;
      }

    info->extension.replacements_sorted = FALSE;

    the_verdict = enter_into_string_index(info->extension.replacement_index,
            &(buffer[0]),
            (void *)(info->extension.replacements.element_count));
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (the_jumper != NULL)
            jumper_do_abort(the_jumper);
        return the_verdict;
      }

    new_item.index = item_num;
    new_item.new_target = new_target;
    the_verdict = map_target_replacement_aa_append(
            &(info->extension.replacements), new_item);

    if ((the_verdict != MISSION_ACCOMPLISHED) && (the_jumper != NULL))
        jumper_do_abort(the_jumper);
    if (the_verdict == MISSION_ACCOMPLISHED)
      {
        if ((info->reference_cluster != NULL) &&
            (value_reference_cluster(map_value_item_target(
                     info->extension.base, item_num)) ==
             info->reference_cluster))
          {
            ++(info->negated_cluster_use_count);
          }
      }
    return the_verdict;
  }

static verdict enter_into_map_replacement_index(map_info *info,
        size_t base_item_num, size_t replacement_list_position)
  {
    char buffer[(sizeof(size_t) * 3) + 2];

    assert(info != NULL);

    assert(info->is_extension);

    sprintf(&(buffer[0]), "%lu", (unsigned long)base_item_num);
    return enter_into_string_index(info->extension.replacement_index,
            &(buffer[0]), (void *)replacement_list_position);
  }

static verdict set_up_map_extension(value *old_value, value *new_value)
  {
    verdict the_verdict;
    map_info *new_info;
    map_info *old_info;
    validator *old_all_keys_validator;
    value *next;

    assert(old_value != NULL);
    assert(new_value != NULL);

    assert(old_value->kind == VK_MAP);
    assert(new_value->kind == VK_MAP);

    assert(new_value->reference_count == 1);
    assert(new_value->cluster_use_count == 0);

    the_verdict = initialize_map_components(new_value, 10, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_value_common(new_value);
        return the_verdict;
      }

    new_info = new_value->u.map.map_info;
    assert(new_info != NULL);

    new_value->u.map.first_extension = NULL;

    the_verdict = initialize_extension_info(new_info, 10, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_value(new_value, NULL);
        return the_verdict;
      }

    new_info->extension.overlaps_sorted = TRUE;
    new_info->is_extension = TRUE;
    map_info_set_extension_base(new_info, old_value);

    update_value_cluster_for_map_info(new_value, NULL, NULL);

    new_value->validator = validator_add_validator(new_value->validator,
                                                   old_value->validator);

    GRAB_SYSTEM_LOCK(old_value->u.map.lock);

    old_info = old_value->u.map.map_info;
    assert(old_info != NULL);

    old_all_keys_validator = old_info->all_keys_validator;

    next = old_value->u.map.first_extension;
    new_value->u.map.next_extension = next;
    old_value->u.map.first_extension = new_value;
    new_value->u.map.previous_extension = NULL;
    if (next != NULL)
      {
        assert(next->u.map.previous_extension == NULL);
        next->u.map.previous_extension = new_value;
      }

    new_info->slippery_count = old_info->slippery_count;
    new_info->item_count = old_info->item_count;

    RELEASE_SYSTEM_LOCK(old_value->u.map.lock);

    new_info->all_keys_validator = validator_add_validator(
            new_info->all_keys_validator, old_all_keys_validator);

    return MISSION_ACCOMPLISHED;
  }

static size_t map_item_num_for_extension_base(map_info *info, size_t item_num)
  {
    size_t_aa *overlaps;
    size_t *overlaps_array;
    size_t overlap_count;
    size_t lower;
    size_t upper;

    assert(info != NULL);

    assert(info->is_extension);

    if (info->extension.overlaps.element_count == 0)
        return item_num;

    overlaps = &(info->extension.overlaps);
    overlap_count = overlaps->element_count;
    overlaps_array = overlaps->array;

    GRAB_SYSTEM_LOCK(info->extension.overlap_sorting_lock);

    if (!(info->extension.overlaps_sorted))
      {
        if (overlap_count > 0)
          {
            qsort(overlaps_array, overlap_count, sizeof(size_t),
                  &compare_size_t);
          }
        info->extension.overlaps_sorted = TRUE;
      }

    RELEASE_SYSTEM_LOCK(info->extension.overlap_sorting_lock);

    lower = 0;
    upper = overlap_count;

    while (lower < upper)
      {
        size_t to_test;

        to_test = (lower + upper) / 2;
        assert(to_test >= lower);
        assert(to_test < upper);

        if (overlaps_array[to_test] <= (item_num + to_test))
            lower = to_test + 1;
        else
            upper = to_test;
      }

    assert(lower == upper);

    return item_num + lower;
  }

static value *map_base_target(map_info *info, size_t item_num)
  {
    value *replacement;

    replacement = map_base_replacement(info, item_num);
    if (replacement != NULL)
        return replacement;

    return map_value_item_target(info->extension.base, item_num);
  }

static value *map_base_replacement(map_info *info, size_t item_num)
  {
    char buffer[(sizeof(size_t) * 3) + 2];
    value *result;

    sprintf(&(buffer[0]), "%lu", (unsigned long)item_num);

    GRAB_SYSTEM_LOCK(info->extension.replacement_lock);

    if (!(exists_in_string_index(info->extension.replacement_index, buffer)))
      {
        RELEASE_SYSTEM_LOCK(info->extension.replacement_lock);
        return NULL;
      }

    result = info->extension.replacements.array[
            (size_t)(lookup_in_string_index(info->extension.replacement_index,
                                            buffer))].new_target;

    RELEASE_SYSTEM_LOCK(info->extension.replacement_lock);

    return result;
  }

static void merge_extension(value *the_extension, value *the_value,
                            jumper *the_jumper)
  {
    map_info *extension_info;
    map_info *base_info;
    size_t_aa *overlaps;
    size_t overlap_count;
    size_t *overlaps_array;
    size_t type_key_count;
    validator *old_validator;
    map_target_replacement_aa *replacements;
    size_t replacement_count;
    map_target_replacement *replacements_array;
    size_t base_base_count;
    size_t overlap_num;
    size_t base_base_overlap_count;
    size_t replacement_num;
    size_t base_base_replacement_count;
    size_t base_filter_count;
    type **base_filters;
    value **base_filter_targets;
    size_t extension_filter_count;
    size_t base_count;
    size_t extension_count;
    verdict the_verdict;
    size_t target_filter_num;
    size_t base_filter_num;
    size_t target_num;
    size_t base_num;
    type **extension_filters;
    value **extension_filter_targets;
    size_t extension_filter_num;
    size_t extension_num;
    validator *all_keys_validator;

    assert(the_extension != NULL);
    assert(the_value != NULL);
    assert(the_jumper != NULL);

    extension_info = the_extension->u.map.map_info;
    assert(extension_info != NULL);

    if (!(extension_info->is_extension))
        return;
    if (extension_info->extension.base != the_value)
        return;

    assert_is_malloced_block_with_exact_size(the_value, sizeof(value));
    assert(the_value->kind == VK_MAP);
    assert(validator_is_valid(the_value->u.map.map_info->all_keys_validator));

    assert(the_extension->kind == VK_MAP);

    if (extension_info->extension.is_uncompressable)
        return;

    if (!(validator_is_valid(extension_info->all_keys_validator)))
        return;

    GRAB_SYSTEM_LOCK(the_value->lock);
    assert(the_value->reference_count >= 1);
    if (the_value->reference_count > 1)
      {
        RELEASE_SYSTEM_LOCK(the_value->lock);
        return;
      }
    the_value->destructing = TRUE;
    RELEASE_SYSTEM_LOCK(the_value->lock);

    base_info = the_value->u.map.map_info;
    assert(base_info != NULL);

    overlaps = &(extension_info->extension.overlaps);
    overlap_count = overlaps->element_count;
    overlaps_array = overlaps->array;

    GRAB_SYSTEM_LOCK(extension_info->extension.overlap_sorting_lock);

    if (!(extension_info->extension.overlaps_sorted))
      {
        if (overlap_count > 0)
          {
            qsort(overlaps_array, overlap_count, sizeof(size_t),
                  &compare_size_t);
          }
        extension_info->extension.overlaps_sorted = TRUE;
      }

    RELEASE_SYSTEM_LOCK(extension_info->extension.overlap_sorting_lock);

    type_key_count = extension_info->type_key_keys.element_count;
    assert(type_key_count == extension_info->type_key_targets.element_count);

    if (type_key_count > 0)
        return;

    old_validator = the_value->validator;
    the_value->validator = get_trivial_validator();
    validator_remove_reference(old_validator);

    replacements = &(extension_info->extension.replacements);
    replacement_count = replacements->element_count;
    replacements_array = replacements->array;

    if (replacement_count > 0)
      {
        GRAB_SYSTEM_LOCK(extension_info->extension.replacement_lock);

        if (!(extension_info->extension.replacements_sorted))
          {
            size_t replacement_num;

            qsort(replacements_array, replacement_count,
                  sizeof(map_target_replacement),
                  &compare_map_target_replacement);

            extension_info->extension.replacements_sorted = TRUE;

            for (replacement_num = 0; replacement_num < replacement_count;
                 ++replacement_num)
              {
                verdict the_verdict;

                the_verdict = enter_into_map_replacement_index(extension_info,
                        replacements_array[replacement_num].index,
                        replacement_num);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    RELEASE_SYSTEM_LOCK(
                            extension_info->extension.replacement_lock);
                    jumper_do_abort(the_jumper);
                    return;
                  }
              }
          }

        RELEASE_SYSTEM_LOCK(extension_info->extension.replacement_lock);
      }

    if (!(base_info->is_extension))
      {
        base_base_count = 0;
      }
    else
      {
        base_base_count =
                (map_value_item_count(base_info->extension.base) -
                 base_info->extension.overlaps.element_count);
      }

    overlap_num = 0;

    while ((overlap_num < overlap_count) &&
           (overlaps_array[overlap_num] < base_base_count))
      {
        ++overlap_num;
      }

    base_base_overlap_count = overlap_num;

    replacement_num = 0;

    while ((replacement_num < replacement_count) &&
           (replacements_array[replacement_num].index < base_base_count))
      {
        ++replacement_num;
      }

    base_base_replacement_count = replacement_num;

    base_filter_count = base_info->type_key_keys.element_count;
    assert(base_filter_count == base_info->type_key_targets.element_count);
    base_filters = base_info->type_key_keys.array;
    base_filter_targets = base_info->type_key_targets.array;

    extension_filter_count = extension_info->type_key_keys.element_count;
    assert(extension_filter_count ==
           extension_info->type_key_targets.element_count);

    base_count = base_info->value_key_keys.element_count;
    assert(base_count == base_info->value_key_targets.element_count);

    extension_count = extension_info->value_key_keys.element_count;
    assert(extension_count == extension_info->value_key_targets.element_count);

    the_verdict = value_aa_make_space(&(base_info->value_key_keys),
                                      base_count + extension_count);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    the_verdict = value_aa_make_space(&(base_info->value_key_targets),
                                      base_count + extension_count);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    the_verdict = type_aa_make_space(&(base_info->type_key_keys),
            base_filter_count + extension_filter_count);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    the_verdict = value_aa_make_space(&(base_info->type_key_targets),
            base_filter_count + extension_filter_count);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    base_filter_num = base_filter_count;

    if (overlap_num < overlap_count)
      {
        size_t element;

        element = overlaps_array[overlap_num];
        assert(element >= base_base_count);
        base_filter_num = element - base_base_count;
        assert(base_filter_num < base_filter_count);
      }

    if (replacement_num < replacement_count)
      {
        map_target_replacement *element;
        size_t try_base_filter_num;

        element = &(replacements_array[replacement_num]);
        assert(element->index >= base_base_count);
        try_base_filter_num = element->index - base_base_count;
        if (try_base_filter_num < base_filter_num)
            base_filter_num = try_base_filter_num;
      }

    target_filter_num = base_filter_num;

    for (; base_filter_num < base_filter_count; ++base_filter_num)
      {
        type *key;
        value *target;

        key = base_filters[base_filter_num];
        target = base_filter_targets[base_filter_num];

        if (overlap_num < overlap_count)
          {
            size_t element;

            element = overlaps_array[overlap_num];
            assert(element >= base_base_count + base_filter_num);
            if (element == base_base_count + base_filter_num)
              {
                ++overlap_num;

                GRAB_SYSTEM_LOCK(the_value->u.map.lock);

                if (type_is_slippery(key))
                    --(base_info->slippery_count);
                if (value_is_slippery(target))
                    --(base_info->slippery_count);

                remove_map_info_to_type_reference(base_info, key, the_jumper);
                remove_map_info_to_value_reference(base_info, target,
                                                   the_jumper);

                RELEASE_SYSTEM_LOCK(the_value->u.map.lock);

                continue;
              }
          }

        while (replacement_num < replacement_count)
          {
            map_target_replacement *element;
            value *new_target;

            element = &(replacements_array[replacement_num]);
            assert(element->index >= base_base_count + base_filter_num);
            if (element->index > base_base_count + base_filter_num)
                break;

            new_target = element->new_target;

            GRAB_SYSTEM_LOCK(the_value->u.map.lock);

            add_map_info_to_value_reference(base_info, new_target);

            if (value_is_slippery(new_target))
                ++(base_info->slippery_count);
            if (value_is_slippery(target))
                --(base_info->slippery_count);

            remove_map_info_to_value_reference(base_info, target, the_jumper);

            RELEASE_SYSTEM_LOCK(the_value->u.map.lock);

            target = new_target;
            ++replacement_num;
          }

        base_filters[target_filter_num] = key;
        base_filter_targets[target_filter_num] = target;
        ++target_filter_num;
      }

    base_info->type_key_keys.element_count = target_filter_num;
    base_info->type_key_targets.element_count = target_filter_num;

    if (!(jumper_flowing_forward(the_jumper)))
      {
        update_value_cluster_for_map_info(the_value,
                extension_info->reference_cluster, the_jumper);
        return;
      }

    base_num = base_count;

    if (overlap_num < overlap_count)
      {
        size_t element;

        element = overlaps_array[overlap_num];
        assert(element >= base_base_count + base_filter_count);
        base_num = element - (base_base_count + base_filter_count);
        assert(base_num < base_count);
      }

    if (replacement_num < replacement_count)
      {
        map_target_replacement *element;
        size_t try_base_num;

        element = &(replacements_array[replacement_num]);
        assert(element->index >= base_base_count + base_filter_count);
        try_base_num = element->index - (base_base_count + base_filter_count);
        assert(try_base_num < base_count);
        if (try_base_num < base_num)
            base_num = try_base_num;
      }

    target_num = base_num;

    for (; base_num < base_count; ++base_num)
      {
        value *key;
        value *target;
        verdict the_verdict;

        key = base_info->value_key_keys.array[base_num];
        target = base_info->value_key_targets.array[base_num];

        if (overlap_num < overlap_count)
          {
            size_t element;

            element = overlaps_array[overlap_num];
            assert(element >= base_base_count + base_filter_count + base_num);
            if (element == base_base_count + base_filter_count + base_num)
              {
                ++overlap_num;

                GRAB_SYSTEM_LOCK(the_value->u.map.lock);

                if (value_is_slippery(key))
                  {
                    --(base_info->slippery_key_count);
                    --(base_info->slippery_count);
                  }
                if (value_is_slippery(target))
                    --(base_info->slippery_count);
                if ((key->kind != VK_INTEGER) ||
                    (oi_kind(key->u.integer) != IIK_FINITE))
                  {
                    --(base_info->non_integer_key_count);
                  }

                switch (key->kind)
                  {
                    case VK_STRING:
                      {
                        remove_from_string_index(base_info->string_key_index,
                                                 key->u.string_data);
                        break;
                      }
                    case VK_INTEGER:
                      {
                        char *string;
                        boolean deallocation_needed;

                        string = key_string_for_integer(key,
                                                        &deallocation_needed);
                        if (string == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            break;
                          }

                        remove_from_string_index(base_info->integer_key_index,
                                                 string);

                        if (deallocation_needed)
                            free(string);

                        break;
                      }
                    default:
                      {
                        break;
                      }
                  }

                remove_map_info_to_value_reference(base_info, key, the_jumper);
                remove_map_info_to_value_reference(base_info, target,
                                                   the_jumper);

                RELEASE_SYSTEM_LOCK(the_value->u.map.lock);

                continue;
              }
          }

        while (replacement_num < replacement_count)
          {
            map_target_replacement *element;
            value *new_target;

            element = &(replacements_array[replacement_num]);
            assert(element->index >=
                   base_base_count + base_filter_count + base_num);
            if (element->index >
                base_base_count + base_filter_count + base_num)
              {
                break;
              }

            new_target = element->new_target;

            GRAB_SYSTEM_LOCK(the_value->u.map.lock);

            replace_value_key_target(base_info, base_num, new_target,
                    base_info->value_key_keys.array[base_num], NULL, NULL,
                    NULL);

            RELEASE_SYSTEM_LOCK(the_value->u.map.lock);

            target = new_target;
            ++replacement_num;
          }

        if (target_num != base_num)
          {
            the_verdict = update_map_indexes(base_info, key, target_num + 1);
            if (the_verdict != MISSION_ACCOMPLISHED)
                jumper_do_abort(the_jumper);
          }

        base_info->value_key_keys.array[target_num] = key;
        base_info->value_key_targets.array[target_num] = target;
        ++target_num;
      }

    base_info->value_key_keys.element_count = target_num;
    base_info->value_key_targets.element_count = target_num;

    if (!(jumper_flowing_forward(the_jumper)))
      {
        update_value_cluster_for_map_info(the_value,
                extension_info->reference_cluster, the_jumper);
        return;
      }

    assert(overlap_num == overlap_count);
    assert(replacement_num == replacement_count);

    extension_filters = extension_info->type_key_keys.array;
    extension_filter_targets = extension_info->type_key_targets.array;

    for (extension_filter_num = 0;
         extension_filter_num < extension_filter_count; ++extension_filter_num)
      {
        verdict the_verdict;

        the_verdict = map_info_append_type_key_item(base_info,
                extension_filters[extension_filter_num],
                extension_filter_targets[extension_filter_num]);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            update_value_cluster_for_map_info(the_value,
                    extension_info->reference_cluster, the_jumper);
            return;
          }
      }

    for (extension_num = 0; extension_num < extension_count; ++extension_num)
      {
        value *key;
        value *target;
        boolean found;
        boolean doubt;
        boolean error;
        boolean match_possible;
        size_t position;
        size_t to_move;
        verdict the_verdict;

        key = extension_info->value_key_keys.array[extension_num];
        assert(key != NULL);
        assert(value_is_valid(key)); /* VERIFIED */

        target = extension_info->value_key_targets.array[extension_num];
        assert(target != NULL);

        assert(validator_is_valid(base_info->all_keys_validator));
                /* VERIFIED */
        assert(value_is_valid(key)); /* VERIFIED */
        position = map_info_find_index_for_key(base_info, key, &found, &doubt,
                                               &error, &match_possible, TRUE);
        if (error)
          {
            jumper_do_abort(the_jumper);
            update_value_cluster_for_map_info(the_value,
                    extension_info->reference_cluster, the_jumper);
            return;
          }

        assert(!doubt);
        assert(!found);
        assert(position > 0);

        to_move = base_info->value_key_keys.element_count;

        the_verdict = map_info_append_value_key_item(base_info, key, target);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            update_value_cluster_for_map_info(the_value,
                    extension_info->reference_cluster, the_jumper);
            return;
          }

        the_verdict = map_info_bubble_value_keys(base_info, position, to_move,
                                                 key, target);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            update_value_cluster_for_map_info(the_value,
                    extension_info->reference_cluster, the_jumper);
            return;
          }
      }

    if (base_info->is_extension)
      {
        size_t replacement_num;
        verdict the_verdict;
        size_t overlap_num;

        for (replacement_num = 0;
             replacement_num < base_base_replacement_count; ++replacement_num)
          {
            map_target_replacement item;
            verdict the_verdict;

            item = replacements_array[replacement_num];

            GRAB_SYSTEM_LOCK(the_value->u.map.lock);

            the_verdict = enter_replacement(base_info,
                    map_item_num_for_extension_base(base_info, item.index),
                    item.new_target, the_jumper);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                RELEASE_SYSTEM_LOCK(the_value->u.map.lock);
                update_value_cluster_for_map_info(the_value,
                        extension_info->reference_cluster, the_jumper);
                return;
              }

            add_map_info_to_value_reference(base_info, item.new_target);

            RELEASE_SYSTEM_LOCK(the_value->u.map.lock);
          }

        the_verdict = size_t_aa_make_space(&(base_info->extension.overlaps),
                base_info->extension.overlaps.element_count +
                base_base_overlap_count);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            update_value_cluster_for_map_info(the_value,
                    extension_info->reference_cluster, the_jumper);
            return;
          }

        for (overlap_num = 0; overlap_num < base_base_overlap_count;
             ++overlap_num)
          {
            size_t new_index;
            verdict the_verdict;

            new_index = map_item_num_for_extension_base(base_info,
                    overlaps_array[overlap_num]);
            assert(base_info->extension.overlaps.element_count + overlap_num <=
                   base_info->extension.overlaps.space);
            base_info->extension.overlaps.array[
                    base_info->extension.overlaps.element_count + overlap_num]
                            = new_index;

            the_verdict = enter_into_map_overlap_index(base_info, new_index);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                update_value_cluster_for_map_info(the_value,
                        extension_info->reference_cluster, the_jumper);
                return;
              }
          }

        if ((base_info->extension.overlaps.element_count != 0) &&
            (base_base_overlap_count != 0))
          {
            base_info->extension.overlaps_sorted = FALSE;
          }

        assert(base_info->extension.overlaps.element_count +
               base_base_overlap_count <= base_info->extension.overlaps.space);
        base_info->extension.overlaps.element_count += base_base_overlap_count;
      }

    base_info->item_count = extension_info->item_count;

    all_keys_validator = extension_info->all_keys_validator;
    extension_info->all_keys_validator = base_info->all_keys_validator;
    base_info->all_keys_validator = all_keys_validator;

    assert(the_extension->u.map.next_extension == NULL);
    assert(the_extension->u.map.previous_extension == NULL);

    the_value->u.map.first_extension = NULL;

    update_value_cluster_for_map_info(the_value,
            extension_info->reference_cluster, the_jumper);

    if (base_info->is_extension)
      {
        value *base;
        value *next_extension;
        value *previous_extension;

        base = base_info->extension.base;
        assert(base != NULL);

        GRAB_SYSTEM_LOCK(base->u.map.lock);

        the_extension->u.map.next_extension = the_value->u.map.next_extension;
        the_extension->u.map.previous_extension =
                the_value->u.map.previous_extension;

        next_extension = the_extension->u.map.next_extension;
        if (next_extension != NULL)
          {
            assert(next_extension->u.map.previous_extension == the_value);
            next_extension->u.map.previous_extension = the_extension;
          }

        previous_extension = the_extension->u.map.previous_extension;
        if (previous_extension != NULL)
          {
            assert(previous_extension->u.map.next_extension == the_value);
            previous_extension->u.map.next_extension = the_extension;
          }
        else
          {
            assert(base->u.map.first_extension == the_value);
            base->u.map.first_extension = the_extension;
          }
      }

    value_add_reference(the_value);
    map_info_remove_reference(extension_info, the_jumper);
    the_extension->u.map.map_info = base_info;

    assert((the_extension->reference_cluster == base_info->reference_cluster)
           ||
           ((the_extension->reference_cluster == NULL) &&
            ((base_info->cluster_use_count -
              base_info->negated_cluster_use_count) == 0)));
    assert(the_extension->cluster_use_count ==
           (base_info->cluster_use_count -
            base_info->negated_cluster_use_count));
    assert(the_value->reference_cluster == base_info->reference_cluster);
    assert(the_value->cluster_use_count ==
           (base_info->cluster_use_count -
            base_info->negated_cluster_use_count));

    if (base_info->is_extension)
      {
        RELEASE_SYSTEM_LOCK(base_info->extension.base->u.map.lock);
      }

    assert(the_value->reference_count == 1);
    DESTROY_SYSTEM_LOCK(the_value->u.map.lock);
    delete_tagalongs(the_value, the_jumper);
    validator_remove_reference(the_value->validator);
    the_value->reference_count = 0;
    DESTROY_SYSTEM_LOCK(the_value->tagalong_lock);
    DESTROY_SYSTEM_LOCK(the_value->lock);
    if ((the_value->reference_cluster != NULL) &&
        (the_value->cluster_use_count > 0))
      {
        reference_cluster_remove_reference(the_value->reference_cluster,
                                           the_jumper);
      }
    free(the_value);
  }

static void replace_value_key_target(map_info *info, size_t item_num,
        value *new_target, value *key, validator **the_validator,
        const source_location *location, jumper *the_jumper)
  {
    boolean are_equal;
    boolean doubt;
    value *old_target;

    assert(info != NULL);
    assert(key != NULL);

    assert(value_is_valid(key)); /* VERIFIED */

    assert(info->value_key_keys.element_count ==
           info->value_key_targets.element_count);
    assert(value_is_valid(key)); /* VERIFIED */
    assert(item_num < info->value_key_keys.element_count);
    assert(value_is_valid(info->value_key_keys.array[item_num]));
            /* VERIFIED */
    if (the_jumper != NULL)
      {
        are_equal = values_are_equal(key, info->value_key_keys.array[item_num],
                                     &doubt, location, the_jumper);
        assert(jumper_flowing_forward(the_jumper));
      }
    else
      {
        are_equal = (key == info->value_key_keys.array[item_num]);
        doubt = FALSE;
      }
    assert(are_equal);
    assert(!doubt);

    old_target = info->value_key_targets.array[item_num];

    if ((key->kind != VK_INTEGER) || (oi_kind(key->u.integer) != IIK_FINITE))
      {
        if ((new_target == NULL) && (old_target != NULL))
          {
            assert(info->non_integer_key_count > 0);
            --(info->non_integer_key_count);
          }
        else if ((new_target != NULL) && (old_target == NULL))
          {
            ++(info->non_integer_key_count);
          }
      }

    update_info_for_value_key_target(info, old_target, new_target, key,
                                     the_validator, the_jumper);

    if (old_target != NULL)
        remove_map_info_to_value_reference(info, old_target, the_jumper);

    info->value_key_targets.array[item_num] = new_target;
  }

static void update_info_for_value_key_target(map_info *info, value *old_target,
        value *new_target, value *key, validator **the_validator,
        jumper *the_jumper)
  {
    assert(info != NULL);
    assert(key != NULL);

    if ((old_target != NULL) && (the_validator != NULL))
      {
        assert(the_jumper != NULL);

        *the_validator = validator_remove_validator(*the_validator,
                value_validator(old_target));
        if (*the_validator == NULL)
          {
            jumper_do_abort(the_jumper);
            return;
          }
      }

    if (new_target != NULL)
      {
        if (the_validator != NULL)
          {
            assert(the_jumper != NULL);

            *the_validator = validator_add_validator(*the_validator,
                    value_validator(new_target));
            if (*the_validator == NULL)
              {
                jumper_do_abort(the_jumper);
                return;
              }
          }

        add_map_info_to_value_reference(info, new_target);
        if (value_is_slippery(new_target))
            ++(info->slippery_count);
      }
    if (old_target != NULL)
      {
        if (value_is_slippery(old_target))
            --(info->slippery_count);
        if (new_target == NULL)
          {
            if (value_is_slippery(key))
                --(info->slippery_count);
          }
      }
    else
      {
        if (new_target != NULL)
          {
            if (value_is_slippery(key))
                ++(info->slippery_count);
          }
      }
  }

static map_info *create_map_info(size_t initial_value_key_space,
                                 size_t initial_type_key_space)
  {
    map_info *result;
    verdict the_verdict;

    result = MALLOC_ONE_OBJECT(map_info);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->lock, free(result); return NULL);

    the_verdict =
            value_aa_init(&(result->value_key_keys), initial_value_key_space);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        DESTROY_SYSTEM_LOCK(result->lock);
        free(result);
        return NULL;
      }

    the_verdict = value_aa_init(&(result->value_key_targets),
                                initial_value_key_space);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->value_key_keys.array);
        DESTROY_SYSTEM_LOCK(result->lock);
        free(result);
        return NULL;
      }

    the_verdict =
            type_aa_init(&(result->type_key_keys), initial_type_key_space);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->value_key_targets.array);
        free(result->value_key_keys.array);
        DESTROY_SYSTEM_LOCK(result->lock);
        free(result);
        return NULL;
      }

    the_verdict =
            value_aa_init(&(result->type_key_targets), initial_type_key_space);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->type_key_keys.array);
        free(result->value_key_targets.array);
        free(result->value_key_keys.array);
        DESTROY_SYSTEM_LOCK(result->lock);
        free(result);
        return NULL;
      }

    result->item_count = 0;

    result->string_key_index = create_string_index();
    if (result->string_key_index == NULL)
      {
        free(result->type_key_targets.array);
        free(result->type_key_keys.array);
        free(result->value_key_targets.array);
        free(result->value_key_keys.array);
        DESTROY_SYSTEM_LOCK(result->lock);
        free(result);
        return NULL;
      }

    result->integer_key_index = create_string_index();
    if (result->integer_key_index == NULL)
      {
        destroy_string_index(result->string_key_index);
        free(result->type_key_targets.array);
        free(result->type_key_keys.array);
        free(result->value_key_targets.array);
        free(result->value_key_keys.array);
        DESTROY_SYSTEM_LOCK(result->lock);
        free(result);
        return NULL;
      }

    result->non_integer_key_count = 0;
    result->slippery_key_count = 0;
    result->slippery_count = 0;

    result->all_keys_validator = get_trivial_validator();
    assert(result->all_keys_validator != NULL);

    result->is_extension = FALSE;

    result->reference_cluster = NULL;
    result->cluster_use_count = 0;
    result->negated_cluster_use_count = 0;
    result->reference_count = 1;

    return result;
  }

static void map_info_add_reference(map_info *info)
  {
    assert(info != NULL);
    assert_is_malloced_block_with_exact_size(info, sizeof(map_info));

    GRAB_SYSTEM_LOCK(info->lock);
    assert(info->reference_count > 0);
    ++(info->reference_count);
    RELEASE_SYSTEM_LOCK(info->lock);
  }

static void map_info_remove_reference(map_info *info, jumper *the_jumper)
  {
    reference_cluster *cluster;
    size_t new_reference_count;

    assert(info != NULL);
    assert_is_malloced_block_with_exact_size(info, sizeof(map_info));

    GRAB_SYSTEM_LOCK(info->lock);
    assert(info->reference_count > 0);
    --(info->reference_count);
    new_reference_count = info->reference_count;
    cluster = info->reference_cluster;
    RELEASE_SYSTEM_LOCK(info->lock);

    if (new_reference_count > 0)
        return;

    validator_remove_reference(info->all_keys_validator);

      {
        value **keys;
        value **targets;
        size_t count;
        size_t number;

        keys = info->value_key_keys.array;
        targets = info->value_key_targets.array;
        assert(keys != NULL);
        assert(targets != NULL);

        count = info->value_key_keys.element_count;
        assert(count == info->value_key_targets.element_count);

        for (number = 0; number < count; ++number)
          {
            value_remove_reference_with_reference_cluster(keys[number],
                                                          the_jumper, cluster);
            if (targets[number] != NULL)
              {
                value_remove_reference_with_reference_cluster(targets[number],
                        the_jumper, cluster);
              }
          }

        free(keys);
        free(targets);
      }

      {
        type **keys;
        value **targets;
        size_t count;
        size_t number;

        keys = info->type_key_keys.array;
        targets = info->type_key_targets.array;
        assert(keys != NULL);
        assert(targets != NULL);

        count = info->type_key_keys.element_count;
        assert(count == info->type_key_targets.element_count);

        for (number = 0; number < count; ++number)
          {
            type_remove_reference_with_reference_cluster(keys[number],
                                                         the_jumper, cluster);
            if (targets[number] != NULL)
              {
                value_remove_reference_with_reference_cluster(targets[number],
                        the_jumper, cluster);
              }
          }

        free(keys);
        free(targets);
      }

    destroy_string_index(info->string_key_index);
    destroy_string_index(info->integer_key_index);

    if (info->is_extension)
      {
        size_t replacement_count;
        map_target_replacement *replacement_array;
        size_t replacement_num;

        free(info->extension.overlaps.array);
        destroy_string_index(info->extension.overlap_index);
        DESTROY_SYSTEM_LOCK(info->extension.overlap_sorting_lock);

        replacement_count = info->extension.replacements.element_count;
        replacement_array = info->extension.replacements.array;
        for (replacement_num = 0; replacement_num < replacement_count;
             ++replacement_num)
          {
            value_remove_reference_with_reference_cluster(
                    replacement_array[replacement_num].new_target, the_jumper,
                    cluster);
          }
        free(replacement_array);

        destroy_string_index(info->extension.replacement_index);
        DESTROY_SYSTEM_LOCK(info->extension.replacement_lock);
        value_remove_reference_with_reference_cluster(info->extension.base,
                                                      the_jumper, cluster);
      }

    DESTROY_SYSTEM_LOCK(info->lock);

    free(info);
  }

static verdict map_info_bubble_value_keys(map_info *info, size_t position,
        size_t to_move, value *key, value *target)
  {
    if (position != to_move + 1)
      {
        verdict final_verdict;

        final_verdict = MISSION_ACCOMPLISHED;

        while (to_move >= position)
          {
            value *local_key;
            verdict the_verdict;

            local_key = info->value_key_keys.array[to_move - 1];
            info->value_key_keys.array[to_move] = local_key;

            info->value_key_targets.array[to_move] =
                    info->value_key_targets.array[to_move - 1];

            the_verdict = update_map_indexes(info, local_key, to_move + 1);
            if (the_verdict != MISSION_ACCOMPLISHED)
                final_verdict = the_verdict;

            --to_move;
          }

        info->value_key_keys.array[position - 1] = key;
        info->value_key_targets.array[position - 1] = target;

        if (final_verdict != MISSION_ACCOMPLISHED)
            return final_verdict;

        final_verdict = update_map_indexes(info, key, position);
        if (final_verdict != MISSION_ACCOMPLISHED)
            return final_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

static verdict initialize_extension_info(map_info *info,
        size_t initial_overlap_space, size_t initial_replacement_space)
  {
    verdict the_verdict;

    the_verdict =
            size_t_aa_init(&(info->extension.overlaps), initial_overlap_space);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    info->extension.overlap_index = create_string_index();
    if (info->extension.overlap_index == NULL)
      {
        free(info->extension.overlaps.array);
        return MISSION_FAILED;
      }

    INITIALIZE_SYSTEM_LOCK(info->extension.overlap_sorting_lock,
            destroy_string_index(info->extension.overlap_index);
            free(info->extension.overlaps.array);
            return MISSION_FAILED);

    the_verdict = map_target_replacement_aa_init(
            &(info->extension.replacements), initial_replacement_space);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        DESTROY_SYSTEM_LOCK(info->extension.overlap_sorting_lock);
        destroy_string_index(info->extension.overlap_index);
        free(info->extension.overlaps.array);
        return the_verdict;
      }

    info->extension.replacement_index = create_string_index();
    if (info->extension.replacement_index == NULL)
      {
        free(info->extension.replacements.array);
        DESTROY_SYSTEM_LOCK(info->extension.overlap_sorting_lock);
        destroy_string_index(info->extension.overlap_index);
        free(info->extension.overlaps.array);
        return MISSION_FAILED;
      }

    info->extension.replacements_sorted = FALSE;

    INITIALIZE_SYSTEM_LOCK(info->extension.replacement_lock,
            destroy_string_index(info->extension.replacement_index);
            free(info->extension.replacements.array);
            DESTROY_SYSTEM_LOCK(info->extension.overlap_sorting_lock);
            destroy_string_index(info->extension.overlap_index);
            free(info->extension.overlaps.array);
            return MISSION_FAILED);

    return MISSION_ACCOMPLISHED;
  }

static boolean might_hit_map_type_key(map_info *info, value *test_key,
                                      jumper *the_jumper)
  {
    size_t filter_count;
    size_t filter_num;

    assert(info != NULL);
    assert(test_key != NULL);
    assert(the_jumper != NULL);

    filter_count = info->type_key_keys.element_count;
    for (filter_num = 0; filter_num < filter_count; ++filter_num)
      {
        boolean doubt;
        boolean is_in;

        is_in = value_is_in_type(test_key,
                info->type_key_keys.array[filter_num], &doubt, NULL, NULL,
                the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return TRUE;
        if (doubt || is_in)
            return TRUE;
      }

    return FALSE;
  }

static void add_intervalue_reference(value *from, value *to)
  {
    reference_cluster *to_cluster;
    boolean add_cluster_reference;

    assert(from != NULL);
    assert(to != NULL);

#ifndef NDEBUG
    GRAB_SYSTEM_LOCK(from->lock);
    assert(from->reference_count - from->no_cluster_reference_count == 1);
    RELEASE_SYSTEM_LOCK(from->lock);
#endif /* !NDEBUG */

    to_cluster = value_reference_cluster(to);
    add_cluster_reference = FALSE;

    GRAB_SYSTEM_LOCK(from->lock);
    if (from->reference_cluster == NULL)
      {
        assert(from->cluster_use_count == 0);
        if (to_cluster != NULL)
          {
            from->reference_cluster = to_cluster;
            add_cluster_reference = TRUE;
            from->cluster_use_count = 1;
          }
      }
    else
      {
        if (from->reference_cluster == to_cluster)
          {
            if (from->cluster_use_count == 0)
                add_cluster_reference = TRUE;
            ++(from->cluster_use_count);
          }
      }
    RELEASE_SYSTEM_LOCK(from->lock);

    if (add_cluster_reference)
        reference_cluster_add_reference(from->reference_cluster);

    value_add_reference_with_reference_cluster(to, from->reference_cluster);
  }

static void add_map_info_to_value_reference(map_info *from, value *to)
  {
    reference_cluster *to_cluster;

    assert(from != NULL);
    assert(to != NULL);

    to_cluster = value_reference_cluster(to);

    GRAB_SYSTEM_LOCK(from->lock);
    if (from->reference_cluster == NULL)
      {
        assert(from->cluster_use_count == 0);
        if (to_cluster != NULL)
          {
            from->reference_cluster = to_cluster;
            from->cluster_use_count = 1;
          }
      }
    else
      {
        if (from->reference_cluster == to_cluster)
            ++(from->cluster_use_count);
      }
    RELEASE_SYSTEM_LOCK(from->lock);

    value_add_reference_with_reference_cluster(to, from->reference_cluster);
  }

static void remove_intervalue_reference(value *from, value *to,
                                        jumper *the_jumper)
  {
    assert(from != NULL);
    assert(to != NULL);

#ifndef NDEBUG
    GRAB_SYSTEM_LOCK(from->lock);
    assert(from->reference_count - from->no_cluster_reference_count == 1);
    RELEASE_SYSTEM_LOCK(from->lock);
#endif /* !NDEBUG */

    assert((from->reference_cluster != NULL) ||
           (value_reference_cluster(to) == NULL));

    if ((from->reference_cluster != NULL) &&
        (from->reference_cluster == value_reference_cluster(to)))
      {
        assert(from->cluster_use_count > 0);
        --(from->cluster_use_count);
        if (from->cluster_use_count == 0)
          {
            reference_cluster_remove_reference(from->reference_cluster,
                                               the_jumper);
          }
      }

    value_remove_reference_with_reference_cluster(to, the_jumper,
                                                  from->reference_cluster);
  }

static void remove_map_info_to_value_reference(map_info *from, value *to,
                                               jumper *the_jumper)
  {
    assert(from != NULL);
    assert(to != NULL);

    assert((from->reference_cluster != NULL) ||
           (value_reference_cluster(to) == NULL));

    if ((from->reference_cluster != NULL) &&
        (from->reference_cluster == value_reference_cluster(to)))
      {
        assert(from->cluster_use_count > 0);
        --(from->cluster_use_count);
      }

    value_remove_reference_with_reference_cluster(to, the_jumper,
                                                  from->reference_cluster);
  }

static void add_map_info_to_type_reference(map_info *from, type *to)
  {
    reference_cluster *to_cluster;

    assert(from != NULL);
    assert(to != NULL);

    to_cluster = type_reference_cluster(to);

    GRAB_SYSTEM_LOCK(from->lock);
    if (from->reference_cluster == NULL)
      {
        assert(from->cluster_use_count == 0);
        if (to_cluster != NULL)
          {
            from->reference_cluster = to_cluster;
            from->cluster_use_count = 1;
          }
      }
    else
      {
        if (from->reference_cluster == to_cluster)
            ++(from->cluster_use_count);
      }
    RELEASE_SYSTEM_LOCK(from->lock);

    type_add_reference_with_reference_cluster(to, from->reference_cluster);
  }

static void remove_map_info_to_type_reference(map_info *from, type *to,
                                              jumper *the_jumper)
  {
    assert(from != NULL);
    assert(to != NULL);

    assert((from->reference_cluster != NULL) ||
           (type_reference_cluster(to) == NULL));

    if ((from->reference_cluster != NULL) &&
        (from->reference_cluster == type_reference_cluster(to)))
      {
        assert(from->cluster_use_count > 0);
        --(from->cluster_use_count);
      }

    type_remove_reference_with_reference_cluster(to, the_jumper,
                                                 from->reference_cluster);
  }

static void value_remove_reference_skip_cluster(value *the_value,
                                                jumper *the_jumper)
  {
    value *the_extension;
    size_t new_reference_count;

    assert(the_value != NULL);
    assert_is_malloced_block_with_exact_size(the_value, sizeof(value));

    if (the_value->kind == VK_MAP)
      {
        GRAB_SYSTEM_LOCK(the_value->u.map.lock);
        the_extension = the_value->u.map.first_extension;
        if (the_extension != NULL)
          {
            GRAB_SYSTEM_LOCK(the_extension->lock);
            if (the_extension->destructing)
              {
                RELEASE_SYSTEM_LOCK(the_extension->lock);
                the_extension = NULL;
              }
            else
              {
                assert(the_extension->reference_count > 0);
                ++(the_extension->reference_count);
                ++(the_extension->no_cluster_reference_count);
                assert(the_extension->reference_count >=
                       the_extension->no_cluster_reference_count);
                RELEASE_SYSTEM_LOCK(the_extension->lock);
              }
          }
        RELEASE_SYSTEM_LOCK(the_value->u.map.lock);
      }
    else
      {
        the_extension = NULL;
      }

    GRAB_SYSTEM_LOCK(the_value->lock);
    assert(the_value->reference_count > 0);
    --(the_value->reference_count);
    new_reference_count = the_value->reference_count;
    if (new_reference_count == 0)
        the_value->destructing = TRUE;
    assert(the_value->no_cluster_reference_count > 0);
    --(the_value->no_cluster_reference_count);
    assert(the_value->reference_count >=
           the_value->no_cluster_reference_count);
    RELEASE_SYSTEM_LOCK(the_value->lock);

    if ((new_reference_count == 1) && (the_extension != NULL) &&
        validator_is_valid(the_value->u.map.map_info->all_keys_validator) &&
        (the_jumper != NULL))
      {
        GRAB_SYSTEM_LOCK(the_extension->u.map.lock);
        if (the_extension->u.map.map_info->reference_count == 1)
            merge_extension(the_extension, the_value, the_jumper);
        RELEASE_SYSTEM_LOCK(the_extension->u.map.lock);
      }

    if (the_extension != NULL)
        value_remove_reference_skip_cluster(the_extension, the_jumper);

    if (new_reference_count == 0)
      {
        ++(the_value->reference_count);
        delete_value(the_value, the_jumper);
      }
  }

static void update_value_cluster_for_map_info(value *the_value,
        reference_cluster *cluster, jumper *the_jumper)
  {
    map_info *info;
    reference_cluster *old_cluster;
    reference_cluster *new_cluster;

    assert(the_value != NULL);

#ifndef NDEBUG
    GRAB_SYSTEM_LOCK(the_value->lock);
    assert(the_value->reference_count - the_value->no_cluster_reference_count
           == 1);
    RELEASE_SYSTEM_LOCK(the_value->lock);
#endif /* !NDEBUG */

    assert(the_value->kind == VK_MAP);

    GRAB_SYSTEM_LOCK(the_value->u.map.lock);
    info = the_value->u.map.map_info;
    assert(info != NULL);
    RELEASE_SYSTEM_LOCK(the_value->u.map.lock);

    GRAB_SYSTEM_LOCK(the_value->lock);
    assert(info->cluster_use_count >= info->negated_cluster_use_count);
    old_cluster = ((the_value->cluster_use_count == 0) ? NULL :
                   the_value->reference_cluster);
    new_cluster =
            ((info->cluster_use_count - info->negated_cluster_use_count == 0) ?
             NULL : info->reference_cluster);
    the_value->reference_cluster = info->reference_cluster;
    the_value->cluster_use_count =
            info->cluster_use_count - info->negated_cluster_use_count;
    if ((old_cluster != new_cluster) && (new_cluster != NULL) &&
        (new_cluster != cluster))
      {
        reference_cluster_add_reference(new_cluster);
      }
    RELEASE_SYSTEM_LOCK(the_value->lock);

    if ((old_cluster != new_cluster) && (old_cluster != NULL) &&
        (old_cluster != cluster))
      {
        reference_cluster_remove_reference(old_cluster, the_jumper);
      }
  }

static void map_info_set_extension_base(map_info *info, value *new_base)
  {
    reference_cluster *to_cluster;

    assert(info != NULL);
    assert(new_base != NULL);

    to_cluster = value_reference_cluster(new_base);

    GRAB_SYSTEM_LOCK(info->lock);
    if (info->reference_cluster == NULL)
      {
        assert(info->cluster_use_count == 0);
        if (to_cluster != NULL)
          {
            info->reference_cluster = to_cluster;
            info->cluster_use_count = new_base->cluster_use_count;
          }
      }
    else
      {
        if (info->reference_cluster == to_cluster)
            info->cluster_use_count += new_base->cluster_use_count;
      }
    RELEASE_SYSTEM_LOCK(info->lock);

    value_add_reference_with_reference_cluster(new_base,
                                               info->reference_cluster);
    info->extension.base = new_base;
  }
