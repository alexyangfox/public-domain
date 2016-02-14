/* file "static_home.c" */

/*
 *  This file contains the implementation of the static_home module.
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
#include "static_home.h"
#include "declaration.h"


struct static_home
  {
    size_t declaration_count;
    declaration **declarations;
  };


extern static_home *create_static_home(size_t static_count,
                                       declaration **static_declarations)
  {
    static_home *result;

    result = MALLOC_ONE_OBJECT(static_home);
    if (result == NULL)
        return NULL;

    result->declaration_count = static_count;
    if (static_count == 0)
      {
        result->declarations = NULL;
      }
    else
      {
        size_t static_num;

        assert(static_declarations != NULL);

        result->declarations = MALLOC_ARRAY(declaration *, static_count);
        if (result->declarations == NULL)
          {
            free(result);
            return NULL;
          }

        for (static_num = 0; static_num < static_count; ++static_num)
          {
            declaration *the_declaration;

            the_declaration = static_declarations[static_num];
            assert(the_declaration != NULL);

            declaration_set_static_parent_pointer(the_declaration, result);
            declaration_set_static_parent_index(the_declaration, static_num);

            result->declarations[static_num] = the_declaration;
          }
      }

    return result;
  }

extern void delete_static_home(static_home *static_home)
  {
    assert(static_home != NULL);

    if (static_home->declarations != NULL)
        free(static_home->declarations);

    free(static_home);
  }

extern size_t static_home_declaration_count(static_home *static_home)
  {
    assert(static_home != NULL);

    return static_home->declaration_count;
  }

extern declaration *static_home_declaration_by_number(static_home *static_home,
                                                      size_t declaration_num)
  {
    assert(static_home != NULL);

    assert(declaration_num < static_home->declaration_count);
    assert(static_home->declarations[declaration_num] != NULL);
    return static_home->declarations[declaration_num];
  }
