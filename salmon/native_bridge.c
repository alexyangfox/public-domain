/* file "native_bridge.c" */

/*
 *  This file contains the implementation of the native_bridge module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <assert.h>
#include "c_foundations/basic.h"
#include "native_bridge.h"
#include "tokenizer.h"
#include "parser.h"
#include "open_statement.h"
#include "statement_block.h"
#include "unbound.h"
#include "bind.h"


typedef struct
  {
    const native_bridge_function_info *statement_table;
    size_t table_size;
    size_t table_position;
    const char *source_file_name;
  } native_bridge_parser_info;


static open_statement *native_bridge_statement_parser(void *data,
        const char **current_labels, size_t current_label_count);
static boolean native_bridge_done_test(void *data);
static verdict native_bridge_include_handler(void *data,
        const char *to_include, statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        const source_location *location, alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed);
static verdict native_bridge_interface_include_handler(void *data,
        const char *to_include, type_expression *interface_type_expression,
        unbound_name_manager **manager, const source_location *location,
        alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed);


extern declaration *create_native_bridge_class_declaration(
        const native_bridge_function_info *function_table,
        size_t table_entry_count, const char *source_file_name)
  {
    native_bridge_parser_info parser_info;
    open_statement_block *open_block;
    unbound_name_manager *name_manager;
    statement_block *the_block;
    formal_arguments *formals;
    routine_declaration *the_routine_declaration;
    verdict the_verdict;
    declaration *the_declaration;

    parser_info.statement_table = function_table;
    parser_info.table_size = table_entry_count;
    parser_info.table_position = 0;
    parser_info.source_file_name = source_file_name;

    open_block = parse_statement_list_through_function(
            &native_bridge_statement_parser, &native_bridge_done_test,
            &parser_info, &native_bridge_include_handler, NULL, NULL, TRUE);

    if (open_block == NULL)
        return NULL;

    formals = create_formal_arguments();
    if (formals == NULL)
      {
        delete_open_statement_block(open_block);
        return NULL;
      }

    decompose_open_statement_block(open_block, &name_manager, &the_block);

    the_routine_declaration = create_routine_declaration(NULL, NULL, formals,
            FALSE, the_block, NULL, PURE_UNSAFE, FALSE, TRUE, NULL,
            unbound_name_manager_static_count(name_manager),
            unbound_name_manager_static_declarations(name_manager));
    if (the_routine_declaration == NULL)
      {
        delete_unbound_name_manager(name_manager);
        return NULL;
      }

    the_verdict = unbound_name_manager_clear_static_list(name_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_routine_declaration(the_routine_declaration);
        delete_unbound_name_manager(name_manager);
        return NULL;
      }

    the_declaration = create_declaration_for_routine("built-ins", FALSE, FALSE,
            TRUE, the_routine_declaration, NULL);
    if (the_declaration == NULL)
      {
        delete_unbound_name_manager(name_manager);
        return NULL;
      }

    the_verdict = bind_return(name_manager, the_routine_declaration);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        declaration_remove_reference(the_declaration);
        delete_unbound_name_manager(name_manager);
        return NULL;
      }

    the_verdict = check_for_unbound(name_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        declaration_remove_reference(the_declaration);
        delete_unbound_name_manager(name_manager);
        return NULL;
      }

    delete_unbound_name_manager(name_manager);

    return the_declaration;
  }

extern verdict parse_statements_for_statement_block_from_native_bridge(
        const native_bridge_function_info *function_table,
        size_t table_entry_count, const char *source_file_name,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        alias_manager *parent_alias_manager)
  {
    native_bridge_parser_info parser_info;

    parser_info.statement_table = function_table;
    parser_info.table_size = table_entry_count;
    parser_info.table_position = 0;
    parser_info.source_file_name = source_file_name;

    return parse_statements_for_statement_block_through_function(
            &native_bridge_statement_parser, &native_bridge_done_test,
            &parser_info, the_statement_block, parent_manager, current_labels,
            &native_bridge_include_handler, NULL, parent_alias_manager, TRUE);
  }


static open_statement *native_bridge_statement_parser(void *data,
        const char **current_labels, size_t current_label_count)
  {
    native_bridge_parser_info *parser_info;
    size_t table_position;
    tokenizer *the_tokenizer;
    size_t line_count;
    const char *follow;
    parser *the_parser;
    native_bridge_routine *handler;
    open_statement *the_open_statement;

    assert(data != NULL);

    parser_info = (native_bridge_parser_info *)data;

    table_position = parser_info->table_position;

    ++(parser_info->table_position);

    assert(table_position < parser_info->table_size);

    the_tokenizer = create_tokenizer(
            parser_info->statement_table[table_position].declaration,
            parser_info->source_file_name);
    if (the_tokenizer == NULL)
        return NULL;

    line_count = 0;
    follow = parser_info->statement_table[table_position].declaration;
    while (*follow != 0)
      {
        if (*follow == '\n')
            ++line_count;
        ++follow;
      }

    set_tokenizer_line_number(the_tokenizer,
            parser_info->statement_table[table_position].line - line_count);

    the_parser = create_parser(the_tokenizer, &native_bridge_include_handler,
            &native_bridge_interface_include_handler, NULL, NULL, TRUE);
    if (the_parser == NULL)
      {
        delete_tokenizer(the_tokenizer);
        return NULL;
      }

    handler = parser_info->statement_table[table_position].handler;

    if (handler != NULL)
      {
        the_open_statement = parse_routine_or_variable_statement(the_parser,
                NULL, NULL, handler,
                parser_info->statement_table[table_position].purity_safety);
      }
    else
      {
        the_open_statement = parse_statement(the_parser, current_labels,
                                             current_label_count);
      }

    delete_parser(the_parser);
    delete_tokenizer(the_tokenizer);

    return the_open_statement;
  }

static boolean native_bridge_done_test(void *data)
  {
    native_bridge_parser_info *parser_info;

    assert(data != NULL);

    parser_info = (native_bridge_parser_info *)data;

    return (parser_info->table_position == parser_info->table_size);
  }

static verdict native_bridge_include_handler(void *data,
        const char *to_include, statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        const source_location *location, alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed)
  {
    assert(data == NULL);

    location_error(location,
            "Attempted to use an include statement within native bridge Salmon"
            " code.");
    return MISSION_FAILED;
  }

static verdict native_bridge_interface_include_handler(void *data,
        const char *to_include, type_expression *interface_type_expression,
        unbound_name_manager **manager, const source_location *location,
        alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed)
  {
    assert(data == NULL);

    location_error(location,
            "Attempted to use an interface include within native bridge Salmon"
            " code.");
    return MISSION_FAILED;
  }
