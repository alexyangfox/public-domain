/* file "open_expression.c" */

/*
 *  This file contains the implementation of the open_expression module.
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
#include "c_foundations/memory_allocation.h"
#include "open_expression.h"
#include "expression.h"
#include "unbound.h"


struct open_expression
  {
    expression *the_expression;
    unbound_name_manager *the_unbound_name_manager;
  };


extern open_expression *create_open_expression(expression *the_expression,
        unbound_name_manager *the_unbound_name_manager)
  {
    open_expression *result;

    result = MALLOC_ONE_OBJECT(open_expression);
    if (result == NULL)
      {
        if (the_expression != NULL)
            delete_expression(the_expression);
        if (the_unbound_name_manager != NULL)
            delete_unbound_name_manager(the_unbound_name_manager);
        return NULL;
      }

    result->the_expression = the_expression;
    result->the_unbound_name_manager = the_unbound_name_manager;

    return result;
  }

extern void delete_open_expression(open_expression *the_open_expression)
  {
    assert(the_open_expression != NULL);

    if (the_open_expression->the_expression != NULL)
        delete_expression(the_open_expression->the_expression);

    if (the_open_expression->the_unbound_name_manager != NULL)
      {
        delete_unbound_name_manager(
                the_open_expression->the_unbound_name_manager);
      }

    free(the_open_expression);
  }

extern expression *open_expression_expression(
        open_expression *the_open_expression)
  {
    assert(the_open_expression != NULL);

    return the_open_expression->the_expression;
  }

extern unbound_name_manager *open_expression_unbound_name_manager(
        open_expression *the_open_expression)
  {
    assert(the_open_expression != NULL);

    return the_open_expression->the_unbound_name_manager;
  }

extern void set_open_expression_expression(
        open_expression *the_open_expression, expression *the_expression)
  {
    assert(the_open_expression != NULL);

    the_open_expression->the_expression = the_expression;
  }

extern void set_open_expression_unbound_name_manager(
        open_expression *the_open_expression,
        unbound_name_manager *the_unbound_name_manager)
  {
    assert(the_open_expression != NULL);

    the_open_expression->the_unbound_name_manager = the_unbound_name_manager;
  }

extern void decompose_open_expression(open_expression *the_open_expression,
        unbound_name_manager **manager, expression **the_expression)
  {
    assert(the_open_expression != NULL);
    assert(manager != NULL);
    assert(the_expression != NULL);

    *the_expression = open_expression_expression(the_open_expression);
    assert((*the_expression) != NULL);

    *manager = open_expression_unbound_name_manager(the_open_expression);
    assert((*manager) != NULL);

    set_open_expression_expression(the_open_expression, NULL);
    set_open_expression_unbound_name_manager(the_open_expression, NULL);

    delete_open_expression(the_open_expression);
  }
