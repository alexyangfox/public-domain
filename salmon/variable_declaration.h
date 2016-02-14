/* file "variable_declaration.h" */

/*
 *  This file contains the interface to the variable_declaration module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef VARIABLE_DECLARATION_H
#define VARIABLE_DECLARATION_H

#include "c_foundations/basic.h"


typedef struct variable_declaration variable_declaration;


#include "type_expression.h"
#include "expression.h"
#include "formal_arguments.h"
#include "statement_block.h"
#include "declaration.h"


extern variable_declaration *create_variable_declaration(
        type_expression *the_type_expression, expression *initializer,
        boolean force_type_in_initialization, boolean is_immutable,
        expression *single_lock);

extern void variable_declaration_add_reference(
        variable_declaration *declaration);
extern void variable_declaration_remove_reference(
        variable_declaration *declaration);
extern void delete_variable_declaration(variable_declaration *declaration);

extern declaration *variable_declaration_declaration(
        variable_declaration *declaration);
extern const char *variable_declaration_name(
        variable_declaration *declaration);
extern type_expression *variable_declaration_type(
        variable_declaration *declaration);
extern expression *variable_declaration_initializer(
        variable_declaration *declaration);
extern boolean variable_declaration_force_type_in_initialization(
        variable_declaration *declaration);
extern boolean variable_declaration_is_immutable(
        variable_declaration *declaration);
extern boolean variable_declaration_is_static(
        variable_declaration *declaration);
extern boolean variable_declaration_is_virtual(
        variable_declaration *declaration);
extern expression *variable_declaration_single_lock(
        variable_declaration *declaration);
extern boolean variable_declaration_automatic_allocation(
        variable_declaration *declaration);

extern void set_variable_declaration_declaration(
        variable_declaration *the_variable_declaration,
        declaration *the_declaration);


#endif /* VARIABLE_DECLARATION_H */
