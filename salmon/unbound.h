/* file "unbound.h" */

/*
 *  This file contains the interface to the unbound module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef UNBOUND_H
#define UNBOUND_H

#include <stddef.h>
#include "c_foundations/basic.h"


typedef enum unbound_use_kind
  {
    UUK_ROUTINE_FOR_RETURN_STATEMENT,
    UUK_ROUTINE_FOR_EXPORT_STATEMENT,
    UUK_ROUTINE_FOR_HIDE_STATEMENT,
    UUK_ROUTINE_FOR_ARGUMENTS_EXPRESSION,
    UUK_ROUTINE_FOR_THIS_EXPRESSION,
    UUK_ROUTINE_FOR_OPERATOR_EXPRESSION,
    UUK_ROUTINE_FOR_OPERATOR_STATEMENT,
    UUK_LOOP_FOR_BREAK_STATEMENT,
    UUK_LOOP_FOR_CONTINUE_STATEMENT,
    UUK_LOOP_FOR_BREAK_EXPRESSION,
    UUK_LOOP_FOR_CONTINUE_EXPRESSION,
    UUK_VARIABLE_FOR_EXPRESSION,
    UUK_DANGLING_OVERLOADED_ROUTINE,
    UUK_USE_FLOW_THROUGH
  } unbound_use_kind;

typedef struct unbound_name_manager unbound_name_manager;
typedef struct unbound_name unbound_name;
typedef struct unbound_use unbound_use;


#include "call.h"
#include "routine_declaration_chain.h"
#include "source_location.h"
#include "statement.h"
#include "declaration.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "tagalong_declaration.h"
#include "lepton_key_declaration.h"
#include "quark_declaration.h"
#include "lock_declaration.h"


extern unbound_name_manager *create_unbound_name_manager(void);

extern void delete_unbound_name_manager(unbound_name_manager *manager);

extern size_t unbound_name_count(unbound_name_manager *manager);
extern unbound_name *unbound_name_number(unbound_name_manager *manager,
                                         size_t name_number);
extern unbound_name *lookup_unbound_name(unbound_name_manager *manager,
                                         const char *name);

extern size_t unbound_name_manager_static_count(unbound_name_manager *manager);
extern declaration **unbound_name_manager_static_declarations(
        unbound_name_manager *manager);

extern unbound_use *add_unbound_return(unbound_name_manager *manager,
        const char *name, statement *return_statement,
        const source_location *location);
extern unbound_use *add_unbound_export(unbound_name_manager *manager,
        const char *name, statement *export_statement,
        const source_location *location);
extern unbound_use *add_unbound_hide(unbound_name_manager *manager,
        const char *name, statement *hide_statement,
        const source_location *location);
extern unbound_use *add_unbound_arguments_expression(
        unbound_name_manager *manager, const char *name,
        expression *arguments_expression, const source_location *location);
extern unbound_use *add_unbound_this_expression(unbound_name_manager *manager,
        const char *name, expression *this_expression,
        const source_location *location);
extern unbound_use *add_unbound_operator_expression(
        unbound_name_manager *manager, const char *name,
        expression *this_expression, const source_location *location);
extern unbound_use *add_unbound_operator_statement(
        unbound_name_manager *manager, const char *name,
        statement *this_statement, const source_location *location);
extern unbound_use *add_unbound_break(unbound_name_manager *manager,
        const char *name, statement *break_statement,
        const source_location *location);
extern unbound_use *add_unbound_continue(unbound_name_manager *manager,
        const char *name, statement *continue_statement,
        const source_location *location);
extern unbound_use *add_unbound_break_expression(unbound_name_manager *manager,
        const char *name, expression *break_expression,
        const source_location *location);
extern unbound_use *add_unbound_continue_expression(
        unbound_name_manager *manager, const char *name,
        expression *continue_expression, const source_location *location);
extern unbound_use *add_unbound_variable_reference(
        unbound_name_manager *manager, const char *name,
        expression *the_expression, const source_location *location);
extern unbound_use *add_dangling_overloaded_routine_reference(
        unbound_name_manager *manager, const char *name,
        routine_declaration_chain *chain, const source_location *location);
extern unbound_use *add_use_flow_through_reference(
        unbound_name_manager *manager, const char *name,
        statement *use_statement, size_t used_for_num,
        const source_location *location);
extern verdict bind_variable_name(unbound_name_manager *manager,
        const char *name, variable_declaration *declaration);
extern verdict bind_routine_name(unbound_name_manager *manager,
        const char *name, routine_declaration_chain *chain);
extern verdict bind_static_routine_name(unbound_name_manager *manager,
        const char *name, routine_declaration_chain *chain);
extern verdict bind_tagalong_name(unbound_name_manager *manager,
        const char *name, tagalong_declaration *declaration);
extern verdict bind_lepton_key_name(unbound_name_manager *manager,
        const char *name, lepton_key_declaration *declaration);
extern verdict bind_quark_name(unbound_name_manager *manager, const char *name,
                               quark_declaration *declaration);
extern verdict bind_lock_name(unbound_name_manager *manager, const char *name,
                              lock_declaration *declaration);
extern verdict bind_label_name(unbound_name_manager *manager, const char *name,
                               statement *declaration);
extern verdict bind_return(unbound_name_manager *manager,
                           routine_declaration *declaration);
extern verdict bind_return_to_block_expression(unbound_name_manager *manager,
                                               expression *block_expression);
extern verdict bind_break_and_continue(unbound_name_manager *manager,
        void *loop_construct, boolean is_parallel, const char **current_labels,
        size_t current_label_count);

extern verdict unbound_name_manager_add_static(unbound_name_manager *manager,
                                               declaration *the_declaration);
extern verdict unbound_name_manager_clear_static_list(
        unbound_name_manager *manager);

extern verdict merge_in_unbound_name_manager(unbound_name_manager *base,
        unbound_name_manager *to_merge_in);
extern verdict merge_in_deferred_unbound_name_manager(
        unbound_name_manager *base, unbound_name_manager *to_merge_in);

extern const char *unbound_name_string(unbound_name *name);
extern size_t unbound_name_use_count(unbound_name *name);
extern unbound_use *unbound_name_use_by_number(unbound_name *name,
                                               size_t use_number);

extern unbound_use_kind get_unbound_use_kind(unbound_use *use);
extern unbound_name *unbound_use_name(unbound_use *use);
extern const source_location *unbound_use_location(unbound_use *use);
extern expression *unbound_use_expression(unbound_use *use);
extern statement *unbound_use_statement(unbound_use *use);
extern routine_declaration_chain *unbound_use_chain(unbound_use *use);
extern statement *unbound_use_use_flow_through_use_statement(unbound_use *use);
extern size_t unbound_use_use_flow_through_used_for_num(unbound_use *use);

extern void remove_unbound_use(unbound_use *use);


#endif /* UNBOUND_H */
