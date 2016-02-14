/* file "open_routine_declaration.h" */

/*
 *  This file contains the interface to the open_routine_declaration module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef OPEN_ROUTINE_DECLARATION_H
#define OPEN_ROUTINE_DECLARATION_H


typedef struct open_routine_declaration open_routine_declaration;


#include "routine_declaration.h"
#include "unbound.h"


extern open_routine_declaration *create_open_routine_declaration(
        routine_declaration *the_routine_declaration,
        unbound_name_manager *the_unbound_name_manager);

extern void delete_open_routine_declaration(
        open_routine_declaration *the_open_routine_declaration);

extern routine_declaration *open_routine_declaration_routine_declaration(
        open_routine_declaration *the_open_routine_declaration);
extern unbound_name_manager *open_routine_declaration_unbound_name_manager(
        open_routine_declaration *the_open_routine_declaration);

extern void set_open_routine_declaration_routine_declaration(
        open_routine_declaration *the_open_routine_declaration,
        routine_declaration *the_routine_declaration);
extern void set_open_routine_declaration_unbound_name_manager(
        open_routine_declaration *the_open_routine_declaration,
        unbound_name_manager *the_unbound_name_manager);


#endif /* OPEN_ROUTINE_DECLARATION_H */
