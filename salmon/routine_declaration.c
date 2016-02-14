/* file "routine_declaration.c" */

/*
 *  This file contains the implementation of the routine_declaration module.
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
#include <string.h>
#include <time.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "routine_declaration.h"
#include "type_expression.h"
#include "formal_arguments.h"
#include "statement_block.h"
#include "expression.h"
#include "routine_declaration_chain.h"
#include "native_bridge.h"
#include "declaration.h"
#include "static_home.h"
#include "platform_dependent.h"


typedef struct in_call_info in_call_info;

struct in_call_info
  {
    salmon_thread *thread;
    size_t count;
    in_call_info *next;
  };

struct routine_declaration
  {
    declaration *declaration;
    type_expression *static_return_type;
    type_expression *dynamic_return_type;
    formal_arguments *the_formal_arguments;
    statement_block *body;
    native_bridge_routine *native_handler;
    purity_safety purity_safety;
    boolean is_pure;
    boolean is_class;
    expression *single_lock;
    size_t static_count;
    routine_declaration_chain *declaration_chain;
    boolean extra_arguments_allowed;
    static_home *static_home;
    o_integer call_count;
    CLOCK_T net_time;
    CLOCK_T local_time;
    DECLARE_SYSTEM_LOCK(call_lock);
    in_call_info *in_calls;
  };


extern routine_declaration *create_routine_declaration(
        type_expression *static_return_type,
        type_expression *dynamic_return_type,
        formal_arguments *the_formal_arguments,
        boolean extra_arguments_allowed, statement_block *body,
        native_bridge_routine *native_handler, purity_safety the_purity_safety,
        boolean is_pure, boolean is_class, expression *single_lock,
        size_t static_count, declaration **static_declarations)
  {
    routine_declaration *result;
    routine_declaration_chain *declaration_chain;

    assert(the_formal_arguments != NULL);
    assert((static_count == 0) || (static_declarations != NULL));

    assert((body == NULL) || (native_handler == NULL));

    result = MALLOC_ONE_OBJECT(routine_declaration);
    if (result == NULL)
      {
      cleanup:
        if (static_return_type != NULL)
            delete_type_expression(static_return_type);
        if (dynamic_return_type != NULL)
            delete_type_expression(dynamic_return_type);
        delete_formal_arguments(the_formal_arguments);
        if (body != NULL)
            delete_statement_block(body);
        if (single_lock != NULL)
            delete_expression(single_lock);
        return NULL;
      }

    declaration_chain = create_routine_declaration_chain(result, NULL);
    if (declaration_chain == NULL)
      {
        free(result);
        goto cleanup;
      }

    result->declaration = NULL;
    result->static_return_type = static_return_type;
    result->dynamic_return_type = dynamic_return_type;
    result->the_formal_arguments = the_formal_arguments;
    result->body = body;
    result->native_handler = native_handler;
    result->purity_safety = the_purity_safety;
    result->declaration_chain = declaration_chain;
    result->extra_arguments_allowed = extra_arguments_allowed;

    result->static_home =
            create_static_home(static_count, static_declarations);
    if (result->static_home == NULL)
      {
        routine_declaration_chain_remove_reference(declaration_chain);
        free(result);
        goto cleanup;
      }

    INITIALIZE_SYSTEM_LOCK(result->call_lock,
        delete_static_home(result->static_home);
        routine_declaration_chain_remove_reference(declaration_chain);
        free(result);
        goto cleanup);

    result->is_pure = is_pure;
    result->is_class = is_class;
    result->single_lock = single_lock;
    result->call_count = oi_zero;
    CLOCK_ZERO(result->net_time);
    CLOCK_ZERO(result->local_time);
    result->in_calls = NULL;

    return result;
  }

extern void routine_declaration_add_reference(routine_declaration *declaration)
  {
    assert(declaration != NULL);

    declaration_add_reference(declaration->declaration);
  }

extern void routine_declaration_remove_reference(
        routine_declaration *declaration)
  {
    assert(declaration != NULL);

    declaration_remove_reference(declaration->declaration);
  }

extern void delete_routine_declaration(routine_declaration *declaration)
  {
    in_call_info *in_calls;

    assert(declaration != NULL);

    if (declaration->static_return_type != NULL)
        delete_type_expression(declaration->static_return_type);
    if (declaration->dynamic_return_type != NULL)
        delete_type_expression(declaration->dynamic_return_type);
    delete_formal_arguments(declaration->the_formal_arguments);
    if (declaration->body != NULL)
        delete_statement_block(declaration->body);
    if (declaration->single_lock != NULL)
        delete_expression(declaration->single_lock);
    routine_declaration_chain_remove_reference(declaration->declaration_chain);
    delete_static_home(declaration->static_home);
    DESTROY_SYSTEM_LOCK(declaration->call_lock);

    in_calls = declaration->in_calls;
    while (in_calls != NULL)
      {
        in_call_info *next;

        next = in_calls->next;
        free(in_calls);
        in_calls = next;
      }

    free(declaration);
  }

extern declaration *routine_declaration_declaration(
        routine_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration->declaration;
  }

extern const char *routine_declaration_name(routine_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_name(declaration->declaration);
  }

extern type_expression *routine_declaration_static_return_type(
        routine_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->static_return_type;
  }

extern type_expression *routine_declaration_dynamic_return_type(
        routine_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->dynamic_return_type;
  }

extern formal_arguments *routine_declaration_formals(
        routine_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->the_formal_arguments;
  }

extern boolean routine_declaration_extra_arguments_allowed(
        routine_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->extra_arguments_allowed;
  }

extern statement_block *routine_declaration_body(
        routine_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->body;
  }

extern native_bridge_routine *routine_declaration_native_handler(
        routine_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->native_handler;
  }

extern purity_safety routine_declaration_purity_safety(
        routine_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->purity_safety;
  }

extern boolean routine_declaration_is_static(routine_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_is_static(declaration->declaration);
  }

extern boolean routine_declaration_is_virtual(routine_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_is_virtual(declaration->declaration);
  }

extern boolean routine_declaration_is_pure(routine_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->is_pure;
  }

extern boolean routine_declaration_is_class(routine_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->is_class;
  }

extern expression *routine_declaration_single_lock(
        routine_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->single_lock;
  }

extern boolean routine_declaration_automatic_allocation(
        routine_declaration *declaration)
  {
    assert(declaration != NULL);

    assert(declaration->declaration != NULL);

    return declaration_automatic_allocation(declaration->declaration);
  }

extern routine_declaration_chain *routine_declaration_declaration_chain(
        routine_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->declaration_chain;
  }

extern static_home *routine_declaration_static_home(
        routine_declaration *declaration)
  {
    assert(declaration != NULL);

    return declaration->static_home;
  }

extern void set_routine_declaration_declaration(
        routine_declaration *the_routine_declaration,
        declaration *the_declaration)
  {
    assert(the_routine_declaration != NULL);
    assert(the_declaration != NULL);

    assert(the_routine_declaration->declaration == NULL);
    the_routine_declaration->declaration = the_declaration;
  }

extern verdict routine_declaration_record_call(
        routine_declaration *the_routine_declaration, CLOCK_T net_time,
        CLOCK_T local_time)
  {
    o_integer new_count;
    CLOCK_T new_time;

    assert(the_routine_declaration != NULL);

    GRAB_SYSTEM_LOCK(the_routine_declaration->call_lock);

    oi_add(new_count, the_routine_declaration->call_count, oi_one);
    if (oi_out_of_memory(new_count))
      {
        RELEASE_SYSTEM_LOCK(the_routine_declaration->call_lock);
        return MISSION_FAILED;
      }

    oi_remove_reference(the_routine_declaration->call_count);
    the_routine_declaration->call_count = new_count;

    CLOCK_ADD(new_time, the_routine_declaration->net_time, net_time);
    the_routine_declaration->net_time = new_time;

    CLOCK_ADD(new_time, the_routine_declaration->local_time, local_time);
    the_routine_declaration->local_time = new_time;

    RELEASE_SYSTEM_LOCK(the_routine_declaration->call_lock);

    return MISSION_ACCOMPLISHED;
  }

extern o_integer routine_declaration_call_count(
        routine_declaration *the_routine_declaration)
  {
    assert(the_routine_declaration != NULL);

    oi_add_reference(the_routine_declaration->call_count);
    return the_routine_declaration->call_count;
  }

extern CLOCK_T routine_declaration_net_time(
        routine_declaration *the_routine_declaration)
  {
    assert(the_routine_declaration != NULL);

    return the_routine_declaration->net_time;
  }

extern CLOCK_T routine_declaration_local_time(
        routine_declaration *the_routine_declaration)
  {
    assert(the_routine_declaration != NULL);

    return the_routine_declaration->local_time;
  }

extern boolean routine_declaration_in_call_on_thread(
        routine_declaration *the_routine_declaration, salmon_thread *thread)
  {
    in_call_info *follow;

    assert(the_routine_declaration != NULL);
    assert(thread != NULL);

    GRAB_SYSTEM_LOCK(the_routine_declaration->call_lock);

    follow = the_routine_declaration->in_calls;
    while (TRUE)
      {
        if (follow == NULL)
          {
            RELEASE_SYSTEM_LOCK(the_routine_declaration->call_lock);
            return FALSE;
          }
        if (follow->thread == thread)
          {
            assert(follow->count > 0);
            RELEASE_SYSTEM_LOCK(the_routine_declaration->call_lock);
            return TRUE;
          }
        follow = follow->next;
      }
  }

extern verdict routine_declaration_start_in_call_on_thread(
        routine_declaration *the_routine_declaration, salmon_thread *thread)
  {
    in_call_info **follow;

    assert(the_routine_declaration != NULL);
    assert(thread != NULL);

    GRAB_SYSTEM_LOCK(the_routine_declaration->call_lock);

    follow = &(the_routine_declaration->in_calls);
    while (TRUE)
      {
        in_call_info *this_record;

        assert(follow != NULL);
        this_record = *follow;
        if (this_record == NULL)
          {
            this_record = MALLOC_ONE_OBJECT(in_call_info);
            if (this_record == NULL)
              {
                RELEASE_SYSTEM_LOCK(the_routine_declaration->call_lock);
                return MISSION_FAILED;
              }

            this_record->thread = thread;
            this_record->count = 1;
            this_record->next = NULL;
            *follow = this_record;

            RELEASE_SYSTEM_LOCK(the_routine_declaration->call_lock);

            return MISSION_ACCOMPLISHED;
          }

        if (this_record->thread == thread)
          {
            assert(this_record->count > 0);
            ++(this_record->count);

            RELEASE_SYSTEM_LOCK(the_routine_declaration->call_lock);

            return MISSION_ACCOMPLISHED;
          }

        follow = &(this_record->next);
      }
  }

extern void routine_declaration_end_in_call_on_thread(
        routine_declaration *the_routine_declaration, salmon_thread *thread)
  {
    in_call_info **follow;

    assert(the_routine_declaration != NULL);
    assert(thread != NULL);

    GRAB_SYSTEM_LOCK(the_routine_declaration->call_lock);

    follow = &(the_routine_declaration->in_calls);
    while (TRUE)
      {
        in_call_info *this_record;

        assert(follow != NULL);
        this_record = *follow;
        assert(this_record != NULL);
        if (this_record->thread == thread)
          {
            assert(this_record->count > 0);
            --(this_record->count);
            if (this_record->count == 0)
              {
                *follow = this_record->next;
                free(this_record);
              }

            RELEASE_SYSTEM_LOCK(the_routine_declaration->call_lock);

            return;
          }
        follow = &(this_record->next);
      }
  }
