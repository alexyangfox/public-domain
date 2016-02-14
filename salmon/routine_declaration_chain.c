/* file "routine_declaration_chain.c" */

/*
 *  This file contains the implementation of the routine_declaration_chain
 *  module.
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
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "routine_declaration_chain.h"
#include "routine_instance_chain.h"
#include "context.h"
#include "routine_instance.h"
#include "routine_declaration.h"
#include "jumper.h"
#include "execute.h"
#include "platform_dependent.h"


struct routine_declaration_chain
  {
    routine_declaration *declaration;
    routine_declaration_chain *next_chain;
    statement *next_use;
    size_t next_used_for_num;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
  };


extern routine_declaration_chain *create_routine_declaration_chain(
        routine_declaration *declaration,
        routine_declaration_chain *next_chain)
  {
    routine_declaration_chain *result;

    assert(declaration != NULL);

    result = MALLOC_ONE_OBJECT(routine_declaration_chain);
    if (result == NULL)
        return NULL;

    result->declaration = declaration;

    result->next_chain = next_chain;
    result->next_use = NULL;
    result->next_used_for_num = 0;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock, free(result); return NULL);

    if (next_chain != NULL)
        routine_declaration_chain_add_reference(next_chain);

    result->reference_count = 1;

    return result;
  }

extern void routine_declaration_chain_add_reference(
        routine_declaration_chain *chain)
  {
    assert(chain != NULL);

    GRAB_SYSTEM_LOCK(chain->reference_lock);
    assert(chain->reference_count > 0);
    ++(chain->reference_count);
    RELEASE_SYSTEM_LOCK(chain->reference_lock);
  }

extern void routine_declaration_chain_remove_reference(
        routine_declaration_chain *chain)
  {
    size_t new_reference_count;

    assert(chain != NULL);

    GRAB_SYSTEM_LOCK(chain->reference_lock);
    assert(chain->reference_count > 0);
    --(chain->reference_count);
    new_reference_count = chain->reference_count;
    RELEASE_SYSTEM_LOCK(chain->reference_lock);

    if (new_reference_count > 0)
        return;

    if (chain->next_chain != NULL)
        routine_declaration_chain_remove_reference(chain->next_chain);

    DESTROY_SYSTEM_LOCK(chain->reference_lock);

    free(chain);
  }

extern routine_declaration *routine_declaration_chain_declaration(
        routine_declaration_chain *chain)
  {
    assert(chain != NULL);

    return chain->declaration;
  }

extern routine_declaration_chain *routine_declaration_chain_next(
        routine_declaration_chain *chain)
  {
    assert(chain != NULL);

    return chain->next_chain;
  }

extern boolean routine_declaration_chain_has_more_than_one_reference(
        routine_declaration_chain *chain)
  {
    assert(chain != NULL);

    return (chain->reference_count > 1);
  }

extern void routine_declaration_chain_set_next(routine_declaration_chain *base,
                                               routine_declaration_chain *next)
  {
    assert(base != NULL);

    assert(base->next_chain == NULL);
    assert(base->next_use == NULL);

    if (next != NULL)
        routine_declaration_chain_add_reference(next);

    base->next_chain = next;
  }

extern void routine_declaration_chain_set_next_to_use_statement(
        routine_declaration_chain *base, statement *use_statement,
        size_t used_for_num)
  {
    assert(base != NULL);

    assert(base->next_chain == NULL);
    assert(base->next_use == NULL);

    base->next_use = use_statement;
    base->next_used_for_num = used_for_num;
  }

extern routine_instance_chain *
        routine_declaration_chain_to_routine_instance_chain(
                routine_declaration_chain *declaration_chain,
                context *the_context, jumper *the_jumper)
  {
    routine_instance *the_routine_instance;
    routine_instance_chain *next;
    routine_instance_chain *result;

    assert(declaration_chain != NULL);
    assert(the_context != NULL);

    the_routine_instance =
            find_routine_instance(the_context, declaration_chain->declaration);
    assert(the_routine_instance != NULL);

    if (declaration_chain->next_chain == NULL)
      {
        if (declaration_chain->next_use == NULL)
          {
            next = NULL;
          }
        else
          {
            routine_declaration_chain *flow_through_declaration_chain;
            use_instance *the_use_instance;
            routine_instance_chain *local_next;

            flow_through_declaration_chain = use_statement_used_for_chain(
                    declaration_chain->next_use,
                    declaration_chain->next_used_for_num);

            if (flow_through_declaration_chain != NULL)
              {
                next = routine_declaration_chain_to_routine_instance_chain(
                        flow_through_declaration_chain, the_context,
                        the_jumper);
                if (next == NULL)
                  {
                    assert(!(jumper_flowing_forward(the_jumper)));
                    return NULL;
                  }
                assert(jumper_flowing_forward(the_jumper));
              }
            else
              {
                declaration *flow_through_declaration;

                flow_through_declaration = use_statement_used_for_declaration(
                        declaration_chain->next_use,
                        declaration_chain->next_used_for_num);

                if ((flow_through_declaration != NULL) &&
                    (declaration_kind(flow_through_declaration) == NK_ROUTINE))
                  {
                    routine_instance *next_routine_instance;

                    next_routine_instance = find_routine_instance(the_context,
                            declaration_chain->declaration);
                    assert(next_routine_instance != NULL);

                    next = create_routine_instance_chain(next_routine_instance,
                                                         NULL);
                    if (next == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        return NULL;
                      }
                  }
                else
                  {
                    statement *flow_through_use;

                    flow_through_use = use_statement_used_for_next_use(
                            declaration_chain->next_use,
                            declaration_chain->next_used_for_num);

                    if (flow_through_use != NULL)
                      {
                        instance *test_instance;
                        jump_target *test_jump_target;

                        find_instance_from_use_statement(flow_through_use,
                                use_statement_used_for_next_used_for_number(
                                        declaration_chain->next_use,
                                        declaration_chain->next_used_for_num),
                                the_context, FALSE, &test_instance, &next,
                                &test_jump_target, NULL, the_jumper);
                        if (!(jumper_flowing_forward(the_jumper)))
                            return NULL;

                        if ((test_instance != NULL) &&
                            (instance_kind(test_instance) == NK_ROUTINE))
                          {
                            assert(next == NULL);

                            next = create_routine_instance_chain(
                                    instance_routine_instance(test_instance),
                                    NULL);
                            if (next == NULL)
                              {
                                jumper_do_abort(the_jumper);
                                return NULL;
                              }
                          }
                      }
                    else
                      {
                        next = NULL;
                      }
                  }
              }

            the_use_instance = find_use_instance(the_context,
                                                 declaration_chain->next_use);
            assert(the_use_instance != NULL);

            local_next = use_instance_chain(the_use_instance,
                                      declaration_chain->next_used_for_num);

            if (local_next != NULL)
              {
                if (next == NULL)
                  {
                    routine_instance_chain_add_reference(local_next);
                    next = local_next;
                  }
                else
                  {
                    local_next =
                            combine_routine_chains(next, local_next, NULL);
                    routine_instance_chain_remove_reference(next, NULL);
                    if (local_next == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        return NULL;
                      }
                    next = local_next;
                  }
              }
            else
              {
                instance *the_instance;

                the_instance = use_instance_instance(the_use_instance,
                        declaration_chain->next_used_for_num);
                if ((the_instance != NULL) &&
                    (instance_kind(the_instance) == NK_ROUTINE))
                  {
                    local_next = create_routine_instance_chain(
                            instance_routine_instance(the_instance), next);
                    if (next != NULL)
                        routine_instance_chain_remove_reference(next, NULL);
                    if (local_next == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        return NULL;
                      }
                    next = local_next;
                  }
              }
          }
      }
    else
      {
        next = routine_declaration_chain_to_routine_instance_chain(
                declaration_chain->next_chain, the_context, the_jumper);
        if (next == NULL)
          {
            assert(!(jumper_flowing_forward(the_jumper)));
            return NULL;
          }
        assert(jumper_flowing_forward(the_jumper));
      }

    result = create_routine_instance_chain(the_routine_instance, next);
    if (next != NULL)
        routine_instance_chain_remove_reference(next, NULL);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }
