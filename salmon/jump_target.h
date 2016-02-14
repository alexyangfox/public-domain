/* file "jump_target.h" */

/*
 *  This file contains the interface to the jump_target module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef JUMP_TARGET_H
#define JUMP_TARGET_H


typedef enum jump_target_kind
  {
    JTK_LABEL,
    JTK_ROUTINE_RETURN,
    JTK_TOP_LEVEL_RETURN,
    JTK_LOOP_CONTINUE,
    JTK_LOOP_BREAK,
    JTK_BLOCK_EXPRESSION_RETURN,
    JTK_TRY_CATCH_CATCH
  } jump_target_kind;

typedef struct jump_target jump_target;


#include "context.h"
#include "statement.h"
#include "routine_declaration.h"
#include "validator.h"
#include "source_location.h"


extern jump_target *create_label_jump_target(context *the_context,
                                             statement *label_statement);
extern jump_target *create_routine_return_jump_target(context *the_context,
        routine_declaration *declaration);
extern jump_target *create_top_level_return_jump_target(context *the_context);
extern jump_target *create_loop_continue_jump_target(context *the_context,
        void *loop_construct, const source_location *location);
extern jump_target *create_loop_break_jump_target(context *the_context,
        void *loop_construct, const source_location *location);
extern jump_target *create_block_expression_return_jump_target(
        context *the_context, expression *block_expression);
extern jump_target *create_try_catch_catch_jump_target(context *the_context,
        statement *try_catch_statement);

extern void jump_target_add_reference(jump_target *the_jump_target);
extern void jump_target_remove_reference(jump_target *the_jump_target);

extern void set_jump_target_scope_exited(jump_target *the_jump_target);
extern void jump_target_set_validator_chain(jump_target *the_jump_target,
                                            validator_chain *chain);

extern jump_target_kind get_jump_target_kind(jump_target *the_jump_target);

extern boolean jump_target_scope_exited(jump_target *the_jump_target);
extern validator_chain *jump_target_validator_chain(
        jump_target *the_jump_target);
extern context *jump_target_context(jump_target *the_jump_target);
extern size_t jump_target_depth(jump_target *the_jump_target);
extern statement *label_jump_target_label_statement(
        jump_target *the_jump_target);
extern routine_declaration *routine_return_jump_target_routine_declaration(
        jump_target *the_jump_target);
extern const source_location *loop_continue_jump_target_loop_location(
        jump_target *the_jump_target);
extern const source_location *loop_break_jump_target_loop_location(
        jump_target *the_jump_target);

extern boolean jump_targets_are_equal(jump_target *target1,
                                      jump_target *target2);
extern int jump_target_structural_order(jump_target *left, jump_target *right);


#endif /* JUMP_TARGET_H */
