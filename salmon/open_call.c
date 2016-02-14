/* file "open_call.c" */

/*
 *  This file contains the implementation of the open_call module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include "c_foundations/memory_allocation.h"
#include "open_call.h"
#include "call.h"
#include "unbound.h"


struct open_call
  {
    call *the_call;
    unbound_name_manager *the_unbound_name_manager;
  };


extern open_call *create_open_call(call *the_call,
        unbound_name_manager *the_unbound_name_manager)
  {
    open_call *result;

    result = MALLOC_ONE_OBJECT(open_call);
    if (result == NULL)
      {
        if (the_call != NULL)
            delete_call(the_call);
        if (the_unbound_name_manager != NULL)
            delete_unbound_name_manager(the_unbound_name_manager);
        return NULL;
      }

    result->the_call = the_call;
    result->the_unbound_name_manager = the_unbound_name_manager;

    return result;
  }

extern void delete_open_call(open_call *the_open_call)
  {
    assert(the_open_call != NULL);

    if (the_open_call->the_call != NULL)
        delete_call(the_open_call->the_call);

    if (the_open_call->the_unbound_name_manager != NULL)
        delete_unbound_name_manager(the_open_call->the_unbound_name_manager);

    free(the_open_call);
  }

extern call *open_call_call(open_call *the_open_call)
  {
    assert(the_open_call != NULL);

    return the_open_call->the_call;
  }

extern unbound_name_manager *open_call_unbound_name_manager(
        open_call *the_open_call)
  {
    assert(the_open_call != NULL);

    return the_open_call->the_unbound_name_manager;
  }

extern void set_open_call_call(open_call *the_open_call, call *the_call)
  {
    assert(the_open_call != NULL);

    the_open_call->the_call = the_call;
  }

extern void set_open_call_unbound_name_manager(open_call *the_open_call,
        unbound_name_manager *the_unbound_name_manager)
  {
    assert(the_open_call != NULL);

    the_open_call->the_unbound_name_manager = the_unbound_name_manager;
  }
