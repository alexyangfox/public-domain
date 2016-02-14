/* file "purity_level.c" */

/*
 *  This file contains the implementation of the purity_level module.
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
#include "c_foundations/memory_allocation.h"
#include "purity_level.h"
#include "execute.h"
#include "platform_dependent.h"


struct purity_level
  {
    purity_level *parent;
    size_t original_depth;
    DECLARE_SYSTEM_LOCK(moved_lock);
    purity_level *moved_level;
    purity_level *first_level;
    DECLARE_SYSTEM_LOCK(sticky_lock_lock);
    lock_instance_aa sticky_locks;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
  };


extern purity_level *create_purity_level(purity_level *parent)
  {
    purity_level *result;

    result = MALLOC_ONE_OBJECT(purity_level);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->moved_lock, free(result); return NULL);

    INITIALIZE_SYSTEM_LOCK(result->sticky_lock_lock,
            DESTROY_SYSTEM_LOCK(result->moved_lock);
            free(result);
            return NULL);

    INITIALIZE_SYSTEM_LOCK(result->reference_lock,
            DESTROY_SYSTEM_LOCK(result->sticky_lock_lock);
            DESTROY_SYSTEM_LOCK(result->moved_lock);
            free(result);
            return NULL);

    result->parent = parent;
    if (parent == NULL)
      {
        verdict the_verdict;

        the_verdict = lock_instance_aa_init(&(result->sticky_locks), 10);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            DESTROY_SYSTEM_LOCK(result->reference_lock);
            DESTROY_SYSTEM_LOCK(result->sticky_lock_lock);
            DESTROY_SYSTEM_LOCK(result->moved_lock);
            free(result);
            return NULL;
          }
        result->original_depth = 0;
        result->first_level = result;
      }
    else
      {
        assert(parent->original_depth < ~(size_t)0);
        result->original_depth = parent->original_depth + 1;
        purity_level_add_reference(parent);
        result->first_level = parent->first_level;
      }
    result->moved_level = NULL;
    result->reference_count = 1;

    return result;
  }

extern size_t purity_level_depth(purity_level *level)
  {
    purity_level *local_moved_level;

    assert(level != NULL);
    assert(level->reference_count > 0);

    GRAB_SYSTEM_LOCK(level->moved_lock);
    local_moved_level = level->moved_level;
    RELEASE_SYSTEM_LOCK(level->moved_lock);

    if (local_moved_level == NULL)
        return level->original_depth;

    return moved_level(level)->original_depth;
  }

extern purity_level *moved_level(purity_level *level)
  {
    purity_level *result;

    assert(level != NULL);
    assert(level->reference_count > 0);

    GRAB_SYSTEM_LOCK(level->moved_lock);

    if (level->moved_level == NULL)
      {
        RELEASE_SYSTEM_LOCK(level->moved_lock);
        return level;
      }

    result = moved_level(level->moved_level);
    level->moved_level = result;

    RELEASE_SYSTEM_LOCK(level->moved_lock);

    return result;
  }

extern purity_level *purity_level_first_level(purity_level *level)
  {
    assert(level != NULL);
    assert(level->reference_count > 0);

    return level->first_level;
  }

extern size_t purity_level_sticky_lock_instance_count(purity_level *level)
  {
    size_t result;

    assert(level != NULL);
    assert(level->reference_count > 0);

    assert(level->parent == NULL);
    GRAB_SYSTEM_LOCK(level->sticky_lock_lock);
    result = level->sticky_locks.element_count;
    RELEASE_SYSTEM_LOCK(level->sticky_lock_lock);
    return result;
  }

extern lock_instance *purity_level_sticky_lock_instance(purity_level *level,
                                                        size_t instance_num)
  {
    lock_instance *result;

    assert(level != NULL);
    assert(level->reference_count > 0);

    assert(level->parent == NULL);

    GRAB_SYSTEM_LOCK(level->sticky_lock_lock);

    assert(instance_num < level->sticky_locks.element_count);
    result = level->sticky_locks.array[instance_num];

    RELEASE_SYSTEM_LOCK(level->sticky_lock_lock);

    return result;
  }

extern void purity_level_move_out(purity_level *level)
  {
    GRAB_SYSTEM_LOCK(level->moved_lock);

    assert(level != NULL);
    assert(level->reference_count > 0);
    assert(level->moved_level == NULL);
    assert(level->parent != NULL);

    level->moved_level = level->parent;

    RELEASE_SYSTEM_LOCK(level->moved_lock);
  }

extern verdict purity_level_add_sticky_lock_instance(purity_level *level,
                                                     lock_instance *instance)
  {
    verdict result;

    assert(level != NULL);
    assert(level->reference_count > 0);

    assert(level->parent == NULL);
    GRAB_SYSTEM_LOCK(level->sticky_lock_lock);
    result = lock_instance_aa_append(&(level->sticky_locks), instance);
    RELEASE_SYSTEM_LOCK(level->sticky_lock_lock);
    return result;
  }

extern void purity_level_add_reference(purity_level *level)
  {
    assert(level != NULL);

    GRAB_SYSTEM_LOCK(level->reference_lock);
    assert(level->reference_count > 0);
    ++(level->reference_count);
    RELEASE_SYSTEM_LOCK(level->reference_lock);
  }

extern void purity_level_remove_reference(purity_level *level)
  {
    size_t new_reference_count;

    assert(level != NULL);

    GRAB_SYSTEM_LOCK(level->reference_lock);
    assert(level->reference_count > 0);
    --(level->reference_count);
    new_reference_count = level->reference_count;
    RELEASE_SYSTEM_LOCK(level->reference_lock);

    if (new_reference_count > 0)
        return;

    if (level->parent != NULL)
        purity_level_remove_reference(level->parent);
    else
        free(level->sticky_locks.array);

    DESTROY_SYSTEM_LOCK(level->reference_lock);
    DESTROY_SYSTEM_LOCK(level->sticky_lock_lock);
    DESTROY_SYSTEM_LOCK(level->moved_lock);

    free(level);
  }
