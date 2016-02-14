/* file "parse.c" */

/*
 *  This file contains functionality for parsing Salmon code into an executable
 *  form and running it.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <assert.h>
#include "../c_foundations/basic.h"
#include "../c_foundations/diagnostic.h"
#include "../c_foundations/memory_allocation.h"
#include "../source_location.h"
#include "../open_expression.h"
#include "../open_statement_block.h"
#include "../object.h"
#include "../tokenizer.h"
#include "../parser.h"
#include "../include.h"
#include "../execute.h"
#include "../token.h"
#include "../jumper.h"
#include "../context.h"
#include "../routine_declaration_chain.h"
#include "../driver.h"


typedef struct
  {
    void (*old_diagnostic_handler)(void *data, diagnostic_kind kind,
            boolean has_message_number, size_t message_number,
            const char *file_name, boolean has_line_number, size_t line_number,
            boolean has_column_number, size_t column_number,
            boolean has_character_count, size_t character_count,
            const char *format, va_list arg);
    void *old_diagnostic_data;
    boolean have_error;
    source_location location;
    char *message;
  } local_diagnostic_data_type;

typedef struct
  {
    unbound_name_manager *manager;
    context *context;
  } manager_hook_data;

typedef struct
  {
    open_expression *open_expression;
    context *context;
  } expression_hook_data;

typedef struct
  {
    open_statement_block *open_statement_block;
    context *context;
  } statement_block_hook_data;


salmoneye_lock *diagnostic_handler_lock = NULL;


static quark *find_tag(context *the_context, const char *name);
static void local_diagnostic_handler(void *data, diagnostic_kind kind,
        boolean has_message_number, size_t message_number,
        const char *file_name, boolean has_line_number, size_t line_number,
        boolean has_column_number, size_t column_number,
        boolean has_character_count, size_t character_count,
        const char *format, va_list arg);
static void bind_value(const char *init_name, value *target,
        manager_hook_data *hook_data, context *the_context, jumper *the_jumper,
        const source_location *location);
static void bind_object(object *the_object, manager_hook_data *hook_data,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static void bind_instance(const char *init_name, instance *target,
        manager_hook_data *hook_data, context *the_context, jumper *the_jumper,
        const source_location *location);
static void bind_routine_chain(const char *init_name,
        routine_instance_chain *target, manager_hook_data *hook_data,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static routine_declaration_chain *chain_to_glue_chain(
        routine_instance_chain *pre_glue, context *glue_context);
static void no_name(declaration *the_declaration, context *the_context,
                    jumper *the_jumper, const source_location *location);
static void check_for_unbound(unbound_name_manager *manager,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static void manager_cleaner(void *hook, jumper *the_jumper);
static void expression_cleaner(void *hook, jumper *the_jumper);
static void statement_block_cleaner(void *hook, jumper *the_jumper);


extern verdict salmoneye_plugin_initialize(void)
  {
    assert(diagnostic_handler_lock == NULL);

    diagnostic_handler_lock = create_salmoneye_lock();
    if (diagnostic_handler_lock == NULL)
        return MISSION_FAILED;

    return MISSION_ACCOMPLISHED;
  }

extern void salmoneye_plugin_clean_up(void)
  {
    assert(diagnostic_handler_lock != NULL);
    destroy_salmoneye_lock(diagnostic_handler_lock);
    diagnostic_handler_lock = NULL;
  }

extern value *unbound_required_names(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    manager_hook_data *hook_data;
    value *result;
    size_t name_count;
    size_t name_num;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 0);

    hook_data = (manager_hook_data *)(object_hook(
            find_top_this_object_value(the_context, 0)));

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    name_count = unbound_name_count(hook_data->manager);

    for (name_num = 0; name_num < name_count; ++name_num)
      {
        unbound_name *this_unbound;
        size_t use_count;
        size_t use_num;

        this_unbound = unbound_name_number(hook_data->manager, name_num);
        use_count = unbound_name_use_count(this_unbound);
        for (use_num = 0; use_num < use_count; ++use_num)
          {
            unbound_use *this_use;

            this_use = unbound_name_use_by_number(this_unbound, use_num);
            switch (get_unbound_use_kind(this_use))
              {
                case UUK_ROUTINE_FOR_RETURN_STATEMENT:
                case UUK_ROUTINE_FOR_EXPORT_STATEMENT:
                case UUK_ROUTINE_FOR_HIDE_STATEMENT:
                case UUK_ROUTINE_FOR_ARGUMENTS_EXPRESSION:
                case UUK_ROUTINE_FOR_THIS_EXPRESSION:
                case UUK_LOOP_FOR_BREAK_STATEMENT:
                case UUK_LOOP_FOR_CONTINUE_STATEMENT:
                case UUK_LOOP_FOR_BREAK_EXPRESSION:
                case UUK_LOOP_FOR_CONTINUE_EXPRESSION:
                case UUK_VARIABLE_FOR_EXPRESSION:
                  {
                    value *new_component;
                    verdict the_verdict;

                    new_component = create_string_value(
                            unbound_name_string(this_unbound));
                    if (new_component == NULL)
                      {
                        jumper_do_abort(the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }

                    the_verdict = add_field(result, NULL, new_component);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        jumper_do_abort(the_jumper);
                        value_remove_reference(new_component, the_jumper);
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }

                    value_remove_reference(new_component, the_jumper);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        value_remove_reference(result, the_jumper);
                        return NULL;
                      }

                    use_num = use_count;
                    break;
                  }
                case UUK_ROUTINE_FOR_OPERATOR_EXPRESSION:
                case UUK_ROUTINE_FOR_OPERATOR_STATEMENT:
                case UUK_DANGLING_OVERLOADED_ROUTINE:
                case UUK_USE_FLOW_THROUGH:
                  {
                    break;
                  }
                default:
                  {
                    assert(FALSE);
                  }
              }
          }
      }

    return result;
  }

extern value *unbound_optional_names(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    manager_hook_data *hook_data;
    value *result;
    size_t name_count;
    size_t name_num;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 0);

    hook_data = (manager_hook_data *)(object_hook(
            find_top_this_object_value(the_context, 0)));

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    name_count = unbound_name_count(hook_data->manager);

    for (name_num = 0; name_num < name_count; ++name_num)
      {
        unbound_name *this_unbound;
        boolean required;
        size_t use_count;
        size_t use_num;

        required = FALSE;
        this_unbound = unbound_name_number(hook_data->manager, name_num);
        use_count = unbound_name_use_count(this_unbound);
        for (use_num = 0; use_num < use_count; ++use_num)
          {
            unbound_use *this_use;

            this_use = unbound_name_use_by_number(this_unbound, use_num);
            switch (get_unbound_use_kind(this_use))
              {
                case UUK_ROUTINE_FOR_RETURN_STATEMENT:
                case UUK_ROUTINE_FOR_EXPORT_STATEMENT:
                case UUK_ROUTINE_FOR_HIDE_STATEMENT:
                case UUK_ROUTINE_FOR_ARGUMENTS_EXPRESSION:
                case UUK_ROUTINE_FOR_THIS_EXPRESSION:
                case UUK_LOOP_FOR_BREAK_STATEMENT:
                case UUK_LOOP_FOR_CONTINUE_STATEMENT:
                case UUK_LOOP_FOR_BREAK_EXPRESSION:
                case UUK_LOOP_FOR_CONTINUE_EXPRESSION:
                case UUK_VARIABLE_FOR_EXPRESSION:
                  {
                    required = TRUE;
                    use_num = use_count;
                    break;
                  }
                case UUK_ROUTINE_FOR_OPERATOR_EXPRESSION:
                case UUK_ROUTINE_FOR_OPERATOR_STATEMENT:
                case UUK_DANGLING_OVERLOADED_ROUTINE:
                case UUK_USE_FLOW_THROUGH:
                  {
                    break;
                  }
                default:
                  {
                    assert(FALSE);
                  }
              }
          }

        if (!required)
          {
            value *new_component;
            verdict the_verdict;

            new_component =
                    create_string_value(unbound_name_string(this_unbound));
            if (new_component == NULL)
              {
                jumper_do_abort(the_jumper);
                value_remove_reference(result, the_jumper);
                return NULL;
              }

            the_verdict = add_field(result, NULL, new_component);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                value_remove_reference(new_component, the_jumper);
                value_remove_reference(result, the_jumper);
                return NULL;
              }

            value_remove_reference(new_component, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(result, the_jumper);
                return NULL;
              }
          }
      }

    return result;
  }

extern value *bind_one_with_name(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *name;
    value *target;
    manager_hook_data *hook_data;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    name = string_value_data(value_component_value(all_arguments_value, 0));
    target = value_component_value(all_arguments_value, 1);

    hook_data = (manager_hook_data *)(object_hook(
            find_top_this_object_value(the_context, 0)));

    bind_value(name, target, hook_data, the_context, the_jumper, location);

    return NULL;
  }

extern value *bind_one(value *all_arguments_value, context *the_context,
                       jumper *the_jumper, const source_location *location)
  {
    value *target;
    manager_hook_data *hook_data;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    target = value_component_value(all_arguments_value, 0);

    hook_data = (manager_hook_data *)(object_hook(
            find_top_this_object_value(the_context, 0)));

    bind_value(NULL, target, hook_data, the_context, the_jumper, location);

    return NULL;
  }

extern value *bind_list(value *all_arguments_value, context *the_context,
                        jumper *the_jumper, const source_location *location)
  {
    value *list;
    manager_hook_data *hook_data;
    size_t component_count;
    size_t component_num;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    list = value_component_value(all_arguments_value, 0);

    hook_data = (manager_hook_data *)(object_hook(
            find_top_this_object_value(the_context, 0)));

    assert(get_value_kind(list) == VK_SEMI_LABELED_VALUE_LIST);
    component_count = value_component_count(list);

    for (component_num = 0; component_num < component_count; ++component_num)
      {
        bind_value(value_component_label(list, component_num),
                value_component_value(list, component_num), hook_data,
                the_context, the_jumper, location);
        if (!(jumper_flowing_forward(the_jumper)))
            return NULL;
      }

    return NULL;
  }

extern value *bind_object_fields(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    object *the_object;
    manager_hook_data *hook_data;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    the_object =
            object_value_data(value_component_value(all_arguments_value, 0));

    hook_data = (manager_hook_data *)(object_hook(
            find_top_this_object_value(the_context, 0)));

    bind_object(the_object, hook_data, the_context, the_jumper, location);

    return NULL;
  }

extern value *bind_standard_library(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    manager_hook_data *hook_data;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 0);

    hook_data = (manager_hook_data *)(object_hook(
            find_top_this_object_value(the_context, 0)));

    bind_object(jumper_standard_library_object(the_jumper), hook_data,
                the_context, the_jumper, location);

    return NULL;
  }

extern value *run_open_expression(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    object *the_object;
    expression_hook_data *hook_data;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    the_object =
            object_value_data(value_component_value(all_arguments_value, 0));

    hook_data = (expression_hook_data *)(object_hook(the_object));
    assert(hook_data != NULL);

    check_for_unbound(
            open_expression_unbound_name_manager(hook_data->open_expression),
            the_context, the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    return evaluate_expression(
            open_expression_expression(hook_data->open_expression),
            hook_data->context, the_jumper);
  }

extern value *run_open_statement_block(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    object *the_object;
    statement_block_hook_data *hook_data;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    the_object =
            object_value_data(value_component_value(all_arguments_value, 0));

    hook_data = (statement_block_hook_data *)(object_hook(the_object));
    assert(hook_data != NULL);

    check_for_unbound(
            open_statement_block_unbound_name_manager(
                    hook_data->open_statement_block), the_context, the_jumper,
            location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    execute_statement_block(
            open_statement_block_statement_block(
                    hook_data->open_statement_block), hook_data->context,
            the_jumper);

    return NULL;
  }

extern value *parse_from_string(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *source;
    value *file_name_value;
    o_integer start_line;
    o_integer start_column;
    value *result;
    object *manager_object;
    boolean is_expression;
    value *executable_directory_value;
    const char *executable_directory;
    const char *directory_paths;
    const char *file_name;
    const char *base_file_name;
    tokenizer *the_tokenizer;
    size_t start_line_size_t;
    verdict the_verdict;
    size_t start_column_size_t;
    void *include_handler_data;
    parser *the_parser;
    local_diagnostic_data_type local_diagnostic_data;
    open_expression *the_open_expression;
    open_statement_block *the_open_statement_block;
    void *hook_data;
    void (*hook_cleaner)(void *hook, jumper *the_jumper);
    token *end_token;
    manager_hook_data *the_manager_hook_data;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 9);
    source = string_value_data(value_component_value(all_arguments_value, 0));
    file_name_value = value_component_value(all_arguments_value, 1);
    start_line =
            integer_value_data(value_component_value(all_arguments_value, 2));
    start_column =
            integer_value_data(value_component_value(all_arguments_value, 3));
    result = value_component_value(all_arguments_value, 4);
    manager_object =
            object_value_data(value_component_value(all_arguments_value, 5));
    is_expression = (get_value_kind(
            value_component_value(all_arguments_value, 6)) == VK_TRUE);
    executable_directory_value = value_component_value(all_arguments_value, 7);
    executable_directory =
            ((get_value_kind(executable_directory_value) == VK_STRING) ?
             string_value_data(executable_directory_value) : NULL);
    directory_paths =
            string_value_data(value_component_value(all_arguments_value, 8));

    switch (get_value_kind(file_name_value))
      {
        case VK_STRING:
            file_name = string_value_data(file_name_value);
            base_file_name = file_name;
            break;
        case VK_NULL:
            file_name = "string";
            base_file_name = "";
            break;
        default:
            assert(FALSE);
      }

    the_tokenizer = create_tokenizer(source, file_name);
    if (the_tokenizer == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    the_verdict = oi_magnitude_to_size_t(start_line, &start_line_size_t);
    if (the_verdict == MISSION_ACCOMPLISHED)
        set_tokenizer_line_number(the_tokenizer, start_line_size_t);
    else
        set_tokenizer_line_number(the_tokenizer, 0);

    the_verdict = oi_magnitude_to_size_t(start_column, &start_column_size_t);
    if (the_verdict == MISSION_ACCOMPLISHED)
        set_tokenizer_column_number(the_tokenizer, start_column_size_t);
    else
        set_tokenizer_column_number(the_tokenizer, 0);

    include_handler_data = create_local_file_include_handler_data(
            base_file_name, directory_paths, executable_directory);
    if (include_handler_data == NULL)
      {
        jumper_do_abort(the_jumper);
        delete_tokenizer(the_tokenizer);
        return NULL;
      }

    the_parser = create_parser(the_tokenizer, &local_file_include_handler,
            &local_file_interface_include_handler, include_handler_data, NULL,
            FALSE);
    if (the_parser == NULL)
      {
        jumper_do_abort(the_jumper);
        delete_local_file_include_handler_data(include_handler_data);
        delete_tokenizer(the_tokenizer);
        return NULL;
      }

    lock_salmoneye_lock(diagnostic_handler_lock);

    local_diagnostic_data.old_diagnostic_handler = get_diagnostic_handler(
            &(local_diagnostic_data.old_diagnostic_data));
    local_diagnostic_data.have_error = FALSE;

    set_diagnostic_handler(&local_diagnostic_data, &local_diagnostic_handler);

    if (is_expression)
      {
        expression_hook_data *block_data;

        the_open_expression =
                parse_expression(the_parser, EPP_TOP, NULL, FALSE);
        the_open_statement_block = NULL;
        if (the_open_expression == NULL)
            goto syntax_error;

        block_data = MALLOC_ONE_OBJECT(expression_hook_data);
        if (block_data == NULL)
          {
            delete_open_expression(the_open_expression);
            goto syntax_error;
          }

        block_data->open_expression = the_open_expression;
        hook_data = block_data;
        hook_cleaner = &expression_cleaner;
      }
    else
      {
        statement_block_hook_data *block_data;

        the_open_statement_block = parse_statement_list(the_parser);
        the_open_expression = NULL;
        if (the_open_statement_block == NULL)
            goto syntax_error;

        block_data = MALLOC_ONE_OBJECT(statement_block_hook_data);
        if (block_data == NULL)
          {
            delete_open_statement_block(the_open_statement_block);
            goto syntax_error;
          }

        block_data->open_statement_block = the_open_statement_block;
        hook_data = block_data;
        hook_cleaner = &statement_block_cleaner;
      }
    if (hook_data == NULL)
      {
      syntax_error:
        set_diagnostic_handler(local_diagnostic_data.old_diagnostic_data,
                               local_diagnostic_data.old_diagnostic_handler);
        unlock_salmoneye_lock(diagnostic_handler_lock);
        if (local_diagnostic_data.have_error)
          {
            static_exception_tag tag_info;

            tag_info.field_name = NULL;
            tag_info.u.quark = find_tag(the_context, "et_parse_syntax_error");
            location_exception(the_jumper, &(local_diagnostic_data.location),
                               &tag_info, local_diagnostic_data.message);
            type_message_deallocate(local_diagnostic_data.message);
            if (local_diagnostic_data.location.file_name != NULL)
              {
                type_message_deallocate(
                        (char *)(local_diagnostic_data.location.file_name));
              }
          }
        else
          {
            jumper_do_abort(the_jumper);
          }
        delete_parser(the_parser);
        delete_local_file_include_handler_data(include_handler_data);
        delete_tokenizer(the_tokenizer);
        return NULL;
      }

    assert(!(local_diagnostic_data.have_error));

    end_token = next_token(the_tokenizer);
    if ((end_token == NULL) || (get_token_kind(end_token) == TK_ERROR))
      {
        if (is_expression)
            delete_open_expression(the_open_expression);
        else
            delete_open_statement_block(the_open_statement_block);
        goto syntax_error;
      }

    assert(!(local_diagnostic_data.have_error));

    set_diagnostic_handler(local_diagnostic_data.old_diagnostic_data,
                           local_diagnostic_data.old_diagnostic_handler);
    unlock_salmoneye_lock(diagnostic_handler_lock);

    if (get_token_kind(end_token) != TK_END_OF_INPUT)
      {
        static_exception_tag tag_info;

        tag_info.field_name = NULL;
        tag_info.u.quark = find_tag(the_context, "et_parse_more");
        location_exception(the_jumper, get_token_location(end_token),
                &tag_info, "Junk was found past the end of the %s.",
                (is_expression ? "expression" : "statement block"));
        if (is_expression)
            delete_open_expression(the_open_expression);
        else
            delete_open_statement_block(the_open_statement_block);
        delete_parser(the_parser);
        delete_local_file_include_handler_data(include_handler_data);
        delete_tokenizer(the_tokenizer);
        return NULL;
      }

    delete_parser(the_parser);
    delete_local_file_include_handler_data(include_handler_data);
    delete_tokenizer(the_tokenizer);

    object_set_hook(object_value_data(result), hook_data);
    object_set_hook_cleaner(object_value_data(result), hook_cleaner);

    the_manager_hook_data = MALLOC_ONE_OBJECT(manager_hook_data);
    if (the_manager_hook_data == NULL)
        return NULL;

    the_manager_hook_data->manager =
            (is_expression ?
             open_expression_unbound_name_manager(the_open_expression) :
             open_statement_block_unbound_name_manager(
                     the_open_statement_block));
    the_manager_hook_data->context = create_glue_context();
    if (the_manager_hook_data->context == NULL)
      {
        free(the_manager_hook_data);
        return NULL;
      }

    object_set_hook(manager_object, the_manager_hook_data);
    object_set_hook_cleaner(manager_object, &manager_cleaner);

    if (is_expression)
      {
        ((expression_hook_data *)hook_data)->context =
                the_manager_hook_data->context;
      }
    else
      {
        ((statement_block_hook_data *)hook_data)->context =
                the_manager_hook_data->context;
      }

    value_add_reference(result);
    return result;
  }

extern value *get_executable_directory(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *directory_chars;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 0);

    directory_chars = context_executable_directory(the_context);

    if (directory_chars == NULL)
        result = create_null_value();
    else
        result = create_string_value(directory_chars);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

extern value *get_directory_paths(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 0);

    result = create_string_value(context_directory_paths(the_context));
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }


static quark *find_tag(context *the_context, const char *name)
  {
    size_t depth;

    assert(the_context != NULL);
    assert(name != NULL);

    depth = 0;

    while (TRUE)
      {
        object *this_object;
        size_t field_num;

        this_object = find_top_this_object_value(the_context, depth);
        assert(this_object != NULL);

        field_num = object_field_lookup(this_object, name);
        if (field_num < object_field_count(this_object))
          {
            return instance_quark_instance(object_field_instance(this_object,
                                                                 field_num));
          }

        ++depth;
      }
  }

static void local_diagnostic_handler(void *data, diagnostic_kind kind,
        boolean has_message_number, size_t message_number,
        const char *file_name, boolean has_line_number, size_t line_number,
        boolean has_column_number, size_t column_number,
        boolean has_character_count, size_t character_count,
        const char *format, va_list arg)
  {
    local_diagnostic_data_type *typed_data;

    assert(data != NULL);

    typed_data = (local_diagnostic_data_type *)data;

    if ((kind != DK_ERROR) || typed_data->have_error)
      {
      do_old:
        (*(typed_data->old_diagnostic_handler))(
                typed_data->old_diagnostic_data, kind, has_message_number,
                message_number, file_name, has_line_number, line_number,
                has_column_number, column_number, has_character_count,
                character_count, format, arg);
        return;
      }

    typed_data->message = vallocate_printf(format, arg);
    if (typed_data->message == NULL)
        goto do_old;

    typed_data->have_error = TRUE;
    typed_data->location.file_name =
        ((file_name == NULL) ? NULL : allocate_printf("%s", file_name));
    if (has_line_number)
      {
        typed_data->location.start_line_number = line_number;
        typed_data->location.end_line_number = line_number;
      }
    else
      {
        typed_data->location.start_line_number = 0;
        typed_data->location.end_line_number = 0;
      }
    if (has_column_number)
      {
        typed_data->location.start_column_number = column_number;
        if (has_character_count)
          {
            typed_data->location.end_column_number =
                    column_number + character_count;
          }
        else
          {
            typed_data->location.end_column_number = column_number;
          }
      }
    else
      {
        typed_data->location.start_column_number = 0;
        typed_data->location.end_column_number = 0;
      }
    typed_data->location.holder = NULL;
  }

static void bind_value(const char *init_name, value *target,
        manager_hook_data *hook_data, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    assert(target != NULL);
    assert(hook_data != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    switch (get_value_kind(target))
      {
        case VK_QUARK:
          {
            bind_instance(init_name,
                    quark_instance_instance(value_quark(target)), hook_data,
                    the_context, the_jumper, location);
            break;
          }
        case VK_LEPTON_KEY:
          {
            bind_instance(init_name,
                    lepton_key_instance_instance(value_lepton_key(target)),
                    hook_data, the_context, the_jumper, location);
            break;
          }
        case VK_SLOT_LOCATION:
          {
            slot_location *slot;

            slot = slot_location_value_data(target);
            assert(slot != NULL);

            if (get_slot_location_kind(slot) == SLK_VARIABLE)
              {
                bind_instance(init_name,
                        variable_instance_instance(
                                variable_slot_location_variable(slot)),
                        hook_data, the_context, the_jumper, location);
                break;
              }

            goto bad_target;
          }
        case VK_ROUTINE:
          {
            bind_instance(init_name,
                    routine_instance_instance(routine_value_data(target)),
                    hook_data, the_context, the_jumper, location);
            break;
          }
        case VK_ROUTINE_CHAIN:
          {
            bind_routine_chain(init_name, routine_chain_value_data(target),
                               hook_data, the_context, the_jumper, location);
            return;
          }
        case VK_TAGALONG_KEY:
          {
            bind_instance(init_name,
                    tagalong_key_instance(tagalong_key_value_data(target)),
                    hook_data, the_context, the_jumper, location);
            break;
          }
        case VK_LOCK:
          {
            bind_instance(init_name,
                    lock_instance_instance(lock_value_data(target)), hook_data,
                    the_context, the_jumper, location);
            break;
          }
        default:
          {
            static_exception_tag tag_info;

          bad_target:
            tag_info.field_name = NULL;
            tag_info.u.quark =
                    find_tag(the_context, "et_parse_bind_bad_value");
            location_exception(the_jumper, location, &tag_info,
                               "In bind(), the value for the target was bad.");
            return;
          }
      }
  }

static void bind_object(object *the_object, manager_hook_data *hook_data,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    size_t field_count;
    size_t field_num;

    assert(the_object != NULL);
    assert(hook_data != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    field_count = object_field_count(the_object);

    for (field_num = 0; field_num < field_count; ++field_num)
      {
        const char *field_name;

        field_name = object_field_name(the_object, field_num);
        if (object_field_is_routine_chain(the_object, field_num))
          {
            bind_routine_chain(field_name,
                    object_field_routine_chain(the_object, field_num),
                    hook_data, the_context, the_jumper, location);
          }
        else
          {
            bind_instance(field_name,
                    object_field_instance(the_object, field_num), hook_data,
                    the_context, the_jumper, location);
          }
        if (!(jumper_flowing_forward(the_jumper)))
            return;
      }
  }

static void bind_instance(const char *init_name, instance *target,
        manager_hook_data *hook_data, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    declaration *the_declaration;
    const char *name;
    verdict the_verdict;

    assert(target != NULL);
    assert(hook_data != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    the_declaration = glue_context_add_instance(hook_data->context, target);
    if (the_declaration == NULL)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    if (init_name != NULL)
      {
        name = init_name;
      }
    else
      {
        name = declaration_name(the_declaration);
        if (name == NULL)
          {
            no_name(the_declaration, the_context, the_jumper, location);
            return;
          }
      }

    switch (declaration_kind(the_declaration))
      {
        case NK_VARIABLE:
          {
            the_verdict = bind_variable_name(hook_data->manager, name,
                    declaration_variable_declaration(the_declaration));
            break;
          }
        case NK_ROUTINE:
          {
            routine_declaration_chain *chain;

            chain = create_routine_declaration_chain(
                    declaration_routine_declaration(the_declaration), NULL);
            if (chain == NULL)
              {
                jumper_do_abort(the_jumper);
                return;
              }

            the_verdict = bind_routine_name(hook_data->manager, name, chain);
            routine_declaration_chain_remove_reference(chain);
            break;
          }
        case NK_TAGALONG:
          {
            the_verdict = bind_tagalong_name(hook_data->manager, name,
                    declaration_tagalong_declaration(the_declaration));
            break;
          }
        case NK_LEPTON_KEY:
          {
            the_verdict = bind_lepton_key_name(hook_data->manager, name,
                    declaration_lepton_key_declaration(the_declaration));
            break;
          }
        case NK_QUARK:
          {
            the_verdict = bind_quark_name(hook_data->manager, name,
                    declaration_quark_declaration(the_declaration));
            break;
          }
        case NK_LOCK:
          {
            the_verdict = bind_lock_name(hook_data->manager, name,
                    declaration_lock_declaration(the_declaration));
            break;
          }
        case NK_JUMP_TARGET:
          {
            assert(FALSE);
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    if (the_verdict != MISSION_ACCOMPLISHED)
        jumper_do_abort(the_jumper);
  }

static void bind_routine_chain(const char *init_name,
        routine_instance_chain *target, manager_hook_data *hook_data,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    routine_declaration_chain *chain;
    const char *name;
    verdict the_verdict;

    assert(target != NULL);
    assert(hook_data != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    chain = chain_to_glue_chain(target, hook_data->context);
    if (chain == NULL)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    if (init_name != NULL)
      {
        name = init_name;
      }
    else
      {
        declaration *the_declaration;

        the_declaration = routine_declaration_declaration(
                routine_declaration_chain_declaration(chain));
        name = declaration_name(the_declaration);
        if (name == NULL)
          {
            no_name(the_declaration, the_context, the_jumper, location);
            return;
          }
      }

    the_verdict = bind_routine_name(hook_data->manager, name, chain);
    routine_declaration_chain_remove_reference(chain);
    if (the_verdict != MISSION_ACCOMPLISHED)
        jumper_do_abort(the_jumper);
  }

static routine_declaration_chain *chain_to_glue_chain(
        routine_instance_chain *pre_glue, context *glue_context)
  {
    routine_instance_chain *next_pre;
    routine_declaration_chain *next_result;
    declaration *the_declaration;
    routine_declaration_chain *result;

    assert(pre_glue != NULL);
    assert(glue_context != NULL);

    next_pre = routine_instance_chain_next(pre_glue);
    if (next_pre == NULL)
      {
        next_result = NULL;
      }
    else
      {
        next_result = chain_to_glue_chain(next_pre, glue_context);
        if (next_result == NULL)
            return NULL;
      }

    the_declaration = glue_context_add_instance(glue_context,
            routine_instance_instance(
                    routine_instance_chain_instance(pre_glue)));
    if (the_declaration == NULL)
      {
        if (next_result != NULL)
            routine_declaration_chain_remove_reference(next_result);
        return NULL;
      }

    result = create_routine_declaration_chain(
            declaration_routine_declaration(the_declaration), next_result);
    if (next_result != NULL)
        routine_declaration_chain_remove_reference(next_result);
    return result;
  }

static void no_name(declaration *the_declaration, context *the_context,
                    jumper *the_jumper, const source_location *location)
  {
    const char *kind_name;
    static_exception_tag tag_info;

    switch (declaration_kind(the_declaration))
      {
        case NK_VARIABLE:
            kind_name = "variable";
            break;
        case NK_ROUTINE:
            kind_name = "routine";
            break;
        case NK_TAGALONG:
            kind_name = "tagalong key";
            break;
        case NK_LEPTON_KEY:
            kind_name = "lepton key";
            break;
        case NK_QUARK:
            kind_name = "quark";
            break;
        case NK_LOCK:
            kind_name = "lock";
            break;
        case NK_JUMP_TARGET:
            assert(FALSE);
            break;
        default:
            assert(FALSE);
      }

    tag_info.field_name = NULL;
    tag_info.u.quark = find_tag(the_context, "et_parse_bind_no_name");
    location_exception(the_jumper, location, &tag_info,
            "In bind(), an attempt was made to bind an un-named %s without "
            "providing a name.", kind_name);
  }

static void check_for_unbound(unbound_name_manager *manager,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    size_t name_count;
    size_t name_num;

    assert(manager != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    name_count = unbound_name_count(manager);

    for (name_num = 0; name_num < name_count; ++name_num)
      {
        unbound_name *this_unbound;
        size_t use_count;
        size_t use_num;

        this_unbound = unbound_name_number(manager, name_num);
        use_count = unbound_name_use_count(this_unbound);
        for (use_num = 0; use_num < use_count; ++use_num)
          {
            unbound_use *this_use;

            this_use = unbound_name_use_by_number(this_unbound, use_num);
            switch (get_unbound_use_kind(this_use))
              {
                case UUK_ROUTINE_FOR_RETURN_STATEMENT:
                case UUK_ROUTINE_FOR_EXPORT_STATEMENT:
                case UUK_ROUTINE_FOR_HIDE_STATEMENT:
                case UUK_ROUTINE_FOR_ARGUMENTS_EXPRESSION:
                case UUK_ROUTINE_FOR_THIS_EXPRESSION:
                case UUK_LOOP_FOR_BREAK_STATEMENT:
                case UUK_LOOP_FOR_CONTINUE_STATEMENT:
                case UUK_LOOP_FOR_BREAK_EXPRESSION:
                case UUK_LOOP_FOR_CONTINUE_EXPRESSION:
                  {
                    static_exception_tag tag_info;

                    tag_info.field_name = NULL;
                    tag_info.u.quark =
                            find_tag(the_context, "et_parse_unbound");
                    location_exception(the_jumper, location, &tag_info,
                            "An attempt was made to execute parsed code while "
                            "`%s' was still unbound.",
                            unbound_name_string(this_unbound));
                    return;
                  }
                case UUK_ROUTINE_FOR_OPERATOR_EXPRESSION:
                case UUK_ROUTINE_FOR_OPERATOR_STATEMENT:
                case UUK_DANGLING_OVERLOADED_ROUTINE:
                case UUK_USE_FLOW_THROUGH:
                case UUK_VARIABLE_FOR_EXPRESSION:
                  {
                    break;
                  }
                default:
                  {
                    assert(FALSE);
                  }
              }
          }
      }
  }

static void manager_cleaner(void *hook, jumper *the_jumper)
  {
    manager_hook_data *hook_data;

    assert(hook != NULL);
    assert(the_jumper != NULL);

    hook_data = (manager_hook_data *)hook;
    assert(hook_data != NULL);
    context_remove_reference(hook_data->context, the_jumper);
    free(hook_data);
  }

static void expression_cleaner(void *hook, jumper *the_jumper)
  {
    expression_hook_data *hook_data;

    assert(hook != NULL);
    assert(the_jumper != NULL);

    hook_data = (expression_hook_data *)hook;
    assert(hook_data != NULL);
    delete_open_expression(hook_data->open_expression);
    free(hook_data);
  }

static void statement_block_cleaner(void *hook, jumper *the_jumper)
  {
    statement_block_hook_data *hook_data;

    assert(hook != NULL);
    assert(the_jumper != NULL);

    hook_data = (statement_block_hook_data *)hook;
    assert(hook_data != NULL);
    delete_open_statement_block(hook_data->open_statement_block);
    free(hook_data);
  }
