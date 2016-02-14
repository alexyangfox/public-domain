/* file "declaration_list.h" */

/*
 *  This file contains the interface to the declaration_list module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef DECLARATION_LIST_H
#define DECLARATION_LIST_H

#include "c_foundations/basic.h"


typedef enum
  {
    NK_VARIABLE,
    NK_ROUTINE,
    NK_TAGALONG,
    NK_LEPTON_KEY,
    NK_QUARK,
    NK_LOCK,
    NK_JUMP_TARGET
  } name_kind;

typedef struct declaration_list declaration_list;


#include <stddef.h>
#include "declaration.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "tagalong_declaration.h"
#include "lepton_key_declaration.h"
#include "quark_declaration.h"
#include "lock_declaration.h"
#include "statement.h"


extern declaration_list *create_declaration_list(void);

extern void delete_declaration_list(declaration_list *the_declaration_list);

extern size_t declaration_list_name_count(
        declaration_list *the_declaration_list);
extern name_kind declaration_list_name_kind(
        declaration_list *the_declaration_list, size_t name_number);
extern declaration *declaration_list_name_declaration(
        declaration_list *the_declaration_list, size_t name_number);
extern variable_declaration *declaration_list_name_variable_declaration(
        declaration_list *the_declaration_list, size_t name_number);
extern routine_declaration *declaration_list_name_routine_declaration(
        declaration_list *the_declaration_list, size_t name_number);
extern tagalong_declaration *declaration_list_name_tagalong_declaration(
        declaration_list *the_declaration_list, size_t name_number);
extern lepton_key_declaration *declaration_list_name_lepton_key_declaration(
        declaration_list *the_declaration_list, size_t name_number);
extern quark_declaration *declaration_list_name_quark_declaration(
        declaration_list *the_declaration_list, size_t name_number);
extern lock_declaration *declaration_list_name_lock_declaration(
        declaration_list *the_declaration_list, size_t name_number);
extern statement *declaration_list_name_jump_target_declaration(
        declaration_list *the_declaration_list, size_t name_number);
extern size_t declaration_list_lookup_name(
        declaration_list *the_declaration_list, const char *name);

extern size_t declaration_list_declaration_count(
        declaration_list *the_declaration_list);
extern declaration *declaration_list_declaration_by_number(
        declaration_list *the_declaration_list, size_t number);

extern verdict declaration_list_append_declaration(
        declaration_list *the_declaration_list, declaration *the_declaration,
        boolean include_in_name_lookup);
extern verdict declaration_list_append_jump_target_declaration(
        declaration_list *the_declaration_list,
        statement *the_jump_target_declaration);

extern const char *name_kind_name(name_kind kind);


#endif /* DECLARATION_LIST_H */
