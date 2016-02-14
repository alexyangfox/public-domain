/* file "lock_instance.c" */

/*
 *  This file contains the implementation of the lock_instance module.
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
#include "c_foundations/basic.h"
#include "c_foundations/trace.h"
#include "c_foundations/memory_allocation.h"
#include "lock_instance.h"
#include "lock_declaration.h"
#include "jumper.h"
#include "o_integer.h"
#include "lock_chain.h"
#include "trace_channels.h"
#include "instance.h"
#include "reference_cluster.h"
#include "purity_level.h"
#include "thread.h"
#include "platform_dependent.h"


struct lock_instance
  {
    lock_declaration *declaration;
    reference_cluster *reference_cluster;
    instance *instance;
    lock_chain *lock_chain;
    boolean scope_exited;
    DECLARE_SYSTEM_LOCK(hold_lock);
    salmon_thread *owner;
    salmon_thread *next_to_awaken;
    o_integer grab_count;
    DECLARE_SYSTEM_LOCK(reference_lock);
    boolean is_context_switching;
    size_t reference_count;
  };


extern lock_instance *create_lock_instance(lock_declaration *declaration,
        purity_level *level, reference_cluster *cluster)
  {
    lock_instance *result;

    assert(declaration != NULL);

    result = MALLOC_ONE_OBJECT(lock_instance);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock, free(result); return NULL);
    INITIALIZE_SYSTEM_LOCK(result->hold_lock,
            DESTROY_SYSTEM_LOCK(result->reference_lock); free(result);
            return NULL);

    result->declaration = declaration;
    result->reference_cluster = cluster;
    result->instance = create_instance_for_lock(result, level);
    if (result->instance == NULL)
      {
        DESTROY_SYSTEM_LOCK(result->hold_lock);
        DESTROY_SYSTEM_LOCK(result->reference_lock);
        free(result);
        return NULL;
      }

    result->scope_exited = FALSE;
    result->owner = NULL;
    result->next_to_awaken = NULL;
    result->grab_count = oi_zero;
    assert(!(oi_out_of_memory(result->grab_count)));
    oi_add_reference(oi_zero);
    result->lock_chain = NULL;
    result->is_context_switching = FALSE;
    result->reference_count = 1;

    lock_declaration_add_reference(declaration);

    return result;
  }

extern lock_declaration *lock_instance_declaration(lock_instance *instance)
  {
    assert(instance != NULL);

    return instance->declaration;
  }

extern boolean lock_instance_is_instantiated(lock_instance *instance)
  {
    assert(instance != NULL);

    assert(instance->instance != NULL);
    return instance_is_instantiated(instance->instance);
  }

extern boolean lock_instance_scope_exited(lock_instance *instance)
  {
    assert(instance != NULL);

    return instance->scope_exited;
  }

extern instance *lock_instance_instance(lock_instance *instance)
  {
    assert(instance != NULL);

    return instance->instance;
  }

extern void set_lock_instance_lock_chain(lock_instance *instance,
        lock_chain *the_lock_chain, jumper *the_jumper)
  {
    assert(instance != NULL);

    assert(!(instance->scope_exited)); /* VERIFIED */

    if (the_lock_chain != NULL)
      {
        lock_chain_add_reference_with_cluster(the_lock_chain,
                                              instance->reference_cluster);
      }
    if (instance->lock_chain != NULL)
      {
        lock_chain_remove_reference_with_cluster(instance->lock_chain,
                instance->reference_cluster, the_jumper);
      }
    instance->lock_chain = the_lock_chain;
  }

extern void set_lock_instance_is_context_switching(lock_instance *instance)
  {
    assert(instance != NULL);

    instance->is_context_switching = TRUE;
  }

extern void set_lock_instance_scope_exited(lock_instance *instance,
                                           jumper *the_jumper)
  {
    assert(instance != NULL);

    assert(!(instance->scope_exited)); /* VERIFIED */

    mark_instance_scope_exited(instance->instance);
    instance->scope_exited = TRUE;

    if (instance->lock_chain != NULL)
      {
        lock_chain_remove_reference_with_cluster(instance->lock_chain,
                instance->reference_cluster, the_jumper);
      }
    instance->lock_chain = NULL;
  }

extern void lock_instance_grab(lock_instance *instance,
        const source_location *location, jumper *the_jumper)
  {
    salmon_thread *my_thread;
    o_integer new_count;
    boolean just_grabbed;
    o_integer old_count;

    assert(instance != NULL);
    assert(the_jumper != NULL);

    assert(jumper_flowing_forward(the_jumper));

    assert(instance->instance != NULL);
    if (!(instance_is_instantiated(instance->instance)))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(lock_grab_uninstantiated),
                "An attempt was made to grab a lock before the lock had been "
                "instantiated by executing its declaration.");
        return;
      }

    if (instance->scope_exited)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(lock_grab_deallocated),
                "An attempt was made to grab a lock after it had been "
                "de-allocated.");
        return;
      }

    if (instance->lock_chain != NULL)
        lock_chain_grab(instance->lock_chain, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return;

    my_thread = jumper_thread(the_jumper);

    GRAB_SYSTEM_LOCK(instance->hold_lock);

    while ((instance->owner != my_thread) && (instance->owner != NULL))
      {
        salmon_thread *blocker;
        salmon_thread *next;

        blocker = instance->owner;
        next = instance->next_to_awaken;
        instance->next_to_awaken = my_thread;

        RELEASE_SYSTEM_LOCK(instance->hold_lock);

        block(my_thread, blocker, instance, the_jumper, location);

        if (next != NULL)
            unblock(next);

        if (!(jumper_flowing_forward(the_jumper)))
            return;

        GRAB_SYSTEM_LOCK(instance->hold_lock);
      }

    assert(!(oi_out_of_memory(instance->grab_count)));
    assert(oi_kind(instance->grab_count) == IIK_FINITE);
    assert(!(oi_is_negative(instance->grab_count)));

    oi_add(new_count, instance->grab_count, oi_one);
    if (oi_out_of_memory(new_count))
      {
        RELEASE_SYSTEM_LOCK(instance->hold_lock);
        jumper_do_abort(the_jumper);
        if (instance->lock_chain != NULL)
            lock_chain_release(instance->lock_chain, location, the_jumper);
        return;
      }

    just_grabbed = (instance->owner == NULL);

    old_count = instance->grab_count;
    instance->grab_count = new_count;
    instance->owner = my_thread;

    RELEASE_SYSTEM_LOCK(instance->hold_lock);

    if (just_grabbed && (instance->is_context_switching))
      {
        start_single_threaded(the_jumper, location);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            GRAB_SYSTEM_LOCK(instance->hold_lock);

            instance->grab_count = old_count;
            oi_remove_reference(new_count);

            assert(instance->owner == my_thread);
            instance->owner = NULL;

            RELEASE_SYSTEM_LOCK(instance->hold_lock);

            if (instance->lock_chain != NULL)
                lock_chain_release(instance->lock_chain, location, the_jumper);

            return;
          }
      }

    oi_remove_reference(old_count);

    trace(jumper_tracer(the_jumper), TC_LOCKS, "%K grabbed [%I].", instance,
          instance->grab_count);
  }

extern void lock_instance_release(lock_instance *instance,
        const source_location *location, jumper *the_jumper)
  {
    o_integer new_count;
    boolean fully_released;

    assert(instance != NULL);
    assert(the_jumper != NULL);

    assert(instance->instance != NULL);
    if (!(instance_is_instantiated(instance->instance)))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(lock_release_uninstantiated),
                "An attempt was made to release a lock before the lock had "
                "been instantiated by executing its declaration.");
        return;
      }

    if (instance->scope_exited)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(lock_release_deallocated),
                "An attempt was made to release a lock after it had been "
                "de-allocated.");
        return;
      }

    GRAB_SYSTEM_LOCK(instance->hold_lock);

    assert(!(oi_out_of_memory(instance->grab_count)));
    assert(oi_kind(instance->grab_count) == IIK_FINITE);
    assert(!(oi_is_negative(instance->grab_count)));

    oi_subtract(new_count, instance->grab_count, oi_one);
    if (oi_out_of_memory(new_count))
      {
        RELEASE_SYSTEM_LOCK(instance->hold_lock);
        jumper_do_abort(the_jumper);
        return;
      }

    assert(oi_kind(new_count) == IIK_FINITE);
    if (oi_is_negative(new_count) ||
        (instance->owner != jumper_thread(the_jumper)))
      {
        RELEASE_SYSTEM_LOCK(instance->hold_lock);
        location_exception(the_jumper, location,
                EXCEPTION_TAG(lock_release_not_held),
                "An attempt was made to release a lock that was not currently "
                "held.");
        oi_remove_reference(new_count);
        return;
      }

    fully_released = oi_equal(new_count, oi_zero);

    if (fully_released)
      {
        instance->owner = NULL;

        if (instance->next_to_awaken != NULL)
          {
            unblock(instance->next_to_awaken);
            instance->next_to_awaken = NULL;
          }
      }

    oi_remove_reference(instance->grab_count);
    instance->grab_count = new_count;

    trace(jumper_tracer(the_jumper), TC_LOCKS, "%K released [%I].", instance,
          instance->grab_count);

    RELEASE_SYSTEM_LOCK(instance->hold_lock);

    if (instance->lock_chain != NULL)
        lock_chain_release(instance->lock_chain, location, the_jumper);

    if (fully_released && (instance->is_context_switching))
        finish_single_threaded(the_jumper);
  }

extern void lock_instance_add_reference(lock_instance *instance)
  {
    lock_instance_add_reference_with_cluster(instance, NULL);
  }

extern void lock_instance_remove_reference(lock_instance *instance,
                                           jumper *the_jumper)
  {
    lock_instance_remove_reference_with_cluster(instance, the_jumper, NULL);
  }

extern void lock_instance_add_reference_with_cluster(lock_instance *instance,
        reference_cluster *cluster)
  {
    assert(instance != NULL);

    GRAB_SYSTEM_LOCK(instance->reference_lock);
    assert(instance->reference_count > 0);
    ++(instance->reference_count);
    RELEASE_SYSTEM_LOCK(instance->reference_lock);

    if ((instance->reference_cluster != NULL) &&
        (instance->reference_cluster != cluster))
      {
        reference_cluster_add_reference(instance->reference_cluster);
      }
  }

extern void lock_instance_remove_reference_with_cluster(
        lock_instance *instance, jumper *the_jumper,
        reference_cluster *cluster)
  {
    size_t new_reference_count;

    assert(instance != NULL);

    GRAB_SYSTEM_LOCK(instance->reference_lock);
    assert(instance->reference_count > 0);
    --(instance->reference_count);
    new_reference_count = instance->reference_count;
    RELEASE_SYSTEM_LOCK(instance->reference_lock);

    if ((instance->reference_cluster != NULL) &&
        (instance->reference_cluster != cluster))
      {
        reference_cluster_remove_reference(instance->reference_cluster,
                                           the_jumper);
      }

    if (new_reference_count > 0)
        return;

    oi_remove_reference(instance->grab_count);

    if (instance->lock_chain != NULL)
      {
        lock_chain_remove_reference_with_cluster(instance->lock_chain,
                instance->reference_cluster, the_jumper);
      }

    delete_instance(instance->instance);

    lock_declaration_remove_reference(instance->declaration);

    DESTROY_SYSTEM_LOCK(instance->hold_lock);
    DESTROY_SYSTEM_LOCK(instance->reference_lock);
    free(instance);
  }

extern reference_cluster *lock_instance_reference_cluster(
        lock_instance *instance)
  {
    assert(instance != NULL);

    return instance->reference_cluster;
  }

extern boolean lock_instances_are_equal(lock_instance *lock_instance1,
                                        lock_instance *lock_instance2)
  {
    assert(lock_instance1 != NULL);
    assert(lock_instance2 != NULL);

    assert(lock_instance_is_instantiated(lock_instance1)); /* VERIFIED */
    assert(lock_instance_is_instantiated(lock_instance2)); /* VERIFIED */
    assert(!(lock_instance1->scope_exited)); /* VERIFIED */
    assert(!(lock_instance2->scope_exited)); /* VERIFIED */

    return (lock_instance1 == lock_instance2);
  }

extern int lock_instance_structural_order(lock_instance *left,
                                          lock_instance *right)
  {
    assert(left != NULL);
    assert(right != NULL);

    assert(lock_instance_is_instantiated(left)); /* VERIFIED */
    assert(lock_instance_is_instantiated(right)); /* VERIFIED */
    assert(!(left->scope_exited)); /* VERIFIED */
    assert(!(right->scope_exited)); /* VERIFIED */

    if (left == right)
        return 0;
    else if (left < right)
        return -1;
    else
        return 1;
  }
