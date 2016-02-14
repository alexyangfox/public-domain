/* file "unicode.h" */

/*
 *  This file contains the interface to the unicode module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef UNICODE_H
#define UNICODE_H

#include <stddef.h>


extern int validate_utf8_character(const char *character_start);
extern int validate_utf8_character_with_error_handler(
        const char *character_start,
        void (*error_handler_function)(void *data, const char *format, ...),
        void *error_handler_data);
extern void convert_utf16_to_utf8(unsigned int *utf16, size_t block_count,
        char *output_buffer, size_t *character_count, size_t *byte_count,
        void (*error_handler_function)(void *data, const char *format, ...),
        void *error_handler_data);
extern unsigned long utf8_to_code_point(const char *character_start);
extern unsigned long utf16_to_code_point(unsigned *u16_start);
extern size_t code_point_to_utf16(unsigned long code_point,
                                  unsigned *output_buffer);
extern size_t code_point_to_utf8(unsigned long code_point,
                                 char *output_buffer);
extern int utf8_string_lexicographical_order_by_code_point(
        const char *utf8_left, const char *utf8_right);
extern size_t utf8_string_character_count(const char *utf8_string);
extern size_t bytes_for_utf8_character_count(const char *utf8_string,
                                             size_t character_count);
extern boolean string_is_utf8(const char *bytes);


#endif /* UNICODE_H */
