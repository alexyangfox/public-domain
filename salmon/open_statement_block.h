/* file "open_statement_block.h" */

/*
 *  This file contains the interface to the open_statement_block module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef OPEN_STATEMENT_BLOCK_H
#define OPEN_STATEMENT_BLOCK_H


typedef struct open_statement_block open_statement_block;


#include "statement_block.h"
#include "unbound.h"


extern open_statement_block *create_open_statement_block(
        statement_block *the_statement_block,
        unbound_name_manager *the_unbound_name_manager);

extern void delete_open_statement_block(
        open_statement_block *the_open_statement_block);

extern statement_block *open_statement_block_statement_block(
        open_statement_block *the_open_statement_block);
extern unbound_name_manager *open_statement_block_unbound_name_manager(
        open_statement_block *the_open_statement_block);

extern void set_open_statement_block_statement_block(
        open_statement_block *the_open_statement_block,
        statement_block *the_statement_block);
extern void set_open_statement_block_unbound_name_manager(
        open_statement_block *the_open_statement_block,
        unbound_name_manager *the_unbound_name_manager);

extern void decompose_open_statement_block(
        open_statement_block *the_open_statement_block,
        unbound_name_manager **manager, statement_block **the_statement_block);


#endif /* OPEN_STATEMENT_BLOCK_H */
