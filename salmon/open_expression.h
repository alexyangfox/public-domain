/* file "open_expression.h" */

/*
 *  This file contains the interface to the open_expression module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef OPEN_EXPRESSION_H
#define OPEN_EXPRESSION_H


typedef struct open_expression open_expression;


#include "expression.h"
#include "unbound.h"


extern open_expression *create_open_expression(expression *the_expression,
        unbound_name_manager *the_unbound_name_manager);

extern void delete_open_expression(open_expression *the_open_expression);

extern expression *open_expression_expression(
        open_expression *the_open_expression);
extern unbound_name_manager *open_expression_unbound_name_manager(
        open_expression *the_open_expression);

extern void set_open_expression_expression(
        open_expression *the_open_expression, expression *the_expression);
extern void set_open_expression_unbound_name_manager(
        open_expression *the_open_expression,
        unbound_name_manager *the_unbound_name_manager);

extern void decompose_open_expression(open_expression *the_open_expression,
        unbound_name_manager **manager, expression **the_expression);


#endif /* OPEN_EXPRESSION_H */
