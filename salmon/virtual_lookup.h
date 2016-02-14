/* file "virtual_lookup.h" */

/*
 *  This file contains the interface to the virtual_lookup module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef VIRTUAL_LOOKUP_H
#define VIRTUAL_LOOKUP_H


typedef struct virtual_lookup virtual_lookup;


#include "statement_block.h"
#include "context.h"
#include "instance.h"
#include "declaration_list.h"
#include "reference_cluster.h"


extern virtual_lookup *create_virtual_lookup(
        statement_block *the_statement_block, context *the_context,
        virtual_lookup *next, reference_cluster *cluster);

extern void delete_virtual_lookup(virtual_lookup *the_virtual_lookup);

extern instance *virtual_lookup_find_instance(
        virtual_lookup *the_virtual_lookup, const char *name, name_kind kind,
        context **owning_context);
extern context *virtual_lookup_context(virtual_lookup *the_virtual_lookup);
extern reference_cluster *virtual_lookup_reference_cluster(
        virtual_lookup *the_virtual_lookup);


#endif /* VIRTUAL_LOOKUP_H */
