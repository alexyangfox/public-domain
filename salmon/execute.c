/* file "execute.c" */

/*
 *  This file contains the implementation of the execute module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <assert.h>
#include <string.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/string_index.h"
#include "c_foundations/trace.h"
#include "c_foundations/auto_array.h"
#include "c_foundations/auto_array_implementation.h"
#include "execute.h"
#include "expression.h"
#include "context.h"
#include "value.h"
#include "call.h"
#include "statement.h"
#include "statement_block.h"
#include "lookup_actual_arguments.h"
#include "slot_location.h"
#include "basket_instance.h"
#include "lepton_key_instance.h"
#include "object.h"
#include "trace_channels.h"
#include "virtual_lookup.h"
#include "use_instance.h"
#include "driver.h"
#include "lock_instance.h"
#include "profile.h"
#include "parser.h"
#include "utility.h"
#include "platform_dependent.h"


AUTO_ARRAY(statement_aa, statement *);


typedef struct
  {
    slot_location *the_slot_location;
    value *(*update_function)(void *update_data, value *existing_value,
            value *new_value, const source_location *the_source_location,
            jumper *the_jumper);
    void *update_data;
    boolean care_about_existing_value;
  } update_data_info;

typedef struct
  {
    value *base_value;
    const source_location *base_location;
    o_integer index_oi;
    o_integer max_oi;
    o_integer step_oi;
    const char *action_description;
    const char *loop_description;
  } simple_iteration_data;

typedef struct
  {
    expression *body;
    expression *filter;
    variable_instance *element_instance;
    context *context;
  } comprehend_data;


AUTO_ARRAY_IMPLEMENTATION(lock_instance_aa, lock_instance *, 0);


static boolean profiling = FALSE;
static expression *comprehend_iterator_expression = NULL;


static value *execute_call_from_values_with_virtuals2(
        routine_instance *the_routine_instance,
        semi_labeled_value_list *pre_order_actuals,
        boolean expect_return_value, virtual_lookup *virtual_parent,
        jumper *the_jumper, const source_location *location);
static value *execute_call_from_values_with_virtuals3(
        routine_declaration *declaration,
        routine_instance *the_routine_instance,
        semi_labeled_value_list *pre_order_actuals,
        boolean expect_return_value, virtual_lookup *virtual_parent,
        jumper *the_jumper, const source_location *location);
static value *execute_call_from_values_with_virtuals4(
        routine_declaration *declaration,
        routine_instance *the_routine_instance,
        semi_labeled_value_list *pre_order_actuals,
        boolean expect_return_value, virtual_lookup *virtual_parent,
        jumper *the_jumper, const source_location *location);
static void require_numeric(const source_location *location,
        expression_kind kind, const char *operand_description,
        value *the_value, jumper *the_jumper);
static void require_integer(const source_location *location,
        expression_kind kind, const char *operand_description,
        value *the_value, jumper *the_jumper);
static void require_boolean(const source_location *location,
        expression_kind kind, const char *operand_description,
        value *the_value, jumper *the_jumper);
static void require_numeric_or_pointer(const source_location *location,
        expression_kind kind, const char *operand_description,
        value *the_value, jumper *the_jumper);
static const char *expression_kind_name(expression_kind kind);
static const char *value_kind_name(value *the_value);
static value *read_field_value(value *base_value, const char *field_name,
        const source_location *location, jumper *the_jumper);
static value *read_from_basket_instance(basket_instance *the_basket_instance,
        lock_instance_aa *delayed_unlocks, const source_location *location,
        jumper *the_jumper);
static void write_to_basket_instance(basket_instance *the_basket_instance,
        value *new_value, boolean force_to_type,
        const source_location *location, jumper *the_jumper);
static value *check_value_type_and_possibly_force(value *the_value,
        type *type_lower_bound, type *type_upper_bound, boolean force_to_type,
        static_exception_tag *mismatch_tag,
        static_exception_tag *match_indeterminate_tag,
        const char *action_description, const source_location *location,
        jumper *the_jumper);
static void get_and_force_slot_contents(slot_location *the_slot_location,
        const source_location *the_source_location,
        value *(*update_function)(void *update_data, value *existing_value,
                value *new_value, const source_location *the_source_location,
                jumper *the_jumper), void *update_data,
        boolean care_about_existing_value, value *new_value,
        jumper *the_jumper);
static value *replace_update_function(void *update_data, value *existing_value,
        value *new_value, const source_location *the_source_location,
        jumper *the_jumper);
static value *need_lookup_update_function(void *update_data,
        value *existing_value, value *new_value,
        const source_location *the_source_location, jumper *the_jumper);
static value *need_field_update_function(void *update_data,
        value *existing_value, value *new_value,
        const source_location *the_source_location, jumper *the_jumper);
static value *need_tagalong_update_function(void *update_data,
        value *existing_value, value *new_value,
        const source_location *the_source_location, jumper *the_jumper);
static value *need_pass_update_function(void *update_data,
        value *existing_value, value *new_value,
        const source_location *the_source_location, jumper *the_jumper);
static void concatenate_onto_semi_labeled_value_list(value *result_value,
        value *addition, const source_location *location, jumper *the_jumper);
static type *evaluate_expression_as_type_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
static rational *rational_from_integer_or_rational_value(value *the_value);
static variable_instance *variable_reference_expression_instance(
        expression *the_expression, context *the_context);
static void export_field_to_object(object *source_object,
        size_t source_field_num, object *target_object,
        const char *target_name, jumper *the_jumper);
static type *check_and_get_lepton_type(lepton_key_instance *key,
        size_t field_count, type **field_types, const char **field_names,
        boolean extra_fields_allowed, const source_location *location,
        jumper *the_jumper);
static value *add_slot_location_and_integer(slot_location *the_slot,
        o_integer the_oi, const source_location *location, jumper *the_jumper);
static void do_variable_lock_grabbing(variable_instance *variable_instance,
        const source_location *location, jumper *the_jumper);
static void do_variable_lock_releasing(variable_instance *variable_instance,
        const source_location *location, jumper *the_jumper);
static void set_variable_value_with_locking(
        variable_instance *variable_instance, value *new_value,
        const source_location *location, jumper *the_jumper);
static lock_chain *declaration_lock_chain(object *base_object,
        expression *lock_expression, context *the_context, jumper *the_jumper,
        const char *declaration_kind_name, static_exception_tag *not_lock_tag);
static verdict try_scoped_overloading(
        routine_declaration_chain *overload_chain,
        statement *overload_use_statement, size_t overload_used_for_number,
        value **result, size_t argument_count, value **arguments,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static verdict try_scoped_field_overloading(value *overload_base,
        value **result, value *base_value, const char *field_name,
        value *new_value, jumper *the_jumper,
        const source_location *the_source_location);
static verdict try_scoped_dereference_overloading(value *overload_base,
        value **result, value *base_value, value *new_value,
        jumper *the_jumper, const source_location *the_source_location);
static value *find_overload_operator(value *base_value,
        const char *routine_name, jumper *the_jumper,
        const source_location *location);
static value *overload_base_for_expression(expression *the_expression,
        context *the_context, jumper *the_jumper);
static verdict start_simple_iteration_data(value *base_value,
        simple_iteration_data *simple_data, const char *action_description,
        const char *loop_description, jumper *the_jumper,
        const source_location *base_location);
static boolean simple_iteration_data_is_done(simple_iteration_data *data);
static value *simple_iteration_data_current(simple_iteration_data *data,
        jumper *the_jumper, const source_location *location);
static void simple_iteration_data_step(simple_iteration_data *data,
                                       jumper *the_jumper);
static void clean_up_simple_iteration_data(simple_iteration_data *data);
static variable_instance *immutable_for_value(value *the_value,
        const char *name, context *the_context,
        const source_location *location, jumper *the_jumper);
static void comprehend_cleaner(void *hook, jumper *the_jumper);
static value *comprehend_transform_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static formal_arguments *create_formals(size_t count, const char **names,
                                        const source_location *location);


extern verdict init_execute_module(void)
  {
    assert(comprehend_iterator_expression == NULL);
    comprehend_iterator_expression = parse_stand_alone_immediate_expression(
            "class(base, transform, magic)\n"
            "  {\n"
            "    class iterator()\n"
            "      {\n"
            "        hide;\n"
            "\n"
            "        immutable base_iterator := base.iterator();\n"
            "        variable done_flag;\n"
            "        variable current_holder;\n"
            "\n"
            "        procedure update()\n"
            "          {\n"
            "            done_flag := base_iterator.is_done();\n"
            "            if (!done_flag)\n"
            "              {\n"
            "                current_holder := transform(\n"
            "                        base_iterator.current(), magic,\n"
            "                        break_target, continue_target);\n"
            "              };\n"
            "            return;\n"
            "          continue_target:\n"
            "            step();\n"
            "            return;\n"
            "          break_target:\n"
            "            done_flag := (1 == 1);\n"
            "          };\n"
            "\n"
            "        export;\n"
            "\n"
            "        function is_done()\n"
            "          (done_flag);\n"
            "        function current()\n"
            "          (current_holder);\n"
            "        procedure step()\n"
            "          {\n"
            "            base_iterator.step();\n"
            "            update();\n"
            "          };\n"
            "\n"
            "        update();\n"
            "      };\n"
            "  }");
    if (comprehend_iterator_expression == NULL)
        return MISSION_FAILED;

    return MISSION_ACCOMPLISHED;
  }

extern void cleanup_execute_module(void)
  {
    assert(comprehend_iterator_expression != NULL);
    delete_expression(comprehend_iterator_expression);
    comprehend_iterator_expression = NULL;
  }

extern value *evaluate_expression(expression *the_expression,
                                  context *the_context, jumper *the_jumper)
  {
    return evaluate_expression_with_virtuals(the_expression, the_context, NULL,
                                             the_jumper);
  }

extern value *evaluate_expression_without_context(expression *the_expression,
                                                  jumper *the_jumper)
  {
    value *top_level_value;
    context *the_context;
    value *result;

    assert(the_expression != NULL);
    assert(the_jumper != NULL);

    top_level_value = create_semi_labeled_value_list_value();
    if (top_level_value == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    the_context = create_top_level_context(top_level_value);
    if (the_context == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(top_level_value, the_jumper);
        return NULL;
      }

    value_remove_reference(top_level_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        exit_context(the_context, the_jumper);
        return NULL;
      }

    result = evaluate_expression(the_expression, the_context, the_jumper);
    exit_context(the_context, the_jumper);

    return result;
  }

extern value *evaluate_expression_with_virtuals(expression *the_expression,
        context *the_context, virtual_lookup *virtual_parent,
        jumper *the_jumper)
  {
    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(jumper_flowing_forward(the_jumper));

    if (have_start_expression_evaluation_watcher(the_jumper))
      {
        do_thread_start_expression_evaluation_watchers(
                jumper_thread(the_jumper), the_expression, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return NULL;
      }

    switch (get_expression_kind(the_expression))
      {
        case EK_CONSTANT:
            return evaluate_constant_expression(the_expression, the_context,
                                                the_jumper);
        case EK_UNBOUND_NAME_REFERENCE:
            expression_exception(the_jumper, the_expression,
                    EXCEPTION_TAG(unbound_name_reference),
                    "An unbound name reference expression was evaluated.");
            return NULL;
        case EK_VARIABLE_REFERENCE:
            return evaluate_variable_reference_expression(the_expression,
                    the_context, the_jumper);
        case EK_ROUTINE_REFERENCE:
            return evaluate_routine_reference_expression(the_expression,
                    the_context, the_jumper);
        case EK_TAGALONG_REFERENCE:
            return evaluate_tagalong_reference_expression(the_expression,
                    the_context, the_jumper);
        case EK_LEPTON_KEY_REFERENCE:
            return evaluate_lepton_key_reference_expression(the_expression,
                    the_context, the_jumper);
        case EK_QUARK_REFERENCE:
            return evaluate_quark_reference_expression(the_expression,
                    the_context, the_jumper);
        case EK_LOCK_REFERENCE:
            return evaluate_lock_reference_expression(the_expression,
                                                      the_context, the_jumper);
        case EK_USE_REFERENCE:
            return evaluate_use_reference_expression(the_expression,
                                                     the_context, the_jumper);
        case EK_LABEL_REFERENCE:
            return evaluate_label_reference_expression(the_expression,
                    the_context, the_jumper);
        case EK_LOOKUP:
            return evaluate_lookup_expression(the_expression, the_context,
                                              the_jumper);
        case EK_LEPTON:
            return evaluate_lepton_expression(the_expression, the_context,
                                              the_jumper);
        case EK_FIELD:
            return evaluate_field_expression(the_expression, the_context,
                                             the_jumper);
        case EK_POINTER_FIELD:
            return evaluate_pointer_field_expression(the_expression,
                                                     the_context, the_jumper);
        case EK_TAGALONG_FIELD:
            return evaluate_tagalong_field_expression(the_expression,
                                                      the_context, the_jumper);
        case EK_STATEMENT_BLOCK:
            return evaluate_statement_block_expression(the_expression,
                    the_context, the_jumper);
        case EK_DECLARATION:
            return evaluate_declaration_expression(the_expression, the_context,
                                                   the_jumper);
        case EK_TYPE:
            return evaluate_type_expression_expression(the_expression,
                    the_context, the_jumper);
        case EK_MAP_LIST:
            return evaluate_map_list_expression(the_expression, the_context,
                                                the_jumper);
        case EK_SEMI_LABELED_EXPRESSION_LIST:
            return evaluate_semi_labeled_expression_list_expression(
                    the_expression, the_context, the_jumper);
        case EK_CALL:
            return evaluate_call_expression(the_expression, the_context,
                                            virtual_parent, the_jumper);
        case EK_CONDITIONAL:
            return evaluate_conditional_expression(the_expression, the_context,
                                                   the_jumper);
        case EK_DEREFERENCE:
        case EK_LOCATION_OF:
        case EK_NEGATE:
        case EK_UNARY_PLUS:
        case EK_BITWISE_NOT:
        case EK_LOGICAL_NOT:
            return evaluate_unary_expression(the_expression, the_context,
                                             the_jumper);
        case EK_ADD:
        case EK_SUBTRACT:
        case EK_MULTIPLY:
        case EK_DIVIDE:
        case EK_DIVIDE_FORCE:
        case EK_REMAINDER:
        case EK_SHIFT_LEFT:
        case EK_SHIFT_RIGHT:
        case EK_LESS_THAN:
        case EK_GREATER_THAN:
        case EK_LESS_THAN_OR_EQUAL:
        case EK_GREATER_THAN_OR_EQUAL:
        case EK_EQUAL:
        case EK_NOT_EQUAL:
        case EK_BITWISE_AND:
        case EK_BITWISE_OR:
        case EK_BITWISE_XOR:
        case EK_LOGICAL_AND:
        case EK_LOGICAL_OR:
        case EK_CONCATENATE:
            return evaluate_binary_expression(the_expression, the_context,
                                              the_jumper);
        case EK_ARGUMENTS:
            return evaluate_arguments_expression(the_expression, the_context,
                                                 the_jumper);
        case EK_THIS:
            return evaluate_this_expression(the_expression, the_context,
                                            the_jumper);
        case EK_IN:
            return evaluate_in_expression(the_expression, the_context,
                                          the_jumper);
        case EK_FORCE:
            return evaluate_force_expression(the_expression, the_context,
                                             the_jumper);
        case EK_BREAK:
            return evaluate_break_expression(the_expression, the_context,
                                             the_jumper);
        case EK_CONTINUE:
            return evaluate_continue_expression(the_expression, the_context,
                                                the_jumper);
        case EK_COMPREHEND:
            return evaluate_comprehend_expression(the_expression, the_context,
                                                  the_jumper);
        case EK_FORALL:
            return evaluate_forall_expression(the_expression, the_context,
                                              the_jumper);
        case EK_EXISTS:
            return evaluate_exists_expression(the_expression, the_context,
                                              the_jumper);
        default:
            assert(FALSE);
            return NULL;
      }
  }

extern value *evaluate_constant_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    value *the_value;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_CONSTANT);
    the_value = constant_expression_value(the_expression);
    assert(the_value != NULL);

    value_add_reference(the_value);
    return the_value;
  }

extern value *evaluate_variable_reference_expression(
        expression *the_expression, context *the_context, jumper *the_jumper)
  {
    variable_instance *instance;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_VARIABLE_REFERENCE);

    instance = variable_reference_expression_instance(the_expression,
                                                      the_context);
    assert(instance != NULL);

    return read_variable_value(instance, NULL,
            get_expression_location(the_expression), the_jumper);
  }

extern value *evaluate_routine_reference_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    routine_instance_chain *instance_chain;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_ROUTINE_REFERENCE);

    instance_chain = routine_declaration_chain_to_routine_instance_chain(
            routine_reference_expression_chain(the_expression), the_context,
            the_jumper);
    if (instance_chain == NULL)
      {
        assert(!(jumper_flowing_forward(the_jumper)));
        return NULL;
      }
    assert(jumper_flowing_forward(the_jumper));

    result = create_routine_chain_value(instance_chain);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    routine_instance_chain_remove_reference(instance_chain, the_jumper);

    return result;
  }

extern value *evaluate_tagalong_reference_expression(
        expression *the_expression, context *the_context, jumper *the_jumper)
  {
    tagalong_key *instance;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_TAGALONG_REFERENCE);

    instance = find_tagalong_instance(the_context,
            tagalong_reference_expression_declaration(the_expression));
    assert(instance != NULL);

    result = create_tagalong_key_value(instance);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    return result;
  }

extern value *evaluate_lepton_key_reference_expression(
        expression *the_expression, context *the_context, jumper *the_jumper)
  {
    lepton_key_instance *instance;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_LEPTON_KEY_REFERENCE);

    instance = find_lepton_key_instance(the_context,
            lepton_key_reference_expression_declaration(the_expression));
    assert(instance != NULL);

    result = create_lepton_key_value(instance);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    return result;
  }

extern value *evaluate_quark_reference_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    quark *instance;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_QUARK_REFERENCE);

    instance = find_quark_instance(the_context,
            quark_reference_expression_declaration(the_expression));
    assert(instance != NULL);

    result = create_quark_value(instance);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    return result;
  }

extern value *evaluate_lock_reference_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    lock_instance *instance;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_LOCK_REFERENCE);

    instance = find_lock_instance(the_context,
            lock_reference_expression_declaration(the_expression));
    assert(instance != NULL);

    result = create_lock_value(instance);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    return result;
  }

extern value *evaluate_use_reference_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    instance *the_instance;
    routine_instance_chain *instance_chain;
    jump_target *the_jump_target;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_USE_REFERENCE);

    find_instance_from_use_statement(
            use_reference_expression_use_statement(the_expression),
            use_reference_expression_used_for_num(the_expression), the_context,
            TRUE, &the_instance, &instance_chain, &the_jump_target,
            get_expression_location(the_expression), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_instance == NULL);
        assert(instance_chain == NULL);
        return NULL;
      }

    if (the_instance != NULL)
      {
        assert(instance_chain == NULL);
        assert(the_jump_target == NULL);

        switch (instance_kind(the_instance))
          {
            case NK_VARIABLE:
                return read_variable_value(
                        instance_variable_instance(the_instance), NULL,
                        get_expression_location(the_expression), the_jumper);
            case NK_ROUTINE:
                result = create_routine_value(
                        instance_routine_instance(the_instance));
                break;
            case NK_TAGALONG:
                result = create_tagalong_key_value(
                        instance_tagalong_instance(the_instance));
                break;
            case NK_LEPTON_KEY:
                result = create_lepton_key_value(
                        instance_lepton_key_instance(the_instance));
                break;
            case NK_QUARK:
                result = create_quark_value(
                        instance_quark_instance(the_instance));
                break;
            case NK_LOCK:
                result = create_lock_value(
                        instance_lock_instance(the_instance));
                break;
            default:
                assert(FALSE);
                result = NULL;
          }

        if (result == NULL)
            jumper_do_abort(the_jumper);
      }
    else if (instance_chain != NULL)
      {
        assert(the_jump_target == NULL);

        result = create_routine_chain_value(instance_chain);
        if (result == NULL)
            jumper_do_abort(the_jumper);
        routine_instance_chain_remove_reference(instance_chain, the_jumper);
      }
    else
      {
        assert(the_jump_target != NULL);

        result = create_jump_target_value(the_jump_target);
        if (result == NULL)
            jumper_do_abort(the_jumper);
      }

    return result;
  }

extern value *evaluate_label_reference_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    jump_target *instance;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_LABEL_REFERENCE);

    instance = find_label_instance(the_context,
            label_reference_expression_declaration(the_expression));
    assert(instance != NULL);

    result = create_jump_target_value(instance);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    return result;
  }

extern value *evaluate_lookup_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    value *base_value;
    lookup_actual_arguments *actuals;
    value *overload_base;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_LOOKUP);

    base_value = evaluate_expression(lookup_expression_base(the_expression),
                                     the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(base_value == NULL);
        return NULL;
      }

    assert(base_value != NULL);

    actuals =
            evaluate_lookup_arguments(the_expression, the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(actuals == NULL);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    assert(actuals != NULL);

    overload_base = overload_base_for_expression(the_expression, the_context,
                                                 the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(overload_base == NULL);
        delete_lookup_actual_arguments(actuals, the_jumper);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    check_lookup_actual_arguments_validity(actuals,
            get_expression_location(the_expression), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (overload_base != NULL)
            value_remove_reference(overload_base, the_jumper);
        delete_lookup_actual_arguments(actuals, the_jumper);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
    result = do_lookup(base_value, actuals, overload_base,
                       get_expression_location(the_expression), the_jumper);
    if (overload_base != NULL)
        value_remove_reference(overload_base, the_jumper);
    value_remove_reference(base_value, the_jumper);
    delete_lookup_actual_arguments(actuals, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        value_remove_reference(result, the_jumper);
        result = NULL;
      }
    return result;
  }

extern value *evaluate_lepton_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    value *base_value;
    lepton_key_instance *key_instance;
    lepton_key_declaration *key_declaration;
    value *lepton_value;
    size_t field_count;
    size_t field_num;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_LEPTON);

    base_value = evaluate_expression(lepton_expression_base(the_expression),
                                     the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(base_value == NULL);
        return NULL;
      }

    assert(base_value != NULL);

    check_value_validity(base_value, get_expression_location(the_expression),
                         the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    if (get_value_kind(base_value) != VK_LEPTON_KEY)
      {
        expression_exception(the_jumper, the_expression,
                EXCEPTION_TAG(lepton_bad_key),
                "The key expression of a lepton expression evaluated to a %s, "
                "not a lepton key.", value_kind_name(base_value));
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    key_instance = value_lepton_key(base_value);
    lepton_key_instance_add_reference(key_instance);
    value_remove_reference(base_value, the_jumper);
    assert(jumper_flowing_forward(the_jumper));

    assert(lepton_key_instance_is_instantiated(key_instance)); /* VERIFIED */
    assert(!(lepton_key_instance_scope_exited(key_instance))); /* VERIFIED */

    key_declaration = lepton_key_instance_declaration(key_instance);

    lepton_value = create_lepton_value(key_instance);
    if (lepton_value == NULL)
      {
        jumper_do_abort(the_jumper);
        lepton_key_instance_remove_reference(key_instance, the_jumper);
        return NULL;
      }

    lepton_key_instance_remove_reference(key_instance, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    field_count = lepton_expression_component_count(the_expression);

    for (field_num = 0; field_num < field_count; ++field_num)
      {
        value *field_value;
        const char *field_name;
        size_t key_field_num;
        verdict the_verdict;

        field_value = evaluate_expression(
                lepton_expression_component_child_expression(the_expression,
                                                             field_num),
                the_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(field_value == NULL);
            value_remove_reference(lepton_value, the_jumper);
            return NULL;
          }

        assert(field_value != NULL);

        if (lepton_key_instance_scope_exited(key_instance))
          {
            expression_exception(the_jumper, the_expression,
                    EXCEPTION_TAG(lepton_key_deleted_in_expression),
                    "While evaluating a lepton expression, the key for the "
                    "expression was deleted while evaluating one of the "
                    "component value expressions.");
            value_remove_reference(field_value, the_jumper);
            value_remove_reference(lepton_value, the_jumper);
            return NULL;
          }

        field_name =
                lepton_expression_component_label(the_expression, field_num);
        assert(field_name != NULL);

        key_field_num =
                lepton_key_lookup_field_by_name(key_declaration, field_name);

        if (key_field_num < lepton_key_field_count(key_declaration))
          {
            type *field_type;
            boolean is_in;
            boolean doubt;
            char *why_not;

            assert(lepton_key_instance_is_instantiated(key_instance));
                    /* VERIFIED */
            assert(!(lepton_key_instance_scope_exited(key_instance)));
                    /* VERIFIED */

            field_type = lepton_key_instance_field_type(key_instance,
                                                        key_field_num);
            assert(field_type != NULL);

            check_type_validity(field_type,
                    get_expression_location(the_expression), the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(field_value, the_jumper);
                value_remove_reference(lepton_value, the_jumper);
                return NULL;
              }

            assert(type_is_valid(field_type)); /* VERIFIED */
            is_in = value_is_in_type(field_value, field_type, &doubt, &why_not,
                    get_expression_location(the_expression), the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(field_value, the_jumper);
                value_remove_reference(lepton_value, the_jumper);
                return NULL;
              }

            if (doubt)
              {
                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(lepton_field_type_match_indeterminate),
                        "While evaluating a lepton expression, %s was unable "
                        "to determine whether the field value for field `%s' "
                        "was in the lepton's type for that field because %s.",
                        interpreter_name(), field_name, why_not);
                free(why_not);
                value_remove_reference(field_value, the_jumper);
                value_remove_reference(lepton_value, the_jumper);
                return NULL;
              }

            if (!is_in)
              {
                if (lepton_expression_component_force(the_expression,
                                                      field_num))
                  {
                    value *forced_value;

                    free(why_not);
                    check_value_validity(field_value,
                            get_expression_location(the_expression),
                            the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(field_value, the_jumper);
                        value_remove_reference(lepton_value, the_jumper);
                        return NULL;
                      }

                    assert(value_is_valid(field_value)); /* VERIFIED */
                    assert(type_is_valid(field_type)); /* VERIFIED */
                    forced_value = force_value_to_type(field_value, field_type,
                            get_expression_location(the_expression),
                            the_jumper);
                    value_remove_reference(field_value, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        if (forced_value != NULL)
                            value_remove_reference(forced_value, the_jumper);
                        value_remove_reference(lepton_value, the_jumper);
                        return NULL;
                      }

                    if (forced_value == NULL)
                      {
                        expression_exception(the_jumper, the_expression,
                                EXCEPTION_TAG(lepton_field_type_cant_force),
                                "While evaluating a lepton expression, the "
                                "field value for field `%s' couldn't be forced"
                                " into the lepton's type for that field.",
                                field_name);
                        value_remove_reference(lepton_value, the_jumper);
                        return NULL;
                      }

                    field_value = forced_value;
                  }
                else
                  {
                    expression_exception(the_jumper, the_expression,
                            EXCEPTION_TAG(lepton_field_type_mismatch),
                            "While evaluating a lepton expression, the field "
                            "value for field `%s' wasn't in the lepton's type "
                            "for that field because %s.", field_name, why_not);
                    free(why_not);
                    value_remove_reference(field_value, the_jumper);
                    value_remove_reference(lepton_value, the_jumper);
                    return NULL;
                  }
              }
          }
        else
          {
            if (!(lepton_key_declaration_additional_fields_allowed(
                          key_declaration)))
              {
                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(lepton_field_disallowed),
                        "While evaluating a lepton expression, a field named "
                        "`%s' was encountered, but the lepton key doesn't "
                        "allow fields with that name.", field_name);
                value_remove_reference(field_value, the_jumper);
                value_remove_reference(lepton_value, the_jumper);
                return NULL;
              }
          }

        the_verdict = add_field(lepton_value, field_name, field_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(field_value, the_jumper);
            value_remove_reference(lepton_value, the_jumper);
            return NULL;
          }

        value_remove_reference(field_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(lepton_value, the_jumper);
            return NULL;
          }
      }

    return lepton_value;
  }

extern value *evaluate_field_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    value *base_value;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_FIELD);

    base_value = evaluate_expression(field_expression_base(the_expression),
                                     the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(base_value == NULL);
        return NULL;
      }

    assert(base_value != NULL);

    result = read_field_value(base_value,
            field_expression_field_name(the_expression),
            get_expression_location(the_expression), the_jumper);
    value_remove_reference(base_value, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        value_remove_reference(result, the_jumper);
        return NULL;
      }
    return result;
  }

extern value *evaluate_pointer_field_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    value *base_value;
    slot_location *the_slot;
    value *indirect_value;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_POINTER_FIELD);

    base_value = evaluate_expression(
            pointer_field_expression_base(the_expression), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(base_value == NULL);
        return NULL;
      }

    assert(base_value != NULL);

    if ((get_value_kind(base_value) != VK_SLOT_LOCATION) ||
        (expression_overload_chain(the_expression) != NULL) ||
        (expression_overload_use_statement(the_expression) != NULL))
      {
        value *field_name_value;
        value *arguments[2];
        verdict the_verdict;

        field_name_value = create_string_value(
                pointer_field_expression_field_name(the_expression));
        if (field_name_value == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        arguments[0] = base_value;
        arguments[1] = field_name_value;
        the_verdict = try_scoped_overloading(
                expression_overload_chain(the_expression),
                expression_overload_use_statement(the_expression),
                expression_overload_use_used_for_number(the_expression),
                &result, 2, &(arguments[0]), the_context, the_jumper,
                get_expression_location(the_expression));
        if (the_verdict == MISSION_ACCOMPLISHED)
          {
            value_remove_reference(field_name_value, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return result;
          }

        arguments[0] = field_name_value;
        the_verdict = try_overloading(base_value, "operator->", &result, 1,
                &(arguments[0]), NULL, the_jumper,
                get_expression_location(the_expression));
        value_remove_reference(field_name_value, the_jumper);
        if (the_verdict == MISSION_ACCOMPLISHED)
          {
            value_remove_reference(base_value, the_jumper);
            return result;
          }
      }

    if (get_value_kind(base_value) != VK_SLOT_LOCATION)
      {
        expression_exception(the_jumper, the_expression,
                EXCEPTION_TAG(pointer_field_reference_bad_base),
                "The base operand of a pointer field access evaluated to "
                "something other than a slot location.");
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    the_slot = slot_location_value_data(base_value);
    assert(the_slot != NULL);

    indirect_value = get_slot_contents(the_slot, NULL,
            get_expression_location(the_expression), the_jumper);
    value_remove_reference(base_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (indirect_value != NULL)
            value_remove_reference(indirect_value, the_jumper);
        return NULL;
      }

    assert(indirect_value != NULL);

    result = read_field_value(indirect_value,
            pointer_field_expression_field_name(the_expression),
            get_expression_location(the_expression), the_jumper);
    value_remove_reference(indirect_value, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        value_remove_reference(result, the_jumper);
        return NULL;
      }
    return result;
  }

extern value *evaluate_tagalong_field_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    value *base_value;
    value *key_value;
    tagalong_key *the_tagalong_key;
    lock_chain *the_lock_chain;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_TAGALONG_FIELD);

    base_value = evaluate_expression(
            tagalong_field_expression_base(the_expression), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(base_value == NULL);
        return NULL;
      }

    assert(base_value != NULL);

    key_value = evaluate_expression(
            tagalong_field_expression_key(the_expression), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(key_value == NULL);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    assert(key_value != NULL);

    check_value_validity(key_value, get_expression_location(the_expression),
                         the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(key_value, the_jumper);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    if (get_value_kind(key_value) != VK_TAGALONG_KEY)
      {
        expression_exception(the_jumper, the_expression,
                EXCEPTION_TAG(tagalong_reference_bad_key),
                "The key operand for a tagalong field access evaluated to "
                "something other than a tagalong key reference.");
        value_remove_reference(key_value, the_jumper);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    the_tagalong_key = tagalong_key_value_data(key_value);
    assert(the_tagalong_key != NULL);

    assert(tagalong_key_is_instantiated(the_tagalong_key)); /* VERIFIED */
    assert(!(tagalong_key_scope_exited(the_tagalong_key))); /* VERIFIED */
    the_lock_chain = tagalong_key_lock_chain(the_tagalong_key);

    if (the_lock_chain != NULL)
      {
        lock_chain_grab(the_lock_chain,
                        get_expression_location(the_expression), the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(key_value, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }
      }

    assert(tagalong_key_is_instantiated(the_tagalong_key)); /* VERIFIED */
    assert(!(tagalong_key_scope_exited(the_tagalong_key))); /* VERIFIED */
    result = lookup_tagalong(base_value, the_tagalong_key, FALSE,
            get_expression_location(the_expression), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(result == NULL);
        value_remove_reference(base_value, the_jumper);
        if (the_lock_chain != NULL)
          {
            lock_chain_release(the_lock_chain,
                    get_expression_location(the_expression), the_jumper);
          }
        value_remove_reference(key_value, the_jumper);
        return NULL;
      }
    if (result == NULL)
      {
        assert(tagalong_key_is_instantiated(the_tagalong_key)); /* VERIFIED */
        assert(!(tagalong_key_scope_exited(the_tagalong_key))); /* VERIFIED */

        result = tagalong_key_default_value(the_tagalong_key);
        if (result == NULL)
          {
            expression_exception(the_jumper, the_expression,
                    EXCEPTION_TAG(tagalong_reference_undefined),
                    "A tagalong field reference using a key without a default "
                    "value accessed an undefined value.");
            value_remove_reference(base_value, the_jumper);
            if (the_lock_chain != NULL)
              {
                lock_chain_release(the_lock_chain,
                        get_expression_location(the_expression), the_jumper);
              }
            value_remove_reference(key_value, the_jumper);
            return NULL;
          }
      }

    value_remove_reference(base_value, the_jumper);
    value_remove_reference(key_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (the_lock_chain != NULL)
          {
            lock_chain_release(the_lock_chain,
                    get_expression_location(the_expression), the_jumper);
          }
        return NULL;
      }

    if (the_lock_chain != NULL)
      {
        lock_chain_release(the_lock_chain,
                get_expression_location(the_expression), the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return NULL;
      }

    value_add_reference(result);

    return result;
  }

extern value *evaluate_statement_block_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    context *block_context;
    value *return_value;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_STATEMENT_BLOCK);

    block_context =
            create_block_expression_context(the_context, the_expression);
    if (block_context == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    execute_statement_block(statement_block_expression_block(the_expression),
                            block_context, the_jumper);
    return_value = block_expression_context_return_value(block_context);

    if (jumper_target(the_jumper) ==
        block_expression_context_return_target(block_context))
      {
        jumper_reached_target(the_jumper);
      }

    exit_context(block_context, the_jumper);

    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (return_value != NULL)
            value_remove_reference(return_value, the_jumper);
        return NULL;
      }

    if (return_value == NULL)
      {
        expression_exception(the_jumper, the_expression,
                EXCEPTION_TAG(statement_block_expression_no_value),
                "Executing the statement block of a statement block expression"
                " didn't return a value.");
        return NULL;
      }

    return return_value;
  }

extern value *evaluate_declaration_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    declaration *the_declaration;
    instance *the_instance;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    the_declaration = declaration_expression_declaration(the_expression);
    assert(the_declaration != NULL);

    the_instance = create_instance_for_declaration(the_declaration,
            jumper_purity_level(the_jumper), NULL);
    if (the_instance == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    execute_declaration(the_declaration, the_instance, the_context, the_jumper,
                        NULL);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        switch (declaration_kind(the_declaration))
          {
            case NK_VARIABLE:
              {
                variable_instance_remove_reference(
                        instance_variable_instance(the_instance), the_jumper);
                return NULL;
              }
            case NK_ROUTINE:
              {
                routine_instance_remove_reference(
                        instance_routine_instance(the_instance), the_jumper);
                return NULL;
              }
            case NK_TAGALONG:
              {
                tagalong_key_remove_reference(
                        instance_tagalong_instance(the_instance), the_jumper);
                return NULL;
              }
            case NK_LEPTON_KEY:
              {
                lepton_key_instance_remove_reference(
                        instance_lepton_key_instance(the_instance),
                        the_jumper);
                return NULL;
              }
            case NK_QUARK:
              {
                quark_remove_reference(instance_quark_instance(the_instance),
                                       the_jumper);
                return NULL;
              }
            case NK_LOCK:
              {
                lock_instance_remove_reference(
                        instance_lock_instance(the_instance), the_jumper);
                return NULL;
              }
            default:
              {
                assert(FALSE);
                return NULL;
              }
          }
      }

    switch (declaration_kind(the_declaration))
      {
        case NK_VARIABLE:
          {
            variable_instance *the_variable_instance;
            slot_location *the_slot_location;

            the_variable_instance = instance_variable_instance(the_instance);
            assert(the_variable_instance != NULL);

            the_slot_location = create_variable_slot_location(
                    the_variable_instance,
                    get_declaration_location(the_declaration), the_jumper);
            if (the_slot_location == NULL)
              {
                assert(!(jumper_flowing_forward(the_jumper)));
                jumper_do_abort(the_jumper);
                variable_instance_remove_reference(the_variable_instance,
                                                   the_jumper);
                return NULL;
              }

            assert(jumper_flowing_forward(the_jumper));

            variable_instance_remove_reference(the_variable_instance,
                                               the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                slot_location_remove_reference(the_slot_location, the_jumper);
                return NULL;
              }

            result = create_slot_location_value(the_slot_location);
            if (result == NULL)
                jumper_do_abort(the_jumper);
            slot_location_remove_reference(the_slot_location, the_jumper);

            break;
          }
        case NK_ROUTINE:
          {
            routine_instance *the_routine_instance;

            the_routine_instance = instance_routine_instance(the_instance);
            assert(the_routine_instance != NULL);

            result = create_routine_value(the_routine_instance);
            if (result == NULL)
                jumper_do_abort(the_jumper);
            routine_instance_remove_reference(the_routine_instance,
                                              the_jumper);

            break;
          }
        case NK_TAGALONG:
          {
            tagalong_key *the_tagalong_key_instance;

            the_tagalong_key_instance =
                    instance_tagalong_instance(the_instance);
            assert(the_tagalong_key_instance != NULL);

            result = create_tagalong_key_value(the_tagalong_key_instance);
            if (result == NULL)
                jumper_do_abort(the_jumper);
            tagalong_key_remove_reference(the_tagalong_key_instance,
                                          the_jumper);

            break;
          }
        case NK_LEPTON_KEY:
          {
            lepton_key_instance *the_lepton_key_instance;

            the_lepton_key_instance =
                    instance_lepton_key_instance(the_instance);
            assert(the_lepton_key_instance != NULL);

            result = create_lepton_key_value(the_lepton_key_instance);
            if (result == NULL)
                jumper_do_abort(the_jumper);
            lepton_key_instance_remove_reference(the_lepton_key_instance,
                                                 the_jumper);

            break;
          }
        case NK_QUARK:
          {
            quark *the_quark_instance;

            the_quark_instance = instance_quark_instance(the_instance);
            assert(the_quark_instance != NULL);

            result = create_quark_value(the_quark_instance);
            if (result == NULL)
                jumper_do_abort(the_jumper);
            quark_remove_reference(the_quark_instance, the_jumper);

            break;
          }
        case NK_LOCK:
          {
            lock_instance *the_lock_instance;

            the_lock_instance = instance_lock_instance(the_instance);
            assert(the_lock_instance != NULL);

            result = create_lock_value(the_lock_instance);
            if (result == NULL)
                jumper_do_abort(the_jumper);
            lock_instance_remove_reference(the_lock_instance, the_jumper);

            break;
          }
        default:
          {
            assert(FALSE);
            result = NULL;
          }
      }

    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern value *evaluate_type_expression_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    type *the_type;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_TYPE);

    the_type = evaluate_type_expression(type_expression_type(the_expression),
                                        the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_type == NULL);
        return NULL;
      }

    assert(the_type != NULL);

    result = create_type_value(the_type);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    type_remove_reference(the_type, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern value *evaluate_map_list_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    value *map_value;
    size_t component_count;
    size_t component_num;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_MAP_LIST);

    map_value = create_map_value();
    if (map_value == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }
    assert(map_value_all_keys_are_valid(map_value)); /* VERIFIED */

    component_count = map_list_expression_component_count(the_expression);

    for (component_num = 0; component_num < component_count; ++component_num)
      {
        value *key_value;
        type *key_type;
        value *target_value;

        if (map_list_expression_is_filter(the_expression, component_num))
          {
            type_expression *key_expression;

            key_expression =
                    map_list_expression_filter(the_expression, component_num);
            key_type = evaluate_type_expression(key_expression, the_context,
                                                the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(key_type == NULL);
                value_remove_reference(map_value, the_jumper);
                return NULL;
              }

            assert(key_type != NULL);
            key_value = NULL;
          }
        else
          {
            expression *key_expression;

            key_expression =
                    map_list_expression_key(the_expression, component_num);
            key_value = evaluate_expression(key_expression, the_context,
                                            the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(key_value == NULL);
                value_remove_reference(map_value, the_jumper);
                return NULL;
              }

            assert(key_value != NULL);
            key_type = NULL;
          }

        target_value = evaluate_expression(
                map_list_expression_target(the_expression, component_num),
                the_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(target_value == NULL);
            if (key_value != NULL)
                value_remove_reference(key_value, the_jumper);
            else
                type_remove_reference(key_type, the_jumper);
            value_remove_reference(map_value, the_jumper);
            return NULL;
          }

        assert(target_value != NULL);

        validator_check_validity(map_value_all_keys_validator(map_value),
                get_expression_location(the_expression), the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(key_value, the_jumper);
            value_remove_reference(target_value, the_jumper);
            value_remove_reference(map_value, the_jumper);
            return NULL;
          }

        if (key_value != NULL)
          {
            check_value_validity(key_value,
                    get_expression_location(the_expression), the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(key_value, the_jumper);
                value_remove_reference(target_value, the_jumper);
                value_remove_reference(map_value, the_jumper);
                return NULL;
              }

            assert(map_value_all_keys_are_valid(map_value)); /* VERIFIED */
            assert(value_is_valid(key_value)); /* VERIFIED */
            map_value = map_value_set(map_value, key_value, target_value,
                    get_expression_location(the_expression), the_jumper);
            assert((map_value == NULL) ||
                   map_value_all_keys_are_valid(map_value)); /* VERIFIED */
            value_remove_reference(key_value, the_jumper);
          }
        else
          {
            check_type_validity(key_type,
                    get_expression_location(the_expression), the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                type_remove_reference(key_type, the_jumper);
                value_remove_reference(target_value, the_jumper);
                value_remove_reference(map_value, the_jumper);
                return NULL;
              }

            assert(map_value_all_keys_are_valid(map_value)); /* VERIFIED */
            assert(type_is_valid(key_type)); /* VERIFIED */
            map_value = map_value_set_filter(map_value, key_type, target_value,
                    get_expression_location(the_expression), the_jumper);
            type_remove_reference(key_type, the_jumper);
          }
        value_remove_reference(target_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(map_value == NULL);
            return NULL;
          }
        assert(map_value != NULL);
      }

    return map_value;
  }

extern value *evaluate_semi_labeled_expression_list_expression(
        expression *the_expression, context *the_context, jumper *the_jumper)
  {
    value *the_value;
    size_t component_count;
    size_t component_num;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    the_value = create_semi_labeled_value_list_value();
    if (the_value == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    component_count = semi_labeled_expression_list_expression_component_count(
            the_expression);

    for (component_num = 0; component_num < component_count; ++component_num)
      {
        expression *child_expression;
        value *child_value;
        const char *label;
        verdict the_verdict;

        child_expression =
                semi_labeled_expression_list_expression_child_expression(
                        the_expression, component_num);

        if (child_expression == NULL)
          {
            child_value = NULL;
          }
        else
          {
            child_value = evaluate_expression(child_expression, the_context,
                                              the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(the_value, the_jumper);
                return NULL;
              }
          }

        label = semi_labeled_expression_list_expression_label(the_expression,
                                                              component_num);

        if ((label != NULL) && value_get_field(label, the_value) != NULL)
          {
            if (child_value != NULL)
                value_remove_reference(child_value, the_jumper);
            value_remove_reference(the_value, the_jumper);
            expression_exception(the_jumper, the_expression,
                    EXCEPTION_TAG(
                            semi_labeled_expression_list_duplicate_label),
                    "A semi-labeled expression list expression specified more "
                    "than one value for the key `%s'.", label);
            return NULL;
          }

        the_verdict = add_field(the_value, label, child_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            if (child_value != NULL)
                value_remove_reference(child_value, the_jumper);
            value_remove_reference(the_value, the_jumper);
            return NULL;
          }

        if (child_value != NULL)
          {
            value_remove_reference(child_value, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(the_value, the_jumper);
                return NULL;
              }
          }
      }

    return the_value;
  }

extern value *evaluate_call_expression(expression *the_expression,
        context *the_context, virtual_lookup *virtual_parent,
        jumper *the_jumper)
  {
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_CALL);
    result = execute_call_with_virtuals(call_expression_call(the_expression),
            TRUE, expression_overload_chain(the_expression),
            expression_overload_use_statement(the_expression),
            expression_overload_use_used_for_number(the_expression),
            the_context, virtual_parent, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(result == NULL);
        return NULL;
      }

    assert(result != NULL);
    return result;
  }

extern value *evaluate_conditional_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    value *test_value;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_CONDITIONAL);
    test_value = evaluate_expression(
            conditional_expression_test(the_expression), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(test_value == NULL);
        return NULL;
      }

    assert(test_value != NULL);
    switch (get_value_kind(test_value))
      {
        case VK_TRUE:
            value_remove_reference(test_value, the_jumper);
            assert(jumper_flowing_forward(the_jumper));
            return evaluate_expression(
                    conditional_expression_then_part(the_expression),
                    the_context, the_jumper);
        case VK_FALSE:
            value_remove_reference(test_value, the_jumper);
            assert(jumper_flowing_forward(the_jumper));
            return evaluate_expression(
                    conditional_expression_else_part(the_expression),
                    the_context, the_jumper);
        default:
            expression_exception(the_jumper, the_expression,
                    EXCEPTION_TAG(conditional_bad_test),
                    "The test argument to a conditional expression evaluated "
                    "to something other than a boolean value.");
            value_remove_reference(test_value, the_jumper);
            return NULL;
      }
  }

extern value *evaluate_unary_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    expression *operand_expression;
    expression_kind kind;
    value *operand_value;
    const source_location *location;
    value *result;
    value *arguments[1];
    verdict the_verdict;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    operand_expression = unary_expression_operand(the_expression);
    assert(operand_expression != NULL);

    kind = get_expression_kind(the_expression);

    if (get_expression_kind(the_expression) == EK_LOCATION_OF)
      {
        slot_location *the_slot_location;
        value *slot_value;

        the_slot_location = evaluate_address_of_expression(operand_expression,
                the_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(the_slot_location == NULL);
            return NULL;
          }

        assert(the_slot_location != NULL);

        slot_value = create_slot_location_value(the_slot_location);
        if (slot_value == NULL)
            jumper_do_abort(the_jumper);
        slot_location_remove_reference(the_slot_location, the_jumper);
        if ((!(jumper_flowing_forward(the_jumper))) && (slot_value != NULL))
          {
            value_remove_reference(slot_value, the_jumper);
            return NULL;
          }

        return slot_value;
      }

    operand_value =
            evaluate_expression(operand_expression, the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(operand_value == NULL);
        return NULL;
      }

    assert(operand_value != NULL);

    location = get_expression_location(the_expression);

    result = NULL;

    arguments[0] = operand_value;
    the_verdict = try_scoped_overloading(
            expression_overload_chain(the_expression),
            expression_overload_use_statement(the_expression),
            expression_overload_use_used_for_number(the_expression), &result,
            1, &(arguments[0]), the_context, the_jumper, location);
    if (the_verdict == MISSION_ACCOMPLISHED)
        goto done;

    the_verdict = try_overloading(operand_value,
            expression_kind_operator_name(kind), &result, 0, NULL, NULL,
            the_jumper, location);
    if (the_verdict == MISSION_ACCOMPLISHED)
        goto done;

    switch (kind)
      {
        case EK_DEREFERENCE:
          {
            slot_location *the_slot;

            if (get_value_kind(operand_value) != VK_SLOT_LOCATION)
              {
                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(dereference_bad_base),
                        "The operand of a pointer dereference expression "
                        "evaluated to something other than a slot location.");
                value_remove_reference(operand_value, the_jumper);
                return NULL;
              }

            the_slot = slot_location_value_data(operand_value);
            assert(the_slot != NULL);

            result = get_slot_contents(the_slot, NULL,
                    get_expression_location(the_expression), the_jumper);

            break;
          }
        case EK_NEGATE:
          {
            require_numeric(location, kind, "operand", operand_value,
                            the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                break;

            if (get_value_kind(operand_value) == VK_RATIONAL)
              {
                rational *operand_rational;
                rational *result_rational;

                operand_rational = rational_value_data(operand_value);

                result_rational = rational_negate(operand_rational);
                if (result_rational == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    break;
                  }

                result = create_rational_value(result_rational);

                rational_remove_reference(result_rational);
              }
            else
              {
                o_integer operand_oi;
                o_integer result_oi;

                operand_oi = integer_value_data(operand_value);

                oi_negate(result_oi, operand_oi);
                if (oi_out_of_memory(result_oi))
                  {
                    jumper_do_abort(the_jumper);
                    break;
                  }

                result = create_integer_value(result_oi);

                oi_remove_reference(result_oi);
              }

            if (result == NULL)
                jumper_do_abort(the_jumper);

            break;
          }
        case EK_UNARY_PLUS:
          {
            require_numeric(location, kind, "operand", operand_value,
                            the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                break;

            result = operand_value;
            value_add_reference(result);

            break;
          }
        case EK_BITWISE_NOT:
          {
            o_integer operand_oi;
            o_integer negate_oi;
            o_integer result_oi;

            require_integer(location, kind, "operand", operand_value,
                            the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                break;

            operand_oi = integer_value_data(operand_value);

            oi_negate(negate_oi, operand_oi);
            if (oi_out_of_memory(negate_oi))
              {
                jumper_do_abort(the_jumper);
                break;
              }

            oi_subtract(result_oi, negate_oi, oi_one);
            oi_remove_reference(negate_oi);
            if (oi_out_of_memory(result_oi))
              {
                jumper_do_abort(the_jumper);
                break;
              }

            result = create_integer_value(result_oi);

            oi_remove_reference(result_oi);

            if (result == NULL)
                jumper_do_abort(the_jumper);

            break;
          }
        case EK_LOGICAL_NOT:
          {
            require_boolean(location, kind, "operand", operand_value,
                            the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                break;

            if (get_value_kind(operand_value) == VK_FALSE)
                result = create_true_value();
            else
                result = create_false_value();

            if (result == NULL)
                jumper_do_abort(the_jumper);

            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

  done:
    value_remove_reference(operand_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (result != NULL)
            value_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern value *evaluate_binary_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    expression *left_expression;
    expression *right_expression;
    value *left_value;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    left_expression = binary_expression_operand1(the_expression);
    right_expression = binary_expression_operand2(the_expression);
    assert(left_expression != NULL);
    assert(right_expression != NULL);

    left_value = evaluate_expression(left_expression, the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(left_value == NULL);
        return NULL;
      }

    assert(left_value != NULL);

    return finish_evaluating_binary_expression(
            get_expression_kind(the_expression), left_value, right_expression,
            expression_overload_chain(the_expression),
            expression_overload_use_statement(the_expression),
            expression_overload_use_used_for_number(the_expression),
            the_context, the_jumper, get_expression_location(the_expression));
  }

extern value *evaluate_arguments_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_ARGUMENTS);

    result = find_arguments_value(the_context,
            arguments_expression_routine(the_expression));
    assert(result != NULL);

    value_add_reference(result);

    return result;
  }

extern value *evaluate_this_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    object *this_object;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_THIS);

    this_object = find_this_object_value(the_context,
            this_expression_class(the_expression));
    assert(this_object != NULL);

    result = create_object_value(this_object);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    return result;
  }

extern value *evaluate_in_expression(expression *the_expression,
                                     context *the_context, jumper *the_jumper)
  {
    value *the_value;
    type *the_type;
    boolean is_in;
    boolean doubt;
    char *why_not;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_IN);
    the_value = evaluate_expression(in_expression_expression(the_expression),
                                    the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_value == NULL);
        return NULL;
      }

    assert(the_value != NULL);

    the_type = evaluate_type_expression(in_expression_type(the_expression),
                                        the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_type == NULL);
        value_remove_reference(the_value, the_jumper);
        return NULL;
      }

    assert(the_type != NULL);

    check_type_validity(the_type,
            get_type_expression_location(in_expression_type(the_expression)),
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(the_type, the_jumper);
        value_remove_reference(the_value, the_jumper);
        return NULL;
      }

    assert(type_is_valid(the_type)); /* VERIFIED */
    is_in = value_is_in_type(the_value, the_type, &doubt, &why_not,
            get_expression_location(the_expression), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(the_type, the_jumper);
        value_remove_reference(the_value, the_jumper);
        return NULL;
      }

    if (doubt)
      {
        expression_exception(the_jumper, the_expression,
                EXCEPTION_TAG(in_indeterminate),
                "When evaluating an `in' expression, %s was unable to "
                "determine whether the value was in the type because %s.",
                interpreter_name(), why_not);
        free(why_not);
        type_remove_reference(the_type, the_jumper);
        value_remove_reference(the_value, the_jumper);
        return NULL;
      }

    if (is_in)
      {
        result = create_true_value();
      }
    else
      {
        free(why_not);
        result = create_false_value();
      }

    if (result == NULL)
        jumper_do_abort(the_jumper);

    value_remove_reference(the_value, the_jumper);
    type_remove_reference(the_type, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        value_remove_reference(result, the_jumper);
        result = NULL;
      }

    return result;
  }

extern value *evaluate_force_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    value *the_value;
    type *the_type;
    value *type_value;
    value *arguments[2];
    verdict the_verdict;
    boolean doubt;
    char *why_not;
    boolean is_in;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_FORCE);
    the_value = evaluate_expression(
            force_expression_expression(the_expression), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_value == NULL);
        return NULL;
      }

    assert(the_value != NULL);

    the_type = evaluate_type_expression(force_expression_type(the_expression),
                                        the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_type == NULL);
        value_remove_reference(the_value, the_jumper);
        return NULL;
      }

    assert(the_type != NULL);

    check_value_validity(the_value,
            get_expression_location(force_expression_expression(
                    the_expression)), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(the_type, the_jumper);
        value_remove_reference(the_value, the_jumper);
        return NULL;
      }

    check_type_validity(the_type,
            get_type_expression_location(force_expression_type(
                    the_expression)), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(the_type, the_jumper);
        value_remove_reference(the_value, the_jumper);
        return NULL;
      }

    type_value = create_type_value(the_type);
    if (type_value == NULL)
      {
        type_remove_reference(the_type, the_jumper);
        value_remove_reference(the_value, the_jumper);
        return result;
      }

    arguments[0] = the_value;
    arguments[1] = type_value;
    the_verdict = try_scoped_overloading(
            expression_overload_chain(the_expression),
            expression_overload_use_statement(the_expression),
            expression_overload_use_used_for_number(the_expression), &result,
            2, &(arguments[0]), the_context, the_jumper,
            get_expression_location(the_expression));
    value_remove_reference(type_value, the_jumper);
    if (the_verdict == MISSION_ACCOMPLISHED)
      {
        type_remove_reference(the_type, the_jumper);
        value_remove_reference(the_value, the_jumper);
        return result;
      }
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(the_type, the_jumper);
        value_remove_reference(the_value, the_jumper);
        return NULL;
      }

    assert(type_is_valid(the_type)); /* VERIFIED */
    is_in = value_is_in_type(the_value, the_type, &doubt, &why_not,
            get_expression_location(the_expression), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(the_type, the_jumper);
        value_remove_reference(the_value, the_jumper);
        return NULL;
      }

    if (doubt)
      {
        expression_exception(the_jumper, the_expression,
                EXCEPTION_TAG(force_match_indeterminate),
                "When evaluating a force expression, %s was unable to "
                "determine whether the value was in the type because %s.",
                interpreter_name(), why_not);
        free(why_not);
        type_remove_reference(the_type, the_jumper);
        value_remove_reference(the_value, the_jumper);
        return NULL;
      }

    if (is_in)
      {
        type_remove_reference(the_type, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(the_value, the_jumper);
            return NULL;
          }
        return the_value;
      }

    free(why_not);

    assert(value_is_valid(the_value)); /* VERIFIED */
    assert(type_is_valid(the_type)); /* VERIFIED */
    result = force_value_to_type(the_value, the_type,
            get_expression_location(the_expression), the_jumper);
    value_remove_reference(the_value, the_jumper);
    type_remove_reference(the_type, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (result != NULL)
            value_remove_reference(result, the_jumper);
        return NULL;
      }
    if (result == NULL)
      {
        expression_exception(the_jumper, the_expression,
                EXCEPTION_TAG(force_cant_force),
                "A force expression was evaluated with a value that couldn't "
                "be forced into the specified type.");
        return NULL;
      }
    return result;
  }

extern value *evaluate_break_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    jump_target *target;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_BREAK);

    target = find_break_target(the_context,
                               break_expression_from(the_expression));
    assert(target != NULL);
    assert(!(jump_target_scope_exited(target))); /* VERIFIED */

    result = create_jump_target_value(target);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    return result;
  }

extern value *evaluate_continue_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    jump_target *target;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_CONTINUE);

    target = find_continue_target(the_context,
                                  continue_expression_with(the_expression));
    assert(target != NULL);
    assert(!(jump_target_scope_exited(target))); /* VERIFIED */

    result = create_jump_target_value(target);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    return result;
  }

extern value *evaluate_comprehend_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    const source_location *base_location;
    value *base_value;
    context *comprehend_context;
    variable_instance *element_instance;
    type *element_variable_type;
    simple_iteration_data simple_data;
    verdict the_verdict;
    value *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_COMPREHEND);

    base_location = get_expression_location(
            comprehend_expression_base(the_expression));

    base_value = evaluate_expression(
            comprehend_expression_base(the_expression), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(base_value == NULL);
        return NULL;
      }

    assert(base_value != NULL);

    check_value_validity_except_map_targets(base_value, base_location,
                                            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    comprehend_context = create_loop_context(the_context, the_expression,
            comprehend_expression_element(the_expression),
            jumper_purity_level(the_jumper),
            get_expression_location(the_expression));
    if (comprehend_context == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    element_instance = loop_context_index(comprehend_context);
    assert(element_instance != NULL);

    assert(!(variable_instance_scope_exited(element_instance))); /* VERIFIED */
    set_variable_instance_instantiated(element_instance);

    element_variable_type = get_anything_type();
    if (element_variable_type == NULL)
      {
        jumper_do_abort(the_jumper);
        exit_context(comprehend_context, the_jumper);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }
    type_add_reference(element_variable_type);

    assert(!(variable_instance_scope_exited(element_instance))); /* VERIFIED */
    set_variable_instance_type(element_instance, element_variable_type,
                               the_jumper);
    type_remove_reference(element_variable_type, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        exit_context(comprehend_context, the_jumper);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    assert(value_is_valid_except_map_targets(base_value)); /* VERIFIED */

    the_verdict = start_simple_iteration_data(base_value, &simple_data,
            "evaluating", "a comprehend expression", the_jumper,
            base_location);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        exit_context(comprehend_context, the_jumper);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        static const char *transform_formal_names[4] =
          { "x", "magic", "break", "continue" };

        value *iterator_value;
        verdict the_verdict;
        formal_arguments *formals;
        statement_block *body;
        routine_declaration *class_declaration;
        declaration *class_generic_declaration;
        routine_instance *class_instance;
        type *return_type;
        object *magic_object;
        comprehend_data *hook_data;
        value *class_value;
        value *actual_values[3];
        formal_arguments *transform_formals;
        type_expression *transform_return_type;
        routine_declaration *transform_declaration;
        declaration *transform_generic_declaration;
        expression *transform_expression;
        value *transform_value;
        value *magic_value;
        value *result_value;

        the_verdict = try_overloading(base_value, "iterator", &iterator_value,
                0, NULL, NULL, the_jumper,
                get_expression_location(the_expression));
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(iterator_value == NULL);
            exit_context(comprehend_context, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            assert(iterator_value == NULL);
            location_exception(the_jumper, base_location,
                    EXCEPTION_TAG(iteration_bad_base),
                    "The base argument to a comprehend expression evaluated to"
                    " something other than an array or semi-labeled value list"
                    " that didn't have an `iterator' function.");
            exit_context(comprehend_context, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        assert(iterator_value != NULL);
        value_remove_reference(iterator_value, the_jumper);

        formals = create_formal_arguments();
        if (formals == NULL)
          {
            jumper_do_abort(the_jumper);
            exit_context(comprehend_context, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        body = create_statement_block();
        if (body == NULL)
          {
            jumper_do_abort(the_jumper);
            delete_formal_arguments(formals);
            exit_context(comprehend_context, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        class_declaration = create_routine_declaration(NULL, NULL, formals,
                FALSE, body, NULL, PURE_UNSAFE, FALSE, TRUE, NULL, 0, NULL);
        if (class_declaration == NULL)
          {
            jumper_do_abort(the_jumper);
            exit_context(comprehend_context, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        class_generic_declaration = create_declaration_for_routine(
                "comprehender", FALSE, FALSE, FALSE, class_declaration,
                get_expression_location(the_expression));
        if (class_generic_declaration == NULL)
          {
            jumper_do_abort(the_jumper);
            exit_context(comprehend_context, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        class_instance = create_routine_instance(class_declaration,
                jumper_purity_level(the_jumper), NULL);
        routine_declaration_remove_reference(class_declaration);
        if (class_instance == NULL)
          {
            jumper_do_abort(the_jumper);
            exit_context(comprehend_context, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        routine_instance_set_up_static_context(class_instance, the_context,
                                               the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            routine_instance_remove_reference(class_instance, the_jumper);
            exit_context(comprehend_context, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        return_type = get_class_type(class_instance);
        if (return_type == NULL)
          {
            jumper_do_abort(the_jumper);
            routine_instance_remove_reference(class_instance, the_jumper);
            exit_context(comprehend_context, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        routine_instance_set_return_type(class_instance, return_type, 1,
                                         the_jumper);
        type_remove_reference(return_type, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            routine_instance_remove_reference(class_instance, the_jumper);
            exit_context(comprehend_context, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        set_instance_instantiated(routine_instance_instance(class_instance));

        magic_object =
                create_object(class_instance, comprehend_context, NULL, NULL);
        routine_instance_remove_reference(class_instance, the_jumper);
        if (magic_object == NULL)
          {
            jumper_do_abort(the_jumper);
            exit_context(comprehend_context, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        hook_data = MALLOC_ONE_OBJECT(comprehend_data);
        if (hook_data == NULL)
          {
            jumper_do_abort(the_jumper);
            object_remove_reference(magic_object, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        hook_data->body = comprehend_expression_body(the_expression);
        hook_data->filter = comprehend_expression_filter(the_expression);
        hook_data->element_instance = element_instance;
        hook_data->context = comprehend_context;
        object_set_hook(magic_object, hook_data);
        object_set_hook_cleaner(magic_object, &comprehend_cleaner);

        class_value = evaluate_expression(comprehend_iterator_expression,
                                          comprehend_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(class_value == NULL);
            object_remove_reference(magic_object, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        assert(class_value != NULL);

        assert(get_value_kind(class_value) == VK_ROUTINE);

        actual_values[0] = base_value;

        transform_formals = create_formals(4, &(transform_formal_names[0]),
                get_expression_location(the_expression));
        if (transform_formals == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(base_value, the_jumper);
            value_remove_reference(class_value, the_jumper);
            object_remove_reference(magic_object, the_jumper);
            return NULL;
          }

        transform_return_type =
                create_constant_type_expression(get_anything_type());
        if (transform_return_type == NULL)
          {
            jumper_do_abort(the_jumper);
            delete_formal_arguments(transform_formals);
            value_remove_reference(base_value, the_jumper);
            value_remove_reference(class_value, the_jumper);
            object_remove_reference(magic_object, the_jumper);
            return NULL;
          }

        transform_declaration = create_routine_declaration(
                transform_return_type, NULL, transform_formals, FALSE, NULL,
                &comprehend_transform_handler, PURE_SAFE, FALSE, FALSE, NULL,
                0, NULL);
        if (transform_declaration == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(base_value, the_jumper);
            value_remove_reference(class_value, the_jumper);
            object_remove_reference(magic_object, the_jumper);
            return NULL;
          }

        transform_generic_declaration = create_declaration_for_routine(
                "transform", FALSE, FALSE, FALSE, transform_declaration,
                get_expression_location(the_expression));
        if (transform_generic_declaration == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(base_value, the_jumper);
            value_remove_reference(class_value, the_jumper);
            object_remove_reference(magic_object, the_jumper);
            return NULL;
          }

        transform_expression =
                create_declaration_expression(transform_generic_declaration);
        if (transform_expression == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(base_value, the_jumper);
            value_remove_reference(class_value, the_jumper);
            object_remove_reference(magic_object, the_jumper);
            return NULL;
          }

        transform_value = evaluate_expression(transform_expression,
                                              comprehend_context, the_jumper);
        delete_expression(transform_expression);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(transform_value == NULL);
            value_remove_reference(base_value, the_jumper);
            value_remove_reference(class_value, the_jumper);
            object_remove_reference(magic_object, the_jumper);
            return NULL;
          }

        assert(transform_value != NULL);
        actual_values[1] = transform_value;

        magic_value = create_object_value(magic_object);
        object_remove_reference(magic_object, the_jumper);
        if (magic_value == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(transform_value, the_jumper);
            value_remove_reference(base_value, the_jumper);
            value_remove_reference(class_value, the_jumper);
            return NULL;
          }
        actual_values[2] = magic_value;

        result_value = execute_call_from_arrays(class_value, 3, NULL,
                &(actual_values[0]), TRUE, the_jumper,
                get_expression_location(the_expression));
        value_remove_reference(magic_value, the_jumper);
        value_remove_reference(transform_value, the_jumper);
        value_remove_reference(base_value, the_jumper);
        value_remove_reference(class_value, the_jumper);
        if ((result_value != NULL) && (!(jumper_flowing_forward(the_jumper))))
          {
            value_remove_reference(result_value, the_jumper);
            return NULL;
          }
        return result_value;
      }

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        exit_context(comprehend_context, the_jumper);
        clean_up_simple_iteration_data(&simple_data);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    while (TRUE)
      {
        value *element_value;
        value *body_result;
        verdict the_verdict;

        if (simple_iteration_data_is_done(&simple_data))
          {
            exit_context(comprehend_context, the_jumper);
            clean_up_simple_iteration_data(&simple_data);
            value_remove_reference(base_value, the_jumper);
            return result;
          }

        element_value = simple_iteration_data_current(&simple_data, the_jumper,
                get_expression_location(the_expression));
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            exit_context(comprehend_context, the_jumper);
            return NULL;
          }

        assert(element_value != NULL);
        assert(variable_instance_is_instantiated(element_instance));
                /* VERIFIED */
        assert(!(variable_instance_scope_exited(element_instance)));
                /* VERIFIED */
        assert(variable_instance_lock_chain(element_instance) == NULL);
        set_variable_instance_value(element_instance, element_value,
                                    the_jumper);
        value_remove_reference(element_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            exit_context(comprehend_context, the_jumper);
            clean_up_simple_iteration_data(&simple_data);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        if (comprehend_expression_filter(the_expression) != NULL)
          {
            value *filter_result;

            filter_result = evaluate_expression(
                    comprehend_expression_filter(the_expression),
                    comprehend_context, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(filter_result == NULL);

                if (jumper_target(the_jumper) ==
                    loop_context_continue_target(comprehend_context))
                  {
                    jumper_reached_target(the_jumper);
                    goto do_continue;
                  }

                clean_up_simple_iteration_data(&simple_data);
                value_remove_reference(base_value, the_jumper);

                if (jumper_target(the_jumper) ==
                    loop_context_break_target(comprehend_context))
                  {
                    jumper_reached_target(the_jumper);
                    exit_context(comprehend_context, the_jumper);
                    return result;
                  }

                value_remove_reference(result, the_jumper);
                exit_context(comprehend_context, the_jumper);
                return NULL;
              }

            assert(filter_result != NULL);
            switch (get_value_kind(filter_result))
              {
                case VK_TRUE:
                    value_remove_reference(filter_result, the_jumper);
                    assert(jumper_flowing_forward(the_jumper));
                    break;
                case VK_FALSE:
                    value_remove_reference(filter_result, the_jumper);
                    assert(jumper_flowing_forward(the_jumper));
                    goto do_continue;
                default:
                    expression_exception(the_jumper,
                            comprehend_expression_filter(the_expression),
                            EXCEPTION_TAG(comprehend_bad_test),
                            "The test argument to a comprehend expression "
                            "evaluated to something other than a boolean "
                            "value.");
                    value_remove_reference(filter_result, the_jumper);
                    clean_up_simple_iteration_data(&simple_data);
                    value_remove_reference(base_value, the_jumper);
                    value_remove_reference(result, the_jumper);
                    exit_context(comprehend_context, the_jumper);
                    return NULL;
              }
          }

        body_result = evaluate_expression(
                comprehend_expression_body(the_expression), comprehend_context,
                the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(body_result == NULL);

            if (jumper_target(the_jumper) ==
                loop_context_continue_target(comprehend_context))
              {
                jumper_reached_target(the_jumper);
                goto do_continue;
              }

            clean_up_simple_iteration_data(&simple_data);
            value_remove_reference(base_value, the_jumper);

            if (jumper_target(the_jumper) ==
                loop_context_break_target(comprehend_context))
              {
                jumper_reached_target(the_jumper);
                exit_context(comprehend_context, the_jumper);
                return result;
              }

            value_remove_reference(result, the_jumper);
            exit_context(comprehend_context, the_jumper);
            return NULL;
          }

        assert(body_result != NULL);
        the_verdict = add_field(result, NULL, body_result);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            exit_context(comprehend_context, the_jumper);
            clean_up_simple_iteration_data(&simple_data);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        value_remove_reference(body_result, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            exit_context(comprehend_context, the_jumper);
            clean_up_simple_iteration_data(&simple_data);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

      do_continue:
        simple_iteration_data_step(&simple_data, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            exit_context(comprehend_context, the_jumper);
            return NULL;
          }
      }
  }

extern value *evaluate_forall_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_FORALL);

    expression_exception(the_jumper, the_expression,
            EXCEPTION_TAG(forall_executed),
            "A forall expression was evaluated.");
    return NULL;
  }

extern value *evaluate_exists_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_expression_kind(the_expression) == EK_EXISTS);

    expression_exception(the_jumper, the_expression,
            EXCEPTION_TAG(exists_executed),
            "An exists expression was evaluated.");
    return NULL;
  }

extern value *finish_evaluating_binary_expression(expression_kind kind,
        value *left_value, expression *right_expression,
        routine_declaration_chain *overload_chain,
        statement *overload_use_statement, size_t overload_used_for_number,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *right_value;
    value *result_value;

    assert(left_value != NULL);
    assert(right_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    if ((kind != EK_LOGICAL_AND) && (kind != EK_LOGICAL_OR))
      {
        right_value =
                evaluate_expression(right_expression, the_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(right_value == NULL);
            value_remove_reference(left_value, the_jumper);
            return NULL;
          }

        assert(right_value != NULL);
      }

    if ((kind != EK_LOGICAL_AND) && (kind != EK_LOGICAL_OR))
      {
        check_value_validity(left_value, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(left_value, the_jumper);
            if ((kind != EK_LOGICAL_AND) && (kind != EK_LOGICAL_OR))
                value_remove_reference(right_value, the_jumper);
            return NULL;
          }

        check_value_validity(right_value, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(left_value, the_jumper);
            value_remove_reference(right_value, the_jumper);
            return NULL;
          }
      }

    result_value = NULL;

    if ((kind != EK_LOGICAL_AND) && (kind != EK_LOGICAL_OR))
      {
        static const char *argument_names[2] = {NULL, "reversed"};

        value *arguments[2];
        verdict the_verdict;

        arguments[0] = left_value;
        arguments[1] = right_value;
        the_verdict = try_scoped_overloading(overload_chain,
                overload_use_statement, overload_used_for_number,
                &result_value, 2, &(arguments[0]), the_context, the_jumper,
                location);
        if (the_verdict == MISSION_ACCOMPLISHED)
            goto done;

        arguments[0] = right_value;
        arguments[1] = create_false_value();
        if (arguments[1] == NULL)
          {
            jumper_do_abort(the_jumper);
            result_value = NULL;
            goto done;
          };
        the_verdict = try_overloading(left_value,
                expression_kind_operator_name(kind), &result_value, 2,
                &(arguments[0]), &(argument_names[0]), the_jumper, location);
        value_remove_reference(arguments[1], NULL);
        if (the_verdict == MISSION_ACCOMPLISHED)
            goto done;

        arguments[0] = left_value;
        arguments[1] = create_true_value();
        if (arguments[1] == NULL)
          {
            jumper_do_abort(the_jumper);
            result_value = NULL;
            goto done;
          };
        the_verdict = try_overloading(right_value,
                expression_kind_operator_name(kind), &result_value, 2,
                &(arguments[0]), &(argument_names[0]), the_jumper, location);
        value_remove_reference(arguments[1], NULL);
        if (the_verdict == MISSION_ACCOMPLISHED)
            goto done;
        if (!(jumper_flowing_forward(the_jumper)))
          {
            result_value = NULL;
            goto done;
          }
      }

    switch (kind)
      {
        case EK_ADD:
        case EK_SUBTRACT:
        case EK_MULTIPLY:
        case EK_DIVIDE:
        case EK_DIVIDE_FORCE:
        case EK_REMAINDER:
        case EK_SHIFT_LEFT:
        case EK_SHIFT_RIGHT:
          {
            o_integer left_oi;
            o_integer right_oi;
            o_integer result_oi;

            if ((kind == EK_ADD) &&
                (get_value_kind(left_value) == VK_SLOT_LOCATION) &&
                (get_value_kind(right_value) == VK_INTEGER))
              {
                result_value = add_slot_location_and_integer(
                        slot_location_value_data(left_value),
                        integer_value_data(right_value), location, the_jumper);
                break;
              }

            if ((kind == EK_ADD) && (get_value_kind(left_value) == VK_INTEGER)
                && (get_value_kind(right_value) == VK_SLOT_LOCATION))
              {
                result_value = add_slot_location_and_integer(
                        slot_location_value_data(right_value),
                        integer_value_data(left_value), location, the_jumper);
                break;
              }

            if ((kind == EK_SUBTRACT) &&
                (get_value_kind(left_value) == VK_SLOT_LOCATION) &&
                (get_value_kind(right_value) == VK_INTEGER))
              {
                o_integer negation;

                oi_negate(negation, integer_value_data(right_value));
                if (oi_out_of_memory(negation))
                    break;

                result_value = add_slot_location_and_integer(
                        slot_location_value_data(left_value), negation,
                        location, the_jumper);
                oi_remove_reference(negation);
                break;
              }

            if ((kind == EK_SUBTRACT) &&
                (get_value_kind(left_value) == VK_SLOT_LOCATION) &&
                (get_value_kind(right_value) == VK_SLOT_LOCATION))
              {
                slot_location *left_slot;
                slot_location *right_slot;
                boolean doubt;
                boolean are_equal;

                assert(value_is_valid(left_value)); /* VERIFIED */
                left_slot = slot_location_value_data(left_value);
                assert(left_slot != NULL);

                assert(value_is_valid(right_value)); /* VERIFIED */
                right_slot = slot_location_value_data(right_value);
                assert(right_slot != NULL);

                assert(slot_location_is_valid(left_slot)); /* VERIFIED */
                assert(slot_location_is_valid(right_slot)); /* VERIFIED */
                are_equal = slot_locations_are_equal(left_slot, right_slot,
                        &doubt, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                    break;
                if (doubt)
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(pointer_subtraction_indeterminate),
                            "When evaluating the subtraction of one pointer "
                            "from another, %s was unable to determine whether "
                            "the two values were equal.", interpreter_name());
                    break;
                  }

                if (are_equal)
                  {
                    result_value = create_integer_value(oi_zero);
                  }
                else
                  {
                    slot_location *left_base;
                    slot_location *right_base;
                    boolean base_doubt;
                    boolean bases_equal;
                    lookup_actual_arguments *left_actuals;
                    lookup_actual_arguments *right_actuals;

                    if ((get_slot_location_kind(left_slot) != SLK_LOOKUP) ||
                        (get_slot_location_kind(right_slot) != SLK_LOOKUP))
                      {
                        location_exception(the_jumper, location,
                                EXCEPTION_TAG(pointer_subtraction_non_lookup),
                                "When evaluating the subtraction of one "
                                "pointer from another, the pointers were "
                                "unequal but one of them was not a lookup.");
                        break;
                      }

                    if (slot_location_overload_base(left_slot) !=
                        slot_location_overload_base(right_slot))
                      {
                        location_exception(the_jumper, location,
                                EXCEPTION_TAG(
                                        pointer_subtraction_overload_mismatch),
                                "When evaluating the subtraction of one "
                                "pointer from another, different overloading "
                                "applied to the two pointers.");
                        break;
                      }

                    assert(slot_location_is_valid(left_slot)); /* VERIFIED */
                    left_base = lookup_slot_location_base(left_slot);
                    assert(left_base != NULL);
                    assert(slot_location_is_valid(left_base)); /* VERIFIED */

                    assert(slot_location_is_valid(right_slot)); /* VERIFIED */
                    right_base = lookup_slot_location_base(right_slot);
                    assert(right_base != NULL);
                    assert(slot_location_is_valid(right_base)); /* VERIFIED */

                    assert(slot_location_is_valid(left_base)); /* VERIFIED */
                    assert(slot_location_is_valid(right_base)); /* VERIFIED */
                    bases_equal = slot_locations_are_equal(left_base,
                            right_base, &base_doubt, location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                        break;
                    if (base_doubt)
                      {
                        location_exception(the_jumper, location,
                                EXCEPTION_TAG(
                                    pointer_subtraction_base_indeterminate),
                                "When evaluating the subtraction of one "
                                "pointer from another, %s was unable to "
                                "determine whether the bases of the two "
                                "pointers were equal.", interpreter_name());
                        break;
                      }

                    if (!bases_equal)
                      {
                        location_exception(the_jumper, location,
                                EXCEPTION_TAG(
                                        pointer_subtraction_base_mismatch),
                                "When evaluating the subtraction of one "
                                "pointer from another, the bases of the two "
                                "pointers were not equal.");
                        break;
                      }

                    assert(slot_location_is_valid(left_slot)); /* VERIFIED */
                    left_actuals = lookup_slot_location_actuals(left_slot);
                    assert(left_actuals != NULL);
                    assert(lookup_actual_arguments_is_valid(left_actuals));
                            /* VERIFIED */

                    assert(slot_location_is_valid(right_slot)); /* VERIFIED */
                    right_actuals = lookup_slot_location_actuals(right_slot);
                    assert(right_actuals != NULL);
                    assert(lookup_actual_arguments_is_valid(right_actuals));
                            /* VERIFIED */

                    assert(lookup_actual_arguments_is_valid(left_actuals));
                            /* VERIFIED */
                    assert(lookup_actual_arguments_is_valid(right_actuals));
                            /* VERIFIED */
                    result_value = lookup_actual_arguments_difference(
                            left_actuals, right_actuals, location, the_jumper);
                  }

                break;
              }

            require_numeric(location, kind, "left operand", left_value,
                            the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                break;

            require_numeric(location, kind, "right operand", right_value,
                            the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                break;

            if ((get_value_kind(left_value) == VK_RATIONAL) ||
                (get_value_kind(right_value) == VK_RATIONAL))
              {
                rational *left_rational;
                rational *right_rational;
                rational *result_rational;

                left_rational =
                        rational_from_integer_or_rational_value(left_value);
                if (left_rational == NULL)
                    break;

                right_rational =
                        rational_from_integer_or_rational_value(right_value);
                if (right_rational == NULL)
                  {
                    rational_remove_reference(left_rational);
                    break;
                  }

                switch (kind)
                  {
                    case EK_ADD:
                      {
                        result_rational =
                                rational_add(left_rational, right_rational);
                        break;
                      }
                    case EK_SUBTRACT:
                      {
                        result_rational = rational_subtract(left_rational,
                                                            right_rational);
                        break;
                      }
                    case EK_MULTIPLY:
                      {
                        result_rational = rational_multiply(left_rational,
                                                            right_rational);
                        break;
                      }
                    case EK_DIVIDE:
                      {
                        result_rational =
                                rational_divide(left_rational, right_rational);
                        break;
                      }
                    case EK_DIVIDE_FORCE:
                      {
                        location_exception(the_jumper, location,
                                EXCEPTION_TAG(divide_force_rational),
                                "The divide-force operation with a non-integer"
                                " rational operand is illegal.");
                        result_rational = NULL;
                        break;
                      }
                    case EK_REMAINDER:
                      {
                        location_exception(the_jumper, location,
                                EXCEPTION_TAG(remainder_rational),
                                "The remainder operation with a non-integer "
                                "rational operand is illegal.");
                        result_rational = NULL;
                        break;
                      }
                    case EK_SHIFT_LEFT:
                      {
                        location_exception(the_jumper, location,
                                EXCEPTION_TAG(left_shift_rational),
                                "The left shift operation with a non-integer "
                                "rational operand is illegal.");
                        result_rational = NULL;
                        break;
                      }
                    case EK_SHIFT_RIGHT:
                      {
                        location_exception(the_jumper, location,
                                EXCEPTION_TAG(right_shift_rational),
                                "The right shift operation with a non-integer "
                                "rational operand is illegal.");
                        result_rational = NULL;
                        break;
                      }
                    default:
                      {
                        assert(FALSE);
                        result_rational = NULL;
                      }
                  }

                rational_remove_reference(left_rational);
                rational_remove_reference(right_rational);

                if (result_rational == NULL)
                    break;

                if (rational_is_integer(result_rational))
                  {
                    result_value = create_integer_value(
                            rational_numerator(result_rational));
                  }
                else
                  {
                    result_value = create_rational_value(result_rational);
                  }

                rational_remove_reference(result_rational);

                break;
              }

            left_oi = integer_value_data(left_value);
            right_oi = integer_value_data(right_value);

            switch (kind)
              {
                case EK_ADD:
                  {
                    oi_add(result_oi, left_oi, right_oi);
                    break;
                  }
                case EK_SUBTRACT:
                  {
                    oi_subtract(result_oi, left_oi, right_oi);
                    break;
                  }
                case EK_MULTIPLY:
                  {
                    oi_multiply(result_oi, left_oi, right_oi);
                    break;
                  }
                case EK_DIVIDE:
                  {
                    rational *the_rational;

                    the_rational = create_rational(left_oi, right_oi);
                    if (the_rational == NULL)
                      {
                        result_oi = oi_null;
                        break;
                      }

                    if (rational_is_integer(the_rational))
                      {
                        result_oi = rational_numerator(the_rational);
                        assert(!(oi_out_of_memory(result_oi)));
                        oi_add_reference(result_oi);
                        rational_remove_reference(the_rational);
                        break;
                      }

                    result_value = create_rational_value(the_rational);

                    rational_remove_reference(the_rational);

                    goto done;
                  }
                case EK_DIVIDE_FORCE:
                  {
                    o_integer other;

                    oi_divide(result_oi, left_oi, right_oi, &other);
                    if (!(oi_out_of_memory(result_oi)))
                        oi_remove_reference(other);

                    break;
                  }
                case EK_REMAINDER:
                  {
                    o_integer other;

                    oi_divide(other, left_oi, right_oi, &result_oi);
                    if (!(oi_out_of_memory(other)))
                      {
                        oi_remove_reference(other);
                      }
                    else
                      {
                        result_oi = oi_null;
                      }

                    break;
                  }
                case EK_SHIFT_LEFT:
                  {
                    oi_shift_left(result_oi, left_oi, right_oi);
                    break;
                  }
                case EK_SHIFT_RIGHT:
                  {
                    oi_shift_right(result_oi, left_oi, right_oi);
                    break;
                  }
                default:
                  {
                    assert(FALSE);
                  }
              }

            if (oi_out_of_memory(result_oi))
                break;

            result_value = create_integer_value(result_oi);

            oi_remove_reference(result_oi);

            break;
          }
        case EK_LESS_THAN:
        case EK_GREATER_THAN:
        case EK_LESS_THAN_OR_EQUAL:
        case EK_GREATER_THAN_OR_EQUAL:
          {
            boolean result_boolean;

            assert(value_is_valid(left_value)); /* VERIFIED */
            assert(value_is_valid(right_value)); /* VERIFIED */

            require_numeric_or_pointer(location, kind, "left operand",
                                       left_value, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                break;

            require_numeric_or_pointer(location, kind, "right operand",
                                       right_value, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                break;

            if ((get_value_kind(left_value) == VK_SLOT_LOCATION) &&
                (get_value_kind(right_value) == VK_SLOT_LOCATION))
              {
                slot_location *left_slot;
                slot_location *right_slot;
                boolean doubt;
                boolean are_equal;
                slot_location *left_base;
                slot_location *right_base;
                boolean base_doubt;
                boolean bases_equal;
                lookup_actual_arguments *left_actuals;
                lookup_actual_arguments *right_actuals;
                int order;
                boolean result_boolean;

                assert(value_is_valid(left_value)); /* VERIFIED */
                left_slot = slot_location_value_data(left_value);
                assert(left_slot != NULL);
                assert(slot_location_is_valid(left_slot)); /* VERIFIED */

                assert(value_is_valid(right_value)); /* VERIFIED */
                right_slot = slot_location_value_data(right_value);
                assert(right_slot != NULL);
                assert(slot_location_is_valid(right_slot)); /* VERIFIED */

                assert(slot_location_is_valid(left_slot)); /* VERIFIED */
                assert(slot_location_is_valid(right_slot)); /* VERIFIED */
                are_equal = slot_locations_are_equal(left_slot, right_slot,
                        &doubt, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                    break;
                if (doubt)
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(pointer_comparison_indeterminate),
                            "When evaluating the order comparison of two "
                            "pointers, %s was unable to determine whether the "
                            "two values were equal.", interpreter_name());
                    break;
                  }

                if (are_equal)
                  {
                    if ((kind == EK_LESS_THAN_OR_EQUAL) ||
                        (kind == EK_GREATER_THAN_OR_EQUAL))
                      {
                        result_value = create_true_value();
                      }
                    else
                      {
                        result_value = create_false_value();
                      }

                    break;
                  }

                if ((get_slot_location_kind(left_slot) != SLK_LOOKUP) ||
                    (get_slot_location_kind(right_slot) != SLK_LOOKUP))
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(pointer_comparison_non_lookup),
                            "When evaluating the order comparison of two "
                            "pointers, the pointers were unequal but one of "
                            "them was not a lookup.");
                    break;
                  }

                if (slot_location_overload_base(left_slot) !=
                    slot_location_overload_base(right_slot))
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(
                                    pointer_comparison_overload_mismatch),
                            "When evaluating the order comparison of two "
                            "pointers, different overloading applied to the "
                            "two pointers.");
                    break;
                  }

                assert(slot_location_is_valid(left_slot)); /* VERIFIED */
                left_base = lookup_slot_location_base(left_slot);
                assert(left_base != NULL);
                assert(slot_location_is_valid(left_base)); /* VERIFIED */

                assert(slot_location_is_valid(right_slot)); /* VERIFIED */
                right_base = lookup_slot_location_base(right_slot);
                assert(right_base != NULL);
                assert(slot_location_is_valid(right_base)); /* VERIFIED */

                assert(slot_location_is_valid(left_base)); /* VERIFIED */
                assert(slot_location_is_valid(right_base)); /* VERIFIED */
                bases_equal = slot_locations_are_equal(left_base, right_base,
                        &base_doubt, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                    break;
                if (base_doubt)
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(
                                    pointer_comparison_base_indeterminate),
                            "When evaluating the order comparison of two "
                            "pointers, %s was unable to determine whether the "
                            "bases of the two pointers were equal.",
                            interpreter_name());
                    break;
                  }

                if (!bases_equal)
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(pointer_comparison_base_mismatch),
                            "When evaluating the order comparison of two "
                            "pointers, the bases of the two pointers were not "
                            "equal.");
                    break;
                  }

                assert(slot_location_is_valid(left_slot)); /* VERIFIED */
                left_actuals = lookup_slot_location_actuals(left_slot);
                assert(left_actuals != NULL);
                assert(lookup_actual_arguments_is_valid(left_actuals));
                        /* VERIFIED */

                assert(slot_location_is_valid(right_slot)); /* VERIFIED */
                right_actuals = lookup_slot_location_actuals(right_slot);
                assert(right_actuals != NULL);
                assert(lookup_actual_arguments_is_valid(right_actuals));
                        /* VERIFIED */

                assert(lookup_actual_arguments_is_valid(left_actuals));
                        /* VERIFIED */
                assert(lookup_actual_arguments_is_valid(right_actuals));
                        /* VERIFIED */
                order = lookup_actual_arguments_order(left_actuals,
                        right_actuals, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                    break;

                switch (kind)
                  {
                    case EK_LESS_THAN:
                        result_boolean = (order < 0);
                        break;
                    case EK_GREATER_THAN:
                        result_boolean = (order > 0);
                        break;
                    case EK_LESS_THAN_OR_EQUAL:
                        result_boolean = (order <= 0);
                        break;
                    case EK_GREATER_THAN_OR_EQUAL:
                        result_boolean = (order >= 0);
                        break;
                    default:
                        assert(FALSE);
                        result_boolean = FALSE;
                  }

                if (result_boolean)
                    result_value = create_true_value();
                else
                    result_value = create_false_value();

                break;
              }
            else if ((get_value_kind(left_value) == VK_INTEGER) &&
                     (get_value_kind(right_value) == VK_INTEGER))
              {
                o_integer left_oi;
                o_integer right_oi;

                left_oi = integer_value_data(left_value);
                right_oi = integer_value_data(right_value);

                switch (kind)
                  {
                    case EK_LESS_THAN:
                        result_boolean = oi_less_than(left_oi, right_oi);
                        break;
                    case EK_GREATER_THAN:
                        result_boolean = oi_less_than(right_oi, left_oi);
                        break;
                    case EK_LESS_THAN_OR_EQUAL:
                        result_boolean = (oi_equal(left_oi, right_oi) ||
                                          oi_less_than(left_oi, right_oi));
                        break;
                    case EK_GREATER_THAN_OR_EQUAL:
                        result_boolean = (oi_equal(left_oi, right_oi) ||
                                          oi_less_than(right_oi, left_oi));
                        break;
                    default:
                        assert(FALSE);
                        result_boolean = FALSE;
                  }
              }
            else if (((get_value_kind(left_value) == VK_INTEGER) ||
                      (get_value_kind(left_value) == VK_RATIONAL)) &&
                     ((get_value_kind(right_value) == VK_INTEGER) ||
                      (get_value_kind(right_value) == VK_RATIONAL)))
              {
                rational *left_rational;
                rational *right_rational;
                boolean error;

                left_rational =
                        rational_from_integer_or_rational_value(left_value);
                if (left_rational == NULL)
                    break;

                right_rational =
                        rational_from_integer_or_rational_value(right_value);
                if (right_rational == NULL)
                  {
                    rational_remove_reference(left_rational);
                    break;
                  }

                switch (kind)
                  {
                    case EK_LESS_THAN:
                        result_boolean = rational_less_than(left_rational,
                                right_rational, &error);
                        break;
                    case EK_GREATER_THAN:
                        result_boolean = rational_less_than(right_rational,
                                left_rational, &error);
                        break;
                    case EK_LESS_THAN_OR_EQUAL:
                        error = FALSE;
                        result_boolean =
                                (rationals_are_equal(left_rational,
                                                     right_rational) ||
                                 rational_less_than(left_rational,
                                                    right_rational, &error));
                        break;
                    case EK_GREATER_THAN_OR_EQUAL:
                        error = FALSE;
                        result_boolean =
                                (rationals_are_equal(left_rational,
                                                     right_rational) ||
                                 rational_less_than(right_rational,
                                                    left_rational, &error));
                        break;
                    default:
                        assert(FALSE);
                        result_boolean = FALSE;
                  }

                rational_remove_reference(left_rational);
                rational_remove_reference(right_rational);

                if (error)
                    break;
              }
            else
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(binary_bad_operands),
                        "Illegal operands to %s expression: %s and %s.",
                        expression_kind_name(kind),
                        value_kind_name(left_value),
                        value_kind_name(right_value));

                break;
              }

            if (result_boolean)
                result_value = create_true_value();
            else
                result_value = create_false_value();

            break;
          }
        case EK_EQUAL:
          {
            boolean equal;
            boolean doubt;

            assert(value_is_valid(left_value)); /* VERIFIED */
            assert(value_is_valid(right_value)); /* VERIFIED */
            equal = values_are_equal(left_value, right_value, &doubt, location,
                                     the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                break;

            if (doubt)
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(equality_test_indeterminate),
                        "When evaluating a binary equality test expression, %s"
                        " was unable to determine whether the two values were "
                        "equal.", interpreter_name());
                break;
              }

            if (equal)
                result_value = create_true_value();
            else
                result_value = create_false_value();

            break;
          }
        case EK_NOT_EQUAL:
          {
            boolean equal;
            boolean doubt;

            assert(value_is_valid(left_value)); /* VERIFIED */
            assert(value_is_valid(right_value)); /* VERIFIED */
            equal = values_are_equal(left_value, right_value, &doubt, location,
                                     the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                break;

            if (doubt)
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(equality_test_indeterminate),
                        "When evaluating a binary equality test expression, %s"
                        " was unable to determine whether the two values were "
                        "equal.", interpreter_name());
                break;
              }

            if (equal)
                result_value = create_false_value();
            else
                result_value = create_true_value();

            break;
          }
        case EK_BITWISE_AND:
        case EK_BITWISE_OR:
        case EK_BITWISE_XOR:
          {
            o_integer left_oi;
            o_integer right_oi;
            o_integer result_oi;

            require_integer(location, kind, "left operand", left_value,
                            the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                break;

            require_integer(location, kind, "right operand", right_value,
                            the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                break;

            left_oi = integer_value_data(left_value);
            right_oi = integer_value_data(right_value);

            switch (kind)
              {
                case EK_BITWISE_AND:
                  {
                    oi_bitwise_and(result_oi, left_oi, right_oi);
                    break;
                  }
                case EK_BITWISE_OR:
                  {
                    oi_bitwise_or(result_oi, left_oi, right_oi);
                    break;
                  }
                case EK_BITWISE_XOR:
                  {
                    oi_bitwise_xor(result_oi, left_oi, right_oi);
                    break;
                  }
                default:
                  {
                    assert(FALSE);
                    result_oi = oi_null;
                  }
              }

            if (oi_out_of_memory(result_oi))
                break;

            result_value = create_integer_value(result_oi);

            oi_remove_reference(result_oi);

            break;
          }
        case EK_LOGICAL_AND:
        case EK_LOGICAL_OR:
          {
            right_value = left_value;
            value_add_reference(right_value);

            require_boolean(location, kind, "left operand", left_value,
                            the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                break;

            if (kind == EK_LOGICAL_AND)
              {
                if (get_value_kind(left_value) == VK_FALSE)
                  {
                    result_value = left_value;
                    value_add_reference(result_value);
                    break;
                  }
              }
            else
              {
                if (get_value_kind(left_value) == VK_TRUE)
                  {
                    result_value = left_value;
                    value_add_reference(result_value);
                    break;
                  }
              }

            value_remove_reference(right_value, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(left_value, the_jumper);
                return NULL;
              }

            right_value = evaluate_expression(right_expression, the_context,
                                              the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(right_value == NULL);
                value_remove_reference(left_value, the_jumper);
                return NULL;
              }

            assert(right_value != NULL);

            require_boolean(location, kind, "right operand", right_value,
                            the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                break;

            result_value = right_value;
            value_add_reference(right_value);

            break;
          }
        case EK_CONCATENATE:
          {
            assert(value_is_valid(left_value)); /* VERIFIED */
            assert(value_is_valid(right_value)); /* VERIFIED */
            if (((get_value_kind(left_value) == VK_STRING) ||
                 (get_value_kind(left_value) == VK_CHARACTER)) &&
                ((get_value_kind(right_value) == VK_STRING) ||
                 (get_value_kind(right_value) == VK_CHARACTER)))
              {
                result_value =
                        value_string_concatenate(left_value, right_value);
              }
            else if (((get_value_kind(left_value) == VK_MAP) ||
                      (get_value_kind(left_value) ==
                       VK_SEMI_LABELED_VALUE_LIST)) &&
                     ((get_value_kind(right_value) == VK_MAP) ||
                      (get_value_kind(right_value) ==
                       VK_SEMI_LABELED_VALUE_LIST)))
              {
                result_value = create_semi_labeled_value_list_value();
                if (result_value == NULL)
                    break;

                assert(value_is_valid(left_value)); /* VERIFIED */
                concatenate_onto_semi_labeled_value_list(result_value,
                        left_value, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    value_remove_reference(result_value, the_jumper);
                    result_value = NULL;
                    break;
                  }

                assert(value_is_valid(right_value)); /* VERIFIED */
                concatenate_onto_semi_labeled_value_list(result_value,
                        right_value, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    value_remove_reference(result_value, the_jumper);
                    result_value = NULL;
                    break;
                  }
              }
            else
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(binary_bad_operands),
                        "Illegal operands to concatenate expression: %s and "
                        "%s.", value_kind_name(left_value),
                        value_kind_name(right_value));
              }

            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

  done:
    if ((result_value == NULL) && jumper_flowing_forward(the_jumper))
        jumper_do_abort(the_jumper);

    value_remove_reference(left_value, the_jumper);
    value_remove_reference(right_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (result_value != NULL)
          {
            value_remove_reference(result_value, the_jumper);
            return NULL;
          }
      }

    return result_value;
  }

extern type *evaluate_type_expression(type_expression *the_type_expression,
                                      context *the_context, jumper *the_jumper)
  {
    assert(the_type_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    switch (get_type_expression_kind(the_type_expression))
      {
        case TEK_CONSTANT:
            return evaluate_constant_type_expression(the_type_expression,
                                                     the_context, the_jumper);
        case TEK_NAME:
            return evaluate_name_type_expression(the_type_expression,
                                                 the_context, the_jumper);
        case TEK_ENUMERATION:
            return evaluate_enumeration_type_expression(the_type_expression,
                    the_context, the_jumper);
        case TEK_NOT:
            return evaluate_not_type_expression(the_type_expression,
                                                the_context, the_jumper);
        case TEK_INTERSECTION:
            return evaluate_intersection_type_expression(the_type_expression,
                    the_context, the_jumper);
        case TEK_UNION:
            return evaluate_union_type_expression(the_type_expression,
                                                  the_context, the_jumper);
        case TEK_XOR:
            return evaluate_xor_type_expression(the_type_expression,
                                                the_context, the_jumper);
        case TEK_EXPRESSION:
            return evaluate_expression_type_expression(the_type_expression,
                    the_context, the_jumper);
        case TEK_ARRAY:
            return evaluate_array_type_expression(the_type_expression,
                                                  the_context, the_jumper);
        case TEK_INTEGER_RANGE:
            return evaluate_integer_range_type_expression(the_type_expression,
                    the_context, the_jumper);
        case TEK_RATIONAL_RANGE:
            return evaluate_rational_range_type_expression(the_type_expression,
                    the_context, the_jumper);
        case TEK_POINTER:
            return evaluate_pointer_type_expression(the_type_expression,
                                                    the_context, the_jumper);
        case TEK_TYPE:
            return evaluate_type_type_expression(the_type_expression,
                                                 the_context, the_jumper);
        case TEK_MAP:
            return evaluate_map_type_expression(the_type_expression,
                                                the_context, the_jumper);
        case TEK_ROUTINE:
            return evaluate_routine_type_expression(the_type_expression,
                                                    the_context, the_jumper);
        case TEK_FIELDS:
            return evaluate_fields_type_expression(the_type_expression,
                                                   the_context, the_jumper);
        case TEK_LEPTON:
            return evaluate_lepton_type_expression(the_type_expression,
                                                   the_context, the_jumper);
        case TEK_MULTISET:
            return evaluate_multiset_type_expression(the_type_expression,
                                                     the_context, the_jumper);
        case TEK_INTERFACE:
            return evaluate_interface_type_expression(the_type_expression,
                                                      the_context, the_jumper);
        case TEK_SEMI_LABELED_VALUE_LIST:
            return evaluate_semi_labeled_value_list_type_expression(
                    the_type_expression, the_context, the_jumper);
        case TEK_REGULAR_EXPRESSION:
            return evaluate_regular_expression_type_expression(
                    the_type_expression, the_context, the_jumper);
        default:
            assert(FALSE);
            return NULL;
      }
  }

extern type *evaluate_constant_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_CONSTANT);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    result = constant_type_expression_type(the_type_expression);
    type_add_reference(result);

    assert(type_is_valid(result)); /* VERIFIED */
    return result;
  }

extern type *evaluate_name_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_NAME);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    return evaluate_expression_as_type_expression(
            name_type_expression_name_expression(the_type_expression),
            the_context, the_jumper);
  }

extern type *evaluate_enumeration_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    size_t count;
    value **values;
    size_t number;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_ENUMERATION);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    count = enumeration_type_expression_case_count(the_type_expression);

    if (count == 0)
      {
        values = NULL;
      }
    else
      {
        values = MALLOC_ARRAY(value *, count);
        if (values == NULL)
            return NULL;
      }

    for (number = 0; number < count; ++number)
      {
        values[number] = evaluate_expression(
                enumeration_type_expression_case(the_type_expression, number),
                the_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(values[number] == NULL);
            while (number > 0)
              {
                --number;
                value_remove_reference(values[number], the_jumper);
              }
            if (values != NULL)
                free(values);
            return NULL;
          }

        assert(values[number] != NULL);

        check_value_validity(values[number],
                get_type_expression_location(the_type_expression), the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(values[number], the_jumper);
            while (number > 0)
              {
                --number;
                value_remove_reference(values[number], the_jumper);
              }
            if (values != NULL)
                free(values);
            return NULL;
          }

        assert(value_is_valid(values[number])); /* VERIFIED */
      }

    result = get_enumeration_type(count, values);

    if (result == NULL)
        jumper_do_abort(the_jumper);

    for (number = 0; number < count; ++number)
        value_remove_reference(values[number], the_jumper);

    if (values != NULL)
        free(values);

    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_not_type_expression(type_expression *the_type_expression,
        context *the_context, jumper *the_jumper)
  {
    type *base;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_NOT);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    base = evaluate_type_expression(
            not_type_expression_base(the_type_expression), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(base == NULL);
        return NULL;
      }

    assert(base != NULL);

    check_type_validity(base,
            get_type_expression_location(not_type_expression_base(
                    the_type_expression)), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(base, the_jumper);
        return NULL;
      }

    assert(type_is_valid(base)); /* VERIFIED */
    result = get_not_type(base);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    type_remove_reference(base, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_intersection_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    type *left;
    type *right;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_INTERSECTION);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    left = evaluate_type_expression(
            intersection_type_expression_left(the_type_expression),
            the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(left == NULL);
        return NULL;
      }

    assert(left != NULL);

    right = evaluate_type_expression(
            intersection_type_expression_right(the_type_expression),
            the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(right == NULL);
        type_remove_reference(left, the_jumper);
        return NULL;
      }

    assert(right != NULL);

    check_type_validity(left,
            get_type_expression_location(intersection_type_expression_left(
                    the_type_expression)), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(left, the_jumper);
        type_remove_reference(right, the_jumper);
        return NULL;
      }

    check_type_validity(right,
            get_type_expression_location(intersection_type_expression_right(
                    the_type_expression)), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(left, the_jumper);
        type_remove_reference(right, the_jumper);
        return NULL;
      }

    assert(type_is_valid(left)); /* VERIFIED */
    assert(type_is_valid(right)); /* VERIFIED */
    result = get_intersection_type(left, right);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    type_remove_reference(left, the_jumper);
    type_remove_reference(right, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_union_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    type *left;
    type *right;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_UNION);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    left = evaluate_type_expression(
            union_type_expression_left(the_type_expression), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(left == NULL);
        return NULL;
      }

    assert(left != NULL);

    right = evaluate_type_expression(
            union_type_expression_right(the_type_expression), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(right == NULL);
        type_remove_reference(left, the_jumper);
        return NULL;
      }

    assert(right != NULL);

    check_type_validity(left,
            get_type_expression_location(union_type_expression_left(
                    the_type_expression)), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(left, the_jumper);
        type_remove_reference(right, the_jumper);
        return NULL;
      }

    check_type_validity(right,
            get_type_expression_location(union_type_expression_right(
                    the_type_expression)), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(left, the_jumper);
        type_remove_reference(right, the_jumper);
        return NULL;
      }

    assert(type_is_valid(left)); /* VERIFIED */
    assert(type_is_valid(right)); /* VERIFIED */
    result = get_union_type(left, right);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    type_remove_reference(left, the_jumper);
    type_remove_reference(right, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_xor_type_expression(type_expression *the_type_expression,
        context *the_context, jumper *the_jumper)
  {
    type *left;
    type *right;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_XOR);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    left = evaluate_type_expression(
            xor_type_expression_left(the_type_expression), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(left == NULL);
        return NULL;
      }

    assert(left != NULL);

    right = evaluate_type_expression(
            xor_type_expression_right(the_type_expression), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(right == NULL);
        type_remove_reference(left, the_jumper);
        return NULL;
      }

    assert(right != NULL);

    check_type_validity(left,
            get_type_expression_location(xor_type_expression_left(
                    the_type_expression)), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(left, the_jumper);
        type_remove_reference(right, the_jumper);
        return NULL;
      }

    check_type_validity(right,
            get_type_expression_location(xor_type_expression_right(
                    the_type_expression)), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(left, the_jumper);
        type_remove_reference(right, the_jumper);
        return NULL;
      }

    assert(type_is_valid(left)); /* VERIFIED */
    assert(type_is_valid(right)); /* VERIFIED */
    result = get_xor_type(left, right);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    type_remove_reference(left, the_jumper);
    type_remove_reference(right, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_expression_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_EXPRESSION);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    return evaluate_expression_as_type_expression(
            expression_type_expression_expression(the_type_expression),
            the_context, the_jumper);
  }

extern type *evaluate_array_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    type_expression *base_expression;
    type *base;
    value *lower_value;
    o_integer lower_oi;
    value *upper_value;
    o_integer upper_oi;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_ARRAY);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    base_expression = array_type_expression_base(the_type_expression);
    base = evaluate_type_expression(base_expression, the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(base == NULL);
        return NULL;
      }

    assert(base != NULL);

    lower_value = evaluate_expression(
            array_type_expression_lower_bound(the_type_expression),
            the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(lower_value == NULL);
        type_remove_reference(base, the_jumper);
        return NULL;
      }

    assert(lower_value != NULL);

    if (get_value_kind(lower_value) != VK_INTEGER)
      {
        type_expression_exception(the_jumper, the_type_expression,
                EXCEPTION_TAG(array_type_bad_lower),
                "The lower bound of an array type expression evaluated to a "
                "%s, not an integer.", value_kind_name(lower_value));
        value_remove_reference(lower_value, the_jumper);
        type_remove_reference(base, the_jumper);
        return NULL;
      }

    lower_oi = integer_value_data(lower_value);
    oi_add_reference(lower_oi);
    value_remove_reference(lower_value, the_jumper);
    assert(jumper_flowing_forward(the_jumper));

    if ((oi_kind(lower_oi) == IIK_UNSIGNED_INFINITY) ||
        (oi_kind(lower_oi) == IIK_ZERO_ZERO))
      {
        type_expression_exception(the_jumper, the_type_expression,
                EXCEPTION_TAG(array_type_bad_lower),
                "The lower bound of an array type expression evaluated to %s.",
                ((oi_kind(lower_oi) == IIK_UNSIGNED_INFINITY) ?
                 "unsigned infinity" : "zero-zero"));
        oi_remove_reference(lower_oi);
        type_remove_reference(base, the_jumper);
        return NULL;
      }

    upper_value = evaluate_expression(
            array_type_expression_upper_bound(the_type_expression),
            the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(upper_value == NULL);
        oi_remove_reference(lower_oi);
        type_remove_reference(base, the_jumper);
        return NULL;
      }

    assert(upper_value != NULL);

    if (get_value_kind(upper_value) != VK_INTEGER)
      {
        type_expression_exception(the_jumper, the_type_expression,
                EXCEPTION_TAG(array_type_bad_upper),
                "The upper bound of an array type expression evaluated to a "
                "%s, not an integer.", value_kind_name(upper_value));
        value_remove_reference(upper_value, the_jumper);
        oi_remove_reference(lower_oi);
        type_remove_reference(base, the_jumper);
        return NULL;
      }

    upper_oi = integer_value_data(upper_value);
    oi_add_reference(upper_oi);
    value_remove_reference(upper_value, the_jumper);
    assert(jumper_flowing_forward(the_jumper));

    if ((oi_kind(upper_oi) == IIK_UNSIGNED_INFINITY) ||
        (oi_kind(upper_oi) == IIK_ZERO_ZERO))
      {
        type_expression_exception(the_jumper, the_type_expression,
                EXCEPTION_TAG(array_type_bad_upper),
                "The upper bound of an array type expression evaluated to %s.",
                ((oi_kind(upper_oi) == IIK_UNSIGNED_INFINITY) ?
                 "unsigned infinity" : "zero-zero"));
        oi_remove_reference(upper_oi);
        oi_remove_reference(lower_oi);
        type_remove_reference(base, the_jumper);
        return NULL;
      }

    check_type_validity(base, get_type_expression_location(base_expression),
                        the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        oi_remove_reference(upper_oi);
        oi_remove_reference(lower_oi);
        type_remove_reference(base, the_jumper);
        return NULL;
      }

    assert(type_is_valid(base)); /* VERIFIED */
    result = get_array_type(base, lower_oi, upper_oi);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    type_remove_reference(base, the_jumper);
    oi_remove_reference(lower_oi);
    oi_remove_reference(upper_oi);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_integer_range_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    value *lower_value;
    o_integer lower_oi;
    value *upper_value;
    o_integer upper_oi;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_INTEGER_RANGE);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    lower_value = evaluate_expression(
            integer_range_type_expression_lower_bound(the_type_expression),
            the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(lower_value == NULL);
        return NULL;
      }

    assert(lower_value != NULL);

    if (get_value_kind(lower_value) != VK_INTEGER)
      {
        type_expression_exception(the_jumper, the_type_expression,
                EXCEPTION_TAG(range_type_bad_lower),
                "The lower bound of an integer range type expression evaluated"
                " to a %s, not an integer.", value_kind_name(lower_value));
        value_remove_reference(lower_value, the_jumper);
        return NULL;
      }

    lower_oi = integer_value_data(lower_value);
    oi_add_reference(lower_oi);
    value_remove_reference(lower_value, the_jumper);
    assert(jumper_flowing_forward(the_jumper));

    if ((oi_kind(lower_oi) == IIK_UNSIGNED_INFINITY) ||
        (oi_kind(lower_oi) == IIK_ZERO_ZERO))
      {
        type_expression_exception(the_jumper, the_type_expression,
                EXCEPTION_TAG(range_type_bad_lower),
                "The lower bound of an integer range type expression evaluated"
                " to %s.",
                ((oi_kind(lower_oi) == IIK_UNSIGNED_INFINITY) ?
                 "unsigned infinity" : "zero-zero"));
        oi_remove_reference(lower_oi);
        return NULL;
      }

    upper_value = evaluate_expression(
            integer_range_type_expression_upper_bound(the_type_expression),
            the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(upper_value == NULL);
        oi_remove_reference(lower_oi);
        return NULL;
      }

    assert(upper_value != NULL);

    if (get_value_kind(upper_value) != VK_INTEGER)
      {
        type_expression_exception(the_jumper, the_type_expression,
                EXCEPTION_TAG(range_type_bad_upper),
                "The upper bound of an integer range type expression evaluated"
                " to a %s, not an integer.", value_kind_name(upper_value));
        value_remove_reference(upper_value, the_jumper);
        oi_remove_reference(lower_oi);
        return NULL;
      }

    upper_oi = integer_value_data(upper_value);
    oi_add_reference(upper_oi);
    value_remove_reference(upper_value, the_jumper);
    assert(jumper_flowing_forward(the_jumper));

    if ((oi_kind(upper_oi) == IIK_UNSIGNED_INFINITY) ||
        (oi_kind(upper_oi) == IIK_ZERO_ZERO))
      {
        type_expression_exception(the_jumper, the_type_expression,
                EXCEPTION_TAG(range_type_bad_upper),
                "The upper bound of an integer range type expression evaluated"
                " to %s.",
                ((oi_kind(upper_oi) == IIK_UNSIGNED_INFINITY) ?
                 "unsigned infinity" : "zero-zero"));
        oi_remove_reference(upper_oi);
        oi_remove_reference(lower_oi);
        return NULL;
      }

    result = get_integer_range_type(lower_oi, upper_oi,
            integer_range_type_expression_lower_is_inclusive(
                    the_type_expression),
            integer_range_type_expression_upper_is_inclusive(
                    the_type_expression));
    oi_remove_reference(lower_oi);
    oi_remove_reference(upper_oi);

    if (result == NULL)
        jumper_do_abort(the_jumper);

    return result;
  }

extern type *evaluate_rational_range_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    value *lower_value;
    rational *lower_rational;
    value *upper_value;
    rational *upper_rational;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) ==
           TEK_RATIONAL_RANGE);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    lower_value = evaluate_expression(
            rational_range_type_expression_lower_bound(the_type_expression),
            the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(lower_value == NULL);
        return NULL;
      }

    assert(lower_value != NULL);

    switch (get_value_kind(lower_value))
      {
        case VK_INTEGER:
          {
            o_integer lower_oi;

            lower_oi = integer_value_data(lower_value);

            if ((oi_kind(lower_oi) == IIK_UNSIGNED_INFINITY) ||
                (oi_kind(lower_oi) == IIK_ZERO_ZERO))
              {
                type_expression_exception(the_jumper, the_type_expression,
                        EXCEPTION_TAG(range_type_bad_lower),
                        "The lower bound of a rational range type expression "
                        "evaluated to %s.",
                        ((oi_kind(lower_oi) == IIK_UNSIGNED_INFINITY) ?
                         "unsigned infinity" : "zero-zero"));
                value_remove_reference(lower_value, the_jumper);
                return NULL;
              }

            lower_rational = create_rational(lower_oi, oi_one);
            if (lower_rational == NULL)
              {
                jumper_do_abort(the_jumper);
                value_remove_reference(lower_value, the_jumper);
                return NULL;
              }

            break;
          }
        case VK_RATIONAL:
          {
            lower_rational = rational_value_data(lower_value);
            rational_add_reference(lower_rational);
            break;
          }
        default:
          {
            type_expression_exception(the_jumper, the_type_expression,
                    EXCEPTION_TAG(range_type_bad_lower),
                    "The lower bound of a rational range type expression "
                    "evaluated to a %s, not a rational.",
                    value_kind_name(lower_value));
            value_remove_reference(lower_value, the_jumper);
            return NULL;
          }
      }

    value_remove_reference(lower_value, the_jumper);
    assert(jumper_flowing_forward(the_jumper));

    upper_value = evaluate_expression(
            rational_range_type_expression_upper_bound(the_type_expression),
            the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(upper_value == NULL);
        rational_remove_reference(lower_rational);
        return NULL;
      }

    assert(upper_value != NULL);

    switch (get_value_kind(upper_value))
      {
        case VK_INTEGER:
          {
            o_integer upper_oi;

            upper_oi = integer_value_data(upper_value);

            if ((oi_kind(upper_oi) == IIK_UNSIGNED_INFINITY) ||
                (oi_kind(upper_oi) == IIK_ZERO_ZERO))
              {
                type_expression_exception(the_jumper, the_type_expression,
                        EXCEPTION_TAG(range_type_bad_upper),
                        "The upper bound of a rational range type expression "
                        "evaluated to %s.",
                        ((oi_kind(upper_oi) == IIK_UNSIGNED_INFINITY) ?
                         "unsigned infinity" : "zero-zero"));
                value_remove_reference(upper_value, the_jumper);
                rational_remove_reference(lower_rational);
                return NULL;
              }

            upper_rational = create_rational(upper_oi, oi_one);
            if (upper_rational == NULL)
              {
                jumper_do_abort(the_jumper);
                value_remove_reference(upper_value, the_jumper);
                rational_remove_reference(lower_rational);
                return NULL;
              }

            break;
          }
        case VK_RATIONAL:
          {
            upper_rational = rational_value_data(upper_value);
            rational_add_reference(upper_rational);
            break;
          }
        default:
          {
            type_expression_exception(the_jumper, the_type_expression,
                    EXCEPTION_TAG(range_type_bad_upper),
                    "The upper bound of a rational range type expression "
                    "evaluated to a %s, not a rational.",
                    value_kind_name(upper_value));
            value_remove_reference(upper_value, the_jumper);
            rational_remove_reference(lower_rational);
            return NULL;
          }
      }

    value_remove_reference(upper_value, the_jumper);
    assert(jumper_flowing_forward(the_jumper));

    result = get_rational_range_type(lower_rational, upper_rational,
            rational_range_type_expression_lower_is_inclusive(
                    the_type_expression),
            rational_range_type_expression_upper_is_inclusive(
                    the_type_expression));
    rational_remove_reference(lower_rational);
    rational_remove_reference(upper_rational);

    if (result == NULL)
        jumper_do_abort(the_jumper);

    return result;
  }

extern type *evaluate_pointer_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    type_expression *base_expression;
    type *base;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_POINTER);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    base_expression = pointer_type_expression_base(the_type_expression);
    base = evaluate_type_expression(base_expression, the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(base == NULL);
        return NULL;
      }

    assert(base != NULL);

    check_type_validity(base, get_type_expression_location(base_expression),
                        the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(base, the_jumper);
        return NULL;
      }

    assert(type_is_valid(base)); /* VERIFIED */
    result = get_pointer_type(base,
            pointer_type_expression_read_allowed(the_type_expression),
            pointer_type_expression_write_allowed(the_type_expression),
            pointer_type_expression_null_allowed(the_type_expression));
    if (result == NULL)
        jumper_do_abort(the_jumper);

    type_remove_reference(base, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_type_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    type_expression *base_expression;
    type *base;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_TYPE);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    base_expression = type_type_expression_base(the_type_expression);
    base = evaluate_type_expression(base_expression, the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(base == NULL);
        return NULL;
      }

    assert(base != NULL);

    check_type_validity(base, get_type_expression_location(base_expression),
                        the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(base, the_jumper);
        return NULL;
      }

    assert(type_is_valid(base)); /* VERIFIED */
    result = get_type_type(base);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    type_remove_reference(base, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_map_type_expression(type_expression *the_type_expression,
        context *the_context, jumper *the_jumper)
  {
    type_expression *key_expression;
    type *key;
    type_expression *target_expression;
    type *target;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_MAP);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    key_expression = map_type_expression_key(the_type_expression);
    key = evaluate_type_expression(key_expression, the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(key == NULL);
        return NULL;
      }

    assert(key != NULL);

    target_expression = map_type_expression_target(the_type_expression);
    target = evaluate_type_expression(target_expression, the_context,
                                      the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(target == NULL);
        type_remove_reference(key, the_jumper);
        return NULL;
      }

    assert(target != NULL);

    check_type_validity(key, get_type_expression_location(key_expression),
                        the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(key, the_jumper);
        type_remove_reference(target, the_jumper);
        return NULL;
      }

    check_type_validity(target,
            get_type_expression_location(target_expression), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(key, the_jumper);
        type_remove_reference(target, the_jumper);
        return NULL;
      }

    assert(type_is_valid(key)); /* VERIFIED */
    assert(type_is_valid(target)); /* VERIFIED */
    result = get_map_type(key, target);
    if (result == NULL)
        jumper_do_abort(the_jumper);

    type_remove_reference(key, the_jumper);
    type_remove_reference(target, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_routine_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    type_expression *return_type_expression;
    type *return_type;
    size_t formal_count;
    type **argument_types;
    const char **argument_names;
    boolean *argument_has_defaults;
    size_t formal_num;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_ROUTINE);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    return_type_expression =
            routine_type_expression_return_type(the_type_expression);
    return_type = evaluate_type_expression(return_type_expression, the_context,
                                           the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(return_type == NULL);
        return NULL;
      }

    assert(return_type != NULL);

    formal_count = routine_type_expression_formal_count(the_type_expression);

    if (formal_count == 0)
      {
        argument_types = NULL;
        argument_names = NULL;
        argument_has_defaults = NULL;
      }
    else
      {
        size_t formal_num;

        argument_types = MALLOC_ARRAY(type *, formal_count);
        if (argument_types == NULL)
          {
            jumper_do_abort(the_jumper);
            type_remove_reference(return_type, the_jumper);
            return NULL;
          }

        argument_names = MALLOC_ARRAY(const char *, formal_count);
        if (argument_names == NULL)
          {
            jumper_do_abort(the_jumper);
            free(argument_types);
            type_remove_reference(return_type, the_jumper);
            return NULL;
          }

        argument_has_defaults = MALLOC_ARRAY(boolean, formal_count);
        if (argument_has_defaults == NULL)
          {
            jumper_do_abort(the_jumper);
            free(argument_names);
            free(argument_types);
            type_remove_reference(return_type, the_jumper);
            return NULL;
          }

        for (formal_num = 0; formal_num < formal_count; ++formal_num)
          {
            argument_types[formal_num] = evaluate_type_expression(
                    routine_type_expression_formal_argument_type(
                            the_type_expression, formal_num), the_context,
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(argument_types[formal_num] == NULL);
                while (formal_num > 0)
                  {
                    --formal_num;
                    type_remove_reference(argument_types[formal_num],
                                          the_jumper);
                  }
                free(argument_has_defaults);
                free(argument_names);
                free(argument_types);
                type_remove_reference(return_type, the_jumper);
                return NULL;
              }

            assert(argument_types[formal_num] != NULL);

            argument_names[formal_num] = routine_type_expression_formal_name(
                    the_type_expression, formal_num);
            argument_has_defaults[formal_num] =
                    routine_type_expression_formal_has_default_value(
                            the_type_expression, formal_num);
          }
      }

    check_type_validity(return_type,
            get_type_expression_location(return_type_expression), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (formal_count > 0)
          {
            size_t formal_num;

            for (formal_num = 0; formal_num < formal_count; ++formal_num)
                type_remove_reference(argument_types[formal_num], the_jumper);
            free(argument_has_defaults);
            free(argument_names);
            free(argument_types);
          }
        type_remove_reference(return_type, the_jumper);
        return NULL;
      }

    for (formal_num = 0; formal_num < formal_count; ++formal_num)
      {
        check_type_validity(argument_types[formal_num],
                get_type_expression_location(
                        routine_type_expression_formal_argument_type(
                                the_type_expression, formal_num)), the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            size_t formal_num;

            for (formal_num = 0; formal_num < formal_count; ++formal_num)
                type_remove_reference(argument_types[formal_num], the_jumper);
            free(argument_has_defaults);
            free(argument_names);
            free(argument_types);
            type_remove_reference(return_type, the_jumper);
            return NULL;
          }
      }

    assert(type_is_valid(return_type)); /* VERIFIED */
    result = get_routine_type(return_type, formal_count, argument_types,
            argument_names, argument_has_defaults,
            routine_type_expression_extra_arguments_allowed(
                    the_type_expression),
            routine_type_expression_extra_arguments_unspecified(
                    the_type_expression));
    if (result == NULL)
        jumper_do_abort(the_jumper);

    if (formal_count > 0)
      {
        size_t formal_num;

        for (formal_num = 0; formal_num < formal_count; ++formal_num)
            type_remove_reference(argument_types[formal_num], the_jumper);
        free(argument_has_defaults);
        free(argument_names);
        free(argument_types);
      }
    type_remove_reference(return_type, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_fields_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    size_t field_count;
    type **field_types;
    const char **field_names;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_FIELDS);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    field_count = fields_type_expression_field_count(the_type_expression);

    if (field_count == 0)
      {
        field_types = NULL;
        field_names = NULL;
      }
    else
      {
        size_t field_num;

        field_types = MALLOC_ARRAY(type *, field_count);
        if (field_types == NULL)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        field_names = MALLOC_ARRAY(const char *, field_count);
        if (field_names == NULL)
          {
            jumper_do_abort(the_jumper);
            free(field_types);
            return NULL;
          }

        for (field_num = 0; field_num < field_count; ++field_num)
          {
            field_types[field_num] = evaluate_type_expression(
                    fields_type_expression_field_type(the_type_expression,
                                                      field_num), the_context,
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(field_types[field_num] == NULL);
                while (field_num > 0)
                  {
                    --field_num;
                    type_remove_reference(field_types[field_num], the_jumper);
                  }
                free(field_names);
                free(field_types);
                return NULL;
              }

            assert(field_types[field_num] != NULL);

            field_names[field_num] = fields_type_expression_field_name(
                    the_type_expression, field_num);
          }
      }

    result = get_fields_type(field_count, field_types, field_names,
            fields_type_expression_extra_fields_allowed(the_type_expression));
    if (result == NULL)
        jumper_do_abort(the_jumper);

    if (field_count > 0)
      {
        size_t field_num;

        for (field_num = 0; field_num < field_count; ++field_num)
            type_remove_reference(field_types[field_num], the_jumper);
        free(field_names);
        free(field_types);
      }
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_lepton_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    value *lepton_value;
    lepton_key_instance *key;
    size_t field_count;
    type **field_types;
    const char **field_names;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_LEPTON);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    lepton_value= evaluate_expression(
            lepton_type_expression_lepton(the_type_expression), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(lepton_value == NULL);
        return NULL;
      }

    assert(lepton_value != NULL);

    if (get_value_kind(lepton_value) != VK_LEPTON_KEY)
      {
        type_expression_exception(the_jumper, the_type_expression,
                EXCEPTION_TAG(lepton_type_bad_key),
                "The key expression of a lepton type expression evaluated to a"
                " %s, not a lepton key.", value_kind_name(lepton_value));
        value_remove_reference(lepton_value, the_jumper);
        return NULL;
      }

    key = value_lepton_key(lepton_value);
    lepton_key_instance_add_reference(key);
    value_remove_reference(lepton_value, the_jumper);
    assert(jumper_flowing_forward(the_jumper));

    field_count = lepton_type_expression_field_count(the_type_expression);

    if (field_count == 0)
      {
        field_types = NULL;
        field_names = NULL;
      }
    else
      {
        size_t field_num;

        field_types = MALLOC_ARRAY(type *, field_count);
        if (field_types == NULL)
          {
            jumper_do_abort(the_jumper);
            lepton_key_instance_remove_reference(key, the_jumper);
            return NULL;
          }

        field_names = MALLOC_ARRAY(const char *, field_count);
        if (field_names == NULL)
          {
            jumper_do_abort(the_jumper);
            free(field_types);
            lepton_key_instance_remove_reference(key, the_jumper);
            return NULL;
          }

        for (field_num = 0; field_num < field_count; ++field_num)
          {
            type_expression *field_type_expression;

            field_type_expression = lepton_type_expression_field_type(
                    the_type_expression, field_num);
            field_types[field_num] = evaluate_type_expression(
                    field_type_expression, the_context, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(field_types[field_num] == NULL);
                while (field_num > 0)
                  {
                    --field_num;
                    type_remove_reference(field_types[field_num], the_jumper);
                  }
                free(field_names);
                free(field_types);
                lepton_key_instance_remove_reference(key, the_jumper);
                return NULL;
              }

            assert(field_types[field_num] != NULL);

            check_type_validity(field_types[field_num],
                    get_type_expression_location(field_type_expression),
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                type_remove_reference(field_types[field_num], the_jumper);
                while (field_num > 0)
                  {
                    --field_num;
                    type_remove_reference(field_types[field_num], the_jumper);
                  }
                free(field_names);
                free(field_types);
                lepton_key_instance_remove_reference(key, the_jumper);
                return NULL;
              }

            assert(type_is_valid(field_types[field_num])); /* VERIFIED */

            field_names[field_num] = lepton_type_expression_field_name(
                    the_type_expression, field_num);
          }
      }

    result = check_and_get_lepton_type(key, field_count, field_types,
            field_names,
            lepton_type_expression_extra_fields_allowed(the_type_expression),
            get_type_expression_location(the_type_expression), the_jumper);
    assert((!(jumper_flowing_forward(the_jumper))) || type_is_valid(result));
            /* VERIFIED */
    lepton_key_instance_remove_reference(key, the_jumper);

    if (field_count > 0)
      {
        size_t field_num;

        for (field_num = 0; field_num < field_count; ++field_num)
            type_remove_reference(field_types[field_num], the_jumper);
        free(field_names);
        free(field_types);
      }
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_multiset_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    size_t field_count;
    type **field_types;
    const char **field_names;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_MULTISET);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    field_count = multiset_type_expression_field_count(the_type_expression);

    if (field_count == 0)
      {
        field_types = NULL;
        field_names = NULL;
      }
    else
      {
        size_t field_num;

        field_types = MALLOC_ARRAY(type *, field_count);
        if (field_types == NULL)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        field_names = MALLOC_ARRAY(const char *, field_count);
        if (field_names == NULL)
          {
            jumper_do_abort(the_jumper);
            free(field_types);
            return NULL;
          }

        for (field_num = 0; field_num < field_count; ++field_num)
          {
            field_types[field_num] = evaluate_type_expression(
                    multiset_type_expression_field_type(the_type_expression,
                                                        field_num),
                    the_context, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(field_types[field_num] == NULL);
                while (field_num > 0)
                  {
                    --field_num;
                    type_remove_reference(field_types[field_num], the_jumper);
                  }
                free(field_names);
                free(field_types);
                return NULL;
              }

            assert(field_types[field_num] != NULL);

            field_names[field_num] = multiset_type_expression_field_name(
                    the_type_expression, field_num);
          }
      }

    result = get_multiset_type(field_count, field_types, field_names,
            multiset_type_expression_extra_fields_allowed(
                    the_type_expression));
    if (result == NULL)
        jumper_do_abort(the_jumper);

    if (field_count > 0)
      {
        size_t field_num;

        for (field_num = 0; field_num < field_count; ++field_num)
            type_remove_reference(field_types[field_num], the_jumper);
        free(field_names);
        free(field_types);
      }
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_interface_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    size_t item_count;
    type **item_types;
    const char **item_names;
    boolean *item_writing_alloweds;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) == TEK_INTERFACE);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    item_count = interface_type_expression_item_count(the_type_expression);

    if (item_count == 0)
      {
        item_types = NULL;
        item_names = NULL;
        item_writing_alloweds = NULL;
      }
    else
      {
        size_t item_num;

        item_types = MALLOC_ARRAY(type *, item_count);
        if (item_types == NULL)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        item_names = MALLOC_ARRAY(const char *, item_count);
        if (item_names == NULL)
          {
            jumper_do_abort(the_jumper);
            free(item_types);
            return NULL;
          }

        item_writing_alloweds = MALLOC_ARRAY(boolean, item_count);
        if (item_writing_alloweds == NULL)
          {
            jumper_do_abort(the_jumper);
            free(item_names);
            free(item_types);
            return NULL;
          }

        for (item_num = 0; item_num < item_count; ++item_num)
          {
            item_types[item_num] = evaluate_type_expression(
                    interface_type_expression_item_type(the_type_expression,
                                                        item_num), the_context,
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(item_types[item_num] == NULL);
                while (item_num > 0)
                  {
                    --item_num;
                    type_remove_reference(item_types[item_num], the_jumper);
                  }
                free(item_writing_alloweds);
                free(item_names);
                free(item_types);
                return NULL;
              }

            assert(item_types[item_num] != NULL);

            item_names[item_num] = interface_type_expression_item_name(
                    the_type_expression, item_num);
            item_writing_alloweds[item_num] =
                    interface_type_expression_item_writing_allowed(
                            the_type_expression, item_num);
          }
      }

    result = get_interface_type(item_count, item_types, item_names,
            item_writing_alloweds,
            interface_type_expression_null_allowed(the_type_expression));
    if (result == NULL)
        jumper_do_abort(the_jumper);

    if (item_count > 0)
      {
        size_t item_num;

        for (item_num = 0; item_num < item_count; ++item_num)
            type_remove_reference(item_types[item_num], the_jumper);
        free(item_writing_alloweds);
        free(item_names);
        free(item_types);
      }
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_semi_labeled_value_list_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    size_t element_count;
    type **element_types;
    const char **element_names;
    size_t element_num;
    type *result;

    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) ==
           TEK_SEMI_LABELED_VALUE_LIST);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    element_count = semi_labeled_value_list_type_expression_element_count(
            the_type_expression);

    if (element_count == 0)
      {
        element_types = NULL;
        element_names = NULL;
      }
    else
      {
        size_t element_num;

        element_types = MALLOC_ARRAY(type *, element_count);
        if (element_types == NULL)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        element_names = MALLOC_ARRAY(const char *, element_count);
        if (element_names == NULL)
          {
            jumper_do_abort(the_jumper);
            free(element_types);
            return NULL;
          }

        for (element_num = 0; element_num < element_count; ++element_num)
          {
            element_types[element_num] = evaluate_type_expression(
                    semi_labeled_value_list_type_expression_element_type(
                            the_type_expression, element_num), the_context,
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(element_types[element_num] == NULL);
                while (element_num > 0)
                  {
                    --element_num;
                    type_remove_reference(element_types[element_num],
                                          the_jumper);
                  }
                free(element_names);
                free(element_types);
                return NULL;
              }

            assert(element_types[element_num] != NULL);

            element_names[element_num] =
                    semi_labeled_value_list_type_expression_element_name(
                            the_type_expression, element_num);
          }
      }

    for (element_num = 0; element_num < element_count; ++element_num)
      {
        check_type_validity(element_types[element_num],
                get_type_expression_location(
                        semi_labeled_value_list_type_expression_element_type(
                                the_type_expression, element_num)),
                the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            size_t element_num;

            for (element_num = 0; element_num < element_count; ++element_num)
                type_remove_reference(element_types[element_num], the_jumper);
            free(element_names);
            free(element_types);
            return NULL;
          }
      }

    result = get_semi_labeled_value_list_type(element_count, element_types,
            element_names,
            semi_labeled_value_list_type_expression_extra_elements_allowed(
                    the_type_expression));
    if (result == NULL)
        jumper_do_abort(the_jumper);

    if (element_count > 0)
      {
        size_t element_num;

        for (element_num = 0; element_num < element_count; ++element_num)
            type_remove_reference(element_types[element_num], the_jumper);
        free(element_names);
        free(element_types);
      }
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

extern type *evaluate_regular_expression_type_expression(
        type_expression *the_type_expression, context *the_context,
        jumper *the_jumper)
  {
    assert(the_type_expression != NULL);
    assert(get_type_expression_kind(the_type_expression) ==
           TEK_REGULAR_EXPRESSION);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    return get_regular_expression_type(
            regular_expression_type_expression_regular_expression(
                    the_type_expression));
  }

extern slot_location *evaluate_address_of_expression(
        expression *the_expression, context *the_context, jumper *the_jumper)
  {
    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(expression_is_addressable(the_expression));

    switch (get_expression_kind(the_expression))
      {
        case EK_VARIABLE_REFERENCE:
          {
            variable_instance *instance;

            instance = variable_reference_expression_instance(the_expression,
                                                              the_context);
            assert(instance != NULL);

            return create_variable_slot_location(instance,
                    get_expression_location(the_expression), the_jumper);
          }
        case EK_USE_REFERENCE:
          {
            instance *the_instance;
            routine_instance_chain *instance_chain;
            jump_target *the_jump_target;

            find_instance_from_use_statement(
                    use_reference_expression_use_statement(the_expression),
                    use_reference_expression_used_for_num(the_expression),
                    the_context, TRUE, &the_instance, &instance_chain,
                    &the_jump_target, get_expression_location(the_expression),
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(the_instance == NULL);
                assert(instance_chain == NULL);
                return NULL;
              }

            if (the_instance != NULL)
              {
                assert(instance_chain == NULL);
                assert(the_jump_target == NULL);

                switch (instance_kind(the_instance))
                  {
                    case NK_VARIABLE:
                        return create_variable_slot_location(
                                instance_variable_instance(the_instance),
                                get_expression_location(the_expression),
                                the_jumper);
                    case NK_ROUTINE:
                      routine_reference:
                        expression_exception(the_jumper, the_expression,
                                EXCEPTION_TAG(routine_addressed),
                                "An attempt was made to take the address of a "
                                "routine.");
                        return NULL;
                    case NK_TAGALONG:
                        expression_exception(the_jumper, the_expression,
                                EXCEPTION_TAG(tagalong_key_addressed),
                                "An attempt was made to take the address of a "
                                "tagalong key.");
                        return NULL;
                    case NK_LEPTON_KEY:
                        expression_exception(the_jumper, the_expression,
                                EXCEPTION_TAG(lepton_key_addressed),
                                "An attempt was made to take the address of a "
                                "lepton key.");
                        return NULL;
                    case NK_QUARK:
                        expression_exception(the_jumper, the_expression,
                                EXCEPTION_TAG(quark_addressed),
                                "An attempt was made to take the address of a "
                                "quark.");
                        return NULL;
                    case NK_LOCK:
                        expression_exception(the_jumper, the_expression,
                                EXCEPTION_TAG(lock_addressed),
                                "An attempt was made to take the address of a "
                                "lock.");
                        return NULL;
                    default:
                        assert(FALSE);
                        return NULL;
                  }
              }
            else if (instance_chain != NULL)
              {
                assert(the_jump_target == NULL);

                routine_instance_chain_remove_reference(instance_chain,
                                                        the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                    return NULL;
                goto routine_reference;
              }
            else
              {
                assert(the_jump_target != NULL);

                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(label_addressed),
                        "An attempt was made to take the address of a label.");
                return NULL;
              }
          }
        case EK_LOOKUP:
          {
            slot_location *base_slot;
            lookup_actual_arguments *actuals;
            value *overload_base;
            slot_location *lookup_slot;

            base_slot = evaluate_address_of_expression(
                    lookup_expression_base(the_expression), the_context,
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(base_slot == NULL);
                return NULL;
              }

            assert(base_slot != NULL);

            actuals = evaluate_lookup_arguments(the_expression, the_context,
                                                the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(actuals == NULL);
                slot_location_remove_reference(base_slot, the_jumper);
                return NULL;
              }

            assert(actuals != NULL);

            overload_base = overload_base_for_expression(the_expression,
                    the_context, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(overload_base == NULL);
                delete_lookup_actual_arguments(actuals, the_jumper);
                slot_location_remove_reference(base_slot, the_jumper);
                return NULL;
              }

            lookup_slot = create_lookup_slot_location(base_slot, actuals,
                    overload_base, get_expression_location(the_expression),
                    the_jumper);
            if (overload_base != NULL)
                value_remove_reference(overload_base, the_jumper);
            slot_location_remove_reference(base_slot, the_jumper);
            if ((!(jumper_flowing_forward(the_jumper))) &&
                (lookup_slot != NULL))
              {
                slot_location_remove_reference(lookup_slot, the_jumper);
                return NULL;
              }

            return lookup_slot;
          }
        case EK_FIELD:
          {
            slot_location *base_slot;
            slot_location *field_slot;

            base_slot = evaluate_address_of_expression(
                    field_expression_base(the_expression), the_context,
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(base_slot == NULL);
                return NULL;
              }

            assert(base_slot != NULL);

            field_slot = create_field_slot_location(base_slot,
                    field_expression_field_name(the_expression), NULL,
                    get_expression_location(the_expression), the_jumper);
            slot_location_remove_reference(base_slot, the_jumper);
            if ((!(jumper_flowing_forward(the_jumper))) &&
                (field_slot != NULL))
              {
                slot_location_remove_reference(field_slot, the_jumper);
                return NULL;
              }

            return field_slot;
          }
        case EK_POINTER_FIELD:
          {
            value *base_value;
            value *overload_base;
            slot_location *base_slot;
            slot_location *field_slot;

            base_value = evaluate_expression(
                    pointer_field_expression_base(the_expression), the_context,
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(base_value == NULL);
                return NULL;
              }

            assert(base_value != NULL);

            overload_base = overload_base_for_expression(the_expression,
                    the_context, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(overload_base == NULL);
                value_remove_reference(base_value, the_jumper);
                return NULL;
              }

            if (get_value_kind(base_value) != VK_SLOT_LOCATION)
              {
                value *field_name_value;
                value *operator_base;

                field_name_value = create_string_value(
                        pointer_field_expression_field_name(the_expression));
                if (field_name_value == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    if (overload_base != NULL)
                        value_remove_reference(overload_base, the_jumper);
                    value_remove_reference(base_value, the_jumper);
                    return NULL;
                  }

                operator_base = find_overload_operator(base_value,
                        "operator->", the_jumper,
                        get_expression_location(the_expression));
                if ((operator_base != NULL) &&
                    ((get_value_kind(operator_base) == VK_ROUTINE) ||
                     (get_value_kind(operator_base) == VK_ROUTINE_CHAIN)))
                  {
                    slot_location *call_slot;

                    assert((get_value_kind(operator_base) == VK_ROUTINE) ||
                           (get_value_kind(operator_base) ==
                            VK_ROUTINE_CHAIN));
                    call_slot = create_call_slot_location(operator_base,
                            "pointer field access", field_name_value, "field",
                            NULL, NULL, overload_base, base_value,
                            get_expression_location(the_expression),
                            the_jumper);

                    value_remove_reference(operator_base, the_jumper);
                    value_remove_reference(field_name_value, the_jumper);
                    if (overload_base != NULL)
                        value_remove_reference(overload_base, the_jumper);
                    value_remove_reference(base_value, the_jumper);
                    if ((!(jumper_flowing_forward(the_jumper))) &&
                        (call_slot != NULL))
                      {
                        slot_location_remove_reference(call_slot, the_jumper);
                        return NULL;
                      }

                    return call_slot;
                  }
                else if (operator_base != NULL)
                  {
                    value_remove_reference(operator_base, the_jumper);
                  }

                if (overload_base != NULL)
                  {
                    slot_location *call_slot;

                    assert((get_value_kind(overload_base) == VK_ROUTINE) ||
                           (get_value_kind(overload_base) ==
                            VK_ROUTINE_CHAIN));
                    call_slot = create_call_slot_location(overload_base,
                            "pointer field access", base_value, NULL,
                            field_name_value, "field", NULL, NULL,
                            get_expression_location(the_expression),
                            the_jumper);

                    value_remove_reference(field_name_value, the_jumper);
                    value_remove_reference(overload_base, the_jumper);
                    value_remove_reference(base_value, the_jumper);
                    if ((!(jumper_flowing_forward(the_jumper))) &&
                        (call_slot != NULL))
                      {
                        slot_location_remove_reference(call_slot, the_jumper);
                        return NULL;
                      }

                    return call_slot;
                  }

                value_remove_reference(field_name_value, the_jumper);

                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(pointer_field_reference_bad_base),
                        "The base operand of a pointer field access evaluated "
                        "to something other than a slot location.");
                value_remove_reference(base_value, the_jumper);
                return NULL;
              }

            base_slot = slot_location_value_data(base_value);
            assert(base_slot != NULL);

            field_slot = create_field_slot_location(base_slot,
                    pointer_field_expression_field_name(the_expression),
                    overload_base, get_expression_location(the_expression),
                    the_jumper);
            if (field_slot == NULL)
                jumper_do_abort(the_jumper);

            if (overload_base != NULL)
                value_remove_reference(overload_base, the_jumper);
            value_remove_reference(base_value, the_jumper);
            if ((!(jumper_flowing_forward(the_jumper))) &&
                (field_slot != NULL))
              {
                slot_location_remove_reference(field_slot, the_jumper);
                return NULL;
              }

            return field_slot;
          }
        case EK_TAGALONG_FIELD:
          {
            slot_location *base_slot;
            value *key_value;
            tagalong_key *the_tagalong_key;
            slot_location *field_slot;

            base_slot = evaluate_address_of_expression(
                    tagalong_field_expression_base(the_expression),
                    the_context, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(base_slot == NULL);
                return NULL;
              }

            assert(base_slot != NULL);

            key_value = evaluate_expression(
                    tagalong_field_expression_key(the_expression), the_context,
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(key_value == NULL);
                slot_location_remove_reference(base_slot, the_jumper);
                return NULL;
              }

            assert(key_value != NULL);

            if (get_value_kind(key_value) != VK_TAGALONG_KEY)
              {
                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(tagalong_reference_bad_key),
                        "The key operand for a tagalong field access evaluated"
                        " to something other than a tagalong key reference.");
                value_remove_reference(key_value, the_jumper);
                slot_location_remove_reference(base_slot, the_jumper);
                return NULL;
              }

            the_tagalong_key = tagalong_key_value_data(key_value);
            assert(the_tagalong_key != NULL);

            field_slot = create_tagalong_field_slot_location(base_slot,
                    the_tagalong_key, get_expression_location(the_expression),
                    the_jumper);

            value_remove_reference(key_value, the_jumper);
            slot_location_remove_reference(base_slot, the_jumper);
            if ((!(jumper_flowing_forward(the_jumper))) &&
                (field_slot != NULL))
              {
                slot_location_remove_reference(field_slot, the_jumper);
                return NULL;
              }
            return field_slot;
          }
        case EK_DEREFERENCE:
          {
            value *base_value;
            value *overload_base;
            slot_location *base_slot;
            slot_location *result_slot;

            base_value = evaluate_expression(
                    unary_expression_operand(the_expression), the_context,
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(base_value == NULL);
                return NULL;
              }

            assert(base_value != NULL);

            overload_base = overload_base_for_expression(the_expression,
                    the_context, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(overload_base == NULL);
                value_remove_reference(base_value, the_jumper);
                return NULL;
              }

            if (get_value_kind(base_value) != VK_SLOT_LOCATION)
              {
                value *operator_base;

                operator_base = find_overload_operator(base_value, "operator*",
                        the_jumper, get_expression_location(the_expression));
                if ((operator_base != NULL) &&
                    ((get_value_kind(operator_base) == VK_ROUTINE) ||
                     (get_value_kind(operator_base) == VK_ROUTINE_CHAIN)))
                  {
                    slot_location *call_slot;

                    assert((get_value_kind(operator_base) == VK_ROUTINE) ||
                           (get_value_kind(operator_base) ==
                            VK_ROUTINE_CHAIN));
                    call_slot = create_call_slot_location(operator_base,
                            "dereference", NULL, NULL, NULL, NULL,
                            overload_base, base_value,
                            get_expression_location(the_expression),
                            the_jumper);

                    value_remove_reference(operator_base, the_jumper);
                    if (overload_base != NULL)
                        value_remove_reference(overload_base, the_jumper);
                    value_remove_reference(base_value, the_jumper);
                    if ((!(jumper_flowing_forward(the_jumper))) &&
                        (call_slot != NULL))
                      {
                        slot_location_remove_reference(call_slot, the_jumper);
                        return NULL;
                      }

                    return call_slot;
                  }
                else if (operator_base != NULL)
                  {
                    value_remove_reference(operator_base, the_jumper);
                  }

                if (overload_base != NULL)
                  {
                    slot_location *call_slot;

                    assert((get_value_kind(overload_base) == VK_ROUTINE) ||
                           (get_value_kind(overload_base) ==
                            VK_ROUTINE_CHAIN));
                    call_slot = create_call_slot_location(overload_base,
                            "dereference", base_value, NULL, NULL, NULL, NULL,
                            NULL, get_expression_location(the_expression),
                            the_jumper);

                    value_remove_reference(overload_base, the_jumper);
                    value_remove_reference(base_value, the_jumper);
                    if ((!(jumper_flowing_forward(the_jumper))) &&
                        (call_slot != NULL))
                      {
                        slot_location_remove_reference(call_slot, the_jumper);
                        return NULL;
                      }

                    return call_slot;
                  }

                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(dereference_bad_base),
                        "The operand of a pointer dereference expression "
                        "evaluated to something other than a slot location.");
                value_remove_reference(base_value, the_jumper);
                return NULL;
              }

            base_slot = slot_location_value_data(base_value);
            assert(base_slot != NULL);

            if (overload_base != NULL)
              {
                result_slot = create_pass_slot_location(base_slot,
                        overload_base, get_expression_location(the_expression),
                        the_jumper);

                value_remove_reference(overload_base, the_jumper);
              }
            else
              {
                slot_location_add_reference(base_slot);
                result_slot = base_slot;
              }

            value_remove_reference(base_value, the_jumper);

            if ((!(jumper_flowing_forward(the_jumper))) &&
                (result_slot != NULL))
              {
                slot_location_remove_reference(result_slot, the_jumper);
                return NULL;
              }

            return result_slot;
          }
        default:
          {
            assert(FALSE);
            return NULL;
          }
      }
  }

extern value *get_slot_contents(slot_location *the_slot_location,
        lock_instance_aa *delayed_unlocks,
        const source_location *the_source_location, jumper *the_jumper)
  {
    assert(the_slot_location != NULL);
    assert(the_source_location != NULL);
    assert(the_jumper != NULL);

    switch (get_slot_location_kind(the_slot_location))
      {
        case SLK_VARIABLE:
          {
            variable_instance *instance;

            instance = variable_slot_location_variable(the_slot_location);
            assert(instance != NULL);

            return read_variable_value(instance, delayed_unlocks,
                                       the_source_location, the_jumper);
          }
        case SLK_LOOKUP:
          {
            slot_location *base_slot;
            value *base_value;
            lookup_actual_arguments *actuals;
            value *result;

            base_slot = lookup_slot_location_base(the_slot_location);
            assert(base_slot != NULL);

            base_value = get_slot_contents(base_slot, delayed_unlocks,
                                           the_source_location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(base_value == NULL);
                return NULL;
              }

            assert(base_value != NULL);

            actuals = lookup_slot_location_actuals(the_slot_location);
            assert(actuals != NULL);

            check_lookup_actual_arguments_validity(actuals,
                    the_source_location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(base_value, the_jumper);
                return NULL;
              }

            assert(lookup_actual_arguments_is_valid(actuals));
                    /* VERIFIED */
            result = do_lookup(base_value, actuals,
                    slot_location_overload_base(the_slot_location),
                    the_source_location, the_jumper);
            value_remove_reference(base_value, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                if (result != NULL)
                    value_remove_reference(result, the_jumper);
                return NULL;
              }
            return result;
          }
        case SLK_FIELD:
          {
            slot_location *base_slot;
            value *overload_base;
            value *base_value;
            value *result;

            base_slot = field_slot_location_base(the_slot_location);
            assert(base_slot != NULL);

            overload_base = slot_location_overload_base(the_slot_location);
            if (overload_base != NULL)
              {
                value *base_value;
                verdict the_verdict;

                base_value = create_slot_location_value(base_slot);
                if (base_value == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    return NULL;
                  }

                the_verdict = try_scoped_field_overloading(overload_base,
                        &result, base_value,
                        field_slot_location_field_name(the_slot_location),
                        NULL, the_jumper, the_source_location);
                value_remove_reference(base_value, the_jumper);
                if (the_verdict == MISSION_ACCOMPLISHED)
                    return result;
                assert(result == NULL);
                if (!(jumper_flowing_forward(the_jumper)))
                    return NULL;
              }

            base_value = get_slot_contents(base_slot, delayed_unlocks,
                                           the_source_location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(base_value == NULL);
                return NULL;
              }

            assert(base_value != NULL);

            result = read_field_value(base_value,
                    field_slot_location_field_name(the_slot_location),
                    the_source_location, the_jumper);
            value_remove_reference(base_value, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                if (result != NULL)
                    value_remove_reference(result, the_jumper);
                return NULL;
              }
            return result;
          }
        case SLK_TAGALONG:
          {
            slot_location *base_slot;
            value *base_value;
            tagalong_key *key;
            lock_chain *the_lock_chain;
            value *result;

            base_slot = tagalong_slot_location_base(the_slot_location);
            assert(base_slot != NULL);

            base_value = get_slot_contents(base_slot, delayed_unlocks,
                                           the_source_location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(base_value == NULL);
                return NULL;
              }

            assert(base_value != NULL);

            key = tagalong_slot_location_key(the_slot_location);
            assert(key != NULL);

            instance_check_validity(tagalong_key_instance(key),
                                    the_source_location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(base_value, the_jumper);
                return NULL;
              }

            assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
            assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */
            the_lock_chain = tagalong_key_lock_chain(key);

            if (the_lock_chain != NULL)
              {
                lock_chain_grab(the_lock_chain, the_source_location,
                                the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    value_remove_reference(base_value, the_jumper);
                    return NULL;
                  }
              }

            assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
            assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */
            result = lookup_tagalong(base_value, key, FALSE,
                                     the_source_location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(result == NULL);
                value_remove_reference(base_value, the_jumper);
                if (the_lock_chain != NULL)
                  {
                    lock_chain_release(the_lock_chain, the_source_location,
                                       the_jumper);
                  }
                return NULL;
              }
            if (result == NULL)
              {
                location_exception(the_jumper, the_source_location,
                        EXCEPTION_TAG(tagalong_unset),
                        "Attempted to read a tagalong from a value for which "
                        "that tagalong was not set.");
                value_remove_reference(base_value, the_jumper);
                if (the_lock_chain != NULL)
                  {
                    lock_chain_release(the_lock_chain, the_source_location,
                                       the_jumper);
                  }
                return NULL;
              }

            value_remove_reference(base_value, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                if (the_lock_chain != NULL)
                  {
                    lock_chain_release(the_lock_chain, the_source_location,
                                       the_jumper);
                  }
                return NULL;
              }

            if (the_lock_chain != NULL)
              {
                if (delayed_unlocks == NULL)
                  {
                    lock_chain_release(the_lock_chain, the_source_location,
                                       the_jumper);
                  }
                else
                  {
                    lock_chain *follow;

                    follow = the_lock_chain;

                    while (follow != NULL)
                      {
                        lock_instance *this_instance;

                        this_instance = lock_chain_head(follow);
                        assert(this_instance != NULL);
                        if (jumper_flowing_forward(the_jumper))
                          {
                            verdict the_verdict;
                            the_verdict = lock_instance_aa_append(
                                    delayed_unlocks, this_instance);
                            if (the_verdict != MISSION_ACCOMPLISHED)
                              {
                                jumper_do_abort(the_jumper);
                                lock_instance_release(this_instance,
                                        the_source_location, the_jumper);
                              }
                          }
                        else
                          {
                            lock_instance_release(this_instance,
                                    the_source_location, the_jumper);
                          }
                        follow = lock_chain_remainder(follow);
                      }
                  }
                if (!(jumper_flowing_forward(the_jumper)))
                    return NULL;
              }

            value_add_reference(result);
            return result;
          }
        case SLK_CALL:
          {
            value *arguments[3];
            const char *argument_names[3];
            size_t argument_count;
            value *result;
            verdict the_verdict;

            arguments[1] = call_slot_location_argument0(the_slot_location);
            arguments[2] = call_slot_location_argument1(the_slot_location);
            argument_names[1] =
                    call_slot_location_argument_name0(the_slot_location);
            argument_names[2] =
                    call_slot_location_argument_name1(the_slot_location);

            if (arguments[1] == NULL)
              {
                assert(argument_names[1] == NULL);
                assert(arguments[2] == NULL);
                assert(argument_names[2] == NULL);
                argument_count = 0;
              }
            else if (arguments[2] == NULL)
              {
                assert(argument_names[2] == NULL);
                argument_count = 1;
              }
            else
              {
                argument_count = 2;
              }

            if (slot_location_overload_base(the_slot_location) != NULL)
              {
                arguments[0] = call_slot_location_argument_for_overload_base(
                        the_slot_location);
                argument_names[0] = NULL;
                the_verdict = try_overloading_from_call_base(
                        slot_location_overload_base(the_slot_location),
                        &result, argument_count + 1, &(arguments[0]),
                        &(argument_names[0]), the_jumper, the_source_location);
                if (the_verdict == MISSION_ACCOMPLISHED)
                    return result;
              }

            the_verdict = try_overloading_from_call_base(
                    call_slot_location_base(the_slot_location), &result,
                    argument_count, &(arguments[1]), &(argument_names[1]),
                    the_jumper, the_source_location);
            if (the_verdict == MISSION_ACCOMPLISHED)
                return result;

            assert(result == NULL);

            location_exception(the_jumper, the_source_location,
                    EXCEPTION_TAG(non_pointer_no_overloaded_match),
                    "No overloaded operator matched a read through an "
                    "overloaded %s on a non-pointer value.",
                    call_slot_location_operation_name(the_slot_location));
            return NULL;
          }
        case SLK_PASS:
          {
            slot_location *base_slot;
            value *overload_base;

            base_slot = pass_slot_location_base(the_slot_location);
            assert(base_slot != NULL);

            overload_base = slot_location_overload_base(the_slot_location);
            if (overload_base != NULL)
              {
                value *base_value;
                value *result;
                verdict the_verdict;

                base_value = create_slot_location_value(base_slot);
                if (base_value == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    return NULL;
                  }

                the_verdict = try_scoped_dereference_overloading(overload_base,
                        &result, base_value, NULL, the_jumper,
                        the_source_location);
                value_remove_reference(base_value, the_jumper);
                if (the_verdict == MISSION_ACCOMPLISHED)
                    return result;
                assert(result == NULL);
                if (!(jumper_flowing_forward(the_jumper)))
                    return NULL;
              }

            return get_slot_contents(base_slot, delayed_unlocks,
                                     the_source_location, the_jumper);
          }
        default:
          {
            assert(FALSE);
            return NULL;
          }
      }
  }

extern void set_slot_contents(slot_location *the_slot_location,
        value *new_value, const source_location *the_source_location,
        jumper *the_jumper)
  {
    get_and_force_slot_contents(the_slot_location, the_source_location,
            &replace_update_function, NULL, FALSE, new_value, the_jumper);
  }

extern lookup_actual_arguments *evaluate_lookup_arguments(
        expression *lookup_expression, context *the_context,
        jumper *the_jumper)
  {
    lookup_actual_arguments *actuals;
    size_t count;
    size_t number;

    assert(lookup_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    count = lookup_expression_component_count(lookup_expression);

    actuals = create_lookup_actual_arguments(count);
    if (actuals == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    for (number = 0; number < count; ++number)
      {
        expression *child_expression;
        expression *upper_bound_expression;
        type_expression *filter_expression;
        value *the_value;
        i_integer_kind the_kind;

        child_expression = lookup_expression_component_child_expression(
                lookup_expression, number);
        upper_bound_expression = lookup_expression_component_upper_bound(
                lookup_expression, number);
        filter_expression =
                lookup_expression_component_filter(lookup_expression, number);

        if (filter_expression != NULL)
          {
            type *the_type;

            assert(child_expression == NULL);
            assert(upper_bound_expression == NULL);

            the_type = evaluate_type_expression(filter_expression, the_context,
                                                the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(the_type == NULL);
                delete_lookup_actual_arguments(actuals, the_jumper);
                return NULL;
              }

            assert(the_type != NULL);

            lookup_actual_arguments_set_filter(actuals, number, the_type,
                                               the_jumper);
            type_remove_reference(the_type, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                delete_lookup_actual_arguments(actuals, the_jumper);
                return NULL;
              }

            continue;
          }

        if (child_expression == NULL)
          {
            assert(upper_bound_expression == NULL);
            continue;
          }

        the_value =
                evaluate_expression(child_expression, the_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(the_value == NULL);
            delete_lookup_actual_arguments(actuals, the_jumper);
            return NULL;
          }

        assert(the_value != NULL);

        if ((upper_bound_expression == NULL) &&
            (get_value_kind(the_value) != VK_INTEGER) && (count != 1))
          {
            expression_exception(the_jumper, lookup_expression,
                    EXCEPTION_TAG(multi_lookup_bad_key),
                    "A multi-dimensional lookup was evaluated with a "
                    "non-integer key.");
            value_remove_reference(the_value, the_jumper);
            delete_lookup_actual_arguments(actuals, the_jumper);
            return NULL;
          }

        if (upper_bound_expression != NULL)
          {
            i_integer_kind the_kind;

            if (get_value_kind(the_value) != VK_INTEGER)
              {
                expression_exception(the_jumper, lookup_expression,
                        EXCEPTION_TAG(lookup_bad_lower),
                        "A lookup was evaluated with a non-integer lower "
                        "bound.");
                value_remove_reference(the_value, the_jumper);
                delete_lookup_actual_arguments(actuals, the_jumper);
                return NULL;
              }

            the_kind = oi_kind(integer_value_data(the_value));
            if ((the_kind == IIK_UNSIGNED_INFINITY) ||
                (the_kind == IIK_ZERO_ZERO))
              {
                expression_exception(the_jumper, lookup_expression,
                        EXCEPTION_TAG(lookup_bad_lower),
                        "A lookup was evaluated with %s as a lower bound.",
                        ((the_kind == IIK_UNSIGNED_INFINITY) ?
                         "unsigned infinity" : "zero-zero"));
                value_remove_reference(the_value, the_jumper);
                delete_lookup_actual_arguments(actuals, the_jumper);
                return NULL;
              }
          }

        lookup_actual_arguments_set_key(actuals, number, the_value,
                                        the_jumper);
        value_remove_reference(the_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            delete_lookup_actual_arguments(actuals, the_jumper);
            return NULL;
          }

        if (upper_bound_expression == NULL)
            continue;

        the_value = evaluate_expression(upper_bound_expression, the_context,
                                        the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(the_value == NULL);
            delete_lookup_actual_arguments(actuals, the_jumper);
            return NULL;
          }

        assert(the_value != NULL);

        if (get_value_kind(the_value) != VK_INTEGER)
          {
            expression_exception(the_jumper, lookup_expression,
                    EXCEPTION_TAG(lookup_bad_upper),
                    "A lookup was evaluated with a non-integer upper bound.");
            value_remove_reference(the_value, the_jumper);
            delete_lookup_actual_arguments(actuals, the_jumper);
            return NULL;
          }

        the_kind = oi_kind(integer_value_data(the_value));
        if ((the_kind == IIK_UNSIGNED_INFINITY) || (the_kind == IIK_ZERO_ZERO))
          {
            expression_exception(the_jumper, lookup_expression,
                    EXCEPTION_TAG(lookup_bad_upper),
                    "A lookup was evaluated with %s as an upper bound.",
                    ((the_kind == IIK_UNSIGNED_INFINITY) ?
                     "unsigned infinity" : "zero-zero"));
            value_remove_reference(the_value, the_jumper);
            delete_lookup_actual_arguments(actuals, the_jumper);
            return NULL;
          }

        lookup_actual_arguments_set_upper_bound(actuals, number, the_value,
                                                the_jumper);
        value_remove_reference(the_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            delete_lookup_actual_arguments(actuals, the_jumper);
            return NULL;
          }
      }

    return actuals;
  }

extern basket_instance *evaluate_basket(basket *the_basket,
        context *the_context, jumper *the_jumper)
  {
    assert(the_basket != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    switch (get_basket_kind(the_basket))
      {
        case BK_EXPRESSION:
          {
            expression *the_expression;
            slot_location *the_slot;
            basket_instance *result;

            the_expression = expression_basket_expression(the_basket);
            assert(the_expression != NULL);

            the_slot = evaluate_address_of_expression(the_expression,
                                                      the_context, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(the_slot == NULL);
                return NULL;
              }

            assert(the_slot != NULL);

            result = create_slot_basket_instance(the_slot);
            if (result == NULL)
                jumper_do_abort(the_jumper);
            slot_location_remove_reference(the_slot, the_jumper);
            if ((!(jumper_flowing_forward(the_jumper))) &&
                (result != NULL))
              {
                delete_basket_instance(result, the_jumper);
                return NULL;
              }

            return result;
          }
        case BK_LIST:
          {
            size_t count;
            size_t number;
            basket_instance *parent_instance;

            count = list_basket_element_count(the_basket);
            parent_instance = create_list_basket_instance(count);
            if (parent_instance == NULL)
              {
                jumper_do_abort(the_jumper);
                return NULL;
              }

            for (number = 0; number < count; ++number)
              {
                verdict the_verdict;
                basket *child_basket;
                basket_instance *child_instance;

                the_verdict = set_list_basket_instance_label(parent_instance,
                        number, list_basket_label(the_basket, number));
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    delete_basket_instance(parent_instance, the_jumper);
                    return NULL;
                  }

                child_basket = list_basket_sub_basket(the_basket, number);
                if (child_basket == NULL)
                    continue;

                child_instance = evaluate_basket(child_basket,
                        the_context, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    assert(child_instance == NULL);
                    delete_basket_instance(parent_instance, the_jumper);
                    return NULL;
                  }

                assert(child_instance != NULL);

                the_verdict = set_list_basket_instance_child(parent_instance,
                        number, child_instance,
                        list_basket_force(the_basket, number));
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    delete_basket_instance(parent_instance, the_jumper);
                    return NULL;
                  }
              }

            return parent_instance;
          }
        default:
          {
            assert(FALSE);
            return NULL;
          }
      }
  }

extern value *execute_call(call *the_call, boolean expect_return_value,
        routine_declaration_chain *overload_chain,
        statement *overload_use_statement, size_t overload_used_for_number,
        context *the_context, jumper *the_jumper)
  {
    return execute_call_with_virtuals(the_call, expect_return_value,
            overload_chain, overload_use_statement, overload_used_for_number,
            the_context, NULL, the_jumper);
  }

extern value *execute_call_with_virtuals(call *the_call,
        boolean expect_return_value, routine_declaration_chain *overload_chain,
        statement *overload_use_statement, size_t overload_used_for_number,
        context *the_context, virtual_lookup *virtual_parent,
        jumper *the_jumper)
  {
    value *base_value;
    semi_labeled_value_list *pre_order_actuals;
    size_t pre_order_count;
    size_t pre_order_num;
    value *result;

    assert(the_call != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    base_value =
            evaluate_expression(call_base(the_call), the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(base_value == NULL);
        return NULL;
      }

    assert(base_value != NULL);

    pre_order_actuals = create_semi_labeled_value_list();
    if (pre_order_actuals == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(base_value, the_jumper);
        return NULL;
      }

    pre_order_count = call_actual_argument_count(the_call);

    for (pre_order_num = 0; pre_order_num < pre_order_count; ++pre_order_num)
      {
        expression *the_expression;
        value *the_value;
        verdict the_verdict;

        the_expression =
                call_actual_argument_expression(the_call, pre_order_num);
        assert(the_expression != NULL);

        the_value =
                evaluate_expression(the_expression, the_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(the_value == NULL);
            delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        assert(the_value != NULL);

        the_verdict = append_value_to_semi_labeled_value_list(
                pre_order_actuals,
                call_formal_argument_name(the_call, pre_order_num), the_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(the_value, the_jumper);
            delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        value_remove_reference(the_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }
      }

    if (((get_value_kind(base_value) != VK_ROUTINE) &&
         (get_value_kind(base_value) != VK_ROUTINE_CHAIN)) ||
        (overload_chain != NULL) || (overload_use_statement != NULL))
      {
        value *actuals_value;
        value *arguments[2];
        verdict the_verdict;

        actuals_value = create_semi_labeled_value_list_value();
        if (actuals_value == NULL)
          {
            jumper_do_abort(the_jumper);
            delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return NULL;
          }

        for (pre_order_num = 0; pre_order_num < pre_order_count;
             ++pre_order_num)
          {
            verdict the_verdict;

            the_verdict = add_field(actuals_value,
                    call_formal_argument_name(the_call, pre_order_num),
                    semi_labeled_value_list_value(pre_order_actuals,
                                                  pre_order_num));
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                value_remove_reference(actuals_value, the_jumper);
                delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
                value_remove_reference(base_value, the_jumper);
                return NULL;
              }
          }

        arguments[0] = base_value;
        arguments[1] = actuals_value;
        the_verdict = try_scoped_overloading(overload_chain,
                overload_use_statement, overload_used_for_number,
                (expect_return_value ? &result : NULL), 2, &(arguments[0]),
                the_context, the_jumper, get_call_location(the_call));
        if (the_verdict == MISSION_ACCOMPLISHED)
          {
            value_remove_reference(actuals_value, the_jumper);
            delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return (expect_return_value ? result : NULL);
          }

        arguments[0] = actuals_value;
        the_verdict = try_overloading(base_value, "operator()",
                (expect_return_value ? &result : NULL), 1, &(arguments[0]),
                NULL, the_jumper, get_call_location(the_call));
        value_remove_reference(actuals_value, the_jumper);
        if (the_verdict == MISSION_ACCOMPLISHED)
          {
            delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
            value_remove_reference(base_value, the_jumper);
            return (expect_return_value ? result : NULL);
          }
      }

    result = execute_call_from_values_with_virtuals(base_value,
            pre_order_actuals, expect_return_value, virtual_parent, the_jumper,
            get_call_location(the_call));
    delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
    value_remove_reference(base_value, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        value_remove_reference(result, the_jumper);
        return NULL;
      }
    return result;
  }

extern value *execute_call_from_values(value *base_value,
        semi_labeled_value_list *pre_order_actuals,
        boolean expect_return_value, jumper *the_jumper,
        const source_location *location)
  {
    return execute_call_from_values_with_virtuals(base_value,
            pre_order_actuals, expect_return_value, NULL, the_jumper,
            location);
  }

extern value *execute_call_from_values_with_virtuals(value *base_value,
        semi_labeled_value_list *pre_order_actuals,
        boolean expect_return_value, virtual_lookup *virtual_parent,
        jumper *the_jumper, const source_location *location)
  {
    routine_instance *the_routine_instance;

    assert(base_value != NULL);
    assert(pre_order_actuals != NULL);
    assert(the_jumper != NULL);

    switch (get_value_kind(base_value))
      {
        case VK_ROUTINE:
          {
            the_routine_instance = routine_value_data(base_value);
            assert(the_routine_instance != NULL);

            if (!(routine_instance_fits_actuals(the_routine_instance,
                          pre_order_actuals, location, the_jumper)))
              {
                if (!(jumper_flowing_forward(the_jumper)))
                    return NULL;
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(overloading_resolution_no_match),
                        "The routine for a call didn't match the actual "
                        "arguments.");
                return NULL;
              }

            assert(routine_instance_is_instantiated(the_routine_instance));
                    /* VERIFIED */
            assert(!(routine_instance_scope_exited(the_routine_instance)));
                    /* VERIFIED */
            break;
          }
        case VK_ROUTINE_CHAIN:
          {
            routine_instance_chain *instance_chain;

            instance_chain = routine_chain_value_data(base_value);
            assert(instance_chain != NULL);

            the_routine_instance = resolve_overloading(instance_chain,
                    pre_order_actuals, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return NULL;

            if (the_routine_instance == NULL)
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(overloading_resolution_no_match),
                        "No routine matched an overloaded call to %U.",
                        base_value);
                return NULL;
              }

            assert(routine_instance_is_instantiated(the_routine_instance));
                    /* VERIFIED */
            assert(!(routine_instance_scope_exited(the_routine_instance)));
                    /* VERIFIED */
            break;
          }
        default:
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(call_no_routine),
                    "The routine expression for a call did not evaluate to a "
                    "routine.");
            return NULL;
          }
      }

    assert(routine_instance_is_instantiated(the_routine_instance));
            /* VERIFIED */
    assert(!(routine_instance_scope_exited(the_routine_instance)));
            /* VERIFIED */
    return execute_call_from_values_with_virtuals2(the_routine_instance,
            pre_order_actuals, expect_return_value, virtual_parent, the_jumper,
            location);
  }

static value *execute_call_from_values_with_virtuals2(
        routine_instance *the_routine_instance,
        semi_labeled_value_list *pre_order_actuals,
        boolean expect_return_value, virtual_lookup *virtual_parent,
        jumper *the_jumper, const source_location *location)
  {
    routine_declaration *declaration;
    lock_chain *the_lock_chain;
    value *result;

    assert(the_routine_instance != NULL);
    assert(pre_order_actuals != NULL);
    assert(the_jumper != NULL);

    assert(routine_instance_is_instantiated(the_routine_instance));
            /* VERIFIED */
    assert(!(routine_instance_scope_exited(the_routine_instance)));
            /* VERIFIED */

    declaration = routine_instance_declaration(the_routine_instance);
    assert(declaration != NULL);

    if (routine_declaration_is_class(declaration) && (!expect_return_value))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(call_class_procedure),
                "A class was called as a procedure.");
        return NULL;
      }

    assert(routine_instance_is_instantiated(the_routine_instance));
            /* VERIFIED */
    assert(!(routine_instance_scope_exited(the_routine_instance)));
            /* VERIFIED */
    the_lock_chain = routine_instance_lock_chain(the_routine_instance);

    if (the_lock_chain != NULL)
      {
        lock_chain_grab(the_lock_chain, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return NULL;
      }

    assert(routine_instance_is_instantiated(the_routine_instance));
            /* VERIFIED */
    assert(!(routine_instance_scope_exited(the_routine_instance)));
            /* VERIFIED */
    result = execute_call_from_values_with_virtuals3(declaration,
            the_routine_instance, pre_order_actuals, expect_return_value,
            virtual_parent, the_jumper, location);

    if (the_lock_chain != NULL)
      {
        lock_chain_release(the_lock_chain, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            if (result != NULL)
                value_remove_reference(result, the_jumper);
            return NULL;
          }
      }

    return result;
  }

static value *execute_call_from_values_with_virtuals3(
        routine_declaration *declaration,
        routine_instance *the_routine_instance,
        semi_labeled_value_list *pre_order_actuals,
        boolean expect_return_value, virtual_lookup *virtual_parent,
        jumper *the_jumper, const source_location *location)
  {
    jumper *callee_jumper;
    value *result;

    assert(declaration != NULL);
    assert(the_routine_instance != NULL);
    assert(pre_order_actuals != NULL);
    assert(the_jumper != NULL);

    assert(routine_instance_is_instantiated(the_routine_instance));
            /* VERIFIED */
    assert(!(routine_instance_scope_exited(the_routine_instance)));
            /* VERIFIED */

    if (routine_declaration_is_pure(declaration))
      {
        callee_jumper = create_purer_jumper(the_jumper);
      }
    else
      {
        callee_jumper = the_jumper;
      }

    increment_routine_instance_active_count(the_routine_instance);

    assert(routine_instance_is_instantiated(the_routine_instance));
            /* VERIFIED */
    assert(!(routine_instance_scope_exited(the_routine_instance)));
            /* VERIFIED */
    result = execute_call_from_values_with_virtuals4(declaration,
            the_routine_instance, pre_order_actuals, expect_return_value,
            virtual_parent, callee_jumper, location);

    decrement_routine_instance_active_count(the_routine_instance);

    if (callee_jumper != the_jumper)
      {
        jumper_transfer_to_parent(callee_jumper);
        delete_jumper(callee_jumper);
      }

    return result;
  }

static value *execute_call_from_values_with_virtuals4(
        routine_declaration *declaration,
        routine_instance *the_routine_instance,
        semi_labeled_value_list *pre_order_actuals,
        boolean expect_return_value, virtual_lookup *virtual_parent,
        jumper *the_jumper, const source_location *location)
  {
    context *declaration_context;
    salmon_thread *the_salmon_thread;
    context *new_context;
    object *the_object;
    formal_arguments *formals;
    size_t formal_count;
    size_t pre_order_count;
    boolean error;
    size_t post_order_count;
    size_t duplicate_formal_argument_num;
    size_t bad_name_actual_argument_num;
    size_t *post_order_array;
    value *all_arguments_value;
    size_t formal_num;
    type_expression *dynamic_return_type_expression;
    statement_block *body;
    CLOCK_T start_time;
    value *result_value;

    assert(declaration != NULL);
    assert(the_routine_instance != NULL);
    assert(pre_order_actuals != NULL);
    assert(the_jumper != NULL);

    assert(routine_instance_is_instantiated(the_routine_instance));
            /* VERIFIED */
    assert(!(routine_instance_scope_exited(the_routine_instance)));
            /* VERIFIED */
    declaration_context = routine_instance_context(the_routine_instance);
    assert(declaration_context != NULL);

    the_salmon_thread = jumper_thread(the_jumper);
    if (salmon_thread_have_stack_limit(the_salmon_thread) &&
        (salmon_thread_stack_usage(the_salmon_thread) >
         (size_t)(0.8 * salmon_thread_stack_limit(the_salmon_thread))))
      {
        location_exception(the_jumper, location, EXCEPTION_TAG(stack_overflow),
                "The size of the call stack exceeded the system limits.");
        return NULL;
      }

    new_context = create_routine_context(declaration_context,
            the_routine_instance, expect_return_value,
            jumper_purity_level(the_jumper));
    if (new_context == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    the_object = NULL;

    if (routine_declaration_is_class(declaration))
      {
        value *return_value;

        assert(routine_instance_is_instantiated(the_routine_instance));
                /* VERIFIED */
        assert(!(routine_instance_scope_exited(the_routine_instance)));
                /* VERIFIED */
        the_object = create_object(the_routine_instance, new_context,
                routine_instance_lock_chain(the_routine_instance),
                ((virtual_parent == NULL) ? NULL :
                 virtual_lookup_reference_cluster(virtual_parent)));
        if (the_object == NULL)
          {
            jumper_do_abort(the_jumper);
            exit_context(new_context, the_jumper);
            return NULL;
          }

        return_value = create_object_value(the_object);
        if (return_value == NULL)
          {
            jumper_do_abort(the_jumper);
            object_remove_reference(the_object, the_jumper);
            return NULL;
          }

        object_remove_reference(the_object, NULL);

        set_routine_return_value(new_context, declaration, return_value,
                                 location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(return_value, the_jumper);
            return NULL;
          }
      }

    formals = routine_declaration_formals(declaration);
    assert(formals != NULL);

    formal_count = formal_arguments_argument_count(formals);

    pre_order_count = semi_labeled_value_list_value_count(pre_order_actuals);

    post_order_array = resolve_parameter_ordering_from_semi_ordered_value_list(
            formals, pre_order_actuals,
            routine_declaration_extra_arguments_allowed(declaration), &error,
            &post_order_count, &duplicate_formal_argument_num,
            &bad_name_actual_argument_num);
    if (error)
      {
        assert(post_order_array == NULL);
        jumper_do_abort(the_jumper);
        if (the_object != NULL)
          {
            value_remove_reference(routine_context_return_value(new_context),
                                   the_jumper);
          }
        else
          {
            exit_context(new_context, the_jumper);
          }
        return NULL;
      }

    assert((post_order_array != NULL) || (pre_order_count == 0));

    all_arguments_value = create_semi_labeled_value_list_value();
    if (all_arguments_value == NULL)
      {
        jumper_do_abort(the_jumper);
        if (post_order_array != NULL)
            free(post_order_array);
        if (the_object != NULL)
          {
            value_remove_reference(routine_context_return_value(new_context),
                                   the_jumper);
          }
        else
          {
            exit_context(new_context, the_jumper);
          }
        return NULL;
      }

    for (formal_num = 0; formal_num < formal_count; ++formal_num)
      {
        variable_declaration *formal_declaration;
        variable_instance *formal_instance;
        value *formal_value;
        boolean formal_is_default;
        boolean force_formal_type;
        type *formal_type;
        verdict the_verdict;

        formal_declaration =
                formal_arguments_formal_by_number(formals, formal_num);
        assert(formal_declaration != NULL);

        formal_instance =
                find_variable_instance(new_context, formal_declaration);
        assert(formal_instance != NULL);

        assert(!(variable_instance_scope_exited(formal_instance)));
                /* VERIFIED */
        assert(routine_instance_is_instantiated(the_routine_instance));
                /* VERIFIED */
        assert(!(routine_instance_scope_exited(the_routine_instance)));
                /* VERIFIED */
        formal_type = routine_instance_argument_type(the_routine_instance,
                                                     formal_num);
        set_variable_instance_type(formal_instance, formal_type, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(all_arguments_value, the_jumper);
            if (post_order_array != NULL)
                free(post_order_array);
            if (the_object != NULL)
              {
                value_remove_reference(
                        routine_context_return_value(new_context), the_jumper);
              }
            else
              {
                exit_context(new_context, the_jumper);
              }
            return NULL;
          }

        if (formal_num < post_order_count)
          {
            size_t pre_order_num;

            assert(post_order_array != NULL);
            pre_order_num = post_order_array[formal_num];
            assert(pre_order_num <= pre_order_count);
            if (pre_order_num < pre_order_count)
              {
                formal_value = semi_labeled_value_list_value(pre_order_actuals,
                                                             pre_order_num);
                assert(formal_value != NULL);
              }
            else
              {
                formal_value = NULL;
              }
          }
        else
          {
            formal_value = NULL;
          }

        if (formal_value == NULL)
          {
            expression *default_value_expression;

            default_value_expression =
                    variable_declaration_initializer(formal_declaration);
            assert(default_value_expression != NULL);

            formal_value = evaluate_expression(default_value_expression,
                                               new_context, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(formal_value == NULL);
                value_remove_reference(all_arguments_value, the_jumper);
                if (post_order_array != NULL)
                    free(post_order_array);
                if (the_object != NULL)
                  {
                    value_remove_reference(
                            routine_context_return_value(new_context),
                            the_jumper);
                  }
                else
                  {
                    exit_context(new_context, the_jumper);
                  }
                return NULL;
              }

            assert(formal_value != NULL);
            assert(!(routine_instance_scope_exited(the_routine_instance)));
                    /* VERIFIED */

            formal_is_default = TRUE;
            force_formal_type =
                    variable_declaration_force_type_in_initialization(
                            formal_declaration);
          }
        else
          {
            formal_is_default = FALSE;
            force_formal_type = FALSE;
          }

        assert(formal_value != NULL);

        assert(!(variable_instance_scope_exited(formal_instance)));
                /* VERIFIED */

        if (formal_type != NULL)
          {
            boolean is_in;
            boolean doubt;
            char *why_not;

            check_type_validity(formal_type, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                if (formal_is_default)
                    value_remove_reference(formal_value, the_jumper);
                value_remove_reference(all_arguments_value, the_jumper);
                if (post_order_array != NULL)
                    free(post_order_array);
                if (the_object != NULL)
                  {
                    value_remove_reference(
                            routine_context_return_value(new_context),
                            the_jumper);
                  }
                else
                  {
                    exit_context(new_context, the_jumper);
                  }
                return NULL;
              }

            assert(type_is_valid(formal_type)); /* VERIFIED */
            is_in = value_is_in_type(formal_value, formal_type, &doubt,
                                     &why_not, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                if (formal_is_default)
                    value_remove_reference(formal_value, the_jumper);
                value_remove_reference(all_arguments_value, the_jumper);
                if (post_order_array != NULL)
                    free(post_order_array);
                if (the_object != NULL)
                  {
                    value_remove_reference(
                            routine_context_return_value(new_context),
                            the_jumper);
                  }
                else
                  {
                    exit_context(new_context, the_jumper);
                  }
                return NULL;
              }

            if (doubt)
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(call_argument_type_match_indeterminate),
                        "While preparing a call, %s was unable to determine "
                        "whether an actual parameter value was in the formal "
                        "parameter's type because %s.", interpreter_name(),
                        why_not);
                free(why_not);
                if (formal_is_default)
                    value_remove_reference(formal_value, the_jumper);
                value_remove_reference(all_arguments_value, the_jumper);
                if (post_order_array != NULL)
                    free(post_order_array);
                if (the_object != NULL)
                  {
                    value_remove_reference(
                            routine_context_return_value(new_context),
                            the_jumper);
                  }
                else
                  {
                    exit_context(new_context, the_jumper);
                  }
                return NULL;
              }

            if (!is_in)
              {
                value *fixed_value;

                if (!force_formal_type)
                  {
                  do_mismatch:
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(call_argument_type_mismatch),
                            "%V was assigned a value that didn't match its "
                            "type because %s.", formal_declaration, why_not);
                    free(why_not);
                    if (formal_is_default)
                        value_remove_reference(formal_value, the_jumper);
                    value_remove_reference(all_arguments_value, the_jumper);
                    if (post_order_array != NULL)
                        free(post_order_array);
                    if (the_object != NULL)
                      {
                        value_remove_reference(
                                routine_context_return_value(new_context),
                                the_jumper);
                      }
                    else
                      {
                        exit_context(new_context, the_jumper);
                      }
                    return NULL;
                  }

                assert(formal_is_default);

                check_value_validity(formal_value, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    free(why_not);
                    value_remove_reference(formal_value, the_jumper);
                    value_remove_reference(all_arguments_value, the_jumper);
                    if (post_order_array != NULL)
                        free(post_order_array);
                    if (the_object != NULL)
                      {
                        value_remove_reference(
                                routine_context_return_value(new_context),
                                the_jumper);
                      }
                    else
                      {
                        exit_context(new_context, the_jumper);
                      }
                    return NULL;
                  }

                assert(formal_is_default);
                assert(value_is_valid(formal_value)); /* VERIFIED */
                assert(type_is_valid(formal_type)); /* VERIFIED */
                fixed_value = force_value_to_type(formal_value, formal_type,
                                                  location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    free(why_not);
                  do_error:
                    value_remove_reference(formal_value, the_jumper);
                    value_remove_reference(all_arguments_value, the_jumper);
                    if (post_order_array != NULL)
                        free(post_order_array);
                    if (the_object != NULL)
                      {
                        value_remove_reference(
                                routine_context_return_value(new_context),
                                the_jumper);
                      }
                    else
                      {
                        exit_context(new_context, the_jumper);
                      }
                    return NULL;
                  }
                if (fixed_value == NULL)
                    goto do_mismatch;
                free(why_not);
                value_remove_reference(formal_value, the_jumper);
                formal_value = fixed_value;
                if (!(jumper_flowing_forward(the_jumper)))
                    goto do_error;
              }
          }

        assert(!(variable_instance_scope_exited(formal_instance)));
                /* VERIFIED */
        set_variable_instance_instantiated(formal_instance);

        assert(variable_instance_is_instantiated(formal_instance));
                /* VERIFIED */
        assert(!(variable_instance_scope_exited(formal_instance)));
                /* VERIFIED */
        set_variable_value_with_locking(formal_instance, formal_value,
                                        location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            if (formal_is_default)
                value_remove_reference(formal_value, the_jumper);
            value_remove_reference(all_arguments_value, the_jumper);
            if (post_order_array != NULL)
                free(post_order_array);
            if (the_object != NULL)
              {
                value_remove_reference(
                        routine_context_return_value(new_context), the_jumper);
              }
            else
              {
                exit_context(new_context, the_jumper);
              }
            return NULL;
          }

        the_verdict = add_field(all_arguments_value,
                variable_declaration_name(formal_declaration), formal_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            if (formal_is_default)
                value_remove_reference(formal_value, the_jumper);
            value_remove_reference(all_arguments_value, the_jumper);
            if (post_order_array != NULL)
                free(post_order_array);
            if (the_object != NULL)
              {
                value_remove_reference(
                        routine_context_return_value(new_context), the_jumper);
              }
            else
              {
                exit_context(new_context, the_jumper);
              }
            return NULL;
          }

        if (formal_is_default)
          {
            value_remove_reference(formal_value, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(all_arguments_value, the_jumper);
                if (post_order_array != NULL)
                    free(post_order_array);
                if (the_object != NULL)
                  {
                    value_remove_reference(
                            routine_context_return_value(new_context),
                            the_jumper);
                  }
                else
                  {
                    exit_context(new_context, the_jumper);
                  }
                return NULL;
              }
          }
      }

    assert((formal_num >= post_order_count) ||
           routine_declaration_extra_arguments_allowed(declaration));

    while (formal_num < post_order_count)
      {
        size_t pre_order_num;
        value *formal_value;
        const char *formal_label;
        verdict the_verdict;

        assert(post_order_array != NULL);
        pre_order_num = post_order_array[formal_num];
        assert(pre_order_num < pre_order_count);
        formal_value = semi_labeled_value_list_value(pre_order_actuals,
                                                     pre_order_num);
        assert(formal_value != NULL);
        formal_label = semi_labeled_value_list_label(pre_order_actuals,
                                                     pre_order_num);

        the_verdict =
                add_field(all_arguments_value, formal_label, formal_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(all_arguments_value, the_jumper);
            if (post_order_array != NULL)
                free(post_order_array);
            if (the_object != NULL)
              {
                value_remove_reference(
                        routine_context_return_value(new_context), the_jumper);
              }
            else
              {
                exit_context(new_context, the_jumper);
              }
            return NULL;
          }

        ++formal_num;
      }

    if (post_order_array != NULL)
        free(post_order_array);

    routine_context_set_all_arguments_value(new_context, all_arguments_value);
    value_remove_reference(all_arguments_value, the_jumper);
    assert(jumper_flowing_forward(the_jumper));

    if (the_object != NULL)
        routine_context_set_this_object_value(new_context, the_object);

    assert(routine_instance_is_instantiated(the_routine_instance));
    assert(!(routine_instance_scope_exited(the_routine_instance)));

    for (formal_num = 0; formal_num < formal_count; ++formal_num)
      {
        type_expression *dynamic_parameter_type_expression;
        type *dynamic_parameter_type;
        boolean is_in;
        boolean doubt;
        char *why_not;

        dynamic_parameter_type_expression =
                formal_arguments_dynamic_type_by_number(formals, formal_num);
        if (dynamic_parameter_type_expression == NULL)
            continue;

        dynamic_parameter_type = evaluate_type_expression(
                dynamic_parameter_type_expression, new_context, the_jumper);
        if (dynamic_parameter_type == NULL)
          {
            assert(!(jumper_flowing_forward(the_jumper)));
            if (the_object == NULL)
              {
                exit_context(new_context, the_jumper);
              }
            else
              {
                value_remove_reference(
                        routine_context_return_value(new_context), the_jumper);
              }
            return NULL;
          }

        assert(jumper_flowing_forward(the_jumper));

        check_type_validity(dynamic_parameter_type, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(dynamic_parameter_type, the_jumper);
            if (the_object == NULL)
              {
                exit_context(new_context, the_jumper);
              }
            else
              {
                value_remove_reference(
                        routine_context_return_value(new_context), the_jumper);
              }
            return NULL;
          }

        assert(type_is_valid(dynamic_parameter_type)); /* VERIFIED */
        is_in = value_is_in_type(
                value_component_value(all_arguments_value, formal_num),
                dynamic_parameter_type, &doubt, &why_not, location,
                the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(dynamic_parameter_type, the_jumper);
            if (the_object == NULL)
              {
                exit_context(new_context, the_jumper);
              }
            else
              {
                value_remove_reference(
                        routine_context_return_value(new_context), the_jumper);
              }
            return NULL;
          }
        if (doubt)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(
                            call_dynamic_parameter_type_match_indeterminate),
                    "%s could not determine whether the value of parameter "
                    "number %lu in a routine call matched the dynamic type of "
                    "that parameter because %s.", interpreter_name(),
                    (unsigned long)(formal_num + 1), why_not);
            free(why_not);
            type_remove_reference(dynamic_parameter_type, the_jumper);
            if (the_object == NULL)
              {
                exit_context(new_context, the_jumper);
              }
            else
              {
                value_remove_reference(
                        routine_context_return_value(new_context), the_jumper);
              }
            return NULL;
          }
        if (!is_in)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(call_dynamic_parameter_type_mismatch),
                    "The value of parameter number %lu in a routine call "
                    "didn't match the dynamic type of that parameter because "
                    "%s.", (unsigned long)(formal_num + 1), why_not);
            free(why_not);
            type_remove_reference(dynamic_parameter_type, the_jumper);
            if (the_object == NULL)
              {
                exit_context(new_context, the_jumper);
              }
            else
              {
                value_remove_reference(
                        routine_context_return_value(new_context), the_jumper);
              }
            return NULL;
          }

        type_remove_reference(dynamic_parameter_type, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            if (the_object == NULL)
              {
                exit_context(new_context, the_jumper);
              }
            else
              {
                value_remove_reference(
                        routine_context_return_value(new_context), the_jumper);
              }
            return NULL;
          }
      }

    dynamic_return_type_expression =
            routine_declaration_dynamic_return_type(declaration);
    if (dynamic_return_type_expression != NULL)
      {
        type *dynamic_return_type;

        dynamic_return_type = evaluate_type_expression(
                dynamic_return_type_expression, new_context, the_jumper);
        if (dynamic_return_type == NULL)
          {
            assert(!(jumper_flowing_forward(the_jumper)));
            if (the_object == NULL)
              {
                exit_context(new_context, the_jumper);
              }
            else
              {
                value_remove_reference(
                        routine_context_return_value(new_context), the_jumper);
              }
            return NULL;
          }

        assert(jumper_flowing_forward(the_jumper));

        routine_context_set_dynamic_return_type(new_context,
                                                dynamic_return_type);
        type_remove_reference(dynamic_return_type, the_jumper);
        assert(jumper_flowing_forward(the_jumper));
      }

    trace(jumper_tracer(the_jumper), TC_CALLS, "Calling %r with arguments %U.",
          declaration, all_arguments_value);

    jumper_push_call_stack(the_jumper, the_routine_instance, location);
    if (!(jumper_flowing_forward(the_jumper)))
        goto skip_body;

    body = routine_declaration_body(declaration);

    if (profiling)
      {
        verdict the_verdict;

        the_verdict = routine_declaration_start_in_call_on_thread(declaration,
                the_salmon_thread);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            goto skip_body;
          }

        GET_CLOCK(start_time);
      }

    if (body != NULL)
      {
        execute_statement_block_with_virtuals(body, new_context, the_jumper,
                                              the_object, virtual_parent);
        result_value = routine_context_return_value(new_context);
      }
    else
      {
        if ((routine_declaration_purity_safety(declaration) != PURE_SAFE) &&
            (purity_level_depth(jumper_purity_level(the_jumper)) > 0))
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(call_side_effect_external_from_pure),
                    "An external routine that might have side effects was "
                    "called from a pure context.");
            result_value = NULL;
            jumper_pop_call_stack(the_jumper);
            goto skip_body;
          }

        result_value = (*(routine_declaration_native_handler(declaration)))(
                all_arguments_value, new_context, the_jumper, location);
      }

    if (profiling)
      {
        CLOCK_T end_time;
        CLOCK_T net_time;
        CLOCK_T non_local_time;
        CLOCK_T local_time;
        verdict the_verdict;

        GET_CLOCK(end_time);

        routine_declaration_end_in_call_on_thread(declaration,
                                                  the_salmon_thread);

        CLOCK_DIFF(net_time, start_time, end_time);

        if (jumper_call_stack_size(the_jumper) > 1)
            jumper_call_stack_back_add_non_local_time(the_jumper, 1, net_time);

        non_local_time = jumper_call_stack_back_non_local_time(the_jumper, 0);
        CLOCK_DIFF(local_time, non_local_time, net_time);

        if (routine_declaration_in_call_on_thread(declaration,
                                                  the_salmon_thread))
          {
            CLOCK_ZERO(net_time);
          }

        the_verdict = routine_declaration_record_call(declaration, net_time,
                                                      local_time);
        if (the_verdict != MISSION_ACCOMPLISHED)
            jumper_do_abort(the_jumper);

        profile_register_routine(declaration);
      }

    jumper_pop_call_stack(the_jumper);

  skip_body:
    open_trace_item(jumper_tracer(the_jumper), TC_CALLS);
    trace_text(jumper_tracer(the_jumper), "Returning ");
    if (result_value == NULL)
        trace_text(jumper_tracer(the_jumper), "no value");
    else
        trace_text(jumper_tracer(the_jumper), "value %U", result_value);
    trace_text(jumper_tracer(the_jumper), " from %r.", declaration);
    close_trace_item(jumper_tracer(the_jumper));

    if (jumper_target(the_jumper) ==
        routine_context_return_target(new_context))
      {
        jumper_reached_target(the_jumper);
      }

    if ((body == NULL) && jumper_flowing_forward(the_jumper) &&
        (result_value != NULL))
      {
        check_routine_return_value(new_context, result_value, location,
                                   the_jumper);
      }

    if (the_object == NULL)
        exit_context(new_context, the_jumper);
    else
        complete_object(the_object);

    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (result_value != NULL)
            value_remove_reference(result_value, the_jumper);
        return NULL;
      }

    if (!expect_return_value)
      {
        if (result_value != NULL)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(call_procedure_return_value),
                    "A value was returned from a procedure call.");
            value_remove_reference(result_value, the_jumper);
            result_value = NULL;
          }
      }
    else
      {
        if (result_value == NULL)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(call_function_no_return_value),
                    "A function call did not return a value.");
          }
      }

    if (the_object != NULL)
      {
        assert(result_value != NULL);
        check_routine_return_value(new_context, result_value, location,
                                   the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result_value, the_jumper);
            return NULL;
          }
      }

    if ((the_object != NULL) && (virtual_parent != NULL))
      {
        context_add_virtual_dependence(virtual_lookup_context(virtual_parent),
                                       the_object);
      }

    return result_value;
  }

extern void execute_statement(statement *the_statement, context *the_context,
                              jumper *the_jumper, object *this_object)
  {
    execute_statement_with_virtuals(the_statement, the_context, the_jumper,
                                    this_object, NULL);
  }

extern void execute_statement_with_virtuals(statement *the_statement,
        context *the_context, jumper *the_jumper, object *this_object,
        virtual_lookup *virtual_parent)
  {
    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    if (have_start_statement_execution_watcher(the_jumper))
      {
        do_thread_start_statement_execution_watchers(jumper_thread(the_jumper),
                the_statement, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return;
      }

    switch (get_statement_kind(the_statement))
      {
        case SK_ASSIGN:
            execute_assign_statement(the_statement, the_context, the_jumper);
            return;
        case SK_INCREMENT:
            execute_increment_statement(the_statement, the_context,
                                        the_jumper);
            return;
        case SK_DECREMENT:
            execute_decrement_statement(the_statement, the_context,
                                        the_jumper);
            return;
        case SK_CALL:
            execute_call_statement(the_statement, the_context, the_jumper);
            return;
        case SK_DECLARATION:
            execute_declaration_statement(the_statement, the_context,
                                          the_jumper, this_object);
            return;
        case SK_IF:
            execute_if_statement(the_statement, the_context, the_jumper);
            return;
        case SK_SWITCH:
            execute_switch_statement(the_statement, the_context, the_jumper);
            return;
        case SK_GOTO:
            execute_goto_statement(the_statement, the_context, the_jumper);
            return;
        case SK_RETURN:
            execute_return_statement(the_statement, the_context, the_jumper);
            return;
        case SK_FOR:
            execute_for_statement(the_statement, the_context, the_jumper);
            return;
        case SK_ITERATE:
            execute_iterate_statement(the_statement, the_context, the_jumper);
            return;
        case SK_WHILE:
            execute_while_statement(the_statement, the_context, the_jumper);
            return;
        case SK_DO_WHILE:
            execute_do_while_statement(the_statement, the_context, the_jumper);
            return;
        case SK_BREAK:
            execute_break_statement(the_statement, the_context, the_jumper);
            return;
        case SK_CONTINUE:
            execute_continue_statement(the_statement, the_context, the_jumper);
            return;
        case SK_LABEL:
            execute_label_statement(the_statement, the_context, the_jumper);
            return;
        case SK_STATEMENT_BLOCK:
            execute_statement_block_statement(the_statement, the_context,
                                              the_jumper);
            return;
        case SK_SINGLE:
            execute_single_statement(the_statement, the_context, the_jumper);
            return;
        case SK_TRY_CATCH:
            execute_try_catch_statement(the_statement, the_context,
                                        the_jumper);
            return;
        case SK_TRY_HANDLE:
            execute_try_handle_statement(the_statement, the_context,
                                         the_jumper);
            return;
        case SK_CLEANUP:
            assert(FALSE);
            return;
        case SK_EXPORT:
            execute_export_statement(the_statement, the_context, the_jumper);
            return;
        case SK_HIDE:
            execute_hide_statement(the_statement, the_context, the_jumper);
            return;
        case SK_USE:
            execute_use_statement(the_statement, the_context, the_jumper,
                                  this_object, virtual_parent);
            return;
        case SK_INCLUDE:
            assert(FALSE);
            return;
        case SK_THEOREM:
            execute_theorem_statement(the_statement, the_context, the_jumper);
            return;
        case SK_ALIAS:
            return;
        default:
            assert(FALSE);
            return;
      }
  }

extern void execute_assign_statement(statement *the_statement,
                                     context *the_context, jumper *the_jumper)
  {
    basket_instance *the_basket_instance;
    expression *right_hand_side;
    boolean force_to_type;
    assignment_kind kind;
    value *new_value;
    lock_instance_aa delayed_unlocks;
    size_t lock_num;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_ASSIGN);
    the_basket_instance = evaluate_basket(
            assign_statement_basket(the_statement), the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_basket_instance == NULL);
        return;
      }

    assert(the_basket_instance != NULL);

    right_hand_side = assign_statement_expression(the_statement);
    assert(right_hand_side != NULL);

    force_to_type = FALSE;

    kind = assign_statement_assignment_kind(the_statement);

    switch (kind)
      {
        case AK_MODULO:
          {
            force_to_type = TRUE;
            /* fall through */
          }
        case AK_SIMPLE:
          {
            new_value = evaluate_expression(right_hand_side, the_context,
                                            the_jumper);
            break;
          }
        case AK_MULTIPLY:
        case AK_DIVIDE:
        case AK_DIVIDE_FORCE:
        case AK_REMAINDER:
        case AK_ADD:
        case AK_SUBTRACT:
        case AK_SHIFT_LEFT:
        case AK_SHIFT_RIGHT:
        case AK_BITWISE_AND:
        case AK_BITWISE_XOR:
        case AK_BITWISE_OR:
        case AK_LOGICAL_AND:
        case AK_LOGICAL_OR:
        case AK_CONCATENATE:
          {
            verdict the_verdict;
            value *old_value;

            the_verdict = lock_instance_aa_init(&delayed_unlocks, 10);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                delete_basket_instance(the_basket_instance, the_jumper);
                return;
              }

            old_value = read_from_basket_instance(the_basket_instance,
                    &delayed_unlocks, get_statement_location(the_statement),
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                size_t lock_num;

                assert(old_value == NULL);
                for (lock_num = 0; lock_num < delayed_unlocks.element_count;
                     ++lock_num)
                  {
                    lock_instance_release(
                            delayed_unlocks.array[
                                    delayed_unlocks.element_count -
                                    (lock_num + 1)],
                            get_statement_location(the_statement), the_jumper);
                  }
                free(delayed_unlocks.array);
                delete_basket_instance(the_basket_instance, the_jumper);
                return;
              }

            assert(old_value != NULL);

            new_value = finish_evaluating_binary_expression(
                    binary_expression_kind_for_assignment(kind), old_value,
                    right_hand_side, statement_overload_chain(the_statement),
                    statement_overload_use_statement(the_statement),
                    statement_overload_use_used_for_number(the_statement),
                    the_context, the_jumper,
                    get_statement_location(the_statement));

            break;
          }
        default:
          {
            assert(FALSE);
            new_value = NULL;
          }
      }

    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(new_value == NULL);
        if ((kind != AK_MODULO) && (kind != AK_SIMPLE))
          {
            size_t lock_num;

            for (lock_num = 0; lock_num < delayed_unlocks.element_count;
                 ++lock_num)
              {
                lock_instance_release(
                        delayed_unlocks.array[
                                delayed_unlocks.element_count -
                                (lock_num + 1)],
                        get_statement_location(the_statement), the_jumper);
              }
            free(delayed_unlocks.array);
          }
        delete_basket_instance(the_basket_instance, the_jumper);
        return;
      }

    assert(new_value != NULL);

    write_to_basket_instance(the_basket_instance, new_value, force_to_type,
            get_statement_location(the_statement), the_jumper);
    delete_basket_instance(the_basket_instance, the_jumper);
    value_remove_reference(new_value, the_jumper);

    if ((kind != AK_MODULO) && (kind != AK_SIMPLE))
      {
        for (lock_num = 0; lock_num < delayed_unlocks.element_count;
             ++lock_num)
          {
            lock_instance_release(
                    delayed_unlocks.array[
                            delayed_unlocks.element_count - (lock_num + 1)],
                    get_statement_location(the_statement), the_jumper);
          }
        free(delayed_unlocks.array);
      }
  }

extern void execute_increment_statement(statement *the_statement,
        context *the_context, jumper *the_jumper)
  {
    basket_instance *the_basket_instance;
    value *one_value;
    expression *right_hand_side;
    lock_instance_aa delayed_unlocks;
    verdict the_verdict;
    value *old_value;
    value *new_value;
    size_t lock_num;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_INCREMENT);
    the_basket_instance = evaluate_basket(
            increment_statement_basket(the_statement), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_basket_instance == NULL);
        return;
      }

    assert(the_basket_instance != NULL);

    one_value = create_integer_value(oi_one);
    if (one_value == NULL)
      {
        jumper_do_abort(the_jumper);
        delete_basket_instance(the_basket_instance, the_jumper);
        return;
      }

    right_hand_side = create_constant_expression(one_value);
    value_remove_reference(one_value, the_jumper);
    assert(jumper_flowing_forward(the_jumper));
    if (right_hand_side == NULL)
      {
        jumper_do_abort(the_jumper);
        delete_basket_instance(the_basket_instance, the_jumper);
        return;
      }

    the_verdict = lock_instance_aa_init(&delayed_unlocks, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        delete_basket_instance(the_basket_instance, the_jumper);
        delete_expression(right_hand_side);
        return;
      }

    old_value = read_from_basket_instance(the_basket_instance,
            &delayed_unlocks, get_statement_location(the_statement),
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        size_t lock_num;

        assert(old_value == NULL);
        for (lock_num = 0; lock_num < delayed_unlocks.element_count;
             ++lock_num)
          {
            lock_instance_release(
                    delayed_unlocks.array[
                            delayed_unlocks.element_count - (lock_num + 1)],
                    get_statement_location(the_statement), the_jumper);
          }
        free(delayed_unlocks.array);
        delete_expression(right_hand_side);
        delete_basket_instance(the_basket_instance, the_jumper);
        return;
      }

    assert(old_value != NULL);

    new_value = finish_evaluating_binary_expression(EK_ADD, old_value,
            right_hand_side, statement_overload_chain(the_statement),
            statement_overload_use_statement(the_statement),
            statement_overload_use_used_for_number(the_statement), the_context,
            the_jumper, get_statement_location(the_statement));
    delete_expression(right_hand_side);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        size_t lock_num;

        assert(new_value == NULL);
        for (lock_num = 0; lock_num < delayed_unlocks.element_count;
             ++lock_num)
          {
            lock_instance_release(
                    delayed_unlocks.array[
                            delayed_unlocks.element_count - (lock_num + 1)],
                    get_statement_location(the_statement), the_jumper);
          }
        free(delayed_unlocks.array);
        delete_basket_instance(the_basket_instance, the_jumper);
        return;
      }

    assert(new_value != NULL);

    write_to_basket_instance(the_basket_instance, new_value, FALSE,
            get_statement_location(the_statement), the_jumper);
    delete_basket_instance(the_basket_instance, the_jumper);
    value_remove_reference(new_value, the_jumper);

    for (lock_num = 0; lock_num < delayed_unlocks.element_count; ++lock_num)
      {
        lock_instance_release(
                delayed_unlocks.array[
                        delayed_unlocks.element_count - (lock_num + 1)],
                get_statement_location(the_statement), the_jumper);
      }
    free(delayed_unlocks.array);
  }

extern void execute_decrement_statement(statement *the_statement,
        context *the_context, jumper *the_jumper)
  {
    basket_instance *the_basket_instance;
    value *one_value;
    expression *right_hand_side;
    lock_instance_aa delayed_unlocks;
    verdict the_verdict;
    value *old_value;
    value *new_value;
    size_t lock_num;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_DECREMENT);
    the_basket_instance = evaluate_basket(
            decrement_statement_basket(the_statement), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_basket_instance == NULL);
        return;
      }

    assert(the_basket_instance != NULL);

    one_value = create_integer_value(oi_one);
    if (one_value == NULL)
      {
        jumper_do_abort(the_jumper);
        delete_basket_instance(the_basket_instance, the_jumper);
        return;
      }

    right_hand_side = create_constant_expression(one_value);
    value_remove_reference(one_value, the_jumper);
    assert(jumper_flowing_forward(the_jumper));
    if (right_hand_side == NULL)
      {
        jumper_do_abort(the_jumper);
        delete_basket_instance(the_basket_instance, the_jumper);
        return;
      }

    the_verdict = lock_instance_aa_init(&delayed_unlocks, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        delete_basket_instance(the_basket_instance, the_jumper);
        delete_expression(right_hand_side);
        return;
      }

    old_value = read_from_basket_instance(the_basket_instance,
            &delayed_unlocks, get_statement_location(the_statement),
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        size_t lock_num;

        assert(old_value == NULL);
        for (lock_num = 0; lock_num < delayed_unlocks.element_count;
             ++lock_num)
          {
            lock_instance_release(
                    delayed_unlocks.array[
                            delayed_unlocks.element_count - (lock_num + 1)],
                    get_statement_location(the_statement), the_jumper);
          }
        free(delayed_unlocks.array);
        delete_basket_instance(the_basket_instance, the_jumper);
        delete_expression(right_hand_side);
        return;
      }

    assert(old_value != NULL);

    new_value = finish_evaluating_binary_expression(EK_SUBTRACT, old_value,
            right_hand_side, statement_overload_chain(the_statement),
            statement_overload_use_statement(the_statement),
            statement_overload_use_used_for_number(the_statement), the_context,
            the_jumper, get_statement_location(the_statement));
    delete_expression(right_hand_side);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        size_t lock_num;

        assert(new_value == NULL);
        for (lock_num = 0; lock_num < delayed_unlocks.element_count;
             ++lock_num)
          {
            lock_instance_release(
                    delayed_unlocks.array[
                            delayed_unlocks.element_count - (lock_num + 1)],
                    get_statement_location(the_statement), the_jumper);
          }
        free(delayed_unlocks.array);
        delete_basket_instance(the_basket_instance, the_jumper);
        return;
      }

    assert(new_value != NULL);

    write_to_basket_instance(the_basket_instance, new_value, FALSE,
            get_statement_location(the_statement), the_jumper);
    delete_basket_instance(the_basket_instance, the_jumper);
    value_remove_reference(new_value, the_jumper);

    for (lock_num = 0; lock_num < delayed_unlocks.element_count; ++lock_num)
      {
        lock_instance_release(
                delayed_unlocks.array[
                        delayed_unlocks.element_count - (lock_num + 1)],
                get_statement_location(the_statement), the_jumper);
      }
    free(delayed_unlocks.array);
  }

extern void execute_call_statement(statement *the_statement,
                                   context *the_context, jumper *the_jumper)
  {
    value *the_value;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_CALL);
    the_value = execute_call(call_statement_call(the_statement), FALSE,
            statement_overload_chain(the_statement),
            statement_overload_use_statement(the_statement),
            statement_overload_use_used_for_number(the_statement), the_context,
            the_jumper);
    assert(the_value == NULL);
  }

extern void execute_declaration_statement(statement *the_statement,
        context *the_context, jumper *the_jumper, object *this_object)
  {
    size_t declaration_count;
    size_t declaration_num;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    declaration_count = declaration_statement_declaration_count(the_statement);
    for (declaration_num = 0; declaration_num < declaration_count;
         ++declaration_num)
      {
        declaration *the_declaration;
        instance *the_instance;

        the_declaration = declaration_statement_declaration(the_statement,
                                                            declaration_num);
        assert(the_declaration != NULL);

        the_instance = find_instance(the_context, the_declaration);
        assert(the_instance != NULL);

        if ((!(declaration_is_static(the_declaration))) &&
            (instance_declaration(the_instance) == the_declaration))
          {
            assert((this_object == NULL) || !(object_is_closed(this_object)));
                    /* VERIFIED */
            execute_declaration(the_declaration, the_instance, the_context,
                                the_jumper, this_object);
            if (!(jumper_flowing_forward(the_jumper)))
                return;
          }

        assert((this_object == NULL) || !(object_is_closed(this_object)));
                /* VERIFIED */
        if ((this_object != NULL) && object_export_enabled(this_object))
          {
            switch (declaration_kind(the_declaration))
              {
                case NK_VARIABLE:
                    assert(!(object_is_closed(this_object))); /* VERIFIED */
                    object_add_variable_field(this_object,
                            instance_variable_instance(the_instance),
                            declaration_name(the_declaration), the_jumper);
                    break;
                case NK_ROUTINE:
                    assert(!(object_is_closed(this_object))); /* VERIFIED */
                    object_add_routine_field(this_object,
                            instance_routine_instance(the_instance),
                            declaration_name(the_declaration), the_jumper);
                    break;
                case NK_TAGALONG:
                    assert(!(object_is_closed(this_object))); /* VERIFIED */
                    object_add_tagalong_field(this_object,
                            instance_tagalong_instance(the_instance),
                            declaration_name(the_declaration), the_jumper);
                    break;
                case NK_LEPTON_KEY:
                    assert(!(object_is_closed(this_object))); /* VERIFIED */
                    object_add_lepton_key_field(this_object,
                            instance_lepton_key_instance(the_instance),
                            declaration_name(the_declaration), the_jumper);
                    break;
                case NK_QUARK:
                    assert(!(object_is_closed(this_object))); /* VERIFIED */
                    object_add_quark_field(this_object,
                            instance_quark_instance(the_instance),
                            declaration_name(the_declaration), the_jumper);
                    break;
                case NK_LOCK:
                    assert(!(object_is_closed(this_object))); /* VERIFIED */
                    object_add_lock_field(this_object,
                            instance_lock_instance(the_instance),
                            declaration_name(the_declaration), the_jumper);
                    break;
                default:
                    assert(FALSE);
              }
          }
      }
  }

extern void execute_if_statement(statement *the_statement,
                                 context *the_context, jumper *the_jumper)
  {
    value *test_value;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_IF);
    test_value = evaluate_expression(if_statement_test(the_statement),
                                     the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(test_value == NULL);
        return;
      }

    assert(test_value != NULL);
    switch (get_value_kind(test_value))
      {
        case VK_TRUE:
          {
            value_remove_reference(test_value, the_jumper);
            assert(jumper_flowing_forward(the_jumper));
            execute_statement_block(if_statement_body(the_statement),
                                    the_context, the_jumper);
            return;
          }
        case VK_FALSE:
          {
            size_t else_if_count;
            size_t else_if_num;
            statement_block *else_block;

            value_remove_reference(test_value, the_jumper);
            assert(jumper_flowing_forward(the_jumper));

            else_if_count = if_statement_else_if_count(the_statement);

            for (else_if_num = 0; else_if_num < else_if_count; ++else_if_num)
              {
                value *test_value;

                test_value = evaluate_expression(
                        if_statement_else_if_test(the_statement, else_if_num),
                        the_context, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    assert(test_value == NULL);
                    return;
                  }

                assert(test_value != NULL);
                switch (get_value_kind(test_value))
                  {
                    case VK_TRUE:
                      {
                        value_remove_reference(test_value, the_jumper);
                        assert(jumper_flowing_forward(the_jumper));
                        execute_statement_block(
                                if_statement_else_if_body(the_statement,
                                                          else_if_num),
                                the_context, the_jumper);
                        return;
                      }
                    case VK_FALSE:
                      {
                        value_remove_reference(test_value, the_jumper);
                        assert(jumper_flowing_forward(the_jumper));
                        break;
                      }
                    default:
                      {
                        expression_exception(the_jumper,
                                if_statement_else_if_test(the_statement,
                                                          else_if_num),
                                EXCEPTION_TAG(if_bad_test),
                                "An `else if' test argument to an if statement"
                                " evaluated to something other than a boolean "
                                "value.");
                        value_remove_reference(test_value, the_jumper);
                        return;
                      }
                  }
              }

            else_block = if_statement_else_body(the_statement);
            if (else_block != NULL)
                execute_statement_block(else_block, the_context, the_jumper);
            return;
          }
        default:
          {
            expression_exception(the_jumper, if_statement_test(the_statement),
                    EXCEPTION_TAG(if_bad_test),
                    "The test argument to an if statement evaluated to "
                    "something other than a boolean value.");
            value_remove_reference(test_value, the_jumper);
            return;
          }
      }
  }

extern void execute_switch_statement(statement *the_statement,
                                     context *the_context, jumper *the_jumper)
  {
    value *base;
    size_t case_count;
    size_t case_num;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_SWITCH);

    base = evaluate_expression(switch_statement_base(the_statement),
                               the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(base == NULL);
        return;
      }

    assert(base != NULL);

    case_count = switch_statement_case_count(the_statement);

    for (case_num = 0; case_num < case_count; ++case_num)
      {
        type_expression *case_expression;
        type *case_type;
        boolean is_in;
        boolean doubt;
        char *why_not;

        case_expression = switch_statement_case_type(the_statement, case_num);
        case_type = evaluate_type_expression(case_expression, the_context,
                                             the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(case_type == NULL);
            value_remove_reference(base, the_jumper);
            return;
          }

        assert(case_type != NULL);

        check_type_validity(case_type,
                get_type_expression_location(case_expression), the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(case_type, the_jumper);
            value_remove_reference(base, the_jumper);
            return;
          }

        assert(type_is_valid(case_type)); /* VERIFIED */
        is_in = value_is_in_type(base, case_type, &doubt, &why_not,
                get_statement_location(the_statement), the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(case_type, the_jumper);
            value_remove_reference(base, the_jumper);
            return;
          }

        if (doubt)
          {
            type_expression_exception(the_jumper,
                    switch_statement_case_type(the_statement, case_num),
                    EXCEPTION_TAG(switch_indeterminate),
                    "While testing a case of a switch statement, %s was unable"
                    " to determine whether the test value was in the case type"
                    " because %s.", interpreter_name(), why_not);
            free(why_not);
            type_remove_reference(case_type, the_jumper);
            value_remove_reference(base, the_jumper);
            return;
          }

        if (is_in)
          {
            type_remove_reference(case_type, the_jumper);
            value_remove_reference(base, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return;

            execute_statement_block(
                    switch_statement_case_block(the_statement, case_num),
                    the_context, the_jumper);
            return;
          }
        else
          {
            free(why_not);
          }

        type_remove_reference(case_type, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(base, the_jumper);
            return;
          }
      }

    value_remove_reference(base, the_jumper);
  }

extern void execute_goto_statement(statement *the_statement,
                                   context *the_context, jumper *the_jumper)
  {
    value *the_value;
    jump_target *the_target;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_GOTO);

    the_value = evaluate_expression(goto_statement_target(the_statement),
                                    the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_value == NULL);
        return;
      }

    assert(the_value != NULL);
    if (get_value_kind(the_value) != VK_JUMP_TARGET)
      {
        statement_exception(the_jumper, the_statement,
                EXCEPTION_TAG(goto_bad_target),
                "The argument to a goto statement evaluated to something other"
                " than a jump target.");
        value_remove_reference(the_value, the_jumper);
        return;
      }

    the_target = jump_target_value_data(the_value);
    assert(the_target != NULL);

    if (jump_target_scope_exited(the_target))
      {
        statement_exception(the_jumper, the_statement,
                EXCEPTION_TAG(goto_target_deallocated),
                "The target of a goto statement had been de-allocated because "
                "its scope had exited.");
        value_remove_reference(the_value, the_jumper);
        return;
      }

    assert(!(jump_target_scope_exited(the_target))); /* VERIFIED */
    jumper_set_target(the_jumper, the_target);
    value_remove_reference(the_value, the_jumper);
    return;
  }

extern void execute_return_statement(statement *the_statement,
                                     context *the_context, jumper *the_jumper)
  {
    expression *return_value_expression;
    value *return_value;
    routine_declaration *from_routine;
    expression *from_block;
    jump_target *target;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_RETURN);

    return_value_expression = return_statement_return_value(the_statement);
    if (return_value_expression != NULL)
      {
        return_value = evaluate_expression(return_value_expression,
                                           the_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(return_value == NULL);
            return;
          }

        assert(return_value != NULL);
      }
    else
      {
        return_value = NULL;
      }

    from_routine = return_statement_from_routine(the_statement);
    from_block = return_statement_from_block_expression(the_statement);
    if (from_block != NULL)
      {
        assert(from_routine == NULL);

        if (return_value == NULL)
          {
            statement_exception(the_jumper, the_statement,
                    EXCEPTION_TAG(statement_block_expression_return_no_value),
                    "A return without a value was executed with reference to a"
                    " statement block expression.");
            return;
          }

        set_block_expression_return_value(the_context, from_block,
                return_value, get_statement_location(the_statement),
                the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return;

        target = find_block_expression_return_target(the_context, from_block);
      }
    else
      {
        set_routine_return_value(the_context, from_routine, return_value,
                get_statement_location(the_statement), the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return;

        target = find_routine_return_target(the_context, from_routine);
      }

    if (jump_target_scope_exited(target))
      {
        statement_exception(the_jumper, the_statement,
                EXCEPTION_TAG(return_target_deallocated),
                "The target of a return statement had been de-allocated "
                "because its scope had exited.");
        return;
      }

    assert(target != NULL);
    assert(!(jump_target_scope_exited(target))); /* VERIFIED */
    jumper_set_target(the_jumper, target);

    return;
  }

extern void execute_for_statement(statement *the_statement,
                                  context *the_context, jumper *the_jumper)
  {
    value *initial_value;
    o_integer index_oi;
    value *step_value;
    o_integer step_oi;
    context *for_context;
    variable_instance *index_instance;
    type *index_variable_type;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_FOR);

    initial_value = evaluate_expression(for_statement_init(the_statement),
                                        the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(initial_value == NULL);
        return;
      }

    assert(initial_value != NULL);
    if (get_value_kind(initial_value) != VK_INTEGER)
      {
        expression_exception(the_jumper, for_statement_init(the_statement),
                EXCEPTION_TAG(for_bad_initial),
                "The initial value argument to a for statement evaluated to "
                "something other than an integer value.");
        value_remove_reference(initial_value, the_jumper);
        return;
      }

    index_oi = integer_value_data(initial_value);
    oi_add_reference(index_oi);
    value_remove_reference(initial_value, the_jumper);
    assert(jumper_flowing_forward(the_jumper));

    step_value = evaluate_expression(for_statement_step(the_statement),
                                     the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(step_value == NULL);
        oi_remove_reference(index_oi);
        return;
      }

    assert(step_value != NULL);
    if (get_value_kind(step_value) != VK_INTEGER)
      {
        expression_exception(the_jumper, for_statement_step(the_statement),
                EXCEPTION_TAG(for_bad_step),
                "The step value argument to a for statement evaluated to "
                "something other than an integer value.");
        value_remove_reference(step_value, the_jumper);
        oi_remove_reference(index_oi);
        return;
      }

    step_oi = integer_value_data(step_value);
    oi_add_reference(step_oi);
    value_remove_reference(step_value, the_jumper);
    assert(jumper_flowing_forward(the_jumper));

    for_context = create_loop_context(the_context, the_statement,
            for_statement_index(the_statement),
            jumper_purity_level(the_jumper),
            get_statement_location(the_statement));
    if (for_context == NULL)
      {
        jumper_do_abort(the_jumper);
        oi_remove_reference(index_oi);
        oi_remove_reference(step_oi);
        return;
      }

    index_instance = loop_context_index(for_context);
    assert(index_instance != NULL);

    assert(!(variable_instance_scope_exited(index_instance))); /* VERIFIED */
    set_variable_instance_instantiated(index_instance);

    index_variable_type = get_integer_type();
    if (index_variable_type == NULL)
      {
        jumper_do_abort(the_jumper);
        oi_remove_reference(index_oi);
        oi_remove_reference(step_oi);
        exit_context(for_context, the_jumper);
        return;
      }

    assert(!(variable_instance_scope_exited(index_instance))); /* VERIFIED */
    set_variable_instance_type(index_instance, index_variable_type,
                               the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        oi_remove_reference(index_oi);
        oi_remove_reference(step_oi);
        exit_context(for_context, the_jumper);
        return;
      }

    while (TRUE)
      {
        value *index_value;
        value *test_value;
        o_integer new_index_oi;

        index_value = create_integer_value(index_oi);
        if (index_value == NULL)
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(index_oi);
            oi_remove_reference(step_oi);
            exit_context(for_context, the_jumper);
            return;
          }

        assert(variable_instance_is_instantiated(index_instance));
                /* VERIFIED */
        assert(!(variable_instance_scope_exited(index_instance)));
                /* VERIFIED */
        assert(variable_instance_lock_chain(index_instance) == NULL);
        set_variable_instance_value(index_instance, index_value, the_jumper);
        value_remove_reference(index_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            oi_remove_reference(index_oi);
            oi_remove_reference(step_oi);
            exit_context(for_context, the_jumper);
            return;
          }

        test_value = evaluate_expression(for_statement_test(the_statement),
                                         for_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(test_value == NULL);
          do_jump:
            if (jumper_target(the_jumper) ==
                loop_context_continue_target(for_context))
              {
                jumper_reached_target(the_jumper);
                goto do_continue;
              }

            oi_remove_reference(index_oi);
            oi_remove_reference(step_oi);

            if (jumper_target(the_jumper) ==
                loop_context_break_target(for_context))
              {
                assert(!(for_statement_is_parallel(the_statement)));
                jumper_reached_target(the_jumper);
              }

            exit_context(for_context, the_jumper);
            return;
          }

        assert(test_value != NULL);
        switch (get_value_kind(test_value))
          {
            case VK_TRUE:
                value_remove_reference(test_value, the_jumper);
                assert(jumper_flowing_forward(the_jumper));
                break;
            case VK_FALSE:
                value_remove_reference(test_value, the_jumper);
                assert(jumper_flowing_forward(the_jumper));
                oi_remove_reference(index_oi);
                oi_remove_reference(step_oi);
                exit_context(for_context, the_jumper);
                return;
            default:
                expression_exception(the_jumper,
                        for_statement_test(the_statement),
                        EXCEPTION_TAG(for_bad_test),
                        "The test argument to a for statement evaluated to "
                        "something other than a boolean value.");
                value_remove_reference(test_value, the_jumper);
                oi_remove_reference(index_oi);
                oi_remove_reference(step_oi);
                exit_context(for_context, the_jumper);
                return;
          }

        execute_statement_block(for_statement_body(the_statement), for_context,
                                the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            goto do_jump;

      do_continue:
        oi_add(new_index_oi, index_oi, step_oi);
        if (oi_out_of_memory(new_index_oi))
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(index_oi);
            oi_remove_reference(step_oi);
            exit_context(for_context, the_jumper);
            return;
          }

        oi_remove_reference(index_oi);
        index_oi = new_index_oi;
      }
  }

extern void execute_iterate_statement(statement *the_statement,
                                      context *the_context, jumper *the_jumper)
  {
    const source_location *base_location;
    value *base_value;
    boolean using_iterator;
    simple_iteration_data simple_data;
    verdict the_verdict;
    value *iterator_value;
    context *iterate_context;
    variable_instance *element_instance;
    type *element_variable_type;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_ITERATE);

    base_location =
            get_expression_location(iterate_statement_base(the_statement));

    base_value = evaluate_expression(iterate_statement_base(the_statement),
                                     the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(base_value == NULL);
        return;
      }

    assert(base_value != NULL);

    check_value_validity_except_map_targets(base_value, base_location,
                                            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(base_value, the_jumper);
        return;
      }

    assert(value_is_valid_except_map_targets(base_value)); /* VERIFIED */

    the_verdict = start_simple_iteration_data(base_value, &simple_data,
            "executing", "an iterate statement", the_jumper, base_location);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(base_value, the_jumper);
        return;
      }
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        verdict the_verdict;

        the_verdict = try_overloading(base_value, "iterator", &iterator_value,
                0, NULL, NULL, the_jumper,
                get_statement_location(the_statement));
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(iterator_value == NULL);
            value_remove_reference(base_value, the_jumper);
            return;
          }
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            assert(iterator_value == NULL);
            location_exception(the_jumper, base_location,
                    EXCEPTION_TAG(iteration_bad_base),
                    "The base argument to an iterate statement evaluated to "
                    "something other than an array or semi-labeled value list "
                    "that didn't have an `iterator' function.");
            value_remove_reference(base_value, the_jumper);
            return;
          }

        assert(iterator_value != NULL);
        using_iterator = TRUE;
      }
    else
      {
        using_iterator = FALSE;
        iterator_value = NULL;
      }

    iterate_context = create_loop_context(the_context, the_statement,
            iterate_statement_element(the_statement),
            jumper_purity_level(the_jumper),
            get_statement_location(the_statement));
    if (iterate_context == NULL)
      {
        jumper_do_abort(the_jumper);
        if (using_iterator)
            value_remove_reference(iterator_value, the_jumper);
        else
            clean_up_simple_iteration_data(&simple_data);
        value_remove_reference(base_value, the_jumper);
        return;
      }

    element_instance = loop_context_index(iterate_context);
    assert(element_instance != NULL);

    assert(!(variable_instance_scope_exited(element_instance))); /* VERIFIED */
    set_variable_instance_instantiated(element_instance);

    element_variable_type = get_anything_type();
    if (element_variable_type == NULL)
      {
        jumper_do_abort(the_jumper);
        exit_context(iterate_context, the_jumper);
        if (using_iterator)
            value_remove_reference(iterator_value, the_jumper);
        else
            clean_up_simple_iteration_data(&simple_data);
        value_remove_reference(base_value, the_jumper);
        return;
      }
    type_add_reference(element_variable_type);

    assert(!(variable_instance_scope_exited(element_instance))); /* VERIFIED */
    set_variable_instance_type(element_instance, element_variable_type,
                               the_jumper);
    type_remove_reference(element_variable_type, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        exit_context(iterate_context, the_jumper);
        if (using_iterator)
            value_remove_reference(iterator_value, the_jumper);
        else
            clean_up_simple_iteration_data(&simple_data);
        value_remove_reference(base_value, the_jumper);
        return;
      }

    while (TRUE)
      {
        boolean done;
        value *element_value;

        if (using_iterator)
          {
            value *done_value;
            verdict the_verdict;

            the_verdict = try_overloading(iterator_value, "is_done",
                    &done_value, 0, NULL, NULL, the_jumper,
                    get_statement_location(the_statement));
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(done_value == NULL);
                exit_context(iterate_context, the_jumper);
                value_remove_reference(iterator_value, the_jumper);
                value_remove_reference(base_value, the_jumper);
                return;
              }
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                assert(done_value == NULL);
                location_exception(the_jumper, base_location,
                        EXCEPTION_TAG(iterator_bad_is_done),
                        "Unable to call is_done() on the iterator of an "
                        "iterate statement.");
                exit_context(iterate_context, the_jumper);
                value_remove_reference(iterator_value, the_jumper);
                value_remove_reference(base_value, the_jumper);
                return;
              }

            assert(done_value != NULL);
            switch (get_value_kind(done_value))
              {
                case VK_TRUE:
                    done = TRUE;
                    break;
                case VK_FALSE:
                    done = FALSE;
                    break;
                default:
                    location_exception(the_jumper, base_location,
                            EXCEPTION_TAG(iterator_bad_is_done),
                            "The return value from is_done() on the iterator "
                            "of an iterate statement was not a boolean.");
                    value_remove_reference(done_value, the_jumper);
                    exit_context(iterate_context, the_jumper);
                    value_remove_reference(iterator_value, the_jumper);
                    value_remove_reference(base_value, the_jumper);
                    return;
              }

            value_remove_reference(done_value, the_jumper);
            assert(jumper_flowing_forward(the_jumper));
          }
        else
          {
            done = simple_iteration_data_is_done(&simple_data);
          }

        if (done)
          {
            exit_context(iterate_context, the_jumper);
            if (using_iterator)
                value_remove_reference(iterator_value, the_jumper);
            else
                clean_up_simple_iteration_data(&simple_data);
            value_remove_reference(base_value, the_jumper);
            return;
          }

        if (using_iterator)
          {
            verdict the_verdict;

            the_verdict = try_overloading(iterator_value, "current",
                    &element_value, 0, NULL, NULL, the_jumper,
                    get_statement_location(the_statement));
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(element_value == NULL);
                exit_context(iterate_context, the_jumper);
                value_remove_reference(iterator_value, the_jumper);
                value_remove_reference(base_value, the_jumper);
                return;
              }
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                assert(element_value == NULL);
                location_exception(the_jumper, base_location,
                        EXCEPTION_TAG(iterator_bad_current),
                        "Unable to call current() on the iterator of an "
                        "iterate statement.");
                exit_context(iterate_context, the_jumper);
                value_remove_reference(iterator_value, the_jumper);
                value_remove_reference(base_value, the_jumper);
                return;
              }
            assert(element_value != NULL);
          }
        else
          {
            element_value = simple_iteration_data_current(&simple_data,
                    the_jumper, get_statement_location(the_statement));
            if (!(jumper_flowing_forward(the_jumper)))
              {
                exit_context(iterate_context, the_jumper);
                return;
              }
          }

        assert(element_value != NULL);
        assert(variable_instance_is_instantiated(element_instance));
                /* VERIFIED */
        assert(!(variable_instance_scope_exited(element_instance)));
                /* VERIFIED */
        assert(variable_instance_lock_chain(element_instance) == NULL);
        set_variable_instance_value(element_instance, element_value,
                                    the_jumper);
        value_remove_reference(element_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            exit_context(iterate_context, the_jumper);
            if (using_iterator)
                value_remove_reference(iterator_value, the_jumper);
            else
                clean_up_simple_iteration_data(&simple_data);
            value_remove_reference(base_value, the_jumper);
            return;
          }

        if (iterate_statement_filter(the_statement) != NULL)
          {
            value *filter_result;

            filter_result = evaluate_expression(
                    iterate_statement_filter(the_statement), iterate_context,
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(filter_result == NULL);

                if (jumper_target(the_jumper) ==
                    loop_context_continue_target(iterate_context))
                  {
                    jumper_reached_target(the_jumper);
                    goto do_continue;
                  }

                if (using_iterator)
                    value_remove_reference(iterator_value, the_jumper);
                else
                    clean_up_simple_iteration_data(&simple_data);
                value_remove_reference(base_value, the_jumper);

                if (jumper_target(the_jumper) ==
                    loop_context_break_target(iterate_context))
                  {
                    assert(!(iterate_statement_is_parallel(the_statement)));
                    jumper_reached_target(the_jumper);
                  }

                exit_context(iterate_context, the_jumper);
                return;
              }

            assert(filter_result != NULL);
            switch (get_value_kind(filter_result))
              {
                case VK_TRUE:
                    value_remove_reference(filter_result, the_jumper);
                    assert(jumper_flowing_forward(the_jumper));
                    break;
                case VK_FALSE:
                    value_remove_reference(filter_result, the_jumper);
                    assert(jumper_flowing_forward(the_jumper));
                    goto do_continue;
                default:
                    expression_exception(the_jumper,
                            iterate_statement_filter(the_statement),
                            EXCEPTION_TAG(iterate_bad_test),
                            "The test argument to an iterate statement "
                            "evaluated to something other than a boolean "
                            "value.");
                    value_remove_reference(filter_result, the_jumper);
                    if (using_iterator)
                        value_remove_reference(iterator_value, the_jumper);
                    else
                        clean_up_simple_iteration_data(&simple_data);
                    value_remove_reference(base_value, the_jumper);
                    exit_context(iterate_context, the_jumper);
                    return;
              }
          }

        execute_statement_block(iterate_statement_body(the_statement),
                                iterate_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            if (jumper_target(the_jumper) ==
                loop_context_continue_target(iterate_context))
              {
                jumper_reached_target(the_jumper);
                goto do_continue;
              }

            if (using_iterator)
                value_remove_reference(iterator_value, the_jumper);
            else
                clean_up_simple_iteration_data(&simple_data);
            value_remove_reference(base_value, the_jumper);

            if (jumper_target(the_jumper) ==
                loop_context_break_target(iterate_context))
              {
                assert(!(iterate_statement_is_parallel(the_statement)));
                jumper_reached_target(the_jumper);
              }

            exit_context(iterate_context, the_jumper);
            return;
          }

      do_continue:
        if (using_iterator)
          {
            verdict the_verdict;

            the_verdict = try_overloading(iterator_value, "step", NULL, 0,
                    NULL, NULL, the_jumper,
                    get_statement_location(the_statement));
            if (!(jumper_flowing_forward(the_jumper)))
              {
                exit_context(iterate_context, the_jumper);
                value_remove_reference(iterator_value, the_jumper);
                value_remove_reference(base_value, the_jumper);
                return;
              }
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                location_exception(the_jumper, base_location,
                        EXCEPTION_TAG(iterator_bad_step),
                        "Unable to call step() on the iterator of an iterate "
                        "statement.");
                exit_context(iterate_context, the_jumper);
                value_remove_reference(iterator_value, the_jumper);
                value_remove_reference(base_value, the_jumper);
                return;
              }
          }
        else
          {
            simple_iteration_data_step(&simple_data, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                exit_context(iterate_context, the_jumper);
                return;
              }
          }
      }
  }

extern void execute_while_statement(statement *the_statement,
                                    context *the_context, jumper *the_jumper)
  {
    context *child_context;
    statement_block *step;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_WHILE);

    child_context = create_loop_context(the_context, the_statement, NULL,
            jumper_purity_level(the_jumper),
            get_statement_location(the_statement));
    if (child_context == NULL)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    step = while_statement_step(the_statement);

    while (TRUE)
      {
        value *test_value;

        test_value = evaluate_expression(while_statement_test(the_statement),
                                         child_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(test_value == NULL);
          do_jump:
            if (jumper_target(the_jumper) ==
                loop_context_continue_target(child_context))
              {
                jumper_reached_target(the_jumper);
                goto do_continue;
              }

            if (jumper_target(the_jumper) ==
                loop_context_break_target(child_context))
              {
                jumper_reached_target(the_jumper);
              }

            exit_context(child_context, the_jumper);
            return;
          }

        assert(test_value != NULL);
        switch (get_value_kind(test_value))
          {
            case VK_TRUE:
                value_remove_reference(test_value, the_jumper);
                assert(jumper_flowing_forward(the_jumper));
                break;
            case VK_FALSE:
                value_remove_reference(test_value, the_jumper);
                assert(jumper_flowing_forward(the_jumper));
                exit_context(child_context, the_jumper);
                return;
            default:
                expression_exception(the_jumper,
                        while_statement_test(the_statement),
                        EXCEPTION_TAG(while_bad_test),
                        "The test argument to a while statement evaluated to "
                        "something other than a boolean value.");
                value_remove_reference(test_value, the_jumper);
                exit_context(child_context, the_jumper);
                return;
          }

        execute_statement_block(while_statement_body(the_statement),
                                child_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            goto do_jump;

      do_continue:
        if (step != NULL)
          {
            execute_statement_block(step, child_context, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                goto do_jump;
          }
      }
  }

extern void execute_do_while_statement(statement *the_statement,
        context *the_context, jumper *the_jumper)
  {
    context *child_context;
    statement_block *step;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_DO_WHILE);

    child_context = create_loop_context(the_context, the_statement, NULL,
            jumper_purity_level(the_jumper),
            get_statement_location(the_statement));
    if (child_context == NULL)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    step = do_while_statement_step(the_statement);

    while (TRUE)
      {
        value *test_value;

        execute_statement_block(do_while_statement_body(the_statement),
                                child_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            goto do_jump;

      do_continue:
        if (step != NULL)
          {
            execute_statement_block(step, child_context, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                goto do_jump;
          }

        test_value = evaluate_expression(
                do_while_statement_test(the_statement), child_context,
                the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(test_value == NULL);
          do_jump:
            if (jumper_target(the_jumper) ==
                loop_context_continue_target(child_context))
              {
                jumper_reached_target(the_jumper);
                goto do_continue;
              }

            if (jumper_target(the_jumper) ==
                loop_context_break_target(child_context))
              {
                jumper_reached_target(the_jumper);
              }

            exit_context(child_context, the_jumper);
            return;
          }

        assert(test_value != NULL);
        switch (get_value_kind(test_value))
          {
            case VK_TRUE:
                value_remove_reference(test_value, the_jumper);
                assert(jumper_flowing_forward(the_jumper));
                break;
            case VK_FALSE:
                value_remove_reference(test_value, the_jumper);
                assert(jumper_flowing_forward(the_jumper));
                exit_context(child_context, the_jumper);
                return;
            default:
                expression_exception(the_jumper,
                        do_while_statement_test(the_statement),
                        EXCEPTION_TAG(do_while_bad_test),
                        "The test argument to a do-while statement evaluated "
                        "to something other than a boolean value.");
                value_remove_reference(test_value, the_jumper);
                exit_context(child_context, the_jumper);
                return;
          }
      }
  }

extern void execute_break_statement(statement *the_statement,
                                    context *the_context, jumper *the_jumper)
  {
    jump_target *target;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    target = find_break_target(the_context,
                               break_statement_from(the_statement));
    assert(target != NULL);
    assert(!(jump_target_scope_exited(target))); /* VERIFIED */
    jumper_set_target(the_jumper, target);
  }

extern void execute_continue_statement(statement *the_statement,
        context *the_context, jumper *the_jumper)
  {
    jump_target *target;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    target = find_continue_target(the_context,
                                  continue_statement_with(the_statement));
    assert(target != NULL);
    assert(!(jump_target_scope_exited(target))); /* VERIFIED */
    jumper_set_target(the_jumper, target);
  }

extern void execute_label_statement(statement *the_statement,
                                    context *the_context, jumper *the_jumper)
  {
    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_LABEL);
  }

extern void execute_statement_block_statement(statement *the_statement,
        context *the_context, jumper *the_jumper)
  {
    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_STATEMENT_BLOCK);

    execute_statement_block(statement_block_statement_block(the_statement),
                            the_context, the_jumper);
  }

extern void execute_single_statement(statement *the_statement,
                                     context *the_context, jumper *the_jumper)
  {
    value *lock_value;
    lock_instance *instance;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_SINGLE);

    expression *lock_expression = single_statement_lock(the_statement);
    if (lock_expression != NULL)
      {
        lock_value = evaluate_expression(lock_expression,
                                         the_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(lock_value == NULL);
            return;
          }

        assert(lock_value != NULL);
      }
    else
      {
        assert(single_statement_lock_declaration(the_statement) != NULL);
        lock_value = single_statement_lock_value(the_statement);
        if (lock_value == NULL)
          {
            purity_level *first_level;
            lock_instance *instance;
            verdict the_verdict;

            first_level =
                    purity_level_first_level(jumper_purity_level(the_jumper));
            instance = create_lock_instance(
                    single_statement_lock_declaration(the_statement),
                    first_level, NULL);
            if (instance == NULL)
              {
                jumper_do_abort(the_jumper);
                return;
              }

            assert(lock_instance_instance(instance) != NULL);
            set_instance_instantiated(
                    lock_instance_instance(instance));

            lock_value = create_lock_value(instance);
            lock_instance_remove_reference(instance, NULL);
            if (lock_value == NULL)
              {
                jumper_do_abort(the_jumper);
                return;
              }

            single_statement_set_lock_value(the_statement, lock_value);
            the_verdict = purity_level_add_sticky_lock_instance(first_level,
                                                                instance);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                set_lock_instance_scope_exited(instance, the_jumper);
                return;
              }
          }
        else
          {
            value_add_reference(lock_value);
          }
      }

    assert(lock_value != NULL);

    if (get_value_kind(lock_value) != VK_LOCK)
      {
        expression_exception(the_jumper, single_statement_lock(the_statement),
                EXCEPTION_TAG(single_lock_not_lock),
                "The `single' expression of a `single' statement evaluated to "
                "something other than a lock value.");
        value_remove_reference(lock_value, the_jumper);
        return;
      }

    instance = lock_value_data(lock_value);
    assert(instance != NULL);

    lock_instance_grab(instance, get_statement_location(the_statement),
                       the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(lock_value, the_jumper);
        return;
      }

    execute_statement_block(single_statement_block(the_statement), the_context,
                            the_jumper);

    lock_instance_release(instance, get_statement_location(the_statement),
                          the_jumper);

    value_remove_reference(lock_value, the_jumper);
  }

extern void execute_try_catch_statement(statement *the_statement,
        context *the_context, jumper *the_jumper)
  {
    context *new_context;
    size_t tagged_count;
    size_t tagged_num;
    statement_block *catcher_to_use;
    variable_declaration *exception_variable;
    verdict the_verdict;
    jumper *child_jumper;
    context *catcher_context;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_TRY_CATCH);

    new_context =
            create_try_catch_statement_context(the_context, the_statement);
    if (new_context == NULL)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    if (try_catch_statement_catcher(the_statement) != NULL)
      {
        jump_target *catcher_target;
        verdict the_verdict;

        catcher_target =
                try_catch_statement_context_default_catch_target(new_context);

        assert(!(jump_target_scope_exited(catcher_target))); /* VERIFIED */
        the_verdict = jumper_push_catcher(the_jumper, catcher_target, NULL);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            exit_context(new_context, the_jumper);
            return;
          }
      }

    tagged_count = try_catch_statement_tagged_catcher_count(the_statement);
    for (tagged_num = tagged_count; tagged_num > 0;)
      {
        jump_target *catcher_target;
        type_expression *tag_type_expression;
        type *tag_type;
        verdict the_verdict;

        --tagged_num;
        catcher_target = try_catch_statement_context_catch_target(new_context,
                                                                  tagged_num);

        tag_type_expression = try_catch_statement_tagged_catcher_tag_type(
                the_statement, tagged_num);
        if (tag_type_expression != NULL)
          {
            tag_type = evaluate_type_expression(tag_type_expression,
                                                the_context, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(tag_type == NULL);
                goto unwind;
              }
          }
        else
          {
            tag_type = get_anything_type();
            type_add_reference(tag_type);
          }

        assert(tag_type != NULL);

        assert(!(jump_target_scope_exited(catcher_target))); /* VERIFIED */
        the_verdict =
                jumper_push_catcher(the_jumper, catcher_target, tag_type);
        type_remove_reference(tag_type, the_jumper);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
          unwind:
            jumper_do_abort(the_jumper);
            while (tagged_num > 0)
              {
                --tagged_num;
                catcher_target = try_catch_statement_context_catch_target(
                        new_context, tagged_num);
                assert(!(jump_target_scope_exited(catcher_target)));
                        /* VERIFIED */
                jumper_pop_catcher(the_jumper, catcher_target);
              }
            exit_context(new_context, the_jumper);
            return;
          }
      }

    execute_statement_block(try_catch_statement_body(the_statement),
                            new_context, the_jumper);

    catcher_to_use = NULL;
    exception_variable = NULL;

    for (tagged_num = 0; tagged_num < tagged_count; ++tagged_num)
      {
        jump_target *catcher_target;

        catcher_target = try_catch_statement_context_catch_target(new_context,
                                                                  tagged_num);
        assert(!(jump_target_scope_exited(catcher_target))); /* VERIFIED */
        jumper_pop_catcher(the_jumper, catcher_target);

        if (jumper_target(the_jumper) == catcher_target)
          {
            jumper_reached_target(the_jumper);
            assert(catcher_to_use == NULL);
            catcher_to_use = try_catch_statement_tagged_catcher_catcher(
                    the_statement, tagged_num);
            exception_variable = try_catch_statement_tagged_catcher_exception(
                    the_statement, tagged_num);
          }
      }

    if (try_catch_statement_catcher(the_statement) != NULL)
      {
        jump_target *catcher_target;

        catcher_target =
                try_catch_statement_context_default_catch_target(new_context);

        assert(!(jump_target_scope_exited(catcher_target))); /* VERIFIED */
        jumper_pop_catcher(the_jumper, catcher_target);

        if (jumper_target(the_jumper) == catcher_target)
          {
            jumper_reached_target(the_jumper);
            assert(catcher_to_use == NULL);
            catcher_to_use = try_catch_statement_catcher(the_statement);
          }
      }

    exit_context(new_context, the_jumper);

    if ((!(jumper_flowing_forward(the_jumper))) &&
        (jumper_target(the_jumper) == NULL))
      {
        assert(catcher_to_use == NULL);
      }

    if (!(jumper_has_unreleased_exception_information(the_jumper)))
      {
        assert(catcher_to_use == NULL);
      }

    if (catcher_to_use == NULL)
        return;

    child_jumper = create_sub_jumper(the_jumper);
    if (child_jumper == NULL)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    the_verdict =
            jumper_release_exception_information(the_jumper, child_jumper);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_jumper(child_jumper);
        jumper_do_abort(the_jumper);
        return;
      }

    if (exception_variable == NULL)
      {
        catcher_context = the_context;
      }
    else
      {
        variable_instance *instance;

        catcher_context = create_singleton_variable_context(the_context,
                exception_variable, jumper_purity_level(child_jumper));
        if (catcher_context == NULL)
          {
            jumper_clear_exception_information(child_jumper);
            delete_jumper(child_jumper);
            jumper_do_abort(the_jumper);
            return;
          }

        instance = find_variable_instance(catcher_context, exception_variable);
        assert(instance != NULL);

        set_variable_instance_type(instance, get_anything_type(),
                                   child_jumper);
        if (!(jumper_flowing_forward(child_jumper)))
          {
            exit_context(catcher_context, child_jumper);
            jumper_transfer_to_parent(child_jumper);
            jumper_clear_exception_information(child_jumper);
            delete_jumper(child_jumper);
            return;
          }

        set_variable_instance_value(instance,
                value_component_value(
                        jumper_exception_information(child_jumper), 0),
                child_jumper);
        if (!(jumper_flowing_forward(child_jumper)))
          {
            exit_context(catcher_context, child_jumper);
            jumper_transfer_to_parent(child_jumper);
            jumper_clear_exception_information(child_jumper);
            delete_jumper(child_jumper);
            return;
          }

        set_variable_instance_instantiated(instance);
      }

    execute_statement_block(catcher_to_use, catcher_context, child_jumper);

    if (catcher_context != the_context)
        exit_context(catcher_context, child_jumper);

    jumper_transfer_to_parent(child_jumper);
    jumper_clear_exception_information(child_jumper);
    delete_jumper(child_jumper);
  }

extern void execute_try_handle_statement(statement *the_statement,
        context *the_context, jumper *the_jumper)
  {
    value *handler_value;
    verdict the_verdict;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_TRY_HANDLE);

    handler_value = evaluate_expression(
            try_handle_statement_handler(the_statement), the_context,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(handler_value == NULL);
        return;
      }

    assert(handler_value != NULL);

    switch (get_value_kind(handler_value))
      {
        case VK_ROUTINE:
            break;
        case VK_ROUTINE_CHAIN:
            break;
        default:
            expression_exception(the_jumper,
                    try_handle_statement_handler(the_statement),
                    EXCEPTION_TAG(try_handle_bad_handler),
                    "The handler expression for a try-handle statement did not"
                    " evaluate to a routine.");
            value_remove_reference(handler_value, the_jumper);
            return;
      }

    the_verdict = jumper_push_handler(the_jumper, handler_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        value_remove_reference(handler_value, the_jumper);
        return;
      }

    execute_statement_block(try_handle_statement_body(the_statement),
                            the_context, the_jumper);

    jumper_pop_handler(the_jumper, handler_value);

    value_remove_reference(handler_value, the_jumper);
  }

extern void execute_export_statement(statement *the_statement,
                                     context *the_context, jumper *the_jumper)
  {
    routine_declaration *from_declaration;
    value *return_value;
    object *the_object;
    size_t item_count;
    size_t item_num;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_EXPORT);

    from_declaration = export_statement_from_routine(the_statement);
    assert(from_declaration != NULL);

    return_value = get_routine_return_value(the_context, from_declaration);
    assert(return_value != NULL);

    assert(get_value_kind(return_value) == VK_OBJECT);
    the_object = object_value_data(return_value);
    assert(the_object != NULL);

    item_count = export_statement_item_count(the_statement);

    if (item_count == 0)
      {
        assert(!(object_is_closed(the_object))); /* VERIFIED */
        object_set_export_mode(the_object, TRUE);
        return;
      }

    for (item_num = 0; item_num < item_count; ++item_num)
      {
        const char *exported_as;
        expression *to_export;

        exported_as =
                export_statement_item_exported_as(the_statement, item_num);
        to_export = export_statement_item_to_export(the_statement, item_num);

        switch (get_expression_kind(to_export))
          {
            case EK_VARIABLE_REFERENCE:
              {
                assert(!(object_is_closed(the_object))); /* VERIFIED */
                object_add_variable_field(the_object,
                        variable_reference_expression_instance(to_export,
                                                               the_context),
                        exported_as, the_jumper);
                break;
              }
            case EK_ROUTINE_REFERENCE:
              {
                routine_declaration_chain *declaration_chain;
                routine_instance_chain *instance_chain;

                declaration_chain =
                        routine_reference_expression_chain(to_export);
                assert(declaration_chain != NULL);

                instance_chain =
                        routine_declaration_chain_to_routine_instance_chain(
                                declaration_chain, the_context, the_jumper);
                if (instance_chain == NULL)
                  {
                    assert(!(jumper_flowing_forward(the_jumper)));
                    return;
                  }
                assert(jumper_flowing_forward(the_jumper));

                assert(!(object_is_closed(the_object))); /* VERIFIED */
                object_add_routine_chain_field(the_object, instance_chain,
                                               exported_as, the_jumper);
                routine_instance_chain_remove_reference(instance_chain,
                                                        the_jumper);
                break;
              }
            case EK_LABEL_REFERENCE:
              {
              label_reference:
                statement_exception(the_jumper, the_statement,
                        EXCEPTION_TAG(export_label),
                        "Attempted to export a label.");
                return;
              }
            case EK_TAGALONG_REFERENCE:
              {
                assert(!(object_is_closed(the_object))); /* VERIFIED */
                object_add_tagalong_field(the_object,
                        find_tagalong_instance(the_context,
                                tagalong_reference_expression_declaration(
                                        to_export)), exported_as, the_jumper);
                break;
              }
            case EK_LEPTON_KEY_REFERENCE:
              {
                assert(!(object_is_closed(the_object))); /* VERIFIED */
                object_add_lepton_key_field(the_object,
                        find_lepton_key_instance(the_context,
                                lepton_key_reference_expression_declaration(
                                        to_export)), exported_as, the_jumper);
                break;
              }
            case EK_QUARK_REFERENCE:
              {
                assert(!(object_is_closed(the_object))); /* VERIFIED */
                object_add_quark_field(the_object,
                        find_quark_instance(the_context,
                                quark_reference_expression_declaration(
                                        to_export)), exported_as, the_jumper);
                break;
              }
            case EK_LOCK_REFERENCE:
              {
                assert(!(object_is_closed(the_object))); /* VERIFIED */
                object_add_lock_field(the_object,
                        find_lock_instance(the_context,
                                lock_reference_expression_declaration(
                                        to_export)), exported_as, the_jumper);
                break;
              }
            case EK_USE_REFERENCE:
              {
                instance *the_instance;
                routine_instance_chain *instance_chain;
                jump_target *the_jump_target;

                find_instance_from_use_statement(
                        use_reference_expression_use_statement(to_export),
                        use_reference_expression_used_for_num(to_export),
                        the_context, TRUE, &the_instance, &instance_chain,
                        &the_jump_target, get_expression_location(to_export),
                        the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    assert(the_instance == NULL);
                    assert(instance_chain == NULL);
                    return;
                  }

                if (the_instance != NULL)
                  {
                    assert(instance_chain == NULL);
                    assert(the_jump_target == NULL);

                    switch (instance_kind(the_instance))
                      {
                        case NK_VARIABLE:
                            assert(!(object_is_closed(the_object)));
                                    /* VERIFIED */
                            object_add_variable_field(the_object,
                                    instance_variable_instance(the_instance),
                                    exported_as, the_jumper);
                            break;
                        case NK_ROUTINE:
                            assert(!(object_is_closed(the_object)));
                                    /* VERIFIED */
                            object_add_routine_field(the_object,
                                    instance_routine_instance(the_instance),
                                    exported_as, the_jumper);
                            break;
                        case NK_TAGALONG:
                            assert(!(object_is_closed(the_object)));
                                    /* VERIFIED */
                            object_add_tagalong_field(the_object,
                                    instance_tagalong_instance(the_instance),
                                    exported_as, the_jumper);
                            break;
                        case NK_LEPTON_KEY:
                            assert(!(object_is_closed(the_object)));
                                    /* VERIFIED */
                            object_add_lepton_key_field(the_object,
                                    instance_lepton_key_instance(the_instance),
                                    exported_as, the_jumper);
                            break;
                        case NK_QUARK:
                            assert(!(object_is_closed(the_object)));
                                    /* VERIFIED */
                            object_add_quark_field(the_object,
                                    instance_quark_instance(the_instance),
                                    exported_as, the_jumper);
                            break;
                        case NK_LOCK:
                            assert(!(object_is_closed(the_object)));
                                    /* VERIFIED */
                            object_add_lock_field(the_object,
                                    instance_lock_instance(the_instance),
                                    exported_as, the_jumper);
                            break;
                        default:
                            assert(FALSE);
                      }
                  }
                else if (instance_chain != NULL)
                  {
                    assert(the_jump_target == NULL);

                    assert(!(object_is_closed(the_object))); /* VERIFIED */
                    object_add_routine_chain_field(the_object, instance_chain,
                                                   exported_as, the_jumper);
                    routine_instance_chain_remove_reference(instance_chain,
                                                            the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                        return;
                  }
                else
                  {
                    assert(the_jump_target != NULL);

                    goto label_reference;
                  }

                break;
              }
            default:
              {
                assert(FALSE);
              }
          }

        if (!(jumper_flowing_forward(the_jumper)))
            return;
      }
  }

extern void execute_hide_statement(statement *the_statement,
                                   context *the_context, jumper *the_jumper)
  {
    routine_declaration *from_declaration;
    value *return_value;
    object *the_object;
    size_t item_count;
    size_t item_num;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_HIDE);

    from_declaration = hide_statement_from_routine(the_statement);
    assert(from_declaration != NULL);

    return_value = get_routine_return_value(the_context, from_declaration);
    assert(return_value != NULL);

    assert(get_value_kind(return_value) == VK_OBJECT);
    the_object = object_value_data(return_value);
    assert(the_object != NULL);

    item_count = hide_statement_item_count(the_statement);

    if (item_count == 0)
      {
        assert(!(object_is_closed(the_object))); /* VERIFIED */
        object_set_export_mode(the_object, FALSE);
        return;
      }

    for (item_num = 0; item_num < item_count; ++item_num)
      {
        assert(!(object_is_closed(the_object))); /* VERIFIED */
        object_remove_field(the_object,
                hide_statement_item_to_hide(the_statement, item_num),
                the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return;
      }
  }

extern void execute_use_statement(statement *the_statement,
        context *the_context, jumper *the_jumper, object *this_object,
        virtual_lookup *virtual_parent)
  {
    variable_declaration *container_declaration;
    variable_instance *container_instance;
    type *anything_type;
    expression *to_use_expression;
    value *to_use_value;
    object *to_use_object;
    string_index *field_instances;
    use_instance *the_use_instance;
    size_t used_for_count;
    size_t used_for_num;
    type_expression *the_type_expression;
    type *the_type;
    boolean doubt;
    char *why_not;
    boolean is_in;

    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_USE);

    container_declaration = use_statement_container(the_statement);
    assert(container_declaration != NULL);

    container_instance =
            find_variable_instance(the_context, container_declaration);
    assert(container_instance != NULL);

    assert(!(variable_instance_scope_exited(container_instance)));
            /* VERIFIED */
    if (!(variable_instance_is_instantiated(container_instance)))
        set_variable_instance_instantiated(container_instance);

    anything_type = get_anything_type();
    if (anything_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    assert(!(variable_instance_scope_exited(container_instance)));
            /* VERIFIED */
    set_variable_instance_type(container_instance, anything_type, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return;

    to_use_expression = use_statement_to_use(the_statement);

    to_use_value = evaluate_expression_with_virtuals(to_use_expression,
            the_context, virtual_parent, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(to_use_value == NULL);
        return;
      }

    assert(to_use_value != NULL);

    assert(variable_instance_is_instantiated(container_instance));
            /* VERIFIED */
    assert(!(variable_instance_scope_exited(container_instance)));
            /* VERIFIED */
    assert(variable_instance_lock_chain(container_instance) == NULL);
    set_variable_instance_value(container_instance, to_use_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(to_use_value, the_jumper);
        return;
      }

    value_remove_reference(to_use_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return;

    switch (get_value_kind(to_use_value))
      {
        case VK_SEMI_LABELED_VALUE_LIST:
        case VK_SEMI_LABELED_MULTI_SET:
        case VK_LEPTON:
            to_use_object = NULL;
            field_instances = create_string_index();
            if (field_instances == NULL)
              {
                jumper_do_abort(the_jumper);
                return;
              }
            break;
        case VK_OBJECT:
            to_use_object = object_value_data(to_use_value);
            assert(to_use_object != NULL);
            field_instances = NULL;
            break;
        default:
            statement_exception(the_jumper, the_statement,
                    EXCEPTION_TAG(use_bad_base),
                    "The expression for a `use' statement evaluated to a value"
                    " without fields.");
            return;
      }

    check_value_validity(to_use_value,
            get_expression_location(to_use_expression), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (field_instances != NULL)
            destroy_string_index(field_instances);
        return;
      }

    assert((to_use_object == NULL) || !(object_is_closed(to_use_object)));
            /* VERIFIED */

    assert((this_object == NULL) || !(object_is_closed(this_object)));
            /* VERIFIED */
    if ((this_object != NULL) && object_export_enabled(this_object))
      {
        size_t for_item_count;

        for_item_count = use_statement_for_item_count(the_statement);

        if (for_item_count > 0)
          {
            size_t for_item_num;

            for (for_item_num = 0; for_item_num < for_item_count;
                 ++for_item_num)
              {
                const char *source_field_name;

                source_field_name = use_statement_for_item_to_export(
                        the_statement, for_item_num);

                if (to_use_object == NULL)
                  {
                    variable_instance *instance;

                    assert(field_instances != NULL);

                    instance = (variable_instance *)(lookup_in_string_index(
                            field_instances, source_field_name));

                    if (instance == NULL)
                      {
                        value *field_value;
                        verdict the_verdict;

                        field_value = value_get_field(source_field_name,
                                                      to_use_value);
                        if (field_value == NULL)
                          {
                            statement_exception(the_jumper, the_statement,
                                    EXCEPTION_TAG(use_missing_field),
                                    "A `use' statement specified using a field"
                                    " that didn't exist in the object.");
                            assert(field_instances != NULL);
                            destroy_string_index(field_instances);
                            return;
                          }

                        instance = immutable_for_value(field_value,
                                source_field_name, the_context,
                                get_expression_location(to_use_expression),
                                the_jumper);
                        if (instance == NULL)
                          {
                            assert(!(jumper_flowing_forward(the_jumper)));
                            assert(field_instances != NULL);
                            destroy_string_index(field_instances);
                            return;
                          }

                        the_verdict = enter_into_string_index(field_instances,
                                source_field_name, instance);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            jumper_do_abort(the_jumper);
                            variable_instance_remove_reference(instance,
                                                               the_jumper);
                            assert(field_instances != NULL);
                            destroy_string_index(field_instances);
                            return;
                          }
                      }
                    else
                      {
                        variable_instance_add_reference(instance);
                      }

                    assert(!(object_is_closed(this_object))); /* VERIFIED */
                    object_add_variable_field(this_object, instance,
                            use_statement_for_item_exported_as(the_statement,
                                    for_item_num), the_jumper);
                    variable_instance_remove_reference(instance, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        assert(field_instances != NULL);
                        destroy_string_index(field_instances);
                        return;
                      }
                  }
                else
                  {
                    size_t source_field_num;

                    assert(!(object_is_closed(to_use_object))); /* VERIFIED */
                    source_field_num = object_field_lookup(to_use_object,
                                                           source_field_name);
                    if (source_field_num >= object_field_count(to_use_object))
                      {
                        statement_exception(the_jumper, the_statement,
                                EXCEPTION_TAG(use_missing_field),
                                "A `use' statement specified using a field "
                                "that didn't exist in the object.");
                        assert(field_instances == NULL);
                        return;
                      }

                    assert(!(object_is_closed(to_use_object))); /* VERIFIED */
                    assert(!(object_is_closed(this_object))); /* VERIFIED */
                    export_field_to_object(to_use_object, source_field_num,
                            this_object,
                            use_statement_for_item_exported_as(the_statement,
                                                               for_item_num),
                            the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        assert(field_instances == NULL);
                        return;
                      }
                  }
              }
          }
        else
          {
            if (to_use_object == NULL)
              {
                size_t source_field_count;
                size_t source_field_num;

                assert(field_instances != NULL);

                source_field_count = value_component_count(to_use_value);

                for (source_field_num = 0;
                     source_field_num < source_field_count; ++source_field_num)
                  {
                    const char *field_name;
                    variable_instance *instance;

                    field_name = value_component_label(to_use_value,
                                                       source_field_num);
                    if (field_name == NULL)
                        continue;

                    if (use_statement_is_exception(the_statement, field_name))
                        continue;

                    instance = (variable_instance *)(lookup_in_string_index(
                            field_instances, field_name));

                    if (instance == NULL)
                      {
                        value *field_value;
                        verdict the_verdict;

                        field_value = value_component_value(to_use_value,
                                                            source_field_num);
                        assert(field_value != NULL);

                        instance = immutable_for_value(field_value, field_name,
                                the_context,
                                get_expression_location(to_use_expression),
                                the_jumper);
                        if (instance == NULL)
                          {
                            assert(!(jumper_flowing_forward(the_jumper)));
                            assert(field_instances != NULL);
                            destroy_string_index(field_instances);
                            return;
                          }

                        the_verdict = enter_into_string_index(field_instances,
                                field_name, instance);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            jumper_do_abort(the_jumper);
                            variable_instance_remove_reference(instance,
                                                               the_jumper);
                            assert(field_instances != NULL);
                            destroy_string_index(field_instances);
                            return;
                          }
                      }
                    else
                      {
                        variable_instance_add_reference(instance);
                      }

                    assert(!(object_is_closed(this_object))); /* VERIFIED */
                    object_add_variable_field(this_object, instance,
                                              field_name, the_jumper);
                    variable_instance_remove_reference(instance, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        assert(field_instances != NULL);
                        destroy_string_index(field_instances);
                        return;
                      }
                  }
              }
            else
              {
                size_t source_field_count;
                size_t source_field_num;

                assert(!(object_is_closed(to_use_object))); /* VERIFIED */
                source_field_count = object_field_count(to_use_object);

                for (source_field_num = 0;
                     source_field_num < source_field_count; ++source_field_num)
                  {
                    const char *field_name;

                    assert(!(object_is_closed(to_use_object))); /* VERIFIED */
                    field_name =
                            object_field_name(to_use_object, source_field_num);
                    assert(field_name != NULL);

                    if (use_statement_is_exception(the_statement, field_name))
                        continue;

                    assert(!(object_is_closed(to_use_object))); /* VERIFIED */
                    assert(!(object_is_closed(this_object))); /* VERIFIED */
                    export_field_to_object(to_use_object, source_field_num,
                            this_object, field_name, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        assert(field_instances == NULL);
                        return;
                      }
                  }
              }
          }
      }

    the_use_instance = find_use_instance(the_context, the_statement);
    assert(the_use_instance != NULL);

    if (use_instance_is_instantiated(the_use_instance))
      {
        statement_exception(the_jumper, the_statement,
                EXCEPTION_TAG(use_multiply_instantiated),
                "A `use' statement was executed a second time in the same "
                "scope.");
        if (field_instances != NULL)
            destroy_string_index(field_instances);
        return;
      }

    use_instance_set_instantiated(the_use_instance);

    used_for_count = use_statement_used_for_count(the_statement);
    for (used_for_num = 0; used_for_num < used_for_count; ++used_for_num)
      {
        const char *name;

        name = use_statement_used_for_name(the_statement, used_for_num);
        assert(name != NULL);

        if (to_use_object == NULL)
          {
            variable_instance *instance;

            assert(field_instances != NULL);

            instance = (variable_instance *)(lookup_in_string_index(
                    field_instances, name));

            if (instance == NULL)
              {
                value *field_value;

                field_value = value_get_field(name, to_use_value);
                if (field_value != NULL)
                  {
                    verdict the_verdict;

                    instance = immutable_for_value(field_value, name,
                            the_context,
                            get_expression_location(to_use_expression),
                            the_jumper);
                    if (instance == NULL)
                      {
                        assert(!(jumper_flowing_forward(the_jumper)));
                        assert(field_instances != NULL);
                        destroy_string_index(field_instances);
                        return;
                      }

                    the_verdict = enter_into_string_index(field_instances,
                                                          name, instance);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        jumper_do_abort(the_jumper);
                        variable_instance_remove_reference(instance,
                                                           the_jumper);
                        assert(field_instances != NULL);
                        destroy_string_index(field_instances);
                        return;
                      }
                  }
              }
            else
              {
                variable_instance_add_reference(instance);
              }

            if (instance != NULL)
              {
                use_instance_set_instance(the_use_instance, used_for_num,
                        variable_instance_instance(instance));
                variable_instance_remove_reference(instance, the_jumper);
                continue;
              }
          }
        else
          {
            size_t field_num;

            assert(!(object_is_closed(to_use_object))); /* VERIFIED */
            field_num = object_field_lookup(to_use_object, name);
            if (field_num < object_field_count(to_use_object))
              {
                assert(!(object_is_closed(to_use_object))); /* VERIFIED */
                if (object_field_is_routine_chain(to_use_object, field_num))
                  {
                    routine_instance_chain *chain;

                    assert(!(object_is_closed(to_use_object))); /* VERIFIED */
                    chain = object_field_routine_chain(to_use_object,
                                                       field_num);

                    use_instance_set_chain(the_use_instance, used_for_num,
                                           chain);
                  }
                else
                  {
                    instance *the_instance;

                    assert(!(object_is_closed(to_use_object))); /* VERIFIED */
                    the_instance =
                            object_field_instance(to_use_object, field_num);

                    use_instance_set_instance(the_use_instance, used_for_num,
                                              the_instance);
                  }
                continue;
              }
          }

        if (!(use_statement_used_for_required(the_statement, used_for_num)))
            continue;

        if ((use_statement_used_for_declaration(the_statement, used_for_num) !=
             NULL) ||
            (use_statement_used_for_chain(the_statement, used_for_num) != NULL)
            ||
            (use_statement_used_for_label_statement(the_statement,
                                                    used_for_num) != NULL))
          {
            continue;
          }

        if (use_statement_used_for_next_use(the_statement, used_for_num) !=
            NULL)
          {
            instance *result_instance;
            routine_instance_chain *result_chain;
            jump_target *result_jump_target;

            find_instance_from_use_statement(
                    use_statement_used_for_next_use(the_statement,
                                                    used_for_num),
                    use_statement_used_for_next_used_for_number(the_statement,
                            used_for_num), the_context, FALSE,
                    &result_instance, &result_chain, &result_jump_target,
                    get_statement_location(the_statement), the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                if (field_instances != NULL)
                    destroy_string_index(field_instances);
                return;
              }

            if ((result_instance != NULL) || (result_chain != NULL) ||
                (result_jump_target != NULL))
              {
                if (result_chain != NULL)
                  {
                    routine_instance_chain_remove_reference(result_chain,
                                                            the_jumper);
                  }
                continue;
              }
          }

        statement_exception(the_jumper, the_statement,
                EXCEPTION_TAG(use_unbound_remains),
                "Unbound use of `%s'%Z that could have been resolved by a "
                "`use' statement was left unbound.", name, " (used at ",
                use_statement_used_for_ultimate_use_location(the_statement,
                        used_for_num), ")");
        if (field_instances != NULL)
            destroy_string_index(field_instances);
        return;
      }

    if (field_instances != NULL)
        destroy_string_index(field_instances);

    the_type_expression = variable_declaration_type(container_declaration);

    the_type = evaluate_type_expression(the_type_expression, the_context,
                                        the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_type == NULL);
        return;
      }
    assert(the_type != NULL);

    check_type_validity(the_type,
            get_type_expression_location(the_type_expression), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(the_type, the_jumper);
        return;
      }

    assert(type_is_valid(the_type)); /* VERIFIED */
    is_in = value_is_in_type(to_use_value, the_type, &doubt, &why_not,
            get_statement_location(the_statement), the_jumper);
    type_remove_reference(the_type, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return;

    if (doubt)
      {
        location_exception(the_jumper, get_statement_location(the_statement),
                EXCEPTION_TAG(use_type_match_indeterminate),
                "When executing a use statement, %s was unable to determine "
                "whether the value was in the type because %s.",
                interpreter_name(), why_not);
        free(why_not);
        return;
      }

    if (!is_in)
      {
        location_exception(the_jumper, get_statement_location(the_statement),
                EXCEPTION_TAG(use_type_mismatch),
                "The value for a use statement didn't match its type because "
                "%s.", why_not);
        free(why_not);
        return;
      }
  }

extern void execute_theorem_statement(statement *the_statement,
                                      context *the_context, jumper *the_jumper)
  {
    assert(the_statement != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_statement_kind(the_statement) == SK_THEOREM);

    /* Executing a theorem statement has no effect.  The claim is *not*
     * evaluated.  Theorem statements are for the sake of static analysis only,
     * not for any runtime behavior. */
  }

extern void execute_statement_block(statement_block *the_statement_block,
                                    context *the_context, jumper *the_jumper)
  {
    execute_statement_block_with_virtuals(the_statement_block, the_context,
                                          the_jumper, NULL, NULL);
  }

extern void execute_statement_block_with_virtuals(
        statement_block *the_statement_block, context *the_context,
        jumper *the_jumper, object *this_object,
        virtual_lookup *virtual_parent)
  {
    reference_cluster *cluster;
    context *local_context;
    virtual_lookup *local_virtual_lookup;
    statement_aa cleanups;
    size_t statement_count;
    size_t statement_num;
    size_t cleanup_num;

    assert(the_statement_block != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    cluster = ((this_object == NULL) ? NULL :
               object_reference_cluster(this_object));
    local_context = create_statement_block_context(the_context,
            the_statement_block, cluster, virtual_parent, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(local_context == NULL);
        return;
      }
    assert(local_context != NULL);

    local_virtual_lookup = create_virtual_lookup(the_statement_block,
            local_context, virtual_parent, cluster);
    if (local_virtual_lookup == NULL)
      {
        jumper_do_abort(the_jumper);
        exit_context(local_context, the_jumper);
        return;
      }

    if (this_object == NULL)
      {
        verdict the_verdict;

        the_verdict = statement_aa_init(&cleanups, 10);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            delete_virtual_lookup(local_virtual_lookup);
            exit_context(local_context, the_jumper);
            return;
          }
      }
    else
      {
        assert(!(object_is_closed(this_object))); /* VERIFIED */
        object_set_block_context(this_object, local_context);
      }

    statement_count = statement_block_statement_count(the_statement_block);
    for (statement_num = 0; statement_num < statement_count; ++statement_num)
      {
        statement *the_statement;

        the_statement =
                statement_block_statement(the_statement_block, statement_num);

        if (get_statement_kind(the_statement) == SK_CLEANUP)
          {
            verdict the_verdict;

            if (this_object == NULL)
              {
                the_verdict = statement_aa_append(&cleanups, the_statement);
              }
            else
              {
                assert(!(object_is_closed(this_object))); /* VERIFIED */
                the_verdict =
                        object_append_cleanup(this_object, the_statement);
              }
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                if (this_object == NULL)
                  {
                    free(cleanups.array);
                  }
                else
                  {
                    assert(!(object_is_closed(this_object))); /* VERIFIED */
                    object_set_block_context(this_object, NULL);
                  }
                delete_virtual_lookup(local_virtual_lookup);
                exit_context(local_context, the_jumper);
                return;
              }

            continue;
          }

        execute_statement_with_virtuals(the_statement, local_context,
                the_jumper, this_object, local_virtual_lookup);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            jump_target *target;
            statement *label_statement;

            target = jumper_target(the_jumper);
            if (target == NULL)
              {
                if (this_object == NULL)
                  {
                    free(cleanups.array);
                  }
                else
                  {
                    assert(!(object_is_closed(this_object))); /* VERIFIED */
                    object_set_block_context(this_object, NULL);
                  }
                delete_virtual_lookup(local_virtual_lookup);
                exit_context(local_context, the_jumper);
                return;
              }

            assert(!(jump_target_scope_exited(target))); /* VERIFIED */
            if (jump_target_context(target) != local_context)
                goto do_cleanup;

            jumper_reached_target(the_jumper);

            assert(!(jump_target_scope_exited(target))); /* VERIFIED */
            label_statement =
                    label_jump_target_label_statement(target);
            assert(label_statement != NULL);

            statement_num = 0;
            while (TRUE)
              {
                assert(statement_num < statement_count);
                if (statement_block_statement(the_statement_block,
                            statement_num) == label_statement)
                  {
                    break;
                  }
                ++statement_num;
              }
          }
      }

  do_cleanup:
    delete_virtual_lookup(local_virtual_lookup);

    if (this_object != NULL)
        return;

    cleanup_num = cleanups.element_count;
    while (cleanup_num > 0)
      {
        statement *cleanup_statement;
        jumper *child_jumper;

        --cleanup_num;

        cleanup_statement = cleanups.array[cleanup_num];

        child_jumper = create_sub_jumper(the_jumper);
        if (child_jumper == NULL)
          {
            jumper_do_abort(the_jumper);
            free(cleanups.array);
            exit_context(local_context, the_jumper);
            return;
          }

        execute_statement_block(cleanup_statement_body(cleanup_statement),
                                local_context, child_jumper);

        if (!(jumper_flowing_forward(child_jumper)))
          {
            jump_target *target;

            target = jumper_target(child_jumper);
            if (target == NULL)
              {
                jumper_do_abort(the_jumper);
                delete_jumper(child_jumper);
                break;
              }

            assert(!(jump_target_scope_exited(target))); /* VERIFIED */
            if (jump_target_context(target) == local_context)
              {
                statement_exception(the_jumper, cleanup_statement,
                        EXCEPTION_TAG(cleanup_jump),
                        "A jump was attempted from within a cleanup statement "
                        "to another part of the statement block containing "
                        "that cleanup statement.");
                delete_jumper(child_jumper);
                break;
              }
          }

        jumper_transfer_to_parent(child_jumper);
        delete_jumper(child_jumper);
      }

    free(cleanups.array);
    exit_context(local_context, the_jumper);
    return;
  }

extern void execute_declaration(declaration *the_declaration,
        instance *the_instance, context *the_context, jumper *the_jumper,
        object *this_object)
  {
    assert(the_declaration != NULL);
    assert(the_instance != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert((this_object == NULL) || !(object_is_closed(this_object)));
            /* VERIFIED */

    if (instance_is_instantiated(the_instance))
      {
        static_exception_tag *tag;

        switch (instance_kind(the_instance))
          {
            case NK_VARIABLE:
                tag = EXCEPTION_TAG(variable_declaration_re_executed);
                break;
            case NK_ROUTINE:
                tag = EXCEPTION_TAG(routine_declaration_re_executed);
                break;
            case NK_TAGALONG:
                tag = EXCEPTION_TAG(tagalong_declaration_re_executed);
                break;
            case NK_LEPTON_KEY:
                tag = EXCEPTION_TAG(lepton_key_declaration_re_executed);
                break;
            case NK_QUARK:
                tag = EXCEPTION_TAG(quark_declaration_re_executed);
                break;
            case NK_LOCK:
                tag = EXCEPTION_TAG(lock_declaration_re_executed);
                break;
            case NK_JUMP_TARGET:
                assert(FALSE);
            default:
                assert(FALSE);
          }

        location_exception(the_jumper,
                get_declaration_location(the_declaration), tag,
                "A %s declaration was executed more than once for the same "
                "instance.", name_kind_name(instance_kind(the_instance)));

        return;
      }

    switch (declaration_kind(the_declaration))
      {
        case NK_VARIABLE:
            assert(!(variable_instance_is_instantiated(
                             instance_variable_instance(the_instance))));
                    /* VERIFIED */
            assert((this_object == NULL) || !(object_is_closed(this_object)));
                    /* VERIFIED */
            execute_variable_declaration(
                    declaration_variable_declaration(the_declaration),
                    instance_variable_instance(the_instance), the_context,
                    the_jumper, this_object);
            break;
        case NK_ROUTINE:
            assert(!(routine_instance_is_instantiated(
                             instance_routine_instance(the_instance))));
                    /* VERIFIED */
            assert((this_object == NULL) || !(object_is_closed(this_object)));
                    /* VERIFIED */
            execute_routine_declaration(
                    declaration_routine_declaration(the_declaration),
                    instance_routine_instance(the_instance), the_context,
                    the_jumper, this_object);
            break;
        case NK_TAGALONG:
            assert(!(tagalong_key_is_instantiated(instance_tagalong_instance(
                             the_instance)))); /* VERIFIED */
            assert((this_object == NULL) || !(object_is_closed(this_object)));
                    /* VERIFIED */
            execute_tagalong_declaration(
                    declaration_tagalong_declaration(the_declaration),
                    instance_tagalong_instance(the_instance), the_context,
                    the_jumper, this_object);
            break;
        case NK_LEPTON_KEY:
            assert(!(lepton_key_instance_is_instantiated(
                             instance_lepton_key_instance(the_instance))));
                    /* VERIFIED */
            execute_lepton_key_declaration(
                    declaration_lepton_key_declaration(the_declaration),
                    instance_lepton_key_instance(the_instance), the_context,
                    the_jumper, this_object);
            break;
        case NK_QUARK:
            break;
        case NK_LOCK:
            assert(!(lock_instance_is_instantiated(instance_lock_instance(
                             the_instance)))); /* VERIFIED */
            assert((this_object == NULL) || !(object_is_closed(this_object)));
                    /* VERIFIED */
            execute_lock_declaration(
                    declaration_lock_declaration(the_declaration),
                    instance_lock_instance(the_instance), the_context,
                    the_jumper, this_object);
            break;
        default:
            assert(FALSE);
      }

    if (!(jumper_flowing_forward(the_jumper)))
        return;

    set_instance_instantiated(the_instance);
  }

extern void execute_variable_declaration(variable_declaration *declaration,
        variable_instance *instance, context *the_context, jumper *the_jumper,
        object *this_object)
  {
    type *new_type;
    lock_chain *the_lock_chain;
    expression *initializer;

    assert(declaration != NULL);
    assert(instance != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(!(variable_instance_is_instantiated(instance))); /* VERIFIED */
    assert((this_object == NULL) || !(object_is_closed(this_object)));
            /* VERIFIED */

    new_type = evaluate_type_expression(variable_declaration_type(declaration),
                                        the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(new_type == NULL);
        return;
      }

    assert(new_type != NULL);

    /*
     * If the variable has automatic de-allocation, if it has been
     * de-allocated, its containing scope has exited, in which case the
     * declaration can't be executing now.
     *
     * If the variable has non-automatic de-allocation, it can only be
     * de-allocated when all references are gone or when it is explicitly
     * de-allocated by a call to delete().  But if declaration is executing
     * now, all references can't be gone.  So the only way it could have been
     * de-allocated at this point is if a call was already made to delete() on
     * this variable instance.  But since the variable instance isn't yet
     * marked as instantiated, the call to delete() would have to have been on
     * an invalid value, which would have caused an exception on parameter type
     * checking, so the delete() would not have succeeded.  Therefor, at this
     * point the variable instance can't possibly have been de-allocated.
     */
    assert(!(variable_instance_scope_exited(instance))); /* VERIFIED */
    set_variable_instance_type(instance, new_type, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(new_type, the_jumper);
        return;
      }

    assert((this_object == NULL) || !(object_is_closed(this_object)));
            /* VERIFIED */
    the_lock_chain = declaration_lock_chain(this_object,
            variable_declaration_single_lock(declaration), the_context,
            the_jumper, "variable", EXCEPTION_TAG(variable_lock_not_lock));
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_lock_chain == NULL);
        type_remove_reference(new_type, the_jumper);
        return;
      }

    assert(!(variable_instance_scope_exited(instance))); /* VERIFIED */
    set_variable_instance_lock_chain(instance, the_lock_chain, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (the_lock_chain != NULL)
            lock_chain_remove_reference(the_lock_chain, the_jumper);
        type_remove_reference(new_type, the_jumper);
        return;
      }

    initializer = variable_declaration_initializer(declaration);
    if (initializer != NULL)
      {
        value *raw_value;
        value *fixed_value;

        raw_value = evaluate_expression(initializer, the_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(raw_value == NULL);
            if (the_lock_chain != NULL)
                lock_chain_remove_reference(the_lock_chain, the_jumper);
            type_remove_reference(new_type, the_jumper);
            return;
          }

        assert(raw_value != NULL);

        check_type_validity(new_type,
                get_type_expression_location(variable_declaration_type(
                        declaration)), the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(raw_value, the_jumper);
            if (the_lock_chain != NULL)
                lock_chain_remove_reference(the_lock_chain, the_jumper);
            type_remove_reference(new_type, the_jumper);
            return;
          }

        assert(type_is_valid(new_type)); /* VERIFIED */
        fixed_value = check_value_type_and_possibly_force(raw_value, new_type,
                new_type,
                variable_declaration_force_type_in_initialization(declaration),
                EXCEPTION_TAG(initialize_variable_mismatch),
                EXCEPTION_TAG(initialize_variable_match_indeterminate),
                "initialization", get_expression_location(initializer),
                the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(fixed_value == NULL);
            value_remove_reference(raw_value, the_jumper);
            if (the_lock_chain != NULL)
                lock_chain_remove_reference(the_lock_chain, the_jumper);
            type_remove_reference(new_type, the_jumper);
            return;
          }
        assert(fixed_value != NULL);

        value_remove_reference(raw_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(fixed_value, the_jumper);
            if (the_lock_chain != NULL)
                lock_chain_remove_reference(the_lock_chain, the_jumper);
            type_remove_reference(new_type, the_jumper);
            return;
          }

        if (the_lock_chain != NULL)
          {
            lock_chain_grab(the_lock_chain,
                            get_expression_location(initializer), the_jumper);
          }
        assert(!(variable_instance_scope_exited(instance))); /* VERIFIED */
        set_variable_instance_value(instance, fixed_value, the_jumper);
        if (the_lock_chain != NULL)
          {
            lock_chain_release(the_lock_chain,
                    get_expression_location(initializer), the_jumper);
          }
        value_remove_reference(fixed_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            if (the_lock_chain != NULL)
                lock_chain_remove_reference(the_lock_chain, the_jumper);
            type_remove_reference(new_type, the_jumper);
            return;
          }
      }

    if (the_lock_chain != NULL)
        lock_chain_remove_reference(the_lock_chain, the_jumper);
    type_remove_reference(new_type, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return;
  }

extern void execute_routine_declaration(routine_declaration *declaration,
        routine_instance *instance, context *the_context, jumper *the_jumper,
        object *this_object)
  {
    type_expression *return_type_expression;
    size_t expected_return_type_internal_references;
    type *return_type;
    formal_arguments *formals;
    size_t argument_count;
    size_t argument_num;
    lock_chain *the_lock_chain;

    assert(declaration != NULL);
    assert(instance != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(!(routine_instance_is_instantiated(instance))); /* VERIFIED */
    assert((this_object == NULL) || !(object_is_closed(this_object)));
            /* VERIFIED */

    assert((routine_declaration_body(declaration) != NULL) ||
           (routine_declaration_native_handler(declaration) != NULL));

    /*
     * See the comment in execute_variable_declaration() for why this routine
     * couldn't have been de-allocated at this point since it hasn't yet been
     * marked as instantiated.
     */
    assert(!(routine_instance_scope_exited(instance))); /* VERIFIED */
    routine_instance_set_up_static_context(instance, the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return;

    return_type_expression =
            routine_declaration_static_return_type(declaration);

    if (return_type_expression == NULL)
      {
        if (routine_declaration_is_class(declaration))
          {
            expected_return_type_internal_references = 1;
            return_type = get_class_type(instance);
            if (return_type == NULL)
              {
                jumper_do_abort(the_jumper);
                return;
              }
          }
        else
          {
            expected_return_type_internal_references = 0;
            return_type = get_nothing_type();
            if (return_type == NULL)
              {
                jumper_do_abort(the_jumper);
                return;
              }
            type_add_reference(return_type);
          }
      }
    else
      {
        expected_return_type_internal_references = 0;
        return_type = evaluate_type_expression(return_type_expression,
                                               the_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(return_type == NULL);
            return;
          }

        assert(return_type != NULL);
      }

    assert(!(routine_instance_scope_exited(instance))); /* VERIFIED */
    routine_instance_set_return_type(instance, return_type,
            expected_return_type_internal_references, the_jumper);
    type_remove_reference(return_type, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return;

    formals = routine_declaration_formals(declaration);
    assert(formals != NULL);

    argument_count = formal_arguments_argument_count(formals);

    for (argument_num = 0; argument_num < argument_count; ++argument_num)
      {
        variable_declaration *this_formal;
        type *argument_type;

        this_formal = formal_arguments_formal_by_number(formals, argument_num);
        assert(this_formal != NULL);

        argument_type = evaluate_type_expression(
                variable_declaration_type(this_formal), the_context,
                the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(argument_type == NULL);
            return;
          }

        assert(argument_type != NULL);

        assert(!(routine_instance_scope_exited(instance))); /* VERIFIED */
        routine_instance_set_argument_type(instance, argument_type,
                                           argument_num, the_jumper);
        type_remove_reference(argument_type, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return;
      }

    assert((this_object == NULL) || !(object_is_closed(this_object)));
            /* VERIFIED */
    the_lock_chain = declaration_lock_chain(this_object,
            routine_declaration_single_lock(declaration), the_context,
            the_jumper, "routine", EXCEPTION_TAG(routine_lock_not_lock));
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_lock_chain == NULL);
        return;
      }

    assert(!(routine_instance_scope_exited(instance))); /* VERIFIED */
    set_routine_instance_lock_chain(instance, the_lock_chain, the_jumper);
    if (the_lock_chain != NULL)
        lock_chain_remove_reference(the_lock_chain, the_jumper);
  }

extern void execute_tagalong_declaration(tagalong_declaration *declaration,
        tagalong_key *instance, context *the_context, jumper *the_jumper,
        object *this_object)
  {
    type *new_type;
    type *on_type;
    lock_chain *the_lock_chain;
    expression *initializer;

    assert(declaration != NULL);
    assert(instance != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(!(tagalong_key_is_instantiated(instance))); /* VERIFIED */
    assert((this_object == NULL) || !(object_is_closed(this_object)));
            /* VERIFIED */

    new_type = evaluate_type_expression(tagalong_declaration_type(declaration),
                                        the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(new_type == NULL);
        return;
      }

    assert(new_type != NULL);

    /*
     * See the comment in execute_variable_declaration() for why this tagalong
     * key couldn't have been de-allocated at this point since it hasn't yet
     * been marked as instantiated.
     */
    assert(!(tagalong_key_scope_exited(instance))); /* VERIFIED */
    set_tagalong_key_type(instance, new_type, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(new_type, the_jumper);
        return;
      }

    on_type = evaluate_type_expression(tagalong_declaration_on(declaration),
                                       the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(on_type == NULL);
        type_remove_reference(new_type, the_jumper);
        return;
      }

    assert(on_type != NULL);

    assert(!(tagalong_key_scope_exited(instance))); /* VERIFIED */
    set_tagalong_key_on_type(instance, on_type, the_jumper);
    type_remove_reference(on_type, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(new_type, the_jumper);
        return;
      }

    assert((this_object == NULL) || !(object_is_closed(this_object)));
            /* VERIFIED */
    the_lock_chain = declaration_lock_chain(this_object,
            tagalong_declaration_single_lock(declaration), the_context,
            the_jumper, "tagalong", EXCEPTION_TAG(tagalong_lock_not_lock));
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_lock_chain == NULL);
        type_remove_reference(new_type, the_jumper);
        return;
      }

    assert(!(tagalong_key_scope_exited(instance))); /* VERIFIED */
    set_tagalong_key_lock_chain(instance, the_lock_chain, the_jumper);
    if (the_lock_chain != NULL)
        lock_chain_remove_reference(the_lock_chain, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        type_remove_reference(new_type, the_jumper);
        return;
      }

    initializer = tagalong_declaration_initializer(declaration);
    if (initializer != NULL)
      {
        value *raw_value;
        value *fixed_value;

        raw_value = evaluate_expression(initializer, the_context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(raw_value == NULL);
            type_remove_reference(new_type, the_jumper);
            return;
          }

        assert(raw_value != NULL);

        check_type_validity(new_type,
                get_type_expression_location(tagalong_declaration_type(
                        declaration)), the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(raw_value, the_jumper);
            type_remove_reference(new_type, the_jumper);
            return;
          }

        assert(type_is_valid(new_type)); /* VERIFIED */
        fixed_value = check_value_type_and_possibly_force(raw_value, new_type,
                new_type,
                tagalong_declaration_force_type_in_initialization(declaration),
                EXCEPTION_TAG(tagalong_default_mismatch),
                EXCEPTION_TAG(tagalong_default_match_indeterminate),
                "tagalong initialization",
                get_expression_location(initializer), the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(fixed_value == NULL);
            value_remove_reference(raw_value, the_jumper);
            type_remove_reference(new_type, the_jumper);
            return;
          }
        assert(fixed_value != NULL);

        value_remove_reference(raw_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(fixed_value, the_jumper);
            type_remove_reference(new_type, the_jumper);
            return;
          }

        assert(!(tagalong_key_scope_exited(instance))); /* VERIFIED */
        set_tagalong_key_default_value(instance, fixed_value, the_jumper);
        value_remove_reference(fixed_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            type_remove_reference(new_type, the_jumper);
            return;
          }
      }

    type_remove_reference(new_type, the_jumper);
  }

extern void execute_lepton_key_declaration(lepton_key_declaration *declaration,
        lepton_key_instance *instance, context *the_context,
        jumper *the_jumper, object *this_object)
  {
    size_t field_count;
    size_t field_num;

    assert(declaration != NULL);
    assert(instance != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(!(lepton_key_instance_is_instantiated(instance))); /* VERIFIED */

    field_count = lepton_key_field_count(declaration);
    for (field_num = 0; field_num < field_count; ++field_num)
      {
        type *field_type;

        field_type = evaluate_type_expression(
                lepton_key_field_type(declaration, field_num), the_context,
                the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(field_type == NULL);
            return;
          }

        assert(field_type != NULL);

        /*
         * See the comment in execute_variable_declaration() for why this
         * lepton key couldn't have been de-allocated at this point since it
         * hasn't yet been marked as instantiated.
         */
        assert(!(lepton_key_instance_scope_exited(instance))); /* VERIFIED */

        set_lepton_key_instance_field_type(instance, field_type, field_num,
                                           the_jumper);
        type_remove_reference(field_type, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return;
      }
  }

extern void execute_lock_declaration(lock_declaration *declaration,
        lock_instance *instance, context *the_context, jumper *the_jumper,
        object *this_object)
  {
    lock_chain *the_lock_chain;

    assert(declaration != NULL);
    assert(instance != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(!(lock_instance_is_instantiated(instance))); /* VERIFIED */
    assert((this_object == NULL) || !(object_is_closed(this_object)));
            /* VERIFIED */

    the_lock_chain = declaration_lock_chain(this_object,
            lock_declaration_single_lock(declaration), the_context, the_jumper,
            "lock", EXCEPTION_TAG(lock_single_lock_not_lock));
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(the_lock_chain == NULL);
        return;
      }

    /*
     * See the comment in execute_variable_declaration() for why this lock
     * couldn't have been de-allocated at this point since it hasn't yet been
     * marked as instantiated.
     */
    assert(!(lock_instance_scope_exited(instance))); /* VERIFIED */

    set_lock_instance_lock_chain(instance, the_lock_chain, the_jumper);
    if (the_lock_chain != NULL)
        lock_chain_remove_reference(the_lock_chain, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return;
  }

extern value *read_variable_value(variable_instance *instance,
        lock_instance_aa *delayed_unlocks, const source_location *location,
        jumper *the_jumper)
  {
    value *the_value;

    assert(instance != NULL);
    assert(the_jumper != NULL);

    if (!(variable_instance_is_instantiated(instance)))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(read_variable_uninstantiated),
                "%V was referenced before it was instantiated.",
                variable_instance_declaration(instance));
        return NULL;
      }

    if (variable_instance_scope_exited(instance))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(read_variable_deallocated),
                "%V was referenced after it had ceased to exist because its "
                "scope had ended.", variable_instance_declaration(instance));
        return NULL;
      }

    assert(variable_instance_is_instantiated(instance)); /* VERIFIED */
    assert(!(variable_instance_scope_exited(instance))); /* VERIFIED */
    do_variable_lock_grabbing(instance, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    assert(variable_instance_is_instantiated(instance)); /* VERIFIED */
    assert(!(variable_instance_scope_exited(instance))); /* VERIFIED */
    the_value = variable_instance_value(instance);

    assert(variable_instance_is_instantiated(instance)); /* VERIFIED */
    assert(!(variable_instance_scope_exited(instance))); /* VERIFIED */
    if (delayed_unlocks == NULL)
      {
        assert(variable_instance_is_instantiated(instance)); /* VERIFIED */
        assert(!(variable_instance_scope_exited(instance))); /* VERIFIED */
        do_variable_lock_releasing(instance, location, the_jumper);
      }
    else
      {
        lock_chain *follow;

        assert(variable_instance_is_instantiated(instance)); /* VERIFIED */
        assert(!(variable_instance_scope_exited(instance))); /* VERIFIED */

        follow = variable_instance_lock_chain(instance);

        while (follow != NULL)
          {
            lock_instance *this_instance;

            this_instance = lock_chain_head(follow);
            assert(this_instance != NULL);
            if (jumper_flowing_forward(the_jumper))
              {
                verdict the_verdict;
                the_verdict = lock_instance_aa_append(delayed_unlocks,
                                                      this_instance);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    lock_instance_release(this_instance, location, the_jumper);
                  }
              }
            else
              {
                lock_instance_release(this_instance, location, the_jumper);
              }
            follow = lock_chain_remainder(follow);
          }
      }
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (the_value != NULL)
            value_remove_reference(the_value, the_jumper);
        return NULL;
      }

    if (the_value == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(read_variable_undefined),
                "%V was referenced while its value was undefined.",
                variable_instance_declaration(instance));
        return NULL;
      }

    return the_value;
  }

extern verdict try_overloading(value *base_value, const char *routine_name,
        value **result, size_t argument_count, value **arguments,
        const char **argument_names, jumper *the_jumper,
        const source_location *location)
  {
    value *call_base;
    verdict the_verdict;

    assert(base_value != NULL);
    assert(routine_name != NULL);
    assert((argument_count == 0) || (arguments != NULL));
    assert(the_jumper != NULL);

    if (result != NULL)
        *result = NULL;

    call_base = find_overload_operator(base_value, routine_name, the_jumper,
                                       location);
    if (call_base == NULL)
      {
        if (jumper_flowing_forward(the_jumper))
            return MISSION_FAILED;
        else
            return MISSION_ACCOMPLISHED;
      }

    if ((get_value_kind(call_base) != VK_ROUTINE) &&
        (get_value_kind(call_base) != VK_ROUTINE_CHAIN))
      {
        value_remove_reference(call_base, the_jumper);
        return MISSION_FAILED;
      }

    the_verdict = try_overloading_from_call_base(call_base, result,
            argument_count, arguments, argument_names, the_jumper, location);
    value_remove_reference(call_base, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return MISSION_ACCOMPLISHED;
    return the_verdict;
  }

extern verdict try_overloading_from_call_base(value *call_base, value **result,
        size_t argument_count, value **arguments, const char **argument_names,
        jumper *the_jumper, const source_location *location)
  {
    semi_labeled_value_list *pre_order_actuals;
    size_t argument_num;
    value *new_base;
    value *call_result;

    if (result != NULL)
        *result = NULL;

    pre_order_actuals = create_semi_labeled_value_list();
    if (pre_order_actuals == NULL)
      {
        jumper_do_abort(the_jumper);
        return MISSION_ACCOMPLISHED;
      }

    for (argument_num = 0; argument_num < argument_count; ++argument_num)
      {
        verdict the_verdict;

        assert(arguments != NULL);
        assert(arguments[argument_num] != NULL);
        the_verdict = append_value_to_semi_labeled_value_list(
                pre_order_actuals,
                ((argument_names == NULL) ? NULL :
                 argument_names[argument_num]), arguments[argument_num]);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
            return MISSION_ACCOMPLISHED;
          }
      }

    new_base = NULL;

    switch (get_value_kind(call_base))
      {
        case VK_ROUTINE:
          {
            if (routine_instance_fits_actuals(routine_value_data(call_base),
                        pre_order_actuals, location, the_jumper))
              {
                break;
              }

            delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
            return MISSION_FAILED;
          }
        case VK_ROUTINE_CHAIN:
          {
            routine_instance_chain *instance_chain;
            routine_instance *the_routine_instance;

            instance_chain = routine_chain_value_data(call_base);
            assert(instance_chain != NULL);

            the_routine_instance = resolve_overloading(instance_chain,
                    pre_order_actuals, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
                return MISSION_ACCOMPLISHED;
              }

            if (the_routine_instance == NULL)
              {
                delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
                return MISSION_FAILED;
              }

            assert(routine_instance_is_instantiated(the_routine_instance));
                    /* VERIFIED */
            assert(!(routine_instance_scope_exited(the_routine_instance)));
                    /* VERIFIED */

            new_base = create_routine_value(the_routine_instance);
            if (new_base == NULL)
              {
                jumper_do_abort(the_jumper);
                delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
                return MISSION_ACCOMPLISHED;
              }

            call_base = new_base;

            break;
          }
        default:
          {
            delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
            return MISSION_FAILED;
          }
      }

    call_result = execute_call_from_values(call_base, pre_order_actuals,
            (result != NULL), the_jumper, location);
    delete_semi_labeled_value_list(pre_order_actuals, the_jumper);
    if (new_base != NULL)
        value_remove_reference(new_base, the_jumper);

    if (result == NULL)
      {
        assert(call_result == NULL);
      }
    else
      {
        if ((!(jumper_flowing_forward(the_jumper))) && (call_result != NULL))
          {
            value_remove_reference(call_result, the_jumper);
            *result = NULL;
          }
        else
          {
            *result = call_result;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

extern void find_overload_type(value *call_base, size_t argument_count,
        parameter_pattern_kind *parameter_pattern_kinds,
        const char **parameter_names, value **exact_parameters,
        type **parameter_lower_types, type **parameter_upper_types,
        boolean is_write, type *write_lower_requirement,
        type *write_upper_requirement, type **result_lower,
        type **result_upper, boolean *always_hits, boolean *never_hits,
        const source_location *location, jumper *the_jumper)
  {
    find_overload_type_with_possible_map_result(call_base, argument_count,
            parameter_pattern_kinds, parameter_names, exact_parameters,
            parameter_lower_types, parameter_upper_types, is_write,
            write_lower_requirement, write_upper_requirement, argument_count,
            result_lower, result_upper, always_hits, never_hits, location,
            the_jumper);
  }

extern void find_overload_type_with_possible_map_result(value *call_base,
        size_t argument_count, parameter_pattern_kind *parameter_pattern_kinds,
        const char **parameter_names, value **exact_parameters,
        type **parameter_lower_types, type **parameter_upper_types,
        boolean is_write, type *write_lower_requirement,
        type *write_upper_requirement, size_t map_key_argument_number,
        type **result_lower, type **result_upper, boolean *always_hits,
        boolean *never_hits, const source_location *location,
        jumper *the_jumper)
  {
    type *result;

    assert(call_base != NULL);
    assert(result_lower != NULL);
    assert(result_upper != NULL);
    assert(always_hits != NULL);
    assert(the_jumper != NULL);

    *result_lower = NULL;
    *result_upper = NULL;
    *always_hits = FALSE;
    if (never_hits != NULL)
        *never_hits = TRUE;

    if (is_write)
      {
        parameter_pattern_kinds[argument_count] = PPK_ANY;
        parameter_names[argument_count] = NULL;
        exact_parameters[argument_count] = NULL;
        parameter_lower_types[argument_count] = NULL;
        parameter_upper_types[argument_count] = NULL;
        if (map_key_argument_number == argument_count)
            ++map_key_argument_number;
        ++argument_count;
      }

    result = get_nothing_type();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        assert(*result_lower == NULL);
        assert(*result_upper == NULL);
        return;
      }
    type_add_reference(result);
    assert(type_is_valid(result)); /* VERIFIED */

    switch (get_value_kind(call_base))
      {
        case VK_ROUTINE:
          {
            type **result_parameter_types;
            routine_instance *instance;
            boolean matches;
            boolean doubt;

            result_parameter_types = MALLOC_ARRAY(type *, argument_count + 1);
            if (result_parameter_types == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(result, the_jumper);
                assert(*result_lower == NULL);
                assert(*result_upper == NULL);
                return;
              }

            instance = routine_value_data(call_base);
            assert(instance != NULL);

            matches = routine_instance_fits_pattern(instance, argument_count,
                    parameter_pattern_kinds, parameter_names, exact_parameters,
                    parameter_lower_types, parameter_upper_types,
                    result_parameter_types, &doubt, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                free(result_parameter_types);
                type_remove_reference(result, the_jumper);
                assert(*result_lower == NULL);
                assert(*result_upper == NULL);
                return;
              }

            if ((matches || doubt) && is_write)
              {
                type *lower_return_match;
                type *upper_return_match;
                type *routine_return_type;
                boolean return_doubt;
                boolean return_matches;

                if (write_lower_requirement != NULL)
                  {
                    assert(write_lower_requirement != NULL);
                    assert(write_upper_requirement != NULL);

                    lower_return_match = write_lower_requirement;
                    upper_return_match = write_upper_requirement;
                  }
                else
                  {
                    assert(write_lower_requirement == NULL);
                    assert(write_upper_requirement == NULL);

                    lower_return_match = get_nothing_type();
                    if (lower_return_match == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        free(result_parameter_types);
                        type_remove_reference(result, the_jumper);
                        assert(*result_lower == NULL);
                        assert(*result_upper == NULL);
                        return;
                      }

                    upper_return_match = lower_return_match;
                  }

                assert(lower_return_match != NULL);
                assert(upper_return_match != NULL);

                assert(routine_instance_is_instantiated(instance));
                        /* VERIFICATION NEEDED */
                assert(!(routine_instance_scope_exited(instance)));
                        /* VERIFICATION NEEDED */
                routine_return_type = routine_instance_valid_return_type(
                        instance, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    assert(routine_return_type == NULL);
                    free(result_parameter_types);
                    type_remove_reference(result, the_jumper);
                    assert(*result_lower == NULL);
                    assert(*result_upper == NULL);
                    return;
                  }

                assert(type_is_valid(routine_return_type)); /* VERIFIED */
                assert(type_is_valid(upper_return_match));
                        /* VERIFICATION NEEDED */
                return_matches = type_is_subset(routine_return_type,
                        upper_return_match, &return_doubt, NULL, location,
                        the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    free(result_parameter_types);
                    type_remove_reference(result, the_jumper);
                    assert(*result_lower == NULL);
                    assert(*result_upper == NULL);
                    return;
                  }

                if ((!return_matches) && (!return_doubt))
                  {
                    matches = FALSE;
                    doubt = FALSE;
                  }
                else if ((!return_matches) || return_doubt)
                  {
                    doubt = TRUE;
                  }

                assert(routine_instance_is_instantiated(instance));
                        /* VERIFICATION NEEDED */
                assert(!(routine_instance_scope_exited(instance)));
                        /* VERIFICATION NEEDED */
                assert(type_is_valid(routine_return_type));
                        /* VERIFICATION NEEDED */
                assert(type_is_valid(lower_return_match));
                        /* VERIFICATION NEEDED */
                return_matches = type_is_subset(routine_return_type,
                        lower_return_match, &return_doubt, NULL, location,
                        the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    free(result_parameter_types);
                    type_remove_reference(result, the_jumper);
                    assert(*result_lower == NULL);
                    assert(*result_upper == NULL);
                    return;
                  }

                if (matches && ((!return_matches) || return_doubt))
                    doubt = TRUE;
              }

            if (matches || doubt)
              {
                type *local_type;
                type *new_result;

                if (never_hits != NULL)
                    *never_hits = FALSE;

                if (is_write)
                  {
                    local_type = result_parameter_types[argument_count - 1];
                    assert(local_type != NULL);
                    assert(type_is_valid(local_type));
                            /* VERIFICATION NEEDED */

                    if (map_key_argument_number < argument_count)
                      {
                        type *key_type;

                        key_type = result_parameter_types[
                                map_key_argument_number];
                        assert(key_type != NULL);

                        assert(type_is_valid(key_type));
                                /* VERIFICATION NEEDED */
                        assert(type_is_valid(local_type)); /* VERIFIED */
                        local_type = get_map_type(key_type, local_type);
                        if (local_type == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            free(result_parameter_types);
                            type_remove_reference(result, the_jumper);
                            assert(*result_lower == NULL);
                            assert(*result_upper == NULL);
                            return;
                          }
                        assert(type_is_valid(local_type)); /* VERIFIED */
                      }
                    else
                      {
                        type_add_reference(local_type);
                      }
                    assert(type_is_valid(local_type)); /* VERIFIED */
                  }
                else
                  {
                    assert(routine_instance_is_instantiated(instance));
                            /* VERIFICATION NEEDED */
                    assert(!(routine_instance_scope_exited(instance)));
                            /* VERIFICATION NEEDED */
                    local_type = routine_instance_valid_return_type(instance,
                            location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        assert(local_type == NULL);
                        free(result_parameter_types);
                        type_remove_reference(result, the_jumper);
                        assert(*result_lower == NULL);
                        assert(*result_upper == NULL);
                        return;
                      }

                    assert(local_type != NULL);
                    type_add_reference(local_type);
                    assert(type_is_valid(local_type)); /* VERIFIED */
                  }

                assert(result != NULL); /* VERIFIED */
                assert(type_is_valid(result)); /* VERIFIED */
                assert(type_is_valid(local_type)); /* VERIFIED */
                new_result = get_union_type(result, local_type);
                if (new_result == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    type_remove_reference(local_type, the_jumper);
                    free(result_parameter_types);
                    type_remove_reference(result, the_jumper);
                    assert(*result_lower == NULL);
                    assert(*result_upper == NULL);
                    return;
                  }
                type_remove_reference(local_type, the_jumper);
                type_remove_reference(result, the_jumper);
                result = new_result;
                assert(result != NULL); /* VERIFIED */
                assert(type_is_valid(result)); /* VERIFIED */
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    free(result_parameter_types);
                    type_remove_reference(result, the_jumper);
                    assert(*result_lower == NULL);
                    assert(*result_upper == NULL);
                    return;
                  }
                if (!doubt)
                    *always_hits = TRUE;
              }

            free(result_parameter_types);

            assert(result != NULL); /* VERIFIED */
            assert(type_is_valid(result)); /* VERIFIED */
            break;
          }
        case VK_ROUTINE_CHAIN:
          {
            routine_instance_chain *chain;

            chain = routine_chain_value_data(call_base);
            assert(chain != NULL);

            assert(result != NULL); /* VERIFIED */
            assert(type_is_valid(result)); /* VERIFIED */

            while (chain != NULL)
              {
                type **result_parameter_types;
                routine_instance *instance;
                boolean matches;
                boolean doubt;

                result_parameter_types =
                        MALLOC_ARRAY(type *, argument_count + 1);
                if (result_parameter_types == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    type_remove_reference(result, the_jumper);
                    assert(*result_lower == NULL);
                    assert(*result_upper == NULL);
                    return;
                  }

                instance = routine_instance_chain_instance(chain);
                assert(instance != NULL);

                chain = routine_instance_chain_next(chain);

                matches = routine_instance_fits_pattern(instance,
                        argument_count, parameter_pattern_kinds,
                        parameter_names, exact_parameters,
                        parameter_lower_types, parameter_upper_types,
                        result_parameter_types, &doubt, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    free(result_parameter_types);
                    type_remove_reference(result, the_jumper);
                    assert(*result_lower == NULL);
                    assert(*result_upper == NULL);
                    return;
                  }

                if ((matches || doubt) && is_write)
                  {
                    type *lower_return_match;
                    type *upper_return_match;
                    type *routine_return_type;
                    boolean return_doubt;
                    boolean return_matches;

                    if (write_lower_requirement != NULL)
                      {
                        assert(write_lower_requirement != NULL);
                        assert(write_upper_requirement != NULL);

                        lower_return_match = write_lower_requirement;
                        upper_return_match = write_upper_requirement;
                      }
                    else
                      {
                        assert(write_lower_requirement == NULL);
                        assert(write_upper_requirement == NULL);

                        lower_return_match = get_nothing_type();
                        if (lower_return_match == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            free(result_parameter_types);
                            type_remove_reference(result, the_jumper);
                            assert(*result_lower == NULL);
                            assert(*result_upper == NULL);
                            return;
                          }

                        upper_return_match = lower_return_match;
                      }

                    assert(lower_return_match != NULL);
                    assert(upper_return_match != NULL);

                    assert(routine_instance_is_instantiated(instance));
                            /* VERIFICATION NEEDED */
                    assert(!(routine_instance_scope_exited(instance)));
                            /* VERIFICATION NEEDED */
                    routine_return_type = routine_instance_valid_return_type(
                            instance, location, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        assert(routine_return_type == NULL);
                        free(result_parameter_types);
                        type_remove_reference(result, the_jumper);
                        assert(*result_lower == NULL);
                        assert(*result_upper == NULL);
                        return;
                      }

                    assert(type_is_valid(routine_return_type)); /* VERIFIED */
                    assert(type_is_valid(upper_return_match));
                            /* VERIFICATION NEEDED */
                    return_matches = type_is_subset(routine_return_type,
                            upper_return_match, &return_doubt, NULL, location,
                            the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        free(result_parameter_types);
                        type_remove_reference(result, the_jumper);
                        assert(*result_lower == NULL);
                        assert(*result_upper == NULL);
                        return;
                      }

                    if ((!return_matches) && (!return_doubt))
                      {
                        matches = FALSE;
                        doubt = FALSE;
                      }
                    else if ((!return_matches) || return_doubt)
                      {
                        doubt = TRUE;
                      }

                    assert(routine_instance_is_instantiated(instance));
                            /* VERIFICATION NEEDED */
                    assert(!(routine_instance_scope_exited(instance)));
                            /* VERIFICATION NEEDED */
                    assert(type_is_valid(routine_return_type));
                            /* VERIFICATION NEEDED */
                    assert(type_is_valid(lower_return_match));
                            /* VERIFICATION NEEDED */
                    return_matches = type_is_subset(routine_return_type,
                            lower_return_match, &return_doubt, NULL, location,
                            the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        free(result_parameter_types);
                        type_remove_reference(result, the_jumper);
                        assert(*result_lower == NULL);
                        assert(*result_upper == NULL);
                        return;
                      }

                    if (matches && ((!return_matches) || return_doubt))
                        doubt = TRUE;
                  }

                if (matches || doubt)
                  {
                    type *local_type;
                    type *new_result;

                    if (never_hits != NULL)
                        *never_hits = FALSE;

                    if (is_write)
                      {
                        local_type =
                                result_parameter_types[argument_count - 1];
                        assert(local_type != NULL);
                        assert(type_is_valid(local_type));
                                /* VERIFICATION NEEDED */

                        if (map_key_argument_number < argument_count)
                          {
                            type *key_type;

                            key_type = result_parameter_types[
                                    map_key_argument_number];
                            assert(key_type != NULL);

                            assert(type_is_valid(key_type));
                                    /* VERIFICATION NEEDED */
                            assert(type_is_valid(local_type)); /* VERIFIED */
                            local_type = get_map_type(key_type, local_type);
                            if (local_type == NULL)
                              {
                                jumper_do_abort(the_jumper);
                                free(result_parameter_types);
                                type_remove_reference(result, the_jumper);
                                assert(*result_lower == NULL);
                                assert(*result_upper == NULL);
                                return;
                              }
                            assert(type_is_valid(local_type)); /* VERIFIED */
                          }
                        else
                          {
                            type_add_reference(local_type);
                          }
                        assert(type_is_valid(local_type)); /* VERIFIED */
                      }
                    else
                      {
                        assert(routine_instance_is_instantiated(instance));
                                /* VERIFICATION NEEDED */
                        assert(!(routine_instance_scope_exited(instance)));
                                /* VERIFICATION NEEDED */
                        local_type = routine_instance_valid_return_type(
                                instance, location, the_jumper);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            assert(local_type == NULL);
                            free(result_parameter_types);
                            type_remove_reference(result, the_jumper);
                            assert(*result_lower == NULL);
                            assert(*result_upper == NULL);
                            return;
                          }

                        assert(local_type != NULL);
                        type_add_reference(local_type);
                        assert(type_is_valid(local_type));
                                /* VERIFICATION NEEDED */
                      }

                    assert(result != NULL); /* VERIFIED */
                    assert(type_is_valid(result)); /* VERIFIED */
                    assert(type_is_valid(local_type)); /* VERIFIED */
                    new_result = get_union_type(result, local_type);
                    if (new_result == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        type_remove_reference(local_type, the_jumper);
                        free(result_parameter_types);
                        type_remove_reference(result, the_jumper);
                        assert(*result_lower == NULL);
                        assert(*result_upper == NULL);
                        return;
                      }
                    type_remove_reference(local_type, the_jumper);
                    type_remove_reference(result, the_jumper);
                    result = new_result;
                    assert(result != NULL); /* VERIFIED */
                    assert(type_is_valid(result)); /* VERIFIED */
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        free(result_parameter_types);
                        type_remove_reference(result, the_jumper);
                        assert(*result_lower == NULL);
                        assert(*result_upper == NULL);
                        return;
                      }
                    if (!doubt)
                      {
                        free(result_parameter_types);
                        *always_hits = TRUE;
                        *result_lower = result;
                        type_add_reference(result);
                        *result_upper = result;
                        assert(type_is_valid(*result_lower)); /* VERIFIED */
                        assert(type_is_valid(*result_upper)); /* VERIFIED */
                        return;
                      }
                  }

                free(result_parameter_types);
                assert(result != NULL); /* VERIFIED */
                assert(type_is_valid(result)); /* VERIFIED */
              }

            assert(result != NULL); /* VERIFIED */
            assert(type_is_valid(result)); /* VERIFIED */
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    assert(result != NULL); /* VERIFIED */
    assert(type_is_valid(result)); /* VERIFIED */
    *result_lower = result;
    type_add_reference(result);
    *result_upper = result;
    assert(type_is_valid(*result_lower)); /* VERIFIED */
    assert(type_is_valid(*result_upper)); /* VERIFIED */
  }

extern void find_instance_from_use_statement(statement *use_statement,
        size_t used_for_num, context *the_context, boolean must_find,
        instance **result_instance, routine_instance_chain **result_chain,
        jump_target **result_jump_target, const source_location *location,
        jumper *the_jumper)
  {
    use_instance *the_use_instance;
    declaration *the_declaration;
    routine_declaration_chain *declaration_chain;
    statement *next_use;
    size_t next_used_for_number;
    routine_instance_chain *the_instance_chain;
    statement *label_statement;

    assert(use_statement != NULL);
    assert(the_context != NULL);
    assert(result_instance != NULL);
    assert(result_chain != NULL);
    assert(result_jump_target != NULL);
    assert(the_jumper != NULL);

    *result_instance = NULL;
    *result_chain = NULL;
    *result_jump_target = NULL;

    the_use_instance = find_use_instance(the_context, use_statement);
    assert(the_use_instance != NULL);

    the_declaration =
            use_statement_used_for_declaration(use_statement, used_for_num);
    declaration_chain =
            use_statement_used_for_chain(use_statement, used_for_num);
    next_use = use_statement_used_for_next_use(use_statement, used_for_num);
    next_used_for_number = use_statement_used_for_next_used_for_number(
            use_statement, used_for_num);

    if (declaration_chain != NULL)
      {
        the_instance_chain =
                routine_declaration_chain_to_routine_instance_chain(
                        declaration_chain, the_context, the_jumper);
        if (the_instance_chain == NULL)
          {
            assert(!(jumper_flowing_forward(the_jumper)));
            return;
          }
        assert(jumper_flowing_forward(the_jumper));
      }
    else
      {
        the_instance_chain = NULL;
      }

    *result_instance = use_instance_instance(the_use_instance, used_for_num);
    if (*result_instance != NULL)
      {
        if (instance_kind(*result_instance) == NK_ROUTINE)
          {
            routine_instance *result_routine_instance;

            result_routine_instance =
                    instance_routine_instance(*result_instance);

            if ((the_declaration != NULL) &&
                (declaration_kind(the_declaration) == NK_ROUTINE))
              {
                instance *tail_instance;
                routine_instance *tail_routine_instance;

                assert(the_instance_chain == NULL);

                *result_instance = NULL;

                tail_instance = find_instance(the_context, the_declaration);
                assert(tail_instance != NULL);

                assert(instance_kind(tail_instance) == NK_ROUTINE);
                tail_routine_instance =
                        instance_routine_instance(tail_instance);
                assert(tail_routine_instance != NULL);

                the_instance_chain = create_routine_instance_chain(
                        tail_routine_instance, NULL);
                if (the_instance_chain == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    return;
                  }
              }

            if (next_use != NULL)
              {
                instance *test_instance;
                jump_target *test_jump_target;

                assert(the_instance_chain == NULL);

                find_instance_from_use_statement(next_use,
                        next_used_for_number, the_context, FALSE,
                        &test_instance, &the_instance_chain, &test_jump_target,
                        location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                    return;

                if ((test_instance != NULL) &&
                    (instance_kind(test_instance) == NK_ROUTINE))
                  {
                    routine_instance *tail_routine_instance;

                    assert(the_instance_chain == NULL);

                    tail_routine_instance =
                            instance_routine_instance(test_instance);
                    assert(tail_routine_instance != NULL);

                    the_instance_chain = create_routine_instance_chain(
                            tail_routine_instance, NULL);
                    if (the_instance_chain == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        return;
                      }
                  }
              }

            if (the_instance_chain != NULL)
              {
                *result_instance = NULL;

                *result_chain = create_routine_instance_chain(
                        result_routine_instance, the_instance_chain);
                if (*result_chain == NULL)
                    jumper_do_abort(the_jumper);
                routine_instance_chain_remove_reference(the_instance_chain,
                                                        the_jumper);
                if ((!(jumper_flowing_forward(the_jumper))) &&
                    (*result_chain != NULL))
                  {
                    routine_instance_chain_remove_reference(*result_chain,
                                                            the_jumper);
                    *result_chain = NULL;
                  }
                return;
              }
          }
        else
          {
            if (the_instance_chain != NULL)
              {
                routine_instance_chain_remove_reference(the_instance_chain,
                                                        the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                    *result_instance = NULL;
              }
          }

        return;
      }

    *result_chain = use_instance_chain(the_use_instance, used_for_num);
    if (*result_chain != NULL)
      {
        if ((the_declaration != NULL) &&
            (declaration_kind(the_declaration) == NK_ROUTINE))
          {
            instance *tail_instance;
            routine_instance *tail_routine_instance;

            assert(the_instance_chain == NULL);

            tail_instance = find_instance(the_context, the_declaration);
            assert(tail_instance != NULL);

            assert(instance_kind(tail_instance) == NK_ROUTINE);
            tail_routine_instance = instance_routine_instance(tail_instance);
            assert(tail_routine_instance != NULL);

            the_instance_chain =
                    create_routine_instance_chain(tail_routine_instance, NULL);
            if (the_instance_chain == NULL)
              {
                jumper_do_abort(the_jumper);
                return;
              }
          }

        if (next_use != NULL)
          {
            instance *test_instance;
            jump_target *test_jump_target;

            assert(the_instance_chain == NULL);

            find_instance_from_use_statement(next_use, next_used_for_number,
                    the_context, FALSE, &test_instance, &the_instance_chain,
                    &test_jump_target, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return;

            if ((test_instance != NULL) &&
                (instance_kind(test_instance) == NK_ROUTINE))
              {
                routine_instance *tail_routine_instance;

                assert(the_instance_chain == NULL);

                tail_routine_instance =
                        instance_routine_instance(test_instance);
                assert(tail_routine_instance != NULL);

                the_instance_chain = create_routine_instance_chain(
                        tail_routine_instance, NULL);
                if (the_instance_chain == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    return;
                  }
              }
          }

        if (the_instance_chain != NULL)
          {
            *result_chain = combine_routine_chains(the_instance_chain,
                                                   *result_chain, NULL);
            if (*result_chain == NULL)
                jumper_do_abort(the_jumper);
            routine_instance_chain_remove_reference(the_instance_chain,
                                                    the_jumper);
            if ((!(jumper_flowing_forward(the_jumper))) &&
                (*result_chain != NULL))
              {
                routine_instance_chain_remove_reference(*result_chain,
                                                        the_jumper);
                *result_chain = NULL;
              }
            return;
          }

        routine_instance_chain_add_reference(*result_chain);
        return;
      }

    if (the_declaration != NULL)
      {
        assert(the_instance_chain == NULL);
        *result_instance = find_instance(the_context, the_declaration);
        assert(*result_instance != NULL);
        return;
      }

    if (the_instance_chain != NULL)
      {
        *result_chain = the_instance_chain;
        return;
      }

    label_statement = use_statement_used_for_label_statement(use_statement,
                                                             used_for_num);
    if (label_statement != NULL)
      {
        *result_jump_target =
                find_label_instance(the_context, label_statement);
        assert(*result_jump_target != NULL);
        return;
      }

    if (next_use != NULL)
      {
        find_instance_from_use_statement(next_use, next_used_for_number,
                the_context, FALSE, result_instance, result_chain,
                result_jump_target, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return;
        if ((*result_instance != NULL) || (*result_chain != NULL) ||
            (*result_jump_target != NULL))
          {
            return;
          }
      }

    if (must_find)
      {
        assert(!(use_instance_is_instantiated(the_use_instance)));
        location_exception(the_jumper, location,
                EXCEPTION_TAG(use_not_executed),
                "Name `%s' was accessed before the `use' statement that could "
                "bind it was executed.",
                use_statement_used_for_name(use_statement, used_for_num));
      }
  }

extern void set_profiling_enable(boolean enabled)
  {
    profiling = enabled;
  }


static void require_numeric(const source_location *location,
        expression_kind kind, const char *operand_description,
        value *the_value, jumper *the_jumper)
  {
    switch (get_value_kind(the_value))
      {
        case VK_INTEGER:
        case VK_RATIONAL:
            return;
        default:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(bad_operands),
                    "Illegal %s to %s expression: %s.", operand_description,
                    expression_kind_name(kind), value_kind_name(the_value));
            return;
      }
  }

static void require_integer(const source_location *location,
        expression_kind kind, const char *operand_description,
        value *the_value, jumper *the_jumper)
  {
    switch (get_value_kind(the_value))
      {
        case VK_INTEGER:
            return;
        default:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(bad_operands),
                    "Illegal %s to %s expression: %s.", operand_description,
                    expression_kind_name(kind), value_kind_name(the_value));
            return;
      }
  }

static void require_boolean(const source_location *location,
        expression_kind kind, const char *operand_description,
        value *the_value, jumper *the_jumper)
  {
    switch (get_value_kind(the_value))
      {
        case VK_TRUE:
        case VK_FALSE:
            return;
        default:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(bad_operands),
                    "Illegal %s to %s expression: %s.", operand_description,
                    expression_kind_name(kind), value_kind_name(the_value));
            return;
      }
  }

static void require_numeric_or_pointer(const source_location *location,
        expression_kind kind, const char *operand_description,
        value *the_value, jumper *the_jumper)
  {
    switch (get_value_kind(the_value))
      {
        case VK_INTEGER:
        case VK_RATIONAL:
        case VK_SLOT_LOCATION:
            return;
        default:
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(bad_operands),
                    "Illegal %s to %s expression: %s.", operand_description,
                    expression_kind_name(kind), value_kind_name(the_value));
            return;
      }
  }

static const char *expression_kind_name(expression_kind kind)
  {
    switch (kind)
      {
        case EK_CONSTANT:
            return "constant";
        case EK_UNBOUND_NAME_REFERENCE:
            return "unbound name reference";
        case EK_VARIABLE_REFERENCE:
            return "variable reference";
        case EK_ROUTINE_REFERENCE:
            return "routine reference";
        case EK_LABEL_REFERENCE:
            return "label reference";
        case EK_TAGALONG_REFERENCE:
            return "tagalong key reference";
        case EK_LEPTON_KEY_REFERENCE:
            return "lepton key reference";
        case EK_QUARK_REFERENCE:
            return "quark reference";
        case EK_LOCK_REFERENCE:
            return "lock reference";
        case EK_USE_REFERENCE:
            return "use reference";
        case EK_LOOKUP:
            return "lookup";
        case EK_LEPTON:
            return "lepton";
        case EK_FIELD:
            return "field";
        case EK_POINTER_FIELD:
            return "pointer field";
        case EK_TAGALONG_FIELD:
            return "tagalong field";
        case EK_STATEMENT_BLOCK:
            return "statement block";
        case EK_DECLARATION:
            return "declaration";
        case EK_TYPE:
            return "type";
        case EK_MAP_LIST:
            return "map list";
        case EK_SEMI_LABELED_EXPRESSION_LIST:
            return "semi-labeled expression list";
        case EK_CALL:
            return "call";
        case EK_CONDITIONAL:
            return "conditional";
        case EK_DEREFERENCE:
            return "dereference";
        case EK_LOCATION_OF:
            return "location-of";
        case EK_NEGATE:
            return "negate";
        case EK_UNARY_PLUS:
            return "unary plus";
        case EK_BITWISE_NOT:
            return "bitwise not";
        case EK_LOGICAL_NOT:
            return "logical not";
        case EK_ADD:
            return "add";
        case EK_SUBTRACT:
            return "subtract";
        case EK_MULTIPLY:
            return "multiply";
        case EK_DIVIDE:
            return "divide";
        case EK_DIVIDE_FORCE:
            return "divide-force";
        case EK_REMAINDER:
            return "remainder";
        case EK_SHIFT_LEFT:
            return "shift left";
        case EK_SHIFT_RIGHT:
            return "shift right";
        case EK_LESS_THAN:
            return "less than";
        case EK_GREATER_THAN:
            return "greater than";
        case EK_LESS_THAN_OR_EQUAL:
            return "less than or equal to";
        case EK_GREATER_THAN_OR_EQUAL:
            return "greater than or equal to";
        case EK_EQUAL:
            return "equal";
        case EK_NOT_EQUAL:
            return "not equal";
        case EK_BITWISE_AND:
            return "bitwise and";
        case EK_BITWISE_OR:
            return "bitwise or";
        case EK_BITWISE_XOR:
            return "bitwise exclusive or";
        case EK_LOGICAL_AND:
            return "logical and";
        case EK_LOGICAL_OR:
            return "logical or";
        case EK_CONCATENATE:
            return "concatenate";
        case EK_ARGUMENTS:
            return "arguments";
        case EK_THIS:
            return "this";
        case EK_IN:
            return "`in'";
        case EK_FORCE:
            return "force";
        case EK_BREAK:
            return "break";
        case EK_CONTINUE:
            return "continue";
        case EK_COMPREHEND:
            return "comprehend";
        case EK_FORALL:
            return "forall";
        case EK_EXISTS:
            return "exists";
        default:
            assert(FALSE);
            return NULL;
      }
  }

static const char *value_kind_name(value *the_value)
  {
    assert(the_value != NULL);

    switch (get_value_kind(the_value))
      {
        case VK_TRUE:
        case VK_FALSE:
            return "boolean";
        case VK_INTEGER:
            return "integer";
        case VK_RATIONAL:
            return "rational";
        case VK_STRING:
            return "string";
        case VK_CHARACTER:
            return "character";
        case VK_REGULAR_EXPRESSION:
            return "regular expression";
        case VK_SEMI_LABELED_VALUE_LIST:
            return "semi-labeled value list";
        case VK_SEMI_LABELED_MULTI_SET:
            return "semi-labeled multi-set";
        case VK_MAP:
            return "map";
        case VK_QUARK:
            return "quark";
        case VK_LEPTON:
            return "lepton";
        case VK_LEPTON_KEY:
            return "lepton key";
        case VK_SLOT_LOCATION:
            return "slot location";
        case VK_NULL:
            return "null";
        case VK_JUMP_TARGET:
            return "jump target";
        case VK_ROUTINE:
            return "routine";
        case VK_ROUTINE_CHAIN:
            return "routine";
        case VK_TYPE:
            return "type";
        case VK_OBJECT:
            return "object";
        case VK_TAGALONG_KEY:
            return "tagalong key";
        case VK_LOCK:
            return "lock";
        default:
            assert(FALSE);
            return NULL;
      }
  }

static value *read_field_value(value *base_value, const char *field_name,
        const source_location *location, jumper *the_jumper)
  {
    assert(base_value != NULL);
    assert(field_name != NULL);
    assert(the_jumper != NULL);

    switch (get_value_kind(base_value))
      {
        case VK_SEMI_LABELED_VALUE_LIST:
        case VK_SEMI_LABELED_MULTI_SET:
        case VK_LEPTON:
          {
            value *result;

            result = value_get_field(field_name, base_value);

            if (result == NULL)
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(field_undefined),
                        "Field `%s' not defined when reading field value.",
                        field_name);
              }
            else
              {
                value_add_reference(result);
              }

            return result;
          }
        case VK_OBJECT:
          {
            object *the_object;
            size_t field_num;

            the_object = object_value_data(base_value);
            assert(the_object != NULL);

            object_check_validity(the_object, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return NULL;

            assert(!(object_is_closed(the_object))); /* VERIFIED */
            field_num = object_field_lookup(the_object, field_name);
            if (field_num >= object_field_count(the_object))
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(object_no_field),
                        "Object has no field named `%s'.", field_name);
                return NULL;
              }

            assert(!(object_is_closed(the_object))); /* VERIFIED */
            return object_field_read_value(the_object, field_num, location,
                                           the_jumper);
          }
        default:
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(field_read_bad_base),
                    "Attempted to read field `%s' from a value that does not "
                    "have fields.", field_name);
            return NULL;
          }
      }
  }

static value *read_from_basket_instance(basket_instance *the_basket_instance,
        lock_instance_aa *delayed_unlocks, const source_location *location,
        jumper *the_jumper)
  {
    assert(the_basket_instance != NULL);
    assert(the_jumper != NULL);

    switch (get_basket_instance_kind(the_basket_instance))
      {
        case BIK_SLOT:
          {
            return get_slot_contents(
                    slot_basket_instance_slot(the_basket_instance),
                    delayed_unlocks, location, the_jumper);
          }
        case BIK_LIST:
          {
            value *parent_value;
            size_t count;
            size_t number;

            parent_value = create_semi_labeled_value_list_value();
            if (parent_value == NULL)
              {
                jumper_do_abort(the_jumper);
                return NULL;
              }

            count = list_basket_instance_element_count(the_basket_instance);

            for (number = 0; number < count; ++number)
              {
                basket_instance *child_basket_instance;
                value *child_value;
                verdict the_verdict;

                child_basket_instance = list_basket_instance_child(
                        the_basket_instance, number);
                if (child_basket_instance == NULL)
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(basket_read_no_component),
                            "Attempted to read from a basket instance that "
                            "doesn't have a component basket for component "
                            "%lu.", (unsigned long)number);
                    value_remove_reference(parent_value, the_jumper);
                    return NULL;
                  }

                child_value = read_from_basket_instance(child_basket_instance,
                        delayed_unlocks, location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    assert(child_value == NULL);
                    value_remove_reference(parent_value, the_jumper);
                    return NULL;
                  }

                assert(child_value != NULL);

                the_verdict = add_field(parent_value,
                        list_basket_instance_label(the_basket_instance,
                                                   number), child_value);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    value_remove_reference(parent_value, the_jumper);
                    return NULL;
                  }
              }

            return parent_value;
          }
        default:
          {
            assert(FALSE);
            return NULL;
          }
      }
  }

static void write_to_basket_instance(basket_instance *the_basket_instance,
        value *new_value, boolean force_to_type,
        const source_location *location, jumper *the_jumper)
  {
    assert(new_value != NULL);
    assert(the_jumper != NULL);

    if (the_basket_instance == NULL)
        return;

    switch (get_basket_instance_kind(the_basket_instance))
      {
        case BIK_SLOT:
          {
            slot_location *the_slot;
            variable_instance *base_variable;
            type *slot_lower;
            type *slot_upper;
            value *forced_value;

            the_slot = slot_basket_instance_slot(the_basket_instance);
            assert(the_slot != NULL);

            slot_location_write_type_bounds(the_slot, &slot_lower, &slot_upper,
                    &base_variable, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(slot_lower == NULL);
                assert(slot_upper == NULL);
                return;
              }
            assert(slot_lower != NULL);
            assert(slot_upper != NULL);

            if (base_variable != NULL)
              {
                if (!(variable_instance_is_instantiated(base_variable)))
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(write_variable_uninstantiated),
                            "An attempt was made to modify %v before it was "
                            "instantiated.",
                            variable_instance_declaration(base_variable));
                    return;
                  }

                if (variable_instance_scope_exited(base_variable))
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(write_variable_deallocated),
                            "An attempt was made to modify %v after it had "
                            "ceased to exist because its scope had ended.",
                            variable_instance_declaration(base_variable));
                    return;
                  }

                if (variable_declaration_is_immutable(
                            variable_instance_declaration(base_variable)))
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(immutable_modification),
                            "An attempt was made to modify %v.",
                            variable_instance_declaration(base_variable));
                    return;
                  }
              }

            check_type_validity(slot_upper, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return;

            assert(type_is_valid(slot_upper)); /* VERIFIED */
            forced_value = check_value_type_and_possibly_force(new_value,
                    slot_lower, slot_upper, force_to_type,
                    EXCEPTION_TAG(assignment_type_mismatch),
                    EXCEPTION_TAG(assignment_type_match_indeterminate),
                    "assignment", location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(forced_value == NULL);
                return;
              }

            assert(forced_value != NULL);

            set_slot_contents(the_slot, forced_value, location, the_jumper);
            value_remove_reference(forced_value, the_jumper);
            return;
          }
        case BIK_LIST:
          {
            size_t next_source_index;
            size_t count;
            size_t number;

            next_source_index = 0;

            count = list_basket_instance_element_count(the_basket_instance);

            for (number = 0; number < count; ++number)
              {
                const char *label;
                value *child_value;
                boolean sub_force;

                label = list_basket_instance_label(the_basket_instance,
                                                   number);

                switch (get_value_kind(new_value))
                  {
                    case VK_SEMI_LABELED_VALUE_LIST:
                      {
                        if (label == NULL)
                          {
                            size_t source_limit;

                            source_limit = value_component_count(new_value);
                            if (next_source_index >= source_limit)
                              {
                                location_exception(the_jumper, location,
                                        EXCEPTION_TAG(assign_multiple_too_few),
                                        "The value being written to a list "
                                        "basket didn't have enough elements.");
                                return;
                              }

                            child_value = value_component_value(new_value,
                                    next_source_index);
                            assert(child_value != NULL);

                            ++next_source_index;
                          }
                        else
                          {
                            child_value = value_get_field(label, new_value);
                            if (child_value == NULL)
                              {
                                location_exception(the_jumper, location,
                                        EXCEPTION_TAG(
                                                write_by_name_missing_field),
                                        "A list basket element requiring field"
                                        " `%s' was written with a value that "
                                        "did not have that field defined.",
                                        label);
                                return;
                              }

                            next_source_index = (value_get_field_index(label,
                                                         new_value) + 1);
                          }

                        value_add_reference(child_value);

                        break;
                      }
                    case VK_SEMI_LABELED_MULTI_SET:
                    case VK_LEPTON:
                      {
                        if (label == NULL)
                          {
                            location_exception(the_jumper, location,
                                    EXCEPTION_TAG(assign_multiple_unordered),
                                    "A basket with an unlabeled element was "
                                    "written with a value that didn't include "
                                    "element ordering information.");
                            return;
                          }

                        child_value = value_get_field(label, new_value);
                        if (child_value == NULL)
                          {
                            location_exception(the_jumper, location,
                                    EXCEPTION_TAG(write_by_name_missing_field),
                                    "A list basket element requiring field "
                                    "`%s' was written with a value that did "
                                    "not have that field defined.", label);
                            return;
                          }

                        value_add_reference(child_value);

                        break;
                      }
                    case VK_MAP:
                      {
                        o_integer oi;
                        value *key_value;
                        boolean doubt;

                        if (label != NULL)
                          {
                            location_exception(the_jumper, location,
                                    EXCEPTION_TAG(write_by_name_missing_field),
                                    "A list basket element requiring field "
                                    "`%s' was written with a value that did "
                                    "not have that field defined.", label);
                            return;
                          }

                        oi_create_from_size_t(oi, next_source_index);
                        ++next_source_index;
                        if (oi_out_of_memory(oi))
                          {
                            jumper_do_abort(the_jumper);
                            return;
                          }

                        key_value = create_integer_value(oi);
                        oi_remove_reference(oi);
                        if (key_value == NULL)
                          {
                            jumper_do_abort(the_jumper);
                            return;
                          }

                        check_value_validity_except_map_targets(new_value,
                                location, the_jumper);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            value_remove_reference(key_value, the_jumper);
                            return;
                          }

                        assert(map_value_all_keys_are_valid(new_value));
                                /* VERIFIED */
                        assert(value_is_valid(key_value)); /* VERIFIED */
                        child_value = map_value_lookup(new_value, key_value,
                                &doubt, location, the_jumper);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            value_remove_reference(key_value, the_jumper);
                            return;
                          }
                        assert(!doubt);
                        if (child_value == NULL)
                          {
                            location_exception(the_jumper, location,
                                    EXCEPTION_TAG(assign_multiple_undefined),
                                    "An undefined element value was written "
                                    "into a list basket.");
                            value_remove_reference(key_value, the_jumper);
                            return;
                          }

                        value_remove_reference(key_value, the_jumper);
                        if (!(jumper_flowing_forward(the_jumper)))
                            return;

                        value_add_reference(child_value);

                        break;
                      }
                    case VK_OBJECT:
                      {
                        if (label == NULL)
                          {
                            location_exception(the_jumper, location,
                                    EXCEPTION_TAG(assign_multiple_unordered),
                                    "A basket with an unlabeled element was "
                                    "written with a value that didn't include "
                                    "element ordering information.");
                            return;
                          }

                        child_value = read_field_value(new_value, label,
                                                       location, the_jumper);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            assert(child_value == NULL);
                            return;
                          }

                        assert(child_value != NULL);

                        break;
                      }
                    default:
                      {
                        if (!force_to_type)
                          {
                            location_exception(the_jumper, location,
                                    EXCEPTION_TAG(
                                            assign_multiple_not_compound),
                                    "Attempted to assign to a list basket from"
                                    " a value that can't be broken into "
                                    "components.");
                            return;
                          }

                        child_value = new_value;

                        value_add_reference(child_value);

                        break;
                      }
                  }

                assert(child_value != NULL);

                sub_force =
                        (force_to_type ||
                         list_basket_instance_force(the_basket_instance,
                                                    number));
                write_to_basket_instance(
                        list_basket_instance_child(the_basket_instance,
                                number), child_value, sub_force, location,
                        the_jumper);
                value_remove_reference(child_value, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                    return;
              }

            return;
          }
        default:
          {
            assert(FALSE);
            return;
          }
      }
  }

static value *check_value_type_and_possibly_force(value *the_value,
        type *type_lower_bound, type *type_upper_bound, boolean force_to_type,
        static_exception_tag *mismatch_tag,
        static_exception_tag *match_indeterminate_tag,
        const char *action_description, const source_location *location,
        jumper *the_jumper)
  {
    boolean upper_doubt;
    char *why_not;
    boolean in_upper;

    assert(the_value != NULL);
    assert(type_lower_bound != NULL);
    assert(type_upper_bound != NULL);
    assert(the_jumper != NULL);

    assert(type_is_valid(type_upper_bound)); /* VERIFIED */

    why_not = NULL;
    assert(type_is_valid(type_upper_bound)); /* VERIFIED */
    in_upper = value_is_in_type(the_value, type_upper_bound, &upper_doubt,
                                &why_not, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;
    if (upper_doubt)
      {
        assert(why_not != NULL);
        location_exception(the_jumper, location, match_indeterminate_tag,
                "On %s, %s was unable to determine whether the new value was "
                "in the proper type because %s.", action_description,
                interpreter_name(), why_not);
        free(why_not);
        return NULL;
      }
    else if (!in_upper)
      {
        assert(why_not != NULL);
        if (force_to_type)
          {
            value *result;

            check_value_validity(the_value, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                free(why_not);
                return NULL;
              }

            assert(value_is_valid(the_value)); /* VERIFIED */
            assert(type_is_valid(type_upper_bound)); /* VERIFIED */
            result = force_value_to_type(the_value, type_upper_bound, location,
                                         the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                free(why_not);
                return NULL;
              }
            if (result != NULL)
              {
                free(why_not);
                return result;
              }
          }

        location_exception(the_jumper, location, mismatch_tag,
                "Type error on %s: value %U is not in type %t because %s.",
                action_description, the_value, type_upper_bound, why_not);
        free(why_not);
        return NULL;
      }

    value_add_reference(the_value);
    return the_value;
  }

static void get_and_force_slot_contents(slot_location *the_slot_location,
        const source_location *the_source_location,
        value *(*update_function)(void *update_data, value *existing_value,
                value *new_value, const source_location *the_source_location,
                jumper *the_jumper), void *update_data,
        boolean care_about_existing_value, value *new_value,
        jumper *the_jumper)
  {
    assert(the_slot_location != NULL);
    assert(the_source_location != NULL);
    assert(update_function != NULL);
    assert(the_jumper != NULL);

    switch (get_slot_location_kind(the_slot_location))
      {
        case SLK_VARIABLE:
          {
            variable_instance *instance;
            lock_chain *the_lock_chain;
            value *old_value;
            value *new_variable_value;

            instance = variable_slot_location_variable(the_slot_location);
            assert(instance != NULL);

            instance_check_validity(variable_instance_instance(instance),
                                    the_source_location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return;

            assert(variable_instance_is_instantiated(instance)); /* VERIFIED */
            assert(!(variable_instance_scope_exited(instance))); /* VERIFIED */
            the_lock_chain = variable_instance_lock_chain(instance);

            if (the_lock_chain != NULL)
              {
                lock_chain_add_reference(the_lock_chain);
                lock_chain_grab(the_lock_chain, the_source_location,
                                the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    lock_chain_remove_reference(the_lock_chain, the_jumper);
                    return;
                  }
              }

            assert(variable_instance_is_instantiated(instance)); /* VERIFIED */
            assert(!(variable_instance_scope_exited(instance))); /* VERIFIED */
            old_value = variable_instance_value(instance);

            new_variable_value = (*update_function)(update_data, old_value,
                    new_value, the_source_location, the_jumper);

            if (variable_instance_scope_exited(instance))
              {
                location_exception(the_jumper, the_source_location,
                        EXCEPTION_TAG(variable_use_after_deallocation),
                        "A variable instance was de-allocated during a write "
                        "to that variable.");
                if (new_variable_value != NULL)
                    value_remove_reference(new_variable_value, the_jumper);
                if (the_lock_chain != NULL)
                  {
                    lock_chain_release(the_lock_chain, the_source_location,
                                       the_jumper);
                    lock_chain_remove_reference(the_lock_chain, the_jumper);
                  }
                if (old_value != NULL)
                    value_remove_reference(old_value, the_jumper);
                return;
              }

            assert(variable_instance_is_instantiated(instance)); /* VERIFIED */
            assert(!(variable_instance_scope_exited(instance))); /* VERIFIED */

            if (new_variable_value != NULL)
              {
                assert(variable_instance_is_instantiated(instance));
                        /* VERIFIED */
                assert(!(variable_instance_scope_exited(instance)));
                        /* VERIFIED */
                if (variable_declaration_is_immutable(
                            variable_instance_declaration(instance)))
                  {
                    value *old_value;

                    old_value = variable_instance_value(instance);
                    if (new_variable_value != old_value)
                      {
                        location_exception(the_jumper, the_source_location,
                                EXCEPTION_TAG(immutable_modification),
                                "An attempt was made to modify %v.",
                                variable_instance_declaration(instance));
                      }
                    if (old_value != NULL)
                        value_remove_reference(old_value, the_jumper);
                  }
                else
                  {
                    set_variable_instance_value(instance, new_variable_value,
                                                the_jumper);
                  }
                value_remove_reference(new_variable_value, the_jumper);
              }

            if (the_lock_chain != NULL)
              {
                lock_chain_release(the_lock_chain, the_source_location,
                                   the_jumper);
                lock_chain_remove_reference(the_lock_chain, the_jumper);
              }

            if (old_value != NULL)
                value_remove_reference(old_value, the_jumper);

            return;
          }
        case SLK_LOOKUP:
          {
            slot_location *base_slot;
            update_data_info back_reference;

            base_slot = lookup_slot_location_base(the_slot_location);
            assert(base_slot != NULL);

            back_reference.the_slot_location = the_slot_location;
            back_reference.update_function = update_function;
            back_reference.update_data = update_data;
            back_reference.care_about_existing_value =
                    care_about_existing_value;

            get_and_force_slot_contents(base_slot, the_source_location,
                    &need_lookup_update_function, &back_reference, TRUE,
                    new_value, the_jumper);
            return;
          }
        case SLK_FIELD:
          {
            slot_location *base_slot;
            value *overload_base;
            update_data_info back_reference;

            base_slot = field_slot_location_base(the_slot_location);
            assert(base_slot != NULL);

            overload_base = slot_location_overload_base(the_slot_location);

            if (overload_base != NULL)
              {
                value *base_value;
                const char *field_name;
                value *new_munged_value;

                base_value = create_slot_location_value(base_slot);
                if (base_value == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    return;
                  }

                field_name = field_slot_location_field_name(the_slot_location);
                assert(field_name != NULL);

                if (!care_about_existing_value)
                  {
                    if (new_value == NULL)
                      {
                        value_remove_reference(base_value, the_jumper);
                        return;
                      }

                    value_add_reference(new_value);
                    new_munged_value = new_value;
                  }
                else
                  {
                    value *existing_value;
                    verdict the_verdict;

                    the_verdict = try_scoped_field_overloading(overload_base,
                            &existing_value, base_value, field_name, NULL,
                            the_jumper, the_source_location);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        assert(existing_value == NULL);
                        value_remove_reference(base_value, the_jumper);
                        return;
                      }

                    if (the_verdict == MISSION_FAILED)
                      {
                        assert(existing_value == NULL);
                        new_munged_value = NULL;
                      }
                    else
                      {
                        new_munged_value = (*update_function)(update_data,
                                existing_value, new_value, the_source_location,
                                the_jumper);

                        if (new_munged_value == NULL)
                          {
                            if (existing_value != NULL)
                              {
                                value_remove_reference(existing_value,
                                                       the_jumper);
                              }
                            value_remove_reference(base_value, the_jumper);
                            return;
                          }

                        if (existing_value != NULL)
                          {
                            value_remove_reference(existing_value, the_jumper);
                            if (!(jumper_flowing_forward(the_jumper)))
                              {
                                value_remove_reference(base_value, the_jumper);
                                return;
                              }
                          }
                      }
                  }

                if (new_munged_value != NULL)
                  {
                    verdict the_verdict;

                    the_verdict = try_scoped_field_overloading(overload_base,
                            NULL, base_value, field_name, new_munged_value,
                            the_jumper, the_source_location);
                    if ((!(jumper_flowing_forward(the_jumper))) ||
                        (the_verdict == MISSION_ACCOMPLISHED))
                      {
                        value_remove_reference(new_munged_value, the_jumper);
                        value_remove_reference(base_value, the_jumper);
                        return;
                      }

                    get_and_force_slot_contents(base_slot, the_source_location,
                            &replace_update_function, NULL, FALSE,
                            new_munged_value, the_jumper);
                    value_remove_reference(new_munged_value, the_jumper);
                    value_remove_reference(base_value, the_jumper);
                    return;
                  }

                value_remove_reference(base_value, the_jumper);
              }

            back_reference.the_slot_location = the_slot_location;
            back_reference.update_function = update_function;
            back_reference.update_data = update_data;
            back_reference.care_about_existing_value =
                    care_about_existing_value;

            get_and_force_slot_contents(base_slot, the_source_location,
                    &need_field_update_function, &back_reference, TRUE,
                    new_value, the_jumper);
            return;
          }
        case SLK_TAGALONG:
          {
            slot_location *base_slot;
            update_data_info back_reference;

            instance_check_validity(
                    tagalong_key_instance(tagalong_slot_location_key(
                            the_slot_location)), the_source_location,
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return;

            base_slot = tagalong_slot_location_base(the_slot_location);
            assert(base_slot != NULL);

            back_reference.the_slot_location = the_slot_location;
            back_reference.update_function = update_function;
            back_reference.update_data = update_data;
            back_reference.care_about_existing_value =
                    care_about_existing_value;

            get_and_force_slot_contents(base_slot, the_source_location,
                    &need_tagalong_update_function, &back_reference, TRUE,
                    new_value, the_jumper);
            return;
          }
        case SLK_CALL:
          {
            value *arguments[4];
            const char *argument_names[4];
            size_t argument_count;
            value *new_munged_value;
            verdict the_verdict;

            arguments[1] = call_slot_location_argument0(the_slot_location);
            arguments[2] = call_slot_location_argument1(the_slot_location);
            argument_names[1] =
                    call_slot_location_argument_name0(the_slot_location);
            argument_names[2] =
                    call_slot_location_argument_name1(the_slot_location);

            if (arguments[1] == NULL)
              {
                assert(argument_names[1] == NULL);
                assert(arguments[2] == NULL);
                assert(argument_names[2] == NULL);
                argument_count = 0;
              }
            else if (arguments[2] == NULL)
              {
                assert(argument_names[2] == NULL);
                argument_count = 1;
              }
            else
              {
                argument_count = 2;
              }

            if (!care_about_existing_value)
              {
                if (new_value != NULL)
                    value_add_reference(new_value);
                new_munged_value = new_value;
              }
            else
              {
                value *old_value;

                if (slot_location_overload_base(the_slot_location) != NULL)
                  {
                    verdict the_verdict;

                    arguments[0] =
                            call_slot_location_argument_for_overload_base(
                                    the_slot_location);
                    argument_names[0] = NULL;
                    the_verdict = try_overloading_from_call_base(
                            slot_location_overload_base(the_slot_location),
                            &old_value, argument_count + 1, &(arguments[0]),
                            &(argument_names[0]), the_jumper,
                            the_source_location);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        assert(old_value == NULL);
                        return;
                      }
                    if (the_verdict == MISSION_ACCOMPLISHED)
                        assert(old_value != NULL);
                    else
                        assert(old_value == NULL);
                  }
                else
                  {
                    old_value = NULL;
                  }

                if (old_value == NULL)
                  {
                    verdict the_verdict;

                    the_verdict = try_overloading_from_call_base(
                            call_slot_location_base(the_slot_location),
                            &old_value, argument_count, &(arguments[1]),
                            &(argument_names[1]), the_jumper,
                            the_source_location);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        assert(old_value == NULL);
                        return;
                      }
                    if (the_verdict == MISSION_ACCOMPLISHED)
                        assert(old_value != NULL);
                    else
                        assert(old_value == NULL);
                  }

                new_munged_value = (*update_function)(update_data, old_value,
                        new_value, the_source_location, the_jumper);
                if (old_value != NULL)
                  {
                    value_remove_reference(old_value, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        if (new_munged_value != NULL)
                          {
                            value_remove_reference(new_munged_value,
                                                   the_jumper);
                          }
                        return;
                      }
                  }
              }

            if (new_munged_value == NULL)
                return;

            arguments[argument_count + 1] = new_munged_value;
            argument_names[argument_count + 1] = NULL;

            if (slot_location_overload_base(the_slot_location) != NULL)
              {
                verdict the_verdict;

                arguments[0] = call_slot_location_argument_for_overload_base(
                        the_slot_location);
                argument_names[0] = NULL;
                the_verdict = try_overloading_from_call_base(
                        slot_location_overload_base(the_slot_location), NULL,
                        argument_count + 2, &(arguments[0]),
                        &(argument_names[0]), the_jumper, the_source_location);
                if (the_verdict == MISSION_ACCOMPLISHED)
                  {
                    value_remove_reference(new_munged_value, the_jumper);
                    return;
                  }
              }

            the_verdict = try_overloading_from_call_base(
                    call_slot_location_base(the_slot_location), NULL,
                    argument_count + 1, &(arguments[1]), &(argument_names[1]),
                    the_jumper, the_source_location);
            if (the_verdict == MISSION_ACCOMPLISHED)
              {
                value_remove_reference(new_munged_value, the_jumper);
                return;
              }

            location_exception(the_jumper, the_source_location,
                    EXCEPTION_TAG(non_pointer_no_overloaded_match),
                    "No overloaded operator matched a write through an "
                    "overloaded %s on a non-pointer value.",
                    call_slot_location_operation_name(the_slot_location));

            value_remove_reference(new_munged_value, the_jumper);
            return;
          }
        case SLK_PASS:
          {
            slot_location *base_slot;
            value *overload_base;
            update_data_info back_reference;

            base_slot = pass_slot_location_base(the_slot_location);
            assert(base_slot != NULL);

            overload_base = slot_location_overload_base(the_slot_location);

            if (overload_base != NULL)
              {
                value *base_value;
                value *new_munged_value;

                base_value = create_slot_location_value(base_slot);
                if (base_value == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    return;
                  }

                if (!care_about_existing_value)
                  {
                    if (new_value == NULL)
                      {
                        value_remove_reference(base_value, the_jumper);
                        return;
                      }

                    value_add_reference(new_value);
                    new_munged_value = new_value;
                  }
                else
                  {
                    value *existing_value;
                    verdict the_verdict;

                    the_verdict = try_scoped_dereference_overloading(
                            overload_base, &existing_value, base_value, NULL,
                            the_jumper, the_source_location);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        assert(existing_value == NULL);
                        value_remove_reference(base_value, the_jumper);
                        return;
                      }

                    if (the_verdict == MISSION_FAILED)
                      {
                        assert(existing_value == NULL);
                        new_munged_value = NULL;
                      }
                    else
                      {
                        new_munged_value = (*update_function)(update_data,
                                existing_value, new_value, the_source_location,
                                the_jumper);

                        if (new_munged_value == NULL)
                          {
                            if (existing_value != NULL)
                              {
                                value_remove_reference(existing_value,
                                                       the_jumper);
                              }
                            value_remove_reference(base_value, the_jumper);
                            return;
                          }

                        if (existing_value != NULL)
                          {
                            value_remove_reference(existing_value, the_jumper);
                            if (!(jumper_flowing_forward(the_jumper)))
                              {
                                value_remove_reference(base_value, the_jumper);
                                return;
                              }
                          }
                      }
                  }

                if (new_munged_value != NULL)
                  {
                    verdict the_verdict;

                    the_verdict = try_scoped_dereference_overloading(
                            overload_base, NULL, base_value, new_munged_value,
                            the_jumper, the_source_location);
                    if ((!(jumper_flowing_forward(the_jumper))) ||
                        (the_verdict == MISSION_ACCOMPLISHED))
                      {
                        value_remove_reference(new_munged_value, the_jumper);
                        value_remove_reference(base_value, the_jumper);
                        return;
                      }

                    get_and_force_slot_contents(base_slot, the_source_location,
                            &replace_update_function, NULL, FALSE,
                            new_munged_value, the_jumper);
                    value_remove_reference(new_munged_value, the_jumper);
                    value_remove_reference(base_value, the_jumper);
                    return;
                  }

                value_remove_reference(base_value, the_jumper);
              }

            back_reference.the_slot_location = the_slot_location;
            back_reference.update_function = update_function;
            back_reference.update_data = update_data;
            back_reference.care_about_existing_value =
                    care_about_existing_value;

            get_and_force_slot_contents(base_slot, the_source_location,
                    &need_pass_update_function, &back_reference, TRUE,
                    new_value, the_jumper);
            return;
          }
        default:
          {
            assert(FALSE);
            return;
          }
      }
  }

static value *replace_update_function(void *update_data, value *existing_value,
        value *new_value, const source_location *the_source_location,
        jumper *the_jumper)
  {
    assert(update_data == NULL);

    if (new_value != NULL)
        value_add_reference(new_value);
    return new_value;
  }

static value *need_lookup_update_function(void *update_data,
        value *existing_value, value *new_value,
        const source_location *the_source_location, jumper *the_jumper)
  {
    update_data_info *back_reference;
    value *base_value;
    lookup_actual_arguments *actuals;

    assert(update_data != NULL);

    back_reference = (update_data_info *)update_data;

    actuals = lookup_slot_location_actuals(back_reference->the_slot_location);
    assert(actuals != NULL);

    check_lookup_actual_arguments_validity(actuals, the_source_location,
                                           the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    assert(lookup_actual_arguments_is_valid(actuals)); /* VERIFIED */
    base_value = set_through_lookup(existing_value, actuals,
            slot_location_overload_base(back_reference->the_slot_location),
            back_reference->update_function, back_reference->update_data,
            back_reference->care_about_existing_value, new_value,
            the_source_location, the_jumper);

    return base_value;
  }

static value *need_field_update_function(void *update_data,
        value *existing_value, value *new_value,
        const source_location *the_source_location, jumper *the_jumper)
  {
    update_data_info *back_reference;
    const char *field_name;
    value *old_field;
    value *new_field;
    value *base_value;

    assert(update_data != NULL);

    back_reference = (update_data_info *)update_data;

    field_name =
            field_slot_location_field_name(back_reference->the_slot_location);
    assert(field_name != NULL);

    if (existing_value != NULL)
      {
        value_kind kind;

        check_value_validity_except_map_targets(existing_value,
                the_source_location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return NULL;

        assert(value_is_valid_except_map_targets(existing_value));
                /* VERIFIED */
        kind = get_value_kind(existing_value);
        if ((kind == VK_SEMI_LABELED_MULTI_SET) || (kind == VK_LEPTON) ||
            (kind == VK_SEMI_LABELED_VALUE_LIST))
          {
            old_field = value_get_field(field_name, existing_value);
            if (old_field != NULL)
                value_add_reference(old_field);
          }
        else if (kind == VK_OBJECT)
          {
            object *the_object;
            size_t field_num;

            the_object = object_value_data(existing_value);
            assert(the_object != NULL);

            assert(!(object_is_closed(the_object))); /* VERIFIED */
            field_num = object_field_lookup(the_object, field_name);
            if (field_num >= object_field_count(the_object))
              {
                location_exception(the_jumper, the_source_location,
                        EXCEPTION_TAG(object_no_field),
                        "Object has no field named `%s'.", field_name);
                return NULL;
              }

            assert(!(object_is_closed(the_object))); /* VERIFIED */
            if (!(object_field_is_variable(the_object, field_num)))
              {
                location_exception(the_jumper, the_source_location,
                        EXCEPTION_TAG(object_write_non_data_field),
                        "An attempt was made to assign to non-data field "
                        "`%s'.", field_name);
                return NULL;
              }

            if (back_reference->care_about_existing_value)
              {
                variable_instance *instance;

                assert(!(object_is_closed(the_object))); /* VERIFIED */
                instance = object_field_variable(the_object, field_num);
                assert(instance != NULL);

                old_field = read_variable_value(instance, NULL,
                        the_source_location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    assert(old_field == NULL);
                    return NULL;
                  }
              }
            else
              {
                old_field = NULL;
              }
          }
        else
          {
            old_field = NULL;
          }
      }
    else
      {
        old_field = NULL;
      }

    if (existing_value != NULL)
        value_add_reference(existing_value);

    new_field = (*(back_reference->update_function))(
            back_reference->update_data, old_field, new_value,
            the_source_location, the_jumper);
    if (old_field != NULL)
        value_remove_reference(old_field, the_jumper);

    if (new_field == NULL)
      {
        if (!(jumper_flowing_forward(the_jumper)))
          {
            if (existing_value != NULL)
                value_remove_reference(existing_value, the_jumper);
            return NULL;
          }
        return existing_value;
      }

    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (existing_value != NULL)
            value_remove_reference(existing_value, the_jumper);
        value_remove_reference(new_field, the_jumper);
        return NULL;
      }

    if (existing_value != NULL)
      {
        value_kind kind;

        kind = get_value_kind(existing_value);
        if ((kind == VK_SEMI_LABELED_MULTI_SET) || (kind == VK_LEPTON))
          {
          do_copy:
            if (value_has_only_one_reference(existing_value))
              {
                base_value = existing_value;
              }
            else
              {
                base_value = copy_value(existing_value);
                value_remove_reference(existing_value, the_jumper);
              }
          }
        else if (kind == VK_SEMI_LABELED_VALUE_LIST)
          {
            size_t field_index;
            size_t count;
            size_t number;

            field_index = value_get_field_index(field_name, existing_value);
            if (field_index < value_component_count(existing_value))
                goto do_copy;

            base_value = create_semi_labeled_multi_set_value();
            if (base_value == NULL)
              {
                jumper_do_abort(the_jumper);
                value_remove_reference(new_field, the_jumper);
                return NULL;
              }

            count = value_component_count(existing_value);

            for (number = 0; number < count; ++number)
              {
                verdict the_verdict;

                the_verdict = add_field(base_value,
                        value_component_label(existing_value, number),
                        value_component_value(existing_value, number));
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    value_remove_reference(new_field, the_jumper);
                    return base_value;
                  }
              }

            value_remove_reference(existing_value, the_jumper);
          }
        else if (kind == VK_OBJECT)
          {
            base_value = existing_value;
          }
        else
          {
            value_remove_reference(existing_value, the_jumper);
            base_value = create_semi_labeled_multi_set_value();
          }
      }
    else
      {
        base_value = create_semi_labeled_multi_set_value();
      }

    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (base_value != NULL)
            value_remove_reference(base_value, the_jumper);
        value_remove_reference(new_field, the_jumper);
        return NULL;
      }

    if (base_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(new_field, the_jumper);
        return NULL;
      }

    switch (get_value_kind(base_value))
      {
        case VK_SEMI_LABELED_VALUE_LIST:
        case VK_SEMI_LABELED_MULTI_SET:
        case VK_LEPTON:
          {
            set_field(base_value, field_name, new_field, the_jumper);
            break;
          }
        case VK_OBJECT:
          {
            object *the_object;
            size_t field_num;
            variable_instance *instance;

            the_object = object_value_data(base_value);
            assert(the_object != NULL);

            if (object_is_closed(the_object))
              {
                location_exception(the_jumper, the_source_location,
                        EXCEPTION_TAG(object_use_after_deallocation),
                        "An object was closed while one of its fields was "
                        "being written.");
                value_remove_reference(base_value, the_jumper);
                value_remove_reference(new_field, the_jumper);
                return NULL;
              }

            assert(!(object_is_closed(the_object))); /* VERIFIED */
            field_num = object_field_lookup(the_object, field_name);
            assert(field_num < object_field_count(the_object));

            assert(!(object_is_closed(the_object))); /* VERIFIED */
            assert(object_field_is_variable(the_object, field_num));

            assert(!(object_is_closed(the_object))); /* VERIFIED */
            instance = object_field_variable(the_object, field_num);
            assert(instance != NULL);

            if (variable_declaration_is_immutable(
                        variable_instance_declaration(instance)))
              {
                location_exception(the_jumper, the_source_location,
                        EXCEPTION_TAG(immutable_modification),
                        "An attempt was made to modify %v.",
                        variable_instance_declaration(instance));
                value_remove_reference(base_value, the_jumper);
                value_remove_reference(new_field, the_jumper);
                return NULL;
              }

            if (!(variable_instance_is_instantiated(instance)))
              {
                location_exception(the_jumper, the_source_location,
                        EXCEPTION_TAG(write_variable_uninstantiated),
                        "An attempt was made to modify %v before it was "
                        "instantiated.",
                        variable_instance_declaration(instance));
                value_remove_reference(base_value, the_jumper);
                value_remove_reference(new_field, the_jumper);
                return NULL;
              }

            if (variable_instance_scope_exited(instance))
              {
                location_exception(the_jumper, the_source_location,
                        EXCEPTION_TAG(write_variable_deallocated),
                        "An attempt was made to modify %v after it had ceased "
                        "to exist because its scope had ended.",
                        variable_instance_declaration(instance));
                value_remove_reference(base_value, the_jumper);
                value_remove_reference(new_field, the_jumper);
                return NULL;
              }

            assert(variable_instance_is_instantiated(instance)); /* VERIFIED */
            assert(!(variable_instance_scope_exited(instance))); /* VERIFIED */

            set_variable_value_with_locking(instance, new_field,
                                            the_source_location, the_jumper);
            break;
          }
        default:
          {
            assert(FALSE);
            return NULL;
          }
      }

    value_remove_reference(new_field, the_jumper);
    return base_value;
  }

static value *need_tagalong_update_function(void *update_data,
        value *existing_value, value *new_value,
        const source_location *the_source_location, jumper *the_jumper)
  {
    update_data_info *back_reference;
    value *base_value;
    tagalong_key *key;
    type *on_type;
    boolean is_in;
    boolean doubt;
    char *why_not;
    lock_chain *the_lock_chain;
    value *old_field;
    value *new_field;

    assert(update_data != NULL);

    back_reference = (update_data_info *)update_data;

    if (existing_value == NULL)
      {
        location_exception(the_jumper, the_source_location,
                EXCEPTION_TAG(tagalong_base_undefined),
                "Attempted to set a tagalong field on an uninitialized "
                "value.");
        return NULL;
      }

    if (value_has_only_one_reference(existing_value))
      {
        value_add_reference(existing_value);
        base_value = existing_value;
      }
    else
      {
        base_value = copy_value(existing_value);
        if (base_value == NULL)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }
      }

    key = tagalong_slot_location_key(back_reference->the_slot_location);
    assert(key != NULL);

    if (tagalong_key_scope_exited(key))
      {
        location_exception(the_jumper, the_source_location,
                EXCEPTION_TAG(tagalong_use_after_deallocation),
                "A tagalong key was de-allocated during a write through that "
                "tagalong key.");
        return base_value;
      }

    assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
    assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */
    on_type = tagalong_key_on_type(key);

    check_type_validity(on_type, the_source_location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return base_value;

    assert(type_is_valid(on_type)); /* VERIFIED */
    is_in = value_is_in_type(base_value, on_type, &doubt, &why_not,
                             the_source_location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return base_value;

    if (doubt)
      {
        location_exception(the_jumper, the_source_location,
                EXCEPTION_TAG(tagalong_type_match_indeterminate),
                "When setting a tagalong field, %s was unable to determine "
                "whether the base value was in the type allowed as a base for "
                "that tagalong because %s.", interpreter_name(), why_not);
        free(why_not);
        return base_value;
      }

    if (!is_in)
      {
        location_exception(the_jumper, the_source_location,
                EXCEPTION_TAG(tagalong_type_mismatch),
                "Attempted to set a tagalong field on a value that wasn't "
                "allowed for that tagalong because %s.", why_not);
        free(why_not);
        return base_value;
      }

    assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
    assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */
    the_lock_chain = tagalong_key_lock_chain(key);

    if (the_lock_chain != NULL)
      {
        lock_chain_grab(the_lock_chain, the_source_location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return base_value;
      }

    assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
    assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */
    old_field = lookup_tagalong(base_value, key, TRUE, the_source_location,
                                the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(old_field == NULL);
        if (the_lock_chain != NULL)
          {
            lock_chain_release(the_lock_chain, the_source_location,
                               the_jumper);
          }
        return base_value;
      }

    new_field = (*(back_reference->update_function))(
            back_reference->update_data, old_field, new_value,
            the_source_location, the_jumper);

    if (tagalong_key_scope_exited(key))
      {
        location_exception(the_jumper, the_source_location,
                EXCEPTION_TAG(tagalong_use_after_deallocation),
                "A tagalong key was de-allocated during a write through that "
                "tagalong key.");
        if (new_field != NULL)
            value_remove_reference(new_field, the_jumper);
        if (the_lock_chain != NULL)
          {
            lock_chain_release(the_lock_chain, the_source_location,
                               the_jumper);
          }
        return base_value;
      }

    if (new_field != NULL)
      {
        assert(tagalong_key_is_instantiated(key)); /* VERIFIED */
        assert(!(tagalong_key_scope_exited(key))); /* VERIFIED */
        set_tagalong(base_value, key, new_field, the_source_location,
                     the_jumper);
        value_remove_reference(new_field, the_jumper);
      }

    if (the_lock_chain != NULL)
        lock_chain_release(the_lock_chain, the_source_location, the_jumper);

    return base_value;
  }

static value *need_pass_update_function(void *update_data,
        value *existing_value, value *new_value,
        const source_location *the_source_location, jumper *the_jumper)
  {
    update_data_info *back_reference;
    value *result;

    assert(update_data != NULL);

    back_reference = (update_data_info *)update_data;

    result = (*(back_reference->update_function))(back_reference->update_data,
            existing_value, new_value, the_source_location, the_jumper);

    if (result == NULL)
      {
        if (!(jumper_flowing_forward(the_jumper)))
            return NULL;
        if (existing_value != NULL)
            value_add_reference(existing_value);
        return existing_value;
      }

    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

static void concatenate_onto_semi_labeled_value_list(value *result_value,
        value *addition, const source_location *location, jumper *the_jumper)
  {
    assert(result_value != NULL);
    assert(addition != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(result_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_is_valid(addition)); /* VERIFIED */

    switch (get_value_kind(addition))
      {
        case VK_MAP:
          {
            boolean doubt;
            boolean is_array;
            o_integer lower_bound;
            o_integer upper_bound;
            boolean lower_doubt;
            boolean upper_doubt;
            o_integer key_oi;

            assert(value_is_valid(addition)); /* VERIFIED */
            is_array =
                    map_value_is_array(addition, &doubt, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return;

            if (doubt)
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(concatenation_array_indeterminate),
                        "While evaluating a concatenate expression, %s was "
                        "unable to determine whether one of the operands was a"
                        " map value whose keys are all integers.",
                        interpreter_name());
                return;
              }

            if (!is_array)
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(concatenation_non_array_map),
                        "Concatenation of a non-array map value is illegal.");
                return;
              }

            map_value_integer_key_bounds(addition, &lower_bound, &upper_bound,
                    &lower_doubt, &upper_doubt, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return;

            if (lower_doubt || upper_doubt)
              {
                if (!(oi_out_of_memory(lower_bound)))
                    oi_remove_reference(lower_bound);
                if (!(oi_out_of_memory(upper_bound)))
                    oi_remove_reference(upper_bound);
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(concatenation_array_indeterminate),
                        "While evaluating a concatenate expression, %s was "
                        "unable to determine the %s index of one of the "
                        "operands.", interpreter_name(),
                        (lower_doubt ? "minimum" : "maximum"));
                return;
              }

            assert(!(oi_out_of_memory(lower_bound)));
            assert(!(oi_out_of_memory(upper_bound)));

            if (oi_less_than(upper_bound, lower_bound))
              {
                oi_remove_reference(lower_bound);
                oi_remove_reference(upper_bound);
                return;
              }

            key_oi = lower_bound;

            while (!(oi_less_than(upper_bound, key_oi)))
              {
                value *key_value;
                boolean doubt;
                value *element_value;
                verdict the_verdict;
                o_integer new_key;

                key_value = create_integer_value(key_oi);
                if (key_value == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    oi_remove_reference(key_oi);
                    oi_remove_reference(upper_bound);
                    return;
                  }

                assert(value_is_valid(addition)); /* VERIFIED */
                assert(map_value_all_keys_are_valid(addition)); /* VERIFIED */
                assert(value_is_valid(key_value)); /* VERIFIED */
                element_value = map_value_lookup(addition, key_value, &doubt,
                                                 location, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    oi_remove_reference(key_oi);
                    oi_remove_reference(upper_bound);
                    return;
                  }
                assert(!doubt);
                if (element_value == NULL)
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(concatenation_sparse_array),
                            "Concatenation of a sparse array is illegal.");
                    value_remove_reference(key_value, the_jumper);
                    oi_remove_reference(key_oi);
                    oi_remove_reference(upper_bound);
                    return;
                  }

                value_remove_reference(key_value, the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    oi_remove_reference(key_oi);
                    oi_remove_reference(upper_bound);
                    return;
                  }

                the_verdict = add_field(result_value, NULL, element_value);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    oi_remove_reference(key_oi);
                    oi_remove_reference(upper_bound);
                    return;
                  }

                oi_add(new_key, key_oi, oi_one);
                oi_remove_reference(key_oi);
                if (oi_out_of_memory(new_key))
                  {
                    jumper_do_abort(the_jumper);
                    oi_remove_reference(upper_bound);
                    return;
                  }
                key_oi = new_key;
              }

            oi_remove_reference(key_oi);
            oi_remove_reference(upper_bound);

            return;
          }
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            size_t component_count;
            size_t component_num;

            component_count = value_component_count(addition);

            for (component_num = 0; component_num < component_count;
                 ++component_num)
              {
                verdict the_verdict;

                the_verdict = add_field(result_value,
                        value_component_label(addition, component_num),
                        value_component_value(addition, component_num));
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    return;
                  }
              }

            return;
          }
        default:
          {
            assert(FALSE);
            return;
          }
      }
  }

static type *evaluate_expression_as_type_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    value *name_value;
    type *result;

    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    name_value = evaluate_expression(the_expression, the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(name_value == NULL);
        return NULL;
      }

    assert(name_value != NULL);

    switch (get_value_kind(name_value))
      {
        case VK_REGULAR_EXPRESSION:
          {
            result = get_regular_expression_type(
                    regular_expression_value_data(name_value));
            assert((result == NULL) || type_is_valid(result)); /* VERIFIED */
            if (result == NULL)
                jumper_do_abort(the_jumper);
            break;
          }
        case VK_LEPTON_KEY:
          {
            result = check_and_get_lepton_type(value_lepton_key(name_value), 0,
                    NULL, NULL, TRUE, get_expression_location(the_expression),
                    the_jumper);
            break;
          }
        case VK_ROUTINE:
          {
            routine_instance *instance;
            routine_declaration *declaration;
            type *boolean_type;
            type *routine_return_type;
            boolean is_subset;
            boolean doubt;
            char *why_not;
            formal_arguments *formals;
            size_t formal_count;

            instance = routine_value_data(name_value);
            assert(instance != NULL);

            declaration = routine_instance_declaration(instance);
            assert(declaration != NULL);

            if (!(routine_instance_is_instantiated(instance)))
              {
                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(routine_type_uninstantiated),
                        "%R was used as a type before it was instantiated by "
                        "executing its declaration.", declaration);
                result = NULL;
                break;
              }

            if (routine_instance_scope_exited(instance))
              {
                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(routine_type_deallocated),
                        "%R was used as a type after it had been deallocated.",
                        declaration);
                result = NULL;
                break;
              }

            assert(routine_instance_is_instantiated(instance)); /* VERIFIED */
            assert(!(routine_instance_scope_exited(instance))); /* VERIFIED */

            if (routine_declaration_is_class(declaration))
              {
                result = get_class_type(instance);
                assert((result == NULL) || type_is_valid(result));
                        /* VERIFIED */
                if (result == NULL)
                    jumper_do_abort(the_jumper);
                break;
              }

            if (!(routine_declaration_is_pure(declaration)))
              {
                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(function_type_non_pure),
                        "A non-pure routine is illegal as a type.");
                result = NULL;
                break;
              }

            boolean_type = get_boolean_type();
            if (boolean_type == NULL)
              {
                jumper_do_abort(the_jumper);
                result = NULL;
                break;
              }
            assert(routine_instance_is_instantiated(instance)); /* VERIFIED */
            assert(!(routine_instance_scope_exited(instance))); /* VERIFIED */
            routine_return_type = routine_instance_valid_return_type(instance,
                    get_expression_location(the_expression), the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(routine_return_type == NULL);
                result = NULL;
                break;
              }

            assert(type_is_valid(routine_return_type)); /* VERIFIED */
            assert(type_is_valid(boolean_type)); /* VERIFIED */
            is_subset = type_is_subset(routine_return_type, boolean_type,
                    &doubt, &why_not, get_expression_location(the_expression),
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                result = NULL;
                break;
              }
            if (doubt)
              {
                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(
                                function_type_return_type_match_indeterminate),
                        "%s wasn't able to determine whether a routine used as"
                        " a type had a boolean return type because %s.",
                        interpreter_name(), why_not);
                free(why_not);
                result = NULL;
                break;
              }
            if (!is_subset)
              {
                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(function_type_return_type_mismatch),
                        "A routine used as a type had a non-boolean return "
                        "type because %s.", why_not);
                free(why_not);
                result = NULL;
                break;
              }

            formals = routine_declaration_formals(declaration);
            assert(formals != NULL);

            formal_count = formal_arguments_argument_count(formals);
            if (formal_count == 0)
              {
                if (!(routine_declaration_extra_arguments_allowed(
                              declaration)))
                  {
                    expression_exception(the_jumper, the_expression,
                            EXCEPTION_TAG(function_type_no_arguments),
                            "A function that doesn't allow any arguments is "
                            "illegal as a type.");
                    result = NULL;
                    break;
                  }
              }
            else
              {
                type *first_formal_type;
                type *anything_type;
                boolean is_subset;
                boolean doubt;
                char *why_not;
                size_t formal_num;

                assert(routine_instance_is_instantiated(instance));
                        /* VERIFIED */
                assert(!(routine_instance_scope_exited(instance)));
                        /* VERIFIED */
                first_formal_type =
                        routine_instance_argument_type(instance, 0);
                assert(first_formal_type != NULL);

                check_type_validity(first_formal_type,
                        get_expression_location(the_expression), the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    result = NULL;
                    break;
                  }

                anything_type = get_anything_type();

                assert(type_is_valid(anything_type)); /* VERIFIED */
                assert(type_is_valid(first_formal_type)); /* VERIFIED */
                is_subset = type_is_subset(anything_type, first_formal_type,
                        &doubt, &why_not,
                        get_expression_location(the_expression), the_jumper);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    result = NULL;
                    break;
                  }
                if (doubt)
                  {
                    expression_exception(the_jumper, the_expression,
                            EXCEPTION_TAG(
                              function_type_argument_type_match_indeterminate),
                            "%s wasn't able to determine whether a function "
                            "used as a type accepts all values as its first "
                            "argument because %s.", interpreter_name(),
                            why_not);
                    free(why_not);
                    result = NULL;
                    break;
                  }
                if (!is_subset)
                  {
                    expression_exception(the_jumper, the_expression,
                            EXCEPTION_TAG(
                                    function_type_argument_type_mismatch),
                            "A function that doesn't accept all values as its "
                            "first argument is illegal as a type, and the type"
                            " of all values is not a sub-type of the type of "
                            "the first argument in this case because %s.",
                            why_not);
                    free(why_not);
                    result = NULL;
                    break;
                  }

                for (formal_num = 1; formal_num < formal_count; ++formal_num)
                  {
                    if (variable_declaration_initializer(
                                formal_arguments_formal_by_number(formals,
                                        formal_num)) == NULL)
                      {
                        expression_exception(the_jumper, the_expression,
                                EXCEPTION_TAG(
                                        function_type_too_many_arguments),
                                "A function that has a formal parameter beyond"
                                " the first without a default value is illegal"
                                " as a type.");
                        result = NULL;
                        break;
                      }
                  }
              }

            assert(routine_instance_is_instantiated(instance)); /* VERIFIED */
            assert(!(routine_instance_scope_exited(instance))); /* VERIFIED */
            result = get_test_routine_type(instance);

            if (result == NULL)
                jumper_do_abort(the_jumper);

            break;
          }
        case VK_ROUTINE_CHAIN:
          {
            routine_instance_chain *instance_chain;
            routine_instance *first_instance;
            type *anything_type;
            type *argument_types[1];
            const char *argument_names[1] = { NULL };
            type *return_type;
            boolean always_resolves;
            boolean never_resolves;
            boolean always_pure;
            routine_instance *only_possible_resolution;
            type *boolean_type;
            boolean is_subset;
            boolean doubt;
            char *why_not;

            instance_chain = routine_chain_value_data(name_value);
            assert(instance_chain != NULL);

            first_instance = routine_instance_chain_instance(instance_chain);
            assert(first_instance != NULL);

            if (routine_declaration_is_class(
                        routine_instance_declaration(first_instance)))
              {
                result = get_class_type(first_instance);
                if (result == NULL)
                    jumper_do_abort(the_jumper);
                break;
              }

            anything_type = get_anything_type();
            argument_types[0] = anything_type;
            get_overloading_information(instance_chain, 1,
                    &(argument_types[0]), &(argument_names[0]), &return_type,
                    &always_resolves, &never_resolves, &always_pure,
                    &only_possible_resolution,
                    get_expression_location(the_expression), the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                result = NULL;
                break;
              }

            assert(routine_instance_chain_is_valid(instance_chain));
                    /* VERIFIED */
            assert(routine_instance_chain_is_valid(instance_chain));
                    /* VERIFIED */
            if (only_possible_resolution != NULL)
              {
                assert(routine_instance_is_instantiated(
                               only_possible_resolution)); /* VERIFIED */
                assert(!(routine_instance_scope_exited(
                                 only_possible_resolution))); /* VERIFIED */
              }

            if (return_type == NULL)
              {
                jumper_do_abort(the_jumper);
                result = NULL;
                break;
              }

            assert(type_is_valid(return_type)); /* VERIFIED */

            if (!always_pure)
              {
                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(function_type_non_pure),
                        "An overloaded routine that is not always pure is "
                        "illegal as a type.");
                type_remove_reference(return_type, the_jumper);
                result = NULL;
                break;
              }

            boolean_type = get_boolean_type();
            if (boolean_type == NULL)
              {
                jumper_do_abort(the_jumper);
                type_remove_reference(return_type, the_jumper);
                result = NULL;
                break;
              }
            assert(type_is_valid(return_type)); /* VERIFIED */
            assert(type_is_valid(boolean_type)); /* VERIFIED */
            is_subset = type_is_subset(return_type, boolean_type, &doubt,
                    &why_not, get_expression_location(the_expression),
                    the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                result = NULL;
                break;
              }
            if (doubt)
              {
                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(
                                function_type_return_type_match_indeterminate),
                        "%s wasn't able to determine whether an overloaded "
                        "routine used as a type had a boolean return type "
                        "because %s.", interpreter_name(), why_not);
                free(why_not);
                type_remove_reference(return_type, the_jumper);
                result = NULL;
                break;
              }
            if (!is_subset)
              {
                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(function_type_return_type_mismatch),
                        "An overloaded routine used as a type had a "
                        "non-boolean return type because %s.", why_not);
                free(why_not);
                type_remove_reference(return_type, the_jumper);
                result = NULL;
                break;
              }
            type_remove_reference(return_type, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                result = NULL;
                break;
              }

            if (!always_resolves)
              {
                expression_exception(the_jumper, the_expression,
                        EXCEPTION_TAG(function_type_resolution_indeterminate),
                        "An overloaded function that isn't guaranteed to "
                        "resolve with a single argument is illegal as a "
                        "type.");
                result = NULL;
                break;
              }

            if (only_possible_resolution != NULL)
              {
                assert(routine_instance_is_instantiated(
                               only_possible_resolution)); /* VERIFIED */
                assert(!(routine_instance_scope_exited(
                                 only_possible_resolution))); /* VERIFIED */
                result = get_test_routine_type(only_possible_resolution);
              }
            else
              {
                assert(routine_instance_chain_is_valid(instance_chain));
                        /* VERIFIED */
                assert(routine_instance_chain_is_valid(instance_chain));
                        /* VERIFIED */
                result = get_test_routine_chain_type(instance_chain);
              }

            if (result == NULL)
                jumper_do_abort(the_jumper);

            break;
          }
        case VK_TYPE:
          {
            result = type_value_data(name_value);
            type_add_reference(result);
            break;
          }
        default:
          {
            expression_exception(the_jumper, the_expression,
                    EXCEPTION_TAG(type_bad_value),
                    "A %s value is illegal as a type.",
                    value_kind_name(name_value));
            result = NULL;
            break;
          }
      }

    value_remove_reference(name_value, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        type_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

static rational *rational_from_integer_or_rational_value(value *the_value)
  {
    o_integer result_oi;
    rational *result;

    if (get_value_kind(the_value) == VK_RATIONAL)
      {
        result = rational_value_data(the_value);
        rational_add_reference(result);
        return result;
      }

    assert(get_value_kind(the_value) == VK_INTEGER);

    result_oi = integer_value_data(the_value);

    result = create_rational(result_oi, oi_one);

    return result;
  }

static variable_instance *variable_reference_expression_instance(
        expression *the_expression, context *the_context)
  {
    variable_declaration *declaration;
    variable_instance *instance;

    assert(the_expression != NULL);
    assert(the_context != NULL);

    assert(get_expression_kind(the_expression) == EK_VARIABLE_REFERENCE);

    declaration = variable_reference_expression_declaration(the_expression);
    assert(declaration != NULL);

    instance = find_variable_instance(the_context, declaration);
    assert(instance != NULL);

    return instance;
  }

static void export_field_to_object(object *source_object,
        size_t source_field_num, object *target_object,
        const char *target_name, jumper *the_jumper)
  {
    assert(source_object != NULL);
    assert(target_object != NULL);
    assert(target_name != NULL);
    assert(the_jumper != NULL);

    assert(!(object_is_closed(source_object))); /* VERIFIED */
    assert(!(object_is_closed(target_object))); /* VERIFIED */

    if (object_field_is_routine_chain(source_object, source_field_num))
      {
        routine_instance_chain *chain;

        assert(!(object_is_closed(source_object))); /* VERIFIED */
        chain = object_field_routine_chain(source_object, source_field_num);

        assert(!(object_is_closed(target_object))); /* VERIFIED */
        object_add_routine_chain_field(target_object, chain, target_name,
                                       the_jumper);
      }
    else
      {
        instance *the_instance;

        assert(!(object_is_closed(source_object))); /* VERIFIED */
        the_instance = object_field_instance(source_object, source_field_num);

        switch (instance_kind(the_instance))
          {
            case NK_VARIABLE:
                assert(!(object_is_closed(target_object))); /* VERIFIED */
                object_add_variable_field(target_object,
                        instance_variable_instance(the_instance), target_name,
                        the_jumper);
                break;
            case NK_ROUTINE:
                assert(!(object_is_closed(target_object))); /* VERIFIED */
                object_add_routine_field(target_object,
                        instance_routine_instance(the_instance), target_name,
                        the_jumper);
                break;
            case NK_TAGALONG:
                assert(!(object_is_closed(target_object))); /* VERIFIED */
                object_add_tagalong_field(target_object,
                        instance_tagalong_instance(the_instance), target_name,
                        the_jumper);
                break;
            case NK_LEPTON_KEY:
                assert(!(object_is_closed(target_object))); /* VERIFIED */
                object_add_lepton_key_field(target_object,
                        instance_lepton_key_instance(the_instance),
                        target_name, the_jumper);
                break;
            case NK_QUARK:
                assert(!(object_is_closed(target_object))); /* VERIFIED */
                object_add_quark_field(target_object,
                        instance_quark_instance(the_instance), target_name,
                        the_jumper);
                break;
            case NK_LOCK:
                assert(!(object_is_closed(target_object))); /* VERIFIED */
                object_add_lock_field(target_object,
                        instance_lock_instance(the_instance), target_name,
                        the_jumper);
                break;
            case NK_JUMP_TARGET:
                assert(FALSE);
            default:
                assert(FALSE);
          }
      }
  }

static type *check_and_get_lepton_type(lepton_key_instance *key,
        size_t field_count, type **field_types, const char **field_names,
        boolean extra_fields_allowed, const source_location *location,
        jumper *the_jumper)
  {
    type *result;

    assert(key != NULL);
    assert(the_jumper != NULL);

    if (!(lepton_key_instance_is_instantiated(key)))
      {
        lepton_key_declaration *declaration;
        const char *key_name;

        declaration = lepton_key_instance_declaration(key);
        assert(declaration != NULL);
        key_name = lepton_key_declaration_name(declaration);
        location_exception(the_jumper, location,
                EXCEPTION_TAG(lepton_key_uninstantiated),
                "%s%s%s was used to create a type before it was instantiated "
                "by executing its declaration.",
                ((key_name == NULL) ? "A lepton key" : "Lepton key `"),
                ((key_name == NULL) ? "" : key_name),
                ((key_name == NULL) ? "" : "'"));
        assert(!(jumper_flowing_forward(the_jumper))); /* VERIFIED */
        return NULL;
      }

    if (lepton_key_instance_scope_exited(key))
      {
        lepton_key_declaration *declaration;
        const char *key_name;

        declaration = lepton_key_instance_declaration(key);
        assert(declaration != NULL);
        key_name = lepton_key_declaration_name(declaration);
        location_exception(the_jumper, location,
                EXCEPTION_TAG(lepton_key_deallocated),
                "%s%s%s was used to create a type after it had been "
                "deallocated.",
                ((key_name == NULL) ? "A lepton key" : "Lepton key `"),
                ((key_name == NULL) ? "" : key_name),
                ((key_name == NULL) ? "" : "'"));
        assert(!(jumper_flowing_forward(the_jumper))); /* VERIFIED */
        return NULL;
      }

    assert(lepton_key_instance_is_instantiated(key)); /* VERIFIED */
    assert(!(lepton_key_instance_scope_exited(key))); /* VERIFIED */
    result = get_lepton_type(key, field_count, field_types, field_names,
                             extra_fields_allowed);
    assert((result == NULL) || type_is_valid(result)); /* VERIFIED */
    if (result == NULL)
        jumper_do_abort(the_jumper);
    assert((!(jumper_flowing_forward(the_jumper))) || type_is_valid(result));
            /* VERIFIED */
    return result;
  }

static value *add_slot_location_and_integer(slot_location *the_slot,
        o_integer the_oi, const source_location *location, jumper *the_jumper)
  {
    slot_location *base;
    lookup_actual_arguments *old_actuals;
    lookup_actual_arguments *new_actuals;
    slot_location *new_slot;
    value *result;

    assert(the_slot != NULL);
    assert(!(oi_out_of_memory(the_oi)));
    assert(the_jumper != NULL);

    if (oi_structural_order(oi_zero, the_oi) == 0)
      {
        value *result;

        result = create_slot_location_value(the_slot);
        if (result == NULL)
            jumper_do_abort(the_jumper);
        return result;
      }

    if (get_slot_location_kind(the_slot) != SLK_LOOKUP)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(pointer_integer_addition_non_lookup),
                "When evaluating the addition of a pointer and a non-zero "
                "integer, the pointer was not a lookup.");
        return NULL;
      }

    base = lookup_slot_location_base(the_slot);
    assert(base != NULL);

    old_actuals = lookup_slot_location_actuals(the_slot);
    assert(old_actuals != NULL);

    new_actuals = lookup_actual_arguments_add(old_actuals, the_oi, location,
                                              the_jumper);
    if (new_actuals == NULL)
        return NULL;

    new_slot = create_lookup_slot_location(base, new_actuals,
            slot_location_overload_base(the_slot), location, the_jumper);
    if (new_slot == NULL)
      {
        assert(!(jumper_flowing_forward(the_jumper)));
        return NULL;
      }

    assert(jumper_flowing_forward(the_jumper));

    result = create_slot_location_value(new_slot);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    slot_location_remove_reference(new_slot, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        value_remove_reference(result, the_jumper);
        return NULL;
      }
    return result;
  }

static void do_variable_lock_grabbing(variable_instance *variable_instance,
        const source_location *location, jumper *the_jumper)
  {
    lock_chain *the_lock_chain;

    assert(variable_instance != NULL);

    assert(variable_instance_is_instantiated(variable_instance));
            /* VERIFIED */
    assert(!(variable_instance_scope_exited(variable_instance)));
            /* VERIFIED */

    the_lock_chain = variable_instance_lock_chain(variable_instance);

    if (the_lock_chain != NULL)
        lock_chain_grab(the_lock_chain, location, the_jumper);
  }

static void do_variable_lock_releasing(variable_instance *variable_instance,
        const source_location *location, jumper *the_jumper)
  {
    lock_chain *the_lock_chain;

    assert(variable_instance != NULL);
    assert(the_jumper != NULL);

    assert(variable_instance_is_instantiated(variable_instance));
            /* VERIFIED */
    assert(!(variable_instance_scope_exited(variable_instance)));
            /* VERIFIED */

    the_lock_chain = variable_instance_lock_chain(variable_instance);

    if (the_lock_chain != NULL)
        lock_chain_release(the_lock_chain, location, the_jumper);
  }

static void set_variable_value_with_locking(
        variable_instance *variable_instance, value *new_value,
        const source_location *location, jumper *the_jumper)
  {
    value *old_value;

    assert(variable_instance != NULL);
    assert(new_value != NULL);

    assert(variable_instance_is_instantiated(variable_instance));
            /* VERIFIED */
    assert(!(variable_instance_scope_exited(variable_instance)));
            /* VERIFIED */

    do_variable_lock_grabbing(variable_instance, location, the_jumper);

    assert(variable_instance_is_instantiated(variable_instance));
            /* VERIFIED */
    assert(!(variable_instance_scope_exited(variable_instance)));
            /* VERIFIED */
    old_value = variable_instance_value(variable_instance);

    assert(!(variable_instance_scope_exited(variable_instance)));
            /* VERIFIED */
    set_variable_instance_value(variable_instance, new_value, the_jumper);

    assert(variable_instance_is_instantiated(variable_instance));
            /* VERIFIED */
    assert(!(variable_instance_scope_exited(variable_instance)));
            /* VERIFIED */
    do_variable_lock_releasing(variable_instance, location, the_jumper);

    if (old_value != NULL)
        value_remove_reference(old_value, the_jumper);
  }

static lock_chain *declaration_lock_chain(object *base_object,
        expression *lock_expression, context *the_context, jumper *the_jumper,
        const char *declaration_kind_name, static_exception_tag *not_lock_tag)
  {
    lock_chain *the_lock_chain;
    value *lock_value;
    lock_instance *the_lock_instance;
    lock_chain *new_lock_chain;

    assert((base_object == NULL) || !(object_is_closed(base_object)));
            /* VERIFIED */
    if (base_object == NULL)
        the_lock_chain = NULL;
    else
        the_lock_chain = object_lock_chain(base_object);

    if (lock_expression == NULL)
        return the_lock_chain;

    lock_value = evaluate_expression(lock_expression, the_context, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(lock_value == NULL);
        if (the_lock_chain != NULL)
            lock_chain_remove_reference(the_lock_chain, the_jumper);
        return NULL;
      }

    assert(lock_value != NULL);

    if (get_value_kind(lock_value) != VK_LOCK)
      {
        expression_exception(the_jumper, lock_expression, not_lock_tag,
                "The `single' expression of a %s declaration evaluated to "
                "something other than a lock value.", declaration_kind_name);
        value_remove_reference(lock_value, the_jumper);
        if (the_lock_chain != NULL)
            lock_chain_remove_reference(the_lock_chain, the_jumper);
        return NULL;
      }

    the_lock_instance = lock_value_data(lock_value);
    assert(the_lock_instance != NULL);

    new_lock_chain = create_lock_chain(the_lock_instance, the_lock_chain);
    if (new_lock_chain == NULL)
      {
        jumper_do_abort(the_jumper);
        new_lock_chain = NULL;
      }
    value_remove_reference(lock_value, the_jumper);
    if (the_lock_chain != NULL)
        lock_chain_remove_reference(the_lock_chain, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (new_lock_chain != NULL))
      {
        lock_chain_remove_reference(new_lock_chain, the_jumper);
        new_lock_chain = NULL;
      }

    return new_lock_chain;
  }

static verdict try_scoped_overloading(
        routine_declaration_chain *overload_chain,
        statement *overload_use_statement, size_t overload_used_for_number,
        value **result, size_t argument_count, value **arguments,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *call_base;
    verdict the_verdict;

    assert((argument_count == 0) || (arguments != NULL));
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    if (result != NULL)
        *result = NULL;

    if (overload_chain != NULL)
      {
        routine_instance_chain *instance_chain;

        instance_chain = routine_declaration_chain_to_routine_instance_chain(
                overload_chain, the_context, the_jumper);
        if (instance_chain == NULL)
          {
            assert(!(jumper_flowing_forward(the_jumper)));
            return MISSION_ACCOMPLISHED;
          }
        assert(jumper_flowing_forward(the_jumper));

        call_base = create_routine_chain_value(instance_chain);
        if (call_base == NULL)
          {
            jumper_do_abort(the_jumper);
            routine_instance_chain_remove_reference(instance_chain,
                                                    the_jumper);
            return MISSION_ACCOMPLISHED;
          }

        routine_instance_chain_remove_reference(instance_chain, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(call_base, the_jumper);
            return MISSION_ACCOMPLISHED;
          }
      }
    else if (overload_use_statement != NULL)
      {
        instance *test_instance;
        routine_instance_chain *test_chain;
        jump_target *test_target;

        find_instance_from_use_statement(overload_use_statement,
                overload_used_for_number, the_context, FALSE, &test_instance,
                &test_chain, &test_target, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return MISSION_ACCOMPLISHED;

        if ((test_instance != NULL) &&
            (instance_kind(test_instance) == NK_ROUTINE))
          {
            call_base = create_routine_value(
                    instance_routine_instance(test_instance));
            if (call_base == NULL)
              {
                jumper_do_abort(the_jumper);
                return MISSION_ACCOMPLISHED;
              }
          }
        else if (test_chain != NULL)
          {
            call_base = create_routine_chain_value(test_chain);
            if (call_base == NULL)
              {
                jumper_do_abort(the_jumper);
                routine_instance_chain_remove_reference(test_chain,
                                                        the_jumper);
                return MISSION_ACCOMPLISHED;
              }
            routine_instance_chain_remove_reference(test_chain, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(call_base, the_jumper);
                return MISSION_ACCOMPLISHED;
              }
          }
        else
          {
            return MISSION_FAILED;
          }
      }
    else
      {
        return MISSION_FAILED;
      }

    the_verdict = try_overloading_from_call_base(call_base, result,
            argument_count, arguments, NULL, the_jumper, location);
    value_remove_reference(call_base, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return MISSION_ACCOMPLISHED;
    return the_verdict;
  }

static verdict try_scoped_field_overloading(value *overload_base,
        value **result, value *base_value, const char *field_name,
        value *new_value, jumper *the_jumper,
        const source_location *the_source_location)
  {
    value *field_name_value;
    value *arguments[3];
    const char *argument_names[3];
    verdict the_verdict;

    if (overload_base == NULL)
        return MISSION_FAILED;

    field_name_value = create_string_value(field_name);
    if (field_name_value == NULL)
      {
        jumper_do_abort(the_jumper);
        return MISSION_ACCOMPLISHED;
      }

    arguments[0] = base_value;
    argument_names[0] = NULL;
    arguments[1] = field_name_value;
    argument_names[1] = "field";
    arguments[2] = new_value;
    argument_names[2] = NULL;
    the_verdict = try_overloading_from_call_base(overload_base, result,
            ((new_value == NULL) ? 2 : 3), &(arguments[0]),
            &(argument_names[0]), the_jumper, the_source_location);
    value_remove_reference(field_name_value, the_jumper);
    return the_verdict;
  }

static verdict try_scoped_dereference_overloading(value *overload_base,
        value **result, value *base_value, value *new_value,
        jumper *the_jumper, const source_location *the_source_location)
  {
    value *arguments[3];
    const char *argument_names[3];

    if (overload_base == NULL)
        return MISSION_FAILED;

    arguments[0] = base_value;
    argument_names[0] = "base";
    arguments[1] = new_value;
    argument_names[1] = NULL;
    return try_overloading_from_call_base(overload_base, result,
            ((new_value == NULL) ? 1 : 2), &(arguments[0]),
            &(argument_names[0]), the_jumper, the_source_location);
  }

static value *find_overload_operator(value *base_value,
        const char *routine_name, jumper *the_jumper,
        const source_location *location)
  {
    assert(base_value != NULL);
    assert(routine_name != NULL);
    assert(the_jumper != NULL);

    switch (get_value_kind(base_value))
      {
        case VK_SEMI_LABELED_VALUE_LIST:
        case VK_SEMI_LABELED_MULTI_SET:
        case VK_LEPTON:
          {
            value *call_base;

            call_base = value_get_field(routine_name, base_value);

            if (call_base != NULL)
                value_add_reference(call_base);

            return call_base;
          }
        case VK_OBJECT:
          {
            object *base_object;
            size_t field_num;

            base_object = object_value_data(base_value);
            assert(base_object != NULL);

            object_check_validity(base_object, location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return NULL;

            assert(!(object_is_closed(base_object))); /* VERIFIED */
            field_num = object_field_lookup(base_object, routine_name);
            if (field_num >= object_field_count(base_object))
                return NULL;

            assert(!(object_is_closed(base_object))); /* VERIFIED */
            return object_field_read_value(base_object, field_num, location,
                                           the_jumper);
          }
        default:
          {
            return NULL;
          }
      }
  }

static value *overload_base_for_expression(expression *the_expression,
        context *the_context, jumper *the_jumper)
  {
    assert(the_expression != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    if (expression_overload_chain(the_expression) != NULL)
      {
        routine_instance_chain *instance_chain;
        value *result;

        instance_chain = routine_declaration_chain_to_routine_instance_chain(
                expression_overload_chain(the_expression), the_context,
                the_jumper);
        if (instance_chain == NULL)
          {
            assert(!(jumper_flowing_forward(the_jumper)));
            return NULL;
          }
        assert(jumper_flowing_forward(the_jumper));

        result = create_routine_chain_value(instance_chain);
        if (result == NULL)
          {
            jumper_do_abort(the_jumper);
            routine_instance_chain_remove_reference(instance_chain,
                                                    the_jumper);
            return NULL;
          }

        routine_instance_chain_remove_reference(instance_chain, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        return result;
      }

    if (expression_overload_use_statement(the_expression) != NULL)
      {
        instance *test_instance;
        routine_instance_chain *test_chain;
        jump_target *test_target;

        find_instance_from_use_statement(
                expression_overload_use_statement(the_expression),
                expression_overload_use_used_for_number(the_expression),
                the_context, FALSE, &test_instance, &test_chain, &test_target,
                get_expression_location(the_expression), the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return NULL;

        if (test_instance != NULL)
          {
            if (instance_kind(test_instance) == NK_ROUTINE)
              {
                value *result;

                result = create_routine_value(
                        instance_routine_instance(test_instance));
                if (result == NULL)
                    jumper_do_abort(the_jumper);
                return result;
              }

            if (instance_kind(test_instance) == NK_VARIABLE)
              {
                return read_variable_value(
                        instance_variable_instance(test_instance), NULL,
                        get_expression_location(the_expression), the_jumper);
              }
          }

        if (test_chain != NULL)
          {
            value *result;

            result = create_routine_chain_value(test_chain);
            if (result == NULL)
                jumper_do_abort(the_jumper);
            routine_instance_chain_remove_reference(test_chain, the_jumper);
            if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
              {
                value_remove_reference(result, the_jumper);
                return NULL;
              }
            return result;
          }
      }

    return NULL;
  }

static verdict start_simple_iteration_data(value *base_value,
        simple_iteration_data *simple_data, const char *action_description,
        const char *loop_description, jumper *the_jumper,
        const source_location *base_location)
  {
    assert(base_value != NULL);
    assert(simple_data != NULL);
    assert(action_description != NULL);
    assert(loop_description != NULL);
    assert(the_jumper != NULL);

    switch (get_value_kind(base_value))
      {
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            o_integer count_oi;

            simple_data->index_oi = oi_zero;
            oi_add_reference(simple_data->index_oi);
            assert(!(oi_out_of_memory(simple_data->index_oi)));

            oi_create_from_size_t(count_oi, value_component_count(base_value));
            if (oi_out_of_memory(count_oi))
              {
                jumper_do_abort(the_jumper);
                oi_remove_reference(simple_data->index_oi);
                return MISSION_FAILED;
              }

            oi_subtract(simple_data->max_oi, count_oi, oi_one);
            oi_remove_reference(count_oi);
            if (oi_out_of_memory(simple_data->max_oi))
              {
                jumper_do_abort(the_jumper);
                oi_remove_reference(simple_data->index_oi);
                return MISSION_FAILED;
              }

            break;
          }
        case VK_MAP:
          {
            boolean doubt;
            boolean is_array;
            boolean min_doubt;
            boolean max_doubt;

            assert(value_is_valid_except_map_targets(base_value));
                    /* VERIFIED */

            assert(value_is_valid_except_map_targets(base_value));
                    /* VERIFIED */
            is_array = map_value_is_array(base_value, &doubt, base_location,
                                          the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return MISSION_FAILED;

            if (doubt)
              {
                location_exception(the_jumper, base_location,
                        EXCEPTION_TAG(iteration_base_array_indeterminate),
                        "While %s %s, %s was unable to determine whether the "
                        "base value was a map value whose keys are all "
                        "integers.", action_description, loop_description,
                        interpreter_name());
                return MISSION_FAILED;
              }

            if (!is_array)
              {
                location_exception(the_jumper, base_location,
                        EXCEPTION_TAG(iteration_bad_base),
                        "The base argument to %s evaluated to a map value "
                        "whose keys are not all integers.", loop_description);
                return MISSION_FAILED;
              }

            map_value_integer_key_bounds(base_value, &(simple_data->index_oi),
                    &(simple_data->max_oi), &min_doubt, &max_doubt,
                    base_location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
                return MISSION_FAILED;

            if (min_doubt || max_doubt)
              {
                if (!(oi_out_of_memory(simple_data->index_oi)))
                    oi_remove_reference(simple_data->index_oi);
                if (!(oi_out_of_memory(simple_data->max_oi)))
                    oi_remove_reference(simple_data->max_oi);
                location_exception(the_jumper, base_location,
                        EXCEPTION_TAG(iteration_base_array_indeterminate),
                        "While %s %s, %s was unable to determine the %s index "
                        "of the base array.", action_description,
                        loop_description, interpreter_name(),
                        (min_doubt ? "minimum" : "maximum"));
                return MISSION_FAILED;
              }

            assert(!(oi_out_of_memory(simple_data->index_oi)));
            assert(!(oi_out_of_memory(simple_data->max_oi)));

            break;
          }
        default:
          {
            return MISSION_FAILED;
          }
      }

    simple_data->base_value = base_value;
    simple_data->base_location = base_location;
    simple_data->step_oi = oi_one;
    assert(!(oi_out_of_memory(simple_data->step_oi)));
    simple_data->action_description = action_description;
    simple_data->loop_description = loop_description;

    return MISSION_ACCOMPLISHED;
  }

static boolean simple_iteration_data_is_done(simple_iteration_data *data)
  {
    assert(data != NULL);

    return oi_less_than(data->max_oi, data->index_oi);
  }

static value *simple_iteration_data_current(simple_iteration_data *data,
        jumper *the_jumper, const source_location *location)
  {
    assert(data != NULL);
    assert(the_jumper != NULL);

    switch (get_value_kind(data->base_value))
      {
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            size_t index_size_t;
            verdict the_verdict;
            value *result;

            assert(oi_kind(data->index_oi) == IIK_FINITE);
            assert(!(oi_is_negative(data->index_oi)));
            the_verdict =
                    oi_magnitude_to_size_t(data->index_oi, &index_size_t);
            assert(the_verdict == MISSION_ACCOMPLISHED);

            result = value_component_value(data->base_value, index_size_t);
            value_add_reference(result);

            return result;
          }
        case VK_MAP:
          {
            value *key_value;
            boolean doubt;
            value *result;

            key_value = create_integer_value(data->index_oi);
            if (key_value == NULL)
              {
                jumper_do_abort(the_jumper);
                clean_up_simple_iteration_data(data);
                value_remove_reference(data->base_value, the_jumper);
                return NULL;
              }

            check_value_validity_except_map_targets(data->base_value,
                    data->base_location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(key_value, the_jumper);
                clean_up_simple_iteration_data(data);
                value_remove_reference(data->base_value, the_jumper);
                return NULL;
              }

            assert(map_value_all_keys_are_valid(data->base_value));
                    /* VERIFIED */
            assert(value_is_valid(key_value)); /* VERIFIED */
            result = map_value_lookup(data->base_value, key_value, &doubt,
                                      location, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(key_value, the_jumper);
                clean_up_simple_iteration_data(data);
                value_remove_reference(data->base_value, the_jumper);
                return NULL;
              }
            assert(!doubt);
            if (result == NULL)
              {
                location_exception(the_jumper, data->base_location,
                        EXCEPTION_TAG(iteration_bad_base),
                        "The base argument to %s evaluated to a sparse array.",
                        data->loop_description);
                value_remove_reference(key_value, the_jumper);
                clean_up_simple_iteration_data(data);
                value_remove_reference(data->base_value, the_jumper);
                return NULL;
              }

            value_remove_reference(key_value, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                clean_up_simple_iteration_data(data);
                value_remove_reference(data->base_value, the_jumper);
                return NULL;
              }

            value_add_reference(result);

            return result;
          }
        default:
          {
            assert(FALSE);
            return NULL;
          }
      }
  }

static void simple_iteration_data_step(simple_iteration_data *data,
                                       jumper *the_jumper)
  {
    o_integer new_index_oi;

    assert(data != NULL);

    oi_add(new_index_oi, data->index_oi, data->step_oi);
    oi_remove_reference(data->index_oi);
    if (oi_out_of_memory(new_index_oi))
      {
        jumper_do_abort(the_jumper);
        oi_remove_reference(data->max_oi);
        value_remove_reference(data->base_value, the_jumper);
        value_remove_reference(data->base_value, the_jumper);
        return;
      }

    data->index_oi = new_index_oi;
  }

static void clean_up_simple_iteration_data(simple_iteration_data *data)
  {
    assert(data != NULL);

    oi_remove_reference(data->max_oi);
    oi_remove_reference(data->index_oi);
  }

static variable_instance *immutable_for_value(value *the_value,
        const char *name, context *the_context,
        const source_location *location, jumper *the_jumper)
  {
    type_expression *the_type_expression;
    variable_declaration *var_declaration;
    declaration *the_declaration;
    variable_instance *result;
    verdict the_verdict;

    assert(the_value != NULL);

    the_type_expression = create_constant_type_expression(get_anything_type());
    if (the_type_expression == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    var_declaration = create_variable_declaration(the_type_expression, NULL,
                                                  FALSE, TRUE, NULL);
    if (var_declaration == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    the_declaration = create_declaration_for_variable(name, FALSE, FALSE, TRUE,
            var_declaration, location);
    if (the_declaration == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_variable_instance(var_declaration,
                                      jumper_purity_level(the_jumper), NULL);
    variable_declaration_remove_reference(var_declaration);
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    set_variable_instance_type(result, get_anything_type(), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        variable_instance_remove_reference(result, the_jumper);
        return NULL;
      }

    set_variable_instance_value(result, the_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        variable_instance_remove_reference(result, the_jumper);
        return NULL;
      }

    set_variable_instance_instantiated(result);

    the_verdict = context_add_extra_instance(the_context,
            variable_instance_instance(result));
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        variable_instance_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

static void comprehend_cleaner(void *hook, jumper *the_jumper)
  {
    assert(hook != NULL);

    free(hook);
  }

static value *comprehend_transform_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *element_value;
    comprehend_data *hook_data;
    jump_target *break_target;
    jump_target *continue_target;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 4);
    element_value = value_component_value(all_arguments_value, 0);
    hook_data = (comprehend_data *)(object_hook(
            object_value_data(value_component_value(all_arguments_value, 1))));
    break_target = jump_target_value_data(
            value_component_value(all_arguments_value, 2));
    continue_target = jump_target_value_data(
            value_component_value(all_arguments_value, 3));
    assert(hook_data != NULL);

    assert(element_value != NULL);
    assert(variable_instance_is_instantiated(hook_data->element_instance));
            /* VERIFIED */
    assert(!(variable_instance_scope_exited(hook_data->element_instance)));
            /* VERIFIED */
    assert(variable_instance_lock_chain(hook_data->element_instance) == NULL);
    set_variable_instance_value(hook_data->element_instance, element_value,
                                the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    if (hook_data->filter != NULL)
      {
        value *filter_result;

        filter_result = evaluate_expression(hook_data->filter,
                                            hook_data->context, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(filter_result == NULL);
            goto do_jump;
          }

        assert(filter_result != NULL);
        switch (get_value_kind(filter_result))
          {
            case VK_TRUE:
                value_remove_reference(filter_result, the_jumper);
                assert(jumper_flowing_forward(the_jumper));
                break;
            case VK_FALSE:
                value_remove_reference(filter_result, the_jumper);
                assert(jumper_flowing_forward(the_jumper));
                assert(!(jump_target_scope_exited(continue_target)));
                        /* VERIFIED */
                jumper_set_target(the_jumper, continue_target);
                return NULL;
            default:
                expression_exception(the_jumper, hook_data->filter,
                        EXCEPTION_TAG(comprehend_bad_test),
                        "The test argument to a comprehend expression "
                        "evaluated to something other than a boolean value.");
                value_remove_reference(filter_result, the_jumper);
                return NULL;
          }
      }

    result = evaluate_expression(hook_data->body, hook_data->context,
                                 the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(result == NULL);

      do_jump:
        if (jumper_target(the_jumper) ==
            loop_context_continue_target(hook_data->context))
          {
            jumper_reached_target(the_jumper);
            assert(!(jump_target_scope_exited(continue_target)));
                    /* VERIFIED */
            jumper_set_target(the_jumper, continue_target);
            return NULL;
          }
        else if (jumper_target(the_jumper) ==
                 loop_context_break_target(hook_data->context))
          {
            jumper_reached_target(the_jumper);
            assert(!(jump_target_scope_exited(break_target))); /* VERIFIED */
            jumper_set_target(the_jumper, break_target);
          }

        return NULL;
      }

    assert(result != NULL);
    return result;
  }

static formal_arguments *create_formals(size_t count, const char **names,
                                        const source_location *location)
  {
    formal_arguments *result;
    size_t formal_num;

    result = create_formal_arguments();
    if (result == NULL)
        return NULL;

    for (formal_num = 0; formal_num < count; ++formal_num)
      {
        type_expression *this_type;
        variable_declaration *the_variable_declaration;
        declaration *the_declaration;
        verdict the_verdict;

        this_type = create_constant_type_expression(get_anything_type());
        if (this_type == NULL)
          {
            delete_formal_arguments(result);
            return NULL;
          }

        the_variable_declaration = create_variable_declaration(this_type, NULL,
                FALSE, TRUE, NULL);
        if (the_variable_declaration == NULL)
          {
            delete_formal_arguments(result);
            return NULL;
          }

        the_declaration = create_declaration_for_variable(names[formal_num],
                FALSE, FALSE, TRUE, the_variable_declaration, location);
        if (the_declaration == NULL)
          {
            delete_formal_arguments(result);
            return NULL;
          }

        the_verdict = add_formal_parameter(result, the_declaration, NULL);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            variable_declaration_remove_reference(the_variable_declaration);
            delete_formal_arguments(result);
            return NULL;
          }
      }

    return result;
  }
