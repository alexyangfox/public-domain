/* file "lock_declaration.c" */

/*
 *  This file contains the implementation of the lock_declaration module.
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
#include "lock_declaration.h"
#include "expression.h"
#include "declaration.h"


struct lock_declaration
  {
    declaration *declaration;
    expression *single_lock;
  };


extern lock_declaration *create_lock_declaration(expression *single_lock)
  {
    lock_declaration *result;

    result = MALLOC_ONE_OBJECT(lock_declaration);
    if (result == NULL)
      {
        if (single_lock != NULL)
            delete_expression(single_lock);
        return NULL;
      }

    result->declaration = NULL;
    result->single_lock = single_lock;

    return result;
  }

extern void lock_declaration_add_reference(lock_declaration *declaration)
  {
    assert(declaration != NULL);

    declaration_add_reference(declaration->declaration);
  }

extern void lock_declaration_remove_reference(lock_declaration *declaration)
  {
    assert(declaration != NULL);

    declaration_remove_reference(declaration->declaration);
  }

extern void delete_lock_declaration(lock_declaration *declaration)
  {
    assert(declaration != NULL);

    if (declaration->single_lock != NULL)
        delete_expression(declaration->single_lock);

    free(declaration);
  }

extern declaration *lock_declaration_declaration(lock_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration->declaration;
  }

extern const char *lock_declaration_name(lock_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_name(declaration->declaration);
  }

extern boolean lock_declaration_is_static(lock_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_is_static(declaration->declaration);
  }

extern boolean lock_declaration_is_virtual(lock_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_is_virtual(declaration->declaration);
  }

extern expression *lock_declaration_single_lock(lock_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->single_lock;
  }

extern boolean lock_declaration_automatic_allocation(
        lock_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_automatic_allocation(declaration->declaration);
  }

extern void set_lock_declaration_declaration(
        lock_declaration *the_lock_declaration, declaration *the_declaration)
  {
    assert(the_lock_declaration != NULL);
    assert(the_declaration != NULL);

    assert(the_lock_declaration->declaration == NULL);
    the_lock_declaration->declaration = the_declaration;
  }
