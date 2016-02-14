/* file "open_routine_declaration.c" */

/*
 *  This file contains the implementation of the open_routine_declaration
 *  module.
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
#include "open_routine_declaration.h"
#include "routine_declaration.h"
#include "unbound.h"


struct open_routine_declaration
  {
    routine_declaration *the_routine_declaration;
    unbound_name_manager *the_unbound_name_manager;
  };


extern open_routine_declaration *create_open_routine_declaration(
        routine_declaration *the_routine_declaration,
        unbound_name_manager *the_unbound_name_manager)
  {
    open_routine_declaration *result;

    result = MALLOC_ONE_OBJECT(open_routine_declaration);
    if (result == NULL)
      {
        if (the_routine_declaration != NULL)
            delete_routine_declaration(the_routine_declaration);
        if (the_unbound_name_manager != NULL)
            delete_unbound_name_manager(the_unbound_name_manager);
        return NULL;
      }

    result->the_routine_declaration = the_routine_declaration;
    result->the_unbound_name_manager = the_unbound_name_manager;

    return result;
  }

extern void delete_open_routine_declaration(
        open_routine_declaration *the_open_routine_declaration)
  {
    assert(the_open_routine_declaration != NULL);

    if (the_open_routine_declaration->the_routine_declaration != NULL)
      {
        delete_routine_declaration(
                the_open_routine_declaration->the_routine_declaration);
      }

    if (the_open_routine_declaration->the_unbound_name_manager != NULL)
      {
        delete_unbound_name_manager(
                the_open_routine_declaration->the_unbound_name_manager);
      }

    free(the_open_routine_declaration);
  }

extern routine_declaration *open_routine_declaration_routine_declaration(
        open_routine_declaration *the_open_routine_declaration)
  {
    assert(the_open_routine_declaration != NULL);

    return the_open_routine_declaration->the_routine_declaration;
  }

extern unbound_name_manager *open_routine_declaration_unbound_name_manager(
        open_routine_declaration *the_open_routine_declaration)
  {
    assert(the_open_routine_declaration != NULL);

    return the_open_routine_declaration->the_unbound_name_manager;
  }

extern void set_open_routine_declaration_routine_declaration(
        open_routine_declaration *the_open_routine_declaration,
        routine_declaration *the_routine_declaration)
  {
    assert(the_open_routine_declaration != NULL);

    the_open_routine_declaration->the_routine_declaration =
            the_routine_declaration;
  }

extern void set_open_routine_declaration_unbound_name_manager(
        open_routine_declaration *the_open_routine_declaration,
        unbound_name_manager *the_unbound_name_manager)
  {
    assert(the_open_routine_declaration != NULL);

    the_open_routine_declaration->the_unbound_name_manager =
            the_unbound_name_manager;
  }
