/* file "routine_instance.h" */

/*
 *  This file contains the interface to the routine_instance module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef ROUTINE_INSTANCE_H
#define ROUTINE_INSTANCE_H

#include "c_foundations/basic.h"


typedef struct routine_instance routine_instance;


#include "routine_declaration.h"
#include "context.h"
#include "lock_chain.h"
#include "jumper.h"
#include "formal_arguments.h"
#include "instance.h"
#include "object.h"
#include "semi_labeled_value_list.h"
#include "purity_level.h"
#include "reference_cluster.h"
#include "source_location.h"


extern routine_instance *create_routine_instance(
        routine_declaration *declaration, purity_level *level,
        reference_cluster *cluster);

extern void routine_instance_add_reference(routine_instance *the_instance);
extern void routine_instance_remove_reference(routine_instance *the_instance,
                                              jumper *the_jumper);

extern void routine_instance_add_reference_with_cluster(
        routine_instance *the_instance, reference_cluster *cluster);
extern void routine_instance_remove_reference_with_cluster(
        routine_instance *the_instance, reference_cluster *cluster,
        jumper *the_jumper);

extern reference_cluster *routine_instance_reference_cluster(
        routine_instance *the_instance);

extern routine_declaration *routine_instance_declaration(
        routine_instance *instance);
extern context *routine_instance_context(routine_instance *the_instance);
extern boolean routine_instance_is_instantiated(
        routine_instance *the_instance);
extern boolean routine_instance_scope_exited(routine_instance *the_instance);
extern type *routine_instance_return_type(routine_instance *the_instance);
extern type *routine_instance_valid_return_type(routine_instance *the_instance,
        const source_location *location, jumper *the_jumper);
extern type *routine_instance_argument_type(routine_instance *the_instance,
                                            size_t argument_number);
extern lock_chain *routine_instance_lock_chain(routine_instance *the_instance);
extern instance *routine_instance_instance(routine_instance *the_instance);
extern boolean routine_instance_is_active(routine_instance *the_instance);

extern void routine_instance_set_return_type(routine_instance *the_instance,
        type *return_type, size_t new_expected_internal_references,
        jumper *the_jumper);
extern void routine_instance_set_argument_type(routine_instance *the_instance,
        type *argument_type, size_t argument_number, jumper *the_jumper);
extern void set_routine_instance_lock_chain(routine_instance *the_instance,
        lock_chain *the_lock_chain, jumper *the_jumper);
extern void routine_instance_set_up_static_context(
        routine_instance *the_instance, context *the_context,
        jumper *the_jumper);
extern void routine_instance_set_scope_exited(routine_instance *the_instance,
                                              jumper *the_jumper);
extern void increment_routine_instance_active_count(
        routine_instance *the_instance);
extern void decrement_routine_instance_active_count(
        routine_instance *the_instance);
extern void routine_instance_lock_live_instance_list(
        routine_instance *the_instance);
extern void routine_instance_unlock_live_instance_list(
        routine_instance *the_instance);
extern object *routine_instance_first_live_instance(
        routine_instance *the_instance);
extern void routine_instance_set_first_live_instance(
        routine_instance *the_instance, object *new_first_live_instance);

extern size_t *resolve_parameter_ordering_from_semi_ordered_value_list(
        formal_arguments *formals, semi_labeled_value_list *pre_order_actuals,
        boolean extra_arguments_allowed, boolean *error,
        size_t *post_order_count, size_t *duplicate_formal_argument_num,
        size_t *bad_name_actual_argument_num);
extern size_t *resolve_parameter_ordering_from_name_array(
        formal_arguments *formals, size_t pre_order_count,
        const char **pre_order_actual_names, boolean extra_arguments_allowed,
        boolean *error, size_t *post_order_count,
        size_t *duplicate_formal_argument_num,
        size_t *bad_name_actual_argument_num);

extern boolean routine_instances_are_equal(routine_instance *routine1,
                                           routine_instance *routine2);
extern int routine_instance_structural_order(routine_instance *left,
                                             routine_instance *right);


#endif /* ROUTINE_INSTANCE_H */
