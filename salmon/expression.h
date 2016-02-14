/* file "expression.h" */

/*
 *  This file contains the interface to the expression module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <stdarg.h>
#include "c_foundations/basic.h"


typedef enum expression_kind
  {
    EK_CONSTANT,
    EK_UNBOUND_NAME_REFERENCE,
    EK_VARIABLE_REFERENCE,
    EK_ROUTINE_REFERENCE,
    EK_LABEL_REFERENCE,
    EK_TAGALONG_REFERENCE,
    EK_LEPTON_KEY_REFERENCE,
    EK_QUARK_REFERENCE,
    EK_LOCK_REFERENCE,
    EK_USE_REFERENCE,
    EK_LOOKUP,
    EK_LEPTON,
    EK_FIELD,
    EK_POINTER_FIELD,
    EK_TAGALONG_FIELD,
    EK_STATEMENT_BLOCK,
    EK_DECLARATION,
    EK_TYPE,
    EK_MAP_LIST,
    EK_SEMI_LABELED_EXPRESSION_LIST,
    EK_CALL,
    EK_CONDITIONAL,

    /* Unary expressions. */
    EK_DEREFERENCE, /* *exp */
    EK_LOCATION_OF, /* &exp */
    EK_NEGATE,
    EK_UNARY_PLUS,
    EK_BITWISE_NOT,
    EK_LOGICAL_NOT,

    /* Binary expressions. */
    EK_ADD,
    EK_SUBTRACT,
    EK_MULTIPLY,
    EK_DIVIDE,
    EK_DIVIDE_FORCE,
    EK_REMAINDER,
    EK_SHIFT_LEFT,
    EK_SHIFT_RIGHT,
    EK_LESS_THAN,
    EK_GREATER_THAN,
    EK_LESS_THAN_OR_EQUAL,
    EK_GREATER_THAN_OR_EQUAL,
    EK_EQUAL,
    EK_NOT_EQUAL,
    EK_BITWISE_AND,
    EK_BITWISE_OR,
    EK_BITWISE_XOR,
    EK_LOGICAL_AND,
    EK_LOGICAL_OR,
    EK_CONCATENATE,

    EK_ARGUMENTS,
    EK_THIS,
    EK_IN,
    EK_FORCE,
    EK_BREAK,
    EK_CONTINUE,
    EK_COMPREHEND,
    EK_FORALL,
    EK_EXISTS
  } expression_kind;

typedef struct expression expression;


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
#include "formal_arguments.h"


extern expression *create_constant_expression(value *the_value);
extern expression *create_unbound_name_reference_expression(void);
extern expression *create_variable_reference_expression(
        variable_declaration *declaration);
extern expression *create_routine_reference_expression(
        routine_declaration_chain *chain);
extern expression *create_label_reference_expression(
        statement *label_statement);
extern expression *create_tagalong_reference_expression(
        tagalong_declaration *declaration);
extern expression *create_lepton_key_reference_expression(
        lepton_key_declaration *declaration);
extern expression *create_quark_reference_expression(
        quark_declaration *declaration);
extern expression *create_lock_reference_expression(
        lock_declaration *declaration);
extern expression *create_use_reference_expression(statement *use_statement,
                                                   size_t used_for_num);
extern expression *create_lookup_expression(expression *base);
extern expression *create_lepton_expression(expression *base);
extern expression *create_field_expression(expression *base,
                                           const char *field_name);
extern expression *create_pointer_field_expression(expression *base,
                                                   const char *field_name);
extern expression *create_tagalong_field_expression(expression *base,
                                                    expression *key);
extern expression *create_statement_block_expression(
        statement_block *the_statement_block);
extern expression *create_declaration_expression(declaration *the_declaration);
extern expression *create_type_expression_expression(type_expression *type);
extern expression *create_map_list_expression(void);
extern expression *create_semi_labeled_expression_list_expression(void);
extern expression *create_call_expression(call *the_call);
extern expression *create_conditional_expression(expression *test,
        expression *then_part, expression *else_part);
extern expression *create_unary_expression(expression_kind kind,
                                           expression *operand);
extern expression *create_binary_expression(expression_kind kind,
        expression *operand1, expression *operand2);
extern expression *create_arguments_expression(void);
extern expression *create_this_expression(void);
extern expression *create_in_expression(expression *the_expression,
                                        type_expression *type);
extern expression *create_force_expression(expression *the_expression,
                                           type_expression *type);
extern expression *create_break_expression(void);
extern expression *create_continue_expression(void);
extern expression *create_comprehend_expression(const char *element_name,
        expression *base, expression *filter, expression *body,
        const source_location *element_declaration_location);
extern expression *create_forall_expression(formal_arguments *formals,
                                            expression *body);
extern expression *create_exists_expression(formal_arguments *formals,
                                            expression *body);

extern void delete_expression(expression *the_expression);
extern void delete_call_expression_save_call(expression *the_expression);

extern expression_kind get_expression_kind(expression *the_expression);
extern routine_declaration_chain *expression_overload_chain(
        expression *the_expression);
extern statement *expression_overload_use_statement(
        expression *the_expression);
extern size_t expression_overload_use_used_for_number(
        expression *the_expression);
extern value *constant_expression_value(expression *the_expression);
extern boolean unbound_name_reference_expression_addressable_required(
        expression *the_expression);
extern variable_declaration *variable_reference_expression_declaration(
        expression *the_expression);
extern routine_declaration_chain *routine_reference_expression_chain(
        expression *the_expression);
extern statement *label_reference_expression_declaration(
        expression *the_expression);
extern tagalong_declaration *tagalong_reference_expression_declaration(
        expression *the_expression);
extern lepton_key_declaration *lepton_key_reference_expression_declaration(
        expression *the_expression);
extern quark_declaration *quark_reference_expression_declaration(
        expression *the_expression);
extern lock_declaration *lock_reference_expression_declaration(
        expression *the_expression);
extern statement *use_reference_expression_use_statement(
        expression *the_expression);
extern size_t use_reference_expression_used_for_num(
        expression *the_expression);
extern expression *lookup_expression_base(expression *the_expression);
extern size_t lookup_expression_component_count(expression *the_expression);
extern expression *lookup_expression_component_child_expression(
        expression *the_expression, size_t component_num);
extern expression *lookup_expression_component_upper_bound(
        expression *the_expression, size_t component_num);
extern type_expression *lookup_expression_component_filter(
        expression *the_expression, size_t component_num);
extern expression *lepton_expression_base(expression *the_expression);
extern size_t lepton_expression_component_count(expression *the_expression);
extern const char *lepton_expression_component_label(
        expression *the_expression, size_t component_num);
extern expression *lepton_expression_component_child_expression(
        expression *the_expression, size_t component_num);
extern boolean lepton_expression_component_force(expression *the_expression,
                                                 size_t component_num);
extern expression *field_expression_base(expression *the_expression);
extern const char *field_expression_field_name(expression *the_expression);
extern expression *pointer_field_expression_base(expression *the_expression);
extern const char *pointer_field_expression_field_name(
        expression *the_expression);
extern expression *tagalong_field_expression_base(expression *the_expression);
extern expression *tagalong_field_expression_key(expression *the_expression);
extern statement_block *statement_block_expression_block(
        expression *the_expression);
extern declaration *declaration_expression_declaration(
        expression *the_expression);
extern type_expression *type_expression_type(expression *the_expression);
extern size_t map_list_expression_component_count(expression *the_expression);
extern boolean map_list_expression_is_filter(expression *the_expression,
                                             size_t component_num);
extern expression *map_list_expression_key(expression *the_expression,
                                           size_t component_num);
extern type_expression *map_list_expression_filter(expression *the_expression,
                                                   size_t component_num);
extern expression *map_list_expression_target(expression *the_expression,
                                              size_t component_num);
extern size_t semi_labeled_expression_list_expression_component_count(
        expression *the_expression);
extern const char *semi_labeled_expression_list_expression_label(
        expression *the_expression, size_t component_num);
extern expression *semi_labeled_expression_list_expression_child_expression(
        expression *the_expression, size_t component_num);
extern call *call_expression_call(expression *the_expression);
extern expression *conditional_expression_test(expression *the_expression);
extern expression *conditional_expression_then_part(
        expression *the_expression);
extern expression *conditional_expression_else_part(
        expression *the_expression);
extern expression *unary_expression_operand(expression *the_expression);
extern expression *binary_expression_operand1(expression *the_expression);
extern expression *binary_expression_operand2(expression *the_expression);
extern routine_declaration *arguments_expression_routine(
        expression *the_expression);
extern routine_declaration *this_expression_class(expression *the_expression);
extern expression *in_expression_expression(expression *the_expression);
extern type_expression *in_expression_type(expression *the_expression);
extern expression *force_expression_expression(expression *the_expression);
extern type_expression *force_expression_type(expression *the_expression);
extern void *break_expression_from(expression *the_expression);
extern void *continue_expression_with(expression *the_expression);
extern variable_declaration *comprehend_expression_element(
        expression *the_expression);
extern expression *comprehend_expression_base(expression *the_expression);
extern expression *comprehend_expression_filter(expression *the_expression);
extern expression *comprehend_expression_body(expression *the_expression);
extern formal_arguments *forall_expression_formals(expression *the_expression);
extern expression *forall_expression_body(expression *the_expression);
extern formal_arguments *exists_expression_formals(expression *the_expression);
extern expression *exists_expression_body(expression *the_expression);

extern boolean expression_is_addressable(expression *the_expression);

extern void set_expression_start_location(expression *the_expression,
                                          const source_location *location);
extern void set_expression_end_location(expression *the_expression,
                                        const source_location *location);
extern void expression_set_overload_chain(expression *the_expression,
        routine_declaration_chain *overload_chain);
extern void expression_set_overload_use(expression *the_expression,
        statement *overload_use_statement, size_t overload_used_for_number);
extern void set_expression_addressable_required(expression *the_expression);

extern void bind_expression_to_variable_declaration(expression *the_expression,
        variable_declaration *declaration);
extern void bind_expression_to_routine_declaration_chain(
        expression *the_expression, routine_declaration_chain *chain);
extern void bind_expression_to_tagalong_declaration(expression *the_expression,
        tagalong_declaration *declaration);
extern void bind_expression_to_lepton_key_declaration(
        expression *the_expression, lepton_key_declaration *declaration);
extern void bind_expression_to_quark_declaration(expression *the_expression,
        quark_declaration *declaration);
extern void bind_expression_to_lock_declaration(expression *the_expression,
                                                lock_declaration *declaration);
extern void bind_expression_to_label_declaration(expression *the_expression,
                                                 statement *declaration);
extern void bind_expression_to_use_statement(expression *the_expression,
        statement *use_statement, size_t used_for_num);
extern void bind_arguments_expression_to_routine_declaration(
        expression *arguments_expression, routine_declaration *declaration);
extern void bind_this_expression_to_routine_declaration(
        expression *this_expression, routine_declaration *declaration);
extern void bind_break_expression_from(expression *break_expression,
                                       void *from);
extern void bind_continue_expression_with(expression *continue_expression,
                                          void *with);
extern verdict add_lookup_component(expression *the_expression,
        expression *child_expression, expression *upper_bound);
extern verdict add_lookup_filter_component(expression *the_expression,
                                           type_expression *filter);
extern verdict add_lepton_component(expression *the_expression,
        const char *label, expression *child_expression, boolean force);
extern verdict add_map_list_expression_component(expression *base,
        expression *key, expression *target);
extern verdict add_map_list_expression_filter_component(expression *base,
        type_expression *filter, expression *target);
extern verdict add_semi_labeled_expression_list_expression_component(
        expression *base, const char *label, expression *child_expression);

extern const source_location *get_expression_location(
        expression *the_expression);

extern void expression_error(expression *the_expression, const char *format,
                             ...);
extern void vexpression_error(expression *the_expression, const char *format,
                              va_list arg);

extern const char *expression_kind_operator_name(expression_kind kind);


#endif /* EXPRESSION_H */
