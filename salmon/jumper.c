/* file "jumper.c" */

/*
 *  This file contains the implementation of the jumper module.
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
#include <stdarg.h>
#include "c_foundations/memory_allocation.h"
#include "c_foundations/basic.h"
#include "c_foundations/trace.h"
#include "c_foundations/buffer_print.h"
#include "c_foundations/auto_array.h"
#include "c_foundations/auto_array_implementation.h"
#include "jumper.h"
#include "jump_target.h"
#include "source_location.h"
#include "expression.h"
#include "statement.h"
#include "type_expression.h"
#include "execute.h"
#include "purity_level.h"
#include "type.h"
#include "driver.h"
#include "utility.h"
#include "platform_dependent.h"


AUTO_ARRAY(value_aa, value *);


typedef struct handler_stack handler_stack;

struct handler_stack
  {
    jumper *jumper;
    boolean is_catcher;
    union
      {
        struct
          {
            jump_target *target;
            type *tag_type;
          } catcher;
        value *handler;
      } u;
    boolean in_handler;
    handler_stack *next;
  };

typedef struct
  {
    routine_instance *callee;
    const source_location *call_site;
    CLOCK_T non_local_time;
  } call_item;

AUTO_ARRAY(call_item_aa, call_item);

struct jumper
  {
    boolean aborting;
    jump_target *target;
    handler_stack *handler_stack;
    object *standard_library_object;
    lepton_key_instance *region_key;
    lepton_key_instance *exception_key;
    value *exception_information;
    value_aa released_exception_information;
    purity_level *purity_level;
    salmon_thread *thread;
    jumper *parent;
    tracer *tracer;
    call_item_aa *call_stack;
    boolean have_start_statement_execution_watcher;
    boolean have_start_expression_evaluation_watcher;
  };


AUTO_ARRAY_IMPLEMENTATION(call_item_aa, call_item, 0);


static jumper *create_jumper_internal(jumper *parent_jumper,
        tracer *the_tracer, purity_level *the_purity_level,
        salmon_thread *the_salmon_thread);
static void handle_exception(jumper *the_jumper,
        handler_stack *the_handler_stack, value *exception_value);
static value *build_exception_value(jumper *the_jumper,
        const source_location *location, static_exception_tag *tag,
        const char *message, va_list ap);
static void add_exception_stack_info(value *exception_value,
        lepton_key_instance *region_key, jumper *the_jumper);
static void add_source_location_field(value *base,
        const source_location *location, const char *field_name,
        lepton_key_instance *region_key, jumper *the_jumper);
static size_t value_component_to_size_t(value *base_value,
                                        const char *field_name);
static verdict append_exception_information(jumper *the_jumper,
                                            value *new_information);
static void jumper_throw_exception_internal(jumper *the_jumper,
        value *exception_value, value *other_value,
        lepton_key_instance *region_key);
static void unhandled_exception(value *exception_value);


extern jumper *create_root_jumper(salmon_thread *the_salmon_thread,
                                  tracer *the_tracer)
  {
    purity_level *the_purity_level;
    jumper *result;

    assert(the_salmon_thread != NULL);

    the_purity_level = create_purity_level(NULL);
    if (the_purity_level == NULL)
        return NULL;

    result = create_jumper_internal(NULL, the_tracer, the_purity_level,
                                    the_salmon_thread);

    return result;
  }

extern jumper *create_sub_thread_jumper(jumper *parent_jumper,
                                        salmon_thread *the_salmon_thread)
  {
    purity_level *the_purity_level;
    jumper *result;

    assert(parent_jumper != NULL);
    assert_is_malloced_block_with_exact_size(parent_jumper, sizeof(jumper));
    assert(the_salmon_thread != NULL);

    assert(parent_jumper->thread != NULL);

    the_purity_level = parent_jumper->purity_level;
    purity_level_add_reference(the_purity_level);

    result = create_jumper_internal(NULL, parent_jumper->tracer,
                                    the_purity_level, the_salmon_thread);
    if (result != NULL)
      {
        result->standard_library_object =
                parent_jumper->standard_library_object;
        result->region_key = parent_jumper->region_key;
        result->exception_key = parent_jumper->exception_key;
      }

    return result;
  }

extern jumper *create_sub_jumper(jumper *parent_jumper)
  {
    purity_level *the_purity_level;
    jumper *result;

    assert(parent_jumper != NULL);
    assert_is_malloced_block_with_exact_size(parent_jumper, sizeof(jumper));

    assert(parent_jumper->thread != NULL);

    the_purity_level = parent_jumper->purity_level;
    purity_level_add_reference(the_purity_level);

    result = create_jumper_internal(parent_jumper, parent_jumper->tracer,
                                    the_purity_level, parent_jumper->thread);

    return result;
  }

extern jumper *create_purer_jumper(jumper *parent_jumper)
  {
    purity_level *the_purity_level;
    jumper *result;

    assert(parent_jumper != NULL);
    assert_is_malloced_block_with_exact_size(parent_jumper, sizeof(jumper));

    assert(parent_jumper->thread != NULL);

    the_purity_level = create_purity_level(parent_jumper->purity_level);
    if (the_purity_level == NULL)
        return NULL;

    result = create_jumper_internal(parent_jumper, parent_jumper->tracer,
                                    the_purity_level, parent_jumper->thread);

    return result;
  }

extern jumper *create_test_jumper(void)
  {
    jumper *result;

    result = MALLOC_ONE_OBJECT(jumper);
    if (result == NULL)
        return NULL;

    result->aborting = FALSE;
    result->target = NULL;
    result->thread = NULL;

    return result;
  }

extern void delete_jumper(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));

    if (the_jumper->thread == NULL)
      {
        free(the_jumper);
        return;
      }

    if (the_jumper->target != NULL)
        jump_target_remove_reference(the_jumper->target);

    if (the_jumper->parent == NULL)
      {
        assert(the_jumper->handler_stack == NULL);

        assert(the_jumper->call_stack->element_count == 0);
        free(the_jumper->call_stack->array);
        free(the_jumper->call_stack);

        set_salmon_thread_current_jumper(the_jumper->thread, NULL);
      }
    else
      {
        assert(the_jumper->handler_stack == the_jumper->parent->handler_stack);

        assert(the_jumper->call_stack == the_jumper->parent->call_stack);

        set_salmon_thread_current_jumper(the_jumper->thread,
                                         the_jumper->parent);
      }

    if (the_jumper->exception_information != NULL)
      {
        value *exception_information;
        jumper *parent;
        size_t component_count;
        size_t component_num;

        exception_information = the_jumper->exception_information;
        assert(exception_information != NULL);

        parent = the_jumper->parent;
        assert(parent != NULL);

        assert(get_value_kind(exception_information) ==
               VK_SEMI_LABELED_VALUE_LIST);

        component_count = value_component_count(exception_information);

        for (component_num = 0; component_num < component_count;
             ++component_num)
          {
            verdict the_verdict;

            the_verdict = append_exception_information(parent,
                    value_component_value(exception_information,
                                          component_num));
            if (the_verdict != MISSION_ACCOMPLISHED)
                break;
          }

        value_remove_reference(exception_information, the_jumper->parent);
      }

    assert(the_jumper->released_exception_information.element_count == 0);
    free(the_jumper->released_exception_information.array);

    purity_level_remove_reference(the_jumper->purity_level);
    salmon_thread_remove_reference(the_jumper->thread);

    free(the_jumper);
  }

extern tracer *jumper_tracer(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    return the_jumper->tracer;
  }

extern object *jumper_standard_library_object(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    return the_jumper->standard_library_object;
  }

extern lepton_key_instance *jumper_region_key(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    return the_jumper->region_key;
  }

extern lepton_key_instance *jumper_exception_key(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    return the_jumper->exception_key;
  }

extern purity_level *jumper_purity_level(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    return the_jumper->purity_level;
  }

extern salmon_thread *jumper_thread(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));

    return the_jumper->thread;
  }

extern size_t jumper_call_stack_size(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    assert(the_jumper->call_stack != NULL);
    return the_jumper->call_stack->element_count;
  }

extern routine_instance *jumper_call_stack_callee(jumper *the_jumper,
                                                  size_t call_num)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    assert(the_jumper->call_stack != NULL);
    assert(call_num < the_jumper->call_stack->element_count);
    return the_jumper->call_stack->array[call_num].callee;
  }

extern const source_location *jumper_call_stack_site(jumper *the_jumper,
                                                     size_t call_num)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    assert(the_jumper->call_stack != NULL);
    assert(call_num < the_jumper->call_stack->element_count);
    return the_jumper->call_stack->array[call_num].call_site;
  }

extern CLOCK_T jumper_call_stack_non_local_time(jumper *the_jumper,
                                                size_t call_num)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    assert(the_jumper->call_stack != NULL);
    assert(call_num < the_jumper->call_stack->element_count);
    return the_jumper->call_stack->array[call_num].non_local_time;
  }

extern void jumper_call_stack_add_non_local_time(jumper *the_jumper,
        size_t call_num, CLOCK_T to_add)
  {
    CLOCK_T new_value;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    assert(the_jumper->call_stack != NULL);
    assert(call_num < the_jumper->call_stack->element_count);
    CLOCK_ADD(new_value,
              the_jumper->call_stack->array[call_num].non_local_time, to_add);
    the_jumper->call_stack->array[call_num].non_local_time = new_value;
  }

extern routine_instance *jumper_call_stack_back_callee(jumper *the_jumper,
                                                       size_t call_num)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    assert(the_jumper->call_stack != NULL);
    assert(call_num < the_jumper->call_stack->element_count);
    return the_jumper->call_stack->array[
            the_jumper->call_stack->element_count - (call_num + 1)].callee;
  }

extern const source_location *jumper_call_stack_back_site(jumper *the_jumper,
                                                          size_t call_num)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    assert(the_jumper->call_stack != NULL);
    assert(call_num < the_jumper->call_stack->element_count);

    return the_jumper->call_stack->array[
            the_jumper->call_stack->element_count - (call_num + 1)].call_site;
  }

extern CLOCK_T jumper_call_stack_back_non_local_time(jumper *the_jumper,
                                                     size_t call_num)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    assert(the_jumper->call_stack != NULL);
    assert(call_num < the_jumper->call_stack->element_count);

    return the_jumper->call_stack->array[
            the_jumper->call_stack->element_count - (call_num + 1)].
                    non_local_time;
  }

extern void jumper_call_stack_back_add_non_local_time(jumper *the_jumper,
        size_t call_num, CLOCK_T to_add)
  {
    size_t from_start;
    CLOCK_T new_value;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    assert(the_jumper->call_stack != NULL);
    assert(call_num < the_jumper->call_stack->element_count);

    from_start = (the_jumper->call_stack->element_count - (call_num + 1));
    CLOCK_ADD(new_value,
            the_jumper->call_stack->array[from_start].non_local_time, to_add);
    the_jumper->call_stack->array[from_start].non_local_time = new_value;
  }

extern boolean have_start_statement_execution_watcher(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    return the_jumper->have_start_statement_execution_watcher;
  }

extern boolean have_start_expression_evaluation_watcher(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    return the_jumper->have_start_expression_evaluation_watcher;
  }

extern void set_have_start_statement_execution_watcher(jumper *the_jumper,
                                                       boolean have_it)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    the_jumper->have_start_statement_execution_watcher = have_it;
  }

extern void set_have_start_expression_evaluation_watcher(jumper *the_jumper,
                                                         boolean have_it)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    the_jumper->have_start_expression_evaluation_watcher = have_it;
  }

extern void jumper_set_standard_library_object(jumper *the_jumper,
                                               object *standard_library_object)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    assert(the_jumper->standard_library_object == NULL);
    the_jumper->standard_library_object = standard_library_object;
  }

extern void jumper_set_region_key(jumper *the_jumper,
                                  lepton_key_instance *region_key)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    assert(the_jumper->region_key == NULL);
    the_jumper->region_key = region_key;
  }

extern void jumper_set_exception_key(jumper *the_jumper,
                                     lepton_key_instance *exception_key)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    assert(the_jumper->exception_key == NULL);
    the_jumper->exception_key = exception_key;
  }

extern boolean jumper_flowing_forward(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));

    return ((the_jumper->target == NULL) && !(the_jumper->aborting));
  }

extern jump_target *jumper_target(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));

    return the_jumper->target;
  }

extern void location_exception(jumper *the_jumper,
        const source_location *location, static_exception_tag *tag,
        const char *message, ...)
  {
    va_list ap;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);
    assert(tag != NULL);
    assert(message != NULL);

    va_start(ap, message);
    vlocation_exception(the_jumper, location, tag, message, ap);
    va_end(ap);
  }

extern void vlocation_exception(jumper *the_jumper,
        const source_location *location, static_exception_tag *tag,
        const char *message, va_list ap)
  {
    handler_stack *the_handler_stack;
    value *exception_value;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);
    assert(tag != NULL);
    assert(message != NULL);

    if (the_jumper->aborting)
      {
        vlocation_error(location, message, ap);
        return;
      }

    the_handler_stack = the_jumper->handler_stack;

    while (TRUE)
      {
        if (the_handler_stack == NULL)
          {
            vlocation_error(location, message, ap);
            jumper_do_abort(the_jumper);
            return;
          }

        if (!(the_handler_stack->in_handler))
            break;

        the_handler_stack = the_handler_stack->next;
      }

    assert(!(the_handler_stack->in_handler));

    the_handler_stack->in_handler = TRUE;

    exception_value =
            build_exception_value(the_jumper, location, tag, message, ap);
    if (exception_value == NULL)
      {
        the_handler_stack->in_handler = FALSE;
        jumper_do_abort(the_jumper);
        return;
      }

    handle_exception(the_jumper, the_handler_stack, exception_value);

    value_remove_reference(exception_value, the_jumper);
    the_handler_stack->in_handler = FALSE;
  }

extern void expression_exception(jumper *the_jumper,
        expression *the_expression, static_exception_tag *tag,
        const char *message, ...)
  {
    va_list ap;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);
    assert(tag != NULL);
    assert(message != NULL);

    va_start(ap, message);
    vlocation_exception(the_jumper, get_expression_location(the_expression),
                        tag, message, ap);
    va_end(ap);
  }

extern void statement_exception(jumper *the_jumper, statement *the_statement,
        static_exception_tag *tag, const char *message, ...)
  {
    va_list ap;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);
    assert(tag != NULL);
    assert(message != NULL);

    va_start(ap, message);
    vlocation_exception(the_jumper, get_statement_location(the_statement), tag,
                        message, ap);
    va_end(ap);
  }

extern void type_expression_exception(jumper *the_jumper,
        type_expression *the_type_expression, static_exception_tag *tag,
        const char *message, ...)
  {
    va_list ap;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);
    assert(tag != NULL);
    assert(message != NULL);

    va_start(ap, message);
    vlocation_exception(the_jumper,
            get_type_expression_location(the_type_expression), tag, message,
            ap);
    va_end(ap);
  }

extern void jumper_throw_exception(jumper *the_jumper, value *exception_value,
                                   value *other_value)
  {
    lepton_key_instance *region_key;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);
    assert(exception_value != NULL);

    assert(!(the_jumper->aborting));

    region_key = the_jumper->region_key;
    if (region_key == NULL)
        return;

    if (value_get_field("source", exception_value) == NULL)
      {
        add_source_location_field(exception_value, NULL, "source", region_key,
                                  the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return;
      }

    jumper_throw_exception_internal(the_jumper, exception_value, other_value,
                                    region_key);
  }

extern void jumper_throw_exception_with_location(jumper *the_jumper,
        value *exception_value, value *other_value,
        const source_location *location)
  {
    lepton_key_instance *region_key;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);
    assert(exception_value != NULL);

    assert(!(the_jumper->aborting));

    region_key = the_jumper->region_key;
    if (region_key == NULL)
        return;

    add_source_location_field(exception_value, location, "source", region_key,
                              the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return;

    jumper_throw_exception_internal(the_jumper, exception_value, other_value,
                                    region_key);
  }

extern void jumper_set_target(jumper *the_jumper, jump_target *new_target)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);
    assert(new_target != NULL);

    assert(!(the_jumper->aborting));

    assert(!(jump_target_scope_exited(new_target))); /* VERIFIED */

    jump_target_add_reference(new_target);
    if (the_jumper->target != NULL)
        jump_target_remove_reference(the_jumper->target);

    the_jumper->target = new_target;
  }

extern void jumper_do_abort(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));

    assert(!(the_jumper->aborting));

    if (the_jumper->target != NULL)
        jump_target_remove_reference(the_jumper->target);
    the_jumper->target = NULL;

    the_jumper->aborting = TRUE;
  }

extern void jumper_reached_target(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    assert(!(the_jumper->aborting));
    assert(the_jumper->target != NULL);

    jump_target_remove_reference(the_jumper->target);
    the_jumper->target = NULL;
  }

extern verdict jumper_push_catcher(jumper *the_jumper, jump_target *target,
                                   type *tag_type)
  {
    handler_stack *new_stack;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);
    assert(target != NULL);

    assert(!(jump_target_scope_exited(target))); /* VERIFIED */

    new_stack = MALLOC_ONE_OBJECT(handler_stack);
    if (new_stack == NULL)
        return MISSION_FAILED;

    new_stack->jumper = the_jumper;
    new_stack->is_catcher = TRUE;
    new_stack->u.catcher.target = target;
    new_stack->u.catcher.tag_type = tag_type;
    jump_target_add_reference(target);
    if (tag_type != NULL)
        type_add_reference(tag_type);
    new_stack->in_handler = FALSE;
    new_stack->next = the_jumper->handler_stack;

    the_jumper->handler_stack = new_stack;

    return MISSION_ACCOMPLISHED;
  }

extern void jumper_pop_catcher(jumper *the_jumper, jump_target *target)
  {
    handler_stack *the_handler_stack;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);
    assert(target != NULL);

    assert(!(jump_target_scope_exited(target))); /* VERIFIED */

    the_handler_stack = the_jumper->handler_stack;
    assert(the_handler_stack != NULL);

    the_jumper->handler_stack = the_handler_stack->next;

    assert(the_handler_stack->is_catcher);
    assert(the_handler_stack->u.catcher.target == target);
    jump_target_remove_reference(target);
    if (the_handler_stack->u.catcher.tag_type != NULL)
      {
        type_remove_reference(the_handler_stack->u.catcher.tag_type,
                              the_jumper);
      }

    free(the_handler_stack);
  }

extern verdict jumper_push_handler(jumper *the_jumper, value *handler)
  {
    handler_stack *new_stack;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);
    assert(handler != NULL);

    new_stack = MALLOC_ONE_OBJECT(handler_stack);
    if (new_stack == NULL)
        return MISSION_FAILED;

    new_stack->jumper = the_jumper;
    new_stack->is_catcher = FALSE;
    new_stack->u.handler = handler;
    value_add_reference(handler);
    new_stack->in_handler = FALSE;
    new_stack->next = the_jumper->handler_stack;

    the_jumper->handler_stack = new_stack;

    return MISSION_ACCOMPLISHED;
  }

extern void jumper_pop_handler(jumper *the_jumper, value *handler)
  {
    handler_stack *the_handler_stack;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);
    assert(handler != NULL);

    the_handler_stack = the_jumper->handler_stack;
    assert(the_handler_stack != NULL);

    the_jumper->handler_stack = the_handler_stack->next;

    assert(!(the_handler_stack->is_catcher));
    assert(the_handler_stack->u.handler == handler);
    value_remove_reference(handler, the_jumper);

    free(the_handler_stack);
  }

extern boolean jumper_has_unreleased_exception_information(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    return (the_jumper->exception_information != NULL);
  }

extern value *jumper_unreleased_exception_information(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    return the_jumper->exception_information;
  }

extern value *jumper_exception_information(jumper *the_jumper)
  {
    size_t count;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    count = the_jumper->released_exception_information.element_count;
    if (count == 0)
        return NULL;

    return the_jumper->released_exception_information.array[count - 1];
  }

extern verdict jumper_release_exception_information(jumper *from_jumper,
                                                    jumper *to_jumper)
  {
    value *to_release;
    verdict the_verdict;

    assert(from_jumper != NULL);
    assert_is_malloced_block_with_exact_size(from_jumper, sizeof(jumper));
    assert(from_jumper->thread != NULL);
    assert(to_jumper != NULL);
    assert_is_malloced_block_with_exact_size(to_jumper, sizeof(jumper));
    assert(to_jumper->thread != NULL);

    to_release = from_jumper->exception_information;
    assert(to_release != NULL);

    from_jumper->exception_information = NULL;

    the_verdict = value_aa_append(
            &(to_jumper->released_exception_information), to_release);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(to_jumper);
        value_remove_reference(to_release, to_jumper);
        return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

extern void jumper_clear_exception_information(jumper *the_jumper)
  {
    size_t count;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    count = the_jumper->released_exception_information.element_count;
    assert(count > 0);

    the_jumper->released_exception_information.element_count = (count - 1);
    value_remove_reference(
            the_jumper->released_exception_information.array[count - 1],
            the_jumper);
  }

extern void jumper_clear_unreleased_exception_information(jumper *the_jumper)
  {
    value *to_clear;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    to_clear = the_jumper->exception_information;
    assert(to_clear != NULL);

    the_jumper->exception_information = NULL;

    if (to_clear != NULL)
        value_remove_reference(to_clear, the_jumper);
  }

extern void jumper_transfer_to_parent(jumper *the_jumper)
  {
    jumper *parent;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    parent = the_jumper->parent;
    assert(parent != NULL);

    if (the_jumper->aborting && !(parent->aborting))
        jumper_do_abort(parent);

    if (the_jumper->target != NULL)
      {
        jump_target *target;
        jump_target *parent_target;

        target = the_jumper->target;
        parent_target = parent->target;

        assert((parent_target == NULL) ||
               !(jump_target_scope_exited(parent_target))); /* VERIFIED */
        assert(!(jump_target_scope_exited(target))); /* VERIFIED */
        if ((parent_target == NULL) ||
            (jump_target_depth(parent_target) >= jump_target_depth(target)))
          {
            assert(!(jump_target_scope_exited(target))); /* VERIFIED */
            jumper_set_target(parent, target);
          }
      }
  }

extern void jumper_push_call_stack(jumper *the_jumper,
        routine_instance *callee, const source_location *call_site)
  {
    verdict the_verdict;
    call_item new_item;

    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);
    assert(callee != NULL);

    assert(the_jumper->call_stack != NULL);

    new_item.callee = callee;
    new_item.call_site = call_site;
    CLOCK_ZERO(new_item.non_local_time);

    the_verdict = call_item_aa_append(the_jumper->call_stack, new_item);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return;
      }
  }

extern void jumper_pop_call_stack(jumper *the_jumper)
  {
    assert(the_jumper != NULL);
    assert_is_malloced_block_with_exact_size(the_jumper, sizeof(jumper));
    assert(the_jumper->thread != NULL);

    assert(the_jumper->call_stack != NULL);
    assert(the_jumper->call_stack->element_count > 0);
    --(the_jumper->call_stack->element_count);
  }


static jumper *create_jumper_internal(jumper *parent_jumper,
        tracer *the_tracer, purity_level *the_purity_level,
        salmon_thread *the_salmon_thread)
  {
    jumper *result;
    verdict the_verdict;

    assert(the_purity_level != NULL);
    assert(the_salmon_thread != NULL);

    result = MALLOC_ONE_OBJECT(jumper);
    if (result == NULL)
      {
        purity_level_remove_reference(the_purity_level);
        return NULL;
      }

    result->aborting = FALSE;
    result->target = NULL;

    if (parent_jumper == NULL)
      {
        result->handler_stack = NULL;
        result->standard_library_object = NULL;
        result->region_key = NULL;
        result->exception_key = NULL;
      }
    else
      {
        result->handler_stack = parent_jumper->handler_stack;
        result->standard_library_object =
                parent_jumper->standard_library_object;
        result->region_key = parent_jumper->region_key;
        result->exception_key = parent_jumper->exception_key;
      }

    result->purity_level = the_purity_level;
    result->exception_information = NULL;

    the_verdict = value_aa_init(&(result->released_exception_information), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        purity_level_remove_reference(the_purity_level);
        return NULL;
      }

    result->parent = parent_jumper;
    result->tracer = the_tracer;
    result->thread = the_salmon_thread;
    salmon_thread_add_reference(the_salmon_thread);

    if (parent_jumper != NULL)
      {
        result->call_stack = parent_jumper->call_stack;
      }
    else
      {
        verdict the_verdict;

        result->call_stack = MALLOC_ONE_OBJECT(call_item_aa);
        if (result->call_stack == NULL)
          {
            free(result->released_exception_information.array);
            free(result);
            purity_level_remove_reference(the_purity_level);
            return NULL;
          }

        the_verdict = call_item_aa_init(result->call_stack, 10);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            free(result->call_stack);
            free(result->released_exception_information.array);
            free(result);
            purity_level_remove_reference(the_purity_level);
            return NULL;
          }
      }

    result->have_start_statement_execution_watcher = FALSE;
    result->have_start_expression_evaluation_watcher = FALSE;

    set_salmon_thread_current_jumper(the_salmon_thread, result);

    return result;
  }

static void handle_exception(jumper *the_jumper,
        handler_stack *the_handler_stack, value *exception_value)
  {
    assert(the_handler_stack->in_handler);

    if (the_handler_stack->is_catcher)
      {
        type *tag_type;
        verdict the_verdict;

        if ((the_handler_stack->jumper->target != NULL) &&
            (jump_target_depth(the_handler_stack->jumper->target) <
             jump_target_depth(the_handler_stack->u.catcher.target)))
          {
            goto do_next;
          }

        tag_type = the_handler_stack->u.catcher.tag_type;
        if (tag_type != NULL)
          {
            value *tag_value;
            jumper *child_jumper;
            boolean doubt;
            boolean in_type;

            if (the_jumper->exception_information != NULL)
                goto do_next;

            tag_value = value_get_field("tag", exception_value);
            if (tag_value == NULL)
                goto do_next;

            child_jumper = create_sub_jumper(the_jumper);
            if (child_jumper == NULL)
              {
                jumper_do_abort(the_jumper);
                return;
              }

            check_type_validity(tag_type, NULL, child_jumper);
            if (!(jumper_flowing_forward(child_jumper)))
              {
                jumper_transfer_to_parent(child_jumper);
                delete_jumper(child_jumper);
                return;
              }

            in_type = value_is_in_type(tag_value, tag_type, &doubt, NULL, NULL,
                                       child_jumper);
            if (!(jumper_flowing_forward(child_jumper)))
              {
                jumper_transfer_to_parent(child_jumper);
                delete_jumper(child_jumper);
                return;
              }

            if (doubt)
              {
                location_exception(child_jumper, NULL,
                        EXCEPTION_TAG(try_catch_tag_match_indeterminate),
                        "%s was unable to determine whether the tag for an "
                        "exception matched the tag type required for a `catch'"
                        " clause of a try-catch statement.",
                        interpreter_name());
                jumper_transfer_to_parent(child_jumper);
                delete_jumper(child_jumper);
                return;
              }

            delete_jumper(child_jumper);

            if (!in_type)
                goto do_next;
          }

        the_verdict =
                append_exception_information(the_jumper, exception_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            return;
          }

        assert(!(jump_target_scope_exited(
                         the_handler_stack->u.catcher.target))); /* VERIFIED */
        jumper_set_target(the_jumper, the_handler_stack->u.catcher.target);
      }
    else
      {
        value *return_value;

        return_value = execute_call_from_arrays(the_handler_stack->u.handler,
                1, NULL, &exception_value, FALSE, the_jumper, NULL);
        assert(return_value == NULL);

        if (jumper_flowing_forward(the_jumper))
          {
            handler_stack *next_handler;

          do_next:
            next_handler = the_handler_stack->next;

            while (TRUE)
              {
                if (next_handler == NULL)
                  {
                    unhandled_exception(exception_value);
                    jumper_do_abort(the_jumper);
                    return;
                  }

                if (!(next_handler->in_handler))
                    break;

                next_handler = next_handler->next;
              }

            assert(!(next_handler->in_handler));

            next_handler->in_handler = TRUE;

            handle_exception(the_jumper, next_handler, exception_value);

            next_handler->in_handler = FALSE;
          }
      }
  }

static value *build_exception_value(jumper *the_jumper,
        const source_location *location, static_exception_tag *tag,
        const char *message, va_list ap)
  {
    object *standard_library_object;
    lepton_key_instance *exception_key;
    lepton_key_instance *region_key;
    value *exception_value;
    value *tag_value;
    verdict the_verdict;
    string_buffer message_buffer;
    int print_result;
    value *message_value;

    assert(the_jumper != NULL);
    assert(tag != NULL);
    assert(message != NULL);

    standard_library_object = the_jumper->standard_library_object;

    if (standard_library_object == NULL)
      {
        vlocation_error(location, message, ap);
        jumper_do_abort(the_jumper);
        return NULL;
      }

    exception_key = the_jumper->exception_key;
    if (exception_key == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    region_key = the_jumper->region_key;
    if (region_key == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    exception_value = create_lepton_value(exception_key);
    if (exception_value == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    if (tag->field_name == NULL)
      {
        tag_value = create_quark_value(tag->u.quark);
        if (tag_value == NULL)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }
      }
    else
      {
        object *tag_object;
        size_t tag_field_num;

        tag_object = tag->u.object;

        if (tag_object == NULL)
          {
            tag_object = standard_library_object;
          }
        else if (object_is_closed(tag_object))
          {
            vlocation_error(location, message, ap);
            jumper_do_abort(the_jumper);
            value_remove_reference(exception_value, the_jumper);
            return NULL;
          }

        assert(!(object_is_closed(tag_object))); /* VERIFIED */
        tag_field_num = object_field_lookup(tag_object, tag->field_name);
        assert(tag_field_num < object_field_count(tag_object));

        tag_value = object_field_read_value(tag_object, tag_field_num, NULL,
                                            the_jumper);
        if (the_jumper->aborting)
          {
            assert(tag_value == NULL);
            value_remove_reference(exception_value, the_jumper);
            return NULL;
          }
      }

    assert(tag_value != NULL);
    assert(get_value_kind(tag_value) == VK_QUARK);

    the_verdict = add_field(exception_value, "tag", tag_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(tag_value, the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    value_remove_reference(tag_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    the_verdict = string_buffer_init(&message_buffer, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    print_result =
            vinterpreter_zero_buffer_printf(&message_buffer, 0, message, ap);
    if (print_result < 0)
      {
        jumper_do_abort(the_jumper);
        free(message_buffer.array);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    message_value = create_string_value(message_buffer.array);
    free(message_buffer.array);
    if (message_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    the_verdict = add_field(exception_value, "message", message_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(message_value, the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    value_remove_reference(message_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    add_source_location_field(exception_value, location, "source", region_key,
                              the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    add_exception_stack_info(exception_value, region_key, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    return exception_value;
  }

static void add_exception_stack_info(value *exception_value,
        lepton_key_instance *region_key, jumper *the_jumper)
  {
    value *call_stack_value;
    size_t call_stack_item_count;
    size_t call_stack_item_num;
    verdict the_verdict;

    call_stack_value = create_semi_labeled_value_list_value();
    if (call_stack_value == NULL)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    call_stack_item_count = jumper_call_stack_size(the_jumper);
    for (call_stack_item_num = 0; call_stack_item_num < call_stack_item_count;
         ++call_stack_item_num)
      {
        value *item_value;
        routine_instance *routine;
        declaration *declaration;
        const char *routine_name;
        value *routine_name_value;
        verdict the_verdict;

        item_value = create_semi_labeled_value_list_value();
        if (item_value == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(call_stack_value, the_jumper);
            return;
          }

        routine =
                jumper_call_stack_back_callee(the_jumper, call_stack_item_num);
        assert(routine != NULL);

        declaration = instance_declaration(routine_instance_instance(routine));
        assert(declaration != NULL);

        routine_name = declaration_name(declaration);
        if (routine_name == NULL)
            routine_name_value = create_null_value();
        else
            routine_name_value = create_string_value(routine_name);
        if (routine_name_value == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(item_value, the_jumper);
            value_remove_reference(call_stack_value, the_jumper);
            return;
          }

        the_verdict = add_field(item_value, "callee_name", routine_name_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(routine_name_value, the_jumper);
            value_remove_reference(item_value, the_jumper);
            value_remove_reference(call_stack_value, the_jumper);
            return;
          }

        value_remove_reference(routine_name_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(item_value, the_jumper);
            value_remove_reference(call_stack_value, the_jumper);
            return;
          }

        add_source_location_field(item_value,
                get_declaration_location(declaration), "callee", region_key,
                the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(item_value, the_jumper);
            value_remove_reference(call_stack_value, the_jumper);
            return;
          }

        add_source_location_field(item_value,
                jumper_call_stack_back_site(the_jumper, call_stack_item_num),
                "call_site", region_key, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(item_value, the_jumper);
            value_remove_reference(call_stack_value, the_jumper);
            return;
          }

        the_verdict = add_field(call_stack_value, NULL, item_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(item_value, the_jumper);
            value_remove_reference(call_stack_value, the_jumper);
            return;
          }

        value_remove_reference(item_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(call_stack_value, the_jumper);
            return;
          }
      }

    the_verdict = add_field(exception_value, "call_stack", call_stack_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(call_stack_value, the_jumper);
        return;
      }

    value_remove_reference(call_stack_value, the_jumper);
  }

static void add_source_location_field(value *base,
        const source_location *location, const char *field_name,
        lepton_key_instance *region_key, jumper *the_jumper)
  {
    value *region_value;
    verdict the_verdict;

    region_value = create_source_location_value_with_key(location, the_jumper,
                                                         region_key);
    if (region_value == NULL)
        return;

    the_verdict = add_field(base, field_name, region_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(region_value, the_jumper);
        return;
      }

    value_remove_reference(region_value, the_jumper);
  }

static size_t value_component_to_size_t(value *base_value,
                                        const char *field_name)
  {
    value *component;
    o_integer the_oi;
    size_t result;
    verdict the_verdict;

    assert(base_value != NULL);
    assert(field_name != NULL);

    component = value_get_field(field_name, base_value);
    if (component == NULL)
        return 0;

    assert(component != NULL);
    assert(get_value_kind(component) == VK_INTEGER);

    the_oi = integer_value_data(component);
    assert(!(oi_out_of_memory(the_oi)));

    if (oi_kind(the_oi) != IIK_FINITE)
        return 0;

    the_verdict = oi_magnitude_to_size_t(the_oi, &result);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return 0;

    return result;
  }

static verdict append_exception_information(jumper *the_jumper,
                                            value *new_information)
  {
    value *exception_information;

    exception_information = the_jumper->exception_information;

    if (exception_information == NULL)
      {
        exception_information = create_semi_labeled_value_list_value();
        if (exception_information == NULL)
            return MISSION_FAILED;

        the_jumper->exception_information = exception_information;
      }
    else
      {
        if (!(value_has_only_one_reference(exception_information)))
          {
            value *copy;

            copy = copy_value(exception_information);
            if (copy == NULL)
                return MISSION_FAILED;

            value_remove_reference(exception_information, the_jumper);
            assert(jumper_flowing_forward(the_jumper));
            exception_information = copy;
            the_jumper->exception_information = exception_information;
          }
      }

    assert(value_has_only_one_reference(exception_information));
    assert(get_value_kind(exception_information) ==
           VK_SEMI_LABELED_VALUE_LIST);

    return add_field(exception_information, NULL, new_information);
  }

static void jumper_throw_exception_internal(jumper *the_jumper,
        value *exception_value, value *other_value,
        lepton_key_instance *region_key)
  {
    handler_stack *the_handler_stack;

    assert(the_jumper != NULL);
    assert(exception_value != NULL);

    assert(!(the_jumper->aborting));

    add_exception_stack_info(exception_value, region_key, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return;

    if (other_value != NULL)
      {
        size_t other_component_count;
        size_t other_component_num;

        assert(value_has_only_named_fields(other_value));

        assert((get_value_kind(other_value) == VK_SEMI_LABELED_VALUE_LIST) ||
               (get_value_kind(other_value) == VK_SEMI_LABELED_MULTI_SET));

        other_component_count = value_component_count(other_value);

        for (other_component_num = 0;
             other_component_num < other_component_count;
             ++other_component_num)
          {
            const char *label;
            value *component_value;
            verdict the_verdict;

            label = value_component_label(other_value, other_component_num);
            assert(label != NULL);

            component_value =
                    value_component_value(other_value, other_component_num);
            assert(component_value != NULL);

            the_verdict = add_field(exception_value, label, component_value);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                return;
              }
          }
      }

    the_handler_stack = the_jumper->handler_stack;

    while (TRUE)
      {
        if (the_handler_stack == NULL)
          {
            unhandled_exception(exception_value);
            jumper_do_abort(the_jumper);
            return;
          }

        if (!(the_handler_stack->in_handler))
            break;

        the_handler_stack = the_handler_stack->next;
      }

    assert(!(the_handler_stack->in_handler));

    the_handler_stack->in_handler = TRUE;

    handle_exception(the_jumper, the_handler_stack, exception_value);

    the_handler_stack->in_handler = FALSE;
  }

static void unhandled_exception(value *exception_value)
  {
    value *source_value;
    value *message_value;
    source_location location;
    const char *message;

    assert(get_value_kind(exception_value) == VK_LEPTON);

    source_value = value_get_field("source", exception_value);
    if (source_value != NULL)
      {
        value *name_value;

        assert(get_value_kind(source_value) == VK_LEPTON);
        name_value = value_get_field("file_name", source_value);
        if (name_value != NULL)
          {
            assert(get_value_kind(name_value) == VK_STRING);
            location.file_name = string_value_data(name_value);
          }
        else
          {
            location.file_name = NULL;
          }

        location.start_line_number =
                value_component_to_size_t(source_value, "start_line");

        location.start_column_number =
                value_component_to_size_t(source_value, "start_column");

        location.end_line_number =
                value_component_to_size_t(source_value, "end_line");

        location.end_column_number =
                value_component_to_size_t(source_value, "end_column");
      }
    else
      {
        location.file_name = NULL;
        location.start_line_number = 0;
        location.start_column_number = 0;
        location.end_line_number = 0;
        location.end_column_number = 0;
      }

    message_value = value_get_field("message", exception_value);
    assert(message_value != NULL);
    assert(get_value_kind(message_value) == VK_STRING);
    message = string_value_data(message_value);

    location_error(&location, "%s", message);
  }
