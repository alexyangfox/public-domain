/* file "file_parser.h" */

/*
 *  This file contains the interface to the file_parser module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef FILE_PARSER_H
#define FILE_PARSER_H

#include <stdio.h>


typedef struct file_parser file_parser;

#include "parser.h"
#include "tokenizer.h"
#include "include.h"


extern file_parser *create_file_parser(const char *file_name, FILE *fp,
        const source_location *location, include_handler_type include_handler,
        interface_include_handler_type interface_include_handler,
        void *include_handler_data, alias_manager *parent_alias_manager,
        boolean *cant_open_file, boolean native_bridge_dll_body_allowed);

extern void delete_file_parser(file_parser *the_file_parser);

extern parser *file_parser_parser(file_parser *the_file_parser);
extern tokenizer *file_parser_tokenizer(file_parser *the_file_parser);


#endif /* FILE_PARSER_H */
