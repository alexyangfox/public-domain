/* file "native_bridge.h" */

/*
 *  This file contains the interface to the native_bridge module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include "value.h"
#include "jumper.h"
#include "unbound.h"
#include "include.h"


#ifndef NATIVE_BRIDGE_H
#define NATIVE_BRIDGE_H


typedef value *native_bridge_routine(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);

typedef enum { PURE_SAFE, PURE_UNSAFE } purity_safety;

typedef struct
  {
    unsigned long line;
    const char *declaration;
    native_bridge_routine *handler;
    purity_safety purity_safety;
  } native_bridge_function_info;


#define ONE_ROUTINE(declaration, handler, purity_safety) \
        { __LINE__, declaration, handler, purity_safety }
#define ONE_STATEMENT(statement)  { __LINE__, statement, NULL, PURE_SAFE }


extern declaration *create_native_bridge_class_declaration(
        const native_bridge_function_info *function_table,
        size_t table_entry_count, const char *source_file_name);
extern verdict parse_statements_for_statement_block_from_native_bridge(
        const native_bridge_function_info *function_table,
        size_t table_entry_count, const char *source_file_name,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        alias_manager *parent_alias_manager);


#endif /* NATIVE_BRIDGE_H */
