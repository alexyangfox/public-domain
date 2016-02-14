/* file "jump_target.c" */

/*
 *  This file contains the implementation of the jump_target module.
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
#include "jump_target.h"
#include "context.h"
#include "statement.h"
#include "validator.h"
#include "source_location.h"
#include "platform_dependent.h"


struct jump_target
  {
    jump_target_kind kind;
    context *context;
    boolean scope_exited;
    union
      {
        statement *statement;
        routine_declaration *routine_declaration;
        struct
          {
            void *construct;
            const source_location *location;
          } loop;
        expression *block_expression;
      } u;
    validator_chain *validator_chain;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
  };


extern jump_target *create_label_jump_target(context *the_context,
                                             statement *label_statement)
  {
    jump_target *result;

    assert(the_context != NULL);
    assert(label_statement != NULL);

    result = MALLOC_ONE_OBJECT(jump_target);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock, free(result); return NULL);

    result->kind = JTK_LABEL;
    result->context = the_context;
    result->scope_exited = FALSE;
    result->u.statement = label_statement;
    result->validator_chain = NULL;

    result->reference_count = 1;

    return result;
  }

extern jump_target *create_routine_return_jump_target(context *the_context,
        routine_declaration *declaration)
  {
    jump_target *result;

    assert(the_context != NULL);
    assert(declaration != NULL);

    result = MALLOC_ONE_OBJECT(jump_target);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock, free(result); return NULL);

    result->kind = JTK_ROUTINE_RETURN;
    result->context = the_context;
    result->scope_exited = FALSE;
    result->u.routine_declaration = declaration;
    result->validator_chain = NULL;

    result->reference_count = 1;

    return result;
  }

extern jump_target *create_top_level_return_jump_target(context *the_context)
  {
    jump_target *result;

    assert(the_context != NULL);

    result = MALLOC_ONE_OBJECT(jump_target);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock, free(result); return NULL);

    result->kind = JTK_TOP_LEVEL_RETURN;
    result->context = the_context;
    result->scope_exited = FALSE;
    result->validator_chain = NULL;

    result->reference_count = 1;

    return result;
  }

extern jump_target *create_loop_continue_jump_target(context *the_context,
        void *loop_construct, const source_location *location)
  {
    jump_target *result;

    assert(the_context != NULL);
    assert(loop_construct != NULL);

    result = MALLOC_ONE_OBJECT(jump_target);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock, free(result); return NULL);

    result->kind = JTK_LOOP_CONTINUE;
    result->context = the_context;
    result->scope_exited = FALSE;
    result->u.loop.construct = loop_construct;
    result->u.loop.location = location;
    result->validator_chain = NULL;

    result->reference_count = 1;

    return result;
  }

extern jump_target *create_loop_break_jump_target(context *the_context,
        void *loop_construct, const source_location *location)
  {
    jump_target *result;

    assert(the_context != NULL);
    assert(loop_construct != NULL);

    result = MALLOC_ONE_OBJECT(jump_target);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock, free(result); return NULL);

    result->kind = JTK_LOOP_BREAK;
    result->context = the_context;
    result->scope_exited = FALSE;
    result->u.loop.construct = loop_construct;
    result->u.loop.location = location;
    result->validator_chain = NULL;

    result->reference_count = 1;

    return result;
  }

extern jump_target *create_block_expression_return_jump_target(
        context *the_context, expression *block_expression)
  {
    jump_target *result;

    assert(the_context != NULL);
    assert(block_expression != NULL);

    result = MALLOC_ONE_OBJECT(jump_target);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock, free(result); return NULL);

    result->kind = JTK_BLOCK_EXPRESSION_RETURN;
    result->context = the_context;
    result->scope_exited = FALSE;
    result->u.block_expression = block_expression;
    result->validator_chain = NULL;

    result->reference_count = 1;

    return result;
  }

extern jump_target *create_try_catch_catch_jump_target(context *the_context,
        statement *try_catch_statement)
  {
    jump_target *result;

    assert(the_context != NULL);
    assert(try_catch_statement != NULL);

    result = MALLOC_ONE_OBJECT(jump_target);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock, free(result); return NULL);

    result->kind = JTK_TRY_CATCH_CATCH;
    result->context = the_context;
    result->scope_exited = FALSE;
    result->u.statement = try_catch_statement;
    result->validator_chain = NULL;

    result->reference_count = 1;

    return result;
  }

extern void jump_target_add_reference(jump_target *the_jump_target)
  {
    assert(the_jump_target != NULL);

    GRAB_SYSTEM_LOCK(the_jump_target->reference_lock);
    ++(the_jump_target->reference_count);
    RELEASE_SYSTEM_LOCK(the_jump_target->reference_lock);
  }

extern void jump_target_remove_reference(jump_target *the_jump_target)
  {
    size_t new_reference_count;

    assert(the_jump_target != NULL);

    GRAB_SYSTEM_LOCK(the_jump_target->reference_lock);
    assert(the_jump_target->reference_count > 0);
    --(the_jump_target->reference_count);
    new_reference_count = the_jump_target->reference_count;
    RELEASE_SYSTEM_LOCK(the_jump_target->reference_lock);

    if (new_reference_count == 0)
      {
        assert(the_jump_target->validator_chain == NULL);
        DESTROY_SYSTEM_LOCK(the_jump_target->reference_lock);
        free(the_jump_target);
      }
  }

extern void set_jump_target_scope_exited(jump_target *the_jump_target)
  {
    assert(the_jump_target != NULL);

    assert(!(the_jump_target->scope_exited)); /* VERIFIED */

    the_jump_target->scope_exited = TRUE;
    validator_chain_mark_deallocated(the_jump_target->validator_chain);
  }

extern void jump_target_set_validator_chain(jump_target *the_jump_target,
                                            validator_chain *chain)
  {
    assert(the_jump_target != NULL);

    the_jump_target->validator_chain = chain;
  }

extern jump_target_kind get_jump_target_kind(jump_target *the_jump_target)
  {
    assert(the_jump_target != NULL);

    return the_jump_target->kind;
  }

extern boolean jump_target_scope_exited(jump_target *the_jump_target)
  {
    assert(the_jump_target != NULL);

    return the_jump_target->scope_exited;
  }

extern validator_chain *jump_target_validator_chain(
        jump_target *the_jump_target)
  {
    assert(the_jump_target != NULL);

    return the_jump_target->validator_chain;
  }

extern context *jump_target_context(jump_target *the_jump_target)
  {
    assert(the_jump_target != NULL);

    assert(!(the_jump_target->scope_exited)); /* VERIFIED */

    return the_jump_target->context;
  }

extern size_t jump_target_depth(jump_target *the_jump_target)
  {
    assert(the_jump_target != NULL);

    assert(!(the_jump_target->scope_exited)); /* VERIFIED */

    assert(the_jump_target->context != NULL);
    return context_depth(the_jump_target->context);
  }

extern statement *label_jump_target_label_statement(
        jump_target *the_jump_target)
  {
    assert(the_jump_target != NULL);

    assert(the_jump_target->kind == JTK_LABEL);
    return the_jump_target->u.statement;
  }

extern routine_declaration *routine_return_jump_target_routine_declaration(
        jump_target *the_jump_target)
  {
    assert(the_jump_target != NULL);

    assert(the_jump_target->kind == JTK_ROUTINE_RETURN);
    return the_jump_target->u.routine_declaration;
  }

extern const source_location *loop_continue_jump_target_loop_location(
        jump_target *the_jump_target)
  {
    assert(the_jump_target != NULL);

    assert(the_jump_target->kind == JTK_LOOP_CONTINUE);
    return the_jump_target->u.loop.location;
  }

extern const source_location *loop_break_jump_target_loop_location(
        jump_target *the_jump_target)
  {
    assert(the_jump_target != NULL);

    assert(the_jump_target->kind == JTK_LOOP_BREAK);
    return the_jump_target->u.loop.location;
  }

extern boolean jump_targets_are_equal(jump_target *target1,
                                      jump_target *target2)
  {
    assert(target1 != NULL);
    assert(target2 != NULL);

    assert(!(target1->scope_exited)); /* VERIFIED */
    assert(!(target2->scope_exited)); /* VERIFIED */

    if (target1->context != target2->context)
        return FALSE;

    if (target1->kind != target2->kind)
        return FALSE;

    switch (target1->kind)
      {
        case JTK_LABEL:
            return (target1->u.statement == target2->u.statement);
        case JTK_ROUTINE_RETURN:
            return (target1->u.routine_declaration ==
                    target2->u.routine_declaration);
        case JTK_TOP_LEVEL_RETURN:
            return TRUE;
        case JTK_LOOP_CONTINUE:
            return (target1->u.loop.construct == target2->u.loop.construct);
        case JTK_LOOP_BREAK:
            return (target1->u.loop.construct == target2->u.loop.construct);
        case JTK_BLOCK_EXPRESSION_RETURN:
            return (target1->u.block_expression ==
                    target2->u.block_expression);
        case JTK_TRY_CATCH_CATCH:
            return (target1->u.statement == target2->u.statement);
        default:
            assert(FALSE);
            return FALSE;
      }
  }

extern int jump_target_structural_order(jump_target *left, jump_target *right)
  {
    assert(left != NULL);
    assert(right != NULL);

    assert(!(left->scope_exited)); /* VERIFIED */
    assert(!(right->scope_exited)); /* VERIFIED */

    if (left->context != right->context)
      {
        if (left->context < right->context)
            return -1;
        else
            return 1;
      }

    if (left->kind != right->kind)
      {
        if (left->kind < right->kind)
            return -1;
        else
            return 1;
      }

    switch (left->kind)
      {
        case JTK_LABEL:
        case JTK_TRY_CATCH_CATCH:
            if (left->u.statement == right->u.statement)
                return 0;
            else if (left->u.statement < right->u.statement)
                return -1;
            else
                return 1;
        case JTK_ROUTINE_RETURN:
            if (left->u.routine_declaration == right->u.routine_declaration)
              {
                return 0;
              }
            else if (left->u.routine_declaration <
                     right->u.routine_declaration)
              {
                return -1;
              }
            else
              {
                return 1;
              }
        case JTK_TOP_LEVEL_RETURN:
            return 0;
        case JTK_LOOP_CONTINUE:
        case JTK_LOOP_BREAK:
            if (left->u.loop.construct == right->u.loop.construct)
                return 0;
            else if (left->u.loop.construct < right->u.loop.construct)
                return -1;
            else
                return 1;
        case JTK_BLOCK_EXPRESSION_RETURN:
            if (left->u.block_expression == right->u.block_expression)
                return 0;
            else if (left->u.block_expression < right->u.block_expression)
                return -1;
            else
                return 1;
        default:
            assert(FALSE);
            return 0;
      }
  }
