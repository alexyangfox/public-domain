/* file "unicode.c" */

/*
 *  This file contains the implementation of the unicode module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <assert.h>
#include "c_foundations/diagnostic.h"
#include "unicode.h"


typedef void diagnostic_handler_type(void *data, diagnostic_kind kind,
        boolean has_message_number, size_t message_number,
        const char *file_name, boolean has_line_number, size_t line_number,
        boolean has_column_number, size_t column_number,
        boolean has_character_count, size_t character_count,
        const char *format, va_list arg);


static void basic_error_wrapper(void *data, const char *format, ...);
static void empty_error_wrapper(void *data, const char *format, ...);


extern int validate_utf8_character(const char *character_start)
  {
    return validate_utf8_character_with_error_handler(character_start,
            &basic_error_wrapper, NULL);
  }

extern int validate_utf8_character_with_error_handler(
        const char *character_start,
        void (*error_handler_function)(void *data, const char *format, ...),
        void *error_handler_data)
  {
    unsigned char character_bits;
    int extra_byte_count;
    unsigned long result;
    int extra_byte_num;

    assert(character_start != NULL);

    character_bits = ((unsigned char)*character_start);

    if ((character_bits & 0x80) == 0)
        return 1;

    /* Then this character is part of a multi-byte UTF-8 character. */

    if ((character_bits & 0x40) == 0)
      {
        (*error_handler_function)(error_handler_data,
                "Bad UTF-8 encoding -- a byte (0x%02x) with 0x2 as its high "
                "two bits was found at the start of a character.",
                (unsigned)character_bits);
        return -1;
      }

    if ((character_bits & 0x20) == 0)
      {
        extra_byte_count = 1;
        result = character_bits & 0x1f;
      }
    else if ((character_bits & 0x10) == 0)
      {
        extra_byte_count = 2;
        result = character_bits & 0xf;
      }
    else if ((character_bits & 0x08) == 0)
      {
        extra_byte_count = 3;
        result = character_bits & 0x7;
      }
    else
      {
        (*error_handler_function)(error_handler_data,
                "Bad UTF-8 encoding -- a byte (0x%02x) with 0x1f as its high "
                "five bits was found at the start of a character.",
                (unsigned)character_bits);
        return -1;
      }

    for (extra_byte_num = 0; extra_byte_num < extra_byte_count;
         ++extra_byte_num)
      {
        character_bits =
                ((unsigned char)(character_start[1 + extra_byte_num]));

        if (character_bits == 0)
          {
            (*error_handler_function)(error_handler_data,
                    "Bad UTF-8 encoding -- the input ends in the middle of a "
                    "multi-byte character.");
            return -(1 + extra_byte_num);
          }

        if ((character_bits & 0xc0) != 0x80)
          {
            (*error_handler_function)(error_handler_data,
                    "Bad UTF-8 encoding -- a character continuation byte "
                    "(0x%02x) was found without 0x2 as its high two bits.",
                    (unsigned)character_bits);
            return -(1 + extra_byte_num);
          }

        result <<= 6;
        result |= (character_bits & 0x3f);
      }

    if ((result >= 0xd800) && (result < 0xe000))
      {
        (*error_handler_function)(error_handler_data,
                "Bad UTF-8 encoding -- UTF-16 surrogate code point encoded.");
        return -(1 + extra_byte_num);
      }

    if (result >= 0x110000)
      {
        (*error_handler_function)(error_handler_data,
                "Bad UTF-8 encoding -- value greater than 0x10ffff encoded.");
        return -(1 + extra_byte_num);
      }

    if ((extra_byte_count == 1) && (result < 0x80))
      {
        (*error_handler_function)(error_handler_data,
                "Bad UTF-8 encoding -- two-byte encoding used unnecessarily.");
        return -(1 + extra_byte_num);
      }

    if ((extra_byte_count == 2) && (result < 0x800))
      {
        (*error_handler_function)(error_handler_data,
                "Bad UTF-8 encoding -- three-byte encoding used "
                "unnecessarily.");
        return -(1 + extra_byte_num);
      }

    if ((extra_byte_count == 3) && (result < 0x10000))
      {
        (*error_handler_function)(error_handler_data,
                "Bad UTF-8 encoding -- four-byte encoding used "
                "unnecessarily.");
        return -(1 + extra_byte_num);
      }

    return 1 + extra_byte_count;
  }

extern void convert_utf16_to_utf8(unsigned int *utf16, size_t block_count,
        char *output_buffer, size_t *character_count, size_t *byte_count,
        void (*error_handler_function)(void *data, const char *format, ...),
        void *error_handler_data)
  {
    size_t block_num;
    size_t u8_num;
    size_t character_num;

    assert(utf16 != NULL);
    assert(output_buffer != NULL);

    block_num = 0;
    u8_num = 0;
    character_num = 0;

    while (block_num < block_count)
      {
        unsigned long first_block;
        unsigned long code_point;
        size_t new_u8s;

        first_block = utf16[block_num];

        if ((first_block & 0xfc00) == 0xdc00)
          {
            (*error_handler_function)(error_handler_data,
                    "Bad UTF-16 encoding -- a word (0x%04x) with 0x37 as its "
                    "high six bits was found at the start of a character.",
                    (unsigned)first_block);
            return;
          }

        if ((first_block & 0xfc00) == 0xd800)
          {
            unsigned long second_block;

            if (block_num + 1 == block_count)
              {
                (*error_handler_function)(error_handler_data,
                        "Bad UTF-16 encoding -- the input ends in the middle "
                        "of a multi-word character.");
                return;
              }

            second_block = utf16[block_num + 1];

            if ((second_block & 0xfc00) != 0xdc00)
              {
                (*error_handler_function)(error_handler_data,
                        "Bad UTF-16 encoding -- a character continuation word "
                        "(0x%04x) was found without 0x37 as its high six "
                        "bits.", (unsigned)second_block);
                return;
              }

            code_point = ((((first_block & 0x3ff) + 0x40) << 10) |
                          (second_block & 0x3ff));
            block_num += 2;
          }
        else
          {
            code_point = first_block;
            ++block_num;
          }

        new_u8s = code_point_to_utf8(code_point, &(output_buffer[u8_num]));
        assert(new_u8s > 0);
        assert(new_u8s <= 4);

        u8_num += new_u8s;

        ++character_num;
      }

    *character_count = character_num;
    *byte_count = u8_num;
  }

extern unsigned long utf8_to_code_point(const char *character_start)
  {
    unsigned char character_bits;
    int extra_byte_count;
    unsigned long result;
    int extra_byte_num;

    assert(character_start != NULL);

    character_bits = ((unsigned char)*character_start);

    if ((character_bits & 0x80) == 0)
        return character_bits;

    /* Then this character is part of a multi-byte UTF-8 character. */

    assert((character_bits & 0x40) != 0);

    if ((character_bits & 0x20) == 0)
      {
        extra_byte_count = 1;
        result = character_bits & 0x1f;
      }
    else if ((character_bits & 0x10) == 0)
      {
        extra_byte_count = 2;
        result = character_bits & 0xf;
      }
    else if ((character_bits & 0x08) == 0)
      {
        extra_byte_count = 3;
        result = character_bits & 0x7;
      }
    else
      {
        assert(FALSE);
        extra_byte_count = 0;
        result = 0;
      }

    for (extra_byte_num = 0; extra_byte_num < extra_byte_count;
         ++extra_byte_num)
      {
        character_bits =
                ((unsigned char)(character_start[1 + extra_byte_num]));

        assert(character_bits != 0);
        assert((character_bits & 0xc0) == 0x80);

        result <<= 6;
        result |= (character_bits & 0x3f);
      }

    assert((result < 0xd800) || (result >= 0xe000));
    assert(result < 0x110000);
    return result;
  }

extern unsigned long utf16_to_code_point(unsigned *u16_start)
  {
    assert(u16_start != NULL);

    assert((u16_start[0] & 0xfc00) != 0xdc00);

    if ((u16_start[0] & 0xfc00) == 0xd800)
      {
        assert((u16_start[1] & 0xfc00) == 0xdc00);
        return ((((((unsigned long)(u16_start[0])) & 0x3ff) + 0x40) << 10) |
                (u16_start[1] & 0x3ff));
      }
    else
      {
        return u16_start[0];
      }
  }

extern size_t code_point_to_utf16(unsigned long code_point,
                                  unsigned *output_buffer)
  {
    assert((code_point < 0xd800) || (code_point >= 0xe000));
    assert(code_point < 0x110000);
    assert(output_buffer != NULL);

    if (code_point <= 0xffff)
      {
        output_buffer[0] = (unsigned)(code_point);
        return 1;
      }
    else
      {
        output_buffer[0] = (unsigned)(((code_point >> 10) - 0x40) | 0xd800);
        output_buffer[1] = (unsigned)((code_point & 0x3ff) | 0xdc00);
        return 2;
      }
  }

extern size_t code_point_to_utf8(unsigned long code_point, char *output_buffer)
  {
    assert((code_point < 0xd800) || (code_point >= 0xe000));
    assert(code_point < 0x110000);
    assert(output_buffer != NULL);

    if ((code_point & 0x7f) == code_point)
      {
        ((unsigned char *)output_buffer)[0] = (unsigned char)code_point;
        return 1;
      }
    else if ((code_point & 0x7ff) == code_point)
      {
        ((unsigned char *)output_buffer)[0] =
                (unsigned char)((code_point >> 6) | 0xc0);
        ((unsigned char *)output_buffer)[1] =
                (unsigned char)((code_point & 0x3f) | 0x80);
        return 2;
      }
    else if ((code_point & 0xffff) == code_point)
      {
        ((unsigned char *)output_buffer)[0] =
                (unsigned char)((code_point >> 12) | 0xe0);
        ((unsigned char *)output_buffer)[1] =
                (unsigned char)(((code_point >> 6) & 0x3f) | 0x80);
        ((unsigned char *)output_buffer)[2] =
                (unsigned char)((code_point & 0x3f) | 0x80);
        return 3;
      }
    else
      {
        assert((code_point & 0x1fffff) == code_point);
        ((unsigned char *)output_buffer)[0] =
                (unsigned char)((code_point >> 18) | 0xf0);
        ((unsigned char *)output_buffer)[1] =
                (unsigned char)(((code_point >> 12) & 0x3f) | 0x80);
        ((unsigned char *)output_buffer)[2] =
                (unsigned char)(((code_point >> 6) & 0x3f) | 0x80);
        ((unsigned char *)output_buffer)[3] =
                (unsigned char)((code_point & 0x3f) | 0x80);
        return 4;
      }
  }

extern int utf8_string_lexicographical_order_by_code_point(
        const char *utf8_left, const char *utf8_right)
  {
    const char *follow_left;
    const char *follow_right;

    assert(utf8_left != NULL);
    assert(utf8_right != NULL);

    follow_left = utf8_left;
    follow_right = utf8_right;

    while (TRUE)
      {
        unsigned char left_char;
        unsigned char right_char;

        left_char = *follow_left;
        right_char = *follow_right;

        if (left_char < right_char)
            return -1;

        if (left_char > right_char)
            return 1;

        if (left_char == 0)
            return 0;

        ++follow_left;
        ++follow_right;
      }
  }

extern size_t utf8_string_character_count(const char *utf8_string)
  {
    const char *follow;
    size_t result;

    follow = utf8_string;
    result = 0;

    while (*follow != 0)
      {
        follow += validate_utf8_character(follow);
        ++result;
      }

    return result;
  }

extern size_t bytes_for_utf8_character_count(const char *utf8_string,
                                             size_t character_count)
  {
    const char *follow;
    size_t characters_left;

    follow = utf8_string;
    characters_left = character_count;

    while (characters_left > 0)
      {
        assert(*follow != 0);
        follow += validate_utf8_character(follow);
        --characters_left;
      }

    return (follow - utf8_string);
  }

extern boolean string_is_utf8(const char *bytes)
  {
    const char *follow;

    follow = bytes;
    while (*follow != 0)
      {
        int character_bytes;

        character_bytes = validate_utf8_character_with_error_handler(follow,
                empty_error_wrapper, NULL);
        if (character_bytes < 0)
            return FALSE;
        assert(character_bytes > 0);
        follow += character_bytes;
      }

    return TRUE;
  }


static void basic_error_wrapper(void *data, const char *format, ...)
  {
    va_list ap;

    assert(data == NULL);

    va_start(ap, format);
    vbasic_error(format, ap);
    va_end(ap);
  }

static void empty_error_wrapper(void *data, const char *format, ...)
  {
    assert(data == NULL);
  }
