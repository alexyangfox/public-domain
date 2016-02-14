/* file "tagalong_key.c" */

/*
 *  This file contains the implementation of the tagalong_key module.
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
#include "tagalong_key.h"
#include "tagalong_declaration.h"
#include "value.h"
#include "type.h"
#include "lock_chain.h"
#include "instance.h"
#include "object.h"
#include "purity_level.h"
#include "reference_cluster.h"
#include "platform_dependent.h"


struct tagalong_key
  {
    tagalong_declaration *declaration;
    reference_cluster *reference_cluster;
    instance *instance;
    boolean scope_exited;
    type *type;
    type *on_type;
    value *default_value;
    lock_chain *lock_chain;
    DECLARE_SYSTEM_LOCK(value_lock);
    value_tagalong_handle *value_handle;
    DECLARE_SYSTEM_LOCK(object_lock);
    object_tagalong_handle *object_handle;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
  };


extern tagalong_key *create_tagalong_key(tagalong_declaration *declaration,
        purity_level *level, reference_cluster *cluster)
  {
    tagalong_key *result;

    assert(declaration != NULL);
    assert(level != NULL);

    result = MALLOC_ONE_OBJECT(tagalong_key);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->value_lock, free(result); return NULL);

    INITIALIZE_SYSTEM_LOCK(result->object_lock,
            DESTROY_SYSTEM_LOCK(result->value_lock);
            free(result);
            return NULL);

    INITIALIZE_SYSTEM_LOCK(result->reference_lock,
            DESTROY_SYSTEM_LOCK(result->object_lock);
            DESTROY_SYSTEM_LOCK(result->value_lock);
            free(result);
            return NULL);

    result->declaration = declaration;
    result->reference_cluster = cluster;
    result->instance = create_instance_for_tagalong(result, level);
    if (result->instance == NULL)
      {
        DESTROY_SYSTEM_LOCK(result->reference_lock);
        DESTROY_SYSTEM_LOCK(result->object_lock);
        DESTROY_SYSTEM_LOCK(result->value_lock);
        free(result);
        return NULL;
      }

    result->scope_exited = FALSE;
    result->type = NULL;
    result->on_type = NULL;
    result->default_value = NULL;
    result->lock_chain = NULL;
    result->value_handle = NULL;
    result->object_handle = NULL;
    result->reference_count = 1;

    tagalong_declaration_add_reference(declaration);

    return result;
  }

extern tagalong_declaration *tagalong_key_declaration(tagalong_key *key)
  {
    assert(key != NULL);

    return key->declaration;
  }

extern boolean tagalong_key_is_instantiated(tagalong_key *key)
  {
    assert(key != NULL);

    assert(key->instance != NULL);
    return instance_is_instantiated(key->instance);
  }

extern boolean tagalong_key_scope_exited(tagalong_key *key)
  {
    assert(key != NULL);

    return key->scope_exited;
  }

extern instance *tagalong_key_instance(tagalong_key *key)
  {
    assert(key != NULL);

    return key->instance;
  }

extern void grab_tagalong_key_value_tagalong_lock(tagalong_key *key)
  {
    assert(key != NULL);

    GRAB_SYSTEM_LOCK(key->value_lock);
  }

extern void release_tagalong_key_value_tagalong_lock(tagalong_key *key)
  {
    assert(key != NULL);

    RELEASE_SYSTEM_LOCK(key->value_lock);
  }

extern value_tagalong_handle *get_value_tagalong_handle(tagalong_key *key)
  {
    assert(key != NULL);

    return key->value_handle;
  }

extern void set_value_tagalong_handle(tagalong_key *key,
        value_tagalong_handle *handle, jumper *the_jumper)
  {
    value_tagalong_handle *old_handle;

    assert(key != NULL);

    old_handle = key->value_handle;
    key->value_handle = handle;

    release_tagalong_key_value_tagalong_lock(key);

    if ((handle != NULL) && (old_handle == NULL))
        tagalong_key_add_reference(key);
    else if ((handle == NULL) && (old_handle != NULL))
        tagalong_key_remove_reference(key, the_jumper);
  }

extern void grab_tagalong_key_object_tagalong_lock(tagalong_key *key)
  {
    assert(key != NULL);

    GRAB_SYSTEM_LOCK(key->object_lock);
  }

extern void release_tagalong_key_object_tagalong_lock(tagalong_key *key)
  {
    assert(key != NULL);

    RELEASE_SYSTEM_LOCK(key->object_lock);
  }

extern object_tagalong_handle *get_object_tagalong_handle(tagalong_key *key)
  {
    assert(key != NULL);

    return key->object_handle;
  }

extern void set_object_tagalong_handle(tagalong_key *key,
        object_tagalong_handle *handle, jumper *the_jumper)
  {
    object_tagalong_handle *old_handle;

    assert(key != NULL);

    old_handle = key->object_handle;
    key->object_handle = handle;

    release_tagalong_key_object_tagalong_lock(key);

    if ((handle != NULL) && (old_handle == NULL))
        tagalong_key_add_reference(key);
    else if ((handle == NULL) && (old_handle != NULL))
        tagalong_key_remove_reference(key, the_jumper);
  }

extern value *tagalong_key_default_value(tagalong_key *key)
  {
    assert(key != NULL);

    assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
    assert(!(key->scope_exited)); /* VERIFIED */

    return key->default_value;
  }

extern lock_chain *tagalong_key_lock_chain(tagalong_key *key)
  {
    assert(key != NULL);

    assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
    assert(!(key->scope_exited)); /* VERIFIED */

    return key->lock_chain;
  }

extern type *tagalong_key_type(tagalong_key *key)
  {
    assert(key != NULL);

    assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
    assert(!(key->scope_exited)); /* VERIFIED */

    return key->type;
  }

extern type *tagalong_key_on_type(tagalong_key *key)
  {
    assert(key != NULL);

    assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
    assert(!(key->scope_exited)); /* VERIFIED */

    return key->on_type;
  }

extern void set_tagalong_key_type(tagalong_key *key, type *the_type,
                                  jumper *the_jumper)
  {
    assert(key != NULL);

    assert(!(key->scope_exited)); /* VERIFIED */

    if (the_type != NULL)
      {
        type_add_reference_with_reference_cluster(the_type,
                                                  key->reference_cluster);
      }
    if (key->type != NULL)
      {
        type_remove_reference_with_reference_cluster(key->type, the_jumper,
                                                     key->reference_cluster);
      }
    key->type = the_type;
  }

extern void set_tagalong_key_on_type(tagalong_key *key, type *on_type,
                                     jumper *the_jumper)
  {
    assert(key != NULL);

    assert(!(key->scope_exited)); /* VERIFIED */

    if (on_type != NULL)
      {
        type_add_reference_with_reference_cluster(on_type,
                                                  key->reference_cluster);
      }
    if (key->on_type != NULL)
      {
        type_remove_reference_with_reference_cluster(key->on_type, the_jumper,
                                                     key->reference_cluster);
      }
    key->on_type = on_type;
  }

extern void set_tagalong_key_default_value(tagalong_key *key,
        value *default_value, jumper *the_jumper)
  {
    assert(key != NULL);

    assert(!(key->scope_exited)); /* VERIFIED */

    if (default_value != NULL)
      {
        value_add_reference_with_reference_cluster(default_value,
                                                   key->reference_cluster);
      }
    if (key->default_value != NULL)
      {
        value_remove_reference_with_reference_cluster(key->default_value,
                the_jumper, key->reference_cluster);
      }
    key->default_value = default_value;
  }

extern void set_tagalong_key_lock_chain(tagalong_key *key,
        lock_chain *the_lock_chain, jumper *the_jumper)
  {
    assert(key != NULL);

    assert(!(key->scope_exited)); /* VERIFIED */

    if (the_lock_chain != NULL)
      {
        lock_chain_add_reference_with_cluster(the_lock_chain,
                                              key->reference_cluster);
      }
    if (key->lock_chain != NULL)
      {
        lock_chain_remove_reference_with_cluster(key->lock_chain,
                key->reference_cluster, the_jumper);
      }
    key->lock_chain = the_lock_chain;
  }

extern void set_tagalong_key_scope_exited(tagalong_key *key,
                                          jumper *the_jumper)
  {
    assert(key != NULL);

    assert(!(key->scope_exited)); /* VERIFIED */

    mark_instance_scope_exited(key->instance);
    key->scope_exited = TRUE;

    if (key->type != NULL)
      {
        type_remove_reference_with_reference_cluster(key->type, the_jumper,
                                                     key->reference_cluster);
      }
    key->type = NULL;

    if (key->on_type != NULL)
      {
        type_remove_reference_with_reference_cluster(key->on_type, the_jumper,
                                                     key->reference_cluster);
      }
    key->on_type = NULL;

    if (key->default_value != NULL)
      {
        value_remove_reference_with_reference_cluster(key->default_value,
                the_jumper, key->reference_cluster);
      }
    key->default_value = NULL;

    if (key->lock_chain != NULL)
      {
        lock_chain_remove_reference_with_cluster(key->lock_chain,
                key->reference_cluster, the_jumper);
      }
    key->lock_chain = NULL;

    while (key->value_handle != NULL)
        kill_value_tagalong(key->value_handle, the_jumper);

    while (key->object_handle != NULL)
        kill_object_tagalong(key->object_handle, the_jumper);
  }

extern void tagalong_key_add_reference(tagalong_key *key)
  {
    tagalong_key_add_reference_with_cluster(key, NULL);
  }

extern void tagalong_key_remove_reference(tagalong_key *key,
                                          jumper *the_jumper)
  {
    tagalong_key_remove_reference_with_cluster(key, the_jumper, NULL);
  }

extern void tagalong_key_add_reference_with_cluster(tagalong_key *key,
                                                    reference_cluster *cluster)
  {
    assert(key != NULL);

    GRAB_SYSTEM_LOCK(key->reference_lock);
    assert(key->reference_count > 0);
    ++(key->reference_count);
    RELEASE_SYSTEM_LOCK(key->reference_lock);

    if ((key->reference_cluster != NULL) &&
        (key->reference_cluster != cluster))
      {
        reference_cluster_add_reference(key->reference_cluster);
      }
  }

extern void tagalong_key_remove_reference_with_cluster(tagalong_key *key,
        jumper *the_jumper, reference_cluster *cluster)
  {
    size_t new_reference_count;

    assert(key != NULL);

    GRAB_SYSTEM_LOCK(key->reference_lock);
    assert(key->reference_count > 0);
    --(key->reference_count);
    new_reference_count = key->reference_count;
    RELEASE_SYSTEM_LOCK(key->reference_lock);

    if ((key->reference_cluster != NULL) &&
        (key->reference_cluster != cluster))
      {
        reference_cluster_remove_reference(key->reference_cluster, the_jumper);
      }

    if (new_reference_count > 0)
        return;

    if (!(key->scope_exited))
        set_tagalong_key_scope_exited(key, the_jumper);

    delete_instance(key->instance);

    tagalong_declaration_remove_reference(key->declaration);

    DESTROY_SYSTEM_LOCK(key->reference_lock);
    DESTROY_SYSTEM_LOCK(key->object_lock);
    DESTROY_SYSTEM_LOCK(key->value_lock);

    free(key);
  }

extern reference_cluster *tagalong_key_reference_cluster(tagalong_key *key)
  {
    assert(key != NULL);

    return key->reference_cluster;
  }

extern boolean tagalong_keys_are_equal(tagalong_key *key1, tagalong_key *key2)
  {
    assert(key1 != NULL);
    assert(key2 != NULL);

    assert(tagalong_key_is_instantiated(key1)); /* VERIFIED */
    assert(tagalong_key_is_instantiated(key2)); /* VERIFIED */
    assert(!(key1->scope_exited)); /* VERIFIED */
    assert(!(key2->scope_exited)); /* VERIFIED */

    return (key1 == key2);
  }

extern int tagalong_key_structural_order(tagalong_key *left,
                                         tagalong_key *right)
  {
    assert(left != NULL);
    assert(right != NULL);

    assert(tagalong_key_is_instantiated(left)); /* VERIFIED */
    assert(tagalong_key_is_instantiated(right)); /* VERIFIED */
    assert(!(left->scope_exited)); /* VERIFIED */
    assert(!(right->scope_exited)); /* VERIFIED */

    if (left == right)
        return 0;
    else if (left < right)
        return -1;
    else
        return 1;
  }
