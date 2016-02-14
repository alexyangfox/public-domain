/* file "tokenizer.c" */

/*
 *  This file contains the implementation of the tokenizer module, which breaks
 *  an input file into tokens.
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
#include <ctype.h>
#include "c_foundations/basic.h"
#include "c_foundations/diagnostic.h"
#include "c_foundations/buffer_print.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/auto_array.h"
#include "c_foundations/auto_array_implementation.h"
#include "token.h"
#include "tokenizer.h"
#include "source_location.h"
#include "unicode.h"
#include "o_integer.h"
#include "regular_expression.h"


AUTO_ARRAY(token_aa, token *);


struct tokenizer
  {
    const char *position;
    token *next_token;
    const char *next_token_raw_position;
    boolean error;
    source_location next_token_location;
    token_aa first_tokens;
  };


static const source_location unknown_location = { NULL, 0, 0, 0, 0 };


static boolean is_hex_digit(char to_test);
static void describe_character_in_diagnostic(const char *character_start,
                                             size_t character_length);
static verdict read_string_or_character_literal_character(
        const char *start_position, char *output_buffer,
        size_t *input_characters_used, size_t *output_characters_written,
        size_t line_number, size_t column_number, tokenizer *the_tokenizer,
        const char *literal_description, char terminator);
static token *next_after_first(tokenizer *the_tokenizer);
static token *consume_after_first(tokenizer *the_tokenizer);


AUTO_ARRAY_IMPLEMENTATION(token_aa, token *, 0);


extern tokenizer *create_tokenizer(const char *input_characters,
                                   const char *source_file_name)
  {
    tokenizer *result;
    file_name_holder *holder;
    verdict the_verdict;

    assert(input_characters != NULL);

    result = MALLOC_ONE_OBJECT(tokenizer);
    if (result == NULL)
        return NULL;

    result->position = input_characters;
    result->next_token = NULL;
    result->error = FALSE;

    holder = create_file_name_holder(source_file_name);
    if (holder == NULL)
      {
        free(result);
        return NULL;
      }

    result->next_token_location.file_name = file_name_holder_name(holder);
    result->next_token_location.start_line_number = 1;
    result->next_token_location.start_column_number = 1;
    result->next_token_location.end_line_number = 1;
    result->next_token_location.end_column_number = 1;
    result->next_token_location.holder = holder;

    the_verdict = token_aa_init(&(result->first_tokens), 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        file_name_holder_remove_reference(holder);
        free(result);
        return NULL;
      }

    return result;
  }

extern void delete_tokenizer(tokenizer *the_tokenizer)
  {
    token *next_token;
    token **array;
    size_t count;
    size_t num;

    assert(the_tokenizer != NULL);

    next_token = the_tokenizer->next_token;
    if (next_token != NULL)
        delete_token(next_token);

    array = the_tokenizer->first_tokens.array;
    assert(array != NULL);
    count = the_tokenizer->first_tokens.element_count;
    for (num = 0; num < count; ++num)
        delete_token(array[num]);
    free(array);

    file_name_holder_remove_reference(
            the_tokenizer->next_token_location.holder);

    free(the_tokenizer);
  }

extern token *next_token(tokenizer *the_tokenizer)
  {
    assert(the_tokenizer != NULL);

    if (the_tokenizer->first_tokens.element_count > 0)
        return the_tokenizer->first_tokens.array[0];
    else
        return next_after_first(the_tokenizer);
  }

extern token *forward_token(tokenizer *the_tokenizer, size_t forward_count)
  {
    if (forward_count == 0)
        return next_token(the_tokenizer);

    if (forward_count < the_tokenizer->first_tokens.element_count)
        return the_tokenizer->first_tokens.array[forward_count];

    while (forward_count > the_tokenizer->first_tokens.element_count)
      {
        token *the_token;
        verdict the_verdict;

        the_token = consume_after_first(the_tokenizer);
        if ((the_token == NULL) || (get_token_kind(the_token) == TK_ERROR) ||
            (get_token_kind(the_token) == TK_END_OF_INPUT))
          {
            return the_token;
          }

        the_verdict =
                token_aa_append(&(the_tokenizer->first_tokens), the_token);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            the_tokenizer->error = TRUE;
            delete_token(the_token);
            return NULL;
          }
      }

    assert(forward_count == the_tokenizer->first_tokens.element_count);

    return next_after_first(the_tokenizer);
  }

extern verdict consume_token(tokenizer *the_tokenizer)
  {
    size_t first_count;
    token *the_token;
    token_kind kind;

    assert(the_tokenizer != NULL);

    if (the_tokenizer->error)
        return MISSION_FAILED;

    first_count = the_tokenizer->first_tokens.element_count;
    if (first_count > 0)
      {
        token **array;
        size_t token_num;

        array = the_tokenizer->first_tokens.array;
        delete_token(array[0]);

        --first_count;
        the_tokenizer->first_tokens.element_count = first_count;
        for (token_num = 0; token_num < first_count; ++token_num)
            array[token_num] = array[token_num + 1];
        return MISSION_ACCOMPLISHED;
      }

    the_token = consume_after_first(the_tokenizer);
    if (the_token == NULL)
        return MISSION_FAILED;

    kind = get_token_kind(the_token);
    if ((kind != TK_END_OF_INPUT) && (kind != TK_ERROR))
        delete_token(the_token);

    return MISSION_ACCOMPLISHED;
  }

extern void set_tokenizer_line_number(tokenizer *the_tokenizer,
                                      size_t new_line_number)
  {
    assert(the_tokenizer != NULL);

    the_tokenizer->next_token_location.start_line_number = new_line_number;
    the_tokenizer->next_token_location.end_line_number = new_line_number;
  }

extern void set_tokenizer_column_number(tokenizer *the_tokenizer,
                                        size_t new_column_number)
  {
    assert(the_tokenizer != NULL);

    the_tokenizer->next_token_location.start_column_number = new_column_number;
    the_tokenizer->next_token_location.end_column_number = new_column_number;
  }

extern const char *tokenizer_raw_position(tokenizer *the_tokenizer)
  {
    assert(the_tokenizer != NULL);

    if (the_tokenizer->next_token == NULL)
        return the_tokenizer->position;
    else
        return the_tokenizer->next_token_raw_position;
  }

extern const source_location *next_token_location(tokenizer *the_tokenizer)
  {
    token *next;

    next = next_token(the_tokenizer);
    if (next == NULL)
        return &unknown_location;
    else
        return get_token_location(next);
  }


static boolean is_hex_digit(char to_test)
  {
    if ((to_test >= '0') && (to_test <= '9'))
        return TRUE;
    if ((to_test >= 'a') && (to_test <= 'f'))
        return TRUE;
    if ((to_test >= 'A') && (to_test <= 'F'))
        return TRUE;
    return FALSE;
  }

static void describe_character_in_diagnostic(const char *character_start,
                                             size_t character_length)
  {
    assert(character_start != NULL);
    assert((character_length >= 1) && (character_length <= 4));

    if (character_length == 1)
      {
        char character;

        character = *character_start;
        if (character == '\'')
            diagnostic_text("`\\\''");
        else if (character == '\"')
            diagnostic_text("`\\\"'");
        else if (character == '\?')
            diagnostic_text("`\\?");
        else if (character == '\\')
            diagnostic_text("`\\\\");
        else if (character == '\a')
            diagnostic_text("`\\a'");
        else if (character == '\b')
            diagnostic_text("`\\b'");
        else if (character == '\f')
            diagnostic_text("`\\f'");
        else if (character == '\n')
            diagnostic_text("`\\n'");
        else if (character == '\r')
            diagnostic_text("`\\r'");
        else if (character == '\t')
            diagnostic_text("`\\t'");
        else if (character == '\v')
            diagnostic_text("`\\v'");
        else if (character == ' ')
            diagnostic_text("` '");
        else if (isprint(character))
            diagnostic_text("`%c'", character);
        else
            diagnostic_text("0x%02x", (unsigned)character);
      }
    else
      {
        unsigned long value;

        switch (character_length)
          {
            case 2:
                value = (((unsigned long)(character_start[0]) & 0x1f) << 6);
                value |= ((unsigned long)(character_start[1]) & 0x3f);
                break;
            case 3:
                value = (((unsigned long)(character_start[0]) & 0xf) << 12);
                value |= (((unsigned long)(character_start[1]) & 0x3f) << 6);
                value |= ((unsigned long)(character_start[2]) & 0x3f);
                break;
            case 4:
                value = (((unsigned long)(character_start[0]) & 0x7) << 18);
                value |= (((unsigned long)(character_start[1]) & 0x3f) << 12);
                value |= (((unsigned long)(character_start[2]) & 0x3f) << 6);
                value |= ((unsigned long)(character_start[3]) & 0x3f);
                break;
            default:
                assert(FALSE);
                value = 0;
          }

        diagnostic_text("0x%lx", value);
      }
  }

static verdict read_string_or_character_literal_character(
        const char *start_position, char *output_buffer,
        size_t *input_characters_used, size_t *output_characters_written,
        size_t line_number, size_t column_number, tokenizer *the_tokenizer,
        const char *literal_description, char terminator)
  {
    assert(start_position != NULL);
    assert(output_buffer != NULL);
    assert(input_characters_used != NULL);
    assert(output_characters_written != NULL);

    switch (*start_position)
      {
        case 0:
          {
            location_error(&(the_tokenizer->next_token_location),
                           "Unterminated %s literal.", literal_description);
            *input_characters_used = 0;
            return MISSION_FAILED;
          }
        case '\n':
          {
            set_diagnostic_source_line_number(line_number);
            set_diagnostic_source_column_number(column_number);
            basic_error("Newline in %s literal.", literal_description);
            unset_diagnostic_source_column_number();
            unset_diagnostic_source_line_number();

            *input_characters_used = 1;
            return MISSION_FAILED;
          }
        case '\"':
        case '`':
          {
            if (*start_position == terminator)
              {
                *input_characters_used = 1;
                *output_characters_written = 0;
                return MISSION_ACCOMPLISHED;
              }
            else
              {
                *input_characters_used = 1;
                output_buffer[0] = *start_position;
                *output_characters_written = 1;
                return MISSION_ACCOMPLISHED;
              }
          }
        case '\\':
          {
            switch (start_position[1])
              {
                case '\'':
                case '\"':
                case '?':
                case '\\':
                  {
                    output_buffer[0] = start_position[1];
                    *input_characters_used = 2;
                    *output_characters_written = 1;
                    return MISSION_ACCOMPLISHED;
                  }
                case 'a':
                  {
                    output_buffer[0] = '\a';
                    *input_characters_used = 2;
                    *output_characters_written = 1;
                    return MISSION_ACCOMPLISHED;
                  }
                case 'b':
                  {
                    output_buffer[0] = '\b';
                    *input_characters_used = 2;
                    *output_characters_written = 1;
                    return MISSION_ACCOMPLISHED;
                  }
                case 'f':
                  {
                    output_buffer[0] = '\f';
                    *input_characters_used = 2;
                    *output_characters_written = 1;
                    return MISSION_ACCOMPLISHED;
                  }
                case 'n':
                  {
                    output_buffer[0] = '\n';
                    *input_characters_used = 2;
                    *output_characters_written = 1;
                    return MISSION_ACCOMPLISHED;
                  }
                case 'r':
                  {
                    output_buffer[0] = '\r';
                    *input_characters_used = 2;
                    *output_characters_written = 1;
                    return MISSION_ACCOMPLISHED;
                  }
                case 't':
                  {
                    output_buffer[0] = '\t';
                    *input_characters_used = 2;
                    *output_characters_written = 1;
                    return MISSION_ACCOMPLISHED;
                  }
                case 'v':
                  {
                    output_buffer[0] = '\v';
                    *input_characters_used = 2;
                    *output_characters_written = 1;
                    return MISSION_ACCOMPLISHED;
                  }
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                  {
                    unsigned long value;

                    value = start_position[1] - '0';

                    if ((start_position[2] >= '0') &&
                        (start_position[2] <= '7'))
                      {
                        value *= 8;
                        value += (start_position[2] - '0');

                        if ((start_position[3] >= '0') &&
                            (start_position[3] <= '7'))
                          {
                            value *= 8;
                            value += (start_position[3] - '0');
                            *input_characters_used = 4;
                          }
                        else
                          {
                            *input_characters_used = 3;
                          }
                      }
                    else
                      {
                        *input_characters_used = 2;
                      }

                    assert(value <= 0x1ff);

                    if (value == 0)
                      {
                        location_error(&(the_tokenizer->next_token_location),
                                "In %s literal, octal escape sequence specified"
                                " the null character.", literal_description);
                        *input_characters_used = 0;
                        return MISSION_FAILED;
                      }

                    if (value > 0x7f)
                      {
                        output_buffer[0] = (char)((value >> 6) | 0xc0);
                        output_buffer[1] = (char)((value & 0x3f) | 0x80);
                        *output_characters_written = 2;
                      }
                    else
                      {
                        output_buffer[0] = (char)value;
                        *output_characters_written = 1;
                      }

                    return MISSION_ACCOMPLISHED;
                  }
                case 'x':
                  {
                    char next_character;
                    unsigned long value;
                    const char *position;

                    next_character = start_position[2];

                    if (!(is_hex_digit(next_character)))
                      {
                        set_diagnostic_source_line_number(line_number);
                        set_diagnostic_source_column_number(column_number + 2);
                        basic_error(
                                "In %s literal, a hex escape sequence had zero"
                                " hex digits.", literal_description);
                        unset_diagnostic_source_column_number();
                        unset_diagnostic_source_line_number();

                        *input_characters_used = 2;
                        return MISSION_FAILED;
                      }

                    value = 0;
                    position = start_position + 2;

                    do
                      {
                        value = (value * 16);
                        if ((next_character >= '0') && (next_character <= '9'))
                          {
                            value += (next_character - '0');
                          }
                        else if ((next_character >= 'a') &&
                                 (next_character <= 'f'))
                          {
                            value += 10;
                            value += (next_character - 'a');
                          }
                        else
                          {
                            assert((next_character >= 'A') &&
                                   (next_character <= 'F'));
                            value += 10;
                            value += (next_character - 'A');
                          }

                        if (value > 0x1fffff)
                          {
                            set_diagnostic_source_line_number(line_number);
                            set_diagnostic_source_column_number(column_number);
                            basic_error(
                                    "In %s literal, hexadecimal escape "
                                    "sequence has more than 21 bits.",
                                    literal_description);
                            unset_diagnostic_source_column_number();
                            unset_diagnostic_source_line_number();
                            *input_characters_used =
                                    (position - start_position);
                            return MISSION_FAILED;
                          }

                        ++position;
                        next_character = *position;
                      } while (is_hex_digit(next_character));

                    *input_characters_used = (position - start_position);

                    assert(value <= 0x1fffff);

                    if (value == 0)
                      {
                        location_error(&(the_tokenizer->next_token_location),
                                "In %s literal, hexadecimal escape sequence "
                                "specified the null character.",
                                literal_description);
                        *input_characters_used = 0;
                        return MISSION_FAILED;
                      }

                    if (value > 0xffff)
                      {
                        output_buffer[0] = (char)((value >> 18) | 0xf0);
                        output_buffer[1] =
                                (char)(((value >> 12) & 0x3f) | 0x80);
                        output_buffer[2] =
                                (char)(((value >> 6) & 0x3f) | 0x80);
                        output_buffer[3] = (char)((value & 0x3f) | 0x80);
                        *output_characters_written = 4;
                      }
                    else if (value > 0x7ff)
                      {
                        output_buffer[0] = (char)((value >> 12) | 0xe0);
                        output_buffer[1] =
                                (char)(((value >> 6) & 0x3f) | 0x80);
                        output_buffer[2] = (char)((value & 0x3f) | 0x80);
                        *output_characters_written = 3;
                      }
                    else if (value > 0x7f)
                      {
                        output_buffer[0] = (char)((value >> 6) | 0xc0);
                        output_buffer[1] = (char)((value & 0x3f) | 0x80);
                        *output_characters_written = 2;
                      }
                    else
                      {
                        output_buffer[0] = (char)value;
                        *output_characters_written = 1;
                      }

                    return MISSION_ACCOMPLISHED;
                  }
                case 0:
                  {
                    location_error(&(the_tokenizer->next_token_location),
                            "Unterminated %s literal.", literal_description);

                    *input_characters_used = 1;
                    return MISSION_FAILED;
                  }
                default:
                  {
                    int character_length;

                    set_diagnostic_source_line_number(line_number);
                    set_diagnostic_source_column_number(column_number);

                    character_length =
                            validate_utf8_character(start_position + 1);
                    if (character_length < 0)
                      {
                        *input_characters_used = 1 + (-character_length);
                      }
                    else
                      {
                        open_error();
                        diagnostic_text(
                                "Illegal escape sequence: `\\' followed by ");
                        describe_character_in_diagnostic(start_position + 1,
                                                         character_length);
                        diagnostic_text(" in %s literal.",
                                        literal_description);
                        close_diagnostic();

                        *input_characters_used = (1 + character_length);
                      }

                    unset_diagnostic_source_column_number();
                    unset_diagnostic_source_line_number();

                    return MISSION_FAILED;
                  }
              }
          }
        default:
          {
            int character_length;
            size_t byte_num;

            set_diagnostic_source_line_number(line_number);
            set_diagnostic_source_column_number(column_number);
            character_length = validate_utf8_character(start_position);
            unset_diagnostic_source_column_number();
            unset_diagnostic_source_line_number();

            if (character_length < 0)
              {
                *input_characters_used = -character_length;
                return MISSION_FAILED;
              }

            assert(character_length > 0);

            for (byte_num = 0; byte_num < character_length; ++byte_num)
                output_buffer[byte_num] = start_position[byte_num];

            *input_characters_used = character_length;
            *output_characters_written = character_length;

            return MISSION_ACCOMPLISHED;
          }
      }
  }

static token *next_after_first(tokenizer *the_tokenizer)
  {
    token *result;
    const char *position;
    size_t line_number;
    size_t column_number;
    char next_character;

    result = the_tokenizer->next_token;
    if (result != NULL)
        return result;

    if (the_tokenizer->error)
        return NULL;

    position = the_tokenizer->position;
    assert(position != NULL);
    line_number = the_tokenizer->next_token_location.start_line_number;
    column_number = the_tokenizer->next_token_location.start_column_number;

    next_character = *position;

    while (TRUE)
      {
        if ((next_character == ' ') || (next_character == '\t'))
          {
            ++position;
            next_character = *position;
            ++column_number;
            continue;
          }

        if (next_character == '\n')
          {
            ++position;
            next_character = *position;
            ++line_number;
            column_number = 1;
            continue;
          }

        if (next_character == '\r')
          {
            ++position;
            next_character = *position;
            continue;
          }

        if ((next_character == '/') && (position[1] == '*'))
          {
            size_t nesting_level;

            position += 2;
            next_character = *position;
            column_number += 2;

            nesting_level = 1;
            while (TRUE)
              {
                int character_length;

                if (next_character == 0)
                    break;

                if ((next_character == '*') && (position[1] == '/'))
                  {
                    position += 2;
                    next_character = *position;
                    column_number += 2;
                    assert(nesting_level > 0);
                    --nesting_level;
                    if (nesting_level == 0)
                        break;
                    continue;
                  }

                if ((next_character == '/') && (position[1] == '*'))
                  {
                    position += 2;
                    next_character = *position;
                    column_number += 2;
                    assert(nesting_level > 0);
                    ++nesting_level;
                    continue;
                  }

                if (next_character == '\n')
                  {
                    ++line_number;
                    column_number = 1;
                  }
                else
                  {
                    ++column_number;
                  }

                character_length = validate_utf8_character(position);
                if (character_length < 0)
                    position += -character_length;
                else
                    position += character_length;
                next_character = *position;
              }

            continue;
          }

        if ((next_character == '/') && (position[1] == '/'))
          {
            position += 2;
            next_character = *position;
            column_number += 2;

            while (TRUE)
              {
                int character_length;

                if (next_character == 0)
                    break;

                if (next_character == '\n')
                  {
                    ++position;
                    next_character = *position;
                    ++line_number;
                    column_number = 1;
                    break;
                  }

                character_length = validate_utf8_character(position);
                if (character_length < 0)
                    position += -character_length;
                else
                    position += character_length;
                next_character = *position;
              }

            continue;
          }

        if (next_character == '#')
          {
            ++position;
            next_character = *position;
            ++column_number;

            while (TRUE)
              {
                int character_length;

                if (next_character == 0)
                    break;

                if (next_character == '\n')
                  {
                    ++position;
                    next_character = *position;
                    ++line_number;
                    column_number = 1;
                    break;
                  }

                character_length = validate_utf8_character(position);
                if (character_length < 0)
                    position += -character_length;
                else
                    position += character_length;
                next_character = *position;
              }

            continue;
          }

        break;
      }

    the_tokenizer->next_token_location.start_line_number = line_number;
    the_tokenizer->next_token_location.start_column_number = column_number;
    the_tokenizer->next_token_location.end_line_number = line_number;
    the_tokenizer->next_token_location.end_column_number = column_number;

    if (((next_character >= 'a') && (next_character <= 'z')) ||
        ((next_character >= 'A') && (next_character <= 'Z')) ||
        (next_character == '_'))
      {
        const char *start;
        char *copy;

        start = position;

        if (strncmp(position, "operator", (sizeof("operator") - 1)) == 0)
          {
            position += (sizeof("operator") - 1);

            switch (*position)
              {
                case '(':
                    if (position[1] == ')')
                        position += 2;
                    break;
                case '[':
                    if (position[1] == ']')
                        position += 2;
                    break;
                case ':':
                    if (position[1] == ':')
                        position += 2;
                    break;
                case '*':
                    ++position;
                    break;
                case '/':
                    if ((position[1] == ':') && (position[2] == ':'))
                        position += 3;
                    else
                        ++position;
                    break;
                case '%':
                    ++position;
                    break;
                case '+':
                    ++position;
                    break;
                case '-':
                    if (position[1] == '>')
                        position += 2;
                    else
                        ++position;
                    break;
                case '<':
                    if (position[1] == '<')
                        position += 2;
                    else if (position[1] == '=')
                        position += 2;
                    else
                        ++position;
                    break;
                case '>':
                    if (position[1] == '>')
                        position += 2;
                    else if (position[1] == '=')
                        position += 2;
                    else
                        ++position;
                    break;
                case '&':
                    ++position;
                    break;
                case '^':
                    ++position;
                    break;
                case '|':
                    ++position;
                    break;
                case '=':
                    if (position[1] == '=')
                        position += 2;
                    break;
                case '!':
                    if (position[1] == '=')
                        position += 2;
                    else
                        ++position;
                    break;
                case '~':
                    ++position;
                    break;
                default:
                    goto not_operator;
              }
          }
        else
          {
            do
              {
                ++position;
              not_operator:
                next_character = *position;
              } while (((next_character >= 'a') && (next_character <= 'z')) ||
                       ((next_character >= 'A') && (next_character <= 'Z')) ||
                       ((next_character >= '0') && (next_character <= '9')) ||
                       (next_character == '_'));
          }

        assert(position > start);
        copy = MALLOC_ARRAY(char, (position - start) + 1);
        if (copy == NULL)
          {
            the_tokenizer->error = TRUE;
            return NULL;
          }

        memcpy(copy, start, (position - start));
        copy[position - start] = 0;

        result = create_identifier_token(copy);
        free(copy);

        column_number += (position - start);
      }
    else
      {
        switch (next_character)
          {
            case '\"':
            case '`':
              {
                char start_character;
                string_buffer data_buffer;
                verdict the_verdict;

                start_character = next_character;

                the_verdict = string_buffer_init(&data_buffer, 10);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    the_tokenizer->error = TRUE;
                    return NULL;
                  }

                ++column_number;
                ++position;

                while (TRUE)
                  {
                    char character_buffer[4];
                    size_t input_characters_used;
                    size_t output_characters_written;
                    verdict the_verdict;

                    next_character = *position;

                    the_verdict = read_string_or_character_literal_character(
                            position, &(character_buffer[0]),
                            &input_characters_used,
                            &output_characters_written, line_number,
                            column_number, the_tokenizer, "string",
                            start_character);

                    while (input_characters_used > 0)
                      {
                        --input_characters_used;

                        assert(next_character != 0);
                        if (next_character == '\n')
                          {
                            ++line_number;
                            column_number = 1;
                          }
                        else
                          {
                            ++column_number;
                          }

                        ++position;
                        next_character = *position;
                      }

                    if (the_verdict != MISSION_ACCOMPLISHED)
                      {
                        result = create_error_token();
                        free(data_buffer.array);
                        goto ready_to_return;
                      }

                    assert(output_characters_written <= 4);
                    if (output_characters_written > 0)
                      {
                        verdict the_verdict;

                        the_verdict = string_buffer_append_array(&data_buffer,
                                output_characters_written,
                                &(character_buffer[0]));
                        if (the_verdict != MISSION_ACCOMPLISHED)
                          {
                            free(data_buffer.array);
                            the_tokenizer->error = TRUE;
                            return NULL;
                          }
                      }

                    if (output_characters_written == 0)
                        break;
                  }

                the_verdict = string_buffer_append(&data_buffer, 0);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    free(data_buffer.array);
                    the_tokenizer->error = TRUE;
                    return NULL;
                  }

                if (start_character == '\"')
                  {
                    result = create_string_literal_token(data_buffer.array);
                  }
                else
                  {
                    assert(start_character == '`');
                    result = create_backtick_expression_literal_token(
                            data_buffer.array);
                  }

                free(data_buffer.array);

                break;
              }
            case '\'':
              {
                char character_buffer[4];
                size_t input_characters_used;
                size_t output_characters_written;
                verdict the_verdict;

                ++position;
                ++column_number;

                the_verdict = read_string_or_character_literal_character(
                        position, &(character_buffer[0]),
                        &input_characters_used, &output_characters_written,
                        line_number, column_number, the_tokenizer, "character",
                        0);

                next_character = *position;

                while (input_characters_used > 0)
                  {
                    --input_characters_used;

                    assert(next_character != 0);
                    if (next_character == '\n')
                      {
                        ++line_number;
                        column_number = 1;
                      }
                    else
                      {
                        ++column_number;
                      }

                    ++position;
                    next_character = *position;
                  }

                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    result = create_error_token();
                    break;
                  }

                if (next_character != '\'')
                  {
                    location_error(&(the_tokenizer->next_token_location),
                            "Bad character literal -- expected closing "
                            "single-quote character not found.");
                    result = create_error_token();
                    break;
                  }

                ++position;
                next_character = *position;
                ++column_number;

                assert(output_characters_written <= 4);

                result =
                        create_character_literal_token(&(character_buffer[0]));

                break;
              }
            case '0':
              {
                if ((position[1] == 'x') || (position[1] == 'X'))
                  {
                    const char *start;
                    o_integer oi;

                    position += 2;
                    column_number += 2;

                    if (!(is_hex_digit(*position)))
                      {
                        location_error(&(the_tokenizer->next_token_location),
                                "Bad hexadecimal integer literal -- 0%c prefix"
                                " must be followed by at least one hexadecimal"
                                " digit.", *(position - 1));
                        result = create_error_token();
                        break;
                      }

                    start = position;
                    do
                      {
                        ++position;
                      } while (is_hex_digit(*position));

                    column_number += (position - start);

                    oi_create_from_hex_ascii(oi, (position - start), start,
                                             FALSE);
                    if (oi_out_of_memory(oi))
                      {
                        result = create_error_token();
                        break;
                      }

                    result = create_hexadecimal_integer_literal_token(oi);
                    oi_remove_reference(oi);

                    break;
                  }

                /* fall through */
              }
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
              {
                const char *start;
                o_integer oi;

                start = position;
                do
                  {
                    ++position;
                  } while ((*position >= '0') && (*position <= '9'));

                column_number += (position - start);

                oi_create_from_decimal_ascii(oi, (position - start), start,
                                             FALSE);
                if (oi_out_of_memory(oi))
                  {
                    result = create_error_token();
                    break;
                  }

                if (((*position == '.') && (position[1] >= '0') &&
                     (position[1] <= '9')) || (*position == 'e') ||
                    (*position == 'E'))
                  {
                    rational *the_rational;

                    the_rational = create_rational(oi, oi_one);
                    oi_remove_reference(oi);
                    if (the_rational == NULL)
                      {
                        result = create_error_token();
                        break;
                      }

                    if ((*position == '.') && (position[1] >= '0') &&
                        (position[1] <= '9'))
                      {
                        const char *start;

                        ++position;
                        ++column_number;

                        start = position;
                        while ((*position >= '0') && (*position <= '9'))
                            ++position;

                        column_number += (position - start);

                        if (position > start)
                          {
                            o_integer numerator;
                            o_integer exponent;
                            o_integer denominator;
                            rational *extra;
                            rational *new_rational;

                            oi_create_from_decimal_ascii(numerator,
                                    (position - start), start, FALSE);
                            if (oi_out_of_memory(numerator))
                              {
                                rational_remove_reference(the_rational);
                                result = create_error_token();
                                break;
                              }

                            oi_create_from_size_t(exponent, position - start);
                            if (oi_out_of_memory(exponent))
                              {
                                oi_remove_reference(numerator);
                                rational_remove_reference(the_rational);
                                result = create_error_token();
                                break;
                              }

                            oi_power_of_ten(denominator, exponent);
                            oi_remove_reference(exponent);
                            if (oi_out_of_memory(denominator))
                              {
                                oi_remove_reference(numerator);
                                rational_remove_reference(the_rational);
                                result = create_error_token();
                                break;
                              }

                            extra = create_rational(numerator, denominator);
                            oi_remove_reference(denominator);
                            oi_remove_reference(numerator);
                            if (extra == NULL)
                              {
                                rational_remove_reference(the_rational);
                                result = create_error_token();
                                break;
                              }

                            new_rational = rational_add(the_rational, extra);
                            rational_remove_reference(extra);
                            rational_remove_reference(the_rational);
                            if (new_rational == NULL)
                              {
                                result = create_error_token();
                                break;
                              }

                            the_rational = new_rational;
                          }
                      }

                    if ((*position == 'e') || (*position == 'E'))
                      {
                        boolean negative;
                        const char *start;
                        o_integer exponent;
                        o_integer factor_oi;
                        rational *factor_rational;
                        rational *new_rational;

                        ++position;
                        ++column_number;

                        negative = FALSE;

                        if (*position == '+')
                          {
                            ++position;
                            ++column_number;
                          }
                        else if (*position == '-')
                          {
                            negative = TRUE;
                            ++position;
                            ++column_number;
                          }

                        if ((*position < '0') || (*position > '9'))
                          {
                            location_error(
                                    &(the_tokenizer->next_token_location),
                                    "Missing exponent in scientific "
                                    "notation.");
                            rational_remove_reference(the_rational);
                            result = create_error_token();
                            break;
                          }

                        start = position;
                        do
                          {
                            ++position;
                          } while ((*position >= '0') && (*position <= '9'));

                        column_number += (position - start);

                        oi_create_from_decimal_ascii(exponent,
                                (position - start), start, FALSE);
                        if (oi_out_of_memory(exponent))
                          {
                            rational_remove_reference(the_rational);
                            result = create_error_token();
                            break;
                          }

                        oi_power_of_ten(factor_oi, exponent);
                        oi_remove_reference(exponent);
                        if (oi_out_of_memory(factor_oi))
                          {
                            rational_remove_reference(the_rational);
                            result = create_error_token();
                            break;
                          }

                        factor_rational = create_rational(factor_oi, oi_one);
                        oi_remove_reference(factor_oi);
                        if (factor_rational == NULL)
                          {
                            rational_remove_reference(the_rational);
                            result = create_error_token();
                            break;
                          }

                        if (negative)
                          {
                            new_rational = rational_divide(the_rational,
                                                           factor_rational);
                          }
                        else
                          {
                            new_rational = rational_multiply(the_rational,
                                                             factor_rational);
                          }
                        rational_remove_reference(factor_rational);
                        rational_remove_reference(the_rational);
                        if (new_rational == NULL)
                          {
                            result = create_error_token();
                            break;
                          }

                        the_rational = new_rational;
                      }

                    result = create_scientific_notation_literal_token(
                            the_rational);
                    rational_remove_reference(the_rational);

                    break;
                  }

                result = create_decimal_integer_literal_token(oi);
                oi_remove_reference(oi);

                break;
              }
            case '(':
              {
                result = create_left_paren_token();
                ++position;
                ++column_number;
                break;
              }
            case ')':
              {
                result = create_right_paren_token();
                ++position;
                ++column_number;
                break;
              }
            case '[':
              {
                result = create_left_bracket_token();
                ++position;
                ++column_number;
                break;
              }
            case ']':
              {
                result = create_right_bracket_token();
                ++position;
                ++column_number;
                break;
              }
            case '{':
              {
                result = create_left_curly_brace_token();
                ++position;
                ++column_number;
                break;
              }
            case '}':
              {
                result = create_right_curly_brace_token();
                ++position;
                ++column_number;
                break;
              }
            case ':':
              {
                if (position[1] == '=')
                  {
                    result = create_assign_token();
                    position += 2;
                    column_number += 2;
                  }
                else if (position[1] == ':')
                  {
                    if (position[2] == '=')
                      {
                        result = create_modulo_assign_token();
                        position += 3;
                        column_number += 3;
                      }
                    else
                      {
                        result = create_force_token();
                        position += 2;
                        column_number += 2;
                      }
                  }
                else
                  {
                    result = create_colon_token();
                    ++position;
                    ++column_number;
                  }
                break;
              }
            case ';':
              {
                result = create_semicolon_token();
                ++position;
                ++column_number;
                break;
              }
            case ',':
              {
                result = create_comma_token();
                ++position;
                ++column_number;
                break;
              }
            case '.':
              {
                if (position[1] == '.')
                  {
                    if (position[2] == '.')
                      {
                        if (position[3] == '.')
                          {
                            result = create_dot_dot_dot_dot_token();
                            position += 4;
                            column_number += 4;
                          }
                        else
                          {
                            result = create_dot_dot_dot_token();
                            position += 3;
                            column_number += 3;
                          }
                      }
                    else
                      {
                        result = create_dot_dot_token();
                        position += 2;
                        column_number += 2;
                      }
                  }
                else
                  {
                    result = create_dot_token();
                    ++position;
                    ++column_number;
                  }
                break;
              }
            case '*':
              {
                if (position[1] == '=')
                  {
                    result = create_multiply_assign_token();
                    position += 2;
                    column_number += 2;
                  }
                else
                  {
                    result = create_star_token();
                    ++position;
                    ++column_number;
                  }
                break;
              }
            case '/':
              {
                if (position[1] == '=')
                  {
                    result = create_divide_assign_token();
                    position += 2;
                    column_number += 2;
                  }
                else if ((position[1] == ':') && (position[2] == ':'))
                  {
                    if (position[3] == '=')
                      {
                        result = create_divide_force_assign_token();
                        position += 4;
                        column_number += 4;
                      }
                    else
                      {
                        result = create_divide_force_token();
                        position += 3;
                        column_number += 3;
                      }
                  }
                else
                  {
                    result = create_divide_token();
                    ++position;
                    ++column_number;
                  }
                break;
              }
            case '%':
              {
                if (position[1] == '=')
                  {
                    result = create_remainder_assign_token();
                    position += 2;
                    column_number += 2;
                  }
                else
                  {
                    result = create_remainder_token();
                    ++position;
                    ++column_number;
                  }
                break;
              }
            case '+':
              {
                if (position[1] == '=')
                  {
                    result = create_add_assign_token();
                    position += 2;
                    column_number += 2;
                  }
                else if (position[1] == '+')
                  {
                    result = create_plus_plus_token();
                    position += 2;
                    column_number += 2;
                  }
                else if ((position[1] == 'o') && (position[2] == 'o'))
                  {
                    position += 3;
                    column_number += 3;

                    result = create_decimal_integer_literal_token(
                            oi_positive_infinity);
                  }
                else
                  {
                    result = create_add_token();
                    ++position;
                    ++column_number;
                  }
                break;
              }
            case '-':
              {
                if (position[1] == '=')
                  {
                    result = create_subtract_assign_token();
                    position += 2;
                    column_number += 2;
                  }
                else if (position[1] == '-')
                  {
                    if (position[2] == '>')
                      {
                        result = create_maps_to_token();
                        position += 3;
                        column_number += 3;
                      }
                    else
                      {
                        result = create_minus_minus_token();
                        position += 2;
                        column_number += 2;
                      }
                  }
                else if (position[1] == '>')
                  {
                    result = create_points_to_token();
                    position += 2;
                    column_number += 2;
                  }
                else if ((position[1] == 'o') && (position[2] == 'o'))
                  {
                    position += 3;
                    column_number += 3;

                    result = create_decimal_integer_literal_token(
                            oi_negative_infinity);
                  }
                else
                  {
                    result = create_dash_token();
                    ++position;
                    ++column_number;
                  }
                break;
              }
            case '<':
              {
                if (position[1] == '<')
                  {
                    if (position[2] == '=')
                      {
                        result = create_shift_left_assign_token();
                        position += 3;
                        column_number += 3;
                      }
                    else
                      {
                        result = create_shift_left_token();
                        position += 2;
                        column_number += 2;
                      }
                  }
                else if (position[1] == '=')
                  {
                    result = create_less_than_or_equal_token();
                    position += 2;
                    column_number += 2;
                  }
                else if ((position[1] == '-') && (position[2] == '-'))
                  {
                    result = create_returns_token();
                    position += 3;
                    column_number += 3;
                  }
                else
                  {
                    result = create_less_than_token();
                    ++position;
                    ++column_number;
                  }
                break;
              }
            case '>':
              {
                if (position[1] == '>')
                  {
                    if (position[2] == '=')
                      {
                        result = create_shift_right_assign_token();
                        position += 3;
                        column_number += 3;
                      }
                    else
                      {
                        result = create_shift_right_token();
                        position += 2;
                        column_number += 2;
                      }
                  }
                else if (position[1] == '=')
                  {
                    result = create_greater_than_or_equal_token();
                    position += 2;
                    column_number += 2;
                  }
                else
                  {
                    result = create_greater_than_token();
                    ++position;
                    ++column_number;
                  }
                break;
              }
            case '&':
              {
                if (position[1] == '=')
                  {
                    result = create_bitwise_and_assign_token();
                    position += 2;
                    column_number += 2;
                  }
                else if (position[1] == '&')
                  {
                    if (position[2] == '=')
                      {
                        result = create_logical_and_assign_token();
                        position += 3;
                        column_number += 3;
                      }
                    else
                      {
                        result = create_logical_and_token();
                        position += 2;
                        column_number += 2;
                      }
                  }
                else
                  {
                    result = create_ampersand_token();
                    ++position;
                    ++column_number;
                  }
                break;
              }
            case '^':
              {
                if (position[1] == '=')
                  {
                    result = create_bitwise_xor_assign_token();
                    position += 2;
                    column_number += 2;
                  }
                else
                  {
                    result = create_bitwise_xor_token();
                    ++position;
                    ++column_number;
                  }
                break;
              }
            case '|':
              {
                if (position[1] == '=')
                  {
                    result = create_bitwise_or_assign_token();
                    position += 2;
                    column_number += 2;
                  }
                else if (position[1] == '|')
                  {
                    if (position[2] == '=')
                      {
                        result = create_logical_or_assign_token();
                        position += 3;
                        column_number += 3;
                      }
                    else
                      {
                        result = create_logical_or_token();
                        position += 2;
                        column_number += 2;
                      }
                  }
                else
                  {
                    result = create_bitwise_or_token();
                    ++position;
                    ++column_number;
                  }
                break;
              }
            case '=':
              {
                if (position[1] == '=')
                  {
                    result = create_equal_token();
                    position += 2;
                    column_number += 2;
                  }
                else
                  {
                    goto bad_character;
                  }
                break;
              }
            case '!':
              {
                if (position[1] == '=')
                  {
                    result = create_not_equal_token();
                    position += 2;
                    column_number += 2;
                  }
                else
                  {
                    result = create_logical_not_token();
                    ++position;
                    ++column_number;
                  }
                break;
              }
            case '?':
              {
                result = create_question_mark_token();
                ++position;
                ++column_number;
                break;
              }
            case '~':
              {
                if (position[1] == '=')
                  {
                    result = create_concatenate_assign_token();
                    position += 2;
                    column_number += 2;
                  }
                else
                  {
                    result = create_bitwise_not_token();
                    ++position;
                    ++column_number;
                  }
                break;
              }
            case '@':
              {
                const char *follow;
                size_t length;
                regular_expression_error the_error;
                regular_expression *the_regular_expression;

                ++position;
                ++column_number;

                follow = position;

                while (*follow != '@')
                  {
                    int character_length;

                    if (*follow == '\\')
                        ++follow;

                    if (*follow == 0)
                      {
                        location_error(&(the_tokenizer->next_token_location),
                                "Unterminated regular expression literal.");
                        result = create_error_token();
                        column_number += (follow - position);
                        position = follow;
                        goto ready_to_break;
                      }

                    character_length = validate_utf8_character(follow);
                    if (character_length < 0)
                      {
                        location_error(&(the_tokenizer->next_token_location),
                                "Bad regular expression literal -- bad UTF-8 "
                                "character found.");
                        result = create_error_token();
                        follow += -character_length;
                        column_number += (follow - position);
                        position = follow;
                        goto ready_to_break;
                      }
                    follow += character_length;
                  }

                assert(follow >= position);
                length = (follow - position);

                the_regular_expression =
                        create_regular_expression_from_pattern(position,
                                length, &the_error);
                position += length + 1;
                column_number += length + 1;
                if (the_regular_expression == NULL)
                  {
                    location_error(&(the_tokenizer->next_token_location),
                            string_for_regular_expression_error(the_error));
                    result = create_error_token();
                    break;
                  }

                result = create_regular_expression_literal_token(
                        the_regular_expression);
                regular_expression_remove_reference(the_regular_expression);

              ready_to_break:
                break;
              }
            case 0:
              {
                result = create_end_of_input_token();
                break;
              }
            default:
            bad_character:
              {
                int character_length;

                set_diagnostic_source_line_number(line_number);
                set_diagnostic_source_column_number(column_number);

                character_length = validate_utf8_character(position);
                if (character_length < 0)
                  {
                    position += -character_length;
                  }
                else
                  {
                    open_error();
                    diagnostic_text("Illegal character in input: ");
                    describe_character_in_diagnostic(position,
                                                     character_length);
                    diagnostic_text(".");
                    close_diagnostic();

                    position += character_length;
                  }

                unset_diagnostic_source_column_number();
                unset_diagnostic_source_line_number();

                result = create_error_token();
                ++column_number;
                break;
              }
          }
      }

  ready_to_return:
    the_tokenizer->next_token_raw_position = the_tokenizer->position;
    the_tokenizer->position = position;
    the_tokenizer->next_token_location.end_line_number = line_number;
    the_tokenizer->next_token_location.end_column_number = column_number;
    if (result != NULL)
        set_token_location(result, &(the_tokenizer->next_token_location));
    the_tokenizer->next_token = result;
    return result;
  }

static token *consume_after_first(tokenizer *the_tokenizer)
  {
    token *to_consume;
    token_kind kind;

    if (the_tokenizer->next_token == NULL)
        next_after_first(the_tokenizer);

    to_consume = the_tokenizer->next_token;
    if (to_consume == NULL)
        return NULL;

    kind = get_token_kind(to_consume);
    if ((kind == TK_END_OF_INPUT) || (kind == TK_ERROR))
        return to_consume;

    the_tokenizer->next_token = NULL;

    the_tokenizer->next_token_location.start_line_number =
            the_tokenizer->next_token_location.end_line_number;
    the_tokenizer->next_token_location.start_column_number =
            the_tokenizer->next_token_location.end_column_number;

    return to_consume;
  }
