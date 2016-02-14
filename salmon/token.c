/* file "token.c" */

/*
 *  This file contains the implementation of the token module.
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
#include "c_foundations/memory_allocation.h"
#include "c_foundations/diagnostic.h"
#include "token.h"
#include "source_location.h"
#include "o_integer.h"
#include "rational.h"
#include "regular_expression.h"


struct token
  {
    token_kind kind;
    union
      {
        char *string_data;
        char character_data[5];
        o_integer oi;
        rational *rational;
        regular_expression *regular_expression;
      } u;
    source_location location;
  };


extern token *create_identifier_token(const char *name)
  {
    char *copy;
    token *result;

    assert(name != NULL);

    copy = MALLOC_ARRAY(char, strlen(name) + 1);
    if (copy == NULL)
        return NULL;

    strcpy(copy, name);

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
      {
        free(copy);
        return NULL;
      }

    result->kind = TK_IDENTIFIER;
    result->u.string_data = copy;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_string_literal_token(const char *data)
  {
    char *copy;
    token *result;

    assert(data != NULL);

    copy = MALLOC_ARRAY(char, strlen(data) + 1);
    if (copy == NULL)
        return NULL;

    strcpy(copy, data);

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
      {
        free(copy);
        return NULL;
      }

    result->kind = TK_STRING_LITERAL;
    result->u.string_data = copy;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_character_literal_token(const char *data)
  {
    token *result;
    unsigned char bits;

    assert(data != NULL);

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_CHARACTER_LITERAL;

    bits = data[0];
    assert(bits != 0);
    assert((bits & 0xc0) != 0x80);
    if ((bits & 0x80) == 0)
      {
        result->u.character_data[0] = data[0];
        result->u.character_data[1] = 0;
      }
    else if ((bits & 0xe0) == 0xc0)
      {
        bits = data[1];
        assert((bits & 0xc0) == 0x80);

        result->u.character_data[0] = data[0];
        result->u.character_data[1] = data[1];
        result->u.character_data[2] = 0;
      }
    else if ((bits & 0xf0) == 0xe0)
      {
        bits = data[1];
        assert((bits & 0xc0) == 0x80);
        bits = data[2];
        assert((bits & 0xc0) == 0x80);

        result->u.character_data[0] = data[0];
        result->u.character_data[1] = data[1];
        result->u.character_data[2] = data[2];
        result->u.character_data[3] = 0;
      }
    else if ((bits & 0xf8) == 0xf0)
      {
        bits = data[1];
        assert((bits & 0xc0) == 0x80);
        bits = data[2];
        assert((bits & 0xc0) == 0x80);
        bits = data[3];
        assert((bits & 0xc0) == 0x80);

        result->u.character_data[0] = data[0];
        result->u.character_data[1] = data[1];
        result->u.character_data[2] = data[2];
        result->u.character_data[3] = data[3];
        result->u.character_data[4] = 0;
      }
    else
      {
        assert(FALSE);
        result->u.character_data[0] = 0;
      }

    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_decimal_integer_literal_token(o_integer oi)
  {
    token *result;

    assert(!(oi_out_of_memory(oi)));

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_DECIMAL_INTEGER_LITERAL;

    result->u.oi = oi;
    oi_add_reference(oi);

    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_hexadecimal_integer_literal_token(o_integer oi)
  {
    token *result;

    assert(!(oi_out_of_memory(oi)));

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_HEXADECIMAL_INTEGER_LITERAL;

    result->u.oi = oi;
    oi_add_reference(oi);

    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_scientific_notation_literal_token(rational *the_rational)
  {
    token *result;

    assert(the_rational != NULL);

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_SCIENTIFIC_NOTATION_LITERAL;

    result->u.rational = the_rational;
    rational_add_reference(the_rational);

    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_regular_expression_literal_token(
        regular_expression *the_regular_expression)
  {
    token *result;

    assert(the_regular_expression != NULL);

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_REGULAR_EXPRESSION_LITERAL;

    result->u.regular_expression = the_regular_expression;
    regular_expression_add_reference(the_regular_expression);

    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_backtick_expression_literal_token(const char *data)
  {
    char *copy;
    token *result;

    assert(data != NULL);

    copy = MALLOC_ARRAY(char, strlen(data) + 1);
    if (copy == NULL)
        return NULL;

    strcpy(copy, data);

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
      {
        free(copy);
        return NULL;
      }

    result->kind = TK_BACKTICK_EXPRESSION_LITERAL;
    result->u.string_data = copy;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_left_paren_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_LEFT_PAREN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_right_paren_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_RIGHT_PAREN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_left_bracket_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_LEFT_BRACKET;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_right_bracket_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_RIGHT_BRACKET;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_left_curly_brace_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_LEFT_CURLY_BRACE;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_right_curly_brace_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_RIGHT_CURLY_BRACE;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_colon_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_COLON;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_semicolon_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_SEMICOLON;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_comma_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_COMMA;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_dot_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_DOT;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_dot_dot_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_DOT_DOT;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_dot_dot_dot_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_DOT_DOT_DOT;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_dot_dot_dot_dot_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_DOT_DOT_DOT_DOT;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_modulo_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_MODULO_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_multiply_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_MULTIPLY_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_divide_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_DIVIDE_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_divide_force_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_DIVIDE_FORCE_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_remainder_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_REMAINDER_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_add_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_ADD_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_subtract_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_SUBTRACT_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_shift_left_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_SHIFT_LEFT_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_shift_right_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_SHIFT_RIGHT_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_bitwise_and_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_BITWISE_AND_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_bitwise_xor_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_BITWISE_XOR_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_bitwise_or_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_BITWISE_OR_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_logical_and_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_LOGICAL_AND_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_logical_or_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_LOGICAL_OR_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_concatenate_assign_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_CONCATENATE_ASSIGN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_plus_plus_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_PLUS_PLUS;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_minus_minus_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_MINUS_MINUS;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_star_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_STAR;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_divide_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_DIVIDE;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_divide_force_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_DIVIDE_FORCE;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_remainder_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_REMAINDER;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_add_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_ADD;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_dash_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_DASH;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_shift_left_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_SHIFT_LEFT;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_shift_right_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_SHIFT_RIGHT;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_ampersand_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_AMPERSAND;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_bitwise_xor_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_BITWISE_XOR;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_bitwise_or_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_BITWISE_OR;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_logical_and_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_LOGICAL_AND;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_logical_or_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_LOGICAL_OR;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_equal_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_EQUAL;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_not_equal_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_NOT_EQUAL;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_less_than_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_LESS_THAN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_greater_than_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_GREATER_THAN;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_less_than_or_equal_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_LESS_THAN_OR_EQUAL;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_greater_than_or_equal_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_GREATER_THAN_OR_EQUAL;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_logical_not_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_LOGICAL_NOT;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_question_mark_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_QUESTION_MARK;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_points_to_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_POINTS_TO;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_bitwise_not_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_BITWISE_NOT;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_returns_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_RETURNS;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_maps_to_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_MAPS_TO;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_force_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_FORCE;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_end_of_input_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_END_OF_INPUT;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern token *create_error_token(void)
  {
    token *result;

    result = MALLOC_ONE_OBJECT(token);
    if (result == NULL)
        return NULL;

    result->kind = TK_ERROR;
    set_source_location(&(result->location), NULL);

    return result;
  }

extern void delete_token(token *the_token)
  {
    assert(the_token != NULL);

    switch (the_token->kind)
      {
        case TK_IDENTIFIER:
        case TK_STRING_LITERAL:
        case TK_BACKTICK_EXPRESSION_LITERAL:
            assert(the_token->u.string_data != NULL);
            free(the_token->u.string_data);
            break;
        case TK_DECIMAL_INTEGER_LITERAL:
        case TK_HEXADECIMAL_INTEGER_LITERAL:
            assert(!(oi_out_of_memory(the_token->u.oi)));
            oi_remove_reference(the_token->u.oi);
            break;
        case TK_SCIENTIFIC_NOTATION_LITERAL:
            rational_remove_reference(the_token->u.rational);
            break;
        case TK_REGULAR_EXPRESSION_LITERAL:
            regular_expression_remove_reference(
                    the_token->u.regular_expression);
            break;
        default:
            break;
      }

    source_location_remove_reference(&(the_token->location));

    free(the_token);
  }

extern const token_kind get_token_kind(token *the_token)
  {
    assert(the_token != NULL);

    return the_token->kind;
  }

extern const char *identifier_token_name(token *the_token)
  {
    assert(the_token != NULL);
    assert(the_token->kind == TK_IDENTIFIER);

    return the_token->u.string_data;
  }

extern const char *string_literal_token_data(token *the_token)
  {
    assert(the_token != NULL);
    assert(the_token->kind == TK_STRING_LITERAL);

    return the_token->u.string_data;
  }

extern const char *character_literal_token_data(token *the_token)
  {
    assert(the_token != NULL);
    assert(the_token->kind == TK_CHARACTER_LITERAL);

    return &(the_token->u.character_data[0]);
  }

extern o_integer integer_literal_token_integer(token *the_token)
  {
    assert(the_token != NULL);
    assert((the_token->kind == TK_DECIMAL_INTEGER_LITERAL) ||
           (the_token->kind == TK_HEXADECIMAL_INTEGER_LITERAL));

    assert(!(oi_out_of_memory(the_token->u.oi)));
    return the_token->u.oi;
  }

extern rational *scientific_notation_literal_token_rational(token *the_token)
  {
    assert(the_token != NULL);
    assert(the_token->kind == TK_SCIENTIFIC_NOTATION_LITERAL);

    assert(the_token->u.rational != NULL);
    return the_token->u.rational;
  }

extern regular_expression *regular_expression_literal_token_data(
        token *the_token)
  {
    assert(the_token != NULL);
    assert(the_token->kind == TK_REGULAR_EXPRESSION_LITERAL);

    assert(the_token->u.regular_expression != NULL);
    return the_token->u.regular_expression;
  }

extern const char *backtick_expression_literal_token_data(token *the_token)
  {
    assert(the_token != NULL);
    assert(the_token->kind == TK_BACKTICK_EXPRESSION_LITERAL);

    return the_token->u.string_data;
  }

extern void set_token_location(token *the_token, source_location *location)
  {
    assert(the_token != NULL);

    set_source_location(&(the_token->location), location);
  }

extern const source_location *get_token_location(token *the_token)
  {
    assert(the_token != NULL);

    return &(the_token->location);
  }

extern void token_error(token *the_token, const char *format, ...)
  {
    va_list ap;

    va_start(ap, format);
    vlocation_error(((the_token == NULL) ? NULL : &(the_token->location)),
                    format, ap);
    va_end(ap);
  }
