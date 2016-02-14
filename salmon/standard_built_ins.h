/* file "standard_built_ins.h" */

/*
 *  This file contains the interface to the standard_built_ins module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef STANDARD_BUILT_INS_H
#define STANDARD_BUILT_INS_H


#include "routine_declaration.h"


typedef enum
  {
    UTF_8,
    UTF_16_LE,
    UTF_16_BE,
    UTF_32_LE,
    UTF_32_BE
  } utf_choice;


extern declaration *create_standard_built_ins_class_declaration(void);
extern void cleanup_standard_built_ins_module(void);

extern void hook_file_pointer_to_object(object *the_object, FILE *fp,
        utf_choice utf_format, char *file_description, jumper *the_jumper);
extern void hook_file_pointer_to_object_for_named_file(object *the_object,
        FILE *fp, utf_choice utf_format, const char *file_name,
        jumper *the_jumper);
extern value *fp_input_text_read_character_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
extern value *fp_input_text_read_string_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
extern value *fp_input_text_read_line_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
extern value *fp_flush_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
extern value *fp_close_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
extern value *fp_tell_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
extern value *fp_seek_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
extern value *fp_seek_end_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
extern value *fp_output_text_print_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
extern value *fp_output_text_printf_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
extern value *fp_input_bit_read_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
extern value *fp_is_end_of_input_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
extern value *fp_output_bit_write_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);

DEFINE_EXCEPTION_TAG(assertion_failure);
DEFINE_EXCEPTION_TAG(make_string_undefined);
DEFINE_EXCEPTION_TAG(from_utf8_undefined);
DEFINE_EXCEPTION_TAG(from_utf8_more_than_one);
DEFINE_EXCEPTION_TAG(from_utf16_undefined);
DEFINE_EXCEPTION_TAG(from_utf16_more_than_one);
DEFINE_EXCEPTION_TAG(string_from_utf8_undefined);
DEFINE_EXCEPTION_TAG(string_from_utf16_undefined);
DEFINE_EXCEPTION_TAG(string_from_utf32_undefined);
DEFINE_EXCEPTION_TAG(delete_variable_component);
DEFINE_EXCEPTION_TAG(delete_variable_overloaded);
DEFINE_EXCEPTION_TAG(delete_variable_automatic);
DEFINE_EXCEPTION_TAG(delete_routine_automatic);
DEFINE_EXCEPTION_TAG(delete_routine_active);
DEFINE_EXCEPTION_TAG(delete_tagalong_key_automatic);
DEFINE_EXCEPTION_TAG(delete_lepton_key_automatic);
DEFINE_EXCEPTION_TAG(delete_quark_automatic);
DEFINE_EXCEPTION_TAG(delete_lock_automatic);
DEFINE_EXCEPTION_TAG(delete_object_incomplete);
DEFINE_EXCEPTION_TAG(split_null_match);
DEFINE_EXCEPTION_TAG(join_undefined);
DEFINE_EXCEPTION_TAG(filter_undefined);
DEFINE_EXCEPTION_TAG(filter_doubt);
DEFINE_EXCEPTION_TAG(substitute_null_match);
DEFINE_EXCEPTION_TAG(parse_regular_expression_bad_pattern);
DEFINE_EXCEPTION_TAG(character_set_undefined);
DEFINE_EXCEPTION_TAG(array_length_indeterminate);
DEFINE_EXCEPTION_TAG(call_undefined);
DEFINE_EXCEPTION_TAG(closed_stream_used);
DEFINE_EXCEPTION_TAG(file_tell_failed);
DEFINE_EXCEPTION_TAG(file_seek_failed);
DEFINE_EXCEPTION_TAG(file_open_failed);
DEFINE_EXCEPTION_TAG(file_io_failure);
DEFINE_EXCEPTION_TAG(directory_read_failed);
DEFINE_EXCEPTION_TAG(remove_failed);
DEFINE_EXCEPTION_TAG(rename_failed);
DEFINE_EXCEPTION_TAG(throw_unnamed_extra);
DEFINE_EXCEPTION_TAG(current_exceptions_no_exception);
DEFINE_EXCEPTION_TAG(bad_utf8);
DEFINE_EXCEPTION_TAG(bad_utf16);
DEFINE_EXCEPTION_TAG(bad_utf32);
DEFINE_EXCEPTION_TAG(read_count_too_big);
DEFINE_EXCEPTION_TAG(read_bit_count_not_8_divisible);
DEFINE_EXCEPTION_TAG(write_count_too_big);
DEFINE_EXCEPTION_TAG(write_bit_count_not_8_divisible);
DEFINE_EXCEPTION_TAG(write_missing_element);
DEFINE_EXCEPTION_TAG(write_element_too_big);
DEFINE_EXCEPTION_TAG(array_too_large);
DEFINE_EXCEPTION_TAG(sprint_not_string);
DEFINE_EXCEPTION_TAG(printf_unclosed_specifier);
DEFINE_EXCEPTION_TAG(printf_too_few_arguments);
DEFINE_EXCEPTION_TAG(printf_zero_argument);
DEFINE_EXCEPTION_TAG(printf_bad_argument_name);
DEFINE_EXCEPTION_TAG(printf_sprint_not_string);
DEFINE_EXCEPTION_TAG(printf_bad_specifier);
DEFINE_EXCEPTION_TAG(printf_not_string);
DEFINE_EXCEPTION_TAG(printf_not_integer);
DEFINE_EXCEPTION_TAG(printf_not_rational);


#endif /* STANDARD_BUILT_INS_H */
