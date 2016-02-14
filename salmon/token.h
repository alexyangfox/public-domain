/* file "token.h" */

/*
 *  This file contains the interface to the token module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef TOKEN_H
#define TOKEN_H


typedef enum token_kind
  {
    TK_IDENTIFIER,
    TK_STRING_LITERAL,
    TK_CHARACTER_LITERAL,
    TK_DECIMAL_INTEGER_LITERAL,
    TK_HEXADECIMAL_INTEGER_LITERAL,
    TK_SCIENTIFIC_NOTATION_LITERAL,
    TK_REGULAR_EXPRESSION_LITERAL,
    TK_BACKTICK_EXPRESSION_LITERAL,
    TK_LEFT_PAREN,            /* "(" */
    TK_RIGHT_PAREN,           /* ")" */
    TK_LEFT_BRACKET,          /* "[" */
    TK_RIGHT_BRACKET,         /* "]" */
    TK_LEFT_CURLY_BRACE,      /* "{" */
    TK_RIGHT_CURLY_BRACE,     /* "}" */
    TK_COLON,                 /* ":" */
    TK_SEMICOLON,             /* ";" */
    TK_COMMA,                 /* "," */
    TK_DOT,                   /* "." */
    TK_DOT_DOT,               /* ".." */
    TK_DOT_DOT_DOT,           /* "..." */
    TK_DOT_DOT_DOT_DOT,       /* "...." */
    TK_ASSIGN,                /* ":=" */
    TK_MODULO_ASSIGN,         /* "::=" */
    TK_MULTIPLY_ASSIGN,       /* "*=" */
    TK_DIVIDE_ASSIGN,         /* "/=" */
    TK_DIVIDE_FORCE_ASSIGN,   /* "/::=" */
    TK_REMAINDER_ASSIGN,      /* "%=" */
    TK_ADD_ASSIGN,            /* "+=" */
    TK_SUBTRACT_ASSIGN,       /* "-=" */
    TK_SHIFT_LEFT_ASSIGN,     /* "<<=" */
    TK_SHIFT_RIGHT_ASSIGN,    /* ">>=" */
    TK_BITWISE_AND_ASSIGN,    /* "&=" */
    TK_BITWISE_XOR_ASSIGN,    /* "^=" */
    TK_BITWISE_OR_ASSIGN,     /* "|=" */
    TK_LOGICAL_AND_ASSIGN,    /* "&&=" */
    TK_LOGICAL_OR_ASSIGN,     /* "||=" */
    TK_CONCATENATE_ASSIGN,    /* "~=" */
    TK_PLUS_PLUS,             /* "++" */
    TK_MINUS_MINUS,           /* "--" */
    TK_STAR,                  /* "*" */
    TK_DIVIDE,                /* "/" */
    TK_DIVIDE_FORCE,          /* "/::" */
    TK_REMAINDER,             /* "%" */
    TK_ADD,                   /* "+" */
    TK_DASH,                  /* "-" */
    TK_SHIFT_LEFT,            /* "<<" */
    TK_SHIFT_RIGHT,           /* ">>" */
    TK_AMPERSAND,             /* "&" */
    TK_BITWISE_XOR,           /* "^" */
    TK_BITWISE_OR,            /* "|" */
    TK_LOGICAL_AND,           /* "&&" */
    TK_LOGICAL_OR,            /* "||" */
    TK_EQUAL,                 /* "==" */
    TK_NOT_EQUAL,             /* "!=" */
    TK_LESS_THAN,             /* "<" */
    TK_GREATER_THAN,          /* ">" */
    TK_LESS_THAN_OR_EQUAL,    /* "<=" */
    TK_GREATER_THAN_OR_EQUAL, /* ">=" */
    TK_LOGICAL_NOT,           /* "!" */
    TK_QUESTION_MARK,         /* "?" */
    TK_POINTS_TO,             /* "->" */
    TK_BITWISE_NOT,           /* "~" */
    TK_RETURNS,               /* "<--" */
    TK_MAPS_TO,               /* "-->" */
    TK_FORCE,                 /* "::" */
    TK_END_OF_INPUT,
    TK_ERROR
  } token_kind;

typedef struct token token;


#include "source_location.h"
#include "o_integer.h"
#include "rational.h"
#include "regular_expression.h"


extern token *create_identifier_token(const char *name);
extern token *create_string_literal_token(const char *data);
extern token *create_character_literal_token(const char *data);
extern token *create_decimal_integer_literal_token(o_integer oi);
extern token *create_hexadecimal_integer_literal_token(o_integer oi);
extern token *create_scientific_notation_literal_token(rational *the_rational);
extern token *create_regular_expression_literal_token(
        regular_expression *the_regular_expression);
extern token *create_backtick_expression_literal_token(const char *data);
extern token *create_left_paren_token(void);
extern token *create_right_paren_token(void);
extern token *create_left_bracket_token(void);
extern token *create_right_bracket_token(void);
extern token *create_left_curly_brace_token(void);
extern token *create_right_curly_brace_token(void);
extern token *create_colon_token(void);
extern token *create_semicolon_token(void);
extern token *create_comma_token(void);
extern token *create_dot_token(void);
extern token *create_dot_dot_token(void);
extern token *create_dot_dot_dot_token(void);
extern token *create_dot_dot_dot_dot_token(void);
extern token *create_assign_token(void);
extern token *create_modulo_assign_token(void);
extern token *create_multiply_assign_token(void);
extern token *create_divide_assign_token(void);
extern token *create_divide_force_assign_token(void);
extern token *create_remainder_assign_token(void);
extern token *create_add_assign_token(void);
extern token *create_subtract_assign_token(void);
extern token *create_shift_left_assign_token(void);
extern token *create_shift_right_assign_token(void);
extern token *create_bitwise_and_assign_token(void);
extern token *create_bitwise_xor_assign_token(void);
extern token *create_bitwise_or_assign_token(void);
extern token *create_logical_and_assign_token(void);
extern token *create_logical_or_assign_token(void);
extern token *create_concatenate_assign_token(void);
extern token *create_plus_plus_token(void);
extern token *create_minus_minus_token(void);
extern token *create_star_token(void);
extern token *create_divide_token(void);
extern token *create_divide_force_token(void);
extern token *create_remainder_token(void);
extern token *create_add_token(void);
extern token *create_dash_token(void);
extern token *create_shift_left_token(void);
extern token *create_shift_right_token(void);
extern token *create_ampersand_token(void);
extern token *create_bitwise_xor_token(void);
extern token *create_bitwise_or_token(void);
extern token *create_logical_and_token(void);
extern token *create_logical_or_token(void);
extern token *create_equal_token(void);
extern token *create_not_equal_token(void);
extern token *create_less_than_token(void);
extern token *create_greater_than_token(void);
extern token *create_less_than_or_equal_token(void);
extern token *create_greater_than_or_equal_token(void);
extern token *create_logical_not_token(void);
extern token *create_question_mark_token(void);
extern token *create_points_to_token(void);
extern token *create_bitwise_not_token(void);
extern token *create_returns_token(void);
extern token *create_maps_to_token(void);
extern token *create_force_token(void);
extern token *create_end_of_input_token(void);
extern token *create_error_token(void);

extern void delete_token(token *the_token);

extern const token_kind get_token_kind(token *the_token);
extern const char *identifier_token_name(token *the_token);
extern const char *string_literal_token_data(token *the_token);
extern const char *character_literal_token_data(token *the_token);
extern o_integer integer_literal_token_integer(token *the_token);
extern rational *scientific_notation_literal_token_rational(token *the_token);
extern regular_expression *regular_expression_literal_token_data(
        token *the_token);
extern const char *backtick_expression_literal_token_data(token *the_token);

extern void set_token_location(token *the_token, source_location *location);

extern const source_location *get_token_location(token *the_token);

extern void token_error(token *the_token, const char *format, ...);


#endif /* TOKEN_H */
