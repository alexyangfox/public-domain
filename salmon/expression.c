/* file "expression.c" */

/*
 *  This file contains the implementation of the expression module.
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
#include <stdarg.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/auto_array.h"
#include "c_foundations/auto_array_implementation.h"
#include "expression.h"
#include "value.h"
#include "declaration.h"
#include "variable_declaration.h"
#include "routine_declaration_chain.h"
#include "tagalong_declaration.h"
#include "lepton_key_declaration.h"
#include "quark_declaration.h"
#include "lock_declaration.h"
#include "statement.h"
#include "statement_block.h"
#include "routine_declaration.h"
#include "type_expression.h"
#include "call.h"
#include "source_location.h"


typedef struct
  {
    boolean is_value;
    union
      {
        type_expression *type_expression;
        expression *value_expression;
      } u;
  } value_or_type_expression;


AUTO_ARRAY(mstring_aa, char *);
AUTO_ARRAY(expression_aa, expression *);
AUTO_ARRAY(type_expression_aa, type_expression *);
AUTO_ARRAY(boolean_aa, boolean);
AUTO_ARRAY(value_or_type_expression_aa, value_or_type_expression);


struct expression
  {
    expression_kind kind;
    routine_declaration_chain *overload_chain;
    statement *overload_use_statement;
    size_t overload_used_for_number;
    union
      {
        value *constant_value;
        boolean unbound_addressable_required;
        variable_declaration *variable_declaration;
        routine_declaration_chain *routine_declaration_chain;
        tagalong_declaration *tagalong_declaration;
        lepton_key_declaration *lepton_key_declaration;
        quark_declaration *quark_declaration;
        lock_declaration *lock_declaration;
        statement *label_statement;
        struct
          {
            statement *use_statement;
            size_t used_for_num;
            boolean addressable_required;
          } use_reference;
        struct
          {
            expression *base;
            expression_aa child_expressions;
            expression_aa upper_bounds;
            type_expression_aa filters;
          } lookup;
        struct
          {
            expression *base;
            mstring_aa labels;
            expression_aa child_expressions;
            boolean_aa forces;
          } lepton;
        struct
          {
            expression *base;
            char *field_name;
          } field;
        struct
          {
            expression *base;
            expression *key;
          } tagalong_field;
        statement_block *statement_block;
        declaration *declaration;
        type_expression *type;
        struct
          {
            value_or_type_expression_aa keys;
            expression_aa targets;
          } map_list;
        struct
          {
            mstring_aa labels;
            expression_aa child_expressions;
          } semi_labeled_expression_list;
        call *call;
        struct
          {
            expression *test;
            expression *then_part;
            expression *else_part;
          } conditional;
        struct
          {
            expression *operand;
          } unary;
        struct
          {
            expression *operand1;
            expression *operand2;
          } binary;
        struct
          {
            routine_declaration *declaration;
          } arguments;
        struct
          {
            routine_declaration *declaration;
          } this;
        struct
          {
            expression *expression;
            type_expression *type;
          } in;
        struct
          {
            expression *expression;
            type_expression *type;
          } force;
        void *break_from;
        void *continue_with;
        struct
          {
            variable_declaration *element_variable;
            declaration *element_declaration;
            expression *base;
            expression *filter;
            expression *body;
          } comprehend;
        struct
          {
            formal_arguments *formals;
            expression *body;
          } forall;
        struct
          {
            formal_arguments *formals;
            expression *body;
          } exists;
      } u;
    source_location location;
  };


AUTO_ARRAY_IMPLEMENTATION(boolean_aa, boolean, 0);
AUTO_ARRAY_IMPLEMENTATION(value_or_type_expression_aa,
                          value_or_type_expression, 0);


static expression *create_empty_expression(void);
static boolean is_unary_kind(expression_kind kind);
static boolean is_binary_kind(expression_kind kind);


extern expression *create_constant_expression(value *the_value)
  {
    expression *result;

    assert(the_value != NULL);

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    value_add_reference(the_value);

    result->kind = EK_CONSTANT;
    result->u.constant_value = the_value;

    return result;
  }

extern expression *create_unbound_name_reference_expression(void)
  {
    expression *result;

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_UNBOUND_NAME_REFERENCE;
    result->u.unbound_addressable_required = FALSE;

    return result;
  }

extern expression *create_variable_reference_expression(
        variable_declaration *declaration)
  {
    expression *result;

    assert(declaration != NULL);

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_VARIABLE_REFERENCE;
    result->u.variable_declaration = declaration;

    return result;
  }

extern expression *create_routine_reference_expression(
        routine_declaration_chain *chain)
  {
    expression *result;

    assert(chain != NULL);

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_ROUTINE_REFERENCE;
    routine_declaration_chain_add_reference(chain);
    result->u.routine_declaration_chain = chain;

    return result;
  }

extern expression *create_label_reference_expression(
        statement *label_statement)
  {
    expression *result;

    assert(label_statement != NULL);

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_LABEL_REFERENCE;
    result->u.label_statement = label_statement;

    return result;
  }

extern expression *create_tagalong_reference_expression(
        tagalong_declaration *declaration)
  {
    expression *result;

    assert(declaration != NULL);

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_TAGALONG_REFERENCE;
    result->u.tagalong_declaration = declaration;

    return result;
  }

extern expression *create_lepton_key_reference_expression(
        lepton_key_declaration *declaration)
  {
    expression *result;

    assert(declaration != NULL);

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_LEPTON_KEY_REFERENCE;
    result->u.lepton_key_declaration = declaration;

    return result;
  }

extern expression *create_quark_reference_expression(
        quark_declaration *declaration)
  {
    expression *result;

    assert(declaration != NULL);

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_QUARK_REFERENCE;
    result->u.quark_declaration = declaration;

    return result;
  }

extern expression *create_lock_reference_expression(
        lock_declaration *declaration)
  {
    expression *result;

    assert(declaration != NULL);

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_LOCK_REFERENCE;
    result->u.lock_declaration = declaration;

    return result;
  }

extern expression *create_use_reference_expression(statement *use_statement,
                                                   size_t used_for_num)
  {
    expression *result;

    assert(use_statement != NULL);

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_USE_REFERENCE;
    result->u.use_reference.use_statement = use_statement;
    result->u.use_reference.used_for_num = used_for_num;
    result->u.use_reference.addressable_required = FALSE;

    return result;
  }

extern expression *create_lookup_expression(expression *base)
  {
    expression *result;
    verdict the_verdict;

    assert(base != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_expression(base);
        return NULL;
      }

    result->kind = EK_LOOKUP;
    result->u.lookup.base = base;

    the_verdict =
            expression_aa_init(&(result->u.lookup.child_expressions), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_expression(base);
        free(result);
        return NULL;
      }

    the_verdict = expression_aa_init(&(result->u.lookup.upper_bounds), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->u.lookup.child_expressions.array);
        delete_expression(base);
        free(result);
        return NULL;
      }

    the_verdict = type_expression_aa_init(&(result->u.lookup.filters), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->u.lookup.upper_bounds.array);
        free(result->u.lookup.child_expressions.array);
        delete_expression(base);
        free(result);
        return NULL;
      }

    return result;
  }

extern expression *create_lepton_expression(expression *base)
  {
    expression *result;
    verdict the_verdict;

    assert(base != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_expression(base);
        return NULL;
      }

    result->kind = EK_LEPTON;
    result->u.lepton.base = base;

    the_verdict = mstring_aa_init(&(result->u.lepton.labels), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_expression(base);
        free(result);
        return NULL;
      }

    the_verdict =
            expression_aa_init(&(result->u.lepton.child_expressions), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->u.lepton.labels.array);
        delete_expression(base);
        free(result);
        return NULL;
      }

    the_verdict = boolean_aa_init(&(result->u.lepton.forces), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->u.lepton.child_expressions.array);
        free(result->u.lepton.labels.array);
        delete_expression(base);
        free(result);
        return NULL;
      }

    return result;
  }

extern expression *create_field_expression(expression *base,
                                           const char *field_name)
  {
    char *copy;
    expression *result;

    assert(base != NULL);
    assert(field_name != NULL);

    copy = MALLOC_ARRAY(char, strlen(field_name) + 1);
    if (copy == NULL)
      {
        delete_expression(base);
        return NULL;
      }
    strcpy(copy, field_name);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_expression(base);
        free(copy);
        return NULL;
      }

    result->kind = EK_FIELD;
    result->u.field.base = base;
    result->u.field.field_name = copy;

    return result;
  }

extern expression *create_pointer_field_expression(expression *base,
                                                   const char *field_name)
  {
    char *copy;
    expression *result;

    assert(base != NULL);
    assert(field_name != NULL);

    copy = MALLOC_ARRAY(char, strlen(field_name) + 1);
    if (copy == NULL)
      {
        delete_expression(base);
        return NULL;
      }
    strcpy(copy, field_name);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_expression(base);
        free(copy);
        return NULL;
      }

    result->kind = EK_POINTER_FIELD;
    result->u.field.base = base;
    result->u.field.field_name = copy;

    return result;
  }

extern expression *create_tagalong_field_expression(expression *base,
                                                    expression *key)
  {
    expression *result;

    assert(base != NULL);
    assert(key != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_expression(base);
        delete_expression(key);
        return NULL;
      }

    result->kind = EK_TAGALONG_FIELD;
    result->u.tagalong_field.base = base;
    result->u.tagalong_field.key = key;

    return result;
  }

extern expression *create_statement_block_expression(
        statement_block *the_statement_block)
  {
    expression *result;

    assert(the_statement_block != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_statement_block(the_statement_block);
        return NULL;
      }

    result->kind = EK_STATEMENT_BLOCK;
    result->u.statement_block = the_statement_block;

    return result;
  }

extern expression *create_declaration_expression(declaration *the_declaration)
  {
    expression *result;

    assert(the_declaration != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        declaration_remove_reference(the_declaration);
        return NULL;
      }

    result->kind = EK_DECLARATION;
    result->u.declaration = the_declaration;

    return result;
  }

extern expression *create_type_expression_expression(type_expression *type)
  {
    expression *result;

    assert(type != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_type_expression(type);
        return NULL;
      }

    result->kind = EK_TYPE;
    result->u.type = type;

    return result;
  }

extern expression *create_map_list_expression(void)
  {
    expression *result;
    verdict the_verdict;

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_MAP_LIST;

    the_verdict =
            value_or_type_expression_aa_init(&(result->u.map_list.keys), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    the_verdict = expression_aa_init(&(result->u.map_list.targets), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->u.map_list.keys.array);
        free(result);
        return NULL;
      }

    return result;
  }

extern expression *create_semi_labeled_expression_list_expression(void)
  {
    expression *result;
    verdict the_verdict;

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_SEMI_LABELED_EXPRESSION_LIST;

    the_verdict = mstring_aa_init(
            &(result->u.semi_labeled_expression_list.labels), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    the_verdict = expression_aa_init(
            &(result->u.semi_labeled_expression_list.child_expressions), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result->u.semi_labeled_expression_list.labels.array);
        free(result);
        return NULL;
      }

    return result;
  }

extern expression *create_call_expression(call *the_call)
  {
    expression *result;

    assert(the_call != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_call(the_call);
        return NULL;
      }

    result->kind = EK_CALL;
    result->u.call = the_call;

    return result;
  }

extern expression *create_conditional_expression(expression *test,
        expression *then_part, expression *else_part)
  {
    expression *result;

    assert(test != NULL);
    assert(then_part != NULL);
    assert(else_part != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_expression(test);
        delete_expression(then_part);
        delete_expression(else_part);
        return NULL;
      }

    result->kind = EK_CONDITIONAL;
    result->u.conditional.test = test;
    result->u.conditional.then_part = then_part;
    result->u.conditional.else_part = else_part;

    return result;
  }

extern expression *create_unary_expression(expression_kind kind,
                                           expression *operand)
  {
    expression *result;

    assert(is_unary_kind(kind));
    assert(operand != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_expression(operand);
        return NULL;
      }

    result->kind = kind;
    result->u.unary.operand = operand;

    return result;
  }

extern expression *create_binary_expression(expression_kind kind,
        expression *operand1, expression *operand2)
  {
    expression *result;

    assert(is_binary_kind(kind));
    assert(operand1 != NULL);
    assert(operand2 != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_expression(operand1);
        delete_expression(operand2);
        return NULL;
      }

    result->kind = kind;
    result->u.binary.operand1 = operand1;
    result->u.binary.operand2 = operand2;

    return result;
  }

extern expression *create_arguments_expression(void)
  {
    expression *result;

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_ARGUMENTS;
    result->u.arguments.declaration = NULL;

    return result;
  }

extern expression *create_this_expression(void)
  {
    expression *result;

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_THIS;
    result->u.this.declaration = NULL;

    return result;
  }

extern expression *create_in_expression(expression *the_expression,
                                        type_expression *type)
  {
    expression *result;

    assert(the_expression != NULL);
    assert(type != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_expression(the_expression);
        delete_type_expression(type);
        return NULL;
      }

    result->kind = EK_IN;
    result->u.in.expression = the_expression;
    result->u.in.type = type;

    return result;
  }

extern expression *create_force_expression(expression *the_expression,
                                           type_expression *type)
  {
    expression *result;

    assert(the_expression != NULL);
    assert(type != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_expression(the_expression);
        delete_type_expression(type);
        return NULL;
      }

    result->kind = EK_FORCE;
    result->u.force.expression = the_expression;
    result->u.force.type = type;

    return result;
  }

extern expression *create_break_expression(void)
  {
    expression *result;

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_BREAK;
    result->u.break_from = NULL;

    return result;
  }

extern expression *create_continue_expression(void)
  {
    expression *result;

    result = create_empty_expression();
    if (result == NULL)
        return NULL;

    result->kind = EK_CONTINUE;
    result->u.continue_with = NULL;

    return result;
  }

extern expression *create_comprehend_expression(const char *element_name,
        expression *base, expression *filter, expression *body,
        const source_location *element_declaration_location)
  {
    expression *result;
    type *the_type;
    type_expression *element_type;
    variable_declaration *element_variable;
    declaration *element_declaration;

    assert(element_name != NULL);
    assert(base != NULL);
    assert(body != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_expression(base);
        if (filter != NULL)
            delete_expression(filter);
        delete_expression(body);
        return NULL;
      }

    the_type = get_anything_type();
    if (the_type == NULL)
      {
        free(result);
        delete_expression(base);
        if (filter != NULL)
            delete_expression(filter);
        delete_expression(body);
        return NULL;
      }

    element_type = create_constant_type_expression(the_type);
    if (element_type == NULL)
      {
        free(result);
        delete_expression(base);
        if (filter != NULL)
            delete_expression(filter);
        delete_expression(body);
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
        delete_expression(body);
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
        delete_expression(body);
        return NULL;
      }

    declaration_set_parent_pointer(element_declaration, result);

    result->kind = EK_COMPREHEND;
    result->u.comprehend.element_variable = element_variable;
    result->u.comprehend.element_declaration = element_declaration;
    result->u.comprehend.base = base;
    result->u.comprehend.filter = filter;
    result->u.comprehend.body = body;

    return result;
  }

extern expression *create_forall_expression(formal_arguments *formals,
                                            expression *body)
  {
    expression *result;

    assert(formals != NULL);
    assert(body != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_formal_arguments(formals);
        delete_expression(body);
        return NULL;
      }

    result->kind = EK_FORALL;
    result->u.forall.formals = formals;
    result->u.forall.body = body;

    return result;
  }

extern expression *create_exists_expression(formal_arguments *formals,
                                            expression *body)
  {
    expression *result;

    assert(formals != NULL);
    assert(body != NULL);

    result = create_empty_expression();
    if (result == NULL)
      {
        delete_formal_arguments(formals);
        delete_expression(body);
        return NULL;
      }

    result->kind = EK_EXISTS;
    result->u.exists.formals = formals;
    result->u.exists.body = body;

    return result;
  }

extern void delete_expression(expression *the_expression)
  {
    assert(the_expression != NULL);

    if (the_expression->overload_chain != NULL)
      {
        routine_declaration_chain_remove_reference(
                the_expression->overload_chain);
      }

    switch (the_expression->kind)
      {
        case EK_CONSTANT:
          {
            assert(the_expression->u.constant_value != NULL);
            value_remove_reference(the_expression->u.constant_value, NULL);
            break;
          }
        case EK_UNBOUND_NAME_REFERENCE:
          {
            break;
          }
        case EK_VARIABLE_REFERENCE:
          {
            break;
          }
        case EK_ROUTINE_REFERENCE:
          {
            routine_declaration_chain_remove_reference(
                    the_expression->u.routine_declaration_chain);
            break;
          }
        case EK_LABEL_REFERENCE:
          {
            break;
          }
        case EK_TAGALONG_REFERENCE:
          {
            break;
          }
        case EK_LEPTON_KEY_REFERENCE:
          {
            break;
          }
        case EK_QUARK_REFERENCE:
          {
            break;
          }
        case EK_LOCK_REFERENCE:
          {
            break;
          }
        case EK_USE_REFERENCE:
          {
            break;
          }
        case EK_LOOKUP:
          {
            size_t count;
            expression **child_expressions;
            expression **upper_bounds;
            type_expression **filters;
            size_t num;

            delete_expression(the_expression->u.lookup.base);

            count = the_expression->u.lookup.child_expressions.element_count;
            assert(count ==
                   the_expression->u.lookup.upper_bounds.element_count);
            assert(count == the_expression->u.lookup.filters.element_count);

            child_expressions =
                    the_expression->u.lookup.child_expressions.array;
            upper_bounds = the_expression->u.lookup.upper_bounds.array;
            filters = the_expression->u.lookup.filters.array;

            assert(child_expressions != NULL);
            assert(upper_bounds != NULL);
            assert(filters != NULL);

            for (num = 0; num < count; ++num)
              {
                if (child_expressions[num] != NULL)
                    delete_expression(child_expressions[num]);
                if (upper_bounds[num] != NULL)
                    delete_expression(upper_bounds[num]);
                if (filters[num] != NULL)
                    delete_type_expression(filters[num]);
              }

            free(child_expressions);
            free(upper_bounds);
            free(filters);

            break;
          }
        case EK_LEPTON:
          {
            size_t count;
            char **labels;
            expression **child_expressions;
            boolean *forces;
            size_t num;

            delete_expression(the_expression->u.lepton.base);

            count = the_expression->u.lepton.labels.element_count;
            assert(count ==
                   the_expression->u.lepton.child_expressions.element_count);
            assert(count == the_expression->u.lepton.forces.element_count);

            labels = the_expression->u.lepton.labels.array;
            child_expressions =
                    the_expression->u.lepton.child_expressions.array;
            forces = the_expression->u.lepton.forces.array;

            assert(labels != NULL);
            assert(child_expressions != NULL);
            assert(forces != NULL);

            for (num = 0; num < count; ++num)
              {
                if (labels[num] != NULL)
                    free(labels[num]);
                if (child_expressions[num] != NULL)
                    delete_expression(child_expressions[num]);
              }

            free(labels);
            free(child_expressions);
            free(forces);

            break;
          }
        case EK_FIELD:
        case EK_POINTER_FIELD:
          {
            delete_expression(the_expression->u.field.base);
            free(the_expression->u.field.field_name);
            break;
          }
        case EK_TAGALONG_FIELD:
          {
            delete_expression(the_expression->u.tagalong_field.base);
            delete_expression(the_expression->u.tagalong_field.key);
            break;
          }
        case EK_STATEMENT_BLOCK:
          {
            delete_statement_block(the_expression->u.statement_block);
            break;
          }
        case EK_DECLARATION:
          {
            declaration_remove_reference(the_expression->u.declaration);
            break;
          }
        case EK_TYPE:
          {
            delete_type_expression(the_expression->u.type);
            break;
          }
        case EK_MAP_LIST:
          {
            size_t component_count;
            value_or_type_expression *keys;
            expression **targets;
            size_t component_num;

            component_count = the_expression->u.map_list.keys.element_count;
            assert(component_count ==
                   the_expression->u.map_list.targets.element_count);

            keys = the_expression->u.map_list.keys.array;
            targets = the_expression->u.map_list.targets.array;

            assert(keys != NULL);
            assert(targets != NULL);

            for (component_num = 0; component_num < component_count;
                 ++component_num)
              {
                if (keys[component_num].is_value)
                  {
                    delete_expression(keys[component_num].u.value_expression);
                  }
                else
                  {
                    delete_type_expression(
                            keys[component_num].u.type_expression);
                  }
                delete_expression(targets[component_num]);
              }

            free(keys);
            free(targets);

            break;
          }
        case EK_SEMI_LABELED_EXPRESSION_LIST:
          {
            size_t count;
            char **labels;
            expression **child_expressions;
            size_t num;

            count = the_expression->u.semi_labeled_expression_list.labels.
                    element_count;
            assert(count ==
                   the_expression->u.semi_labeled_expression_list.
                           child_expressions.element_count);

            labels = the_expression->u.semi_labeled_expression_list.labels.
                    array;
            child_expressions = the_expression->u.semi_labeled_expression_list.
                    child_expressions.array;

            assert(labels != NULL);
            assert(child_expressions != NULL);

            for (num = 0; num < count; ++num)
              {
                if (labels[num] != NULL)
                    free(labels[num]);
                if (child_expressions[num] != NULL)
                    delete_expression(child_expressions[num]);
              }

            free(labels);
            free(child_expressions);

            break;
          }
        case EK_CALL:
          {
            delete_call(the_expression->u.call);
            break;
          }
        case EK_CONDITIONAL:
          {
            delete_expression(the_expression->u.conditional.test);
            delete_expression(the_expression->u.conditional.then_part);
            delete_expression(the_expression->u.conditional.else_part);
            break;
          }
        case EK_DEREFERENCE:
        case EK_LOCATION_OF:
        case EK_NEGATE:
        case EK_UNARY_PLUS:
        case EK_BITWISE_NOT:
        case EK_LOGICAL_NOT:
          {
            delete_expression(the_expression->u.unary.operand);
            break;
          }
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
          {
            delete_expression(the_expression->u.binary.operand1);
            delete_expression(the_expression->u.binary.operand2);
            break;
          }
        case EK_ARGUMENTS:
          {
            break;
          }
        case EK_THIS:
          {
            break;
          }
        case EK_IN:
          {
            delete_expression(the_expression->u.in.expression);
            delete_type_expression(the_expression->u.in.type);
            break;
          }
        case EK_FORCE:
          {
            delete_expression(the_expression->u.force.expression);
            delete_type_expression(the_expression->u.force.type);
            break;
          }
        case EK_BREAK:
          {
            break;
          }
        case EK_CONTINUE:
          {
            break;
          }
        case EK_COMPREHEND:
          {
            declaration_remove_reference(
                    the_expression->u.comprehend.element_declaration);
            delete_expression(the_expression->u.comprehend.base);
            if (the_expression->u.comprehend.filter != NULL)
                delete_expression(the_expression->u.comprehend.filter);
            delete_expression(the_expression->u.comprehend.body);
            break;
          }
        case EK_FORALL:
          {
            delete_formal_arguments(the_expression->u.forall.formals);
            delete_expression(the_expression->u.forall.body);
            break;
          }
        case EK_EXISTS:
          {
            delete_formal_arguments(the_expression->u.exists.formals);
            delete_expression(the_expression->u.exists.body);
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    source_location_remove_reference(&(the_expression->location));

    free(the_expression);
  }

extern void delete_call_expression_save_call(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_CALL);

    source_location_remove_reference(&(the_expression->location));

    free(the_expression);
  }

extern expression_kind get_expression_kind(expression *the_expression)
  {
    assert(the_expression != NULL);

    return the_expression->kind;
  }

extern routine_declaration_chain *expression_overload_chain(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    return the_expression->overload_chain;
  }

extern statement *expression_overload_use_statement(expression *the_expression)
  {
    assert(the_expression != NULL);

    return the_expression->overload_use_statement;
  }

extern size_t expression_overload_use_used_for_number(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    return the_expression->overload_used_for_number;
  }

extern value *constant_expression_value(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_CONSTANT);
    assert(the_expression->u.constant_value != NULL);
    return the_expression->u.constant_value;
  }

extern boolean unbound_name_reference_expression_addressable_required(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_UNBOUND_NAME_REFERENCE);
    return the_expression->u.unbound_addressable_required;
  }

extern variable_declaration *variable_reference_expression_declaration(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_VARIABLE_REFERENCE);
    return the_expression->u.variable_declaration;
  }

extern routine_declaration_chain *routine_reference_expression_chain(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_ROUTINE_REFERENCE);
    return the_expression->u.routine_declaration_chain;
  }

extern statement *label_reference_expression_declaration(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LABEL_REFERENCE);
    return the_expression->u.label_statement;
  }

extern tagalong_declaration *tagalong_reference_expression_declaration(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_TAGALONG_REFERENCE);
    return the_expression->u.tagalong_declaration;
  }

extern lepton_key_declaration *lepton_key_reference_expression_declaration(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LEPTON_KEY_REFERENCE);
    return the_expression->u.lepton_key_declaration;
  }

extern quark_declaration *quark_reference_expression_declaration(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_QUARK_REFERENCE);
    return the_expression->u.quark_declaration;
  }

extern lock_declaration *lock_reference_expression_declaration(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LOCK_REFERENCE);
    return the_expression->u.lock_declaration;
  }

extern statement *use_reference_expression_use_statement(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_USE_REFERENCE);
    return the_expression->u.use_reference.use_statement;
  }

extern size_t use_reference_expression_used_for_num(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_USE_REFERENCE);
    return the_expression->u.use_reference.used_for_num;
  }

extern expression *lookup_expression_base(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LOOKUP);
    return the_expression->u.lookup.base;
  }

extern size_t lookup_expression_component_count(expression *the_expression)
  {
    size_t count;

    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LOOKUP);

    count = the_expression->u.lookup.child_expressions.element_count;
    assert(count == the_expression->u.lookup.upper_bounds.element_count);
    assert(count == the_expression->u.lookup.filters.element_count);

    return count;
  }

extern expression *lookup_expression_component_child_expression(
        expression *the_expression, size_t component_num)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LOOKUP);
    assert(component_num < lookup_expression_component_count(the_expression));
    return the_expression->u.lookup.child_expressions.array[component_num];
  }

extern expression *lookup_expression_component_upper_bound(
        expression *the_expression, size_t component_num)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LOOKUP);
    assert(component_num < lookup_expression_component_count(the_expression));
    return the_expression->u.lookup.upper_bounds.array[component_num];
  }

extern type_expression *lookup_expression_component_filter(
        expression *the_expression, size_t component_num)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LOOKUP);
    assert(component_num < lookup_expression_component_count(the_expression));
    return the_expression->u.lookup.filters.array[component_num];
  }

extern expression *lepton_expression_base(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LEPTON);
    return the_expression->u.lepton.base;
  }

extern size_t lepton_expression_component_count(expression *the_expression)
  {
    size_t count;

    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LEPTON);

    count = the_expression->u.lepton.labels.element_count;
    assert(count == the_expression->u.lepton.child_expressions.element_count);
    assert(count == the_expression->u.lepton.forces.element_count);

    return count;
  }

extern const char *lepton_expression_component_label(
        expression *the_expression, size_t component_num)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LEPTON);
    assert(component_num < lepton_expression_component_count(the_expression));
    return the_expression->u.lepton.labels.array[component_num];
  }

extern expression *lepton_expression_component_child_expression(
        expression *the_expression, size_t component_num)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LEPTON);
    assert(component_num < lepton_expression_component_count(the_expression));
    return the_expression->u.lepton.child_expressions.array[component_num];
  }

extern boolean lepton_expression_component_force(expression *the_expression,
                                                 size_t component_num)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LEPTON);
    assert(component_num < lepton_expression_component_count(the_expression));
    return the_expression->u.lepton.forces.array[component_num];
  }

extern expression *field_expression_base(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_FIELD);
    return the_expression->u.field.base;
  }

extern const char *field_expression_field_name(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_FIELD);
    return the_expression->u.field.field_name;
  }

extern expression *pointer_field_expression_base(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_POINTER_FIELD);
    return the_expression->u.field.base;
  }

extern const char *pointer_field_expression_field_name(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_POINTER_FIELD);
    return the_expression->u.field.field_name;
  }

extern expression *tagalong_field_expression_base(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_TAGALONG_FIELD);
    return the_expression->u.tagalong_field.base;
  }

extern expression *tagalong_field_expression_key(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_TAGALONG_FIELD);
    return the_expression->u.tagalong_field.key;
  }

extern statement_block *statement_block_expression_block(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_STATEMENT_BLOCK);
    return the_expression->u.statement_block;
  }

extern declaration *declaration_expression_declaration(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_DECLARATION);
    return the_expression->u.declaration;
  }

extern type_expression *type_expression_type(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_TYPE);
    return the_expression->u.type;
  }

extern size_t map_list_expression_component_count(expression *the_expression)
  {
    size_t count;

    assert(the_expression != NULL);

    assert(the_expression->kind == EK_MAP_LIST);

    count = the_expression->u.map_list.keys.element_count;
    assert(count == the_expression->u.map_list.targets.element_count);

    return count;
  }

extern boolean map_list_expression_is_filter(expression *the_expression,
                                             size_t component_num)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_MAP_LIST);
    assert(component_num <
           map_list_expression_component_count(the_expression));
    return !(the_expression->u.map_list.keys.array[component_num].is_value);
  }

extern expression *map_list_expression_key(expression *the_expression,
                                           size_t component_num)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_MAP_LIST);
    assert(component_num <
           map_list_expression_component_count(the_expression));
    return the_expression->u.map_list.keys.array[component_num].u.
            value_expression;
  }

extern type_expression *map_list_expression_filter(expression *the_expression,
                                                   size_t component_num)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_MAP_LIST);
    assert(component_num <
           map_list_expression_component_count(the_expression));
    return the_expression->u.map_list.keys.array[component_num].u.
            type_expression;
  }

extern expression *map_list_expression_target(expression *the_expression,
                                              size_t component_num)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_MAP_LIST);
    assert(component_num <
           map_list_expression_component_count(the_expression));
    return the_expression->u.map_list.targets.array[component_num];
  }

extern size_t semi_labeled_expression_list_expression_component_count(
        expression *the_expression)
  {
    size_t count;

    assert(the_expression != NULL);

    assert(the_expression->kind == EK_SEMI_LABELED_EXPRESSION_LIST);

    count = the_expression->u.semi_labeled_expression_list.labels.
            element_count;
    assert(count ==
           the_expression->u.semi_labeled_expression_list.child_expressions.
                   element_count);

    return count;
  }

extern const char *semi_labeled_expression_list_expression_label(
        expression *the_expression, size_t component_num)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_SEMI_LABELED_EXPRESSION_LIST);
    assert(component_num <
           semi_labeled_expression_list_expression_component_count(
                   the_expression));
    return the_expression->u.semi_labeled_expression_list.labels.array[
            component_num];
  }

extern expression *semi_labeled_expression_list_expression_child_expression(
        expression *the_expression, size_t component_num)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_SEMI_LABELED_EXPRESSION_LIST);
    assert(component_num <
           semi_labeled_expression_list_expression_component_count(
                   the_expression));
    return the_expression->u.semi_labeled_expression_list.child_expressions.
            array[component_num];
  }

extern call *call_expression_call(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_CALL);
    return the_expression->u.call;
  }

extern expression *conditional_expression_test(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_CONDITIONAL);
    return the_expression->u.conditional.test;
  }

extern expression *conditional_expression_then_part(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_CONDITIONAL);
    return the_expression->u.conditional.then_part;
  }

extern expression *conditional_expression_else_part(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_CONDITIONAL);
    return the_expression->u.conditional.else_part;
  }

extern expression *unary_expression_operand(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(is_unary_kind(the_expression->kind));
    return the_expression->u.unary.operand;
  }

extern expression *binary_expression_operand1(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(is_binary_kind(the_expression->kind));
    return the_expression->u.binary.operand1;
  }

extern expression *binary_expression_operand2(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(is_binary_kind(the_expression->kind));
    return the_expression->u.binary.operand2;
  }

extern routine_declaration *arguments_expression_routine(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_ARGUMENTS);
    return the_expression->u.arguments.declaration;
  }

extern routine_declaration *this_expression_class(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_THIS);
    return the_expression->u.this.declaration;
  }

extern expression *in_expression_expression(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_IN);
    return the_expression->u.in.expression;
  }

extern type_expression *in_expression_type(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_IN);
    return the_expression->u.in.type;
  }

extern expression *force_expression_expression(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_FORCE);
    return the_expression->u.force.expression;
  }

extern type_expression *force_expression_type(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_FORCE);
    return the_expression->u.force.type;
  }

extern void *break_expression_from(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_BREAK);
    return the_expression->u.break_from;
  }

extern void *continue_expression_with(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_CONTINUE);
    return the_expression->u.continue_with;
  }

extern variable_declaration *comprehend_expression_element(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_COMPREHEND);
    return the_expression->u.comprehend.element_variable;
  }

extern expression *comprehend_expression_base(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_COMPREHEND);
    return the_expression->u.comprehend.base;
  }

extern expression *comprehend_expression_filter(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_COMPREHEND);
    return the_expression->u.comprehend.filter;
  }

extern expression *comprehend_expression_body(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_COMPREHEND);
    return the_expression->u.comprehend.body;
  }

extern formal_arguments *forall_expression_formals(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_FORALL);
    return the_expression->u.forall.formals;
  }

extern expression *forall_expression_body(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_FORALL);
    return the_expression->u.forall.body;
  }

extern formal_arguments *exists_expression_formals(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_EXISTS);
    return the_expression->u.exists.formals;
  }

extern expression *exists_expression_body(expression *the_expression)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_EXISTS);
    return the_expression->u.exists.body;
  }

extern boolean expression_is_addressable(expression *the_expression)
  {
    assert(the_expression != NULL);

    switch (the_expression->kind)
      {
        case EK_CONSTANT:
            return FALSE;
        case EK_UNBOUND_NAME_REFERENCE:
            return TRUE;
        case EK_VARIABLE_REFERENCE:
            return TRUE;
        case EK_ROUTINE_REFERENCE:
            return FALSE;
        case EK_LABEL_REFERENCE:
            return FALSE;
        case EK_TAGALONG_REFERENCE:
            return FALSE;
        case EK_LEPTON_KEY_REFERENCE:
            return FALSE;
        case EK_QUARK_REFERENCE:
            return FALSE;
        case EK_LOCK_REFERENCE:
            return FALSE;
        case EK_USE_REFERENCE:
            return TRUE;
        case EK_LOOKUP:
            return expression_is_addressable(the_expression->u.lookup.base);
        case EK_LEPTON:
            return FALSE;
        case EK_FIELD:
            return expression_is_addressable(the_expression->u.field.base);
        case EK_POINTER_FIELD:
            return TRUE;
        case EK_TAGALONG_FIELD:
            return expression_is_addressable(
                    the_expression->u.tagalong_field.base);
        case EK_STATEMENT_BLOCK:
            return FALSE;
        case EK_DECLARATION:
            return FALSE;
        case EK_TYPE:
            return FALSE;
        case EK_MAP_LIST:
            return FALSE;
        case EK_SEMI_LABELED_EXPRESSION_LIST:
            return FALSE;
        case EK_CALL:
            return FALSE;
        case EK_CONDITIONAL:
            return FALSE;
        case EK_DEREFERENCE:
            return TRUE;
        case EK_LOCATION_OF:
            return FALSE;
        case EK_NEGATE:
            return FALSE;
        case EK_UNARY_PLUS:
            return FALSE;
        case EK_BITWISE_NOT:
            return FALSE;
        case EK_LOGICAL_NOT:
            return FALSE;
        case EK_ADD:
            return FALSE;
        case EK_SUBTRACT:
            return FALSE;
        case EK_MULTIPLY:
            return FALSE;
        case EK_DIVIDE:
            return FALSE;
        case EK_DIVIDE_FORCE:
            return FALSE;
        case EK_REMAINDER:
            return FALSE;
        case EK_SHIFT_LEFT:
            return FALSE;
        case EK_SHIFT_RIGHT:
            return FALSE;
        case EK_LESS_THAN:
            return FALSE;
        case EK_GREATER_THAN:
            return FALSE;
        case EK_LESS_THAN_OR_EQUAL:
            return FALSE;
        case EK_GREATER_THAN_OR_EQUAL:
            return FALSE;
        case EK_EQUAL:
            return FALSE;
        case EK_NOT_EQUAL:
            return FALSE;
        case EK_BITWISE_AND:
            return FALSE;
        case EK_BITWISE_OR:
            return FALSE;
        case EK_BITWISE_XOR:
            return FALSE;
        case EK_LOGICAL_AND:
            return FALSE;
        case EK_LOGICAL_OR:
            return FALSE;
        case EK_CONCATENATE:
            return FALSE;
        case EK_ARGUMENTS:
            return FALSE;
        case EK_THIS:
            return FALSE;
        case EK_IN:
            return FALSE;
        case EK_FORCE:
            return FALSE;
        case EK_BREAK:
            return FALSE;
        case EK_CONTINUE:
            return FALSE;
        case EK_COMPREHEND:
            return FALSE;
        case EK_FORALL:
            return FALSE;
        case EK_EXISTS:
            return FALSE;
        default:
            assert(FALSE);
            return FALSE;
      }
  }

extern void set_expression_start_location(expression *the_expression,
                                          const source_location *location)
  {
    assert(the_expression != NULL);

    set_location_start(&(the_expression->location), location);
  }

extern void set_expression_end_location(expression *the_expression,
                                        const source_location *location)
  {
    assert(the_expression != NULL);

    set_location_end(&(the_expression->location), location);
  }

extern void expression_set_overload_chain(expression *the_expression,
        routine_declaration_chain *overload_chain)
  {
    assert(the_expression != NULL);

    assert(the_expression->overload_chain == NULL);
    assert(the_expression->overload_use_statement == NULL);
    routine_declaration_chain_add_reference(overload_chain);
    the_expression->overload_chain = overload_chain;
  }

extern void expression_set_overload_use(expression *the_expression,
        statement *overload_use_statement, size_t overload_used_for_number)
  {
    assert(the_expression != NULL);

    assert(the_expression->overload_chain == NULL);
    assert(the_expression->overload_use_statement == NULL);
    the_expression->overload_use_statement = overload_use_statement;
    the_expression->overload_used_for_number = overload_used_for_number;
  }

extern void set_expression_addressable_required(expression *the_expression)
  {
    assert(the_expression != NULL);

    switch (the_expression->kind)
      {
        case EK_CONSTANT:
            assert(FALSE);
        case EK_UNBOUND_NAME_REFERENCE:
            the_expression->u.unbound_addressable_required = TRUE;
            break;
        case EK_VARIABLE_REFERENCE:
            break;
        case EK_ROUTINE_REFERENCE:
        case EK_LABEL_REFERENCE:
        case EK_TAGALONG_REFERENCE:
        case EK_LEPTON_KEY_REFERENCE:
        case EK_QUARK_REFERENCE:
        case EK_LOCK_REFERENCE:
            assert(FALSE);
        case EK_USE_REFERENCE:
            the_expression->u.use_reference.addressable_required = TRUE;
            break;
        case EK_LOOKUP:
            set_expression_addressable_required(the_expression->u.lookup.base);
            break;
        case EK_LEPTON:
            assert(FALSE);
        case EK_FIELD:
            set_expression_addressable_required(the_expression->u.field.base);
            break;
        case EK_POINTER_FIELD:
            break;
        case EK_TAGALONG_FIELD:
            set_expression_addressable_required(
                    the_expression->u.tagalong_field.base);
            break;
        case EK_STATEMENT_BLOCK:
        case EK_DECLARATION:
        case EK_TYPE:
        case EK_MAP_LIST:
        case EK_SEMI_LABELED_EXPRESSION_LIST:
        case EK_CALL:
        case EK_CONDITIONAL:
            assert(FALSE);
        case EK_DEREFERENCE:
            break;
        case EK_LOCATION_OF:
        case EK_NEGATE:
        case EK_UNARY_PLUS:
        case EK_BITWISE_NOT:
        case EK_LOGICAL_NOT:
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
        case EK_ARGUMENTS:
        case EK_THIS:
        case EK_IN:
        case EK_FORCE:
        case EK_BREAK:
        case EK_CONTINUE:
        case EK_COMPREHEND:
        case EK_FORALL:
        case EK_EXISTS:
            assert(FALSE);
        default:
            assert(FALSE);
      }
  }

extern void bind_expression_to_variable_declaration(expression *the_expression,
        variable_declaration *declaration)
  {
    assert(the_expression != NULL);
    assert(declaration != NULL);

    assert(the_expression->kind == EK_UNBOUND_NAME_REFERENCE);
    the_expression->kind = EK_VARIABLE_REFERENCE;
    the_expression->u.variable_declaration = declaration;
  }

extern void bind_expression_to_routine_declaration_chain(
        expression *the_expression, routine_declaration_chain *chain)
  {
    assert(the_expression != NULL);
    assert(chain != NULL);

    assert(the_expression->kind == EK_UNBOUND_NAME_REFERENCE);
    assert(!(the_expression->u.unbound_addressable_required));
    the_expression->kind = EK_ROUTINE_REFERENCE;
    routine_declaration_chain_add_reference(chain);
    the_expression->u.routine_declaration_chain = chain;
  }

extern void bind_expression_to_tagalong_declaration(expression *the_expression,
        tagalong_declaration *declaration)
  {
    assert(the_expression != NULL);
    assert(declaration != NULL);

    assert(the_expression->kind == EK_UNBOUND_NAME_REFERENCE);
    assert(!(the_expression->u.unbound_addressable_required));
    the_expression->kind = EK_TAGALONG_REFERENCE;
    the_expression->u.tagalong_declaration = declaration;
  }

extern void bind_expression_to_lepton_key_declaration(
        expression *the_expression, lepton_key_declaration *declaration)
  {
    assert(the_expression != NULL);
    assert(declaration != NULL);

    assert(the_expression->kind == EK_UNBOUND_NAME_REFERENCE);
    assert(!(the_expression->u.unbound_addressable_required));
    the_expression->kind = EK_LEPTON_KEY_REFERENCE;
    the_expression->u.lepton_key_declaration = declaration;
  }

extern void bind_expression_to_quark_declaration(expression *the_expression,
        quark_declaration *declaration)
  {
    assert(the_expression != NULL);
    assert(declaration != NULL);

    assert(the_expression->kind == EK_UNBOUND_NAME_REFERENCE);
    assert(!(the_expression->u.unbound_addressable_required));
    the_expression->kind = EK_QUARK_REFERENCE;
    the_expression->u.quark_declaration = declaration;
  }

extern void bind_expression_to_lock_declaration(expression *the_expression,
                                                lock_declaration *declaration)
  {
    assert(the_expression != NULL);
    assert(declaration != NULL);

    assert(the_expression->kind == EK_UNBOUND_NAME_REFERENCE);
    assert(!(the_expression->u.unbound_addressable_required));
    the_expression->kind = EK_LOCK_REFERENCE;
    the_expression->u.lock_declaration = declaration;
  }

extern void bind_expression_to_label_declaration(expression *the_expression,
                                                 statement *declaration)
  {
    assert(the_expression != NULL);

    assert(the_expression->kind == EK_UNBOUND_NAME_REFERENCE);
    assert(!(the_expression->u.unbound_addressable_required));
    the_expression->kind = EK_LABEL_REFERENCE;
    the_expression->u.label_statement = declaration;
  }

extern void bind_expression_to_use_statement(expression *the_expression,
        statement *use_statement, size_t used_for_num)
  {
    boolean addressable_required;

    assert(the_expression != NULL);
    assert(use_statement != NULL);

    assert(the_expression->kind == EK_UNBOUND_NAME_REFERENCE);
    addressable_required = the_expression->u.unbound_addressable_required;
    the_expression->kind = EK_USE_REFERENCE;
    the_expression->u.use_reference.use_statement = use_statement;
    the_expression->u.use_reference.used_for_num = used_for_num;
    the_expression->u.use_reference.addressable_required =
            addressable_required;
  }

extern void bind_arguments_expression_to_routine_declaration(
        expression *arguments_expression, routine_declaration *declaration)
  {
    assert(arguments_expression != NULL);

    assert(arguments_expression->kind == EK_ARGUMENTS);
    arguments_expression->u.arguments.declaration = declaration;
  }

extern void bind_this_expression_to_routine_declaration(
        expression *this_expression, routine_declaration *declaration)
  {
    assert(this_expression != NULL);

    assert(this_expression->kind == EK_THIS);
    this_expression->u.this.declaration = declaration;
  }

extern void bind_break_expression_from(expression *break_expression,
                                       void *from)
  {
    assert(break_expression != NULL);

    assert(break_expression->kind == EK_BREAK);
    break_expression->u.break_from = from;
  }

extern void bind_continue_expression_with(expression *continue_expression,
                                          void *with)
  {
    assert(continue_expression != NULL);

    assert(continue_expression->kind == EK_CONTINUE);
    continue_expression->u.continue_with = with;
  }

extern verdict add_lookup_component(expression *the_expression,
        expression *child_expression, expression *upper_bound)
  {
    verdict the_verdict;

    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LOOKUP);

    the_verdict = expression_aa_append(
            &(the_expression->u.lookup.child_expressions), child_expression);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (child_expression != NULL)
            delete_expression(child_expression);
        if (upper_bound != NULL)
            delete_expression(upper_bound);
        return the_verdict;
      }

    the_verdict = expression_aa_append(
            &(the_expression->u.lookup.upper_bounds), upper_bound);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(the_expression->u.lookup.child_expressions.element_count > 0);
        --(the_expression->u.lookup.child_expressions.element_count);
        if (child_expression != NULL)
            delete_expression(child_expression);
        if (upper_bound != NULL)
            delete_expression(upper_bound);
        return the_verdict;
      }

    the_verdict = type_expression_aa_append(
            &(the_expression->u.lookup.filters), NULL);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(the_expression->u.lookup.upper_bounds.element_count > 0);
        --(the_expression->u.lookup.upper_bounds.element_count);
        assert(the_expression->u.lookup.child_expressions.element_count > 0);
        --(the_expression->u.lookup.child_expressions.element_count);
        if (child_expression != NULL)
            delete_expression(child_expression);
        if (upper_bound != NULL)
            delete_expression(upper_bound);
        return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict add_lookup_filter_component(expression *the_expression,
                                           type_expression *filter)
  {
    verdict the_verdict;

    assert(the_expression != NULL);

    assert(the_expression->kind == EK_LOOKUP);

    the_verdict = expression_aa_append(
            &(the_expression->u.lookup.child_expressions), NULL);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (filter != NULL)
            delete_type_expression(filter);
        return the_verdict;
      }

    the_verdict = expression_aa_append(
            &(the_expression->u.lookup.upper_bounds), NULL);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(the_expression->u.lookup.child_expressions.element_count > 0);
        --(the_expression->u.lookup.child_expressions.element_count);
        if (filter != NULL)
            delete_type_expression(filter);
        return the_verdict;
      }

    the_verdict = type_expression_aa_append(
            &(the_expression->u.lookup.filters), filter);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(the_expression->u.lookup.upper_bounds.element_count > 0);
        --(the_expression->u.lookup.upper_bounds.element_count);
        assert(the_expression->u.lookup.child_expressions.element_count > 0);
        --(the_expression->u.lookup.child_expressions.element_count);
        if (filter != NULL)
            delete_type_expression(filter);
        return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict add_lepton_component(expression *the_expression,
        const char *label, expression *child_expression, boolean force)
  {
    verdict the_verdict;
    char *copy;

    assert(the_expression != NULL);
    assert(label != NULL);

    assert(the_expression->kind == EK_LEPTON);

    copy = MALLOC_ARRAY(char, strlen(label) + 1);
    if (copy == NULL)
      {
        if (child_expression != NULL)
            delete_expression(child_expression);
        return MISSION_FAILED;
      }
    strcpy(copy, label);

    the_verdict = mstring_aa_append(&(the_expression->u.lepton.labels), copy);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (copy != NULL)
            free(copy);
        if (child_expression != NULL)
            delete_expression(child_expression);
        return the_verdict;
      }

    the_verdict = expression_aa_append(
            &(the_expression->u.lepton.child_expressions), child_expression);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(the_expression->u.lepton.labels.element_count > 0);
        --(the_expression->u.lepton.labels.element_count);
        if (copy != NULL)
            free(copy);
        if (child_expression != NULL)
            delete_expression(child_expression);
        return the_verdict;
      }

    the_verdict = boolean_aa_append(&(the_expression->u.lepton.forces), force);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(the_expression->u.lepton.child_expressions.element_count > 0);
        --(the_expression->u.lepton.child_expressions.element_count);
        assert(the_expression->u.lepton.labels.element_count > 0);
        --(the_expression->u.lepton.labels.element_count);
        if (copy != NULL)
            free(copy);
        if (child_expression != NULL)
            delete_expression(child_expression);
        return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict add_map_list_expression_component(expression *base,
        expression *key, expression *target)
  {
    value_or_type_expression new_key_block;
    verdict the_verdict;

    assert(base != NULL);
    assert(key != NULL);
    assert(target != NULL);

    assert(base->kind == EK_MAP_LIST);

    new_key_block.is_value = TRUE;
    new_key_block.u.value_expression = key;
    the_verdict = value_or_type_expression_aa_append(&(base->u.map_list.keys),
                                                     new_key_block);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_expression(key);
        delete_expression(target);
        return the_verdict;
      }

    the_verdict = expression_aa_append(&(base->u.map_list.targets), target);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(base->u.map_list.keys.element_count > 0);
        --(base->u.map_list.keys.element_count);
        delete_expression(key);
        delete_expression(target);
        return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict add_map_list_expression_filter_component(expression *base,
        type_expression *filter, expression *target)
  {
    value_or_type_expression new_key_block;
    verdict the_verdict;

    assert(base != NULL);
    assert(filter != NULL);
    assert(target != NULL);

    assert(base->kind == EK_MAP_LIST);

    new_key_block.is_value = FALSE;
    new_key_block.u.type_expression = filter;
    the_verdict = value_or_type_expression_aa_append(&(base->u.map_list.keys),
                                                     new_key_block);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_type_expression(filter);
        delete_expression(target);
        return the_verdict;
      }

    the_verdict = expression_aa_append(&(base->u.map_list.targets), target);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(base->u.map_list.keys.element_count > 0);
        --(base->u.map_list.keys.element_count);
        delete_type_expression(filter);
        delete_expression(target);
        return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

extern verdict add_semi_labeled_expression_list_expression_component(
        expression *base, const char *label, expression *child_expression)
  {
    verdict the_verdict;
    char *copy;

    assert(base != NULL);

    assert(base->kind == EK_SEMI_LABELED_EXPRESSION_LIST);

    if (label == NULL)
      {
        copy = NULL;
      }
    else
      {
        copy = MALLOC_ARRAY(char, strlen(label) + 1);
        if (copy == NULL)
          {
            if (child_expression != NULL)
                delete_expression(child_expression);
            return MISSION_FAILED;
          }
        strcpy(copy, label);
      }

    the_verdict = mstring_aa_append(
            &(base->u.semi_labeled_expression_list.labels), copy);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (copy != NULL)
            free(copy);
        if (child_expression != NULL)
            delete_expression(child_expression);
        return the_verdict;
      }

    the_verdict = expression_aa_append(
            &(base->u.semi_labeled_expression_list.child_expressions),
            child_expression);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        assert(base->u.semi_labeled_expression_list.labels.element_count > 0);
        --(base->u.semi_labeled_expression_list.labels.element_count);
        if (copy != NULL)
            free(copy);
        if (child_expression != NULL)
            delete_expression(child_expression);
        return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

extern const source_location *get_expression_location(
        expression *the_expression)
  {
    assert(the_expression != NULL);

    return &(the_expression->location);
  }

extern void expression_error(expression *the_expression, const char *format,
                             ...)
  {
    va_list ap;

    assert(the_expression != NULL);
    assert(format != NULL);

    va_start(ap, format);
    vexpression_error(the_expression, format, ap);
    va_end(ap);
  }

extern void vexpression_error(expression *the_expression, const char *format,
                              va_list arg)
  {
    assert(the_expression != NULL);
    assert(format != NULL);

    vlocation_error(&(the_expression->location), format, arg);
  }

extern const char *expression_kind_operator_name(expression_kind kind)
  {
    switch (kind)
      {
        case EK_ADD:
            return "operator+";
        case EK_SUBTRACT:
            return "operator-";
        case EK_MULTIPLY:
            return "operator*";
        case EK_DIVIDE:
            return "operator/";
        case EK_DIVIDE_FORCE:
            return "operator/::";
        case EK_REMAINDER:
            return "operator%";
        case EK_SHIFT_LEFT:
            return "operator<<";
        case EK_SHIFT_RIGHT:
            return "operator>>";
        case EK_LESS_THAN:
            return "operator<";
        case EK_GREATER_THAN:
            return "operator>";
        case EK_LESS_THAN_OR_EQUAL:
            return "operator<=";
        case EK_GREATER_THAN_OR_EQUAL:
            return "operator>=";
        case EK_EQUAL:
            return "operator==";
        case EK_NOT_EQUAL:
            return "operator!=";
        case EK_BITWISE_AND:
            return "operator&";
        case EK_BITWISE_OR:
            return "operator|";
        case EK_BITWISE_XOR:
            return "operator^";
        case EK_CONCATENATE:
            return "operator~";
        case EK_DEREFERENCE:
            return "operator*";
        case EK_NEGATE:
            return "operator-";
        case EK_UNARY_PLUS:
            return "operator+";
        case EK_BITWISE_NOT:
            return "operator~";
        case EK_LOGICAL_NOT:
            return "operator!";
        default:
            assert(FALSE);
            return NULL;
      }
  }


static expression *create_empty_expression(void)
  {
    expression *result;

    result = MALLOC_ONE_OBJECT(expression);
    if (result == NULL)
        return NULL;

    result->overload_chain = NULL;
    result->overload_use_statement = NULL;
    result->overload_used_for_number = 0;
    set_source_location(&(result->location), NULL);

    return result;
  }

static boolean is_unary_kind(expression_kind kind)
  {
    switch (kind)
      {
        case EK_DEREFERENCE:
        case EK_LOCATION_OF:
        case EK_NEGATE:
        case EK_UNARY_PLUS:
        case EK_BITWISE_NOT:
        case EK_LOGICAL_NOT:
            return TRUE;
        default:
            return FALSE;
      }
  }

static boolean is_binary_kind(expression_kind kind)
  {
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
            return TRUE;
        default:
            return FALSE;
      }
  }
