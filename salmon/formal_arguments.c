/* file "formal_arguments.c" */

/*
 *  This file contains the implementation of the formal_arguments module.
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
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/auto_array.h"
#include "c_foundations/auto_array_implementation.h"
#include "formal_arguments.h"
#include "declaration.h"
#include "variable_declaration.h"
#include "c_foundations/try.h"


typedef struct
  {
    declaration *declaration;
    type_expression *dynamic_type;
  } one_formal;


AUTO_ARRAY(one_formal_aa, one_formal);


struct formal_arguments
  {
    one_formal_aa declarations;
  };


AUTO_ARRAY_IMPLEMENTATION(one_formal_aa, one_formal, 0);


extern formal_arguments *create_formal_arguments(void)
  {
    formal_arguments *result;

    result = MALLOC_ONE_OBJECT(formal_arguments);
    if (result == NULL)
        goto abort;

    TRY(free_result, one_formal_aa_init(&(result->declarations), 10));

    return result;

  free_result:
    free(result);
    DO_ABORT(NULL);
  }

extern void delete_formal_arguments(formal_arguments *the_formal_arguments)
  {
    one_formal *array;
    size_t count;
    size_t num;

    assert(the_formal_arguments != NULL);

    array = the_formal_arguments->declarations.array;
    count = the_formal_arguments->declarations.element_count;
    for (num = 0; num < count; ++num)
      {
        declaration_remove_reference(array[num].declaration);
        if (array[num].dynamic_type != NULL)
            delete_type_expression(array[num].dynamic_type);
      }

    free(array);

    free(the_formal_arguments);
  }

extern size_t formal_arguments_argument_count(
        formal_arguments *the_formal_arguments)
  {
    assert(the_formal_arguments != NULL);

    return the_formal_arguments->declarations.element_count;
  }

extern variable_declaration *formal_arguments_formal_by_number(
        formal_arguments *the_formal_arguments, size_t number)
  {
    size_t count;

    assert(the_formal_arguments != NULL);

    count = the_formal_arguments->declarations.element_count;

    assert(number < count);
    return declaration_variable_declaration(
            the_formal_arguments->declarations.array[number].declaration);
  }

extern type_expression *formal_arguments_dynamic_type_by_number(
        formal_arguments *the_formal_arguments, size_t number)
  {
    size_t count;

    assert(the_formal_arguments != NULL);

    count = the_formal_arguments->declarations.element_count;

    assert(number < count);
    return the_formal_arguments->declarations.array[number].dynamic_type;
  }

extern verdict add_formal_parameter(formal_arguments *the_formal_arguments,
        declaration *new_formal, type_expression *dynamic_type)
  {
    one_formal new_item;

    assert(the_formal_arguments != NULL);
    assert(new_formal != NULL);

    declaration_set_parent_index(new_formal,
            the_formal_arguments->declarations.element_count);
    declaration_set_parent_pointer(new_formal, the_formal_arguments);

    new_item.declaration = new_formal;
    new_item.dynamic_type = dynamic_type;

    return one_formal_aa_append(&(the_formal_arguments->declarations),
                                new_item);
  }
