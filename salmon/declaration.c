/* file "declaration.c" */

/*
 *  This file contains the implementation of the declaration module.
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
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "declaration.h"
#include "source_location.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "tagalong_declaration.h"
#include "lepton_key_declaration.h"
#include "quark_declaration.h"
#include "lock_declaration.h"
#include "declaration_list.h"
#include "static_home.h"
#include "platform_dependent.h"


struct declaration
  {
    char *name;
    boolean is_static;
    boolean is_virtual;
    boolean automatic_allocation;
    name_kind kind;
    union
      {
        variable_declaration *variable;
        routine_declaration *routine;
        tagalong_declaration *tagalong;
        lepton_key_declaration *lepton_key;
        quark_declaration *quark;
        lock_declaration *lock;
      } u;
    void *parent_pointer;
    size_t parent_index;
    static_home *static_parent_pointer;
    size_t static_parent_index;
    source_location location;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
  };


static declaration *create_empty_declaration(const char *name,
        boolean is_static, boolean is_virtual, boolean automatic_allocation,
        const source_location *location);


extern declaration *create_declaration_for_variable(const char *name,
        boolean is_static, boolean is_virtual, boolean automatic_allocation,
        variable_declaration *the_variable_declaration,
        const source_location *location)
  {
    declaration *result;

    result = create_empty_declaration(name, is_static, is_virtual,
                                      automatic_allocation, location);
    if (result == NULL)
      {
        delete_variable_declaration(the_variable_declaration);
        return NULL;
      }

    result->kind = NK_VARIABLE;
    result->u.variable = the_variable_declaration;

    set_variable_declaration_declaration(the_variable_declaration, result);

    return result;
  }

extern declaration *create_declaration_for_routine(const char *name,
        boolean is_static, boolean is_virtual, boolean automatic_allocation,
        routine_declaration *the_routine_declaration,
        const source_location *location)
  {
    declaration *result;

    result = create_empty_declaration(name, is_static, is_virtual,
                                      automatic_allocation, location);
    if (result == NULL)
      {
        delete_routine_declaration(the_routine_declaration);
        return NULL;
      }

    result->kind = NK_ROUTINE;
    result->u.routine = the_routine_declaration;

    set_routine_declaration_declaration(the_routine_declaration, result);

    return result;
  }

extern declaration *create_declaration_for_tagalong(const char *name,
        boolean is_static, boolean is_virtual, boolean automatic_allocation,
        tagalong_declaration *the_tagalong_declaration,
        const source_location *location)
  {
    declaration *result;

    result = create_empty_declaration(name, is_static, is_virtual,
                                      automatic_allocation, location);
    if (result == NULL)
      {
        delete_tagalong_declaration(the_tagalong_declaration);
        return NULL;
      }

    result->kind = NK_TAGALONG;
    result->u.tagalong = the_tagalong_declaration;

    set_tagalong_declaration_declaration(the_tagalong_declaration, result);

    return result;
  }

extern declaration *create_declaration_for_lepton_key(const char *name,
        boolean is_static, boolean is_virtual, boolean automatic_allocation,
        lepton_key_declaration *the_lepton_key_declaration,
        const source_location *location)
  {
    declaration *result;

    result = create_empty_declaration(name, is_static, is_virtual,
                                      automatic_allocation, location);
    if (result == NULL)
      {
        delete_lepton_key_declaration(the_lepton_key_declaration);
        return NULL;
      }

    result->kind = NK_LEPTON_KEY;
    result->u.lepton_key = the_lepton_key_declaration;

    set_lepton_key_declaration_declaration(the_lepton_key_declaration, result);

    return result;
  }

extern declaration *create_declaration_for_quark(const char *name,
        boolean is_static, boolean is_virtual, boolean automatic_allocation,
        quark_declaration *the_quark_declaration,
        const source_location *location)
  {
    declaration *result;

    result = create_empty_declaration(name, is_static, is_virtual,
                                      automatic_allocation, location);
    if (result == NULL)
      {
        delete_quark_declaration(the_quark_declaration);
        return NULL;
      }

    result->kind = NK_QUARK;
    result->u.quark = the_quark_declaration;

    set_quark_declaration_declaration(the_quark_declaration, result);

    return result;
  }

extern declaration *create_declaration_for_lock(const char *name,
        boolean is_static, boolean is_virtual, boolean automatic_allocation,
        lock_declaration *the_lock_declaration,
        const source_location *location)
  {
    declaration *result;

    result = create_empty_declaration(name, is_static, is_virtual,
                                      automatic_allocation, location);
    if (result == NULL)
      {
        delete_lock_declaration(the_lock_declaration);
        return NULL;
      }

    result->kind = NK_LOCK;
    result->u.lock = the_lock_declaration;

    set_lock_declaration_declaration(the_lock_declaration, result);

    return result;
  }

extern void declaration_add_reference(declaration *declaration)
  {
    assert(declaration != NULL);

    GRAB_SYSTEM_LOCK(declaration->reference_lock);
    assert(declaration->reference_count > 0);
    ++(declaration->reference_count);
    RELEASE_SYSTEM_LOCK(declaration->reference_lock);
  }

extern void declaration_remove_reference(declaration *declaration)
  {
    size_t new_reference_count;

    assert(declaration != NULL);

    GRAB_SYSTEM_LOCK(declaration->reference_lock);
    assert(declaration->reference_count > 0);
    --(declaration->reference_count);
    new_reference_count = declaration->reference_count;
    RELEASE_SYSTEM_LOCK(declaration->reference_lock);

    if (new_reference_count > 0)
        return;

    if (declaration->name != NULL)
        free(declaration->name);

    switch (declaration->kind)
      {
        case NK_VARIABLE:
            delete_variable_declaration(declaration->u.variable);
            break;
        case NK_ROUTINE:
            delete_routine_declaration(declaration->u.routine);
            break;
        case NK_TAGALONG:
            delete_tagalong_declaration(declaration->u.tagalong);
            break;
        case NK_LEPTON_KEY:
            delete_lepton_key_declaration(declaration->u.lepton_key);
            break;
        case NK_QUARK:
            delete_quark_declaration(declaration->u.quark);
            break;
        case NK_LOCK:
            delete_lock_declaration(declaration->u.lock);
            break;
        default:
            assert(FALSE);
      }

    source_location_remove_reference(&(declaration->location));

    DESTROY_SYSTEM_LOCK(declaration->reference_lock);

    free(declaration);
  }

extern const char *declaration_name(declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->name;
  }

extern boolean declaration_is_static(declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->is_static;
  }

extern boolean declaration_is_virtual(declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->is_virtual;
  }

extern boolean declaration_automatic_allocation(declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->automatic_allocation;
  }

extern name_kind declaration_kind(declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->kind;
  }

extern variable_declaration *declaration_variable_declaration(
        declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->kind == NK_VARIABLE);
    return declaration->u.variable;
  }

extern routine_declaration *declaration_routine_declaration(
        declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->kind == NK_ROUTINE);
    return declaration->u.routine;
  }

extern tagalong_declaration *declaration_tagalong_declaration(
        declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->kind == NK_TAGALONG);
    return declaration->u.tagalong;
  }

extern lepton_key_declaration *declaration_lepton_key_declaration(
        declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->kind == NK_LEPTON_KEY);
    return declaration->u.lepton_key;
  }

extern quark_declaration *declaration_quark_declaration(
        declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->kind == NK_QUARK);
    return declaration->u.quark;
  }

extern lock_declaration *declaration_lock_declaration(declaration *declaration)

  {
    assert(declaration != NULL);

    assert(declaration->kind == NK_LOCK);
    return declaration->u.lock;
  }

extern void *declaration_parent_pointer(declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->parent_pointer;
  }

extern size_t declaration_parent_index(declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->parent_index;
  }

extern static_home *declaration_static_parent_pointer(declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->static_parent_pointer;
  }

extern size_t declaration_static_parent_index(declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->is_static);
    return declaration->static_parent_index;
  }

extern void declaration_set_parent_pointer(declaration *declaration,
                                           void *parent)
  {
    assert(declaration != NULL);

    declaration->parent_pointer = parent;
  }

extern void declaration_set_parent_index(declaration *declaration,
                                         size_t parent_index)
  {
    assert(declaration != NULL);

    declaration->parent_index = parent_index;
  }

extern void declaration_set_static_parent_pointer(declaration *declaration,
                                                  static_home *parent)
  {
    assert(declaration != NULL);

    assert(declaration->is_static);
    declaration->static_parent_pointer = parent;
  }

extern void declaration_set_static_parent_index(declaration *declaration,
                                                size_t parent_index)
  {
    assert(declaration != NULL);

    assert(declaration->is_static);
    declaration->static_parent_index = parent_index;
  }

extern void set_declaration_start_location(declaration *declaration,
                                           const source_location *location)
  {
    assert(declaration != NULL);
    assert(location != NULL);

    set_location_start(&(declaration->location), location);
  }

extern void set_declaration_end_location(declaration *declaration,
                                         const source_location *location)
  {
    assert(declaration != NULL);
    assert(location != NULL);

    set_location_end(&(declaration->location), location);
  }

extern const source_location *get_declaration_location(
        declaration *declaration)
  {
    assert(declaration != NULL);

    return &(declaration->location);
  }


static declaration *create_empty_declaration(const char *name,
        boolean is_static, boolean is_virtual, boolean automatic_allocation,
        const source_location *location)
  {
    declaration *result;
    char *name_copy;

    result = MALLOC_ONE_OBJECT(declaration);
    if (result == NULL)
        return NULL;

    if (name == NULL)
      {
        name_copy = NULL;
      }
    else
      {
        name_copy = MALLOC_ARRAY(char, strlen(name) + 1);
        if (name_copy == NULL)
          {
            free(result);
            return NULL;
          }

        strcpy(name_copy, name);
      }

    result->name = name_copy;
    result->is_static = is_static;
    result->is_virtual = is_virtual;
    result->automatic_allocation = automatic_allocation;
    result->parent_pointer = NULL;
    result->parent_index = 0;
    result->static_parent_pointer = NULL;
    result->static_parent_index = 0;

    set_source_location(&(result->location), location);

    INITIALIZE_SYSTEM_LOCK(result->reference_lock,
            if (name_copy != NULL)
                free(name_copy);
            free(result);
            return NULL);

    result->reference_count = 1;

    return result;
  }
