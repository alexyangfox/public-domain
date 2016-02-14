/* file "tagalong_declaration.h" */

/*
 *  This file contains the interface to the tagalong_declaration module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef TAGALONG_DECLARATION_H
#define TAGALONG_DECLARATION_H

#include "c_foundations/basic.h"


typedef struct tagalong_declaration tagalong_declaration;


#include "type_expression.h"
#include "expression.h"
#include "declaration.h"


extern tagalong_declaration *create_tagalong_declaration(type_expression *type,
        expression *initializer, boolean force_type_in_initialization,
        expression *single_lock, type_expression *on, boolean is_object);

extern void tagalong_declaration_add_reference(
        tagalong_declaration *declaration);
extern void tagalong_declaration_remove_reference(
        tagalong_declaration *declaration);
extern void delete_tagalong_declaration(tagalong_declaration *declaration);

extern declaration *tagalong_declaration_declaration(
        tagalong_declaration *declaration);
extern const char *tagalong_declaration_name(
        tagalong_declaration *declaration);
extern type_expression *tagalong_declaration_type(
        tagalong_declaration *declaration);
extern expression *tagalong_declaration_initializer(
        tagalong_declaration *declaration);
extern boolean tagalong_declaration_force_type_in_initialization(
        tagalong_declaration *declaration);
extern boolean tagalong_declaration_is_static(
        tagalong_declaration *declaration);
extern boolean tagalong_declaration_is_virtual(
        tagalong_declaration *declaration);
extern expression *tagalong_declaration_single_lock(
        tagalong_declaration *declaration);
extern boolean tagalong_declaration_automatic_allocation(
        tagalong_declaration *declaration);
extern type_expression *tagalong_declaration_on(
        tagalong_declaration *declaration);
extern boolean tagalong_declaration_is_object(
        tagalong_declaration *declaration);

extern void set_tagalong_declaration_declaration(
        tagalong_declaration *the_tagalong_declaration,
        declaration *the_declaration);


#endif /* TAGALONG_DECLARATION_H */
