/* file "use_instance.c" */

/*
 *  This file contains the implementation of the use_instance module.
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
#include "use_instance.h"
#include "instance.h"
#include "routine_instance_chain.h"


typedef struct
  {
    instance *instance;
    routine_instance_chain *chain;
  } used_for_item;

struct use_instance
  {
    boolean is_instantiated;
    reference_cluster *cluster;
    size_t used_for_count;
    used_for_item *used_for_items;
  };


extern use_instance *create_use_instance(size_t used_for_count,
                                         reference_cluster *cluster)
  {
    use_instance *result;

    result = MALLOC_ONE_OBJECT(use_instance);
    if (result == NULL)
        return NULL;

    result->is_instantiated = FALSE;
    result->cluster = cluster;
    result->used_for_count = used_for_count;

    if (used_for_count == 0)
      {
        result->used_for_items = NULL;
      }
    else
      {
        size_t used_for_num;

        result->used_for_items = MALLOC_ARRAY(used_for_item, used_for_count);
        if (result->used_for_items == NULL)
          {
            free(result);
            return NULL;
          }

        for (used_for_num = 0; used_for_num < used_for_count; ++used_for_num)
          {
            used_for_item *this_item;

            this_item = &(result->used_for_items[used_for_num]);
            this_item->instance = NULL;
            this_item->chain = NULL;
          }
      }

    return result;
  }

extern void delete_use_instance(use_instance *the_use_instance,
                                jumper *the_jumper)
  {
    size_t used_for_count;
    used_for_item *used_for_items;

    assert(the_use_instance != NULL);

    used_for_count = the_use_instance->used_for_count;
    used_for_items = the_use_instance->used_for_items;

    if (used_for_items != NULL)
      {
        size_t used_for_num;

        assert(used_for_count > 0);

        for (used_for_num = 0; used_for_num < used_for_count; ++used_for_num)
          {
            used_for_item *this_item;

            this_item = &(the_use_instance->used_for_items[used_for_num]);

            if (this_item->instance != NULL)
              {
                instance_remove_reference_with_cluster(this_item->instance,
                        the_use_instance->cluster, the_jumper);
              }
            if (this_item->chain != NULL)
              {
                routine_instance_chain_remove_reference_with_cluster(
                        this_item->chain, the_use_instance->cluster,
                        the_jumper);
              }
          }

        free(used_for_items);
      }
    else
      {
        assert(used_for_count == 0);
      }

    free(the_use_instance);
  }

extern boolean use_instance_is_instantiated(use_instance *the_use_instance)
  {
    assert(the_use_instance != NULL);

    return the_use_instance->is_instantiated;
  }

extern instance *use_instance_instance(use_instance *the_use_instance,
                                       size_t used_for_number)
  {
    assert(the_use_instance != NULL);
    assert(used_for_number < the_use_instance->used_for_count);
    assert(the_use_instance->used_for_items != NULL);

    return the_use_instance->used_for_items[used_for_number].instance;
  }

extern routine_instance_chain *use_instance_chain(
        use_instance *the_use_instance, size_t used_for_number)
  {
    assert(the_use_instance != NULL);
    assert(used_for_number < the_use_instance->used_for_count);
    assert(the_use_instance->used_for_items != NULL);

    return the_use_instance->used_for_items[used_for_number].chain;
  }

extern void use_instance_set_instantiated(use_instance *the_use_instance)
  {
    assert(the_use_instance != NULL);

    the_use_instance->is_instantiated = TRUE;
  }

extern void use_instance_set_instance(use_instance *the_use_instance,
        size_t used_for_number, instance *the_instance)
  {
    assert(the_use_instance != NULL);
    assert(used_for_number < the_use_instance->used_for_count);
    assert(the_use_instance->used_for_items != NULL);
    assert(the_instance != NULL);

    assert(the_use_instance->used_for_items[used_for_number].instance == NULL);
    assert(the_use_instance->used_for_items[used_for_number].chain == NULL);
    instance_add_reference_with_cluster(the_instance,
                                        the_use_instance->cluster);
    the_use_instance->used_for_items[used_for_number].instance = the_instance;
  }

extern void use_instance_set_chain(use_instance *the_use_instance,
        size_t used_for_number, routine_instance_chain *chain)
  {
    assert(the_use_instance != NULL);
    assert(used_for_number < the_use_instance->used_for_count);
    assert(the_use_instance->used_for_items != NULL);
    assert(chain != NULL);

    assert(the_use_instance->used_for_items[used_for_number].instance == NULL);
    assert(the_use_instance->used_for_items[used_for_number].chain == NULL);
    routine_instance_chain_add_reference_with_cluster(chain,
            the_use_instance->cluster);
    the_use_instance->used_for_items[used_for_number].chain = chain;
  }
