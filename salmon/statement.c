/* file "statement.c" */

/*
 *  This file contains the implementation of the statement module.
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
#include "statement.h"
#include "basket.h"
#include "call.h"
#include "source_location.h"
#include "declaration.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "lock_declaration.h"
#include "type_expression.h"
#include "statement_block.h"
#include "routine_declaration_chain.h"


typedef struct
  {
    char *exported_as;
    expression *to_export;
  } export_item;

typedef struct
  {
    char *exported_as;
    char *to_export;
  } use_for_item;

typedef struct
  {
    expression *test;
    statement_block *body;
  } else_if_item;

typedef struct
  {
    variable_declaration *exception_variable;
    type_expression *tag_type;
    statement_block *catcher;
  } catch_item;

typedef struct
  {
    boolean flow_through_allowed;
    char *name;
    boolean required;
    declaration *declaration;
    routine_declaration_chain *chain;
    statement *label_statement;
    statement *next_statement;
    size_t next_used_for_number;
    source_location ultimate_use_location;
  } used_for_item;


AUTO_ARRAY(declaration_aa, declaration *);
AUTO_ARRAY(type_expression_aa, type_expression *);
AUTO_ARRAY(statement_block_aa, statement_block *);
AUTO_ARRAY(export_item_aa, export_item);
AUTO_ARRAY(mstring_aa, char *);
AUTO_ARRAY(use_for_item_aa, use_for_item);
AUTO_ARRAY(else_if_item_aa, else_if_item);
AUTO_ARRAY(catch_item_aa, catch_item);
AUTO_ARRAY(used_for_item_aa, used_for_item);


struct statement
  {
    statement_kind kind;
    routine_declaration_chain *overload_chain;
    statement *overload_use_statement;
    size_t overload_used_for_number;
    union
      {
        struct
          {
            assignment_kind kind;
            basket *basket;
            expression *expression;
          } assign;
        call *call;
        declaration_aa declarations;
        struct
          {
            expression *test;
            statement_block *body;
            statement_block *else_body;
            else_if_item_aa else_ifs;
          } if_statement;
        struct
          {
            expression *base;
            type_expression_aa case_types;
            statement_block_aa case_blocks;
          } switch_statement;
        expression *goto_target;
        struct
          {
            expression *return_value;
            boolean to_routine;
            union
              {
                routine_declaration *declaration;
                expression *expression;
              } u;
          } return_statement;
        struct
          {
            variable_declaration *index_variable;
            declaration *index_declaration;
            expression *init;
            expression *test;
            expression *step;
            statement_block *body;
            boolean is_parallel;
          } for_statement;
        struct
          {
            variable_declaration *element_variable;
            declaration *element_declaration;
            expression *base;
            expression *filter;
            statement_block *body;
            boolean is_parallel;
          } iterate_statement;
        struct
          {
            expression *test;
            statement_block *body;
            statement_block *step;
          } while_statement;
        void *break_from;
        void *continue_with;
        char *label_name;
        statement_block *block;
        struct
          {
            expression *lock;
            statement_block *block;
            declaration *lock_declaration;
            value *lock_value;
          } single;
        struct
          {
            statement_block *body;
            statement_block *catcher;
            catch_item_aa items;
          } try_catch;
        struct
          {
            statement_block *body;
            expression *handler;
          } try_handle;
        struct
          {
            statement_block *body;
          } cleanup;
        struct
          {
            routine_declaration *from;
            export_item_aa items;
          } export;
        struct
          {
            routine_declaration *from;
            mstring_aa items;
          } hide;
        struct
          {
            expression *to_use;
            boolean named;
            variable_declaration *container_variable_declaration;
            declaration *container_declaration;
            use_for_item_aa for_items;
            string_index *exceptions;
            statement_block *parent;
            size_t parent_index;
            used_for_item_aa used_for_items;
          } use;
        struct
          {
            char *file;
          } include;
        expression *theorem_claim;
        struct
          {
            char *alias;
            char *target;
          } alias;
      } u;
    source_location location;
  };


AUTO_ARRAY_IMPLEMENTATION(declaration_aa, declaration *, 0);
AUTO_ARRAY_IMPLEMENTATION(type_expression_aa, type_expression *, 0);
AUTO_ARRAY_IMPLEMENTATION(statement_block_aa, statement_block *, 0);
AUTO_ARRAY_IMPLEMENTATION(export_item_aa, export_item, 0);
AUTO_ARRAY_IMPLEMENTATION(mstring_aa, char *, 0);
AUTO_ARRAY_IMPLEMENTATION(use_for_item_aa, use_for_item, 0);
AUTO_ARRAY_IMPLEMENTATION(else_if_item_aa, else_if_item, 0);
AUTO_ARRAY_IMPLEMENTATION(catch_item_aa, catch_item, 0);
AUTO_ARRAY_IMPLEMENTATION(used_for_item_aa, used_for_item, 0);


static statement *create_empty_statement(void);


extern statement *create_assign_statement(basket *the_basket,
        expression *the_expression, assignment_kind kind)
  {
    statement *result;

    assert(the_basket != NULL);
    assert(the_expression != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_basket(the_basket);
        delete_expression(the_expression);
        return NULL;
      }

    result->kind = SK_ASSIGN;
    result->u.assign.kind = kind;
    result->u.assign.basket = the_basket;
    result->u.assign.expression = the_expression;

    return result;
  }

extern statement *create_increment_statement(basket *the_basket)
  {
    statement *result;

    assert(the_basket != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_basket(the_basket);
        return NULL;
      }

    result->kind = SK_INCREMENT;
    result->u.assign.basket = the_basket;
    result->u.assign.expression = NULL;

    return result;
  }

extern statement *create_decrement_statement(basket *the_basket)
  {
    statement *result;

    assert(the_basket != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_basket(the_basket);
        return NULL;
      }

    result->kind = SK_DECREMENT;
    result->u.assign.basket = the_basket;
    result->u.assign.expression = NULL;

    return result;
  }

extern statement *create_call_statement(call *the_call)
  {
    statement *result;

    assert(the_call != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_call(the_call);
        return NULL;
      }

    result->kind = SK_CALL;
    result->u.call = the_call;

    set_source_location(&(result->location), get_call_location(the_call));

    return result;
  }

extern statement *create_declaration_statement(void)
  {
    statement *result;
    verdict the_verdict;

    result = create_empty_statement();
    if (result == NULL)
        return NULL;

    result->kind = SK_DECLARATION;

    the_verdict = declaration_aa_init(&(result->u.declarations), 1);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    return result;
  }

extern statement *create_if_statement(expression *test, statement_block *body,
                                      statement_block *else_body)
  {
    statement *result;
    verdict the_verdict;

    assert(test != NULL);
    assert(body != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_expression(test);
        delete_statement_block(body);
        if (else_body != NULL)
            delete_statement_block(else_body);
        return NULL;
      }

    result->kind = SK_IF;
    result->u.if_statement.test = test;
    result->u.if_statement.body = body;
    result->u.if_statement.else_body = else_body;

    the_verdict = else_if_item_aa_init(&(result->u.if_statement.else_ifs), 1);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        delete_expression(test);
        delete_statement_block(body);
        if (else_body != NULL)
            delete_statement_block(else_body);
        return NULL;
      }

    return result;
  }

extern statement *create_switch_statement(expression *base)
  {
    statement *result;
    verdict the_verdict;

    assert(base != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_expression(base);
        return NULL;
      }

    result->kind = SK_SWITCH;

    result->u.switch_statement.base = base;

    the_verdict = type_expression_aa_init(
            &(result->u.switch_statement.case_types), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        delete_expression(base);
        return NULL;
      }

    the_verdict = statement_block_aa_init(
            &(result->u.switch_statement.case_blocks), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->u.switch_statement.case_types.array);
        free(result);
        delete_expression(base);
        return NULL;
      }

    return result;
  }

extern statement *create_goto_statement(expression *target)
  {
    statement *result;

    assert(target != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_expression(target);
        return NULL;
      }

    result->kind = SK_GOTO;
    result->u.goto_target = target;

    return result;
  }

extern statement *create_return_statement(expression *return_value)
  {
    statement *result;

    result = create_empty_statement();
    if (result == NULL)
      {
        if (return_value != NULL)
            delete_expression(return_value);
        return NULL;
      }

    result->kind = SK_RETURN;
    result->u.return_statement.return_value = return_value;
    result->u.return_statement.to_routine = TRUE;
    result->u.return_statement.u.declaration = NULL;

    return result;
  }

extern statement *create_for_statement(const char *index_name,
        expression *init, expression *test, expression *step,
        statement_block *body, boolean is_parallel,
        const source_location *index_declaration_location)
  {
    statement *result;
    type *integer_type;
    type_expression *index_type;
    variable_declaration *index_variable;
    declaration *index_declaration;

    assert(index_name != NULL);
    assert(init != NULL);
    assert(test != NULL);
    assert(step != NULL);
    assert(body != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_expression(init);
        delete_expression(test);
        delete_expression(step);
        delete_statement_block(body);
        return NULL;
      }

    integer_type = get_integer_type();
    if (integer_type == NULL)
      {
        free(result);
        delete_expression(init);
        delete_expression(test);
        delete_expression(step);
        delete_statement_block(body);
        return NULL;
      }

    index_type = create_constant_type_expression(integer_type);
    if (index_type == NULL)
      {
        free(result);
        delete_expression(init);
        delete_expression(test);
        delete_expression(step);
        delete_statement_block(body);
        return NULL;
      }

    index_variable =
            create_variable_declaration(index_type, NULL, FALSE, TRUE, NULL);
    if (index_variable == NULL)
      {
        free(result);
        delete_expression(init);
        delete_expression(test);
        delete_expression(step);
        delete_statement_block(body);
        return NULL;
      }

    index_declaration = create_declaration_for_variable(index_name, FALSE,
            FALSE, TRUE, index_variable, index_declaration_location);
    if (index_variable == NULL)
      {
        free(result);
        delete_expression(init);
        delete_expression(test);
        delete_expression(step);
        delete_statement_block(body);
        return NULL;
      }

    declaration_set_parent_pointer(index_declaration, result);

    result->kind = SK_FOR;
    result->u.for_statement.index_variable = index_variable;
    result->u.for_statement.index_declaration = index_declaration;
    result->u.for_statement.init = init;
    result->u.for_statement.test = test;
    result->u.for_statement.step = step;
    result->u.for_statement.body = body;
    result->u.for_statement.is_parallel = is_parallel;

    return result;
  }

extern statement *create_iterate_statement(const char *element_name,
        expression *base, expression *filter, statement_block *body,
        boolean is_parallel,
        const source_location *element_declaration_location)
  {
    statement *result;
    type *the_type;
    type_expression *element_type;
    variable_declaration *element_variable;
    declaration *element_declaration;

    assert(element_name != NULL);
    assert(base != NULL);
    assert(body != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_expression(base);
        if (filter != NULL)
            delete_expression(filter);
        delete_statement_block(body);
        return NULL;
      }

    the_type = get_anything_type();
    if (the_type == NULL)
      {
        free(result);
        delete_expression(base);
        if (filter != NULL)
            delete_expression(filter);
        delete_statement_block(body);
        return NULL;
      }

    element_type = create_constant_type_expression(the_type);
    if (element_type == NULL)
      {
        free(result);
        delete_expression(base);
        if (filter != NULL)
            delete_expression(filter);
        delete_statement_block(body);
        return NULL;
      }

    element_variable =
            create_variable_declaration(element_type, NULL, FALSE, TRUE, NULL);
    if (element_variable == NULL)
      {
        free(result);
        delete_expression(base);
        if (filter != NULL)
            delete_expression(filter);
        delete_statement_block(body);
        return NULL;
      }

    element_declaration = create_declaration_for_variable(element_name, FALSE,
            FALSE, TRUE, element_variable, element_declaration_location);
    if (element_declaration == NULL)
      {
        free(result);
        delete_expression(base);
        if (filter != NULL)
            delete_expression(filter);
        delete_statement_block(body);
        return NULL;
      }

    declaration_set_parent_pointer(element_declaration, result);

    result->kind = SK_ITERATE;
    result->u.iterate_statement.element_variable = element_variable;
    result->u.iterate_statement.element_declaration = element_declaration;
    result->u.iterate_statement.base = base;
    result->u.iterate_statement.filter = filter;
    result->u.iterate_statement.body = body;
    result->u.iterate_statement.is_parallel = is_parallel;

    return result;
  }

extern statement *create_while_statement(expression *test,
        statement_block *body, statement_block *step)
  {
    statement *result;

    assert(test != NULL);
    assert(body != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_expression(test);
        delete_statement_block(body);
        if (step != NULL)
            delete_statement_block(step);
        return NULL;
      }

    result->kind = SK_WHILE;
    result->u.while_statement.test = test;
    result->u.while_statement.body = body;
    result->u.while_statement.step = step;

    return result;
  }

extern statement *create_do_while_statement(expression *test,
        statement_block *body, statement_block *step)
  {
    statement *result;

    assert(test != NULL);
    assert(body != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_expression(test);
        delete_statement_block(body);
        if (step != NULL)
            delete_statement_block(step);
        return NULL;
      }

    result->kind = SK_DO_WHILE;
    result->u.while_statement.test = test;
    result->u.while_statement.body = body;
    result->u.while_statement.step = step;

    return result;
  }

extern statement *create_break_statement(void)
  {
    statement *result;

    result = create_empty_statement();
    if (result == NULL)
        return NULL;

    result->kind = SK_BREAK;
    result->u.break_from = NULL;

    return result;
  }

extern statement *create_continue_statement(void)
  {
    statement *result;

    result = create_empty_statement();
    if (result == NULL)
        return NULL;

    result->kind = SK_CONTINUE;
    result->u.continue_with = NULL;

    return result;
  }

extern statement *create_label_statement(const char *label_name)
  {
    statement *result;
    char *name_copy;

    assert(label_name != NULL);

    result = create_empty_statement();
    if (result == NULL)
        return NULL;

    name_copy = MALLOC_ARRAY(char, strlen(label_name) + 1);
    if (name_copy == NULL)
      {
        free(result);
        return NULL;
      }

    strcpy(name_copy, label_name);

    result->kind = SK_LABEL;
    result->u.label_name = name_copy;

    return result;
  }

extern statement *create_statement_block_statement(statement_block *block)
  {
    statement *result;

    assert(block != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_statement_block(block);
        return NULL;
      }

    result->kind = SK_STATEMENT_BLOCK;
    result->u.block = block;

    return result;
  }

extern statement *create_single_statement(expression *lock,
        statement_block *block, declaration *lock_declaration)
  {
    statement *result;

    assert(block != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        if (lock != NULL)
            delete_expression(lock);
        delete_statement_block(block);
        if (lock_declaration != NULL)
            declaration_remove_reference(lock_declaration);
        return NULL;
      }

    result->kind = SK_SINGLE;
    result->u.single.lock = lock;
    result->u.single.block = block;
    result->u.single.lock_declaration = lock_declaration;
    result->u.single.lock_value = NULL;

    return result;
  }

extern statement *create_try_catch_statement(statement_block *body,
                                             statement_block *catcher)
  {
    statement *result;
    verdict the_verdict;

    assert(body != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_statement_block(body);
        if (catcher != NULL)
            delete_statement_block(catcher);
        return NULL;
      }

    the_verdict = catch_item_aa_init(&(result->u.try_catch.items), 1);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(body);
        delete_statement_block(body);
        if (catcher != NULL)
            delete_statement_block(catcher);
        return NULL;
      }

    result->kind = SK_TRY_CATCH;
    result->u.try_catch.body = body;
    result->u.try_catch.catcher = catcher;

    return result;
  }

extern statement *create_try_handle_statement(statement_block *body,
                                              expression *handler)
  {
    statement *result;

    assert(body != NULL);
    assert(handler != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_statement_block(body);
        delete_expression(handler);
        return NULL;
      }

    result->kind = SK_TRY_HANDLE;
    result->u.try_handle.body = body;
    result->u.try_handle.handler = handler;

    return result;
  }

extern statement *create_cleanup_statement(statement_block *body)
  {
    statement *result;

    assert(body != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_statement_block(body);
        return NULL;
      }

    result->kind = SK_CLEANUP;
    result->u.cleanup.body = body;

    return result;
  }

extern statement *create_export_statement(void)
  {
    statement *result;
    verdict the_verdict;

    result = create_empty_statement();
    if (result == NULL)
        return NULL;

    result->kind = SK_EXPORT;
    result->u.export.from = NULL;

    the_verdict = export_item_aa_init(&(result->u.export.items), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    return result;
  }

extern statement *create_hide_statement(void)
  {
    statement *result;
    verdict the_verdict;

    result = create_empty_statement();
    if (result == NULL)
        return NULL;

    result->kind = SK_HIDE;
    result->u.hide.from = NULL;

    the_verdict = mstring_aa_init(&(result->u.hide.items), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    return result;
  }

extern statement *create_use_statement(expression *to_use,
        type_expression *specified_type, const char *name)
  {
    statement *result;
    type_expression *container_type;
    verdict the_verdict;

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_expression(to_use);
        if (specified_type != NULL)
            delete_type_expression(specified_type);
        return NULL;
      }

    result->kind = SK_USE;
    result->u.use.to_use = to_use;
    result->u.use.named = (name != NULL);

    if (specified_type != NULL)
      {
        container_type = specified_type;
      }
    else
      {
        type *anything_type;

        anything_type = get_anything_type();
        if (anything_type == NULL)
          {
            free(result);
            delete_expression(to_use);
            return NULL;
          }

        container_type = create_constant_type_expression(anything_type);
        if (container_type == NULL)
          {
            free(result);
            delete_expression(to_use);
            return NULL;
          }
      }

    result->u.use.container_variable_declaration = create_variable_declaration(
            container_type, NULL, FALSE, TRUE, NULL);
    if (result->u.use.container_variable_declaration == NULL)
      {
        free(result);
        delete_expression(to_use);
        return NULL;
      }

    result->u.use.container_declaration = create_declaration_for_variable(
            ((name == NULL) ? "used" : name), FALSE, FALSE, TRUE,
            result->u.use.container_variable_declaration,
            get_expression_location(to_use));
    if (result->u.use.container_declaration == NULL)
      {
        free(result);
        delete_expression(to_use);
        return NULL;
      }

    the_verdict = use_for_item_aa_init(&(result->u.use.for_items), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        declaration_remove_reference(result->u.use.container_declaration);
        free(result);
        delete_expression(to_use);
        return NULL;
      }

    result->u.use.exceptions = create_string_index();
    if (result->u.use.exceptions == NULL)
      {
        free(result->u.use.for_items.array);
        declaration_remove_reference(result->u.use.container_declaration);
        free(result);
        delete_expression(to_use);
        return NULL;
      }

    result->u.use.parent = NULL;
    result->u.use.parent_index = 0;

    the_verdict = used_for_item_aa_init(&(result->u.use.used_for_items), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        destroy_string_index(result->u.use.exceptions);
        free(result->u.use.for_items.array);
        declaration_remove_reference(result->u.use.container_declaration);
        free(result);
        delete_expression(to_use);
        return NULL;
      }

    return result;
  }

extern statement *create_include_statement(const char *include_file)
  {
    statement *result;
    char *name_copy;

    assert(include_file != NULL);

    result = create_empty_statement();
    if (result == NULL)
        return NULL;

    name_copy = MALLOC_ARRAY(char, strlen(include_file) + 1);
    if (name_copy == NULL)
      {
        free(result);
        return NULL;
      }

    strcpy(name_copy, include_file);

    result->kind = SK_INCLUDE;
    result->u.include.file = name_copy;

    return result;
  }

extern statement *create_theorem_statement(expression *claim)
  {
    statement *result;

    assert(claim != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        delete_expression(claim);
        return NULL;
      }

    result->kind = SK_THEOREM;
    result->u.theorem_claim = claim;

    return result;
  }

extern statement *create_alias_statement(char *alias, char *target)
  {
    statement *result;

    assert(alias != NULL);
    assert(target != NULL);

    result = create_empty_statement();
    if (result == NULL)
      {
        free(alias);
        free(target);
        return NULL;
      }

    result->kind = SK_ALIAS;
    result->u.alias.alias = alias;
    result->u.alias.target = target;

    return result;
  }

extern void delete_statement(statement *the_statement)
  {
    assert(the_statement != NULL);

    if (the_statement->overload_chain != NULL)
      {
        routine_declaration_chain_remove_reference(
                the_statement->overload_chain);
      }

    switch (the_statement->kind)
      {
        case SK_ASSIGN:
          {
            assert(the_statement->u.assign.basket != NULL);
            delete_basket(the_statement->u.assign.basket);
            assert(the_statement->u.assign.expression != NULL);
            delete_expression(the_statement->u.assign.expression);
            break;
          }
        case SK_INCREMENT:
        case SK_DECREMENT:
          {
            assert(the_statement->u.assign.basket != NULL);
            delete_basket(the_statement->u.assign.basket);
            assert(the_statement->u.assign.expression == NULL);
            break;
          }
        case SK_CALL:
          {
            assert(the_statement->u.call != NULL);
            delete_call(the_statement->u.call);
            break;
          }
        case SK_DECLARATION:
          {
            declaration **declarations;
            size_t declaration_count;
            size_t declaration_num;

            declarations = the_statement->u.declarations.array;

            declaration_count = the_statement->u.declarations.element_count;
            for (declaration_num = 0; declaration_num < declaration_count;
                 ++declaration_num)
              {
                declaration_remove_reference(declarations[declaration_num]);
              }

            free(declarations);

            break;
          }
        case SK_IF:
          {
            else_if_item *else_ifs;
            size_t else_if_count;
            size_t else_if_num;

            delete_expression(the_statement->u.if_statement.test);
            delete_statement_block(the_statement->u.if_statement.body);
            if (the_statement->u.if_statement.else_body != NULL)
              {
                delete_statement_block(
                        the_statement->u.if_statement.else_body);
              }

            else_ifs = the_statement->u.if_statement.else_ifs.array;
            else_if_count =
                    the_statement->u.if_statement.else_ifs.element_count;
            for (else_if_num = 0; else_if_num < else_if_count; ++else_if_num)
              {
                delete_expression(else_ifs[else_if_num].test);
                delete_statement_block(else_ifs[else_if_num].body);
              }
            free(else_ifs);

            break;
          }
        case SK_SWITCH:
          {
            type_expression **case_types;
            statement_block **case_blocks;
            size_t count;
            size_t case_num;

            delete_expression(the_statement->u.switch_statement.base);

            case_types = the_statement->u.switch_statement.case_types.array;
            case_blocks = the_statement->u.switch_statement.case_blocks.array;

            assert(case_types != NULL);
            assert(case_blocks != NULL);

            count = the_statement->u.switch_statement.case_types.element_count;
            assert(count ==
                   the_statement->u.switch_statement.case_blocks.
                           element_count);

            for (case_num = 0; case_num < count; ++case_num)
              {
                delete_type_expression(case_types[case_num]);
                delete_statement_block(case_blocks[case_num]);
              }

            free(case_types);
            free(case_blocks);

            break;
          }
        case SK_GOTO:
          {
            delete_expression(the_statement->u.goto_target);
            break;
          }
        case SK_RETURN:
          {
            if (the_statement->u.return_statement.return_value != NULL)
              {
                delete_expression(
                        the_statement->u.return_statement.return_value);
              }
            break;
          }
        case SK_FOR:
          {
            declaration_remove_reference(
                    the_statement->u.for_statement.index_declaration);
            delete_expression(the_statement->u.for_statement.init);
            delete_expression(the_statement->u.for_statement.test);
            delete_expression(the_statement->u.for_statement.step);
            delete_statement_block(the_statement->u.for_statement.body);
            break;
          }
        case SK_ITERATE:
          {
            declaration_remove_reference(
                    the_statement->u.iterate_statement.element_declaration);
            delete_expression(the_statement->u.iterate_statement.base);
            if (the_statement->u.iterate_statement.filter != NULL)
                delete_expression(the_statement->u.iterate_statement.filter);
            delete_statement_block(the_statement->u.iterate_statement.body);
            break;
          }
        case SK_WHILE:
          {
            delete_expression(the_statement->u.while_statement.test);
            delete_statement_block(the_statement->u.while_statement.body);
            if (the_statement->u.while_statement.step != NULL)
                delete_statement_block(the_statement->u.while_statement.step);
            break;
          }
        case SK_DO_WHILE:
          {
            delete_expression(the_statement->u.while_statement.test);
            delete_statement_block(the_statement->u.while_statement.body);
            if (the_statement->u.while_statement.step != NULL)
                delete_statement_block(the_statement->u.while_statement.step);
            break;
          }
        case SK_BREAK:
          {
            break;
          }
        case SK_CONTINUE:
          {
            break;
          }
        case SK_LABEL:
          {
            free(the_statement->u.label_name);
            break;
          }
        case SK_STATEMENT_BLOCK:
          {
            delete_statement_block(the_statement->u.block);
            break;
          }
        case SK_SINGLE:
          {
            if (the_statement->u.single.lock != NULL)
                delete_expression(the_statement->u.single.lock);
            delete_statement_block(the_statement->u.single.block);
            if (the_statement->u.single.lock_declaration != NULL)
              {
                declaration_remove_reference(
                        the_statement->u.single.lock_declaration);
              }
            if (the_statement->u.single.lock_value != NULL)
              {
                value_remove_reference(the_statement->u.single.lock_value,
                                       NULL);
              }
            break;
          }
        case SK_TRY_CATCH:
          {
            catch_item *items;
            size_t item_count;
            size_t item_num;

            delete_statement_block(the_statement->u.try_catch.body);
            if (the_statement->u.try_catch.catcher != NULL)
                delete_statement_block(the_statement->u.try_catch.catcher);

            items = the_statement->u.try_catch.items.array;
            item_count = the_statement->u.try_catch.items.element_count;
            for (item_num = 0; item_num < item_count; ++item_num)
              {
                variable_declaration_remove_reference(
                        items[item_num].exception_variable);
                if (items[item_num].tag_type != NULL)
                    delete_type_expression(items[item_num].tag_type);
                delete_statement_block(items[item_num].catcher);
              }
            free(items);

            break;
          }
        case SK_TRY_HANDLE:
          {
            delete_statement_block(the_statement->u.try_handle.body);
            delete_expression(the_statement->u.try_handle.handler);
            break;
          }
        case SK_CLEANUP:
          {
            delete_statement_block(the_statement->u.cleanup.body);
            break;
          }
        case SK_EXPORT:
          {
            export_item *array;
            size_t item_count;
            size_t item_num;

            array = the_statement->u.export.items.array;

            item_count = the_statement->u.export.items.element_count;

            for (item_num = 0; item_num < item_count; ++item_num)
              {
                free(array[item_num].exported_as);
                delete_expression(array[item_num].to_export);
              }

            free(array);

            break;
          }
        case SK_HIDE:
          {
            char **array;
            size_t item_count;
            size_t item_num;

            array = the_statement->u.hide.items.array;

            item_count = the_statement->u.hide.items.element_count;

            for (item_num = 0; item_num < item_count; ++item_num)
                free(array[item_num]);

            free(array);

            break;
          }
        case SK_USE:
          {
            use_for_item *use_for_array;
            size_t item_count;
            size_t item_num;
            used_for_item *used_for_items;

            delete_expression(the_statement->u.use.to_use);
            declaration_remove_reference(
                    the_statement->u.use.container_declaration);

            use_for_array = the_statement->u.use.for_items.array;

            item_count = the_statement->u.use.for_items.element_count;
            for (item_num = 0; item_num < item_count; ++item_num)
              {
                free(use_for_array[item_num].exported_as);
                free(use_for_array[item_num].to_export);
              }

            free(use_for_array);

            destroy_string_index(the_statement->u.use.exceptions);

            used_for_items = the_statement->u.use.used_for_items.array;

            item_count = the_statement->u.use.used_for_items.element_count;
            for (item_num = 0; item_num < item_count; ++item_num)
              {
                if (used_for_items[item_num].chain != NULL)
                  {
                    routine_declaration_chain_remove_reference(
                            used_for_items[item_num].chain);
                  }
                free(used_for_items[item_num].name);
                source_location_remove_reference(
                        &(used_for_items[item_num].ultimate_use_location));
              }

            free(used_for_items);

            break;
          }
        case SK_INCLUDE:
          {
            free(the_statement->u.include.file);
            break;
          }
        case SK_THEOREM:
          {
            delete_expression(the_statement->u.theorem_claim);
            break;
          }
        case SK_ALIAS:
          {
            free(the_statement->u.alias.alias);
            free(the_statement->u.alias.target);
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    source_location_remove_reference(&(the_statement->location));

    free(the_statement);
  }

extern statement_kind get_statement_kind(statement *the_statement)
  {
    assert(the_statement != NULL);

    return the_statement->kind;
  }

extern routine_declaration_chain *statement_overload_chain(
        statement *the_statement)
  {
    assert(the_statement != NULL);

    return the_statement->overload_chain;
  }

extern statement *statement_overload_use_statement(statement *the_statement)
  {
    assert(the_statement != NULL);

    return the_statement->overload_use_statement;
  }

extern size_t statement_overload_use_used_for_number(statement *the_statement)
  {
    assert(the_statement != NULL);

    return the_statement->overload_used_for_number;
  }

extern basket *assign_statement_basket(statement *assign_statement)
  {
    assert(assign_statement != NULL);

    assert(assign_statement->kind == SK_ASSIGN);
    return assign_statement->u.assign.basket;
  }

extern expression *assign_statement_expression(statement *assign_statement)
  {
    assert(assign_statement != NULL);

    assert(assign_statement->kind == SK_ASSIGN);
    return assign_statement->u.assign.expression;
  }

extern assignment_kind assign_statement_assignment_kind(
        statement *assign_statement)
  {
    assert(assign_statement != NULL);

    assert(assign_statement->kind == SK_ASSIGN);
    return assign_statement->u.assign.kind;
  }

extern basket *increment_statement_basket(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_INCREMENT);
    return the_statement->u.assign.basket;
  }

extern basket *decrement_statement_basket(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_DECREMENT);
    return the_statement->u.assign.basket;
  }

extern call *call_statement_call(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_CALL);
    assert(the_statement->u.call != NULL);
    return the_statement->u.call;
  }

extern size_t declaration_statement_declaration_count(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_DECLARATION);
    return the_statement->u.declarations.element_count;
  }

extern declaration *declaration_statement_declaration(statement *the_statement,
        size_t declaration_number)
  {
    assert(the_statement != NULL);
    assert(declaration_number < the_statement->u.declarations.element_count);

    assert(the_statement->kind == SK_DECLARATION);
    return the_statement->u.declarations.array[declaration_number];
  }

extern expression *if_statement_test(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_IF);
    return the_statement->u.if_statement.test;
  }

extern statement_block *if_statement_body(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_IF);
    return the_statement->u.if_statement.body;
  }

extern size_t if_statement_else_if_count(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_IF);
    return the_statement->u.if_statement.else_ifs.element_count;
  }

extern expression *if_statement_else_if_test(statement *the_statement,
                                             size_t else_if_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_IF);
    assert(else_if_number <
           the_statement->u.if_statement.else_ifs.element_count);
    return the_statement->u.if_statement.else_ifs.array[else_if_number].test;
  }

extern statement_block *if_statement_else_if_body(statement *the_statement,
                                                  size_t else_if_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_IF);
    assert(else_if_number <
           the_statement->u.if_statement.else_ifs.element_count);
    return the_statement->u.if_statement.else_ifs.array[else_if_number].body;
  }

extern statement_block *if_statement_else_body(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_IF);
    return the_statement->u.if_statement.else_body;
  }

extern expression *switch_statement_base(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_SWITCH);
    return the_statement->u.switch_statement.base;
  }

extern size_t switch_statement_case_count(statement *the_statement)
  {
    size_t count;

    assert(the_statement != NULL);

    assert(the_statement->kind == SK_SWITCH);
    count = the_statement->u.switch_statement.case_types.element_count;
    assert(count ==
           the_statement->u.switch_statement.case_blocks.element_count);
    return count;
  }

extern type_expression *switch_statement_case_type(statement *the_statement,
                                                   size_t case_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_SWITCH);
    assert(case_number < switch_statement_case_count(the_statement));
    return the_statement->u.switch_statement.case_types.array[case_number];
  }

extern statement_block *switch_statement_case_block(statement *the_statement,
                                                    size_t case_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_SWITCH);
    assert(case_number < switch_statement_case_count(the_statement));
    return the_statement->u.switch_statement.case_blocks.array[case_number];
  }

extern expression *goto_statement_target(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_GOTO);
    return the_statement->u.goto_target;
  }

extern expression *return_statement_return_value(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_RETURN);
    return the_statement->u.return_statement.return_value;
  }

extern routine_declaration *return_statement_from_routine(
        statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_RETURN);
    if (the_statement->u.return_statement.to_routine)
        return the_statement->u.return_statement.u.declaration;
    else
        return NULL;
  }

extern expression *return_statement_from_block_expression(
        statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_RETURN);
    if (the_statement->u.return_statement.to_routine)
        return NULL;
    else
        return the_statement->u.return_statement.u.expression;
  }

extern variable_declaration *for_statement_index(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_FOR);
    return the_statement->u.for_statement.index_variable;
  }

extern expression *for_statement_init(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_FOR);
    return the_statement->u.for_statement.init;
  }

extern expression *for_statement_test(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_FOR);
    return the_statement->u.for_statement.test;
  }

extern expression *for_statement_step(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_FOR);
    return the_statement->u.for_statement.step;
  }

extern statement_block *for_statement_body(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_FOR);
    return the_statement->u.for_statement.body;
  }

extern boolean for_statement_is_parallel(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_FOR);
    return the_statement->u.for_statement.is_parallel;
  }

extern variable_declaration *iterate_statement_element(
        statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_ITERATE);
    return the_statement->u.iterate_statement.element_variable;
  }

extern expression *iterate_statement_base(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_ITERATE);
    return the_statement->u.iterate_statement.base;
  }

extern expression *iterate_statement_filter(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_ITERATE);
    return the_statement->u.iterate_statement.filter;
  }

extern statement_block *iterate_statement_body(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_ITERATE);
    return the_statement->u.iterate_statement.body;
  }

extern boolean iterate_statement_is_parallel(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_ITERATE);
    return the_statement->u.iterate_statement.is_parallel;
  }

extern expression *while_statement_test(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_WHILE);
    return the_statement->u.while_statement.test;
  }

extern statement_block *while_statement_body(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_WHILE);
    return the_statement->u.while_statement.body;
  }

extern statement_block *while_statement_step(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_WHILE);
    return the_statement->u.while_statement.step;
  }

extern expression *do_while_statement_test(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_DO_WHILE);
    return the_statement->u.while_statement.test;
  }

extern statement_block *do_while_statement_body(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_DO_WHILE);
    return the_statement->u.while_statement.body;
  }

extern statement_block *do_while_statement_step(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_DO_WHILE);
    return the_statement->u.while_statement.step;
  }

extern void *break_statement_from(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_BREAK);
    return the_statement->u.break_from;
  }

extern void *continue_statement_with(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_CONTINUE);
    return the_statement->u.continue_with;
  }

extern const char *label_statement_name(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_LABEL);
    return the_statement->u.label_name;
  }

extern statement_block *statement_block_statement_block(
        statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_STATEMENT_BLOCK);
    return the_statement->u.block;
  }

extern expression *single_statement_lock(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_SINGLE);
    return the_statement->u.single.lock;
  }

extern statement_block *single_statement_block(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_SINGLE);
    return the_statement->u.single.block;
  }

extern lock_declaration *single_statement_lock_declaration(
        statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_SINGLE);
    return declaration_lock_declaration(
            the_statement->u.single.lock_declaration);
  }

extern value *single_statement_lock_value(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_SINGLE);
    return the_statement->u.single.lock_value;
  }

extern statement_block *try_catch_statement_body(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_TRY_CATCH);
    return the_statement->u.try_catch.body;
  }

extern size_t try_catch_statement_tagged_catcher_count(
        statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_TRY_CATCH);
    return the_statement->u.try_catch.items.element_count;
  }

extern variable_declaration *try_catch_statement_tagged_catcher_exception(
        statement *the_statement, size_t tagged_catcher_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_TRY_CATCH);
    assert(tagged_catcher_number <
           the_statement->u.try_catch.items.element_count);
    return the_statement->u.try_catch.items.array[tagged_catcher_number].
            exception_variable;
  }

extern type_expression *try_catch_statement_tagged_catcher_tag_type(
        statement *the_statement, size_t tagged_catcher_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_TRY_CATCH);
    assert(tagged_catcher_number <
           the_statement->u.try_catch.items.element_count);
    return the_statement->u.try_catch.items.array[tagged_catcher_number].
            tag_type;
  }

extern statement_block *try_catch_statement_tagged_catcher_catcher(
        statement *the_statement, size_t tagged_catcher_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_TRY_CATCH);
    assert(tagged_catcher_number <
           the_statement->u.try_catch.items.element_count);
    return the_statement->u.try_catch.items.array[tagged_catcher_number].
            catcher;
  }

extern statement_block *try_catch_statement_catcher(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_TRY_CATCH);
    return the_statement->u.try_catch.catcher;
  }

extern statement_block *try_handle_statement_body(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_TRY_HANDLE);
    return the_statement->u.try_handle.body;
  }

extern expression *try_handle_statement_handler(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_TRY_HANDLE);
    return the_statement->u.try_handle.handler;
  }

extern statement_block *cleanup_statement_body(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_CLEANUP);
    return the_statement->u.cleanup.body;
  }

extern routine_declaration *export_statement_from_routine(
        statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_EXPORT);
    return the_statement->u.export.from;
  }

extern size_t export_statement_item_count(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_EXPORT);
    return the_statement->u.export.items.element_count;
  }

extern const char *export_statement_item_exported_as(statement *the_statement,
                                                     size_t item_num)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_EXPORT);
    assert(item_num < the_statement->u.export.items.element_count);
    return the_statement->u.export.items.array[item_num].exported_as;
  }

extern expression *export_statement_item_to_export(statement *the_statement,
                                                   size_t item_num)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_EXPORT);
    assert(item_num < the_statement->u.export.items.element_count);
    return the_statement->u.export.items.array[item_num].to_export;
  }

extern routine_declaration *hide_statement_from_routine(
        statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_HIDE);
    return the_statement->u.hide.from;
  }

extern size_t hide_statement_item_count(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_HIDE);
    return the_statement->u.hide.items.element_count;
  }

extern const char *hide_statement_item_to_hide(statement *the_statement,
                                               size_t item_num)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_HIDE);
    assert(item_num < the_statement->u.hide.items.element_count);
    return the_statement->u.hide.items.array[item_num];
  }

extern expression *use_statement_to_use(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    return the_statement->u.use.to_use;
  }

extern boolean use_statement_named(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    return the_statement->u.use.named;
  }

extern variable_declaration *use_statement_container(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    return the_statement->u.use.container_variable_declaration;
  }

extern size_t use_statement_for_item_count(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    return the_statement->u.use.for_items.element_count;
  }

extern const char *use_statement_for_item_exported_as(statement *the_statement,
                                                      size_t item_num)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    assert(item_num < the_statement->u.use.for_items.element_count);
    return the_statement->u.use.for_items.array[item_num].exported_as;
  }

extern const char *use_statement_for_item_to_export(statement *the_statement,
                                                    size_t item_num)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    assert(item_num < the_statement->u.use.for_items.element_count);
    return the_statement->u.use.for_items.array[item_num].to_export;
  }

extern boolean use_statement_is_exception(statement *the_statement,
                                          const char *name)
  {
    assert(the_statement != NULL);
    assert(name != NULL);

    assert(the_statement->kind == SK_USE);
    return exists_in_string_index(the_statement->u.use.exceptions, name);
  }

extern statement_block *use_statement_parent(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    return the_statement->u.use.parent;
  }

extern size_t use_statement_parent_index(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    return the_statement->u.use.parent_index;
  }

extern size_t use_statement_used_for_count(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    return the_statement->u.use.used_for_items.element_count;
  }

extern const char *use_statement_used_for_name(statement *the_statement,
                                               size_t used_for_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    assert(used_for_number <
           the_statement->u.use.used_for_items.element_count);
    return the_statement->u.use.used_for_items.array[used_for_number].name;
  }

extern boolean use_statement_used_for_required(statement *the_statement,
                                               size_t used_for_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    assert(used_for_number <
           the_statement->u.use.used_for_items.element_count);
    return the_statement->u.use.used_for_items.array[used_for_number].required;
  }

extern boolean use_statement_used_for_flow_through_allowed(
        statement *the_statement, size_t used_for_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    assert(used_for_number <
           the_statement->u.use.used_for_items.element_count);
    return the_statement->u.use.used_for_items.array[used_for_number].
            flow_through_allowed;
  }

extern declaration *use_statement_used_for_declaration(
        statement *the_statement, size_t used_for_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    assert(used_for_number <
           the_statement->u.use.used_for_items.element_count);
    return the_statement->u.use.used_for_items.array[used_for_number].
            declaration;
  }

extern routine_declaration_chain *use_statement_used_for_chain(
        statement *the_statement, size_t used_for_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    assert(used_for_number <
           the_statement->u.use.used_for_items.element_count);
    return the_statement->u.use.used_for_items.array[used_for_number].chain;
  }

extern statement *use_statement_used_for_label_statement(
        statement *the_statement, size_t used_for_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    assert(used_for_number <
           the_statement->u.use.used_for_items.element_count);
    return the_statement->u.use.used_for_items.array[used_for_number].
            label_statement;
  }

extern statement *use_statement_used_for_next_use(statement *the_statement,
                                                  size_t used_for_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    assert(used_for_number <
           the_statement->u.use.used_for_items.element_count);
    return the_statement->u.use.used_for_items.array[used_for_number].
            next_statement;
  }

extern size_t use_statement_used_for_next_used_for_number(
        statement *the_statement, size_t used_for_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    assert(used_for_number <
           the_statement->u.use.used_for_items.element_count);
    return the_statement->u.use.used_for_items.array[used_for_number].
            next_used_for_number;
  }

extern source_location *use_statement_used_for_ultimate_use_location(
        statement *the_statement, size_t used_for_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    assert(used_for_number <
           the_statement->u.use.used_for_items.element_count);
    return &(the_statement->u.use.used_for_items.array[used_for_number].
                     ultimate_use_location);
  }

extern const char *include_statement_file_name(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_INCLUDE);
    return the_statement->u.include.file;
  }

extern expression *theorem_statement_claim(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_THEOREM);
    return the_statement->u.theorem_claim;
  }

extern const char *alias_statement_alias(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_ALIAS);
    return the_statement->u.alias.alias;
  }

extern const char *alias_statement_target(statement *the_statement)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_ALIAS);
    return the_statement->u.alias.target;
  }

extern void set_statement_start_location(statement *the_statement,
                                         const source_location *location)
  {
    assert(the_statement != NULL);

    set_location_start(&(the_statement->location), location);
  }

extern void set_statement_end_location(statement *the_statement,
                                       const source_location *location)
  {
    assert(the_statement != NULL);

    set_location_end(&(the_statement->location), location);
  }

extern void statement_set_overload_chain(statement *the_statement,
        routine_declaration_chain *overload_chain)
  {
    assert(the_statement != NULL);

    assert(the_statement->overload_chain == NULL);
    assert(the_statement->overload_use_statement == NULL);
    routine_declaration_chain_add_reference(overload_chain);
    the_statement->overload_chain = overload_chain;
  }

extern void statement_set_overload_use(statement *the_statement,
        statement *overload_use_statement, size_t overload_used_for_number)
  {
    assert(the_statement != NULL);

    assert(the_statement->overload_chain == NULL);
    assert(the_statement->overload_use_statement == NULL);
    the_statement->overload_use_statement = overload_use_statement;
    the_statement->overload_used_for_number = overload_used_for_number;
  }

extern verdict declaration_statement_add_declaration(statement *the_statement,
        declaration *the_declaration)
  {
    verdict the_verdict;

    assert(the_statement != NULL);
    assert(the_declaration != NULL);

    the_verdict = declaration_aa_append(&(the_statement->u.declarations),
                                        the_declaration);
    if (the_verdict != MISSION_ACCOMPLISHED)
        declaration_remove_reference(the_declaration);
    return the_verdict;
  }

extern verdict add_if_statement_else_if(statement *the_statement,
        expression *test, statement_block *body)
  {
    else_if_item new_item;
    verdict the_verdict;

    assert(the_statement != NULL);
    assert(test != NULL);
    assert(body != NULL);

    assert(the_statement->kind == SK_IF);

    new_item.test = test;
    new_item.body = body;

    the_verdict = else_if_item_aa_append(
            &(the_statement->u.if_statement.else_ifs), new_item);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_expression(test);
        delete_statement_block(body);
      }

    return the_verdict;
  }

extern void add_if_statement_else(statement *the_statement,
                                  statement_block *body)
  {
    assert(the_statement != NULL);
    assert(body != NULL);

    assert(the_statement->kind == SK_IF);
    assert(the_statement->u.if_statement.else_body == NULL);
    the_statement->u.if_statement.else_body = body;
  }

extern verdict add_switch_statement_case(statement *the_statement,
        type_expression *type, statement_block *block)
  {
    verdict the_verdict;

    assert(the_statement != NULL);
    assert(type != NULL);
    assert(block != NULL);

    assert(the_statement->kind == SK_SWITCH);

    the_verdict = type_expression_aa_append(
            &(the_statement->u.switch_statement.case_types), type);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_type_expression(type);
        delete_statement_block(block);
        return the_verdict;
      }

    the_verdict = statement_block_aa_append(
            &(the_statement->u.switch_statement.case_blocks), block);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(the_statement->u.switch_statement.case_types.element_count > 0);
        --(the_statement->u.switch_statement.case_types.element_count);
        delete_type_expression(type);
        delete_statement_block(block);
        return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_return_statement_to_routine_declaration(
        statement *return_statement, routine_declaration *declaration)
  {
    assert(return_statement != NULL);
    assert(declaration != NULL);

    assert(return_statement->kind == SK_RETURN);
    assert(return_statement->u.return_statement.to_routine);
    assert(return_statement->u.return_statement.u.declaration == NULL);
    return_statement->u.return_statement.u.declaration = declaration;

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_return_statement_to_block_expression(
        statement *return_statement, expression *the_expression)
  {
    assert(return_statement != NULL);
    assert(the_expression != NULL);

    assert(return_statement->kind == SK_RETURN);
    assert(return_statement->u.return_statement.to_routine);
    assert(return_statement->u.return_statement.u.declaration == NULL);
    return_statement->u.return_statement.to_routine = FALSE;
    return_statement->u.return_statement.u.expression = the_expression;

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_export_statement_to_from_declaration(
        statement *export_statement, routine_declaration *declaration)
  {
    assert(export_statement != NULL);
    assert(declaration != NULL);

    assert(export_statement->kind == SK_EXPORT);
    assert(export_statement->u.export.from == NULL);
    export_statement->u.export.from = declaration;

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_hide_statement_to_from_declaration(
        statement *hide_statement, routine_declaration *declaration)
  {
    assert(hide_statement != NULL);
    assert(declaration != NULL);

    assert(hide_statement->kind == SK_HIDE);
    assert(hide_statement->u.hide.from == NULL);
    hide_statement->u.hide.from = declaration;

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_break_statement_from(statement *break_statement,
                                         void *from)
  {
    assert(break_statement != NULL);

    assert(break_statement->kind == SK_BREAK);
    assert(break_statement->u.break_from == NULL);

    assert(from != NULL);

    break_statement->u.break_from = from;

    return MISSION_ACCOMPLISHED;
  }

extern verdict bind_continue_statement_with(statement *continue_statement,
                                            void *with)
  {
    assert(continue_statement != NULL);

    assert(continue_statement->kind == SK_CONTINUE);
    assert(continue_statement->u.continue_with == NULL);

    assert(with != NULL);

    continue_statement->u.continue_with = with;

    return MISSION_ACCOMPLISHED;
  }

extern void single_statement_set_lock_value(statement *the_statement,
                                            value *the_value)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_SINGLE);

    assert(the_statement->u.single.lock_value == NULL);
    value_add_reference(the_value);
    the_statement->u.single.lock_value = the_value;
  }

extern verdict add_try_catch_statement_tagged_catcher(statement *the_statement,
        const char *exception_name, type_expression *tag_type,
        statement_block *catcher,
        const source_location *exception_declaration_location)
  {
    type *the_type;
    type_expression *exception_type;
    variable_declaration *exception_variable;
    declaration *exception_declaration;
    catch_item new_item;
    verdict the_verdict;

    assert(the_statement != NULL);
    assert(exception_name != NULL);
    assert(catcher != NULL);

    assert(the_statement->kind == SK_TRY_CATCH);

    the_type = get_anything_type();
    if (the_type == NULL)
      {
        if (tag_type != NULL)
            delete_type_expression(tag_type);
        delete_statement_block(catcher);
        return MISSION_FAILED;
      }

    exception_type = create_constant_type_expression(the_type);
    if (exception_type == NULL)
      {
        if (tag_type != NULL)
            delete_type_expression(tag_type);
        delete_statement_block(catcher);
        return MISSION_FAILED;
      }

    exception_variable = create_variable_declaration(exception_type, NULL,
                                                     FALSE, TRUE, NULL);
    if (exception_variable == NULL)
      {
        if (tag_type != NULL)
            delete_type_expression(tag_type);
        delete_statement_block(catcher);
        return MISSION_FAILED;
      }

    exception_declaration = create_declaration_for_variable(exception_name,
            FALSE, FALSE, TRUE, exception_variable,
            exception_declaration_location);
    if (exception_declaration == NULL)
      {
        if (tag_type != NULL)
            delete_type_expression(tag_type);
        delete_statement_block(catcher);
        return MISSION_FAILED;
      }

    declaration_set_parent_pointer(exception_declaration, the_statement);

    new_item.exception_variable = exception_variable;
    new_item.tag_type = tag_type;
    new_item.catcher = catcher;

    the_verdict = catch_item_aa_append(&(the_statement->u.try_catch.items),
                                       new_item);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        variable_declaration_remove_reference(exception_variable);
        if (tag_type != NULL)
            delete_type_expression(tag_type);
        delete_statement_block(catcher);
      }

    return the_verdict;
  }

extern void add_try_catch_statement_catcher(statement *the_statement,
                                            statement_block *catcher)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_TRY_CATCH);

    the_statement->u.try_catch.catcher = catcher;
  }

extern verdict export_statement_add_item(statement *export_statement,
        const char *exported_as, expression *to_export)
  {
    char *name_copy;
    export_item new_item;
    verdict result;

    assert(export_statement != NULL);
    assert(exported_as != NULL);
    assert(to_export != NULL);

    assert(export_statement->kind == SK_EXPORT);

    name_copy = MALLOC_ARRAY(char, strlen(exported_as) + 1);
    if (name_copy == NULL)
      {
        delete_expression(to_export);
        return MISSION_FAILED;
      }

    strcpy(name_copy, exported_as);

    new_item.exported_as = name_copy;
    new_item.to_export = to_export;

    result = export_item_aa_append(&(export_statement->u.export.items),
                                   new_item);
    if (result != MISSION_ACCOMPLISHED)
      {
        free(name_copy);
        delete_expression(to_export);
      }

    return result;
  }

extern verdict hide_statement_add_item(statement *hide_statement,
                                       const char *to_hide)
  {
    char *name_copy;
    verdict result;

    assert(hide_statement != NULL);
    assert(to_hide != NULL);

    assert(hide_statement->kind == SK_HIDE);

    name_copy = MALLOC_ARRAY(char, strlen(to_hide) + 1);
    if (name_copy == NULL)
        return MISSION_FAILED;

    strcpy(name_copy, to_hide);

    result = mstring_aa_append(&(hide_statement->u.hide.items), name_copy);
    if (result != MISSION_ACCOMPLISHED)
        free(name_copy);

    return result;
  }

extern verdict use_statement_add_for_item(statement *use_statement,
        const char *exported_as, const char *to_export)
  {
    char *exported_as_copy;
    char *to_export_copy;
    use_for_item new_item;
    verdict result;

    assert(use_statement != NULL);
    assert(exported_as != NULL);
    assert(to_export != NULL);

    assert(use_statement->kind == SK_USE);

    exported_as_copy = MALLOC_ARRAY(char, strlen(exported_as) + 1);
    if (exported_as_copy == NULL)
        return MISSION_FAILED;

    strcpy(exported_as_copy, exported_as);

    to_export_copy = MALLOC_ARRAY(char, strlen(to_export) + 1);
    if (to_export_copy == NULL)
      {
        free(exported_as_copy);
        return MISSION_FAILED;
      }

    strcpy(to_export_copy, to_export);

    new_item.exported_as = exported_as_copy;
    new_item.to_export = to_export_copy;

    result = use_for_item_aa_append(&(use_statement->u.use.for_items),
                                    new_item);
    if (result != MISSION_ACCOMPLISHED)
      {
        free(to_export_copy);
        free(exported_as_copy);
      }

    return result;
  }

extern verdict use_statement_add_except_item(statement *use_statement,
                                             const char *to_hide)
  {
    assert(use_statement != NULL);

    assert(use_statement->kind == SK_USE);
    return enter_into_string_index(use_statement->u.use.exceptions, to_hide,
                                   use_statement);
  }

extern void use_statement_set_parent(statement *use_statement,
        statement_block *parent, size_t parent_index)
  {
    assert(use_statement != NULL);

    assert(use_statement->kind == SK_USE);
    use_statement->u.use.parent = parent;
    use_statement->u.use.parent_index = parent_index;
  }

extern verdict use_statement_add_used_for_case(statement *use_statement,
        const char *name, boolean flow_through_allowed,
        const source_location *ultimate_use_location)
  {
    char *name_copy;
    used_for_item new_item;

    assert(use_statement != NULL);
    assert(use_statement->kind == SK_USE);
    assert(name != NULL);

    name_copy = MALLOC_ARRAY(char, strlen(name) + 1);
    if (name_copy == NULL)
        return MISSION_FAILED;

    strcpy(name_copy, name);

    new_item.flow_through_allowed = flow_through_allowed;
    new_item.name = name_copy;
    new_item.required = FALSE;
    new_item.declaration = NULL;
    new_item.chain = NULL;
    new_item.label_statement = NULL;
    new_item.next_statement = NULL;
    new_item.next_used_for_number = 0;
    set_source_location(&(new_item.ultimate_use_location),
                        ultimate_use_location);
    return used_for_item_aa_append(&(use_statement->u.use.used_for_items),
                                   new_item);
  }

extern void use_statement_set_used_for_required(statement *the_statement,
        size_t used_for_number, boolean required)
  {
    assert(the_statement != NULL);

    assert(the_statement->kind == SK_USE);
    assert(used_for_number <
           the_statement->u.use.used_for_items.element_count);
    the_statement->u.use.used_for_items.array[used_for_number].required =
            required;
  }

extern void use_statement_set_used_for_declaration(statement *the_statement,
        size_t used_for_number, declaration *the_declaration)
  {
    assert(the_statement != NULL);
    assert(the_declaration != NULL);

    assert(the_statement->kind == SK_USE);
    assert(used_for_number <
           the_statement->u.use.used_for_items.element_count);

    assert(the_statement->u.use.used_for_items.array[used_for_number].
                   declaration == NULL);
    assert(the_statement->u.use.used_for_items.array[used_for_number].chain ==
           NULL);
    assert(the_statement->u.use.used_for_items.array[used_for_number].
                   label_statement == NULL);
    assert(the_statement->u.use.used_for_items.array[used_for_number].
                   next_statement == NULL);

    the_statement->u.use.used_for_items.array[used_for_number].declaration =
            the_declaration;
  }

extern void use_statement_set_used_for_chain(statement *the_statement,
        size_t used_for_number, routine_declaration_chain *chain)
  {
    assert(the_statement != NULL);
    assert(chain != NULL);

    assert(the_statement->kind == SK_USE);
    assert(used_for_number <
           the_statement->u.use.used_for_items.element_count);

    assert(the_statement->u.use.used_for_items.array[used_for_number].
                   declaration == NULL);
    assert(the_statement->u.use.used_for_items.array[used_for_number].chain ==
           NULL);
    assert(the_statement->u.use.used_for_items.array[used_for_number].
                   label_statement == NULL);
    assert(the_statement->u.use.used_for_items.array[used_for_number].
                   next_statement == NULL);

    routine_declaration_chain_add_reference(chain);
    the_statement->u.use.used_for_items.array[used_for_number].chain = chain;
  }

extern void use_statement_set_used_for_label_statement(
        statement *the_statement, size_t used_for_number,
        statement *label_statement)
  {
    assert(the_statement != NULL);
    assert(label_statement != NULL);

    assert(the_statement->kind == SK_USE);
    assert(label_statement->kind == SK_LABEL);
    assert(used_for_number <
           the_statement->u.use.used_for_items.element_count);

    assert(the_statement->u.use.used_for_items.array[used_for_number].
                   declaration == NULL);
    assert(the_statement->u.use.used_for_items.array[used_for_number].chain ==
           NULL);
    assert(the_statement->u.use.used_for_items.array[used_for_number].
                   label_statement == NULL);
    assert(the_statement->u.use.used_for_items.array[used_for_number].
                   next_statement == NULL);

    the_statement->u.use.used_for_items.array[used_for_number].label_statement
            = label_statement;
  }

extern void use_statement_set_used_for_next_used(statement *the_statement,
        size_t used_for_number, statement *next_statement,
        size_t next_used_for_number)
  {
    assert(the_statement != NULL);
    assert(next_statement != NULL);

    assert(the_statement->kind == SK_USE);
    assert(next_statement->kind == SK_USE);
    assert(used_for_number <
           the_statement->u.use.used_for_items.element_count);

    assert(the_statement->u.use.used_for_items.array[used_for_number].
                   declaration == NULL);
    assert(the_statement->u.use.used_for_items.array[used_for_number].chain ==
           NULL);
    assert(the_statement->u.use.used_for_items.array[used_for_number].
                   label_statement == NULL);
    assert(the_statement->u.use.used_for_items.array[used_for_number].
                   next_statement == NULL);

    the_statement->u.use.used_for_items.array[used_for_number].next_statement =
            next_statement;
    the_statement->u.use.used_for_items.array[used_for_number].
            next_used_for_number = next_used_for_number;
  }

extern const source_location *get_statement_location(statement *the_statement)
  {
    assert(the_statement != NULL);

    return &(the_statement->location);
  }

extern void statement_error(statement *the_statement, const char *format, ...)
  {
    va_list ap;

    assert(the_statement != NULL);
    assert(format != NULL);

    va_start(ap, format);
    vstatement_error(the_statement, format, ap);
    va_end(ap);
  }

extern void vstatement_error(statement *the_statement, const char *format,
                             va_list arg)
  {
    assert(the_statement != NULL);
    assert(format != NULL);

    vlocation_error(&(the_statement->location), format, arg);
  }

extern expression_kind binary_expression_kind_for_assignment(
        assignment_kind the_assignment_kind)
  {
    switch (the_assignment_kind)
      {
        case AK_MODULO:
            assert(FALSE);
            return EK_ADD;
        case AK_SIMPLE:
            assert(FALSE);
            return EK_ADD;
        case AK_MULTIPLY:
            return EK_MULTIPLY;
        case AK_DIVIDE:
            return EK_DIVIDE;
        case AK_DIVIDE_FORCE:
            return EK_DIVIDE_FORCE;
        case AK_REMAINDER:
            return EK_REMAINDER;
        case AK_ADD:
            return EK_ADD;
        case AK_SUBTRACT:
            return EK_SUBTRACT;
        case AK_SHIFT_LEFT:
            return EK_SHIFT_LEFT;
        case AK_SHIFT_RIGHT:
            return EK_SHIFT_RIGHT;
        case AK_BITWISE_AND:
            return EK_BITWISE_AND;
        case AK_BITWISE_XOR:
            return EK_BITWISE_XOR;
        case AK_BITWISE_OR:
            return EK_BITWISE_OR;
        case AK_LOGICAL_AND:
            return EK_LOGICAL_AND;
        case AK_LOGICAL_OR:
            return EK_LOGICAL_OR;
        case AK_CONCATENATE:
            return EK_CONCATENATE;
        default:
            assert(FALSE);
            return EK_ADD;
      }
  }


static statement *create_empty_statement(void)
  {
    statement *result;

    result = MALLOC_ONE_OBJECT(statement);
    if (result == NULL)
        return NULL;

    result->overload_chain = NULL;
    result->overload_use_statement = NULL;
    result->overload_used_for_number = 0;
    set_source_location(&(result->location), NULL);

    return result;
  }
