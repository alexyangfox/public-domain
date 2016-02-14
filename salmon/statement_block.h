/* file "statement_block.h" */

/*
 *  This file contains the interface to the statement_block module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef STATEMENT_BLOCK_H
#define STATEMENT_BLOCK_H

#include "c_foundations/basic.h"


typedef struct statement_block statement_block;


#include <stddef.h>
#include "statement.h"
#include "declaration.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "tagalong_declaration.h"
#include "lepton_key_declaration.h"
#include "quark_declaration.h"
#include "lock_declaration.h"
#include "declaration_list.h"
#include "source_location.h"


extern statement_block *create_statement_block(void);

extern void delete_statement_block(statement_block *the_statement_block);

extern size_t statement_block_statement_count(
        statement_block *the_statement_block);
extern statement *statement_block_statement(
        statement_block *the_statement_block, size_t statement_number);

extern size_t statement_block_name_count(statement_block *the_statement_block);
extern name_kind statement_block_name_kind(
        statement_block *the_statement_block, size_t name_number);
extern declaration *statement_block_name_declaration(
        statement_block *the_statement_block, size_t name_number);
extern variable_declaration *statement_block_name_variable_declaration(
        statement_block *the_statement_block, size_t name_number);
extern routine_declaration *statement_block_name_routine_declaration(
        statement_block *the_statement_block, size_t name_number);
extern tagalong_declaration *statement_block_name_tagalong_declaration(
        statement_block *the_statement_block, size_t name_number);
extern lepton_key_declaration *statement_block_name_lepton_key_declaration(
        statement_block *the_statement_block, size_t name_number);
extern quark_declaration *statement_block_name_quark_declaration(
        statement_block *the_statement_block, size_t name_number);
extern lock_declaration *statement_block_name_lock_declaration(
        statement_block *the_statement_block, size_t name_number);
extern statement *statement_block_name_jump_target_declaration(
        statement_block *the_statement_block, size_t name_number);
extern size_t statement_block_lookup_name(statement_block *the_statement_block,
                                          const char *name);

extern size_t statement_block_use_statement_count(
        statement_block *the_statement_block);
extern statement *statement_block_use_statement(
        statement_block *the_statement_block, size_t use_statement_number);

extern size_t statement_block_dangling_routine_count(
        statement_block *the_statement_block);
extern routine_declaration *statement_block_dangling_routine_declaration(
        statement_block *the_statement_block, size_t number);

extern size_t statement_block_declaration_count(
        statement_block *the_statement_block);
extern declaration *statement_block_declaration_by_number(
        statement_block *the_statement_block, size_t number);

extern void set_statement_block_start_location(
        statement_block *the_statement_block, const source_location *location);
extern void set_statement_block_end_location(
        statement_block *the_statement_block, const source_location *location);
extern verdict append_statement_to_block(statement_block *the_statement_block,
                                         statement *the_statement);

extern const source_location *get_statement_block_location(
        statement_block *the_statement_block);


#endif /* STATEMENT_BLOCK_H */
