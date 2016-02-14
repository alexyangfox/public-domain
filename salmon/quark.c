/* file "quark.c" */

/*
 *  This file contains the implementation of the quark module.
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
#include "quark.h"
#include "quark_declaration.h"
#include "instance.h"
#include "reference_cluster.h"
#include "jumper.h"
#include "purity_level.h"
#include "platform_dependent.h"


struct quark
  {
    quark_declaration *declaration;
    reference_cluster *reference_cluster;
    instance *instance;
    boolean scope_exited;
    o_integer order_index;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
  };


DECLARE_SYSTEM_LOCK(next_order_lock);
static o_integer next_order_index;


extern quark *create_quark(quark_declaration *declaration, purity_level *level,
                           reference_cluster *cluster)
  {
    quark *result;
    o_integer next_oi;

    assert(declaration != NULL);
    assert(level != NULL);

    result = MALLOC_ONE_OBJECT(quark);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock, free(result); return NULL);

    result->declaration = declaration;
    result->reference_cluster = cluster;
    result->instance = create_instance_for_quark(result, level);
    if (result->instance == NULL)
      {
        DESTROY_SYSTEM_LOCK(result->reference_lock);
        free(result);
        return NULL;
      }

    result->scope_exited = FALSE;

    GRAB_SYSTEM_LOCK(next_order_lock);
    assert(!(oi_out_of_memory(next_order_index)));

    result->order_index = next_order_index;

    oi_add(next_oi, next_order_index, oi_one);
    if (oi_out_of_memory(next_oi))
      {
        RELEASE_SYSTEM_LOCK(next_order_lock);
        delete_instance(result->instance);
        DESTROY_SYSTEM_LOCK(result->reference_lock);
        free(result);
        return NULL;
      }

    next_order_index = next_oi;

    RELEASE_SYSTEM_LOCK(next_order_lock);

    result->reference_count = 1;

    quark_declaration_add_reference(declaration);

    return result;
  }

extern quark_declaration *quark_instance_declaration(quark *the_quark)
  {
    assert(the_quark != NULL);

    return the_quark->declaration;
  }

extern boolean quark_is_instantiated(quark *the_quark)
  {
    assert(the_quark != NULL);

    assert(the_quark->instance != NULL);
    return instance_is_instantiated(the_quark->instance);
  }

extern boolean quark_scope_exited(quark *the_quark)
  {
    assert(the_quark != NULL);

    return the_quark->scope_exited;
  }

extern instance *quark_instance_instance(quark *the_quark)
  {
    assert(the_quark != NULL);

    return the_quark->instance;
  }

extern verdict set_quark_scope_exited(quark *the_quark)
  {
    assert(the_quark != NULL);

    assert(!(the_quark->scope_exited)); /* VERIFIED */

    mark_instance_scope_exited(the_quark->instance);
    the_quark->scope_exited = TRUE;
    return MISSION_ACCOMPLISHED;
  }

extern void quark_add_reference(quark *the_quark)
  {
    quark_add_reference_with_cluster(the_quark, NULL);
  }

extern void quark_remove_reference(quark *the_quark, jumper *the_jumper)
  {
    quark_remove_reference_with_cluster(the_quark, the_jumper, NULL);
  }

extern void quark_add_reference_with_cluster(quark *the_quark,
                                             reference_cluster *cluster)
  {
    assert(the_quark != NULL);

    GRAB_SYSTEM_LOCK(the_quark->reference_lock);
    assert(the_quark->reference_count > 0);
    ++(the_quark->reference_count);
    RELEASE_SYSTEM_LOCK(the_quark->reference_lock);

    if ((the_quark->reference_cluster != NULL) &&
        (the_quark->reference_cluster != cluster))
      {
        reference_cluster_add_reference(the_quark->reference_cluster);
      }
  }

extern void quark_remove_reference_with_cluster(quark *the_quark,
        jumper *the_jumper, reference_cluster *cluster)
  {
    size_t new_reference_count;

    assert(the_quark != NULL);

    GRAB_SYSTEM_LOCK(the_quark->reference_lock);
    assert(the_quark->reference_count > 0);
    --(the_quark->reference_count);
    new_reference_count = the_quark->reference_count;
    RELEASE_SYSTEM_LOCK(the_quark->reference_lock);

    if ((the_quark->reference_cluster != NULL) &&
        (the_quark->reference_cluster != cluster))
      {
        reference_cluster_remove_reference(the_quark->reference_cluster,
                                           the_jumper);
      }

    if (new_reference_count > 0)
        return;

    delete_instance(the_quark->instance);

    quark_declaration_remove_reference(the_quark->declaration);

    oi_remove_reference(the_quark->order_index);
    DESTROY_SYSTEM_LOCK(the_quark->reference_lock);

    free(the_quark);
  }

extern reference_cluster *quark_reference_cluster(quark *the_quark)
  {
    assert(the_quark != NULL);

    return the_quark->reference_cluster;
  }

extern boolean quarks_are_equal(quark *quark1, quark *quark2)
  {
    assert(quark1 != NULL);
    assert(quark2 != NULL);

    assert(quark_is_instantiated(quark1)); /* VERIFIED */
    assert(quark_is_instantiated(quark2)); /* VERIFIED */
    assert(!(quark1->scope_exited)); /* VERIFIED */
    assert(!(quark2->scope_exited)); /* VERIFIED */

    return (quark1 == quark2);
  }

extern int quark_structural_order(quark *left, quark *right)
  {
    assert(left != NULL);
    assert(right != NULL);

    assert(quark_is_instantiated(left)); /* VERIFIED */
    assert(quark_is_instantiated(right)); /* VERIFIED */
    assert(!(left->scope_exited)); /* VERIFIED */
    assert(!(right->scope_exited)); /* VERIFIED */

    if (left == right)
        return 0;
    else if (oi_less_than(left->order_index, right->order_index))
        return -1;
    else
        return 1;
  }

extern verdict init_quark_module(void)
  {
    INITIALIZE_SYSTEM_LOCK(next_order_lock, return MISSION_FAILED);
    next_order_index = oi_one;
    assert(!(oi_out_of_memory(next_order_index)));
    oi_add_reference(oi_one);
    return MISSION_ACCOMPLISHED;
  }

extern void cleanup_quark_module(void)
  {
    assert(!(oi_out_of_memory(next_order_index)));
    oi_remove_reference(next_order_index);
    DESTROY_SYSTEM_LOCK(next_order_lock);
  }
