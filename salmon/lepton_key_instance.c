/* file "lepton_key_instance.c" */

/*
 *  This file contains the implementation of the lepton_key_instance module.
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
#include "lepton_key_instance.h"
#include "lepton_key_declaration.h"
#include "type.h"
#include "instance.h"
#include "reference_cluster.h"
#include "purity_level.h"
#include "platform_dependent.h"


struct lepton_key_instance
  {
    lepton_key_declaration *declaration;
    reference_cluster *reference_cluster;
    instance *instance;
    boolean scope_exited;
    type **field_types;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
  };


extern lepton_key_instance *create_lepton_key_instance(
        lepton_key_declaration *declaration, purity_level *level,
        reference_cluster *cluster)
  {
    lepton_key_instance *result;
    size_t field_count;

    assert(declaration != NULL);
    assert(level != NULL);

    result = MALLOC_ONE_OBJECT(lepton_key_instance);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock, free(result); return NULL);

    result->declaration = declaration;
    result->reference_cluster = cluster;
    result->instance = create_instance_for_lepton_key(result, level);
    if (result->instance == NULL)
      {
        DESTROY_SYSTEM_LOCK(result->reference_lock);
        free(result);
        return NULL;
      }

    result->scope_exited = FALSE;
    result->reference_count = 1;

    field_count = lepton_key_field_count(declaration);
    if (field_count == 0)
      {
        result->field_types = NULL;
      }
    else
      {
        size_t field_num;

        result->field_types = MALLOC_ARRAY(type *, field_count);
        if (result->field_types == NULL)
          {
            DESTROY_SYSTEM_LOCK(result->reference_lock);
            free(result);
            return NULL;
          }

        for (field_num = 0; field_num < field_count; ++field_num)
            result->field_types[field_num] = NULL;
      }

    lepton_key_declaration_add_reference(declaration);

    return result;
  }

extern lepton_key_declaration *lepton_key_instance_declaration(
        lepton_key_instance *instance)
  {
    assert(instance != NULL);

    return instance->declaration;
  }

extern boolean lepton_key_instance_is_instantiated(
        lepton_key_instance *instance)
  {
    assert(instance != NULL);

    assert(instance->instance != NULL);
    return instance_is_instantiated(instance->instance);
  }

extern boolean lepton_key_instance_scope_exited(lepton_key_instance *instance)
  {
    assert(instance != NULL);

    return instance->scope_exited;
  }

extern type *lepton_key_instance_field_type(lepton_key_instance *instance,
                                            size_t type_num)
  {
    assert(instance != NULL);

    assert(lepton_key_instance_is_instantiated(instance)); /* VERIFIED */
    assert(!(instance->scope_exited)); /* VERIFIED */

    assert(type_num < lepton_key_field_count(instance->declaration));
    assert(instance->field_types != NULL);
    return instance->field_types[type_num];
  }

extern instance *lepton_key_instance_instance(lepton_key_instance *instance)
  {
    assert(instance != NULL);

    return instance->instance;
  }

extern void set_lepton_key_instance_field_type(lepton_key_instance *instance,
        type *field_type, size_t type_num, jumper *the_jumper)
  {
    assert(instance != NULL);

    assert(!(instance->scope_exited)); /* VERIFIED */
    assert(type_num < lepton_key_field_count(instance->declaration));
    assert(instance->field_types != NULL);

    if (field_type != NULL)
      {
        type_add_reference_with_reference_cluster(field_type,
                                                  instance->reference_cluster);
      }
    if (instance->field_types[type_num] != NULL)
      {
        type_remove_reference_with_reference_cluster(
                instance->field_types[type_num], the_jumper,
                instance->reference_cluster);
      }
    instance->field_types[type_num] = field_type;
  }

extern void set_lepton_key_instance_scope_exited(lepton_key_instance *instance,
                                                 jumper *the_jumper)
  {
    size_t field_count;
    size_t field_num;

    assert(instance != NULL);

    assert(!(instance->scope_exited)); /* VERIFIED */

    mark_instance_scope_exited(instance->instance);
    instance->scope_exited = TRUE;

    field_count = lepton_key_field_count(instance->declaration);

    for (field_num = 0; field_num < field_count; ++field_num)
      {
        if (instance->field_types[field_num] != NULL)
          {
            type_remove_reference_with_reference_cluster(
                    instance->field_types[field_num], the_jumper,
                    instance->reference_cluster);
          }
        instance->field_types[field_num] = NULL;
      }
  }

extern void lepton_key_instance_add_reference(lepton_key_instance *instance)
  {
    lepton_key_instance_add_reference_with_cluster(instance, NULL);
  }

extern void lepton_key_instance_remove_reference(lepton_key_instance *instance,
                                                 jumper *the_jumper)
  {
    lepton_key_instance_remove_reference_with_cluster(instance, the_jumper,
                                                      NULL);
  }

extern void lepton_key_instance_add_reference_with_cluster(
        lepton_key_instance *instance, reference_cluster *cluster)
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

extern void lepton_key_instance_remove_reference_with_cluster(
        lepton_key_instance *instance, jumper *the_jumper,
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

    if (!(instance->scope_exited))
        set_lepton_key_instance_scope_exited(instance, the_jumper);

    if (instance->field_types != NULL)
        free(instance->field_types);

    delete_instance(instance->instance);

    lepton_key_declaration_remove_reference(instance->declaration);

    DESTROY_SYSTEM_LOCK(instance->reference_lock);

    free(instance);
  }

extern reference_cluster *lepton_key_instance_reference_cluster(
        lepton_key_instance *instance)
  {
    assert(instance != NULL);

    return instance->reference_cluster;
  }

extern boolean lepton_key_instances_are_equal(lepton_key_instance *instance1,
                                              lepton_key_instance *instance2)
  {
    assert(instance1 != NULL);
    assert(instance2 != NULL);

    assert(lepton_key_instance_is_instantiated(instance1)); /* VERIFIED */
    assert(lepton_key_instance_is_instantiated(instance2)); /* VERIFIED */
    assert(!(instance1->scope_exited)); /* VERIFIED */
    assert(!(instance2->scope_exited)); /* VERIFIED */

    return (instance1 == instance2);
  }

extern int lepton_key_instance_structural_order(lepton_key_instance *left,
                                                lepton_key_instance *right)
  {
    assert(left != NULL);
    assert(right != NULL);

    assert(lepton_key_instance_is_instantiated(left)); /* VERIFIED */
    assert(lepton_key_instance_is_instantiated(right)); /* VERIFIED */
    assert(!(left->scope_exited)); /* VERIFIED */
    assert(!(right->scope_exited)); /* VERIFIED */

    if (left == right)
        return 0;
    else if (left < right)
        return -1;
    else
        return 1;
  }
