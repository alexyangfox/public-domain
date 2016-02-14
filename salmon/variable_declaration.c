/* file "variable_declaration.c" */

/*
 *  This file contains the implementation of the variable_declaration module.
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
#include "c_foundations/memory_allocation.h"
#include "variable_declaration.h"
#include "type_expression.h"
#include "expression.h"
#include "declaration.h"


struct variable_declaration
  {
    declaration *declaration;
    type_expression *type;
    expression *initializer;
    boolean force_type_in_initialization;
    boolean is_immutable;
    expression *single_lock;
  };


extern variable_declaration *create_variable_declaration(
        type_expression *the_type_expression, expression *initializer,
        boolean force_type_in_initialization, boolean is_immutable,
        expression *single_lock)
  {
    variable_declaration *result;

    assert(the_type_expression != NULL);

    result = MALLOC_ONE_OBJECT(variable_declaration);
    if (result == NULL)
      {
        delete_type_expression(the_type_expression);
        if (initializer != NULL)
            delete_expression(initializer);
        if (single_lock != NULL)
            delete_expression(single_lock);
        return NULL;
      }

    result->declaration = NULL;
    result->type = the_type_expression;
    result->initializer = initializer;
    result->force_type_in_initialization = force_type_in_initialization;
    result->is_immutable = is_immutable;
    result->single_lock = single_lock;

    return result;
  }

extern void variable_declaration_add_reference(
        variable_declaration *declaration)
  {
    assert(declaration != NULL);

    declaration_add_reference(declaration->declaration);
  }

extern void variable_declaration_remove_reference(
        variable_declaration *declaration)
  {
    assert(declaration != NULL);

    declaration_remove_reference(declaration->declaration);
  }

extern void delete_variable_declaration(variable_declaration *declaration)
  {
    assert(declaration != NULL);

    delete_type_expression(declaration->type);
    if (declaration->initializer != NULL)
        delete_expression(declaration->initializer);
    if (declaration->single_lock != NULL)
        delete_expression(declaration->single_lock);

    free(declaration);
  }

extern declaration *variable_declaration_declaration(
        variable_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration->declaration;
  }

extern const char *variable_declaration_name(variable_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_name(declaration->declaration);
  }

extern type_expression *variable_declaration_type(
        variable_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->type;
  }

extern expression *variable_declaration_initializer(
        variable_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->initializer;
  }

extern boolean variable_declaration_force_type_in_initialization(
        variable_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->force_type_in_initialization;
  }

extern boolean variable_declaration_is_immutable(
        variable_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->is_immutable;
  }

extern boolean variable_declaration_is_static(
        variable_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_is_static(declaration->declaration);
  }

extern boolean variable_declaration_is_virtual(
        variable_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_is_virtual(declaration->declaration);
  }

extern expression *variable_declaration_single_lock(
        variable_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->single_lock;
  }

extern boolean variable_declaration_automatic_allocation(
        variable_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_automatic_allocation(declaration->declaration);
  }

extern void set_variable_declaration_declaration(
        variable_declaration *the_variable_declaration,
        declaration *the_declaration)
  {
    assert(the_variable_declaration != NULL);
    assert(the_declaration != NULL);

    assert(the_variable_declaration->declaration == NULL);
    the_variable_declaration->declaration = the_declaration;
  }
