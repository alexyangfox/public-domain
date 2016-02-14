/* file "open_statement.h" */

/*
 *  This file contains the interface to the open_statement module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef OPEN_STATEMENT_H
#define OPEN_STATEMENT_H


typedef struct open_statement open_statement;


#include "statement.h"
#include "unbound.h"


extern open_statement *create_open_statement(statement *the_statement,
        unbound_name_manager *the_unbound_name_manager);

extern void delete_open_statement(open_statement *the_open_statement);

extern statement *open_statement_statement(open_statement *the_open_statement);
extern unbound_name_manager *open_statement_unbound_name_manager(
        open_statement *the_open_statement);

extern void set_open_statement_statement(open_statement *the_open_statement,
                                         statement *the_statement);
extern void set_open_statement_unbound_name_manager(
        open_statement *the_open_statement,
        unbound_name_manager *the_unbound_name_manager);

extern void decompose_open_statement(open_statement *the_open_statement,
        unbound_name_manager **manager, statement **the_statement);


#endif /* OPEN_STATEMENT_H */
