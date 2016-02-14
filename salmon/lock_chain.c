/* file "lock_chain.c" */

/*
 *  This file contains the implementation of the lock_chain module.
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
#include "c_foundations/memory_allocation.h"
#include "lock_chain.h"
#include "lock_instance.h"
#include "source_location.h"
#include "jumper.h"
#include "platform_dependent.h"
#include "reference_cluster.h"


struct lock_chain
  {
    lock_instance *head;
    lock_chain *remainder;
    reference_cluster *reference_cluster;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
  };


extern lock_chain *create_lock_chain(lock_instance *head,
                                     lock_chain *remainder)
  {
    lock_chain *result;

    assert(head != NULL);

    result = MALLOC_ONE_OBJECT(lock_chain);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock, free(result); return NULL);

    result->reference_cluster = lock_instance_reference_cluster(head);
    if ((result->reference_cluster == NULL) && (remainder != NULL))
        result->reference_cluster = remainder->reference_cluster;

    lock_instance_add_reference_with_cluster(head, result->reference_cluster);
    if (remainder != NULL)
      {
        lock_chain_add_reference_with_cluster(remainder,
                                              result->reference_cluster);
      }
    result->head = head;
    result->remainder = remainder;

    result->reference_count = 1;

    if (result->reference_cluster != NULL)
        reference_cluster_add_reference(result->reference_cluster);

    return result;
  }

extern lock_instance *lock_chain_head(lock_chain *chain)
  {
    assert(chain != NULL);

    return chain->head;
  }

extern lock_chain *lock_chain_remainder(lock_chain *chain)
  {
    assert(chain != NULL);

    return chain->remainder;
  }

extern void lock_chain_grab(lock_chain *chain, const source_location *location,
                            jumper *the_jumper)
  {
    assert(chain != NULL);

    if (chain->remainder != NULL)
        lock_chain_grab(chain->remainder, location, the_jumper);

    lock_instance_grab(chain->head, location, the_jumper);
  }

extern void lock_chain_release(lock_chain *chain,
        const source_location *location, jumper *the_jumper)
  {
    assert(chain != NULL);

    lock_instance_release(chain->head, location, the_jumper);

    if (chain->remainder != NULL)
        lock_chain_release(chain->remainder, location, the_jumper);
  }

extern void lock_chain_add_reference(lock_chain *chain)
  {
    assert(chain != NULL);

    GRAB_SYSTEM_LOCK(chain->reference_lock);
    assert(chain->reference_count > 0);
    ++(chain->reference_count);
    RELEASE_SYSTEM_LOCK(chain->reference_lock);

    if (chain->reference_cluster != NULL)
        reference_cluster_add_reference(chain->reference_cluster);
  }

extern void lock_chain_remove_reference(lock_chain *chain, jumper *the_jumper)
  {
    size_t new_reference_count;

    assert(chain != NULL);

    GRAB_SYSTEM_LOCK(chain->reference_lock);
    assert(chain->reference_count > 0);
    --(chain->reference_count);
    new_reference_count = chain->reference_count;
    RELEASE_SYSTEM_LOCK(chain->reference_lock);

    if (chain->reference_cluster != NULL)
      {
        reference_cluster_remove_reference(chain->reference_cluster,
                                           the_jumper);
      }

    if (new_reference_count > 0)
        return;

    lock_instance_remove_reference_with_cluster(chain->head, the_jumper,
                                                chain->reference_cluster);
    if (chain->remainder != NULL)
      {
        lock_chain_remove_reference_with_cluster(chain->remainder,
                chain->reference_cluster, the_jumper);
      }

    DESTROY_SYSTEM_LOCK(chain->reference_lock);

    free(chain);
  }

extern void lock_chain_add_reference_with_cluster(lock_chain *chain,
                                                  reference_cluster *cluster)
  {
    assert(chain != NULL);

    GRAB_SYSTEM_LOCK(chain->reference_lock);
    assert(chain->reference_count > 0);
    ++(chain->reference_count);
    RELEASE_SYSTEM_LOCK(chain->reference_lock);

    if ((chain->reference_cluster != NULL) &&
        (chain->reference_cluster != cluster))
      {
        reference_cluster_add_reference(chain->reference_cluster);
      }
  }

extern void lock_chain_remove_reference_with_cluster(lock_chain *chain,
        reference_cluster *cluster, jumper *the_jumper)
  {
    size_t new_reference_count;

    assert(chain != NULL);

    GRAB_SYSTEM_LOCK(chain->reference_lock);
    assert(chain->reference_count > 0);
    --(chain->reference_count);
    new_reference_count = chain->reference_count;
    RELEASE_SYSTEM_LOCK(chain->reference_lock);

    if ((chain->reference_cluster != NULL) &&
        (chain->reference_cluster != cluster))
      {
        reference_cluster_remove_reference(chain->reference_cluster,
                                           the_jumper);
      }

    if (new_reference_count > 0)
        return;

    lock_instance_remove_reference_with_cluster(chain->head, the_jumper,
                                                chain->reference_cluster);
    if (chain->remainder != NULL)
      {
        lock_chain_remove_reference_with_cluster(chain->remainder,
                chain->reference_cluster, the_jumper);
      }

    DESTROY_SYSTEM_LOCK(chain->reference_lock);

    free(chain);
  }
