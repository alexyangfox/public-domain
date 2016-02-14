/* file "routine_declaration_chain.h" */

/*
 *  This file contains the interface to the routine_declaration_chain module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef ROUTINE_DECLARATION_CHAIN_H
#define ROUTINE_DECLARATION_CHAIN_H


typedef struct routine_declaration_chain routine_declaration_chain;


#include "context.h"
#include "routine_instance_chain.h"
#include "routine_declaration.h"
#include "jumper.h"


extern routine_declaration_chain *create_routine_declaration_chain(
        routine_declaration *declaration,
        routine_declaration_chain *next_chain);

extern void routine_declaration_chain_add_reference(
        routine_declaration_chain *chain);
extern void routine_declaration_chain_remove_reference(
        routine_declaration_chain *chain);

extern routine_declaration *routine_declaration_chain_declaration(
        routine_declaration_chain *chain);
extern routine_declaration_chain *routine_declaration_chain_next(
        routine_declaration_chain *chain);
extern boolean routine_declaration_chain_has_more_than_one_reference(
        routine_declaration_chain *chain);

extern void routine_declaration_chain_set_next(routine_declaration_chain *base,
        routine_declaration_chain *next);
extern void routine_declaration_chain_set_next_to_use_statement(
        routine_declaration_chain *base, statement *use_statement,
        size_t used_for_num);

extern routine_instance_chain *
        routine_declaration_chain_to_routine_instance_chain(
                routine_declaration_chain *declaration_chain,
                context *the_context, jumper *the_jumper);


#endif /* ROUTINE_DECLARATION_CHAIN_H */
