/* file "reference_cluster.c" */

/*
 *  This file contains the implementation of the reference_cluster module.
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
#include "reference_cluster.h"
#include "object.h"
#include "context.h"


typedef enum
  {
    RCK_OBJECT,
    RCK_CONTEXT
  } reference_cluster_kind;

struct reference_cluster
  {
    reference_cluster_kind kind;
    union
      {
        object *object;
        context *context;
      } u;
  };


extern reference_cluster *create_object_reference_cluster(object *the_object)
  {
    reference_cluster *result;

    result = MALLOC_ONE_OBJECT(reference_cluster);
    if (result == NULL)
        return NULL;

    result->kind = RCK_OBJECT;
    result->u.object = the_object;

    return result;
  }

extern reference_cluster *create_context_reference_cluster(
        context *the_context)
  {
    reference_cluster *result;

    result = MALLOC_ONE_OBJECT(reference_cluster);
    if (result == NULL)
        return NULL;

    result->kind = RCK_CONTEXT;
    result->u.context = the_context;

    return result;
  }

extern void delete_reference_cluster(reference_cluster *cluster)
  {
    assert(cluster != NULL);

    free(cluster);
  }

extern void reference_cluster_add_reference(reference_cluster *cluster)
  {
    assert(cluster != NULL);

    switch (cluster->kind)
      {
        case RCK_OBJECT:
            object_add_reference(cluster->u.object);
            break;
        case RCK_CONTEXT:
            context_add_reference(cluster->u.context);
            break;
        default:
            assert(FALSE);
      }
  }

extern void reference_cluster_remove_reference(reference_cluster *cluster,
                                               jumper *the_jumper)
  {
    assert(cluster != NULL);

    assert(cluster != NULL);

    switch (cluster->kind)
      {
        case RCK_OBJECT:
            object_remove_reference(cluster->u.object, the_jumper);
            break;
        case RCK_CONTEXT:
            context_remove_reference(cluster->u.context, the_jumper);
            break;
        default:
            assert(FALSE);
      }
  }

extern void reference_cluster_add_internal_reference(
        reference_cluster *cluster)
  {
    assert(cluster != NULL);

    switch (cluster->kind)
      {
        case RCK_OBJECT:
            object_add_internal_reference(cluster->u.object);
            break;
        case RCK_CONTEXT:
            context_add_internal_reference(cluster->u.context);
            break;
        default:
            assert(FALSE);
      }
  }

extern void reference_cluster_remove_internal_reference(
        reference_cluster *cluster)
  {
    assert(cluster != NULL);

    assert(cluster != NULL);

    switch (cluster->kind)
      {
        case RCK_OBJECT:
            object_remove_internal_reference(cluster->u.object);
            break;
        case RCK_CONTEXT:
            context_remove_internal_reference(cluster->u.context);
            break;
        default:
            assert(FALSE);
      }
  }
