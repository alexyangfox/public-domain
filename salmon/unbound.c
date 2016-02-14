/* file "unbound.c" */

/*
 *  This file contains the implementation of the unbound module.
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
#include <string.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/string_index.h"
#include "c_foundations/auto_array.h"
#include "c_foundations/auto_array_implementation.h"
#include "unbound.h"
#include "call.h"
#include "routine_declaration_chain.h"
#include "source_location.h"
#include "statement.h"
#include "declaration.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "tagalong_declaration.h"
#include "lepton_key_declaration.h"
#include "quark_declaration.h"
#include "lock_declaration.h"


AUTO_ARRAY(unbound_name_aa, unbound_name *);
AUTO_ARRAY(unbound_use_aa, unbound_use *);
AUTO_ARRAY(declaration_aa, declaration *);


struct unbound_name_manager
  {
    string_index *index;
    unbound_name_aa unbound_names;
    declaration_aa statics;
    unbound_name_manager *deferred;
  };

struct unbound_name
  {
    char *name_chars;
    unbound_name_manager *parent;
    unbound_use_aa unbound_uses;
  };

struct unbound_use
  {
    unbound_name *parent;
    size_t parent_slot_number;
    unbound_use_kind kind;
    union
      {
        call *the_call;
        statement *return_statement;
        statement *export_statement;
        statement *hide_statement;
        statement *break_statement;
        statement *continue_statement;
        expression *expression;
        statement *statement;
        routine_declaration_chain *chain;
        struct
          {
            statement *use_statement;
            size_t used_for_num;
          } use_flow_through;
      } u;
    source_location location;
  };


static void delete_unbound_name(unbound_name *the_unbound_name);
static void delete_unbound_use(unbound_use *the_unbound_use);
static unbound_name *find_or_create_unbound_name(unbound_name_manager *manager,
                                                 const char *name);
static unbound_use *add_unbound_use(unbound_name_manager *manager,
        const char *name, const source_location *location);
static verdict bind_return_for_name(unbound_name_manager *manager,
        const char *name, routine_declaration *declaration);
static verdict bind_break_and_continue_for_name(unbound_name_manager *manager,
        const char *name, void *loop_construct, boolean is_parallel);


AUTO_ARRAY_IMPLEMENTATION(unbound_name_aa, unbound_name *, 0);
AUTO_ARRAY_IMPLEMENTATION(unbound_use_aa, unbound_use *, 0);


extern unbound_name_manager *create_unbound_name_manager(void)
  {
    unbound_name_manager *result;
    verdict the_verdict;
    string_index *index;

    result = MALLOC_ONE_OBJECT(unbound_name_manager);
    if (result == NULL)
        return NULL;

    the_verdict = unbound_name_aa_init(&(result->unbound_names), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    index = create_string_index();
    if (index == NULL)
      {
        free(result->unbound_names.array);
        free(result);
        return NULL;
      }
    result->index = index;

    result->statics.element_count = 0;
    result->statics.array = NULL;
    result->deferred = NULL;

    return result;
  }

extern void delete_unbound_name_manager(unbound_name_manager *manager)
  {
    unbound_name **array;
    size_t count;

    assert(manager != NULL);

    destroy_string_index(manager->index);

    array = manager->unbound_names.array;
    assert(array != NULL);
    count = manager->unbound_names.element_count;
    while (count > 0)
      {
        --count;
        delete_unbound_name(array[count]);
      }
    free(array);

    if (manager->statics.array != NULL)
        free(manager->statics.array);

    if (manager->deferred != NULL)
        delete_unbound_name_manager(manager->deferred);

    free(manager);
  }

extern size_t unbound_name_count(unbound_name_manager *manager)
  {
    assert(manager != NULL);

    return manager->unbound_names.element_count;
  }

extern unbound_name *unbound_name_number(unbound_name_manager *manager,
                                         size_t name_number)
  {
    assert(manager != NULL);
    assert(name_number < manager->unbound_names.element_count);

    return manager->unbound_names.array[name_number];
  }

extern unbound_name *lookup_unbound_name(unbound_name_manager *manager,
                                         const char *name)
  {
    assert(manager != NULL);
    assert(name != NULL);

    return (unbound_name *)(lookup_in_string_index(manager->index, name));
  }

extern size_t unbound_name_manager_static_count(unbound_name_manager *manager)
  {
    assert(manager != NULL);

    return manager->statics.element_count;
  }

extern declaration **unbound_name_manager_static_declarations(
        unbound_name_manager *manager)
  {
    assert(manager != NULL);

    return manager->statics.array;
  }

extern unbound_use *add_unbound_return(unbound_name_manager *manager,
        const char *name, statement *return_statement,
        const source_location *location)
  {
    unbound_use *result;

    assert(manager != NULL);
    assert(return_statement != NULL);
    assert(location != NULL);

    result = add_unbound_use(manager, ((name == NULL) ? "" : name), location);
    if (result == NULL)
        return NULL;

    result->kind = UUK_ROUTINE_FOR_RETURN_STATEMENT;
    result->u.return_statement = return_statement;

    return result;
  }

extern unbound_use *add_unbound_export(unbound_name_manager *manager,
        const char *name, statement *export_statement,
        const source_location *location)
  {
    unbound_use *result;

    assert(manager != NULL);
    assert(export_statement != NULL);
    assert(location != NULL);

    result = add_unbound_use(manager, ((name == NULL) ? "" : name), location);
    if (result == NULL)
        return NULL;

    result->kind = UUK_ROUTINE_FOR_EXPORT_STATEMENT;
    result->u.export_statement = export_statement;

    return result;
  }

extern unbound_use *add_unbound_hide(unbound_name_manager *manager,
        const char *name, statement *hide_statement,
        const source_location *location)
  {
    unbound_use *result;

    assert(manager != NULL);
    assert(hide_statement != NULL);
    assert(location != NULL);

    result = add_unbound_use(manager, ((name == NULL) ? "" : name), location);
    if (result == NULL)
        return NULL;

    result->kind = UUK_ROUTINE_FOR_HIDE_STATEMENT;
    result->u.hide_statement = hide_statement;

    return result;
  }

extern unbound_use *add_unbound_arguments_expression(
        unbound_name_manager *manager, const char *name,
        expression *arguments_expression, const source_location *location)
  {
    unbound_use *result;

    assert(manager != NULL);
    assert(arguments_expression != NULL);
    assert(location != NULL);

    result = add_unbound_use(manager, ((name == NULL) ? "" : name), location);
    if (result == NULL)
        return NULL;

    result->kind = UUK_ROUTINE_FOR_ARGUMENTS_EXPRESSION;
    result->u.expression = arguments_expression;

    return result;
  }

extern unbound_use *add_unbound_this_expression(unbound_name_manager *manager,
        const char *name, expression *this_expression,
        const source_location *location)
  {
    unbound_use *result;

    assert(manager != NULL);
    assert(this_expression != NULL);
    assert(location != NULL);

    result = add_unbound_use(manager, ((name == NULL) ? "" : name), location);
    if (result == NULL)
        return NULL;

    result->kind = UUK_ROUTINE_FOR_THIS_EXPRESSION;
    result->u.expression = this_expression;

    return result;
  }

extern unbound_use *add_unbound_operator_expression(
        unbound_name_manager *manager, const char *name,
        expression *this_expression, const source_location *location)
  {
    unbound_use *result;

    assert(manager != NULL);
    assert(this_expression != NULL);
    assert(location != NULL);

    result = add_unbound_use(manager, ((name == NULL) ? "" : name), location);
    if (result == NULL)
        return NULL;

    result->kind = UUK_ROUTINE_FOR_OPERATOR_EXPRESSION;
    result->u.expression = this_expression;

    return result;
  }

extern unbound_use *add_unbound_operator_statement(
        unbound_name_manager *manager, const char *name,
        statement *this_statement, const source_location *location)
  {
    unbound_use *result;

    assert(manager != NULL);
    assert(this_statement != NULL);
    assert(location != NULL);

    result = add_unbound_use(manager, ((name == NULL) ? "" : name), location);
    if (result == NULL)
        return NULL;

    result->kind = UUK_ROUTINE_FOR_OPERATOR_STATEMENT;
    result->u.statement = this_statement;

    return result;
  }

extern unbound_use *add_unbound_break(unbound_name_manager *manager,
        const char *name, statement *break_statement,
        const source_location *location)
  {
    unbound_use *result;

    assert(manager != NULL);
    assert(break_statement != NULL);
    assert(location != NULL);

    result = add_unbound_use(manager, ((name == NULL) ? "" : name), location);
    if (result == NULL)
        return NULL;

    result->kind = UUK_LOOP_FOR_BREAK_STATEMENT;
    result->u.break_statement = break_statement;

    return result;
  }

extern unbound_use *add_unbound_continue(unbound_name_manager *manager,
        const char *name, statement *continue_statement,
        const source_location *location)
  {
    unbound_use *result;

    assert(manager != NULL);
    assert(continue_statement != NULL);
    assert(location != NULL);

    result = add_unbound_use(manager, ((name == NULL) ? "" : name), location);
    if (result == NULL)
        return NULL;

    result->kind = UUK_LOOP_FOR_CONTINUE_STATEMENT;
    result->u.continue_statement = continue_statement;

    return result;
  }

extern unbound_use *add_unbound_break_expression(unbound_name_manager *manager,
        const char *name, expression *break_expression,
        const source_location *location)
  {
    unbound_use *result;

    assert(manager != NULL);
    assert(break_expression != NULL);
    assert(location != NULL);

    result = add_unbound_use(manager, ((name == NULL) ? "" : name), location);
    if (result == NULL)
        return NULL;

    result->kind = UUK_LOOP_FOR_BREAK_EXPRESSION;
    result->u.expression = break_expression;

    return result;
  }

extern unbound_use *add_unbound_continue_expression(
        unbound_name_manager *manager, const char *name,
        expression *continue_expression, const source_location *location)
  {
    unbound_use *result;

    assert(manager != NULL);
    assert(continue_expression != NULL);
    assert(location != NULL);

    result = add_unbound_use(manager, ((name == NULL) ? "" : name), location);
    if (result == NULL)
        return NULL;

    result->kind = UUK_LOOP_FOR_CONTINUE_EXPRESSION;
    result->u.expression = continue_expression;

    return result;
  }

extern unbound_use *add_unbound_variable_reference(
        unbound_name_manager *manager, const char *name,
        expression *the_expression, const source_location *location)
  {
    unbound_use *result;

    assert(manager != NULL);
    assert(name != NULL);
    assert(the_expression != NULL);
    assert(location != NULL);

    result = add_unbound_use(manager, name, location);
    if (result == NULL)
        return NULL;

    result->kind = UUK_VARIABLE_FOR_EXPRESSION;
    result->u.expression = the_expression;

    return result;
  }

extern unbound_use *add_dangling_overloaded_routine_reference(
        unbound_name_manager *manager, const char *name,
        routine_declaration_chain *chain, const source_location *location)
  {
    unbound_use *result;

    assert(manager != NULL);
    assert(name != NULL);
    assert(chain != NULL);
    assert(location != NULL);

    result = add_unbound_use(manager, name, location);
    if (result == NULL)
        return NULL;

    result->kind = UUK_DANGLING_OVERLOADED_ROUTINE;
    result->u.chain = chain;

    return result;
  }

extern unbound_use *add_use_flow_through_reference(
        unbound_name_manager *manager, const char *name,
        statement *use_statement, size_t used_for_num,
        const source_location *location)
  {
    unbound_use *result;

    assert(manager != NULL);
    assert(name != NULL);
    assert(use_statement != NULL);
    assert(get_statement_kind(use_statement) == SK_USE);
    assert(location != NULL);

    result = add_unbound_use(manager, name, location);
    if (result == NULL)
        return NULL;

    result->kind = UUK_USE_FLOW_THROUGH;
    result->u.use_flow_through.use_statement = use_statement;
    result->u.use_flow_through.used_for_num = used_for_num;

    return result;
  }

extern verdict bind_variable_name(unbound_name_manager *manager,
        const char *name, variable_declaration *declaration)
  {
    unbound_name *the_unbound_name;
    size_t use_num;

    assert(manager != NULL);
    assert(name != NULL);
    assert(declaration != NULL);

    if ((manager->deferred != NULL) &&
        variable_declaration_is_static(declaration))
      {
        verdict the_verdict;

        the_verdict = bind_variable_name(manager->deferred, name, declaration);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    the_unbound_name =
            (unbound_name *)(lookup_in_string_index(manager->index, name));
    if (the_unbound_name == NULL)
        return MISSION_ACCOMPLISHED;

    for (use_num = 0; use_num < the_unbound_name->unbound_uses.element_count;)
      {
        unbound_use *use;

        use = the_unbound_name->unbound_uses.array[use_num];
        assert(use != NULL);
        if (use->kind == UUK_VARIABLE_FOR_EXPRESSION)
          {
            bind_expression_to_variable_declaration(use->u.expression,
                                                    declaration);

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_USE_FLOW_THROUGH)
          {
            use_statement_set_used_for_declaration(
                    use->u.use_flow_through.use_statement,
                    use->u.use_flow_through.used_for_num,
                    variable_declaration_declaration(declaration));

            remove_unbound_use(use);
          }
        else
          {
            ++use_num;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_routine_name(unbound_name_manager *manager,
        const char *name, routine_declaration_chain *chain)
  {
    unbound_name *the_unbound_name;
    size_t use_num;

    assert(manager != NULL);
    assert(name != NULL);
    assert(chain != NULL);

    the_unbound_name =
            (unbound_name *)(lookup_in_string_index(manager->index, name));
    if (the_unbound_name == NULL)
        return MISSION_ACCOMPLISHED;

    for (use_num = 0; use_num < the_unbound_name->unbound_uses.element_count;)
      {
        unbound_use *use;

        use = the_unbound_name->unbound_uses.array[use_num];
        assert(use != NULL);
        if (use->kind == UUK_VARIABLE_FOR_EXPRESSION)
          {
            if (unbound_name_reference_expression_addressable_required(
                        use->u.expression))
              {
                location_error(&(use->location),
                               "The location of a routine was taken.");
                return MISSION_FAILED;
              }
            bind_expression_to_routine_declaration_chain(use->u.expression,
                                                         chain);

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_DANGLING_OVERLOADED_ROUTINE)
          {
            routine_declaration_chain_set_next(use->u.chain, chain);

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_ROUTINE_FOR_OPERATOR_EXPRESSION)
          {
            expression_set_overload_chain(use->u.expression, chain);

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_ROUTINE_FOR_OPERATOR_STATEMENT)
          {
            statement_set_overload_chain(use->u.statement, chain);

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_USE_FLOW_THROUGH)
          {
            use_statement_set_used_for_chain(
                    use->u.use_flow_through.use_statement,
                    use->u.use_flow_through.used_for_num, chain);

            remove_unbound_use(use);
          }
        else
          {
            ++use_num;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_static_routine_name(unbound_name_manager *manager,
        const char *name, routine_declaration_chain *chain)
  {
    assert(manager != NULL);
    assert(name != NULL);
    assert(chain != NULL);

    if (manager->deferred != NULL)
        return bind_routine_name(manager->deferred, name, chain);

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_tagalong_name(unbound_name_manager *manager,
        const char *name, tagalong_declaration *declaration)
  {
    unbound_name *the_unbound_name;
    size_t use_num;

    assert(manager != NULL);
    assert(name != NULL);
    assert(declaration != NULL);

    if ((manager->deferred != NULL) &&
        tagalong_declaration_is_static(declaration))
      {
        verdict the_verdict;

        the_verdict = bind_tagalong_name(manager->deferred, name, declaration);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    the_unbound_name =
            (unbound_name *)(lookup_in_string_index(manager->index, name));
    if (the_unbound_name == NULL)
        return MISSION_ACCOMPLISHED;

    for (use_num = 0; use_num < the_unbound_name->unbound_uses.element_count;)
      {
        unbound_use *use;

        use = the_unbound_name->unbound_uses.array[use_num];
        assert(use != NULL);
        if (use->kind == UUK_VARIABLE_FOR_EXPRESSION)
          {
            if (unbound_name_reference_expression_addressable_required(
                        use->u.expression))
              {
                location_error(&(use->location),
                               "The location of a tagalong was taken.");
                return MISSION_FAILED;
              }
            bind_expression_to_tagalong_declaration(use->u.expression,
                                                    declaration);

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_USE_FLOW_THROUGH)
          {
            use_statement_set_used_for_declaration(
                    use->u.use_flow_through.use_statement,
                    use->u.use_flow_through.used_for_num,
                    tagalong_declaration_declaration(declaration));

            remove_unbound_use(use);
          }
        else
          {
            ++use_num;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_lepton_key_name(unbound_name_manager *manager,
        const char *name, lepton_key_declaration *declaration)
  {
    unbound_name *the_unbound_name;
    size_t use_num;

    assert(manager != NULL);
    assert(name != NULL);
    assert(declaration != NULL);

    if ((manager->deferred != NULL) &&
        lepton_key_declaration_is_static(declaration))
      {
        verdict the_verdict;

        the_verdict =
                bind_lepton_key_name(manager->deferred, name, declaration);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    the_unbound_name =
            (unbound_name *)(lookup_in_string_index(manager->index, name));
    if (the_unbound_name == NULL)
        return MISSION_ACCOMPLISHED;

    for (use_num = 0; use_num < the_unbound_name->unbound_uses.element_count;)
      {
        unbound_use *use;

        use = the_unbound_name->unbound_uses.array[use_num];
        assert(use != NULL);
        if (use->kind == UUK_VARIABLE_FOR_EXPRESSION)
          {
            if (unbound_name_reference_expression_addressable_required(
                        use->u.expression))
              {
                location_error(&(use->location),
                               "The location of a lepton key was taken.");
                return MISSION_FAILED;
              }
            bind_expression_to_lepton_key_declaration(use->u.expression,
                                                      declaration);

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_USE_FLOW_THROUGH)
          {
            use_statement_set_used_for_declaration(
                    use->u.use_flow_through.use_statement,
                    use->u.use_flow_through.used_for_num,
                    lepton_key_declaration_declaration(declaration));

            remove_unbound_use(use);
          }
        else
          {
            ++use_num;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_quark_name(unbound_name_manager *manager, const char *name,
                               quark_declaration *declaration)
  {
    unbound_name *the_unbound_name;
    size_t use_num;

    assert(manager != NULL);
    assert(name != NULL);
    assert(declaration != NULL);

    if ((manager->deferred != NULL) &&
        quark_declaration_is_static(declaration))
      {
        verdict the_verdict;

        the_verdict = bind_quark_name(manager->deferred, name, declaration);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    the_unbound_name =
            (unbound_name *)(lookup_in_string_index(manager->index, name));
    if (the_unbound_name == NULL)
        return MISSION_ACCOMPLISHED;

    for (use_num = 0; use_num < the_unbound_name->unbound_uses.element_count;)
      {
        unbound_use *use;

        use = the_unbound_name->unbound_uses.array[use_num];
        assert(use != NULL);
        if (use->kind == UUK_VARIABLE_FOR_EXPRESSION)
          {
            if (unbound_name_reference_expression_addressable_required(
                        use->u.expression))
              {
                location_error(&(use->location),
                               "The location of a quark was taken.");
                return MISSION_FAILED;
              }
            bind_expression_to_quark_declaration(use->u.expression,
                                                 declaration);

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_USE_FLOW_THROUGH)
          {
            use_statement_set_used_for_declaration(
                    use->u.use_flow_through.use_statement,
                    use->u.use_flow_through.used_for_num,
                    quark_declaration_declaration(declaration));

            remove_unbound_use(use);
          }
        else
          {
            ++use_num;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_lock_name(unbound_name_manager *manager, const char *name,
                              lock_declaration *declaration)
  {
    unbound_name *the_unbound_name;
    size_t use_num;

    assert(manager != NULL);
    assert(name != NULL);
    assert(declaration != NULL);

    if ((manager->deferred != NULL) && lock_declaration_is_static(declaration))
      {
        verdict the_verdict;

        the_verdict = bind_lock_name(manager->deferred, name, declaration);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    the_unbound_name =
            (unbound_name *)(lookup_in_string_index(manager->index, name));
    if (the_unbound_name == NULL)
        return MISSION_ACCOMPLISHED;

    for (use_num = 0; use_num < the_unbound_name->unbound_uses.element_count;)
      {
        unbound_use *use;

        use = the_unbound_name->unbound_uses.array[use_num];
        assert(use != NULL);
        if (use->kind == UUK_VARIABLE_FOR_EXPRESSION)
          {
            if (unbound_name_reference_expression_addressable_required(
                        use->u.expression))
              {
                location_error(&(use->location),
                               "The location of a lock was taken.");
                return MISSION_FAILED;
              }
            bind_expression_to_lock_declaration(use->u.expression,
                                                declaration);

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_USE_FLOW_THROUGH)
          {
            use_statement_set_used_for_declaration(
                    use->u.use_flow_through.use_statement,
                    use->u.use_flow_through.used_for_num,
                    lock_declaration_declaration(declaration));

            remove_unbound_use(use);
          }
        else
          {
            ++use_num;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_label_name(unbound_name_manager *manager, const char *name,
                               statement *declaration)
  {
    unbound_name *the_unbound_name;
    size_t use_num;

    assert(manager != NULL);
    assert(name != NULL);
    assert(declaration != NULL);

    assert(get_statement_kind(declaration) == SK_LABEL);

    the_unbound_name =
            (unbound_name *)(lookup_in_string_index(manager->index, name));
    if (the_unbound_name == NULL)
        return MISSION_ACCOMPLISHED;

    for (use_num = 0; use_num < the_unbound_name->unbound_uses.element_count;)
      {
        unbound_use *use;

        use = the_unbound_name->unbound_uses.array[use_num];
        assert(use != NULL);
        if (use->kind == UUK_VARIABLE_FOR_EXPRESSION)
          {
            if (unbound_name_reference_expression_addressable_required(
                        use->u.expression))
              {
                location_error(&(use->location),
                        "The location of a jump target label was taken.");
                return MISSION_FAILED;
              }
            bind_expression_to_label_declaration(use->u.expression,
                                                 declaration);

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_USE_FLOW_THROUGH)
          {
            use_statement_set_used_for_label_statement(
                    use->u.use_flow_through.use_statement,
                    use->u.use_flow_through.used_for_num, declaration);

            remove_unbound_use(use);
          }
        else
          {
            ++use_num;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_return(unbound_name_manager *manager,
                           routine_declaration *declaration)
  {
    verdict the_verdict;
    const char *name;

    the_verdict = bind_return_for_name(manager, "", declaration);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    name = routine_declaration_name(declaration);
    if (name == NULL)
        return MISSION_ACCOMPLISHED;
    return bind_return_for_name(manager, name, declaration);
  }

extern verdict bind_return_to_block_expression(unbound_name_manager *manager,
                                               expression *block_expression)
  {
    unbound_name *the_unbound_name;
    size_t use_num;

    assert(manager != NULL);
    assert(block_expression != NULL);

    the_unbound_name =
            (unbound_name *)(lookup_in_string_index(manager->index, ""));
    if (the_unbound_name == NULL)
        return MISSION_ACCOMPLISHED;

    for (use_num = 0; use_num < the_unbound_name->unbound_uses.element_count;)
      {
        unbound_use *use;

        use = the_unbound_name->unbound_uses.array[use_num];
        assert(use != NULL);
        if (use->kind == UUK_ROUTINE_FOR_RETURN_STATEMENT)
          {
            verdict the_verdict;

            the_verdict = bind_return_statement_to_block_expression(
                    use->u.return_statement, block_expression);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;

            remove_unbound_use(use);
          }
        else
          {
            ++use_num;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_break_and_continue(unbound_name_manager *manager,
        void *loop_construct, boolean is_parallel, const char **current_labels,
        size_t current_label_count)
  {
    verdict the_verdict;
    size_t label_num;

    the_verdict = bind_break_and_continue_for_name(manager, "", loop_construct,
                                                   is_parallel);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    for (label_num = 0; label_num < current_label_count; ++label_num)
      {
        verdict the_verdict;

        assert(current_labels != NULL);
        assert(current_labels[label_num] != NULL);

        the_verdict = bind_break_and_continue_for_name(manager,
                current_labels[label_num], loop_construct, is_parallel);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict unbound_name_manager_add_static(unbound_name_manager *manager,
                                               declaration *the_declaration)
  {
    assert(manager != NULL);

    if (manager->statics.array == NULL)
      {
        verdict the_verdict;

        the_verdict = declaration_aa_init(&(manager->statics), 10);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            manager->statics.element_count = 0;
            manager->statics.array = NULL;
            return the_verdict;
          }
      }

    return declaration_aa_append(&(manager->statics), the_declaration);
  }

extern verdict unbound_name_manager_clear_static_list(
        unbound_name_manager *manager)
  {
    verdict the_verdict;

    assert(manager != NULL);

    if (manager->deferred == NULL)
      {
        the_verdict = MISSION_ACCOMPLISHED;
      }
    else
      {
        the_verdict =
                merge_in_unbound_name_manager(manager, manager->deferred);
        manager->deferred = NULL;
      }

    if (manager->statics.array == NULL)
        return the_verdict;

    free(manager->statics.array);

    manager->statics.element_count = 0;
    manager->statics.array = NULL;

    return the_verdict;
  }

extern verdict merge_in_unbound_name_manager(unbound_name_manager *base,
                                             unbound_name_manager *to_merge_in)
  {
    string_index *base_index;
    string_index *merge_in_index;
    size_t merge_in_name_count;
    unbound_name **merge_in_name_array;
    size_t merge_in_name_num;

    assert(base != NULL);
    assert(to_merge_in != NULL);

    if (to_merge_in->deferred != NULL)
      {
        if (base->deferred == NULL)
          {
            base->deferred = to_merge_in->deferred;
            to_merge_in->deferred = NULL;
          }
        else
          {
            verdict the_verdict;

            the_verdict = merge_in_unbound_name_manager(base->deferred,
                                                        to_merge_in->deferred);
            to_merge_in->deferred = NULL;
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(to_merge_in);
                return the_verdict;
              }
          }
      }

    if (to_merge_in->statics.array != NULL)
      {
        if (base->statics.array == NULL)
          {
            assert(base->statics.element_count == 0);
            base->statics = to_merge_in->statics;
          }
        else
          {
            declaration **array;
            size_t count;
            size_t index;

            array = to_merge_in->statics.array;
            count = to_merge_in->statics.element_count;

            for (index = 0; index < count; ++index)
              {
                verdict the_verdict;

                the_verdict =
                        declaration_aa_append(&(base->statics), array[index]);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_unbound_name_manager(to_merge_in);
                    return the_verdict;
                  }
              }

            free(to_merge_in->statics.array);
          }
      }

    base_index = base->index;
    assert(base_index != NULL);

    merge_in_index = to_merge_in->index;
    assert(merge_in_index != NULL);
    destroy_string_index(merge_in_index);

    merge_in_name_count = to_merge_in->unbound_names.element_count;
    merge_in_name_array = to_merge_in->unbound_names.array;

    free(to_merge_in);

    for (merge_in_name_num = 0; merge_in_name_num < merge_in_name_count;
         ++merge_in_name_num)
      {
        unbound_name *merge_in_name;
        char *key;
        unbound_name *base_name;

        merge_in_name = merge_in_name_array[merge_in_name_num];
        assert(merge_in_name != NULL);
        key = merge_in_name->name_chars;
        assert(key != NULL);
        base_name = (unbound_name *)(lookup_in_string_index(base_index, key));

        if (base_name == NULL)
          {
            verdict the_verdict;

            the_verdict = unbound_name_aa_append(&(base->unbound_names),
                                                 merge_in_name);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                for (; merge_in_name_num < merge_in_name_count;
                     ++merge_in_name_num)
                  {
                    delete_unbound_name(
                            merge_in_name_array[merge_in_name_num]);
                  }
                free(merge_in_name_array);
                return the_verdict;
              }

            the_verdict =
                    enter_into_string_index(base_index, key, merge_in_name);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                assert(base->unbound_names.element_count > 0);
                --(base->unbound_names.element_count);
                for (; merge_in_name_num < merge_in_name_count;
                     ++merge_in_name_num)
                  {
                    delete_unbound_name(
                            merge_in_name_array[merge_in_name_num]);
                  }
                free(merge_in_name_array);
                return the_verdict;
              }

            merge_in_name->parent = base;
          }
        else
          {
            unbound_use **use_array;
            size_t use_count;
            size_t parent_slot_number;
            size_t use_num;

            free(key);
            use_array = merge_in_name->unbound_uses.array;
            use_count = merge_in_name->unbound_uses.element_count;
            free(merge_in_name);

            parent_slot_number = base_name->unbound_uses.element_count;
            for (use_num = 0; use_num < use_count; ++use_num)
              {
                unbound_use *the_use;
                verdict the_verdict;

                the_use = use_array[use_num];
                the_use->parent = base_name;

                the_use->parent_slot_number = parent_slot_number;
                ++parent_slot_number;

                the_verdict = unbound_use_aa_append(&(base_name->unbound_uses),
                                                    the_use);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    for (; use_num < use_count; ++use_num)
                        delete_unbound_use(use_array[use_num]);
                    free(use_array);

                    ++merge_in_name_num;
                    for (; merge_in_name_num < merge_in_name_count;
                         ++merge_in_name_num)
                      {
                        delete_unbound_name(
                                merge_in_name_array[merge_in_name_num]);
                      }
                    free(merge_in_name_array);
                    return the_verdict;
                  }
              }

            free(use_array);
          }
      }

    free(merge_in_name_array);

    return MISSION_ACCOMPLISHED;
  }

extern verdict merge_in_deferred_unbound_name_manager(
        unbound_name_manager *base, unbound_name_manager *to_merge_in)
  {
    assert(base != NULL);
    assert(to_merge_in != NULL);

    if (base->deferred == NULL)
      {
        base->deferred = to_merge_in;
        return MISSION_ACCOMPLISHED;
      }

    return merge_in_unbound_name_manager(base->deferred, to_merge_in);
  }

extern const char *unbound_name_string(unbound_name *name)
  {
    assert(name != NULL);

    return name->name_chars;
  }

extern size_t unbound_name_use_count(unbound_name *name)
  {
    assert(name != NULL);

    return name->unbound_uses.element_count;
  }

extern unbound_use *unbound_name_use_by_number(unbound_name *name,
                                               size_t use_number)
  {
    assert(name != NULL);
    assert(use_number < name->unbound_uses.element_count);

    return name->unbound_uses.array[use_number];
  }

extern unbound_use_kind get_unbound_use_kind(unbound_use *use)
  {
    assert(use != NULL);

    return use->kind;
  }

extern unbound_name *unbound_use_name(unbound_use *use)
  {
    unbound_name *parent;

    assert(use != NULL);

    parent = use->parent;
    assert(parent != NULL);
    return parent;
  }

extern const source_location *unbound_use_location(unbound_use *use)
  {
    assert(use != NULL);

    return &(use->location);
  }

extern expression *unbound_use_expression(unbound_use *use)
  {
    assert(use != NULL);
    assert((use->kind == UUK_ROUTINE_FOR_ARGUMENTS_EXPRESSION) ||
           (use->kind == UUK_ROUTINE_FOR_THIS_EXPRESSION) ||
           (use->kind == UUK_ROUTINE_FOR_OPERATOR_EXPRESSION) ||
           (use->kind == UUK_VARIABLE_FOR_EXPRESSION));

    return use->u.expression;
  }

extern statement *unbound_use_statement(unbound_use *use)
  {
    assert(use != NULL);
    assert(use->kind == UUK_ROUTINE_FOR_OPERATOR_STATEMENT);

    return use->u.statement;
  }

extern routine_declaration_chain *unbound_use_chain(unbound_use *use)
  {
    assert(use != NULL);
    assert(use->kind == UUK_DANGLING_OVERLOADED_ROUTINE);

    return use->u.chain;
  }

extern statement *unbound_use_use_flow_through_use_statement(unbound_use *use)
  {
    assert(use != NULL);
    assert(use->kind == UUK_USE_FLOW_THROUGH);

    return use->u.use_flow_through.use_statement;
  }

extern size_t unbound_use_use_flow_through_used_for_num(unbound_use *use)
  {
    assert(use != NULL);
    assert(use->kind == UUK_USE_FLOW_THROUGH);

    return use->u.use_flow_through.used_for_num;
  }

extern void remove_unbound_use(unbound_use *use)
  {
    unbound_name *parent;
    size_t parent_slot_number;
    size_t parent_count;
    unbound_use **array;

    assert(use != NULL);

    parent = use->parent;
    assert(parent != NULL);

    parent_slot_number = use->parent_slot_number;
    parent_count = parent->unbound_uses.element_count;
    assert(parent_slot_number < parent_count);

    array = parent->unbound_uses.array;
    assert(array[parent_slot_number] == use);

    if ((parent_slot_number + 1) < parent_count)
      {
        unbound_use *move_back;

        move_back = array[parent_count - 1];
        assert(move_back != use);
        assert(move_back->parent == parent);
        assert(move_back->parent_slot_number == (parent_count - 1));
        move_back->parent_slot_number = parent_slot_number;
        array[parent_slot_number] = move_back;
      }

    parent->unbound_uses.element_count = (parent_count - 1);

    delete_unbound_use(use);
  }


static void delete_unbound_name(unbound_name *the_unbound_name)
  {
    unbound_use **array;
    size_t count;

    assert(the_unbound_name != NULL);

    assert(the_unbound_name->name_chars != NULL);
    free(the_unbound_name->name_chars);

    array = the_unbound_name->unbound_uses.array;
    count = the_unbound_name->unbound_uses.element_count;
    while (count > 0)
      {
        --count;
        delete_unbound_use(array[count]);
      }
    free(array);

    free(the_unbound_name);
  }

static void delete_unbound_use(unbound_use *the_unbound_use)
  {
    assert(the_unbound_use != NULL);

    source_location_remove_reference(&(the_unbound_use->location));

    free(the_unbound_use);
  }

static unbound_name *find_or_create_unbound_name(unbound_name_manager *manager,
                                                 const char *name)
  {
    void *existing;
    unbound_name *result;
    char *name_copy;
    verdict the_verdict;

    assert(manager != NULL);
    assert(name != NULL);

    existing = lookup_in_string_index(manager->index, name);
    if (existing != NULL)
        return (unbound_name *)existing;

    result = MALLOC_ONE_OBJECT(unbound_name);
    if (result == NULL)
        return NULL;

    name_copy = MALLOC_ARRAY(char, strlen(name) + 1);
    if (name_copy == NULL)
      {
        free(result);
        return NULL;
      }
    strcpy(name_copy, name);
    result->name_chars = name_copy;

    result->parent = manager;

    the_verdict = unbound_use_aa_init(&(result->unbound_uses), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(name_copy);
        free(result);
        return NULL;
      }

    the_verdict = unbound_name_aa_append(&(manager->unbound_names), result);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name(result);
        return NULL;
      }

    the_verdict = enter_into_string_index(manager->index, name, result);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(manager->unbound_names.element_count > 0);
        --(manager->unbound_names.element_count);
        delete_unbound_name(result);
        return NULL;
      }

    return result;
  }

static unbound_use *add_unbound_use(unbound_name_manager *manager,
        const char *name, const source_location *location)
  {
    unbound_name *the_unbound_name;
    unbound_use *result;
    verdict the_verdict;

    the_unbound_name = find_or_create_unbound_name(manager, name);
    if (the_unbound_name == NULL)
        return NULL;

    result = MALLOC_ONE_OBJECT(unbound_use);
    if (result == NULL)
        return NULL;

    result->parent = the_unbound_name;
    result->parent_slot_number = the_unbound_name->unbound_uses.element_count;

    the_verdict =
            unbound_use_aa_append(&(the_unbound_name->unbound_uses), result);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    set_source_location(&(result->location), location);

    return result;
  }

static verdict bind_return_for_name(unbound_name_manager *manager,
        const char *name, routine_declaration *declaration)
  {
    unbound_name *the_unbound_name;
    size_t use_num;

    assert(manager != NULL);
    assert(name != NULL);
    assert(declaration != NULL);

    the_unbound_name =
            (unbound_name *)(lookup_in_string_index(manager->index, name));
    if (the_unbound_name == NULL)
        return MISSION_ACCOMPLISHED;

    for (use_num = 0; use_num < the_unbound_name->unbound_uses.element_count;)
      {
        unbound_use *use;

        use = the_unbound_name->unbound_uses.array[use_num];
        assert(use != NULL);
        if (use->kind == UUK_ROUTINE_FOR_RETURN_STATEMENT)
          {
            verdict the_verdict;

            the_verdict = bind_return_statement_to_routine_declaration(
                    use->u.return_statement, declaration);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_ROUTINE_FOR_EXPORT_STATEMENT)
          {
            if (routine_declaration_is_class(declaration))
              {
                verdict the_verdict;

                the_verdict = bind_export_statement_to_from_declaration(
                        use->u.return_statement, declaration);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return the_verdict;

                remove_unbound_use(use);
              }
            else
              {
                ++use_num;
              }
          }
        else if (use->kind == UUK_ROUTINE_FOR_HIDE_STATEMENT)
          {
            if (routine_declaration_is_class(declaration))
              {
                verdict the_verdict;

                the_verdict = bind_hide_statement_to_from_declaration(
                        use->u.return_statement, declaration);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return the_verdict;

                remove_unbound_use(use);
              }
            else
              {
                ++use_num;
              }
          }
        else if (use->kind == UUK_ROUTINE_FOR_ARGUMENTS_EXPRESSION)
          {
            bind_arguments_expression_to_routine_declaration(use->u.expression,
                                                             declaration);

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_ROUTINE_FOR_THIS_EXPRESSION)
          {
            if (routine_declaration_is_class(declaration))
              {
                bind_this_expression_to_routine_declaration(use->u.expression,
                                                            declaration);

                remove_unbound_use(use);
              }
            else
              {
                ++use_num;
              }
          }
        else
          {
            ++use_num;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

static verdict bind_break_and_continue_for_name(unbound_name_manager *manager,
        const char *name, void *loop_construct, boolean is_parallel)
  {
    unbound_name *the_unbound_name;
    size_t use_num;

    assert(manager != NULL);
    assert(name != NULL);
    assert(loop_construct != NULL);

    the_unbound_name =
            (unbound_name *)(lookup_in_string_index(manager->index, name));
    if (the_unbound_name == NULL)
        return MISSION_ACCOMPLISHED;

    for (use_num = 0; use_num < the_unbound_name->unbound_uses.element_count;)
      {
        unbound_use *use;

        use = the_unbound_name->unbound_uses.array[use_num];
        assert(use != NULL);
        if (use->kind == UUK_LOOP_FOR_BREAK_STATEMENT)
          {
            verdict the_verdict;

            if (is_parallel)
              {
                location_error(&(use->location),
                               "A break statement refers to a parallel loop.");
                return MISSION_FAILED;
              }

            the_verdict = bind_break_statement_from(use->u.break_statement,
                                                    loop_construct);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_LOOP_FOR_CONTINUE_STATEMENT)
          {
            verdict the_verdict;

            the_verdict = bind_continue_statement_with(
                    use->u.continue_statement, loop_construct);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_LOOP_FOR_BREAK_EXPRESSION)
          {
            if (is_parallel)
              {
                location_error(&(use->location),
                        "A break expression refers to a parallel loop.");
                return MISSION_FAILED;
              }

            bind_break_expression_from(use->u.expression, loop_construct);

            remove_unbound_use(use);
          }
        else if (use->kind == UUK_LOOP_FOR_CONTINUE_EXPRESSION)
          {
            bind_continue_expression_with(use->u.expression, loop_construct);

            remove_unbound_use(use);
          }
        else
          {
            ++use_num;
          }
      }

    return MISSION_ACCOMPLISHED;
  }
