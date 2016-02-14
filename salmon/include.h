/* file "include.h" */

/*
 *  This file contains the interface to the include module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef INCLUDE_H
#define INCLUDE_H


typedef struct alias_manager alias_manager;


#include "c_foundations/basic.h"
#include "c_foundations/auto_array.h"


AUTO_ARRAY(string_aa, const char *);


#include "statement_block.h"
#include "unbound.h"
#include "source_location.h"
#include "platform_dependent.h"


typedef verdict (*include_handler_type)(void *data, const char *to_include,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        const source_location *location, alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed);
typedef verdict (*interface_include_handler_type)(void *data,
        const char *to_include, type_expression *interface_type_expression,
        unbound_name_manager **manager, const source_location *location,
        alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed);


extern verdict init_include_module(void);

extern void *create_local_file_include_handler_data(const char *base_file_name,
        const char *directory_paths, const char *executable_directory);
extern verdict local_file_include_handler(void *data, const char *to_include,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        const source_location *location, alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed);
extern verdict local_file_interface_include_handler(void *data,
        const char *to_include, type_expression *interface_type_expression,
        unbound_name_manager **manager, const source_location *location,
        alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed);
extern dynamic_library_handle *open_dynamic_library_in_path(void *data,
        const char *to_include, boolean *file_not_found, char **error_message);
extern void delete_local_file_include_handler_data(void *data);

extern void cleanup_include_module(void);


#endif /* INCLUDE_H */
