/* file "context.h" */

/*
 *  This file contains the interface to the context module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef CONTEXT_H
#define CONTEXT_H


typedef struct context context;


#include "statement.h"
#include "statement_block.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "tagalong_declaration.h"
#include "lepton_key_declaration.h"
#include "quark_declaration.h"
#include "lock_declaration.h"
#include "variable_instance.h"
#include "routine_instance.h"
#include "tagalong_key.h"
#include "lepton_key_instance.h"
#include "quark.h"
#include "lock_instance.h"
#include "jump_target.h"
#include "source_location.h"
#include "jumper.h"
#include "static_home.h"
#include "object.h"
#include "virtual_lookup.h"
#include "use_instance.h"
#include "purity_level.h"
#include "reference_cluster.h"


extern context *create_top_level_context(value *arguments);
extern context *create_statement_block_context(context *parent,
        statement_block *the_statement_block, reference_cluster *cluster,
        virtual_lookup *virtual_parent, jumper *the_jumper);
extern context *create_routine_context(context *parent,
        routine_instance *the_routine_instance, boolean expect_return_value,
        purity_level *level);
extern context *create_loop_context(context *parent, void *loop_construct,
        variable_declaration *element_declaration, purity_level *level,
        const source_location *location);
extern context *create_block_expression_context(context *parent,
                                                expression *the_expression);
extern context *create_try_catch_statement_context(context *parent,
        statement *try_catch_statement);
extern context *create_static_context(context *parent,
        static_home *the_static_home, jumper *the_jumper);
extern context *create_singleton_variable_context(context *parent,
        variable_declaration *the_variable_declaration, purity_level *level);
extern context *create_glue_context();

extern void exit_context(context *the_context, jumper *the_jumper);

extern size_t context_depth(context *the_context);

extern instance *find_instance(context *the_context,
                               declaration *the_declaration);
extern variable_instance *find_variable_instance(context *the_context,
        variable_declaration *declaration);
extern routine_instance *find_routine_instance(context *the_context,
        routine_declaration *declaration);
extern tagalong_key *find_tagalong_instance(context *the_context,
                                            tagalong_declaration *declaration);
extern lepton_key_instance *find_lepton_key_instance(context *the_context,
        lepton_key_declaration *declaration);
extern quark *find_quark_instance(context *the_context,
                                  quark_declaration *declaration);
extern lock_instance *find_lock_instance(context *the_context,
                                         lock_declaration *declaration);
extern jump_target *find_label_instance(context *the_context,
                                        statement *declaration);
extern jump_target *find_break_target(context *the_context,
                                      void *loop_construct);
extern jump_target *find_continue_target(context *the_context,
                                         void *loop_construct);
extern jump_target *find_routine_return_target(context *the_context,
        routine_declaration *declaration);
extern jump_target *find_block_expression_return_target(context *the_context,
        expression *block_expression);
extern value *find_arguments_value(context *the_context,
                                   routine_declaration *declaration);
extern object *find_this_object_value(context *the_context,
                                      routine_declaration *declaration);
extern object *find_top_this_object_value(context *the_context, size_t level);
extern use_instance *find_use_instance(context *the_context,
                                       statement *use_statement);
extern void set_routine_return_value(context *the_context,
        routine_declaration *declaration, value *return_value,
        const source_location *location, jumper *the_jumper);
extern void check_routine_return_value(context *the_context,
        value *return_value, const source_location *location,
        jumper *the_jumper);
extern value *get_routine_return_value(context *the_context,
                                       routine_declaration *declaration);
extern boolean nearest_routine_expects_return_value(context *the_context);
extern void set_block_expression_return_value(context *the_context,
        expression *block_expression, value *return_value,
        const source_location *location, jumper *the_jumper);
extern verdict context_add_extra_instance(context *the_context,
                                          instance *extra);
extern declaration *glue_context_add_instance(context *the_context,
                                              instance *instance);

extern void top_level_context_set_directory_paths(context *the_context,
                                                  const char *new_value);
extern void top_level_context_set_executable_directory(context *the_context,
                                                       const char *new_value);
extern const char *context_directory_paths(context *the_context);
extern const char *context_executable_directory(context *the_context);

extern jump_target *routine_context_return_target(context *the_context);
extern value *routine_context_return_value(context *the_context);
extern void routine_context_set_all_arguments_value(context *the_context,
        value *all_arguments_value);
extern void routine_context_set_dynamic_return_type(context *the_context,
                                                    type *new_type);
extern void routine_context_set_this_object_value(context *the_context,
                                                  object *this_object);

extern void *loop_context_loop_construct(context *the_context);
extern jump_target *loop_context_continue_target(context *the_context);
extern jump_target *loop_context_break_target(context *the_context);
extern variable_instance *loop_context_index(context *the_context);

extern jump_target *block_expression_context_return_target(
        context *the_context);
extern value *block_expression_context_return_value(context *the_context);

extern jump_target *try_catch_statement_context_default_catch_target(
        context *the_context);
extern jump_target *try_catch_statement_context_catch_target(
        context *the_context, size_t clause_num);

extern verdict context_add_virtual_dependence(context *parent, object *child);

extern void context_add_reference(context *the_context);
extern void context_remove_reference(context *the_context, jumper *the_jumper);
extern void context_add_internal_reference(context *the_context);
extern void context_remove_internal_reference(context *the_context);

DEFINE_EXCEPTION_TAG(pure_virtual_no_override);
DEFINE_EXCEPTION_TAG(call_class_return_value);
DEFINE_EXCEPTION_TAG(call_function_no_return_value);
DEFINE_EXCEPTION_TAG(call_return_type_mismatch);
DEFINE_EXCEPTION_TAG(call_return_type_match_indeterminate);
DEFINE_EXCEPTION_TAG(call_two_return_values);
DEFINE_EXCEPTION_TAG(call_procedure_return_value);


#endif /* CONTEXT_H */
