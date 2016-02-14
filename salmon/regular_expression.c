/* file "regular_expression.c" */

/*
 *  This file contains the implementation of the regular_expression module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */

/*
 *  Note that some of the algorithms and data structures in this file are based
 *  on ideas from Chapter 3 of Aho, Sethi and Ullman (the Dragon Book).
 */


#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/auto_array.h"
#include "c_foundations/auto_array_implementation.h"
#include "regular_expression.h"
#include "unicode.h"
#include "platform_dependent.h"


typedef struct
  {
    unsigned char link_first_char[2];
    unsigned char link_last_char[2];
    size_t next[2];
  } nfa_state;

AUTO_ARRAY(nfa_state_aa, nfa_state);
AUTO_ARRAY_IMPLEMENTATION(nfa_state_aa, nfa_state, 0);

typedef struct
  {
    size_t state_count;
    size_t start_state;
    size_t end_state;
    nfa_state *state_table;
  } nfa;

#define DFA_CACHE_MAX_STATES 200

typedef struct
  {
    size_t nfa_state_num;
    size_t next_element;
  } dfa_state_element;

AUTO_ARRAY(dfa_state_element_aa, dfa_state_element);
AUTO_ARRAY_IMPLEMENTATION(dfa_state_element_aa, dfa_state_element, 0);

typedef struct dfa_state_successor dfa_state_successor;

struct dfa_state_successor
  {
    size_t successor_num;
    size_t next_for_successor;
    size_t previous_for_successor;
  };

typedef struct
  {
    size_t first_element_num;
    size_t *elements_bit_vector;
    dfa_state_successor successors[2][256];
    boolean contains_end;
    size_t first_predecessor_incoming_link;
    size_t next_most_recently_used_dfa_state;
    size_t next_least_recently_used_dfa_state;
  } dfa_state;

AUTO_ARRAY(dfa_state_aa, dfa_state);
AUTO_ARRAY_IMPLEMENTATION(dfa_state_aa, dfa_state, 0);

typedef struct
  {
    dfa_state_element_aa element_pool;
    size_t first_free_element;
    dfa_state_aa state_pool;
    size_t used_dfa_state_count;
    size_t most_recently_used_dfa_state;
    size_t least_recently_used_dfa_state;
    nfa *nfa;
  } dfa_cache;

struct regular_expression
  {
    char *pattern;
    nfa nfa;
    DECLARE_SYSTEM_LOCK(cache_lock);
    dfa_cache dfa_cache;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
  };

struct re_follower
  {
    regular_expression *regular_expression;
    dfa_cache dfa_cache;
    boolean beyond_possible_match;
    size_t current_dfa_state_num;
    DECLARE_SYSTEM_LOCK(lock);
  };

typedef struct
  {
    unsigned long first;
    unsigned long last;
  } code_point_range;

AUTO_ARRAY(code_point_range_aa, code_point_range);
AUTO_ARRAY_IMPLEMENTATION(code_point_range_aa, code_point_range, 0);


static verdict create_nfa_for_pattern(nfa *the_nfa, const char *pattern_chars,
                                      regular_expression_error *error);
static verdict nfa_add_sub_pattern(nfa_state_aa *state_pool,
        const char *pattern_chars, size_t *start_state, size_t *end_state,
        size_t *pattern_chars_used, regular_expression_error *error);
static void deallocate_nfa(nfa *the_nfa);
static verdict init_dfa_cache(dfa_cache *the_dfa_cache, nfa *the_nfa);
static void deallocate_dfa_cache(dfa_cache *the_dfa_cache);
static int compare_ranges(const void *left_data, const void *right_data);
static size_t merge_ranges(code_point_range *range_array, size_t count);
static verdict add_ranges_to_nfa(nfa_state_aa *state_pool,
        size_t original_end_state, const code_point_range *range_array,
        size_t range_count, size_t *new_end_state);
static verdict add_range_to_nfa(nfa_state_aa *state_pool, size_t from_state,
            size_t to_state, const code_point_range *the_range);
static verdict add_link_to_nfa(nfa_state_aa *state_pool, size_t from_state,
        size_t to_state, unsigned char link_first_char,
        unsigned char link_last_char);
static verdict add_multi_byte_character_range_to_nfa(nfa_state_aa *state_pool,
        size_t from_state, size_t to_state, unsigned long first_code_point,
        unsigned long last_code_point, size_t byte_count);
static verdict add_string_range_to_nfa(nfa_state_aa *state_pool,
        size_t from_state, size_t to_state, const char *first_bytes,
        const char *last_bytes, size_t byte_count);
static unsigned long range_character_specification_to_code_point(
        const char *pattern_chars, size_t *bytes_in_character_specification,
        const code_point_range **ranges, size_t *range_count,
        regular_expression_error *error);
static unsigned long hex_character_specification_to_code_point(
        const char *pattern_chars, size_t *bytes_in_character_specification,
        regular_expression_error *error);
static const code_point_range *ranges_for_escaped_letter(char letter,
                                                         size_t *range_count);
static verdict do_repeat_and_skip(nfa_state_aa *state_pool, boolean skip,
        boolean repeat, size_t last_end_state, size_t *current_end_state);
static verdict duplicate_nfa_portion(nfa *duplicate,
        size_t original_state_count, nfa_state *original_states,
        size_t original_starting_point, size_t original_end);
static verdict add_nfa_copy(nfa_state_aa *state_pool, size_t copy_start_state,
                            nfa *to_copy, size_t *copy_end);
static boolean dfa_cache_matches(dfa_cache *the_dfa_cache, nfa *the_nfa,
        const char *target_string, boolean at_start_of_target,
        boolean exact_match, boolean start_limited, size_t start_within,
        boolean *error, size_t *longest_match);
static size_t dfa_cache_find_or_make_successor(dfa_cache *the_dfa_cache,
        size_t current_dfa_state_num, unsigned char next_char,
        boolean omit_start);
static void dfa_cache_mark_state_most_recently_used(dfa_cache *the_dfa_cache,
                                                    size_t state_num);
static size_t get_dfa_state(dfa_cache *the_dfa_cache, dfa_state *predecessor,
        unsigned char new_byte, boolean start_also, unsigned char *range_low,
        unsigned char *range_high);
static void clear_used_dfa_state(dfa_cache *the_dfa_cache,
                                 size_t to_clear_state_num);
static verdict initialize_dfa_state(dfa_cache *the_dfa_cache,
                                    size_t to_initialize_state_num);
static verdict build_new_dfa_state(dfa_cache *the_dfa_cache,
        dfa_state *old_dfa_state, size_t new_dfa_state_num,
        unsigned char new_byte, boolean start_also, unsigned char *range_low,
        unsigned char *range_high);
static verdict add_nfa_successors_to_dfa_state(dfa_cache *the_dfa_cache,
        dfa_state *the_dfa_state, size_t nfa_state_num, unsigned char new_byte,
        size_t *lower_bound, size_t *upper_bound);
static verdict add_nfa_state_and_null_successors_to_dfa_state(
        dfa_cache *the_dfa_cache, dfa_state *the_dfa_state,
        size_t nfa_state_num);
static void clear_and_make_state_lru(dfa_cache *the_dfa_cache,
                                     size_t dfa_state_num);
static void dfa_cache_insert_state_as_most_recently_used(
        dfa_cache *the_dfa_cache, size_t state_num);


extern regular_expression *create_regular_expression_from_pattern(
        const char *pattern_chars, size_t pattern_length,
        regular_expression_error *error)
  {
    regular_expression *result;
    char *pattern_copy;
    verdict the_verdict;

    assert(pattern_chars != NULL);
    assert(error != NULL);

    result = MALLOC_ONE_OBJECT(regular_expression);
    if (result == NULL)
      {
        *error = REE_MEMORY_ALLOCATION;
        return NULL;
      }

    INITIALIZE_SYSTEM_LOCK(result->reference_lock,
            free(result);
            *error = REE_MEMORY_ALLOCATION;
            return NULL);

    INITIALIZE_SYSTEM_LOCK(result->cache_lock,
            DESTROY_SYSTEM_LOCK(result->reference_lock);
            free(result);
            *error = REE_MEMORY_ALLOCATION;
            return NULL);

    pattern_copy = MALLOC_ARRAY(char, pattern_length + 1);
    if (pattern_copy == NULL)
      {
        DESTROY_SYSTEM_LOCK(result->cache_lock);
        DESTROY_SYSTEM_LOCK(result->reference_lock);
        free(result);
        *error = REE_MEMORY_ALLOCATION;
        return NULL;
      }

    memcpy(pattern_copy, pattern_chars, pattern_length);
    pattern_copy[pattern_length] = 0;

    result->pattern = pattern_copy;

    the_verdict = create_nfa_for_pattern(&(result->nfa), pattern_copy, error);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(pattern_copy);
        DESTROY_SYSTEM_LOCK(result->cache_lock);
        DESTROY_SYSTEM_LOCK(result->reference_lock);
        free(result);
        return NULL;
      }

    the_verdict = init_dfa_cache(&(result->dfa_cache), &(result->nfa));
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        deallocate_nfa(&(result->nfa));
        free(pattern_copy);
        DESTROY_SYSTEM_LOCK(result->cache_lock);
        DESTROY_SYSTEM_LOCK(result->reference_lock);
        free(result);
        *error = REE_MEMORY_ALLOCATION;
        return NULL;
      }

    result->reference_count = 1;

    return result;
  }

extern regular_expression *create_exact_string_regular_expression(
        const char *match_string)
  {
    char *buffer;
    const char *follow_source;
    char *follow_target;
    regular_expression_error error;
    regular_expression *result;

    assert(match_string != NULL);

    buffer = MALLOC_ARRAY(char, (strlen(match_string) * 2) + 1);
    if (buffer == NULL)
        return NULL;

    follow_source = match_string;
    follow_target = buffer;

    while (*follow_source != 0)
      {
        switch (*follow_source)
          {
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '.':
            case '*':
            case '+':
            case '\\':
            case '|':
            case '?':
            case '^':
            case '$':
                *follow_target = '\\';
                ++follow_target;
                break;
            default:
                break;
          }

        *follow_target = *follow_source;
        ++follow_source;
        ++follow_target;
      }

    *follow_target = 0;

    result = create_regular_expression_from_pattern(buffer,
            follow_target - buffer, &error);
    free(buffer);
    assert((result != NULL) || (error == REE_MEMORY_ALLOCATION));
    return result;
  }

extern regular_expression *create_character_set_regular_expression(
        const char **characters, size_t character_count)
  {
    char *buffer;
    char *follow_target;
    size_t character_num;
    regular_expression_error error;
    regular_expression *result;

    assert(characters != NULL);

    buffer = MALLOC_ARRAY(char, (character_count * 4) + 3);
    if (buffer == NULL)
        return NULL;

    follow_target = buffer;

    *follow_target = '[';
    ++follow_target;

    for (character_num = 0; character_num < character_count; ++character_num)
      {
        const char *follow_source;

        follow_source = characters[character_num];

        switch (*follow_source)
          {
            case '[':
            case ']':
            case '\\':
            case '^':
            case '-':
                *follow_target = '\\';
                ++follow_target;
                break;
            default:
                break;
          }

        while (*follow_source != 0)
          {
            *follow_target = *follow_source;
            ++follow_source;
            ++follow_target;
          }
      }

    *follow_target = ']';
    ++follow_target;

    *follow_target = 0;

    result = create_regular_expression_from_pattern(buffer,
            follow_target - buffer, &error);
    free(buffer);
    assert((result != NULL) || (error == REE_MEMORY_ALLOCATION));
    return result;
  }

extern regular_expression *create_character_range_regular_expression(
        const char *lower, const char *upper)
  {
    char *buffer;
    char *follow_target;
    size_t character_num;
    regular_expression_error error;
    regular_expression *result;

    assert(lower != NULL);
    assert(upper != NULL);
    assert(strlen(lower) <= 4);
    assert(strlen(upper) <= 4);

    buffer = MALLOC_ARRAY(char, 14);
    if (buffer == NULL)
        return NULL;

    follow_target = buffer;

    *follow_target = '[';
    ++follow_target;

    for (character_num = 0; character_num < 2; ++character_num)
      {
        const char *follow_source;

        if (character_num == 1)
          {
            *follow_target = '-';
            ++follow_target;
          }

        follow_source = ((character_num == 0) ? lower : upper);

        switch (*follow_source)
          {
            case '[':
            case ']':
            case '\\':
            case '^':
            case '-':
                *follow_target = '\\';
                ++follow_target;
                break;
            default:
                break;
          }

        while (*follow_source != 0)
          {
            *follow_target = *follow_source;
            ++follow_source;
            ++follow_target;
          }
      }

    *follow_target = ']';
    ++follow_target;

    *follow_target = 0;

    result = create_regular_expression_from_pattern(buffer,
            follow_target - buffer, &error);
    free(buffer);
    assert((result != NULL) || (error == REE_MEMORY_ALLOCATION));
    return result;
  }

extern regular_expression *create_concatenation_regular_expression(
        regular_expression *left, regular_expression *right)
  {
    char *buffer;
    regular_expression_error error;
    regular_expression *result;

    assert(left != NULL);
    assert(right != NULL);

    buffer = MALLOC_ARRAY(char,
                          strlen(left->pattern) + strlen(right->pattern) + 5);
    if (buffer == NULL)
        return NULL;

    sprintf(buffer, "(%s)(%s)", left->pattern, right->pattern);

    result = create_regular_expression_from_pattern(buffer, strlen(buffer),
                                                    &error);
    free(buffer);
    assert((result != NULL) || (error == REE_MEMORY_ALLOCATION));
    return result;
  }

extern regular_expression *create_or_regular_expression(
        regular_expression *left, regular_expression *right)
  {
    char *buffer;
    regular_expression_error error;
    regular_expression *result;

    assert(left != NULL);
    assert(right != NULL);

    buffer = MALLOC_ARRAY(char,
                          strlen(left->pattern) + strlen(right->pattern) + 6);
    if (buffer == NULL)
        return NULL;

    sprintf(buffer, "(%s)|(%s)", left->pattern, right->pattern);

    result = create_regular_expression_from_pattern(buffer, strlen(buffer),
                                                    &error);
    free(buffer);
    assert((result != NULL) || (error == REE_MEMORY_ALLOCATION));
    return result;
  }

extern regular_expression *create_repeat_zero_or_more_regular_expression(
        regular_expression *base)
  {
    char *buffer;
    regular_expression_error error;
    regular_expression *result;

    assert(base != NULL);

    buffer = MALLOC_ARRAY(char, strlen(base->pattern) + 4);
    if (buffer == NULL)
        return NULL;

    sprintf(buffer, "(%s)*", base->pattern);

    result = create_regular_expression_from_pattern(buffer, strlen(buffer),
                                                    &error);
    free(buffer);
    assert((result != NULL) || (error == REE_MEMORY_ALLOCATION));
    return result;
  }

extern regular_expression *create_repeat_one_or_more_regular_expression(
        regular_expression *base)
  {
    char *buffer;
    regular_expression_error error;
    regular_expression *result;

    assert(base != NULL);

    buffer = MALLOC_ARRAY(char, strlen(base->pattern) + 4);
    if (buffer == NULL)
        return NULL;

    sprintf(buffer, "(%s)+", base->pattern);

    result = create_regular_expression_from_pattern(buffer, strlen(buffer),
                                                    &error);
    free(buffer);
    assert((result != NULL) || (error == REE_MEMORY_ALLOCATION));
    return result;
  }

extern boolean matches(regular_expression *the_regular_expression,
        const char *test_string, boolean is_start, boolean *error)
  {
    boolean result;

    assert(the_regular_expression != NULL);
    assert(test_string != NULL);

    GRAB_SYSTEM_LOCK(the_regular_expression->cache_lock);
    result = dfa_cache_matches(&(the_regular_expression->dfa_cache),
            &(the_regular_expression->nfa), test_string, is_start, FALSE,
            FALSE, 0, error, NULL);
    RELEASE_SYSTEM_LOCK(the_regular_expression->cache_lock);
    return result;
  }

extern boolean longest_match(regular_expression *the_regular_expression,
        const char *test_string, boolean is_start, size_t *match_start,
        size_t *match_length, boolean *error)
  {
    boolean any_match;
    size_t first_match_lower;
    size_t first_match_upper;

    assert(the_regular_expression != NULL);
    assert(test_string != NULL);
    assert(match_start != NULL);
    assert(match_length != NULL);

    GRAB_SYSTEM_LOCK(the_regular_expression->cache_lock);
    any_match = dfa_cache_matches(&(the_regular_expression->dfa_cache),
            &(the_regular_expression->nfa), test_string, is_start, FALSE,
            FALSE, 0, error, NULL);
    RELEASE_SYSTEM_LOCK(the_regular_expression->cache_lock);
    if ((any_match == FALSE) || *error)
        return FALSE;

    first_match_lower = 0;
    first_match_upper = strlen(test_string);

    while (first_match_lower < first_match_upper)
      {
        boolean local_is_start;
        size_t midpoint;
        boolean local_match;

        local_is_start = (is_start && (first_match_lower == 0));
        midpoint = ((first_match_upper - first_match_lower) / 2);
        GRAB_SYSTEM_LOCK(the_regular_expression->cache_lock);
        local_match = dfa_cache_matches(&(the_regular_expression->dfa_cache),
                &(the_regular_expression->nfa),
                test_string + first_match_lower, local_is_start, FALSE, TRUE,
                midpoint, error, NULL);
        RELEASE_SYSTEM_LOCK(the_regular_expression->cache_lock);
        if (*error)
            return FALSE;

        assert(first_match_upper > (first_match_lower + midpoint));
        if (local_match)
          {
            first_match_upper = (first_match_lower + midpoint);
          }
        else
          {
            first_match_lower += (midpoint + 1);
          }
      }

    assert(first_match_lower == first_match_upper);
    *match_start = first_match_lower;

    GRAB_SYSTEM_LOCK(the_regular_expression->cache_lock);
    any_match = dfa_cache_matches(&(the_regular_expression->dfa_cache),
            &(the_regular_expression->nfa), test_string + first_match_lower,
            (is_start && (first_match_lower == 0)), FALSE, TRUE, 0, error,
            match_length);
    RELEASE_SYSTEM_LOCK(the_regular_expression->cache_lock);
    assert(any_match || *error);

    return TRUE;
  }

extern const char *regular_expression_pattern(
        regular_expression *the_regular_expression)
  {
    assert(the_regular_expression != NULL);

    return the_regular_expression->pattern;
  }

extern re_follower *regular_expression_create_follower(
        regular_expression *the_regular_expression)
  {
    re_follower *result;
    verdict the_verdict;

    assert(the_regular_expression != NULL);

    result = MALLOC_ONE_OBJECT(re_follower);
    if (result == NULL)
        return NULL;

    INITIALIZE_SYSTEM_LOCK(result->lock, free(result); return NULL);

    result->regular_expression = the_regular_expression;

    the_verdict = init_dfa_cache(&(result->dfa_cache),
                                 &(the_regular_expression->nfa));
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        DESTROY_SYSTEM_LOCK(result->lock);
        free(result);
        return NULL;
      }

    regular_expression_add_reference(the_regular_expression);

    result->beyond_possible_match = FALSE;
    result->current_dfa_state_num = 0;

    return result;
  }

extern verdict re_follower_transit(re_follower *follower,
                                   const char *character)
  {
    dfa_cache *the_dfa_cache;
    const char *follow;

    assert(follower != NULL);
    assert(character != NULL);

    GRAB_SYSTEM_LOCK(follower->lock);

    if (follower->beyond_possible_match)
      {
        RELEASE_SYSTEM_LOCK(follower->lock);
        return MISSION_ACCOMPLISHED;
      }

    the_dfa_cache = &(follower->dfa_cache);

    follow = character;

    while (TRUE)
      {
        unsigned char next_char;
        size_t next_dfa_state_num;

        assert(follower->current_dfa_state_num <
               the_dfa_cache->state_pool.element_count);

        next_char = (unsigned char)*follow;

        if (next_char == 0)
          {
            RELEASE_SYSTEM_LOCK(follower->lock);
            return MISSION_ACCOMPLISHED;
          }

        next_dfa_state_num = dfa_cache_find_or_make_successor(the_dfa_cache,
                follower->current_dfa_state_num, next_char, TRUE);
        if (next_dfa_state_num == 0)
          {
            RELEASE_SYSTEM_LOCK(follower->lock);
            return MISSION_FAILED;
          }

        if (next_dfa_state_num == 1)
          {
            follower->beyond_possible_match = TRUE;
            RELEASE_SYSTEM_LOCK(follower->lock);
            return MISSION_ACCOMPLISHED;
          }

        assert(next_dfa_state_num >= 2);
        follower->current_dfa_state_num = next_dfa_state_num - 2;

        dfa_cache_mark_state_most_recently_used(the_dfa_cache,
                follower->current_dfa_state_num);

        ++follow;
      }
  }

extern verdict re_follower_end_transit(re_follower *follower)
  {
    dfa_cache *the_dfa_cache;
    size_t next_dfa_state_num;

    assert(follower != NULL);

    GRAB_SYSTEM_LOCK(follower->lock);

    if (follower->beyond_possible_match)
      {
        RELEASE_SYSTEM_LOCK(follower->lock);
        return MISSION_ACCOMPLISHED;
      }

    the_dfa_cache = &(follower->dfa_cache);

    assert(follower->current_dfa_state_num <
           the_dfa_cache->state_pool.element_count);

    next_dfa_state_num = dfa_cache_find_or_make_successor(the_dfa_cache,
            follower->current_dfa_state_num, 0xff, TRUE);
    if (next_dfa_state_num == 0)
      {
        RELEASE_SYSTEM_LOCK(follower->lock);
        return MISSION_FAILED;
      }

    if (next_dfa_state_num == 1)
      {
        follower->beyond_possible_match = TRUE;
        RELEASE_SYSTEM_LOCK(follower->lock);
        return MISSION_ACCOMPLISHED;
      }

    assert(next_dfa_state_num >= 2);
    follower->current_dfa_state_num = next_dfa_state_num - 2;
    assert(follower->current_dfa_state_num <
           the_dfa_cache->state_pool.element_count);

    dfa_cache_mark_state_most_recently_used(the_dfa_cache,
                                            follower->current_dfa_state_num);

    RELEASE_SYSTEM_LOCK(follower->lock);

    return MISSION_ACCOMPLISHED;
  }

extern boolean re_follower_is_in_accepting_state(re_follower *follower)
  {
    boolean result;

    assert(follower != NULL);

    GRAB_SYSTEM_LOCK(follower->lock);

    if (follower->beyond_possible_match)
      {
        RELEASE_SYSTEM_LOCK(follower->lock);
        return FALSE;
      }
    assert(follower->current_dfa_state_num <
           follower->dfa_cache.state_pool.element_count);
    result = follower->dfa_cache.state_pool.array[
            follower->current_dfa_state_num].contains_end;

    RELEASE_SYSTEM_LOCK(follower->lock);

    return result;
  }

extern boolean re_follower_more_possible(re_follower *follower)
  {
    boolean is_beyond;

    assert(follower != NULL);

    GRAB_SYSTEM_LOCK(follower->lock);
    is_beyond = follower->beyond_possible_match;
    RELEASE_SYSTEM_LOCK(follower->lock);

    if (is_beyond)
        return FALSE;
    return TRUE;
  }

extern void delete_re_follower(re_follower *follower)
  {
    regular_expression *base;
    size_t base_reference_count;

    base = follower->regular_expression;
    assert(base != NULL);

    GRAB_SYSTEM_LOCK(base->reference_lock);
    base_reference_count = base->reference_count;
    RELEASE_SYSTEM_LOCK(base->reference_lock);

    if ((base_reference_count > 1) &&
        (base->dfa_cache.used_dfa_state_count >
         follower->dfa_cache.used_dfa_state_count))
      {
        GRAB_SYSTEM_LOCK(base->cache_lock);
        deallocate_dfa_cache(&(base->dfa_cache));
        base->dfa_cache = follower->dfa_cache;
        RELEASE_SYSTEM_LOCK(base->cache_lock);
      }
    else
      {
        deallocate_dfa_cache(&(follower->dfa_cache));
      }

    regular_expression_remove_reference(base);
    DESTROY_SYSTEM_LOCK(follower->lock);
    free(follower);
  }

extern void regular_expression_add_reference(
        regular_expression *the_regular_expression)
  {
    assert(the_regular_expression != NULL);

    GRAB_SYSTEM_LOCK(the_regular_expression->reference_lock);
    assert(the_regular_expression->reference_count > 0);
    ++(the_regular_expression->reference_count);
    RELEASE_SYSTEM_LOCK(the_regular_expression->reference_lock);
  }

extern void regular_expression_remove_reference(
        regular_expression *the_regular_expression)
  {
    size_t new_reference_count;

    assert(the_regular_expression != NULL);

    GRAB_SYSTEM_LOCK(the_regular_expression->reference_lock);
    assert(the_regular_expression->reference_count > 0);
    --(the_regular_expression->reference_count);
    new_reference_count = the_regular_expression->reference_count;
    RELEASE_SYSTEM_LOCK(the_regular_expression->reference_lock);

    if (new_reference_count > 0)
        return;

    free(the_regular_expression->pattern);
    deallocate_nfa(&(the_regular_expression->nfa));
    deallocate_dfa_cache(&(the_regular_expression->dfa_cache));
    DESTROY_SYSTEM_LOCK(the_regular_expression->cache_lock);
    DESTROY_SYSTEM_LOCK(the_regular_expression->reference_lock);

    free(the_regular_expression);
  }

extern int regular_expression_structural_order(
        regular_expression *regular_expression1,
        regular_expression *regular_expression2)
  {
    assert(regular_expression1 != NULL);
    assert(regular_expression2 != NULL);

    assert(regular_expression1->pattern != NULL);
    assert(regular_expression2->pattern != NULL);
    return utf8_string_lexicographical_order_by_code_point(
            regular_expression1->pattern, regular_expression2->pattern);
  }

extern const char *string_for_regular_expression_error(
        regular_expression_error the_error)
  {
    switch (the_error)
      {
        case REE_MEMORY_ALLOCATION:
            return "Failed trying to allocate memory.";
        case REE_MISMATCHED_RIGHT_PAREN:
            return "The regular expression pattern contains a right "
                   "parenthesis with no matching left parenthesis.";
        case REE_MISMATCHED_LEFT_PAREN:
            return "The regular expression pattern contains a left parenthesis"
                   " with no matching right parenthesis.";
        case REE_EMPTY_STARRED:
            return "The regular expression pattern contains an asterix without"
                   " a pattern to its left.";
        case REE_EMPTY_PLUSSED:
            return "The regular expression pattern contains a plus sign "
                   "without a pattern to its left.";
        case REE_EMPTY_OPTIONAL:
            return "The regular expression pattern contains a question mark "
                   "without a pattern to its left.";
        case REE_EMPTY_BRACED:
            return "The regular expression pattern contains a left curly brace"
                   " without a pattern to its left.";
        case REE_UNTERMINATED_BRACKET:
            return "The regular expression pattern contains a character range "
                   "without a closing right square bracket.";
        case REE_HEX_NO_DIGITS:
            return "The regular expression pattern contains a backslash-x that"
                   " isn't followed by at least one hex digit.";
        case REE_HEX_ZERO:
            return "The regular expression pattern contains a hex escape "
                   "sequence encoding character zero.";
        case REE_HEX_UTF_16_SURROGATE:
            return "The regular expression pattern contains a hex escape "
                   "sequence encoding a UTF-16 surrogate character, which is "
                   "not a valid Unicode character.";
        case REE_HEX_TOO_LARGE:
            return "The regular expression pattern contains a hex escape "
                   "sequence encoding a value greater than any Unicode code "
                   "point.";
        case REE_BAD_ESCAPE_LETTER:
            return "The regular expression pattern contains an escaped letter "
                   "that is not a valid escape sequence.";
        case REE_RANGE_START_IS_RANGE:
            return "The regular expression pattern contains a character range "
                   "specification that starts with an escape sequence "
                   "representing more than a single character.";
        case REE_RANGE_END_IS_RANGE:
            return "The regular expression pattern contains a character range "
                   "specification that ends with an escape sequence "
                   "representing more than a single character.";
        case REE_MISSING_BRACE_COMMA:
            return "The regular expression pattern contains a repeat "
                   "specification in curly braces that doesn't have a comma "
                   "after the minimum repetition count.";
        case REE_MISSING_BRACE_CLOSE:
            return "The regular expression pattern contains a repeat "
                   "specification in curly braces that doesn't have a closing "
                   "curly brace where expected.";
        case REE_BRACE_LOWER_GREATER_THAN_UPPER:
            return "The regular expression pattern contains a repeat "
                   "specification in curly braces in which the lower bound on "
                   "the number of repetitions is greater than the upper "
                   "bound.";
        default:
            assert(FALSE);
            return NULL;
      }
  }


static verdict create_nfa_for_pattern(nfa *the_nfa, const char *pattern_chars,
                                      regular_expression_error *error)
  {
    nfa_state_aa state_pool;
    verdict the_verdict;
    size_t pattern_chars_used;

    assert(the_nfa != NULL);
    assert(pattern_chars != NULL);
    assert(error != NULL);

    the_verdict = nfa_state_aa_init(&state_pool, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        *error = REE_MEMORY_ALLOCATION;
        return the_verdict;
      }

    the_verdict = nfa_add_sub_pattern(&state_pool, pattern_chars,
            &(the_nfa->start_state), &(the_nfa->end_state),
            &pattern_chars_used, error);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(state_pool.array);
        return the_verdict;
      }

    if (pattern_chars[pattern_chars_used] != 0)
      {
        assert(pattern_chars[pattern_chars_used] == ')');
        *error = REE_MISMATCHED_RIGHT_PAREN;
        free(state_pool.array);
        return MISSION_FAILED;
      }

    the_nfa->state_count = state_pool.element_count;
    the_nfa->state_table = state_pool.array;
    return MISSION_ACCOMPLISHED;
  }

static verdict nfa_add_sub_pattern(nfa_state_aa *state_pool,
        const char *pattern_chars, size_t *start_state, size_t *end_state,
        size_t *pattern_chars_used, regular_expression_error *error)
  {
    nfa_state state_buffer;
    size_t current_start_state;
    size_t current_end_state;
    verdict the_verdict;
    size_t position;
    size_t last_end_state;
    char next_char;

    assert(state_pool != NULL);
    assert(pattern_chars != NULL);
    assert(start_state != NULL);
    assert(end_state != NULL);
    assert(pattern_chars_used != NULL);
    assert(error != NULL);

    current_start_state = state_pool->element_count;
    current_end_state = current_start_state;

    state_buffer.next[0] = 0;
    state_buffer.next[1] = 0;

    the_verdict = nfa_state_aa_append(state_pool, state_buffer);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        *error = REE_MEMORY_ALLOCATION;
        return the_verdict;
      }

    position = 0;
    last_end_state = current_end_state;

    next_char = pattern_chars[position];

    while (TRUE)
      {
        switch (next_char)
          {
            case 0:
            case ')':
              {
                *start_state = current_start_state;
                *end_state = current_end_state;
                *pattern_chars_used = position;

                return MISSION_ACCOMPLISHED;
              }
            case '\\':
              {
                ++position;
                next_char = pattern_chars[position];

                if (((next_char >= 'a') && (next_char <= 'z')) ||
                    ((next_char >= 'A') && (next_char <= 'Z')))
                  {
                    const code_point_range *letter_ranges;
                    size_t letter_range_count;
                    verdict the_verdict;

                    if ((next_char == 'x') || (next_char == 'X'))
                      {
                        size_t byte_count;
                        unsigned long code_point;
                        size_t new_state;
                        verdict the_verdict;
                        code_point_range the_range;

                        code_point = hex_character_specification_to_code_point(
                                &(pattern_chars[position - 1]), &byte_count,
                                error);
                        if (code_point == 0)
                            return MISSION_FAILED;

                        assert(byte_count > 1);
                        position += byte_count - 1;

                        new_state = state_pool->element_count;

                        state_buffer.next[0] = 0;
                        state_buffer.next[1] = 0;

                        the_verdict =
                                nfa_state_aa_append(state_pool, state_buffer);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            *error = REE_MEMORY_ALLOCATION;
                            return the_verdict;
                          }

                        the_range.first = code_point;
                        the_range.last = code_point;

                        the_verdict = add_range_to_nfa(state_pool,
                                current_end_state, new_state, &the_range);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            *error = REE_MEMORY_ALLOCATION;
                            return the_verdict;
                          }

                        current_end_state = new_state;

                        break;
                      }

                    letter_ranges = ranges_for_escaped_letter(next_char,
                            &letter_range_count);
                    if (letter_ranges == NULL)
                      {
                        *error = REE_BAD_ESCAPE_LETTER;
                        return MISSION_FAILED;
                      }

                    the_verdict = add_ranges_to_nfa(state_pool,
                            current_end_state, letter_ranges,
                            letter_range_count, &current_end_state);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        *error = REE_MEMORY_ALLOCATION;
                        return the_verdict;
                      }

                    ++position;

                    break;
                  }

                goto plain_character;
              }
            case '|':
              {
                size_t alternate_start_state;
                size_t alternate_end_state;
                size_t alternate_chars_used;
                verdict the_verdict;
                size_t new_start_state;
                size_t new_end_state;

                the_verdict = nfa_add_sub_pattern(state_pool,
                        &(pattern_chars[position + 1]), &alternate_start_state,
                        &alternate_end_state, &alternate_chars_used, error);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    *error = REE_MEMORY_ALLOCATION;
                    return the_verdict;
                  }

                new_start_state = state_pool->element_count;

                state_buffer.link_first_char[0] = 0;
                state_buffer.link_last_char[0] = 0;
                state_buffer.next[0] = current_start_state + 1;
                state_buffer.link_first_char[1] = 0;
                state_buffer.link_last_char[1] = 0;
                state_buffer.next[1] = alternate_start_state + 1;

                the_verdict = nfa_state_aa_append(state_pool, state_buffer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    *error = REE_MEMORY_ALLOCATION;
                    return the_verdict;
                  }

                new_end_state = state_pool->element_count;

                state_buffer.next[0] = 0;
                state_buffer.next[1] = 0;

                the_verdict = nfa_state_aa_append(state_pool, state_buffer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    *error = REE_MEMORY_ALLOCATION;
                    return the_verdict;
                  }

                assert(state_pool->array[current_end_state].next[0] == 0);
                assert(state_pool->array[current_end_state].next[1] == 0);

                state_pool->array[current_end_state].link_first_char[0] = 0;
                state_pool->array[current_end_state].link_last_char[0] = 0;
                state_pool->array[current_end_state].next[0] =
                        new_end_state + 1;

                assert(state_pool->array[alternate_end_state].next[0] == 0);
                assert(state_pool->array[alternate_end_state].next[1] == 0);

                state_pool->array[alternate_end_state].link_first_char[0] = 0;
                state_pool->array[alternate_end_state].link_last_char[0] = 0;
                state_pool->array[alternate_end_state].next[0] =
                        new_end_state + 1;

                *start_state = new_start_state;
                *end_state = new_end_state;
                *pattern_chars_used = position + alternate_chars_used + 1;

                return MISSION_ACCOMPLISHED;
              }
            case '(':
              {
                size_t alternate_start_state;
                size_t alternate_end_state;
                size_t alternate_chars_used;
                verdict the_verdict;

                the_verdict = nfa_add_sub_pattern(state_pool,
                        &(pattern_chars[position + 1]), &alternate_start_state,
                        &alternate_end_state, &alternate_chars_used, error);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    *error = REE_MEMORY_ALLOCATION;
                    return the_verdict;
                  }

                position += alternate_chars_used + 1;
                if (pattern_chars[position] != ')')
                  {
                    *error = REE_MISMATCHED_LEFT_PAREN;
                    return MISSION_FAILED;
                  }
                ++position;

                assert(state_pool->array[current_end_state].next[0] == 0);
                assert(state_pool->array[current_end_state].next[1] == 0);

                state_pool->array[current_end_state].link_first_char[0] = 0;
                state_pool->array[current_end_state].link_last_char[0] = 0;
                state_pool->array[current_end_state].next[0] =
                        alternate_start_state + 1;

                current_end_state = alternate_end_state;

                break;
              }
            case '*':
              {
                *error = REE_EMPTY_STARRED;
                return MISSION_FAILED;
              }
            case '+':
              {
                *error = REE_EMPTY_PLUSSED;
                return MISSION_FAILED;
              }
            case '?':
              {
                *error = REE_EMPTY_OPTIONAL;
                return MISSION_FAILED;
              }
            case '{':
              {
                *error = REE_EMPTY_BRACED;
                return MISSION_FAILED;
              }
            case '[':
              {
                boolean negate;
                code_point_range_aa ranges;
                verdict the_verdict;

                ++position;

                if (pattern_chars[position] == '^')
                  {
                    negate = TRUE;
                    ++position;
                  }
                else
                  {
                    negate = FALSE;
                  }

                the_verdict = code_point_range_aa_init(&ranges, 10);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    *error = REE_MEMORY_ALLOCATION;
                    return the_verdict;
                  }

                while (TRUE)
                  {
                    const code_point_range *first_ranges;
                    size_t range_count;
                    size_t bytes_in_character_specification;
                    code_point_range new_range;
                    verdict the_verdict;

                    if (pattern_chars[position] == ']')
                        break;

                    new_range.first =
                            range_character_specification_to_code_point(
                                    &(pattern_chars[position]),
                                    &bytes_in_character_specification,
                                    &first_ranges, &range_count, error);
                    if ((new_range.first == 0) && (first_ranges == NULL))
                      {
                        free(ranges.array);
                        return MISSION_FAILED;
                      }

                    position += bytes_in_character_specification;

                    if (pattern_chars[position] != '-')
                      {
                        if (new_range.first == 0)
                          {
                            verdict the_verdict;

                            assert(first_ranges != NULL);

                            the_verdict = code_point_range_aa_append_array(
                                    &ranges, range_count, first_ranges);
                            if (the_verdict != MISSION_ACCOMPLISHED)
                              {
                                free(ranges.array);
                                *error = REE_MEMORY_ALLOCATION;
                                return the_verdict;
                              }

                            continue;
                          }

                        new_range.last = new_range.first;
                      }
                    else
                      {
                        size_t second_bytes_in_character_specification;
                        const code_point_range *second_ranges;
                        size_t second_range_count;

                        if (new_range.first == 0)
                          {
                            assert(first_ranges != NULL);
                            *error = REE_RANGE_START_IS_RANGE;
                            free(ranges.array);
                            return MISSION_FAILED;
                          }

                        ++position;

                        new_range.last =
                                range_character_specification_to_code_point(
                                    &(pattern_chars[position]),
                                    &second_bytes_in_character_specification,
                                    &second_ranges, &second_range_count,
                                    error);
                        if (new_range.last == 0)
                          {
                            if (second_ranges != NULL)
                                *error = REE_RANGE_END_IS_RANGE;
                            free(ranges.array);
                            return MISSION_FAILED;
                          }

                        position += second_bytes_in_character_specification;
                      }

                    the_verdict =
                            code_point_range_aa_append(&ranges, new_range);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        free(ranges.array);
                        *error = REE_MEMORY_ALLOCATION;
                        return the_verdict;
                      }
                  }

                ++position;

                if (ranges.element_count > 0)
                  {
                    qsort(ranges.array, ranges.element_count,
                          sizeof(code_point_range), &compare_ranges);

                    ranges.element_count =
                            merge_ranges(ranges.array, ranges.element_count);
                  }

                if (negate)
                  {
                    size_t target_num;
                    size_t source_num;
                    unsigned long previous_range_end;

                    target_num = 0;
                    source_num = 0;
                    previous_range_end = 0;

                    while (source_num < ranges.element_count)
                      {
                        unsigned long new_first;
                        unsigned long new_last;

                        assert(target_num <= source_num);

                        new_first = ranges.array[source_num].first;
                        new_last = ranges.array[source_num].last;

                        if (previous_range_end + 1 < new_first)
                          {
                            ranges.array[target_num].first =
                                    previous_range_end + 1;
                            ranges.array[target_num].last = new_first - 1;
                            ++target_num;
                          }
                        else
                          {
                            assert(source_num == 0);
                          }

                        previous_range_end = new_last;

                        ++source_num;
                      }

                    ranges.element_count = target_num;

                    if (previous_range_end < 0x10ffff)
                      {
                        code_point_range new_range;
                        verdict the_verdict;

                        new_range.first = previous_range_end + 1;
                        new_range.last = 0x10ffff;
                        the_verdict =
                                code_point_range_aa_append(&ranges, new_range);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            free(ranges.array);
                            *error = REE_MEMORY_ALLOCATION;
                            return the_verdict;
                          }
                      }
                  }

                the_verdict = add_ranges_to_nfa(state_pool, current_end_state,
                        ranges.array, ranges.element_count,
                        &current_end_state);
                free(ranges.array);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    *error = REE_MEMORY_ALLOCATION;
                    return the_verdict;
                  }

                break;
              }
            case '.':
              {
                code_point_range ranges[1];
                verdict the_verdict;

                ranges[0].first = 1;
                ranges[0].last = 0x10ffff;
                the_verdict = add_ranges_to_nfa(state_pool, current_end_state,
                        &(ranges[0]), 1, &current_end_state);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    *error = REE_MEMORY_ALLOCATION;
                    return the_verdict;
                  }

                ++position;

                break;
              }
            case '^':
              {
                next_char = 0xfe;
                goto plain_character;
              }
            case '$':
              {
                next_char = 0xff;
                goto plain_character;
              }
            default:
              plain_character:
              {
                size_t new_state;
                verdict the_verdict;

                new_state = state_pool->element_count;

                assert(state_pool->array[current_end_state].next[0] == 0);
                assert(state_pool->array[current_end_state].next[1] == 0);

                state_pool->array[current_end_state].link_first_char[0] =
                        next_char;
                state_pool->array[current_end_state].link_last_char[0] =
                        next_char;
                state_pool->array[current_end_state].next[0] = new_state + 1;

                state_buffer.next[0] = 0;
                state_buffer.next[1] = 0;

                the_verdict = nfa_state_aa_append(state_pool, state_buffer);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    *error = REE_MEMORY_ALLOCATION;
                    return the_verdict;
                  }

                current_end_state = new_state;
                ++position;
              }
          }

        next_char = pattern_chars[position];

        if ((next_char == '*') || (next_char == '+') || (next_char == '?') ||
            (next_char == '{'))
          {
            boolean repeat;
            boolean skip;
            verdict the_verdict;

            repeat = FALSE;
            skip = FALSE;

            while (TRUE)
              {
                if (next_char == '*')
                  {
                    repeat = TRUE;
                    skip = TRUE;
                  }
                else if (next_char == '+')
                  {
                    repeat = TRUE;
                  }
                else if (next_char == '?')
                  {
                    skip = TRUE;
                  }
                else if (next_char == '{')
                  {
                    size_t at_least_count;
                    boolean no_upper_limit;
                    size_t at_most_count;
                    verdict the_verdict;
                    nfa duplicate;
                    size_t new_end_state;
                    size_t repetition_num;

                    ++position;

                    at_least_count = 0;
                    while ((pattern_chars[position] >= '0') &&
                           (pattern_chars[position] <= '9'))
                      {
                        if (!skip)
                          {
                            size_t digit_value;

                            digit_value = pattern_chars[position] - '0';

                            if (at_least_count >
                                (((~(size_t)0) - digit_value) / 10))
                              {
                                *error = REE_MEMORY_ALLOCATION;
                                return MISSION_FAILED;
                              }
                            at_least_count *= 10;
                            at_least_count += digit_value;
                          }

                        ++position;
                      }

                    if (pattern_chars[position] != ',')
                      {
                        *error = REE_MISSING_BRACE_COMMA;
                        return MISSION_FAILED;
                      }

                    ++position;

                    no_upper_limit = repeat;
                    if ((pattern_chars[position] < '0') ||
                        (pattern_chars[position] > '9'))
                      {
                        no_upper_limit = TRUE;
                      }
                    else
                      {
                        at_most_count = 0;
                        while ((pattern_chars[position] >= '0') &&
                               (pattern_chars[position] <= '9'))
                          {
                            if (!no_upper_limit)
                              {
                                size_t digit_value;

                                digit_value = pattern_chars[position] - '0';

                                if (at_most_count >
                                    (((~(size_t)0) - digit_value) / 10))
                                  {
                                    *error = REE_MEMORY_ALLOCATION;
                                    return MISSION_FAILED;
                                  }
                                at_most_count *= 10;
                                at_most_count += digit_value;
                              }

                            ++position;
                          }
                      }

                    if (pattern_chars[position] != '}')
                      {
                        *error = REE_MISSING_BRACE_CLOSE;
                        return MISSION_FAILED;
                      }

                    ++position;

                    if ((!no_upper_limit) && (at_most_count < at_least_count))
                      {
                        *error = REE_BRACE_LOWER_GREATER_THAN_UPPER;
                        return MISSION_FAILED;
                      }

                    next_char = pattern_chars[position];

                    if (skip && repeat)
                        continue;

                    if (no_upper_limit && (at_least_count <= 1))
                      {
                        repeat = TRUE;
                        if (at_least_count == 0)
                            skip = TRUE;
                        continue;
                      }

                    if ((!no_upper_limit) && (at_most_count == 1))
                      {
                        if (at_least_count == 0)
                            skip = TRUE;
                        continue;
                      }

                    if ((!no_upper_limit) && (at_most_count == 0))
                      {
                        current_end_state = last_end_state;
                        state_pool->array[last_end_state].next[0] = 0;
                        state_pool->array[last_end_state].next[1] = 0;
                        continue;
                      }

                    the_verdict = do_repeat_and_skip(state_pool, skip, repeat,
                            last_end_state, &current_end_state);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        *error = REE_MEMORY_ALLOCATION;
                        return the_verdict;
                      }

                    the_verdict = duplicate_nfa_portion(&duplicate,
                            state_pool->element_count, state_pool->array,
                            last_end_state, current_end_state);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        *error = REE_MEMORY_ALLOCATION;
                        return the_verdict;
                      }

                    new_end_state = state_pool->element_count;

                    state_buffer.next[0] = 0;
                    state_buffer.next[1] = 0;

                    the_verdict =
                            nfa_state_aa_append(state_pool, state_buffer);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        *error = REE_MEMORY_ALLOCATION;
                        free(duplicate.state_table);
                        return the_verdict;
                      }

                    for (repetition_num = 1; repetition_num < at_least_count;
                         ++repetition_num)
                      {
                        verdict the_verdict;

                        the_verdict = add_nfa_copy(state_pool,
                                current_end_state, &duplicate,
                                &current_end_state);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            *error = REE_MEMORY_ALLOCATION;
                            free(duplicate.state_table);
                            return the_verdict;
                          }
                      }

                    if (at_least_count == 0)
                      {
                        verdict the_verdict;

                        the_verdict = add_link_to_nfa(state_pool,
                                last_end_state, new_end_state, 0, 0);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            *error = REE_MEMORY_ALLOCATION;
                            free(duplicate.state_table);
                            return the_verdict;
                          }
                      }
                    else
                      {
                        verdict the_verdict;

                        the_verdict = add_link_to_nfa(state_pool,
                                current_end_state, new_end_state, 0, 0);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            *error = REE_MEMORY_ALLOCATION;
                            free(duplicate.state_table);
                            return the_verdict;
                          }
                      }

                    if (no_upper_limit)
                      {
                        size_t loop_start_state;
                        verdict the_verdict;
                        size_t loop_end_state;

                        assert(at_least_count > 1);

                        loop_start_state = state_pool->element_count;

                        state_buffer.next[0] = 0;
                        state_buffer.next[1] = 0;

                        the_verdict =
                                nfa_state_aa_append(state_pool, state_buffer);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            *error = REE_MEMORY_ALLOCATION;
                            free(duplicate.state_table);
                            return the_verdict;
                          }

                        the_verdict = add_nfa_copy(state_pool,
                                loop_start_state, &duplicate, &loop_end_state);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            *error = REE_MEMORY_ALLOCATION;
                            free(duplicate.state_table);
                            return the_verdict;
                          }

                        the_verdict = add_link_to_nfa(state_pool,
                                current_end_state, loop_start_state, 0, 0);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            *error = REE_MEMORY_ALLOCATION;
                            free(duplicate.state_table);
                            return the_verdict;
                          }

                        the_verdict = add_link_to_nfa(state_pool,
                                loop_end_state, current_end_state, 0, 0);
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            *error = REE_MEMORY_ALLOCATION;
                            free(duplicate.state_table);
                            return the_verdict;
                          }
                      }
                    else
                      {
                        for (repetition_num = at_least_count;
                             repetition_num < at_most_count; ++repetition_num)
                          {
                            verdict the_verdict;

                            if (repetition_num > 0)
                              {
                                size_t block_start_state;
                                verdict the_verdict;
                                size_t block_end_state;

                                block_start_state = state_pool->element_count;

                                state_buffer.next[0] = 0;
                                state_buffer.next[1] = 0;

                                the_verdict = nfa_state_aa_append(state_pool,
                                        state_buffer);
                                if (the_verdict != MISSION_ACCOMPLISHED)
                                  {
                                    *error = REE_MEMORY_ALLOCATION;
                                    free(duplicate.state_table);
                                    return the_verdict;
                                  }

                                the_verdict = add_nfa_copy(state_pool,
                                        block_start_state, &duplicate,
                                        &block_end_state);
                                if (the_verdict != MISSION_ACCOMPLISHED)
                                  {
                                    *error = REE_MEMORY_ALLOCATION;
                                    free(duplicate.state_table);
                                    return the_verdict;
                                  }

                                the_verdict = add_link_to_nfa(state_pool,
                                        current_end_state, block_start_state,
                                        0, 0);
                                if (the_verdict != MISSION_ACCOMPLISHED)
                                  {
                                    *error = REE_MEMORY_ALLOCATION;
                                    free(duplicate.state_table);
                                    return the_verdict;
                                  }

                                current_end_state = block_end_state;
                              }

                            the_verdict = add_link_to_nfa(state_pool,
                                    current_end_state, new_end_state, 0, 0);
                            if (the_verdict != MISSION_ACCOMPLISHED)
                              {
                                *error = REE_MEMORY_ALLOCATION;
                                free(duplicate.state_table);
                                return the_verdict;
                              }
                          }
                      }

                    free(duplicate.state_table);

                    current_end_state = new_end_state;

                    repeat = FALSE;
                    skip = FALSE;

                    continue;
                  }
                else
                  {
                    break;
                  }

                ++position;
                next_char = pattern_chars[position];
              }

            the_verdict = do_repeat_and_skip(state_pool, skip, repeat,
                    last_end_state, &current_end_state);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                *error = REE_MEMORY_ALLOCATION;
                return the_verdict;
              }
          }

        /* Make sure we don't updated the last_end_state pointer to the middle
         * of a multi-byte UTF-8 character. */
        if ((next_char & 0xc0) != 0x80)
            last_end_state = current_end_state;
      }
  }

static void deallocate_nfa(nfa *the_nfa)
  {
    assert(the_nfa != NULL);

    assert(the_nfa->state_table != NULL);
    free(the_nfa->state_table);
  }

static verdict init_dfa_cache(dfa_cache *the_dfa_cache, nfa *the_nfa)
  {
    verdict the_verdict;
    dfa_state state_buffer;
    unsigned char dummy_range_low;
    unsigned char dummy_range_high;

    assert(the_dfa_cache != NULL);
    assert(the_nfa != NULL);

    the_verdict =
            dfa_state_element_aa_init(&(the_dfa_cache->element_pool), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_dfa_cache->first_free_element = 0;

    the_verdict = dfa_state_aa_init(&(the_dfa_cache->state_pool), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(the_dfa_cache->element_pool.array);
        return the_verdict;
      }

    the_dfa_cache->used_dfa_state_count = 2;
    the_dfa_cache->most_recently_used_dfa_state = 0;
    the_dfa_cache->least_recently_used_dfa_state = 0;
    the_dfa_cache->nfa = the_nfa;

    the_verdict =
            dfa_state_aa_append(&(the_dfa_cache->state_pool), state_buffer);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_verdict =
            dfa_state_aa_append(&(the_dfa_cache->state_pool), state_buffer);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_verdict = initialize_dfa_state(the_dfa_cache, 0);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_verdict = initialize_dfa_state(the_dfa_cache, 1);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_dfa_cache->state_pool.array[1].contains_end = FALSE;

    the_verdict = add_nfa_state_and_null_successors_to_dfa_state(the_dfa_cache,
            &(the_dfa_cache->state_pool.array[1]), the_nfa->start_state);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    the_verdict = build_new_dfa_state(the_dfa_cache,
            &(the_dfa_cache->state_pool.array[1]), 0, 0xfe, TRUE,
            &dummy_range_low, &dummy_range_high);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    return MISSION_ACCOMPLISHED;
  }

static void deallocate_dfa_cache(dfa_cache *the_dfa_cache)
  {
    dfa_state *state_array;
    size_t state_count;
    size_t state_num;

    assert(the_dfa_cache != NULL);

    state_array = the_dfa_cache->state_pool.array;
    assert(state_array != NULL);

    state_count = the_dfa_cache->state_pool.element_count;

    for (state_num = 0; state_num < state_count; ++state_num)
      {
        assert(state_array[state_num].elements_bit_vector != NULL);
        free(state_array[state_num].elements_bit_vector);
      }

    free(the_dfa_cache->element_pool.array);
    free(state_array);
  }

static int compare_ranges(const void *left_data, const void *right_data)
  {
    const code_point_range *left_range;
    const code_point_range *right_range;

    assert(left_data != NULL);
    assert(right_data != NULL);

    left_range = (const code_point_range *)left_data;
    right_range = (const code_point_range *)right_data;

    if (left_range->first < right_range->first)
        return -1;
    else if (left_range->first > right_range->first)
        return 1;
    else if (left_range->last < right_range->last)
        return -1;
    else if (left_range->last > right_range->last)
        return 1;
    else
        return 0;
  }

static size_t merge_ranges(code_point_range *range_array, size_t count)
  {
    size_t new_count;
    size_t num_handled;

    if (count == 0)
        return 0;

    new_count = 1;
    num_handled = 1;

    while (num_handled < count)
      {
        unsigned long old_first;
        unsigned long old_last;
        unsigned long new_first;
        unsigned long new_last;

        assert(new_count <= num_handled);

        old_first = range_array[new_count - 1].first;
        old_last = range_array[new_count - 1].last;
        new_first = range_array[num_handled].first;
        new_last = range_array[num_handled].last;

        assert(old_first <= new_first);
        assert(old_first <= old_last);
        assert(new_first <= new_last);

        if (old_first == new_first)
          {
            assert(old_last <= new_last);
            range_array[new_count - 1].last = new_last;
          }
        else
          {
            assert(old_first < new_first);

            if (new_first <= old_last + 1)
              {
                if (old_last < new_last)
                    range_array[new_count - 1].last = new_last;
              }
            else
              {
                if (new_count < num_handled)
                  {
                    range_array[new_count].last =
                            range_array[num_handled].last;
                    range_array[new_count].first =
                            range_array[num_handled].first;
                  }
                ++new_count;
              }
          }

        ++num_handled;
      }

    return new_count;
  }

static verdict add_ranges_to_nfa(nfa_state_aa *state_pool,
        size_t original_end_state, const code_point_range *range_array,
        size_t range_count, size_t *new_end_state)
  {
    size_t new_state;
    nfa_state state_buffer;
    verdict the_verdict;
    size_t range_num;

    assert(state_pool != NULL);
    assert(range_array != NULL);
    assert(new_end_state != NULL);

    new_state = state_pool->element_count;

    state_buffer.next[0] = 0;
    state_buffer.next[1] = 0;

    the_verdict = nfa_state_aa_append(state_pool, state_buffer);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    for (range_num = 0; range_num < range_count; ++range_num)
      {
        verdict the_verdict;

        the_verdict = add_range_to_nfa(state_pool, original_end_state,
                                       new_state, &(range_array[range_num]));
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    *new_end_state = new_state;

    return MISSION_ACCOMPLISHED;
  }

static verdict add_range_to_nfa(nfa_state_aa *state_pool, size_t from_state,
            size_t to_state, const code_point_range *the_range)
  {
    unsigned long first;
    unsigned long last;

    assert(state_pool != NULL);
    assert(the_range != NULL);

    first = the_range->first;
    last = the_range->last;

    assert(first <= last);
    assert(first > 0);

    if (first < 0x80)
      {
        verdict the_verdict;

        the_verdict = add_link_to_nfa(state_pool, from_state, to_state,
                (unsigned char)first,
                ((last < 0x80) ? (unsigned char)last : (unsigned char)0x7f));
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    if ((first < 0x800) && (last >= 0x80))
      {
        verdict the_verdict;

        the_verdict = add_multi_byte_character_range_to_nfa(state_pool,
                from_state, to_state, ((first < 0x80) ? 0x80 : first),
                ((last < 0x800) ? last : 0x7ff), 2);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    if ((first < 0x10000) && (last >= 0x800))
      {
        verdict the_verdict;

        the_verdict = add_multi_byte_character_range_to_nfa(state_pool,
                from_state, to_state, ((first < 0x800) ? 0x800 : first),
                ((last < 0x10000) ? last : 0xffff), 3);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    if (last >= 0x10000)
      {
        verdict the_verdict;

        the_verdict = add_multi_byte_character_range_to_nfa(state_pool,
                from_state, to_state, ((first < 0x10000) ? 0x10000 : first),
                last, 4);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

static verdict add_link_to_nfa(nfa_state_aa *state_pool, size_t from_state,
        size_t to_state, unsigned char link_first_char,
        unsigned char link_last_char)
  {
    nfa_state *from_info;
    size_t link_num;
    size_t new_state;
    verdict the_verdict;

    assert(state_pool != NULL);
    assert(link_first_char <= link_last_char);

    from_info = &(state_pool->array[from_state]);

    for (link_num = 0; link_num < 2; ++link_num)
      {
        if (from_info->next[link_num] == 0)
          {
            from_info->link_first_char[link_num] = link_first_char;
            from_info->link_last_char[link_num] = link_last_char;
            from_info->next[link_num] = to_state + 1;
            return MISSION_ACCOMPLISHED;
          }
      }

    new_state = state_pool->element_count;

    the_verdict = nfa_state_aa_append(state_pool, *from_info);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return the_verdict;

    from_info = &(state_pool->array[from_state]);

    from_info->link_first_char[0] = 0;
    from_info->link_last_char[0] = 0;
    from_info->next[0] = new_state + 1;

    from_info->link_first_char[1] = link_first_char;
    from_info->link_last_char[1] = link_last_char;
    from_info->next[1] = to_state + 1;

    return MISSION_ACCOMPLISHED;
  }

static verdict add_multi_byte_character_range_to_nfa(nfa_state_aa *state_pool,
        size_t from_state, size_t to_state, unsigned long first_code_point,
        unsigned long last_code_point, size_t byte_count)
  {
    char first_buffer[5];
    size_t buffer_byte_count;
    char last_buffer[5];

    assert(state_pool != NULL);
    assert(byte_count >= 1);
    assert(byte_count <= 4);

    buffer_byte_count = code_point_to_utf8(first_code_point, first_buffer);
    assert(buffer_byte_count == byte_count);

    buffer_byte_count = code_point_to_utf8(last_code_point, last_buffer);
    assert(buffer_byte_count == byte_count);

    return add_string_range_to_nfa(state_pool, from_state, to_state,
            &(first_buffer[0]), &(last_buffer[0]), byte_count);
  }

static verdict add_string_range_to_nfa(nfa_state_aa *state_pool,
        size_t from_state, size_t to_state, const char *first_bytes,
        const char *last_bytes, size_t byte_count)
  {
    unsigned char first_0;
    unsigned char last_0;
    nfa_state state_buffer;
    size_t byte_num;
    boolean separate_low_needed;
    unsigned char mid_low;
    boolean separate_high_needed;
    unsigned char mid_high;
    static const char low_buffer[3] = { (char)0x01, (char)0x01, (char)0x01 };
    static const char high_buffer[3] = { (char)0xfd, (char)0xfd, (char)0xfd };

    assert(state_pool != NULL);
    assert(first_bytes != NULL);
    assert(last_bytes != NULL);
    assert(byte_count > 0);
    assert(byte_count <= 4);

    first_0 = (unsigned char)(first_bytes[0]);
    last_0 = (unsigned char)(last_bytes[0]);

    assert(first_0 > 0);
    assert(first_0 <= 0xfd);
    assert(last_0 > 0);
    assert(last_0 <= 0xfd);
    assert(first_0 <= last_0);

    if (byte_count == 1)
      {
        return add_link_to_nfa(state_pool, from_state, to_state, first_0,
                               last_0);
      }

    state_buffer.next[0] = 0;
    state_buffer.next[1] = 0;

    separate_low_needed = FALSE;
    for (byte_num = 1; byte_num < byte_count; ++byte_num)
      {
        assert(((unsigned char)(first_bytes[byte_num])) >= 0x1);
        if (((unsigned char)(first_bytes[byte_num])) > 0x1)
          {
            separate_low_needed = TRUE;
            break;
          }
      }

    if (!separate_low_needed)
      {
        mid_low = first_0;
      }
    else
      {
        size_t new_bottom_state;
        verdict the_verdict;

        mid_low = first_0 + 1;

        new_bottom_state = state_pool->element_count;

        the_verdict = nfa_state_aa_append(state_pool, state_buffer);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        the_verdict = add_link_to_nfa(state_pool, from_state, new_bottom_state,
                                      first_0, first_0);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        if (first_0 == last_0)
          {
            return add_string_range_to_nfa(state_pool, new_bottom_state,
                    to_state, first_bytes + 1, last_bytes + 1, byte_count - 1);
          }

        the_verdict = add_string_range_to_nfa(state_pool, new_bottom_state,
                to_state, first_bytes + 1, &(high_buffer[0]), byte_count - 1);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    separate_high_needed = FALSE;
    for (byte_num = 1; byte_num < byte_count; ++byte_num)
      {
        assert(((unsigned char)(last_bytes[byte_num])) <= 0xfd);
        if (((unsigned char)(last_bytes[byte_num])) < 0xfd)
          {
            separate_high_needed = TRUE;
            break;
          }
      }

    if (!separate_high_needed)
        mid_high = last_0;
    else
        mid_high = last_0 - 1;

    if (mid_low <= mid_high)
      {
        size_t new_mid_state;
        verdict the_verdict;

        new_mid_state = state_pool->element_count;

        the_verdict = nfa_state_aa_append(state_pool, state_buffer);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        the_verdict = add_link_to_nfa(state_pool, from_state, new_mid_state,
                                      mid_low, mid_high);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        the_verdict = add_string_range_to_nfa(state_pool, new_mid_state,
                to_state, &(low_buffer[0]), &(high_buffer[0]), byte_count - 1);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    if (separate_high_needed)
      {
        size_t new_top_state;
        verdict the_verdict;

        new_top_state = state_pool->element_count;

        the_verdict = nfa_state_aa_append(state_pool, state_buffer);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        the_verdict = add_link_to_nfa(state_pool, from_state, new_top_state,
                                      last_0, last_0);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        if (first_0 == last_0)
          {
            return add_string_range_to_nfa(state_pool, new_top_state,
                    to_state, first_bytes + 1, last_bytes + 1, byte_count - 1);
          }

        the_verdict = add_string_range_to_nfa(state_pool, new_top_state,
                to_state, &(low_buffer[0]), last_bytes + 1, byte_count - 1);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

static unsigned long range_character_specification_to_code_point(
        const char *pattern_chars, size_t *bytes_in_character_specification,
        const code_point_range **ranges, size_t *range_count,
        regular_expression_error *error)
  {
    size_t position;
    int character_bytes;

    assert(pattern_chars != NULL);
    assert(bytes_in_character_specification != NULL);
    assert(ranges != NULL);
    assert(range_count != NULL);
    assert(error != NULL);

    position = 0;

    if (pattern_chars[position] == '\\')
      {
        char escaped_byte;

        ++position;

        escaped_byte = pattern_chars[position];

        if ((escaped_byte == 'x') || (escaped_byte == 'X'))
          {
            *ranges = NULL;
            return hex_character_specification_to_code_point(pattern_chars,
                    bytes_in_character_specification, error);
          }

        if (((escaped_byte >= 'a') && (escaped_byte <= 'z')) ||
            ((escaped_byte >= 'A') && (escaped_byte <= 'Z')))
          {
            size_t letter_range_count;
            const code_point_range *letter_ranges;

            *bytes_in_character_specification = 2;

            letter_ranges = ranges_for_escaped_letter(escaped_byte,
                                                      &letter_range_count);
            if (letter_ranges == NULL)
              {
                *ranges = NULL;
                *error = REE_BAD_ESCAPE_LETTER;
                return 0;
              }

            assert(letter_range_count > 0);

            if ((letter_range_count == 1) &&
                (letter_ranges[0].first == letter_ranges[0].last))
              {
                assert(letter_ranges[0].first != 0);
                return letter_ranges[0].first;
              }

            *ranges = letter_ranges;
            *range_count = letter_range_count;

            return 0;
          }
      }

    if (pattern_chars[position] == 0)
      {
        *ranges = NULL;
        *error = REE_UNTERMINATED_BRACKET;
        return 0;
      }

    character_bytes = validate_utf8_character(&(pattern_chars[position]));
    assert(character_bytes >= 1);
    assert(character_bytes <= 4);

    *bytes_in_character_specification = position + character_bytes;

    return utf8_to_code_point(&(pattern_chars[position]));
  }

static unsigned long hex_character_specification_to_code_point(
        const char *pattern_chars, size_t *bytes_in_character_specification,
        regular_expression_error *error)
  {
    size_t char_num;
    unsigned long code_point;

    assert(pattern_chars != NULL);
    assert(bytes_in_character_specification != NULL);
    assert(error != NULL);

    assert(pattern_chars[0] == '\\');
    assert((pattern_chars[1] == 'x') || (pattern_chars[1] == 'X'));

    char_num = 2;
    code_point = 0;

    while (TRUE)
      {
        char inchar;
        unsigned long digit_value;

        inchar = pattern_chars[char_num];

        if ((inchar >= '0') && (inchar <= '9'))
          {
            digit_value = inchar - '0';
          }
        else if ((inchar >= 'a') && (inchar <= 'f'))
          {
            digit_value = (inchar - 'a') + 0xa;
          }
        else if ((inchar >= 'A') && (inchar <= 'F'))
          {
            digit_value = (inchar - 'A') + 0xa;
          }
        else
          {
            if (char_num == 2)
              {
                *error = REE_HEX_NO_DIGITS;
                return 0;
              }

            if (code_point == 0)
              {
                *error = REE_HEX_ZERO;
                return 0;
              }
            else if ((code_point >= 0xd800) && (code_point < 0xe000))
              {
                *error = REE_HEX_UTF_16_SURROGATE;
                return 0;
              }
            return code_point;
          }

        assert(digit_value < 16);

        code_point = ((code_point * 16) + digit_value);

        if (code_point > 0x10ffff)
          {
            *error = REE_HEX_TOO_LARGE;
            return 0;
          }

        ++char_num;
      }
  }

static const code_point_range *ranges_for_escaped_letter(char letter,
                                                         size_t *range_count)
  {
    assert(((letter >= 'a') && (letter <= 'z')) ||
           ((letter >= 'A') && (letter <= 'Z')));
    assert(range_count != NULL);

    switch (letter)
      {
        case 'a':
          {
            static const code_point_range a_ranges[1] = {{'\a', '\a'}};

            *range_count = 1;
            return &(a_ranges[0]);
          }
        case 'b':
          {
            static const code_point_range b_ranges[1] = {{'\b', '\b'}};

            *range_count = 1;
            return &(b_ranges[0]);
          }
        case 'f':
          {
            static const code_point_range f_ranges[1] = {{'\f', '\f'}};

            *range_count = 1;
            return &(f_ranges[0]);
          }
        case 'n':
          {
            static const code_point_range n_ranges[1] = {{'\n', '\n'}};

            *range_count = 1;
            return &(n_ranges[0]);
          }
        case 'r':
          {
            static const code_point_range r_ranges[1] = {{'\r', '\r'}};

            *range_count = 1;
            return &(r_ranges[0]);
          }
        case 't':
          {
            static const code_point_range t_ranges[1] = {{'\t', '\t'}};

            *range_count = 1;
            return &(t_ranges[0]);
          }
        case 'v':
          {
            static const code_point_range v_ranges[1] = {{'\v', '\v'}};

            *range_count = 1;
            return &(v_ranges[0]);
          }
        case 's':
          {
            static const code_point_range s_ranges[6] =
              {
                {' ', ' '}, {'\t', '\t'}, {'\r', '\r'}, {'\n', '\n'},
                {'\v', '\v'}, {'\f', '\f'}
              };

            *range_count = 6;
            return &(s_ranges[0]);
          }
        case 'd':
          {
            static const code_point_range d_ranges[1] = {{'0', '9'}};

            *range_count = 1;
            return &(d_ranges[0]);
          }
        case 'h':
          {
            static const code_point_range h_ranges[3] =
              {{'0', '9'}, {'a', 'f'}, {'A', 'F'}};

            *range_count = 3;
            return &(h_ranges[0]);
          }
        default:
          {
            return NULL;
          }
      }
  }

static verdict do_repeat_and_skip(nfa_state_aa *state_pool, boolean skip,
        boolean repeat, size_t last_end_state, size_t *current_end_state)
  {
    size_t repeat_state;

    assert(state_pool != NULL);

    if (!skip)
      {
        repeat_state = last_end_state;
      }
    else
      {
        size_t new_state;
        verdict the_verdict;

        new_state = state_pool->element_count;

        the_verdict = nfa_state_aa_append(state_pool,
                                          state_pool->array[last_end_state]);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            return the_verdict;
          }

        state_pool->array[last_end_state].link_first_char[0] = 0;
        state_pool->array[last_end_state].link_last_char[0] = 0;
        state_pool->array[last_end_state].next[0] = new_state + 1;
        state_pool->array[last_end_state].link_first_char[1] = 0;
        state_pool->array[last_end_state].link_last_char[1] = 0;
        state_pool->array[last_end_state].next[1] = (*current_end_state) + 1;

        repeat_state = new_state;
      }

    if (repeat)
      {
        size_t new_end_state;
        nfa_state state_buffer;
        verdict the_verdict;

        new_end_state = state_pool->element_count;

        state_buffer.next[0] = 0;
        state_buffer.next[1] = 0;

        the_verdict = nfa_state_aa_append(state_pool, state_buffer);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;

        assert(state_pool->array[*current_end_state].next[0] == 0);
        assert(state_pool->array[*current_end_state].next[1] == 0);

        state_pool->array[*current_end_state].link_first_char[0] = 0;
        state_pool->array[*current_end_state].link_last_char[0] = 0;
        state_pool->array[*current_end_state].next[0] =
                repeat_state + 1;
        state_pool->array[*current_end_state].link_first_char[1] = 0;
        state_pool->array[*current_end_state].link_last_char[1] = 0;
        state_pool->array[*current_end_state].next[1] =
                new_end_state + 1;

        *current_end_state = new_end_state;
      }

    return MISSION_ACCOMPLISHED;
  }

static verdict duplicate_nfa_portion(nfa *duplicate,
        size_t original_state_count, nfa_state *original_states,
        size_t original_starting_point, size_t original_end)
  {
    size_t *state_old_to_new;
    size_t *state_new_to_old;
    size_t *to_do_array;
    size_t to_do_count;
    size_t state_num;
    size_t new_state_count;
    nfa_state *new_state_array;

    assert(duplicate != NULL);
    assert(original_state_count > 0);
    assert(original_states != NULL);
    assert(original_starting_point < original_state_count);
    assert(original_end < original_state_count);

    state_old_to_new = MALLOC_ARRAY(size_t, original_state_count);
    if (state_old_to_new == NULL)
        return MISSION_FAILED;

    state_new_to_old = MALLOC_ARRAY(size_t, original_state_count);
    if (state_new_to_old == NULL)
      {
        free(state_old_to_new);
        return MISSION_FAILED;
      }

    to_do_array = MALLOC_ARRAY(size_t, original_state_count);
    if (to_do_array == NULL)
      {
        free(state_new_to_old);
        free(state_old_to_new);
        return MISSION_FAILED;
      }

    for (state_num = 0; state_num < original_state_count; ++state_num)
        state_old_to_new[state_num] = original_state_count;

    to_do_count = 0;
    new_state_count = 0;

    state_old_to_new[original_starting_point] = new_state_count;
    state_new_to_old[new_state_count] = original_starting_point;
    to_do_array[to_do_count] = original_starting_point;
    ++to_do_count;
    ++new_state_count;

    while (to_do_count > 0)
      {
        size_t current_old_num;
        size_t link_num;

        --to_do_count;
        current_old_num = to_do_array[to_do_count];

        for (link_num = 0; link_num < 2; ++link_num)
          {
            size_t link_target;

            link_target = original_states[current_old_num].next[link_num];
            if (link_target == 0)
                continue;
            --link_target;

            assert(state_old_to_new[link_target] <= original_state_count);
            if (state_old_to_new[link_target] < original_state_count)
                continue;

            state_old_to_new[link_target] = new_state_count;
            state_new_to_old[new_state_count] = link_target;
            to_do_array[to_do_count] = link_target;
            ++to_do_count;
            ++new_state_count;
          }
      }

    free(to_do_array);

    assert(state_old_to_new[original_end] <= original_state_count);
    if (state_old_to_new[original_end] == original_state_count)
        ++new_state_count;

    assert(new_state_count > 0);
    new_state_array = MALLOC_ARRAY(nfa_state, new_state_count);
    if (new_state_array == NULL)
      {
        free(state_new_to_old);
        free(state_old_to_new);
        return MISSION_FAILED;
      }

    if (state_old_to_new[original_end] == original_state_count)
      {
        nfa_state *new_state;
        size_t link_num;

        --new_state_count;

        new_state = &(new_state_array[new_state_count]);

        for (link_num = 0; link_num < 2; ++link_num)
            new_state->next[link_num] = 0;
      }

    for (state_num = 0; state_num < new_state_count; ++state_num)
      {
        nfa_state *new_state;
        size_t old_state_num;
        nfa_state *old_state;
        size_t link_num;

        new_state = &(new_state_array[state_num]);

        old_state_num = state_new_to_old[state_num];
        assert(old_state_num < original_state_count);
        assert(state_old_to_new[old_state_num] == state_num);
        old_state = &(original_states[old_state_num]);

        for (link_num = 0; link_num < 2; ++link_num)
          {
            size_t old_next;
            size_t new_next;

            old_next = old_state->next[link_num];
            if (old_next == 0)
              {
                new_state->next[link_num] = 0;
                continue;
              }
            --old_next;
            assert(old_next < original_state_count);

            new_next = state_old_to_new[old_next];
            assert(new_next < new_state_count);
            new_state->next[link_num] = new_next + 1;

            new_state->link_first_char[link_num] =
                    old_state->link_first_char[link_num];
            new_state->link_last_char[link_num] =
                    old_state->link_last_char[link_num];
          }
      }

    if (state_old_to_new[original_end] == original_state_count)
        ++new_state_count;

    duplicate->state_count = new_state_count;
    duplicate->start_state = 0;
    if (state_old_to_new[original_end] == original_state_count)
        duplicate->end_state = (new_state_count - 1);
    else
        duplicate->end_state = state_old_to_new[original_end];
    duplicate->state_table = new_state_array;

    free(state_new_to_old);
    free(state_old_to_new);

    return MISSION_ACCOMPLISHED;
  }

static verdict add_nfa_copy(nfa_state_aa *state_pool, size_t copy_start_state,
                            nfa *to_copy, size_t *copy_end)
  {
    size_t source_state_count;
    size_t source_start_state;
    size_t original_target_state_count;
    size_t source_state_num;
    size_t source_end_state;

    assert(state_pool != NULL);
    assert(copy_start_state < state_pool->element_count);
    assert(to_copy != NULL);
    assert(copy_end != NULL);

    source_state_count = to_copy->state_count;
    source_start_state = to_copy->start_state;
    original_target_state_count = state_pool->element_count;

    for (source_state_num = 0; source_state_num < source_state_count;
         ++source_state_num)
      {
        nfa_state *source_state;
        nfa_state state_buffer;
        nfa_state *target_state;
        size_t link_num;

        source_state = &(to_copy->state_table[source_state_num]);

        if (source_state_num == source_start_state)
          {
            target_state = &(state_pool->array[copy_start_state]);
            assert(target_state->next[0] == 0);
            assert(target_state->next[1] == 0);
          }
        else
          {
            target_state = &state_buffer;
          }

        for (link_num = 0; link_num < 2; ++link_num)
          {
            size_t source_next;
            size_t target_next;

            source_next = source_state->next[link_num];
            if (source_next == 0)
              {
                target_state->next[link_num] = 0;
                continue;
              }
            --source_next;

            if (source_next == source_start_state)
                target_next = copy_start_state;
            else if (source_next < source_start_state)
                target_next = original_target_state_count + source_next;
            else
                target_next = original_target_state_count + (source_next - 1);

            target_state->next[link_num] = target_next;
            target_state->link_first_char[link_num] =
                    source_state->link_first_char[link_num];
            target_state->link_last_char[link_num] =
                    source_state->link_last_char[link_num];
          }

        if (source_state_num != source_start_state)
          {
            verdict the_verdict;

            the_verdict = nfa_state_aa_append(state_pool, state_buffer);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;
          }
      }

    source_end_state = to_copy->end_state;
    if (source_end_state > source_start_state)
        --source_end_state;

    *copy_end = original_target_state_count + source_end_state;

    return MISSION_ACCOMPLISHED;
  }

static boolean dfa_cache_matches(dfa_cache *the_dfa_cache, nfa *the_nfa,
        const char *target_string, boolean at_start_of_target,
        boolean exact_match, boolean start_limited, size_t start_within,
        boolean *error, size_t *longest_match)
  {
    boolean match_found;
    size_t current_dfa_state_num;
    const char *follow;

    assert(the_dfa_cache != NULL);
    assert(the_nfa != NULL);
    assert(target_string != NULL);
    assert(error != NULL);

    if (exact_match && !at_start_of_target)
      {
        *error = FALSE;
        return FALSE;
      }

    match_found = FALSE;

    current_dfa_state_num = (at_start_of_target ? 0 : 1);

    follow = target_string;

    while (TRUE)
      {
        unsigned char next_char;
        boolean omit_start;
        size_t next_dfa_state_num;

        assert(current_dfa_state_num <
               the_dfa_cache->state_pool.element_count);

        if (the_dfa_cache->state_pool.array[current_dfa_state_num].
                    contains_end)
          {
            if (longest_match != NULL)
              {
                match_found = TRUE;
                *longest_match = (follow - target_string);
              }
            else
              {
                *error = FALSE;
                return TRUE;
              }
          }

        next_char = (unsigned char)*follow;

        if (next_char == 0)
            next_char = 0xff;

        omit_start = (exact_match ||
                      (start_limited &&
                       (start_within <= (follow - target_string))));

        next_dfa_state_num = dfa_cache_find_or_make_successor(the_dfa_cache,
                current_dfa_state_num, next_char, omit_start);
        if (next_dfa_state_num == 0)
          {
            *error = TRUE;
            return FALSE;
          }

        if (next_dfa_state_num == 1)
          {
            *error = FALSE;
            return match_found;
          }

        assert(next_dfa_state_num >= 2);
        current_dfa_state_num = next_dfa_state_num - 2;

        dfa_cache_mark_state_most_recently_used(the_dfa_cache,
                                                current_dfa_state_num);

        if (next_char == 0xff)
          {
            *error = FALSE;
            if (the_dfa_cache->state_pool.array[current_dfa_state_num].
                        contains_end)
              {
                if (longest_match != NULL)
                    *longest_match = (follow - target_string);
                match_found = TRUE;
              }
            return match_found;
          }

        ++follow;
      }
  }

static size_t dfa_cache_find_or_make_successor(dfa_cache *the_dfa_cache,
        size_t current_dfa_state_num, unsigned char next_char,
        boolean omit_start)
  {
    dfa_state *current_dfa_state;
    size_t block_num;
    dfa_state_successor *successor_block;
    size_t next_dfa_state_num;
    unsigned char range_low;
    unsigned char range_high;
    dfa_state *next_dfa_state;
    unsigned char range_element_num;

    assert(current_dfa_state_num < the_dfa_cache->state_pool.element_count);
    current_dfa_state =
            &(the_dfa_cache->state_pool.array[current_dfa_state_num]);

    block_num = (omit_start ? 0 : 1);

    successor_block = &(current_dfa_state->successors[block_num][0]);

    next_dfa_state_num = successor_block[next_char].successor_num;

    if (next_dfa_state_num != 0)
        return next_dfa_state_num;

    next_dfa_state_num = get_dfa_state(the_dfa_cache, current_dfa_state,
            next_char, !omit_start, &range_low, &range_high);

    /*
     * The call to get_dfa_state might have expanded the state_pool, so the
     * current_dfa_state and successor_block pointers might be pointing into
     * freed memory.  So now we'll set those pointers again to correct that
     * problem.
     */
    current_dfa_state =
            &(the_dfa_cache->state_pool.array[current_dfa_state_num]);
    successor_block = &(current_dfa_state->successors[block_num][0]);

    if (next_dfa_state_num < 2)
        return next_dfa_state_num;

    assert((next_dfa_state_num - 2) < the_dfa_cache->state_pool.element_count);
    next_dfa_state =
            &(the_dfa_cache->state_pool.array[(next_dfa_state_num - 2)]);

    assert(range_low <= next_char);
    assert(range_high >= next_char);

    range_element_num = range_low;
    while (TRUE)
      {
        dfa_state_successor *this_successor_link;

        this_successor_link = &(successor_block[range_element_num]);
        if (this_successor_link->successor_num == 0)
          {
            size_t next;

            this_successor_link->successor_num = next_dfa_state_num;
            next = next_dfa_state->first_predecessor_incoming_link;
            this_successor_link->next_for_successor = next;
            this_successor_link->previous_for_successor = 512;
            if (next != 512)
              {
                current_dfa_state->successors[next / 256][next % 256].
                        previous_for_successor =
                                ((block_num * 256) + range_element_num);
              }
            next_dfa_state->first_predecessor_incoming_link =
                    ((block_num * 256) + range_element_num);
          }

        if (range_element_num == range_high)
            break;

        ++range_element_num;
      }

    return next_dfa_state_num;
  }

static void dfa_cache_mark_state_most_recently_used(dfa_cache *the_dfa_cache,
                                                    size_t state_num)
  {
    dfa_state *state_array;
    size_t second_state_num;
    size_t old_next;
    size_t old_previous;

    assert(the_dfa_cache != NULL);
    assert(state_num < the_dfa_cache->state_pool.element_count);

    if (state_num < 2)
        return;

    if (the_dfa_cache->most_recently_used_dfa_state == state_num)
        return;

    state_array = the_dfa_cache->state_pool.array;
    assert(state_array != NULL);

    second_state_num = the_dfa_cache->most_recently_used_dfa_state;
    old_next = state_array[state_num].next_most_recently_used_dfa_state;
    old_previous = state_array[state_num].next_least_recently_used_dfa_state;

    the_dfa_cache->most_recently_used_dfa_state = state_num;
    assert(state_array[second_state_num].next_least_recently_used_dfa_state ==
           0);
    state_array[second_state_num].next_least_recently_used_dfa_state =
            state_num;

    state_array[state_num].next_most_recently_used_dfa_state =
            second_state_num;
    state_array[state_num].next_least_recently_used_dfa_state = 0;

    if (old_next == 0)
      {
        assert(the_dfa_cache->least_recently_used_dfa_state == state_num);
        the_dfa_cache->least_recently_used_dfa_state = old_previous;
      }
    else
      {
        assert(state_array[old_next].next_least_recently_used_dfa_state ==
               state_num);
        state_array[old_next].next_least_recently_used_dfa_state =
                old_previous;
      }

    assert(old_previous != 0);
    assert(state_array[old_previous].next_most_recently_used_dfa_state ==
           state_num);
    state_array[old_previous].next_most_recently_used_dfa_state = old_next;
  }

static size_t get_dfa_state(dfa_cache *the_dfa_cache, dfa_state *predecessor,
        unsigned char new_byte, boolean start_also, unsigned char *range_low,
        unsigned char *range_high)
  {
    size_t new_state_num;
    verdict the_verdict;
    size_t bit_vector_block_count;
    size_t *vector1;
    size_t cached_state_count;
    size_t cached_state_num;

    assert(the_dfa_cache != NULL);
    assert(predecessor != NULL);
    assert(range_low != NULL);
    assert(range_high != NULL);

    new_state_num = the_dfa_cache->used_dfa_state_count;
    ++(the_dfa_cache->used_dfa_state_count);
    if (new_state_num >= DFA_CACHE_MAX_STATES)
      {
        dfa_state *new_state;
        size_t next_least_state_num;

        --(the_dfa_cache->used_dfa_state_count);
        new_state_num = the_dfa_cache->least_recently_used_dfa_state;
        assert(new_state_num > 1);
        assert(new_state_num < the_dfa_cache->used_dfa_state_count);

        assert(new_state_num < the_dfa_cache->state_pool.element_count);
        new_state = &(the_dfa_cache->state_pool.array[new_state_num]);

        next_least_state_num = new_state->next_least_recently_used_dfa_state;
        the_dfa_cache->least_recently_used_dfa_state = next_least_state_num;
        assert(the_dfa_cache->state_pool.array[next_least_state_num].
                       next_most_recently_used_dfa_state == new_state_num);
        the_dfa_cache->state_pool.array[next_least_state_num].
                next_most_recently_used_dfa_state = 0;
        assert(new_state->next_most_recently_used_dfa_state == 0);

        clear_used_dfa_state(the_dfa_cache, new_state_num);
      }
    else if (new_state_num >= the_dfa_cache->state_pool.element_count)
      {
        size_t predecessor_num;
        dfa_state state_buffer;
        verdict the_verdict;

        assert(new_state_num == the_dfa_cache->state_pool.element_count);

        assert(predecessor >= &(the_dfa_cache->state_pool.array[0]));
        predecessor_num =
                (predecessor - &(the_dfa_cache->state_pool.array[0]));
        assert(predecessor_num < the_dfa_cache->state_pool.element_count);

        the_verdict = dfa_state_aa_append(&(the_dfa_cache->state_pool),
                                          state_buffer);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return 0;

        predecessor = &(the_dfa_cache->state_pool.array[predecessor_num]);

        the_verdict = initialize_dfa_state(the_dfa_cache, new_state_num);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            --(the_dfa_cache->state_pool.element_count);
            return 0;
          }
      }

    assert(new_state_num < the_dfa_cache->state_pool.element_count);

    the_verdict = build_new_dfa_state(the_dfa_cache, predecessor,
            new_state_num, new_byte, start_also, range_low, range_high);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        clear_and_make_state_lru(the_dfa_cache, new_state_num);
        return 0;
      }

    if (the_dfa_cache->state_pool.array[new_state_num].first_element_num == 0)
      {
        clear_and_make_state_lru(the_dfa_cache, new_state_num);
        return 1;
      }

    bit_vector_block_count =
            (((the_dfa_cache->nfa->state_count) + ((sizeof(size_t) * 8) - 1)) /
             (sizeof(size_t) * 8));

    cached_state_count = the_dfa_cache->used_dfa_state_count;
    assert(cached_state_count >= 2);

    vector1 =
            the_dfa_cache->state_pool.array[new_state_num].elements_bit_vector;
    assert(vector1 != NULL);

    for (cached_state_num = 0; cached_state_num < cached_state_count;
         ++cached_state_num)
      {
        size_t *vector2;
        size_t bit_vector_block_num;

        if (cached_state_num == new_state_num)
            continue;

        vector2 = the_dfa_cache->state_pool.array[cached_state_num].
                elements_bit_vector;
        assert(vector2 != NULL);

        for (bit_vector_block_num = 0;
             bit_vector_block_num < bit_vector_block_count;
             ++bit_vector_block_num)
          {
            if (vector1[bit_vector_block_num] != vector2[bit_vector_block_num])
                break;
          }
        if (bit_vector_block_num < bit_vector_block_count)
            continue;

        clear_and_make_state_lru(the_dfa_cache, new_state_num);

        return cached_state_num + 2;
      }

    dfa_cache_insert_state_as_most_recently_used(the_dfa_cache, new_state_num);

    return new_state_num + 2;
  }

static void clear_used_dfa_state(dfa_cache *the_dfa_cache,
                                 size_t to_clear_state_num)
  {
    dfa_state *new_state;
    size_t follow;
    size_t block_num;
    size_t element_num;

    assert(the_dfa_cache != NULL);
    assert(to_clear_state_num > 1);
    assert(to_clear_state_num < the_dfa_cache->used_dfa_state_count);

    assert(to_clear_state_num < the_dfa_cache->state_pool.element_count);
    new_state = &(the_dfa_cache->state_pool.array[to_clear_state_num]);

    follow = new_state->first_predecessor_incoming_link;
    while (follow != 512)
      {
        dfa_state_successor *item;
        size_t next;

        item = &(new_state->successors[follow / 256][follow % 256]);
        next = item->next_for_successor;
        assert(item->successor_num == to_clear_state_num + 2);
        item->next_for_successor = 512;
        item->previous_for_successor = 512;
        item->successor_num = 0;
        follow = next;
      }

    new_state->first_predecessor_incoming_link = 512;

    for (block_num = 0; block_num < 2; ++block_num)
      {
        size_t byte_num;

        for (byte_num = 0; byte_num < 256; ++byte_num)
          {
            dfa_state_successor *item;
            size_t next;
            size_t previous;

            item = &(new_state->successors[block_num][byte_num]);

            next = item->next_for_successor;
            previous = item->previous_for_successor;

            if (item->successor_num < 2)
              {
                assert(next == 512);
                assert(previous == 512);
              }
            else
              {
                if (next != 512)
                  {
                    dfa_state_successor *next_pointer;

                    next_pointer =
                            &(new_state->successors[next / 256][next % 256]);
                    assert(next_pointer->previous_for_successor ==
                           ((block_num * 256) + byte_num));
                    next_pointer->previous_for_successor = previous;
                    item->next_for_successor = 512;
                  }

                if (previous != 512)
                  {
                    dfa_state_successor *previous_pointer;

                    previous_pointer = &(new_state->successors[previous / 256][
                                                 previous % 256]);
                    assert(previous_pointer->next_for_successor ==
                           ((block_num * 256) + byte_num));
                    previous_pointer->next_for_successor = next;
                    item->previous_for_successor = 512;
                  }
                else
                  {
                    dfa_state *successor_state;

                    successor_state =
                            &(the_dfa_cache->state_pool.array[
                                      item->successor_num - 2]);
                    assert(successor_state->first_predecessor_incoming_link ==
                           ((block_num * 256) + byte_num));
                    successor_state->first_predecessor_incoming_link = next;
                  }
              }

            item->successor_num = 0;
          }
      }

    assert(new_state->elements_bit_vector != NULL);

    element_num = new_state->first_element_num;

    if (element_num > 0)
      {
        while (TRUE)
          {
            dfa_state_element *element_data;
            size_t nfa_state_num;
            size_t bit_block_num;
            size_t bit_within_block_num;

            assert((element_num - 1) <
                   the_dfa_cache->element_pool.element_count);
            element_data =
                    &(the_dfa_cache->element_pool.array[element_num - 1]);

            nfa_state_num = element_data->nfa_state_num;
            bit_block_num = (nfa_state_num / (sizeof(size_t) * 8));
            bit_within_block_num = (nfa_state_num % (sizeof(size_t) * 8));
            assert((new_state->elements_bit_vector[bit_block_num] &
                    (((size_t)0x1) << bit_within_block_num)) != 0);
            new_state->elements_bit_vector[bit_block_num] &=
                    ~(((size_t)0x1) << bit_within_block_num);

            element_num = element_data->next_element;

            if (element_num == 0)
              {
                element_data->next_element = the_dfa_cache->first_free_element;
                the_dfa_cache->first_free_element =
                        new_state->first_element_num;
                break;
              }
          }
        new_state->first_element_num = 0;
      }
  }

static verdict initialize_dfa_state(dfa_cache *the_dfa_cache,
                                    size_t to_initialize_state_num)
  {
    dfa_state *new_state;
    size_t block_num;
    size_t nfa_state_count;
    size_t bit_vector_block_count;
    size_t *elements_bit_vector;
    size_t bit_vector_block_num;

    assert(the_dfa_cache != NULL);
    assert(to_initialize_state_num < the_dfa_cache->used_dfa_state_count);

    assert(to_initialize_state_num < the_dfa_cache->state_pool.element_count);
    new_state = &(the_dfa_cache->state_pool.array[to_initialize_state_num]);

    new_state->first_predecessor_incoming_link = 512;

    for (block_num = 0; block_num < 2; ++block_num)
      {
        size_t byte_num;

        for (byte_num = 0; byte_num < 256; ++byte_num)
          {
            dfa_state_successor *item;

            item = &(new_state->successors[block_num][byte_num]);

            item->next_for_successor = 512;
            item->previous_for_successor = 512;
            item->successor_num = 0;
          }
      }

    nfa_state_count = the_dfa_cache->nfa->state_count;
    assert(nfa_state_count > 0);

    bit_vector_block_count =
            ((nfa_state_count + ((sizeof(size_t) * 8) - 1)) /
             (sizeof(size_t) * 8));

    elements_bit_vector = MALLOC_ARRAY(size_t, bit_vector_block_count);
    if (elements_bit_vector == NULL)
        return MISSION_FAILED;

    for (bit_vector_block_num = 0;
         bit_vector_block_num < bit_vector_block_count; ++bit_vector_block_num)
      {
        elements_bit_vector[bit_vector_block_num] = 0;
      }

    new_state->elements_bit_vector = elements_bit_vector;

    new_state->first_element_num = 0;

    new_state->next_most_recently_used_dfa_state = 0;
    new_state->next_least_recently_used_dfa_state = 0;

    return MISSION_ACCOMPLISHED;
  }

static verdict build_new_dfa_state(dfa_cache *the_dfa_cache,
        dfa_state *old_dfa_state, size_t new_dfa_state_num,
        unsigned char new_byte, boolean start_also, unsigned char *range_low,
        unsigned char *range_high)
  {
    dfa_state *new_dfa_state;
    size_t lower_bound;
    size_t upper_bound;
    size_t old_element_num;

    assert(the_dfa_cache != NULL);
    assert(old_dfa_state != NULL);
    assert(new_dfa_state_num < the_dfa_cache->used_dfa_state_count);
    assert(range_low != NULL);
    assert(range_high != NULL);

    new_dfa_state = &(the_dfa_cache->state_pool.array[new_dfa_state_num]);

    assert(new_dfa_state->first_element_num == 0);

    new_dfa_state->contains_end = FALSE;

    lower_bound = 0x0;
    upper_bound = 0xff;

    old_element_num = old_dfa_state->first_element_num;
    while (old_element_num > 0)
      {
        dfa_state_element *old_element_data;
        verdict the_verdict;

        assert((old_element_num - 1) <
               the_dfa_cache->element_pool.element_count);
        old_element_data =
                &(the_dfa_cache->element_pool.array[old_element_num - 1]);
        old_element_num = old_element_data->next_element;

        the_verdict = add_nfa_successors_to_dfa_state(the_dfa_cache,
                new_dfa_state, old_element_data->nfa_state_num, new_byte,
                &lower_bound, &upper_bound);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    if (start_also)
      {
        verdict the_verdict;

        the_verdict = add_nfa_state_and_null_successors_to_dfa_state(
                the_dfa_cache, new_dfa_state, the_dfa_cache->nfa->start_state);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    *range_low = lower_bound;
    *range_high = upper_bound;

    return MISSION_ACCOMPLISHED;
  }

static verdict add_nfa_successors_to_dfa_state(dfa_cache *the_dfa_cache,
        dfa_state *the_dfa_state, size_t nfa_state_num, unsigned char new_byte,
        size_t *lower_bound, size_t *upper_bound)
  {
    nfa_state *the_nfa_state;
    size_t link_num;

    assert(the_dfa_cache != NULL);
    assert(the_dfa_state != NULL);
    assert(the_dfa_cache->nfa != NULL);
    assert(nfa_state_num < the_dfa_cache->nfa->state_count);
    assert(lower_bound != NULL);
    assert(upper_bound != NULL);
    assert(the_dfa_cache->nfa->state_table != NULL);

    the_nfa_state = &(the_dfa_cache->nfa->state_table[nfa_state_num]);

    for (link_num = 0; link_num < 2; ++link_num)
      {
        size_t next_nfa_state_num;
        size_t bit_vector_block_num;
        size_t bit_vector_bit_within_block_num;
        verdict the_verdict;

        next_nfa_state_num = the_nfa_state->next[link_num];
        if (next_nfa_state_num == 0)
            continue;
        --next_nfa_state_num;
        assert(next_nfa_state_num < the_dfa_cache->nfa->state_count);

        if (new_byte < the_nfa_state->link_first_char[link_num])
          {
            if (*upper_bound > (the_nfa_state->link_first_char[link_num] - 1))
                *upper_bound = (the_nfa_state->link_first_char[link_num] - 1);
            continue;
          }
        if (new_byte > the_nfa_state->link_last_char[link_num])
          {
            if (*lower_bound < (the_nfa_state->link_last_char[link_num] + 1))
                *lower_bound = (the_nfa_state->link_last_char[link_num] + 1);
            continue;
          }

        bit_vector_block_num = (next_nfa_state_num / (sizeof(size_t) * 8));
        bit_vector_bit_within_block_num =
                (next_nfa_state_num % (sizeof(size_t) * 8));

        if ((the_dfa_state->elements_bit_vector[bit_vector_block_num] &
             (((size_t)0x1) << bit_vector_bit_within_block_num)) != 0)
          {
            continue;
          }

        if (*lower_bound < the_nfa_state->link_first_char[link_num])
            *lower_bound = the_nfa_state->link_first_char[link_num];
        if (*upper_bound > the_nfa_state->link_last_char[link_num])
            *upper_bound = the_nfa_state->link_last_char[link_num];

        the_verdict = add_nfa_state_and_null_successors_to_dfa_state(
                the_dfa_cache, the_dfa_state, next_nfa_state_num);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }

static verdict add_nfa_state_and_null_successors_to_dfa_state(
        dfa_cache *the_dfa_cache, dfa_state *the_dfa_state,
        size_t nfa_state_num)
  {
    size_t bit_vector_block_num;
    size_t bit_vector_bit_within_block_num;
    size_t new_element_num;
    size_t dummy_lower_bound;
    size_t dummy_upper_bound;

    bit_vector_block_num = (nfa_state_num / (sizeof(size_t) * 8));
    bit_vector_bit_within_block_num = (nfa_state_num % (sizeof(size_t) * 8));

    if ((the_dfa_state->elements_bit_vector[bit_vector_block_num] &
         (((size_t)0x1) << bit_vector_bit_within_block_num)) != 0)
      {
        return MISSION_ACCOMPLISHED;
      }

    the_dfa_state->elements_bit_vector[bit_vector_block_num] |=
            (((size_t)0x1) << bit_vector_bit_within_block_num);

    if (nfa_state_num == the_dfa_cache->nfa->end_state)
        the_dfa_state->contains_end = TRUE;

    new_element_num = the_dfa_cache->first_free_element;

    if (new_element_num == 0)
      {
        dfa_state_element element_buffer;
        verdict the_verdict;

        element_buffer.next_element = the_dfa_state->first_element_num;
        element_buffer.nfa_state_num = nfa_state_num;

        new_element_num = the_dfa_cache->element_pool.element_count + 1;

        the_verdict = dfa_state_element_aa_append(
                &(the_dfa_cache->element_pool), element_buffer);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }
    else
      {
        dfa_state_element *new_element_data;

        assert((new_element_num - 1) <
               the_dfa_cache->element_pool.element_count);
        new_element_data =
                &(the_dfa_cache->element_pool.array[new_element_num - 1]);
        the_dfa_cache->first_free_element = new_element_data->next_element;
        new_element_data->next_element = the_dfa_state->first_element_num;
        new_element_data->nfa_state_num = nfa_state_num;
      }

    the_dfa_state->first_element_num = new_element_num;

    dummy_lower_bound = 0;
    dummy_upper_bound = 0;
    return add_nfa_successors_to_dfa_state(the_dfa_cache, the_dfa_state,
            nfa_state_num, 0, &dummy_lower_bound, &dummy_upper_bound);
  }

static void clear_and_make_state_lru(dfa_cache *the_dfa_cache,
                                     size_t dfa_state_num)
  {
    dfa_state *state_data;
    size_t next_least_state_num;

    assert(the_dfa_cache != NULL);
    assert(dfa_state_num > 1);
    assert(dfa_state_num < the_dfa_cache->used_dfa_state_count);
    assert(dfa_state_num < the_dfa_cache->state_pool.element_count);

    clear_used_dfa_state(the_dfa_cache, dfa_state_num);

    state_data = &(the_dfa_cache->state_pool.array[dfa_state_num]);

    next_least_state_num = the_dfa_cache->least_recently_used_dfa_state;

    state_data->next_least_recently_used_dfa_state = next_least_state_num;
    state_data->next_most_recently_used_dfa_state = 0;

    the_dfa_cache->least_recently_used_dfa_state = dfa_state_num;
    if (next_least_state_num == 0)
      {
        assert(the_dfa_cache->most_recently_used_dfa_state == 0);
        the_dfa_cache->most_recently_used_dfa_state = dfa_state_num;
      }
    else
      {
        assert(the_dfa_cache->state_pool.array[next_least_state_num].
                       next_most_recently_used_dfa_state == 0);
        the_dfa_cache->state_pool.array[next_least_state_num].
                next_most_recently_used_dfa_state = dfa_state_num;
      }
  }

static void dfa_cache_insert_state_as_most_recently_used(
        dfa_cache *the_dfa_cache, size_t state_num)
  {
    dfa_state *state_array;
    size_t second_state_num;

    assert(the_dfa_cache != NULL);
    assert(state_num < the_dfa_cache->state_pool.element_count);
    assert(state_num >= 2);

    state_array = the_dfa_cache->state_pool.array;
    assert(state_array != NULL);

    second_state_num = the_dfa_cache->most_recently_used_dfa_state;

    if ((second_state_num == 0) &&
        (the_dfa_cache->least_recently_used_dfa_state == 0))
      {
        the_dfa_cache->least_recently_used_dfa_state = state_num;
      }
    the_dfa_cache->most_recently_used_dfa_state = state_num;
    assert(state_array[second_state_num].next_least_recently_used_dfa_state ==
           0);
    state_array[second_state_num].next_least_recently_used_dfa_state =
            state_num;

    state_array[state_num].next_most_recently_used_dfa_state =
            second_state_num;
    state_array[state_num].next_least_recently_used_dfa_state = 0;
  }
