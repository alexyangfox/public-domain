/* file "parser.h" */

/*
 *  This file contains the interface to the parser module, which uses the token
 *  output from the tokenizer module to build a parse tree.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef PARSER_H
#define PARSER_H

#include "c_foundations/basic.h"


typedef struct parser parser;


#include "precedence.h"
#include "tokenizer.h"
#include "open_expression.h"
#include "open_basket.h"
#include "open_type_expression.h"
#include "open_call.h"
#include "open_statement.h"
#include "open_statement_block.h"
#include "statement_block.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "tagalong_declaration.h"
#include "lepton_key_declaration.h"
#include "quark_declaration.h"
#include "lock_declaration.h"
#include "declaration.h"
#include "native_bridge.h"
#include "unbound.h"
#include "include.h"


extern parser *create_parser(tokenizer *the_tokenizer,
        include_handler_type include_handler,
        interface_include_handler_type interface_include_handler,
        void *include_handler_data, alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed);
extern void delete_parser(parser *the_parser);

extern open_expression *parse_expression(parser *the_parser,
        expression_parsing_precedence precedence,
        unbound_use **naked_call_overloading_use, boolean stop_with_call);
extern open_expression *parse_expression_with_end_location(parser *the_parser,
        expression_parsing_precedence precedence,
        unbound_use **naked_call_overloading_use, boolean stop_with_call,
        source_location *end_location);
extern open_expression *parse_lookup_expression_tail(parser *the_parser,
        expression *base, unbound_name_manager *manager);
extern open_expression *parse_lepton_expression_tail(parser *the_parser,
        expression *base, unbound_name_manager *manager);
extern open_expression *parse_routine_or_new_expression(parser *the_parser,
        expression *single_lock, unbound_name_manager *single_lock_manager);
extern open_expression *parse_construct_expression(parser *the_parser,
        expression *single_lock, unbound_name_manager *single_lock_manager);
extern open_expression *parse_semi_labeled_expression_list_expression(
        parser *the_parser);
extern open_basket *parse_basket(parser *the_parser);
extern open_basket *parse_expression_basket(parser *the_parser);
extern open_basket *parse_list_basket(parser *the_parser);
extern open_type_expression *parse_type_expression(parser *the_parser,
        type_expression_parsing_precedence precedence);
extern open_type_expression *parse_type_expression_with_end_location(
        parser *the_parser, type_expression_parsing_precedence precedence,
        source_location *end_location);
extern verdict parse_interface_item_list(parser *the_parser,
        type_expression *interface_type_expression,
        unbound_name_manager **manager, token_kind expected_finish);
extern open_type_expression *parse_range_type_expression(parser *the_parser);
extern open_type_expression *parse_semi_labeled_value_list_type_expression(
        parser *the_parser);
extern verdict parse_formal_type_list(parser *the_parser,
        token_kind left_bound_kind, token_kind right_bound_kind,
        unbound_name_manager **manager, void *data,
        verdict (*set_extra_allowed)(void *data),
        verdict (*set_extra_unspecified)(void *data, boolean *error),
        verdict (*add_formal)(void *data, const char *name,
                type_expression *formal_type, boolean has_default),
        source_location *end_location);
extern verdict parse_field_type_list(parser *the_parser,
        token_kind left_bound_kind, token_kind right_bound_kind,
        unbound_name_manager **manager, type_expression *base_type,
        verdict (*set_extra_allowed)(type_expression *base_type),
        verdict (*add_field)(type_expression *base_type, const char *name,
                             type_expression *field_type));
extern open_call *parse_call_suffix(parser *the_parser, expression *base,
                                    unbound_name_manager *base_manager);
extern open_statement *parse_statement(parser *the_parser,
        const char **current_labels, size_t current_label_count);
extern open_statement *parse_routine_or_variable_statement(parser *the_parser,
        expression *single_lock, unbound_name_manager *single_lock_manager,
        native_bridge_routine *native_handler,
        purity_safety the_purity_safety);
extern open_statement *parse_cleanup_statement(parser *the_parser);
extern open_statement *parse_if_statement(parser *the_parser);
extern open_statement *parse_switch_statement(parser *the_parser);
extern open_statement *parse_goto_statement(parser *the_parser);
extern open_statement *parse_return_statement(parser *the_parser);
extern open_statement *parse_for_statement(parser *the_parser,
        boolean is_parallel, const char **current_labels,
        size_t current_label_count);
extern open_statement *parse_iterate_statement(parser *the_parser,
        boolean is_parallel, const char **current_labels,
        size_t current_label_count);
extern open_statement *parse_while_statement(parser *the_parser,
        const char **current_labels, size_t current_label_count);
extern open_statement *parse_do_while_statement(parser *the_parser,
        const char **current_labels, size_t current_label_count);
extern open_statement *parse_break_statement(parser *the_parser);
extern open_statement *parse_continue_statement(parser *the_parser);
extern open_statement *parse_export_statement(parser *the_parser);
extern open_statement *parse_hide_statement(parser *the_parser);
extern open_statement *parse_use_statement(parser *the_parser);
extern open_statement *parse_include_statement(parser *the_parser);
extern open_statement *parse_theorem_statement(parser *the_parser);
extern open_statement *parse_alias_statement(parser *the_parser);
extern open_statement *parse_assignment_statement(parser *the_parser);
extern open_statement *parse_assignment_statement_suffix(parser *the_parser,
        basket *the_basket, unbound_name_manager *manager);
extern open_statement *parse_increment_statement(parser *the_parser);
extern open_statement *parse_decrement_statement(parser *the_parser);
extern open_statement *parse_label_statement(parser *the_parser);
extern open_statement *parse_statement_block_statement(parser *the_parser);
extern open_statement *parse_try_statement(parser *the_parser);
extern open_statement_block *parse_statement_list(parser *the_parser);
extern open_statement_block *parse_statement_list_through_function(
        open_statement *(*statement_parser)(void *data,
                const char **current_labels, size_t current_label_count),
        boolean (*done)(void *data), void *data,
        include_handler_type include_handler, void *include_handler_data,
        alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed);
extern verdict parse_statements_for_statement_block_through_function(
        open_statement *(*statement_parser)(void *data,
                const char **current_labels, size_t current_label_count),
        boolean (*done)(void *data), void *data,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        include_handler_type include_handler, void *include_handler_data,
        alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed);
extern open_statement_block *parse_statement_block(parser *the_parser);
extern open_statement_block *parse_braced_statement_block(parser *the_parser);
extern verdict parse_statements_for_statement_block(parser *the_parser,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels);
extern declaration *parse_routine_declaration(parser *the_parser,
        const char *opening, boolean return_ok, boolean return_required,
        boolean is_class, boolean is_static, boolean is_ageless,
        boolean is_virtual, boolean is_pure, expression *single_lock,
        unbound_name_manager *single_lock_manager,
        unbound_name_manager **result_manager,
        native_bridge_routine *native_handler, purity_safety the_purity_safety,
        const source_location *start_location);
extern declaration *parse_variable_declaration(parser *the_parser,
        boolean immutable, boolean is_static, boolean is_ageless,
        boolean is_virtual, expression *single_lock,
        unbound_name_manager *single_lock_manager,
        unbound_name_manager **result_manager, type_expression **dynamic_type,
        unbound_name_manager **dynamic_type_manager);
extern declaration *parse_tagalong_declaration(parser *the_parser,
        boolean is_static, boolean is_ageless, boolean is_virtual,
        expression *single_lock, unbound_name_manager *single_lock_manager,
        unbound_name_manager **result_manager);
extern declaration *parse_lepton_key_declaration(parser *the_parser,
        boolean is_static, boolean is_ageless, boolean is_virtual,
        unbound_name_manager **result_manager);
extern declaration *parse_quark_declaration(parser *the_parser,
        boolean is_static, boolean is_ageless, boolean is_virtual,
        unbound_name_manager **result_manager);
extern declaration *parse_lock_declaration(parser *the_parser,
        boolean is_static, boolean is_ageless, boolean is_virtual,
        expression *single_lock, unbound_name_manager *single_lock_manager,
        unbound_name_manager **result_manager);
extern declaration *parse_declaration(parser *the_parser, boolean is_static,
        boolean is_ageless, boolean is_virtual, boolean is_pure,
        expression *single_lock, unbound_name_manager *single_lock_manager,
        unbound_name_manager **result_manager, boolean is_formal,
        boolean is_dynamic, native_bridge_routine *native_handler,
        purity_safety the_purity_safety,
        verdict (*add_declaration)(void *data, declaration *new_declaration),
        void *add_declaration_data, type_expression **dynamic_type,
        unbound_name_manager **dynamic_type_manager);
extern verdict verify_end_of_input(parser *the_parser);

#define parse_stand_alone_immediate_expression(text) \
        parse_stand_alone_immediate_expression_with_source_info(text, \
                __FILE__, __LINE__)
extern expression *parse_stand_alone_immediate_expression_with_source_info(
        const char *text, const char *source_file_name,
        size_t source_line_number);


#endif /* PARSER_H */
