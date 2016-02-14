/* file "formal_arguments.h" */

/*
 *  This file contains the interface to the formal_arguments module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef FORMAL_ARGUMENTS_H
#define FORMAL_ARGUMENTS_H

#include <stddef.h>


typedef struct formal_arguments formal_arguments;


#include "c_foundations/basic.h"
#include "declaration.h"
#include "variable_declaration.h"


extern formal_arguments *create_formal_arguments(void);

extern void delete_formal_arguments(formal_arguments *the_formal_arguments);

extern size_t formal_arguments_argument_count(
        formal_arguments *the_formal_arguments);
extern variable_declaration *formal_arguments_formal_by_number(
        formal_arguments *the_formal_arguments, size_t number);
extern type_expression *formal_arguments_dynamic_type_by_number(
        formal_arguments *the_formal_arguments, size_t number);

extern verdict add_formal_parameter(formal_arguments *the_formal_arguments,
        declaration *new_formal, type_expression *dynamic_type);


#endif /* FORMAL_ARGUMENTS_H */
