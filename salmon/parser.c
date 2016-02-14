/* file "parser.c" */

/*
 *  This file contains the implementation of the parser module, which uses the
 *  token output from the tokenizer module to build a parse tree.
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
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/auto_array.h"
#include "c_foundations/string_index.h"
#include "parser.h"
#include "token.h"
#include "tokenizer.h"
#include "open_expression.h"
#include "expression.h"
#include "open_basket.h"
#include "basket.h"
#include "open_type_expression.h"
#include "type_expression.h"
#include "open_call.h"
#include "call.h"
#include "open_statement.h"
#include "statement.h"
#include "open_statement_block.h"
#include "statement_block.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "tagalong_declaration.h"
#include "lepton_key_declaration.h"
#include "quark_declaration.h"
#include "lock_declaration.h"
#include "formal_arguments.h"
#include "file_parser.h"
#include "include.h"
#include "unbound.h"
#include "platform_dependent.h"


struct parser
  {
    tokenizer *tokenizer;
    include_handler_type include_handler;
    interface_include_handler_type interface_include_handler;
    void *include_handler_data;
    alias_manager *alias_manager;
    boolean alias_manager_owned;
    boolean native_bridge_dll_body_allowed;
  };

struct alias_manager
  {
    string_index *local;
    alias_manager *next_non_null;
  };

typedef struct
  {
    parser *parser;
    statement *have_one;
  } one_statement_data;


static verdict expect(tokenizer *the_tokenizer, token_kind expected_kind);
static verdict expect2(tokenizer *the_tokenizer, token_kind expected_kind1,
                       token_kind expected_kind2, token_kind *which);
static verdict expect_and_eat(tokenizer *the_tokenizer,
                              token_kind expected_kind);
static verdict expect_and_eat2(tokenizer *the_tokenizer,
        token_kind expected_kind1, token_kind expected_kind2,
        token_kind *which);
static verdict expect_and_eat_keyword(parser *the_parser, const char *keyword);
static char *expect_and_eat_identifier_return_name_copy(parser *the_parser);
static char *expect_and_eat_aliased_identifier_return_name_copy(
        parser *the_parser);
static char *expect_and_eat_possibly_aliased_identifier_return_name_copy(
        parser *the_parser, boolean check_aliases);
static char *expect_and_eat_string_literal_return_name_copy(
        parser *the_parser);
static const char *name_for_token_kind(token_kind kind);
static boolean next_is(tokenizer *the_tokenizer, token_kind kind);
static boolean next_is_keyword(parser *the_parser, const char *keyword);
static void decompose_open_type_expression(
        open_type_expression *the_open_type_expression,
        unbound_name_manager **manager, type_expression **the_type_expression);
static void decompose_open_call(open_call *the_open_call,
        unbound_name_manager **manager, call **the_call);
static void decompose_open_basket(open_basket *the_open_basket,
        unbound_name_manager **manager, basket **the_basket);
static expression_parsing_precedence token_prefix_precedence(token_kind kind);
static expression_parsing_precedence token_postfix_precedence(token_kind kind);
static type_expression_parsing_precedence token_prefix_type_precedence(
        token_kind kind);
static type_expression_parsing_precedence token_postfix_type_precedence(
        token_kind kind);
static boolean token_is_left_associative(token_kind kind);
static boolean token_is_unary_postfix_operator(token_kind kind);
static boolean token_is_binary_operator(token_kind kind);
static expression_kind token_unary_prefix_expression_kind(token_kind kind);
static expression_kind token_unary_postfix_expression_kind(token_kind kind);
static expression_kind token_binary_expression_kind(token_kind kind);
static void show_previous_declaration(statement_block *the_statement_block,
                                      size_t name_number);
static verdict parse_data_declaration_tail(parser *the_parser,
        unbound_name_manager **manager, char **name,
        type_expression **the_type, expression **initializer,
        boolean *initializer_is_forced, type_expression **dynamic_type,
        unbound_name_manager **dynamic_type_manager,
        source_location *end_location);
static expression *create_oi_expression(o_integer oi);
static boolean is_possible_range_type_expression(parser *the_parser);
static verdict semi_labeled_value_list_type_expression_set_extra_allowed(
        void *data);
static verdict semi_labeled_value_list_type_expression_set_extra_unspecified(
        void *data, boolean *error);
static verdict semi_labeled_value_list_type_expression_add_formal(void *data,
        const char *name, type_expression *formal_type, boolean has_default);
static verdict routine_type_expression_set_extra_allowed(void *data);
static verdict routine_type_expression_set_extra_unspecified(void *data,
                                                             boolean *error);
static verdict routine_type_expression_add_formal_generic(void *data,
        const char *name, type_expression *formal_type, boolean has_default);
static verdict fields_set_extra_allowed(type_expression *base_type);
static verdict fields_add_field(type_expression *base_type, const char *name,
                                type_expression *field_type);
static verdict lepton_set_extra_allowed(type_expression *base_type);
static verdict lepton_add_field(type_expression *base_type, const char *name,
                                type_expression *field_type);
static verdict multiset_set_extra_allowed(type_expression *base_type);
static verdict multiset_add_field(type_expression *base_type, const char *name,
                                  type_expression *field_type);
static open_statement *parser_statement_parser(void *data,
        const char **current_labels, size_t current_label_count);
static boolean parser_done_test(void *data);
static open_statement *parser_single_statement_parser(void *data,
        const char **current_labels, size_t current_label_count);
static boolean parser_single_statement_done_test(void *data);
static expression *anonymous_lock_expression(const source_location *location);
static verdict handle_use_statement_for_unbound_name(statement *use_statement,
        const char *internal_name, unbound_name *the_unbound_name,
        unbound_name_manager *manager, boolean flow_through_allowed);
static verdict statement_add_declaration(void *data,
                                         declaration *new_declaration);
static verdict do_statement_end_semicolon(parser *the_parser,
                                          statement *the_statement);
static formal_arguments *parse_formal_arguments(parser *the_parser,
        boolean *extra_arguments_allowed,
        unbound_name_manager **result_static_manager,
        unbound_name_manager **result_dynamic_manager,
        source_location *end_location);
static verdict bind_formals(formal_arguments *formals,
        unbound_name_manager *manager, alias_manager *the_alias_manager);
static verdict ignore_until(tokenizer *the_tokenizer, token_kind end_kind);
static const char *aliased_token_name(token *the_token, parser *the_parser);
static const char *try_aliasing(const char *base, parser *the_parser);
static const char *try_aliasing_from_alias_manager(const char *base,
        alias_manager *alias_manager);
static unbound_use *add_aliased_unbound_operator_expression(
        unbound_name_manager *manager, const char *name,
        expression *this_expression, parser *the_parser,
        const source_location *location);
static verdict resolve_statement_block(statement_block *the_statement_block,
        unbound_name_manager *the_unbound_name_manager,
        alias_manager *parent_alias_manager);
static open_expression *parse_range_expression_tail(parser *the_parser,
        boolean open_is_inclusive, open_expression *open_lower,
        const source_location *start_location);


extern parser *create_parser(tokenizer *the_tokenizer,
        include_handler_type include_handler,
        interface_include_handler_type interface_include_handler,
        void *include_handler_data, alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed)
  {
    parser *result;

    assert(the_tokenizer != NULL);
    assert(include_handler != NULL);
    assert(interface_include_handler != NULL);

    result = MALLOC_ONE_OBJECT(parser);
    if (result == NULL)
        return NULL;

    result->tokenizer = the_tokenizer;
    result->include_handler = include_handler;
    result->interface_include_handler = interface_include_handler;
    result->include_handler_data = include_handler_data;

    if (parent_alias_manager == NULL)
      {
        result->alias_manager_owned = TRUE;
        result->alias_manager = MALLOC_ONE_OBJECT(alias_manager);
        if (result->alias_manager == NULL)
          {
            free(result);
            return NULL;
          }
        result->alias_manager->local = NULL;
        result->alias_manager->next_non_null = NULL;
      }
    else
      {
        result->alias_manager = parent_alias_manager;
        result->alias_manager_owned = FALSE;
      }

    result->native_bridge_dll_body_allowed = native_bridge_dll_body_allowed;

    return result;
  }

extern void delete_parser(parser *the_parser)
  {
    assert(the_parser != NULL);

    if (the_parser->alias_manager_owned)
      {
        alias_manager *the_alias_manager;

        the_alias_manager = the_parser->alias_manager;
        assert(the_alias_manager != NULL);
        if (the_alias_manager->local != NULL)
            destroy_string_index(the_alias_manager->local);
        free(the_alias_manager);
      }

    free(the_parser);
  }

/*
 *      <expression> :
 *          <constant-expression> |
 *          <name-reference-expression> |
 *          <lookup-expression> |
 *          <lepton-expression> |
 *          <field-reference-expression> |
 *          <pointer-field-reference-expression> |
 *          <tagalong-field-reference-expression> |
 *          <statement-block-expression> |
 *          <new-expression> |
 *          <routine-expression> |
 *          <tagalong-expression> |
 *          <lepton-key-expression> |
 *          <quark-expression> |
 *          <lock-expression> |
 *          <construct-expression> |
 *          <type-value-expression> |
 *          <map-list-expression> |
 *          <semi-labeled-expression-list-expression> |
 *          <call-expression> |
 *          <conditional-expression> |
 *          <unary-expression> |
 *          <binary-expression> |
 *          <arguments-expression> |
 *          <in-expression> |
 *          <force-expression> |
 *          <break-expression> |
 *          <continue-expression> |
 *          <comprehend-expression> |
 *          <backtick-expression> |
 *          <forall-expression> |
 *          <exists-expression> |
 *          "(" <expression> ")"
 *
 *      <constant-expression> :
 *          <string-constant-expression> |
 *          <character-constant-expression> |
 *          <integer-constant-expression> |
 *          <rational-constant-expression> |
 *          <regular-expression-constant-expression>
 *
 *      <string-constant-expression> :
 *          <string-literal-token>
 *
 *      <character-constant-expression> :
 *          <character-literal-token>
 *
 *      <integer-constant-expression> :
 *          <decimal-integer-literal-token> |
 *          <hexadecimal-integer-literal-token>
 *
 *      <rational-constant-expression> :
 *          <scientific-notation-literal-token>
 *
 *      <regular-expression-constant-expression> :
 *          <regular-expression-literal-token>
 *
 *      <name-reference-expression> :
 *          <identifier>
 *
 *      <field-reference-expression> :
 *          <expression> "." <identifier>
 *
 *      <pointer-field-reference-expression> :
 *          <expression> "->" <identifier>
 *
 *      <tagalong-field-reference-expression> :
 *          <expression> ".." <expression>
 *
 *      <statement-block-expression> :
 *          <braced-statement-block>
 *
 *      <new-expression> :
 *          <unnamed-data-declaration>
 *
 *      <routine-expression> :
 *          <unnamed-routine-declaration>
 *
 *      <tagalong-expression> :
 *          <unnamed-tagalong-declaration>
 *
 *      <lepton-key-expression> :
 *          <unnamed-lepton-declaration>
 *
 *      <quark-expression> :
 *          <unnamed-quark-declaration>
 *
 *      <lock-expression> :
 *          <unnamed-lock-declaration>
 *
 *      <construct-expression> :
 *          { <single-prefix> }? "construct" <call>
 *
 *      <type-value-expression> :
 *          "type" <type-expression>
 *
 *      <map-list-expression> :
 *          "<<" <map-item-list> ">>"
 *
 *      <map-item-list> :
 *          <empty> |
 *          <non-empty-map-item-list>
 *
 *      <non-empty-map-item-list> :
 *          <map-item> |
 *          <map-item> "," <non-empty-map-item-list>
 *
 *      <map-item> :
 *          "(" <expression> "-->" <expression> ")" |
 *          "(" "*" { ":" <type-expression> }? "-->" <expression> ")"
 *
 *      <call-expression> :
 *          <call>
 *
 *      <call> :
 *          <expression> "(" <semi-labeled-expression-list> ")"
 *
 *      <conditional-expression> :
 *          <expression> "?" <expression> ":" <expression>
 *
 *      <unary-expression> :
 *          <dereference-expression> |
 *          <location-of-expression> |
 *          <negate-expression> |
 *          <unary-plus-expression> |
 *          <bitwise-not-expression> |
 *          <logical-not-expression>
 *
 *      <dereference-expression> :
 *          "*" <expression>
 *
 *      <location-of-expression> :
 *          "&" <expression>
 *
 *      <negate-expression> :
 *          "-" <expression>
 *
 *      <unary-plus-expression> :
 *          "+" <expression>
 *
 *      <bitwise-not-expression> :
 *          "~" <expression>
 *
 *      <logical-not-expression> :
 *          "!" <expression>
 *
 *      <binary-expression> :
 *          <expression> <binary-operator> <expression>
 *
 *      <binary-operator> :
 *          "+" |
 *          "-" |
 *          "*" |
 *          "/" |
 *          "/::" |
 *          "%" |
 *          "<<" |
 *          ">>" |
 *          "<" |
 *          ">" |
 *          "<=" |
 *          ">=" |
 *          "==" |
 *          "!=" |
 *          "&" |
 *          "|" |
 *          "^" |
 *          "&&" |
 *          "||" |
 *          "~"
 *
 *      <arguments-expression> :
 *          "arguments" { "of" <identifier> }?
 *
 *      <this-expression> :
 *          "this" { "of" <identifier> }?
 *
 *      <in-expression> :
 *          <expression> "in" <type-expression>
 *
 *      <force-expression> :
 *          <expression> "::" <type-expression>
 *
 *      <break-expression> :
 *          "break" { "from" <identifier> }?
 *
 *      <continue-expression> :
 *          "continue" { "with" <identifier> }?
 *
 *      <comprehend-expression> :
 *          "comprehend" "(" <identifier> ";" <expression>
 *                  { ";" <expression> }? ")" <expression>
 *
 *      <backtick-expression> :
 *          <backtick-expression-literal-token>
 *
 *      <forall-expression> :
 *          "forall" "(" <formal-argument-list> ")" <expression>
 *
 *      <exists-expression> :
 *          "exists" "(" <formal-argument-list> ")" <expression>
 */
extern open_expression *parse_expression(parser *the_parser,
        expression_parsing_precedence precedence,
        unbound_use **naked_call_overloading_use, boolean stop_with_call)
  {
    return parse_expression_with_end_location(the_parser, precedence,
            naked_call_overloading_use, stop_with_call, NULL);
  }

extern open_expression *parse_expression_with_end_location(parser *the_parser,
        expression_parsing_precedence precedence,
        unbound_use **naked_call_overloading_use, boolean stop_with_call,
        source_location *end_location)
  {
    source_location start_location;
    tokenizer *the_tokenizer;
    token *the_token;
    token_kind kind;
    unbound_name_manager *manager;
    expression *result;

    assert(the_parser != NULL);

    if (naked_call_overloading_use != NULL)
        *naked_call_overloading_use = NULL;

    the_tokenizer = the_parser->tokenizer;
    assert(the_tokenizer != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));

    the_token = next_token(the_tokenizer);
    if (the_token == NULL)
        return NULL;

    kind = get_token_kind(the_token);

    if (kind == TK_ERROR)
        return NULL;

    switch (kind)
      {
        case TK_STRING_LITERAL:
          {
            value *the_value;
            verdict the_verdict;

            the_value =
                    create_string_value(string_literal_token_data(the_token));
            if (the_value == NULL)
                return NULL;

            result = create_constant_expression(the_value);
            value_remove_reference(the_value, NULL);
            if (result == NULL)
                return NULL;

            set_expression_end_location(result,
                                        next_token_location(the_tokenizer));
            if (end_location != NULL)
                *end_location = *(next_token_location(the_tokenizer));

            the_verdict = consume_token(the_tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(result);
                return NULL;
              }

            manager = create_unbound_name_manager();
            if (manager == NULL)
              {
                delete_expression(result);
                return NULL;
              }

            break;
          }
        case TK_CHARACTER_LITERAL:
          {
            value *the_value;
            verdict the_verdict;

            the_value = create_character_value(
                    character_literal_token_data(the_token));
            if (the_value == NULL)
                return NULL;

            result = create_constant_expression(the_value);
            value_remove_reference(the_value, NULL);
            if (result == NULL)
                return NULL;

            set_expression_end_location(result,
                                        next_token_location(the_tokenizer));
            if (end_location != NULL)
                *end_location = *(next_token_location(the_tokenizer));

            the_verdict = consume_token(the_tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(result);
                return NULL;
              }

            manager = create_unbound_name_manager();
            if (manager == NULL)
              {
                delete_expression(result);
                return NULL;
              }

            break;
          }
        case TK_DECIMAL_INTEGER_LITERAL:
        case TK_HEXADECIMAL_INTEGER_LITERAL:
          {
            value *the_value;
            verdict the_verdict;

            the_value = create_integer_value(
                    integer_literal_token_integer(the_token));
            if (the_value == NULL)
                return NULL;

            result = create_constant_expression(the_value);
            value_remove_reference(the_value, NULL);
            if (result == NULL)
                return NULL;

            set_expression_end_location(result,
                                        next_token_location(the_tokenizer));
            if (end_location != NULL)
                *end_location = *(next_token_location(the_tokenizer));

            the_verdict = consume_token(the_tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(result);
                return NULL;
              }

            manager = create_unbound_name_manager();
            if (manager == NULL)
              {
                delete_expression(result);
                return NULL;
              }

            break;
          }
        case TK_SCIENTIFIC_NOTATION_LITERAL:
          {
            rational *the_rational;
            value *the_value;
            verdict the_verdict;

            the_rational =
                    scientific_notation_literal_token_rational(the_token);
            assert(the_rational != NULL);

            if (rational_is_integer(the_rational))
              {
                the_value =
                        create_integer_value(rational_numerator(the_rational));
              }
            else
              {
                the_value = create_rational_value(the_rational);
              }
            if (the_value == NULL)
                return NULL;

            result = create_constant_expression(the_value);
            value_remove_reference(the_value, NULL);
            if (result == NULL)
                return NULL;

            set_expression_end_location(result,
                                        next_token_location(the_tokenizer));
            if (end_location != NULL)
                *end_location = *(next_token_location(the_tokenizer));

            the_verdict = consume_token(the_tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(result);
                return NULL;
              }

            manager = create_unbound_name_manager();
            if (manager == NULL)
              {
                delete_expression(result);
                return NULL;
              }

            break;
          }
        case TK_REGULAR_EXPRESSION_LITERAL:
          {
            value *the_value;
            verdict the_verdict;

            the_value = create_regular_expression_value(
                    regular_expression_literal_token_data(the_token));
            if (the_value == NULL)
                return NULL;

            result = create_constant_expression(the_value);
            value_remove_reference(the_value, NULL);
            if (result == NULL)
                return NULL;

            set_expression_end_location(result,
                                        next_token_location(the_tokenizer));
            if (end_location != NULL)
                *end_location = *(next_token_location(the_tokenizer));

            the_verdict = consume_token(the_tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(result);
                return NULL;
              }

            manager = create_unbound_name_manager();
            if (manager == NULL)
              {
                delete_expression(result);
                return NULL;
              }

            break;
          }
        case TK_BACKTICK_EXPRESSION_LITERAL:
          {
            const char *expression_string;
            expression *system_base;
            unbound_use *use;
            call *system_call;
            call *sprint_call;
            const char *remaining_expression;
            verdict the_verdict;

            expression_string =
                    backtick_expression_literal_token_data(the_token);
            assert(expression_string != NULL);

            system_base = create_unbound_name_reference_expression();
            if (system_base == NULL)
                return NULL;

            manager = create_unbound_name_manager();
            if (manager == NULL)
              {
                delete_expression(system_base);
                return NULL;
              }

            use = add_unbound_variable_reference(manager, "system",
                    system_base, get_token_location(the_token));
            if (use == NULL)
              {
                delete_expression(system_base);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            system_call = create_call(system_base, 0, NULL, NULL,
                                      get_token_location(the_token));
            if (system_call == NULL)
              {
                delete_unbound_name_manager(manager);
                return NULL;
              }

            sprint_call = NULL;

            remaining_expression = expression_string;

            while (*remaining_expression != 0)
              {
                if ((*remaining_expression == '$') &&
                    (remaining_expression[1] != '$'))
                  {
                    expression *sub_expression;
                    verdict the_verdict;

                    ++remaining_expression;

                    if (((*remaining_expression >= 'a') &&
                         (*remaining_expression <= 'z')) ||
                        ((*remaining_expression >= 'A') &&
                         (*remaining_expression <= 'Z')) ||
                        (*remaining_expression == '_'))
                      {
                        const char *follow;
                        char *buffer;
                        unbound_use *use;

                        follow = remaining_expression + 1;

                        while (((*follow >= 'a') && (*follow <= 'z')) ||
                               ((*follow >= 'A') && (*follow <= 'Z')) ||
                               ((*follow >= '0') && (*follow <= '9')) ||
                               (*follow == '_'))
                          {
                            ++follow;
                          }

                        assert(follow > remaining_expression);
                        buffer = MALLOC_ARRAY(char,
                                (follow - remaining_expression) + 1);
                        if (buffer == NULL)
                          {
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        memcpy(buffer, remaining_expression,
                               (follow - remaining_expression));
                        buffer[follow - remaining_expression] = 0;

                        sub_expression =
                                create_unbound_name_reference_expression();
                        if (sub_expression == NULL)
                          {
                            free(buffer);
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        use = add_unbound_variable_reference(manager,
                                try_aliasing(buffer, the_parser),
                                sub_expression, get_token_location(the_token));
                        free(buffer);
                        if (use == NULL)
                          {
                            delete_expression(sub_expression);
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        remaining_expression = follow;
                      }
                    else if (*remaining_expression == '(')
                      {
                        tokenizer *sub_tokenizer;
                        parser *sub_parser;
                        open_expression *sub_open;
                        unbound_name_manager *sub_manager;
                        verdict the_verdict;
                        const char *expression_end;

                        ++remaining_expression;

                        sub_tokenizer = create_tokenizer(remaining_expression,
                                get_token_location(the_token)->file_name);
                        if (sub_tokenizer == NULL)
                          {
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        set_tokenizer_line_number(sub_tokenizer,
                                get_token_location(the_token)->
                                        start_line_number);

                        /* Note that the following computation of the column
                         * number isn't always exactly right.  That's because
                         * we don't know at this point exactly what
                         * transformations the tokenizer did to the input to
                         * produce the string it gave us for the backtick
                         * expression.  For example, escape sequences in the
                         * backtick expression can map several source file
                         * characters into a single character in the string we
                         * have here.  But in most cases this calculation will
                         * be right, and it's not worth the considerable
                         * trouble it would be to get exactly the right value
                         * here since it will almost certainly be very close in
                         * all practical cases -- close enough that the user
                         * will be able to figure out where the problem is in
                         * virtually every case that's likely to actually
                         * occur. */
                        set_tokenizer_column_number(sub_tokenizer,
                                get_token_location(the_token)->
                                        start_column_number +
                                (remaining_expression - expression_string) +
                                1);

                        sub_parser = create_parser(sub_tokenizer,
                                the_parser->include_handler,
                                the_parser->interface_include_handler,
                                the_parser->include_handler_data,
                                the_parser->alias_manager,
                                the_parser->native_bridge_dll_body_allowed);
                        if (sub_parser == NULL)
                          {
                            delete_tokenizer(sub_tokenizer);
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        sub_open = parse_expression(sub_parser, EPP_TOP, NULL,
                                                    FALSE);
                        if (sub_open == NULL)
                          {
                            delete_parser(sub_parser);
                            delete_tokenizer(sub_tokenizer);
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        decompose_open_expression(sub_open, &sub_manager,
                                                  &sub_expression);

                        the_verdict = merge_in_unbound_name_manager(manager,
                                sub_manager);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            delete_expression(sub_expression);
                            delete_parser(sub_parser);
                            delete_tokenizer(sub_tokenizer);
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        expression_end =
                                tokenizer_raw_position(sub_tokenizer);
                        assert(expression_end > remaining_expression);
                        assert(expression_end <=
                               (remaining_expression +
                                strlen(remaining_expression)));
                        remaining_expression = expression_end;

                        delete_parser(sub_parser);
                        delete_tokenizer(sub_tokenizer);

                        if (*remaining_expression != ')')
                          {
                            token_error(the_token,
                                    "Syntax error -- in a backtick expression,"
                                    " a parenthesized expression following a $"
                                    " doesn't close with a proper "
                                    "parenthesis.");
                            delete_expression(sub_expression);
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        ++remaining_expression;
                      }
                    else
                      {
                        token_error(the_token,
                                "Syntax error -- in a backtick expression, a $"
                                " appears not followed by another dollar sign,"
                                " an identifier, or a left parenthesis.");
                        delete_call(system_call);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    assert(sub_expression != NULL);

                    if (sprint_call == NULL)
                      {
                        expression *sprint_base;
                        unbound_use *use;
                        expression *sprint_expression;
                        verdict the_verdict;

                        sprint_base =
                                create_unbound_name_reference_expression();
                        if (sprint_base == NULL)
                          {
                            delete_expression(sub_expression);
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        use = add_unbound_variable_reference(manager, "sprint",
                                sprint_base, get_token_location(the_token));
                        if (use == NULL)
                          {
                            delete_expression(sprint_base);
                            delete_expression(sub_expression);
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        sprint_call = create_call(sprint_base, 0, NULL, NULL,
                                get_token_location(the_token));
                        if (sprint_call == NULL)
                          {
                            delete_expression(sub_expression);
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        sprint_expression =
                                create_call_expression(sprint_call);
                        if (sprint_expression == NULL)
                          {
                            delete_expression(sub_expression);
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        the_verdict = append_argument_to_call(system_call,
                                sprint_expression, NULL);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            delete_expression(sub_expression);
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }
                      }

                    assert(sprint_call != NULL);

                    the_verdict = append_argument_to_call(sprint_call,
                            sub_expression, NULL);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_call(system_call);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }
                  }
                else
                  {
                    const char *follow;
                    char *buffer;
                    char *next_to_set;
                    const char *next_to_read;
                    value *string_value;
                    expression *string_expression;

                    follow = remaining_expression;
                    while (*follow != 0)
                      {
                        if (*follow == '$')
                          {
                            if (follow[1] == '$')
                                follow += 2;
                            else
                                break;
                          }
                        else
                          {
                            ++follow;
                          }
                      }

                    assert(follow > remaining_expression);

                    buffer = MALLOC_ARRAY(char,
                                          (follow - remaining_expression) + 1);
                    if (buffer == NULL)
                      {
                        delete_call(system_call);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    next_to_set = buffer;
                    next_to_read = remaining_expression;

                    while (next_to_read < follow)
                      {
                        if (*next_to_read == '$')
                          {
                            ++next_to_read;
                            assert(next_to_read < follow);
                            assert(*next_to_read == '$');
                          }
                        *next_to_set = *next_to_read;
                        ++next_to_set;
                        ++next_to_read;
                      }

                    assert(next_to_set <
                           (buffer + ((follow - remaining_expression) + 1)));
                    *next_to_set = 0;

                    string_value = create_string_value(buffer);
                    free(buffer);
                    if (string_value == NULL)
                      {
                        delete_call(system_call);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    string_expression =
                            create_constant_expression(string_value);
                    value_remove_reference(string_value, NULL);
                    if (string_expression == NULL)
                      {
                        delete_call(system_call);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    if ((remaining_expression == expression_string) &&
                        (*follow == 0))
                      {
                        verdict the_verdict;

                        assert(sprint_call == NULL);

                        the_verdict = append_argument_to_call(system_call,
                                string_expression, NULL);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }
                      }
                    else
                      {
                        verdict the_verdict;

                        if (sprint_call == NULL)
                          {
                            expression *sprint_base;
                            unbound_use *use;
                            expression *sprint_expression;
                            verdict the_verdict;

                            sprint_base =
                                    create_unbound_name_reference_expression();
                            if (sprint_base == NULL)
                              {
                                delete_expression(string_expression);
                                delete_call(system_call);
                                delete_unbound_name_manager(manager);
                                return NULL;
                              }

                            use = add_unbound_variable_reference(manager,
                                    "sprint", sprint_base,
                                    get_token_location(the_token));
                            if (use == NULL)
                              {
                                delete_expression(sprint_base);
                                delete_expression(string_expression);
                                delete_call(system_call);
                                delete_unbound_name_manager(manager);
                                return NULL;
                              }

                            sprint_call = create_call(sprint_base, 0, NULL,
                                    NULL, get_token_location(the_token));
                            if (sprint_call == NULL)
                              {
                                delete_expression(string_expression);
                                delete_call(system_call);
                                delete_unbound_name_manager(manager);
                                return NULL;
                              }

                            sprint_expression =
                                    create_call_expression(sprint_call);
                            if (sprint_expression == NULL)
                              {
                                delete_expression(string_expression);
                                delete_call(system_call);
                                delete_unbound_name_manager(manager);
                                return NULL;
                              }

                            the_verdict = append_argument_to_call(system_call,
                                    sprint_expression, NULL);
                            if (the_verdict != MISSION_ACCOMPLISHED)
                              {
                                delete_expression(string_expression);
                                delete_call(system_call);
                                delete_unbound_name_manager(manager);
                                return NULL;
                              }
                          }

                        assert(sprint_call != NULL);

                        the_verdict = append_argument_to_call(sprint_call,
                                string_expression, NULL);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            delete_call(system_call);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }
                      }

                    remaining_expression = follow;
                  }
              }

            result = create_call_expression(system_call);
            if (result == NULL)
              {
                delete_unbound_name_manager(manager);
                return NULL;
              }

            set_expression_end_location(result, get_token_location(the_token));
            if (end_location != NULL)
                *end_location = *(get_token_location(the_token));

            use = add_aliased_unbound_operator_expression(manager,
                    "operator()", result, the_parser,
                    get_token_location(the_token));
            if (use == NULL)
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            if (naked_call_overloading_use != NULL)
                *naked_call_overloading_use = use;

            the_verdict = consume_token(the_tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            if (stop_with_call)
              {
                set_expression_start_location(result, &start_location);
                return create_open_expression(result, manager);
              }

            break;
          }
        case TK_IDENTIFIER:
          {
            const char *identifier_string;
            unbound_use *use;
            verdict the_verdict;

            identifier_string = aliased_token_name(the_token, the_parser);

            if (strcmp(identifier_string, "single") == 0)
              {
                verdict the_verdict;
                expression *single_lock;
                unbound_name_manager *single_manager;
                token *the_token;
                open_expression *open_result;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                if (next_is(the_parser->tokenizer, TK_LEFT_PAREN))
                  {
                    verdict the_verdict;
                    open_expression *open_lock;

                    the_verdict = expect_and_eat(the_parser->tokenizer,
                                                 TK_LEFT_PAREN);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                        return NULL;

                    open_lock =
                            parse_expression(the_parser, EPP_TOP, NULL, FALSE);
                    if (open_lock == NULL)
                        return NULL;

                    the_verdict = expect_and_eat(the_parser->tokenizer,
                                                 TK_RIGHT_PAREN);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_open_expression(open_lock);
                        return NULL;
                      }

                    decompose_open_expression(open_lock, &single_manager,
                                              &single_lock);
                  }
                else
                  {
                    single_lock = anonymous_lock_expression(&start_location);
                    if (single_lock == NULL)
                        return NULL;

                    single_manager = create_unbound_name_manager();
                    if (single_manager == NULL)
                      {
                        delete_expression(single_lock);
                        return NULL;
                      }
                  }

                the_token = next_token(the_tokenizer);
                if ((the_token == NULL) ||
                    (get_token_kind(the_token) == TK_ERROR))
                  {
                    if (single_lock != NULL)
                        delete_expression(single_lock);
                    if (single_manager != NULL)
                        delete_unbound_name_manager(single_manager);
                    return NULL;
                  }

                if (get_token_kind(the_token) != TK_IDENTIFIER)
                  {
                    token_error(the_token,
                            "Syntax error -- after the `single' keyword%s, "
                            "expected %sone of the keywords `construct', "
                            "`static', `virtual', `pure', `procedure', "
                            "`function', `routine', `class', `variable', "
                            "`immutable', `tagalong', `lepton', `quark', or "
                            "`lock', but found %s.",
                            ((single_lock == NULL) ? "" :
                             " and its lock expression"),
                            ((single_lock == NULL) ? "a left parenthesis or " :
                             ""),
                            name_for_token_kind(get_token_kind(the_token)));
                    if (single_lock != NULL)
                        delete_expression(single_lock);
                    if (single_manager != NULL)
                        delete_unbound_name_manager(single_manager);
                    return NULL;
                  }

                identifier_string = aliased_token_name(the_token, the_parser);

                if (strcmp(identifier_string, "construct") == 0)
                  {
                    open_result = parse_construct_expression(the_parser,
                            single_lock, single_manager);
                  }
                else if ((strcmp(identifier_string, "static") == 0) ||
                         (strcmp(identifier_string, "virtual") == 0) ||
                         (strcmp(identifier_string, "pure") == 0) ||
                         (strcmp(identifier_string, "procedure") == 0) ||
                         (strcmp(identifier_string, "function") == 0) ||
                         (strcmp(identifier_string, "routine") == 0) ||
                         (strcmp(identifier_string, "class") == 0) ||
                         (strcmp(identifier_string, "variable") == 0) ||
                         (strcmp(identifier_string, "immutable") == 0) ||
                         (strcmp(identifier_string, "tagalong") == 0) ||
                         (strcmp(identifier_string, "lepton") == 0) ||
                         (strcmp(identifier_string, "quark") == 0) ||
                         (strcmp(identifier_string, "lock") == 0))
                  {
                    open_result = parse_routine_or_new_expression(the_parser,
                            single_lock, single_manager);
                  }
                else
                  {
                    token_error(the_token,
                            "Syntax error -- after the `single' keyword%s, "
                            "expected %sone of the keywords `construct', "
                            "`static', `virtual', `pure', `procedure', "
                            "`function', `routine', `class', `variable', "
                            "`immutable', `tagalong', `lepton', `quark', or "
                            "`lock', but found identifier `%s'.",
                            ((single_lock == NULL) ? "" :
                             " and its lock expression"),
                            ((single_lock == NULL) ? "a left parenthesis or " :
                             ""), identifier_string);
                    if (single_lock != NULL)
                        delete_expression(single_lock);
                    if (single_manager != NULL)
                        delete_unbound_name_manager(single_manager);
                    return NULL;
                  }

                if (open_result == NULL)
                    return NULL;

                decompose_open_expression(open_result, &manager, &result);

                if (end_location != NULL)
                    *end_location = *(get_expression_location(result));

                break;
              }

            if ((strcmp(identifier_string, "routine") == 0) ||
                (strcmp(identifier_string, "function") == 0) ||
                (strcmp(identifier_string, "procedure") == 0) ||
                (strcmp(identifier_string, "class") == 0) ||
                (strcmp(identifier_string, "variable") == 0) ||
                (strcmp(identifier_string, "immutable") == 0) ||
                (strcmp(identifier_string, "tagalong") == 0) ||
                (strcmp(identifier_string, "lepton") == 0) ||
                (strcmp(identifier_string, "quark") == 0) ||
                (strcmp(identifier_string, "lock") == 0) ||
                (strcmp(identifier_string, "static") == 0) ||
                (strcmp(identifier_string, "virtual") == 0) ||
                (strcmp(identifier_string, "pure") == 0))
              {
                open_expression *open_result;

                open_result = parse_routine_or_new_expression(the_parser, NULL,
                                                              NULL);
                if (open_result == NULL)
                    return NULL;

                decompose_open_expression(open_result, &manager, &result);

                if (end_location != NULL)
                    *end_location = *(get_expression_location(result));

                break;
              }

            if (strcmp(identifier_string, "construct") == 0)
              {
                open_expression *open_result;

                open_result =
                        parse_construct_expression(the_parser, NULL, NULL);
                if (open_result == NULL)
                    return NULL;

                decompose_open_expression(open_result, &manager, &result);

                if (end_location != NULL)
                    *end_location = *(get_expression_location(result));

                break;
              }

            if (strcmp(identifier_string, "type") == 0)
              {
                verdict the_verdict;
                source_location type_end_location;
                open_type_expression *open_type;
                type_expression *type;

                the_verdict = consume_token(the_tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                open_type = parse_type_expression_with_end_location(the_parser,
                        TEPP_TOP, &type_end_location);
                if (open_type == NULL)
                    return NULL;

                decompose_open_type_expression(open_type, &manager, &type);

                result = create_type_expression_expression(type);
                if (result == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_expression_end_location(result, &type_end_location);
                if (end_location != NULL)
                    *end_location = type_end_location;

                break;
              }

            if (strcmp(identifier_string, "arguments") == 0)
              {
                source_location arguments_end_location;
                verdict the_verdict;
                char *name;
                unbound_use *use;

                arguments_end_location = *(next_token_location(the_tokenizer));

                the_verdict = consume_token(the_tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                result = create_arguments_expression();
                if (result == NULL)
                    return NULL;

                if (next_is_keyword(the_parser, "of"))
                  {
                    verdict the_verdict;

                    the_verdict = expect_and_eat_keyword(the_parser, "of");
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_expression(result);
                        return NULL;
                      }

                    arguments_end_location =
                            *(next_token_location(the_tokenizer));

                    name = expect_and_eat_aliased_identifier_return_name_copy(
                            the_parser);
                    if (name == NULL)
                      {
                        delete_expression(result);
                        return NULL;
                      }
                  }
                else
                  {
                    name = NULL;
                  }

                set_expression_end_location(result, &arguments_end_location);
                if (end_location != NULL)
                    *end_location = arguments_end_location;

                manager = create_unbound_name_manager();
                if (manager == NULL)
                  {
                    if (name != NULL)
                        free(name);
                    delete_expression(result);
                    return NULL;
                  }

                use = add_unbound_arguments_expression(manager, name, result,
                                                       &start_location);
                if (name != NULL)
                    free(name);
                if (use == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    delete_expression(result);
                    return NULL;
                  }

                break;
              }

            if (strcmp(identifier_string, "this") == 0)
              {
                source_location this_end_location;
                verdict the_verdict;
                char *name;
                unbound_use *use;

                this_end_location = *(next_token_location(the_tokenizer));

                the_verdict = consume_token(the_tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                result = create_this_expression();
                if (result == NULL)
                    return NULL;

                if (next_is_keyword(the_parser, "of"))
                  {
                    verdict the_verdict;

                    the_verdict = expect_and_eat_keyword(the_parser, "of");
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_expression(result);
                        return NULL;
                      }

                    this_end_location = *(next_token_location(the_tokenizer));

                    name = expect_and_eat_identifier_return_name_copy(
                            the_parser);
                    if (name == NULL)
                      {
                        delete_expression(result);
                        return NULL;
                      }
                  }
                else
                  {
                    name = NULL;
                  }

                set_expression_end_location(result, &this_end_location);
                if (end_location != NULL)
                    *end_location = this_end_location;

                manager = create_unbound_name_manager();
                if (manager == NULL)
                  {
                    if (name != NULL)
                        free(name);
                    delete_expression(result);
                    return NULL;
                  }

                use = add_unbound_this_expression(manager, name, result,
                                                  &start_location);
                if (name != NULL)
                    free(name);
                if (use == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    delete_expression(result);
                    return NULL;
                  }

                break;
              }

            if (strcmp(identifier_string, "break") == 0)
              {
                source_location this_end_location;
                verdict the_verdict;
                char *name_copy;
                unbound_use *use;

                this_end_location = *(next_token_location(the_tokenizer));

                the_verdict = consume_token(the_tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                if (next_is_keyword(the_parser, "from"))
                  {
                    the_verdict = expect_and_eat_keyword(the_parser, "from");
                    if (the_verdict != MISSION_ACCOMPLISHED)
                        return NULL;

                    this_end_location = *(next_token_location(the_tokenizer));

                    name_copy =
                            expect_and_eat_aliased_identifier_return_name_copy(
                                    the_parser);
                    if (name_copy == NULL)
                        return NULL;
                  }
                else
                  {
                    name_copy = NULL;
                  }

                manager = create_unbound_name_manager();
                if (manager == NULL)
                  {
                    if (name_copy != NULL)
                        free(name_copy);
                    return NULL;
                  }

                result = create_break_expression();
                if (result == NULL)
                  {
                    if (name_copy != NULL)
                        free(name_copy);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_expression_end_location(result, &this_end_location);
                if (end_location != NULL)
                    *end_location = this_end_location;

                use = add_unbound_break_expression(manager, name_copy, result,
                                                   &start_location);
                if (name_copy != NULL)
                    free(name_copy);
                if (use == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    delete_expression(result);
                    return NULL;
                  }

                break;
              }

            if (strcmp(identifier_string, "continue") == 0)
              {
                source_location this_end_location;
                verdict the_verdict;
                char *name_copy;
                unbound_use *use;

                this_end_location = *(next_token_location(the_tokenizer));

                the_verdict = consume_token(the_tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                if (next_is_keyword(the_parser, "with"))
                  {
                    the_verdict = expect_and_eat_keyword(the_parser, "with");
                    if (the_verdict != MISSION_ACCOMPLISHED)
                        return NULL;

                    this_end_location = *(next_token_location(the_tokenizer));

                    name_copy =
                            expect_and_eat_aliased_identifier_return_name_copy(
                                    the_parser);
                    if (name_copy == NULL)
                        return NULL;
                  }
                else
                  {
                    name_copy = NULL;
                  }

                manager = create_unbound_name_manager();
                if (manager == NULL)
                  {
                    if (name_copy != NULL)
                        free(name_copy);
                    return NULL;
                  }

                result = create_continue_expression();
                if (result == NULL)
                  {
                    if (name_copy != NULL)
                        free(name_copy);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_expression_end_location(result, &this_end_location);
                if (end_location != NULL)
                    *end_location = this_end_location;

                use = add_unbound_continue_expression(manager, name_copy,
                                                      result, &start_location);
                if (name_copy != NULL)
                    free(name_copy);
                if (use == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    delete_expression(result);
                    return NULL;
                  }

                break;
              }

            if (strcmp(identifier_string, "comprehend") == 0)
              {
                verdict the_verdict;
                source_location element_declaration_location;
                char *name_copy;
                open_expression *open_base;
                expression *base;
                unbound_name_manager *filter_manager;
                expression *filter;
                source_location local_end_location;
                open_expression *open_body;
                unbound_name_manager *body_manager;
                expression *body;

                the_verdict = consume_token(the_tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                the_verdict = expect_and_eat(the_tokenizer, TK_LEFT_PAREN);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                element_declaration_location =
                        *(next_token_location(the_tokenizer));

                name_copy =
                        expect_and_eat_identifier_return_name_copy(the_parser);
                if (name_copy == NULL)
                    return NULL;

                the_verdict = expect_and_eat(the_tokenizer, TK_SEMICOLON);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    free(name_copy);
                    return NULL;
                  }

                open_base = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
                if (open_base == NULL)
                  {
                    free(name_copy);
                    return NULL;
                  }

                decompose_open_expression(open_base, &manager, &base);

                if (next_is(the_tokenizer, TK_SEMICOLON))
                  {
                    verdict the_verdict;
                    open_expression *open_filter;

                    the_verdict = consume_token(the_tokenizer);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_expression(base);
                        delete_unbound_name_manager(manager);
                        free(name_copy);
                        return NULL;
                      }

                    open_filter =
                            parse_expression(the_parser, EPP_TOP, NULL, FALSE);
                    if (open_filter == NULL)
                      {
                        delete_expression(base);
                        delete_unbound_name_manager(manager);
                        free(name_copy);
                        return NULL;
                      }

                    decompose_open_expression(open_filter, &filter_manager,
                                              &filter);
                  }
                else
                  {
                    filter_manager = NULL;
                    filter = NULL;
                  }

                the_verdict = expect_and_eat(the_tokenizer, TK_RIGHT_PAREN);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    if (filter != NULL)
                        delete_expression(filter);
                    if (filter_manager != NULL)
                        delete_unbound_name_manager(filter_manager);
                    delete_expression(base);
                    delete_unbound_name_manager(manager);
                    free(name_copy);
                    return NULL;
                  }

                open_body = parse_expression_with_end_location(the_parser,
                        EPP_TOP, NULL, FALSE, &local_end_location);
                if (open_body == NULL)
                  {
                    if (filter != NULL)
                        delete_expression(filter);
                    if (filter_manager != NULL)
                        delete_unbound_name_manager(filter_manager);
                    delete_expression(base);
                    delete_unbound_name_manager(manager);
                    free(name_copy);
                    return NULL;
                  }

                decompose_open_expression(open_body, &body_manager, &body);

                if (filter_manager != NULL)
                  {
                    verdict the_verdict;

                    the_verdict = merge_in_unbound_name_manager(body_manager,
                            filter_manager);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_expression(body);
                        delete_unbound_name_manager(body_manager);
                        assert(filter != NULL);
                        delete_expression(filter);
                        delete_expression(base);
                        delete_unbound_name_manager(manager);
                        free(name_copy);
                        return NULL;
                      }
                  }

                result = create_comprehend_expression(name_copy, base, filter,
                        body, &element_declaration_location);
                if (result == NULL)
                  {
                    delete_unbound_name_manager(body_manager);
                    delete_unbound_name_manager(manager);
                    free(name_copy);
                    return NULL;
                  }

                if (strcmp(try_aliasing(name_copy, the_parser), name_copy) !=
                    0)
                  {
                    location_warning(&element_declaration_location,
                            "`%s' was used to declare the element of a "
                            "comprehend expression, but that name is aliased "
                            "to `%s', so referencing this element by this name"
                            " will not work.", name_copy,
                            try_aliasing(name_copy, the_parser));
                  }

                the_verdict = bind_variable_name(body_manager, name_copy,
                        comprehend_expression_element(result));
                free(name_copy);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(body_manager);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_verdict = bind_break_and_continue(body_manager, result,
                                                      FALSE, NULL, 0);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(body_manager);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_verdict =
                        merge_in_unbound_name_manager(manager, body_manager);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_expression_end_location(result, &local_end_location);
                if (end_location != NULL)
                    *end_location = local_end_location;

                break;
              }

            if ((strcmp(identifier_string, "forall") == 0) ||
                (strcmp(identifier_string, "exists") == 0))
              {
                boolean is_forall;
                verdict the_verdict;
                unbound_name_manager *static_manager;
                formal_arguments *formals;
                boolean extra_arguments_allowed;
                source_location local_end_location;
                open_expression *open_body;
                unbound_name_manager *body_manager;
                expression *body;

                is_forall = (strcmp(identifier_string, "forall") == 0);

                the_verdict = consume_token(the_tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                manager = NULL;
                static_manager = NULL;

                formals = parse_formal_arguments(the_parser,
                        &extra_arguments_allowed, &static_manager, &manager,
                        NULL);
                if (formals == NULL)
                    return NULL;

                open_body = parse_expression_with_end_location(the_parser,
                        EPP_TOP, NULL, FALSE, &local_end_location);
                if (open_body == NULL)
                  {
                    delete_formal_arguments(formals);
                    if (manager != NULL)
                        delete_unbound_name_manager(manager);
                    if (static_manager != NULL)
                        delete_unbound_name_manager(static_manager);
                    return NULL;
                  }

                decompose_open_expression(open_body, &body_manager, &body);

                if (manager == NULL)
                  {
                    manager = body_manager;
                  }
                else
                  {
                    verdict the_verdict;

                    the_verdict = merge_in_unbound_name_manager(manager,
                                                                body_manager);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_expression(body);
                        delete_formal_arguments(formals);
                        delete_unbound_name_manager(manager);
                        if (static_manager != NULL)
                            delete_unbound_name_manager(static_manager);
                        return NULL;
                      }
                  }

                the_verdict = bind_formals(formals, manager,
                                           the_parser->alias_manager);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(body);
                    delete_formal_arguments(formals);
                    delete_unbound_name_manager(manager);
                    if (static_manager != NULL)
                        delete_unbound_name_manager(static_manager);
                    return NULL;
                  }

                if (static_manager != NULL)
                  {
                    verdict the_verdict;

                    the_verdict = merge_in_unbound_name_manager(manager,
                            static_manager);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_expression(body);
                        delete_formal_arguments(formals);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }
                  }

                if (is_forall)
                    result = create_forall_expression(formals, body);
                else
                    result = create_exists_expression(formals, body);
                if (result == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_expression_end_location(result, &local_end_location);
                if (end_location != NULL)
                    *end_location = local_end_location;

                break;
              }

            result = create_unbound_name_reference_expression();
            if (result == NULL)
                return NULL;

            set_expression_end_location(result, get_token_location(the_token));
            if (end_location != NULL)
                *end_location = *(get_token_location(the_token));

            manager = create_unbound_name_manager();
            if (manager == NULL)
              {
                delete_expression(result);
                return NULL;
              }

            use = add_unbound_variable_reference(manager, identifier_string,
                    result, get_token_location(the_token));
            if (use == NULL)
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            the_verdict = consume_token(the_tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            break;
          }
        case TK_LEFT_PAREN:
          {
            verdict the_verdict;
            open_expression *open_result;
            token_kind next_kind;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return NULL;

            open_result = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
            if (open_result == NULL)
                return NULL;

            if (end_location != NULL)
                *end_location = *(next_token_location(the_parser->tokenizer));

            the_verdict = expect_and_eat2(the_parser->tokenizer,
                    TK_RIGHT_PAREN, TK_DOT_DOT_DOT, &next_kind);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_open_expression(open_result);
                return NULL;
              }

            if (next_kind == TK_DOT_DOT_DOT)
              {
                open_result = parse_range_expression_tail(the_parser, FALSE,
                        open_result, &start_location);
                if (open_result == NULL)
                    return NULL;

                decompose_open_expression(open_result, &manager, &result);

                if (end_location != NULL)
                    *end_location = *(get_expression_location(result));

                break;
              }

            decompose_open_expression(open_result, &manager, &result);

            goto skip_start_set;
          }
        case TK_LEFT_CURLY_BRACE:
          {
            open_statement_block *open_block;
            statement_block *block;
            source_location block_end_location;
            verdict the_verdict;

            open_block = parse_braced_statement_block(the_parser);
            if (open_block == NULL)
                return NULL;

            decompose_open_statement_block(open_block, &manager, &block);

            block_end_location = *get_statement_block_location(block);

            result = create_statement_block_expression(block);
            if (result == NULL)
              {
                delete_unbound_name_manager(manager);
                return NULL;
              }

            set_expression_end_location(result, &block_end_location);
            if (end_location != NULL)
                *end_location = block_end_location;

            the_verdict = bind_return_to_block_expression(manager, result);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            break;
          }
        case TK_SHIFT_LEFT:
          {
            verdict the_verdict;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return NULL;

            result = create_map_list_expression();
            if (result == NULL)
                return NULL;

            manager = create_unbound_name_manager();
            if (manager == NULL)
              {
                delete_expression(result);
                return NULL;
              }

            if (!(next_is(the_parser->tokenizer, TK_SHIFT_RIGHT)))
              {
                while (TRUE)
                  {
                    verdict the_verdict;
                    expression *key_value_expression;
                    type_expression *key_type_expression;
                    open_expression *open_target;
                    unbound_name_manager *child_manager;
                    expression *target;

                    the_verdict = expect_and_eat(the_parser->tokenizer,
                                                 TK_LEFT_PAREN);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    if (next_is(the_parser->tokenizer, TK_STAR))
                      {
                        verdict the_verdict;

                        the_verdict = consume_token(the_parser->tokenizer);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            delete_expression(result);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        key_value_expression = NULL;

                        if (next_is(the_parser->tokenizer, TK_COLON))
                          {
                            verdict the_verdict;
                            open_type_expression *open_type;
                            unbound_name_manager *child_manager;

                            the_verdict = consume_token(the_parser->tokenizer);
                            if (the_verdict != MISSION_ACCOMPLISHED)
                              {
                                delete_expression(result);
                                delete_unbound_name_manager(manager);
                                return NULL;
                              }

                            open_type = parse_type_expression(the_parser,
                                                              TEPP_TYPE);
                            if (open_type == NULL)
                              {
                                delete_expression(result);
                                delete_unbound_name_manager(manager);
                                return NULL;
                              }

                            decompose_open_type_expression(open_type,
                                    &child_manager, &key_type_expression);

                            the_verdict = merge_in_unbound_name_manager(
                                    manager, child_manager);
                            if (the_verdict != MISSION_ACCOMPLISHED)
                              {
                                delete_type_expression(key_type_expression);
                                delete_expression(result);
                                delete_unbound_name_manager(manager);
                                return NULL;
                              }
                          }
                        else
                          {
                            key_type_expression = NULL;
                          }
                      }
                    else
                      {
                        open_expression *open_key;
                        unbound_name_manager *child_manager;

                        open_key = parse_expression(the_parser, EPP_TOP, NULL,
                                                    FALSE);
                        if (open_key == NULL)
                          {
                            delete_expression(result);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        decompose_open_expression(open_key, &child_manager,
                                                  &key_value_expression);

                        the_verdict = merge_in_unbound_name_manager(manager,
                                child_manager);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            delete_expression(key_value_expression);
                            delete_expression(result);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        key_type_expression = NULL;
                      }

                    the_verdict =
                            expect_and_eat(the_parser->tokenizer, TK_MAPS_TO);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        if (key_value_expression != NULL)
                            delete_expression(key_value_expression);
                        if (key_type_expression != NULL)
                            delete_type_expression(key_type_expression);
                        delete_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    open_target =
                            parse_expression(the_parser, EPP_TOP, NULL, FALSE);
                    if (open_target == NULL)
                      {
                        if (key_value_expression != NULL)
                            delete_expression(key_value_expression);
                        if (key_type_expression != NULL)
                            delete_type_expression(key_type_expression);
                        delete_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    the_verdict = expect_and_eat(the_parser->tokenizer,
                                                 TK_RIGHT_PAREN);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_open_expression(open_target);
                        if (key_value_expression != NULL)
                            delete_expression(key_value_expression);
                        if (key_type_expression != NULL)
                            delete_type_expression(key_type_expression);
                        delete_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    decompose_open_expression(open_target, &child_manager,
                                              &target);

                    the_verdict = merge_in_unbound_name_manager(manager,
                                                                child_manager);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_expression(target);
                        if (key_value_expression != NULL)
                            delete_expression(key_value_expression);
                        if (key_type_expression != NULL)
                            delete_type_expression(key_type_expression);
                        delete_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    if (key_value_expression != NULL)
                      {
                        the_verdict = add_map_list_expression_component(result,
                                key_value_expression, target);
                      }
                    else
                      {
                        if (key_type_expression == NULL)
                          {
                            type *anything_type;

                            anything_type = get_anything_type();
                            if (anything_type == NULL)
                              {
                                delete_expression(target);
                                delete_expression(result);
                                delete_unbound_name_manager(manager);
                                return NULL;
                              }

                            key_type_expression =
                                    create_constant_type_expression(
                                            anything_type);
                            if (key_type_expression == NULL)
                              {
                                delete_expression(target);
                                delete_expression(result);
                                delete_unbound_name_manager(manager);
                                return NULL;
                              }
                          }

                        the_verdict = add_map_list_expression_filter_component(
                                result, key_type_expression, target);
                      }

                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    if (next_is(the_parser->tokenizer, TK_SHIFT_RIGHT))
                        break;

                    the_verdict =
                            expect_and_eat(the_parser->tokenizer, TK_COMMA);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }
                  }
              }

            assert(next_is(the_parser->tokenizer, TK_SHIFT_RIGHT));

            set_expression_end_location(result,
                    next_token_location(the_parser->tokenizer));
            if (end_location != NULL)
                *end_location = *(next_token_location(the_parser->tokenizer));

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            break;
          }
        case TK_LEFT_BRACKET:
          {
            open_expression *open_result;

            open_result =
                    parse_semi_labeled_expression_list_expression(the_parser);
            if (open_result == NULL)
                return NULL;

            decompose_open_expression(open_result, &manager, &result);

            if (end_location != NULL)
                *end_location = *(get_expression_location(result));

            break;
          }
        case TK_STAR:
        case TK_AMPERSAND:
        case TK_DASH:
        case TK_ADD:
        case TK_BITWISE_NOT:
        case TK_LOGICAL_NOT:
          {
            source_location start_location;
            verdict the_verdict;
            open_expression *open_result;
            source_location base_end_location;
            expression_kind the_expression_kind;

            start_location = *(next_token_location(the_parser->tokenizer));

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return NULL;

            open_result = parse_expression_with_end_location(the_parser,
                    token_prefix_precedence(kind), NULL, FALSE,
                    &base_end_location);
            if (open_result == NULL)
                return NULL;

            decompose_open_expression(open_result, &manager, &result);

            if ((kind == TK_AMPERSAND) &&
                (!(expression_is_addressable(result))))
              {
                location_error(&start_location,
                        "Syntax error -- operand of unary location-of operator"
                        " is not addressable.");
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            if (kind == TK_AMPERSAND)
                set_expression_addressable_required(result);

            the_expression_kind = token_unary_prefix_expression_kind(kind);
            result = create_unary_expression(the_expression_kind, result);
            if (result == NULL)
              {
                delete_unbound_name_manager(manager);
                return NULL;
              }

            set_expression_end_location(result, &base_end_location);
            if (end_location != NULL)
                *end_location = base_end_location;

            if (the_expression_kind != EK_LOCATION_OF)
              {
                unbound_use *use;

                use = add_aliased_unbound_operator_expression(manager,
                        expression_kind_operator_name(the_expression_kind),
                        result, the_parser, &start_location);
                if (use == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    delete_expression(result);
                    return NULL;
                  }
              }

            break;
          }
        default:
          {
            token_error(the_token, "Syntax error -- expected expression.");
            return NULL;
          }
      }

    while (TRUE)
      {
        token *the_token;
        token_kind kind;

        set_expression_start_location(result, &start_location);

      skip_start_set:
        the_token = next_token(the_tokenizer);
        if (the_token == NULL)
          {
            delete_expression(result);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        kind = get_token_kind(the_token);

        if ((token_postfix_precedence(kind) < precedence) ||
            ((token_postfix_precedence(kind) == precedence) &&
             token_is_left_associative(kind)))
          {
            break;
          }

        if (token_is_unary_postfix_operator(kind))
          {
            source_location suffix_end_location;
            verdict the_verdict;

            suffix_end_location =
                    *(next_token_location(the_parser->tokenizer));

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            result = create_unary_expression(
                    token_unary_postfix_expression_kind(kind), result);
            if (result == NULL)
              {
                delete_unbound_name_manager(manager);
                return NULL;
              }

            set_expression_end_location(result, &suffix_end_location);
            if (end_location != NULL)
                *end_location = suffix_end_location;

            if (naked_call_overloading_use != NULL)
                *naked_call_overloading_use = NULL;

            continue;
          }

        if (token_is_binary_operator(kind))
          {
            source_location operator_location;
            verdict the_verdict;
            open_expression *open_operand2;
            unbound_name_manager *manager2;
            expression *operand2;
            source_location operand2_end_location;
            expression_kind the_expression_kind;

            operator_location = *(next_token_location(the_parser->tokenizer));

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            open_operand2 = parse_expression_with_end_location(the_parser,
                    token_postfix_precedence(kind), NULL, FALSE,
                    &operand2_end_location);
            if (open_operand2 == NULL)
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            decompose_open_expression(open_operand2, &manager2, &operand2);

            the_verdict = merge_in_unbound_name_manager(manager, manager2);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(operand2);
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            the_expression_kind = token_binary_expression_kind(kind);

            result = create_binary_expression(the_expression_kind, result,
                                              operand2);
            if (result == NULL)
              {
                delete_unbound_name_manager(manager);
                return NULL;
              }

            set_expression_end_location(result, &operand2_end_location);
            if (end_location != NULL)
                *end_location = operand2_end_location;

            if ((the_expression_kind != EK_LOGICAL_AND) &&
                (the_expression_kind != EK_LOGICAL_OR))
              {
                unbound_use *use;

                use = add_aliased_unbound_operator_expression(manager,
                        expression_kind_operator_name(the_expression_kind),
                        result, the_parser, &operator_location);
                if (use == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    delete_expression(result);
                    return NULL;
                  }
              }

            if (naked_call_overloading_use != NULL)
                *naked_call_overloading_use = NULL;

            continue;
          }

        switch (kind)
          {
            case TK_IDENTIFIER:
              {
                const char *identifier_string;

                identifier_string = aliased_token_name(the_token, the_parser);

                if (strcmp(identifier_string, "in") == 0)
                  {
                    verdict the_verdict;
                    source_location type_end_location;
                    open_type_expression *open_type;
                    unbound_name_manager *type_manager;
                    type_expression *type;

                    the_verdict = consume_token(the_parser->tokenizer);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    open_type = parse_type_expression_with_end_location(
                            the_parser, TEPP_TOP, &type_end_location);
                    if (open_type == NULL)
                      {
                        delete_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    decompose_open_type_expression(open_type, &type_manager,
                                                   &type);

                    the_verdict = merge_in_unbound_name_manager(manager,
                                                                type_manager);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_type_expression(type);
                        delete_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    result = create_in_expression(result, type);
                    if (result == NULL)
                      {
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    set_expression_end_location(result, &type_end_location);
                    if (end_location != NULL)
                        *end_location = type_end_location;

                    if (naked_call_overloading_use != NULL)
                        *naked_call_overloading_use = NULL;

                    continue;
                  }

                break;
              }
            case TK_LEFT_BRACKET:
              {
                token *second_token;
                open_expression *the_open_expression;

                second_token = forward_token(the_parser->tokenizer, 1);
                if ((second_token == NULL) ||
                    (get_token_kind(second_token) == TK_ERROR))
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                if (get_token_kind(second_token) == TK_RIGHT_BRACKET)
                  {
                    open_expression *the_open_expression;

                    the_open_expression = parse_lepton_expression_tail(
                            the_parser, result, manager);
                    if (the_open_expression == NULL)
                        return NULL;

                    decompose_open_expression(the_open_expression, &manager,
                                              &result);

                    if (end_location != NULL)
                        *end_location = *(get_expression_location(result));

                    if (naked_call_overloading_use != NULL)
                        *naked_call_overloading_use = NULL;

                    continue;
                  }

                if (get_token_kind(second_token) == TK_IDENTIFIER)
                  {
                    token *third_token;

                    third_token = forward_token(the_parser->tokenizer, 2);
                    if ((third_token == NULL) ||
                        (get_token_kind(third_token) == TK_ERROR))
                      {
                        delete_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    if ((get_token_kind(third_token) == TK_ASSIGN) ||
                        (get_token_kind(third_token) == TK_MODULO_ASSIGN))
                      {
                        open_expression *the_open_expression;

                        the_open_expression = parse_lepton_expression_tail(
                                the_parser, result, manager);
                        if (the_open_expression == NULL)
                            return NULL;

                        decompose_open_expression(the_open_expression,
                                                  &manager, &result);

                        if (end_location != NULL)
                            *end_location = *(get_expression_location(result));

                        if (naked_call_overloading_use != NULL)
                            *naked_call_overloading_use = NULL;

                        continue;
                      }
                  }

                the_open_expression = parse_lookup_expression_tail(the_parser,
                        result, manager);
                if (the_open_expression == NULL)
                    return NULL;

                decompose_open_expression(the_open_expression, &manager,
                                          &result);

                if (end_location != NULL)
                    *end_location = *(get_expression_location(result));

                if (naked_call_overloading_use != NULL)
                    *naked_call_overloading_use = NULL;

                continue;
              }
            case TK_DOT:
            case TK_POINTS_TO:
              {
                source_location operator_location;
                verdict the_verdict;

                operator_location =
                        *(next_token_location(the_parser->tokenizer));

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_verdict = expect(the_parser->tokenizer, TK_IDENTIFIER);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_token = next_token(the_tokenizer);
                assert(the_token != NULL);

                if (kind == TK_DOT)
                  {
                    result = create_field_expression(result,
                            identifier_token_name(the_token));
                  }
                else
                  {
                    assert(kind == TK_POINTS_TO);
                    result = create_pointer_field_expression(result,
                            identifier_token_name(the_token));
                  }
                if (result == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_expression_end_location(result,
                                            get_token_location(the_token));
                if (end_location != NULL)
                    *end_location = *(get_token_location(the_token));

                if (kind == TK_POINTS_TO)
                  {
                    unbound_use *use;

                    use = add_aliased_unbound_operator_expression(manager,
                            "operator->", result, the_parser,
                            &operator_location);
                    if (use == NULL)
                      {
                        delete_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }
                  }

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                if (naked_call_overloading_use != NULL)
                    *naked_call_overloading_use = NULL;

                continue;
              }
            case TK_DOT_DOT:
              {
                verdict the_verdict;
                source_location operand2_end_location;
                open_expression *open_operand2;
                unbound_name_manager *manager2;
                expression *operand2;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                open_operand2 = parse_expression_with_end_location(the_parser,
                        EPP_TAGALONG, NULL, FALSE, &operand2_end_location);
                if (open_operand2 == NULL)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                decompose_open_expression(open_operand2, &manager2, &operand2);

                the_verdict = merge_in_unbound_name_manager(manager, manager2);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(operand2);
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                result = create_tagalong_field_expression(result, operand2);
                if (result == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_expression_end_location(result, &operand2_end_location);
                if (end_location != NULL)
                    *end_location = operand2_end_location;

                if (naked_call_overloading_use != NULL)
                    *naked_call_overloading_use = NULL;

                continue;
              }
            case TK_LEFT_PAREN:
              {
                source_location operator_location;
                open_call *the_open_call;
                call *the_call;
                unbound_use *use;

                operator_location =
                        *(next_token_location(the_parser->tokenizer));

                the_open_call = parse_call_suffix(the_parser, result, manager);
                if (the_open_call == NULL)
                    return NULL;

                decompose_open_call(the_open_call, &manager, &the_call);

                set_call_start_location(the_call, &start_location);

                result = create_call_expression(the_call);
                if (result == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_expression_end_location(result,
                                            get_call_location(the_call));
                if (end_location != NULL)
                    *end_location = *(get_call_location(the_call));

                use = add_aliased_unbound_operator_expression(manager,
                        "operator()", result, the_parser, &operator_location);
                if (use == NULL)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                if (naked_call_overloading_use != NULL)
                    *naked_call_overloading_use = use;

                if (stop_with_call)
                  {
                    return create_open_expression(result, manager);
                  }

                continue;
              }
            case TK_QUESTION_MARK:
              {
                verdict the_verdict;
                open_expression *open_operand2;
                source_location operand3_end_location;
                open_expression *open_operand3;
                unbound_name_manager *manager2;
                expression *operand2;
                expression *operand3;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                open_operand2 =
                        parse_expression(the_parser, EPP_TOP, NULL, FALSE);
                if (open_operand2 == NULL)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_verdict = expect_and_eat(the_parser->tokenizer, TK_COLON);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_open_expression(open_operand2);
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                open_operand3 = parse_expression_with_end_location(the_parser,
                        EPP_CONDITIONAL, NULL, FALSE, &operand3_end_location);
                if (open_operand3 == NULL)
                  {
                    delete_open_expression(open_operand2);
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                decompose_open_expression(open_operand2, &manager2, &operand2);

                the_verdict = merge_in_unbound_name_manager(manager, manager2);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_open_expression(open_operand3);
                    delete_expression(operand2);
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                decompose_open_expression(open_operand3, &manager2, &operand3);

                the_verdict = merge_in_unbound_name_manager(manager, manager2);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(operand3);
                    delete_expression(operand2);
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                result = create_conditional_expression(result, operand2,
                                                       operand3);
                if (result == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_expression_end_location(result, &operand3_end_location);
                if (end_location != NULL)
                    *end_location = operand3_end_location;

                if (naked_call_overloading_use != NULL)
                    *naked_call_overloading_use = NULL;

                continue;
              }
            case TK_FORCE:
              {
                source_location operator_location;
                verdict the_verdict;
                source_location type_end_location;
                open_type_expression *open_type;
                unbound_name_manager *type_manager;
                type_expression *type;
                unbound_use *use;

                operator_location =
                        *(next_token_location(the_parser->tokenizer));

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                open_type = parse_type_expression_with_end_location(the_parser,
                        TEPP_TOP, &type_end_location);
                if (open_type == NULL)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                decompose_open_type_expression(open_type, &type_manager,
                                               &type);

                the_verdict =
                        merge_in_unbound_name_manager(manager, type_manager);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(type);
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                result = create_force_expression(result, type);
                if (result == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_expression_end_location(result, &type_end_location);
                if (end_location != NULL)
                    *end_location = type_end_location;

                use = add_aliased_unbound_operator_expression(manager,
                        "operator::", result, the_parser, &operator_location);
                if (use == NULL)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                if (naked_call_overloading_use != NULL)
                    *naked_call_overloading_use = NULL;

                continue;
              }
            default:
              {
                break;
              }
          }

        break;
      }

    return create_open_expression(result, manager);
  }

/*
 *      <lookup-expression> :
 *          <expression> "[" <lookup-element-list> "]"
 *
 *      <lookup-element-list> :
 *          <lookup-element> |
 *          <lookup-element> "," <lookup-element-list>
 *
 *      <lookup-element> :
 *          <expression> |
 *          <expression> "..." <expression> |
 *          "*" { ":" <type-expression> }?
 */
extern open_expression *parse_lookup_expression_tail(parser *the_parser,
        expression *base, unbound_name_manager *manager)
  {
    expression *result;
    unbound_use *use;
    verdict the_verdict;

    assert(the_parser != NULL);
    assert(base != NULL);
    assert(manager != NULL);

    result = create_lookup_expression(base);
    if (result == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    use = add_aliased_unbound_operator_expression(manager, "operator[]",
            result, the_parser, next_token_location(the_parser->tokenizer));
    if (use == NULL)
      {
        delete_expression(result);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_LEFT_BRACKET);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_expression(result);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    while (TRUE)
      {
        expression *lower;
        expression *upper;
        type_expression *filter_type;
        verdict the_verdict;
        token *the_token;

        if (next_is(the_parser->tokenizer, TK_STAR))
          {
            verdict the_verdict;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            if (next_is(the_parser->tokenizer, TK_COLON))
              {
                verdict the_verdict;
                open_type_expression *open_filter;
                unbound_name_manager *child_manager;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                open_filter = parse_type_expression(the_parser, TEPP_TOP);
                if (open_filter == NULL)
                  {
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                decompose_open_type_expression(open_filter, &child_manager,
                                               &filter_type);

                the_verdict =
                        merge_in_unbound_name_manager(manager, child_manager);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(filter_type);
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }
              }
            else
              {
                filter_type = NULL;
              }

            lower = NULL;
            upper = NULL;
          }
        else
          {
            open_expression *open_lower;
            unbound_name_manager *child_manager;
            verdict the_verdict;

            open_lower = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
            if (open_lower == NULL)
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            decompose_open_expression(open_lower, &child_manager, &lower);

            the_verdict =
                    merge_in_unbound_name_manager(manager, child_manager);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(lower);
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            if (next_is(the_parser->tokenizer, TK_DOT_DOT_DOT))
              {
                verdict the_verdict;
                open_expression *open_upper;
                unbound_name_manager *child_manager;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(lower);
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                open_upper =
                        parse_expression(the_parser, EPP_TOP, NULL, FALSE);
                if (open_upper == NULL)
                  {
                    delete_expression(lower);
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                decompose_open_expression(open_upper, &child_manager, &upper);

                the_verdict =
                        merge_in_unbound_name_manager(manager, child_manager);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(upper);
                    delete_expression(lower);
                    delete_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }
              }
            else
              {
                upper = NULL;
              }

            filter_type = NULL;
          }

        if (filter_type == NULL)
          {
            the_verdict = add_lookup_component(result, lower, upper);
          }
        else
          {
            assert(lower == NULL);
            assert(upper == NULL);
            the_verdict = add_lookup_filter_component(result, filter_type);
          }
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_expression(result);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        the_token = next_token(the_parser->tokenizer);
        if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
          {
            delete_expression(result);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        if (get_token_kind(the_token) == TK_RIGHT_BRACKET)
            break;

        if (get_token_kind(the_token) != TK_COMMA)
          {
            boolean dot_dot_dot_ok;

            dot_dot_dot_ok = ((lower != NULL) && (upper == NULL));
            token_error(the_token, "Syntax error -- expected %s%s%s%s or %s.",
                    name_for_token_kind(TK_COMMA),
                    (dot_dot_dot_ok ? ", " : ""),
                    (dot_dot_dot_ok ? name_for_token_kind(TK_DOT_DOT_DOT) :
                                      ""), (dot_dot_dot_ok ? "," : ""),
                    name_for_token_kind(TK_RIGHT_BRACKET));
            delete_expression(result);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_expression(result);
            delete_unbound_name_manager(manager);
            return NULL;
          }
      }

    set_expression_end_location(result,
                                next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_RIGHT_BRACKET);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_expression(result);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    return create_open_expression(result, manager);
  }

/*
 *      <lepton-expression> :
 *          <expression> "[" <lepton-element-list> "]"
 *
 *      <lepton-element-list> :
 *          <empty> |
 *          <non-empty-lepton-element-list>
 *
 *      <non-empty-lepton-element-list> :
 *          <lepton-element> |
 *          <lepton-element> "," <non-empty-lepton-element-list>
 *
 *      <lepton-element> :
 *          <identifier> { ":=" | "::=" } <expression>
 */
extern open_expression *parse_lepton_expression_tail(parser *the_parser,
        expression *base, unbound_name_manager *manager)
  {
    expression *result;
    verdict the_verdict;

    assert(the_parser != NULL);
    assert(base != NULL);
    assert(manager != NULL);

    result = create_lepton_expression(base);
    if (result == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_LEFT_BRACKET);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_expression(result);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    if (!(next_is(the_parser->tokenizer, TK_RIGHT_BRACKET)))
      {
        while (TRUE)
          {
            char *name;
            token *the_token;
            token_kind kind;
            boolean force;
            verdict the_verdict;
            open_expression *open_field;
            expression *field_expression;
            unbound_name_manager *child_manager;

            name = expect_and_eat_identifier_return_name_copy(the_parser);
            if (name == NULL)
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            the_token = next_token(the_parser->tokenizer);
            if (the_token == NULL)
              {
                free(name);
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            kind = get_token_kind(the_token);

            if (next_is(the_parser->tokenizer, TK_ASSIGN))
              {
                force = FALSE;
              }
            else if (next_is(the_parser->tokenizer, TK_MODULO_ASSIGN))
              {
                force = TRUE;
              }
            else
              {
                if (kind != TK_ERROR)
                  {
                    token_error(the_token,
                            "Syntax error -- expected %s or %s, found %s.",
                            name_for_token_kind(TK_ASSIGN),
                            name_for_token_kind(TK_MODULO_ASSIGN),
                            name_for_token_kind(kind));
                  }
                free(name);
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                free(name);
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            open_field = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
            if (open_field == NULL)
              {
                free(name);
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            decompose_open_expression(open_field, &child_manager,
                                      &field_expression);

            the_verdict =
                    merge_in_unbound_name_manager(manager, child_manager);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(field_expression);
                free(name);
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            the_verdict = add_lepton_component(result, name, field_expression,
                                               force);
            free(name);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            the_token = next_token(the_parser->tokenizer);
            if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            if (get_token_kind(the_token) == TK_RIGHT_BRACKET)
                break;

            if (get_token_kind(the_token) != TK_COMMA)
              {
                token_error(the_token, "Syntax error -- expected %s or %s.",
                        name_for_token_kind(TK_COMMA),
                        name_for_token_kind(TK_RIGHT_BRACKET));
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }
          }
      }

    set_expression_end_location(result,
                                next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_RIGHT_BRACKET);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_expression(result);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    return create_open_expression(result, manager);
  }

extern open_expression *parse_routine_or_new_expression(parser *the_parser,
        expression *single_lock, unbound_name_manager *single_lock_manager)
  {
    source_location start_location;
    unbound_name_manager *manager;
    declaration *the_declaration;
    expression *result;

    assert(the_parser != NULL);
    assert((single_lock == NULL) == (single_lock_manager == NULL));

    start_location = *(next_token_location(the_parser->tokenizer));

    the_declaration = parse_declaration(the_parser, FALSE, FALSE, FALSE, FALSE,
            single_lock, single_lock_manager, &manager, FALSE, TRUE, NULL,
            PURE_UNSAFE, NULL, NULL, NULL, NULL);
    if (the_declaration == NULL)
        return NULL;

    if (declaration_name(the_declaration) != NULL)
      {
        location_error(&start_location,
                "Syntax error -- in a declaration expression, a name was given"
                " for the declaration.");
        declaration_remove_reference(the_declaration);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    result = create_declaration_expression(the_declaration);
    if (result == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    set_expression_end_location(result,
                                get_declaration_location(the_declaration));

    return create_open_expression(result, manager);
  }

extern open_expression *parse_construct_expression(parser *the_parser,
        expression *single_lock, unbound_name_manager *single_lock_manager)
  {
    verdict the_verdict;
    open_expression *the_open_expression;
    unbound_name_manager *manager;
    expression *call_expression;
    unbound_use *naked_call_overloading_use;
    call *the_call;
    type *the_type;
    type_expression *the_type_expression;
    expression *the_call_expression;
    unbound_use *use;
    variable_declaration *the_variable_declaration;
    declaration *the_declaration;
    expression *result;

    assert(the_parser != NULL);
    assert((single_lock == NULL) == (single_lock_manager == NULL));

    the_verdict = expect_and_eat_keyword(the_parser, "construct");
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (single_lock != NULL)
            delete_expression(single_lock);
        if (single_lock_manager != NULL)
            delete_unbound_name_manager(single_lock_manager);
        return NULL;
      }

    the_open_expression = parse_expression(the_parser, EPP_TOP,
                                           &naked_call_overloading_use, TRUE);
    if (the_open_expression == NULL)
      {
        if (single_lock != NULL)
            delete_expression(single_lock);
        if (single_lock_manager != NULL)
            delete_unbound_name_manager(single_lock_manager);
        return NULL;
      }

    decompose_open_expression(the_open_expression, &manager, &call_expression);

    if (naked_call_overloading_use == NULL)
      {
        expression_error(call_expression,
                "Syntax error -- bad syntax for `construct' expression.");
        delete_expression(call_expression);
        delete_unbound_name_manager(manager);
        if (single_lock != NULL)
            delete_expression(single_lock);
        if (single_lock_manager != NULL)
            delete_unbound_name_manager(single_lock_manager);
        return NULL;
      }

    remove_unbound_use(naked_call_overloading_use);

    assert(get_expression_kind(call_expression) == EK_CALL);
    the_call = call_expression_call(call_expression);
    assert(the_call != NULL);

    delete_call_expression_save_call(call_expression);

    if (single_lock_manager != NULL)
      {
        verdict the_verdict;

        the_verdict =
                merge_in_unbound_name_manager(single_lock_manager, manager);
        manager = single_lock_manager;
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_call(the_call);
            delete_unbound_name_manager(manager);
            if (single_lock != NULL)
                delete_expression(single_lock);
            return NULL;
          }
      }

    the_type = get_anything_type();
    if (the_type == NULL)
      {
        delete_call(the_call);
        if (single_lock != NULL)
            delete_expression(single_lock);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_type_expression = create_constant_type_expression(the_type);
    if (the_type_expression == NULL)
      {
        delete_call(the_call);
        if (single_lock != NULL)
            delete_expression(single_lock);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_call_expression = create_call_expression(the_call);
    if (the_call_expression == NULL)
      {
        delete_type_expression(the_type_expression);
        if (single_lock != NULL)
            delete_expression(single_lock);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    use = add_aliased_unbound_operator_expression(manager, "operator()",
            the_call_expression, the_parser, get_call_location(the_call));
    if (use == NULL)
      {
        delete_expression(the_call_expression);
        delete_type_expression(the_type_expression);
        if (single_lock != NULL)
            delete_expression(single_lock);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_variable_declaration = create_variable_declaration(the_type_expression,
            the_call_expression, FALSE, TRUE, single_lock);
    if (the_variable_declaration == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_declaration = create_declaration_for_variable(NULL, FALSE, FALSE,
            FALSE, the_variable_declaration, get_call_location(the_call));
    if (the_declaration == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    result = create_declaration_expression(the_declaration);
    if (result == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    set_expression_end_location(result,
                                get_declaration_location(the_declaration));

    return create_open_expression(result, manager);
  }

/*
 *      <semi-labeled-expression-list-expression> :
 *          "[" <semi-labeled-expression-list> "]"
 *
 *      <semi-labeled-expression-list> :
 *          <empty> |
 *          <non-empty-semi-labeled-expression-list>
 *
 *      <non-empty-semi-labeled-expression-list> :
 *          <semi-labeled-expression> |
 *          <semi-labeled-expression> ","
 *                  <non-empty-semi-labeled-expression-list>
 *
 *      <semi-labeled-expression> :
 *          { <identifier> ":=" }? { <expression> }?
 */
extern open_expression *parse_semi_labeled_expression_list_expression(
        parser *the_parser)
  {
    tokenizer *the_tokenizer;
    expression *the_expression;
    unbound_name_manager *manager;
    open_expression *the_open_expression;
    token *the_token;
    verdict the_verdict;
    token_kind kind;

    assert(the_parser != NULL);

    the_tokenizer = the_parser->tokenizer;
    assert(the_tokenizer != NULL);

    the_expression = create_semi_labeled_expression_list_expression();
    if (the_expression == NULL)
        return NULL;

    manager = create_unbound_name_manager();
    if (manager == NULL)
      {
        delete_expression(the_expression);
        return NULL;
      }

    the_open_expression = create_open_expression(the_expression, manager);
    if (the_open_expression == NULL)
        return NULL;

    the_token = next_token(the_tokenizer);
    if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
      {
        delete_open_expression(the_open_expression);
        return NULL;
      }

    set_expression_start_location(the_expression,
                                  get_token_location(the_token));

    the_verdict = expect_and_eat(the_tokenizer, TK_LEFT_BRACKET);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_expression(the_open_expression);
        return NULL;
      }

    the_token = next_token(the_tokenizer);
    if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
      {
        delete_open_expression(the_open_expression);
        return NULL;
      }

    kind = get_token_kind(the_token);

    if (kind != TK_RIGHT_BRACKET)
      {
        boolean first;

        first = TRUE;

        while (TRUE)
          {
            char *label;
            open_expression *open_argument;
            unbound_name_manager *argument_manager;
            expression *argument;
            verdict the_verdict;
            token *the_token;
            token_kind kind;

            label = NULL;

            if (next_is(the_parser->tokenizer, TK_IDENTIFIER))
              {
                token *second_token;

                second_token = forward_token(the_parser->tokenizer, 1);
                if ((second_token == NULL) ||
                    (get_token_kind(second_token) == TK_ERROR))
                  {
                    delete_open_expression(the_open_expression);
                    return NULL;
                  }

                if (get_token_kind(second_token) == TK_ASSIGN)
                  {
                    token *the_token;
                    const char *id_chars;
                    verdict the_verdict;

                    the_token = next_token(the_tokenizer);
                    assert(the_token != NULL);

                    id_chars = identifier_token_name(the_token);

                    label = MALLOC_ARRAY(char, strlen(id_chars) + 1);
                    if (label == NULL)
                      {
                        delete_open_expression(the_open_expression);
                        return NULL;
                      }

                    strcpy(label, id_chars);

                    the_verdict = consume_token(the_tokenizer);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        free(label);
                        delete_open_expression(the_open_expression);
                        return NULL;
                      }

                    the_verdict = consume_token(the_tokenizer);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        free(label);
                        delete_open_expression(the_open_expression);
                        return NULL;
                      }
                  }
              }

            open_argument = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
            if (open_argument == NULL)
              {
                if (label != NULL)
                    free(label);
                delete_open_expression(the_open_expression);
                return NULL;
              }

            if ((label == NULL) && first &&
                next_is(the_parser->tokenizer, TK_DOT_DOT_DOT))
              {
                source_location start_location;
                verdict the_verdict;

                start_location = *(get_expression_location(the_expression));

                delete_open_expression(the_open_expression);

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_open_expression(open_argument);
                    return NULL;
                  }

                return parse_range_expression_tail(the_parser, TRUE,
                        open_argument, &start_location);
              }

            first = FALSE;

            decompose_open_expression(open_argument, &argument_manager,
                                      &argument);

            the_verdict =
                    merge_in_unbound_name_manager(manager, argument_manager);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(argument);
                if (label != NULL)
                    free(label);
                delete_open_expression(the_open_expression);
                return NULL;
              }

            the_verdict =
                    add_semi_labeled_expression_list_expression_component(
                            the_expression, label, argument);
            if (label != NULL)
                free(label);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_open_expression(the_open_expression);
                return NULL;
              }

            the_token = next_token(the_tokenizer);
            if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
              {
                delete_open_expression(the_open_expression);
                return NULL;
              }

            kind = get_token_kind(the_token);

            if (kind == TK_RIGHT_BRACKET)
                break;

            if (kind != TK_COMMA)
              {
                token_error(the_token, "Syntax error -- expected %s or %s.",
                        name_for_token_kind(TK_COMMA),
                        name_for_token_kind(TK_RIGHT_BRACKET));
                delete_open_expression(the_open_expression);
                return NULL;
              }

            the_verdict = consume_token(the_tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_open_expression(the_open_expression);
                return NULL;
              }
          }
      }

    set_expression_end_location(the_expression,
                                next_token_location(the_tokenizer));

    the_verdict = consume_token(the_tokenizer);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_expression(the_open_expression);
        return NULL;
      }

    return the_open_expression;
  }

extern open_basket *parse_basket(parser *the_parser)
  {
    token *the_token;

    assert(the_parser != NULL);

    the_token = next_token(the_parser->tokenizer);
    if (the_token == NULL)
        return NULL;

    if (get_token_kind(the_token) == TK_ERROR)
        return NULL;

    if (get_token_kind(the_token) == TK_LEFT_BRACKET)
        return parse_list_basket(the_parser);
    else
        return parse_expression_basket(the_parser);
  }

extern open_basket *parse_expression_basket(parser *the_parser)
  {
    open_expression *the_open_expression;
    unbound_name_manager *manager;
    expression *the_expression;
    basket *the_basket;

    assert(the_parser != NULL);

    the_open_expression = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
    if (the_open_expression == NULL)
        return NULL;

    decompose_open_expression(the_open_expression, &manager, &the_expression);

    if (!(expression_is_addressable(the_expression)))
      {
        expression_error(the_expression,
                "Syntax error -- assignment basket expression is not "
                "addressable.");
        delete_expression(the_expression);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    set_expression_addressable_required(the_expression);

    the_basket = create_expression_basket(the_expression);
    if (the_basket == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    return create_open_basket(the_basket, manager);
  }

extern open_basket *parse_list_basket(parser *the_parser)
  {
    verdict the_verdict;
    basket *the_basket;
    unbound_name_manager *manager;

    assert(the_parser != NULL);

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_LEFT_BRACKET);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    the_basket = create_list_basket();
    if (the_basket == NULL)
        return NULL;

    manager = create_unbound_name_manager();
    if (manager == NULL)
      {
        delete_basket(the_basket);
        return NULL;
      }

    while (TRUE)
      {
        token *the_token;
        open_basket *child_open_basket;
        unbound_name_manager *child_manager;
        basket *child_basket;
        const char *label;

        the_token = next_token(the_parser->tokenizer);
        if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
          {
            delete_unbound_name_manager(manager);
            delete_basket(the_basket);
            return NULL;
          }

        if ((get_token_kind(the_token) == TK_COMMA) ||
            (get_token_kind(the_token) == TK_RIGHT_BRACKET) ||
            (get_token_kind(the_token) == TK_ASSIGN) ||
            (get_token_kind(the_token) == TK_MODULO_ASSIGN))
          {
            const char *label;
            verdict the_verdict;

            if ((get_token_kind(the_token) == TK_ASSIGN) ||
                (get_token_kind(the_token) == TK_MODULO_ASSIGN))
              {
                token *second_token;

                second_token = forward_token(the_parser->tokenizer, 1);
                if ((second_token == NULL) ||
                    (get_token_kind(second_token) == TK_ERROR))
                  {
                    delete_unbound_name_manager(manager);
                    delete_basket(the_basket);
                    return NULL;
                  }

                if (get_token_kind(second_token) != TK_IDENTIFIER)
                  {
                    token_error(second_token,
                            "Syntax error -- expected identifier, found %s.",
                            name_for_token_kind(get_token_kind(second_token)));
                    delete_unbound_name_manager(manager);
                    delete_basket(the_basket);
                    return NULL;
                  }

                label = identifier_token_name(second_token);
              }
            else
              {
                label = NULL;
              }

            the_verdict = basket_add_sub_basket(the_basket, label, NULL,
                    (get_token_kind(the_token) == TK_MODULO_ASSIGN));
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_basket(the_basket);
                return NULL;
              }

            if ((get_token_kind(the_token) == TK_ASSIGN) ||
                (get_token_kind(the_token) == TK_MODULO_ASSIGN))
              {
                verdict the_verdict;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_unbound_name_manager(manager);
                    delete_basket(the_basket);
                    return NULL;
                  }

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_unbound_name_manager(manager);
                    delete_basket(the_basket);
                    return NULL;
                  }
              }

            if (get_token_kind(the_token) == TK_RIGHT_BRACKET)
                break;

            the_verdict = expect_and_eat(the_parser->tokenizer, TK_COMMA);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_basket(the_basket);
                return NULL;
              }

            continue;
          }

        child_open_basket = parse_basket(the_parser);
        if (child_open_basket == NULL)
          {
            delete_unbound_name_manager(manager);
            delete_basket(the_basket);
            return NULL;
          }

        decompose_open_basket(child_open_basket, &child_manager,
                              &child_basket);

        the_verdict = merge_in_unbound_name_manager(manager, child_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_basket(child_basket);
            delete_unbound_name_manager(manager);
            delete_basket(the_basket);
            return NULL;
          }

        the_token = next_token(the_parser->tokenizer);
        if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
          {
            delete_basket(child_basket);
            delete_unbound_name_manager(manager);
            delete_basket(the_basket);
            return NULL;
          }

        if ((get_token_kind(the_token) == TK_ASSIGN) ||
            (get_token_kind(the_token) == TK_MODULO_ASSIGN))
          {
            token *second_token;

            second_token = forward_token(the_parser->tokenizer, 1);
            if ((second_token == NULL) ||
                (get_token_kind(second_token) == TK_ERROR))
              {
                delete_basket(child_basket);
                delete_unbound_name_manager(manager);
                delete_basket(the_basket);
                return NULL;
              }

            if (get_token_kind(second_token) != TK_IDENTIFIER)
              {
                token_error(second_token,
                        "Syntax error -- expected identifier, found %s.",
                        name_for_token_kind(get_token_kind(second_token)));
                delete_basket(child_basket);
                delete_unbound_name_manager(manager);
                delete_basket(the_basket);
                return NULL;
              }

            label = identifier_token_name(second_token);
          }
        else
          {
            label = NULL;
          }

        the_verdict = basket_add_sub_basket(the_basket, label, child_basket,
                (get_token_kind(the_token) == TK_MODULO_ASSIGN));
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_basket(the_basket);
            return NULL;
          }

        if ((get_token_kind(the_token) == TK_ASSIGN) ||
            (get_token_kind(the_token) == TK_MODULO_ASSIGN))
          {
            verdict the_verdict;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_basket(the_basket);
                return NULL;
              }

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_basket(the_basket);
                return NULL;
              }
          }

        the_token = next_token(the_parser->tokenizer);
        if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
          {
            delete_unbound_name_manager(manager);
            delete_basket(the_basket);
            return NULL;
          }

        if (get_token_kind(the_token) == TK_RIGHT_BRACKET)
            break;

        the_verdict = expect_and_eat(the_parser->tokenizer, TK_COMMA);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_basket(the_basket);
            return NULL;
          }
      }

    the_verdict = consume_token(the_parser->tokenizer);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_basket(the_basket);
        return NULL;
      }

    return create_open_basket(the_basket, manager);
  }

/*
 *      <type-expression> :
 *          <name-type-expression> |
 *          <enumeration-type-expression> |
 *          <not-type-expression> |
 *          <intersection-type-expression> |
 *          <union-type-expression> |
 *          <xor-type-expression> |
 *          <expression-type-expression> |
 *          <array-type-expression> |
 *          <range-type-expression> |
 *          <pointer-type-expression> |
 *          <type-type-expression> |
 *          <map-type-expression> |
 *          <routine-type-expression> |
 *          <fields-type-expression> |
 *          <lepton-type-expression> |
 *          <multiset-type-expression> |
 *          <interface-type-expression> |
 *          <semi-labeled-value-list-type-expression> |
 *          <regular-expression-type-expression> |
 *          <lazy-type-expression> |
 *          "(" <type-expression> ")"
 *
 *      <name-type-expression> :
 *          <identifier>
 *
 *      <enumeration-type-expression> :
 *          "{" <expression-list> "}"
 *
 *      <expression-list> :
 *          <empty> |
 *          <non-empty-expression-list>
 *
 *      <non-empty-expression-list> :
 *          <expression> |
 *          <expression> "," <non-empty-expression-list>
 *
 *      <not-type-expression> :
 *          "!" <type-expression>
 *
 *      <intersection-type-expression> :
 *          <type-expression> "&" <type-expression>
 *
 *      <union-type-expression> :
 *          <type-expression> "|" <type-expression>
 *
 *      <xor-type-expression> :
 *          <type-expression> "^" <type-expression>
 *
 *      <expression-type-expression> :
 *          "<<" <expression> ">>"
 *
 *      <array-type-expression> :
 *          "array" |
 *          "array" "[" <type-expression> "]" |
 *          <type-expression> "[" <expression> "]" |
 *          <type-expression> "[" <expression> "..." <expression> "]"
 *
 *      <pointer-type-expression> :
 *          "*" <type-expression> |
 *          "*" "." <type-expression> |
 *          "+" <type-expression> |
 *          "+" "." <type-expression> |
 *          "*" "+" <type-expression> |
 *          "*" "+" "." <type-expression>
 *
 *      <type-type-expression> :
 *          "type" <type-expression>
 *
 *      <map-type-expression> :
 *          <type-expression> "-->" <type-expression>
 *
 *      <routine-type-expression> :
 *          <type-expression> "<--" "(" <formal-type-list> ")"
 *
 *      <formal-type-list> :
 *          <empty> |
 *          <non-empty-formal-type-list>
 *
 *      <non-empty-formal-type-list> :
 *          "..." |
 *          "*" |
 *          <formal-type> |
 *          <formal-type> "," <non-empty-formal-type-list>
 *
 *      <formal-type> :
 *          { <identifier> ":" }? <type-expression> { ":=" "*" }?
 *
 *      <fields-type-expression> :
 *          "fields" "[" <field-type-list> "]"
 *
 *      <lepton-type-expression> :
 *          "lepton" <expression> "[" <field-type-list> "]"
 *
 *      <multiset-type-expression> :
 *          "multiset" "[" <field-type-list> "]"
 *
 *      <interface-type-expression> :
 *          "interface" { "." }? "[" <interface-item-list> "]"
 *
 *      <regular-expression-type-expression> :
 *          <regular-expression-literal-token>
 *
 *      <lazy-type-expression> :
 *          "lazy" <identifier>
 */
extern open_type_expression *parse_type_expression(parser *the_parser,
        type_expression_parsing_precedence precedence)
  {
    return parse_type_expression_with_end_location(the_parser, precedence,
                                                   NULL);
  }

extern open_type_expression *parse_type_expression_with_end_location(
        parser *the_parser, type_expression_parsing_precedence precedence,
        source_location *end_location)
  {
    source_location start_location;
    token *the_token;
    token_kind kind;
    unbound_name_manager *manager;
    type_expression *result;

    assert(the_parser != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));

    the_token = next_token(the_parser->tokenizer);
    if (the_token == NULL)
        return NULL;

    kind = get_token_kind(the_token);

    if (kind == TK_ERROR)
        return NULL;

    switch (kind)
      {
        case TK_IDENTIFIER:
          {
            const char *identifier_string;
            expression *name_expression;
            unbound_use *use;
            verdict the_verdict;

            identifier_string = aliased_token_name(the_token, the_parser);

            if (strcmp(identifier_string, "array") == 0)
              {
                verdict the_verdict;
                source_location local_end_location;
                expression *lower;
                expression *upper;

                local_end_location =
                        *(next_token_location(the_parser->tokenizer));

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                if (next_is(the_parser->tokenizer, TK_LEFT_BRACKET))
                  {
                    verdict the_verdict;
                    open_type_expression *open_result;

                    the_verdict = consume_token(the_parser->tokenizer);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                        return NULL;

                    open_result = parse_type_expression(the_parser, TEPP_TOP);
                    if (open_result == NULL)
                        return NULL;

                    local_end_location =
                            *(next_token_location(the_parser->tokenizer));

                    the_verdict = expect_and_eat(the_parser->tokenizer,
                                                 TK_RIGHT_BRACKET);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_open_type_expression(open_result);
                        return NULL;
                      }

                    decompose_open_type_expression(open_result, &manager,
                                                   &result);
                  }
                else
                  {
                    type *the_type;

                    the_type = get_anything_type();
                    if (the_type == NULL)
                        return NULL;

                    result = create_constant_type_expression(the_type);
                    if (result == NULL)
                        return NULL;

                    manager = create_unbound_name_manager();
                    if (manager == NULL)
                      {
                        delete_type_expression(result);
                        return NULL;
                      }
                  }

                lower = create_oi_expression(oi_negative_infinity);
                if (lower == NULL)
                  {
                    delete_type_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                upper = create_oi_expression(oi_positive_infinity);
                if (upper == NULL)
                  {
                    delete_expression(lower);
                    delete_type_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                result = create_array_type_expression(result, lower, upper);
                if (result == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_type_expression_end_location(result, &local_end_location);
                if (end_location != NULL)
                    *end_location = local_end_location;

                break;
              }

            if (strcmp(identifier_string, "type") == 0)
              {
                verdict the_verdict;
                source_location local_end_location;
                open_type_expression *open_result;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                open_result = parse_type_expression_with_end_location(
                        the_parser, TEPP_TYPE, &local_end_location);
                if (open_result == NULL)
                    return NULL;

                decompose_open_type_expression(open_result, &manager, &result);

                result = create_type_type_expression(result);
                if (result == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_type_expression_end_location(result, &local_end_location);
                if (end_location != NULL)
                    *end_location = local_end_location;

                break;
              }

            if (strcmp(identifier_string, "fields") == 0)
              {
                verdict the_verdict;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                result = create_fields_type_expression(FALSE);
                if (result == NULL)
                    return NULL;

                manager = NULL;

                the_verdict = parse_field_type_list(the_parser,
                        TK_LEFT_BRACKET, TK_RIGHT_BRACKET, &manager, result,
                        &fields_set_extra_allowed, &fields_add_field);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(result);
                    if (manager != NULL)
                        delete_unbound_name_manager(manager);
                    return NULL;
                  }

                if (manager == NULL)
                  {
                    manager = create_unbound_name_manager();
                    if (manager == NULL)
                      {
                        delete_type_expression(result);
                        return NULL;
                      }
                  }

                if (end_location != NULL)
                    *end_location = *get_type_expression_location(result);

                break;
              }

            if (strcmp(identifier_string, "lepton") == 0)
              {
                verdict the_verdict;
                open_expression *open_base;
                expression *base_expression;
                unbound_name_manager *child_manager;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                open_base = parse_expression(the_parser, EPP_TAGALONG, NULL,
                                             FALSE);
                if (open_base == NULL)
                    return NULL;

                decompose_open_expression(open_base, &manager,
                                          &base_expression);

                result = create_lepton_type_expression(base_expression, FALSE);
                if (result == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                child_manager = NULL;

                the_verdict = parse_field_type_list(the_parser,
                        TK_LEFT_BRACKET, TK_RIGHT_BRACKET, &child_manager,
                        result, &lepton_set_extra_allowed, &lepton_add_field);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(result);
                    if (child_manager != NULL)
                        delete_unbound_name_manager(child_manager);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                if (child_manager != NULL)
                  {
                    verdict the_verdict;

                    the_verdict = merge_in_unbound_name_manager(manager,
                                                                child_manager);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_type_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }
                  }

                if (end_location != NULL)
                    *end_location = *get_type_expression_location(result);

                break;
              }

            if (strcmp(identifier_string, "multiset") == 0)
              {
                verdict the_verdict;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                result = create_multiset_type_expression(FALSE);
                if (result == NULL)
                    return NULL;

                manager = NULL;

                the_verdict = parse_field_type_list(the_parser,
                        TK_LEFT_BRACKET, TK_RIGHT_BRACKET, &manager, result,
                        &multiset_set_extra_allowed, &multiset_add_field);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(result);
                    if (manager != NULL)
                        delete_unbound_name_manager(manager);
                    return NULL;
                  }

                if (manager == NULL)
                  {
                    manager = create_unbound_name_manager();
                    if (manager == NULL)
                      {
                        delete_type_expression(result);
                        return NULL;
                      }
                  }

                if (end_location != NULL)
                    *end_location = *get_type_expression_location(result);

                break;
              }

            if (strcmp(identifier_string, "interface") == 0)
              {
                verdict the_verdict;
                boolean null_allowed;
                source_location local_end_location;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                if (next_is(the_parser->tokenizer, TK_DOT))
                  {
                    verdict the_verdict;

                    the_verdict = consume_token(the_parser->tokenizer);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                        return NULL;

                    null_allowed = TRUE;
                  }
                else
                  {
                    null_allowed = FALSE;
                  }

                the_verdict =
                        expect_and_eat(the_parser->tokenizer, TK_LEFT_BRACKET);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                result = create_interface_type_expression(null_allowed);
                if (result == NULL)
                    return NULL;

                manager = NULL;

                the_verdict = parse_interface_item_list(the_parser, result,
                        &manager, TK_RIGHT_BRACKET);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(result);
                    if (manager != NULL)
                        delete_unbound_name_manager(manager);
                    return NULL;
                  }

                local_end_location =
                        *(next_token_location(the_parser->tokenizer));

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(result);
                    if (manager != NULL)
                        delete_unbound_name_manager(manager);
                    return NULL;
                  }

                if (manager == NULL)
                  {
                    manager = create_unbound_name_manager();
                    if (manager == NULL)
                      {
                        delete_type_expression(result);
                        return NULL;
                      }
                  }

                set_type_expression_end_location(result, &local_end_location);
                if (end_location != NULL)
                    *end_location = local_end_location;

                break;
              }

            if (strcmp(identifier_string, "lazy") == 0)
              {
                verdict the_verdict;
                token *the_token;
                source_location local_end_location;
                const char *identifier_string;
                expression *name_expression;
                unbound_use *use;
                type_expression *name_type_expression;
                type *anything_type;
                type_expression *anything_type_expression;
                variable_declaration *the_formal;
                declaration *formal_declaration;
                expression *formal_reference;
                expression *in_expression;
                statement *return_statement;
                statement_block *body;
                formal_arguments *formals;
                type *boolean_type;
                type_expression *boolean_type_expression;
                routine_declaration *the_routine;
                declaration *the_declaration;
                expression *routine_expression;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_verdict = expect(the_parser->tokenizer, TK_IDENTIFIER);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                the_token = next_token(the_parser->tokenizer);
                if (the_token == NULL)
                    return NULL;

                local_end_location = *get_token_location(the_token);

                identifier_string = aliased_token_name(the_token, the_parser);
                assert(identifier_string != NULL);

                name_expression = create_unbound_name_reference_expression();
                if (name_expression == NULL)
                    return NULL;

                set_expression_start_location(name_expression,
                                              get_token_location(the_token));

                manager = create_unbound_name_manager();
                if (manager == NULL)
                  {
                    delete_expression(name_expression);
                    return NULL;
                  }

                use = add_unbound_variable_reference(manager,
                        identifier_string, name_expression,
                        get_token_location(the_token));
                if (use == NULL)
                  {
                    delete_expression(name_expression);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                name_type_expression =
                        create_name_type_expression(name_expression);
                if (name_type_expression == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                anything_type = get_anything_type();
                if (anything_type == NULL)
                  {
                    delete_type_expression(name_type_expression);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                anything_type_expression =
                        create_constant_type_expression(anything_type);
                if (anything_type_expression == NULL)
                  {
                    delete_type_expression(name_type_expression);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_formal = create_variable_declaration(
                        anything_type_expression, NULL, FALSE, TRUE, NULL);
                if (the_formal == NULL)
                  {
                    delete_type_expression(name_type_expression);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                formal_declaration = create_declaration_for_variable("x",
                        FALSE, FALSE, TRUE, the_formal,
                        get_token_location(the_token));
                if (formal_declaration == NULL)
                  {
                    delete_type_expression(name_type_expression);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                formal_reference =
                        create_variable_reference_expression(the_formal);
                if (formal_reference == NULL)
                  {
                    declaration_remove_reference(formal_declaration);
                    delete_type_expression(name_type_expression);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                in_expression = create_in_expression(formal_reference,
                                                     name_type_expression);
                if (in_expression == NULL)
                  {
                    declaration_remove_reference(formal_declaration);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                return_statement = create_return_statement(in_expression);
                if (return_statement == NULL)
                  {
                    declaration_remove_reference(formal_declaration);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                body = create_statement_block();
                if (body == NULL)
                  {
                    delete_statement(return_statement);
                    declaration_remove_reference(formal_declaration);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_verdict =
                        append_statement_to_block(body, return_statement);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_statement_block(body);
                    declaration_remove_reference(formal_declaration);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                formals = create_formal_arguments();
                if (formals == NULL)
                  {
                    delete_statement_block(body);
                    declaration_remove_reference(formal_declaration);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_verdict = add_formal_parameter(formals, formal_declaration,
                                                   NULL);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_formal_arguments(formals);
                    delete_statement_block(body);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                boolean_type = get_boolean_type();
                if (boolean_type == NULL)
                  {
                    delete_formal_arguments(formals);
                    delete_statement_block(body);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                boolean_type_expression =
                        create_constant_type_expression(boolean_type);
                if (boolean_type_expression == NULL)
                  {
                    delete_formal_arguments(formals);
                    delete_statement_block(body);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_routine = create_routine_declaration(
                        boolean_type_expression, NULL, formals, FALSE, body,
                        NULL, PURE_UNSAFE, TRUE, FALSE, NULL, 0, NULL);
                if (the_routine == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_verdict = bind_return_statement_to_routine_declaration(
                        return_statement, the_routine);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_routine_declaration(the_routine);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_declaration = create_declaration_for_routine(NULL, FALSE,
                        FALSE, FALSE, the_routine,
                        get_token_location(the_token));
                if (the_declaration == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                routine_expression =
                        create_declaration_expression(the_declaration);
                if (routine_expression == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                result = create_expression_type_expression(routine_expression);
                if (result == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_type_expression_end_location(result, &local_end_location);
                if (end_location != NULL)
                    *end_location = local_end_location;

                break;
              }

            name_expression = create_unbound_name_reference_expression();
            if (name_expression == NULL)
                return NULL;

            set_expression_start_location(name_expression,
                                          get_token_location(the_token));

            manager = create_unbound_name_manager();
            if (manager == NULL)
              {
                delete_expression(name_expression);
                return NULL;
              }

            use = add_unbound_variable_reference(manager, identifier_string,
                    name_expression, get_token_location(the_token));
            if (use == NULL)
              {
                delete_expression(name_expression);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            result = create_name_type_expression(name_expression);
            if (result == NULL)
              {
                delete_unbound_name_manager(manager);
                return NULL;
              }

            set_type_expression_end_location(result,
                                             get_token_location(the_token));
            if (end_location != NULL)
                *end_location = *get_token_location(the_token);

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_type_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            break;
          }
        case TK_LEFT_CURLY_BRACE:
          {
            verdict the_verdict;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return NULL;

            result = create_enumeration_type_expression();
            if (result == NULL)
                return NULL;

            manager = NULL;

            if (!(next_is(the_parser->tokenizer, TK_RIGHT_CURLY_BRACE)))
              {
                while (TRUE)
                  {
                    open_expression *open_element;
                    expression *element;
                    unbound_name_manager *child_manager;
                    verdict the_verdict;
                    token *the_token;

                    open_element =
                            parse_expression(the_parser, EPP_TOP, NULL, FALSE);
                    if (open_element == NULL)
                      {
                        delete_type_expression(result);
                        if (manager != NULL)
                            delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    decompose_open_expression(open_element, &child_manager,
                                              &element);

                    the_verdict = enumeration_type_expression_add_case(result,
                            element);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_unbound_name_manager(child_manager);
                        delete_type_expression(result);
                        if (manager != NULL)
                            delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    if (manager == NULL)
                      {
                        manager = child_manager;
                      }
                    else
                      {
                        the_verdict = merge_in_unbound_name_manager(manager,
                                child_manager);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            delete_type_expression(result);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }
                      }

                    if (next_is(the_parser->tokenizer, TK_RIGHT_CURLY_BRACE))
                        break;

                    the_token = next_token(the_parser->tokenizer);
                    if ((the_token == NULL) ||
                        (get_token_kind(the_token) == TK_ERROR))
                      {
                        delete_type_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    if (get_token_kind(the_token) != TK_COMMA)
                      {
                        token_error(the_token,
                                "Syntax error -- expected %s or %s.",
                                name_for_token_kind(TK_COMMA),
                                name_for_token_kind(TK_RIGHT_CURLY_BRACE));
                        delete_type_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    the_verdict = consume_token(the_parser->tokenizer);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_type_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }
                  }
              }

            set_type_expression_end_location(result,
                    next_token_location(the_parser->tokenizer));
            if (end_location != NULL)
                *end_location = *(next_token_location(the_parser->tokenizer));

            the_verdict = expect_and_eat(the_parser->tokenizer,
                                         TK_RIGHT_CURLY_BRACE);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_type_expression(result);
                if (manager != NULL)
                    delete_unbound_name_manager(manager);
                return NULL;
              }

            if (manager == NULL)
              {
                manager = create_unbound_name_manager();
                if (manager == NULL)
                  {
                    delete_type_expression(result);
                    return NULL;
                  }
              }

            break;
          }
        case TK_LOGICAL_NOT:
          {
            verdict the_verdict;
            open_type_expression *open_result;
            source_location local_end_location;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return NULL;

            open_result = parse_type_expression_with_end_location(the_parser,
                    token_prefix_type_precedence(kind), &local_end_location);
            if (open_result == NULL)
                return NULL;

            decompose_open_type_expression(open_result, &manager, &result);

            result = create_not_type_expression(result);
            if (result == NULL)
              {
                delete_unbound_name_manager(manager);
                return NULL;
              }

            set_type_expression_end_location(result, &local_end_location);
            if (end_location != NULL)
                *end_location = local_end_location;

            break;
          }
        case TK_SHIFT_LEFT:
          {
            verdict the_verdict;
            open_expression *the_open_expression;
            expression *the_expression;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return NULL;

            the_open_expression =
                    parse_expression(the_parser, EPP_SHIFT, NULL, FALSE);
            if (the_open_expression == NULL)
                return NULL;

            decompose_open_expression(the_open_expression, &manager,
                                      &the_expression);

            result = create_expression_type_expression(the_expression);
            if (result == NULL)
              {
                delete_unbound_name_manager(manager);
                return NULL;
              }

            set_type_expression_end_location(result,
                    next_token_location(the_parser->tokenizer));
            if (end_location != NULL)
                *end_location = *(next_token_location(the_parser->tokenizer));

            the_verdict =
                    expect_and_eat(the_parser->tokenizer, TK_SHIFT_RIGHT);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_type_expression(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            break;
          }
        case TK_LEFT_BRACKET:
          {
            open_type_expression *open_result;

            if (is_possible_range_type_expression(the_parser))
              {
                open_result = parse_range_type_expression(the_parser);
              }
            else
              {
                open_result = parse_semi_labeled_value_list_type_expression(
                        the_parser);
              }

            if (open_result == NULL)
                return NULL;

            decompose_open_type_expression(open_result, &manager, &result);

            if (end_location != NULL)
                *end_location = *get_type_expression_location(result);

            break;
          }
        case TK_STAR:
        case TK_ADD:
          {
            boolean read_allowed;
            boolean write_allowed;
            boolean null_allowed;
            source_location local_end_location;
            open_type_expression *open_result;

            if (next_is(the_parser->tokenizer, TK_STAR))
              {
                verdict the_verdict;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                read_allowed = TRUE;
              }
            else
              {
                read_allowed = FALSE;
              }

            if (next_is(the_parser->tokenizer, TK_ADD))
              {
                verdict the_verdict;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                write_allowed = TRUE;
              }
            else
              {
                write_allowed = FALSE;
              }

            if (next_is(the_parser->tokenizer, TK_DOT))
              {
                verdict the_verdict;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                null_allowed = TRUE;
              }
            else
              {
                null_allowed = FALSE;
              }

            open_result = parse_type_expression_with_end_location(the_parser,
                    token_prefix_type_precedence(TK_STAR),
                    &local_end_location);
            if (open_result == NULL)
                return NULL;

            decompose_open_type_expression(open_result, &manager, &result);

            result = create_pointer_type_expression(result, read_allowed,
                    write_allowed, null_allowed);
            if (result == NULL)
              {
                delete_unbound_name_manager(manager);
                return NULL;
              }

            set_type_expression_end_location(result, &local_end_location);
            if (end_location != NULL)
                *end_location = local_end_location;

            break;
          }
        case TK_REGULAR_EXPRESSION_LITERAL:
          {
            verdict the_verdict;

            result = create_regular_expression_type_expression(
                    regular_expression_literal_token_data(the_token));
            if (result == NULL)
                return NULL;

            set_type_expression_end_location(result,
                    next_token_location(the_parser->tokenizer));
            if (end_location != NULL)
                *end_location = *(next_token_location(the_parser->tokenizer));

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_type_expression(result);
                return NULL;
              }

            manager = create_unbound_name_manager();
            if (manager == NULL)
              {
                delete_type_expression(result);
                return NULL;
              }

            break;
          }
        case TK_LEFT_PAREN:
          {
            verdict the_verdict;
            open_type_expression *open_result;

            if (is_possible_range_type_expression(the_parser))
              {
                open_result = parse_range_type_expression(the_parser);

                if (open_result == NULL)
                    return NULL;

                decompose_open_type_expression(open_result, &manager, &result);

                if (end_location != NULL)
                    *end_location = *get_type_expression_location(result);

                break;
              }

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return NULL;

            open_result = parse_type_expression(the_parser, TEPP_TOP);
            if (open_result == NULL)
                return NULL;

            if (end_location != NULL)
                *end_location = *(next_token_location(the_parser->tokenizer));

            the_verdict =
                    expect_and_eat(the_parser->tokenizer, TK_RIGHT_PAREN);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_open_type_expression(open_result);
                return NULL;
              }

            decompose_open_type_expression(open_result, &manager, &result);

            goto skip_start_set;
          }
        default:
          {
            token_error(the_token,
                        "Syntax error -- expected type expression.");
            return NULL;
          }
      }

    while (TRUE)
      {
        token *the_token;
        token_kind kind;

        set_type_expression_start_location(result, &start_location);

      skip_start_set:
        the_token = next_token(the_parser->tokenizer);
        if (the_token == NULL)
          {
            delete_type_expression(result);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        kind = get_token_kind(the_token);

        if ((token_postfix_type_precedence(kind) < precedence) ||
            ((token_postfix_type_precedence(kind) == precedence) &&
             token_is_left_associative(kind)))
          {
            break;
          }

        switch (kind)
          {
            case TK_AMPERSAND:
            case TK_BITWISE_OR:
            case TK_BITWISE_XOR:
            case TK_MAPS_TO:
              {
                verdict the_verdict;
                source_location end_location2;
                open_type_expression *open_operand2;
                unbound_name_manager *manager2;
                type_expression *operand2;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                open_operand2 = parse_type_expression_with_end_location(
                        the_parser, token_postfix_type_precedence(kind),
                        &end_location2);
                if (open_operand2 == NULL)
                  {
                    delete_type_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                decompose_open_type_expression(open_operand2, &manager2,
                                               &operand2);

                the_verdict = merge_in_unbound_name_manager(manager, manager2);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(operand2);
                    delete_type_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                switch (kind)
                  {
                    case TK_AMPERSAND:
                        result = create_intersection_type_expression(result,
                                                                     operand2);
                        break;
                    case TK_BITWISE_OR:
                        result =
                                create_union_type_expression(result, operand2);
                        break;
                    case TK_BITWISE_XOR:
                        result = create_xor_type_expression(result, operand2);
                        break;
                    case TK_MAPS_TO:
                        result = create_map_type_expression(result, operand2);
                        break;
                    default:
                        assert(FALSE);
                  }

                if (result == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_type_expression_end_location(result, &end_location2);
                if (end_location != NULL)
                    *end_location = end_location2;

                continue;
              }
            case TK_LEFT_BRACKET:
              {
                verdict the_verdict;
                open_expression *open1;
                expression *expression1;
                unbound_name_manager *child_manager;
                token *the_token;
                expression *lower;
                expression *upper;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                open1 = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
                if (open1 == NULL)
                  {
                    delete_type_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                decompose_open_expression(open1, &child_manager, &expression1);

                the_verdict =
                        merge_in_unbound_name_manager(manager, child_manager);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(expression1);
                    delete_type_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_token = next_token(the_parser->tokenizer);
                if ((the_token == NULL) ||
                    (get_token_kind(the_token) == TK_ERROR))
                  {
                    delete_expression(expression1);
                    delete_type_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                switch (get_token_kind(the_token))
                  {
                    case TK_RIGHT_BRACKET:
                      {
                        expression *one_expression;

                        lower = create_oi_expression(oi_zero);
                        if (lower == NULL)
                          {
                            delete_expression(expression1);
                            delete_type_expression(result);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        one_expression = create_oi_expression(oi_one);
                        if (one_expression == NULL)
                          {
                            delete_expression(lower);
                            delete_expression(expression1);
                            delete_type_expression(result);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        upper = create_binary_expression(EK_SUBTRACT,
                                expression1, one_expression);
                        if (upper == NULL)
                          {
                            delete_expression(lower);
                            delete_type_expression(result);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        break;
                      }
                    case TK_DOT_DOT_DOT:
                      {
                        verdict the_verdict;
                        open_expression *open2;
                        unbound_name_manager *manager2;

                        lower = expression1;

                        the_verdict = consume_token(the_parser->tokenizer);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            delete_expression(expression1);
                            delete_type_expression(result);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        open2 = parse_expression(the_parser, EPP_TOP, NULL,
                                                 FALSE);
                        if (open2 == NULL)
                          {
                            delete_expression(expression1);
                            delete_type_expression(result);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        decompose_open_expression(open2, &manager2, &upper);

                        the_verdict = merge_in_unbound_name_manager(manager,
                                                                    manager2);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            delete_expression(upper);
                            delete_expression(expression1);
                            delete_type_expression(result);
                            delete_unbound_name_manager(manager);
                            return NULL;
                          }

                        break;
                      }
                    default:
                      {
                        token_error(the_token,
                                "Syntax error -- expected %s or %s.",
                                name_for_token_kind(TK_DOT_DOT_DOT),
                                name_for_token_kind(TK_RIGHT_BRACKET));
                        delete_expression(expression1);
                        delete_type_expression(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }
                  }

                result = create_array_type_expression(result, lower, upper);
                if (result == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_type_expression_end_location(result,
                        next_token_location(the_parser->tokenizer));
                if (end_location != NULL)
                  {
                    *end_location =
                            *next_token_location(the_parser->tokenizer);
                  }

                the_verdict = expect_and_eat(the_parser->tokenizer,
                                             TK_RIGHT_BRACKET);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                continue;
              }
            case TK_RETURNS:
              {
                verdict the_verdict;
                source_location local_end_location;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                result = create_routine_type_expression(result, FALSE);
                if (result == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_verdict = parse_formal_type_list(the_parser, TK_LEFT_PAREN,
                        TK_RIGHT_PAREN, &manager, result,
                        &routine_type_expression_set_extra_allowed,
                        &routine_type_expression_set_extra_unspecified,
                        &routine_type_expression_add_formal_generic,
                        &local_end_location);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                set_type_expression_end_location(result, &local_end_location);
                if (end_location != NULL)
                    *end_location = local_end_location;

                continue;
              }
            default:
              {
                break;
              }
          }

        break;
      }

    return create_open_type_expression(result, manager);
  }

/*
 *      <interface-item-list> :
 *          <empty> |
 *          <non-empty-interface-item-list>
 *
 *      <non-empty-interface-item-list> :
 *          <interface-item> |
 *          <interface-item> "," <non-empty-interface-item-list>
 *
 *      <interface-item> :
 *          <identifier> ":" { "-" }? <type-expression> |
 *          "include" <string-literal-token>
 */
extern verdict parse_interface_item_list(parser *the_parser,
        type_expression *interface_type_expression,
        unbound_name_manager **manager, token_kind expected_finish)
  {
    assert(the_parser != NULL);
    assert(interface_type_expression != NULL);
    assert(manager != NULL);

    if (next_is(the_parser->tokenizer, expected_finish))
        return MISSION_ACCOMPLISHED;

    while (TRUE)
      {
        char *name;
        token *the_token;
        verdict the_verdict;

        name = expect_and_eat_identifier_return_name_copy(the_parser);
        if (name == NULL)
            return MISSION_FAILED;

        if ((strcmp(try_aliasing(name, the_parser), "include") == 0) &&
            next_is(the_parser->tokenizer, TK_STRING_LITERAL))
          {
            token *the_token;
            const char *include_file;
            verdict the_verdict;

            free(name);

            the_token = next_token(the_parser->tokenizer);
            assert(the_token != NULL);

            include_file = string_literal_token_data(the_token);
            assert(include_file != NULL);

            the_verdict = (*(the_parser->interface_include_handler))(
                    the_parser->include_handler_data, include_file,
                    interface_type_expression, manager,
                    get_token_location(the_token), the_parser->alias_manager,
                    the_parser->native_bridge_dll_body_allowed);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;
          }
        else
          {
            verdict the_verdict;
            boolean writing_allowed;
            open_type_expression *open_item_type;
            unbound_name_manager *child_manager;
            type_expression *item_type;

            the_verdict = expect_and_eat(the_parser->tokenizer, TK_COLON);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                free(name);
                return the_verdict;
              }

            if (next_is(the_parser->tokenizer, TK_DASH))
              {
                verdict the_verdict;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    free(name);
                    return the_verdict;
                  }
                writing_allowed = FALSE;
              }
            else
              {
                writing_allowed = TRUE;
              }

            open_item_type = parse_type_expression(the_parser, TEPP_TOP);
            if (open_item_type == NULL)
              {
                free(name);
                return MISSION_FAILED;
              }

            decompose_open_type_expression(open_item_type, &child_manager,
                                           &item_type);

            if (*manager == NULL)
              {
                *manager = child_manager;
              }
            else
              {
                the_verdict =
                        merge_in_unbound_name_manager(*manager, child_manager);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(item_type);
                    free(name);
                    return the_verdict;
                  }
              }

            the_verdict = interface_type_expression_add_item(
                    interface_type_expression, item_type, name,
                    writing_allowed);
            free(name);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;
          }

        the_token = next_token(the_parser->tokenizer);
        if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
            return MISSION_FAILED;

        if (get_token_kind(the_token) == expected_finish)
            return MISSION_ACCOMPLISHED;

        if (get_token_kind(the_token) != TK_COMMA)
          {
            token_error(the_token, "Syntax error -- expected %s or %s.",
                    name_for_token_kind(TK_COMMA),
                    name_for_token_kind(expected_finish));
            return MISSION_FAILED;
          }

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }
  }

/*
 *      <range-type-expression> :
 *          { "[" | "(" } <expression> { "..." | "...." } <expression>
 *                  { "]" | ")" }
 */
extern open_type_expression *parse_range_type_expression(parser *the_parser)
  {
    token_kind left_kind;
    verdict the_verdict;
    open_expression *open_lower;
    token_kind middle_kind;
    open_expression *open_upper;
    source_location end_location;
    token_kind right_kind;
    expression *lower_expression;
    expression *upper_expression;
    unbound_name_manager *lower_manager;
    unbound_name_manager *upper_manager;
    type_expression *result;

    the_verdict = expect_and_eat2(the_parser->tokenizer, TK_LEFT_BRACKET,
                                  TK_LEFT_PAREN, &left_kind);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    open_lower = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
    if (open_lower == NULL)
        return NULL;

    the_verdict = expect_and_eat2(the_parser->tokenizer, TK_DOT_DOT_DOT,
                                  TK_DOT_DOT_DOT_DOT, &middle_kind);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_expression(open_lower);
        return NULL;
      }

    open_upper = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
    if (open_upper == NULL)
      {
        delete_open_expression(open_lower);
        return NULL;
      }

    end_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat2(the_parser->tokenizer, TK_RIGHT_BRACKET,
                                  TK_RIGHT_PAREN, &right_kind);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_expression(open_upper);
        delete_open_expression(open_lower);
        return NULL;
      }

    decompose_open_expression(open_lower, &lower_manager, &lower_expression);
    decompose_open_expression(open_upper, &upper_manager, &upper_expression);

    the_verdict = merge_in_unbound_name_manager(lower_manager, upper_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(lower_manager);
        delete_expression(lower_expression);
        delete_expression(upper_expression);
        return NULL;
      }

    if (middle_kind == TK_DOT_DOT_DOT)
      {
        result = create_integer_range_type_expression(lower_expression,
                upper_expression, (left_kind == TK_LEFT_BRACKET),
                (right_kind == TK_RIGHT_BRACKET));
      }
    else
      {
        result = create_rational_range_type_expression(lower_expression,
                upper_expression, (left_kind == TK_LEFT_BRACKET),
                (right_kind == TK_RIGHT_BRACKET));
      }
    if (result == NULL)
      {
        delete_unbound_name_manager(lower_manager);
        return NULL;
      }

    set_type_expression_end_location(result, &end_location);

    return create_open_type_expression(result, lower_manager);
  }

/*
 *      <semi-labeled-value-list-type-expression> :
 *          "[" <formal-type-list> "]"
 */
extern open_type_expression *parse_semi_labeled_value_list_type_expression(
        parser *the_parser)
  {
    type_expression *result;
    unbound_name_manager *manager;
    source_location end_location;
    verdict the_verdict;

    result = create_semi_labeled_value_list_type_expression(FALSE);
    if (result == NULL)
        return NULL;

    manager = NULL;

    the_verdict = parse_formal_type_list(the_parser, TK_LEFT_BRACKET,
            TK_RIGHT_BRACKET, &manager, result,
            &semi_labeled_value_list_type_expression_set_extra_allowed,
            &semi_labeled_value_list_type_expression_set_extra_unspecified,
            &semi_labeled_value_list_type_expression_add_formal,
            &end_location);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_type_expression(result);
        if (manager != NULL)
            delete_unbound_name_manager(manager);
        return NULL;
      }

    set_type_expression_end_location(result, &end_location);

    if (manager == NULL)
      {
        manager = create_unbound_name_manager();
        if (manager == NULL)
          {
            delete_type_expression(result);
            return NULL;
          }
      }

    return create_open_type_expression(result, manager);
  }

/*
 *      <formal-type-list> :
 *          <empty> |
 *          <non-empty-formal-type-list>
 *
 *      <non-empty-formal-type-list> :
 *          "..." |
 *          "*" |
 *          <formal-type> |
 *          <formal-type> "," <non-empty-formal-type-list>
 *
 *      <formal-type> :
 *          { <identifier> ":" }? <type-expression> { ":=" "*" }?
 */
extern verdict parse_formal_type_list(parser *the_parser,
        token_kind left_bound_kind, token_kind right_bound_kind,
        unbound_name_manager **manager, void *data,
        verdict (*set_extra_allowed)(void *data),
        verdict (*set_extra_unspecified)(void *data, boolean *error),
        verdict (*add_formal)(void *data, const char *name,
                type_expression *formal_type, boolean has_default),
        source_location *end_location)
  {
    verdict the_verdict;
    token *the_token;
    token_kind kind;

    assert(the_parser != NULL);
    assert(manager != NULL);
    assert(set_extra_allowed != NULL);
    assert(set_extra_unspecified != NULL);
    assert(add_formal != NULL);
    assert(end_location != NULL);

    the_verdict = expect_and_eat(the_parser->tokenizer, left_bound_kind);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_token = next_token(the_parser->tokenizer);
    if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
        return MISSION_FAILED;

    kind = get_token_kind(the_token);

    if (kind == right_bound_kind)
      {
        *end_location = *get_token_location(the_token);
        return consume_token(the_parser->tokenizer);
      }

    while (TRUE)
      {
        char *name;
        open_type_expression *open_formal;
        unbound_name_manager *child_manager;
        type_expression *formal_type;
        boolean has_default;
        verdict the_verdict;

        if (kind == TK_DOT_DOT_DOT)
          {
            verdict the_verdict;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;

            *end_location = *next_token_location(the_parser->tokenizer);

            the_verdict =
                    expect_and_eat(the_parser->tokenizer, right_bound_kind);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;

            return (*set_extra_allowed)(data);
          }

        if (kind == TK_STAR)
          {
            token *second_token;

            second_token = forward_token(the_parser->tokenizer, 1);
            if ((second_token == NULL) ||
                (get_token_kind(second_token) == TK_ERROR))
              {
                return MISSION_FAILED;
              }

            if (get_token_kind(second_token) == right_bound_kind)
              {
                boolean error;
                verdict the_verdict;

                the_verdict = (*set_extra_unspecified)(data, &error);
                if (error)
                    return MISSION_FAILED;

                if (the_verdict == MISSION_ACCOMPLISHED)
                  {
                    the_verdict = consume_token(the_parser->tokenizer);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                        return the_verdict;

                    *end_location =
                            *next_token_location(the_parser->tokenizer);
                    return expect_and_eat(the_parser->tokenizer,
                                          right_bound_kind);
                  }
              }
          }

        if (kind == TK_IDENTIFIER)
          {
            token *second_token;

            second_token = forward_token(the_parser->tokenizer, 1);
            if ((second_token == NULL) ||
                (get_token_kind(second_token) == TK_ERROR))
              {
                return MISSION_FAILED;
              }

            if (get_token_kind(second_token) == TK_COLON)
              {
                const char *id_chars;
                verdict the_verdict;

                id_chars = identifier_token_name(the_token);
                assert(id_chars != NULL);

                name = MALLOC_ARRAY(char, strlen(id_chars) + 1);
                if (name == NULL)
                    return MISSION_FAILED;

                strcpy(name, id_chars);

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    free(name);
                    return the_verdict;
                  }

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    free(name);
                    return the_verdict;
                  }
              }
            else
              {
                name = NULL;
              }
          }
        else
          {
            name = NULL;
          }

        open_formal = parse_type_expression(the_parser, TEPP_TOP);
        if (open_formal == NULL)
          {
            if (name != NULL)
                free(name);
            return MISSION_FAILED;
          }

        decompose_open_type_expression(open_formal, &child_manager,
                                       &formal_type);

        if (*manager == NULL)
          {
            *manager = child_manager;
          }
        else
          {
            verdict the_verdict;

            the_verdict =
                    merge_in_unbound_name_manager(*manager, child_manager);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_type_expression(formal_type);
                if (name != NULL)
                    free(name);
                return the_verdict;
              }
          }

        if (next_is(the_parser->tokenizer, TK_ASSIGN))
          {
            verdict the_verdict;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_type_expression(formal_type);
                if (name != NULL)
                    free(name);
                return the_verdict;
              }

            the_verdict = expect_and_eat(the_parser->tokenizer, TK_STAR);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_type_expression(formal_type);
                if (name != NULL)
                    free(name);
                return the_verdict;
              }

            has_default = TRUE;
          }
        else
          {
            has_default = FALSE;
          }

        the_verdict = (*add_formal)(data, name, formal_type, has_default);
        if (name != NULL)
            free(name);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        if (next_is(the_parser->tokenizer, right_bound_kind))
          {
            *end_location = *next_token_location(the_parser->tokenizer);
            return consume_token(the_parser->tokenizer);
          }

        the_token = next_token(the_parser->tokenizer);
        if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
            return MISSION_FAILED;

        kind = get_token_kind(the_token);

        if (kind != TK_COMMA)
          {
            token_error(the_token, "Syntax error -- expected %s or %s.",
                    name_for_token_kind(TK_COMMA),
                    name_for_token_kind(right_bound_kind));
            return MISSION_FAILED;
          }

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        the_token = next_token(the_parser->tokenizer);
        if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
            return MISSION_FAILED;

        kind = get_token_kind(the_token);
      }
  }

/*
 *      <field-type-list> :
 *          <empty> |
 *          <non-empty-field-type-list>
 *
 *      <non-empty-field-type-list> :
 *          "..." |
 *          <field-type> |
 *          <field-type> "," <non-empty-field-type-list>
 *
 *      <field-type> :
 *          <identifier> ":" <type-expression>
 */
extern verdict parse_field_type_list(parser *the_parser,
        token_kind left_bound_kind, token_kind right_bound_kind,
        unbound_name_manager **manager, type_expression *base_type,
        verdict (*set_extra_allowed)(type_expression *base_type),
        verdict (*add_field)(type_expression *base_type, const char *name,
                             type_expression *field_type))
  {
    verdict the_verdict;
    token *the_token;
    token_kind kind;

    assert(the_parser != NULL);
    assert(manager != NULL);
    assert(base_type != NULL);
    assert(set_extra_allowed != NULL);
    assert(add_field != NULL);

    the_verdict = expect_and_eat(the_parser->tokenizer, left_bound_kind);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_token = next_token(the_parser->tokenizer);
    if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
        return MISSION_FAILED;

    kind = get_token_kind(the_token);

    if (kind == right_bound_kind)
      {
        set_type_expression_end_location(base_type,
                next_token_location(the_parser->tokenizer));
        return consume_token(the_parser->tokenizer);
      }

    while (TRUE)
      {
        char *name;
        verdict the_verdict;
        open_type_expression *open_field;
        unbound_name_manager *child_manager;
        type_expression *field_type;

        if (kind == TK_DOT_DOT_DOT)
          {
            verdict the_verdict;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;

            set_type_expression_end_location(base_type,
                    next_token_location(the_parser->tokenizer));
            the_verdict =
                    expect_and_eat(the_parser->tokenizer, right_bound_kind);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;

            return (*set_extra_allowed)(base_type);
          }

        name = expect_and_eat_identifier_return_name_copy(the_parser);
        if (name == NULL)
            return MISSION_FAILED;

        the_verdict = expect_and_eat(the_parser->tokenizer, TK_COLON);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            free(name);
            return the_verdict;
          }

        open_field = parse_type_expression(the_parser, TEPP_TOP);
        if (open_field == NULL)
          {
            free(name);
            return MISSION_FAILED;
          }

        decompose_open_type_expression(open_field, &child_manager,
                                       &field_type);

        if (*manager == NULL)
          {
            *manager = child_manager;
          }
        else
          {
            verdict the_verdict;

            the_verdict =
                    merge_in_unbound_name_manager(*manager, child_manager);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_type_expression(field_type);
                free(name);
                return the_verdict;
              }
          }

        the_verdict = (*add_field)(base_type, name, field_type);
        free(name);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        if (next_is(the_parser->tokenizer, right_bound_kind))
          {
            set_type_expression_end_location(base_type,
                    next_token_location(the_parser->tokenizer));
            return consume_token(the_parser->tokenizer);
          }

        the_token = next_token(the_parser->tokenizer);
        if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
            return MISSION_FAILED;

        kind = get_token_kind(the_token);

        if (kind != TK_COMMA)
          {
            token_error(the_token, "Syntax error -- expected %s or %s.",
                    name_for_token_kind(TK_COMMA),
                    name_for_token_kind(right_bound_kind));
            return MISSION_FAILED;
          }

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        the_token = next_token(the_parser->tokenizer);
        if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
            return MISSION_FAILED;

        kind = get_token_kind(the_token);
      }
  }

extern open_call *parse_call_suffix(parser *the_parser, expression *base,
                                    unbound_name_manager *base_manager)
  {
    tokenizer *the_tokenizer;
    token *the_token;
    call *the_call;
    open_call *the_open_call;
    verdict the_verdict;
    token_kind kind;

    assert(the_parser != NULL);
    assert(base != NULL);
    assert(base_manager != NULL);

    the_tokenizer = the_parser->tokenizer;
    assert(the_tokenizer != NULL);

    the_call = create_call(base, 0, NULL, NULL, get_expression_location(base));
    if (the_call == NULL)
      {
        delete_unbound_name_manager(base_manager);
        return NULL;
      }

    the_open_call = create_open_call(the_call, base_manager);
    if (the_open_call == NULL)
      {
        return NULL;
      }

    the_verdict = expect_and_eat(the_tokenizer, TK_LEFT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_call(the_open_call);
        return NULL;
      }

    the_token = next_token(the_tokenizer);
    if (the_token == NULL)
      {
        delete_open_call(the_open_call);
        return NULL;
      }

    kind = get_token_kind(the_token);

    if (kind == TK_ERROR)
      {
        delete_open_call(the_open_call);
        return NULL;
      }

    if (kind != TK_RIGHT_PAREN)
      {
        while (TRUE)
          {
            char *label;
            open_expression *the_open_expression;
            unbound_name_manager *parameter_unbound_name_manager;
            expression *the_expression;
            verdict the_verdict;
            token *the_token;
            token_kind kind;

            label = NULL;

            if (next_is(the_parser->tokenizer, TK_IDENTIFIER))
              {
                token *second_token;

                second_token = forward_token(the_parser->tokenizer, 1);
                if ((second_token == NULL) ||
                    (get_token_kind(second_token) == TK_ERROR))
                  {
                    delete_open_call(the_open_call);
                    return NULL;
                  }

                if (get_token_kind(second_token) == TK_ASSIGN)
                  {
                    token *the_token;
                    const char *id_chars;
                    verdict the_verdict;

                    the_token = next_token(the_tokenizer);
                    assert(the_token != NULL);

                    id_chars = identifier_token_name(the_token);

                    label = MALLOC_ARRAY(char, strlen(id_chars) + 1);
                    if (label == NULL)
                      {
                        delete_open_call(the_open_call);
                        return NULL;
                      }

                    strcpy(label, id_chars);

                    the_verdict = consume_token(the_tokenizer);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_open_call(the_open_call);
                        return NULL;
                      }

                    the_verdict = consume_token(the_tokenizer);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_open_call(the_open_call);
                        return NULL;
                      }
                  }
              }

            the_open_expression =
                    parse_expression(the_parser, EPP_TOP, NULL, FALSE);
            if (the_open_expression == NULL)
              {
                if (label != NULL)
                    free(label);
                delete_open_call(the_open_call);
                return NULL;
              }

            decompose_open_expression(the_open_expression,
                    &parameter_unbound_name_manager, &the_expression);

            the_verdict = merge_in_unbound_name_manager(base_manager,
                    parameter_unbound_name_manager);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(the_expression);
                if (label != NULL)
                    free(label);
                delete_open_call(the_open_call);
                return NULL;
              }

            the_verdict =
                    append_argument_to_call(the_call, the_expression, label);
            if (label != NULL)
                free(label);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_open_call(the_open_call);
                return NULL;
              }

            the_token = next_token(the_tokenizer);
            if (the_token == NULL)
              {
                delete_open_call(the_open_call);
                return NULL;
              }

            kind = get_token_kind(the_token);

            if (kind == TK_ERROR)
              {
                delete_open_call(the_open_call);
                return NULL;
              }

            if (kind == TK_RIGHT_PAREN)
                break;

            if (kind != TK_COMMA)
              {
                token_error(the_token, "Syntax error -- expected %s or %s.",
                        name_for_token_kind(TK_COMMA),
                        name_for_token_kind(TK_RIGHT_PAREN));
                delete_open_call(the_open_call);
                return NULL;
              }

            the_verdict = consume_token(the_tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_open_call(the_open_call);
                return NULL;
              }
          }
      }

    set_call_end_location(the_call, next_token_location(the_tokenizer));

    the_verdict = consume_token(the_tokenizer);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_call(the_open_call);
        return NULL;
      }

    return the_open_call;
  }

/*
 *      <statement> :
 *          <non-block-non-if-statement> |
 *          <statement-block-statement> |
 *          <if-statement>
 *
 *      <non-block-non-if-statement> :
 *          <assign-statement> |
 *          <increment-statement> |
 *          <decrement-statement> |
 *          <call-statement> |
 *          <variable-declaration-statement> |
 *          <routine-declaration-statement> |
 *          <tagalong-declaration-statement> |
 *          <lepton-declaration-statement> |
 *          <quark-declaration-statement> |
 *          <lock-declaration-statement> |
 *          <switch-statement> |
 *          <goto-statement> |
 *          <return-statement> |
 *          <for-statement> |
 *          <iterate-statement> |
 *          <while-statement> |
 *          <do-while-statement> |
 *          <break-statement> |
 *          <continue-statement> |
 *          <label-statement> |
 *          <single-statement> |
 *          <try-catch-statement> |
 *          <try-handle-statement> |
 *          <cleanup-statement> |
 *          <export-statement> |
 *          <hide-statement> |
 *          <use-statement> |
 *          <include-statement> |
 *          <quark-enumeration-statement> |
 *          <theorem-statement> |
 *          <alias-statement> |
 *          <print-line-statement> |
 *          <backtick-statement>
 *
 *      <assign-statement> :
 *          <basket> <assign-operator> <expression> ";"
 *
 *      <basket> :
 *          <expression> |
 *          "[" <basket-item-list> "]"
 *
 *      <basket-item-list> :
 *          <empty> |
 *          <non-empty-basket-item-list>
 *
 *      <non-empty-basket-item-list> :
 *          <basket-item> |
 *          <basket-item> "," <non-empty-basket-item-list>
 *
 *      <basket-item> :
 *          { <basket> }? { { ":=" | "::=" } <identifier> }?
 *
 *      <assign-operator> :
 *          ":=" |
 *          "::=" |
 *          "*=" |
 *          "/=" |
 *          "/::=" |
 *          "%=" |
 *          "+=" |
 *          "-=" |
 *          "<<=" |
 *          ">>=" |
 *          "&=" |
 *          "^=" |
 *          "|=" |
 *          "&&=" |
 *          "||=" |
 *          "~="
 *
 *      <call-statement> :
 *          <call> ";"
 *
 *      <variable-declaration-statement> :
 *          <named-data-declaration> ";"
 *
 *      <routine-declaration-statement> :
 *          <named-routine-declaration> ";"
 *
 *      <tagalong-declaration-statement> :
 *          <named-tagalong-declaration> ";"
 *
 *      <lepton-declaration-statement> :
 *          <named-lepton-declaration> ";"
 *
 *      <quark-declaration-statement> :
 *          <named-quark-declaration> ";"
 *
 *      <lock-declaration-statement> :
 *          <named-lock-declaration> ";"
 *
 *      <single-statement> :
 *          <single-prefix> <braced-statement-block> ";"
 *
 *      <single-prefix> :
 *          "single" { "(" <expression> ")" }?
 *
 *      <quark-enumeration-statement> :
 *          <data-prefix> "quark" "enumeration" <identifier>
 *                  "{" <quark-name-list> "}" ";"
 *
 *      <quark-name-list> :
 *          <identifier> |
 *          <identifier> <quark-name-list>
 *
 *      <print-line-statement> :
 *          <expression> "!"
 *
 *      <backtick-statement> :
 *          <backtick-expression-literal-token> ";"
 */
extern open_statement *parse_statement(parser *the_parser,
        const char **current_labels, size_t current_label_count)
  {
    tokenizer *the_tokenizer;
    token *the_token;
    source_location start_location;
    token_kind kind;
    open_statement *result;
    unbound_use *naked_call_overloading_use;
    open_expression *the_open_expression;
    unbound_name_manager *manager;
    expression *the_expression;
    basket *the_basket;

    assert(the_parser != NULL);
    assert((current_label_count == 0) || (current_labels != NULL));

    the_tokenizer = the_parser->tokenizer;
    assert(the_tokenizer != NULL);

    the_token = next_token(the_tokenizer);
    if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
        return NULL;

    start_location = *(next_token_location(the_parser->tokenizer));

    kind = get_token_kind(the_token);

    switch (kind)
      {
        case TK_IDENTIFIER:
          {
            const char *id_chars;

            id_chars = aliased_token_name(the_token, the_parser);
            assert(id_chars != NULL);

            if ((strcmp(id_chars, "routine") == 0) ||
                (strcmp(id_chars, "function") == 0) ||
                (strcmp(id_chars, "procedure") == 0) ||
                (strcmp(id_chars, "class") == 0) ||
                (strcmp(id_chars, "variable") == 0) ||
                (strcmp(id_chars, "immutable") == 0) ||
                (strcmp(id_chars, "tagalong") == 0) ||
                (strcmp(id_chars, "lepton") == 0) ||
                (strcmp(id_chars, "quark") == 0) ||
                (strcmp(id_chars, "lock") == 0) ||
                (strcmp(id_chars, "static") == 0) ||
                (strcmp(id_chars, "virtual") == 0) ||
                (strcmp(id_chars, "pure") == 0))
              {
                result = parse_routine_or_variable_statement(the_parser, NULL,
                        NULL, NULL, PURE_UNSAFE);
                goto add_location;
              }
            else if (strcmp(id_chars, "ageless") == 0)
              {
                token *second_token;

                second_token = forward_token(the_tokenizer, 1);
                if (second_token == NULL)
                    return NULL;

                if (get_token_kind(second_token) == TK_IDENTIFIER)
                  {
                    token *third_token;

                    third_token = forward_token(the_tokenizer, 2);
                    if (third_token == NULL)
                        return NULL;

                    if (get_token_kind(third_token) == TK_COLON)
                      {
                        result = parse_label_statement(the_parser);
                        goto add_location;
                      }
                  }

                result = parse_routine_or_variable_statement(the_parser, NULL,
                        NULL, NULL, PURE_UNSAFE);
                goto add_location;
              }
            else if (strcmp(id_chars, "single") == 0)
              {
                verdict the_verdict;
                expression *single_lock;
                unbound_name_manager *single_manager;
                token *the_token;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                if (next_is(the_parser->tokenizer, TK_LEFT_PAREN))
                  {
                    verdict the_verdict;
                    open_expression *open_lock;

                    the_verdict = expect_and_eat(the_parser->tokenizer,
                                                 TK_LEFT_PAREN);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                        return NULL;

                    open_lock =
                            parse_expression(the_parser, EPP_TOP, NULL, FALSE);
                    if (open_lock == NULL)
                        return NULL;

                    the_verdict = expect_and_eat(the_parser->tokenizer,
                                                 TK_RIGHT_PAREN);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_open_expression(open_lock);
                        return NULL;
                      }

                    decompose_open_expression(open_lock, &single_manager,
                                              &single_lock);
                  }
                else
                  {
                    single_lock = NULL;
                    single_manager = NULL;
                  }

                the_token = next_token(the_tokenizer);
                if ((the_token == NULL) ||
                    (get_token_kind(the_token) == TK_ERROR))
                  {
                    if (single_lock != NULL)
                        delete_expression(single_lock);
                    if (single_manager != NULL)
                        delete_unbound_name_manager(single_manager);
                    return NULL;
                  }

                if (get_token_kind(the_token) == TK_LEFT_CURLY_BRACE)
                  {
                    lock_declaration *single_lock_declaration;
                    declaration *single_declaration;
                    open_statement_block *open_block;
                    unbound_name_manager *block_manager;
                    statement_block *block;
                    statement *result_statement;
                    verdict the_verdict;

                    if (single_lock == NULL)
                      {
                        assert(single_manager == NULL);

                        single_lock_declaration =
                                create_lock_declaration(NULL);
                        if (single_lock_declaration == NULL)
                            return NULL;

                        single_declaration = create_declaration_for_lock(
                                "single-statement", FALSE, FALSE, TRUE,
                                single_lock_declaration,
                                get_token_location(the_token));
                        if (single_declaration == NULL)
                            return NULL;
                      }
                    else
                      {
                        single_declaration = NULL;
                      }

                    open_block = parse_braced_statement_block(the_parser);
                    if (open_block == NULL)
                      {
                        if (single_declaration != NULL)
                            declaration_remove_reference(single_declaration);
                        if (single_lock != NULL)
                            delete_expression(single_lock);
                        if (single_manager != NULL)
                            delete_unbound_name_manager(single_manager);
                        return NULL;
                      }

                    decompose_open_statement_block(open_block, &block_manager,
                                                   &block);

                    if (single_manager != NULL)
                      {
                        verdict the_verdict;

                        the_verdict = merge_in_unbound_name_manager(
                                single_manager, block_manager);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            delete_statement_block(block);
                            if (single_declaration != NULL)
                              {
                                declaration_remove_reference(
                                        single_declaration);
                              }
                            if (single_lock != NULL)
                                delete_expression(single_lock);
                            delete_unbound_name_manager(single_manager);
                            return NULL;
                          }

                        block_manager = single_manager;
                      }

                    result_statement = create_single_statement(single_lock,
                            block, single_declaration);
                    if (result_statement == NULL)
                      {
                        delete_unbound_name_manager(block_manager);
                        return NULL;
                      }

                    set_statement_end_location(result_statement,
                            next_token_location(the_parser->tokenizer));

                    the_verdict = expect_and_eat(the_parser->tokenizer,
                                                 TK_SEMICOLON);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_statement(result_statement);
                        delete_unbound_name_manager(block_manager);
                        return NULL;
                      }

                    result = create_open_statement(result_statement,
                                                   block_manager);
                    goto add_location;
                  }

                if (single_lock == NULL)
                  {
                    assert(single_manager == NULL);

                    single_lock = anonymous_lock_expression(&start_location);
                    if (single_lock == NULL)
                        return NULL;

                    single_manager = create_unbound_name_manager();
                    if (single_manager == NULL)
                      {
                        delete_expression(single_lock);
                        return NULL;
                      }
                  }

                result = parse_routine_or_variable_statement(the_parser,
                        single_lock, single_manager, NULL, PURE_UNSAFE);
                goto add_location;
              }
            else if (strcmp(id_chars, "cleanup") == 0)
              {
                result = parse_cleanup_statement(the_parser);
                goto add_location;
              }
            else if (strcmp(id_chars, "if") == 0)
              {
                result = parse_if_statement(the_parser);
                goto add_location;
              }
            else if (strcmp(id_chars, "switch") == 0)
              {
                result = parse_switch_statement(the_parser);
                goto add_location;
              }
            else if (strcmp(id_chars, "goto") == 0)
              {
                result = parse_goto_statement(the_parser);
                goto add_location;
              }
            else if (strcmp(id_chars, "return") == 0)
              {
                result = parse_return_statement(the_parser);
                goto add_location;
              }
            else if (strcmp(id_chars, "parallel") == 0)
              {
                verdict the_verdict;

                the_verdict = expect_and_eat_keyword(the_parser, "parallel");
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return NULL;

                the_token = next_token(the_tokenizer);
                if (the_token == NULL)
                    return NULL;

                kind = get_token_kind(the_token);

                if (kind != TK_IDENTIFIER)
                  {
                    token_error(the_token,
                            "Syntax error -- expected keyword `for' or keyword"
                            " `iterate' after keyword `parallel', but found "
                            "%s.", name_for_token_kind(kind));
                    return NULL;
                  }

                id_chars = aliased_token_name(the_token, the_parser);
                assert(id_chars != NULL);

                if (strcmp(id_chars, "for") == 0)
                  {
                    result = parse_for_statement(the_parser, TRUE,
                            current_labels, current_label_count);
                  }
                else if (strcmp(id_chars, "iterate") == 0)
                  {
                    result = parse_iterate_statement(the_parser, TRUE,
                            current_labels, current_label_count);
                  }
                else
                  {
                    token_error(the_token,
                            "Syntax error -- expected keyword `for' or keyword"
                            " `iterate' after keyword `parallel', but found "
                            "identifier `%s'.", id_chars);
                    return NULL;
                  }

                goto add_location;
              }
            else if (strcmp(id_chars, "for") == 0)
              {
                result = parse_for_statement(the_parser, FALSE, current_labels,
                                             current_label_count);
                goto add_location;
              }
            else if (strcmp(id_chars, "iterate") == 0)
              {
                result = parse_iterate_statement(the_parser, FALSE,
                        current_labels, current_label_count);
                goto add_location;
              }
            else if (strcmp(id_chars, "while") == 0)
              {
                result = parse_while_statement(the_parser, current_labels,
                                               current_label_count);
                goto add_location;
              }
            else if (strcmp(id_chars, "do") == 0)
              {
                result = parse_do_while_statement(the_parser, current_labels,
                                                  current_label_count);
                goto add_location;
              }
            else if (strcmp(id_chars, "break") == 0)
              {
                result = parse_break_statement(the_parser);
                goto add_location;
              }
            else if (strcmp(id_chars, "continue") == 0)
              {
                result = parse_continue_statement(the_parser);
                goto add_location;
              }
            else if (strcmp(id_chars, "try") == 0)
              {
                result = parse_try_statement(the_parser);
                goto add_location;
              }
            else if (strcmp(id_chars, "export") == 0)
              {
                result = parse_export_statement(the_parser);
                goto add_location;
              }
            else if (strcmp(id_chars, "hide") == 0)
              {
                result = parse_hide_statement(the_parser);
                goto add_location;
              }
            else if (strcmp(id_chars, "use") == 0)
              {
                result = parse_use_statement(the_parser);
                goto add_location;
              }
            else if (strcmp(id_chars, "include") == 0)
              {
                result = parse_include_statement(the_parser);
                goto add_location;
              }
            else if (strcmp(id_chars, "theorem") == 0)
              {
                result = parse_theorem_statement(the_parser);
                goto add_location;
              }
            else if (strcmp(id_chars, "alias") == 0)
              {
                result = parse_alias_statement(the_parser);
                goto add_location;
              }
            else
              {
                token *second_token;

                second_token = forward_token(the_tokenizer, 1);
                if (second_token == NULL)
                    return NULL;

                switch (get_token_kind(second_token))
                  {
                    case TK_COLON:
                        result = parse_label_statement(the_parser);
                        goto add_location;
                    case TK_ERROR:
                        return NULL;
                    default:
                        break;
                  }

                break;
              }
          }
        case TK_PLUS_PLUS:
          {
            result = parse_increment_statement(the_parser);
            goto add_location;
          }
        case TK_MINUS_MINUS:
          {
            result = parse_decrement_statement(the_parser);
            goto add_location;
          }
        case TK_LEFT_BRACKET:
          {
            result = parse_assignment_statement(the_parser);
            goto add_location;
          }
        case TK_LEFT_CURLY_BRACE:
          {
            result = parse_statement_block_statement(the_parser);
            goto add_location;
          }
        case TK_ERROR:
          {
            return NULL;
          }
        default:
          {
            break;
          }
      }

    the_open_expression = parse_expression(the_parser, EPP_TOP,
                                           &naked_call_overloading_use, FALSE);
    if (the_open_expression == NULL)
        return NULL;

    decompose_open_expression(the_open_expression, &manager, &the_expression);

    if (next_is(the_parser->tokenizer, TK_LOGICAL_NOT))
      {
        source_location call_location;
        verdict the_verdict;
        expression *print_base;
        unbound_use *use;
        expression *actual_argument_expressions[2];
        value *line_end_value;
        const char *formal_argument_names[2] = { NULL, NULL };
        call *print_call;
        statement *call_statement;

        set_source_location(&call_location, &start_location);
        set_location_end(&call_location,
                         next_token_location(the_parser->tokenizer));

        the_verdict = expect_and_eat(the_parser->tokenizer, TK_LOGICAL_NOT);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            source_location_remove_reference(&call_location);
            delete_expression(the_expression);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        print_base = create_unbound_name_reference_expression();
        if (print_base == NULL)
          {
            source_location_remove_reference(&call_location);
            delete_expression(the_expression);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        use = add_unbound_variable_reference(manager, "print", print_base,
                                             &call_location);
        if (use == NULL)
          {
            source_location_remove_reference(&call_location);
            delete_expression(print_base);
            delete_expression(the_expression);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        actual_argument_expressions[0] = the_expression;

        line_end_value = create_string_value("\n");
        if (line_end_value == NULL)
          {
            source_location_remove_reference(&call_location);
            delete_expression(print_base);
            delete_expression(the_expression);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        actual_argument_expressions[1] =
                create_constant_expression(line_end_value);
        value_remove_reference(line_end_value, NULL);
        if (actual_argument_expressions[1] == NULL)
          {
            source_location_remove_reference(&call_location);
            delete_expression(print_base);
            delete_expression(the_expression);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        print_call = create_call(print_base, 2, actual_argument_expressions,
                                 formal_argument_names, &call_location);
        if (print_call == NULL)
          {
            source_location_remove_reference(&call_location);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        call_statement = create_call_statement(print_call);
        if (call_statement == NULL)
          {
            source_location_remove_reference(&call_location);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        set_statement_start_location(call_statement, &call_location);
        set_statement_end_location(call_statement, &call_location);

        source_location_remove_reference(&call_location);

        return create_open_statement(call_statement, manager);
      }

    if (naked_call_overloading_use != NULL)
      {
        call *the_call;
        statement *call_statement;
        unbound_use *use;
        verdict the_verdict;

        remove_unbound_use(naked_call_overloading_use);

        assert(get_expression_kind(the_expression) == EK_CALL);
        the_call = call_expression_call(the_expression);
        assert(the_call != NULL);

        delete_call_expression_save_call(the_expression);

        call_statement = create_call_statement(the_call);
        if (call_statement == NULL)
          {
            delete_unbound_name_manager(manager);
            return NULL;
          }

        use = add_unbound_operator_statement(manager, "operator()",
                call_statement, get_call_location(the_call));
        if (use == NULL)
          {
            delete_statement(call_statement);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        the_verdict = do_statement_end_semicolon(the_parser, call_statement);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_statement(call_statement);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        result = create_open_statement(call_statement, manager);
        goto add_location;
      }

    if (!(expression_is_addressable(the_expression)))
      {
        expression_error(the_expression,
                "Syntax error -- assignment basket expression is not "
                "addressable.");
        delete_expression(the_expression);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    set_expression_addressable_required(the_expression);

    the_basket = create_expression_basket(the_expression);
    if (the_basket == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    result =
            parse_assignment_statement_suffix(the_parser, the_basket, manager);

  add_location:
    if (result != NULL)
      {
        set_statement_start_location(open_statement_statement(result),
                                     &start_location);
      }

    return result;
  }

extern open_statement *parse_routine_or_variable_statement(parser *the_parser,
        expression *single_lock, unbound_name_manager *single_lock_manager,
        native_bridge_routine *native_handler, purity_safety the_purity_safety)
  {
    unbound_name_manager *manager;
    declaration *the_declaration;
    statement *result_statement;
    verdict the_verdict;

    assert(the_parser != NULL);
    assert((single_lock == NULL) == (single_lock_manager == NULL));

    result_statement = create_declaration_statement();
    if (result_statement == NULL)
        return NULL;

    the_declaration = parse_declaration(the_parser, FALSE, FALSE, FALSE, FALSE,
            single_lock, single_lock_manager, &manager, FALSE, FALSE,
            native_handler, the_purity_safety, statement_add_declaration,
            result_statement, NULL, NULL);
    if (the_declaration == NULL)
      {
        delete_statement(result_statement);
        return NULL;
      }

    the_verdict = do_statement_end_semicolon(the_parser, result_statement);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_statement(result_statement);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    if (declaration_is_static(the_declaration))
      {
        unbound_name_manager *new_manager;
        verdict the_verdict;
        size_t declaration_count;
        size_t declaration_num;

        new_manager = create_unbound_name_manager();
        if (new_manager == NULL)
          {
            delete_statement(result_statement);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        the_verdict =
                merge_in_deferred_unbound_name_manager(new_manager, manager);
        manager = new_manager;
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_statement(result_statement);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        declaration_count =
                declaration_statement_declaration_count(result_statement);
        for (declaration_num = 0; declaration_num < declaration_count;
             ++declaration_num)
          {
            verdict the_verdict;

            the_verdict = unbound_name_manager_add_static(manager,
                    declaration_statement_declaration(result_statement,
                                                      declaration_num));
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_statement(result_statement);
                delete_unbound_name_manager(manager);
                return NULL;
              }
          }
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <cleanup-statement> :
 *          "cleanup" <statement-block> ";"
 */
extern open_statement *parse_cleanup_statement(parser *the_parser)
  {
    source_location start_location;
    verdict the_verdict;
    open_statement_block *open_body;
    unbound_name_manager *manager;
    statement_block *body;
    statement *result_statement;

    assert(the_parser != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat_keyword(the_parser, "cleanup");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    open_body = parse_statement_block(the_parser);
    if (open_body == NULL)
        return NULL;

    decompose_open_statement_block(open_body, &manager, &body);

    result_statement = create_cleanup_statement(body);
    if (result_statement == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_verdict = do_statement_end_semicolon(the_parser, result_statement);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <if-statement> :
 *          "if" "(" <expression> ")" <statement-block>
 *                  { <else-if-clause-list> }?
 *                  { "else" <non-if-statement-block> }? ";"
 *
 *      <else-if-clause-list> :
 *          <else-if-clause> |
 *          <else-if-clause> | <else-if-clause-list>
 *
 *      <else-if-clause> :
 *          "else" "if" "(" <expression> ")" <statement-block>
 */
extern open_statement *parse_if_statement(parser *the_parser)
  {
    verdict the_verdict;
    open_expression *open_test;
    unbound_name_manager *manager;
    expression *test;
    open_statement_block *open_body;
    unbound_name_manager *child_manager;
    statement_block *body;
    statement *result_statement;

    assert(the_parser != NULL);

    the_verdict = expect_and_eat_keyword(the_parser, "if");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_LEFT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    open_test = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
    if (open_test == NULL)
        return NULL;

    decompose_open_expression(open_test, &manager, &test);

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_RIGHT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_expression(test);
        return NULL;
      }

    open_body = parse_statement_block(the_parser);
    if (open_body == NULL)
      {
        delete_unbound_name_manager(manager);
        delete_expression(test);
        return NULL;
      }

    decompose_open_statement_block(open_body, &child_manager, &body);

    the_verdict = merge_in_unbound_name_manager(manager, child_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_expression(test);
        delete_statement_block(body);
        return NULL;
      }

    result_statement = create_if_statement(test, body, NULL);
    if (result_statement == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    while (next_is_keyword(the_parser, "else"))
      {
        verdict the_verdict;
        expression *else_if_test;
        open_statement_block *open_else_body;
        statement_block *else_body;

        the_verdict = expect_and_eat_keyword(the_parser, "else");
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_statement(result_statement);
            return NULL;
          }

        if (next_is_keyword(the_parser, "if"))
          {
            verdict the_verdict;
            open_expression *open_else_if_test;
            unbound_name_manager *test_manager;

            the_verdict = expect_and_eat_keyword(the_parser, "if");
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            the_verdict = expect_and_eat(the_parser->tokenizer, TK_LEFT_PAREN);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            open_else_if_test =
                    parse_expression(the_parser, EPP_TOP, NULL, FALSE);
            if (open_else_if_test == NULL)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            decompose_open_expression(open_else_if_test, &test_manager,
                                      &else_if_test);

            the_verdict = merge_in_unbound_name_manager(manager, test_manager);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(else_if_test);
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            the_verdict =
                    expect_and_eat(the_parser->tokenizer, TK_RIGHT_PAREN);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_expression(else_if_test);
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }
          }
        else
          {
            else_if_test = NULL;
          }

        open_else_body = parse_statement_block(the_parser);
        if (open_else_body == NULL)
          {
            if (else_if_test != NULL)
                delete_expression(else_if_test);
            delete_unbound_name_manager(manager);
            delete_statement(result_statement);
            return NULL;
          }

        decompose_open_statement_block(open_else_body, &child_manager,
                                       &else_body);

        the_verdict = merge_in_unbound_name_manager(manager, child_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_statement_block(else_body);
            if (else_if_test != NULL)
                delete_expression(else_if_test);
            delete_unbound_name_manager(manager);
            delete_statement(result_statement);
            return NULL;
          }

        if (else_if_test == NULL)
          {
            add_if_statement_else(result_statement, else_body);
            break;
          }

        the_verdict = add_if_statement_else_if(result_statement, else_if_test,
                                               else_body);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_statement(result_statement);
            return NULL;
          }
      }

    set_statement_end_location(result_statement,
                               next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_SEMICOLON);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <switch-statement> :
 *          "switch" "(" <expression> ")" <case-list> ";"
 *
 *      <case-list> :
 *          <empty> |
 *          <case> | <case-list>
 *
 *      <case> :
 *          "case" "(" <type-expression> ")" <statement-block>
 */
extern open_statement *parse_switch_statement(parser *the_parser)
  {
    verdict the_verdict;
    open_expression *open_base;
    expression *base;
    unbound_name_manager *manager;
    statement *result;

    the_verdict = expect_and_eat_keyword(the_parser, "switch");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_LEFT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    open_base = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
    if (open_base == NULL)
        return NULL;

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_RIGHT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_expression(open_base);
        return NULL;
      }

    decompose_open_expression(open_base, &manager, &base);

    result = create_switch_statement(base);
    if (result == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    while (TRUE)
      {
        token *the_token;
        verdict the_verdict;
        open_type_expression *open_case;
        type_expression *case_type;
        unbound_name_manager *child_manager;
        open_statement_block *open_body;
        statement_block *body;

        the_token = next_token(the_parser->tokenizer);
        if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
          {
            delete_unbound_name_manager(manager);
            delete_statement(result);
            return NULL;
          }

        if (get_token_kind(the_token) == TK_SEMICOLON)
            break;

        if (!(next_is_keyword(the_parser, "case")))
          {
            token_error(the_token,
                    "Syntax error -- expected %s or keyword `case'.",
                    name_for_token_kind(TK_SEMICOLON));
            delete_unbound_name_manager(manager);
            delete_statement(result);
            return NULL;
          }

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_statement(result);
            return NULL;
          }

        the_verdict = expect_and_eat(the_parser->tokenizer, TK_LEFT_PAREN);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_statement(result);
            return NULL;
          }

        open_case = parse_type_expression(the_parser, TEPP_TOP);
        if (open_case == NULL)
          {
            delete_unbound_name_manager(manager);
            delete_statement(result);
            return NULL;
          }

        the_verdict = expect_and_eat(the_parser->tokenizer, TK_RIGHT_PAREN);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_open_type_expression(open_case);
            delete_unbound_name_manager(manager);
            delete_statement(result);
            return NULL;
          }

        decompose_open_type_expression(open_case, &child_manager, &case_type);

        the_verdict = merge_in_unbound_name_manager(manager, child_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_type_expression(case_type);
            delete_unbound_name_manager(manager);
            delete_statement(result);
            return NULL;
          }

        open_body = parse_statement_block(the_parser);
        if (open_body == NULL)
          {
            delete_type_expression(case_type);
            delete_unbound_name_manager(manager);
            delete_statement(result);
            return NULL;
          }

        decompose_open_statement_block(open_body, &child_manager, &body);

        the_verdict = merge_in_unbound_name_manager(manager, child_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_statement_block(body);
            delete_type_expression(case_type);
            delete_unbound_name_manager(manager);
            delete_statement(result);
            return NULL;
          }

        the_verdict = add_switch_statement_case(result, case_type, body);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_statement(result);
            return NULL;
          }
      }

    set_statement_end_location(result,
                               next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_SEMICOLON);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result);
        return NULL;
      }

    return create_open_statement(result, manager);
  }

/*
 *      <goto-statement> :
 *          "goto" <expression> ";"
 */
extern open_statement *parse_goto_statement(parser *the_parser)
  {
    verdict the_verdict;
    open_expression *open_target;
    source_location end_location;
    unbound_name_manager *manager;
    expression *target;
    statement *result_statement;

    assert(the_parser != NULL);

    the_verdict = expect_and_eat_keyword(the_parser, "goto");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    open_target = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
    if (open_target == NULL)
        return NULL;

    end_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_SEMICOLON);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_expression(open_target);
        return NULL;
      }

    decompose_open_expression(open_target, &manager, &target);

    result_statement = create_goto_statement(target);
    if (result_statement == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    set_statement_end_location(result_statement, &end_location);

    return create_open_statement(result_statement, manager);
  }

/*
 *      <return-statement> :
 *          "return" { <expression> }? { "from" <identifier> }? ";"
 */
extern open_statement *parse_return_statement(parser *the_parser)
  {
    source_location start_location;
    verdict the_verdict;
    open_expression *open_return_value;
    char *name_copy;
    unbound_name_manager *manager;
    expression *return_value;
    statement *result_statement;
    unbound_use *use;

    assert(the_parser != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat_keyword(the_parser, "return");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    if (!next_is(the_parser->tokenizer, TK_SEMICOLON))
      {
        if (!next_is_keyword(the_parser, "from"))
          {
            open_return_value =
                    parse_expression(the_parser, EPP_TOP, NULL, FALSE);
            if (open_return_value == NULL)
                return NULL;
          }
        else
          {
            open_return_value = NULL;
          }

        if (next_is_keyword(the_parser, "from"))
          {
            the_verdict = expect_and_eat_keyword(the_parser, "from");
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                if (open_return_value != NULL)
                    delete_open_expression(open_return_value);
                return NULL;
              }

            name_copy = expect_and_eat_aliased_identifier_return_name_copy(
                    the_parser);
            if (name_copy == NULL)
              {
                if (open_return_value != NULL)
                    delete_open_expression(open_return_value);
                return NULL;
              }
          }
        else
          {
            name_copy = NULL;
          }
      }
    else
      {
        open_return_value = NULL;
        name_copy = NULL;
      }

    if (open_return_value == NULL)
      {
        manager = create_unbound_name_manager();
        if (manager == NULL)
          {
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        return_value = NULL;
      }
    else
      {
        decompose_open_expression(open_return_value, &manager, &return_value);
      }

    result_statement = create_return_statement(return_value);
    if (result_statement == NULL)
      {
        if (name_copy != NULL)
            free(name_copy);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_verdict = do_statement_end_semicolon(the_parser, result_statement);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_statement(result_statement);
        if (name_copy != NULL)
            free(name_copy);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    use = add_unbound_return(manager, name_copy, result_statement,
                             &start_location);
    if (name_copy != NULL)
        free(name_copy);
    if (use == NULL)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <for-statement> :
 *          { "parallel" }? "for" "(" <identifier> ";" <expression> ";"
 *                  <expression> { ";" <expression> }? ")"
 *                  <statement-block> ";"
 */
extern open_statement *parse_for_statement(parser *the_parser,
        boolean is_parallel, const char **current_labels,
        size_t current_label_count)
  {
    verdict the_verdict;
    source_location index_declaration_location;
    char *index_name_copy;
    open_expression *open_init;
    unbound_name_manager *manager;
    expression *init;
    open_expression *open_test;
    unbound_name_manager *child_manager;
    expression *test;
    expression *step;
    open_statement_block *open_body;
    statement_block *body;
    source_location end_location;
    statement *result_statement;

    assert(the_parser != NULL);
    assert((current_label_count == 0) || (current_labels != NULL));

    the_verdict = expect_and_eat_keyword(the_parser, "for");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_LEFT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    index_declaration_location = *(next_token_location(the_parser->tokenizer));

    index_name_copy = expect_and_eat_identifier_return_name_copy(the_parser);
    if (index_name_copy == NULL)
        return NULL;

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_SEMICOLON);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(index_name_copy);
        return NULL;
      }

    open_init = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
    if (open_init == NULL)
      {
        free(index_name_copy);
        return NULL;
      }

    decompose_open_expression(open_init, &manager, &init);

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_SEMICOLON);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(index_name_copy);
        delete_unbound_name_manager(manager);
        delete_expression(init);
        return NULL;
      }

    open_test = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
    if (open_test == NULL)
      {
        free(index_name_copy);
        delete_unbound_name_manager(manager);
        delete_expression(init);
        return NULL;
      }

    decompose_open_expression(open_test, &child_manager, &test);

    the_verdict = merge_in_unbound_name_manager(manager, child_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(index_name_copy);
        delete_unbound_name_manager(manager);
        delete_expression(init);
        delete_expression(test);
        return NULL;
      }

    if (next_is(the_parser->tokenizer, TK_SEMICOLON))
      {
        verdict the_verdict;
        open_expression *open_step;
        unbound_name_manager *child_manager;

        the_verdict = expect_and_eat(the_parser->tokenizer, TK_SEMICOLON);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            free(index_name_copy);
            delete_unbound_name_manager(manager);
            delete_expression(init);
            delete_expression(test);
            return NULL;
          }

        open_step = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
        if (open_step == NULL)
          {
            free(index_name_copy);
            delete_unbound_name_manager(manager);
            delete_expression(init);
            delete_expression(test);
            return NULL;
          }

        decompose_open_expression(open_step, &child_manager, &step);

        the_verdict = merge_in_unbound_name_manager(manager, child_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            free(index_name_copy);
            delete_unbound_name_manager(manager);
            delete_expression(init);
            delete_expression(test);
            delete_expression(step);
            return NULL;
          }
      }
    else
      {
        step = create_oi_expression(oi_one);
        if (step == NULL)
          {
            free(index_name_copy);
            delete_unbound_name_manager(manager);
            delete_expression(init);
            delete_expression(test);
            return NULL;
          }
      }

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_RIGHT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(index_name_copy);
        delete_unbound_name_manager(manager);
        delete_expression(init);
        delete_expression(test);
        delete_expression(step);
        return NULL;
      }

    open_body = parse_statement_block(the_parser);
    if (open_body == NULL)
      {
        free(index_name_copy);
        delete_unbound_name_manager(manager);
        delete_expression(init);
        delete_expression(test);
        delete_expression(step);
        return NULL;
      }

    decompose_open_statement_block(open_body, &child_manager, &body);

    the_verdict = merge_in_unbound_name_manager(manager, child_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(index_name_copy);
        delete_unbound_name_manager(manager);
        delete_expression(init);
        delete_expression(test);
        delete_expression(step);
        delete_statement_block(body);
        return NULL;
      }

    end_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_SEMICOLON);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(index_name_copy);
        delete_unbound_name_manager(manager);
        delete_expression(init);
        delete_expression(test);
        delete_expression(step);
        delete_statement_block(body);
        return NULL;
      }

    result_statement = create_for_statement(index_name_copy, init, test, step,
            body, is_parallel, &index_declaration_location);
    if (result_statement == NULL)
      {
        free(index_name_copy);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    set_statement_end_location(result_statement, &end_location);

    if (strcmp(try_aliasing(index_name_copy, the_parser), index_name_copy) !=
        0)
      {
        location_warning(&index_declaration_location,
                "`%s' was used to declare the index of a ``for'' statement, "
                "but that name is aliased to `%s', so referencing this index "
                "by this name will not work.", index_name_copy,
                try_aliasing(index_name_copy, the_parser));
      }

    the_verdict = bind_variable_name(manager, index_name_copy,
                                     for_statement_index(result_statement));
    free(index_name_copy);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    the_verdict = bind_break_and_continue(manager, result_statement,
            is_parallel, current_labels, current_label_count);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <iterate-statement> :
 *          { "parallel" }? "iterate" "(" <identifier> ";" <expression>
 *                  { ";" <expression> }? ")" <statement-block> ";"
 */
extern open_statement *parse_iterate_statement(parser *the_parser,
        boolean is_parallel, const char **current_labels,
        size_t current_label_count)
  {
    verdict the_verdict;
    source_location element_declaration_location;
    char *index_name_copy;
    open_expression *open_base;
    unbound_name_manager *manager;
    expression *base;
    unbound_name_manager *filter_manager;
    expression *filter;
    unbound_name_manager *child_manager;
    open_statement_block *open_body;
    statement_block *body;
    source_location end_location;
    statement *result_statement;

    assert(the_parser != NULL);
    assert((current_label_count == 0) || (current_labels != NULL));

    the_verdict = expect_and_eat_keyword(the_parser, "iterate");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_LEFT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    element_declaration_location =
            *(next_token_location(the_parser->tokenizer));

    index_name_copy = expect_and_eat_identifier_return_name_copy(the_parser);
    if (index_name_copy == NULL)
        return NULL;

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_SEMICOLON);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(index_name_copy);
        return NULL;
      }

    open_base = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
    if (open_base == NULL)
      {
        free(index_name_copy);
        return NULL;
      }

    decompose_open_expression(open_base, &manager, &base);

    if (next_is(the_parser->tokenizer, TK_SEMICOLON))
      {
        verdict the_verdict;
        open_expression *open_filter;

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            free(index_name_copy);
            delete_unbound_name_manager(manager);
            delete_expression(base);
            return NULL;
          }

        open_filter = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
        if (open_filter == NULL)
          {
            free(index_name_copy);
            delete_unbound_name_manager(manager);
            delete_expression(base);
            return NULL;
          }

        decompose_open_expression(open_filter, &filter_manager, &filter);
      }
    else
      {
        filter_manager = NULL;
        filter = NULL;
      }

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_RIGHT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (filter != NULL)
            delete_expression(filter);
        if (filter_manager != NULL)
            delete_unbound_name_manager(filter_manager);
        free(index_name_copy);
        delete_unbound_name_manager(manager);
        delete_expression(base);
        return NULL;
      }

    open_body = parse_statement_block(the_parser);
    if (open_body == NULL)
      {
        if (filter != NULL)
            delete_expression(filter);
        if (filter_manager != NULL)
            delete_unbound_name_manager(filter_manager);
        free(index_name_copy);
        delete_unbound_name_manager(manager);
        delete_expression(base);
        return NULL;
      }

    decompose_open_statement_block(open_body, &child_manager, &body);

    if (filter_manager != NULL)
      {
        verdict the_verdict;

        the_verdict =
                merge_in_unbound_name_manager(child_manager, filter_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_statement_block(body);
            delete_unbound_name_manager(child_manager);
            assert(filter != NULL);
            delete_expression(filter);
            delete_expression(base);
            delete_unbound_name_manager(manager);
            free(index_name_copy);
            return NULL;
          }
      }

    end_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_SEMICOLON);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (filter != NULL)
            delete_expression(filter);
        free(index_name_copy);
        delete_unbound_name_manager(child_manager);
        delete_unbound_name_manager(manager);
        delete_expression(base);
        delete_statement_block(body);
        return NULL;
      }

    result_statement = create_iterate_statement(index_name_copy, base, filter,
            body, is_parallel, &element_declaration_location);
    if (result_statement == NULL)
      {
        free(index_name_copy);
        delete_unbound_name_manager(child_manager);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    set_statement_end_location(result_statement, &end_location);

    if (strcmp(try_aliasing(index_name_copy, the_parser), index_name_copy) !=
        0)
      {
        location_warning(&element_declaration_location,
                "`%s' was used to declare the index of an ``iterate'' "
                "statement, but that name is aliased to `%s', so referencing "
                "this index by this name will not work.", index_name_copy,
                try_aliasing(index_name_copy, the_parser));
      }

    the_verdict = bind_variable_name(child_manager, index_name_copy,
            iterate_statement_element(result_statement));
    free(index_name_copy);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(child_manager);
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    the_verdict = bind_break_and_continue(child_manager, result_statement,
            is_parallel, current_labels, current_label_count);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(child_manager);
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    the_verdict = merge_in_unbound_name_manager(manager, child_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <while-statement> :
 *          "while" "(" <expression> ")" <optional-step> <statement-block>
 *                  <optional-step> ";"
 *
 *      <optional-step> :
 *          <empty> |
 *          "step" <statement-block>
 */
extern open_statement *parse_while_statement(parser *the_parser,
        const char **current_labels, size_t current_label_count)
  {
    verdict the_verdict;
    open_expression *open_test;
    unbound_name_manager *manager;
    expression *test;
    statement_block *step;
    open_statement_block *open_body;
    unbound_name_manager *child_manager;
    statement_block *body;
    statement *result_statement;

    assert(the_parser != NULL);
    assert((current_label_count == 0) || (current_labels != NULL));

    the_verdict = expect_and_eat_keyword(the_parser, "while");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_LEFT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    open_test = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
    if (open_test == NULL)
        return NULL;

    decompose_open_expression(open_test, &manager, &test);

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_RIGHT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_expression(test);
        return NULL;
      }

    if (next_is_keyword(the_parser, "step"))
      {
        verdict the_verdict;
        open_statement_block *open_step;

        the_verdict = expect_and_eat_keyword(the_parser, "step");
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_expression(test);
            return NULL;
          }

        open_step = parse_statement_block(the_parser);
        if (open_step == NULL)
          {
            delete_unbound_name_manager(manager);
            delete_expression(test);
            return NULL;
          }

        decompose_open_statement_block(open_step, &child_manager, &step);

        the_verdict = merge_in_unbound_name_manager(manager, child_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_expression(test);
            delete_statement_block(step);
            return NULL;
          }
      }
    else
      {
        step = NULL;
      }

    open_body = parse_statement_block(the_parser);
    if (open_body == NULL)
      {
        delete_unbound_name_manager(manager);
        delete_expression(test);
        if (step != NULL)
            delete_statement_block(step);
        return NULL;
      }

    decompose_open_statement_block(open_body, &child_manager, &body);

    the_verdict = merge_in_unbound_name_manager(manager, child_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_expression(test);
        if (step != NULL)
            delete_statement_block(step);
        delete_statement_block(body);
        return NULL;
      }

    if ((step == NULL) && next_is_keyword(the_parser, "step"))
      {
        verdict the_verdict;
        open_statement_block *open_step;

        the_verdict = expect_and_eat_keyword(the_parser, "step");
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_expression(test);
            delete_statement_block(body);
            return NULL;
          }

        open_step = parse_statement_block(the_parser);
        if (open_step == NULL)
          {
            delete_unbound_name_manager(manager);
            delete_expression(test);
            delete_statement_block(body);
            return NULL;
          }

        decompose_open_statement_block(open_step, &child_manager, &step);

        the_verdict = merge_in_unbound_name_manager(manager, child_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_expression(test);
            delete_statement_block(body);
            delete_statement_block(step);
            return NULL;
          }
      }

    result_statement = create_while_statement(test, body, step);
    if (result_statement == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    set_statement_end_location(result_statement,
                               next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_SEMICOLON);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    the_verdict = bind_break_and_continue(manager, result_statement, FALSE,
                                          current_labels, current_label_count);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <do-while-statement> :
 *          "do" <statement-block> "while" "(" <expression> ")"
 *                  <optional-step> ";"
 */
extern open_statement *parse_do_while_statement(parser *the_parser,
        const char **current_labels, size_t current_label_count)
  {
    verdict the_verdict;
    open_statement_block *open_body;
    unbound_name_manager *manager;
    statement_block *body;
    open_expression *open_test;
    unbound_name_manager *child_manager;
    expression *test;
    statement_block *step;
    statement *result_statement;

    assert(the_parser != NULL);
    assert((current_label_count == 0) || (current_labels != NULL));

    the_verdict = expect_and_eat_keyword(the_parser, "do");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    open_body = parse_statement_block(the_parser);
    if (open_body == NULL)
        return NULL;

    decompose_open_statement_block(open_body, &manager, &body);

    the_verdict = expect_and_eat_keyword(the_parser, "while");
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement_block(body);
        return NULL;
      }

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_LEFT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement_block(body);
        return NULL;
      }

    open_test = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
    if (open_test == NULL)
      {
        delete_unbound_name_manager(manager);
        delete_statement_block(body);
        return NULL;
      }

    decompose_open_expression(open_test, &child_manager, &test);

    the_verdict = merge_in_unbound_name_manager(manager, child_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement_block(body);
        delete_expression(test);
        return NULL;
      }

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_RIGHT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement_block(body);
        delete_expression(test);
        return NULL;
      }

    if (next_is_keyword(the_parser, "step"))
      {
        verdict the_verdict;
        open_statement_block *open_step;

        the_verdict = expect_and_eat_keyword(the_parser, "step");
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_statement_block(body);
            delete_expression(test);
            return NULL;
          }

        open_step = parse_statement_block(the_parser);
        if (open_step == NULL)
          {
            delete_unbound_name_manager(manager);
            delete_statement_block(body);
            delete_expression(test);
            return NULL;
          }

        decompose_open_statement_block(open_step, &child_manager, &step);

        the_verdict = merge_in_unbound_name_manager(manager, child_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_statement_block(body);
            delete_expression(test);
            delete_statement_block(step);
            return NULL;
          }
      }
    else
      {
        step = NULL;
      }

    result_statement = create_do_while_statement(test, body, step);
    if (result_statement == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    set_statement_end_location(result_statement,
                               next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_SEMICOLON);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    the_verdict = bind_break_and_continue(manager, result_statement, FALSE,
                                          current_labels, current_label_count);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <break-statement> :
 *          "break" { "from" <identifier> }? ";"
 */
extern open_statement *parse_break_statement(parser *the_parser)
  {
    source_location start_location;
    verdict the_verdict;
    char *name_copy;
    unbound_name_manager *manager;
    statement *result_statement;
    unbound_use *use;

    assert(the_parser != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat_keyword(the_parser, "break");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    if (next_is_keyword(the_parser, "from"))
      {
        the_verdict = expect_and_eat_keyword(the_parser, "from");
        if (the_verdict != MISSION_ACCOMPLISHED)
            return NULL;

        name_copy =
                expect_and_eat_aliased_identifier_return_name_copy(the_parser);
        if (name_copy == NULL)
            return NULL;
      }
    else
      {
        name_copy = NULL;
      }

    manager = create_unbound_name_manager();
    if (manager == NULL)
      {
        if (name_copy != NULL)
            free(name_copy);
        return NULL;
      }

    result_statement = create_break_statement();
    if (result_statement == NULL)
      {
        if (name_copy != NULL)
            free(name_copy);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_verdict = do_statement_end_semicolon(the_parser, result_statement);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (name_copy != NULL)
            free(name_copy);
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    use = add_unbound_break(manager, name_copy, result_statement,
                            &start_location);
    if (name_copy != NULL)
        free(name_copy);
    if (use == NULL)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <continue-statement> :
 *          "continue" { "with" <identifier> }? ";"
 */
extern open_statement *parse_continue_statement(parser *the_parser)
  {
    source_location start_location;
    verdict the_verdict;
    char *name_copy;
    unbound_name_manager *manager;
    statement *result_statement;
    unbound_use *use;

    assert(the_parser != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat_keyword(the_parser, "continue");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    if (next_is_keyword(the_parser, "with"))
      {
        the_verdict = expect_and_eat_keyword(the_parser, "with");
        if (the_verdict != MISSION_ACCOMPLISHED)
            return NULL;

        name_copy =
                expect_and_eat_aliased_identifier_return_name_copy(the_parser);
        if (name_copy == NULL)
            return NULL;
      }
    else
      {
        name_copy = NULL;
      }

    manager = create_unbound_name_manager();
    if (manager == NULL)
      {
        if (name_copy != NULL)
            free(name_copy);
        return NULL;
      }

    result_statement = create_continue_statement();
    if (result_statement == NULL)
      {
        if (name_copy != NULL)
            free(name_copy);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_verdict = do_statement_end_semicolon(the_parser, result_statement);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (name_copy != NULL)
            free(name_copy);
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    use = add_unbound_continue(manager, name_copy, result_statement,
                               &start_location);
    if (name_copy != NULL)
        free(name_copy);
    if (use == NULL)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <export-statement> :
 *          "export" { <export-item-list> }? { "from" <identifier> }? ";"
 *
 *      <export-item-list> :
 *          <export-item> |
 *          <export-item-list> "," <export-item>
 *
 *      <export-item> :
 *          <identifier> { "as" <identifier> }?
 */
extern open_statement *parse_export_statement(parser *the_parser)
  {
    source_location start_location;
    verdict the_verdict;
    unbound_name_manager *manager;
    statement *result_statement;
    char *from_copy;
    unbound_use *use;

    assert(the_parser != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat_keyword(the_parser, "export");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    manager = create_unbound_name_manager();
    if (manager == NULL)
        return NULL;

    result_statement = create_export_statement();
    if (result_statement == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    if (!(next_is(the_parser->tokenizer, TK_SEMICOLON)))
      {
        while (TRUE)
          {
            verdict the_verdict;
            token *the_token;
            const char *id_chars;
            expression *name_expression;
            unbound_use *use;
            char *export_name;

            the_verdict = expect(the_parser->tokenizer, TK_IDENTIFIER);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            the_token = next_token(the_parser->tokenizer);
            assert(the_token != NULL);

            id_chars = aliased_token_name(the_token, the_parser);
            assert(id_chars != NULL);

            if (strcmp(id_chars, "from") == 0)
              {
                verdict the_verdict;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_unbound_name_manager(manager);
                    delete_statement(result_statement);
                    return NULL;
                  }

                from_copy = expect_and_eat_aliased_identifier_return_name_copy(
                        the_parser);
                if (from_copy == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    delete_statement(result_statement);
                    return NULL;
                  }

                break;
              }

            name_expression = create_unbound_name_reference_expression();
            if (name_expression == NULL)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            use = add_unbound_variable_reference(manager, id_chars,
                    name_expression, &start_location);
            if (use == NULL)
              {
                delete_expression(name_expression);
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            export_name =
                    expect_and_eat_identifier_return_name_copy(the_parser);
            if (export_name == NULL)
              {
                delete_expression(name_expression);
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            if (next_is_keyword(the_parser, "as"))
              {
                verdict the_verdict;

                free(export_name);

                the_verdict = expect_and_eat_keyword(the_parser, "as");
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_expression(name_expression);
                    delete_unbound_name_manager(manager);
                    delete_statement(result_statement);
                    return NULL;
                  }

                export_name =
                        expect_and_eat_identifier_return_name_copy(the_parser);
                if (export_name == NULL)
                  {
                    delete_expression(name_expression);
                    delete_unbound_name_manager(manager);
                    delete_statement(result_statement);
                    return NULL;
                  }
              }

            the_verdict = export_statement_add_item(result_statement,
                    export_name, name_expression);
            free(export_name);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            if (next_is(the_parser->tokenizer, TK_SEMICOLON))
              {
                from_copy = NULL;
                break;
              }

            the_verdict = expect_and_eat(the_parser->tokenizer, TK_COMMA);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }
          }
      }
    else
      {
        from_copy = NULL;
      }

    use = add_unbound_export(manager, from_copy, result_statement,
                             &start_location);
    if (from_copy != NULL)
        free(from_copy);
    if (use == NULL)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    the_verdict = do_statement_end_semicolon(the_parser, result_statement);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <hide-statement> :
 *          "hide" { <hide-item-list> }? { "from" <identifier> }? ";"
 *
 *      <hide-item-list> :
 *          <hide-item> |
 *          <hide-item-list> "," <hide-item>
 *
 *      <hide-item> :
 *          <identifier>
 */
extern open_statement *parse_hide_statement(parser *the_parser)
  {
    source_location start_location;
    verdict the_verdict;
    unbound_name_manager *manager;
    statement *result_statement;
    char *from_copy;
    unbound_use *use;

    assert(the_parser != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat_keyword(the_parser, "hide");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    manager = create_unbound_name_manager();
    if (manager == NULL)
        return NULL;

    result_statement = create_hide_statement();
    if (result_statement == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    if (!(next_is(the_parser->tokenizer, TK_SEMICOLON)))
      {
        while (TRUE)
          {
            verdict the_verdict;
            token *the_token;
            const char *id_chars;

            the_verdict = expect(the_parser->tokenizer, TK_IDENTIFIER);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            the_token = next_token(the_parser->tokenizer);
            assert(the_token != NULL);

            id_chars = aliased_token_name(the_token, the_parser);
            assert(id_chars != NULL);

            if (strcmp(id_chars, "from") == 0)
              {
                verdict the_verdict;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_unbound_name_manager(manager);
                    delete_statement(result_statement);
                    return NULL;
                  }

                from_copy = expect_and_eat_aliased_identifier_return_name_copy(
                        the_parser);
                if (from_copy == NULL)
                  {
                    delete_unbound_name_manager(manager);
                    delete_statement(result_statement);
                    return NULL;
                  }

                break;
              }

            the_verdict = hide_statement_add_item(result_statement, id_chars);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            if (next_is(the_parser->tokenizer, TK_SEMICOLON))
              {
                from_copy = NULL;
                break;
              }

            the_verdict = expect_and_eat(the_parser->tokenizer, TK_COMMA);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }
          }
      }
    else
      {
        from_copy = NULL;
      }

    use = add_unbound_hide(manager, from_copy, result_statement,
                           &start_location);
    if (from_copy != NULL)
        free(from_copy);
    if (use == NULL)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    the_verdict = do_statement_end_semicolon(the_parser, result_statement);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <use-statement> :
 *          "use" { <identifier> ":=" }?
 *                  { <expression> | <string-literal-token> }
 *                  { ":"
 *                    { <type-expression> | <string-literal-token> } }?
 *                  { <use-suffix> }? ";"
 *
 *      <use-suffix> :
 *          "for" <export-item-list> |
 *          "except" <hide-item-list>
 */
extern open_statement *parse_use_statement(parser *the_parser)
  {
    source_location start_location;
    verdict the_verdict;
    char *name_copy;
    unbound_name_manager *manager;
    expression *to_use;
    type_expression *use_type;
    unbound_name_manager *type_manager;
    statement *result_statement;

    assert(the_parser != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat_keyword(the_parser, "use");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    name_copy = NULL;
    if (next_is(the_parser->tokenizer, TK_IDENTIFIER))
      {
        token *second_token;

        second_token = forward_token(the_parser->tokenizer, 1);
        if ((second_token != NULL) &&
            (get_token_kind(second_token) == TK_ASSIGN))
          {
            verdict the_verdict;

            name_copy = expect_and_eat_identifier_return_name_copy(the_parser);
            if (name_copy == NULL)
                return NULL;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                free(name_copy);
                return NULL;
              }
          }
      }

    if (next_is(the_parser->tokenizer, TK_STRING_LITERAL))
      {
        token *the_token;
        const char *include_file;
        statement_block *body;
        string_aa current_labels;
        verdict the_verdict;
        type *the_type;
        type_expression *return_type;
        formal_arguments *formals;
        routine_declaration *the_routine;
        declaration *the_declaration;
        expression *call_base;
        call *the_call;

        the_token = next_token(the_parser->tokenizer);
        assert(the_token != NULL);

        include_file = string_literal_token_data(the_token);
        assert(include_file != NULL);

        body = create_statement_block();
        if (body == NULL)
          {
            consume_token(the_parser->tokenizer);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        manager = create_unbound_name_manager();
        if (manager == NULL)
          {
            consume_token(the_parser->tokenizer);
            delete_statement_block(body);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        the_verdict = string_aa_init(&current_labels, 10);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            consume_token(the_parser->tokenizer);
            delete_unbound_name_manager(manager);
            delete_statement_block(body);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        the_verdict = (*the_parser->include_handler)(
                the_parser->include_handler_data, include_file, body, manager,
                &current_labels, get_token_location(the_token),
                the_parser->alias_manager,
                the_parser->native_bridge_dll_body_allowed);
        free(current_labels.array);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            consume_token(the_parser->tokenizer);
            delete_unbound_name_manager(manager);
            delete_statement_block(body);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_statement_block(body);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        the_verdict = resolve_statement_block(body, manager,
                                              the_parser->alias_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_statement_block(body);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        the_type = get_anything_type();
        if (the_type == NULL)
          {
            delete_unbound_name_manager(manager);
            delete_statement_block(body);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        return_type = create_constant_type_expression(the_type);
        if (return_type == NULL)
          {
            delete_unbound_name_manager(manager);
            delete_statement_block(body);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        formals = create_formal_arguments();
        if (formals == NULL)
          {
            delete_type_expression(return_type);
            delete_unbound_name_manager(manager);
            delete_statement_block(body);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        the_routine = create_routine_declaration(return_type, NULL, formals,
                FALSE, body, NULL, PURE_UNSAFE, FALSE, TRUE, NULL, 0, NULL);
        if (the_routine == NULL)
          {
            delete_unbound_name_manager(manager);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        the_declaration = create_declaration_for_routine(NULL, FALSE, FALSE,
                FALSE, the_routine, &start_location);
        if (the_declaration == NULL)
          {
            delete_unbound_name_manager(manager);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        the_verdict = bind_return(manager, the_routine);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            declaration_remove_reference(the_declaration);
            delete_unbound_name_manager(manager);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        call_base = create_declaration_expression(the_declaration);
        if (call_base == NULL)
          {
            delete_unbound_name_manager(manager);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        the_call = create_call(call_base, 0, NULL, NULL, &start_location);
        if (the_call == NULL)
          {
            delete_unbound_name_manager(manager);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        to_use = create_call_expression(the_call);
        if (to_use == NULL)
          {
            delete_unbound_name_manager(manager);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }
      }
    else
      {
        open_expression *open_to_use;

        open_to_use = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
        if (open_to_use == NULL)
          {
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        decompose_open_expression(open_to_use, &manager, &to_use);
      }

    type_manager = NULL;

    if (next_is(the_parser->tokenizer, TK_COLON))
      {
        verdict the_verdict;

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_expression(to_use);
            delete_unbound_name_manager(manager);
            if (name_copy != NULL)
                free(name_copy);
            return NULL;
          }

        if (next_is(the_parser->tokenizer, TK_STRING_LITERAL))
          {
            token *the_token;
            const char *include_file;
            verdict the_verdict;

            the_token = next_token(the_parser->tokenizer);
            assert(the_token != NULL);

            include_file = string_literal_token_data(the_token);
            assert(include_file != NULL);

            use_type = create_interface_type_expression(FALSE);
            if (use_type == NULL)
              {
                consume_token(the_parser->tokenizer);
                delete_expression(to_use);
                delete_unbound_name_manager(manager);
                if (name_copy != NULL)
                    free(name_copy);
                return NULL;
              }

            the_verdict = (*(the_parser->interface_include_handler))(
                    the_parser->include_handler_data, include_file, use_type,
                    &type_manager, get_token_location(the_token),
                    the_parser->alias_manager,
                    the_parser->native_bridge_dll_body_allowed);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                consume_token(the_parser->tokenizer);
                delete_type_expression(use_type);
                delete_expression(to_use);
                delete_unbound_name_manager(manager);
                if (name_copy != NULL)
                    free(name_copy);
                return NULL;
              }

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                if (type_manager != NULL)
                    delete_unbound_name_manager(type_manager);
                delete_type_expression(use_type);
                delete_expression(to_use);
                delete_unbound_name_manager(manager);
                if (name_copy != NULL)
                    free(name_copy);
                return NULL;
              }
          }
        else
          {
            open_type_expression *open_type;
            unbound_name_manager *child_manager;
            verdict the_verdict;

            open_type = parse_type_expression(the_parser, TEPP_TOP);
            if (open_type == NULL)
              {
                delete_expression(to_use);
                delete_unbound_name_manager(manager);
                if (name_copy != NULL)
                    free(name_copy);
                return NULL;
              }

            decompose_open_type_expression(open_type, &child_manager,
                                           &use_type);

            the_verdict =
                    merge_in_unbound_name_manager(manager, child_manager);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_type_expression(use_type);
                delete_expression(to_use);
                delete_unbound_name_manager(manager);
                if (name_copy != NULL)
                    free(name_copy);
                return NULL;
              }
          }
      }
    else
      {
        use_type = NULL;
      }

    result_statement = create_use_statement(to_use, use_type, name_copy);
    if (name_copy != NULL)
        free(name_copy);
    if (result_statement == NULL)
      {
        if (type_manager != NULL)
            delete_unbound_name_manager(type_manager);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    if (type_manager != NULL)
      {
        size_t name_count;
        size_t name_num;
        verdict the_verdict;

        name_count = unbound_name_count(type_manager);
        for (name_num = 0; name_num < name_count; ++name_num)
          {
            unbound_name *the_unbound_name;
            const char *name_string;
            verdict the_verdict;

            the_unbound_name = unbound_name_number(type_manager,
                    (name_count - (name_num + 1)));
            assert(the_unbound_name != NULL);

            name_string = unbound_name_string(the_unbound_name);
            assert(name_string != NULL);

            the_verdict = handle_use_statement_for_unbound_name(
                    result_statement, name_string, the_unbound_name,
                    type_manager, TRUE);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                if (type_manager != NULL)
                    delete_unbound_name_manager(type_manager);
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }
          }

        the_verdict = merge_in_unbound_name_manager(manager, type_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_statement(result_statement);
            return NULL;
          }
      }

    if (next_is_keyword(the_parser, "for"))
      {
        verdict the_verdict;

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_statement(result_statement);
            return NULL;
          }

        while (TRUE)
          {
            verdict the_verdict;
            char *source_name;
            char *export_name;

            source_name =
                    expect_and_eat_identifier_return_name_copy(the_parser);
            if (source_name == NULL)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            if (next_is_keyword(the_parser, "as"))
              {
                verdict the_verdict;

                the_verdict = expect_and_eat_keyword(the_parser, "as");
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    free(source_name);
                    delete_unbound_name_manager(manager);
                    delete_statement(result_statement);
                    return NULL;
                  }

                export_name =
                        expect_and_eat_identifier_return_name_copy(the_parser);
                if (export_name == NULL)
                  {
                    free(source_name);
                    delete_unbound_name_manager(manager);
                    delete_statement(result_statement);
                    return NULL;
                  }
              }
            else
              {
                export_name = source_name;
              }

            the_verdict = use_statement_add_for_item(result_statement,
                    export_name, source_name);
            if (export_name != source_name)
                free(export_name);
            free(source_name);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            if (next_is(the_parser->tokenizer, TK_SEMICOLON))
                break;

            the_verdict = expect_and_eat(the_parser->tokenizer, TK_COMMA);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }
          }
      }
    else if (next_is_keyword(the_parser, "except"))
      {
        verdict the_verdict;

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(manager);
            delete_statement(result_statement);
            return NULL;
          }

        while (TRUE)
          {
            verdict the_verdict;
            token *the_token;
            const char *id_chars;

            the_verdict = expect(the_parser->tokenizer, TK_IDENTIFIER);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            the_token = next_token(the_parser->tokenizer);
            assert(the_token != NULL);

            id_chars = identifier_token_name(the_token);
            assert(id_chars != NULL);

            the_verdict =
                    use_statement_add_except_item(result_statement, id_chars);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }

            if (next_is(the_parser->tokenizer, TK_SEMICOLON))
                break;

            the_verdict = expect_and_eat(the_parser->tokenizer, TK_COMMA);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(manager);
                delete_statement(result_statement);
                return NULL;
              }
          }
      }

    the_verdict = do_statement_end_semicolon(the_parser, result_statement);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result_statement);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <include-statement> :
 *          "include" <string-literal-token> ";"
 */
extern open_statement *parse_include_statement(parser *the_parser)
  {
    verdict the_verdict;
    token *the_token;
    const char *include_file;
    unbound_name_manager *manager;
    statement *result_statement;

    assert(the_parser != NULL);

    the_verdict = expect_and_eat_keyword(the_parser, "include");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    the_verdict = expect(the_parser->tokenizer, TK_STRING_LITERAL);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    the_token = next_token(the_parser->tokenizer);
    assert(the_token != NULL);
    assert(get_token_kind(the_token) == TK_STRING_LITERAL);

    include_file = string_literal_token_data(the_token);
    assert(include_file != NULL);

    manager = create_unbound_name_manager();
    if (manager == NULL)
        return NULL;

    result_statement = create_include_statement(include_file);
    if (result_statement == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_verdict = consume_token(the_parser->tokenizer);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_statement(result_statement);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_verdict = do_statement_end_semicolon(the_parser, result_statement);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_statement(result_statement);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <theorem-statement> :
 *          "theorem" "(" <expression> ")"
 *                  { "proof" "{" <proof-item-list> "}" }? ";"
 *
 *      <proof-item-list> :
 *          <proof-item> |
 *          <proof-item> <proof-item-list>
 */
extern open_statement *parse_theorem_statement(parser *the_parser)
  {
    verdict the_verdict;
    open_expression *open_claim;
    expression *claim;
    unbound_name_manager *manager;
    statement *result_statement;

    assert(the_parser != NULL);

    the_verdict = expect_and_eat_keyword(the_parser, "theorem");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_LEFT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    open_claim = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
    if (open_claim == NULL)
        return NULL;

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_RIGHT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_expression(open_claim);
        return NULL;
      }

    if (next_is_keyword(the_parser, "proof"))
      {
        verdict the_verdict;

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_open_expression(open_claim);
            return NULL;
          }

        the_verdict =
                expect_and_eat(the_parser->tokenizer, TK_LEFT_CURLY_BRACE);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_open_expression(open_claim);
            return NULL;
          }

        the_verdict =
                ignore_until(the_parser->tokenizer, TK_RIGHT_CURLY_BRACE);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_open_expression(open_claim);
            return NULL;
          }

        the_verdict =
                expect_and_eat(the_parser->tokenizer, TK_RIGHT_CURLY_BRACE);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_open_expression(open_claim);
            return NULL;
          }
      }

    decompose_open_expression(open_claim, &manager, &claim);

    result_statement = create_theorem_statement(claim);
    if (result_statement == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_verdict = do_statement_end_semicolon(the_parser, result_statement);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_statement(result_statement);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <alias-statement> :
 *          "alias" <identifier> <identifier> ";"
 */
extern open_statement *parse_alias_statement(parser *the_parser)
  {
    verdict the_verdict;
    char *alias_copy;
    char *target_copy;
    statement *result_statement;
    unbound_name_manager *manager;
    string_index *local;

    assert(the_parser != NULL);

    the_verdict = expect_and_eat_keyword(the_parser, "alias");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    alias_copy = expect_and_eat_identifier_return_name_copy(the_parser);
    if (alias_copy == NULL)
        return NULL;

    target_copy =
            expect_and_eat_aliased_identifier_return_name_copy(the_parser);
    if (target_copy == NULL)
      {
        free(alias_copy);
        return NULL;
      }

    result_statement = create_alias_statement(alias_copy, target_copy);
    if (result_statement == NULL)
        return NULL;

    manager = create_unbound_name_manager();
    if (manager == NULL)
      {
        delete_statement(result_statement);
        return NULL;
      }

    the_verdict = do_statement_end_semicolon(the_parser, result_statement);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_statement(result_statement);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    local = the_parser->alias_manager->local;
    if (local == NULL)
      {
        local = create_string_index();
        if (local == NULL)
          {
            delete_statement(result_statement);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        the_parser->alias_manager->local = local;
      }

    assert(local != NULL);

    the_verdict = enter_into_string_index(local, alias_copy, target_copy);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_statement(result_statement);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

extern open_statement *parse_assignment_statement(parser *the_parser)
  {
    token *the_token;
    open_basket *the_open_basket;
    unbound_name_manager *manager;
    basket *the_basket;

    assert(the_parser != NULL);

    the_token = next_token(the_parser->tokenizer);
    if (the_token == NULL)
        return NULL;

    the_open_basket = parse_basket(the_parser);
    if (the_open_basket == NULL)
        return NULL;

    decompose_open_basket(the_open_basket, &manager, &the_basket);

    return parse_assignment_statement_suffix(the_parser, the_basket, manager);
  }

extern open_statement *parse_assignment_statement_suffix(parser *the_parser,
        basket *the_basket, unbound_name_manager *manager)
  {
    source_location operator_location;
    token *the_token;
    assignment_kind the_assignment_kind;
    verdict the_verdict;
    open_expression *the_open_expression;
    source_location end_location;
    unbound_name_manager *expression_manager;
    expression *the_expression;
    statement *the_statement;

    operator_location = *(next_token_location(the_parser->tokenizer));

    the_token = next_token(the_parser->tokenizer);
    if (the_token == NULL)
      {
        delete_basket(the_basket);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    switch (get_token_kind(the_token))
      {
        case TK_ASSIGN:
            the_assignment_kind = AK_SIMPLE;
            break;
        case TK_MODULO_ASSIGN:
            the_assignment_kind = AK_MODULO;
            break;
        case TK_MULTIPLY_ASSIGN:
            the_assignment_kind = AK_MULTIPLY;
            break;
        case TK_DIVIDE_ASSIGN:
            the_assignment_kind = AK_DIVIDE;
            break;
        case TK_DIVIDE_FORCE_ASSIGN:
            the_assignment_kind = AK_DIVIDE_FORCE;
            break;
        case TK_REMAINDER_ASSIGN:
            the_assignment_kind = AK_REMAINDER;
            break;
        case TK_ADD_ASSIGN:
            the_assignment_kind = AK_ADD;
            break;
        case TK_SUBTRACT_ASSIGN:
            the_assignment_kind = AK_SUBTRACT;
            break;
        case TK_SHIFT_LEFT_ASSIGN:
            the_assignment_kind = AK_SHIFT_LEFT;
            break;
        case TK_SHIFT_RIGHT_ASSIGN:
            the_assignment_kind = AK_SHIFT_RIGHT;
            break;
        case TK_BITWISE_AND_ASSIGN:
            the_assignment_kind = AK_BITWISE_AND;
            break;
        case TK_BITWISE_XOR_ASSIGN:
            the_assignment_kind = AK_BITWISE_XOR;
            break;
        case TK_BITWISE_OR_ASSIGN:
            the_assignment_kind = AK_BITWISE_OR;
            break;
        case TK_LOGICAL_AND_ASSIGN:
            the_assignment_kind = AK_LOGICAL_AND;
            break;
        case TK_LOGICAL_OR_ASSIGN:
            the_assignment_kind = AK_LOGICAL_OR;
            break;
        case TK_CONCATENATE_ASSIGN:
            the_assignment_kind = AK_CONCATENATE;
            break;
        case TK_ERROR:
            delete_basket(the_basket);
            delete_unbound_name_manager(manager);
            return NULL;
        default:
            token_error(the_token,
                    "Syntax error -- expected assignment operator, found %s.",
                    name_for_token_kind(get_token_kind(the_token)));
            delete_basket(the_basket);
            delete_unbound_name_manager(manager);
            return NULL;
      }

    the_verdict = consume_token(the_parser->tokenizer);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_basket(the_basket);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_open_expression = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
    if (the_open_expression == NULL)
      {
        delete_basket(the_basket);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    end_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_SEMICOLON);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_expression(the_open_expression);
        delete_basket(the_basket);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    decompose_open_expression(the_open_expression, &expression_manager,
                              &the_expression);

    the_statement = create_assign_statement(the_basket, the_expression,
                                            the_assignment_kind);
    if (the_statement == NULL)
      {
        delete_unbound_name_manager(manager);
        delete_unbound_name_manager(expression_manager);
        return NULL;
      }

    set_statement_end_location(the_statement, &end_location);

    the_verdict = merge_in_unbound_name_manager(manager, expression_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(the_statement);
        return NULL;
      }

    if ((the_assignment_kind != AK_SIMPLE) &&
        (the_assignment_kind != AK_MODULO))
      {
        unbound_use *use;

        use = add_unbound_operator_statement(manager,
                expression_kind_operator_name(
                        binary_expression_kind_for_assignment(
                                the_assignment_kind)), the_statement,
                &operator_location);
        if (use == NULL)
          {
            delete_unbound_name_manager(manager);
            delete_statement(the_statement);
            return NULL;
          }
      }

    return create_open_statement(the_statement, manager);
  }

/*
 *      <increment-statement> :
 *          "++" <basket> ";"
 */
extern open_statement *parse_increment_statement(parser *the_parser)
  {
    source_location start_location;
    verdict the_verdict;
    open_basket *the_open_basket;
    unbound_name_manager *manager;
    basket *the_basket;
    statement *the_statement;
    unbound_use *use;

    assert(the_parser != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_PLUS_PLUS);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    the_open_basket = parse_basket(the_parser);
    if (the_open_basket == NULL)
        return NULL;

    decompose_open_basket(the_open_basket, &manager, &the_basket);

    the_statement = create_increment_statement(the_basket);
    if (the_statement == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_verdict = do_statement_end_semicolon(the_parser, the_statement);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(the_statement);
        return NULL;
      }

    use = add_unbound_operator_statement(manager,
            expression_kind_operator_name(EK_ADD), the_statement,
            &start_location);
    if (use == NULL)
      {
        delete_unbound_name_manager(manager);
        delete_statement(the_statement);
        return NULL;
      }

    return create_open_statement(the_statement, manager);
  }

/*
 *      <decrement-statement> :
 *          "--" <basket> ";"
 */
extern open_statement *parse_decrement_statement(parser *the_parser)
  {
    source_location start_location;
    verdict the_verdict;
    open_basket *the_open_basket;
    unbound_name_manager *manager;
    basket *the_basket;
    statement *the_statement;
    unbound_use *use;

    assert(the_parser != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_MINUS_MINUS);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    the_open_basket = parse_basket(the_parser);
    if (the_open_basket == NULL)
        return NULL;

    decompose_open_basket(the_open_basket, &manager, &the_basket);

    the_statement = create_decrement_statement(the_basket);
    if (the_statement == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_verdict = do_statement_end_semicolon(the_parser, the_statement);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(the_statement);
        return NULL;
      }

    use = add_unbound_operator_statement(manager,
            expression_kind_operator_name(EK_SUBTRACT), the_statement,
            &start_location);
    if (use == NULL)
      {
        delete_unbound_name_manager(manager);
        delete_statement(the_statement);
        return NULL;
      }

    return create_open_statement(the_statement, manager);
  }

/*
 *      <label-statement> :
 *          <identifier> ":"
 */
extern open_statement *parse_label_statement(parser *the_parser)
  {
    source_location start_location;
    boolean is_ageless;
    char *name_copy;
    verdict the_verdict;
    statement *result_statement;
    unbound_name_manager *manager;

    assert(the_parser != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));

    is_ageless = FALSE;

    if (next_is_keyword(the_parser, "ageless"))
      {
        verdict the_verdict;

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return NULL;
        is_ageless = TRUE;
      }

    name_copy = expect_and_eat_identifier_return_name_copy(the_parser);
    if (name_copy == NULL)
        return NULL;

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_COLON);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(name_copy);
        return NULL;
      }

    if (is_ageless)
      {
        location_error(&start_location,
                "Error -- ageless label statements are an optional extension "
                "to the Salmon language that is not supported by this "
                "implementation.");
        free(name_copy);
        return NULL;
      }

    result_statement = create_label_statement(name_copy);
    free(name_copy);
    if (result_statement == NULL)
        return NULL;

    manager = create_unbound_name_manager();
    if (manager == NULL)
      {
        delete_statement(result_statement);
        return NULL;
      }

    return create_open_statement(result_statement, manager);
  }

/*
 *      <statement-block-statement> :
 *          <braced-statement-block> ";"
 */
extern open_statement *parse_statement_block_statement(parser *the_parser)
  {
    open_statement_block *open_block;
    statement_block *block;
    unbound_name_manager *manager;
    statement *result;
    verdict the_verdict;

    assert(the_parser != NULL);

    open_block = parse_braced_statement_block(the_parser);
    if (open_block == NULL)
        return NULL;

    decompose_open_statement_block(open_block, &manager, &block);

    result = create_statement_block_statement(block);
    if (result == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_verdict = do_statement_end_semicolon(the_parser, result);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_unbound_name_manager(manager);
        delete_statement(result);
        return NULL;
      }

    return create_open_statement(result, manager);
  }

/*
 *      <try-catch-statement> :
 *          "try" <statement-block> { <catch-tagged-list> }?
 *                  { "catch" <statement-block> }? ";"
 *
 *      <catch-tagged-list> :
 *          <catch-tagged-item> |
 *          <catch-tagged-item-list> "," <catch-tagged-item>
 *
 *      <catch-tagged-item> :
 *          "catch" "(" <identifier> { "tagged" <type-expression> }? ")"
 *                  <statement-block>
 *
 *      <try-handle-statement> :
 *          "try" <statement-block> "handle" <expression> ";"
 */
extern open_statement *parse_try_statement(parser *the_parser)
  {
    verdict the_verdict;
    open_statement_block *open_block;
    statement_block *try_block;
    unbound_name_manager *manager;
    token *the_token;
    statement *result;

    assert(the_parser != NULL);

    the_verdict = expect_and_eat_keyword(the_parser, "try");
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    open_block = parse_statement_block(the_parser);
    if (open_block == NULL)
        return NULL;

    decompose_open_statement_block(open_block, &manager, &try_block);

    the_token = next_token(the_parser->tokenizer);
    if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
      {
        delete_statement_block(try_block);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    if (next_is_keyword(the_parser, "catch"))
      {
        result = create_try_catch_statement(try_block, NULL);
        if (result == NULL)
          {
            delete_unbound_name_manager(manager);
            return NULL;
          }

        while (TRUE)
          {
            verdict the_verdict;
            source_location exception_declaration_location;
            char *name_copy;
            type_expression *tag_type;
            open_statement_block *open_catch;
            statement_block *catch_block;
            unbound_name_manager *child_manager;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_statement(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            if (next_is(the_parser->tokenizer, TK_LEFT_PAREN))
              {
                verdict the_verdict;

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_statement(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                exception_declaration_location =
                        *(next_token_location(the_parser->tokenizer));

                name_copy =
                        expect_and_eat_identifier_return_name_copy(the_parser);
                if (name_copy == NULL)
                  {
                    delete_statement(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                if (next_is_keyword(the_parser, "tagged"))
                  {
                    verdict the_verdict;
                    open_type_expression *open_type;
                    unbound_name_manager *child_manager;

                    the_verdict = consume_token(the_parser->tokenizer);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        free(name_copy);
                        delete_statement(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    open_type = parse_type_expression(the_parser, TEPP_TOP);
                    if (open_type == NULL)
                      {
                        free(name_copy);
                        delete_statement(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }

                    decompose_open_type_expression(open_type, &child_manager,
                                                   &tag_type);

                    the_verdict = merge_in_unbound_name_manager(manager,
                                                                child_manager);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_type_expression(tag_type);
                        free(name_copy);
                        delete_statement(result);
                        delete_unbound_name_manager(manager);
                        return NULL;
                      }
                  }
                else
                  {
                    tag_type = NULL;
                  }

                the_verdict =
                        expect_and_eat(the_parser->tokenizer, TK_RIGHT_PAREN);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    if (tag_type != NULL)
                        delete_type_expression(tag_type);
                    free(name_copy);
                    delete_statement(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }
              }
            else
              {
                name_copy = NULL;
                tag_type = NULL;
              }

            open_catch = parse_statement_block(the_parser);
            if (open_catch == NULL)
              {
                if (tag_type != NULL)
                    delete_type_expression(tag_type);
                if (name_copy != NULL)
                    free(name_copy);
                delete_statement(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            decompose_open_statement_block(open_catch, &child_manager,
                                           &catch_block);

            if (name_copy != NULL)
              {
                size_t catcher_num;
                verdict the_verdict;

                if (strcmp(try_aliasing(name_copy, the_parser), name_copy) !=
                    0)
                  {
                    location_warning(&exception_declaration_location,
                            "`%s' was used to declare the exception immutable "
                            "of a ``catch'' clause of a try-catch statement, "
                            "but that name is aliased to `%s', so referencing "
                            "this immutable by this name will not work.",
                            name_copy, try_aliasing(name_copy, the_parser));
                  }

                catcher_num = try_catch_statement_tagged_catcher_count(result);

                the_verdict = add_try_catch_statement_tagged_catcher(result,
                        name_copy, tag_type, catch_block,
                        &exception_declaration_location);
                catch_block = NULL;
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_unbound_name_manager(child_manager);
                    free(name_copy);
                    delete_statement(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }

                the_verdict = bind_variable_name(child_manager, name_copy,
                        try_catch_statement_tagged_catcher_exception(result,
                                catcher_num));
                free(name_copy);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_statement(result);
                    delete_unbound_name_manager(manager);
                    return NULL;
                  }
              }

            the_verdict =
                    merge_in_unbound_name_manager(manager, child_manager);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                if (catch_block != NULL)
                    delete_statement_block(catch_block);
                delete_statement(result);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            if (catch_block != NULL)
              {
                add_try_catch_statement_catcher(result, catch_block);
                break;
              }

            if (!(next_is_keyword(the_parser, "catch")))
                break;
          }
      }
    else if (next_is_keyword(the_parser, "handle"))
      {
        verdict the_verdict;
        open_expression *open_handle;
        expression *handle_expression;
        unbound_name_manager *child_manager;

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_statement_block(try_block);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        open_handle = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
        if (open_handle == NULL)
          {
            delete_statement_block(try_block);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        decompose_open_expression(open_handle, &child_manager,
                                  &handle_expression);

        result = create_try_handle_statement(try_block, handle_expression);
        if (result == NULL)
          {
            delete_unbound_name_manager(child_manager);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        the_verdict = merge_in_unbound_name_manager(manager, child_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_statement(result);
            delete_unbound_name_manager(manager);
            return NULL;
          }
      }
    else
      {
        token_error(the_token,
                "Syntax error -- expected keyword `catch' or keyword "
                "`handle'.");
        delete_statement_block(try_block);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    set_statement_end_location(result,
                               next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_SEMICOLON);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_statement(result);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    return create_open_statement(result, manager);
  }

/*
 *      <statement-list> :
 *          <empty> |
 *          <statement> <statement-list>
 */
extern open_statement_block *parse_statement_list(parser *the_parser)
  {
    assert(the_parser != NULL);

    return parse_statement_list_through_function(&parser_statement_parser,
            &parser_done_test, the_parser, the_parser->include_handler,
            the_parser->include_handler_data, the_parser->alias_manager,
            the_parser->native_bridge_dll_body_allowed);
  }

extern open_statement_block *parse_statement_list_through_function(
        open_statement *(*statement_parser)(void *data,
                const char **current_labels, size_t current_label_count),
        boolean (*done)(void *data), void *data,
        include_handler_type include_handler, void *include_handler_data,
        alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed)
  {
    unbound_name_manager *the_unbound_name_manager;
    statement_block *the_statement_block;
    open_statement_block *the_open_statement_block;
    string_aa current_labels;
    verdict the_verdict;

    the_unbound_name_manager = create_unbound_name_manager();
    if (the_unbound_name_manager == NULL)
        return NULL;

    the_statement_block = create_statement_block();
    if (the_statement_block == NULL)
      {
        delete_unbound_name_manager(the_unbound_name_manager);
        return NULL;
      }

    the_open_statement_block = create_open_statement_block(the_statement_block,
            the_unbound_name_manager);
    if (the_open_statement_block == NULL)
        return NULL;

    the_verdict = string_aa_init(&current_labels, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_statement_block(the_open_statement_block);
        return NULL;
      }

    the_verdict = parse_statements_for_statement_block_through_function(
            statement_parser, done, data, the_statement_block,
            the_unbound_name_manager, &current_labels, include_handler,
            include_handler_data, parent_alias_manager,
            native_bridge_dll_body_allowed);
    free(current_labels.array);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_statement_block(the_open_statement_block);
        return NULL;
      }

    the_verdict = resolve_statement_block(the_statement_block,
            the_unbound_name_manager, parent_alias_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_statement_block(the_open_statement_block);
        return NULL;
      }

    return the_open_statement_block;
  }

extern verdict parse_statements_for_statement_block_through_function(
        open_statement *(*statement_parser)(void *data,
                const char **current_labels, size_t current_label_count),
        boolean (*done)(void *data), void *data,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels,
        include_handler_type include_handler, void *include_handler_data,
        alias_manager *parent_alias_manager,
        boolean native_bridge_dll_body_allowed)
  {
    while (TRUE)
      {
        open_statement *the_open_statement;
        statement *the_statement;
        unbound_name_manager *child_manager;
        verdict the_verdict;

        if ((*done)(data))
            return MISSION_ACCOMPLISHED;

        the_open_statement = (*statement_parser)(data, current_labels->array,
                current_labels->element_count);
        if (the_open_statement == NULL)
            return MISSION_FAILED;

        decompose_open_statement(the_open_statement, &child_manager,
                                 &the_statement);

        if (the_statement != NULL)
          {
            if (get_statement_kind(the_statement) == SK_LABEL)
              {
                verdict the_verdict;

                the_verdict = string_aa_append(current_labels,
                        label_statement_name(the_statement));
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_statement(the_statement);
                    delete_unbound_name_manager(child_manager);
                    return the_verdict;
                  }
              }
            else
              {
                current_labels->element_count = 0;
              }

            if (get_statement_kind(the_statement) == SK_DECLARATION)
              {
                size_t declaration_count;
                size_t declaration_num;

                declaration_count =
                        declaration_statement_declaration_count(the_statement);
                for (declaration_num = 0; declaration_num < declaration_count;
                     ++declaration_num)
                  {
                    declaration *the_declaration;
                    size_t count;
                    size_t index;

                    the_declaration = declaration_statement_declaration(
                            the_statement, declaration_num);
                    count = statement_block_name_count(the_statement_block);
                    index = statement_block_lookup_name(the_statement_block,
                            declaration_name(the_declaration));
                    if ((index < count) &&
                        ((statement_block_name_kind(the_statement_block, index)
                          != NK_ROUTINE) ||
                         (declaration_kind(the_declaration) != NK_ROUTINE)))
                      {
                        statement_error(the_statement,
                                "This declaration of %a conflicts with a "
                                "previous declaration.", the_declaration);
                        show_previous_declaration(the_statement_block, index);
                        delete_statement(the_statement);
                        delete_unbound_name_manager(child_manager);
                        return MISSION_FAILED;
                      }
                  }
              }
            else if ((get_statement_kind(the_statement) == SK_USE) &&
                     use_statement_named(the_statement))
              {
                declaration *the_declaration;
                size_t count;
                size_t index;

                the_declaration = variable_declaration_declaration(
                        use_statement_container(the_statement));
                count = statement_block_name_count(the_statement_block);
                index = statement_block_lookup_name(the_statement_block,
                        declaration_name(the_declaration));
                if (index < count)
                  {
                    statement_error(the_statement,
                            "The name `%s' of this use statement conflicts "
                            "with a previous declaration.",
                            declaration_name(the_declaration));
                    show_previous_declaration(the_statement_block, index);
                    delete_statement(the_statement);
                    delete_unbound_name_manager(child_manager);
                    return MISSION_FAILED;
                  }
              }
            else if (get_statement_kind(the_statement) == SK_INCLUDE)
              {
                verdict the_verdict;

                delete_unbound_name_manager(child_manager);

                the_verdict = (*include_handler)(include_handler_data,
                        include_statement_file_name(the_statement),
                        the_statement_block, parent_manager, current_labels,
                        get_statement_location(the_statement),
                        parent_alias_manager, native_bridge_dll_body_allowed);
                delete_statement(the_statement);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return the_verdict;

                continue;
              }

            the_verdict = append_statement_to_block(the_statement_block,
                                                    the_statement);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(child_manager);
                return MISSION_FAILED;
              }
          }

        the_verdict =
                merge_in_unbound_name_manager(parent_manager, child_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return MISSION_FAILED;
      }
  }

/*
 *      <statement-block> :
 *          <non-if-statement-block> |
 *          <if-statement>
 *
 *      <non-if-statement-block> :
 *          <braced-statement-block> |
 *          <non-block-non-if-statement>
 */
extern open_statement_block *parse_statement_block(parser *the_parser)
  {
    one_statement_data one_data;
    open_statement_block *result;
    const source_location *location;

    assert(the_parser != NULL);

    if (next_is(the_parser->tokenizer, TK_LEFT_CURLY_BRACE))
        return parse_braced_statement_block(the_parser);

    one_data.parser = the_parser;
    one_data.have_one = NULL;

    result = parse_statement_list_through_function(
            &parser_single_statement_parser,
            &parser_single_statement_done_test, &one_data,
            the_parser->include_handler, the_parser->include_handler_data,
            the_parser->alias_manager,
            the_parser->native_bridge_dll_body_allowed);
    if (result == NULL)
        return NULL;

    assert(one_data.have_one != NULL);
    location = get_statement_location(one_data.have_one);

    set_statement_block_start_location(
            open_statement_block_statement_block(result), location);
    set_statement_block_end_location(
            open_statement_block_statement_block(result), location);

    return result;
  }

/*
 *      <braced-statement-block> :
 *          "{" <statement-list> "}"
 */
extern open_statement_block *parse_braced_statement_block(parser *the_parser)
  {
    source_location start_location;
    verdict the_verdict;
    alias_manager *old_alias_manager;
    alias_manager *new_alias_manager;
    open_statement_block *result;

    assert(the_parser != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_LEFT_CURLY_BRACE);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    old_alias_manager = the_parser->alias_manager;
    assert(old_alias_manager != NULL);

    new_alias_manager = MALLOC_ONE_OBJECT(alias_manager);
    if (new_alias_manager == NULL)
        return NULL;

    assert(new_alias_manager != NULL);
    new_alias_manager->local = NULL;
    if (old_alias_manager->local != NULL)
        new_alias_manager->next_non_null = old_alias_manager;
    else
        new_alias_manager->next_non_null = old_alias_manager->next_non_null;
    the_parser->alias_manager = new_alias_manager;

    result = parse_statement_list(the_parser);

    assert(new_alias_manager != NULL);
    assert(the_parser->alias_manager == new_alias_manager);
    the_parser->alias_manager = old_alias_manager;
    if (new_alias_manager->local != NULL)
        destroy_string_index(new_alias_manager->local);
    free(new_alias_manager);

    if (result == NULL)
        return NULL;

    set_statement_block_start_location(
            open_statement_block_statement_block(result), &start_location);
    set_statement_block_end_location(
            open_statement_block_statement_block(result),
            next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_RIGHT_CURLY_BRACE);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_statement_block(result);
        return NULL;
      }

    return result;
  }

extern verdict parse_statements_for_statement_block(parser *the_parser,
        statement_block *the_statement_block,
        unbound_name_manager *parent_manager, string_aa *current_labels)
  {
    assert(the_parser != NULL);

    return parse_statements_for_statement_block_through_function(
            &parser_statement_parser, &parser_done_test, the_parser,
            the_statement_block, parent_manager, current_labels,
            the_parser->include_handler, the_parser->include_handler_data,
            the_parser->alias_manager,
            the_parser->native_bridge_dll_body_allowed);
  }

extern declaration *parse_routine_declaration(parser *the_parser,
        const char *opening, boolean return_ok, boolean return_required,
        boolean is_class, boolean is_static, boolean is_ageless,
        boolean is_virtual, boolean is_pure, expression *single_lock,
        unbound_name_manager *single_lock_manager,
        unbound_name_manager **result_manager,
        native_bridge_routine *native_handler, purity_safety the_purity_safety,
        const source_location *start_location)
  {
    char *name_copy;
    verdict the_verdict;
    source_location end_location;
    formal_arguments *the_formal_arguments;
    boolean extra_arguments_allowed;
    unbound_name_manager *static_manager;
    unbound_name_manager *manager;
    type_expression *static_return_type;
    type_expression *dynamic_return_type;
    unbound_name_manager *body_manager;
    statement_block *body;
    routine_declaration *the_routine_declaration;
    declaration *the_declaration;

    if (next_is(the_parser->tokenizer, TK_IDENTIFIER))
      {
        name_copy = expect_and_eat_identifier_return_name_copy(the_parser);
        if (name_copy == NULL)
          {
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }
      }
    else
      {
        name_copy = NULL;
      }

    static_manager = NULL;
    manager = single_lock_manager;

    the_formal_arguments = parse_formal_arguments(the_parser,
            &extra_arguments_allowed, &static_manager, &manager,
            &end_location);
    if (the_formal_arguments == NULL)
      {
        if (name_copy != NULL)
            free(name_copy);
        if (single_lock != NULL)
            delete_expression(single_lock);
        return NULL;
      }

    if (next_is(the_parser->tokenizer, TK_IDENTIFIER))
      {
        token *the_token;
        verdict the_verdict;
        open_type_expression *return_open_type;
        unbound_name_manager *return_type_manager;

        the_token = next_token(the_parser->tokenizer);
        assert(the_token != NULL);

        if (strcmp(aliased_token_name(the_token, the_parser), "returns") != 0)
          {
            token_error(the_token,
                    "Syntax error -- expected keyword `returns' or left curly "
                    "brace.");
            delete_formal_arguments(the_formal_arguments);
            if (name_copy != NULL)
                free(name_copy);
            if (static_manager != NULL)
                delete_unbound_name_manager(static_manager);
            if (manager != NULL)
                delete_unbound_name_manager(manager);
            if (single_lock != NULL)
                delete_expression(single_lock);
            return NULL;
          }

        if (!return_ok)
          {
            token_error(the_token,
                    "Syntax error -- no return type is allowed for a %s.",
                    opening);
            delete_formal_arguments(the_formal_arguments);
            if (name_copy != NULL)
                free(name_copy);
            if (static_manager != NULL)
                delete_unbound_name_manager(static_manager);
            if (manager != NULL)
                delete_unbound_name_manager(manager);
            if (single_lock != NULL)
                delete_expression(single_lock);
            return NULL;
          }

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_formal_arguments(the_formal_arguments);
            if (name_copy != NULL)
                free(name_copy);
            if (static_manager != NULL)
                delete_unbound_name_manager(static_manager);
            if (manager != NULL)
                delete_unbound_name_manager(manager);
            if (single_lock != NULL)
                delete_expression(single_lock);
            return NULL;
          }

        return_open_type = parse_type_expression_with_end_location(the_parser,
                TEPP_TOP, &end_location);
        if (return_open_type == NULL)
          {
            delete_formal_arguments(the_formal_arguments);
            if (name_copy != NULL)
                free(name_copy);
            if (static_manager != NULL)
                delete_unbound_name_manager(static_manager);
            if (manager != NULL)
                delete_unbound_name_manager(manager);
            if (single_lock != NULL)
                delete_expression(single_lock);
            return NULL;
          }

        decompose_open_type_expression(return_open_type, &return_type_manager,
                                       &static_return_type);

        assert(static_return_type != NULL);
        assert(return_type_manager != NULL);

        if (static_manager == NULL)
          {
            static_manager = return_type_manager;
          }
        else
          {
            verdict the_verdict;

            the_verdict = merge_in_unbound_name_manager(static_manager,
                                                        return_type_manager);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                if (static_return_type != NULL)
                    delete_type_expression(static_return_type);
                delete_formal_arguments(the_formal_arguments);
                if (name_copy != NULL)
                    free(name_copy);
                if (static_manager != NULL)
                    delete_unbound_name_manager(static_manager);
                if (manager != NULL)
                    delete_unbound_name_manager(manager);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                return NULL;
              }
          }

        if (next_is(the_parser->tokenizer, TK_DIVIDE))
          {
            verdict the_verdict;
            open_type_expression *dynamic_return_open_type;
            unbound_name_manager *child_manager;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                if (static_return_type != NULL)
                    delete_type_expression(static_return_type);
                delete_formal_arguments(the_formal_arguments);
                if (name_copy != NULL)
                    free(name_copy);
                if (static_manager != NULL)
                    delete_unbound_name_manager(static_manager);
                if (manager != NULL)
                    delete_unbound_name_manager(manager);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                return NULL;
              }

            dynamic_return_open_type = parse_type_expression_with_end_location(
                    the_parser, TEPP_TOP, &end_location);
            if (dynamic_return_open_type == NULL)
              {
                if (static_return_type != NULL)
                    delete_type_expression(static_return_type);
                delete_formal_arguments(the_formal_arguments);
                if (name_copy != NULL)
                    free(name_copy);
                if (static_manager != NULL)
                    delete_unbound_name_manager(static_manager);
                if (manager != NULL)
                    delete_unbound_name_manager(manager);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                return NULL;
              }

            decompose_open_type_expression(dynamic_return_open_type,
                    &child_manager, &dynamic_return_type);

            assert(dynamic_return_type != NULL);
            assert(child_manager != NULL);

            if (manager == NULL)
              {
                manager = child_manager;
              }
            else
              {
                verdict the_verdict;

                the_verdict =
                        merge_in_unbound_name_manager(manager, child_manager);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_type_expression(dynamic_return_type);
                    if (static_return_type != NULL)
                        delete_type_expression(static_return_type);
                    delete_formal_arguments(the_formal_arguments);
                    if (name_copy != NULL)
                        free(name_copy);
                    if (static_manager != NULL)
                        delete_unbound_name_manager(static_manager);
                    if (manager != NULL)
                        delete_unbound_name_manager(manager);
                    if (single_lock != NULL)
                        delete_expression(single_lock);
                    return NULL;
                  }
              }
          }
        else
          {
            dynamic_return_type = NULL;
          }
      }
    else
      {
        if (!return_required)
          {
            static_return_type = NULL;
          }
        else
          {
            type *the_type;

            the_type = get_anything_type();
            if (the_type == NULL)
              {
                delete_formal_arguments(the_formal_arguments);
                if (name_copy != NULL)
                    free(name_copy);
                if (static_manager != NULL)
                    delete_unbound_name_manager(static_manager);
                delete_unbound_name_manager(manager);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                return NULL;
              }

            static_return_type = create_constant_type_expression(the_type);
            if (static_return_type == NULL)
              {
                delete_formal_arguments(the_formal_arguments);
                if (name_copy != NULL)
                    free(name_copy);
                if (static_manager != NULL)
                    delete_unbound_name_manager(static_manager);
                delete_unbound_name_manager(manager);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                return NULL;
              }
          }
        dynamic_return_type = NULL;
      }

    if (native_handler == NULL)
      {
        if (next_is(the_parser->tokenizer, TK_LEFT_PAREN))
          {
            source_location return_start_location;
            verdict the_verdict;
            open_expression *open_return_value;
            expression *return_value;
            statement *return_statement;
            source_location return_end_location;
            unbound_use *use;

            return_start_location =
                    *(next_token_location(the_parser->tokenizer));

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                if (static_return_type != NULL)
                    delete_type_expression(static_return_type);
                if (dynamic_return_type != NULL)
                    delete_type_expression(dynamic_return_type);
                delete_formal_arguments(the_formal_arguments);
                if (name_copy != NULL)
                    free(name_copy);
                if (static_manager != NULL)
                    delete_unbound_name_manager(static_manager);
                if (manager != NULL)
                    delete_unbound_name_manager(manager);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                return NULL;
              }

            open_return_value =
                    parse_expression(the_parser, EPP_TOP, NULL, FALSE);
            if (open_return_value == NULL)
              {
                if (static_return_type != NULL)
                    delete_type_expression(static_return_type);
                if (dynamic_return_type != NULL)
                    delete_type_expression(dynamic_return_type);
                delete_formal_arguments(the_formal_arguments);
                if (name_copy != NULL)
                    free(name_copy);
                if (static_manager != NULL)
                    delete_unbound_name_manager(static_manager);
                if (manager != NULL)
                    delete_unbound_name_manager(manager);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                return NULL;
              }

            return_end_location =
                    *(next_token_location(the_parser->tokenizer));

            the_verdict =
                    expect_and_eat(the_parser->tokenizer, TK_RIGHT_PAREN);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_open_expression(open_return_value);
                if (static_return_type != NULL)
                    delete_type_expression(static_return_type);
                if (dynamic_return_type != NULL)
                    delete_type_expression(dynamic_return_type);
                delete_formal_arguments(the_formal_arguments);
                if (name_copy != NULL)
                    free(name_copy);
                if (static_manager != NULL)
                    delete_unbound_name_manager(static_manager);
                if (manager != NULL)
                    delete_unbound_name_manager(manager);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                return NULL;
              }

            decompose_open_expression(open_return_value, &body_manager,
                                      &return_value);

            return_statement = create_return_statement(return_value);
            if (return_statement == NULL)
              {
                delete_unbound_name_manager(body_manager);
                if (static_return_type != NULL)
                    delete_type_expression(static_return_type);
                if (dynamic_return_type != NULL)
                    delete_type_expression(dynamic_return_type);
                delete_formal_arguments(the_formal_arguments);
                if (name_copy != NULL)
                    free(name_copy);
                if (static_manager != NULL)
                    delete_unbound_name_manager(static_manager);
                if (manager != NULL)
                    delete_unbound_name_manager(manager);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                return NULL;
              }

            set_statement_start_location(return_statement,
                                         &return_start_location);
            set_statement_end_location(return_statement, &return_end_location);
            end_location = return_end_location;

            use = add_unbound_return(body_manager, NULL, return_statement,
                                     &return_start_location);
            if (use == NULL)
              {
                delete_statement(return_statement);
                delete_unbound_name_manager(body_manager);
                if (static_return_type != NULL)
                    delete_type_expression(static_return_type);
                if (dynamic_return_type != NULL)
                    delete_type_expression(dynamic_return_type);
                delete_formal_arguments(the_formal_arguments);
                if (name_copy != NULL)
                    free(name_copy);
                if (static_manager != NULL)
                    delete_unbound_name_manager(static_manager);
                if (manager != NULL)
                    delete_unbound_name_manager(manager);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                return NULL;
              }

            body = create_statement_block();
            if (body == NULL)
              {
                delete_statement(return_statement);
                delete_unbound_name_manager(body_manager);
                if (static_return_type != NULL)
                    delete_type_expression(static_return_type);
                if (dynamic_return_type != NULL)
                    delete_type_expression(dynamic_return_type);
                delete_formal_arguments(the_formal_arguments);
                if (name_copy != NULL)
                    free(name_copy);
                if (static_manager != NULL)
                    delete_unbound_name_manager(static_manager);
                if (manager != NULL)
                    delete_unbound_name_manager(manager);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                return NULL;
              }

            the_verdict = append_statement_to_block(body, return_statement);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_statement_block(body);
                delete_unbound_name_manager(body_manager);
                if (static_return_type != NULL)
                    delete_type_expression(static_return_type);
                if (dynamic_return_type != NULL)
                    delete_type_expression(dynamic_return_type);
                delete_formal_arguments(the_formal_arguments);
                if (name_copy != NULL)
                    free(name_copy);
                if (static_manager != NULL)
                    delete_unbound_name_manager(static_manager);
                if (manager != NULL)
                    delete_unbound_name_manager(manager);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                return NULL;
              }
          }
        else if (next_is(the_parser->tokenizer, TK_ASSIGN))
          {
            verdict the_verdict;
            source_location identifier_location;
            char *format_name_copy;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
              error_return:
                if (static_return_type != NULL)
                    delete_type_expression(static_return_type);
                if (dynamic_return_type != NULL)
                    delete_type_expression(dynamic_return_type);
                delete_formal_arguments(the_formal_arguments);
                if (name_copy != NULL)
                    free(name_copy);
                if (static_manager != NULL)
                    delete_unbound_name_manager(static_manager);
                if (manager != NULL)
                    delete_unbound_name_manager(manager);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                return NULL;
              }

            identifier_location =
                    *(next_token_location(the_parser->tokenizer));
            format_name_copy =
                    expect_and_eat_identifier_return_name_copy(the_parser);
            if (format_name_copy == NULL)
                goto error_return;

            if (strcmp(format_name_copy, "null") == 0)
              {
                free(format_name_copy);

                if (!is_virtual)
                  {
                    location_error(&identifier_location,
                            "A null body is only allowed on virtual "
                            "routines.");
                    goto error_return;
                  }

                if (is_static)
                  {
                    location_error(&identifier_location,
                            "A null body is not allowed on static routines.");
                    goto error_return;
                  }

                goto null_body;
              }
            else if (strcmp(format_name_copy, "nb_dll") == 0)
              {
                char *dll_name;
                verdict the_verdict;
                char *symbol_name;
                dynamic_library_handle *dll_handle;
                boolean file_not_found;
                char *error_message;

                free(format_name_copy);

                if (!(the_parser->native_bridge_dll_body_allowed))
                  {
                    location_error(&identifier_location,
                            "An attempt was made to use an `nb_dll' routine "
                            "body when such routine bodies were disallowed.");
                    goto error_return;
                  }

                dll_name = expect_and_eat_string_literal_return_name_copy(
                        the_parser);
                if (dll_name == NULL)
                    goto error_return;

                the_verdict = expect_and_eat(the_parser->tokenizer, TK_COLON);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    free(dll_name);
                    goto error_return;
                  }

                symbol_name = expect_and_eat_string_literal_return_name_copy(
                        the_parser);
                if (symbol_name == NULL)
                  {
                    free(dll_name);
                    goto error_return;
                  }

                if (next_is_keyword(the_parser, "pure_safe"))
                  {
                    verdict the_verdict;

                    the_verdict = consume_token(the_parser->tokenizer);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        free(dll_name);
                        free(symbol_name);
                        goto error_return;
                      }

                    the_purity_safety = PURE_SAFE;
                  }
                else
                  {
                    the_purity_safety = PURE_UNSAFE;
                  }

                dll_handle = open_dynamic_library_in_path(
                        the_parser->include_handler_data, dll_name,
                        &file_not_found, &error_message);

                if ((dll_handle == NULL) && file_not_found)
                  {
                    char **alternates;

                    alternates = alternate_dynamic_library_names(dll_name);
                    if (alternates != NULL)
                      {
                        size_t alt_num;

                        for (alt_num = 0; alternates[alt_num] != NULL;
                             ++alt_num)
                          {
                            dll_handle = open_dynamic_library_in_path(
                                    the_parser->include_handler_data,
                                    alternates[alt_num], &file_not_found,
                                    &error_message);
                            if ((dll_handle != NULL) || !file_not_found)
                                break;
                          }

                        for (alt_num = 0; alternates[alt_num] != NULL;
                             ++alt_num)
                          {
                            free(alternates[alt_num]);
                          }
                        free(alternates);
                      }
                  }

                if (dll_handle == NULL)
                  {
                    if (file_not_found)
                      {
                        location_error(&identifier_location,
                                "Unable to load dll `%s' for an `nb_dll' "
                                "routine body: file not found.", dll_name);
                      }
                    else if (error_message != NULL)
                      {
                        location_error(&identifier_location,
                                "Unable to load dll `%s' for an `nb_dll' "
                                "routine body: %s.", dll_name, error_message);
                        free(error_message);
                      }
                    else
                      {
                        location_error(&identifier_location,
                                "Unable to load dll `%s' for an `nb_dll' "
                                "routine body.", dll_name);
                      }
                    free(dll_name);
                    free(symbol_name);
                    goto error_return;
                  }

                native_handler = (native_bridge_routine *)(
                        find_dynamic_library_symbol(dll_handle, symbol_name));
                if (native_handler == NULL)
                  {
                    location_error(&identifier_location,
                            "Unable to find symbol `%s' in dll `%s' for an "
                            "`nb_dll' routine body.", symbol_name, dll_name);
                    free(dll_name);
                    free(symbol_name);
                    goto error_return;
                  }

                free(dll_name);
                free(symbol_name);

                body_manager = create_unbound_name_manager();
                if (body_manager == NULL)
                    goto error_return;

                body = NULL;
              }
            else
              {
                location_error(&identifier_location,
                        "Implementation-specific routine body format `%s' is "
                        "not supported by this implementation.",
                        format_name_copy);
                free(format_name_copy);
                goto error_return;
              }
          }
        else
          {
            open_statement_block *open_body;

            open_body = parse_braced_statement_block(the_parser);
            if (open_body == NULL)
              {
                if (static_return_type != NULL)
                    delete_type_expression(static_return_type);
                if (dynamic_return_type != NULL)
                    delete_type_expression(dynamic_return_type);
                delete_formal_arguments(the_formal_arguments);
                if (name_copy != NULL)
                    free(name_copy);
                if (static_manager != NULL)
                    delete_unbound_name_manager(static_manager);
                if (manager != NULL)
                    delete_unbound_name_manager(manager);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                return NULL;
              }

            decompose_open_statement_block(open_body, &body_manager, &body);

            end_location = *get_statement_block_location(body);
          }
      }
    else
      {
      null_body:
        body = NULL;
        body_manager = create_unbound_name_manager();
        if (body_manager == NULL)
          {
            if (static_return_type != NULL)
                delete_type_expression(static_return_type);
            if (dynamic_return_type != NULL)
                delete_type_expression(dynamic_return_type);
            delete_formal_arguments(the_formal_arguments);
            if (name_copy != NULL)
                free(name_copy);
            if (static_manager != NULL)
                delete_unbound_name_manager(static_manager);
            if (manager != NULL)
                delete_unbound_name_manager(manager);
            if (single_lock != NULL)
                delete_expression(single_lock);
            return NULL;
          }
      }

    the_routine_declaration = create_routine_declaration(static_return_type,
            dynamic_return_type, the_formal_arguments, extra_arguments_allowed,
            body, native_handler, the_purity_safety, is_pure, is_class,
            single_lock, unbound_name_manager_static_count(body_manager),
            unbound_name_manager_static_declarations(body_manager));
    if (the_routine_declaration == NULL)
      {
        delete_unbound_name_manager(body_manager);
        if (name_copy != NULL)
            free(name_copy);
        if (static_manager != NULL)
            delete_unbound_name_manager(static_manager);
        if (manager != NULL);
            delete_unbound_name_manager(manager);
        return NULL;
      }

    the_declaration = create_declaration_for_routine(name_copy, is_static,
            is_virtual, (name_copy != NULL) && !is_ageless,
            the_routine_declaration, start_location);
    if (name_copy != NULL)
        free(name_copy);
    if (the_declaration == NULL)
      {
        delete_unbound_name_manager(body_manager);
        if (static_manager != NULL)
            delete_unbound_name_manager(static_manager);
        if (manager != NULL);
            delete_unbound_name_manager(manager);
        return NULL;
      }

    set_declaration_end_location(the_declaration, &end_location);

    the_verdict = bind_return(body_manager, the_routine_declaration);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        declaration_remove_reference(the_declaration);
        delete_unbound_name_manager(body_manager);
        if (static_manager != NULL)
            delete_unbound_name_manager(static_manager);
        if (manager != NULL);
            delete_unbound_name_manager(manager);
        return NULL;
      }

    if (manager == NULL)
      {
        manager = body_manager;
      }
    else
      {
        verdict the_verdict;

        the_verdict = merge_in_unbound_name_manager(manager, body_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            if (static_manager != NULL)
                delete_unbound_name_manager(static_manager);
            delete_unbound_name_manager(manager);
            declaration_remove_reference(the_declaration);
            return NULL;
          }
      }

    assert(manager != NULL);

    the_verdict = bind_formals(the_formal_arguments, manager,
                               the_parser->alias_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (static_manager != NULL)
            delete_unbound_name_manager(static_manager);
        delete_unbound_name_manager(manager);
        declaration_remove_reference(the_declaration);
        return NULL;
      }

    the_verdict = unbound_name_manager_clear_static_list(manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (static_manager != NULL)
            delete_unbound_name_manager(static_manager);
        delete_unbound_name_manager(manager);
        declaration_remove_reference(the_declaration);
        return NULL;
      }

    if (static_manager != NULL)
      {
        verdict the_verdict;

        the_verdict = merge_in_unbound_name_manager(manager, static_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            declaration_remove_reference(the_declaration);
            delete_unbound_name_manager(manager);
            return NULL;
          }
      }

    *result_manager = manager;
    return the_declaration;
  }

extern declaration *parse_variable_declaration(parser *the_parser,
        boolean immutable, boolean is_static, boolean is_ageless,
        boolean is_virtual, expression *single_lock,
        unbound_name_manager *single_lock_manager,
        unbound_name_manager **result_manager, type_expression **dynamic_type,
        unbound_name_manager **dynamic_type_manager)
  {
    source_location start_location;
    source_location end_location;
    verdict the_verdict;
    char *name_copy;
    type_expression *the_type_expression;
    expression *the_expression;
    boolean initialization_is_forced;
    variable_declaration *the_variable_declaration;
    declaration *the_declaration;

    assert(the_parser != NULL);
    assert((single_lock == NULL) == (single_lock_manager == NULL));
    assert(result_manager != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));
    end_location = start_location;

    *result_manager = single_lock_manager;

    the_verdict = parse_data_declaration_tail(the_parser, result_manager,
            &name_copy, &the_type_expression, &the_expression,
            &initialization_is_forced, dynamic_type, dynamic_type_manager,
            &end_location);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (single_lock != NULL)
            delete_expression(single_lock);
        return NULL;
      }

    the_variable_declaration = create_variable_declaration(the_type_expression,
            the_expression, initialization_is_forced, immutable, single_lock);
    if (the_variable_declaration == NULL)
      {
        if (name_copy != NULL)
            free(name_copy);
        delete_unbound_name_manager(*result_manager);
        return NULL;
      }

    the_declaration = create_declaration_for_variable(name_copy, is_static,
            is_virtual, (name_copy != NULL) && !is_ageless,
            the_variable_declaration, &start_location);
    if (name_copy != NULL)
        free(name_copy);
    if (the_declaration == NULL)
      {
        delete_unbound_name_manager(*result_manager);
        return NULL;
      }

    set_declaration_end_location(the_declaration, &end_location);

    return the_declaration;
  }

extern declaration *parse_tagalong_declaration(parser *the_parser,
        boolean is_static, boolean is_ageless, boolean is_virtual,
        expression *single_lock, unbound_name_manager *single_lock_manager,
        unbound_name_manager **result_manager)
  {
    source_location start_location;
    source_location end_location;
    unbound_name_manager *manager;
    verdict the_verdict;
    char *name_copy;
    type_expression *the_type_expression;
    expression *the_expression;
    boolean initialization_is_forced;
    boolean is_object;
    type_expression *on_expression;
    tagalong_declaration *the_tagalong_declaration;
    declaration *the_declaration;

    assert(the_parser != NULL);
    assert((single_lock == NULL) == (single_lock_manager == NULL));
    assert(result_manager != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));
    end_location = start_location;

    manager = single_lock_manager;

    the_verdict = parse_data_declaration_tail(the_parser, &manager, &name_copy,
            &the_type_expression, &the_expression, &initialization_is_forced,
            NULL, NULL, &end_location);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (single_lock != NULL)
            delete_expression(single_lock);
        return NULL;
      }

    assert(manager != NULL);

    if (next_is_keyword(the_parser, "on"))
      {
        verdict the_verdict;
        open_type_expression *open_on;
        unbound_name_manager *child_manager;

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            if (name_copy != NULL)
                free(name_copy);
            delete_type_expression(the_type_expression);
            if (the_expression != NULL)
                delete_expression(the_expression);
            if (single_lock != NULL)
                delete_expression(single_lock);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        if (next_is_keyword(the_parser, "object"))
          {
            verdict the_verdict;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                if (name_copy != NULL)
                    free(name_copy);
                delete_type_expression(the_type_expression);
                if (the_expression != NULL)
                    delete_expression(the_expression);
                if (single_lock != NULL)
                    delete_expression(single_lock);
                delete_unbound_name_manager(manager);
                return NULL;
              }

            is_object = TRUE;
          }
        else
          {
            is_object = FALSE;
          }

        open_on = parse_type_expression_with_end_location(the_parser, TEPP_TOP,
                                                          &end_location);
        if (open_on == NULL)
          {
            if (name_copy != NULL)
                free(name_copy);
            delete_type_expression(the_type_expression);
            if (the_expression != NULL)
                delete_expression(the_expression);
            if (single_lock != NULL)
                delete_expression(single_lock);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        decompose_open_type_expression(open_on, &child_manager,
                                       &on_expression);

        the_verdict = merge_in_unbound_name_manager(manager, child_manager);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            if (name_copy != NULL)
                free(name_copy);
            delete_type_expression(the_type_expression);
            if (the_expression != NULL)
                delete_expression(the_expression);
            if (single_lock != NULL)
                delete_expression(single_lock);
            delete_unbound_name_manager(manager);
            delete_type_expression(on_expression);
            return NULL;
          }
      }
    else
      {
        type *anything_type;

        is_object = FALSE;

        anything_type = get_anything_type();
        if (anything_type == NULL)
          {
            if (name_copy != NULL)
                free(name_copy);
            delete_type_expression(the_type_expression);
            if (the_expression != NULL)
                delete_expression(the_expression);
            if (single_lock != NULL)
                delete_expression(single_lock);
            delete_unbound_name_manager(manager);
            return NULL;
          }

        on_expression = create_constant_type_expression(anything_type);
        if (on_expression == NULL)
          {
            if (name_copy != NULL)
                free(name_copy);
            delete_type_expression(the_type_expression);
            if (the_expression != NULL)
                delete_expression(the_expression);
            if (single_lock != NULL)
                delete_expression(single_lock);
            delete_unbound_name_manager(manager);
            return NULL;
          }
      }

    the_tagalong_declaration = create_tagalong_declaration(the_type_expression,
            the_expression, initialization_is_forced, single_lock,
            on_expression, is_object);
    if (the_tagalong_declaration == NULL)
      {
        if (name_copy != NULL)
            free(name_copy);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    the_declaration = create_declaration_for_tagalong(name_copy, is_static,
            is_virtual, (name_copy != NULL) && !is_ageless,
            the_tagalong_declaration, &start_location);
    if (name_copy != NULL)
        free(name_copy);
    if (the_declaration == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    *result_manager = manager;

    set_declaration_end_location(the_declaration, &end_location);

    return the_declaration;
  }

extern declaration *parse_lepton_key_declaration(parser *the_parser,
        boolean is_static, boolean is_ageless, boolean is_virtual,
        unbound_name_manager **result_manager)
  {
    source_location start_location;
    source_location end_location;
    char *name_copy;
    lepton_key_declaration *the_lepton_key_declaration;
    declaration *the_declaration;
    verdict the_verdict;

    assert(the_parser != NULL);
    assert(result_manager != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));
    end_location = start_location;

    if (next_is(the_parser->tokenizer, TK_IDENTIFIER))
      {
        name_copy = expect_and_eat_identifier_return_name_copy(the_parser);
        if (name_copy == NULL)
            return NULL;
      }
    else
      {
        name_copy = NULL;
      }

    *result_manager = create_unbound_name_manager();
    if (*result_manager == NULL)
      {
        if (name_copy != NULL)
            free(name_copy);
        return NULL;
      }

    the_lepton_key_declaration = create_lepton_key_declaration(TRUE);
    if (the_lepton_key_declaration == NULL)
      {
        if (name_copy != NULL)
            free(name_copy);
        delete_unbound_name_manager(*result_manager);
        return NULL;
      }

    the_declaration = create_declaration_for_lepton_key(name_copy, is_static,
            is_virtual, (name_copy != NULL) && !is_ageless,
            the_lepton_key_declaration, &start_location);
    if (name_copy != NULL)
        free(name_copy);
    if (the_declaration == NULL)
      {
        delete_unbound_name_manager(*result_manager);
        return NULL;
      }

    if (!(next_is(the_parser->tokenizer, TK_LEFT_BRACKET)))
      {
        set_declaration_end_location(the_declaration, &end_location);
        return the_declaration;
      }

    the_verdict = consume_token(the_parser->tokenizer);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        declaration_remove_reference(the_declaration);
        delete_unbound_name_manager(*result_manager);
        return NULL;
      }

    if (next_is(the_parser->tokenizer, TK_RIGHT_BRACKET))
      {
        verdict the_verdict;

        set_declaration_end_location(the_declaration,
                next_token_location(the_parser->tokenizer));

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            declaration_remove_reference(the_declaration);
            delete_unbound_name_manager(*result_manager);
            return NULL;
          }

        the_verdict = lepton_key_declaration_set_additional_fields_allowed(
                the_lepton_key_declaration, FALSE);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            declaration_remove_reference(the_declaration);
            delete_unbound_name_manager(*result_manager);
            return NULL;
          }

        return the_declaration;
      }

    while (TRUE)
      {
        token *the_token;
        char *field_name;
        type_expression *field_type;
        verdict the_verdict;

        the_token = next_token(the_parser->tokenizer);
        if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
          {
            declaration_remove_reference(the_declaration);
            delete_unbound_name_manager(*result_manager);
            return NULL;
          }

        if (next_is(the_parser->tokenizer, TK_DOT_DOT_DOT))
          {
            verdict the_verdict;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                declaration_remove_reference(the_declaration);
                delete_unbound_name_manager(*result_manager);
                return NULL;
              }

            set_declaration_end_location(the_declaration,
                    next_token_location(the_parser->tokenizer));

            the_verdict =
                    expect_and_eat(the_parser->tokenizer, TK_RIGHT_BRACKET);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                declaration_remove_reference(the_declaration);
                delete_unbound_name_manager(*result_manager);
                return NULL;
              }

            the_verdict = lepton_key_declaration_set_additional_fields_allowed(
                    the_lepton_key_declaration, TRUE);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                declaration_remove_reference(the_declaration);
                delete_unbound_name_manager(*result_manager);
                return NULL;
              }

            return the_declaration;
          }

        if (get_token_kind(the_token) != TK_IDENTIFIER)
          {
            location_error(next_token_location(the_parser->tokenizer),
                    "Syntax error -- expected %s or identifier, but found %s.",
                    name_for_token_kind(TK_DOT_DOT_DOT),
                    name_for_token_kind(get_token_kind(the_token)));
            declaration_remove_reference(the_declaration);
            delete_unbound_name_manager(*result_manager);
            return NULL;
          }

        field_name = expect_and_eat_identifier_return_name_copy(the_parser);
        if (field_name == NULL)
          {
            declaration_remove_reference(the_declaration);
            delete_unbound_name_manager(*result_manager);
            return NULL;
          }

        if (next_is(the_parser->tokenizer, TK_COLON))
          {
            verdict the_verdict;
            open_type_expression *open_type;
            unbound_name_manager *child_manager;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                free(field_name);
                declaration_remove_reference(the_declaration);
                delete_unbound_name_manager(*result_manager);
                return NULL;
              }

            open_type = parse_type_expression(the_parser, TEPP_TOP);
            if (open_type == NULL)
              {
                free(field_name);
                declaration_remove_reference(the_declaration);
                delete_unbound_name_manager(*result_manager);
                return NULL;
              }

            decompose_open_type_expression(open_type, &child_manager,
                                           &field_type);
            the_verdict = merge_in_unbound_name_manager(*result_manager,
                                                        child_manager);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_type_expression(field_type);
                free(field_name);
                declaration_remove_reference(the_declaration);
                delete_unbound_name_manager(*result_manager);
                return NULL;
              }
          }
        else
          {
            type *anything_type;

            anything_type = get_anything_type();
            if (anything_type == NULL)
              {
                free(field_name);
                declaration_remove_reference(the_declaration);
                delete_unbound_name_manager(*result_manager);
                return NULL;
              }

            field_type = create_constant_type_expression(anything_type);
            if (field_type == NULL)
              {
                free(field_name);
                declaration_remove_reference(the_declaration);
                delete_unbound_name_manager(*result_manager);
                return NULL;
              }
          }

        assert(field_type != NULL);

        the_verdict = lepton_key_add_field(the_lepton_key_declaration,
                                           field_name, field_type);
        free(field_name);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            declaration_remove_reference(the_declaration);
            delete_unbound_name_manager(*result_manager);
            return NULL;
          }

        if (next_is(the_parser->tokenizer, TK_RIGHT_BRACKET))
          {
            verdict the_verdict;

            set_declaration_end_location(the_declaration,
                    next_token_location(the_parser->tokenizer));

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                declaration_remove_reference(the_declaration);
                delete_unbound_name_manager(*result_manager);
                return NULL;
              }

            the_verdict = lepton_key_declaration_set_additional_fields_allowed(
                    the_lepton_key_declaration, FALSE);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                declaration_remove_reference(the_declaration);
                delete_unbound_name_manager(*result_manager);
                return NULL;
              }

            return the_declaration;
          }

        the_token = next_token(the_parser->tokenizer);
        if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
          {
            declaration_remove_reference(the_declaration);
            delete_unbound_name_manager(*result_manager);
            return NULL;
          }

        if (get_token_kind(the_token) != TK_COMMA)
          {
            location_error(next_token_location(the_parser->tokenizer),
                    "Syntax error -- expected right square bracket or a comma,"
                    " but found %s.",
                    name_for_token_kind(get_token_kind(the_token)));
            declaration_remove_reference(the_declaration);
            delete_unbound_name_manager(*result_manager);
            return NULL;
          }

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            declaration_remove_reference(the_declaration);
            delete_unbound_name_manager(*result_manager);
            return NULL;
          }
      }
  }

extern declaration *parse_quark_declaration(parser *the_parser,
        boolean is_static, boolean is_ageless, boolean is_virtual,
        unbound_name_manager **result_manager)
  {
    source_location start_location;
    source_location end_location;
    char *name_copy;
    quark_declaration *the_quark_declaration;
    declaration *the_declaration;

    assert(the_parser != NULL);
    assert(result_manager != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));
    end_location = start_location;

    if (next_is(the_parser->tokenizer, TK_IDENTIFIER))
      {
        end_location = *(next_token_location(the_parser->tokenizer));
        name_copy = expect_and_eat_identifier_return_name_copy(the_parser);
        if (name_copy == NULL)
            return NULL;
      }
    else
      {
        name_copy = NULL;
      }

    *result_manager = create_unbound_name_manager();
    if (*result_manager == NULL)
      {
        if (name_copy != NULL)
            free(name_copy);
        return NULL;
      }

    the_quark_declaration = create_quark_declaration();
    if (the_quark_declaration == NULL)
      {
        if (name_copy != NULL)
            free(name_copy);
        delete_unbound_name_manager(*result_manager);
        return NULL;
      }

    the_declaration = create_declaration_for_quark(name_copy, is_static,
            is_virtual, (name_copy != NULL) && !is_ageless,
            the_quark_declaration, &start_location);
    if (name_copy != NULL)
        free(name_copy);
    if (the_declaration == NULL)
      {
        delete_unbound_name_manager(*result_manager);
        return NULL;
      }

    set_declaration_end_location(the_declaration, &end_location);

    return the_declaration;
  }

extern declaration *parse_lock_declaration(parser *the_parser,
        boolean is_static, boolean is_ageless, boolean is_virtual,
        expression *single_lock, unbound_name_manager *single_lock_manager,
        unbound_name_manager **result_manager)
  {
    source_location start_location;
    source_location end_location;
    char *name_copy;
    lock_declaration *the_lock_declaration;
    declaration *the_declaration;

    assert(the_parser != NULL);
    assert((single_lock == NULL) == (single_lock_manager == NULL));
    assert(result_manager != NULL);

    start_location = *(next_token_location(the_parser->tokenizer));
    end_location = start_location;

    if (next_is(the_parser->tokenizer, TK_IDENTIFIER))
      {
        end_location = *(next_token_location(the_parser->tokenizer));
        name_copy = expect_and_eat_identifier_return_name_copy(the_parser);
        if (name_copy == NULL)
          {
            if (single_lock != NULL)
                delete_expression(single_lock);
            if (single_lock_manager != NULL)
                delete_unbound_name_manager(single_lock_manager);
            return NULL;
          }
      }
    else
      {
        name_copy = NULL;
      }

    if (single_lock_manager != NULL)
      {
        *result_manager = single_lock_manager;
      }
    else
      {
        *result_manager = create_unbound_name_manager();
        if (*result_manager == NULL)
          {
            if (name_copy != NULL)
                free(name_copy);
            if (single_lock != NULL)
                delete_expression(single_lock);
            return NULL;
          }
      }

    the_lock_declaration = create_lock_declaration(single_lock);
    if (the_lock_declaration == NULL)
      {
        if (name_copy != NULL)
            free(name_copy);
        delete_unbound_name_manager(*result_manager);
        return NULL;
      }

    the_declaration = create_declaration_for_lock(name_copy, is_static,
            is_virtual, (name_copy != NULL) && !is_ageless,
            the_lock_declaration, &start_location);
    if (name_copy != NULL)
        free(name_copy);
    if (the_declaration == NULL)
      {
        delete_unbound_name_manager(*result_manager);
        return NULL;
      }

    set_declaration_end_location(the_declaration, &end_location);

    return the_declaration;
  }

/*
 *      <named-data-declaration> :
 *          <data-prefix> <data-keyword> <data-declaration-item-list>
 *
 *      <data-declaration-item-list> :
 *          <data-declaration-item> |
 *          <data-declaration-item> "," <data-declaration-item-list>
 *
 *      <data-declaration-item> :
 *          <identifier> <anonymous-data-declaration>
 *
 *      <unnamed-data-declaration> :
 *          <data-prefix> <data-keyword> <anonymous-data-declaration>
 *
 *      <data-prefix> :
 *          "static" <data-prefix> |
 *          "ageless" <data-prefix> |
 *          "virtual" <data-prefix> |
 *          <single-prefix> <data-prefix> |
 *          <empty>
 *
 *      <data-keyword> :
 *          "variable" |
 *          "immutable"
 *
 *      <anonymous-data-declaration> :
 *          { ":" <type-expression> }? { { ":=" | "::=" } <expression> }?
 *
 *      <named-routine-declaration> :
 *          <routine-prefix> <routine-keyword>
 *                  <routine-declaration-item-list>
 *
 *      <routine-declaration-item-list> :
 *          <routine-declaration-item> |
 *          <routine-declaration-item> "," <routine-declaration-item-list>
 *
 *      <routine-declaration-item> :
 *          <identifier> <anonymous-routine-declaration>
 *
 *      <unnamed-routine-declaration> :
 *          <routine-prefix> <routine-keyword>
 *                  <anonymous-routine-declaration>
 *
 *      <routine-prefix> :
 *          "static" <routine-prefix> |
 *          "ageless" <routine-prefix> |
 *          "virtual" <routine-prefix> |
 *          <single-prefix> <routine-prefix> |
 *          "pure" <routine-prefix> |
 *          <empty>
 *
 *      <routine-keyword> :
 *          "routine" |
 *          "function" |
 *          "procedure" |
 *          "class"
 *
 *      <anonymous-routine-declaration> :
 *          "(" <formal-argument-list> ")"
 *                  { "returns" <type-expression>
 *                    { "/" <type-expression> }? }? <routine-body>
 *
 *      <routine-body> :
 *          <braced-statement-block> |
 *          "(" <expression> ")" |
 *          ":=" <alternate-body>
 *
 *      <alternate-body> :
 *          "null" | <implementation-specific-alternate-body>
 *
 *      <implementation-specific-alternate-body>:
 *          "nb_dll" <string-literal-token> ":" <string-literal-token>
 *                  { "pure_safe" }?
 *
 *      <formal-argument-list> :
 *          <empty> |
 *          <non-empty-formal-argument-list>
 *
 *      <non-empty-formal-argument-list> :
 *          "..." |
 *          <formal-argument> |
 *          <formal-argument> "," <non-empty-formal-argument-list>
 *
 *      <formal-argument> :
 *          <data-prefix> <identifier>
 *                  { ":" <type-expression> { "/" <type-expression> }? }?
 *                  { { ":=" | "::=" } <expression> }?
 *
 *      <named-tagalong-declaration> :
 *          <data-prefix> "tagalong" <tagalong-declaration-item-list>
 *
 *      <tagalong-declaration-item-list> :
 *          <tagalong-declaration-item> |
 *          <tagalong-declaration-item> "," <tagalong-declaration-item-list>
 *
 *      <tagalong-declaration-item> :
 *          <identifier> <anonymous-data-declaration>
 *                  { "on" { "object" }? <type-expression> }?
 *
 *      <unnamed-tagalong-declaration> :
 *          <data-prefix> "tagalong" <anonymous-data-declaration>
 *                  { "on" { "object" }? <type-expression> }?
 *
 *      <named-lepton-declaration> :
 *          <lepton-prefix> "lepton" <lepton-declaration-item-list>
 *
 *      <lepton-declaration-item-list> :
 *          <lepton-declaration-item> |
 *          <lepton-declaration-item> "," <lepton-declaration-item-list>
 *
 *      <lepton-declaration-item> :
 *          <identifier> { "[" <lepton-field-list> "]" }?
 *
 *      <unnamed-lepton-declaration> :
 *          <data-prefix> "lepton" { "[" <lepton-field-list> "]" }?
 *
 *      <lepton-field-list> :
 *          <empty> |
 *          <non-empty-lepton-field-list>
 *
 *      <non-empty-lepton-field-list>
 *          "..." |
 *          <lepton-field-item> |
 *          <lepton-field-item> "," <non-empty-lepton-field-list>
 *
 *      <lepton-field-item> :
 *          <identifier> { ":" <type-expression> }?
 *
 *      <named-quark-declaration> :
 *          <quark-prefix> "quark" <quark-declaration-item-list>
 *
 *      <quark-declaration-item-list> :
 *          <quark-declaration-item> |
 *          <quark-declaration-item> "," <quark-declaration-item-list>
 *
 *      <quark-declaration-item> :
 *          <identifier>
 *
 *      <unnamed-quark-declaration> :
 *          <quark-prefix> "quark"
 *
 *      <quark-prefix> :
 *          "static" <quark-prefix> |
 *          "ageless" <quark-prefix> |
 *          "virtual" <quark-prefix> |
 *          <empty>
 *
 *      <named-lock-declaration> :
 *          <data-prefix> "lock" <lock-declaration-item-list>
 *
 *      <lock-declaration-item-list> :
 *          <lock-declaration-item> |
 *          <lock-declaration-item> "," <lock-declaration-item-list>
 *
 *      <lock-declaration-item> :
 *          <identifier>
 *
 *      <unnamed-lock-declaration> :
 *          <data-prefix> "lock"
 */
extern declaration *parse_declaration(parser *the_parser, boolean is_static,
        boolean is_ageless, boolean is_virtual, boolean is_pure,
        expression *single_lock, unbound_name_manager *single_lock_manager,
        unbound_name_manager **result_manager, boolean is_formal,
        boolean is_dynamic, native_bridge_routine *native_handler,
        purity_safety the_purity_safety,
        verdict (*add_declaration)(void *data, declaration *new_declaration),
        void *add_declaration_data, type_expression **dynamic_type,
        unbound_name_manager **dynamic_type_manager)
  {
    typedef enum
      {
        KK_STATIC,
        KK_AGELESS,
        KK_VIRTUAL,
        KK_PURE,
        KK_SINGLE,
        KK_ROUTINE,
        KK_VARIABLE,
        KK_TAGALONG,
        KK_LEPTON,
        KK_QUARK,
        KK_LOCK
      } keyword_kind;

    source_location start_location;
    token *the_token;
    const char *id_chars;
    keyword_kind which_keyword;
    char *opening;
    boolean return_ok;
    boolean return_required;
    boolean is_class;
    boolean is_immutable;
    verdict the_verdict;
    unbound_name_manager *parent_manager;

    assert(the_parser != NULL);
    assert((single_lock == NULL) == (single_lock_manager == NULL));
    assert(result_manager != NULL);

    if (dynamic_type != NULL)
        *dynamic_type = NULL;
    if (dynamic_type_manager != NULL)
        *dynamic_type_manager = NULL;

    start_location = *(next_token_location(the_parser->tokenizer));

    the_token = next_token(the_parser->tokenizer);
    if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR))
      {
        if (single_lock != NULL)
          {
            delete_expression(single_lock);
            delete_unbound_name_manager(single_lock_manager);
          }
        return NULL;
      }

    if (get_token_kind(the_token) != TK_IDENTIFIER)
      {
        if (is_formal)
          {
            is_immutable = TRUE;
            which_keyword = KK_VARIABLE;
            goto prefix_done;
          }
        token_error(the_token,
                "Syntax error -- expected keyword %s%s%s%s`procedure', "
                "`function', `routine', `class', `variable', `immutable', "
                "`tagalong', `lepton', `quark', or `lock', but found %s.",
                (is_static ? "" : "`static', "),
                (is_virtual ? "" : "`virtual', "), (is_pure ? "" : "`pure', "),
                ((single_lock != NULL) ? "" : "`single', "),
                name_for_token_kind(get_token_kind(the_token)));
        if (single_lock != NULL)
          {
            delete_expression(single_lock);
            delete_unbound_name_manager(single_lock_manager);
          }
        return NULL;
      }

    id_chars = aliased_token_name(the_token, the_parser);

    if (strcmp(id_chars, "static") == 0)
      {
        if (is_static)
          {
            token_error(the_token,
                        "Syntax error -- keyword `static' is duplicated.");
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        if (is_dynamic)
          {
            token_error(the_token,
                        "Syntax error -- keyword `static' is not allowed on "
                        "declaration expressions.");
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        which_keyword = KK_STATIC;
      }
    else if (strcmp(id_chars, "ageless") == 0)
      {
        if (is_ageless)
          {
            token_error(the_token,
                        "Syntax error -- keyword `ageless' is duplicated.");
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        if (is_dynamic)
          {
            token_error(the_token,
                        "Syntax error -- keyword `ageless' is not allowed on "
                        "declaration expressions.");
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        which_keyword = KK_AGELESS;
      }
    else if (strcmp(id_chars, "virtual") == 0)
      {
        if (is_virtual)
          {
            token_error(the_token,
                        "Syntax error -- keyword `virtual' is duplicated.");
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        if (is_dynamic)
          {
            token_error(the_token,
                        "Syntax error -- keyword `virtual' is not allowed on "
                        "declaration expressions.");
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        which_keyword = KK_VIRTUAL;
      }
    else if (strcmp(id_chars, "pure") == 0)
      {
        if (is_formal)
            goto bad_keyword;

        if (is_pure)
          {
            token_error(the_token,
                        "Syntax error -- keyword `pure' is duplicated.");
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        which_keyword = KK_PURE;
      }
    else if (strcmp(id_chars, "single") == 0)
      {
        if (single_lock != NULL)
          {
            token_error(the_token,
                        "Syntax error -- keyword `single' is duplicated.");
            delete_expression(single_lock);
            delete_unbound_name_manager(single_lock_manager);
            return NULL;
          }

        which_keyword = KK_SINGLE;
      }
    else if (strcmp(id_chars, "procedure") == 0)
      {
        if (is_formal)
            goto bad_keyword;

        which_keyword = KK_ROUTINE;
        opening = "procedure";
        return_ok = FALSE;
        return_required = FALSE;
        is_class = FALSE;
      }
    else if (strcmp(id_chars, "function") == 0)
      {
        if (is_formal)
            goto bad_keyword;

        which_keyword = KK_ROUTINE;
        opening = "function";
        return_ok = TRUE;
        return_required = TRUE;
        is_class = FALSE;
      }
    else if (strcmp(id_chars, "routine") == 0)
      {
        if (is_formal)
            goto bad_keyword;

        which_keyword = KK_ROUTINE;
        opening = "routine";
        return_ok = TRUE;
        return_required = FALSE;
        is_class = FALSE;
      }
    else if (strcmp(id_chars, "class") == 0)
      {
        if (is_formal)
            goto bad_keyword;

        which_keyword = KK_ROUTINE;
        opening = "class";
        return_ok = TRUE;
        return_required = FALSE;
        is_class = TRUE;
      }
    else if ((strcmp(id_chars, "variable") == 0) ||
             (strcmp(id_chars, "immutable") == 0))
      {
        if (is_formal)
          {
          bad_keyword:
            token_error(the_token,
                    "Syntax error -- Keyword `%s' is not allowed in a formal "
                    "parameter list.", id_chars);
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        if (is_pure)
          {
            token_error(the_token,
                    "Syntax error -- The `pure' keyword isn't allowed on %s "
                    "declarations.", id_chars);
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        is_immutable = (strcmp(id_chars, "immutable") == 0);

        which_keyword = KK_VARIABLE;
      }
    else if (strcmp(id_chars, "tagalong") == 0)
      {
        if (is_formal)
            goto bad_keyword;

        if (is_pure)
          {
            token_error(the_token,
                    "Syntax error -- The `pure' keyword isn't allowed on "
                    "tagalong declarations.");
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        which_keyword = KK_TAGALONG;
      }
    else if (strcmp(id_chars, "lepton") == 0)
      {
        if (is_formal)
            goto bad_keyword;

        if (is_pure)
          {
            token_error(the_token,
                    "Syntax error -- The `pure' keyword isn't allowed on "
                    "lepton declarations.");
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        if (single_lock != NULL)
          {
            token_error(the_token,
                    "Syntax error -- The `single' keyword isn't allowed on "
                    "lepton declarations.");
            delete_expression(single_lock);
            delete_unbound_name_manager(single_lock_manager);
            return NULL;
          }

        assert(single_lock == NULL);
        assert(single_lock_manager == NULL);

        which_keyword = KK_LEPTON;
      }
    else if (strcmp(id_chars, "quark") == 0)
      {
        if (is_formal)
            goto bad_keyword;

        if (is_pure)
          {
            token_error(the_token,
                    "Syntax error -- The `pure' keyword isn't allowed on quark"
                    " declarations.");
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        if (single_lock != NULL)
          {
            token_error(the_token,
                    "Syntax error -- The `single' keyword isn't allowed on "
                    "quark declarations.");
            delete_expression(single_lock);
            delete_unbound_name_manager(single_lock_manager);
            return NULL;
          }

        assert(single_lock == NULL);
        assert(single_lock_manager == NULL);

        which_keyword = KK_QUARK;
      }
    else if (strcmp(id_chars, "lock") == 0)
      {
        if (is_formal)
            goto bad_keyword;

        if (is_pure)
          {
            token_error(the_token,
                    "Syntax error -- The `pure' keyword isn't allowed on lock "
                    "declarations.");
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        which_keyword = KK_LOCK;
      }
    else
      {
        if (is_formal)
          {
            is_immutable = TRUE;
            which_keyword = KK_VARIABLE;
            goto prefix_done;
          }
        token_error(the_token,
                "Syntax error -- expected keyword %s%s%s%s`procedure', "
                "`function', `routine', `class', `variable', `immutable', "
                "`tagalong', `lepton', `quark', or `lock', but found "
                "identifier `%s'.", (is_static ? "" : "`static', "),
                (is_virtual ? "" : "`virtual', "), (is_pure ? "" : "`pure', "),
                ((single_lock != NULL) ? "" : "`single', "), id_chars);
        if (single_lock != NULL)
          {
            delete_expression(single_lock);
            delete_unbound_name_manager(single_lock_manager);
          }
        return NULL;
      }

    the_verdict = consume_token(the_parser->tokenizer);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (single_lock != NULL)
          {
            delete_expression(single_lock);
            delete_unbound_name_manager(single_lock_manager);
          }
        return NULL;
      }

    if ((which_keyword == KK_QUARK) &&
        next_is_keyword(the_parser, "enumeration"))
      {
        verdict the_verdict;
        const char *enumeration_name;
        type_expression *enumeration_type;
        expression *enumeration_expression;
        type *any_quark_type;
        type *type_of_quarks_type;
        type_expression *immutable_type_expression;
        variable_declaration *the_variable_declaration;
        declaration *the_declaration;

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        if (add_declaration == NULL)
          {
            token_error(the_token,
                    "Syntax error -- quark enumeration used as an "
                    "expression.");
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        the_verdict = expect(the_parser->tokenizer, TK_IDENTIFIER);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        enumeration_name =
                identifier_token_name(next_token(the_parser->tokenizer));
        assert(enumeration_name != NULL);

        enumeration_type = create_enumeration_type_expression();
        if (enumeration_type == NULL)
          {
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        enumeration_expression =
                create_type_expression_expression(enumeration_type);
        if (enumeration_expression == NULL)
          {
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        any_quark_type = get_any_quark_type();
        if (any_quark_type == NULL)
          {
            delete_expression(enumeration_expression);
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        assert(type_is_valid(any_quark_type)); /* VERIFIED */
        type_of_quarks_type = get_type_type(any_quark_type);
        if (type_of_quarks_type == NULL)
          {
            delete_expression(enumeration_expression);
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        immutable_type_expression =
                create_constant_type_expression(type_of_quarks_type);
        type_remove_reference(type_of_quarks_type, NULL);
        if (immutable_type_expression == NULL)
          {
            delete_expression(enumeration_expression);
            if (single_lock != NULL)
              {
                delete_expression(single_lock);
                delete_unbound_name_manager(single_lock_manager);
              }
            return NULL;
          }

        the_variable_declaration = create_variable_declaration(
                immutable_type_expression, enumeration_expression, FALSE, TRUE,
                single_lock);
        if (the_variable_declaration == NULL)
          {
            if (single_lock != NULL)
                delete_unbound_name_manager(single_lock_manager);
            return NULL;
          }

        the_declaration = create_declaration_for_variable(enumeration_name,
                is_static, is_virtual, TRUE, the_variable_declaration,
                next_token_location(the_parser->tokenizer));
        if (the_declaration == NULL)
          {
            if (single_lock != NULL)
                delete_unbound_name_manager(single_lock_manager);
            return NULL;
          }

        the_verdict = consume_token(the_parser->tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            declaration_remove_reference(the_declaration);
            if (single_lock != NULL)
                delete_unbound_name_manager(single_lock_manager);
            return NULL;
          }

        the_verdict =
                expect_and_eat(the_parser->tokenizer, TK_LEFT_CURLY_BRACE);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            declaration_remove_reference(the_declaration);
            if (single_lock != NULL)
                delete_unbound_name_manager(single_lock_manager);
            return NULL;
          }

        while (TRUE)
          {
            verdict the_verdict;
            const char *quark_name;
            quark_declaration *the_quark_declaration;
            declaration *local_declaration;
            expression *quark_reference_expression;

            the_verdict = expect(the_parser->tokenizer, TK_IDENTIFIER);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                declaration_remove_reference(the_declaration);
                if (single_lock != NULL)
                    delete_unbound_name_manager(single_lock_manager);
                return NULL;
              }

            quark_name =
                    identifier_token_name(next_token(the_parser->tokenizer));
            assert(quark_name != NULL);

            the_quark_declaration = create_quark_declaration();
            if (the_quark_declaration == NULL)
              {
                declaration_remove_reference(the_declaration);
                if (single_lock != NULL)
                    delete_unbound_name_manager(single_lock_manager);
                return NULL;
              }

            local_declaration = create_declaration_for_quark(quark_name,
                    is_static, is_virtual, TRUE, the_quark_declaration,
                    next_token_location(the_parser->tokenizer));
            if (local_declaration == NULL)
              {
                declaration_remove_reference(the_declaration);
                if (single_lock != NULL)
                    delete_unbound_name_manager(single_lock_manager);
                return NULL;
              }

            the_verdict =
                    add_declaration(add_declaration_data, local_declaration);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                declaration_remove_reference(the_declaration);
                if (single_lock != NULL)
                    delete_unbound_name_manager(single_lock_manager);
                return NULL;
              }

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                declaration_remove_reference(the_declaration);
                if (single_lock != NULL)
                    delete_unbound_name_manager(single_lock_manager);
                return NULL;
              }

            quark_reference_expression =
                    create_quark_reference_expression(the_quark_declaration);
            if (quark_reference_expression == NULL)
              {
                declaration_remove_reference(the_declaration);
                if (single_lock != NULL)
                    delete_unbound_name_manager(single_lock_manager);
                return NULL;
              }

            the_verdict = enumeration_type_expression_add_case(
                    enumeration_type, quark_reference_expression);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                declaration_remove_reference(the_declaration);
                if (single_lock != NULL)
                    delete_unbound_name_manager(single_lock_manager);
                return NULL;
              }

            if (next_is(the_parser->tokenizer, TK_RIGHT_CURLY_BRACE))
              {
                verdict the_verdict;

                the_verdict =
                        add_declaration(add_declaration_data, the_declaration);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    if (single_lock != NULL)
                        delete_unbound_name_manager(single_lock_manager);
                    return NULL;
                  }

                set_declaration_end_location(local_declaration,
                        next_token_location(the_parser->tokenizer));

                the_verdict = consume_token(the_parser->tokenizer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    if (single_lock != NULL)
                        delete_unbound_name_manager(single_lock_manager);
                    return NULL;
                  }

                if (single_lock_manager == NULL)
                  {
                    single_lock_manager = create_unbound_name_manager();
                    if (single_lock_manager == NULL)
                        return NULL;
                  }

                *result_manager = single_lock_manager;
                return local_declaration;
              }

            the_verdict = expect_and_eat(the_parser->tokenizer, TK_COMMA);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                declaration_remove_reference(the_declaration);
                if (single_lock != NULL)
                    delete_unbound_name_manager(single_lock_manager);
                return NULL;
              }
          }
      }

  prefix_done:
    parent_manager = NULL;

    while (TRUE)
      {
        declaration *the_declaration;
        verdict the_verdict;

        start_location = *(next_token_location(the_parser->tokenizer));

        switch (which_keyword)
          {
            case KK_STATIC:
              {
                the_declaration = parse_declaration(the_parser, TRUE,
                        is_ageless, is_virtual, is_pure, single_lock,
                        single_lock_manager, result_manager, is_formal,
                        is_dynamic, native_handler, the_purity_safety,
                        add_declaration, add_declaration_data, dynamic_type,
                        dynamic_type_manager);
                break;
              }
            case KK_AGELESS:
              {
                the_declaration = parse_declaration(the_parser, is_static,
                        TRUE, is_virtual, is_pure, single_lock,
                        single_lock_manager, result_manager, is_formal,
                        is_dynamic, native_handler, the_purity_safety,
                        add_declaration, add_declaration_data, dynamic_type,
                        dynamic_type_manager);
                break;
              }
            case KK_VIRTUAL:
              {
                the_declaration = parse_declaration(the_parser, is_static,
                        is_ageless, TRUE, is_pure, single_lock,
                        single_lock_manager, result_manager, is_formal,
                        is_dynamic, native_handler, the_purity_safety,
                        add_declaration, add_declaration_data, dynamic_type,
                        dynamic_type_manager);
                break;
              }
            case KK_PURE:
              {
                the_declaration = parse_declaration(the_parser, is_static,
                        is_ageless, is_virtual, TRUE, single_lock,
                        single_lock_manager, result_manager, is_formal,
                        is_dynamic, native_handler, the_purity_safety,
                        add_declaration, add_declaration_data, dynamic_type,
                        dynamic_type_manager);
                break;
              }
            case KK_SINGLE:
              {
                expression *new_lock;
                unbound_name_manager *new_manager;

                assert(single_lock == NULL);
                assert(single_lock_manager == NULL);

                if (next_is(the_parser->tokenizer, TK_LEFT_PAREN))
                  {
                    verdict the_verdict;
                    open_expression *open_lock;

                    the_verdict = expect_and_eat(the_parser->tokenizer,
                                                 TK_LEFT_PAREN);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                        return NULL;

                    open_lock =
                            parse_expression(the_parser, EPP_TOP, NULL, FALSE);
                    if (open_lock == NULL)
                        return NULL;

                    the_verdict = expect_and_eat(the_parser->tokenizer,
                                                 TK_RIGHT_PAREN);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        delete_open_expression(open_lock);
                        return NULL;
                      }

                    decompose_open_expression(open_lock, &new_manager,
                                              &new_lock);
                  }
                else
                  {
                    new_lock = anonymous_lock_expression(&start_location);
                    if (new_lock == NULL)
                        return NULL;

                    new_manager = create_unbound_name_manager();
                    if (new_manager == NULL)
                      {
                        delete_expression(new_lock);
                        return NULL;
                      }
                  }

                the_declaration = parse_declaration(the_parser, is_static,
                        is_ageless, is_virtual, is_pure, new_lock, new_manager,
                        result_manager, is_formal, is_dynamic, native_handler,
                        the_purity_safety, add_declaration,
                        add_declaration_data, dynamic_type,
                        dynamic_type_manager);
                break;
              }
            case KK_ROUTINE:
              {
                the_declaration = parse_routine_declaration(the_parser,
                        opening, return_ok, return_required, is_class,
                        is_static, is_ageless, is_virtual, is_pure,
                        single_lock, single_lock_manager, result_manager,
                        native_handler, the_purity_safety, &start_location);
                break;
              }
            case KK_VARIABLE:
              {
                the_declaration = parse_variable_declaration(the_parser,
                        is_immutable, is_static, is_ageless, is_virtual,
                        single_lock, single_lock_manager, result_manager,
                        dynamic_type, dynamic_type_manager);
                break;
              }
            case KK_TAGALONG:
              {
                the_declaration = parse_tagalong_declaration(the_parser,
                        is_static, is_ageless, is_virtual, single_lock,
                        single_lock_manager, result_manager);
                break;
              }
            case KK_LEPTON:
              {
                the_declaration = parse_lepton_key_declaration(the_parser,
                        is_static, is_ageless, is_virtual, result_manager);
                break;
              }
            case KK_QUARK:
              {
                the_declaration = parse_quark_declaration(the_parser,
                        is_static, is_ageless, is_virtual, result_manager);
                break;
              }
            case KK_LOCK:
              {
                the_declaration = parse_lock_declaration(the_parser, is_static,
                        is_ageless, is_virtual, single_lock,
                        single_lock_manager, result_manager);
                break;
              }
            default:
              {
                assert(FALSE);
                the_declaration = NULL;
              }
          }

        if (parent_manager != NULL)
          {
            verdict the_verdict;

            the_verdict = merge_in_unbound_name_manager(parent_manager,
                                                        *result_manager);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_unbound_name_manager(parent_manager);
                declaration_remove_reference(the_declaration);
                return NULL;
              }
            *result_manager = parent_manager;
          }
        else
          {
            parent_manager = *result_manager;
          }

        if (the_declaration != NULL)
            set_declaration_start_location(the_declaration, &start_location);

        switch (which_keyword)
          {
            case KK_STATIC:
            case KK_AGELESS:
            case KK_VIRTUAL:
            case KK_PURE:
            case KK_SINGLE:
                return the_declaration;
            case KK_ROUTINE:
            case KK_VARIABLE:
            case KK_TAGALONG:
            case KK_LEPTON:
            case KK_QUARK:
            case KK_LOCK:
                break;
            default:
                assert(FALSE);
          }

        if ((the_declaration == NULL) || (add_declaration == NULL))
            return the_declaration;

        the_verdict = add_declaration(add_declaration_data, the_declaration);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(*result_manager);
            return NULL;
          }

        if (!(next_is(the_parser->tokenizer, TK_COMMA)))
            return the_declaration;

        the_verdict = expect_and_eat(the_parser->tokenizer, TK_COMMA);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            delete_unbound_name_manager(*result_manager);
            return NULL;
          }
      }
  }

extern verdict verify_end_of_input(parser *the_parser)
  {
    return expect_and_eat(the_parser->tokenizer, TK_END_OF_INPUT);
  }

extern expression *parse_stand_alone_immediate_expression_with_source_info(
        const char *text, const char *source_file_name,
        size_t source_line_number)
  {
    tokenizer *the_tokenizer;
    parser *the_parser;
    open_expression *the_open_expression;
    verdict the_verdict;
    unbound_name_manager *manager;
    expression *result;

    the_tokenizer = create_tokenizer(text, source_file_name);
    if (the_tokenizer == NULL)
        return NULL;

    set_tokenizer_line_number(the_tokenizer, source_line_number);
    set_tokenizer_column_number(the_tokenizer, 1);

    the_parser = create_parser(the_tokenizer, &local_file_include_handler,
            &local_file_interface_include_handler, NULL, NULL, TRUE);
    if (the_parser == NULL)
      {
        delete_tokenizer(the_tokenizer);
        return NULL;
      }

    the_open_expression =
            parse_routine_or_new_expression(the_parser, NULL, NULL);
    the_verdict = verify_end_of_input(the_parser);
    delete_parser(the_parser);
    delete_tokenizer(the_tokenizer);
    if (the_open_expression == NULL)
        return NULL;
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_expression(the_open_expression);
        return NULL;
      }

    decompose_open_expression(the_open_expression, &manager, &result);
    delete_unbound_name_manager(manager);

    assert(result != NULL);
    return result;
  }


static verdict expect(tokenizer *the_tokenizer, token_kind expected_kind)
  {
    token *the_token;
    token_kind kind;

    assert(the_tokenizer != NULL);

    the_token = next_token(the_tokenizer);
    if (the_token == NULL)
        return MISSION_FAILED;

    kind = get_token_kind(the_token);

    if (kind == TK_ERROR)
        return MISSION_FAILED;

    if (kind != expected_kind)
      {
        token_error(the_token, "Syntax error -- expected %s, found %s.",
                name_for_token_kind(expected_kind), name_for_token_kind(kind));
        return MISSION_FAILED;
      }

    return MISSION_ACCOMPLISHED;
  }

static verdict expect2(tokenizer *the_tokenizer, token_kind expected_kind1,
                       token_kind expected_kind2, token_kind *which)
  {
    token *the_token;
    token_kind kind;

    assert(the_tokenizer != NULL);
    assert(which != NULL);

    the_token = next_token(the_tokenizer);
    if (the_token == NULL)
        return MISSION_FAILED;

    kind = get_token_kind(the_token);

    if (kind == TK_ERROR)
        return MISSION_FAILED;

    if ((kind != expected_kind1) && (kind != expected_kind2))
      {
        token_error(the_token, "Syntax error -- expected %s or %s, found %s.",
                name_for_token_kind(expected_kind1),
                name_for_token_kind(expected_kind2),
                name_for_token_kind(kind));
        return MISSION_FAILED;
      }

    *which = kind;
    return MISSION_ACCOMPLISHED;
  }

static verdict expect_and_eat(tokenizer *the_tokenizer,
                              token_kind expected_kind)
  {
    verdict the_verdict;

    assert(the_tokenizer != NULL);

    the_verdict = expect(the_tokenizer, expected_kind);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    return consume_token(the_tokenizer);
  }

static verdict expect_and_eat2(tokenizer *the_tokenizer,
        token_kind expected_kind1, token_kind expected_kind2,
        token_kind *which)
  {
    verdict the_verdict;

    assert(the_tokenizer != NULL);
    assert(which != NULL);

    the_verdict =
            expect2(the_tokenizer, expected_kind1, expected_kind2, which);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    return consume_token(the_tokenizer);
  }

static verdict expect_and_eat_keyword(parser *the_parser, const char *keyword)
  {
    token *the_token;
    token_kind kind;
    const char *id_chars;

    assert(the_parser != NULL);
    assert(keyword != NULL);

    the_token = next_token(the_parser->tokenizer);
    if (the_token == NULL)
        return MISSION_FAILED;

    kind = get_token_kind(the_token);

    if (kind == TK_ERROR)
        return MISSION_FAILED;

    if (kind != TK_IDENTIFIER)
      {
        token_error(the_token, "Syntax error -- expected keyword `%s'.",
                    keyword);
        return MISSION_FAILED;
      }

    id_chars = aliased_token_name(the_token, the_parser);
    assert(id_chars != NULL);

    if (strcmp(id_chars, keyword) != 0)
      {
        token_error(the_token, "Syntax error -- expected keyword `%s'.",
                    keyword);
        return MISSION_FAILED;
      }

    return consume_token(the_parser->tokenizer);
  }

static char *expect_and_eat_identifier_return_name_copy(parser *the_parser)
  {
    return expect_and_eat_possibly_aliased_identifier_return_name_copy(
            the_parser, FALSE);
  }

static char *expect_and_eat_aliased_identifier_return_name_copy(
        parser *the_parser)
  {
    return expect_and_eat_possibly_aliased_identifier_return_name_copy(
            the_parser, TRUE);
  }

static char *expect_and_eat_possibly_aliased_identifier_return_name_copy(
        parser *the_parser, boolean check_aliases)
  {
    tokenizer *the_tokenizer;
    verdict the_verdict;
    token *the_token;
    const char *original_chars;
    char *result;

    assert(the_parser != NULL);

    the_tokenizer = the_parser->tokenizer;

    assert(the_tokenizer != NULL);

    the_verdict = expect(the_tokenizer, TK_IDENTIFIER);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    the_token = next_token(the_tokenizer);
    assert(the_token != NULL);

    if (check_aliases)
        original_chars = aliased_token_name(the_token, the_parser);
    else
        original_chars = identifier_token_name(the_token);

    result = MALLOC_ARRAY(char, strlen(original_chars) + 1);
    if (result == NULL)
        return NULL;

    strcpy(result, original_chars);

    the_verdict = consume_token(the_tokenizer);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    return result;
  }

static char *expect_and_eat_string_literal_return_name_copy(parser *the_parser)
  {
    tokenizer *the_tokenizer;
    verdict the_verdict;
    token *the_token;
    const char *original_chars;
    char *result;

    assert(the_parser != NULL);

    the_tokenizer = the_parser->tokenizer;

    assert(the_tokenizer != NULL);

    the_verdict = expect(the_tokenizer, TK_STRING_LITERAL);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    the_token = next_token(the_tokenizer);
    assert(the_token != NULL);

    original_chars = string_literal_token_data(the_token);

    result = MALLOC_ARRAY(char, strlen(original_chars) + 1);
    if (result == NULL)
        return NULL;

    strcpy(result, original_chars);

    the_verdict = consume_token(the_tokenizer);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(result);
        return NULL;
      }

    return result;
  }

static const char *name_for_token_kind(token_kind kind)
  {
    switch (kind)
      {
        case TK_IDENTIFIER:
            return "identifier";
        case TK_STRING_LITERAL:
            return "string literal";
        case TK_CHARACTER_LITERAL:
            return "character literal";
        case TK_DECIMAL_INTEGER_LITERAL:
            return "decimal integer literal";
        case TK_HEXADECIMAL_INTEGER_LITERAL:
            return "hexadecimal integer literal";
        case TK_SCIENTIFIC_NOTATION_LITERAL:
            return "scientific notation literal";
        case TK_REGULAR_EXPRESSION_LITERAL:
            return "regular expression literal";
        case TK_LEFT_PAREN:
            return "left parenthesis (\"(\")";
        case TK_RIGHT_PAREN:
            return "right parenthesis (\")\")";
        case TK_LEFT_BRACKET:
            return "left square bracket (\"[\")";
        case TK_RIGHT_BRACKET:
            return "right square bracket (\"]\")";
        case TK_LEFT_CURLY_BRACE:
            return "left curly brace (\"{\")";
        case TK_RIGHT_CURLY_BRACE:
            return "right curly brace (\"}\")";
        case TK_COLON:
            return "colon (\":\")";
        case TK_SEMICOLON:
            return "semicolon (\";\")";
        case TK_COMMA:
            return "comma (\",\")";
        case TK_DOT:
            return "dot (\".\")";
        case TK_DOT_DOT:
            return "two dots (\"..\")";
        case TK_DOT_DOT_DOT:
            return "three dots (\"...\")";
        case TK_DOT_DOT_DOT_DOT:
            return "four dots (\"....\")";
        case TK_ASSIGN:
            return "simple assignment operator (\":=\")";
        case TK_MODULO_ASSIGN:
            return "module assignment operator (\"::=\")";
        case TK_MULTIPLY_ASSIGN:
            return "multiply assignment operator (\"*=\")";
        case TK_DIVIDE_ASSIGN:
            return "divide assignment operator (\"/=\")";
        case TK_DIVIDE_FORCE_ASSIGN:
            return "divide-force assignment operator (\"/::=\")";
        case TK_REMAINDER_ASSIGN:
            return "remainder assignment operator (\"%=\")";
        case TK_ADD_ASSIGN:
            return "add assignment operator (\"+=\")";
        case TK_SUBTRACT_ASSIGN:
            return "subtract assignment operator (\"-=\")";
        case TK_SHIFT_LEFT_ASSIGN:
            return "shift left assignment operator (\"<<=\")";
        case TK_SHIFT_RIGHT_ASSIGN:
            return "shift right assignment operator (\">>=\")";
        case TK_BITWISE_AND_ASSIGN:
            return "bitwise and assignment operator (\"&=\")";
        case TK_BITWISE_XOR_ASSIGN:
            return "bitwise exclusive or assignment operator (\"^=\")";
        case TK_BITWISE_OR_ASSIGN:
            return "bitwise inclusive or assignment operator (\"|=\")";
        case TK_LOGICAL_AND_ASSIGN:
            return "logical and assignment operator (\"&&=\")";
        case TK_LOGICAL_OR_ASSIGN:
            return "logical or assignment operator (\"||=\")";
        case TK_CONCATENATE_ASSIGN:
            return "concatenate assignment operator (\"~=\")";
        case TK_PLUS_PLUS:
            return "increment operator (\"++\")";
        case TK_MINUS_MINUS:
            return "decrement operator (\"--\")";
        case TK_STAR:
            return "star (\"*\")";
        case TK_DIVIDE:
            return "forward slash (\"/\")";
        case TK_DIVIDE_FORCE:
            return "divide-force (\"/::\")";
        case TK_REMAINDER:
            return "remainder (\"%\")";
        case TK_ADD:
            return "add (\"+\")";
        case TK_DASH:
            return "dash (\"-\")";
        case TK_SHIFT_LEFT:
            return "shift left (\"<<\")";
        case TK_SHIFT_RIGHT:
            return "shift right (\">>\")";
        case TK_AMPERSAND:
            return "ampersand (\"&\")";
        case TK_BITWISE_XOR:
            return "bitwise exclusive or (\"^\")";
        case TK_BITWISE_OR:
            return "bitwise inclusive or (\"|\")";
        case TK_LOGICAL_AND:
            return "logical and (\"&&\")";
        case TK_LOGICAL_OR:
            return "logical or (\"||\")";
        case TK_EQUAL:
            return "equal (\"==\")";
        case TK_NOT_EQUAL:
            return "not equal (\"!=\")";
        case TK_LESS_THAN:
            return "less than (\"<\")";
        case TK_GREATER_THAN:
            return "greater than (\">\")";
        case TK_LESS_THAN_OR_EQUAL:
            return "less than or equal (\"<=\")";
        case TK_GREATER_THAN_OR_EQUAL:
            return "greater than or equal (\">=\")";
        case TK_LOGICAL_NOT:
            return "logical not (\"!\")";
        case TK_QUESTION_MARK:
            return "question mark (\"?\")";
        case TK_POINTS_TO:
            return "points to (\"->\")";
        case TK_BITWISE_NOT:
            return "tilde (\"~\")";
        case TK_RETURNS:
            return "returns (\"<--\")";
        case TK_MAPS_TO:
            return "maps to (\"-->\")";
        case TK_FORCE:
            return "force (\"::\")";
        case TK_END_OF_INPUT:
            return "end of input";
        case TK_ERROR:
            return "error";
        default:
            assert(FALSE);
            return NULL;
      }
  }

static boolean next_is(tokenizer *the_tokenizer, token_kind kind)
  {
    token *the_token;

    assert(the_tokenizer != NULL);

    the_token = next_token(the_tokenizer);
    if (the_token == NULL)
        return FALSE;

    return (get_token_kind(the_token) == kind);
  }

static boolean next_is_keyword(parser *the_parser, const char *keyword)
  {
    token *the_token;

    assert(the_parser != NULL);
    assert(keyword != NULL);

    the_token = next_token(the_parser->tokenizer);
    if (the_token == NULL)
        return FALSE;

    if (get_token_kind(the_token) != TK_IDENTIFIER)
        return FALSE;

    return (strcmp(aliased_token_name(the_token, the_parser), keyword) == 0);
  }

static void decompose_open_type_expression(
        open_type_expression *the_open_type_expression,
        unbound_name_manager **manager, type_expression **the_type_expression)
  {
    assert(the_open_type_expression != NULL);
    assert(manager != NULL);
    assert(the_type_expression != NULL);

    *the_type_expression =
            open_type_expression_type_expression(the_open_type_expression);
    assert((*the_type_expression) != NULL);

    *manager = open_type_expression_unbound_name_manager(
            the_open_type_expression);
    assert((*manager) != NULL);

    set_open_type_expression_type_expression(the_open_type_expression, NULL);
    set_open_type_expression_unbound_name_manager(the_open_type_expression,
                                                  NULL);

    delete_open_type_expression(the_open_type_expression);
  }

static void decompose_open_call(open_call *the_open_call,
        unbound_name_manager **manager, call **the_call)
  {
    assert(the_open_call != NULL);
    assert(manager != NULL);
    assert(the_call != NULL);

    *the_call = open_call_call(the_open_call);
    assert((*the_call) != NULL);

    *manager = open_call_unbound_name_manager(the_open_call);
    assert((*manager) != NULL);

    set_open_call_call(the_open_call, NULL);
    set_open_call_unbound_name_manager(the_open_call, NULL);

    delete_open_call(the_open_call);
  }

static void decompose_open_basket(open_basket *the_open_basket,
        unbound_name_manager **manager, basket **the_basket)
  {
    assert(the_open_basket != NULL);
    assert(manager != NULL);
    assert(the_basket != NULL);

    *the_basket = open_basket_basket(the_open_basket);
    assert((*the_basket) != NULL);

    *manager = open_basket_unbound_name_manager(the_open_basket);
    assert((*manager) != NULL);

    set_open_basket_basket(the_open_basket, NULL);
    set_open_basket_unbound_name_manager(the_open_basket, NULL);

    delete_open_basket(the_open_basket);
  }

static expression_parsing_precedence token_prefix_precedence(token_kind kind)
  {
    return EPP_UNARY;
  }

static expression_parsing_precedence token_postfix_precedence(token_kind kind)
  {
    switch (kind)
      {
        case TK_STAR:
            return EPP_MULTIPLICATIVE;
        case TK_DIVIDE:
            return EPP_MULTIPLICATIVE;
        case TK_DIVIDE_FORCE:
            return EPP_MULTIPLICATIVE;
        case TK_REMAINDER:
            return EPP_MULTIPLICATIVE;
        case TK_ADD:
            return EPP_ADDITIVE;
        case TK_DASH:
            return EPP_ADDITIVE;
        case TK_SHIFT_LEFT:
            return EPP_SHIFT;
        case TK_SHIFT_RIGHT:
            return EPP_SHIFT;
        case TK_AMPERSAND:
            return EPP_BITWISE_AND;
        case TK_BITWISE_XOR:
            return EPP_BITWISE_XOR;
        case TK_BITWISE_OR:
            return EPP_BITWISE_OR;
        case TK_LOGICAL_AND:
            return EPP_LOGICAL_AND;
        case TK_LOGICAL_OR:
            return EPP_LOGICAL_OR;
        case TK_BITWISE_NOT:
            return EPP_CONCATENATE;
        case TK_EQUAL:
            return EPP_EQUALITY;
        case TK_NOT_EQUAL:
            return EPP_EQUALITY;
        case TK_LESS_THAN:
            return EPP_RELATIONAL;
        case TK_GREATER_THAN:
            return EPP_RELATIONAL;
        case TK_LESS_THAN_OR_EQUAL:
            return EPP_RELATIONAL;
        case TK_GREATER_THAN_OR_EQUAL:
            return EPP_RELATIONAL;
        case TK_LEFT_BRACKET:
            return EPP_POSTFIX;
        case TK_DOT:
            return EPP_POSTFIX;
        case TK_POINTS_TO:
            return EPP_POSTFIX;
        case TK_DOT_DOT:
            return EPP_POSTFIX;
        case TK_LEFT_PAREN:
            return EPP_POSTFIX;
        case TK_QUESTION_MARK:
            return EPP_CONDITIONAL;
        case TK_IDENTIFIER:
            return EPP_POSTFIX;
        case TK_FORCE:
            return EPP_FORCE;
        default:
            return EPP_TOP;
      }
  }

static type_expression_parsing_precedence token_prefix_type_precedence(
        token_kind kind)
  {
    switch (kind)
      {
        case TK_LOGICAL_NOT:
            return TEPP_NOT;
        case TK_STAR:
            return TEPP_POINTER;
        default:
            return TEPP_TOP;
      }
  }

static type_expression_parsing_precedence token_postfix_type_precedence(
        token_kind kind)
  {
    switch (kind)
      {
        case TK_AMPERSAND:
            return TEPP_AND;
        case TK_BITWISE_XOR:
            return TEPP_XOR;
        case TK_BITWISE_OR:
            return TEPP_OR;
        case TK_LEFT_BRACKET:
            return TEPP_ARRAY;
        case TK_MAPS_TO:
            return TEPP_MAP;
        case TK_RETURNS:
            return TEPP_ROUTINE;
        default:
            return TEPP_TOP;
      }
  }

static boolean token_is_left_associative(token_kind kind)
  {
    return TRUE;
  }

static boolean token_is_unary_postfix_operator(token_kind kind)
  {
    return FALSE;
  }

static boolean token_is_binary_operator(token_kind kind)
  {
    switch (kind)
      {
        case TK_STAR:
        case TK_DIVIDE:
        case TK_DIVIDE_FORCE:
        case TK_REMAINDER:
        case TK_ADD:
        case TK_DASH:
        case TK_SHIFT_LEFT:
        case TK_SHIFT_RIGHT:
        case TK_AMPERSAND:
        case TK_BITWISE_XOR:
        case TK_BITWISE_OR:
        case TK_LOGICAL_AND:
        case TK_LOGICAL_OR:
        case TK_BITWISE_NOT:
        case TK_EQUAL:
        case TK_NOT_EQUAL:
        case TK_LESS_THAN:
        case TK_GREATER_THAN:
        case TK_LESS_THAN_OR_EQUAL:
        case TK_GREATER_THAN_OR_EQUAL:
            return TRUE;
        default:
            return FALSE;
      }
  }

static expression_kind token_unary_prefix_expression_kind(token_kind kind)
  {
    switch (kind)
      {
        case TK_STAR:
            return EK_DEREFERENCE;
        case TK_AMPERSAND:
            return EK_LOCATION_OF;
        case TK_DASH:
            return EK_NEGATE;
        case TK_ADD:
            return EK_UNARY_PLUS;
        case TK_BITWISE_NOT:
            return EK_BITWISE_NOT;
        case TK_LOGICAL_NOT:
            return EK_LOGICAL_NOT;
        default:
            assert(FALSE);
            return EK_CONSTANT;
      }
  }

static expression_kind token_unary_postfix_expression_kind(token_kind kind)
  {
    assert(FALSE);
    return EK_CONSTANT;
  }

static expression_kind token_binary_expression_kind(token_kind kind)
  {
    switch (kind)
      {
        case TK_STAR:
            return EK_MULTIPLY;
        case TK_DIVIDE:
            return EK_DIVIDE;
        case TK_DIVIDE_FORCE:
            return EK_DIVIDE_FORCE;
        case TK_REMAINDER:
            return EK_REMAINDER;
        case TK_ADD:
            return EK_ADD;
        case TK_DASH:
            return EK_SUBTRACT;
        case TK_SHIFT_LEFT:
            return EK_SHIFT_LEFT;
        case TK_SHIFT_RIGHT:
            return EK_SHIFT_RIGHT;
        case TK_AMPERSAND:
            return EK_BITWISE_AND;
        case TK_BITWISE_XOR:
            return EK_BITWISE_XOR;
        case TK_BITWISE_OR:
            return EK_BITWISE_OR;
        case TK_LOGICAL_AND:
            return EK_LOGICAL_AND;
        case TK_LOGICAL_OR:
            return EK_LOGICAL_OR;
        case TK_BITWISE_NOT:
            return EK_CONCATENATE;
        case TK_EQUAL:
            return EK_EQUAL;
        case TK_NOT_EQUAL:
            return EK_NOT_EQUAL;
        case TK_LESS_THAN:
            return EK_LESS_THAN;
        case TK_GREATER_THAN:
            return EK_GREATER_THAN;
        case TK_LESS_THAN_OR_EQUAL:
            return EK_LESS_THAN_OR_EQUAL;
        case TK_GREATER_THAN_OR_EQUAL:
            return EK_GREATER_THAN_OR_EQUAL;
        default:
            assert(FALSE);
            return EK_CONSTANT;
      }
  }

static void show_previous_declaration(statement_block *the_statement_block,
                                      size_t name_number)
  {
    const source_location *location;

    assert(the_statement_block != NULL);

    switch (statement_block_name_kind(the_statement_block, name_number))
      {
        case NK_VARIABLE:
        case NK_ROUTINE:
        case NK_TAGALONG:
        case NK_LEPTON_KEY:
        case NK_QUARK:
        case NK_LOCK:
            location = get_declaration_location(
                    statement_block_name_declaration(the_statement_block,
                                                     name_number));
            break;
        case NK_JUMP_TARGET:
            location = get_statement_location(
                    statement_block_name_jump_target_declaration(
                            the_statement_block, name_number));
            break;
        default:
            assert(FALSE);
            location = NULL;
      }

    location_error(location, "The previous declaration was here.");
  }

static verdict parse_data_declaration_tail(parser *the_parser,
        unbound_name_manager **manager, char **name,
        type_expression **the_type, expression **initializer,
        boolean *initializer_is_forced, type_expression **dynamic_type,
        unbound_name_manager **dynamic_type_manager,
        source_location *end_location)
  {
    token *the_next_token;
    token_kind next_kind;

    assert(the_parser != NULL);
    assert(manager != NULL);
    assert(name != NULL);
    assert(the_type != NULL);
    assert(initializer != NULL);
    assert(initializer_is_forced != NULL);
    assert(end_location != NULL);

    if (next_is(the_parser->tokenizer, TK_IDENTIFIER))
      {
        *end_location = *(next_token_location(the_parser->tokenizer));
        *name = expect_and_eat_identifier_return_name_copy(the_parser);
        if (*name == NULL)
          {
            if (*manager != NULL)
                delete_unbound_name_manager(*manager);
            return MISSION_FAILED;
          }
      }
    else
      {
        *name = NULL;
      }

    if (next_is(the_parser->tokenizer, TK_COLON))
      {
        verdict the_verdict;
        open_type_expression *open_type;
        unbound_name_manager *child_manager;

        the_verdict = expect_and_eat(the_parser->tokenizer, TK_COLON);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            if (*name != NULL)
                free(*name);
            if (*manager != NULL)
                delete_unbound_name_manager(*manager);
            return the_verdict;
          }

        open_type = parse_type_expression_with_end_location(the_parser,
                TEPP_TOP, end_location);
        if (open_type == NULL)
          {
            if (*name != NULL)
                free(*name);
            if (*manager != NULL)
                delete_unbound_name_manager(*manager);
            return MISSION_FAILED;
          }

        decompose_open_type_expression(open_type, &child_manager, the_type);

        if (*manager == NULL)
          {
            *manager = child_manager;
          }
        else
          {
            verdict the_verdict;

            the_verdict =
                    merge_in_unbound_name_manager(*manager, child_manager);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                if (*name != NULL)
                    free(*name);
                delete_type_expression(*the_type);
                delete_unbound_name_manager(*manager);
                return the_verdict;
              }
          }

        if ((dynamic_type != NULL) &&
            next_is(the_parser->tokenizer, TK_DIVIDE))
          {
            verdict the_verdict;
            open_type_expression *dynamic_open_type;

            assert(dynamic_type_manager != NULL);

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                if (*name != NULL)
                    free(*name);
                delete_type_expression(*the_type);
                delete_unbound_name_manager(*manager);
                return the_verdict;
              }

            dynamic_open_type = parse_type_expression_with_end_location(
                    the_parser, TEPP_TOP, end_location);
            if (dynamic_open_type == NULL)
              {
                if (*name != NULL)
                    free(*name);
                delete_type_expression(*the_type);
                delete_unbound_name_manager(*manager);
                return MISSION_FAILED;
              }

            decompose_open_type_expression(dynamic_open_type,
                                           dynamic_type_manager, dynamic_type);
          }
      }
    else
      {
        type *anything_type;

        anything_type = get_anything_type();
        if (anything_type == NULL)
          {
            if (*name != NULL)
                free(*name);
            if (*manager != NULL)
                delete_unbound_name_manager(*manager);
            return MISSION_FAILED;
          }

        *the_type = create_constant_type_expression(anything_type);
        if (*the_type == NULL)
          {
            if (*name != NULL)
                free(*name);
            if (*manager != NULL)
                delete_unbound_name_manager(*manager);
            return MISSION_FAILED;
          }
      }

    assert(the_parser->tokenizer != NULL);

    the_next_token = next_token(the_parser->tokenizer);
    if (the_next_token == NULL)
        next_kind = TK_ERROR;
    else
        next_kind = get_token_kind(the_next_token);

    if ((next_kind == TK_ASSIGN) || (next_kind == TK_MODULO_ASSIGN))
      {
        verdict the_verdict;
        open_expression *open_initializer;
        unbound_name_manager *child_manager;

        *initializer_is_forced = (next_kind == TK_MODULO_ASSIGN);

        the_verdict = expect_and_eat(the_parser->tokenizer, next_kind);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            if (*name != NULL)
                free(*name);
            delete_type_expression(*the_type);
            if (*manager != NULL)
                delete_unbound_name_manager(*manager);
            if ((dynamic_type_manager != NULL) &&
                (*dynamic_type_manager != NULL))
              {
                delete_unbound_name_manager(*dynamic_type_manager);
              }
            return MISSION_FAILED;
          }

        open_initializer = parse_expression_with_end_location(the_parser,
                EPP_TOP, NULL, FALSE, end_location);
        if (open_initializer == NULL)
          {
            if (*name != NULL)
                free(*name);
            delete_type_expression(*the_type);
            if (*manager != NULL)
                delete_unbound_name_manager(*manager);
            if ((dynamic_type_manager != NULL) &&
                (*dynamic_type_manager != NULL))
              {
                delete_unbound_name_manager(*dynamic_type_manager);
              }
            return MISSION_FAILED;
          }

        decompose_open_expression(open_initializer, &child_manager,
                                  initializer);

        if (dynamic_type_manager != NULL)
          {
            if (*dynamic_type_manager == NULL)
              {
                *dynamic_type_manager = child_manager;
              }
            else
              {
                verdict the_verdict;

                the_verdict = merge_in_unbound_name_manager(
                        *dynamic_type_manager, child_manager);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    if (*name != NULL)
                        free(*name);
                    delete_type_expression(*the_type);
                    delete_expression(*initializer);
                    if (*manager != NULL)
                        delete_unbound_name_manager(*manager);
                    delete_unbound_name_manager(*dynamic_type_manager);
                    return the_verdict;
                  }
              }
          }
        else if (*manager == NULL)
          {
            *manager = child_manager;
          }
        else
          {
            verdict the_verdict;

            the_verdict =
                    merge_in_unbound_name_manager(*manager, child_manager);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                if (*name != NULL)
                    free(*name);
                delete_type_expression(*the_type);
                delete_expression(*initializer);
                delete_unbound_name_manager(*manager);
                return the_verdict;
              }
          }
      }
    else
      {
        *initializer = NULL;
        *initializer_is_forced = FALSE;
      }

    if (*manager == NULL)
      {
        *manager = create_unbound_name_manager();
        if (*manager == NULL)
          {
            if (*name != NULL)
                free(*name);
            delete_type_expression(*the_type);
            if (*initializer != NULL)
                delete_expression(*initializer);
            if ((dynamic_type_manager != NULL) &&
                (*dynamic_type_manager != NULL))
              {
                delete_unbound_name_manager(*dynamic_type_manager);
              }
            return MISSION_FAILED;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

static expression *create_oi_expression(o_integer oi)
  {
    value *the_value;
    expression *result;

    the_value = create_integer_value(oi);
    if (the_value == NULL)
        return NULL;

    result = create_constant_expression(the_value);
    value_remove_reference(the_value, NULL);
    return result;
  }

static boolean is_possible_range_type_expression(parser *the_parser)
  {
    token *first_token;
    token *test_token;
    token_kind kind;
    size_t forward_count;
    size_t nesting_level;

    assert(the_parser != NULL);

    first_token = next_token(the_parser->tokenizer);
    assert(first_token != NULL);
    assert((get_token_kind(first_token) == TK_LEFT_BRACKET) ||
           (get_token_kind(first_token) == TK_LEFT_PAREN));

    test_token = forward_token(the_parser->tokenizer, 1);
    if ((test_token == NULL) || (get_token_kind(test_token) == TK_ERROR))
        return FALSE;

    kind = get_token_kind(test_token);

    if (kind == TK_DOT_DOT_DOT)
        return FALSE;

    forward_count = 1;
    nesting_level = 0;

    while (TRUE)
      {
        if (kind == TK_END_OF_INPUT)
            return FALSE;

        if ((kind == TK_RIGHT_BRACKET) || (kind == TK_RIGHT_CURLY_BRACE) ||
            (kind == TK_RIGHT_PAREN))
          {
            if (nesting_level == 0)
                return FALSE;
            --nesting_level;
          }

        if ((kind == TK_LEFT_BRACKET) || (kind == TK_LEFT_CURLY_BRACE) ||
            (kind == TK_LEFT_PAREN))
          {
            ++nesting_level;
          }

        if ((nesting_level == 0) && (kind == TK_COMMA))
            return FALSE;

        ++forward_count;

        test_token = forward_token(the_parser->tokenizer, forward_count);
        if ((test_token == NULL) || (get_token_kind(test_token) == TK_ERROR))
            return FALSE;

        kind = get_token_kind(test_token);

        if ((nesting_level == 0) &&
            ((kind == TK_DOT_DOT_DOT) || (kind == TK_DOT_DOT_DOT_DOT)))
          {
            return TRUE;
          }
      }
  }

static verdict semi_labeled_value_list_type_expression_set_extra_allowed(
        void *data)
  {
    assert(data != NULL);

    return semi_labeled_value_list_type_expression_set_extra_elements_allowed(
            (type_expression *)data, TRUE);
  }

static verdict semi_labeled_value_list_type_expression_set_extra_unspecified(
        void *data, boolean *error)
  {
    assert(data != NULL);

    *error = FALSE;
    return MISSION_FAILED;
  }

static verdict semi_labeled_value_list_type_expression_add_formal(void *data,
        const char *name, type_expression *formal_type, boolean has_default)
  {
    assert(data != NULL);

    if (has_default)
      {
        location_error(get_type_expression_location(formal_type),
                "Default values are not allowed for semi-labeled value list "
                "type expression elements.");
        return MISSION_FAILED;
      }

    return semi_labeled_value_list_type_expression_add_element(
            (type_expression *)data, formal_type, name);
  }

static verdict routine_type_expression_set_extra_allowed(void *data)
  {
    assert(data != NULL);

    return routine_type_expression_set_extra_arguments_allowed(
            (type_expression *)data, TRUE);
  }

static verdict routine_type_expression_set_extra_unspecified(void *data,
                                                             boolean *error)
  {
    verdict the_verdict;

    assert(data != NULL);

    the_verdict = routine_type_expression_set_extra_arguments_unspecified(
            (type_expression *)data, TRUE);
    *error = (the_verdict != MISSION_ACCOMPLISHED);
    return the_verdict;
  }

static verdict routine_type_expression_add_formal_generic(void *data,
        const char *name, type_expression *formal_type, boolean has_default)
  {
    assert(data != NULL);

    return routine_type_expression_add_formal((type_expression *)data,
                                              formal_type, name, has_default);
  }

static verdict fields_set_extra_allowed(type_expression *base_type)
  {
    assert(base_type != NULL);

    return fields_type_expression_set_extra_fields_allowed(base_type, TRUE);
  }

static verdict fields_add_field(type_expression *base_type, const char *name,
                                type_expression *field_type)
  {
    assert(base_type != NULL);

    return fields_type_expression_add_field(base_type, field_type, name);
  }

static verdict lepton_set_extra_allowed(type_expression *base_type)
  {
    assert(base_type != NULL);

    return lepton_type_expression_set_extra_fields_allowed(base_type, TRUE);
  }

static verdict lepton_add_field(type_expression *base_type, const char *name,
                                type_expression *field_type)
  {
    assert(base_type != NULL);

    return lepton_type_expression_add_field(base_type, field_type, name);
  }

static verdict multiset_set_extra_allowed(type_expression *base_type)
  {
    assert(base_type != NULL);

    return multiset_type_expression_set_extra_fields_allowed(base_type, TRUE);
  }

static verdict multiset_add_field(type_expression *base_type, const char *name,
                                  type_expression *field_type)
  {
    assert(base_type != NULL);

    return multiset_type_expression_add_field(base_type, field_type, name);
  }

static open_statement *parser_statement_parser(void *data,
        const char **current_labels, size_t current_label_count)
  {
    assert(data != NULL);

    return parse_statement((parser *)data, current_labels,
                           current_label_count);
  }

static boolean parser_done_test(void *data)
  {
    parser *the_parser;

    assert(data != NULL);

    the_parser = (parser *)data;

    while (TRUE)
      {
        token *the_token;
        token_kind kind;

        the_token = next_token(the_parser->tokenizer);
        if (the_token == NULL)
            return FALSE;

        kind = get_token_kind(the_token);

        if (kind == TK_ERROR)
            return FALSE;

        if (kind == TK_SEMICOLON)
          {
            verdict the_verdict;

            the_verdict = consume_token(the_parser->tokenizer);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return FALSE;

            continue;
          }

        if ((kind == TK_END_OF_INPUT) || (kind == TK_RIGHT_CURLY_BRACE))
            return TRUE;

        return FALSE;
      }
  }

static open_statement *parser_single_statement_parser(void *data,
        const char **current_labels, size_t current_label_count)
  {
    one_statement_data *typed_data;
    open_statement *result;

    typed_data = (one_statement_data *)data;
    assert(typed_data->have_one == NULL);

    result = parse_statement(typed_data->parser, current_labels,
                             current_label_count);

    assert(typed_data->have_one == NULL);
    if (result != NULL)
        typed_data->have_one = open_statement_statement(result);

    return result;
  }

static boolean parser_single_statement_done_test(void *data)
  {
    return (((one_statement_data *)data)->have_one != NULL);
  }

static expression *anonymous_lock_expression(const source_location *location)
  {
    lock_declaration *the_lock_declaration;
    declaration *the_declaration;

    the_lock_declaration = create_lock_declaration(NULL);
    if (the_lock_declaration == NULL)
        return NULL;

    the_declaration = create_declaration_for_lock(NULL, FALSE, FALSE, FALSE,
            the_lock_declaration, location);
    if (the_declaration == NULL)
        return NULL;

    return create_declaration_expression(the_declaration);
  }

static verdict handle_use_statement_for_unbound_name(statement *use_statement,
        const char *internal_name, unbound_name *the_unbound_name,
        unbound_name_manager *manager, boolean flow_through_allowed)
  {
    boolean have_used_for_num;
    size_t used_for_num;
    size_t use_count;
    size_t use_num;

    have_used_for_num = FALSE;

    use_count = unbound_name_use_count(the_unbound_name);
    for (use_num = 0; use_num < use_count; ++use_num)
      {
        unbound_use *the_unbound_use;

        the_unbound_use = unbound_name_use_by_number(the_unbound_name,
                use_count - (use_num + 1));
        assert(the_unbound_use != NULL);

        switch (get_unbound_use_kind(the_unbound_use))
          {
            case UUK_ROUTINE_FOR_RETURN_STATEMENT:
            case UUK_ROUTINE_FOR_EXPORT_STATEMENT:
            case UUK_ROUTINE_FOR_HIDE_STATEMENT:
            case UUK_ROUTINE_FOR_ARGUMENTS_EXPRESSION:
            case UUK_ROUTINE_FOR_THIS_EXPRESSION:
                continue;
            case UUK_ROUTINE_FOR_OPERATOR_EXPRESSION:
            case UUK_ROUTINE_FOR_OPERATOR_STATEMENT:
                break;
            case UUK_LOOP_FOR_BREAK_STATEMENT:
            case UUK_LOOP_FOR_CONTINUE_STATEMENT:
            case UUK_LOOP_FOR_BREAK_EXPRESSION:
            case UUK_LOOP_FOR_CONTINUE_EXPRESSION:
                continue;
            case UUK_VARIABLE_FOR_EXPRESSION:
            case UUK_DANGLING_OVERLOADED_ROUTINE:
            case UUK_USE_FLOW_THROUGH:
                break;
            default:
                assert(FALSE);
          }

        if (!have_used_for_num)
          {
            const source_location *ultimate_use_location;
            verdict the_verdict;

            used_for_num = use_statement_used_for_count(use_statement);

            if (get_unbound_use_kind(the_unbound_use) == UUK_USE_FLOW_THROUGH)
              {
                ultimate_use_location =
                        use_statement_used_for_ultimate_use_location(
                                unbound_use_use_flow_through_use_statement(
                                        the_unbound_use),
                                unbound_use_use_flow_through_used_for_num(
                                        the_unbound_use));
              }
            else
              {
                ultimate_use_location =
                        unbound_use_location(the_unbound_use);
              }

            the_verdict = use_statement_add_used_for_case(use_statement,
                    internal_name, flow_through_allowed,
                    ultimate_use_location);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;

            have_used_for_num = TRUE;
          }

        assert(have_used_for_num);

        switch (get_unbound_use_kind(the_unbound_use))
          {
            case UUK_ROUTINE_FOR_OPERATOR_EXPRESSION:
                expression_set_overload_use(
                        unbound_use_expression(the_unbound_use), use_statement,
                        used_for_num);
                remove_unbound_use(the_unbound_use);
                break;
            case UUK_ROUTINE_FOR_OPERATOR_STATEMENT:
                statement_set_overload_use(
                        unbound_use_statement(the_unbound_use), use_statement,
                        used_for_num);
                remove_unbound_use(the_unbound_use);
                break;
            case UUK_VARIABLE_FOR_EXPRESSION:
                bind_expression_to_use_statement(
                        unbound_use_expression(the_unbound_use), use_statement,
                        used_for_num);
                use_statement_set_used_for_required(use_statement,
                                                    used_for_num, TRUE);
                remove_unbound_use(the_unbound_use);
                break;
            case UUK_DANGLING_OVERLOADED_ROUTINE:
                routine_declaration_chain_set_next_to_use_statement(
                        unbound_use_chain(the_unbound_use), use_statement,
                        used_for_num);
                remove_unbound_use(the_unbound_use);
                break;
            case UUK_USE_FLOW_THROUGH:
                use_statement_set_used_for_next_used(
                        unbound_use_use_flow_through_use_statement(
                                the_unbound_use),
                        unbound_use_use_flow_through_used_for_num(
                                the_unbound_use), use_statement, used_for_num);
                remove_unbound_use(the_unbound_use);
                break;
            default:
                assert(FALSE);
          }
      }

    return MISSION_ACCOMPLISHED;
  }

static verdict statement_add_declaration(void *data,
                                         declaration *new_declaration)
  {
    statement *the_statement;

    assert(data != NULL);
    assert(new_declaration != NULL);

    the_statement = (statement *)data;

    if (declaration_name(new_declaration) == NULL)
      {
        location_error(get_declaration_location(new_declaration),
                "Syntax error -- in a declaration statement, no name was given"
                " for the declaration.");
        return MISSION_FAILED;
      }

    return declaration_statement_add_declaration(the_statement,
                                                 new_declaration);
  }

static verdict do_statement_end_semicolon(parser *the_parser,
                                          statement *the_statement)
  {
    verdict the_verdict;

    assert(the_parser != NULL);
    assert(the_statement != NULL);

    the_verdict = expect(the_parser->tokenizer, TK_SEMICOLON);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    set_statement_end_location(the_statement,
                               next_token_location(the_parser->tokenizer));

    return consume_token(the_parser->tokenizer);
  }

static formal_arguments *parse_formal_arguments(parser *the_parser,
        boolean *extra_arguments_allowed,
        unbound_name_manager **result_static_manager,
        unbound_name_manager **result_dynamic_manager,
        source_location *end_location)
  {
    verdict the_verdict;
    formal_arguments *the_formal_arguments;

    assert(the_parser != NULL);
    assert(extra_arguments_allowed != NULL);
    assert(result_static_manager != NULL);
    assert(result_dynamic_manager != NULL);

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_LEFT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        if (*result_static_manager != NULL)
            delete_unbound_name_manager(*result_static_manager);
        if (*result_dynamic_manager != NULL)
            delete_unbound_name_manager(*result_dynamic_manager);
        return NULL;
      }

    the_formal_arguments = create_formal_arguments();
    if (the_formal_arguments == NULL)
      {
        if (*result_static_manager != NULL)
            delete_unbound_name_manager(*result_static_manager);
        if (*result_dynamic_manager != NULL)
            delete_unbound_name_manager(*result_dynamic_manager);
        return NULL;
      }

    *extra_arguments_allowed = FALSE;

    if (!next_is(the_parser->tokenizer, TK_RIGHT_PAREN))
      {
        while (TRUE)
          {
            unbound_name_manager *child_manager;
            type_expression *dynamic_type;
            unbound_name_manager *dynamic_type_manager;
            declaration *parameter_declaration;
            verdict the_verdict;

            if (next_is(the_parser->tokenizer, TK_DOT_DOT_DOT))
              {
                verdict the_verdict;

                *extra_arguments_allowed = TRUE;

                the_verdict =
                        expect_and_eat(the_parser->tokenizer, TK_DOT_DOT_DOT);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    delete_formal_arguments(the_formal_arguments);
                    if (*result_static_manager != NULL)
                        delete_unbound_name_manager(*result_static_manager);
                    if (*result_dynamic_manager != NULL)
                        delete_unbound_name_manager(*result_dynamic_manager);
                    return NULL;
                  }

                if (!next_is(the_parser->tokenizer, TK_RIGHT_PAREN))
                  {
                    if (!next_is(the_parser->tokenizer, TK_ERROR))
                      {
                        token_error(next_token(the_parser->tokenizer),
                                "Syntax error -- `...' in a formal parameter "
                                "list must be followed by a right "
                                "parenthesis.");
                      }
                    delete_formal_arguments(the_formal_arguments);
                    if (*result_static_manager != NULL)
                        delete_unbound_name_manager(*result_static_manager);
                    if (*result_dynamic_manager != NULL)
                        delete_unbound_name_manager(*result_dynamic_manager);
                    return NULL;
                  }

                break;
              }

            parameter_declaration = parse_declaration(the_parser, FALSE, FALSE,
                    FALSE, FALSE, NULL, NULL, &child_manager, TRUE, FALSE,
                    NULL, PURE_UNSAFE, NULL, NULL, &dynamic_type,
                    &dynamic_type_manager);
            if (parameter_declaration == NULL)
              {
                if (dynamic_type != NULL)
                    delete_type_expression(dynamic_type);
                if (dynamic_type_manager != NULL)
                    delete_unbound_name_manager(dynamic_type_manager);
                delete_formal_arguments(the_formal_arguments);
                if (*result_static_manager != NULL)
                    delete_unbound_name_manager(*result_static_manager);
                if (*result_dynamic_manager != NULL)
                    delete_unbound_name_manager(*result_dynamic_manager);
                return NULL;
              }

            if (*result_static_manager == NULL)
              {
                *result_static_manager = child_manager;
              }
            else
              {
                verdict the_verdict;

                the_verdict = merge_in_unbound_name_manager(
                        *result_static_manager, child_manager);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    if (dynamic_type != NULL)
                        delete_type_expression(dynamic_type);
                    if (dynamic_type_manager != NULL)
                        delete_unbound_name_manager(dynamic_type_manager);
                    declaration_remove_reference(parameter_declaration);
                    delete_formal_arguments(the_formal_arguments);
                    delete_unbound_name_manager(*result_static_manager);
                    if (*result_dynamic_manager != NULL)
                        delete_unbound_name_manager(*result_dynamic_manager);
                    return NULL;
                  }
              }

            if (dynamic_type_manager != NULL)
              {
                if (*result_dynamic_manager == NULL)
                  {
                    *result_dynamic_manager = dynamic_type_manager;
                  }
                else
                  {
                    verdict the_verdict;

                    the_verdict = merge_in_unbound_name_manager(
                            *result_dynamic_manager, dynamic_type_manager);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        if (dynamic_type != NULL)
                            delete_type_expression(dynamic_type);
                        declaration_remove_reference(parameter_declaration);
                        delete_formal_arguments(the_formal_arguments);
                        delete_unbound_name_manager(*result_static_manager);
                        delete_unbound_name_manager(*result_dynamic_manager);
                        return NULL;
                      }
                  }
              }

            the_verdict = add_formal_parameter(the_formal_arguments,
                    parameter_declaration, dynamic_type);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_formal_arguments(the_formal_arguments);
                delete_unbound_name_manager(*result_static_manager);
                if (*result_dynamic_manager != NULL)
                    delete_unbound_name_manager(*result_dynamic_manager);
                return NULL;
              }

            if (next_is(the_parser->tokenizer, TK_RIGHT_PAREN))
                break;

            the_verdict = expect_and_eat(the_parser->tokenizer, TK_COMMA);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                delete_formal_arguments(the_formal_arguments);
                delete_unbound_name_manager(*result_static_manager);
                if (*result_dynamic_manager != NULL)
                    delete_unbound_name_manager(*result_dynamic_manager);
                return NULL;
              }
          }
      }

    if (end_location != NULL)
        *end_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat(the_parser->tokenizer, TK_RIGHT_PAREN);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_formal_arguments(the_formal_arguments);
        if (*result_static_manager != NULL)
            delete_unbound_name_manager(*result_static_manager);
        if (*result_dynamic_manager != NULL)
            delete_unbound_name_manager(*result_dynamic_manager);
        return NULL;
      }

    return the_formal_arguments;
  }

static verdict bind_formals(formal_arguments *formals,
        unbound_name_manager *manager, alias_manager *the_alias_manager)
  {
    size_t formal_count;
    size_t formal_num;

    assert(formals != NULL);
    assert(manager != NULL);

    formal_count = formal_arguments_argument_count(formals);
    for (formal_num = 0; formal_num < formal_count; ++formal_num)
      {
        variable_declaration *declaration;
        const char *formal_name;

        declaration = formal_arguments_formal_by_number(formals, formal_num);
        assert(declaration != NULL);
        formal_name = variable_declaration_name(declaration);
        if (formal_name != NULL)
          {
            const char *alias;
            verdict the_verdict;

            alias = try_aliasing_from_alias_manager(formal_name,
                                                    the_alias_manager);

            if (strcmp(alias, formal_name) != 0)
              {
                location_warning(
                        get_declaration_location(
                                variable_declaration_declaration(declaration)),
                        "`%s' was used as the name of a formal parameter, but "
                        "that name is aliased to `%s', so referencing this "
                        "parameter by this name will not work.", formal_name,
                        alias);
              }

            the_verdict =
                    bind_variable_name(manager, formal_name, declaration);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

static verdict ignore_until(tokenizer *the_tokenizer, token_kind end_kind)
  {
    assert(the_tokenizer != NULL);

    while (TRUE)
      {
        token *the_token;
        token_kind kind;
        verdict the_verdict;

        the_token = next_token(the_tokenizer);
        if (the_token == NULL)
            return MISSION_FAILED;

        kind = get_token_kind(the_token);

        if (kind == end_kind)
            return MISSION_ACCOMPLISHED;

        the_verdict = consume_token(the_tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        switch (kind)
          {
            case TK_ERROR:
              {
                return MISSION_FAILED;
              }
            case TK_LEFT_PAREN:
              {
                the_verdict = ignore_until(the_tokenizer, TK_RIGHT_PAREN);
                break;
              }
            case TK_LEFT_BRACKET:
              {
                the_verdict = ignore_until(the_tokenizer, TK_RIGHT_BRACKET);
                break;
              }
            case TK_LEFT_CURLY_BRACE:
              {
                the_verdict =
                        ignore_until(the_tokenizer, TK_RIGHT_CURLY_BRACE);
                break;
              }
            default:
              {
                continue;
              }
          }

        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        the_verdict = consume_token(the_tokenizer);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }
  }

static const char *aliased_token_name(token *the_token, parser *the_parser)
  {
    return try_aliasing(identifier_token_name(the_token), the_parser);
  }

static const char *try_aliasing(const char *base, parser *the_parser)
  {
    assert(base != NULL);
    assert(the_parser != NULL);

    return try_aliasing_from_alias_manager(base, the_parser->alias_manager);
  }

static const char *try_aliasing_from_alias_manager(const char *base,
        alias_manager *the_alias_manager)
  {
    alias_manager *follow;

    assert(base != NULL);

    follow = the_alias_manager;
    while (follow != NULL)
      {
        string_index *lookup;

        lookup = follow->local;
        if (lookup != NULL)
          {
            const char *result;

            result = (const char *)(lookup_in_string_index(lookup, base));
            if (result != NULL)
                return result;
          }

        follow = follow->next_non_null;
      }

    return base;
  }

static unbound_use *add_aliased_unbound_operator_expression(
        unbound_name_manager *manager, const char *name,
        expression *this_expression, parser *the_parser,
        const source_location *location)
  {
    return add_unbound_operator_expression(manager,
            try_aliasing(name, the_parser), this_expression, location);
  }

static verdict resolve_statement_block(statement_block *the_statement_block,
        unbound_name_manager *the_unbound_name_manager,
        alias_manager *parent_alias_manager)
  {
    size_t name_count;
    size_t name_num;
    size_t use_statement_count;
    size_t use_statement_num;

    name_count = statement_block_name_count(the_statement_block);
    for (name_num = 0; name_num < name_count; ++name_num)
      {
        name_kind kind;
        const char *name_chars;
        const source_location *name_location;
        verdict the_verdict;

        kind = statement_block_name_kind(the_statement_block, name_num);
        switch (kind)
          {
            case NK_VARIABLE:
              {
                variable_declaration *declaration;

                declaration = statement_block_name_variable_declaration(
                        the_statement_block, name_num);
                name_chars = variable_declaration_name(declaration);
                name_location = get_declaration_location(
                        variable_declaration_declaration(declaration));

                if (name_chars != NULL)
                  {
                    the_verdict = bind_variable_name(the_unbound_name_manager,
                                                     name_chars, declaration);
                  }
                else
                  {
                    the_verdict = MISSION_ACCOMPLISHED;
                  }

                break;
              }
            case NK_ROUTINE:
              {
                routine_declaration *declaration;

                declaration = statement_block_name_routine_declaration(
                        the_statement_block, name_num);
                name_chars = routine_declaration_name(declaration);
                name_location = get_declaration_location(
                        routine_declaration_declaration(declaration));

                if (name_chars != NULL)
                  {
                    routine_declaration_chain *declaration_chain;
                    routine_declaration_chain *static_head;
                    routine_declaration_chain *static_tail;
                    routine_declaration *the_routine_declaration;
                    unbound_use *use;

                    declaration_chain =
                            routine_declaration_declaration_chain(declaration);
                    assert(declaration_chain != NULL);

                    the_verdict = bind_routine_name(the_unbound_name_manager,
                            name_chars, declaration_chain);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                        break;

                    static_head = NULL;
                    static_tail = NULL;
                    while (TRUE)
                      {
                        routine_declaration *declaration;
                        routine_declaration_chain *next;

                        declaration = routine_declaration_chain_declaration(
                                declaration_chain);
                        assert(declaration != NULL);

                        if (routine_declaration_is_static(declaration))
                          {
                            routine_declaration_chain *new_chain;

                            new_chain = create_routine_declaration_chain(
                                    declaration, NULL);
                            if (new_chain == NULL)
                              {
                                if (static_head != NULL)
                                  {
                                    routine_declaration_chain_remove_reference(
                                            static_head);
                                  }
                                return MISSION_FAILED;
                              }

                            if (static_head == NULL)
                              {
                                assert(static_tail == NULL);
                                static_head = new_chain;
                              }
                            else
                              {
                                assert(static_tail != NULL);
                                routine_declaration_chain_set_next(static_tail,
                                                                   new_chain);
                                routine_declaration_chain_remove_reference(
                                        new_chain);
                              }
                            static_tail = new_chain;
                          }

                        next = routine_declaration_chain_next(
                                declaration_chain);
                        if (next == NULL)
                            break;
                        declaration_chain = next;
                      }
                    assert(declaration_chain != NULL);

                    the_routine_declaration =
                            routine_declaration_chain_declaration(
                                    declaration_chain);
                    assert(the_routine_declaration != NULL);

                    use = add_dangling_overloaded_routine_reference(
                            the_unbound_name_manager, name_chars,
                            declaration_chain,
                            get_declaration_location(
                                    routine_declaration_declaration(
                                            the_routine_declaration)));
                    if (use == NULL)
                      {
                        the_verdict = MISSION_FAILED;
                        break;
                      }

                    if (static_head == NULL)
                      {
                        assert(static_tail == NULL);
                        break;
                      }

                    the_verdict = bind_static_routine_name(
                            the_unbound_name_manager, name_chars, static_head);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                        break;

                    if (routine_declaration_chain_has_more_than_one_reference(
                                static_head))
                      {
                        routine_declaration *tail_declaration;
                        unbound_use *use;

                        tail_declaration =
                                routine_declaration_chain_declaration(
                                        static_tail);
                        use = add_dangling_overloaded_routine_reference(
                                the_unbound_name_manager, name_chars,
                                static_tail,
                                get_declaration_location(
                                        routine_declaration_declaration(
                                                tail_declaration)));
                        if (use == NULL)
                          {
                            the_verdict = MISSION_FAILED;
                            break;
                          }
                      }

                    routine_declaration_chain_remove_reference(static_head);
                  }
                else
                  {
                    the_verdict = MISSION_ACCOMPLISHED;
                  }

                break;
              }
            case NK_TAGALONG:
              {
                tagalong_declaration *declaration;

                declaration = statement_block_name_tagalong_declaration(
                        the_statement_block, name_num);
                name_chars = tagalong_declaration_name(declaration);
                name_location = get_declaration_location(
                        tagalong_declaration_declaration(declaration));

                if (name_chars != NULL)
                  {
                    the_verdict = bind_tagalong_name(the_unbound_name_manager,
                                                     name_chars, declaration);
                  }
                else
                  {
                    the_verdict = MISSION_ACCOMPLISHED;
                  }

                break;
              }
            case NK_LEPTON_KEY:
              {
                lepton_key_declaration *declaration;

                declaration = statement_block_name_lepton_key_declaration(
                        the_statement_block, name_num);
                name_chars = lepton_key_declaration_name(declaration);
                name_location = get_declaration_location(
                        lepton_key_declaration_declaration(declaration));

                if (name_chars != NULL)
                  {
                    the_verdict = bind_lepton_key_name(
                            the_unbound_name_manager, name_chars, declaration);
                  }
                else
                  {
                    the_verdict = MISSION_ACCOMPLISHED;
                  }

                break;
              }
            case NK_QUARK:
              {
                quark_declaration *declaration;

                declaration = statement_block_name_quark_declaration(
                        the_statement_block, name_num);
                name_chars = quark_declaration_name(declaration);
                name_location = get_declaration_location(
                        quark_declaration_declaration(declaration));

                if (name_chars != NULL)
                  {
                    the_verdict = bind_quark_name(the_unbound_name_manager,
                                                  name_chars, declaration);
                  }
                else
                  {
                    the_verdict = MISSION_ACCOMPLISHED;
                  }

                break;
              }
            case NK_LOCK:
              {
                lock_declaration *declaration;

                declaration = statement_block_name_lock_declaration(
                        the_statement_block, name_num);
                name_chars = lock_declaration_name(declaration);
                name_location = get_declaration_location(
                        lock_declaration_declaration(declaration));

                if (name_chars != NULL)
                  {
                    the_verdict = bind_lock_name(the_unbound_name_manager,
                                                 name_chars, declaration);
                  }
                else
                  {
                    the_verdict = MISSION_ACCOMPLISHED;
                  }

                break;
              }
            case NK_JUMP_TARGET:
              {
                statement *declaration;

                declaration = statement_block_name_jump_target_declaration(
                        the_statement_block, name_num);
                name_chars = label_statement_name(declaration);
                assert(name_chars != NULL);
                name_location = get_statement_location(declaration);

                the_verdict = bind_label_name(the_unbound_name_manager,
                                              name_chars, declaration);

                break;
              }
            default:
              {
                assert(FALSE);
                the_verdict = MISSION_FAILED;
              }
          }

        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        if (name_chars != NULL)
          {
            const char *alias;

            alias = try_aliasing_from_alias_manager(name_chars,
                                                    parent_alias_manager);

            if (strcmp(alias, name_chars) != 0)
              {
                location_warning(name_location,
                        "`%s' was used as the name in a declaration, but that "
                        "name is aliased to `%s', so referencing this "
                        "declaration by this name will not work.", name_chars,
                        alias);
              }
          }
      }

    use_statement_count =
            statement_block_use_statement_count(the_statement_block);
    for (use_statement_num = 0; use_statement_num < use_statement_count;
         ++use_statement_num)
      {
        statement *use_statement;
        size_t for_item_count;
        size_t used_for_item_count;
        size_t used_for_item_num;

        use_statement = statement_block_use_statement(the_statement_block,
                use_statement_count - (use_statement_num + 1));
        assert(use_statement != NULL);
        assert(get_statement_kind(use_statement) == SK_USE);

        for_item_count = use_statement_for_item_count(use_statement);

        if (for_item_count == 0)
          {
            size_t name_count;
            size_t name_num;

            name_count = unbound_name_count(the_unbound_name_manager);
            for (name_num = 0; name_num < name_count; ++name_num)
              {
                unbound_name *the_unbound_name;
                const char *name_string;
                verdict the_verdict;

                the_unbound_name = unbound_name_number(
                        the_unbound_name_manager,
                        (name_count - (name_num + 1)));
                assert(the_unbound_name != NULL);

                name_string = unbound_name_string(the_unbound_name);
                assert(name_string != NULL);

                if (use_statement_is_exception(use_statement, name_string))
                    continue;

                the_verdict = handle_use_statement_for_unbound_name(
                        use_statement, name_string, the_unbound_name,
                        the_unbound_name_manager, TRUE);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return the_verdict;
              }
          }
        else
          {
            size_t for_item_num;

            for (for_item_num = 0; for_item_num < for_item_count;
                 ++for_item_num)
              {
                const char *to_export;
                const char *exported_as;
                unbound_name *the_unbound_name;

                to_export = use_statement_for_item_to_export(use_statement,
                                                             for_item_num);
                assert(to_export != NULL);

                exported_as = use_statement_for_item_exported_as(use_statement,
                                                                 for_item_num);
                assert(exported_as != NULL);

                the_unbound_name = lookup_unbound_name(
                        the_unbound_name_manager, exported_as);
                if (the_unbound_name != NULL)
                  {
                    verdict the_verdict;

                    the_verdict = handle_use_statement_for_unbound_name(
                            use_statement, to_export, the_unbound_name,
                            the_unbound_name_manager, FALSE);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                        return the_verdict;
                  }
              }
          }

        used_for_item_count = use_statement_used_for_count(use_statement);
        for (used_for_item_num = 0; used_for_item_num < used_for_item_count;
             ++used_for_item_num)
          {
            unbound_use *use;

            if (!(use_statement_used_for_flow_through_allowed(use_statement,
                          used_for_item_num)))
              {
                continue;
              }

            use = add_use_flow_through_reference(the_unbound_name_manager,
                    use_statement_used_for_name(use_statement,
                                                used_for_item_num),
                    use_statement, used_for_item_num,
                    get_statement_location(use_statement));
            if (use == NULL)
                return MISSION_FAILED;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

static open_expression *parse_range_expression_tail(parser *the_parser,
        boolean open_is_inclusive, open_expression *open_lower,
        const source_location *start_location)
  {
    open_expression *open_upper;
    source_location local_end_location;
    token_kind close_kind;
    verdict the_verdict;
    unbound_name_manager *manager;
    expression *lower;
    unbound_name_manager *child_manager;
    expression *upper;
    expression *call_base;
    unbound_use *use;
    value *lower_is_inclusive_value;
    value *upper_is_inclusive_value;
    expression *lower_is_inclusive_expression;
    expression *upper_is_inclusive_expression;
    expression *actual_argument_expressions[4];
    const char *formal_argument_names[4] =
      { "lower_bound", "upper_bound", "lower_is_inclusive",
        "upper_is_inclusive" };
    call *the_call;
    expression *result;

    open_upper = parse_expression(the_parser, EPP_TOP, NULL, FALSE);
    if (open_upper == NULL)
      {
        delete_open_expression(open_lower);
        return NULL;
      }

    local_end_location = *(next_token_location(the_parser->tokenizer));

    the_verdict = expect_and_eat2(the_parser->tokenizer, TK_RIGHT_PAREN,
                                  TK_RIGHT_BRACKET, &close_kind);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_open_expression(open_upper);
        delete_open_expression(open_lower);
        return NULL;
      }

    decompose_open_expression(open_lower, &manager, &lower);
    decompose_open_expression(open_upper, &child_manager, &upper);

    the_verdict = merge_in_unbound_name_manager(manager, child_manager);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        delete_expression(upper);
        delete_expression(lower);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    call_base = create_unbound_name_reference_expression();
    if (call_base == NULL)
      {
        delete_expression(upper);
        delete_expression(lower);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    use = add_unbound_variable_reference(manager, "integer_range", call_base,
                                         start_location);
    if (use == NULL)
      {
        delete_expression(call_base);
        delete_expression(upper);
        delete_expression(lower);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    if (open_is_inclusive)
        lower_is_inclusive_value = create_true_value();
    else
        lower_is_inclusive_value = create_false_value();
    if (lower_is_inclusive_value == NULL)
      {
        delete_expression(call_base);
        delete_expression(upper);
        delete_expression(lower);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    lower_is_inclusive_expression =
            create_constant_expression(lower_is_inclusive_value);
    value_remove_reference(lower_is_inclusive_value, NULL);
    if (lower_is_inclusive_expression == NULL)
      {
        delete_expression(call_base);
        delete_expression(upper);
        delete_expression(lower);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    if (close_kind == TK_RIGHT_BRACKET)
        upper_is_inclusive_value = create_true_value();
    else
        upper_is_inclusive_value = create_false_value();
    if (upper_is_inclusive_value == NULL)
      {
        delete_expression(lower_is_inclusive_expression);
        delete_expression(call_base);
        delete_expression(upper);
        delete_expression(lower);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    upper_is_inclusive_expression =
            create_constant_expression(upper_is_inclusive_value);
    value_remove_reference(upper_is_inclusive_value, NULL);
    if (upper_is_inclusive_expression == NULL)
      {
        delete_expression(lower_is_inclusive_expression);
        delete_expression(call_base);
        delete_expression(upper);
        delete_expression(lower);
        delete_unbound_name_manager(manager);
        return NULL;
      }

    actual_argument_expressions[0] = lower;
    actual_argument_expressions[1] = upper;
    actual_argument_expressions[2] = lower_is_inclusive_expression;
    actual_argument_expressions[3] = upper_is_inclusive_expression;

    the_call = create_call(call_base, 4, actual_argument_expressions,
                           formal_argument_names, start_location);
    if (the_call == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    result = create_call_expression(the_call);
    if (result == NULL)
      {
        delete_unbound_name_manager(manager);
        return NULL;
      }

    set_expression_end_location(result, &local_end_location);

    return create_open_expression(result, manager);
  }
