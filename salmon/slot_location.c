/* file "slot_location.c" */

/*
 *  This file contains the implementation of the slot_location module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "slot_location.h"
#include "variable_instance.h"
#include "lookup_actual_arguments.h"
#include "tagalong_key.h"
#include "source_location.h"
#include "jumper.h"
#include "unicode.h"
#include "execute.h"
#include "validator.h"
#include "reference_cluster.h"
#include "platform_dependent.h"


struct slot_location
  {
    slot_location_kind kind;
    union
      {
        variable_instance *variable;
        struct
          {
            slot_location *base;
            lookup_actual_arguments *actuals;
          } lookup;
        struct
          {
            slot_location *base;
            char *field_name;
          } field;
        struct
          {
            slot_location *base;
            tagalong_key *key;
          } tagalong_field;
        struct
          {
            value *base;
            const char *operation_name;
            value *argument0;
            const char *argument_name0;
            value *argument1;
            const char *argument_name1;
            value *argument_for_overload_base;
          } call;
        struct
          {
            slot_location *base;
          } pass;
      } u;
    boolean slippery;
    value *overload_base;
    DECLARE_SYSTEM_LOCK(cache_lock);
    type *lower_read_type;
    type *upper_read_type;
    type *lower_write_type;
    type *upper_write_type;
    variable_instance *base_variable;
    reference_cluster *reference_cluster;
    validator *validator;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
  };

typedef struct
  {
    slot_location *slot_location;
    const source_location *location;
    jumper *jumper;
    void (*update_function)(void *update_data, type *in_lower_read,
            type *in_upper_read, type *in_lower_write, type *in_upper_write,
            variable_instance *in_base_variable, type **out_lower_read,
            type **out_upper_read, type **out_lower_write,
            type **out_upper_write, variable_instance **out_base_variable);
    void *update_data;
  } slot_update_data;


static void compute_dereference_type_bounds(slot_location *the_slot_location,
        void (*update_function)(void *update_data, type *in_lower_read,
                type *in_upper_read, type *in_lower_write,
                type *in_upper_write, variable_instance *in_base_variable,
                type **out_lower_read, type **out_upper_read,
                type **out_lower_write, type **out_upper_write,
                variable_instance **out_base_variable), void *update_data,
        type **lower_read, type **upper_read, type **lower_write,
        type **upper_write, variable_instance **base_variable,
        reference_cluster *cluster, const source_location *location,
        jumper *the_jumper);
static void base_update_function(void *update_data, type *in_lower_read,
        type *in_upper_read, type *in_lower_write, type *in_upper_write,
        variable_instance *in_base_variable, type **out_lower_read,
        type **out_upper_read, type **out_lower_write, type **out_upper_write,
        variable_instance **out_base_variable);
static void slot_update_function(void *update_data, type *in_lower_read,
        type *in_upper_read, type *in_lower_write, type *in_upper_write,
        variable_instance *in_base_variable, type **out_lower_read,
        type **out_upper_read, type **out_lower_write, type **out_upper_write,
        variable_instance **out_base_variable);
static slot_location *create_empty_slot_location(void);


extern slot_location *create_variable_slot_location(
        variable_instance *instance,
        const source_location *the_source_location, jumper *the_jumper)
  {
    slot_location *result;

    assert(instance != NULL);
    assert(the_jumper != NULL);

    result = create_empty_slot_location();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result->kind = SLK_VARIABLE;
    result->u.variable = instance;
    result->slippery = FALSE;
    result->overload_base = NULL;
    result->reference_count = 1;

    result->lower_read_type = NULL;
    result->upper_read_type = NULL;
    result->lower_write_type = NULL;
    result->upper_write_type = NULL;
    result->base_variable = NULL;

    result->reference_cluster = variable_instance_reference_cluster(instance);
    if (result->reference_cluster != NULL)
        reference_cluster_add_reference(result->reference_cluster);
    variable_instance_add_reference_with_cluster(instance,
                                                 result->reference_cluster);

    result->validator = get_trivial_validator();
    assert(result->validator != NULL);

    result->validator = validator_add_instance(result->validator,
            variable_instance_instance(instance));
    if (result->validator == NULL)
      {
        slot_location_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern slot_location *create_lookup_slot_location(slot_location *base_slot,
        lookup_actual_arguments *actuals, value *overload_base,
        const source_location *the_source_location, jumper *the_jumper)
  {
    slot_location *result;
    reference_cluster *actuals_cluster;

    assert(base_slot != NULL);
    assert(actuals != NULL);
    assert(the_jumper != NULL);

    if (overload_base != NULL)
      {
        assert((get_value_kind(overload_base) == VK_ROUTINE) ||
               (get_value_kind(overload_base) == VK_ROUTINE_CHAIN));
      }

    result = create_empty_slot_location();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        delete_lookup_actual_arguments(actuals, the_jumper);
        return NULL;
      }

    result->kind = SLK_LOOKUP;
    result->u.lookup.base = base_slot;
    result->u.lookup.actuals = actuals;
    result->slippery =
            (slot_location_is_slippery(base_slot) ||
             lookup_actual_arguments_are_slippery(actuals) ||
             ((overload_base != NULL) && value_is_slippery(overload_base)));
    result->overload_base = overload_base;
    result->reference_count = 1;

    result->lower_read_type = NULL;
    result->upper_read_type = NULL;
    result->lower_write_type = NULL;
    result->upper_write_type = NULL;
    result->base_variable = NULL;

    result->reference_cluster = slot_location_reference_cluster(base_slot);
    if ((result->reference_cluster == NULL) && (overload_base != NULL))
        result->reference_cluster = value_reference_cluster(overload_base);
    actuals_cluster = lookup_actual_arguments_reference_cluster(actuals);
    if (result->reference_cluster == NULL)
        result->reference_cluster = actuals_cluster;
    if (result->reference_cluster != NULL)
        reference_cluster_add_reference(result->reference_cluster);
    slot_location_add_reference_with_cluster(base_slot,
                                             result->reference_cluster);
    if (overload_base != NULL)
      {
        value_add_reference_with_reference_cluster(overload_base,
                                                   result->reference_cluster);
      }
    if ((actuals_cluster != NULL) &&
        (actuals_cluster != result->reference_cluster))
      {
        reference_cluster_add_reference(actuals_cluster);
      }

    result->validator = get_trivial_validator();
    assert(result->validator != NULL);

    result->validator = validator_add_validator(result->validator,
            slot_location_validator(base_slot));
    if (result->validator == NULL)
      {
        slot_location_remove_reference(result, the_jumper);
        return NULL;
      }

    result->validator = validator_add_validator(result->validator,
            lookup_actual_arguments_validator(actuals));
    if (result->validator == NULL)
      {
        slot_location_remove_reference(result, the_jumper);
        return NULL;
      }

    if (overload_base != NULL)
      {
        result->validator = validator_add_validator(result->validator,
                value_validator(overload_base));
        if (result->validator == NULL)
          {
            slot_location_remove_reference(result, the_jumper);
            return NULL;
          }
      }

    return result;
  }

extern slot_location *create_field_slot_location(slot_location *base_slot,
        const char *field_name, value *overload_base,
        const source_location *the_source_location, jumper *the_jumper)
  {
    slot_location *result;
    char *name_copy;

    assert(base_slot != NULL);
    assert(field_name != NULL);
    assert(the_jumper != NULL);

    if (overload_base != NULL)
      {
        assert((get_value_kind(overload_base) == VK_ROUTINE) ||
               (get_value_kind(overload_base) == VK_ROUTINE_CHAIN));
      }

    result = create_empty_slot_location();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    name_copy = MALLOC_ARRAY(char, strlen(field_name) + 1);
    if (name_copy == NULL)
      {
        jumper_do_abort(the_jumper);
        DESTROY_SYSTEM_LOCK(result->reference_lock);
        DESTROY_SYSTEM_LOCK(result->cache_lock);
        free(result);
        return NULL;
      }
    strcpy(name_copy, field_name);

    result->kind = SLK_FIELD;
    result->u.field.base = base_slot;
    result->u.field.field_name = name_copy;
    result->slippery =
            (slot_location_is_slippery(base_slot) ||
             ((overload_base != NULL) && value_is_slippery(overload_base)));
    result->overload_base = overload_base;
    result->reference_count = 1;

    result->lower_read_type = NULL;
    result->upper_read_type = NULL;
    result->lower_write_type = NULL;
    result->upper_write_type = NULL;
    result->base_variable = NULL;

    result->reference_cluster = slot_location_reference_cluster(base_slot);
    if ((result->reference_cluster == NULL) && (overload_base != NULL))
        result->reference_cluster = value_reference_cluster(overload_base);
    if (result->reference_cluster != NULL)
        reference_cluster_add_reference(result->reference_cluster);
    slot_location_add_reference_with_cluster(base_slot,
                                             result->reference_cluster);
    if (overload_base != NULL)
      {
        value_add_reference_with_reference_cluster(overload_base,
                                                   result->reference_cluster);
      }

    result->validator = get_trivial_validator();
    assert(result->validator != NULL);

    result->validator = validator_add_validator(result->validator,
            slot_location_validator(base_slot));
    if (result->validator == NULL)
      {
        slot_location_remove_reference(result, the_jumper);
        return NULL;
      }

    if (overload_base != NULL)
      {
        result->validator = validator_add_validator(result->validator,
                value_validator(overload_base));
        if (result->validator == NULL)
          {
            slot_location_remove_reference(result, the_jumper);
            return NULL;
          }
      }

    return result;
  }

extern slot_location *create_tagalong_field_slot_location(
        slot_location *base_slot, tagalong_key *key,
        const source_location *the_source_location, jumper *the_jumper)
  {
    slot_location *result;

    assert(base_slot != NULL);
    assert(key != NULL);
    assert(the_jumper != NULL);

    result = create_empty_slot_location();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result->kind = SLK_TAGALONG;
    result->u.tagalong_field.base = base_slot;
    result->u.tagalong_field.key = key;
    result->slippery = slot_location_is_slippery(base_slot);
    result->overload_base = NULL;
    result->reference_count = 1;

    result->lower_read_type = NULL;
    result->upper_read_type = NULL;
    result->lower_write_type = NULL;
    result->upper_write_type = NULL;
    result->base_variable = NULL;

    result->reference_cluster = slot_location_reference_cluster(base_slot);
    if (result->reference_cluster == NULL)
        result->reference_cluster = tagalong_key_reference_cluster(key);
    if (result->reference_cluster != NULL)
        reference_cluster_add_reference(result->reference_cluster);
    slot_location_add_reference_with_cluster(base_slot,
                                             result->reference_cluster);
    tagalong_key_add_reference_with_cluster(key, result->reference_cluster);

    result->validator = get_trivial_validator();
    assert(result->validator != NULL);

    result->validator = validator_add_validator(result->validator,
            slot_location_validator(base_slot));
    if (result->validator == NULL)
      {
        slot_location_remove_reference(result, the_jumper);
        return NULL;
      }

    result->validator = validator_add_instance(result->validator,
                                               tagalong_key_instance(key));
    if (result->validator == NULL)
      {
        slot_location_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern slot_location *create_call_slot_location(value *base,
        const char *operation_name, value *argument0,
        const char *argument_name0, value *argument1,
        const char *argument_name1, value *overload_base,
        value *argument_for_overload_base,
        const source_location *the_source_location, jumper *the_jumper)
  {
    slot_location *result;

    assert(base != NULL);
    assert(operation_name != NULL);
    assert(the_jumper != NULL);

    assert((get_value_kind(base) == VK_ROUTINE) ||
           (get_value_kind(base) == VK_ROUTINE_CHAIN));
    if (overload_base != NULL)
      {
        assert((get_value_kind(overload_base) == VK_ROUTINE) ||
               (get_value_kind(overload_base) == VK_ROUTINE_CHAIN));
      }

    result = create_empty_slot_location();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result->kind = SLK_CALL;
    result->u.call.base = base;
    result->u.call.operation_name = operation_name;
    result->u.call.argument0 = argument0;
    result->u.call.argument_name0 = argument_name0;
    result->u.call.argument1 = argument1;
    result->u.call.argument_name1 = argument_name1;
    result->u.call.argument_for_overload_base = argument_for_overload_base;
    result->slippery =
            (value_is_slippery(base) ||
             ((argument0 != NULL) && value_is_slippery(argument0)) ||
             ((argument1 != NULL) && value_is_slippery(argument1)) ||
             ((overload_base != NULL) && value_is_slippery(overload_base)) ||
             ((argument_for_overload_base != NULL) &&
              value_is_slippery(argument_for_overload_base)));
    result->overload_base = overload_base;
    result->reference_count = 1;

    result->lower_read_type = NULL;
    result->upper_read_type = NULL;
    result->lower_write_type = NULL;
    result->upper_write_type = NULL;
    result->base_variable = NULL;

    result->reference_cluster = value_reference_cluster(base);
    if ((result->reference_cluster == NULL) && (argument0 != NULL))
        result->reference_cluster = value_reference_cluster(argument0);
    if ((result->reference_cluster == NULL) && (argument1 != NULL))
        result->reference_cluster = value_reference_cluster(argument1);
    if ((result->reference_cluster == NULL) && (overload_base != NULL))
        result->reference_cluster = value_reference_cluster(overload_base);
    if ((result->reference_cluster == NULL) &&
        (argument_for_overload_base != NULL))
      {
        result->reference_cluster =
                value_reference_cluster(argument_for_overload_base);
      }
    if (result->reference_cluster != NULL)
        reference_cluster_add_reference(result->reference_cluster);
    value_add_reference_with_reference_cluster(base,
                                               result->reference_cluster);
    if (argument0 != NULL)
      {
        value_add_reference_with_reference_cluster(argument0,
                                                   result->reference_cluster);
      }
    if (argument1 != NULL)
      {
        value_add_reference_with_reference_cluster(argument1,
                                                   result->reference_cluster);
      }
    if (overload_base != NULL)
      {
        value_add_reference_with_reference_cluster(overload_base,
                                                   result->reference_cluster);
      }
    if (argument_for_overload_base != NULL)
      {
        value_add_reference_with_reference_cluster(argument_for_overload_base,
                                                   result->reference_cluster);
      }

    result->validator = get_trivial_validator();
    assert(result->validator != NULL);

    result->validator =
            validator_add_validator(result->validator, value_validator(base));
    if (result->validator == NULL)
      {
        slot_location_remove_reference(result, the_jumper);
        return NULL;
      }

    if (argument0 != NULL)
      {
        result->validator = validator_add_validator(result->validator,
                value_validator(argument0));
        if (result->validator == NULL)
          {
            slot_location_remove_reference(result, the_jumper);
            return NULL;
          }
      }

    if (argument1 != NULL)
      {
        result->validator = validator_add_validator(result->validator,
                value_validator(argument1));
        if (result->validator == NULL)
          {
            slot_location_remove_reference(result, the_jumper);
            return NULL;
          }
      }

    if (overload_base != NULL)
      {
        result->validator = validator_add_validator(result->validator,
                value_validator(overload_base));
        if (result->validator == NULL)
          {
            slot_location_remove_reference(result, the_jumper);
            return NULL;
          }
      }

    if (argument_for_overload_base != NULL)
      {
        result->validator = validator_add_validator(result->validator,
                value_validator(argument_for_overload_base));
        if (result->validator == NULL)
          {
            slot_location_remove_reference(result, the_jumper);
            return NULL;
          }
      }

    return result;
  }

extern slot_location *create_pass_slot_location(slot_location *base_slot,
        value *overload_base, const source_location *the_source_location,
        jumper *the_jumper)
  {
    slot_location *result;

    assert(base_slot != NULL);
    assert(the_jumper != NULL);

    if (overload_base != NULL)
      {
        assert((get_value_kind(overload_base) == VK_ROUTINE) ||
               (get_value_kind(overload_base) == VK_ROUTINE_CHAIN));
      }

    result = create_empty_slot_location();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result->kind = SLK_PASS;
    result->u.pass.base = base_slot;
    result->slippery =
            (slot_location_is_slippery(base_slot) ||
             ((overload_base != NULL) && value_is_slippery(overload_base)));
    result->overload_base = overload_base;
    result->reference_count = 1;

    result->lower_read_type = NULL;
    result->upper_read_type = NULL;
    result->lower_write_type = NULL;
    result->upper_write_type = NULL;
    result->base_variable = NULL;

    result->reference_cluster = slot_location_reference_cluster(base_slot);
    if ((result->reference_cluster == NULL) && (overload_base != NULL))
        result->reference_cluster = value_reference_cluster(overload_base);
    if (result->reference_cluster != NULL)
        reference_cluster_add_reference(result->reference_cluster);
    slot_location_add_reference_with_cluster(base_slot,
                                             result->reference_cluster);
    if (overload_base != NULL)
      {
        value_add_reference_with_reference_cluster(overload_base,
                                                   result->reference_cluster);
      }

    result->validator = get_trivial_validator();
    assert(result->validator != NULL);

    result->validator = validator_add_validator(result->validator,
            slot_location_validator(base_slot));
    if (result->validator == NULL)
      {
        slot_location_remove_reference(result, the_jumper);
        return NULL;
      }

    if (overload_base != NULL)
      {
        result->validator = validator_add_validator(result->validator,
                value_validator(overload_base));
        if (result->validator == NULL)
          {
            slot_location_remove_reference(result, the_jumper);
            return NULL;
          }
      }

    return result;
  }

extern void slot_location_add_reference(slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    GRAB_SYSTEM_LOCK(the_slot_location->reference_lock);
    assert(the_slot_location->reference_count > 0);
    ++(the_slot_location->reference_count);
    RELEASE_SYSTEM_LOCK(the_slot_location->reference_lock);

    if (the_slot_location->reference_cluster != NULL)
        reference_cluster_add_reference(the_slot_location->reference_cluster);
  }

extern void slot_location_remove_reference(slot_location *the_slot_location,
                                           jumper *the_jumper)
  {
    return slot_location_remove_reference_with_cluster(the_slot_location, NULL,
                                                       the_jumper);
  }

extern void slot_location_add_reference_with_cluster(
        slot_location *the_slot_location, reference_cluster *cluster)
  {
    assert(the_slot_location != NULL);

    GRAB_SYSTEM_LOCK(the_slot_location->reference_lock);
    assert(the_slot_location->reference_count > 0);
    ++(the_slot_location->reference_count);
    RELEASE_SYSTEM_LOCK(the_slot_location->reference_lock);

    if ((the_slot_location->reference_cluster != NULL) &&
        (the_slot_location->reference_cluster != cluster))
      {
        reference_cluster_add_reference(the_slot_location->reference_cluster);
      }
  }

extern void slot_location_remove_reference_with_cluster(
        slot_location *the_slot_location, reference_cluster *cluster,
        jumper *the_jumper)
  {
    size_t new_reference_count;

    assert(the_slot_location != NULL);

    GRAB_SYSTEM_LOCK(the_slot_location->reference_lock);
    assert(the_slot_location->reference_count > 0);
    --(the_slot_location->reference_count);
    new_reference_count = the_slot_location->reference_count;
    RELEASE_SYSTEM_LOCK(the_slot_location->reference_lock);

    if ((the_slot_location->reference_cluster != NULL) &&
        (the_slot_location->reference_cluster != cluster))
      {
        reference_cluster_remove_reference(
                the_slot_location->reference_cluster, the_jumper);
      }

    if (new_reference_count > 0)
        return;

    if (the_slot_location->validator != NULL)
        validator_remove_reference(the_slot_location->validator);

    if (the_slot_location->overload_base != NULL)
      {
        value_remove_reference_with_reference_cluster(
                the_slot_location->overload_base, the_jumper,
                the_slot_location->reference_cluster);
      }

    switch (the_slot_location->kind)
      {
        case SLK_VARIABLE:
          {
            variable_instance_remove_reference_with_cluster(
                    the_slot_location->u.variable, the_jumper,
                    the_slot_location->reference_cluster);
            break;
          }
        case SLK_LOOKUP:
          {
            reference_cluster *actuals_cluster;

            actuals_cluster = lookup_actual_arguments_reference_cluster(
                    the_slot_location->u.lookup.actuals);
            if ((actuals_cluster != NULL) &&
                (actuals_cluster != the_slot_location->reference_cluster))
              {
                reference_cluster_remove_reference(actuals_cluster,
                                                   the_jumper);
              }

            slot_location_remove_reference_with_cluster(
                    the_slot_location->u.lookup.base,
                    the_slot_location->reference_cluster, the_jumper);
            delete_lookup_actual_arguments(
                    the_slot_location->u.lookup.actuals, the_jumper);
            break;
          }
        case SLK_FIELD:
          {
            slot_location_remove_reference_with_cluster(
                    the_slot_location->u.field.base,
                    the_slot_location->reference_cluster, the_jumper);
            free(the_slot_location->u.field.field_name);
            break;
          }
        case SLK_TAGALONG:
          {
            slot_location_remove_reference_with_cluster(
                    the_slot_location->u.tagalong_field.base,
                    the_slot_location->reference_cluster, the_jumper);
            tagalong_key_remove_reference_with_cluster(
                    the_slot_location->u.tagalong_field.key, the_jumper,
                    the_slot_location->reference_cluster);
            break;
          }
        case SLK_CALL:
          {
            value_remove_reference_with_reference_cluster(
                    the_slot_location->u.call.base, the_jumper,
                    the_slot_location->reference_cluster);
            if (the_slot_location->u.call.argument0 != NULL)
              {
                value_remove_reference_with_reference_cluster(
                        the_slot_location->u.call.argument0, the_jumper,
                        the_slot_location->reference_cluster);
              }
            if (the_slot_location->u.call.argument1 != NULL)
              {
                value_remove_reference_with_reference_cluster(
                        the_slot_location->u.call.argument1, the_jumper,
                        the_slot_location->reference_cluster);
              }
            if (the_slot_location->u.call.argument_for_overload_base != NULL)
              {
                value_remove_reference_with_reference_cluster(
                        the_slot_location->u.call.argument_for_overload_base,
                        the_jumper, the_slot_location->reference_cluster);
              }
            break;
          }
        case SLK_PASS:
          {
            slot_location_remove_reference_with_cluster(
                    the_slot_location->u.pass.base,
                    the_slot_location->reference_cluster, the_jumper);
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    if (the_slot_location->lower_read_type != NULL)
        type_remove_reference(the_slot_location->lower_read_type, the_jumper);
    if (the_slot_location->upper_read_type != NULL)
        type_remove_reference(the_slot_location->upper_read_type, the_jumper);
    if (the_slot_location->lower_write_type != NULL)
        type_remove_reference(the_slot_location->lower_write_type, the_jumper);
    if (the_slot_location->upper_write_type != NULL)
        type_remove_reference(the_slot_location->upper_write_type, the_jumper);
    if (the_slot_location->base_variable != NULL)
      {
        variable_instance_remove_reference_with_cluster(
                the_slot_location->base_variable, the_jumper,
                the_slot_location->reference_cluster);
      }

    DESTROY_SYSTEM_LOCK(the_slot_location->reference_lock);
    DESTROY_SYSTEM_LOCK(the_slot_location->cache_lock);

    free(the_slot_location);
  }

extern reference_cluster *slot_location_reference_cluster(
        slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    return the_slot_location->reference_cluster;
  }

extern slot_location_kind get_slot_location_kind(
        slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    return the_slot_location->kind;
  }

extern variable_instance *variable_slot_location_variable(
        slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_VARIABLE);
    return the_slot_location->u.variable;
  }

extern slot_location *lookup_slot_location_base(
        slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_LOOKUP);
    return the_slot_location->u.lookup.base;
  }

extern lookup_actual_arguments *lookup_slot_location_actuals(
        slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_LOOKUP);
    return the_slot_location->u.lookup.actuals;
  }

extern slot_location *field_slot_location_base(
        slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_FIELD);
    return the_slot_location->u.field.base;
  }

extern const char *field_slot_location_field_name(
        slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_FIELD);
    return the_slot_location->u.field.field_name;
  }

extern slot_location *tagalong_slot_location_base(
        slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_TAGALONG);
    return the_slot_location->u.tagalong_field.base;
  }

extern tagalong_key *tagalong_slot_location_key(
        slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_TAGALONG);
    return the_slot_location->u.tagalong_field.key;
  }

extern value *call_slot_location_base(slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_CALL);
    return the_slot_location->u.call.base;
  }

extern const char *call_slot_location_operation_name(
        slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_CALL);
    return the_slot_location->u.call.operation_name;
  }

extern value *call_slot_location_argument0(slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_CALL);
    return the_slot_location->u.call.argument0;
  }

extern const char *call_slot_location_argument_name0(
        slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_CALL);
    return the_slot_location->u.call.argument_name0;
  }

extern value *call_slot_location_argument1(slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_CALL);
    return the_slot_location->u.call.argument1;
  }

extern const char *call_slot_location_argument_name1(
        slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_CALL);
    return the_slot_location->u.call.argument_name1;
  }

extern value *call_slot_location_argument_for_overload_base(
        slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_CALL);
    return the_slot_location->u.call.argument_for_overload_base;
  }

extern slot_location *pass_slot_location_base(slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    assert(the_slot_location->kind == SLK_PASS);
    return the_slot_location->u.pass.base;
  }

extern boolean slot_location_is_valid(slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    return validator_is_valid(the_slot_location->validator);
  }

extern void check_slot_location_validity(slot_location *the_slot_location,
        const source_location *location, jumper *the_jumper)
  {
    assert(the_slot_location != NULL);
    assert(the_jumper != NULL);

    validator_check_validity(the_slot_location->validator, location,
                             the_jumper);
  }

extern validator *slot_location_validator(slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    return the_slot_location->validator;
  }

extern boolean slot_locations_are_equal(slot_location *slot_location1,
        slot_location *slot_location2, boolean *doubt,
        const source_location *location, jumper *the_jumper)
  {
    assert(slot_location1 != NULL);
    assert(slot_location2 != NULL);
    assert(doubt != NULL);
    assert(the_jumper != NULL);

    assert(slot_location_is_valid(slot_location1)); /* VERIFIED */
    assert(slot_location_is_valid(slot_location2)); /* VERIFIED */

    *doubt = FALSE;

    if (slot_location1->kind != slot_location2->kind)
        return FALSE;

    if ((slot_location1->overload_base != NULL) &&
        (slot_location2->overload_base == NULL))
      {
        return FALSE;
      }
    if ((slot_location1->overload_base == NULL) &&
        (slot_location2->overload_base != NULL))
      {
        return FALSE;
      }
    if ((slot_location1->overload_base != NULL) &&
        (slot_location2->overload_base != NULL))
      {
        boolean local_equal;
        boolean local_doubt;

        assert(value_is_valid(slot_location1->overload_base)); /* VERIFIED */
        assert(value_is_valid(slot_location2->overload_base)); /* VERIFIED */
        local_equal = values_are_equal(slot_location1->overload_base,
                slot_location2->overload_base, &local_doubt, location,
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

    switch (slot_location1->kind)
      {
        case SLK_VARIABLE:
          {
            if (slot_location1->u.variable != slot_location2->u.variable)
              {
                *doubt = FALSE;
                return FALSE;
              }
            return TRUE;
          }
        case SLK_LOOKUP:
          {
            boolean local_equal;
            boolean local_doubt;

            assert(slot_location_is_valid(slot_location1->u.lookup.base));
                    /* VERIFIED */
            assert(slot_location_is_valid(slot_location2->u.lookup.base));
                    /* VERIFIED */
            local_equal = slot_locations_are_equal(
                    slot_location1->u.lookup.base,
                    slot_location2->u.lookup.base, &local_doubt, location,
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

            assert(lookup_actual_arguments_is_valid(
                           slot_location1->u.lookup.actuals)); /* VERIFIED */
            assert(lookup_actual_arguments_is_valid(
                           slot_location2->u.lookup.actuals)); /* VERIFIED */
            local_equal = lookup_actual_arguments_are_equal(
                    slot_location1->u.lookup.actuals,
                    slot_location2->u.lookup.actuals, &local_doubt, location,
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

            return TRUE;
          }
        case SLK_FIELD:
          {
            boolean local_equal;
            boolean local_doubt;

            assert(slot_location_is_valid(slot_location1->u.field.base));
                    /* VERIFIED */
            assert(slot_location_is_valid(slot_location2->u.field.base));
                    /* VERIFIED */
            local_equal = slot_locations_are_equal(
                    slot_location1->u.field.base, slot_location2->u.field.base,
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

            if (strcmp(slot_location1->u.field.field_name,
                       slot_location2->u.field.field_name) != 0)
              {
                *doubt = FALSE;
                return FALSE;
              }

            return TRUE;
          }
        case SLK_TAGALONG:
          {
            boolean local_equal;
            boolean local_doubt;

            assert(slot_location_is_valid(
                           slot_location1->u.tagalong_field.base));
                    /* VERIFIED */
            assert(slot_location_is_valid(
                           slot_location2->u.tagalong_field.base));
                    /* VERIFIED */
            local_equal = slot_locations_are_equal(
                    slot_location1->u.tagalong_field.base,
                    slot_location2->u.tagalong_field.base, &local_doubt,
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

            assert(tagalong_key_is_instantiated(
                           slot_location1->u.tagalong_field.key));
                    /* VERIFIED */
            assert(tagalong_key_is_instantiated(
                           slot_location2->u.tagalong_field.key));
                    /* VERIFIED */
            assert(!(tagalong_key_scope_exited(
                             slot_location1->u.tagalong_field.key)));
                    /* VERIFIED */
            assert(!(tagalong_key_scope_exited(
                             slot_location2->u.tagalong_field.key)));
                    /* VERIFIED */
            if (!(tagalong_keys_are_equal(slot_location1->u.tagalong_field.key,
                          slot_location2->u.tagalong_field.key)))
              {
                *doubt = FALSE;
                return FALSE;
              }

            return TRUE;
          }
        case SLK_CALL:
          {
            boolean local_equal;
            boolean local_doubt;

            assert(value_is_valid(slot_location1->u.call.base)); /* VERIFIED */
            assert(value_is_valid(slot_location2->u.call.base)); /* VERIFIED */
            local_equal = values_are_equal(slot_location1->u.call.base,
                    slot_location2->u.call.base, &local_doubt, location,
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

            if (strcmp(slot_location1->u.call.operation_name,
                       slot_location2->u.call.operation_name) != 0)
              {
                *doubt = FALSE;
                return FALSE;
              }

            if (slot_location1->u.call.argument0 != NULL)
              {
                if (slot_location2->u.call.argument0 == NULL)
                  {
                    *doubt = FALSE;
                    return FALSE;
                  }

                assert(value_is_valid(slot_location1->u.call.argument0));
                        /* VERIFIED */
                assert(value_is_valid(slot_location2->u.call.argument0));
                        /* VERIFIED */
                local_equal = values_are_equal(
                        slot_location1->u.call.argument0,
                        slot_location2->u.call.argument0, &local_doubt,
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
            else if (slot_location2->u.call.argument0 != NULL)
              {
                *doubt = FALSE;
                return FALSE;
              }

            if (slot_location1->u.call.argument_name0 != NULL)
              {
                if (slot_location2->u.call.argument_name0 == NULL)
                  {
                    *doubt = FALSE;
                    return FALSE;
                  }

                if (strcmp(slot_location1->u.call.argument_name0,
                           slot_location2->u.call.argument_name0) != 0)
                  {
                    *doubt = FALSE;
                    return FALSE;
                  }
              }
            else if (slot_location2->u.call.argument_name0 != NULL)
              {
                *doubt = FALSE;
                return FALSE;
              }

            if (slot_location1->u.call.argument1 != NULL)
              {
                if (slot_location2->u.call.argument1 == NULL)
                  {
                    *doubt = FALSE;
                    return FALSE;
                  }

                assert(value_is_valid(slot_location1->u.call.argument1));
                        /* VERIFIED */
                assert(value_is_valid(slot_location2->u.call.argument1));
                        /* VERIFIED */
                local_equal = values_are_equal(
                        slot_location1->u.call.argument1,
                        slot_location2->u.call.argument1, &local_doubt,
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
            else if (slot_location2->u.call.argument1 != NULL)
              {
                *doubt = FALSE;
                return FALSE;
              }

            if (slot_location1->u.call.argument_name1 != NULL)
              {
                if (slot_location2->u.call.argument_name1 == NULL)
                  {
                    *doubt = FALSE;
                    return FALSE;
                  }

                if (strcmp(slot_location1->u.call.argument_name1,
                           slot_location2->u.call.argument_name1) != 0)
                  {
                    *doubt = FALSE;
                    return FALSE;
                  }
              }
            else if (slot_location2->u.call.argument_name1 != NULL)
              {
                *doubt = FALSE;
                return FALSE;
              }

            if (slot_location1->u.call.argument_for_overload_base != NULL)
              {
                if (slot_location2->u.call.argument_for_overload_base == NULL)
                  {
                    *doubt = FALSE;
                    return FALSE;
                  }

                assert(value_is_valid(
                        slot_location1->u.call.argument_for_overload_base));
                                /* VERIFIED */
                assert(value_is_valid(
                        slot_location2->u.call.argument_for_overload_base));
                                /* VERIFIED */
                local_equal = values_are_equal(
                        slot_location1->u.call.argument_for_overload_base,
                        slot_location2->u.call.argument_for_overload_base,
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
            else if (slot_location2->u.call.argument_for_overload_base != NULL)
              {
                *doubt = FALSE;
                return FALSE;
              }

            return TRUE;
          }
        case SLK_PASS:
          {
            boolean local_equal;
            boolean local_doubt;

            assert(slot_location_is_valid(slot_location1->u.pass.base));
                    /* VERIFIED */
            assert(slot_location_is_valid(slot_location2->u.pass.base));
                    /* VERIFIED */
            local_equal = slot_locations_are_equal(slot_location1->u.pass.base,
                    slot_location2->u.pass.base, &local_doubt, location,
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

            return TRUE;
          }
        default:
          {
            assert(FALSE);
            return FALSE;
          }
      }
  }

extern int slot_location_structural_order(slot_location *left,
                                          slot_location *right)
  {
    assert(left != NULL);
    assert(right != NULL);

    assert(slot_location_is_valid(left)); /* VERIFIED */
    assert(slot_location_is_valid(right)); /* VERIFIED */

    if (left->kind != right->kind)
      {
        if (left->kind < right->kind)
            return -1;
        else
            return 1;
      }

    if ((left->overload_base != NULL) && (right->overload_base == NULL))
        return -1;
    if ((left->overload_base == NULL) && (right->overload_base != NULL))
        return 1;
    if ((left->overload_base != NULL) && (right->overload_base != NULL))
      {
        int overload_order;

        assert(value_is_valid(left->overload_base)); /* VERIFIED */
        assert(value_is_valid(right->overload_base)); /* VERIFIED */
        overload_order = value_structural_order(left->overload_base,
                                                right->overload_base);
        if (overload_order != 0)
            return overload_order;
      }

    switch (left->kind)
      {
        case SLK_VARIABLE:
          {
            if (left->u.variable == right->u.variable)
                return 0;
            else if (left->u.variable < right->u.variable)
                return -1;
            else
                return 1;
          }
        case SLK_LOOKUP:
          {
            int base_order;

            assert(slot_location_is_valid(left->u.lookup.base)); /* VERIFIED */
            assert(slot_location_is_valid(right->u.lookup.base));
                    /* VERIFIED */
            base_order = slot_location_structural_order(left->u.lookup.base,
                                                        right->u.lookup.base);
            if (base_order != 0)
                return base_order;
            assert(lookup_actual_arguments_is_valid(left->u.lookup.actuals));
                    /* VERIFIED */
            assert(lookup_actual_arguments_is_valid(right->u.lookup.actuals));
                    /* VERIFIED */
            return lookup_actual_arguments_structural_order(
                    left->u.lookup.actuals, right->u.lookup.actuals);
          }
        case SLK_FIELD:
          {
            int base_order;

            assert(slot_location_is_valid(left->u.field.base)); /* VERIFIED */
            assert(slot_location_is_valid(right->u.field.base)); /* VERIFIED */
            base_order = slot_location_structural_order(left->u.field.base,
                                                        right->u.field.base);
            if (base_order != 0)
                return base_order;
            return utf8_string_lexicographical_order_by_code_point(
                    left->u.field.field_name, right->u.field.field_name);
          }
        case SLK_TAGALONG:
          {
            int base_order;

            assert(slot_location_is_valid(left->u.tagalong_field.base));
                    /* VERIFIED */
            assert(slot_location_is_valid(right->u.tagalong_field.base));
                    /* VERIFIED */
            base_order = slot_location_structural_order(
                    left->u.tagalong_field.base, right->u.tagalong_field.base);
            if (base_order != 0)
                return base_order;
            assert(tagalong_key_is_instantiated(left->u.tagalong_field.key));
                    /* VERIFIED */
            assert(tagalong_key_is_instantiated(right->u.tagalong_field.key));
                    /* VERIFIED */
            assert(!(tagalong_key_scope_exited(left->u.tagalong_field.key)));
                    /* VERIFIED */
            assert(!(tagalong_key_scope_exited(right->u.tagalong_field.key)));
                    /* VERIFIED */
            return tagalong_key_structural_order(left->u.tagalong_field.key,
                                                 right->u.tagalong_field.key);
          }
        case SLK_CALL:
          {
            int local_order;

            assert(value_is_valid(left->u.call.base)); /* VERIFIED */
            assert(value_is_valid(right->u.call.base)); /* VERIFIED */
            local_order = value_structural_order(left->u.call.base,
                                                 right->u.call.base);
            if (local_order != 0)
                return local_order;

            local_order = utf8_string_lexicographical_order_by_code_point(
                    left->u.call.operation_name, right->u.call.operation_name);
            if (local_order != 0)
                return local_order;

            if (left->u.call.argument0 != NULL)
              {
                if (right->u.call.argument0 == NULL)
                    return 1;

                assert(value_is_valid(left->u.call.argument0)); /* VERIFIED */
                assert(value_is_valid(right->u.call.argument0)); /* VERIFIED */
                local_order = value_structural_order(left->u.call.argument0,
                                                     right->u.call.argument0);
                if (local_order != 0)
                    return local_order;
              }
            else if (right->u.call.argument0 != NULL)
              {
                return -1;
              }

            if (left->u.call.argument_name0 != NULL)
              {
                if (right->u.call.argument_name0 == NULL)
                    return 1;

                local_order = utf8_string_lexicographical_order_by_code_point(
                        left->u.call.argument_name0,
                        right->u.call.argument_name0);
                if (local_order != 0)
                    return local_order;
              }
            else if (right->u.call.argument_name0 != NULL)
              {
                return -1;
              }

            if (left->u.call.argument1 != NULL)
              {
                if (right->u.call.argument1 == NULL)
                    return 1;

                assert(value_is_valid(left->u.call.argument1)); /* VERIFIED */
                assert(value_is_valid(right->u.call.argument1)); /* VERIFIED */
                local_order = value_structural_order(left->u.call.argument1,
                                                     right->u.call.argument1);
                if (local_order != 0)
                    return local_order;
              }
            else if (right->u.call.argument1 != NULL)
              {
                return -1;
              }

            if (left->u.call.argument_name1 != NULL)
              {
                if (right->u.call.argument_name1 == NULL)
                    return 1;

                local_order = utf8_string_lexicographical_order_by_code_point(
                        left->u.call.argument_name1,
                        right->u.call.argument_name1);
                if (local_order != 0)
                    return local_order;
              }
            else if (right->u.call.argument_name1 != NULL)
              {
                return -1;
              }

            if (left->u.call.argument_for_overload_base != NULL)
              {
                if (right->u.call.argument_for_overload_base == NULL)
                    return 1;

                assert(value_is_valid(
                               left->u.call.argument_for_overload_base));
                        /* VERIFIED */
                assert(value_is_valid(
                               right->u.call.argument_for_overload_base));
                        /* VERIFIED */
                local_order = value_structural_order(
                        left->u.call.argument_for_overload_base,
                        right->u.call.argument_for_overload_base);
                if (local_order != 0)
                    return local_order;
              }
            else if (right->u.call.argument_for_overload_base != NULL)
              {
                return -1;
              }

            return 0;
          }
        case SLK_PASS:
          {
            assert(slot_location_is_valid(left->u.pass.base)); /* VERIFIED */
            assert(slot_location_is_valid(right->u.pass.base)); /* VERIFIED */
            return slot_location_structural_order(left->u.pass.base,
                                                  right->u.pass.base);
          }
        default:
          {
            assert(FALSE);
            return 0;
          }
      }
  }

extern boolean slot_location_is_slippery(slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    return the_slot_location->slippery;
  }

extern value *slot_location_overload_base(slot_location *the_slot_location)
  {
    assert(the_slot_location != NULL);

    return the_slot_location->overload_base;
  }

extern void slot_location_dereference_type_bounds(
        slot_location *the_slot_location, type **lower_read, type **upper_read,
        type **lower_write, type **upper_write,
        variable_instance **base_variable, const source_location *location,
        jumper *the_jumper)
  {
    assert(the_slot_location != NULL);

    GRAB_SYSTEM_LOCK(the_slot_location->cache_lock);

    if (the_slot_location->lower_read_type == NULL)
      {
        assert(the_slot_location->lower_read_type == NULL);
        assert(the_slot_location->upper_read_type == NULL);
        assert(the_slot_location->lower_write_type == NULL);
        assert(the_slot_location->upper_write_type == NULL);
        assert(the_slot_location->base_variable == NULL);

        compute_dereference_type_bounds(the_slot_location,
                &base_update_function, NULL,
                &(the_slot_location->lower_read_type),
                &(the_slot_location->upper_read_type),
                &(the_slot_location->lower_write_type),
                &(the_slot_location->upper_write_type),
                &(the_slot_location->base_variable),
                the_slot_location->reference_cluster, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(the_slot_location->lower_read_type == NULL);
            assert(the_slot_location->upper_read_type == NULL);
            assert(the_slot_location->lower_write_type == NULL);
            assert(the_slot_location->upper_write_type == NULL);
            assert(the_slot_location->base_variable == NULL);
          }
        else
          {
            assert(the_slot_location->lower_read_type != NULL);
            assert(the_slot_location->upper_read_type != NULL);
            assert(the_slot_location->lower_write_type != NULL);
            assert(the_slot_location->upper_write_type != NULL);
            if (the_slot_location->base_variable != NULL)
              {
                variable_instance_add_reference_with_cluster(
                        the_slot_location->base_variable,
                        the_slot_location->reference_cluster);
              }
          }
      }
    else
      {
        assert(the_slot_location->lower_read_type != NULL);
        assert(the_slot_location->upper_read_type != NULL);
        assert(the_slot_location->lower_write_type != NULL);
        assert(the_slot_location->upper_write_type != NULL);
      }

    if (lower_read != NULL)
        *lower_read = the_slot_location->lower_read_type;
    if (upper_read != NULL)
        *upper_read = the_slot_location->upper_read_type;
    if (lower_write != NULL)
        *lower_write = the_slot_location->lower_write_type;
    if (upper_write != NULL)
        *upper_write = the_slot_location->upper_write_type;
    if (base_variable != NULL)
        *base_variable = the_slot_location->base_variable;

    RELEASE_SYSTEM_LOCK(the_slot_location->cache_lock);
  }

extern void slot_location_read_type_bounds(slot_location *the_slot_location,
        type **lower, type **upper, const source_location *location,
        jumper *the_jumper)
  {
    slot_location_dereference_type_bounds(the_slot_location, lower, upper,
            NULL, NULL, NULL, location, the_jumper);
  }

extern void slot_location_write_type_bounds(slot_location *the_slot_location,
        type **lower, type **upper, variable_instance **base_variable,
        const source_location *location, jumper *the_jumper)
  {
    slot_location_dereference_type_bounds(the_slot_location, NULL, NULL, lower,
            upper, base_variable, location, the_jumper);
  }

extern void print_slot_location(slot_location *the_slot_location,
        void (*text_printer)(void *data, const char *format, ...), void *data,
        void (*value_printer)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data), expression_parsing_precedence precedence)
  {
    assert(the_slot_location != NULL);
    assert(text_printer != NULL);
    assert(value_printer != NULL);

    switch (the_slot_location->kind)
      {
        case SLK_VARIABLE:
            print_instance(text_printer, data,
                    variable_instance_instance(the_slot_location->u.variable));
            break;
        case SLK_LOOKUP:
            if (precedence > EPP_POSTFIX)
                (*text_printer)(data, "(");
            print_slot_location(the_slot_location->u.lookup.base, text_printer,
                                data, value_printer, EPP_POSTFIX);
            (*text_printer)(data, "[");
            print_lookup_actual_arguments(the_slot_location->u.lookup.actuals,
                                          text_printer, data, value_printer);
            (*text_printer)(data, "]");
            if (precedence > EPP_POSTFIX)
                (*text_printer)(data, ")");
            break;
        case SLK_FIELD:
            if (precedence > EPP_POSTFIX)
                (*text_printer)(data, "(");
            print_slot_location(the_slot_location->u.field.base, text_printer,
                                data, value_printer, EPP_POSTFIX);
            (*text_printer)(data, ".%s",
                            the_slot_location->u.field.field_name);
            if (precedence > EPP_POSTFIX)
                (*text_printer)(data, ")");
            break;
        case SLK_TAGALONG:
            if (precedence > EPP_TAGALONG)
                (*text_printer)(data, "(");
            print_slot_location(the_slot_location->u.tagalong_field.base,
                    text_printer, data, value_printer, EPP_TAGALONG);
            (*text_printer)(data, "..");
            print_instance(text_printer, data,
                    tagalong_key_instance(
                            the_slot_location->u.tagalong_field.key));
            if (precedence > EPP_TAGALONG)
                (*text_printer)(data, ")");
            break;
        case SLK_CALL:
            if (precedence > EPP_POSTFIX)
                (*text_printer)(data, "(");
            (*value_printer)(the_slot_location->u.call.base, text_printer,
                             data);
            (*text_printer)(data, "(");
            if (the_slot_location->u.call.argument0 != NULL)
              {
                (*value_printer)(the_slot_location->u.call.argument0,
                                 text_printer, data);
                if (the_slot_location->u.call.argument1 != NULL)
                  {
                    (*text_printer)(data, ", ");
                    (*value_printer)(the_slot_location->u.call.argument1,
                                     text_printer, data);
                  }
                (*text_printer)(data, ", ");
              }
            (*text_printer)(data, "...)");
            if (precedence > EPP_POSTFIX)
                (*text_printer)(data, ")");
            break;
        case SLK_PASS:
            print_slot_location(the_slot_location->u.pass.base, text_printer,
                                data, value_printer, precedence);
            break;
        default:
            assert(FALSE);
      }
  }


static void compute_dereference_type_bounds(slot_location *the_slot_location,
        void (*update_function)(void *update_data, type *in_lower_read,
                type *in_upper_read, type *in_lower_write,
                type *in_upper_write, variable_instance *in_base_variable,
                type **out_lower_read, type **out_upper_read,
                type **out_lower_write, type **out_upper_write,
                variable_instance **out_base_variable), void *update_data,
        type **lower_read, type **upper_read, type **lower_write,
        type **upper_write, variable_instance **base_variable,
        reference_cluster *cluster, const source_location *location,
        jumper *the_jumper)
  {
    slot_update_data the_slot_update_data;

    assert(the_slot_location != NULL);
    assert(the_jumper != NULL);

    assert(jumper_flowing_forward(the_jumper));

    the_slot_update_data.slot_location = the_slot_location;
    the_slot_update_data.location = location;
    the_slot_update_data.jumper = the_jumper;
    the_slot_update_data.update_function = update_function;
    the_slot_update_data.update_data = update_data;

    if (the_slot_location->lower_read_type != NULL)
      {
        assert(the_slot_location->upper_read_type != NULL);
        assert(the_slot_location->lower_write_type != NULL);
        assert(the_slot_location->upper_write_type != NULL);

        assert(type_is_valid(the_slot_location->lower_read_type));
                /* VERIFICATION NEEDED */
        assert(type_is_valid(the_slot_location->upper_read_type));
                /* VERIFICATION NEEDED */
        assert(type_is_valid(the_slot_location->lower_write_type));
                /* VERIFICATION NEEDED */
        assert(type_is_valid(the_slot_location->upper_write_type));
                /* VERIFICATION NEEDED */
        (*update_function)(update_data, the_slot_location->lower_read_type,
                the_slot_location->upper_read_type,
                the_slot_location->lower_write_type,
                the_slot_location->upper_write_type,
                the_slot_location->base_variable, lower_read, upper_read,
                lower_write, upper_write, base_variable);

        return;
      }

    assert(the_slot_location->lower_read_type == NULL);
    assert(the_slot_location->upper_read_type == NULL);
    assert(the_slot_location->lower_write_type == NULL);
    assert(the_slot_location->upper_write_type == NULL);
    assert(the_slot_location->base_variable == NULL);

    if (lower_write != NULL)
        *lower_write = NULL;
    if (upper_write != NULL)
        *upper_write = NULL;
    if (lower_read != NULL)
        *lower_read = NULL;
    if (upper_read != NULL)
        *upper_read = NULL;
    if (base_variable != NULL)
        *base_variable = NULL;

    switch (the_slot_location->kind)
      {
        case SLK_VARIABLE:
          {
            variable_instance *instance;
            type *to_unref;
            type *read_result;
            type *write_result;

            instance = the_slot_location->u.variable;
            assert(instance != NULL);

            to_unref = NULL;

            instance_check_validity(variable_instance_instance(instance),
                                    location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                goto error_return;

            if (variable_declaration_is_immutable(
                        variable_instance_declaration(instance)))
              {
                value *the_value;
                value *values[1];

                assert(variable_instance_is_instantiated(instance));
                        /* VERIFICATION NEEDED */
                assert(!(variable_instance_scope_exited(instance)));
                        /* VERIFICATION NEEDED */
                the_value = variable_instance_value(instance);
                if (the_value == NULL)
                  {
                    read_result = get_nothing_type();
                  }
                else
                  {
                    check_value_validity(the_value, location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(the_value, the_jumper);
                        goto error_return;
                      }

                    values[0] = the_value;
                    read_result = get_enumeration_type(1, values);
                    if (read_result == NULL)
                      {
                        value_remove_reference(the_value, the_jumper);
                        jumper_do_abort(the_jumper);
                        goto error_return;
                      }
                    value_remove_reference(the_value, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        type_remove_reference(read_result, the_jumper);
                        value_remove_reference(the_value, the_jumper);
                        goto error_return;
                      }
                    to_unref = read_result;
                  }

                write_result = get_nothing_type();
                if (write_result == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    type_remove_reference(to_unref, the_jumper);
                    goto error_return;
                  }
              }
            else
              {
                assert(variable_instance_is_instantiated(instance));
                        /* VERIFICATION NEEDED */
                assert(!(variable_instance_scope_exited(instance)));
                        /* VERIFICATION NEEDED */
                read_result = variable_instance_type(instance);
                assert(read_result != NULL);

                check_type_validity(read_result, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                    goto error_return;

                write_result = read_result;
              }

            assert(type_is_valid(read_result)); /* VERIFIED */
            assert(type_is_valid(write_result)); /* VERIFIED */
            (*update_function)(update_data, read_result, read_result,
                    write_result, write_result, instance, lower_read,
                    upper_read, lower_write, upper_write, base_variable);

            if (to_unref != NULL)
                type_remove_reference(to_unref, the_jumper);

            return;
          }
        case SLK_LOOKUP:
          {
            compute_dereference_type_bounds(the_slot_location->u.lookup.base,
                    &slot_update_function, &the_slot_update_data, lower_read,
                    upper_read, lower_write, upper_write, base_variable,
                    cluster, location, the_jumper);

            return;
          }
        case SLK_FIELD:
          {
            value *overload_base;

            compute_dereference_type_bounds(the_slot_location->u.field.base,
                    &slot_update_function, &the_slot_update_data, lower_read,
                    upper_read, lower_write, upper_write, base_variable,
                    cluster, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                goto error_return;

            overload_base = the_slot_location->overload_base;

            if (overload_base != NULL)
              {
                value *base_value;
                value *field_name_value;
                parameter_pattern_kind parameter_pattern_kinds[3];
                const char *parameter_names[3];
                value *exact_parameters[3];
                type *parameter_lower_types[3];
                type *parameter_upper_types[3];
                type *overload_lower;
                type *overload_upper;
                boolean always_hits;
                boolean never_hits;

                base_value = create_slot_location_value(
                        the_slot_location->u.field.base);
                if (base_value == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    goto error_return;
                  }

                field_name_value = create_string_value(
                        the_slot_location->u.field.field_name);
                if (field_name_value == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    value_remove_reference(base_value, the_jumper);
                    goto error_return;
                  }

                parameter_pattern_kinds[0] = PPK_EXACT;
                parameter_names[0] = NULL;
                assert(value_is_valid(base_value)); /* VERIFICATION NEEDED */
                exact_parameters[0] = base_value;
                parameter_lower_types[0] = NULL;
                parameter_upper_types[0] = NULL;
                parameter_pattern_kinds[1] = PPK_EXACT;
                parameter_names[1] = "field";
                assert(value_is_valid(field_name_value)); /* VERIFIED */
                exact_parameters[1] = field_name_value;
                parameter_lower_types[1] = NULL;
                parameter_upper_types[1] = NULL;

                find_overload_type(overload_base, 2,
                        &(parameter_pattern_kinds[0]), &(parameter_names[0]),
                        &(exact_parameters[0]), &(parameter_lower_types[0]),
                        &(parameter_upper_types[0]), FALSE, NULL, NULL,
                        &overload_lower, &overload_upper, &always_hits, NULL,
                        location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    assert(overload_lower == NULL);
                    assert(overload_upper == NULL);
                    value_remove_reference(base_value, the_jumper);
                    value_remove_reference(field_name_value, the_jumper);
                    goto error_return;
                  }

                assert(overload_lower != NULL);
                assert(overload_upper != NULL);

                if (lower_read != NULL)
                  {
                    type *new_lower_read;

                    if (always_hits)
                      {
                        new_lower_read = overload_lower;
                      }
                    else
                      {
                        assert(type_is_valid(*lower_read));
                                /* VERIFICATION NEEDED */
                        assert(type_is_valid(overload_lower));
                                /* VERIFICATION NEEDED */
                        new_lower_read =
                                get_union_type(*lower_read, overload_lower);
                        if (new_lower_read == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            type_remove_reference(overload_lower, the_jumper);
                            type_remove_reference(overload_upper, the_jumper);
                            value_remove_reference(base_value, the_jumper);
                            value_remove_reference(field_name_value,
                                                   the_jumper);
                            goto error_return;
                          }
                        type_remove_reference(overload_lower, the_jumper);
                      }
                    type_remove_reference(*lower_read, the_jumper);
                    *lower_read = new_lower_read;
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        type_remove_reference(overload_upper, the_jumper);
                        value_remove_reference(base_value, the_jumper);
                        value_remove_reference(field_name_value, the_jumper);
                        goto error_return;
                      }
                  }
                else
                  {
                    type_remove_reference(overload_lower, the_jumper);
                  }

                if (upper_read != NULL)
                  {
                    type *new_upper_read;

                    if (always_hits)
                      {
                        new_upper_read = overload_upper;
                      }
                    else
                      {
                        assert(type_is_valid(*upper_read));
                                /* VERIFICATION NEEDED */
                        assert(type_is_valid(overload_upper));
                                /* VERIFICATION NEEDED */
                        new_upper_read =
                                get_union_type(*upper_read, overload_upper);
                        if (new_upper_read == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            type_remove_reference(overload_upper, the_jumper);
                            value_remove_reference(base_value, the_jumper);
                            value_remove_reference(field_name_value,
                                                   the_jumper);
                            goto error_return;
                          }
                        type_remove_reference(overload_upper, the_jumper);
                      }
                    type_remove_reference(*upper_read, the_jumper);
                    *upper_read = new_upper_read;
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(base_value, the_jumper);
                        value_remove_reference(field_name_value, the_jumper);
                        goto error_return;
                      }
                  }
                else
                  {
                    type_remove_reference(overload_upper, the_jumper);
                  }

                find_overload_type(overload_base, 2,
                        &(parameter_pattern_kinds[0]), &(parameter_names[0]),
                        &(exact_parameters[0]), &(parameter_lower_types[0]),
                        &(parameter_upper_types[0]), TRUE, NULL, NULL,
                        &overload_lower, &overload_upper, &always_hits,
                        &never_hits, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    assert(overload_lower == NULL);
                    assert(overload_upper == NULL);
                    value_remove_reference(base_value, the_jumper);
                    value_remove_reference(field_name_value, the_jumper);
                    goto error_return;
                  }

                assert(overload_lower != NULL);
                assert(overload_upper != NULL);

                if ((!never_hits) && (base_variable != NULL))
                    *base_variable = NULL;

                if (lower_write != NULL)
                  {
                    type *new_lower_write;

                    if (always_hits)
                      {
                        new_lower_write = overload_lower;
                      }
                    else
                      {
                        assert(type_is_valid(*lower_write));
                                /* VERIFICATION NEEDED */
                        assert(type_is_valid(overload_lower));
                                /* VERIFICATION NEEDED */
                        new_lower_write =
                                get_union_type(*lower_write, overload_lower);
                        if (new_lower_write == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            type_remove_reference(overload_lower, the_jumper);
                            type_remove_reference(overload_upper, the_jumper);
                            value_remove_reference(base_value, the_jumper);
                            value_remove_reference(field_name_value,
                                                   the_jumper);
                            goto error_return;
                          }
                        type_remove_reference(overload_lower, the_jumper);
                      }
                    type_remove_reference(*lower_write, the_jumper);
                    *lower_write = new_lower_write;
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        type_remove_reference(overload_upper, the_jumper);
                        value_remove_reference(base_value, the_jumper);
                        value_remove_reference(field_name_value, the_jumper);
                        goto error_return;
                      }
                  }
                else
                  {
                    type_remove_reference(overload_lower, the_jumper);
                  }

                if (upper_write != NULL)
                  {
                    type *new_upper_write;

                    if (always_hits)
                      {
                        new_upper_write = overload_upper;
                      }
                    else
                      {
                        assert(type_is_valid(*upper_write));
                                /* VERIFICATION NEEDED */
                        assert(type_is_valid(overload_upper));
                                /* VERIFICATION NEEDED */
                        new_upper_write =
                                get_union_type(*upper_write, overload_upper);
                        if (new_upper_write == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            type_remove_reference(overload_upper, the_jumper);
                            value_remove_reference(base_value, the_jumper);
                            value_remove_reference(field_name_value,
                                                   the_jumper);
                            goto error_return;
                          }
                        type_remove_reference(overload_upper, the_jumper);
                      }
                    type_remove_reference(*upper_write, the_jumper);
                    *upper_write = new_upper_write;
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(base_value, the_jumper);
                        value_remove_reference(field_name_value, the_jumper);
                        goto error_return;
                      }
                  }
                else
                  {
                    type_remove_reference(overload_upper, the_jumper);
                  }

                value_remove_reference(base_value, the_jumper);
                value_remove_reference(field_name_value, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                    goto error_return;
              }

            return;
          }
        case SLK_TAGALONG:
          {
            compute_dereference_type_bounds(
                    the_slot_location->u.tagalong_field.base,
                    &slot_update_function, &the_slot_update_data, lower_read,
                    upper_read, lower_write, upper_write, base_variable,
                    cluster, location, the_jumper);

            return;
          }
        case SLK_CALL:
          {
            parameter_pattern_kind parameter_pattern_kinds[4];
            const char *parameter_names[4];
            value *exact_parameters[4];
            type *parameter_lower_types[4];
            type *parameter_upper_types[4];
            size_t parameter_count;
            value *base;
            value *overload_base;
            type *overload_lower;
            type *overload_upper;
            boolean always_hits;

            if (base_variable != NULL)
                *base_variable = NULL;

            if (lower_read != NULL)
              {
                *lower_read = get_nothing_type();
                if (*lower_read == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    goto error_return;
                  }
                type_add_reference(*lower_read);
              }

            if (upper_read != NULL)
              {
                *upper_read = get_nothing_type();
                if (*upper_read == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    goto error_return;
                  }
                type_add_reference(*upper_read);
              }

            if (lower_write != NULL)
              {
                *lower_write = get_nothing_type();
                if (*lower_write == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    goto error_return;
                  }
                type_add_reference(*lower_write);
              }

            if (upper_write != NULL)
              {
                *upper_write = get_nothing_type();
                if (*upper_write == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    goto error_return;
                  }
                type_add_reference(*upper_write);
              }

            parameter_pattern_kinds[1] = PPK_EXACT;
            parameter_names[1] = the_slot_location->u.call.argument_name0;
            exact_parameters[1] = the_slot_location->u.call.argument0;
            assert((exact_parameters[1] == NULL) ||
                   value_is_valid(exact_parameters[1]));
                    /* VERIFICATION NEEDED */
            parameter_lower_types[1] = NULL;
            parameter_upper_types[1] = NULL;
            parameter_pattern_kinds[2] = PPK_EXACT;
            parameter_names[2] = the_slot_location->u.call.argument_name1;
            exact_parameters[2] = the_slot_location->u.call.argument1;
            assert((exact_parameters[2] == NULL) ||
                   value_is_valid(exact_parameters[2]));
                    /* VERIFICATION NEEDED */
            parameter_lower_types[2] = NULL;
            parameter_upper_types[2] = NULL;

            if (exact_parameters[1] == NULL)
              {
                assert(parameter_names[1] == NULL);
                assert(exact_parameters[2] == NULL);
                assert(parameter_names[2] == NULL);
                parameter_count = 0;
              }
            else if (exact_parameters[2] == NULL)
              {
                assert(parameter_names[2] == NULL);
                parameter_count = 1;
              }
            else
              {
                parameter_count = 2;
              }

            base = the_slot_location->u.call.base;
            assert(base != NULL);

            overload_base = the_slot_location->overload_base;

            find_overload_type(base, parameter_count,
                    &(parameter_pattern_kinds[1]), &(parameter_names[1]),
                    &(exact_parameters[1]), &(parameter_lower_types[1]),
                    &(parameter_upper_types[1]), FALSE, NULL, NULL,
                    &overload_lower, &overload_upper, &always_hits, NULL,
                    location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(overload_lower == NULL);
                assert(overload_upper == NULL);
                goto error_return;
              }

            assert(overload_lower != NULL);
            assert(overload_upper != NULL);

            if (lower_read != NULL)
              {
                type *new_lower_read;

                if (always_hits)
                  {
                    new_lower_read = overload_lower;
                  }
                else
                  {
                    assert(type_is_valid(*lower_read));
                            /* VERIFICATION NEEDED */
                    assert(type_is_valid(overload_lower));
                            /* VERIFICATION NEEDED */
                    new_lower_read =
                            get_union_type(*lower_read, overload_lower);
                    if (new_lower_read == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        type_remove_reference(overload_lower, the_jumper);
                        type_remove_reference(overload_upper, the_jumper);
                        goto error_return;
                      }
                    type_remove_reference(overload_lower, the_jumper);
                  }

                type_remove_reference(*lower_read, the_jumper);
                *lower_read = new_lower_read;
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    type_remove_reference(overload_upper, the_jumper);
                    goto error_return;
                  }
              }
            else
              {
                type_remove_reference(overload_lower, the_jumper);
              }

            if (upper_read != NULL)
              {
                type *new_upper_read;

                if (always_hits)
                  {
                    new_upper_read = overload_upper;
                  }
                else
                  {
                    assert(type_is_valid(*upper_read));
                            /* VERIFICATION NEEDED */
                    assert(type_is_valid(overload_upper));
                            /* VERIFICATION NEEDED */
                    new_upper_read =
                            get_union_type(*upper_read, overload_upper);
                    if (new_upper_read == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        type_remove_reference(overload_upper, the_jumper);
                        goto error_return;
                      }
                    type_remove_reference(overload_upper, the_jumper);
                  }

                type_remove_reference(*upper_read, the_jumper);
                *upper_read = new_upper_read;
                if (!(jumper_flowing_forward(the_jumper)))
                    goto error_return;
              }
            else
              {
                type_remove_reference(overload_upper, the_jumper);
              }

            if (overload_base != NULL)
              {
                parameter_pattern_kinds[0] = PPK_EXACT;
                parameter_names[0] = NULL;
                exact_parameters[0] =
                        the_slot_location->u.call.argument_for_overload_base;
                assert((exact_parameters[0] == NULL) ||
                       value_is_valid(exact_parameters[0]));
                        /* VERIFICATION NEEDED */
                parameter_lower_types[0] = NULL;
                parameter_upper_types[0] = NULL;

                find_overload_type(overload_base, parameter_count + 1,
                        &(parameter_pattern_kinds[0]), &(parameter_names[0]),
                        &(exact_parameters[0]), &(parameter_lower_types[0]),
                        &(parameter_upper_types[0]), FALSE, NULL, NULL,
                        &overload_lower, &overload_upper, &always_hits, NULL,
                        location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    assert(overload_lower == NULL);
                    assert(overload_upper == NULL);
                    goto error_return;
                  }

                assert(overload_lower != NULL);
                assert(overload_upper != NULL);

                if (lower_read != NULL)
                  {
                    type *new_lower_read;

                    if (always_hits)
                      {
                        new_lower_read = overload_lower;
                      }
                    else
                      {
                        assert(type_is_valid(*lower_read));
                                /* VERIFICATION NEEDED */
                        assert(type_is_valid(overload_lower));
                                /* VERIFICATION NEEDED */
                        new_lower_read =
                                get_union_type(*lower_read, overload_lower);
                        if (new_lower_read == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            type_remove_reference(overload_lower, the_jumper);
                            type_remove_reference(overload_upper, the_jumper);
                            goto error_return;
                          }
                        type_remove_reference(overload_lower, the_jumper);
                      }
                    type_remove_reference(*lower_read, the_jumper);
                    *lower_read = new_lower_read;
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        type_remove_reference(overload_upper, the_jumper);
                        goto error_return;
                      }
                  }
                else
                  {
                    type_remove_reference(overload_lower, the_jumper);
                  }

                if (upper_read != NULL)
                  {
                    type *new_upper_read;

                    if (always_hits)
                      {
                        new_upper_read = overload_upper;
                      }
                    else
                      {
                        assert(type_is_valid(*upper_read));
                                /* VERIFICATION NEEDED */
                        assert(type_is_valid(overload_upper));
                                /* VERIFICATION NEEDED */
                        new_upper_read =
                                get_union_type(*upper_read, overload_upper);
                        if (new_upper_read == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            type_remove_reference(overload_upper, the_jumper);
                            goto error_return;
                          }
                        type_remove_reference(overload_upper, the_jumper);
                      }
                    type_remove_reference(*upper_read, the_jumper);
                    *upper_read = new_upper_read;
                    if (!(jumper_flowing_forward(the_jumper)))
                        goto error_return;
                  }
                else
                  {
                    type_remove_reference(overload_upper, the_jumper);
                  }
              }

            find_overload_type(base, parameter_count,
                    &(parameter_pattern_kinds[1]), &(parameter_names[1]),
                    &(exact_parameters[1]), &(parameter_lower_types[1]),
                    &(parameter_upper_types[1]), TRUE, NULL, NULL,
                    &overload_lower, &overload_upper, &always_hits, NULL,
                    location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(overload_lower == NULL);
                assert(overload_upper == NULL);
                goto error_return;
              }

            assert(overload_lower != NULL);
            assert(overload_upper != NULL);

            if (lower_write != NULL)
              {
                type *new_lower_write;

                if (always_hits)
                  {
                    new_lower_write = overload_lower;
                  }
                else
                  {
                    assert(type_is_valid(*lower_write));
                            /* VERIFICATION NEEDED */
                    assert(type_is_valid(overload_lower));
                            /* VERIFICATION NEEDED */
                    new_lower_write =
                            get_union_type(*lower_write, overload_lower);
                    if (new_lower_write == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        type_remove_reference(overload_lower, the_jumper);
                        type_remove_reference(overload_upper, the_jumper);
                        goto error_return;
                      }
                    type_remove_reference(overload_lower, the_jumper);
                  }
                type_remove_reference(*lower_write, the_jumper);
                *lower_write = new_lower_write;
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    type_remove_reference(overload_upper, the_jumper);
                    goto error_return;
                  }
              }
            else
              {
                type_remove_reference(overload_lower, the_jumper);
              }

            if (upper_write != NULL)
              {
                type *new_upper_write;

                if (always_hits)
                  {
                    new_upper_write = overload_upper;
                  }
                else
                  {
                    assert(type_is_valid(*upper_write));
                            /* VERIFICATION NEEDED */
                    assert(type_is_valid(overload_upper));
                            /* VERIFICATION NEEDED */
                    new_upper_write =
                            get_union_type(*upper_write, overload_upper);
                    if (new_upper_write == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        type_remove_reference(overload_upper, the_jumper);
                        goto error_return;
                      }
                    type_remove_reference(overload_upper, the_jumper);
                  }
                type_remove_reference(*upper_write, the_jumper);
                *upper_write = new_upper_write;
                if (!(jumper_flowing_forward(the_jumper)))
                    goto error_return;
              }
            else
              {
                type_remove_reference(overload_upper, the_jumper);
              }

            if (overload_base != NULL)
              {
                parameter_pattern_kinds[0] = PPK_EXACT;
                parameter_names[0] = NULL;
                exact_parameters[0] =
                        the_slot_location->u.call.argument_for_overload_base;
                assert((exact_parameters[0] == NULL) ||
                       value_is_valid(exact_parameters[0]));
                        /* VERIFICATION NEEDED */
                parameter_lower_types[0] = NULL;
                parameter_upper_types[0] = NULL;

                find_overload_type(overload_base, parameter_count + 1,
                        &(parameter_pattern_kinds[0]), &(parameter_names[0]),
                        &(exact_parameters[0]), &(parameter_lower_types[0]),
                        &(parameter_upper_types[0]), TRUE, NULL, NULL,
                        &overload_lower, &overload_upper, &always_hits, NULL,
                        location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    assert(overload_lower == NULL);
                    assert(overload_upper == NULL);
                    goto error_return;
                  }

                assert(overload_lower != NULL);
                assert(overload_upper != NULL);

                if (lower_write != NULL)
                  {
                    type *new_lower_write;

                    if (always_hits)
                      {
                        new_lower_write = overload_lower;
                      }
                    else
                      {
                        assert(type_is_valid(*lower_write));
                                /* VERIFICATION NEEDED */
                        assert(type_is_valid(overload_lower));
                                /* VERIFICATION NEEDED */
                        new_lower_write =
                                get_union_type(*lower_write, overload_lower);
                        if (new_lower_write == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            type_remove_reference(overload_lower, the_jumper);
                            type_remove_reference(overload_upper, the_jumper);
                            goto error_return;
                          }
                        type_remove_reference(overload_lower, the_jumper);
                      }
                    type_remove_reference(*lower_write, the_jumper);
                    *lower_write = new_lower_write;
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        type_remove_reference(overload_upper, the_jumper);
                        goto error_return;
                      }
                  }
                else
                  {
                    type_remove_reference(overload_lower, the_jumper);
                  }

                if (upper_write != NULL)
                  {
                    type *new_upper_write;

                    if (always_hits)
                      {
                        new_upper_write = overload_upper;
                      }
                    else
                      {
                        assert(type_is_valid(*upper_write));
                                /* VERIFICATION NEEDED */
                        assert(type_is_valid(overload_upper));
                                /* VERIFICATION NEEDED */
                        new_upper_write =
                                get_union_type(*upper_write, overload_upper);
                        if (new_upper_write == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            type_remove_reference(overload_upper, the_jumper);
                            goto error_return;
                          }
                        type_remove_reference(overload_upper, the_jumper);
                      }
                    type_remove_reference(*upper_write, the_jumper);
                    *upper_write = new_upper_write;
                    if (!(jumper_flowing_forward(the_jumper)))
                        goto error_return;
                  }
                else
                  {
                    type_remove_reference(overload_upper, the_jumper);
                  }
              }

            return;
          }
        case SLK_PASS:
          {
            value *overload_base;
            value *base_value;
            parameter_pattern_kind parameter_pattern_kinds[2];
            const char *parameter_names[2];
            value *exact_parameters[2];
            type *parameter_lower_types[2];
            type *parameter_upper_types[2];
            type *overload_lower;
            type *overload_upper;
            boolean always_hits;
            boolean never_hits;

            compute_dereference_type_bounds(the_slot_location->u.pass.base,
                    &slot_update_function, &the_slot_update_data, lower_read,
                    upper_read, lower_write, upper_write, base_variable,
                    cluster, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                goto error_return;

            overload_base = the_slot_location->overload_base;

            if (overload_base == NULL)
                return;

            check_slot_location_validity(the_slot_location->u.pass.base,
                                         location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                goto error_return;

            base_value =
                    create_slot_location_value(the_slot_location->u.pass.base);
            if (base_value == NULL)
              {
                jumper_do_abort(the_jumper);
                goto error_return;
              }

            parameter_pattern_kinds[0] = PPK_EXACT;
            parameter_names[0] = NULL;
            assert(value_is_valid(base_value)); /* VERIFIED */
            exact_parameters[0] = base_value;
            parameter_lower_types[0] = NULL;
            parameter_upper_types[0] = NULL;

            find_overload_type(overload_base, 1, &(parameter_pattern_kinds[0]),
                    &(parameter_names[0]), &(exact_parameters[0]),
                    &(parameter_lower_types[0]), &(parameter_upper_types[0]),
                    FALSE, NULL, NULL, &overload_lower, &overload_upper,
                    &always_hits, NULL, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(overload_lower == NULL);
                assert(overload_upper == NULL);
                value_remove_reference(base_value, the_jumper);
                goto error_return;
              }

            assert(overload_lower != NULL);
            assert(overload_upper != NULL);

            if (lower_read != NULL)
              {
                type *new_lower_read;

                if (always_hits)
                  {
                    new_lower_read = overload_lower;
                  }
                else
                  {
                    assert(type_is_valid(*lower_read));
                            /* VERIFICATION NEEDED */
                    assert(type_is_valid(overload_lower));
                            /* VERIFICATION NEEDED */
                    new_lower_read =
                            get_union_type(*lower_read, overload_lower);
                    if (new_lower_read == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        type_remove_reference(overload_lower, the_jumper);
                        type_remove_reference(overload_upper, the_jumper);
                        value_remove_reference(base_value, the_jumper);
                        goto error_return;
                      }
                    type_remove_reference(overload_lower, the_jumper);
                  }
                type_remove_reference(*lower_read, the_jumper);
                *lower_read = new_lower_read;
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    type_remove_reference(overload_upper, the_jumper);
                    value_remove_reference(base_value, the_jumper);
                    goto error_return;
                  }
              }
            else
              {
                type_remove_reference(overload_lower, the_jumper);
              }

            if (upper_read != NULL)
              {
                type *new_upper_read;

                if (always_hits)
                  {
                    new_upper_read = overload_upper;
                  }
                else
                  {
                    assert(type_is_valid(*upper_read));
                            /* VERIFICATION NEEDED */
                    assert(type_is_valid(overload_upper));
                            /* VERIFICATION NEEDED */
                    new_upper_read =
                            get_union_type(*upper_read, overload_upper);
                    if (new_upper_read == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        type_remove_reference(overload_upper, the_jumper);
                        value_remove_reference(base_value, the_jumper);
                        goto error_return;
                      }
                    type_remove_reference(overload_upper, the_jumper);
                  }
                type_remove_reference(*upper_read, the_jumper);
                *upper_read = new_upper_read;
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    value_remove_reference(base_value, the_jumper);
                    goto error_return;
                  }
              }
            else
              {
                type_remove_reference(overload_upper, the_jumper);
              }

            find_overload_type(overload_base, 1, &(parameter_pattern_kinds[0]),
                    &(parameter_names[0]), &(exact_parameters[0]),
                    &(parameter_lower_types[0]), &(parameter_upper_types[0]),
                    TRUE, NULL, NULL, &overload_lower, &overload_upper,
                    &always_hits, &never_hits, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(overload_lower == NULL);
                assert(overload_upper == NULL);
                value_remove_reference(base_value, the_jumper);
                goto error_return;
              }

            assert(overload_lower != NULL);
            assert(overload_upper != NULL);

            if ((!never_hits) && (base_variable != NULL))
                *base_variable = NULL;

            if (lower_write != NULL)
              {
                type *new_lower_write;

                if (always_hits)
                  {
                    new_lower_write = overload_lower;
                  }
                else
                  {
                    assert(type_is_valid(*lower_write));
                            /* VERIFICATION NEEDED */
                    assert(type_is_valid(overload_lower));
                            /* VERIFICATION NEEDED */
                    new_lower_write =
                            get_union_type(*lower_write, overload_lower);
                    if (new_lower_write == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        type_remove_reference(overload_lower, the_jumper);
                        type_remove_reference(overload_upper, the_jumper);
                        value_remove_reference(base_value, the_jumper);
                        goto error_return;
                      }
                    type_remove_reference(overload_lower, the_jumper);
                  }
                type_remove_reference(*lower_write, the_jumper);
                *lower_write = new_lower_write;
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    type_remove_reference(overload_upper, the_jumper);
                    value_remove_reference(base_value, the_jumper);
                    goto error_return;
                  }
              }
            else
              {
                type_remove_reference(overload_lower, the_jumper);
              }

            if (upper_write != NULL)
              {
                type *new_upper_write;

                if (always_hits)
                  {
                    new_upper_write = overload_upper;
                  }
                else
                  {
                    assert(type_is_valid(*upper_write));
                            /* VERIFICATION NEEDED */
                    assert(type_is_valid(overload_upper));
                            /* VERIFICATION NEEDED */
                    new_upper_write =
                            get_union_type(*upper_write, overload_upper);
                    if (new_upper_write == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        type_remove_reference(overload_upper, the_jumper);
                        value_remove_reference(base_value, the_jumper);
                        goto error_return;
                      }
                    type_remove_reference(overload_upper, the_jumper);
                  }
                type_remove_reference(*upper_write, the_jumper);
                *upper_write = new_upper_write;
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    value_remove_reference(base_value, the_jumper);
                    goto error_return;
                  }
              }
            else
              {
                type_remove_reference(overload_upper, the_jumper);
              }

            value_remove_reference(base_value, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                goto error_return;

            return;
          }
        default:
          {
            assert(FALSE);
          }
      }

  error_return:
    assert(!(jumper_flowing_forward(the_jumper)));
    if ((lower_read != NULL) && (*lower_read != NULL))
      {
        type_remove_reference(*lower_read, the_jumper);
        *lower_read = NULL;
      }
    if ((upper_read != NULL) && (*upper_read != NULL))
      {
        type_remove_reference(*upper_read, the_jumper);
        *upper_read = NULL;
      }
    if ((lower_write != NULL) && (*lower_write != NULL))
      {
        type_remove_reference(*lower_write, the_jumper);
        *lower_write = NULL;
      }
    if ((upper_write != NULL) && (*upper_write != NULL))
      {
        type_remove_reference(*upper_write, the_jumper);
        *upper_write = NULL;
      }
    if ((base_variable != NULL) && (*base_variable != NULL))
        *base_variable = NULL;
  }

static void base_update_function(void *update_data, type *in_lower_read,
        type *in_upper_read, type *in_lower_write, type *in_upper_write,
        variable_instance *in_base_variable, type **out_lower_read,
        type **out_upper_read, type **out_lower_write, type **out_upper_write,
        variable_instance **out_base_variable)
  {
    assert(update_data == NULL);
    assert(in_lower_read != NULL);
    assert(in_upper_read != NULL);
    assert(in_lower_write != NULL);
    assert(in_upper_write != NULL);

    assert(type_is_valid(in_lower_read)); /* VERIFIED */
    assert(type_is_valid(in_upper_read)); /* VERIFIED */
    assert(type_is_valid(in_lower_write)); /* VERIFIED */
    assert(type_is_valid(in_upper_write)); /* VERIFIED */

    if (out_lower_read != NULL)
      {
        type_add_reference(in_lower_read);
        *out_lower_read = in_lower_read;
      }
    if (out_upper_read != NULL)
      {
        type_add_reference(in_upper_read);
        *out_upper_read = in_upper_read;
      }
    if (out_lower_write != NULL)
      {
        type_add_reference(in_lower_write);
        *out_lower_write = in_lower_write;
      }
    if (out_upper_write != NULL)
      {
        type_add_reference(in_upper_write);
        *out_upper_write = in_upper_write;
      }
    if (out_base_variable != NULL)
        *out_base_variable = in_base_variable;

    assert((out_lower_read == NULL) || (*out_lower_read == NULL) ||
           type_is_valid(*out_lower_read)); /* VERIFIED */
    assert((out_upper_read == NULL) || (*out_upper_read == NULL) ||
           type_is_valid(*out_upper_read)); /* VERIFIED */
    assert((out_lower_write == NULL) || (*out_lower_write == NULL) ||
           type_is_valid(*out_lower_write)); /* VERIFIED */
    assert((out_upper_write == NULL) || (*out_upper_write == NULL) ||
           type_is_valid(*out_upper_write)); /* VERIFIED */
  }

static void slot_update_function(void *update_data, type *in_lower_read,
        type *in_upper_read, type *in_lower_write, type *in_upper_write,
        variable_instance *in_base_variable, type **out_lower_read,
        type **out_upper_read, type **out_lower_write, type **out_upper_write,
        variable_instance **out_base_variable)
  {
    slot_update_data *the_slot_update_data;
    slot_location *the_slot_location;
    const source_location *location;
    jumper *the_jumper;
    void (*update_function)(void *update_data, type *in_lower_read,
            type *in_upper_read, type *in_lower_write, type *in_upper_write,
            variable_instance *in_base_variable, type **out_lower_read,
            type **out_upper_read, type **out_lower_write,
            type **out_upper_write, variable_instance **out_base_variable);
    void *child_update_data;

    assert(update_data != NULL);
    assert(in_lower_read != NULL);
    assert(in_upper_read != NULL);
    assert(in_lower_write != NULL);
    assert(in_upper_write != NULL);

    assert(type_is_valid(in_lower_read)); /* VERIFICATION NEEDED */
    assert(type_is_valid(in_upper_read)); /* VERIFICATION NEEDED */
    assert(type_is_valid(in_lower_write)); /* VERIFICATION NEEDED */
    assert(type_is_valid(in_upper_write)); /* VERIFICATION NEEDED */

    if (out_lower_read != NULL)
        *out_lower_read = NULL;
    if (out_upper_read != NULL)
        *out_upper_read = NULL;
    if (out_lower_write != NULL)
        *out_lower_write = NULL;
    if (out_upper_write != NULL)
        *out_upper_write = NULL;

    the_slot_update_data = (slot_update_data *)update_data;

    the_slot_location = the_slot_update_data->slot_location;
    assert(the_slot_location != NULL);
    location = the_slot_update_data->location;
    the_jumper = the_slot_update_data->jumper;
    assert(the_jumper != NULL);
    update_function = the_slot_update_data->update_function;
    assert(update_function != NULL);
    child_update_data = the_slot_update_data->update_data;

    switch (the_slot_location->kind)
      {
        case SLK_VARIABLE:
          {
            assert(FALSE);
          }
        case SLK_LOOKUP:
          {
            check_lookup_actual_arguments_validity(
                    the_slot_location->u.lookup.actuals, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                goto error_return;

            assert(type_is_valid(in_lower_read)); /* VERIFIED */
            assert(type_is_valid(in_upper_read)); /* VERIFIED */
            assert(type_is_valid(in_lower_write)); /* VERIFIED */
            assert(type_is_valid(in_upper_write)); /* VERIFIED */
            assert(lookup_actual_arguments_is_valid(
                           the_slot_location->u.lookup.actuals));
                    /* VERIFIED */
            find_lookup_types(in_lower_read, in_upper_read, in_lower_write,
                    in_upper_write, in_base_variable,
                    the_slot_location->u.lookup.actuals,
                    the_slot_location->overload_base, update_function,
                    child_update_data, out_lower_read, out_upper_read,
                    out_lower_write, out_upper_write, out_base_variable,
                    location, the_jumper);

            assert((out_lower_read == NULL) || (*out_lower_read == NULL) ||
                   type_is_valid(*out_lower_read)); /* VERIFIED */
            assert((out_upper_read == NULL) || (*out_upper_read == NULL) ||
                   type_is_valid(*out_upper_read)); /* VERIFIED */
            assert((out_lower_write == NULL) || (*out_lower_write == NULL) ||
                   type_is_valid(*out_lower_write)); /* VERIFIED */
            assert((out_upper_write == NULL) || (*out_upper_write == NULL) ||
                   type_is_valid(*out_upper_write)); /* VERIFIED */
            return;
          }
        case SLK_FIELD:
          {
            type *field_lower_read;
            type *field_upper_read;
            type *field_lower_write;
            type *field_upper_write;
            variable_instance *result_base_variable;

            assert(type_is_valid(in_lower_read)); /* VERIFIED */
            field_lower_read = type_field(in_lower_read,
                    the_slot_location->u.field.field_name, LOU_LOWER);
            assert((field_lower_read == NULL) ||
                   type_is_valid(field_lower_read)); /* VERIFIED */
            if (field_lower_read == NULL)
              {
                jumper_do_abort(the_jumper);
                goto error_return;
              }
            assert(type_is_valid(field_lower_read)); /* VERIFIED */

            assert(type_is_valid(in_upper_read)); /* VERIFIED */
            field_upper_read = type_field(in_upper_read,
                    the_slot_location->u.field.field_name, LOU_UPPER);
            assert((field_upper_read == NULL) ||
                   type_is_valid(field_upper_read)); /* VERIFIED */
            if (field_upper_read == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(field_lower_read, the_jumper);
                goto error_return;
              }
            assert(type_is_valid(field_upper_read)); /* VERIFIED */

            assert(type_is_valid(in_lower_write)); /* VERIFIED */
            field_lower_write = type_field(in_lower_write,
                    the_slot_location->u.field.field_name, LOU_LOWER);
            assert((field_lower_write == NULL) ||
                   type_is_valid(field_lower_write)); /* VERIFIED */
            if (field_lower_write == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(field_lower_read, the_jumper);
                type_remove_reference(field_upper_read, the_jumper);
                goto error_return;
              }
            assert(type_is_valid(field_lower_write)); /* VERIFIED */

            assert(type_is_valid(field_lower_write)); /* VERIFIED */
            assert(type_is_valid(field_lower_read)); /* VERIFIED */
            field_lower_write = augment_write_type_from_read_type(
                    field_lower_write, in_lower_read,
                    the_slot_location->u.field.field_name, LOU_LOWER, NULL);
            assert((field_lower_write == NULL) ||
                   type_is_valid(field_lower_write)); /* VERIFIED */
            if (field_lower_write == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(field_lower_read, the_jumper);
                type_remove_reference(field_upper_read, the_jumper);
                goto error_return;
              }
            assert(type_is_valid(field_lower_write)); /* VERIFIED */

            assert(type_is_valid(in_upper_write)); /* VERIFIED */
            field_upper_write = type_field(in_upper_write,
                    the_slot_location->u.field.field_name, LOU_UPPER);
            assert((field_upper_write == NULL) ||
                   type_is_valid(field_upper_write)); /* VERIFIED */
            if (field_upper_write == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(field_lower_read, the_jumper);
                type_remove_reference(field_upper_read, the_jumper);
                type_remove_reference(field_lower_write, the_jumper);
                goto error_return;
              }
            assert(type_is_valid(field_upper_write)); /* VERIFIED */

            result_base_variable = in_base_variable;

            assert(type_is_valid(field_upper_write)); /* VERIFIED */
            assert(type_is_valid(field_upper_read)); /* VERIFIED */
            field_upper_write = augment_write_type_from_read_type(
                    field_upper_write, in_upper_read,
                    the_slot_location->u.field.field_name, LOU_UPPER,
                    &result_base_variable);
            assert((field_upper_write == NULL) ||
                   type_is_valid(field_upper_write)); /* VERIFIED */
            if (field_upper_write == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(field_lower_read, the_jumper);
                type_remove_reference(field_upper_read, the_jumper);
                type_remove_reference(field_lower_write, the_jumper);
                goto error_return;
              }
            assert(type_is_valid(field_upper_write)); /* VERIFIED */

            assert(type_is_valid(field_lower_read)); /* VERIFIED */
            assert(type_is_valid(field_upper_read)); /* VERIFIED */
            assert(type_is_valid(field_lower_write)); /* VERIFIED */
            assert(type_is_valid(field_upper_write)); /* VERIFIED */
            (*update_function)(child_update_data, field_lower_read,
                    field_upper_read, field_lower_write, field_upper_write,
                    result_base_variable, out_lower_read, out_upper_read,
                    out_lower_write, out_upper_write, out_base_variable);

            type_remove_reference(field_lower_read, the_jumper);
            type_remove_reference(field_upper_read, the_jumper);
            type_remove_reference(field_lower_write, the_jumper);
            type_remove_reference(field_upper_write, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                goto error_return;

            assert((out_lower_read == NULL) || (*out_lower_read == NULL) ||
                   type_is_valid(*out_lower_read)); /* VERIFIED */
            assert((out_upper_read == NULL) || (*out_upper_read == NULL) ||
                   type_is_valid(*out_upper_read)); /* VERIFIED */
            assert((out_lower_write == NULL) || (*out_lower_write == NULL) ||
                   type_is_valid(*out_lower_write)); /* VERIFIED */
            assert((out_upper_write == NULL) || (*out_upper_write == NULL) ||
                   type_is_valid(*out_upper_write)); /* VERIFIED */
            return;
          }
        case SLK_TAGALONG:
          {
            tagalong_key *key;
            boolean doubt;
            boolean no_match;
            type *data_type;
            type *read_type;
            boolean is_subset;
            type *next_lower_write;

            key = the_slot_location->u.tagalong_field.key;
            assert(key != NULL);

            if (tagalong_key_scope_exited(key))
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(tagalong_reference_scope_exited),
                        "A write was attempted through a tagalong key after "
                        "that key had ceased to exist.");
                goto error_return;
              }

            check_type_validity(tagalong_key_on_type(key), location,
                                the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                goto error_return;

            assert(tagalong_key_is_instantiated(key));
                    /* VERIFICATION NEEDED */
            assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */
            data_type = tagalong_key_type(key);
            assert(data_type != NULL);

            check_type_validity(data_type, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                goto error_return;

            assert(type_is_valid(data_type)); /* VERIFIED */

            assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
            assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */
            assert(type_is_valid(in_upper_read)); /* VERIFIED */
            assert(type_is_valid(tagalong_key_on_type(key))); /* VERIFIED */
            no_match = intersection_empty(in_upper_read,
                    tagalong_key_on_type(key), &doubt, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                goto error_return;
            if (no_match && !doubt)
              {
                read_type = get_nothing_type();
                if (read_type == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    goto error_return;
                  }
                assert(type_is_valid(read_type)); /* VERIFIED */
              }
            else
              {
                read_type = data_type;
                assert(read_type != NULL);
                assert(type_is_valid(read_type)); /* VERIFIED */
              }

            assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
            assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */
            assert(type_is_valid(in_lower_write)); /* VERIFIED */
            assert(type_is_valid(tagalong_key_on_type(key))); /* VERIFIED */
            is_subset = type_is_subset(in_lower_write,
                    tagalong_key_on_type(key), &doubt, NULL, location,
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                goto error_return;
            if ((!is_subset) || doubt)
              {
                next_lower_write = get_nothing_type();
                if (next_lower_write == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    goto error_return;
                  }
                assert(type_is_valid(next_lower_write)); /* VERIFIED */
              }
            else
              {
                next_lower_write = data_type;
                assert(type_is_valid(next_lower_write)); /* VERIFIED */
              }

            assert(type_is_valid(read_type)); /* VERIFIED */
            assert(type_is_valid(next_lower_write)); /* VERIFIED */
            assert(type_is_valid(data_type)); /* VERIFIED */
            (*update_function)(child_update_data, read_type, read_type,
                    next_lower_write, data_type, in_base_variable,
                    out_lower_read, out_upper_read, out_lower_write,
                    out_upper_write, out_base_variable);

            if (!(jumper_flowing_forward(the_jumper)))
                goto error_return;

            assert((out_lower_read == NULL) || (*out_lower_read == NULL) ||
                   type_is_valid(*out_lower_read)); /* VERIFIED */
            assert((out_upper_read == NULL) || (*out_upper_read == NULL) ||
                   type_is_valid(*out_upper_read)); /* VERIFIED */
            assert((out_lower_write == NULL) || (*out_lower_write == NULL) ||
                   type_is_valid(*out_lower_write)); /* VERIFIED */
            assert((out_upper_write == NULL) || (*out_upper_write == NULL) ||
                   type_is_valid(*out_upper_write)); /* VERIFIED */
            return;
          }
        case SLK_CALL:
          {
            assert(FALSE);
          }
        case SLK_PASS:
          {
            assert(type_is_valid(in_lower_read)); /* VERIFIED */
            assert(type_is_valid(in_upper_read)); /* VERIFIED */
            assert(type_is_valid(in_lower_write)); /* VERIFIED */
            assert(type_is_valid(in_upper_write)); /* VERIFIED */
            (*update_function)(child_update_data, in_lower_read, in_upper_read,
                    in_lower_write, in_upper_write, in_base_variable,
                    out_lower_read, out_upper_read, out_lower_write,
                    out_upper_write, out_base_variable);

            if (!(jumper_flowing_forward(the_jumper)))
                goto error_return;

            assert((out_lower_read == NULL) || (*out_lower_read == NULL) ||
                   type_is_valid(*out_lower_read)); /* VERIFIED */
            assert((out_upper_read == NULL) || (*out_upper_read == NULL) ||
                   type_is_valid(*out_upper_read)); /* VERIFIED */
            assert((out_lower_write == NULL) || (*out_lower_write == NULL) ||
                   type_is_valid(*out_lower_write)); /* VERIFIED */
            assert((out_upper_write == NULL) || (*out_upper_write == NULL) ||
                   type_is_valid(*out_upper_write)); /* VERIFIED */
            return;
          }
        default:
          {
            assert(FALSE);
          }
      }

  error_return:
    assert(!(jumper_flowing_forward(the_jumper)));
    if ((out_lower_read != NULL) && (*out_lower_read != NULL))
      {
        type_remove_reference(*out_lower_read, the_jumper);
        *out_lower_read = NULL;
      }
    if ((out_upper_read != NULL) && (*out_upper_read != NULL))
      {
        type_remove_reference(*out_upper_read, the_jumper);
        *out_upper_read = NULL;
      }
    if ((out_lower_write != NULL) && (*out_lower_write != NULL))
      {
        type_remove_reference(*out_lower_write, the_jumper);
        *out_lower_write = NULL;
      }
    if ((out_upper_write != NULL) && (*out_upper_write != NULL))
      {
        type_remove_reference(*out_upper_write, the_jumper);
        *out_upper_write = NULL;
      }
    if ((out_base_variable != NULL) && (*out_base_variable != NULL))
        *out_base_variable = NULL;

    assert((out_lower_read == NULL) || (*out_lower_read == NULL));
            /* VERIFIED */
    assert((out_upper_read == NULL) || (*out_upper_read == NULL));
            /* VERIFIED */
    assert((out_lower_write == NULL) || (*out_lower_write == NULL));
            /* VERIFIED */
    assert((out_upper_write == NULL) || (*out_upper_write == NULL));
            /* VERIFIED */
  }

static slot_location *create_empty_slot_location(void)
  {
    slot_location *result;

    result = MALLOC_ONE_OBJECT(slot_location);

    result->reference_cluster = NULL;

    INITIALIZE_SYSTEM_LOCK(result->cache_lock, free(result); return NULL);

    INITIALIZE_SYSTEM_LOCK(result->reference_lock,
            DESTROY_SYSTEM_LOCK(result->cache_lock);
            free(result);
            return NULL);

    return result;
  }
