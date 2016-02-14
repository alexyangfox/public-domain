/* file "execute.h" */

/*
 *  This file contains the interface to the execute module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef EXECUTE_H
#define EXECUTE_H

#include "c_foundations/basic.h"
#include "c_foundations/auto_array.h"
#include "expression.h"
#include "context.h"
#include "value.h"
#include "call.h"
#include "statement.h"
#include "statement_block.h"
#include "jumper.h"
#include "jump_target.h"
#include "slot_location.h"
#include "lookup_actual_arguments.h"
#include "basket_instance.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "tagalong_declaration.h"
#include "lepton_key_declaration.h"
#include "quark_declaration.h"
#include "lock_declaration.h"
#include "lock_instance.h"
#include "virtual_lookup.h"


AUTO_ARRAY(lock_instance_aa, lock_instance *);


extern verdict init_execute_module(void);
extern void cleanup_execute_module(void);

extern value *evaluate_expression(expression *the_expression,
                                  context *the_context, jumper *the_jumper);
extern value *evaluate_expression_without_context(expression *the_expression,
                                                  jumper *the_jumper);
extern value *evaluate_expression_with_virtuals(expression *the_expression,
        context *the_context, virtual_lookup *virtual_parent,
        jumper *the_jumper);
extern value *evaluate_constant_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_variable_reference_expression(
        expression *the_expression, context *the_context, jumper *the_jumper);
extern value *evaluate_routine_reference_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_tagalong_reference_expression(
        expression *the_expression, context *the_context, jumper *the_jumper);
extern value *evaluate_lepton_key_reference_expression(
        expression *the_expression, context *the_context, jumper *the_jumper);
extern value *evaluate_quark_reference_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_lock_reference_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_use_reference_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_label_reference_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_lookup_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_lepton_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_field_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_pointer_field_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_tagalong_field_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_statement_block_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_declaration_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_type_expression_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_map_list_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_semi_labeled_expression_list_expression(
        expression *the_expression, context *the_context, jumper *the_jumper);
extern value *evaluate_call_expression(expression *the_expression,
        context *the_context, virtual_lookup *virtual_parent,
        jumper *the_jumper);
extern value *evaluate_conditional_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_unary_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_binary_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_arguments_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_this_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_in_expression(expression *the_expression,
                                     context *the_context, jumper *the_jumper);
extern value *evaluate_force_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_break_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_continue_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_comprehend_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_forall_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *evaluate_exists_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
extern value *finish_evaluating_binary_expression(expression_kind kind,
        value *left_value, expression *right_expression,
        routine_declaration_chain *overload_chain,
        statement *overload_use_statement, size_t overload_used_for_number,
        context *the_context, jumper *the_jumper,
        const source_location *location);
extern type *evaluate_type_expression(type_expression *the_type_expression,
        context *the_context, jumper *the_jumper);
extern type *evaluate_constant_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_name_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_enumeration_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_not_type_expression(type_expression *the_type_expression,
        context *the_context, jumper *the_jumper);
extern type *evaluate_intersection_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_union_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_xor_type_expression(type_expression *the_type_expression,
        context *the_context, jumper *the_jumper);
extern type *evaluate_expression_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_array_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_integer_range_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_rational_range_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_pointer_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_type_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_map_type_expression(type_expression *the_type_expression,
        context *the_context, jumper *the_jumper);
extern type *evaluate_routine_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_fields_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_lepton_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_multiset_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_interface_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_semi_labeled_value_list_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern type *evaluate_regular_expression_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper);
extern slot_location *evaluate_address_of_expression(
        expression *the_expression, context *the_context, jumper *the_jumper);
extern value *get_slot_contents(slot_location *the_slot_location,
        lock_instance_aa *delayed_unlocks,
        const source_location *the_source_location, jumper *the_jumper);
extern void set_slot_contents(slot_location *the_slot_location,
        value *new_value, const source_location *the_source_location,
        jumper *the_jumper);
extern lookup_actual_arguments *evaluate_lookup_arguments(
        expression *lookup_expression, context *the_context,
        jumper *the_jumper);
extern basket_instance *evaluate_basket(basket *the_basket,
        context *the_context, jumper *the_jumper);
extern value *execute_call(call *the_call, boolean expect_return_value,
        routine_declaration_chain *overload_chain,
        statement *overload_use_statement, size_t overload_used_for_number,
        context *the_context, jumper *the_jumper);
extern value *execute_call_with_virtuals(call *the_call,
        boolean expect_return_value, routine_declaration_chain *overload_chain,
        statement *overload_use_statement, size_t overload_used_for_number,
        context *the_context, virtual_lookup *virtual_parent,
        jumper *the_jumper);
extern value *execute_call_from_values(value *base_value,
        semi_labeled_value_list *pre_order_actuals,
        boolean expect_return_value, jumper *the_jumper,
        const source_location *location);
extern value *execute_call_from_values_with_virtuals(value *base_value,
        semi_labeled_value_list *pre_order_actuals,
        boolean expect_return_value, virtual_lookup *virtual_parent,
        jumper *the_jumper, const source_location *location);
extern void execute_statement(statement *the_statement, context *the_context,
                              jumper *the_jumper, object *this_object);
extern void execute_statement_with_virtuals(statement *the_statement,
        context *the_context, jumper *the_jumper, object *this_object,
        virtual_lookup *virtual_parent);
extern void execute_assign_statement(statement *the_statement,
                                     context *the_context, jumper *the_jumper);
extern void execute_increment_statement(statement *the_statement,
        context *the_context, jumper *the_jumper);
extern void execute_decrement_statement(statement *the_statement,
        context *the_context, jumper *the_jumper);
extern void execute_call_statement(statement *the_statement,
                                   context *the_context, jumper *the_jumper);
extern void execute_declaration_statement(statement *the_statement,
        context *the_context, jumper *the_jumper, object *this_object);
extern void execute_if_statement(statement *the_statement,
                                 context *the_context, jumper *the_jumper);
extern void execute_switch_statement(statement *the_statement,
                                     context *the_context, jumper *the_jumper);
extern void execute_goto_statement(statement *the_statement,
                                   context *the_context, jumper *the_jumper);
extern void execute_return_statement(statement *the_statement,
                                     context *the_context, jumper *the_jumper);
extern void execute_for_statement(statement *the_statement,
                                  context *the_context, jumper *the_jumper);
extern void execute_iterate_statement(statement *the_statement,
        context *the_context, jumper *the_jumper);
extern void execute_while_statement(statement *the_statement,
                                    context *the_context, jumper *the_jumper);
extern void execute_do_while_statement(statement *the_statement,
        context *the_context, jumper *the_jumper);
extern void execute_break_statement(statement *the_statement,
                                    context *the_context, jumper *the_jumper);
extern void execute_continue_statement(statement *the_statement,
        context *the_context, jumper *the_jumper);
extern void execute_label_statement(statement *the_statement,
                                    context *the_context, jumper *the_jumper);
extern void execute_statement_block_statement(statement *the_statement,
        context *the_context, jumper *the_jumper);
extern void execute_single_statement(statement *the_statement,
                                     context *the_context, jumper *the_jumper);
extern void execute_try_catch_statement(statement *the_statement,
        context *the_context, jumper *the_jumper);
extern void execute_try_handle_statement(statement *the_statement,
        context *the_context, jumper *the_jumper);
extern void execute_export_statement(statement *the_statement,
                                     context *the_context, jumper *the_jumper);
extern void execute_hide_statement(statement *the_statement,
                                   context *the_context, jumper *the_jumper);
extern void execute_use_statement(statement *the_statement,
        context *the_context, jumper *the_jumper, object *this_object,
        virtual_lookup *virtual_parent);
extern void execute_theorem_statement(statement *the_statement,
        context *the_context, jumper *the_jumper);
extern void execute_statement_block(statement_block *the_statement_block,
                                    context *the_context, jumper *the_jumper);
extern void execute_statement_block_with_virtuals(
        statement_block *the_statement_block, context *the_context,
        jumper *the_jumper, object *this_object,
        virtual_lookup *virtual_parent);
extern void execute_declaration(declaration *the_declaration,
        instance *the_instance, context *the_context, jumper *the_jumper,
        object *this_object);
extern void execute_variable_declaration(variable_declaration *declaration,
        variable_instance *instance, context *the_context, jumper *the_jumper,
        object *this_object);
extern void execute_routine_declaration(routine_declaration *declaration,
        routine_instance *instance, context *the_context, jumper *the_jumper,
        object *this_object);
extern void execute_tagalong_declaration(tagalong_declaration *declaration,
        tagalong_key *instance, context *the_context, jumper *the_jumper,
        object *this_object);
extern void execute_lepton_key_declaration(lepton_key_declaration *declaration,
        lepton_key_instance *instance, context *the_context,
        jumper *the_jumper, object *this_object);
extern void execute_lock_declaration(lock_declaration *declaration,
        lock_instance *instance, context *the_context, jumper *the_jumper,
        object *this_object);
extern value *read_variable_value(variable_instance *instance,
        lock_instance_aa *delayed_unlocks, const source_location *location,
        jumper *the_jumper);
extern verdict try_overloading(value *base_value, const char *routine_name,
        value **result, size_t argument_count, value **arguments,
        const char **argument_names, jumper *the_jumper,
        const source_location *location);
extern verdict try_overloading_from_call_base(value *call_base, value **result,
        size_t argument_count, value **arguments, const char **argument_names,
        jumper *the_jumper, const source_location *location);
extern void find_overload_type(value *call_base, size_t argument_count,
        parameter_pattern_kind *parameter_pattern_kinds,
        const char **parameter_names, value **exact_parameters,
        type **parameter_lower_types, type **parameter_upper_types,
        boolean is_write, type *write_lower_requirement,
        type *write_upper_requirement, type **result_lower,
        type **result_upper, boolean *always_hits, boolean *never_hits,
        const source_location *location, jumper *the_jumper);
extern void find_overload_type_with_possible_map_result(value *call_base,
        size_t argument_count, parameter_pattern_kind *parameter_pattern_kinds,
        const char **parameter_names, value **exact_parameters,
        type **parameter_lower_types, type **parameter_upper_types,
        boolean is_write, type *write_lower_requirement,
        type *write_upper_requirement, size_t map_key_argument_number,
        type **result_lower, type **result_upper, boolean *always_hits,
        boolean *never_hits, const source_location *location,
        jumper *the_jumper);
extern void find_instance_from_use_statement(statement *use_statement,
        size_t used_for_num, context *the_context, boolean must_find,
        instance **result_instance, routine_instance_chain **result_chain,
        jump_target **result_jump_target, const source_location *location,
        jumper *the_jumper);

extern void set_profiling_enable(boolean enabled);

DEFINE_EXCEPTION_TAG(unbound_name_reference);
DEFINE_EXCEPTION_TAG(lepton_bad_key);
DEFINE_EXCEPTION_TAG(lepton_key_deleted_in_expression);
DEFINE_EXCEPTION_TAG(lepton_field_type_cant_force);
DEFINE_EXCEPTION_TAG(lepton_field_type_mismatch);
DEFINE_EXCEPTION_TAG(lepton_field_type_match_indeterminate);
DEFINE_EXCEPTION_TAG(lepton_field_disallowed);
DEFINE_EXCEPTION_TAG(pointer_field_reference_bad_base);
DEFINE_EXCEPTION_TAG(tagalong_reference_bad_key);
DEFINE_EXCEPTION_TAG(tagalong_reference_undefined);
DEFINE_EXCEPTION_TAG(statement_block_expression_no_value);
DEFINE_EXCEPTION_TAG(statement_block_expression_return_no_value);
DEFINE_EXCEPTION_TAG(semi_labeled_expression_list_duplicate_label);
DEFINE_EXCEPTION_TAG(conditional_bad_test);
DEFINE_EXCEPTION_TAG(dereference_bad_base);
DEFINE_EXCEPTION_TAG(in_indeterminate);
DEFINE_EXCEPTION_TAG(force_indeterminate);
DEFINE_EXCEPTION_TAG(force_cant_force);
DEFINE_EXCEPTION_TAG(comprehend_bad_test);
DEFINE_EXCEPTION_TAG(forall_executed);
DEFINE_EXCEPTION_TAG(exists_executed);
DEFINE_EXCEPTION_TAG(pointer_subtraction_indeterminate);
DEFINE_EXCEPTION_TAG(pointer_subtraction_non_lookup);
DEFINE_EXCEPTION_TAG(pointer_subtraction_overload_mismatch);
DEFINE_EXCEPTION_TAG(pointer_subtraction_base_indeterminate);
DEFINE_EXCEPTION_TAG(pointer_subtraction_base_mismatch);
DEFINE_EXCEPTION_TAG(divide_force_rational);
DEFINE_EXCEPTION_TAG(remainder_rational);
DEFINE_EXCEPTION_TAG(left_shift_rational);
DEFINE_EXCEPTION_TAG(right_shift_rational);
DEFINE_EXCEPTION_TAG(pointer_comparison_indeterminate);
DEFINE_EXCEPTION_TAG(pointer_comparison_non_lookup);
DEFINE_EXCEPTION_TAG(pointer_comparison_overload_mismatch);
DEFINE_EXCEPTION_TAG(pointer_comparison_base_indeterminate);
DEFINE_EXCEPTION_TAG(pointer_comparison_base_mismatch);
DEFINE_EXCEPTION_TAG(binary_bad_operands);
DEFINE_EXCEPTION_TAG(equality_test_indeterminate);
DEFINE_EXCEPTION_TAG(array_type_bad_lower);
DEFINE_EXCEPTION_TAG(array_type_bad_upper);
DEFINE_EXCEPTION_TAG(range_type_bad_lower);
DEFINE_EXCEPTION_TAG(range_type_bad_upper);
DEFINE_EXCEPTION_TAG(lepton_type_bad_key);
DEFINE_EXCEPTION_TAG(routine_addressed);
DEFINE_EXCEPTION_TAG(label_addressed);
DEFINE_EXCEPTION_TAG(tagalong_key_addressed);
DEFINE_EXCEPTION_TAG(lepton_key_addressed);
DEFINE_EXCEPTION_TAG(quark_addressed);
DEFINE_EXCEPTION_TAG(lock_addressed);
DEFINE_EXCEPTION_TAG(tagalong_unset);
DEFINE_EXCEPTION_TAG(tagalong_base_undefined);
DEFINE_EXCEPTION_TAG(tagalong_type_mismatch);
DEFINE_EXCEPTION_TAG(tagalong_type_match_indeterminate);
DEFINE_EXCEPTION_TAG(non_pointer_no_overloaded_match);
DEFINE_EXCEPTION_TAG(multi_lookup_bad_key);
DEFINE_EXCEPTION_TAG(lookup_bad_lower);
DEFINE_EXCEPTION_TAG(lookup_bad_upper);
DEFINE_EXCEPTION_TAG(overloading_resolution_no_match);
DEFINE_EXCEPTION_TAG(stack_overflow);
DEFINE_EXCEPTION_TAG(call_no_routine);
DEFINE_EXCEPTION_TAG(call_class_procedure);
DEFINE_EXCEPTION_TAG(call_argument_type_mismatch);
DEFINE_EXCEPTION_TAG(call_argument_type_match_indeterminate);
DEFINE_EXCEPTION_TAG(call_dynamic_parameter_type_mismatch);
DEFINE_EXCEPTION_TAG(call_dynamic_parameter_type_match_indeterminate);
DEFINE_EXCEPTION_TAG(call_side_effect_external_from_pure);
DEFINE_EXCEPTION_TAG(if_bad_test);
DEFINE_EXCEPTION_TAG(switch_indeterminate);
DEFINE_EXCEPTION_TAG(goto_bad_target);
DEFINE_EXCEPTION_TAG(goto_target_deallocated);
DEFINE_EXCEPTION_TAG(return_target_deallocated);
DEFINE_EXCEPTION_TAG(for_bad_initial);
DEFINE_EXCEPTION_TAG(for_bad_step);
DEFINE_EXCEPTION_TAG(for_bad_test);
DEFINE_EXCEPTION_TAG(iteration_base_array_indeterminate);
DEFINE_EXCEPTION_TAG(iteration_bad_base);
DEFINE_EXCEPTION_TAG(iterator_bad_is_done);
DEFINE_EXCEPTION_TAG(iterator_bad_current);
DEFINE_EXCEPTION_TAG(iterator_bad_step);
DEFINE_EXCEPTION_TAG(iterate_bad_test);
DEFINE_EXCEPTION_TAG(while_bad_test);
DEFINE_EXCEPTION_TAG(do_while_bad_test);
DEFINE_EXCEPTION_TAG(single_lock_not_lock);
DEFINE_EXCEPTION_TAG(try_handle_bad_handler);
DEFINE_EXCEPTION_TAG(export_label);
DEFINE_EXCEPTION_TAG(use_type_match_indeterminate);
DEFINE_EXCEPTION_TAG(use_type_mismatch);
DEFINE_EXCEPTION_TAG(use_bad_base);
DEFINE_EXCEPTION_TAG(use_missing_field);
DEFINE_EXCEPTION_TAG(use_not_executed);
DEFINE_EXCEPTION_TAG(use_multiply_instantiated);
DEFINE_EXCEPTION_TAG(use_unbound_remains);
DEFINE_EXCEPTION_TAG(cleanup_jump);
DEFINE_EXCEPTION_TAG(variable_lock_not_lock);
DEFINE_EXCEPTION_TAG(routine_lock_not_lock);
DEFINE_EXCEPTION_TAG(tagalong_lock_not_lock);
DEFINE_EXCEPTION_TAG(lock_single_lock_not_lock);
DEFINE_EXCEPTION_TAG(initialize_variable_mismatch);
DEFINE_EXCEPTION_TAG(initialize_variable_match_indeterminate);
DEFINE_EXCEPTION_TAG(tagalong_default_mismatch);
DEFINE_EXCEPTION_TAG(tagalong_default_match_indeterminate);
DEFINE_EXCEPTION_TAG(variable_declaration_re_executed);
DEFINE_EXCEPTION_TAG(routine_declaration_re_executed);
DEFINE_EXCEPTION_TAG(tagalong_declaration_re_executed);
DEFINE_EXCEPTION_TAG(lepton_key_declaration_re_executed);
DEFINE_EXCEPTION_TAG(quark_declaration_re_executed);
DEFINE_EXCEPTION_TAG(lock_declaration_re_executed);
DEFINE_EXCEPTION_TAG(read_variable_uninstantiated);
DEFINE_EXCEPTION_TAG(read_variable_deallocated);
DEFINE_EXCEPTION_TAG(read_variable_undefined);
DEFINE_EXCEPTION_TAG(bad_operands);
DEFINE_EXCEPTION_TAG(field_undefined);
DEFINE_EXCEPTION_TAG(object_no_field);
DEFINE_EXCEPTION_TAG(object_write_non_data_field);
DEFINE_EXCEPTION_TAG(field_read_bad_base);
DEFINE_EXCEPTION_TAG(basket_read_no_component);
DEFINE_EXCEPTION_TAG(immutable_modification);
DEFINE_EXCEPTION_TAG(write_variable_uninstantiated);
DEFINE_EXCEPTION_TAG(write_variable_deallocated);
DEFINE_EXCEPTION_TAG(assign_multiple_too_few);
DEFINE_EXCEPTION_TAG(assign_multiple_undefined);
DEFINE_EXCEPTION_TAG(assign_multiple_unordered);
DEFINE_EXCEPTION_TAG(assign_multiple_not_compound);
DEFINE_EXCEPTION_TAG(write_by_name_missing_field);
DEFINE_EXCEPTION_TAG(assignment_type_mismatch);
DEFINE_EXCEPTION_TAG(assignment_type_match_indeterminate);
DEFINE_EXCEPTION_TAG(concatenation_array_indeterminate);
DEFINE_EXCEPTION_TAG(concatenation_non_array_map);
DEFINE_EXCEPTION_TAG(concatenation_sparse_array);
DEFINE_EXCEPTION_TAG(routine_type_uninstantiated);
DEFINE_EXCEPTION_TAG(routine_type_deallocated);
DEFINE_EXCEPTION_TAG(function_type_non_pure);
DEFINE_EXCEPTION_TAG(function_type_return_type_mismatch);
DEFINE_EXCEPTION_TAG(function_type_return_type_match_indeterminate);
DEFINE_EXCEPTION_TAG(function_type_no_arguments);
DEFINE_EXCEPTION_TAG(function_type_argument_type_mismatch);
DEFINE_EXCEPTION_TAG(function_type_argument_type_match_indeterminate);
DEFINE_EXCEPTION_TAG(function_type_too_many_arguments);
DEFINE_EXCEPTION_TAG(function_type_resolution_indeterminate);
DEFINE_EXCEPTION_TAG(type_bad_value);
DEFINE_EXCEPTION_TAG(lepton_key_uninstantiated);
DEFINE_EXCEPTION_TAG(lepton_key_deallocated);
DEFINE_EXCEPTION_TAG(pointer_integer_addition_non_lookup);


#endif /* EXECUTE_H */
