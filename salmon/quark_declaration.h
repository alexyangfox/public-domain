/* file "quark_declaration.h" */

/*
 *  This file contains the interface to the quark_declaration module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef QUARK_DECLARATION_H
#define QUARK_DECLARATION_H

#include "c_foundations/basic.h"


typedef struct quark_declaration quark_declaration;


#include "declaration.h"


extern quark_declaration *create_quark_declaration(void);

extern void quark_declaration_add_reference(quark_declaration *declaration);
extern void quark_declaration_remove_reference(quark_declaration *declaration);
extern void delete_quark_declaration(quark_declaration *declaration);

extern declaration *quark_declaration_declaration(
        quark_declaration *declaration);
extern const char *quark_declaration_name(quark_declaration *declaration);
extern boolean quark_declaration_is_static(quark_declaration *declaration);
extern boolean quark_declaration_is_virtual(quark_declaration *declaration);
extern boolean quark_declaration_automatic_allocation(
        quark_declaration *declaration);

extern void set_quark_declaration_declaration(
        quark_declaration *the_quark_declaration,
        declaration *the_declaration);


#endif /* QUARK_DECLARATION_H */
