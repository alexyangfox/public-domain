/* file "open_statement.c" */

/*
 *  This file contains the implementation of the open_statement module.
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
#include "open_statement.h"
#include "statement.h"
#include "unbound.h"


struct open_statement
  {
    statement *the_statement;
    unbound_name_manager *the_unbound_name_manager;
  };


extern open_statement *create_open_statement(statement *the_statement,
        unbound_name_manager *the_unbound_name_manager)
  {
    open_statement *result;

    result = MALLOC_ONE_OBJECT(open_statement);
    if (result == NULL)
      {
        if (the_statement != NULL)
            delete_statement(the_statement);
        if (the_unbound_name_manager != NULL)
            delete_unbound_name_manager(the_unbound_name_manager);
        return NULL;
      }

    result->the_statement = the_statement;
    result->the_unbound_name_manager = the_unbound_name_manager;

    return result;
  }

extern void delete_open_statement(open_statement *the_open_statement)
  {
    assert(the_open_statement != NULL);

    if (the_open_statement->the_statement != NULL)
        delete_statement(the_open_statement->the_statement);

    if (the_open_statement->the_unbound_name_manager != NULL)
      {
        delete_unbound_name_manager(
                the_open_statement->the_unbound_name_manager);
      }

    free(the_open_statement);
  }

extern statement *open_statement_statement(open_statement *the_open_statement)
  {
    assert(the_open_statement != NULL);

    return the_open_statement->the_statement;
  }

extern unbound_name_manager *open_statement_unbound_name_manager(
        open_statement *the_open_statement)
  {
    assert(the_open_statement != NULL);

    return the_open_statement->the_unbound_name_manager;
  }

extern void set_open_statement_statement(open_statement *the_open_statement,
                                         statement *the_statement)
  {
    assert(the_open_statement != NULL);

    the_open_statement->the_statement = the_statement;
  }

extern void set_open_statement_unbound_name_manager(
        open_statement *the_open_statement,
        unbound_name_manager *the_unbound_name_manager)
  {
    assert(the_open_statement != NULL);

    the_open_statement->the_unbound_name_manager = the_unbound_name_manager;
  }

extern void decompose_open_statement(open_statement *the_open_statement,
        unbound_name_manager **manager, statement **the_statement)
  {
    assert(the_open_statement != NULL);
    assert(manager != NULL);
    assert(the_statement != NULL);

    *the_statement = open_statement_statement(the_open_statement);
    assert((*the_statement) != NULL);

    *manager = open_statement_unbound_name_manager(the_open_statement);
    assert((*manager) != NULL);

    set_open_statement_statement(the_open_statement, NULL);
    set_open_statement_unbound_name_manager(the_open_statement, NULL);

    delete_open_statement(the_open_statement);
  }
