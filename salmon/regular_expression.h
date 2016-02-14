/* file "regular_expression.h" */

/*
 *  This file contains the interface to the regular_expression module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef REGULAR_EXPRESSION_H
#define REGULAR_EXPRESSION_H

#include "c_foundations/basic.h"


typedef enum
  {
    REE_MEMORY_ALLOCATION,
    REE_MISMATCHED_RIGHT_PAREN,
    REE_MISMATCHED_LEFT_PAREN,
    REE_EMPTY_STARRED,
    REE_EMPTY_PLUSSED,
    REE_EMPTY_OPTIONAL,
    REE_EMPTY_BRACED,
    REE_UNTERMINATED_BRACKET,
    REE_HEX_NO_DIGITS,
    REE_HEX_ZERO,
    REE_HEX_UTF_16_SURROGATE,
    REE_HEX_TOO_LARGE,
    REE_BAD_ESCAPE_LETTER,
    REE_RANGE_START_IS_RANGE,
    REE_RANGE_END_IS_RANGE,
    REE_MISSING_BRACE_COMMA,
    REE_MISSING_BRACE_CLOSE,
    REE_BRACE_LOWER_GREATER_THAN_UPPER
  } regular_expression_error;

typedef struct regular_expression regular_expression;
typedef struct re_follower re_follower;


extern regular_expression *create_regular_expression_from_pattern(
        const char *pattern_chars, size_t pattern_length,
        regular_expression_error *error);
extern regular_expression *create_exact_string_regular_expression(
        const char *match_string);
extern regular_expression *create_character_set_regular_expression(
        const char **characters, size_t character_count);
extern regular_expression *create_character_range_regular_expression(
        const char *lower, const char *upper);
extern regular_expression *create_concatenation_regular_expression(
        regular_expression *left, regular_expression *right);
extern regular_expression *create_or_regular_expression(
        regular_expression *left, regular_expression *right);
extern regular_expression *create_repeat_zero_or_more_regular_expression(
        regular_expression *base);
extern regular_expression *create_repeat_one_or_more_regular_expression(
        regular_expression *base);

extern boolean matches(regular_expression *the_regular_expression,
        const char *test_string, boolean is_start, boolean *error);
extern boolean longest_match(regular_expression *the_regular_expression,
        const char *test_string, boolean is_start, size_t *match_start,
        size_t *match_length, boolean *error);

extern const char *regular_expression_pattern(
        regular_expression *the_regular_expression);

extern re_follower *regular_expression_create_follower(
        regular_expression *the_regular_expression);

extern verdict re_follower_transit(re_follower *follower,
                                   const char *character);
extern verdict re_follower_end_transit(re_follower *follower);

extern boolean re_follower_is_in_accepting_state(re_follower *follower);
extern boolean re_follower_more_possible(re_follower *follower);

extern void delete_re_follower(re_follower *follower);

extern void regular_expression_add_reference(
        regular_expression *the_regular_expression);
extern void regular_expression_remove_reference(
        regular_expression *the_regular_expression);

extern int regular_expression_structural_order(
        regular_expression *regular_expression1,
        regular_expression *regular_expression2);

extern const char *string_for_regular_expression_error(
        regular_expression_error the_error);


#endif /* REGULAR_EXPRESSION_H */
