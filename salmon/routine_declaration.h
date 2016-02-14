/* file "routine_declaration.h" */

/*
 *  This file contains the interface to the routine_declaration module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef ROUTINE_DECLARATION_H
#define ROUTINE_DECLARATION_H

#include "c_foundations/basic.h"


typedef struct routine_declaration routine_declaration;


#include "type_expression.h"
#include "formal_arguments.h"
#include "statement_block.h"
#include "expression.h"
#include "routine_declaration_chain.h"
#include "native_bridge.h"
#include "declaration.h"
#include "static_home.h"
#include "platform_dependent.h"
#include "thread.h"


extern routine_declaration *create_routine_declaration(
        type_expression *static_return_type,
        type_expression *dynamic_return_type,
        formal_arguments *the_formal_arguments,
        boolean extra_arguments_allowed, statement_block *body,
        native_bridge_routine *native_handler, purity_safety the_purity_safety,
        boolean is_pure, boolean is_class, expression *single_lock,
        size_t static_count, declaration **static_declarations);

extern void routine_declaration_add_reference(
        routine_declaration *declaration);
extern void routine_declaration_remove_reference(
        routine_declaration *declaration);
extern void delete_routine_declaration(routine_declaration *declaration);

extern declaration *routine_declaration_declaration(
        routine_declaration *declaration);
extern const char *routine_declaration_name(routine_declaration *declaration);
extern type_expression *routine_declaration_static_return_type(
        routine_declaration *declaration);
extern type_expression *routine_declaration_dynamic_return_type(
        routine_declaration *declaration);
extern formal_arguments *routine_declaration_formals(
        routine_declaration *declaration);
extern boolean routine_declaration_extra_arguments_allowed(
        routine_declaration *declaration);
extern statement_block *routine_declaration_body(
        routine_declaration *declaration);
extern native_bridge_routine *routine_declaration_native_handler(
        routine_declaration *declaration);
extern purity_safety routine_declaration_purity_safety(
        routine_declaration *declaration);
extern boolean routine_declaration_is_static(routine_declaration *declaration);
extern boolean routine_declaration_is_virtual(
        routine_declaration *declaration);
extern boolean routine_declaration_is_pure(routine_declaration *declaration);
extern boolean routine_declaration_is_class(routine_declaration *declaration);
extern expression *routine_declaration_single_lock(
        routine_declaration *declaration);
extern boolean routine_declaration_automatic_allocation(
        routine_declaration *declaration);
extern routine_declaration_chain *routine_declaration_declaration_chain(
        routine_declaration *declaration);
extern static_home *routine_declaration_static_home(
        routine_declaration *declaration);

extern void set_routine_declaration_declaration(
        routine_declaration *the_routine_declaration,
        declaration *the_declaration);

extern verdict routine_declaration_record_call(
        routine_declaration *the_routine_declaration, CLOCK_T net_time,
        CLOCK_T local_time);
extern o_integer routine_declaration_call_count(
        routine_declaration *the_routine_declaration);
extern CLOCK_T routine_declaration_net_time(
        routine_declaration *the_routine_declaration);
extern CLOCK_T routine_declaration_local_time(
        routine_declaration *the_routine_declaration);
extern boolean routine_declaration_in_call_on_thread(
        routine_declaration *the_routine_declaration, salmon_thread *thread);
extern verdict routine_declaration_start_in_call_on_thread(
        routine_declaration *the_routine_declaration, salmon_thread *thread);
extern void routine_declaration_end_in_call_on_thread(
        routine_declaration *the_routine_declaration, salmon_thread *thread);


#endif /* ROUTINE_DECLARATION_H */
