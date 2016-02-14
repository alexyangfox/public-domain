/* file "virtual_lookup.c" */

/*
 *  This file contains the implementation of the virtual_lookup module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include "c_foundations/memory_allocation.h"
#include "virtual_lookup.h"
#include "statement_block.h"
#include "context.h"
#include "instance.h"


struct virtual_lookup
  {
    statement_block *statement_block;
    context *context;
    virtual_lookup *next;
    reference_cluster *cluster;
  };


extern virtual_lookup *create_virtual_lookup(
        statement_block *the_statement_block, context *the_context,
        virtual_lookup *next, reference_cluster *cluster)
  {
    virtual_lookup *result;

    result = MALLOC_ONE_OBJECT(virtual_lookup);
    if (result == NULL)
        return NULL;

    result->statement_block = the_statement_block;
    result->context = the_context;
    result->next = next;
    result->cluster = cluster;

    return result;
  }

extern void delete_virtual_lookup(virtual_lookup *the_virtual_lookup)
  {
    assert(the_virtual_lookup != NULL);

    free(the_virtual_lookup);
  }

extern instance *virtual_lookup_find_instance(
        virtual_lookup *the_virtual_lookup, const char *name, name_kind kind,
        context **owning_context)
  {
    size_t instance_number;
    declaration *the_declaration;
    instance *result;

    if (the_virtual_lookup == NULL)
        return NULL;

    instance_number = statement_block_lookup_name(
            the_virtual_lookup->statement_block, name);
    if ((instance_number ==
         statement_block_name_count(the_virtual_lookup->statement_block)) ||
        (statement_block_name_kind(the_virtual_lookup->statement_block,
                                   instance_number) != kind))
      {
        return virtual_lookup_find_instance(the_virtual_lookup->next, name,
                                            kind, owning_context);
      }

    the_declaration = statement_block_name_declaration(
            the_virtual_lookup->statement_block, instance_number);
    assert(the_declaration != NULL);

    result = find_instance(the_virtual_lookup->context, the_declaration);
    assert(result != NULL);
    if (owning_context != NULL)
        *owning_context = the_virtual_lookup->context;
    return result;
  }

extern context *virtual_lookup_context(virtual_lookup *the_virtual_lookup)
  {
    assert(the_virtual_lookup != NULL);

    return the_virtual_lookup->context;
  }

extern reference_cluster *virtual_lookup_reference_cluster(
        virtual_lookup *the_virtual_lookup)
  {
    assert(the_virtual_lookup != NULL);

    return the_virtual_lookup->cluster;
  }
