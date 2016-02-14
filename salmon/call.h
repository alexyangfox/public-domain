/* file "call.h" */

/*
 *  This file contains the interface to the call module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef CALL_H
#define CALL_H

#include "c_foundations/basic.h"


typedef struct call call;


#include "expression.h"
#include "source_location.h"


extern call *create_call(expression *base, size_t actual_argument_count,
        expression **actual_argument_expressions,
        const char **formal_argument_names, const source_location *location);

extern void delete_call(call *the_call);

extern expression *call_base(call *the_call);
extern size_t call_actual_argument_count(call *the_call);
extern expression *call_actual_argument_expression(call *the_call,
                                                   size_t argument_num);
extern const char *call_formal_argument_name(call *the_call,
                                             size_t argument_num);

extern verdict append_argument_to_call(call *the_call,
        expression *actual_argument_expression,
        const char *formal_argument_name);

extern void set_call_start_location(call *the_call,
                                    const source_location *location);
extern void set_call_end_location(call *the_call,
                                  const source_location *location);

extern const source_location *get_call_location(call *the_call);


#endif /* CALL_H */
