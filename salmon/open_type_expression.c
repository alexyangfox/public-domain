/* file "open_type_expression.c" */

/*
 *  This file contains the implementation of the open_type_expression module.
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
#include "open_type_expression.h"
#include "type_expression.h"
#include "unbound.h"


struct open_type_expression
  {
    type_expression *the_type_expression;
    unbound_name_manager *the_unbound_name_manager;
  };


extern open_type_expression *create_open_type_expression(
        type_expression *the_type_expression,
        unbound_name_manager *the_unbound_name_manager)
  {
    open_type_expression *result;

    result = MALLOC_ONE_OBJECT(open_type_expression);
    if (result == NULL)
      {
        if (the_type_expression != NULL)
            delete_type_expression(the_type_expression);
        if (the_unbound_name_manager != NULL)
            delete_unbound_name_manager(the_unbound_name_manager);
        return NULL;
      }

    result->the_type_expression = the_type_expression;
    result->the_unbound_name_manager = the_unbound_name_manager;

    return result;
  }

extern void delete_open_type_expression(
        open_type_expression *the_open_type_expression)
  {
    assert(the_open_type_expression != NULL);

    if (the_open_type_expression->the_type_expression != NULL)
        delete_type_expression(the_open_type_expression->the_type_expression);

    if (the_open_type_expression->the_unbound_name_manager != NULL)
      {
        delete_unbound_name_manager(
                the_open_type_expression->the_unbound_name_manager);
      }

    free(the_open_type_expression);
  }

extern type_expression *open_type_expression_type_expression(
        open_type_expression *the_open_type_expression)
  {
    assert(the_open_type_expression != NULL);

    return the_open_type_expression->the_type_expression;
  }

extern unbound_name_manager *open_type_expression_unbound_name_manager(
        open_type_expression *the_open_type_expression)
  {
    assert(the_open_type_expression != NULL);

    return the_open_type_expression->the_unbound_name_manager;
  }

extern void set_open_type_expression_type_expression(
        open_type_expression *the_open_type_expression,
        type_expression *the_type_expression)
  {
    assert(the_open_type_expression != NULL);

    the_open_type_expression->the_type_expression = the_type_expression;
  }

extern void set_open_type_expression_unbound_name_manager(
        open_type_expression *the_open_type_expression,
        unbound_name_manager *the_unbound_name_manager)
  {
    assert(the_open_type_expression != NULL);

    the_open_type_expression->the_unbound_name_manager =
            the_unbound_name_manager;
  }
