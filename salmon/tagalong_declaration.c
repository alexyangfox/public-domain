/* file "tagalong_declaration.c" */

/*
 *  This file contains the implementation of the tagalong_declaration module.
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
#include "tagalong_declaration.h"
#include "type_expression.h"
#include "expression.h"
#include "declaration.h"


struct tagalong_declaration
  {
    declaration *declaration;
    type_expression *type;
    expression *initializer;
    boolean force_type_in_initialization;
    expression *single_lock;
    type_expression *on;
    boolean is_object;
  };


extern tagalong_declaration *create_tagalong_declaration(type_expression *type,
        expression *initializer, boolean force_type_in_initialization,
        expression *single_lock, type_expression *on, boolean is_object)
  {
    tagalong_declaration *result;

    assert(type != NULL);
    assert(on != NULL);

    result = MALLOC_ONE_OBJECT(tagalong_declaration);
    if (result == NULL)
      {
        delete_type_expression(type);
        if (initializer != NULL)
            delete_expression(initializer);
        if (single_lock != NULL)
            delete_expression(single_lock);
        delete_type_expression(on);
        return NULL;
      }

    result->declaration = NULL;
    result->type = type;
    result->initializer = initializer;
    result->force_type_in_initialization = force_type_in_initialization;
    result->single_lock = single_lock;
    result->on = on;
    result->is_object = is_object;

    return result;
  }

extern void tagalong_declaration_add_reference(
        tagalong_declaration *declaration)
  {
    assert(declaration != NULL);

    declaration_add_reference(declaration->declaration);
  }

extern void tagalong_declaration_remove_reference(
        tagalong_declaration *declaration)
  {
    assert(declaration != NULL);

    declaration_remove_reference(declaration->declaration);
  }

extern void delete_tagalong_declaration(tagalong_declaration *declaration)
  {
    assert(declaration != NULL);

    delete_type_expression(declaration->type);
    if (declaration->initializer != NULL)
        delete_expression(declaration->initializer);
    if (declaration->single_lock != NULL)
        delete_expression(declaration->single_lock);
    delete_type_expression(declaration->on);

    free(declaration);
  }

extern declaration *tagalong_declaration_declaration(
        tagalong_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration->declaration;
  }

extern const char *tagalong_declaration_name(tagalong_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_name(declaration->declaration);
  }

extern type_expression *tagalong_declaration_type(
        tagalong_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->type;
  }

extern expression *tagalong_declaration_initializer(
        tagalong_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->initializer;
  }

extern boolean tagalong_declaration_force_type_in_initialization(
        tagalong_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->force_type_in_initialization;
  }

extern boolean tagalong_declaration_is_static(
        tagalong_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_is_static(declaration->declaration);
  }

extern boolean tagalong_declaration_is_virtual(
        tagalong_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_is_virtual(declaration->declaration);
  }

extern expression *tagalong_declaration_single_lock(
        tagalong_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->single_lock;
  }

extern boolean tagalong_declaration_automatic_allocation(
        tagalong_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_automatic_allocation(declaration->declaration);
  }

extern type_expression *tagalong_declaration_on(
        tagalong_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->on;
  }

extern boolean tagalong_declaration_is_object(
        tagalong_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->is_object;
  }

extern void set_tagalong_declaration_declaration(
        tagalong_declaration *the_tagalong_declaration,
        declaration *the_declaration)
  {
    assert(the_tagalong_declaration != NULL);
    assert(the_declaration != NULL);

    assert(the_tagalong_declaration->declaration == NULL);
    the_tagalong_declaration->declaration = the_declaration;
  }
