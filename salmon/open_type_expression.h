/* file "open_type_expression.h" */

/*
 *  This file contains the interface to the open_type_expression module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef OPEN_TYPE_EXPRESSION_H
#define OPEN_TYPE_EXPRESSION_H


typedef struct open_type_expression open_type_expression;


#include "type_expression.h"
#include "unbound.h"


extern open_type_expression *create_open_type_expression(
        type_expression *the_type_expression,
        unbound_name_manager *the_unbound_name_manager);

extern void delete_open_type_expression(
        open_type_expression *the_open_type_expression);

extern type_expression *open_type_expression_type_expression(
        open_type_expression *the_open_type_expression);
extern unbound_name_manager *open_type_expression_unbound_name_manager(
        open_type_expression *the_open_type_expression);

extern void set_open_type_expression_type_expression(
        open_type_expression *the_open_type_expression,
        type_expression *the_type_expression);
extern void set_open_type_expression_unbound_name_manager(
        open_type_expression *the_open_type_expression,
        unbound_name_manager *the_unbound_name_manager);


#endif /* OPEN_TYPE_EXPRESSION_H */
