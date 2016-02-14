/* file "open_statement_block.c" */

/*
 *  This file contains the implementation of the open_statement_block module.
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
#include "open_statement_block.h"
#include "statement_block.h"
#include "unbound.h"


struct open_statement_block
  {
    statement_block *the_statement_block;
    unbound_name_manager *the_unbound_name_manager;
  };


extern open_statement_block *create_open_statement_block(
        statement_block *the_statement_block,
        unbound_name_manager *the_unbound_name_manager)
  {
    open_statement_block *result;

    result = MALLOC_ONE_OBJECT(open_statement_block);
    if (result == NULL)
      {
        if (the_statement_block != NULL)
            delete_statement_block(the_statement_block);
        if (the_unbound_name_manager != NULL)
            delete_unbound_name_manager(the_unbound_name_manager);
        return NULL;
      }

    result->the_statement_block = the_statement_block;
    result->the_unbound_name_manager = the_unbound_name_manager;

    return result;
  }

extern void delete_open_statement_block(
        open_statement_block *the_open_statement_block)
  {
    assert(the_open_statement_block != NULL);

    if (the_open_statement_block->the_statement_block != NULL)
        delete_statement_block(the_open_statement_block->the_statement_block);

    if (the_open_statement_block->the_unbound_name_manager != NULL)
      {
        delete_unbound_name_manager(
                the_open_statement_block->the_unbound_name_manager);
      }

    free(the_open_statement_block);
  }

extern statement_block *open_statement_block_statement_block(
        open_statement_block *the_open_statement_block)
  {
    assert(the_open_statement_block != NULL);

    return the_open_statement_block->the_statement_block;
  }

extern unbound_name_manager *open_statement_block_unbound_name_manager(
        open_statement_block *the_open_statement_block)
  {
    assert(the_open_statement_block != NULL);

    return the_open_statement_block->the_unbound_name_manager;
  }

extern void set_open_statement_block_statement_block(
        open_statement_block *the_open_statement_block,
        statement_block *the_statement_block)
  {
    assert(the_open_statement_block != NULL);

    the_open_statement_block->the_statement_block = the_statement_block;
  }

extern void set_open_statement_block_unbound_name_manager(
        open_statement_block *the_open_statement_block,
        unbound_name_manager *the_unbound_name_manager)
  {
    assert(the_open_statement_block != NULL);

    the_open_statement_block->the_unbound_name_manager =
            the_unbound_name_manager;
  }

extern void decompose_open_statement_block(
        open_statement_block *the_open_statement_block,
        unbound_name_manager **manager, statement_block **the_statement_block)
  {
    assert(the_open_statement_block != NULL);
    assert(manager != NULL);
    assert(the_statement_block != NULL);

    *the_statement_block =
            open_statement_block_statement_block(the_open_statement_block);
    assert((*the_statement_block) != NULL);

    *manager = open_statement_block_unbound_name_manager(
            the_open_statement_block);
    assert((*manager) != NULL);

    set_open_statement_block_statement_block(the_open_statement_block, NULL);
    set_open_statement_block_unbound_name_manager(the_open_statement_block,
                                                  NULL);

    delete_open_statement_block(the_open_statement_block);
  }
