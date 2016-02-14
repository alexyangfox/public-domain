/* file "statement.h" */

/*
 *  This file contains the interface to the statement module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef STATEMENT_H
#define STATEMENT_H


typedef enum statement_kind
  {
    SK_ASSIGN,
    SK_INCREMENT,
    SK_DECREMENT,
    SK_CALL,
    SK_DECLARATION,
    SK_IF,
    SK_SWITCH,
    SK_GOTO,
    SK_RETURN,
    SK_FOR,
    SK_ITERATE,
    SK_WHILE,
    SK_DO_WHILE,
    SK_BREAK,
    SK_CONTINUE,
    SK_LABEL,
    SK_STATEMENT_BLOCK,
    SK_SINGLE,
    SK_TRY_CATCH,
    SK_TRY_HANDLE,
    SK_CLEANUP,
    SK_EXPORT,
    SK_HIDE,
    SK_USE,
    SK_INCLUDE,
    SK_THEOREM,
    SK_ALIAS
  } statement_kind;

typedef enum assignment_kind
  {
    AK_SIMPLE,
    AK_MODULO,
    AK_MULTIPLY,
    AK_DIVIDE,
    AK_DIVIDE_FORCE,
    AK_REMAINDER,
    AK_ADD,
    AK_SUBTRACT,
    AK_SHIFT_LEFT,
    AK_SHIFT_RIGHT,
    AK_BITWISE_AND,
    AK_BITWISE_XOR,
    AK_BITWISE_OR,
    AK_LOGICAL_AND,
    AK_LOGICAL_OR,
    AK_CONCATENATE
  } assignment_kind;

typedef struct statement statement;


#include "basket.h"
#include "call.h"
#include "source_location.h"
#include "declaration.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "lock_declaration.h"
#include "type_expression.h"
#include "statement_block.h"
#include "routine_declaration_chain.h"


extern statement *create_assign_statement(basket *the_basket,
        expression *the_expression, assignment_kind kind);
extern statement *create_increment_statement(basket *the_basket);
extern statement *create_decrement_statement(basket *the_basket);
extern statement *create_call_statement(call *the_call);
extern statement *create_declaration_statement(void);
extern statement *create_if_statement(expression *test, statement_block *body,
                                      statement_block *else_body);
extern statement *create_switch_statement(expression *base);
extern statement *create_goto_statement(expression *target);
extern statement *create_return_statement(expression *return_value);
extern statement *create_for_statement(const char *index_name,
        expression *init, expression *test, expression *step,
        statement_block *body, boolean is_parallel,
        const source_location *index_declaration_location);
extern statement *create_iterate_statement(const char *element_name,
        expression *base, expression *filter, statement_block *body,
        boolean is_parallel,
        const source_location *element_declaration_location);
extern statement *create_while_statement(expression *test,
        statement_block *body, statement_block *step);
extern statement *create_do_while_statement(expression *test,
        statement_block *body, statement_block *step);
extern statement *create_break_statement(void);
extern statement *create_continue_statement(void);
extern statement *create_label_statement(const char *label_name);
extern statement *create_statement_block_statement(statement_block *block);
extern statement *create_single_statement(expression *lock,
        statement_block *block, declaration *lock_declaration);
extern statement *create_try_catch_statement(statement_block *body,
                                             statement_block *catcher);
extern statement *create_try_handle_statement(statement_block *body,
                                              expression *handler);
extern statement *create_cleanup_statement(statement_block *body);
extern statement *create_export_statement(void);
extern statement *create_hide_statement(void);
extern statement *create_use_statement(expression *to_use,
        type_expression *specified_type, const char *name);
extern statement *create_include_statement(const char *include_file);
extern statement *create_theorem_statement(expression *claim);
extern statement *create_alias_statement(char *alias, char *target);

extern void delete_statement(statement *the_statement);

extern statement_kind get_statement_kind(statement *the_statement);
extern routine_declaration_chain *statement_overload_chain(
        statement *the_statement);
extern statement *statement_overload_use_statement(statement *the_statement);
extern size_t statement_overload_use_used_for_number(statement *the_statement);
extern basket *assign_statement_basket(statement *assign_statement);
extern expression *assign_statement_expression(statement *assign_statement);
extern assignment_kind assign_statement_assignment_kind(
        statement *assign_statement);
extern basket *increment_statement_basket(statement *the_statement);
extern basket *decrement_statement_basket(statement *the_statement);
extern call *call_statement_call(statement *the_statement);
extern size_t declaration_statement_declaration_count(
        statement *the_statement);
extern declaration *declaration_statement_declaration(statement *the_statement,
        size_t declaration_number);
extern expression *if_statement_test(statement *the_statement);
extern statement_block *if_statement_body(statement *the_statement);
extern size_t if_statement_else_if_count(statement *the_statement);
extern expression *if_statement_else_if_test(statement *the_statement,
                                             size_t else_if_number);
extern statement_block *if_statement_else_if_body(statement *the_statement,
                                                  size_t else_if_number);
extern statement_block *if_statement_else_body(statement *the_statement);
extern expression *switch_statement_base(statement *the_statement);
extern size_t switch_statement_case_count(statement *the_statement);
extern type_expression *switch_statement_case_type(statement *the_statement,
                                                   size_t case_number);
extern statement_block *switch_statement_case_block(statement *the_statement,
                                                    size_t case_number);
extern expression *goto_statement_target(statement *the_statement);
extern expression *return_statement_return_value(statement *the_statement);
extern routine_declaration *return_statement_from_routine(
        statement *the_statement);
extern expression *return_statement_from_block_expression(
        statement *the_statement);
extern variable_declaration *for_statement_index(statement *the_statement);
extern expression *for_statement_init(statement *the_statement);
extern expression *for_statement_test(statement *the_statement);
extern expression *for_statement_step(statement *the_statement);
extern statement_block *for_statement_body(statement *the_statement);
extern boolean for_statement_is_parallel(statement *the_statement);
extern variable_declaration *iterate_statement_element(
        statement *the_statement);
extern expression *iterate_statement_base(statement *the_statement);
extern expression *iterate_statement_filter(statement *the_statement);
extern statement_block *iterate_statement_body(statement *the_statement);
extern boolean iterate_statement_is_parallel(statement *the_statement);
extern expression *while_statement_test(statement *the_statement);
extern statement_block *while_statement_body(statement *the_statement);
extern statement_block *while_statement_step(statement *the_statement);
extern expression *do_while_statement_test(statement *the_statement);
extern statement_block *do_while_statement_body(statement *the_statement);
extern statement_block *do_while_statement_step(statement *the_statement);
extern void *break_statement_from(statement *the_statement);
extern void *continue_statement_with(statement *the_statement);
extern const char *label_statement_name(statement *the_statement);
extern statement_block *statement_block_statement_block(
        statement *the_statement);
extern expression *single_statement_lock(statement *the_statement);
extern statement_block *single_statement_block(statement *the_statement);
extern lock_declaration *single_statement_lock_declaration(
        statement *the_statement);
extern value *single_statement_lock_value(statement *the_statement);
extern statement_block *try_catch_statement_body(statement *the_statement);
extern size_t try_catch_statement_tagged_catcher_count(
        statement *the_statement);
extern variable_declaration *try_catch_statement_tagged_catcher_exception(
        statement *the_statement, size_t tagged_catcher_number);
extern type_expression *try_catch_statement_tagged_catcher_tag_type(
        statement *the_statement, size_t tagged_catcher_number);
extern statement_block *try_catch_statement_tagged_catcher_catcher(
        statement *the_statement, size_t tagged_catcher_number);
extern statement_block *try_catch_statement_catcher(statement *the_statement);
extern statement_block *try_handle_statement_body(statement *the_statement);
extern expression *try_handle_statement_handler(statement *the_statement);
extern statement_block *cleanup_statement_body(statement *the_statement);
extern routine_declaration *export_statement_from_routine(
        statement *the_statement);
extern size_t export_statement_item_count(statement *the_statement);
extern const char *export_statement_item_exported_as(statement *the_statement,
                                                     size_t item_num);
extern expression *export_statement_item_to_export(statement *the_statement,
                                                   size_t item_num);
extern routine_declaration *hide_statement_from_routine(
        statement *the_statement);
extern size_t hide_statement_item_count(statement *the_statement);
extern const char *hide_statement_item_to_hide(statement *the_statement,
                                               size_t item_num);
extern expression *use_statement_to_use(statement *the_statement);
extern boolean use_statement_named(statement *the_statement);
extern variable_declaration *use_statement_container(statement *the_statement);
extern size_t use_statement_for_item_count(statement *the_statement);
extern const char *use_statement_for_item_exported_as(statement *the_statement,
                                                      size_t item_num);
extern const char *use_statement_for_item_to_export(statement *the_statement,
                                                    size_t item_num);
extern boolean use_statement_is_exception(statement *the_statement,
                                          const char *name);
extern statement_block *use_statement_parent(statement *the_statement);
extern size_t use_statement_parent_index(statement *the_statement);
extern size_t use_statement_used_for_count(statement *the_statement);
extern const char *use_statement_used_for_name(statement *the_statement,
                                               size_t used_for_number);
extern boolean use_statement_used_for_required(statement *the_statement,
                                               size_t used_for_number);
extern boolean use_statement_used_for_flow_through_allowed(
        statement *the_statement, size_t used_for_number);
extern declaration *use_statement_used_for_declaration(
        statement *the_statement, size_t used_for_number);
extern routine_declaration_chain *use_statement_used_for_chain(
        statement *the_statement, size_t used_for_number);
extern statement *use_statement_used_for_label_statement(
        statement *the_statement, size_t used_for_number);
extern statement *use_statement_used_for_next_use(statement *the_statement,
                                                  size_t used_for_number);
extern size_t use_statement_used_for_next_used_for_number(
        statement *the_statement, size_t used_for_number);
extern source_location *use_statement_used_for_ultimate_use_location(
        statement *the_statement, size_t used_for_number);
extern const char *include_statement_file_name(statement *the_statement);
extern expression *theorem_statement_claim(statement *the_statement);
extern const char *alias_statement_alias(statement *the_statement);
extern const char *alias_statement_target(statement *the_statement);

extern void set_statement_start_location(statement *the_statement,
                                         const source_location *location);
extern void set_statement_end_location(statement *the_statement,
                                       const source_location *location);
extern void statement_set_overload_chain(statement *the_statement,
        routine_declaration_chain *overload_chain);
extern void statement_set_overload_use(statement *the_statement,
        statement *overload_use_statement, size_t overload_used_for_number);
extern verdict declaration_statement_add_declaration(statement *the_statement,
        declaration *the_declaration);
extern verdict add_if_statement_else_if(statement *the_statement,
        expression *test, statement_block *body);
extern void add_if_statement_else(statement *the_statement,
                                  statement_block *body);
extern verdict add_switch_statement_case(statement *the_statement,
        type_expression *type, statement_block *block);
extern verdict bind_return_statement_to_routine_declaration(
        statement *return_statement, routine_declaration *declaration);
extern verdict bind_return_statement_to_block_expression(
        statement *return_statement, expression *the_expression);
extern verdict bind_export_statement_to_from_declaration(
        statement *export_statement, routine_declaration *declaration);
extern verdict bind_hide_statement_to_from_declaration(
        statement *hide_statement, routine_declaration *declaration);
extern verdict bind_break_statement_from(statement *break_statement,
                                         void *from);
extern verdict bind_continue_statement_with(statement *continue_statement,
                                            void *with);
extern void single_statement_set_lock_value(statement *the_statement,
                                            value *the_value);
extern verdict add_try_catch_statement_tagged_catcher(statement *the_statement,
        const char *exception_name, type_expression *tag_type,
        statement_block *catcher,
        const source_location *exception_declaration_location);
extern void add_try_catch_statement_catcher(statement *the_statement,
                                            statement_block *catcher);
extern verdict export_statement_add_item(statement *export_statement,
        const char *exported_as, expression *to_export);
extern verdict hide_statement_add_item(statement *hide_statement,
                                       const char *to_hide);
extern verdict use_statement_add_for_item(statement *use_statement,
        const char *exported_as, const char *to_export);
extern verdict use_statement_add_except_item(statement *use_statement,
                                             const char *to_hide);
extern void use_statement_set_parent(statement *use_statement,
        statement_block *parent, size_t parent_index);
extern verdict use_statement_add_used_for_case(statement *use_statement,
        const char *name, boolean flow_through_allowed,
        const source_location *ultimate_use_location);
extern void use_statement_set_used_for_required(statement *the_statement,
        size_t used_for_number, boolean required);
extern void use_statement_set_used_for_declaration(statement *the_statement,
        size_t used_for_number, declaration *the_declaration);
extern void use_statement_set_used_for_chain(statement *the_statement,
        size_t used_for_number, routine_declaration_chain *chain);
extern void use_statement_set_used_for_label_statement(
        statement *the_statement, size_t used_for_number,
        statement *label_statement);
extern void use_statement_set_used_for_next_used(statement *the_statement,
        size_t used_for_number, statement *next_statement,
        size_t next_used_for_number);

extern const source_location *get_statement_location(statement *the_statement);

extern void statement_error(statement *the_statement, const char *format, ...);
extern void vstatement_error(statement *the_statement, const char *format,
                             va_list arg);

extern expression_kind binary_expression_kind_for_assignment(
        assignment_kind the_assignment_kind);


#endif /* STATEMENT_H */
