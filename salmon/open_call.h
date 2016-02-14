/* file "open_call.h" */

/*
 *  This file contains the interface to the open_call module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef OPEN_CALL_H
#define OPEN_CALL_H


typedef struct open_call open_call;


#include "call.h"
#include "unbound.h"


extern open_call *create_open_call(call *the_call,
        unbound_name_manager *the_unbound_name_manager);

extern void delete_open_call(open_call *the_open_call);

extern call *open_call_call(open_call *the_open_call);
extern unbound_name_manager *open_call_unbound_name_manager(
        open_call *the_open_call);

extern void set_open_call_call(open_call *the_open_call, call *the_call);
extern void set_open_call_unbound_name_manager(open_call *the_open_call,
        unbound_name_manager *the_unbound_name_manager);


#endif /* OPEN_CALL_H */
