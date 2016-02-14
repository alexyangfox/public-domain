/* file "routine_instance.c" */

/*
 *  This file contains the implementation of the routine_instance module.
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
#include <string.h>
#include "c_foundations/memory_allocation.h"
#include "routine_instance.h"
#include "native_bridge.h"
#include "routine_declaration.h"
#include "context.h"
#include "lock_chain.h"
#include "instance.h"
#include "object.h"
#include "unicode.h"
#include "purity_level.h"
#include "reference_cluster.h"
#include "platform_dependent.h"


struct routine_instance
  {
    routine_declaration *declaration;
    reference_cluster *reference_cluster;
    instance *instance;
    context *parent_context;
    context *static_context;
    boolean scope_exited;
    type *return_type;
    type *augmented_return_type;
    type **argument_types;
    lock_chain *lock_chain;
    boolean has_name;
    DECLARE_SYSTEM_LOCK(live_instance_lock);
    object *live_instances;
    size_t expected_internal_references;
    DECLARE_SYSTEM_LOCK(lock);
    size_t reference_count;
    size_t active_count;
  };


static void set_expected_internal_reference_count(
        routine_instance *the_instance,
        size_t new_expected_internal_references, jumper *the_jumper);


extern routine_instance *create_routine_instance(
        routine_declaration *declaration, purity_level *level,
        reference_cluster *cluster)
  {
    routine_instance *result;
    size_t argument_count;

    assert(declaration != NULL);
    assert(level != NULL);

    result = MALLOC_ONE_OBJECT(routine_instance);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->lock, free(result); return NULL);

    INITIALIZE_SYSTEM_LOCK(result->live_instance_lock,
            DESTROY_SYSTEM_LOCK(result->lock); free(result); return NULL);

    result->declaration = declaration;
    result->reference_cluster = cluster;
    result->instance = create_instance_for_routine(result, level);
    if (result->instance == NULL)
      {
        DESTROY_SYSTEM_LOCK(result->live_instance_lock);
        DESTROY_SYSTEM_LOCK(result->lock);
        free(result);
        return NULL;
      }

    result->parent_context = NULL;
    result->static_context = NULL;
    result->scope_exited = FALSE;
    result->return_type = NULL;
    result->augmented_return_type = NULL;

    argument_count = formal_arguments_argument_count(
            routine_declaration_formals(declaration));

    if (argument_count == 0)
      {
        result->argument_types = NULL;
      }
    else
      {
        type **argument_types;
        size_t argument_num;

        argument_types = MALLOC_ARRAY(type *, argument_count);
        if (argument_types == NULL)
          {
            delete_instance(result->instance);
            DESTROY_SYSTEM_LOCK(result->live_instance_lock);
            DESTROY_SYSTEM_LOCK(result->lock);
            free(result);
            return NULL;
          }

        for (argument_num = 0; argument_num < argument_count; ++argument_num)
            argument_types[argument_num] = NULL;

        result->argument_types = argument_types;
      }

    result->lock_chain = NULL;
    result->has_name = (routine_declaration_name(declaration) != NULL);
    result->live_instances = NULL;
    result->expected_internal_references = 0;
    result->reference_count = 1;
    result->active_count = 0;

    if (result->reference_cluster != NULL)
        reference_cluster_add_internal_reference(result->reference_cluster);

    routine_declaration_add_reference(declaration);

    return result;
  }

extern void routine_instance_add_reference(routine_instance *the_instance)
  {
    routine_instance_add_reference_with_cluster(the_instance, NULL);
  }

extern void routine_instance_remove_reference(routine_instance *the_instance,
                                              jumper *the_jumper)
  {
    routine_instance_remove_reference_with_cluster(the_instance, NULL,
                                                   the_jumper);
  }

extern void routine_instance_add_reference_with_cluster(
        routine_instance *the_instance, reference_cluster *cluster)
  {
    size_t new_reference_count;
    boolean scope_exited;
    context *parent_context;
    size_t expected_internal_references;

    assert(the_instance != NULL);

    GRAB_SYSTEM_LOCK(the_instance->lock);
    assert(the_instance->reference_count > 0);
    ++(the_instance->reference_count);
    new_reference_count = the_instance->reference_count;
    scope_exited = the_instance->scope_exited;
    parent_context = the_instance->parent_context;
    expected_internal_references = the_instance->expected_internal_references;
    RELEASE_SYSTEM_LOCK(the_instance->lock);

    if (the_instance->has_name && (!scope_exited) &&
        (new_reference_count == (2 + expected_internal_references)) &&
        (parent_context != NULL))
      {
        context_add_reference(parent_context);
      }

    if ((the_instance->reference_cluster != NULL) &&
        (the_instance->reference_cluster != cluster))
      {
        reference_cluster_add_reference(the_instance->reference_cluster);
      }
  }

extern void routine_instance_remove_reference_with_cluster(
        routine_instance *the_instance, reference_cluster *cluster,
        jumper *the_jumper)
  {
    size_t new_reference_count;
    boolean scope_was_exited;
    context *parent_context;
    size_t expected_internal_references;
    type **argument_types;
    size_t argument_count;

    assert(the_instance != NULL);

    GRAB_SYSTEM_LOCK(the_instance->lock);
    assert(the_instance->reference_count > 0);
    --(the_instance->reference_count);
    new_reference_count = the_instance->reference_count;
    scope_was_exited = the_instance->scope_exited;
    parent_context = the_instance->parent_context;
    expected_internal_references = the_instance->expected_internal_references;
    RELEASE_SYSTEM_LOCK(the_instance->lock);

    if ((the_instance->reference_cluster != NULL) &&
        (the_instance->reference_cluster != cluster))
      {
        GRAB_SYSTEM_LOCK(the_instance->lock);
        ++(the_instance->reference_count);
        RELEASE_SYSTEM_LOCK(the_instance->lock);
        reference_cluster_remove_reference(the_instance->reference_cluster,
                                           the_jumper);
        assert(the_instance->reference_count > 0);
        GRAB_SYSTEM_LOCK(the_instance->lock);
        --(the_instance->reference_count);
        new_reference_count = the_instance->reference_count;
        RELEASE_SYSTEM_LOCK(the_instance->lock);
      }

    if ((new_reference_count == (1 + expected_internal_references)) &&
        (!scope_was_exited) && (parent_context != NULL) &&
        the_instance->has_name)
      {
        context_remove_reference(parent_context, the_jumper);
      }

    if (new_reference_count > 0)
      {
        if ((new_reference_count == expected_internal_references) &&
            (!scope_was_exited))
          {
            routine_instance_set_scope_exited(the_instance, the_jumper);
          }

        return;
      }

    GRAB_SYSTEM_LOCK(the_instance->lock);
    scope_was_exited = the_instance->scope_exited;
    parent_context = the_instance->parent_context;
    RELEASE_SYSTEM_LOCK(the_instance->lock);

    if ((!(scope_was_exited)) &&
        (the_instance->static_context != NULL))
      {
        exit_context(the_instance->static_context, the_jumper);
      }

    if ((!(scope_was_exited)) && (parent_context != NULL) &&
        !(the_instance->has_name))
      {
        context_remove_reference(parent_context, the_jumper);
      }

    if (the_instance->return_type != NULL)
      {
        type_remove_reference_with_reference_cluster(the_instance->return_type,
                the_jumper, the_instance->reference_cluster);
      }
    if (the_instance->augmented_return_type != NULL)
      {
        type_remove_reference_with_reference_cluster(
                the_instance->augmented_return_type, the_jumper,
                the_instance->reference_cluster);
      }

    argument_types = the_instance->argument_types;
    argument_count = formal_arguments_argument_count(
            routine_declaration_formals(the_instance->declaration));

    if (argument_count == 0)
      {
        assert(argument_types == NULL);
      }
    else
      {
        size_t argument_num;

        for (argument_num = 0; argument_num < argument_count; ++argument_num)
          {
            if (argument_types[argument_num] != NULL)
              {
                type_remove_reference_with_reference_cluster(
                        argument_types[argument_num], the_jumper,
                        the_instance->reference_cluster);
              }
          }
        free(argument_types);
      }

    if (the_instance->lock_chain != NULL)
      {
        lock_chain_remove_reference_with_cluster(the_instance->lock_chain,
                the_instance->reference_cluster, the_jumper);
      }

    if (the_instance->reference_cluster != NULL)
      {
        reference_cluster_remove_internal_reference(
                the_instance->reference_cluster);
      }

    delete_instance(the_instance->instance);

    routine_declaration_remove_reference(the_instance->declaration);

    DESTROY_SYSTEM_LOCK(the_instance->live_instance_lock);
    DESTROY_SYSTEM_LOCK(the_instance->lock);
    free(the_instance);
  }

extern reference_cluster *routine_instance_reference_cluster(
        routine_instance *the_instance)
  {
    assert(the_instance != NULL);

    return the_instance->reference_cluster;
  }

extern routine_declaration *routine_instance_declaration(
        routine_instance *instance)
  {
    assert(instance != NULL);

    return instance->declaration;
  }

extern context *routine_instance_context(routine_instance *the_instance)
  {
    assert(the_instance != NULL);

    assert(routine_instance_is_instantiated(the_instance)); /* VERIFIED */
    assert(!(the_instance->scope_exited)); /* VERIFIED */

    return the_instance->static_context;
  }

extern boolean routine_instance_is_instantiated(routine_instance *the_instance)
  {
    assert(the_instance != NULL);

    assert(the_instance->instance != NULL);
    return instance_is_instantiated(the_instance->instance);
  }

extern boolean routine_instance_scope_exited(routine_instance *the_instance)
  {
    assert(the_instance != NULL);

    return the_instance->scope_exited;
  }

extern type *routine_instance_return_type(routine_instance *the_instance)
  {
    assert(the_instance != NULL);

    assert(routine_instance_is_instantiated(the_instance)); /* VERIFIED */
    assert(!(the_instance->scope_exited)); /* VERIFIED */

    return the_instance->return_type;
  }

extern type *routine_instance_valid_return_type(routine_instance *the_instance,
        const source_location *location, jumper *the_jumper)
  {
    type *result;
    boolean augment_needed;

    assert(the_instance != NULL);
    assert(the_jumper != NULL);

    assert(routine_instance_is_instantiated(the_instance)); /* VERIFIED */
    assert(!(the_instance->scope_exited)); /* VERIFIED */
    assert(jumper_flowing_forward(the_jumper));

    result = the_instance->augmented_return_type;
    augment_needed = (result == NULL);
    if (augment_needed)
        result = the_instance->return_type;
    assert(result != NULL);

    check_type_validity(result, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    if (augment_needed)
      {
        if (routine_declaration_is_class(the_instance->declaration))
          {
            type *class_type;

            class_type = get_class_type(the_instance);
            if (class_type == NULL)
              {
                jumper_do_abort(the_jumper);
                return NULL;
              }

            assert(type_is_valid(result)); /* VERIFIED */
            assert(type_is_valid(class_type)); /* VERIFIED */
            result = get_intersection_type(result, class_type);
            type_remove_reference(class_type, the_jumper);
            assert(jumper_flowing_forward(the_jumper));
            if (result == NULL)
              {
                jumper_do_abort(the_jumper);
                return NULL;
              }

            set_expected_internal_reference_count(the_instance, 1, the_jumper);
            assert(jumper_flowing_forward(the_jumper));

            type_add_reference_with_reference_cluster(result,
                    the_instance->reference_cluster);
            type_remove_reference(result, the_jumper);
            assert(jumper_flowing_forward(the_jumper));
          }
        else
          {
            type_add_reference_with_reference_cluster(result,
                    the_instance->reference_cluster);
          }

        GRAB_SYSTEM_LOCK(the_instance->lock);
        if (the_instance->augmented_return_type == NULL)
            the_instance->augmented_return_type = result;
        else
            type_remove_reference(result, the_jumper);
        RELEASE_SYSTEM_LOCK(the_instance->lock);
        assert(jumper_flowing_forward(the_jumper));

        result = the_instance->augmented_return_type;
      }

    return result;
  }

extern type *routine_instance_argument_type(routine_instance *the_instance,
                                            size_t argument_number)
  {
    assert(the_instance != NULL);

    assert(routine_instance_is_instantiated(the_instance)); /* VERIFIED */
    assert(!(the_instance->scope_exited)); /* VERIFIED */

    return the_instance->argument_types[argument_number];
  }

extern lock_chain *routine_instance_lock_chain(routine_instance *the_instance)
  {
    assert(the_instance != NULL);

    assert(routine_instance_is_instantiated(the_instance)); /* VERIFIED */
    assert(!(the_instance->scope_exited)); /* VERIFIED */

    return the_instance->lock_chain;
  }

extern instance *routine_instance_instance(routine_instance *the_instance)
  {
    assert(the_instance != NULL);

    return the_instance->instance;
  }

extern boolean routine_instance_is_active(routine_instance *the_instance)
  {
    assert(the_instance != NULL);

    return (the_instance->active_count > 0);
  }

extern void routine_instance_set_return_type(routine_instance *the_instance,
        type *return_type, size_t new_expected_internal_references,
        jumper *the_jumper)
  {
    type *old_return_type;

    assert(the_instance != NULL);

    assert(!(the_instance->scope_exited)); /* VERIFIED */

    old_return_type = the_instance->return_type;
    if (return_type != NULL)
      {
        if ((the_instance->reference_cluster == NULL) &&
            (the_instance->reference_count == 1))
          {
            the_instance->reference_cluster =
                    type_reference_cluster(return_type);
            if (the_instance->reference_cluster != NULL)
              {
                reference_cluster_add_reference(
                        the_instance->reference_cluster);
                reference_cluster_add_internal_reference(
                        the_instance->reference_cluster);
              }
          }
        type_add_reference_with_reference_cluster(return_type,
                the_instance->reference_cluster);
      }
    the_instance->return_type = return_type;

    set_expected_internal_reference_count(the_instance,
            new_expected_internal_references, the_jumper);

    if (old_return_type != NULL)
      {
        type_remove_reference_with_reference_cluster(old_return_type,
                the_jumper, the_instance->reference_cluster);
      }
  }

extern void routine_instance_set_argument_type(routine_instance *the_instance,
        type *argument_type, size_t argument_number, jumper *the_jumper)
  {
    assert(the_instance != NULL);

    assert(!(the_instance->scope_exited)); /* VERIFIED */

    if (argument_type != NULL)
      {
        if ((the_instance->reference_cluster == NULL) &&
            (the_instance->reference_count == 1))
          {
            the_instance->reference_cluster =
                    type_reference_cluster(argument_type);
            if (the_instance->reference_cluster != NULL)
              {
                reference_cluster_add_reference(
                        the_instance->reference_cluster);
                reference_cluster_add_internal_reference(
                        the_instance->reference_cluster);
              }
          }
        type_add_reference_with_reference_cluster(argument_type,
                the_instance->reference_cluster);
      }
    if (the_instance->argument_types[argument_number] != NULL)
      {
        type_remove_reference_with_reference_cluster(
                the_instance->argument_types[argument_number], the_jumper,
                the_instance->reference_cluster);
      }
    the_instance->argument_types[argument_number] = argument_type;
  }

extern void set_routine_instance_lock_chain(routine_instance *the_instance,
        lock_chain *the_lock_chain, jumper *the_jumper)
  {
    assert(the_instance != NULL);

    assert(!(the_instance->scope_exited)); /* VERIFIED */

    if (the_lock_chain != NULL)
      {
        lock_chain_add_reference_with_cluster(the_lock_chain,
                                              the_instance->reference_cluster);
      }
    if (the_instance->lock_chain != NULL)
      {
        lock_chain_remove_reference_with_cluster(the_instance->lock_chain,
                the_instance->reference_cluster, the_jumper);
      }
    the_instance->lock_chain = the_lock_chain;
  }

extern void routine_instance_set_up_static_context(
        routine_instance *the_instance, context *the_context,
        jumper *the_jumper)
  {
    size_t reference_count;
    size_t expected_internal_references;

    assert(the_instance != NULL);

    assert(!(the_instance->scope_exited)); /* VERIFIED */

    GRAB_SYSTEM_LOCK(the_instance->lock);
    reference_count = the_instance->reference_count;
    expected_internal_references = the_instance->expected_internal_references;
    the_instance->parent_context = the_context;
    RELEASE_SYSTEM_LOCK(the_instance->lock);

    if ((!(the_instance->has_name)) ||
        (reference_count > (1 + expected_internal_references)))
      {
        context_add_reference(the_context);
      }
    the_instance->static_context = create_static_context(the_context,
            routine_declaration_static_home(the_instance->declaration),
            the_jumper);
  }

extern void routine_instance_set_scope_exited(routine_instance *the_instance,
                                              jumper *the_jumper)
  {
    size_t reference_count;
    boolean scope_was_exited;
    context *parent_context;
    size_t old_expected_internal_references;
    type **argument_types;
    size_t argument_count;
    size_t argument_num;

    assert(the_instance != NULL);

    while (TRUE)
      {
        GRAB_SYSTEM_LOCK(the_instance->live_instance_lock);

        if (the_instance->live_instances == NULL)
          {
            RELEASE_SYSTEM_LOCK(the_instance->live_instance_lock);
            break;
          }

        /* The following procedure call will release the lock. */
        close_object_for_class_exit(the_instance->live_instances, the_jumper);
      }

    routine_instance_add_reference(the_instance);

    mark_instance_scope_exited(the_instance->instance);

    GRAB_SYSTEM_LOCK(the_instance->lock);
    scope_was_exited = the_instance->scope_exited;
    the_instance->scope_exited = TRUE;
    reference_count = the_instance->reference_count;
    parent_context = the_instance->parent_context;
    old_expected_internal_references =
            the_instance->expected_internal_references;
    the_instance->expected_internal_references = 0;
    RELEASE_SYSTEM_LOCK(the_instance->lock);

    if ((!scope_was_exited) && (the_instance->static_context != NULL))
        exit_context(the_instance->static_context, the_jumper);
    if ((!scope_was_exited) && (parent_context != NULL) &&
        the_instance->has_name)
      {
        if (reference_count > 1 + old_expected_internal_references)
            context_remove_reference(parent_context, the_jumper);
      }
    if ((!scope_was_exited) && (parent_context != NULL) &&
        !(the_instance->has_name))
      {
        context_remove_reference(parent_context, the_jumper);
      }

    if (the_instance->return_type != NULL)
      {
        type_remove_reference_with_reference_cluster(the_instance->return_type,
                the_jumper, the_instance->reference_cluster);
      }
    the_instance->return_type = NULL;
    if (the_instance->augmented_return_type != NULL)
      {
        type_remove_reference_with_reference_cluster(
                the_instance->augmented_return_type, the_jumper,
                the_instance->reference_cluster);
      }
    the_instance->augmented_return_type = NULL;

    argument_types = the_instance->argument_types;
    argument_count = formal_arguments_argument_count(
            routine_declaration_formals(the_instance->declaration));

    for (argument_num = 0; argument_num < argument_count; ++argument_num)
      {
        if (argument_types[argument_num] != NULL)
          {
            type_remove_reference_with_reference_cluster(
                    argument_types[argument_num], the_jumper,
                    the_instance->reference_cluster);
          }
        argument_types[argument_num] = NULL;
      }

    if (the_instance->lock_chain != NULL)
      {
        lock_chain_remove_reference_with_cluster(the_instance->lock_chain,
                the_instance->reference_cluster, the_jumper);
      }
    the_instance->lock_chain = NULL;

    routine_instance_remove_reference(the_instance, the_jumper);
  }

extern void increment_routine_instance_active_count(
        routine_instance *the_instance)
  {
    assert(the_instance != NULL);

    assert(the_instance->active_count < ((size_t)-1));
    ++(the_instance->active_count);
  }

extern void decrement_routine_instance_active_count(
        routine_instance *the_instance)
  {
    assert(the_instance != NULL);

    assert(the_instance->active_count > 0);
    --(the_instance->active_count);
  }

extern void routine_instance_lock_live_instance_list(
        routine_instance *the_instance)
  {
    assert(the_instance != NULL);

    GRAB_SYSTEM_LOCK(the_instance->live_instance_lock);
  }

extern void routine_instance_unlock_live_instance_list(
        routine_instance *the_instance)
  {
    assert(the_instance != NULL);

    RELEASE_SYSTEM_LOCK(the_instance->live_instance_lock);
  }

extern object *routine_instance_first_live_instance(
        routine_instance *the_instance)
  {
    assert(the_instance != NULL);

    return the_instance->live_instances;
  }

extern void routine_instance_set_first_live_instance(
        routine_instance *the_instance, object *new_first_live_instance)
  {
    assert(the_instance != NULL);

    the_instance->live_instances = new_first_live_instance;
  }

extern size_t *resolve_parameter_ordering_from_semi_ordered_value_list(
        formal_arguments *formals, semi_labeled_value_list *pre_order_actuals,
        boolean extra_arguments_allowed, boolean *error,
        size_t *post_order_count, size_t *duplicate_formal_argument_num,
        size_t *bad_name_actual_argument_num)
  {
    size_t pre_order_count;
    const char **pre_order_actual_names;
    size_t *result;

    assert(formals != NULL);
    assert(pre_order_actuals != NULL);
    assert(error != NULL);
    assert(post_order_count != NULL);
    assert(duplicate_formal_argument_num != NULL);
    assert(bad_name_actual_argument_num != NULL);

    pre_order_count = semi_labeled_value_list_value_count(pre_order_actuals);

    if (pre_order_count == 0)
      {
        pre_order_actual_names = NULL;
      }
    else
      {
        size_t pre_order_num;

        pre_order_actual_names = MALLOC_ARRAY(const char *, pre_order_count);
        if (pre_order_actual_names == NULL)
          {
            *error = TRUE;
            return NULL;
          }

        for (pre_order_num = 0; pre_order_num < pre_order_count;
             ++pre_order_num)
          {
            pre_order_actual_names[pre_order_num] =
                    semi_labeled_value_list_label(pre_order_actuals,
                                                  pre_order_num);
          }
      }

    result = resolve_parameter_ordering_from_name_array(formals,
            pre_order_count, pre_order_actual_names, extra_arguments_allowed,
            error, post_order_count, duplicate_formal_argument_num,
            bad_name_actual_argument_num);
    if (pre_order_actual_names != NULL)
        free(pre_order_actual_names);
    return result;
  }

extern size_t *resolve_parameter_ordering_from_name_array(
        formal_arguments *formals, size_t pre_order_count,
        const char **pre_order_actual_names, boolean extra_arguments_allowed,
        boolean *error, size_t *post_order_count,
        size_t *duplicate_formal_argument_num,
        size_t *bad_name_actual_argument_num)
  {
    size_t formal_count;
    size_t *result;
    size_t local_post_order_count;
    size_t post_order_num;
    size_t pre_order_num;

    assert(formals != NULL);
    assert((pre_order_count == 0) || (pre_order_actual_names != NULL));
    assert(error != NULL);
    assert(post_order_count != NULL);
    assert(duplicate_formal_argument_num != NULL);
    assert(bad_name_actual_argument_num != NULL);

    if (pre_order_count == 0)
      {
        *error = FALSE;
        *post_order_count = 0;
        return NULL;
      }

    formal_count = formal_arguments_argument_count(formals);

    result = MALLOC_ARRAY(size_t, pre_order_count + formal_count);
    if (result == NULL)
      {
        *error = TRUE;
        return NULL;
      }

    local_post_order_count = 0;
    post_order_num = 0;

    for (pre_order_num = 0; pre_order_num < pre_order_count; ++pre_order_num)
      {
        const char *label;

        label = pre_order_actual_names[pre_order_num];

        if (label != NULL)
          {
            size_t formal_num;

            for (formal_num = 0; formal_num < formal_count; ++formal_num)
              {
                variable_declaration *this_formal;
                const char *formal_name;

                this_formal =
                        formal_arguments_formal_by_number(formals, formal_num);
                formal_name = variable_declaration_name(this_formal);
                if ((formal_name != NULL) && (strcmp(label, formal_name) == 0))
                  {
                    post_order_num = formal_num;
                    break;
                  }
              }

            if (formal_num >= formal_count)
              {
                if (!extra_arguments_allowed)
                  {
                    free(result);
                    *error = FALSE;
                    *duplicate_formal_argument_num = formal_count;
                    *bad_name_actual_argument_num = pre_order_num;
                    return NULL;
                  }

                post_order_num = formal_count;
                if (post_order_num < local_post_order_count)
                    post_order_num = local_post_order_count;
              }
          }

        assert(post_order_num < pre_order_count + formal_count);

        while (local_post_order_count <= post_order_num)
          {
            assert(local_post_order_count < pre_order_count + formal_count);
            result[local_post_order_count] = pre_order_count;
            ++local_post_order_count;
          }

        if (result[post_order_num] != pre_order_count)
          {
            assert(post_order_num < formal_count);
            free(result);
            *error = FALSE;
            *duplicate_formal_argument_num = post_order_num;
            return NULL;
          }

        result[post_order_num] = pre_order_num;

        ++post_order_num;
      }

    assert(local_post_order_count <= pre_order_count + formal_count);
    *post_order_count = local_post_order_count;

    *error = FALSE;
    return result;
  }

extern boolean routine_instances_are_equal(routine_instance *routine1,
                                           routine_instance *routine2)
  {
    assert(routine1 != NULL);
    assert(routine2 != NULL);

    assert(routine_instance_is_instantiated(routine1)); /* VERIFIED */
    assert(routine_instance_is_instantiated(routine2)); /* VERIFIED */
    assert(!(routine1->scope_exited)); /* VERIFIED */
    assert(!(routine2->scope_exited)); /* VERIFIED */

    return (routine1 == routine2);
  }

extern int routine_instance_structural_order(routine_instance *left,
                                             routine_instance *right)
  {
    const char *left_name;
    const char *right_name;

    assert(left != NULL);
    assert(right != NULL);

    assert(routine_instance_is_instantiated(left)); /* VERIFIED */
    assert(routine_instance_is_instantiated(right)); /* VERIFIED */
    assert(!(left->scope_exited)); /* VERIFIED */
    assert(!(right->scope_exited)); /* VERIFIED */

    if (left == right)
        return 0;

    left_name = routine_declaration_name(left->declaration);
    right_name = routine_declaration_name(right->declaration);

    if ((left_name == NULL) && (right_name != NULL))
        return -1;
    if ((left_name != NULL) && (right_name == NULL))
        return 1;
    if ((left_name != NULL) && (right_name != NULL))
      {
        int name_order;

        name_order = utf8_string_lexicographical_order_by_code_point(left_name,
                right_name);
        if (name_order != 0)
            return name_order;
      }

    if (left < right)
        return -1;
    else
        return 1;
  }


static void set_expected_internal_reference_count(
        routine_instance *the_instance,
        size_t new_expected_internal_references, jumper *the_jumper)
  {
    size_t reference_count;
    size_t old_expected_internal_references;

    assert(the_instance != NULL);

    GRAB_SYSTEM_LOCK(the_instance->lock);
    reference_count = the_instance->reference_count;
    old_expected_internal_references =
            the_instance->expected_internal_references;
    the_instance->expected_internal_references =
            new_expected_internal_references;
    RELEASE_SYSTEM_LOCK(the_instance->lock);

    if (the_instance->has_name && (the_instance->parent_context != NULL) &&
        (reference_count <= (1 + old_expected_internal_references)) &&
        (reference_count > (1 + new_expected_internal_references)))
      {
        context_add_reference(the_instance->parent_context);
      }
    if (the_instance->has_name && (the_instance->parent_context != NULL) &&
        (reference_count <= (1 + new_expected_internal_references)) &&
        (reference_count > (1 + old_expected_internal_references)))
      {
        context_remove_reference(the_instance->parent_context, the_jumper);
      }
  }
