/* file "lepton_key_declaration.h" */

/*
 *  This file contains the interface to the lepton_key_declaration module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef LEPTON_KEY_DECLARATION_H
#define LEPTON_KEY_DECLARATION_H

#include <stddef.h>
#include "c_foundations/basic.h"


typedef struct lepton_key_declaration lepton_key_declaration;


#include "expression.h"
#include "type_expression.h"
#include "declaration.h"


extern lepton_key_declaration *create_lepton_key_declaration(
        boolean additional_fields_allowed);

extern void lepton_key_declaration_add_reference(
        lepton_key_declaration *declaration);
extern void lepton_key_declaration_remove_reference(
        lepton_key_declaration *declaration);
extern void delete_lepton_key_declaration(lepton_key_declaration *declaration);

extern declaration *lepton_key_declaration_declaration(
        lepton_key_declaration *declaration);
extern const char *lepton_key_declaration_name(
        lepton_key_declaration *declaration);
extern boolean lepton_key_declaration_additional_fields_allowed(
        lepton_key_declaration *declaration);
extern boolean lepton_key_declaration_is_static(
        lepton_key_declaration *declaration);
extern boolean lepton_key_declaration_is_virtual(
        lepton_key_declaration *declaration);
extern boolean lepton_key_declaration_automatic_allocation(
        lepton_key_declaration *declaration);

extern size_t lepton_key_field_count(lepton_key_declaration *declaration);
extern const char *lepton_key_field_name(lepton_key_declaration *declaration,
                                         size_t field_num);
extern type_expression *lepton_key_field_type(
        lepton_key_declaration *declaration, size_t field_num);

extern size_t lepton_key_lookup_field_by_name(
        lepton_key_declaration *declaration, const char *field_name);

extern verdict lepton_key_add_field(lepton_key_declaration *declaration,
        const char *field_name, type_expression *field_type);
extern verdict lepton_key_declaration_set_additional_fields_allowed(
        lepton_key_declaration *declaration,
        boolean additional_fields_allowed);

extern void set_lepton_key_declaration_declaration(
        lepton_key_declaration *the_lepton_key_declaration,
        declaration *the_declaration);


#endif /* LEPTON_KEY_DECLARATION_H */
