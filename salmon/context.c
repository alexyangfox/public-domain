/* file "context.c" */

/*
 *  This file contains the implementation of the context module.
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
#include "c_foundations/memory_allocation.h"
#include "c_foundations/string_index.h"
#include "c_foundations/auto_array.h"
#include "c_foundations/auto_array_implementation.h"
#include "context.h"
#include "type.h"
#include "statement.h"
#include "statement_block.h"
#include "variable_instance.h"
#include "routine_instance.h"
#include "tagalong_key.h"
#include "lepton_key_instance.h"
#include "quark.h"
#include "lock_instance.h"
#include "jump_target.h"
#include "source_location.h"
#include "jumper.h"
#include "formal_arguments.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "tagalong_declaration.h"
#include "lepton_key_declaration.h"
#include "quark_declaration.h"
#include "lock_declaration.h"
#include "execute.h"
#include "object.h"
#include "driver.h"
#include "purity_level.h"
#include "reference_cluster.h"
#include "platform_dependent.h"


AUTO_ARRAY(instance_aa, instance *);
AUTO_ARRAY(object_aa, object *);
AUTO_ARRAY(variable_instance_aa, variable_instance *);
AUTO_ARRAY(jump_target_aa, jump_target *);
AUTO_ARRAY(declaration_aa, declaration *);


typedef enum context_kind
  {
    CK_TOP_LEVEL,
    CK_STATEMENT_BLOCK,
    CK_ROUTINE,
    CK_LOOP_CONSTRUCT,
    CK_BLOCK_EXPRESSION,
    CK_TRY_CATCH_STATEMENT,
    CK_STATIC,
    CK_SINGLETON_VARIABLE,
    CK_GLUE
  } context_kind;

struct context
  {
    context_kind kind;
    context *next;
    size_t depth;
    instance_aa *static_instances;
    object_aa virtual_children;
    boolean exited;
    union
      {
        struct
          {
            value *arguments;
            jump_target *return_target;
            DECLARE_SYSTEM_LOCK(return_lock);
            value *return_value;
            const char *directory_paths;
            const char *executable_directory;
          } top_level;
        struct
          {
            statement_block *statement_block;
            instance_aa local_instances;
            instance_aa extra_instances;
            jump_target_aa labels;
            string_index *label_index;
            use_instance **use_instances;
            reference_cluster *reference_cluster;
          } statement_block;
        struct
          {
            routine_instance *instance;
            routine_declaration *declaration;
            variable_instance_aa parameters;
            jump_target *return_target;
            boolean expect_return_value;
            type *dynamic_return_type;
            DECLARE_SYSTEM_LOCK(return_lock);
            value *return_value;
            value *all_arguments_value;
            object *this_object;
          } routine;
        struct
          {
            void *loop_construct;
            variable_instance *index;
            jump_target *continue_target;
            jump_target *break_target;
          } loop_construct;
        struct
          {
            expression *block_expression;
            jump_target *return_target;
            DECLARE_SYSTEM_LOCK(return_lock);
            value *return_value;
          } block_expression;
        struct
          {
            statement *try_catch_statement;
            jump_target *default_catch_target;
            jump_target **catch_targets;
          } try_catch_statement;
        struct
          {
            static_home *static_home;
            instance_aa instances;
          } static_home;
        struct
          {
            declaration *declaration;
            instance *instance;
          } singleton_variable;
        struct
          {
            declaration_aa declarations;
            instance_aa instances;
          } glue;
      } u;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
    size_t internal_reference_count;
  };


AUTO_ARRAY_IMPLEMENTATION(instance_aa, instance *, 0);
AUTO_ARRAY_IMPLEMENTATION(object_aa, object *, 0);
AUTO_ARRAY_IMPLEMENTATION(variable_instance_aa, variable_instance *, 0);
AUTO_ARRAY_IMPLEMENTATION(jump_target_aa, jump_target *, 0);


static context *create_empty_context(context *parent);
static void finish_context(context *the_context, jumper *the_jumper,
                           size_t new_internal_reference_count);
static void delete_context(context *the_context);
static void check_routine_return_value_internal(context *the_context,
        value *return_value, boolean unlock, const source_location *location,
        jumper *the_jumper);


extern context *create_top_level_context(value *arguments)
  {
    context *result;

    assert(arguments != NULL);

    result = create_empty_context(NULL);
    if (result == NULL)
        return NULL;

    result->kind = CK_TOP_LEVEL;

    result->u.top_level.arguments = arguments;

    result->u.top_level.return_target =
            create_top_level_return_jump_target(result);
    if (result->u.top_level.return_target == NULL)
      {
        free(result);
        return NULL;
      }

    INITIALIZE_SYSTEM_LOCK(result->u.top_level.return_lock,
            jump_target_remove_reference(result->u.top_level.return_target);
            free(result);
            return NULL);

    result->u.top_level.return_value = NULL;
    value_add_reference(arguments);

    result->u.top_level.directory_paths = NULL;
    result->u.top_level.executable_directory = NULL;

    return result;
  }

extern context *create_statement_block_context(context *parent,
        statement_block *the_statement_block, reference_cluster *cluster,
        virtual_lookup *virtual_parent, jumper *the_jumper)
  {
    purity_level *level;
    context *result;
    verdict the_verdict;
    size_t use_statement_count;
    size_t declaration_count;
    size_t declaration_num;
    size_t statement_count;
    size_t statement_num;

    assert(the_statement_block != NULL);
    assert(the_jumper != NULL);

    level = jumper_purity_level(the_jumper);
    assert(level != NULL);

    result = create_empty_context(parent);
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result->kind = CK_STATEMENT_BLOCK;

    result->u.statement_block.statement_block = the_statement_block;

    the_verdict =
            instance_aa_init(&(result->u.statement_block.local_instances), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        free(result);
        return NULL;
      }

    the_verdict =
            instance_aa_init(&(result->u.statement_block.extra_instances), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        free(result->u.statement_block.local_instances.array);
        free(result);
        return NULL;
      }

    the_verdict = jump_target_aa_init(&(result->u.statement_block.labels), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        free(result->u.statement_block.extra_instances.array);
        free(result->u.statement_block.local_instances.array);
        free(result);
        return NULL;
      }

    result->u.statement_block.label_index = create_string_index();
    if (result->u.statement_block.label_index == NULL)
      {
        jumper_do_abort(the_jumper);
        free(result->u.statement_block.labels.array);
        free(result->u.statement_block.extra_instances.array);
        free(result->u.statement_block.local_instances.array);
        free(result);
        return NULL;
      }

    use_statement_count =
            statement_block_use_statement_count(the_statement_block);
    if (use_statement_count == 0)
      {
        result->u.statement_block.use_instances = NULL;
      }
    else
      {
        use_instance **use_instances;
        size_t use_statement_num;

        use_instances = MALLOC_ARRAY(use_instance *, use_statement_count);
        if (use_instances == NULL)
          {
            jumper_do_abort(the_jumper);
            destroy_string_index(result->u.statement_block.label_index);
            free(result->u.statement_block.labels.array);
            free(result->u.statement_block.extra_instances.array);
            free(result->u.statement_block.local_instances.array);
            free(result);
            return NULL;
          }

        for (use_statement_num = 0; use_statement_num < use_statement_count;
             ++use_statement_num)
          {
            statement *use_statement;
            use_instance *the_use_instance;

            use_statement = statement_block_use_statement(the_statement_block,
                                                          use_statement_num);
            assert(use_statement != NULL);

            the_use_instance = create_use_instance(
                    use_statement_used_for_count(use_statement), cluster);
            if (the_use_instance == NULL)
              {
                jumper_do_abort(the_jumper);
                while (use_statement_num > 0)
                  {
                    --use_statement_num;
                    delete_use_instance(use_instances[use_statement_num],
                                        the_jumper);
                  }
                free(use_instances);
                destroy_string_index(result->u.statement_block.label_index);
                free(result->u.statement_block.labels.array);
                free(result->u.statement_block.extra_instances.array);
                free(result->u.statement_block.local_instances.array);
                free(result);
                return NULL;
              }

            use_instances[use_statement_num] = the_use_instance;
          }

        result->u.statement_block.use_instances = use_instances;
      }

    result->u.statement_block.reference_cluster = cluster;

    context_add_reference(parent);

    declaration_count =
            statement_block_declaration_count(the_statement_block);
    for (declaration_num = 0; declaration_num < declaration_count;
         ++declaration_num)
      {
        declaration *the_declaration;
        boolean instance_created_here;
        instance *the_instance;
        verdict the_verdict;

        the_declaration = statement_block_declaration_by_number(
                the_statement_block, declaration_num);
        assert(the_declaration != NULL);

        instance_created_here = FALSE;

        if (declaration_is_virtual(the_declaration))
          {
            context *owning_context;

            the_instance = virtual_lookup_find_instance(virtual_parent,
                    declaration_name(the_declaration),
                    declaration_kind(the_declaration), &owning_context);

            if ((the_instance == NULL) &&
                (declaration_kind(the_declaration) == NK_ROUTINE))
              {
                routine_declaration *the_routine_declaration;

                the_routine_declaration =
                        declaration_routine_declaration(the_declaration);
                if ((routine_declaration_body(the_routine_declaration) == NULL)
                    &&
                    (routine_declaration_native_handler(
                             the_routine_declaration) == NULL))
                  {
                    location_exception(the_jumper,
                            get_declaration_location(the_declaration),
                            EXCEPTION_TAG(pure_virtual_no_override),
                            "The statement block containing a pure virtual "
                            "routine (%a) was entered without an override for "
                            "that routine.", the_declaration);
                    context_remove_reference(result, the_jumper);
                    return NULL;
                  }
              }
          }
        else
          {
            the_instance = NULL;
          }

        if (the_instance == NULL)
          {
            if (declaration_is_static(the_declaration))
              {
                size_t static_num;

                assert(result->static_instances != NULL);

                static_num = declaration_static_parent_index(the_declaration);
                assert(static_num < result->static_instances->element_count);

                the_instance = result->static_instances->array[static_num];
              }
            else
              {
                the_instance = create_instance_for_declaration(the_declaration,
                                                               level, cluster);
                if (the_instance == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    context_remove_reference(result, the_jumper);
                    return NULL;
                  }
                instance_created_here = TRUE;
                assert(!(instance_scope_exited(the_instance))); /* VERIFIED */
              }
          }

        the_verdict = instance_aa_append(
                &(result->u.statement_block.local_instances), the_instance);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            if (instance_declaration(the_instance) == the_declaration)
              {
                assert(instance_created_here);
                assert(!(instance_scope_exited(the_instance))); /* VERIFIED */
                set_instance_scope_exited(the_instance, the_jumper);
                instance_remove_reference_with_cluster(the_instance, cluster,
                                                       the_jumper);
              }
            context_remove_reference(result, the_jumper);
            return NULL;
          }
      }

    statement_count = statement_block_statement_count(the_statement_block);
    for (statement_num = 0; statement_num < statement_count; ++statement_num)
      {
        statement *the_statement;

        the_statement =
                statement_block_statement(the_statement_block, statement_num);
        switch (get_statement_kind(the_statement))
          {
            case SK_LABEL:
              {
                jump_target *target;
                verdict the_verdict;

                target = create_label_jump_target(result, the_statement);
                if (target == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    context_remove_reference(result, the_jumper);
                    return NULL;
                  }

                the_verdict = jump_target_aa_append(
                        &(result->u.statement_block.labels), target);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    jump_target_remove_reference(target);
                    context_remove_reference(result, the_jumper);
                    return NULL;
                  }

                the_verdict = enter_into_string_index(
                        result->u.statement_block.label_index,
                        label_statement_name(the_statement), target);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    context_remove_reference(result, the_jumper);
                    return NULL;
                  }

                break;
              }
            default:
              {
                break;
              }
          }
      }

    return result;
  }

extern context *create_routine_context(context *parent,
        routine_instance *the_routine_instance, boolean expect_return_value,
        purity_level *level)
  {
    routine_declaration *declaration;
    context *result;
    verdict the_verdict;
    formal_arguments *formals;
    size_t formal_count;
    size_t formal_num;

    assert(the_routine_instance != NULL);
    assert(level != NULL);

    declaration = routine_instance_declaration(the_routine_instance);
    assert(declaration != NULL);

    result = create_empty_context(parent);
    if (result == NULL)
        return NULL;

    result->kind = CK_ROUTINE;

    result->u.routine.instance = the_routine_instance;
    result->u.routine.declaration = declaration;

    the_verdict =
            variable_instance_aa_init(&(result->u.routine.parameters), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    result->u.routine.return_target =
            create_routine_return_jump_target(result, declaration);
    if (result->u.routine.return_target == NULL)
      {
        free(result->u.routine.parameters.array);
        free(result);
        return NULL;
      }

    INITIALIZE_SYSTEM_LOCK(result->u.routine.return_lock,
            jump_target_remove_reference(result->u.routine.return_target);
            free(result);
            return NULL);

    result->u.routine.expect_return_value = expect_return_value;
    result->u.routine.dynamic_return_type = NULL;
    result->u.routine.return_value = NULL;
    result->u.routine.all_arguments_value = NULL;
    result->u.routine.this_object = NULL;

    formals = routine_declaration_formals(declaration);
    formal_count = formal_arguments_argument_count(formals);
    for (formal_num = 0; formal_num < formal_count; ++formal_num)
      {
        variable_declaration *declaration;
        variable_instance *instance;
        verdict the_verdict;

        declaration = formal_arguments_formal_by_number(formals, formal_num);
        assert(declaration != NULL);
        instance = create_variable_instance(declaration, level, NULL);
        if (instance == NULL)
          {
            context_remove_reference(result, NULL);
            return NULL;
          }

        the_verdict = variable_instance_aa_append(
                &(result->u.routine.parameters), instance);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            assert(!(variable_instance_scope_exited(instance))); /* VERIFIED */
            set_variable_instance_scope_exited(instance, NULL);
            variable_instance_remove_reference(instance, NULL);
            context_remove_reference(result, NULL);
            return NULL;
          }
      }

    context_add_reference(parent);

    return result;
  }

extern context *create_loop_context(context *parent, void *loop_construct,
        variable_declaration *element_declaration, purity_level *level,
        const source_location *location)
  {
    context *result;
    variable_instance *index;
    jump_target *continue_target;
    jump_target *break_target;

    assert(loop_construct != NULL);
    assert(level != NULL);

    result = create_empty_context(parent);
    if (result == NULL)
        return NULL;

    result->kind = CK_LOOP_CONSTRUCT;

    if (element_declaration != NULL)
      {
        index = create_variable_instance(element_declaration, level, NULL);
        if (index == NULL)
          {
            free(result);
            return NULL;
          }
      }
    else
      {
        index = NULL;
      }

    assert((index == NULL) || !(variable_instance_scope_exited(index)));
            /* VERIFIED */

    continue_target =
            create_loop_continue_jump_target(result, loop_construct, location);
    if (continue_target == NULL)
      {
        if (index != NULL)
          {
            assert(!(variable_instance_scope_exited(index))); /* VERIFIED */
            set_variable_instance_scope_exited(index, NULL);
            variable_instance_remove_reference(index, NULL);
          }
        free(result);
        return NULL;
      }

    break_target =
            create_loop_break_jump_target(result, loop_construct, location);
    if (break_target == NULL)
      {
        jump_target_remove_reference(continue_target);
        if (index != NULL)
          {
            assert(!(variable_instance_scope_exited(index))); /* VERIFIED */
            set_variable_instance_scope_exited(index, NULL);
            variable_instance_remove_reference(index, NULL);
          }
        free(result);
        return NULL;
      }

    result->u.loop_construct.loop_construct = loop_construct;
    result->u.loop_construct.index = index;
    result->u.loop_construct.continue_target = continue_target;
    result->u.loop_construct.break_target = break_target;

    context_add_reference(parent);

    return result;
  }

extern context *create_block_expression_context(context *parent,
                                                expression *the_expression)
  {
    context *result;

    assert(the_expression != NULL);

    result = create_empty_context(parent);
    if (result == NULL)
        return NULL;

    result->kind = CK_BLOCK_EXPRESSION;

    result->u.block_expression.block_expression = the_expression;

    result->u.block_expression.return_target =
            create_block_expression_return_jump_target(result, the_expression);
    if (result->u.block_expression.return_target == NULL)
      {
        free(result);
        return NULL;
      }

    INITIALIZE_SYSTEM_LOCK(result->u.block_expression.return_lock,
            jump_target_remove_reference(
                    result->u.block_expression.return_target);
            free(result);
            return NULL);

    result->u.block_expression.return_value = NULL;

    context_add_reference(parent);

    return result;
  }

extern context *create_try_catch_statement_context(context *parent,
        statement *try_catch_statement)
  {
    context *result;
    size_t tagged_count;

    assert(try_catch_statement != NULL);

    result = create_empty_context(parent);
    if (result == NULL)
        return NULL;

    result->kind = CK_TRY_CATCH_STATEMENT;

    result->u.try_catch_statement.try_catch_statement = try_catch_statement;

    if (try_catch_statement_catcher(try_catch_statement) != NULL)
      {
        jump_target *catch_target;

        catch_target = create_try_catch_catch_jump_target(result,
                                                          try_catch_statement);
        if (catch_target == NULL)
          {
            free(result);
            return NULL;
          }

        result->u.try_catch_statement.default_catch_target = catch_target;
      }
    else
      {
        result->u.try_catch_statement.default_catch_target = NULL;
      }

    tagged_count =
            try_catch_statement_tagged_catcher_count(try_catch_statement);
    if (tagged_count == 0)
      {
        result->u.try_catch_statement.catch_targets = NULL;
      }
    else
      {
        jump_target **catch_targets;
        size_t tagged_num;

        catch_targets = MALLOC_ARRAY(jump_target *, tagged_count);
        if (catch_targets == NULL)
          {
          cleanup:
            if (result->u.try_catch_statement.default_catch_target != NULL)
              {
                jump_target_remove_reference(
                        result->u.try_catch_statement.default_catch_target);
              }
            free(result);
            return NULL;
          }

        for (tagged_num = 0; tagged_num < tagged_count; ++tagged_num)
          {
            catch_targets[tagged_num] = create_try_catch_catch_jump_target(
                    result, try_catch_statement);
            if (catch_targets[tagged_num] == NULL)
              {
                while (tagged_num > 0)
                  {
                    --tagged_num;
                    jump_target_remove_reference(catch_targets[tagged_num]);
                  }
                goto cleanup;
              }
          }

        result->u.try_catch_statement.catch_targets = catch_targets;
      }

    context_add_reference(parent);

    return result;
  }

extern context *create_static_context(context *parent,
        static_home *the_static_home, jumper *the_jumper)
  {
    context *result;
    verdict the_verdict;
    size_t declaration_count;
    size_t declaration_num;

    assert(the_static_home != NULL);
    assert(the_jumper != NULL);

    result = create_empty_context(parent);
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result->kind = CK_STATIC;

    result->u.static_home.static_home = the_static_home;

    the_verdict = instance_aa_init(&(result->u.static_home.instances), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        free(result);
        return NULL;
      }

    result->static_instances = &(result->u.static_home.instances);

    declaration_count = static_home_declaration_count(the_static_home);
    for (declaration_num = 0; declaration_num < declaration_count;
         ++declaration_num)
      {
        declaration *the_declaration;
        instance *the_instance;
        verdict the_verdict;

        the_declaration = static_home_declaration_by_number(the_static_home,
                                                            declaration_num);
        assert(the_declaration != NULL);

        the_instance = create_instance_for_declaration(the_declaration,
                jumper_purity_level(the_jumper), NULL);
        if (the_instance == NULL)
          {
            jumper_do_abort(the_jumper);
            exit_context(result, the_jumper);
            return NULL;
          }

        the_verdict = instance_aa_append(&(result->u.static_home.instances),
                                         the_instance);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            assert(!(instance_scope_exited(the_instance))); /* VERIFIED */
            set_instance_scope_exited(the_instance, the_jumper);
            instance_remove_reference(the_instance, the_jumper);
            exit_context(result, the_jumper);
            return NULL;
          }
      }

    for (declaration_num = 0; declaration_num < declaration_count;
         ++declaration_num)
      {
        declaration *the_declaration;
        instance *the_instance;

        the_declaration = static_home_declaration_by_number(the_static_home,
                                                            declaration_num);
        assert(the_declaration != NULL);

        the_instance = result->u.static_home.instances.array[declaration_num];
        assert(the_instance != NULL);

        execute_declaration(the_declaration, the_instance, result, the_jumper,
                            NULL);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            exit_context(result, the_jumper);
            return NULL;
          }
      }

    return result;
  }

extern context *create_singleton_variable_context(context *parent,
        variable_declaration *the_variable_declaration, purity_level *level)
  {
    context *result;
    declaration *the_declaration;

    result = create_empty_context(parent);
    if (result == NULL)
        return NULL;

    result->kind = CK_SINGLETON_VARIABLE;

    the_declaration =
            variable_declaration_declaration(the_variable_declaration);
    result->u.singleton_variable.declaration = the_declaration;
    result->u.singleton_variable.instance = create_instance_for_declaration(
            the_declaration, level, NULL);
    if (result->u.singleton_variable.instance == NULL)
      {
        free(result);
        return NULL;
      }

    context_add_reference(parent);

    return result;
  }

extern context *create_glue_context()
  {
    context *result;
    verdict the_verdict;

    result = create_empty_context(NULL);
    if (result == NULL)
        return NULL;

    result->kind = CK_GLUE;

    the_verdict = declaration_aa_init(&(result->u.glue.declarations), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    the_verdict = instance_aa_init(&(result->u.glue.instances), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->u.glue.declarations.array);
        free(result);
        return NULL;
      }

    return result;
  }

extern void exit_context(context *the_context, jumper *the_jumper)
  {
    size_t virtual_child_count;
    size_t virtual_child_num;

    assert(the_context != NULL);

    assert(the_context->reference_count > 0);

    if (the_context->exited)
      {
        context_remove_reference(the_context, the_jumper);
        return;
      }

    switch (the_context->kind)
      {
        case CK_TOP_LEVEL:
          {
            assert(the_context->u.top_level.arguments != NULL);
            value_remove_reference(the_context->u.top_level.arguments,
                                   the_jumper);
            the_context->u.top_level.arguments = NULL;

            assert(the_context->u.top_level.return_target != NULL);
            assert(!(jump_target_scope_exited(
                    the_context->u.top_level.return_target))); /* VERIFIED */
            set_jump_target_scope_exited(
                    the_context->u.top_level.return_target);

            break;
          }
        case CK_STATEMENT_BLOCK:
          {
            statement_block *the_statement_block;
            instance **local_instances;
            size_t local_count;
            size_t local_num;
            instance **extra_instances;
            size_t extra_count;
            size_t extra_num;
            jump_target **labels;
            size_t label_count;
            size_t label_num;

            the_statement_block =
                    the_context->u.statement_block.statement_block;

            local_instances =
                    the_context->u.statement_block.local_instances.array;
            local_count = the_context->u.statement_block.local_instances.
                    element_count;
            for (local_num = 0; local_num < local_count; ++local_num)
              {
                instance *this_instance;
                declaration *this_declaration;

                this_instance = local_instances[local_num];
                this_declaration = statement_block_declaration_by_number(
                        the_statement_block, local_num);
                if (instance_declaration(this_instance) == this_declaration)
                  {
                    if ((!(declaration_is_static(this_declaration))) &&
                        declaration_automatic_allocation(this_declaration))
                      {
                        assert(!(instance_scope_exited(this_instance)));
                                /* VERIFIED */
                        set_instance_scope_exited(this_instance, the_jumper);
                      }
                  }
              }

            extra_instances =
                    the_context->u.statement_block.extra_instances.array;
            extra_count = the_context->u.statement_block.extra_instances.
                    element_count;
            for (extra_num = 0; extra_num < extra_count; ++extra_num)
              {
                instance *this_instance;

                this_instance = extra_instances[extra_num];
                assert(!(instance_scope_exited(this_instance))); /* VERIFIED */
                set_instance_scope_exited(this_instance, the_jumper);
              }

            labels = the_context->u.statement_block.labels.array;
            label_count = the_context->u.statement_block.labels.element_count;
            for (label_num = 0; label_num < label_count; ++label_num)
              {
                assert(labels[label_num] != NULL);
                assert(!(jump_target_scope_exited(labels[label_num])));
                        /* VERIFIED */
                set_jump_target_scope_exited(labels[label_num]);
              }

            break;
          }
        case CK_ROUTINE:
          {
            variable_instance **parameters;
            size_t parameter_count;
            size_t parameter_num;

            parameters = the_context->u.routine.parameters.array;
            parameter_count = the_context->u.routine.parameters.element_count;
            for (parameter_num = 0; parameter_num < parameter_count;
                 ++parameter_num)
              {
                if (declaration_automatic_allocation(
                            variable_declaration_declaration(
                                    variable_instance_declaration(
                                            parameters[parameter_num]))))
                  {
                    assert(!(variable_instance_scope_exited(
                                     parameters[parameter_num])));
                            /* VERIFIED */
                    set_variable_instance_scope_exited(
                            parameters[parameter_num], the_jumper);
                  }
              }

            if (the_context->u.routine.dynamic_return_type != NULL)
              {
                type_remove_reference(
                        the_context->u.routine.dynamic_return_type,
                        the_jumper);
                the_context->u.routine.dynamic_return_type = NULL;
              }

            if (the_context->u.routine.all_arguments_value != NULL)
              {
                value_remove_reference(
                        the_context->u.routine.all_arguments_value,
                        the_jumper);
                the_context->u.routine.all_arguments_value = NULL;
              }

            the_context->u.routine.this_object = NULL;

            assert(the_context->u.routine.return_target != NULL);
            assert(!(jump_target_scope_exited(
                    the_context->u.routine.return_target))); /* VERIFIED */
            set_jump_target_scope_exited(the_context->u.routine.return_target);

            break;
          }
        case CK_LOOP_CONSTRUCT:
          {
            if (the_context->u.loop_construct.index != NULL)
              {
                assert(!(variable_instance_scope_exited(
                                 the_context->u.loop_construct.index)));
                        /* VERIFIED */
                set_variable_instance_scope_exited(
                        the_context->u.loop_construct.index, the_jumper);
              }

            assert(the_context->u.loop_construct.continue_target != NULL);
            assert(!(jump_target_scope_exited(
                    the_context->u.loop_construct.continue_target)));
                            /* VERIFIED */
            set_jump_target_scope_exited(
                    the_context->u.loop_construct.continue_target);
            assert(the_context->u.loop_construct.break_target != NULL);
            assert(!(jump_target_scope_exited(
                    the_context->u.loop_construct.break_target)));
                            /* VERIFIED */
            set_jump_target_scope_exited(
                    the_context->u.loop_construct.break_target);

            break;
          }
        case CK_BLOCK_EXPRESSION:
          {
            assert(the_context->u.block_expression.return_target != NULL);
            assert(!(jump_target_scope_exited(
                    the_context->u.block_expression.return_target)));
                            /* VERIFIED */
            set_jump_target_scope_exited(
                    the_context->u.block_expression.return_target);
            break;
          }
        case CK_TRY_CATCH_STATEMENT:
          {
            jump_target **catch_targets;

            if (the_context->u.try_catch_statement.default_catch_target !=
                NULL)
              {
                assert(!(jump_target_scope_exited(
                        the_context->u.try_catch_statement.
                                default_catch_target))); /* VERIFIED */
                set_jump_target_scope_exited(
                        the_context->u.try_catch_statement.
                                default_catch_target);
              }

            catch_targets = the_context->u.try_catch_statement.catch_targets;
            if (catch_targets != NULL)
              {
                size_t tagged_count;
                size_t tagged_num;

                tagged_count = try_catch_statement_tagged_catcher_count(
                        the_context->u.try_catch_statement.
                                try_catch_statement);
                assert(tagged_count > 0);
                for (tagged_num = 0; tagged_num < tagged_count; ++tagged_num)
                    set_jump_target_scope_exited(catch_targets[tagged_num]);
              }

            break;
          }
        case CK_STATIC:
          {
            instance **instances;
            size_t static_count;
            size_t static_num;

            instances = the_context->u.static_home.instances.array;
            static_count = the_context->u.static_home.instances.element_count;
            for (static_num = 0; static_num < static_count; ++static_num)
              {
                if (declaration_automatic_allocation(
                            instance_declaration(instances[static_num])))
                  {
                    assert(!(instance_scope_exited(instances[static_num])));
                            /* VERIFIED */
                    set_instance_scope_exited(instances[static_num],
                                              the_jumper);
                  }
              }

            break;
          }
        case CK_SINGLETON_VARIABLE:
          {
            set_instance_scope_exited(
                    the_context->u.singleton_variable.instance, the_jumper);
            break;
          }
        case CK_GLUE:
          {
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    virtual_child_count = the_context->virtual_children.element_count;
    for (virtual_child_num = 0; virtual_child_num < virtual_child_count;
         ++virtual_child_num)
      {
        object *child_object;

        child_object = the_context->virtual_children.array[virtual_child_num];
        if (!object_is_closed(child_object))
            close_object(child_object, the_jumper);
        object_remove_reference(child_object, the_jumper);
      }

    the_context->exited = TRUE;

    context_remove_reference(the_context, the_jumper);
  }

extern size_t context_depth(context *the_context)
  {
    if (the_context == NULL)
        return 0;

    return the_context->depth;
  }

extern instance *find_instance(context *the_context,
                               declaration *the_declaration)
  {
    context *follow_context;

    assert(the_declaration != NULL);

    follow_context = the_context;

    while (TRUE)
      {
        if (follow_context == NULL)
            return NULL;

        switch (follow_context->kind)
          {
            case CK_TOP_LEVEL:
              {
                break;
              }
            case CK_STATEMENT_BLOCK:
              {
                size_t index;

                if (declaration_parent_pointer(the_declaration) !=
                    follow_context->u.statement_block.statement_block)
                  {
                    break;
                  }

                index = declaration_parent_index(the_declaration);
                assert(index <=
                       follow_context->u.statement_block.local_instances.
                               element_count);
                return follow_context->u.statement_block.local_instances.array[
                        index];
              }
            case CK_ROUTINE:
              {
                size_t index;

                if (declaration_parent_pointer(the_declaration) !=
                    routine_declaration_formals(
                            follow_context->u.routine.declaration))
                  {
                    break;
                  }

                index = declaration_parent_index(the_declaration);
                assert(index <=
                       follow_context->u.routine.parameters.element_count);
                return variable_instance_instance(
                        follow_context->u.routine.parameters.array[index]);
              }
            case CK_LOOP_CONSTRUCT:
              {
                if (declaration_parent_pointer(the_declaration) !=
                    follow_context->u.loop_construct.loop_construct)
                  {
                    break;
                  }

                assert(follow_context->u.loop_construct.index != NULL);
                assert(variable_instance_declaration(
                               follow_context->u.loop_construct.index) ==
                       declaration_variable_declaration(the_declaration));
                return variable_instance_instance(
                        follow_context->u.loop_construct.index);
              }
            case CK_BLOCK_EXPRESSION:
              {
                break;
              }
            case CK_TRY_CATCH_STATEMENT:
              {
                break;
              }
            case CK_STATIC:
              {
                size_t index;

                if (declaration_static_parent_pointer(the_declaration) !=
                    follow_context->u.static_home.static_home)
                  {
                    break;
                  }

                index = declaration_static_parent_index(the_declaration);
                assert(index <=
                       follow_context->u.static_home.instances.element_count);
                return follow_context->u.static_home.instances.array[index];
              }
            case CK_SINGLETON_VARIABLE:
              {
                if (the_declaration ==
                    follow_context->u.singleton_variable.declaration)
                  {
                    return follow_context->u.singleton_variable.instance;
                  }
                break;
              }
            case CK_GLUE:
              {
                size_t index;

                if (declaration_parent_pointer(the_declaration) !=
                    follow_context)
                  {
                    break;
                  }

                index = declaration_parent_index(the_declaration);
                assert(index < follow_context->u.glue.instances.element_count);
                return follow_context->u.glue.instances.array[index];
              }
            default:
              {
                assert(FALSE);
              }
          }

        follow_context = follow_context->next;
      }
  }

extern variable_instance *find_variable_instance(context *the_context,
        variable_declaration *declaration)
  {
    instance *the_instance;

    assert(declaration != NULL);

    the_instance = find_instance(the_context,
            variable_declaration_declaration(declaration));
    if (the_instance == NULL)
        return NULL;

    return instance_variable_instance(the_instance);
  }

extern routine_instance *find_routine_instance(context *the_context,
        routine_declaration *declaration)
  {
    instance *the_instance;

    assert(declaration != NULL);

    the_instance = find_instance(the_context,
                                 routine_declaration_declaration(declaration));
    if (the_instance == NULL)
        return NULL;

    return instance_routine_instance(the_instance);
  }

extern tagalong_key *find_tagalong_instance(context *the_context,
                                            tagalong_declaration *declaration)
  {
    instance *the_instance;

    assert(declaration != NULL);

    the_instance = find_instance(the_context,
            tagalong_declaration_declaration(declaration));
    if (the_instance == NULL)
        return NULL;

    return instance_tagalong_instance(the_instance);
  }

extern lepton_key_instance *find_lepton_key_instance(context *the_context,
        lepton_key_declaration *declaration)
  {
    instance *the_instance;

    assert(declaration != NULL);

    the_instance = find_instance(the_context,
            lepton_key_declaration_declaration(declaration));
    if (the_instance == NULL)
        return NULL;

    return instance_lepton_key_instance(the_instance);
  }

extern quark *find_quark_instance(context *the_context,
                                  quark_declaration *declaration)
  {
    instance *the_instance;

    assert(declaration != NULL);

    the_instance = find_instance(the_context,
                                 quark_declaration_declaration(declaration));
    if (the_instance == NULL)
        return NULL;

    return instance_quark_instance(the_instance);
  }

extern lock_instance *find_lock_instance(context *the_context,
                                         lock_declaration *declaration)
  {
    instance *the_instance;

    assert(declaration != NULL);

    the_instance = find_instance(the_context,
                                 lock_declaration_declaration(declaration));
    if (the_instance == NULL)
        return NULL;

    return instance_lock_instance(the_instance);
  }

extern jump_target *find_label_instance(context *the_context,
                                        statement *declaration)
  {
    assert(declaration != NULL);

    if (the_context == NULL)
        return NULL;

    switch (the_context->kind)
      {
        case CK_TOP_LEVEL:
          {
            break;
          }
        case CK_STATEMENT_BLOCK:
          {
            jump_target **labels;
            size_t label_count;
            size_t label_num;

            labels = the_context->u.statement_block.labels.array;
            label_count = the_context->u.statement_block.labels.element_count;
            for (label_num = 0; label_num < label_count; ++label_num)
              {
                assert(!(jump_target_scope_exited(labels[label_num])));
                        /* VERIFIED */
                if (label_jump_target_label_statement(labels[label_num]) ==
                    declaration)
                  {
                    return labels[label_num];
                  }
              }

            break;
          }
        case CK_ROUTINE:
          {
            break;
          }
        case CK_LOOP_CONSTRUCT:
          {
            break;
          }
        case CK_BLOCK_EXPRESSION:
          {
            break;
          }
        case CK_TRY_CATCH_STATEMENT:
          {
            break;
          }
        case CK_STATIC:
          {
            break;
          }
        case CK_SINGLETON_VARIABLE:
          {
            break;
          }
        case CK_GLUE:
          {
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    return find_label_instance(the_context->next, declaration);
  }

extern jump_target *find_break_target(context *the_context,
                                      void *loop_construct)
  {
    assert(loop_construct != NULL);

    if (the_context == NULL)
        return NULL;

    if ((the_context->kind == CK_LOOP_CONSTRUCT) &&
        (the_context->u.loop_construct.loop_construct == loop_construct))
      {
        return the_context->u.loop_construct.break_target;
      }

    return find_break_target(the_context->next, loop_construct);
  }

extern jump_target *find_continue_target(context *the_context,
                                         void *loop_construct)
  {
    assert(loop_construct != NULL);

    if (the_context == NULL)
        return NULL;

    if ((the_context->kind == CK_LOOP_CONSTRUCT) &&
        (the_context->u.loop_construct.loop_construct == loop_construct))
      {
        return the_context->u.loop_construct.continue_target;
      }

    return find_continue_target(the_context->next, loop_construct);
  }

extern jump_target *find_routine_return_target(context *the_context,
        routine_declaration *declaration)
  {
    if (the_context == NULL)
        return NULL;

    if ((the_context->kind == CK_TOP_LEVEL) && (declaration == NULL))
      {
        return the_context->u.top_level.return_target;
      }

    if ((the_context->kind == CK_ROUTINE) &&
        (the_context->u.routine.declaration == declaration))
      {
        return the_context->u.routine.return_target;
      }

    return find_routine_return_target(the_context->next, declaration);
  }

extern jump_target *find_block_expression_return_target(context *the_context,
        expression *block_expression)
  {
    assert(block_expression != NULL);

    if (the_context == NULL)
        return NULL;

    if ((the_context->kind == CK_BLOCK_EXPRESSION) &&
        (the_context->u.block_expression.block_expression == block_expression))
      {
        return the_context->u.block_expression.return_target;
      }

    return find_block_expression_return_target(the_context->next,
                                               block_expression);
  }

extern value *find_arguments_value(context *the_context,
                                   routine_declaration *declaration)
  {
    if (the_context == NULL)
        return NULL;

    if ((the_context->kind == CK_ROUTINE) &&
        (the_context->u.routine.declaration == declaration))
      {
        return the_context->u.routine.all_arguments_value;
      }

    if ((the_context->kind == CK_TOP_LEVEL) && (declaration == NULL))
        return the_context->u.top_level.arguments;

    return find_arguments_value(the_context->next, declaration);
  }

extern object *find_this_object_value(context *the_context,
                                      routine_declaration *declaration)
  {
    if (the_context == NULL)
        return NULL;

    if ((the_context->kind == CK_ROUTINE) &&
        (the_context->u.routine.declaration == declaration))
      {
        return the_context->u.routine.this_object;
      }

    return find_this_object_value(the_context->next, declaration);
  }

extern object *find_top_this_object_value(context *the_context, size_t level)
  {
    if (the_context == NULL)
        return NULL;

    if ((the_context->kind == CK_ROUTINE) &&
        routine_declaration_is_class(the_context->u.routine.declaration))
      {
        if (level == 0)
            return the_context->u.routine.this_object;
        else
            return find_top_this_object_value(the_context->next, level - 1);
      }

    return find_top_this_object_value(the_context->next, level);
  }

extern use_instance *find_use_instance(context *the_context,
                                       statement *use_statement)
  {
    assert(use_statement != NULL);
    assert(get_statement_kind(use_statement) == SK_USE);

    if (the_context == NULL)
        return NULL;

    if ((the_context->kind == CK_STATEMENT_BLOCK) &&
        (the_context->u.statement_block.statement_block ==
         use_statement_parent(use_statement)))
      {
        size_t use_num;

        use_num = use_statement_parent_index(use_statement);
        assert(use_num <
               statement_block_use_statement_count(
                       the_context->u.statement_block.statement_block));
        assert(the_context->u.statement_block.use_instances != NULL);
        return the_context->u.statement_block.use_instances[use_num];
      }

    return find_use_instance(the_context->next, use_statement);
  }

extern void set_routine_return_value(context *the_context,
        routine_declaration *declaration, value *return_value,
        const source_location *location, jumper *the_jumper)
  {
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    if ((the_context->kind == CK_TOP_LEVEL) && (declaration == NULL))
      {
        if (return_value == NULL)
            return;

        GRAB_SYSTEM_LOCK(the_context->u.top_level.return_lock);

        if (the_context->u.top_level.return_value != NULL)
          {
            RELEASE_SYSTEM_LOCK(the_context->u.top_level.return_lock);
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(call_two_return_values),
                    "A second return was executed at the top level.");
            value_remove_reference(return_value, the_jumper);
            return;
          }

        the_context->u.top_level.return_value = return_value;

        RELEASE_SYSTEM_LOCK(the_context->u.top_level.return_lock);

        return;
      }

    if ((the_context->kind != CK_ROUTINE) ||
        (the_context->u.routine.declaration != declaration))
      {
        set_routine_return_value(the_context->next, declaration, return_value,
                                 location, the_jumper);
        return;
      }

    GRAB_SYSTEM_LOCK(the_context->u.routine.return_lock);

    if (routine_declaration_is_class(declaration))
      {
        if (the_context->u.routine.return_value == NULL)
          {
            assert(return_value != NULL);
            assert(get_value_kind(return_value) == VK_OBJECT);
            the_context->u.routine.return_value = return_value;
            RELEASE_SYSTEM_LOCK(the_context->u.routine.return_lock);
          }
        else if (return_value != NULL)
          {
            RELEASE_SYSTEM_LOCK(the_context->u.routine.return_lock);
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(call_class_return_value),
                    "A value was returned from a class constructor call.");
            value_remove_reference(return_value, the_jumper);
          }

        return;
      }

    if (the_context->u.routine.expect_return_value)
      {
        if (return_value == NULL)
          {
            RELEASE_SYSTEM_LOCK(the_context->u.routine.return_lock);
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(call_function_no_return_value),
                    "A function call did not return a value.");
            return;
          }

        assert(jumper_flowing_forward(the_jumper));

        check_routine_return_value_internal(the_context, return_value, TRUE,
                                            location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(return_value, the_jumper);
            return;
          }

        if (the_context->u.routine.return_value != NULL)
          {
            RELEASE_SYSTEM_LOCK(the_context->u.routine.return_lock);
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(call_two_return_values),
                    "A second return was executed within a function call.");
            value_remove_reference(return_value, the_jumper);
            return;
          }
      }
    else
      {
        if (return_value != NULL)
          {
            RELEASE_SYSTEM_LOCK(the_context->u.routine.return_lock);
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(call_procedure_return_value),
                    "A value was returned from a procedure call.");
            value_remove_reference(return_value, the_jumper);
            return;
          }

        assert(the_context->u.routine.return_value == NULL);
      }

    the_context->u.routine.return_value = return_value;

    RELEASE_SYSTEM_LOCK(the_context->u.routine.return_lock);

    return;
  }

extern void check_routine_return_value(context *the_context,
        value *return_value, const source_location *location,
        jumper *the_jumper)
  {
    check_routine_return_value_internal(the_context, return_value, FALSE,
                                        location, the_jumper);
  }

extern value *get_routine_return_value(context *the_context,
                                       routine_declaration *declaration)
  {
    assert(the_context != NULL);

    if ((the_context->kind == CK_TOP_LEVEL) && (declaration == NULL))
        return the_context->u.top_level.return_value;

    if ((the_context->kind != CK_ROUTINE) ||
        (the_context->u.routine.declaration != declaration))
      {
        return get_routine_return_value(the_context->next, declaration);
      }

    return the_context->u.routine.return_value;
  }

extern boolean nearest_routine_expects_return_value(context *the_context)
  {
    assert(the_context != NULL);

    if (the_context->kind != CK_ROUTINE)
        return nearest_routine_expects_return_value(the_context->next);

    return the_context->u.routine.expect_return_value;
  }

extern void set_block_expression_return_value(context *the_context,
        expression *block_expression, value *return_value,
        const source_location *location, jumper *the_jumper)
  {
    assert(the_context != NULL);
    assert(block_expression != NULL);
    assert(return_value != NULL);
    assert(the_jumper != NULL);

    if ((the_context->kind != CK_BLOCK_EXPRESSION) ||
        (the_context->u.block_expression.block_expression != block_expression))
      {
        set_block_expression_return_value(the_context->next, block_expression,
                                          return_value, location, the_jumper);
        return;
      }

    assert(return_value != NULL);

    GRAB_SYSTEM_LOCK(the_context->u.block_expression.return_lock);

    if (the_context->u.block_expression.return_value != NULL)
      {
        RELEASE_SYSTEM_LOCK(the_context->u.block_expression.return_lock);
        location_exception(the_jumper, location,
                EXCEPTION_TAG(call_two_return_values),
                "A second return was executed for a block expression.");
        value_remove_reference(return_value, the_jumper);
        return;
      }

    the_context->u.block_expression.return_value = return_value;

    RELEASE_SYSTEM_LOCK(the_context->u.block_expression.return_lock);
  }

extern verdict context_add_extra_instance(context *the_context,
                                          instance *extra)
  {
    verdict the_verdict;

    assert(the_context != NULL);

    assert(the_context->kind == CK_STATEMENT_BLOCK);
    the_verdict = instance_aa_append(
            &(the_context->u.statement_block.extra_instances), extra);
    if (the_verdict == MISSION_ACCOMPLISHED)
      {
        instance_add_reference_with_cluster(extra,
                the_context->u.statement_block.reference_cluster);
      }
    return the_verdict;
  }

extern declaration *glue_context_add_instance(context *the_context,
                                              instance *instance)
  {
    declaration *original_declaration;
    const char *name;
    boolean is_static;
    boolean is_virtual;
    boolean automatic_allocation;
    const source_location *location;
    declaration *the_declaration;
    verdict the_verdict;

    assert(the_context != NULL);
    assert(instance != NULL);

    assert(the_context->kind == CK_GLUE);

    original_declaration = instance_declaration(instance);
    assert(original_declaration != NULL);

    name = declaration_name(original_declaration);
    is_static = declaration_is_static(original_declaration);
    is_virtual = declaration_is_virtual(original_declaration);
    automatic_allocation =
            declaration_automatic_allocation(original_declaration);
    location = get_declaration_location(original_declaration);

    switch (instance_kind(instance))
      {
        case NK_VARIABLE:
          {
            variable_declaration *original_variable_declaration;
            type_expression *the_type_expression;
            variable_declaration *the_variable_declaration;

            original_variable_declaration =
                    declaration_variable_declaration(original_declaration);

            the_type_expression =
                    create_constant_type_expression(get_anything_type());
            if (the_type_expression == NULL)
                return NULL;

            the_variable_declaration = create_variable_declaration(
                    the_type_expression, NULL,
                    variable_declaration_force_type_in_initialization(
                            original_variable_declaration),
                    variable_declaration_is_immutable(
                            original_variable_declaration), NULL);
            if (the_variable_declaration == NULL)
                return NULL;

            the_declaration = create_declaration_for_variable(name, is_static,
                    is_virtual, automatic_allocation, the_variable_declaration,
                    location);
            break;
          }
        case NK_ROUTINE:
          {
            routine_declaration *original_routine_declaration;
            formal_arguments *the_formal_arguments;
            statement_block *body;
            routine_declaration *the_routine_declaration;

            original_routine_declaration =
                    declaration_routine_declaration(original_declaration);

            the_formal_arguments = create_formal_arguments();
            if (the_formal_arguments == NULL)
                return NULL;

            body = create_statement_block();
            if (body == NULL)
              {
                delete_formal_arguments(the_formal_arguments);
                return NULL;
              }

            the_routine_declaration = create_routine_declaration(NULL, NULL,
                    the_formal_arguments,
                    routine_declaration_extra_arguments_allowed(
                            original_routine_declaration), body, NULL,
                    routine_declaration_purity_safety(
                            original_routine_declaration),
                    routine_declaration_is_pure(original_routine_declaration),
                    routine_declaration_is_class(original_routine_declaration),
                    NULL, 0, NULL);
            if (the_routine_declaration == NULL)
                return NULL;

            the_declaration = create_declaration_for_routine(name, is_static,
                    is_virtual, automatic_allocation, the_routine_declaration,
                    location);
            break;
          }
        case NK_TAGALONG:
          {
            tagalong_declaration *original_tagalong_declaration;
            type_expression *type_expression1;
            type_expression *type_expression2;
            tagalong_declaration *the_tagalong_declaration;

            original_tagalong_declaration =
                    declaration_tagalong_declaration(original_declaration);

            type_expression1 =
                    create_constant_type_expression(get_anything_type());
            if (type_expression1 == NULL)
                return NULL;

            type_expression2 =
                    create_constant_type_expression(get_anything_type());
            if (type_expression1 == NULL)
              {
                delete_type_expression(type_expression1);
                return NULL;
              }

            the_tagalong_declaration = create_tagalong_declaration(
                    type_expression1, NULL,
                    tagalong_declaration_force_type_in_initialization(
                            original_tagalong_declaration), NULL,
                    type_expression2,
                    tagalong_declaration_is_object(
                            original_tagalong_declaration));
            if (the_tagalong_declaration == NULL)
                return NULL;

            the_declaration = create_declaration_for_tagalong(name, is_static,
                    is_virtual, automatic_allocation, the_tagalong_declaration,
                    location);
            break;
          }
        case NK_LEPTON_KEY:
          {
            lepton_key_declaration *original_lepton_key_declaration;
            lepton_key_declaration *the_lepton_key_declaration;

            original_lepton_key_declaration =
                    declaration_lepton_key_declaration(original_declaration);

            the_lepton_key_declaration = create_lepton_key_declaration(
                    lepton_key_declaration_additional_fields_allowed(
                            original_lepton_key_declaration));
            if (the_lepton_key_declaration == NULL)
                return NULL;

            the_declaration = create_declaration_for_lepton_key(name,
                    is_static, is_virtual, automatic_allocation,
                    the_lepton_key_declaration, location);
            break;
          }
        case NK_QUARK:
          {
            quark_declaration *the_quark_declaration;

            the_quark_declaration = create_quark_declaration();
            if (the_quark_declaration == NULL)
                return NULL;

            the_declaration = create_declaration_for_quark(name, is_static,
                    is_virtual, automatic_allocation, the_quark_declaration,
                    location);
            break;
          }
        case NK_LOCK:
          {
            lock_declaration *the_lock_declaration;

            the_lock_declaration = create_lock_declaration(NULL);
            if (the_lock_declaration == NULL)
                return NULL;

            the_declaration = create_declaration_for_lock(name, is_static,
                    is_virtual, automatic_allocation, the_lock_declaration,
                    location);
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

    if (the_declaration == NULL)
        return NULL;

    declaration_set_parent_pointer(the_declaration, the_context);
    declaration_set_parent_index(the_declaration,
            the_context->u.glue.declarations.element_count);

    the_verdict = declaration_aa_append(&(the_context->u.glue.declarations),
                                        the_declaration);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        declaration_remove_reference(the_declaration);
        return NULL;
      }

    the_verdict =
            instance_aa_append(&(the_context->u.glue.instances), instance);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(the_context->u.glue.declarations.element_count > 0);
        --(the_context->u.glue.declarations.element_count);
        declaration_remove_reference(the_declaration);
        return NULL;
      }

    instance_add_reference(instance);

    return the_declaration;
  }

extern void top_level_context_set_directory_paths(context *the_context,
                                                  const char *new_value)
  {
    assert(the_context != NULL);

    assert(the_context->kind == CK_TOP_LEVEL);
    the_context->u.top_level.directory_paths = new_value;
  }

extern void top_level_context_set_executable_directory(context *the_context,
                                                       const char *new_value)
  {
    assert(the_context != NULL);

    assert(the_context->kind == CK_TOP_LEVEL);
    the_context->u.top_level.executable_directory = new_value;
  }

extern const char *context_directory_paths(context *the_context)
  {
    assert(the_context != NULL);

    if (the_context->kind == CK_TOP_LEVEL)
        return the_context->u.top_level.directory_paths;
    else
        return context_directory_paths(the_context->next);
  }

extern const char *context_executable_directory(context *the_context)
  {
    assert(the_context != NULL);

    if (the_context->kind == CK_TOP_LEVEL)
        return the_context->u.top_level.executable_directory;
    else
        return context_executable_directory(the_context->next);
  }

extern jump_target *routine_context_return_target(context *the_context)
  {
    assert(the_context != NULL);

    assert(the_context->kind == CK_ROUTINE);
    return the_context->u.routine.return_target;
  }

extern value *routine_context_return_value(context *the_context)
  {
    assert(the_context != NULL);

    assert(the_context->kind == CK_ROUTINE);
    return the_context->u.routine.return_value;
  }

extern void routine_context_set_all_arguments_value(context *the_context,
                                                    value *all_arguments_value)
  {
    assert(the_context != NULL);
    assert(all_arguments_value != NULL);

    assert(the_context->kind == CK_ROUTINE);
    value_add_reference(all_arguments_value);
    assert(the_context->u.routine.all_arguments_value == NULL);
    the_context->u.routine.all_arguments_value = all_arguments_value;
  }

extern void routine_context_set_dynamic_return_type(context *the_context,
                                                    type *new_type)
  {
    assert(the_context != NULL);
    assert(new_type != NULL);

    assert(the_context->kind == CK_ROUTINE);
    type_add_reference(new_type);
    assert(the_context->u.routine.dynamic_return_type == NULL);
    the_context->u.routine.dynamic_return_type = new_type;
  }

extern void routine_context_set_this_object_value(context *the_context,
                                                  object *this_object)
  {
    assert(the_context != NULL);
    assert(this_object != NULL);

    assert(the_context->kind == CK_ROUTINE);
    assert(the_context->u.routine.this_object == NULL);
    the_context->u.routine.this_object = this_object;
  }

extern void *loop_context_loop_construct(context *the_context)
  {
    assert(the_context != NULL);

    assert(the_context->kind == CK_LOOP_CONSTRUCT);
    return the_context->u.loop_construct.loop_construct;
  }

extern jump_target *loop_context_continue_target(context *the_context)
  {
    assert(the_context != NULL);

    assert(the_context->kind == CK_LOOP_CONSTRUCT);
    return the_context->u.loop_construct.continue_target;
  }

extern jump_target *loop_context_break_target(context *the_context)
  {
    assert(the_context != NULL);

    assert(the_context->kind == CK_LOOP_CONSTRUCT);
    return the_context->u.loop_construct.break_target;
  }

extern variable_instance *loop_context_index(context *the_context)
  {
    assert(the_context != NULL);

    assert(the_context->kind == CK_LOOP_CONSTRUCT);
    assert(the_context->u.loop_construct.index != NULL);
    return the_context->u.loop_construct.index;
  }

extern jump_target *block_expression_context_return_target(
        context *the_context)
  {
    assert(the_context != NULL);

    assert(the_context->kind == CK_BLOCK_EXPRESSION);
    return the_context->u.block_expression.return_target;
  }

extern value *block_expression_context_return_value(context *the_context)
  {
    assert(the_context != NULL);

    assert(the_context->kind == CK_BLOCK_EXPRESSION);
    return the_context->u.block_expression.return_value;
  }

extern jump_target *try_catch_statement_context_default_catch_target(
        context *the_context)
  {
    assert(the_context != NULL);

    assert(the_context->kind == CK_TRY_CATCH_STATEMENT);
    assert(try_catch_statement_catcher(
                   the_context->u.try_catch_statement.try_catch_statement) !=
           NULL);
    return the_context->u.try_catch_statement.default_catch_target;
  }

extern jump_target *try_catch_statement_context_catch_target(
        context *the_context, size_t clause_num)
  {
    assert(the_context != NULL);

    assert(the_context->kind == CK_TRY_CATCH_STATEMENT);
    assert(clause_num <
           try_catch_statement_tagged_catcher_count(
                   the_context->u.try_catch_statement.try_catch_statement));
    assert(the_context->u.try_catch_statement.catch_targets != NULL);
    return the_context->u.try_catch_statement.catch_targets[clause_num];
  }

extern verdict context_add_virtual_dependence(context *parent, object *child)
  {
    assert(parent != NULL);
    assert(child != NULL);

    object_add_reference(child);
    return object_aa_append(&(parent->virtual_children), child);
  }

extern void context_add_reference(context *the_context)
  {
    if (the_context != NULL)
      {
        GRAB_SYSTEM_LOCK(the_context->reference_lock);
        assert(the_context->reference_count > 0);
        ++(the_context->reference_count);
        RELEASE_SYSTEM_LOCK(the_context->reference_lock);
      }
  }

extern void context_remove_reference(context *the_context, jumper *the_jumper)
  {
    if (the_context != NULL)
      {
        size_t new_reference_count;
        size_t new_internal_reference_count;

        GRAB_SYSTEM_LOCK(the_context->reference_lock);
        assert(the_context->reference_count > 0);
        --(the_context->reference_count);
        new_reference_count = the_context->reference_count;
        new_internal_reference_count = the_context->internal_reference_count;
        RELEASE_SYSTEM_LOCK(the_context->reference_lock);

        if (new_reference_count == 0)
          {
            finish_context(the_context, the_jumper,
                           new_internal_reference_count);
          }
      }
  }

extern void context_add_internal_reference(context *the_context)
  {
    if (the_context != NULL)
      {
        GRAB_SYSTEM_LOCK(the_context->reference_lock);
        assert((the_context->reference_count > 0) ||
               (the_context->internal_reference_count > 0));
        ++(the_context->internal_reference_count);
        RELEASE_SYSTEM_LOCK(the_context->reference_lock);
      }
  }

extern void context_remove_internal_reference(context *the_context)
  {
    if (the_context != NULL)
      {
        boolean all_zero;

        GRAB_SYSTEM_LOCK(the_context->reference_lock);
        assert(the_context->internal_reference_count > 0);
        --(the_context->internal_reference_count);
        all_zero = ((the_context->reference_count == 0) &&
                    (the_context->internal_reference_count == 0));
        RELEASE_SYSTEM_LOCK(the_context->reference_lock);

        if (all_zero)
            delete_context(the_context);
      }
  }


static context *create_empty_context(context *parent)
  {
    context *result;
    verdict the_verdict;

    result = MALLOC_ONE_OBJECT(context);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock, free(result); return NULL);

    result->next = parent;
    result->depth = context_depth(parent) + 1;
    result->static_instances =
            ((parent == NULL) ? NULL : parent->static_instances);

    the_verdict = object_aa_init(&(result->virtual_children), 1);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        DESTROY_SYSTEM_LOCK(result->reference_lock);
        free(result);
        return NULL;
      }

    result->exited = FALSE;
    result->reference_count = 1;
    result->internal_reference_count = 0;

    return result;
  }

static void finish_context(context *the_context, jumper *the_jumper,
                           size_t new_internal_reference_count)
  {
    assert(the_context != NULL);

    assert(the_context->reference_count == 0);

    free(the_context->virtual_children.array);

    switch (the_context->kind)
      {
        case CK_TOP_LEVEL:
          {
            assert(the_context->u.top_level.return_target != NULL);
            jump_target_remove_reference(
                    the_context->u.top_level.return_target);

            break;
          }
        case CK_STATEMENT_BLOCK:
          {
            statement_block *the_statement_block;
            instance **local_instances;
            size_t local_count;
            size_t local_num;
            instance **extra_instances;
            size_t extra_count;
            size_t extra_num;
            jump_target **labels;
            size_t label_count;
            size_t label_num;
            size_t use_statement_count;
            use_instance **use_instances;

            the_statement_block =
                    the_context->u.statement_block.statement_block;

            local_instances =
                    the_context->u.statement_block.local_instances.array;
            local_count = the_context->u.statement_block.local_instances.
                    element_count;
            for (local_num = 0; local_num < local_count; ++local_num)
              {
                instance *this_instance;
                declaration *this_declaration;

                this_instance = local_instances[local_num];
                this_declaration = statement_block_declaration_by_number(
                        the_statement_block, local_num);
                if ((instance_declaration(this_instance) == this_declaration)
                    && (!(declaration_is_static(this_declaration))))
                  {
                    instance_remove_reference_with_cluster(this_instance,
                            the_context->u.statement_block.reference_cluster,
                            the_jumper);
                  }
              }
            free(local_instances);

            extra_instances =
                    the_context->u.statement_block.extra_instances.array;
            extra_count = the_context->u.statement_block.extra_instances.
                    element_count;
            for (extra_num = 0; extra_num < extra_count; ++extra_num)
              {
                instance *this_instance;

                this_instance = extra_instances[extra_num];
                instance_remove_reference_with_cluster(this_instance,
                        the_context->u.statement_block.reference_cluster,
                        the_jumper);
              }
            free(extra_instances);

            labels = the_context->u.statement_block.labels.array;
            label_count = the_context->u.statement_block.labels.element_count;
            for (label_num = 0; label_num < label_count; ++label_num)
              {
                assert(labels[label_num] != NULL);
                jump_target_remove_reference(labels[label_num]);
              }
            free(labels);

            destroy_string_index(the_context->u.statement_block.label_index);

            use_statement_count = statement_block_use_statement_count(
                    the_context->u.statement_block.statement_block);
            use_instances = the_context->u.statement_block.use_instances;
            if (use_instances != NULL)
              {
                size_t use_statement_num;

                assert(use_statement_count > 0);

                for (use_statement_num = 0;
                     use_statement_num < use_statement_count;
                     ++use_statement_num)
                  {
                    delete_use_instance(use_instances[use_statement_num],
                                        the_jumper);
                  }

                free(use_instances);
              }
            else
              {
                assert(use_statement_count == 0);
              }

            break;
          }
        case CK_ROUTINE:
          {
            variable_instance **parameters;
            size_t parameter_count;
            size_t parameter_num;

            parameters = the_context->u.routine.parameters.array;
            parameter_count = the_context->u.routine.parameters.element_count;
            for (parameter_num = 0; parameter_num < parameter_count;
                 ++parameter_num)
              {
                variable_instance_remove_reference(parameters[parameter_num],
                                                   the_jumper);
              }
            free(parameters);

            assert(the_context->u.routine.return_target != NULL);
            jump_target_remove_reference(the_context->u.routine.return_target);

            break;
          }
        case CK_LOOP_CONSTRUCT:
          {
            if (the_context->u.loop_construct.index != NULL)
              {
                variable_instance_remove_reference(
                        the_context->u.loop_construct.index, the_jumper);
              }
            assert(the_context->u.loop_construct.continue_target != NULL);
            jump_target_remove_reference(
                    the_context->u.loop_construct.continue_target);
            assert(the_context->u.loop_construct.break_target != NULL);
            jump_target_remove_reference(
                    the_context->u.loop_construct.break_target);
            break;
          }
        case CK_BLOCK_EXPRESSION:
          {
            assert(the_context->u.block_expression.return_target != NULL);
            jump_target_remove_reference(
                    the_context->u.block_expression.return_target);

            break;
          }
        case CK_TRY_CATCH_STATEMENT:
          {
            jump_target **catch_targets;

            if (the_context->u.try_catch_statement.default_catch_target !=
                NULL)
              {
                jump_target_remove_reference(
                        the_context->u.try_catch_statement.
                                default_catch_target);
              }

            catch_targets = the_context->u.try_catch_statement.catch_targets;
            if (catch_targets != NULL)
              {
                size_t tagged_count;
                size_t tagged_num;

                tagged_count = try_catch_statement_tagged_catcher_count(
                        the_context->u.try_catch_statement.
                                try_catch_statement);
                assert(tagged_count > 0);
                for (tagged_num = 0; tagged_num < tagged_count; ++tagged_num)
                    jump_target_remove_reference(catch_targets[tagged_num]);
                free(catch_targets);
              }

            break;
          }
        case CK_STATIC:
          {
            instance **instances;
            size_t static_count;
            size_t static_num;

            instances = the_context->u.static_home.instances.array;
            static_count = the_context->u.static_home.instances.element_count;
            for (static_num = 0; static_num < static_count; ++static_num)
                instance_remove_reference(instances[static_num], the_jumper);
            free(instances);

            break;
          }
        case CK_SINGLETON_VARIABLE:
          {
            instance_remove_reference_with_cluster(
                    the_context->u.singleton_variable.instance, NULL,
                    the_jumper);
            break;
          }
        case CK_GLUE:
          {
            declaration **declarations;
            instance **instances;
            size_t count;
            size_t num;

            declarations = the_context->u.glue.declarations.array;
            instances = the_context->u.glue.instances.array;
            count = the_context->u.glue.declarations.element_count;
            assert(count == the_context->u.glue.instances.element_count);
            for (num = 0; num < count; ++num)
              {
                declaration_remove_reference(declarations[num]);
                instance_remove_reference(instances[num], the_jumper);
              }
            free(declarations);
            free(instances);

            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    if (the_context->kind != CK_STATIC)
        context_remove_reference(the_context->next, the_jumper);

    if (new_internal_reference_count == 0)
        delete_context(the_context);
  }

static void delete_context(context *the_context)
  {
    assert(the_context != NULL);

    assert(the_context->reference_count == 0);
    assert(the_context->internal_reference_count == 0);

    switch (the_context->kind)
      {
        case CK_TOP_LEVEL:
            DESTROY_SYSTEM_LOCK(the_context->u.top_level.return_lock);
            break;
        case CK_ROUTINE:
            DESTROY_SYSTEM_LOCK(the_context->u.routine.return_lock);
            break;
        case CK_BLOCK_EXPRESSION:
            DESTROY_SYSTEM_LOCK(the_context->u.block_expression.return_lock);
            break;
        default:
            break;
      }

    DESTROY_SYSTEM_LOCK(the_context->reference_lock);

    free(the_context);
  }

static void check_routine_return_value_internal(context *the_context,
        value *return_value, boolean unlock, const source_location *location,
        jumper *the_jumper)
  {
    type *return_type;
    boolean is_in;
    boolean doubt;
    char *why_not;

    assert(routine_instance_is_instantiated(
                   the_context->u.routine.instance)); /* VERIFIED */
    assert(!(routine_instance_scope_exited(
                     the_context->u.routine.instance))); /* VERIFIED */
    return_type =
            routine_instance_return_type(the_context->u.routine.instance);
    assert(type_is_valid(return_type)); /* VERIFICATION NEEDED */
    is_in = value_is_in_type(return_value, return_type, &doubt, &why_not,
                             location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (unlock)
          {
            RELEASE_SYSTEM_LOCK(the_context->u.routine.return_lock);
          }
        return;
      }
    if (doubt)
      {
        if (unlock)
          {
            RELEASE_SYSTEM_LOCK(the_context->u.routine.return_lock);
          }
        location_exception(the_jumper, location,
                EXCEPTION_TAG(call_return_type_match_indeterminate),
                "%s could not determine whether the value returned from a "
                "function call matched the static return type of the "
                "function because %s.", interpreter_name(), why_not);
        free(why_not);
        return;
      }
    if (!is_in)
      {
        if (unlock)
          {
            RELEASE_SYSTEM_LOCK(the_context->u.routine.return_lock);
          }
        location_exception(the_jumper, location,
                EXCEPTION_TAG(call_return_type_mismatch),
                "The value returned from a function call didn't match the "
                "static return type of the function because %s.", why_not);
        free(why_not);
        return;
      }

    if (the_context->u.routine.dynamic_return_type != NULL)
      {
        boolean is_in;
        boolean doubt;
        char *why_not;

        assert(type_is_valid(the_context->u.routine.dynamic_return_type));
                /* VERIFICATION NEEDED */
        is_in = value_is_in_type(return_value,
                the_context->u.routine.dynamic_return_type, &doubt,
                &why_not, location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            if (unlock)
              {
                RELEASE_SYSTEM_LOCK(the_context->u.routine.return_lock);
              }
            return;
          }
        if (doubt)
          {
            if (unlock)
              {
                RELEASE_SYSTEM_LOCK(the_context->u.routine.return_lock);
              }
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(call_return_type_match_indeterminate),
                    "%s could not determine whether the value returned "
                    "from a function call matched the dynamic return type "
                    "of the function because %s.", interpreter_name(),
                    why_not);
            free(why_not);
            return;
          }
        if (!is_in)
          {
            if (unlock)
              {
                RELEASE_SYSTEM_LOCK(the_context->u.routine.return_lock);
              }
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(call_return_type_mismatch),
                    "The value returned from a function call didn't match "
                    "the dynamic return type of the function because %s.",
                    why_not);
            free(why_not);
            return;
          }
      }
  }
