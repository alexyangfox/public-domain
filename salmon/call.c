/* file "call.c" */

/*
 *  This file contains the implementation of the call module.
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
#include "c_foundations/auto_array.h"
#include "c_foundations/auto_array_implementation.h"
#include "call.h"
#include "expression.h"
#include "source_location.h"


AUTO_ARRAY(expression_aa, expression *);
AUTO_ARRAY(mstring_aa, char *);


struct call
  {
    expression *base;
    expression_aa actual_argument_expressions;
    mstring_aa formal_argument_names;
    source_location location;
  };


static verdict initialize_call(call *the_call, size_t actual_argument_count,
        expression **actual_argument_expressions,
        const char **formal_argument_names, const source_location *location);


AUTO_ARRAY_IMPLEMENTATION(expression_aa, expression *, 0);


extern call *create_call(expression *base, size_t actual_argument_count,
        expression **actual_argument_expressions,
        const char **formal_argument_names, const source_location *location)
  {
    call *result;
    verdict the_verdict;

    result = MALLOC_ONE_OBJECT(call);
    if (result == NULL)
      {
        size_t actual_num;

        delete_expression(base);
        for (actual_num = 0; actual_num < actual_argument_count; ++actual_num)
            delete_expression(actual_argument_expressions[actual_num]);

        return NULL;
      }

    result->base = base;

    the_verdict = initialize_call(result, actual_argument_count,
            actual_argument_expressions, formal_argument_names, location);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        size_t actual_num;

        delete_expression(base);
        for (actual_num = 0; actual_num < actual_argument_count; ++actual_num)
            delete_expression(actual_argument_expressions[actual_num]);
        free(result);

        return NULL;
      }

    return result;
  }

extern void delete_call(call *the_call)
  {
    expression **expression_array;
    char **name_array;
    size_t count;

    assert(the_call != NULL);

    delete_expression(the_call->base);

    expression_array = the_call->actual_argument_expressions.array;
    assert(expression_array != NULL);
    count = the_call->actual_argument_expressions.element_count;
    while (count > 0)
      {
        expression *going;

        --count;
        going = expression_array[count];
        assert(going != NULL);
        delete_expression(going);
      }
    free(expression_array);

    name_array = the_call->formal_argument_names.array;
    assert(name_array != NULL);
    count = the_call->formal_argument_names.element_count;
    while (count > 0)
      {
        char *going;

        --count;
        going = name_array[count];
        if (going != NULL)
            free(going);
      }
    free(name_array);

    source_location_remove_reference(&(the_call->location));

    free(the_call);
  }

extern expression *call_base(call *the_call)
  {
    assert(the_call != NULL);

    return the_call->base;
  }

extern size_t call_actual_argument_count(call *the_call)
  {
    assert(the_call != NULL);

    assert(the_call->actual_argument_expressions.element_count ==
           the_call->formal_argument_names.element_count);
    return the_call->actual_argument_expressions.element_count;
  }

extern expression *call_actual_argument_expression(call *the_call,
                                                   size_t argument_num)
  {
    assert(the_call != NULL);

    assert(the_call->actual_argument_expressions.element_count ==
           the_call->formal_argument_names.element_count);
    assert(argument_num < the_call->actual_argument_expressions.element_count);
    return the_call->actual_argument_expressions.array[argument_num];
  }

extern const char *call_formal_argument_name(call *the_call,
                                             size_t argument_num)
  {
    assert(the_call != NULL);

    assert(the_call->actual_argument_expressions.element_count ==
           the_call->formal_argument_names.element_count);
    assert(argument_num < the_call->formal_argument_names.element_count);
    return the_call->formal_argument_names.array[argument_num];
  }

extern verdict append_argument_to_call(call *the_call,
        expression *actual_argument_expression,
        const char *formal_argument_name)
  {
    char *copy;
    verdict the_verdict;

    assert(the_call != NULL);
    assert(actual_argument_expression != NULL);

    if (formal_argument_name == NULL)
      {
        copy = NULL;
      }
    else
      {
        copy = MALLOC_ARRAY(char, strlen(formal_argument_name) + 1);
        if (copy == NULL)
          {
            delete_expression(actual_argument_expression);
            return MISSION_FAILED;
          }

        strcpy(copy, formal_argument_name);
      }

    the_verdict = expression_aa_append(
            &(the_call->actual_argument_expressions),
            actual_argument_expression);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(copy);
        delete_expression(actual_argument_expression);
        return the_verdict;
      }

    the_verdict = mstring_aa_append(&(the_call->formal_argument_names), copy);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(the_call->actual_argument_expressions.element_count > 0);
        --(the_call->actual_argument_expressions.element_count);
        free(copy);
        delete_expression(actual_argument_expression);
        return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

extern void set_call_start_location(call *the_call,
                                    const source_location *location)
  {
    set_location_start(&(the_call->location), location);
  }

extern void set_call_end_location(call *the_call,
                                  const source_location *location)
  {
    set_location_end(&(the_call->location), location);
  }

extern const source_location *get_call_location(call *the_call)
  {
    return &(the_call->location);
  }


static verdict initialize_call(call *the_call, size_t actual_argument_count,
        expression **actual_argument_expressions,
        const char **formal_argument_names, const source_location *location)
  {
    size_t space;
    verdict the_verdict;

    assert(the_call != NULL);

    space = ((actual_argument_count < 10) ? 10 : actual_argument_count);

    the_verdict = expression_aa_init(&(the_call->actual_argument_expressions),
                                     space);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_verdict = mstring_aa_init(&(the_call->formal_argument_names), space);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(the_call->actual_argument_expressions.array);
        return the_verdict;
      }

    if (actual_argument_count == 0)
      {
        assert(actual_argument_expressions == NULL);
        assert(formal_argument_names == NULL);
      }
    else
      {
        size_t arg_num;

        assert(actual_argument_expressions != NULL);
        assert(formal_argument_names != NULL);

        for (arg_num = 0; arg_num < actual_argument_count; ++arg_num)
          {
            expression *the_expression;
            verdict the_verdict;
            const char *original_name;
            char *name_copy;

            the_expression = actual_argument_expressions[arg_num];
            assert(the_expression != NULL);
            the_verdict = expression_aa_append(
                    &(the_call->actual_argument_expressions), the_expression);
            assert(the_verdict == MISSION_ACCOMPLISHED);

            original_name = formal_argument_names[arg_num];
            if (original_name == NULL)
              {
                name_copy = NULL;
              }
            else
              {
                name_copy = MALLOC_ARRAY(char, strlen(original_name) + 1);
                if (name_copy == NULL)
                  {
                    free(the_call->actual_argument_expressions.array);
                    while (arg_num > 0)
                      {
                        char *going;

                        --arg_num;
                        going = the_call->formal_argument_names.array[arg_num];
                        if (going != NULL)
                            free(going);
                      }
                    free(the_call->formal_argument_names.array);
                    return MISSION_FAILED;
                  }
                strcpy(name_copy, original_name);
              }
            the_verdict = mstring_aa_append(&(the_call->formal_argument_names),
                                            name_copy);
            assert(the_verdict == MISSION_ACCOMPLISHED);
          }
      }

    set_source_location(&(the_call->location), location);

    return MISSION_ACCOMPLISHED;
  }
