/* file "variable_instance.c" */

/*
 *  This file contains the implementation of the variable_instance module.
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
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "variable_instance.h"
#include "variable_declaration.h"
#include "type.h"
#include "value.h"
#include "lock_chain.h"
#include "trace_channels.h"
#include "instance.h"
#include "reference_cluster.h"
#include "platform_dependent.h"


struct variable_instance
  {
    variable_declaration *declaration;
    reference_cluster *reference_cluster;
    instance *instance;
    boolean scope_exited;
    type *type;
    DECLARE_SYSTEM_LOCK(value_lock);
    value *value;
    lock_chain *lock_chain;
    DECLARE_SYSTEM_LOCK(lock);
    size_t reference_count;
  };


extern variable_instance *create_variable_instance(
        variable_declaration *declaration, purity_level *level,
        reference_cluster *cluster)
  {
    variable_instance *result;

    assert(declaration != NULL);
    assert(level != NULL);

    result = MALLOC_ONE_OBJECT(variable_instance);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->value_lock, free(result); return NULL);
    INITIALIZE_SYSTEM_LOCK(result->lock,
            DESTROY_SYSTEM_LOCK(result->value_lock);
            free(result);
            return NULL);

    result->declaration = declaration;
    result->reference_cluster = cluster;
    result->instance = create_instance_for_variable(result, level);
    if (result->instance == NULL)
      {
        DESTROY_SYSTEM_LOCK(result->lock);
        DESTROY_SYSTEM_LOCK(result->value_lock);
        free(result);
        return NULL;
      }

    result->scope_exited = FALSE;
    result->type = NULL;
    result->value = NULL;
    result->lock_chain = NULL;
    result->reference_count = 1;

    variable_declaration_add_reference(declaration);

    return result;
  }

extern variable_declaration *variable_instance_declaration(
        variable_instance *the_variable_instance)
  {
    assert(the_variable_instance != NULL);

    return the_variable_instance->declaration;
  }

extern boolean variable_instance_is_instantiated(
        variable_instance *the_variable_instance)
  {
    assert(the_variable_instance != NULL);

    assert(the_variable_instance->instance != NULL);
    return instance_is_instantiated(the_variable_instance->instance);
  }

extern boolean variable_instance_scope_exited(
        variable_instance *the_variable_instance)
  {
    assert(the_variable_instance != NULL);

    return the_variable_instance->scope_exited;
  }

extern type *variable_instance_type(variable_instance *the_variable_instance)
  {
    assert(the_variable_instance != NULL);

    assert(!(the_variable_instance->scope_exited)); /* VERIFIED */
    assert(variable_instance_is_instantiated(the_variable_instance));
            /* VERIFIED */

    return the_variable_instance->type;
  }

extern value *variable_instance_value(variable_instance *the_variable_instance)
  {
    value *result;

    assert(the_variable_instance != NULL);

    GRAB_SYSTEM_LOCK(the_variable_instance->value_lock);

    assert(!(the_variable_instance->scope_exited)); /* VERIFIED */
    assert(variable_instance_is_instantiated(the_variable_instance));
            /* VERIFIED */

    result = the_variable_instance->value;
    if (result != NULL)
        value_add_reference(result);

    RELEASE_SYSTEM_LOCK(the_variable_instance->value_lock);

    return result;
  }

extern lock_chain *variable_instance_lock_chain(
        variable_instance *the_variable_instance)
  {
    assert(the_variable_instance != NULL);

    assert(!(the_variable_instance->scope_exited)); /* VERIFIED */
    assert(variable_instance_is_instantiated(the_variable_instance));
            /* VERIFIED */

    return the_variable_instance->lock_chain;
  }

extern instance *variable_instance_instance(
        variable_instance *the_variable_instance)
  {
    assert(the_variable_instance != NULL);

    return the_variable_instance->instance;
  }

extern void set_variable_instance_type(
        variable_instance *the_variable_instance, type *the_type,
        jumper *the_jumper)
  {
    type *old_type;

    assert(the_variable_instance != NULL);

    assert(!(the_variable_instance->scope_exited)); /* VERIFIED */

    if (the_type != NULL)
      {
        type_add_reference_with_reference_cluster(the_type,
                the_variable_instance->reference_cluster);
      }
    old_type = the_variable_instance->type;
    the_variable_instance->type = the_type;
    if (old_type != NULL)
      {
        type_remove_reference_with_reference_cluster(old_type, the_jumper,
                the_variable_instance->reference_cluster);
      }
  }

extern void set_variable_instance_value(
        variable_instance *the_variable_instance, value *the_value,
        jumper *the_jumper)
  {
    value *old_value;

    assert(the_variable_instance != NULL);

    if (the_value != NULL)
      {
        value_add_reference_with_reference_cluster(the_value,
                the_variable_instance->reference_cluster);
      }

    GRAB_SYSTEM_LOCK(the_variable_instance->value_lock);

    assert(!(the_variable_instance->scope_exited)); /* VERIFIED */

    if (the_value == NULL)
      {
        trace(jumper_tracer(the_jumper), TC_ASSIGNMENTS, "Undefining %v.",
              variable_instance_declaration(the_variable_instance));
      }
    else
      {
        trace(jumper_tracer(the_jumper), TC_ASSIGNMENTS,
              "Assigning value %U to %v.", the_value,
              variable_instance_declaration(the_variable_instance));
      }

    old_value = the_variable_instance->value;
    the_variable_instance->value = the_value;

    RELEASE_SYSTEM_LOCK(the_variable_instance->value_lock);

    if (old_value != NULL)
      {
        value_remove_reference_with_reference_cluster(old_value, the_jumper,
                the_variable_instance->reference_cluster);
      }
  }

extern void set_variable_instance_lock_chain(
        variable_instance *the_variable_instance, lock_chain *the_lock_chain,
        jumper *the_jumper)
  {
    lock_chain *old_lock_chain;

    assert(the_variable_instance != NULL);

    assert(!(the_variable_instance->scope_exited)); /* VERIFIED */

    if (the_lock_chain != NULL)
      {
        lock_chain_add_reference_with_cluster(the_lock_chain,
                the_variable_instance->reference_cluster);
      }
    old_lock_chain = the_variable_instance->lock_chain;
    the_variable_instance->lock_chain = the_lock_chain;
    if (old_lock_chain != NULL)
      {
        lock_chain_remove_reference_with_cluster(old_lock_chain,
                the_variable_instance->reference_cluster, the_jumper);
      }
  }

extern void set_variable_instance_instantiated(
        variable_instance *the_variable_instance)
  {
    assert(the_variable_instance != NULL);

    assert(!(the_variable_instance->scope_exited)); /* VERIFIED */

    assert(the_variable_instance->instance != NULL);
    set_instance_instantiated(the_variable_instance->instance);
  }

extern void set_variable_instance_scope_exited(
        variable_instance *the_variable_instance, jumper *the_jumper)
  {
    value *the_value;
    type *the_type;
    lock_chain *the_lock_chain;

    assert(the_variable_instance != NULL);

    GRAB_SYSTEM_LOCK(the_variable_instance->value_lock);

    assert(!(the_variable_instance->scope_exited)); /* VERIFIED */

    the_variable_instance->scope_exited = TRUE;

    mark_instance_scope_exited(the_variable_instance->instance);

    the_value = the_variable_instance->value;
    the_variable_instance->value = NULL;

    the_type = the_variable_instance->type;
    the_variable_instance->type = NULL;

    the_lock_chain = the_variable_instance->lock_chain;
    the_variable_instance->lock_chain = NULL;

    RELEASE_SYSTEM_LOCK(the_variable_instance->value_lock);

    if (the_value != NULL)
      {
        value_remove_reference_with_reference_cluster(the_value, the_jumper,
                the_variable_instance->reference_cluster);
      }

    if (the_type != NULL)
      {
        type_remove_reference_with_reference_cluster(the_type, the_jumper,
                the_variable_instance->reference_cluster);
      }

    if (the_lock_chain != NULL)
      {
        lock_chain_remove_reference_with_cluster(the_lock_chain,
                the_variable_instance->reference_cluster, the_jumper);
      }
  }

extern void variable_instance_add_reference(
        variable_instance *the_variable_instance)
  {
    assert(the_variable_instance != NULL);
    assert_is_malloced_block_with_exact_size(the_variable_instance,
                                             sizeof(variable_instance));

    GRAB_SYSTEM_LOCK(the_variable_instance->lock);
    assert(the_variable_instance->reference_count > 0);
    ++(the_variable_instance->reference_count);
    RELEASE_SYSTEM_LOCK(the_variable_instance->lock);

    if (the_variable_instance->reference_cluster != NULL)
      {
        reference_cluster_add_reference(
                the_variable_instance->reference_cluster);
      }
  }

extern void variable_instance_remove_reference(
        variable_instance *the_variable_instance, jumper *the_jumper)
  {
    size_t new_reference_count;

    assert(the_variable_instance != NULL);
    assert_is_malloced_block_with_exact_size(the_variable_instance,
                                             sizeof(variable_instance));

    GRAB_SYSTEM_LOCK(the_variable_instance->lock);
    assert(the_variable_instance->reference_count > 0);
    --(the_variable_instance->reference_count);
    new_reference_count = the_variable_instance->reference_count;
    RELEASE_SYSTEM_LOCK(the_variable_instance->lock);

    if (the_variable_instance->reference_cluster != NULL)
      {
        reference_cluster_remove_reference(
                the_variable_instance->reference_cluster, the_jumper);
      }

    if (new_reference_count > 0)
        return;

    if (!(the_variable_instance->scope_exited))
        set_variable_instance_scope_exited(the_variable_instance, the_jumper);

    assert(the_variable_instance->scope_exited);
    assert(the_variable_instance->value == NULL);
    assert(the_variable_instance->type == NULL);
    assert(the_variable_instance->lock_chain == NULL);

    delete_instance(the_variable_instance->instance);

    variable_declaration_remove_reference(the_variable_instance->declaration);

    DESTROY_SYSTEM_LOCK(the_variable_instance->lock);
    DESTROY_SYSTEM_LOCK(the_variable_instance->value_lock);
    free(the_variable_instance);
  }

extern void variable_instance_add_reference_with_cluster(
        variable_instance *the_variable_instance, reference_cluster *cluster)
  {
    assert(the_variable_instance != NULL);
    assert_is_malloced_block_with_exact_size(the_variable_instance,
                                             sizeof(variable_instance));

    GRAB_SYSTEM_LOCK(the_variable_instance->lock);
    assert(the_variable_instance->reference_count > 0);
    ++(the_variable_instance->reference_count);
    RELEASE_SYSTEM_LOCK(the_variable_instance->lock);

    if ((the_variable_instance->reference_cluster != NULL) &&
        (the_variable_instance->reference_cluster != cluster))
      {
        reference_cluster_add_reference(
                the_variable_instance->reference_cluster);
      }
  }

extern void variable_instance_remove_reference_with_cluster(
        variable_instance *the_variable_instance, jumper *the_jumper,
        reference_cluster *cluster)
  {
    size_t new_reference_count;

    assert(the_variable_instance != NULL);
    assert_is_malloced_block_with_exact_size(the_variable_instance,
                                             sizeof(variable_instance));

    GRAB_SYSTEM_LOCK(the_variable_instance->lock);
    assert(the_variable_instance->reference_count > 0);
    --(the_variable_instance->reference_count);
    new_reference_count = the_variable_instance->reference_count;
    RELEASE_SYSTEM_LOCK(the_variable_instance->lock);

    if ((the_variable_instance->reference_cluster != NULL) &&
        (the_variable_instance->reference_cluster != cluster))
      {
        reference_cluster_remove_reference(
                the_variable_instance->reference_cluster, the_jumper);
      }

    if (new_reference_count > 0)
        return;

    if (!(the_variable_instance->scope_exited))
        set_variable_instance_scope_exited(the_variable_instance, the_jumper);

    assert(the_variable_instance->scope_exited);
    assert(the_variable_instance->value == NULL);
    assert(the_variable_instance->type == NULL);
    assert(the_variable_instance->lock_chain == NULL);

    delete_instance(the_variable_instance->instance);

    variable_declaration_remove_reference(the_variable_instance->declaration);

    DESTROY_SYSTEM_LOCK(the_variable_instance->lock);
    DESTROY_SYSTEM_LOCK(the_variable_instance->value_lock);
    free(the_variable_instance);
  }

extern reference_cluster *variable_instance_reference_cluster(
        variable_instance *the_variable_instance)
  {
    assert(the_variable_instance != NULL);

    return the_variable_instance->reference_cluster;
  }
