/* file "declaration.h" */

/*
 *  This file contains the interface to the declaration module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef DECLARATION_H
#define DECLARATION_H

#include "c_foundations/basic.h"


typedef struct declaration declaration;


#include "source_location.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "tagalong_declaration.h"
#include "lepton_key_declaration.h"
#include "quark_declaration.h"
#include "lock_declaration.h"
#include "declaration_list.h"
#include "static_home.h"


extern declaration *create_declaration_for_variable(const char *name,
        boolean is_static, boolean is_virtual, boolean automatic_allocation,
        variable_declaration *the_variable_declaration,
        const source_location *location);
extern declaration *create_declaration_for_routine(const char *name,
        boolean is_static, boolean is_virtual, boolean automatic_allocation,
        routine_declaration *the_routine_declaration,
        const source_location *location);
extern declaration *create_declaration_for_tagalong(const char *name,
        boolean is_static, boolean is_virtual, boolean automatic_allocation,
        tagalong_declaration *the_tagalong_declaration,
        const source_location *location);
extern declaration *create_declaration_for_lepton_key(const char *name,
        boolean is_static, boolean is_virtual, boolean automatic_allocation,
        lepton_key_declaration *the_lepton_key_declaration,
        const source_location *location);
extern declaration *create_declaration_for_quark(const char *name,
        boolean is_static, boolean is_virtual, boolean automatic_allocation,
        quark_declaration *the_quark_declaration,
        const source_location *location);
extern declaration *create_declaration_for_lock(const char *name,
        boolean is_static, boolean is_virtual, boolean automatic_allocation,
        lock_declaration *the_lock_declaration,
        const source_location *location);

extern void declaration_add_reference(declaration *declaration);
extern void declaration_remove_reference(declaration *declaration);

extern const char *declaration_name(declaration *declaration);
extern boolean declaration_is_static(declaration *declaration);
extern boolean declaration_is_virtual(declaration *declaration);
extern boolean declaration_automatic_allocation(declaration *declaration);

extern name_kind declaration_kind(declaration *declaration);

extern variable_declaration *declaration_variable_declaration(
        declaration *declaration);
extern routine_declaration *declaration_routine_declaration(
        declaration *declaration);
extern tagalong_declaration *declaration_tagalong_declaration(
        declaration *declaration);
extern lepton_key_declaration *declaration_lepton_key_declaration(
        declaration *declaration);
extern quark_declaration *declaration_quark_declaration(
        declaration *declaration);
extern lock_declaration *declaration_lock_declaration(
        declaration *declaration);

extern void *declaration_parent_pointer(declaration *declaration);
extern size_t declaration_parent_index(declaration *declaration);
extern static_home *declaration_static_parent_pointer(
        declaration *declaration);
extern size_t declaration_static_parent_index(declaration *declaration);

extern void declaration_set_parent_pointer(declaration *declaration,
                                           void *parent);
extern void declaration_set_parent_index(declaration *declaration,
                                         size_t parent_index);
extern void declaration_set_static_parent_pointer(declaration *declaration,
                                                  static_home *parent);
extern void declaration_set_static_parent_index(declaration *declaration,
                                                size_t parent_index);

extern void set_declaration_start_location(declaration *declaration,
                                           const source_location *location);
extern void set_declaration_end_location(declaration *declaration,
                                         const source_location *location);

extern const source_location *get_declaration_location(
        declaration *declaration);


#endif /* DECLARATION_H */
