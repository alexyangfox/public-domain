/* file "standard_built_ins.c" */

/*
 *  This file contains the implementation of the standard_built_ins module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/buffer_print.h"
#include "c_foundations/diagnostic.h"
#include "standard_built_ins.h"
#include "include.h"
#include "value.h"
#include "routine_instance.h"
#include "type.h"
#include "native_bridge.h"
#include "unicode.h"
#include "execute.h"
#include "driver.h"
#include "utility.h"
#include "platform_dependent.h"


#define PRINTF_PAD_BLOCK_SIZE 40


typedef struct
  {
    verdict verdict;
    jumper *jumper;
    const source_location *location;
  } call_printer_data;

typedef struct
  {
    string_buffer *output_buffer;
    call_printer_data cp_data;
  } string_printer_data;

typedef struct fp_call_printer_data fp_call_printer_data;

struct fp_call_printer_data
  {
    FILE *fp;
    call_printer_data cp_data;
    utf_choice utf_format;
    char *file_description;
  };

typedef struct
  {
    jumper *jumper;
    const source_location *location;
  } directory_read_error_handler_data;

typedef void diagnostic_handler_type(void *data, diagnostic_kind kind,
        boolean has_message_number, size_t message_number,
        const char *file_name, boolean has_line_number, size_t line_number,
        boolean has_column_number, size_t column_number,
        boolean has_character_count, size_t character_count,
        const char *format, va_list arg);

typedef struct
  {
    jumper *jumper;
    const source_location *location;
    object *the_object;
    lepton_key_instance *io_error_key;
  } exception_error_handler_data;


DECLARE_SYSTEM_LOCK(getenv_lock);


static value *get_integer_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_rational_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_string_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_character_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_regular_expression_type_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_any_quark_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_any_lepton_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_lepton_key_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_jump_target_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_any_class_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_object_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_tagalong_key_type_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_any_lock_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_null_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_true_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_false_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *characters_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *make_string_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *from_utf8_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *from_utf16_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *from_utf32_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *to_utf8_character_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *to_utf16_character_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *to_utf32_character_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *string_from_utf8_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *string_from_utf16_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *string_from_utf32_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *to_utf8_string_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *to_utf16_string_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *to_utf32_string_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *delete_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *matches_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *split_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *join_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *join_function_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *filter_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *filter_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *substitute_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *substitute_function_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *pattern_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *parse_regular_expression_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *exact_string_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *exact_character_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *character_set_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *character_range_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *concatenate_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *or_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *repeat_zero_or_more_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *repeat_one_or_more_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *re_follower_init_hook_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *re_follower_transit_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *re_follower_end_transit_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *re_follower_is_in_accepting_state_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *re_follower_more_possible_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *length_string_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *length_array_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *tag_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *call_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *internal_call_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *return_value_expected_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *set_fp_stream_to_standard_input_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *set_fp_stream_to_standard_output_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *set_fp_stream_to_standard_error_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *set_fp_stream_to_named_input_text_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *set_fp_stream_to_named_output_text_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *set_fp_stream_to_named_input_output_text_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *set_fp_stream_to_named_text_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location, boolean input, boolean output);
static value *set_fp_stream_to_named_input_bit_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *set_fp_stream_to_named_output_bit_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *set_fp_stream_to_named_input_output_bit_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *set_fp_stream_to_named_bit_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location, boolean input, boolean output);
static value *file_exists_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *directory_exists_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *directory_contents_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *remove_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *rename_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *sprint_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *sprintf_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *system_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *assert_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *why_not_in_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *initialize_context_switching_lock_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_time_and_date_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *throw_new_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *throw_old_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *current_exceptions_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *numerator_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *denominator_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *power_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *set_source_region_key_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *set_exception_key_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static value *set_standard_library_object_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location);
static value *get_environment_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);
static exception_error_handler_data *create_exception_error_data(
        jumper *the_jumper, const source_location *location);
static exception_error_handler_data *create_exception_error_data_with_object(
        jumper *the_jumper, const source_location *location,
        object *the_object, lepton_key_instance *io_error_key);
static void exception_error_handler(void *data, const char *format, ...);
static size_t array_value_element_count(value *array_value,
        const char *routine_name, jumper *the_jumper,
        const source_location *location);
static value *array_value_element_value(value *array_value, size_t element_num,
        jumper *the_jumper, const source_location *location);
static unsigned long array_value_element_u32(value *array_value,
        size_t element_num, jumper *the_jumper,
        const source_location *location, static_exception_tag *undefined_tag,
        const char *function_name);
static void overload_print_value(value *the_value,
        void (*printer)(void *data, const char *format, ...), void *data,
        call_printer_data *cp_data,
        void (*override)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data));
static void overload_printf(value *all_arguments_value,
        size_t format_argument_number,
        void (*printer)(void *data, const char *format, ...), void *data,
        call_printer_data *cp_data,
        void (*override)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data));
static value *try_formatting_call(value *to_convert, const char *format,
        size_t format_length, boolean flag_plus, boolean flag_space,
        boolean flag_octothorp, o_integer zero_width, o_integer precision,
        jumper *the_jumper, const source_location *location);
static void print_with_width(
        void (*printer)(void *data, const char *format, ...), void *data,
        const char *to_print, o_integer width, boolean flag_minus,
        boolean use_zeros, size_t zeros_position, char zeros_character,
        jumper *the_jumper);
static void do_padding(void (*printer)(void *data, const char *format, ...),
        void *data, o_integer width, size_t characters_used,
        char pad_character, jumper *the_jumper);
static void print_value_with_formatting(value *to_convert, const char *format,
        size_t format_length,
        void (*printer)(void *data, const char *format, ...), void *data,
        call_printer_data *cp_data,
        void (*override)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data), boolean flag_plus, boolean flag_space,
        boolean flag_octothorp, boolean *have_zeros_position,
        size_t *zeros_position, char *zeros_character, o_integer precision,
        jumper *the_jumper);
static void print_integer(value *to_convert, size_t base, boolean use_capitals,
        void (*printer)(void *data, const char *format, ...), void *data,
        boolean flag_plus, boolean flag_space, boolean flag_octothorp,
        boolean *have_zeros_position, size_t *zeros_position,
        char *zeros_character, o_integer precision, boolean complement,
        const source_location *location, jumper *the_jumper);
static void do_print_oi(o_integer to_convert, size_t base,
        boolean use_capitals,
        void (*printer)(void *data, const char *format, ...), void *data,
        boolean flag_plus, boolean flag_space, boolean flag_octothorp,
        boolean *have_zeros_position, size_t *zeros_position,
        char *zeros_character, o_integer precision, boolean complement,
        size_t *real_digit_count, jumper *the_jumper);
static void print_rational(value *to_convert, size_t base,
        boolean use_capitals,
        void (*printer)(void *data, const char *format, ...), void *data,
        boolean flag_plus, boolean flag_space, boolean flag_octothorp,
        boolean *have_zeros_position, size_t *zeros_position,
        char *zeros_character, o_integer precision, boolean complement,
        boolean decimal_point_format_allowed,
        boolean scientific_notation_allowed, char exponent_character,
        const source_location *location, jumper *the_jumper);
static void print_in_decimal_point_format(rational *to_print, size_t base,
        boolean use_capitals,
        void (*printer)(void *data, const char *format, ...), void *data,
        boolean flag_plus, boolean flag_space, boolean flag_octothorp,
        boolean *have_zeros_position, size_t *zeros_position,
        char *zeros_character, o_integer precision, boolean complement,
        boolean omit_trailing_point, boolean omit_trailing_zeros,
        boolean exactly_one_pre_point, boolean *overflow, jumper *the_jumper,
        const source_location *location);
static void string_print_value(value *the_value,
        void (*printer)(void *data, const char *format, ...), void *data);
static void string_printer(void *data, const char *format, ...);
static void fp_call_print_value(value *the_value,
        void (*printer)(void *data, const char *format, ...), void *data);
static void fp_call_printer(void *data, const char *format, ...);
static void null_printer(void *data, const char *format, ...);
static utf_choice utf_choice_from_value(value *format_value);
static utf_choice utf_choice_from_endian_value(value *format_value);
static boolean boolean_from_value(value *the_value);
static value *vcreate_io_error_value(lepton_key_instance *io_error_key,
        jumper *the_jumper, const char *message, va_list ap);
static void set_fp_object_error_flag_if_needed(object *the_object, FILE *fp,
        lepton_key_instance *io_error_key, jumper *the_jumper);
static void set_fp_object_eof_and_error_flags_if_needed(object *the_object,
        FILE *fp, lepton_key_instance *io_error_key, jumper *the_jumper);
static void object_set_variable_field(object *the_object,
        const char *field_name, value *new_value, jumper *the_jumper);
static void set_fp_object_error_info(object *the_object,
        lepton_key_instance *io_error_key, jumper *the_jumper,
        const char *format, ...);
static void set_fp_object_error_info_and_do_exception(object *the_object,
        lepton_key_instance *io_error_key, jumper *the_jumper,
        const source_location *location, const char *format, ...);
static void vset_fp_object_error_info(object *the_object,
        lepton_key_instance *io_error_key, jumper *the_jumper,
        const char *format, va_list ap);
static value *vset_fp_object_error_info_return_error_value(object *the_object,
        lepton_key_instance *io_error_key, jumper *the_jumper,
        const char *format, va_list ap);
static void set_fp_object_eof_info(object *the_object, jumper *the_jumper);
static void exception_and_fp_error(object *the_object,
        lepton_key_instance *io_error_key, jumper *the_jumper,
        const source_location *location, static_exception_tag *tag,
        const char *format, ...);
static void tell_error_handler(void *data, const char *format, ...);
static void seek_error_handler(void *data, const char *format, ...);
static fp_call_printer_data *fp_data_for_argument(value *all_arguments_value,
                                                  size_t argument_number);
static const char *string_for_argument(value *all_arguments_value,
                                       size_t argument_number);
static o_integer integer_for_argument(value *all_arguments_value,
                                      size_t argument_number);
static object *object_for_argument(value *all_arguments_value,
                                   size_t argument_number);
static regular_expression *regular_expression_for_argument(
        value *all_arguments_value, size_t argument_number);
static type *type_for_argument(value *all_arguments_value,
                               size_t argument_number);
static unsigned long hex_digit_to_bits(char digit);
static o_integer oi_power(o_integer base, o_integer exponent);
static rational *find_mantissa_and_exponent(rational *whole_rational,
                                            size_t base, o_integer *exponent);
static size_t approximate_digit_count(o_integer the_oi, size_t base);
static size_t identifier_length(const char *string);
static o_integer map_value_length(value *array_value, const char *routine_name,
        jumper *the_jumper, const source_location *location);
static void directory_read_error_handler(void *data, const char *format, ...);
static void delete_for_routine_instance(routine_instance *instance,
        jumper *the_jumper, const source_location *location);
static verdict fp_read_character(fp_call_printer_data *fp_data,
        char *char_buffer, object *the_object,
        lepton_key_instance *io_error_key, jumper *the_jumper,
        const source_location *location);
static void destroy_fp_data(fp_call_printer_data *fp_data);
static void re_follower_cleaner(void *hook, jumper *the_jumper);
static void fp_cleaner(void *hook, jumper *the_jumper);


static const native_bridge_function_info function_list[] =
  {
    ONE_STATEMENT("export;"),

    ONE_STATEMENT("immutable u8 := type [0 ... (1 << 8));"),
    ONE_STATEMENT("immutable u16 := type [0 ... (1 << 16));"),
    ONE_STATEMENT("immutable u32 := type [0 ... (1 << 32));"),
    ONE_STATEMENT("immutable u64 := type [0 ... (1 << 64));"),
    ONE_STATEMENT("immutable u128 := type [0 ... (1 << 128));"),

    ONE_STATEMENT("immutable s8 := type [(-(1 << 7)) ... (1 << 7));"),
    ONE_STATEMENT("immutable s16 := type [(-(1 << 15)) ... (1 << 15));"),
    ONE_STATEMENT("immutable s32 := type [(-(1 << 31)) ... (1 << 31));"),
    ONE_STATEMENT("immutable s64 := type [(-(1 << 63)) ... (1 << 63));"),
    ONE_STATEMENT("immutable s128 := type [(-(1 << 127)) ... (1 << 127));"),

    ONE_STATEMENT("hide;"),

    ONE_ROUTINE("function get_integer_type();",
                &get_integer_type_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_rational_type();",
                &get_rational_type_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_string_type();",
                &get_string_type_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_character_type();",
                &get_character_type_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_regular_expression_type();",
                &get_regular_expression_type_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_any_quark_type();",
                &get_any_quark_type_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_any_lepton_type();",
                &get_any_lepton_type_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_lepton_key_type();",
                &get_lepton_key_type_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_jump_target_type();",
                &get_jump_target_type_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_any_class_type();",
                &get_any_class_type_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_object_type();",
                &get_object_type_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_tagalong_key_type();",
                &get_tagalong_key_type_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_any_lock_type();",
                &get_any_lock_type_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_null();", &get_null_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_true();", &get_true_handler_function, PURE_SAFE),
    ONE_ROUTINE("function get_false();", &get_false_handler_function,
                PURE_SAFE),

    ONE_STATEMENT("export;"),

    ONE_STATEMENT("immutable integer := get_integer_type();"),
    ONE_STATEMENT("immutable rational := get_rational_type();"),
    ONE_STATEMENT("immutable string := get_string_type();"),
    ONE_STATEMENT("immutable character := get_character_type();"),
    ONE_STATEMENT(
            "immutable regular_expression := get_regular_expression_type();"),
    ONE_STATEMENT("immutable any_quark := get_any_quark_type();"),
    ONE_STATEMENT("immutable any_lepton := get_any_lepton_type();"),
    ONE_STATEMENT("immutable lepton_key := get_lepton_key_type();"),
    ONE_STATEMENT("immutable jump_target := get_jump_target_type();"),
    ONE_STATEMENT("immutable any_class := get_any_class_type();"),
    ONE_STATEMENT("immutable object := get_object_type();"),
    ONE_STATEMENT("immutable tagalong_key := get_tagalong_key_type();"),
    ONE_STATEMENT("immutable any_lock := get_any_lock_type();"),
    ONE_STATEMENT("ageless immutable null := get_null();"),
    ONE_STATEMENT("ageless immutable true := get_true();"),
    ONE_STATEMENT("ageless immutable false := get_false();"),
    ONE_STATEMENT("immutable negative_infinity := -oo;"),
    ONE_STATEMENT("immutable positive_infinity := +oo;"),
    ONE_STATEMENT("immutable unsigned_infinity := 1/0;"),
    ONE_STATEMENT("immutable zero_zero := 0/0;"),

    ONE_STATEMENT("immutable boolean := type { true, false };"),

    ONE_ROUTINE("function characters(: string) returns array[character];",
                &characters_handler_function, PURE_SAFE),
    ONE_ROUTINE("function make_string(: array[character]) returns string;",
                &make_string_handler_function, PURE_SAFE),

    ONE_STATEMENT("immutable unicode_code_point :=\n"
                  "        type [0 ... 0xd7ff] | [0xe000 ... 0x10ffff];"),

    ONE_ROUTINE("function from_utf8(: u8[4]) returns character;",
                &from_utf8_handler_function, PURE_SAFE),
    ONE_ROUTINE("function from_utf16(: u16[2]) returns character;",
                &from_utf16_handler_function, PURE_SAFE),
    ONE_ROUTINE("function from_utf32(: unicode_code_point) returns character;",
                &from_utf32_handler_function, PURE_SAFE),
    ONE_ROUTINE("function to_utf8(: character) returns u8[4];",
                &to_utf8_character_handler_function, PURE_SAFE),
    ONE_ROUTINE("function to_utf16(: character) returns u16[2];",
                &to_utf16_character_handler_function, PURE_SAFE),
    ONE_ROUTINE("function to_utf32(: character) returns unicode_code_point;",
                &to_utf32_character_handler_function, PURE_SAFE),

    ONE_ROUTINE("function string_from_utf8(: array[u8]) returns string;",
                &string_from_utf8_handler_function, PURE_SAFE),
    ONE_ROUTINE("function string_from_utf16(: array[u16]) returns string;",
                &string_from_utf16_handler_function, PURE_SAFE),
    ONE_ROUTINE(
            "function string_from_utf32(: array[unicode_code_point]) returns\n"
            "        string;", &string_from_utf32_handler_function, PURE_SAFE),
    ONE_ROUTINE("function to_utf8(: string) returns array[u8];",
                &to_utf8_string_handler_function, PURE_SAFE),
    ONE_ROUTINE("function to_utf16(: string) returns array[u16];",
                &to_utf16_string_handler_function, PURE_SAFE),
    ONE_ROUTINE(
            "function to_utf32(: string) returns array[unicode_code_point];",
            &to_utf32_string_handler_function, PURE_SAFE),

    ONE_ROUTINE(
            "procedure delete(: (*!{}) | ((!{}) <-- (*)) | tagalong_key | "
            "lepton_key | any_quark | any_lock | object);",
            &delete_handler_function, PURE_SAFE),

    ONE_ROUTINE(
            "function matches(: regular_expression, : string) returns "
            "boolean;", &matches_handler_function, PURE_SAFE),
    ONE_ROUTINE(
            "function split(: regular_expression, : string) returns "
            "array[string];", &split_handler_function, PURE_SAFE),
    ONE_ROUTINE("function join(: array[string], : string) returns string;",
                &join_handler_function, PURE_SAFE),
    ONE_ROUTINE(
            "function join(: array[string], : string <-- ()) returns string;",
            &join_function_handler_function, PURE_SAFE),
    ONE_ROUTINE(
            "function filter(: regular_expression, : array[string]) returns "
            "array[string];", &filter_handler_function, PURE_SAFE),
    ONE_ROUTINE("function filter(: type !{}, : array) returns array;",
                &filter_type_handler_function, PURE_SAFE),
    ONE_ROUTINE(
            "function substitute(: regular_expression, base : string, "
            "replacement : string) returns string;",
            &substitute_handler_function, PURE_SAFE),
    ONE_ROUTINE(
            "function substitute(: regular_expression, base : string,\n"
            "        replacement : string <-- (string)) returns string;",
            &substitute_function_handler_function, PURE_SAFE),
    ONE_ROUTINE("function pattern(: regular_expression) returns string;",
                &pattern_handler_function, PURE_SAFE),
    ONE_ROUTINE(
            "function parse_regular_expression(pattern : string) returns\n"
            "        regular_expression;",
            &parse_regular_expression_handler_function, PURE_SAFE),
    ONE_ROUTINE("function exact_string(: string) returns regular_expression;",
                &exact_string_handler_function, PURE_SAFE),
    ONE_ROUTINE(
            "function exact_character(: character) returns "
                    "regular_expression;", &exact_character_handler_function,
            PURE_SAFE),
    ONE_ROUTINE(
            "function character_set(: array[character]) returns\n"
            "        regular_expression;", &character_set_handler_function,
            PURE_SAFE),
    ONE_ROUTINE(
            "function character_range(lower : character, upper : character)\n"
            "        returns regular_expression;",
            &character_range_handler_function, PURE_SAFE),
    ONE_ROUTINE(
            "function concatenate(: regular_expression, "
                                 ": regular_expression)\n"
            "        returns regular_expression;",
            &concatenate_handler_function, PURE_SAFE),
    ONE_STATEMENT(
        "function operator~(left : regular_expression,\n"
        "        right : regular_expression) returns regular_expression\n"
        "  (concatenate(left, right));"),
    ONE_ROUTINE(
            "function or(: regular_expression, : regular_expression) returns\n"
            "        regular_expression;", &or_handler_function, PURE_SAFE),
    ONE_STATEMENT(
            "function operator|(left : regular_expression,\n"
            "        right : regular_expression) returns regular_expression\n"
            "  (or(left, right));"),
    ONE_ROUTINE(
            "function repeat_zero_or_more(: regular_expression) returns\n"
            "        regular_expression;",
            &repeat_zero_or_more_handler_function, PURE_SAFE),
    ONE_STATEMENT(
            "function operator*(to_repeat : regular_expression)\n"
            "        returns regular_expression\n"
            "  (repeat_zero_or_more(to_repeat));"),
    ONE_ROUTINE(
            "function repeat_one_or_more(: regular_expression) returns\n"
            "        regular_expression;",
            &repeat_one_or_more_handler_function, PURE_SAFE),
    ONE_STATEMENT(
            "function operator+(to_repeat : regular_expression)\n"
            "        returns regular_expression\n"
            "  (repeat_one_or_more(to_repeat));"),

    ONE_STATEMENT("quark end_of_input;"),

    ONE_STATEMENT(
            "immutable regular_expression_follower := type interface\n"
            "  [\n"
            "    transit :- {} <-- (character | {end_of_input}),\n"
            "    is_in_accepting_state :- boolean <-- (),\n"
            "    more_possible :- boolean <-- ()\n"
            "  ];"),

    ONE_STATEMENT("hide;"),

    ONE_STATEMENT(
            "class re_follower(the_re : regular_expression)\n"
            "  {\n"
            "    hide;\n"
            "\n"
            "    re_follower_init_hook(this, the_re);\n"
            "\n"
            "    export;\n"
            "\n"
            "    procedure transit(\n"
            "            new_character : character | {end_of_input})\n"
            "      {\n"
            "        if (new_character != end_of_input)\n"
            "            re_follower_transit(this, new_character);\n"
            "        else\n"
            "            re_follower_end_transit(this);;\n"
            "      };\n"
            "    function is_in_accepting_state() returns boolean\n"
            "      (re_follower_is_in_accepting_state(this));\n"
            "    function more_possible() returns boolean\n"
            "      (re_follower_more_possible(this));\n"
            "  };"),

    ONE_ROUTINE(
            "procedure re_follower_init_hook(follower : re_follower,\n"
            "                                the_re : regular_expression);",
            &re_follower_init_hook_handler_function, PURE_SAFE),
    ONE_ROUTINE(
            "procedure re_follower_transit(follower : re_follower,\n"
            "                              : character);",
            &re_follower_transit_handler_function, PURE_SAFE),
    ONE_ROUTINE("procedure re_follower_end_transit(follower : re_follower);",
                &re_follower_end_transit_handler_function, PURE_SAFE),
    ONE_ROUTINE(
            "function re_follower_is_in_accepting_state(\n"
            "        follower : re_follower) returns boolean;",
            &re_follower_is_in_accepting_state_handler_function, PURE_SAFE),
    ONE_ROUTINE(
            "function re_follower_more_possible(follower : re_follower)\n"
            "        returns boolean;",
            &re_follower_more_possible_handler_function, PURE_SAFE),

    ONE_STATEMENT("export;"),

    ONE_STATEMENT(
            "function create_follower(\n"
            "        the_regular_expression : regular_expression) returns\n"
            "                regular_expression_follower\n"
            "  {\n"
            "    return re_follower(the_regular_expression);\n"
            "  };"),

    ONE_ROUTINE("function length(: string) returns [0 ... +oo);",
                &length_string_handler_function, PURE_SAFE),
    ONE_ROUTINE("function length(: array) returns [0 ... +oo);",
                &length_array_handler_function, PURE_SAFE),

    ONE_ROUTINE(
            "function tag(value, label : string) returns [!{}] / [{value}];",
            &tag_handler_function, PURE_SAFE),

    ONE_ROUTINE(
            "function call(to_call : (!{}) <-- (*), arguments : [...])\n"
            "        returns !{};", &call_handler_function, PURE_SAFE),

    ONE_ROUTINE(
            "function internal_call(to_call : (!{}) <-- (*),\n"
            "                       arguments : [...]) returns !{};",
            &internal_call_handler_function, PURE_SAFE),

    ONE_ROUTINE(
            "function return_value_expected(: jump_target) returns boolean;",
            &return_value_expected_handler_function, PURE_SAFE),

    ONE_STATEMENT(
            "class integer_range(ageless lower_bound : [-oo...+oo],\n"
            "        ageless upper_bound : [-oo...+oo],\n"
            "        ageless lower_is_inclusive : boolean,\n"
            "        ageless upper_is_inclusive : boolean)\n"
            "  {\n"
            "    class iterator()\n"
            "      {\n"
            "        hide;\n"
            "\n"
            "        variable position : [lower_bound...(upper_bound + 1)]\n"
            "                := lower_bound;\n"
            "\n"
            "        export;\n"
            "\n"
            "        function is_done() returns boolean\n"
            "          (upper_is_inclusive ? (position > upper_bound) :\n"
            "           (position >= upper_bound));\n"
            "        function current() returns [lower_bound...upper_bound]\n"
            "          (position);\n"
            "        procedure step()\n"
            "          { ++position; };\n"
            "        function sprint() returns string\n"
            "          ((lower_is_inclusive ? \"[\" : \"(\") ~\n"
            "           sprint(lower_bound) ~ \"...\" ~ sprint(upper_bound)\n"
            "           ~ (upper_is_inclusive ? \"]\" : \")\"));\n"
            "\n"
            "        if (!lower_is_inclusive)\n"
            "            step();;\n"
            "      };\n"
            "  };"),

    ONE_STATEMENT("lepton io_error[message : string];"),

    ONE_STATEMENT(
            "immutable output_text_stream := type interface\n"
            "  [\n"
            "    print :- {} <-- (...),\n"
            "    printf :- {} <-- (format : string, ...),\n"
            "    close :- {} <-- (),\n"
            "    have_error :- boolean,\n"
            "    which_error :- io_error\n"
            "  ];"),

    ONE_STATEMENT(
            "immutable input_text_stream := type interface\n"
            "  [\n"
            "    read_character :- (character | {end_of_input}) <-- (),\n"
            "    read_string :- (string | {end_of_input}) <-- (\n"
            "            count : [0...+oo], minimum : [0...+oo] := *),\n"
            "    read_line :- (string | {end_of_input}) <-- (),\n"
            "    is_end_of_input :- boolean,\n"
            "    close :- {} <-- (),\n"
            "    have_error :- boolean,\n"
            "    which_error :- io_error\n"
            "  ];"),

    ONE_STATEMENT(
            "immutable look_ahead_stream := type interface\n"
            "  [\n"
            "    next_character :- (character | {end_of_input}) <-- (),\n"
            "    look_ahead :- (character | {end_of_input}) <-- ([0...+oo))\n"
            "  ];"),

    ONE_STATEMENT(
            "immutable output_bit_stream := type interface\n"
            "  [\n"
            "    write :- {} <-- (bits : [1...+oo),\n"
            "                     [0...+oo) | array[[0...+oo)]),\n"
            "    close :- {} <-- (),\n"
            "    have_error :- boolean,\n"
            "    which_error :- io_error\n"
            "  ];"),

    ONE_STATEMENT(
            "immutable input_bit_stream := type interface\n"
            "  [\n"
            "    read :- (array[[0...+oo)] | {end_of_input}) <-- (\n"
            "            bits : [1...+oo), count : [0...+oo),\n"
            "            minimum : [0...+oo) := *),\n"
            "    is_end_of_input :- boolean,\n"
            "    close :- {} <-- (),\n"
            "    have_error :- boolean,\n"
            "    which_error :- io_error\n"
            "  ];"),

    ONE_STATEMENT(
            "immutable seekable_stream := type interface\n"
            "  [\n"
            "    tell :- [0...+oo) <-- (),\n"
            "    seek :- {} <-- ([0...+oo)),\n"
            "    seek_end :- {} <-- ()\n"
            "  ];"),

    ONE_STATEMENT(
            "quark enumeration utf_choice\n"
            "  {utf_8, utf_16_le, utf_16_be, utf_32_le, utf_32_be};"),

    ONE_STATEMENT("quark enumeration endianness {big_endian, little_endian};"),

    ONE_STATEMENT("hide;"),

    ONE_STATEMENT(
            "class fp_input_text_stream()\n"
            "  {\n"
            "    use fp_seekable_stream(this);\n"
            "\n"
            "    hide;\n"
            "\n"
            "    virtual variable close_needed : boolean := true;\n"
            "\n"
            "    export;\n"
            "\n"
            "    function read_character() returns\n"
            "            (character | {end_of_input})\n"
            "      {\n"
            "        return fp_input_text_read_character(this);\n"
            "      };\n"
            "    function read_string(count : [0...+oo],\n"
            "            minimum : [0...+oo] / [0...count] := count)\n"
            "                    returns (string | {end_of_input})\n"
            "      {\n"
            "        return fp_input_text_read_string(this, count, minimum);\n"
            "      };\n"
            "    function read_line() returns (string | {end_of_input})\n"
            "      {\n"
            "        return fp_input_text_read_line(this);\n"
            "      };\n"
            "    variable is_end_of_input : boolean := false;\n"
            "    procedure close()\n"
            "      {\n"
            "        if (close_needed)\n"
            "          {\n"
            "            fp_input_text_close(this);\n"
            "            close_needed := false;\n"
            "          };\n"
            "      };\n"
            "    variable have_error : boolean := false;\n"
            "    variable which_error : io_error;\n"
            "  };"),

    ONE_ROUTINE(
            "function fp_input_text_read_character(: fp_input_text_stream,\n"
            "        : {io_error} := io_error,\n"
            "        : {end_of_input} := end_of_input) returns\n"
            "                (character | {end_of_input});",
            &fp_input_text_read_character_handler_function, PURE_UNSAFE),

    ONE_ROUTINE(
            "function fp_input_text_read_string(: fp_input_text_stream,\n"
            "        count : [0...+oo],\n"
            "        minimum : [0...+oo] / [0...count] := count,\n"
            "        : {io_error} := io_error,\n"
            "        : {end_of_input} := end_of_input) returns\n"
            "                (string | {end_of_input});",
            &fp_input_text_read_string_handler_function, PURE_UNSAFE),

    ONE_ROUTINE(
            "function fp_input_text_read_line(: fp_input_text_stream,\n"
            "        : {io_error} := io_error,\n"
            "        : {end_of_input} := end_of_input) returns\n"
            "                (string | {end_of_input});",
            &fp_input_text_read_line_handler_function, PURE_UNSAFE),

    ONE_ROUTINE("procedure fp_input_text_close(: fp_input_text_stream);",
                &fp_close_handler_function, PURE_UNSAFE),

    ONE_ROUTINE("function fp_tell(: object) returns [0...+oo);",
                &fp_tell_handler_function, PURE_UNSAFE),

    ONE_ROUTINE("procedure fp_seek(: object, position : [0...+oo));",
                &fp_seek_handler_function, PURE_UNSAFE),

    ONE_ROUTINE("procedure fp_seek_end(: object);",
                &fp_seek_end_handler_function, PURE_UNSAFE),

    ONE_STATEMENT(
            "class fp_seekable_stream(fp_stream : object)\n"
            "  {\n"
            "    function tell() returns [0...+oo)\n"
            "      (fp_tell(fp_stream));\n"
            "    procedure seek(position : [0...+oo))\n"
            "      { fp_seek(fp_stream, position); };\n"
            "    procedure seek_end()\n"
            "      { fp_seek_end(fp_stream); };\n"
            "  };"),

    ONE_STATEMENT(
            "class fp_output_text_stream()\n"
            "  {\n"
            "    use fp_seekable_stream(this);\n"
            "\n"
            "    hide;\n"
            "\n"
            "    virtual variable close_needed : boolean := true;\n"
            "\n"
            "    export;\n"
            "\n"
            "    procedure print(...)\n"
            "      {\n"
            "        internal_call(fp_output_text_print,\n"
            "             [this, io_error] ~ arguments);\n"
            "      };\n"
            "    procedure printf(format : string, ...)\n"
            "      {\n"
            "        internal_call(fp_output_text_printf,\n"
            "             [this, io_error] ~ arguments);\n"
            "      };\n"
            "    procedure flush()\n"
            "      {\n"
            "        fp_output_text_flush(this);\n"
            "      };\n"
            "    procedure close()\n"
            "      {\n"
            "        if (close_needed)\n"
            "          {\n"
            "            fp_output_text_close(this);\n"
            "            close_needed := false;\n"
            "          };\n"
            "      };\n"
            "    variable have_error : boolean := false;\n"
            "    variable which_error : io_error;\n"
            "  };"),

    ONE_ROUTINE(
            "procedure fp_output_text_print(: fp_output_text_stream,\n"
            "                               : {io_error}, ...);",
            &fp_output_text_print_handler_function, PURE_UNSAFE),

    ONE_ROUTINE(
            "procedure fp_output_text_printf(: fp_output_text_stream,\n"
            "        : {io_error}, format : string, ...);",
            &fp_output_text_printf_handler_function, PURE_UNSAFE),

    ONE_ROUTINE("procedure fp_output_text_flush(: fp_output_text_stream);",
                &fp_flush_handler_function, PURE_UNSAFE),

    ONE_ROUTINE("procedure fp_output_text_close(: fp_output_text_stream);",
                &fp_close_handler_function, PURE_UNSAFE),

    ONE_STATEMENT(
            "class fp_input_output_text_stream()\n"
            "  {\n"
            "    hide;\n"
            "\n"
            "    variable close_needed : boolean := true;\n"
            "\n"
            "    export;\n"
            "\n"
            "    use fp_input_text_stream();\n"
            "    use fp_output_text_stream();\n"
            "  };"),

    ONE_STATEMENT(
            "class fp_input_bit_stream()\n"
            "  {\n"
            "    use fp_seekable_stream(this);\n"
            "\n"
            "    hide;\n"
            "\n"
            "    virtual variable close_needed : boolean := true;\n"
            "\n"
            "    export;\n"
            "\n"
            "    function read(bits : [1...+oo), count : [0...+oo),\n"
            "                  minimum : [0...+oo) / [0...count] := count)\n"
            "            returns (array[[0...+oo)] | {end_of_input})\n"
            "      {\n"
            "        return fp_input_bit_read(this, bits, count, minimum);\n"
            "      };\n"
            "    variable is_end_of_input : boolean := false;\n"
            "    procedure close()\n"
            "      {\n"
            "        if (close_needed)\n"
            "          {\n"
            "            fp_input_bit_close(this);\n"
            "            close_needed := false;\n"
            "          };\n"
            "      };\n"
            "    variable have_error : boolean := false;\n"
            "    variable which_error : io_error;\n"
            "  };"),

    ONE_ROUTINE(
            "function fp_input_bit_read(: fp_input_bit_stream,\n"
            "        bits : [1...+oo), count : [0...+oo),\n"
            "        minimum : [0...+oo), : {io_error} := io_error,\n"
            "        : {end_of_input} := end_of_input) returns\n"
            "                (array[[0...+oo)] | {end_of_input});",
            &fp_input_bit_read_handler_function, PURE_UNSAFE),

    ONE_ROUTINE("procedure fp_input_bit_close(: fp_input_bit_stream);",
                &fp_close_handler_function, PURE_UNSAFE),

    ONE_ROUTINE(
            "function fp_is_end_of_input(\n"
            "        : fp_input_text_stream | fp_input_bit_stream) returns\n"
            "                boolean;", &fp_is_end_of_input_handler_function,
            PURE_UNSAFE),

    ONE_STATEMENT(
            "class fp_output_bit_stream()\n"
            "  {\n"
            "    use fp_seekable_stream(this);\n"
            "\n"
            "    hide;\n"
            "\n"
            "    virtual variable close_needed : boolean := true;\n"
            "\n"
            "    export;\n"
            "\n"
            "    procedure write(bits : [1...+oo),\n"
            "                    data : [0...+oo) | array[[0...+oo)])\n"
            "      {\n"
            "        fp_output_bit_write(this, bits, data, io_error);\n"
            "      };\n"
            "    procedure flush()\n"
            "      {\n"
            "        fp_output_bit_flush(this);\n"
            "      };\n"
            "    procedure close()\n"
            "      {\n"
            "        if (close_needed)\n"
            "          {\n"
            "            fp_output_bit_close(this);\n"
            "            close_needed := false;\n"
            "          };\n"
            "      };\n"
            "    variable have_error : boolean := false;\n"
            "    variable which_error : io_error;\n"
            "  };"),

    ONE_ROUTINE(
            "procedure fp_output_bit_write(: fp_output_bit_stream,\n"
            "        bits : [1...+oo), data : [0...+oo) | array[[0...+oo)],\n"
            "        : {io_error});", &fp_output_bit_write_handler_function,
            PURE_UNSAFE),

    ONE_ROUTINE("procedure fp_output_bit_flush(: fp_output_bit_stream);",
                &fp_flush_handler_function, PURE_UNSAFE),

    ONE_ROUTINE("procedure fp_output_bit_close(: fp_output_bit_stream);",
                &fp_close_handler_function, PURE_UNSAFE),

    ONE_STATEMENT(
            "class fp_input_output_bit_stream()\n"
            "  {\n"
            "    hide;\n"
            "\n"
            "    variable close_needed : boolean := true;\n"
            "\n"
            "    export;\n"
            "\n"
            "    use fp_input_bit_stream();\n"
            "    use fp_output_bit_stream();\n"
            "  };"),

    ONE_ROUTINE(
            "function set_fp_stream_to_standard_input(\n"
            "        : fp_input_text_stream) returns fp_input_text_stream;",
            &set_fp_stream_to_standard_input_handler_function, PURE_SAFE),

    ONE_ROUTINE(
            "function set_fp_stream_to_standard_output(\n"
            "        : fp_output_text_stream) returns fp_output_text_stream;",
            &set_fp_stream_to_standard_output_handler_function, PURE_SAFE),

    ONE_ROUTINE(
            "function set_fp_stream_to_standard_error(\n"
            "        : fp_output_text_stream) returns fp_output_text_stream;",
            &set_fp_stream_to_standard_error_handler_function, PURE_SAFE),

    ONE_ROUTINE(
            "function set_fp_stream_to_named_input_text_file(\n"
            "        : fp_input_text_stream, file_name : string,\n"
            "        format : utf_choice, : {io_error} := io_error)\n"
            "        returns fp_input_text_stream;",
            &set_fp_stream_to_named_input_text_file_handler_function,
            PURE_UNSAFE),

    ONE_ROUTINE(
            "function set_fp_stream_to_named_output_text_file(\n"
            "        : fp_output_text_stream, file_name : string,\n"
            "        format : utf_choice, : {io_error} := io_error,\n"
            "        append : boolean) returns fp_output_text_stream;",
            &set_fp_stream_to_named_output_text_file_handler_function,
            PURE_UNSAFE),

    ONE_ROUTINE(
            "function set_fp_stream_to_named_input_output_text_file(\n"
            "        : fp_input_text_stream & fp_output_text_stream,\n"
            "        file_name : string, format : utf_choice,\n"
            "        : {io_error} := io_error)\n"
            "        returns fp_input_text_stream & fp_output_text_stream;",
            &set_fp_stream_to_named_input_output_text_file_handler_function,
            PURE_UNSAFE),

    ONE_ROUTINE(
            "function set_fp_stream_to_named_input_bit_file(\n"
            "        : fp_input_bit_stream, file_name : string,\n"
            "        which_endian : endianness, : {io_error} := io_error)\n"
            "                returns fp_input_bit_stream;",
            &set_fp_stream_to_named_input_bit_file_handler_function,
            PURE_UNSAFE),

    ONE_ROUTINE(
            "function set_fp_stream_to_named_output_bit_file(\n"
            "        : fp_output_bit_stream, file_name : string,\n"
            "        which_endian : endianness, : {io_error} := io_error)\n"
            "                returns fp_output_bit_stream;",
            &set_fp_stream_to_named_output_bit_file_handler_function,
            PURE_UNSAFE),

    ONE_ROUTINE(
            "function set_fp_stream_to_named_input_output_bit_file(\n"
            "        : fp_input_bit_stream & fp_output_bit_stream,\n"
            "        file_name : string, which_endian : endianness,\n"
            "        : {io_error} := io_error) returns\n"
            "                fp_input_bit_stream & fp_output_bit_stream;",
            &set_fp_stream_to_named_input_output_bit_file_handler_function,
            PURE_UNSAFE),

    ONE_STATEMENT(
            "function get_standard_input() returns input_text_stream\n"
            "  {\n"
            "    variable result := set_fp_stream_to_standard_input(\n"
            "            fp_input_text_stream());\n"
            "    result.is_end_of_input := fp_is_end_of_input(result);\n"
            "    return result;\n"
            "  };"),

    ONE_STATEMENT(
            "function get_standard_output() returns output_text_stream\n"
            "  {\n"
            "    return set_fp_stream_to_standard_output(\n"
            "            fp_output_text_stream());\n"
            "  };"),

    ONE_STATEMENT(
            "function get_standard_error() returns output_text_stream\n"
            "  {\n"
            "    return set_fp_stream_to_standard_error(\n"
            "            fp_output_text_stream());\n"
            "  };"),

    ONE_STATEMENT(
            "class look_ahead_buffer(stream : input_text_stream)\n"
            "  {\n"
            "    export;\n"
            "\n"
            "    function read_character() returns\n"
            "            (character | {end_of_input})\n"
            "      {\n"
            "        if (length(buffer) == 0)\n"
            "          {\n"
            "            immutable result := stream.read_character();\n"
            "            copy_flags();\n"
            "            return result;\n"
            "          }\n"
            "        else\n"
            "          {\n"
            "            immutable result := buffer[0];\n"
            "            buffer := buffer[1...(length(buffer) - 1)];\n"
            "            if (length(buffer) == 0)\n"
            "                copy_flags();;\n"
            "            return result;\n"
            "          };\n"
            "      };\n"
            "    function read_string(count : [0...+oo],\n"
            "            minimum : [0...+oo] / [0...count] := count) returns\n"
            "                    (string | {end_of_input})\n"
            "      {\n"
            "        if (length(buffer) == 0)\n"
            "          {\n"
            "            immutable result :=\n"
            "                    stream.read_string(count, minimum);\n"
            "            copy_flags();\n"
            "            return result;\n"
            "          }\n"
            "        else if (count <= length(buffer))\n"
            "          {\n"
            "            immutable result :=\n"
            "                    make_string(buffer[0...(count - 1)]);\n"
            "            buffer := buffer[count...(length(buffer) - 1)];\n"
            "            if (length(buffer) == 0)\n"
            "                copy_flags();;\n"
            "            return result;\n"
            "          }\n"
            "        else\n"
            "          {\n"
            "            immutable tail := stream.read_string(\n"
            "                    count - length(buffer),\n"
            "                    ((minimum > length(buffer)) ?\n"
            "                     minimum - length(buffer) : 0));\n"
            "            immutable old_buffer := buffer;\n"
            "            buffer := [];\n"
            "            copy_flags();\n"
            "            if (tail == end_of_input)\n"
            "                return end_of_input;;\n"
            "            return make_string(old_buffer) ~ tail;\n"
            "          };\n"
            "      };\n"
            "    function read_line() returns (string | {end_of_input})\n"
            "      {\n"
            "        immutable buffer_length : [0...+oo) := length(buffer);\n"
            "        if (buffer_length == 0)\n"
            "          {\n"
            "            immutable result := stream.read_line();\n"
            "            copy_flags();\n"
            "            return result;\n"
            "          };\n"
            "        for (position; 0; position < buffer_length; 1)\n"
            "          {\n"
            "            if (buffer[position] == '\\n')\n"
            "              {\n"
            "                immutable result :=\n"
            "                        make_string(buffer[0...position - 1]);\n"
            "                buffer := buffer[\n"
            "                        (position + 1)...(buffer_length - 1)];\n"
            "                if (buffer_length == 0)\n"
            "                    copy_flags();;\n"
            "                return result;\n"
            "              };\n"
            "          };\n"
            "        immutable tail := stream.read_line();\n"
            "        immutable old_buffer := buffer;\n"
            "        buffer := [];\n"
            "        copy_flags();\n"
            "        if (tail == end_of_input)\n"
            "            return old_buffer;;\n"
            "        return make_string(old_buffer) ~ tail;\n"
            "      };\n"
            "    procedure close()\n"
            "      {\n"
            "        stream.close();\n"
            "        buffer := [];\n"
            "        copy_flags();\n"
            "      };\n"
            "\n"
            "    variable is_end_of_input : boolean;\n"
            "    variable have_error : boolean;\n"
            "    variable which_error : io_error;\n"
            "\n"
            "    function next_character() returns\n"
            "            (character | {end_of_input})\n"
            "      {\n"
            "        return look_ahead(0);\n"
            "      };\n"
            "\n"
            "    function look_ahead(amount : [0...+oo)) returns\n"
            "            (character | {end_of_input})\n"
            "      {\n"
            "        while (length(buffer) <= amount)\n"
            "          {\n"
            "            immutable new := stream.read_character();\n"
            "            if (new == end_of_input)\n"
            "                return new;;\n"
            "            buffer ~= [new];\n"
            "          };\n"
            "        return buffer[amount];\n"
            "      };\n"
            "\n"
            "    function base() returns input_text_stream\n"
            "      (stream);\n"
            "\n"
            "    hide;\n"
            "\n"
            "    variable buffer : array[character] := [];\n"
            "\n"
            "    procedure copy_flags()\n"
            "      {\n"
            "        is_end_of_input := stream.is_end_of_input;\n"
            "        have_error := stream.have_error;\n"
            "        if (have_error)\n"
            "            which_error := stream.which_error;;\n"
            "      };\n"
            "    copy_flags();\n"
            "  };"),

    ONE_STATEMENT("export;"),

    ONE_STATEMENT(
            "function add_text_look_ahead_buffer(stream : input_text_stream)\n"
            "        returns input_text_stream & look_ahead_stream\n"
            "  {\n"
            "    return look_ahead_buffer(stream);\n"
            "  };"),

    ONE_STATEMENT(
            "variable standard_input : input_text_stream & look_ahead_stream\n"
            "        := add_text_look_ahead_buffer(get_standard_input());"),
    ONE_STATEMENT("cleanup delete(standard_input.base());;\n"),

    ONE_STATEMENT(
            "variable standard_output : output_text_stream :=\n"
            "        get_standard_output();"),
    ONE_STATEMENT("cleanup delete(standard_output);;\n"),

    ONE_STATEMENT(
            "variable standard_error : output_text_stream :=\n"
            "        get_standard_error();"),
    ONE_STATEMENT("cleanup delete(standard_error);;\n"),

    ONE_STATEMENT(
            "function open_input_text_file(file_name : string,\n"
            "                              format : utf_choice := utf_8)\n"
            "        returns (input_text_stream & seekable_stream)\n"
            "  (internal_call(set_fp_stream_to_named_input_text_file,\n"
            "                 [fp_input_text_stream(), file_name, format]));"),

    ONE_STATEMENT(
            "function open_output_text_file(file_name : string,\n"
            "        format : utf_choice := utf_8,\n"
            "        append : boolean := false)\n"
            "                returns (output_text_stream & seekable_stream)\n"
            "  (internal_call(set_fp_stream_to_named_output_text_file,\n"
            "           [fp_output_text_stream(), file_name, format,\n"
            "            append := append]));"),

    ONE_STATEMENT(
            "function open_input_output_text_file(file_name : string,\n"
            "        format : utf_choice := utf_8)\n"
            "        returns (input_text_stream & output_text_stream &\n"
            "                 seekable_stream)\n"
            "  (internal_call(set_fp_stream_to_named_input_output_text_file,\n"
            "           [fp_input_output_text_stream(), file_name,\n"
            "            format]));"),

    ONE_STATEMENT(
            "function open_input_bit_file(file_name : string,\n"
            "        which_endian : endianness) returns\n"
            "                (input_bit_stream & seekable_stream)\n"
            "  (internal_call(set_fp_stream_to_named_input_bit_file,\n"
            "           [fp_input_bit_stream(), file_name, which_endian]));"),

    ONE_STATEMENT(
            "function open_output_bit_file(file_name : string,\n"
            "        which_endian : endianness) returns\n"
            "                (output_bit_stream & seekable_stream)\n"
            "  (internal_call(set_fp_stream_to_named_output_bit_file,\n"
            "           [fp_output_bit_stream(), file_name, which_endian]));"),

    ONE_STATEMENT(
            "function open_input_output_bit_file(file_name : string,\n"
            "        which_endian : endianness) returns\n"
            "                (input_bit_stream & output_bit_stream &\n"
            "                 seekable_stream)\n"
            "  (internal_call(set_fp_stream_to_named_input_output_bit_file,\n"
            "           [fp_input_output_bit_stream(), file_name,\n"
            "            which_endian]));"),

    ONE_STATEMENT("hide;"),

    ONE_STATEMENT(
            "class string_stream(base : string)\n"
            "  {\n"
            "    function read_character() returns\n"
            "            (character | {end_of_input})\n"
            "      {\n"
            "        assert(!closed);\n"
            "        if (position >= length(by_character))\n"
            "            return end_of_input;;\n"
            "        immutable result := by_character[position];\n"
            "        ++position;\n"
            "        is_end_of_input := (length(by_character) == position);\n"
            "        return result;\n"
            "      };\n"
            "    function read_string(count : [0...+oo],\n"
            "            minimum : [0...+oo] / [0...count] := count) returns\n"
            "                    (string | {end_of_input})\n"
            "      {\n"
            "        assert(!closed);\n"
            "        variable result_count : [0...+oo);\n"
            "        if (position + count > length(by_character))\n"
            "          {\n"
            "            result_count := length(by_character) - position;\n"
            "            if (result_count < minimum)\n"
            "                return end_of_input;;\n"
            "          }\n"
            "        else\n"
            "          {\n"
            "            result_count := count;\n"
            "          };\n"
            "        immutable result :=\n"
            "                make_string(by_character[position...\n"
            "                        ((position + result_count) - 1)]);\n"
            "        position += result_count;\n"
            "        is_end_of_input := (length(by_character) == position);\n"
            "        return result;\n"
            "      };\n"
            "    function read_line() returns (string | {end_of_input})\n"
            "      {\n"
            "        assert(!closed);\n"
            "        immutable character_length : [0...+oo) :=\n"
            "                length(by_character);\n"
            "        if (position >= character_length)\n"
            "            return end_of_input;;\n"
            "        variable new_position : [0...+oo) := position;\n"
            "        while (new_position < character_length)\n"
            "          {\n"
            "            if (by_character[new_position] == '\\n')\n"
            "              {\n"
            "                immutable result := make_string(\n"
            "                        by_character[position...\n"
            "                                     (new_position - 1)]);\n"
            "                position := new_position + 1;\n"
            "                is_end_of_input :=\n"
            "                        (character_length == position);\n"
            "                return result;\n"
            "              };\n"
            "            ++new_position;\n"
            "          };\n"
            "        immutable result := make_string(\n"
            "                by_character[position...\n"
            "                             (character_length - 1)]);\n"
            "        position := character_length;\n"
            "        is_end_of_input := true;\n"
            "        return result;\n"
            "      };\n"
            "    procedure close()\n"
            "      {\n"
            "        assert(!closed);\n"
            "        closed := true;\n"
            "      };\n"
            "    function next_character() returns\n"
            "            (character | {end_of_input})\n"
            "      {\n"
            "        assert(!closed);\n"
            "        if (position >= length(by_character))\n"
            "            return end_of_input;;\n"
            "        return by_character[position];\n"
            "      };\n"
            "    function look_ahead(amount : [0...+oo)) returns\n"
            "            (character | {end_of_input})\n"
            "      {\n"
            "        assert(!closed);\n"
            "        if (position + amount >= length(by_character))\n"
            "            return end_of_input;;\n"
            "        return by_character[position + amount];\n"
            "      };\n"
            "\n"
            "    variable is_end_of_input : boolean := (length(base) == 0);\n"
            "    immutable have_error : boolean := false;\n"
            "    variable which_error : io_error;\n"
            "\n"
            "    hide;\n"
            "\n"
            "    variable position : integer := 0;\n"
            "    immutable by_character : array[character] :=\n"
            "            characters(base);\n"
            "    variable closed : boolean := false;\n"
            "  };"),

    ONE_STATEMENT("export;"),

    ONE_STATEMENT(
            "function open_input_string(base : string) returns\n"
            "        input_text_stream & look_ahead_stream\n"
            "  {\n"
            "    return string_stream(base);\n"
            "  };"),

    ONE_STATEMENT("hide;"),

    ONE_STATEMENT(
            "class string_output_stream(location : +string)\n"
            "  {\n"
            "    procedure print(...)\n"
            "      {\n"
            "        assert(!closed);\n"
            "        *location ~= internal_call(sprint, arguments);\n"
            "      };\n"
            "    procedure printf(format : string, ...)\n"
            "      {\n"
            "        assert(!closed);\n"
            "        *location ~= internal_call(sprintf, arguments);\n"
            "      };\n"
            "    procedure close()\n"
            "      {\n"
            "        assert(!closed);\n"
            "        closed := true;\n"
            "      };\n"
            "\n"
            "    immutable have_error : boolean := false;\n"
            "    variable which_error : io_error;\n"
            "\n"
            "    hide;\n"
            "\n"
            "    variable closed : boolean := false;\n"
            "  };"),

    ONE_STATEMENT("export;"),

    ONE_STATEMENT(
            "function open_output_string(location : +string) returns\n"
            "        output_text_stream\n"
            "  {\n"
            "    return string_output_stream(location);\n"
            "  };"),

    ONE_STATEMENT("quark exception_tag_scanf_unexpected_end_of_input;"),
    ONE_STATEMENT("quark exception_tag_scanf_plain_text_mismatch;"),
    ONE_STATEMENT("quark exception_tag_scanf_unterminate_specifier;"),
    ONE_STATEMENT("quark exception_tag_scanf_no_regular_expression_match;"),
    ONE_STATEMENT("quark exception_tag_scanf_bad_value_specifier;"),
    ONE_STATEMENT("quark exception_tag_scanf_bad_integer;"),
    ONE_STATEMENT("quark exception_tag_scanf_missing_exponent;"),
    ONE_STATEMENT("quark exception_tag_scanf_bad_string_value;"),
    ONE_STATEMENT("quark exception_tag_scanf_bad_character_value;"),
    ONE_STATEMENT("quark exception_tag_scanf_bad_regular_expression;"),
    ONE_STATEMENT("quark exception_tag_scanf_bad_list_value;"),
    ONE_STATEMENT("quark exception_tag_scanf_bad_map_value;"),
    ONE_STATEMENT("quark exception_tag_scanf_bad_type_value;"),

    ONE_STATEMENT("hide;"),
    ONE_STATEMENT(
            "immutable english_letters := type\n"
            "  {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',\n"
            "   'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',\n"
            "   'y', 'z',\n"
            "   'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',\n"
            "   'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',\n"
            "   'Y', 'Z'};"),
    ONE_STATEMENT(
            "immutable decimal_digits := type\n"
            "  {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};"),
    ONE_STATEMENT(
            "immutable whitespace := type\n"
            "  {' ', '\\n', '\\t', '\\r', '\\f', '\\v'};"),
    ONE_STATEMENT("export;"),

    ONE_STATEMENT(
            "function scanf(stream : input_text_stream & look_ahead_stream,\n"
            "               format : string) returns [...]\n"
            "  {\n"
            "    use stream;\n"
            "\n"
            "    function match_one(match_char : character)\n"
            "      {\n"
            "        immutable next := read_character();\n"
            "        if (next == end_of_input)\n"
            "          {\n"
            "            throw(exception_tag_scanf_unexpected_end_of_input,\n"
            "                  \"Unexpected end-of-input encountered in \" ~\n"
            "                  \"scanf().\");\n"
            "          };\n"
            "        if (next != match_char)\n"
            "          {\n"
            "            throw(exception_tag_scanf_plain_text_mismatch,\n"
            "                  sprint(\"Plain text mismatch encountered \" ~\n"
            "                         \"in scanf(): expected `\",\n"
            "                         match_char, \"', found `\", next,\n"
            "                         \"'.\"));\n"
            "          };\n"
            "      };\n"
            "\n"
            "    function forward_is(test : string) returns boolean\n"
            "      {\n"
            "        immutable items := characters(test);\n"
            "        for (position; 0; position < length(test); 1)\n"
            "          {\n"
            "            if (items[position] != look_ahead(position))\n"
            "                return false;;\n"
            "          };\n"
            "        return true;\n"
            "      };\n"
            "\n"
            "    function negate(value : rational,\n"
            "            complement_negative : boolean, base : integer,\n"
            "            digit_count : integer) returns rational\n"
            "      {\n"
            "        return (complement_negative ?\n"
            "                (value - (power(base, digit_count))) :\n"
            "                -value);\n"
            "      };\n"
            "\n"
            "    function do_digits(base : [1...+oo),\n"
            "            finalize : !{} <-- (value : integer,\n"
            "                                digit_count : integer))\n"
            "            returns !{}\n"
            "      {\n"
            "        variable result : [0...+oo) := 0;\n"
            "        variable digit_count : [0...+oo) := 0;\n"
            "        procedure do_result()\n"
            "          {\n"
            "            return finalize(value := result,\n"
            "                    digit_count := digit_count) from do_digits;\n"
            "          };\n"
            "\n"
            "        while (true)\n"
            "          {\n"
            "            immutable ahead := look_ahead(0);\n"
            "            if (ahead == end_of_input)\n"
            "                do_result();;\n"
            "            immutable next := to_utf32(ahead);\n"
            "            variable digit_value : [0...+oo);\n"
            "\n"
            "            switch (next)\n"
            "            case ([to_utf32('0')...to_utf32('9')])\n"
            "                digit_value := next - to_utf32('0');\n"
            "            case ([to_utf32('a')...to_utf32('z')])\n"
            "                digit_value := (next - to_utf32('a')) + 0xa;\n"
            "            case ([to_utf32('A')...to_utf32('Z')])\n"
            "                digit_value := (next - to_utf32('A')) + 0xa;\n"
            "            case (!{})\n"
            "                do_result();;\n"
            "\n"
            "            if (digit_value >= base)\n"
            "                do_result();;\n"
            "\n"
            "            result := ((result * base) + digit_value);\n"
            "            ++digit_count;\n"
            "            [] := [read_character()];\n"
            "          };\n"
            "      };\n"
            "\n"
            "    function do_digits_and_point(base : [1...+oo),\n"
            "            complement_negative : boolean := true,\n"
            "            add_sign : rational <-- (value : rational,\n"
            "                                     digit_count : integer),\n"
            "            has_point : +.boolean := null) returns rational\n"
            "      {\n"
            "        variable base_digit_count : integer;\n"
            "        variable result : rational :=\n"
            "                do_digits(base, finalize := function(\n"
            "                        value : integer, digit_count : integer)\n"
            "                                returns integer\n"
            "          {\n"
            "            base_digit_count := digit_count;\n"
            "            return value;\n"
            "          });\n"
            "\n"
            "        if (look_ahead(0) == '.')\n"
            "          {\n"
            "            [] := [read_character()];\n"
            "\n"
            "            immutable fraction : rational := do_digits(\n"
            "                    base := base,\n"
            "                    finalize := function(value : integer,\n"
            "                            digit_count : integer) returns\n"
            "                                    rational\n"
            "              {\n"
            "                return (value / power(base, digit_count));\n"
            "              });\n"
            "            if (has_point != null)\n"
            "                *has_point := true;;\n"
            "            result += fraction;\n"
            "          };\n"
            "\n"
            "        return add_sign(result,\n"
            "                        digit_count := base_digit_count);\n"
            "      };\n"
            "\n"
            "    function do_digits_and_exponent(base : [1...+oo),\n"
            "            complement_negative : boolean,\n"
            "            add_sign : rational <-- (value : rational,\n"
            "                                     digit_count : integer),\n"
            "            optional : boolean,\n"
            "            has_point_or_exponent : +.boolean := null) returns\n"
            "                    rational\n"
            "      {\n"
            "        immutable result : rational := do_digits_and_point(\n"
            "                base := base, add_sign := add_sign,\n"
            "                has_point := has_point_or_exponent);\n"
            "\n"
            "        if ((look_ahead(0) == ((base < 0xe) ? 'e' : 'x')) ||\n"
            "            (look_ahead(0) == ((base < 0xe) ? 'E' : 'X')))\n"
            "          {\n"
            "            [] := [read_character()];\n"
            "\n"
            "            variable exponent_negative := false;\n"
            "            if ((look_ahead(0) == '-') ||\n"
            "                (look_ahead(0) == '+'))\n"
            "              {\n"
            "                immutable sign := read_character();\n"
            "                exponent_negative := (sign == '-');\n"
            "              };\n"
            "            immutable exponent := do_digits(base := base,\n"
            "                    finalize := function(value : integer,\n"
            "                            digit_count : integer) returns\n"
            "                                    rational\n"
            "              {\n"
            "                if (digit_count == 0)\n"
            "                  {\n"
            "                    throw(exception_tag_scanf_bad_integer,\n"
            "                          \"Expected exponent digits not \" ~\n"
            "                          \"found.\");\n"
            "                  };\n"
            "                return (exponent_negative ?\n"
            "                        negate(value, complement_negative,\n"
            "                               base, digit_count) :\n"
            "                        value);\n"
            "              });\n"
            "            if (has_point_or_exponent != null)\n"
            "                *has_point_or_exponent := true;;\n"
            "            return result * power(base, exponent);\n"
            "          }\n"
            "        else if (!optional)\n"
            "          {\n"
            "            throw(exception_tag_scanf_missing_exponent,\n"
            "                  \"Expected exponent not found.\");\n"
            "          };\n"
            "\n"
            "        return result;\n"
            "      };\n"
            "\n"
            "    function do_digits_and_mandatory_exponent(base : [1...+oo),\n"
            "            complement_negative : boolean,\n"
            "            add_sign : rational <-- (value : rational,\n"
            "                                     digit_count : integer))\n"
            "                    returns rational\n"
            "      {\n"
            "        return do_digits_and_exponent(base := base,\n"
            "                complement_negative := complement_negative,\n"
            "                add_sign := add_sign, optional := false);\n"
            "      };\n"
            "\n"
            "    procedure check_for_non_finite(\n"
            "            handler : {} <-- ({+oo, -oo, 1/0}))\n"
            "      {\n"
            "        procedure test(text : string, result : {+oo, -oo, 1/0})\n"
            "          {\n"
            "            if (forward_is(text))\n"
            "              {\n"
            "                [] := [read_string(length(text))];\n"
            "                handler(result);\n"
            "              };\n"
            "          };\n"
            "\n"
            "        test(\"+oo\", +oo);\n"
            "        test(\"-oo\", -oo);\n"
            "        test(\"oo\", 1/0);\n"
            "        test(\"+infinity\", +oo);\n"
            "        test(\"-infinity\", -oo);\n"
            "        test(\"infinity\", 1/0);\n"
            "      };\n"
            "\n"
            "    function do_prefix(base : [1...+oo),\n"
            "            finite_unsigned : boolean,\n"
            "            complement_negative : boolean,\n"
            "            look_for_0x : boolean, result_type : type !{},\n"
            "            base_parser :\n"
            "                    !{} <--\n"
            "                    (base : [1...+oo),\n"
            "                     complement_negative : boolean,\n"
            "                     add_sign : rational <-- (value : rational,\n"
            "                             digit_count : integer)))\n"
            "            returns rational / result_type\n"
            "      {\n"
            "        variable new_base : [1...+oo) := base;\n"
            "        variable is_negative : boolean := false;\n"
            "        variable looking_for_0x : boolean := look_for_0x;\n"
            "\n"
            "        procedure check_0x()\n"
            "          {\n"
            "            if (looking_for_0x &&\n"
            "                (forward_is(\"0x\") || forward_is(\"0X\")))\n"
            "              {\n"
            "                [] := [read_string(2)];\n"
            "                new_base := 16;\n"
            "              };\n"
            "          };\n"
            "\n"
            "        check_0x();\n"
            "\n"
            "        if (!finite_unsigned)\n"
            "          {\n"
            "            if (base == new_base)\n"
            "              {\n"
            "                check_for_non_finite(\n"
            "                        procedure(x : {+oo, -oo, 1/0})\n"
            "                          { return x from do_prefix; });\n"
            "              };\n"
            "\n"
            "            if ((look_ahead(0) == '-') ||\n"
            "                (look_ahead(0) == '+'))\n"
            "              {\n"
            "                immutable sign := read_character();\n"
            "                is_negative := (sign == '-');\n"
            "                check_0x();\n"
            "              };\n"
            "          };\n"
            "\n"
            "        variable result : result_type;\n"
            "        variable digit_count : [0...+oo);\n"
            "        return base_parser(new_base,\n"
            "                complement_negative := complement_negative,\n"
            "                add_sign := function(value : rational,\n"
            "                        digit_count : integer) returns rational\n"
            "          {\n"
            "\n"
            "            if (digit_count == 0)\n"
            "              {\n"
            "                throw(exception_tag_scanf_bad_integer,\n"
            "                      \"Expected digits not found.\");\n"
            "              };\n"
            "            return (is_negative ?\n"
            "                    negate(value, complement_negative,\n"
            "                           new_base, digit_count) :\n"
            "                    value);\n"
            "          });\n"
            "      };\n"
            "\n"
            "    function do_integer(base : [1...+oo),\n"
            "            finite_unsigned : boolean := false,\n"
            "            complement_negative : boolean := false,\n"
            "            look_for_0x : boolean := false) returns integer\n"
            "      {\n"
            "        return do_prefix(base := base,\n"
            "                finite_unsigned := finite_unsigned,\n"
            "                complement_negative := complement_negative,\n"
            "                look_for_0x := look_for_0x,\n"
            "                result_type := integer,\n"
            "                base_parser := function(base : [1...+oo),\n"
            "                        complement_negative : boolean,\n"
            "                        add_sign : rational <--\n"
            "                                (value : rational,\n"
            "                                 digit_count : integer))\n"
            "                        returns rational\n"
            "          {\n"
            "            return do_digits(base := base,\n"
            "                             finalize := add_sign);\n"
            "          });\n"
            "      };\n"
            "\n"
            "    function do_point(base : [1...+oo),\n"
            "            complement_negative : boolean := false,\n"
            "            look_for_0x : boolean := false) returns rational\n"
            "      {\n"
            "        return do_prefix(base := base,\n"
            "                finite_unsigned := false,\n"
            "                complement_negative := complement_negative,\n"
            "                look_for_0x := look_for_0x,\n"
            "                result_type := rational,\n"
            "                base_parser := do_digits_and_point);\n"
            "      };\n"
            "\n"
            "    function do_exponent(base : [1...+oo),\n"
            "            complement_negative : boolean := false,\n"
            "            look_for_0x : boolean := false) returns rational\n"
            "      {\n"
            "        return do_prefix(base := base,\n"
            "                finite_unsigned := false,\n"
            "                complement_negative := complement_negative,\n"
            "                look_for_0x := look_for_0x,\n"
            "                result_type := rational,\n"
            "                base_parser :=\n"
            "                        do_digits_and_mandatory_exponent);\n"
            "      };\n"
            "\n"
            "    function do_optional_exponent(base : [1...+oo),\n"
            "            complement_negative : boolean := false,\n"
            "            look_for_0x : boolean := false,\n"
            "            has_point_or_exponent : +.boolean := null) returns\n"
            "                    rational\n"
            "      {\n"
            "        function base_parser(base : [1...+oo),\n"
            "                complement_negative : boolean,\n"
            "                add_sign : rational <-- (value : rational,\n"
            "                        digit_count : integer)) returns\n"
            "                                rational\n"
            "          {\n"
            "            return do_digits_and_exponent(base := base,\n"
            "                    complement_negative := complement_negative,\n"
            "                    add_sign := add_sign, optional := true,\n"
            "                    has_point_or_exponent :=\n"
            "                            has_point_or_exponent);\n"
            "          };\n"
            "\n"
            "        return do_prefix(base := base,\n"
            "                finite_unsigned := false,\n"
            "                complement_negative := complement_negative,\n"
            "                look_for_0x := look_for_0x,\n"
            "                result_type := rational,\n"
            "                base_parser := base_parser);\n"
            "      };\n"
            "\n"
            "    function do_fraction(base : [1...+oo),\n"
            "            complement_negative : boolean := false,\n"
            "            look_for_0x : boolean := false,\n"
            "            or_point : boolean := false) returns rational\n"
            "      {\n"
            "        function do_one(or_point : boolean) returns rational\n"
            "          {\n"
            "            if (or_point)\n"
            "              {\n"
            "                return do_optional_exponent(base := base,\n"
            "                        complement_negative :=\n"
            "                                complement_negative,\n"
            "                        look_for_0x := look_for_0x,\n"
            "                        has_point_or_exponent :=\n"
            "                                &has_point_or_exponent);\n"
            "              }\n"
            "            else\n"
            "              {\n"
            "                return do_integer(base := base,\n"
            "                        complement_negative :=\n"
            "                                complement_negative,\n"
            "                        look_for_0x := look_for_0x);\n"
            "              };\n"
            "          };\n"
            "\n"
            "        variable has_point_or_exponent : boolean := false;\n"
            "        immutable numerator := do_one(or_point);\n"
            "        if (has_point_or_exponent)\n"
            "            return numerator;;\n"
            "        if (look_ahead(0) != '/')\n"
            "            return numerator;;\n"
            "\n"
            "        [] := [read_character()];\n"
            "        immutable denominator := do_one(false);\n"
            "        return numerator / denominator;\n"
            "      };\n"
            "\n"
            "    function read_character_with_escapes(\n"
            "            terminator : character, bad : {} <-- (string))\n"
            "                    returns character | {end_of_input}\n"
            "      {\n"
            "        immutable next := read_character();\n"
            "\n"
            "        switch (next)\n"
            "        case ({end_of_input})\n"
            "          {\n"
            "            bad(\"unexpected end-of-input\");\n"
            "          }\n"
            "        case ({terminator})\n"
            "          {\n"
            "            return end_of_input;\n"
            "          }\n"
            "        case ({'\\n'})\n"
            "          {\n"
            "            bad(\"newline encountered in string\");\n"
            "          }\n"
            "        case ({'\\\\'})\n"
            "          {\n"
            "            immutable octal_digits := type\n"
            "                    {'0', '1', '2', '3', '4', '5', '6', '7'};\n"
            "            immutable hex_digits := type\n"
            "                    {'0', '1', '2', '3', '4', '5', '6', '7',\n"
            "                     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',\n"
            "                     'A', 'B', 'C', 'D', 'E', 'F'};\n"
            "            immutable escaped := read_character();\n"
            "            switch (escaped)\n"
            "            case ({end_of_input})\n"
            "              {\n"
            "                bad(\"unexpected end-of-input\");\n"
            "              }\n"
            "            case ({'\\'', '\\\"', '?', '\\\\'})\n"
            "              {\n"
            "                return escaped;\n"
            "              }\n"
            "            case ({'a'})\n"
            "              {\n"
            "                return '\\a';\n"
            "              }\n"
            "            case ({'b'})\n"
            "              {\n"
            "                return '\\b';\n"
            "              }\n"
            "            case ({'f'})\n"
            "              {\n"
            "                return '\\f';\n"
            "              }\n"
            "            case ({'n'})\n"
            "              {\n"
            "                return '\\n';\n"
            "              }\n"
            "            case ({'r'})\n"
            "              {\n"
            "                return '\\r';\n"
            "              }\n"
            "            case ({'t'})\n"
            "              {\n"
            "                return '\\t';\n"
            "              }\n"
            "            case ({'v'})\n"
            "              {\n"
            "                return '\\v';\n"
            "              }\n"
            "            case (octal_digits)\n"
            "              {\n"
            "                variable value : [0...0x1ff] :=\n"
            "                        to_utf32(escaped) - to_utf32('0');\n"
            "\n"
            "                immutable second := look_ahead(0);\n"
            "                if (second in octal_digits)\n"
            "                  {\n"
            "                    [] := [read_character()];\n"
            "                    value *= 8;\n"
            "                    value +=\n"
            "                            (to_utf32(second) - to_utf32('0'));\n"
            "\n"
            "                    immutable third := look_ahead(0);\n"
            "                    if (third in octal_digits)\n"
            "                      {\n"
            "                        [] := [read_character()];\n"
            "                        value *= 8;\n"
            "                        value += (to_utf32(third) -\n"
            "                                  to_utf32('0'));\n"
            "                      };\n"
            "                  };\n"
            "\n"
            "                return from_utf32(value);\n"
            "              }\n"
            "            case ({'x'})\n"
            "              {\n"
            "                variable next_character :\n"
            "                        character | {end_of_input} :=\n"
            "                                look_ahead(0);\n"
            "\n"
            "                if (!(next_character in hex_digits))\n"
            "                  {\n"
            "                    bad(\"a hex escape sequence had zero \" ~\n"
            "                        \"hex digits\");\n"
            "                  };\n"
            "\n"
            "                variable value : [0...+oo) := 0;\n"
            "                do\n"
            "                  {\n"
            "                    value *= 16;\n"
            "                    value += to_utf32(next_character);\n"
            "                    if (next_character in\n"
            "                        {'0', '1', '2', '3', '4', '5', '6',\n"
            "                         '7', '8', '9'})\n"
            "                      {\n"
            "                        value -= to_utf32('0');\n"
            "                      }\n"
            "                    else if (next_character in\n"
            "                             {'a', 'b', 'c', 'd', 'e', 'f'})\n"
            "                      {\n"
            "                        value += 10;\n"
            "                        value -= to_utf32('a');\n"
            "                      }\n"
            "                    else\n"
            "                      {\n"
            "                        assert(next_character in\n"
            "                               {'A', 'B', 'C', 'D', 'E', 'F'});\n"
            "                        value += 10;\n"
            "                        value -= to_utf32('A');\n"
            "                      };\n"
            "\n"
            "                    if (value > 0x1fffff)\n"
            "                      {\n"
            "                        bad(\"hexadecimal escape sequence \" ~\n"
            "                            \"has more than 21 bits\");\n"
            "                      };\n"
            "\n"
            "                    next_character := look_ahead(0);\n"
            "                  } while (next_character in hex_digits);\n"
            "\n"
            "                assert(value <= 0x1fffff);\n"
            "                return from_utf32(value);\n"
            "              }\n"
            "            case (character)\n"
            "              {\n"
            "                bad(\"illegal escape sequence\");\n"
            "              };\n"
            "          }\n"
            "        case (character)\n"
            "          {\n"
            "            return next;\n"
            "          };\n"
            "      };\n"
            "\n"
            "    function do_string() returns string\n"
            "      {\n"
            "        [] := [read_character()];\n"
            "        variable result : string := \"\";\n"
            "        while (true)\n"
            "          {\n"
            "            immutable next := read_character_with_escapes(\n"
            "                    '\\\"',\n"
            "                procedure(message)\n"
            "                  {\n"
            "                    throw(exception_tag_scanf_bad_string_value,\n"
            "                          sprint(\"Bad string value in \" ~\n"
            "                                 \"scanf(): \", message,\n"
            "                                 \".\"));\n"
            "                  });\n"
            "            if (next == end_of_input)\n"
            "                return result;;\n"
            "            result ~= next;\n"
            "          };\n"
            "      };\n"
            "\n"
            "    function do_character() returns character\n"
            "      {\n"
            "        [] := [read_character()];\n"
            "        immutable result := read_character_with_escapes('\\\'',\n"
            "            procedure(message)\n"
            "              {\n"
            "                throw(exception_tag_scanf_bad_character_value,\n"
            "                      sprint(\"Bad character value in \" ~\n"
            "                             \"scanf(): \", message, \".\"));\n"
            "                  });\n"
            "        if (result == end_of_input)\n"
            "          {\n"
            "            throw(exception_tag_scanf_bad_character_value,\n"
            "                  \"Bad character value in scanf(): \" ~\n"
            "                  \"unexpected end-of-input.\");\n"
            "          };\n"
            "        immutable close := read_character();\n"
            "        if (close != '\\\'')\n"
            "          {\n"
            "            throw(exception_tag_scanf_bad_character_value,\n"
            "                  \"Bad character value in scanf(): missing\" ~\n"
            "                  \" close quote.\");\n"
            "          };\n"
            "        return result;\n"
            "      };\n"
            "\n"
            "    function do_regular_expression() returns regular_expression\n"
            "      {\n"
            "        [] := [read_character()];\n"
            "        variable result : string := \"\";\n"
            "        variable escaped : boolean := false;\n"
            "        while (true)\n"
            "          {\n"
            "            immutable next := read_character();\n"
            "            if (next == end_of_input)\n"
            "              {\n"
            "                throw(\n"
            "                    exception_tag_scanf_bad_regular_expression,\n"
            "                      \"Bad regular expression in scanf(): \" ~\n"
            "                      \"end-of-input encountered while \" ~\n"
            "                      \"parsing regular expression.\");\n"
            "              };\n"
            "            if (escaped)\n"
            "              {\n"
            "                escaped := false;\n"
            "              }\n"
            "            else\n"
            "              {\n"
            "                if (next == '\\\\')\n"
            "                    escaped := true;;\n"
            "                if (next == '@')\n"
            "                    return parse_regular_expression(result);;\n"
            "              };\n"
            "            result ~= next;\n"
            "          };\n"
            "      };\n"
            "\n"
            "    procedure slurp_whitespace()\n"
            "      {\n"
            "        while (look_ahead(0) in whitespace)\n"
            "            [] := [read_character()];;\n"
            "      };\n"
            "\n"
            "    function is_english_letter(to_test : character) returns\n"
            "            boolean\n"
            "      {\n"
            "        return to_test in english_letters;\n"
            "      };\n"
            "\n"
            "    function is_digit(to_test : character) returns boolean\n"
            "      {\n"
            "        return to_test in decimal_digits;\n"
            "      };\n"
            "\n"
            "    function is_whitespace(to_test : character) returns boolean\n"
            "      {\n"
            "        return to_test in whitespace;\n"
            "      };\n"
            "\n"
            "    function find_identifier(\n"
            "            get_character : (character | {end_of_input}) <--\n"
            "                             ([0...+oo))) returns\n"
            "                    string | {null}\n"
            "      {\n"
            "        immutable first := get_character(0);\n"
            "        if ((first == end_of_input) ||\n"
            "            !(is_english_letter(first) || (first == '_')))\n"
            "          {\n"
            "            return null;\n"
            "          };\n"
            "\n"
            "        variable result : string := \"\" ~ first;\n"
            "        variable position : [1...+oo) := 1;\n"
            "        while (true)\n"
            "          {\n"
            "            immutable next := get_character(position);\n"
            "            if ((next == end_of_input) ||\n"
            "                !(is_english_letter(next) || (next == '_') ||\n"
            "                  is_digit(next)))\n"
            "              {\n"
            "                break;\n"
            "              };\n"
            "            result ~= next;\n"
            "            ++position;\n"
            "          };\n"
            "\n"
            "        if (result == \"operator\")\n"
            "          {\n"
            "            switch (get_character(position))\n"
            "            case ({'('})\n"
            "              {\n"
            "                if (get_character(position + 1) == ')')\n"
            "                    return \"operator()\";;\n"
            "              }\n"
            "            case ({'['})\n"
            "              {\n"
            "                if (get_character(position + 1) == ']')\n"
            "                    return \"operator[]\";;\n"
            "              }\n"
            "            case ({':'})\n"
            "              {\n"
            "                if (get_character(position + 1) == ':')\n"
            "                    return \"operator::\";;\n"
            "              }\n"
            "            case ({'*'})\n"
            "              { return \"operator*\"; }\n"
            "            case ({'/'})\n"
            "              {\n"
            "                if ((get_character(position + 1) == ':') &&\n"
            "                    (get_character(position + 2) == ':'))\n"
            "                  {\n"
            "                    return \"operator/::\";\n"
            "                  }\n"
            "                else\n"
            "                  {\n"
            "                    return \"operator/\";\n"
            "                  };\n"
            "              }\n"
            "            case ({'%'})\n"
            "              { return \"operator%\"; }\n"
            "            case ({'+'})\n"
            "              { return \"operator+\"; }\n"
            "            case ({'-'})\n"
            "              {\n"
            "                if (get_character(position + 1) == '>')\n"
            "                    return \"operator->\";\n"
            "                else\n"
            "                    return \"operator-\";;\n"
            "              }\n"
            "            case ({'<'})\n"
            "              {\n"
            "                if (get_character(position + 1) == '<')\n"
            "                    return \"operator<<\";\n"
            "                else if (get_character(position + 1) == '=')\n"
            "                    return \"operator<=\";\n"
            "                else\n"
            "                    return \"operator<\";;\n"
            "              }\n"
            "            case ({'>'})\n"
            "              {\n"
            "                if (get_character(position + 1) == '>')\n"
            "                    return \"operator>>\";\n"
            "                else if (get_character(position + 1) == '=')\n"
            "                    return \"operator>=\";\n"
            "                else\n"
            "                    return \"operator>\";;\n"
            "              }\n"
            "            case ({'&'})\n"
            "              { return \"operator&\"; }\n"
            "            case ({'^'})\n"
            "              { return \"operator^\"; }\n"
            "            case ({'|'})\n"
            "              { return \"operator|\"; }\n"
            "            case ({'='})\n"
            "              {\n"
            "                if (get_character(position + 1) == '=')\n"
            "                    return \"operator==\";;\n"
            "              }\n"
            "            case ({'!'})\n"
            "              {\n"
            "                if (get_character(position + 1) == '=')\n"
            "                    return \"operator!=\";\n"
            "                else\n"
            "                    return \"operator!\";;\n"
            "              }\n"
            "            case ({'~'})\n"
            "              { return \"operator~\"; };\n"
            "          };\n"
            "\n"
            "        return result;\n"
            "      };\n"
            "\n"
            "    function do_tag() returns string | {null}\n"
            "      {\n"
            "        immutable result : string | {null} :=\n"
            "                find_identifier(look_ahead);\n"
            "        if (result == null)\n"
            "            return null;;\n"
            "        variable position : [1...+oo) := length(result);\n"
            "\n"
            "        while (is_whitespace(look_ahead(position)))\n"
            "            ++position;;\n"
            "\n"
            "        if ((look_ahead(position) == ':') &&\n"
            "            (look_ahead(position + 1) == '='))\n"
            "          {\n"
            "            [] := [read_string(position + 2)];\n"
            "            slurp_whitespace();\n"
            "            return result;\n"
            "          };\n"
            "\n"
            "        if (result == \"operator:=\")\n"
            "          {\n"
            "            [] := [read_string(length(result))];\n"
            "            slurp_whitespace();\n"
            "            return \"operator\";\n"
            "          };\n"
            "\n"
            "        return null;\n"
            "      };\n"
            "\n"
            "    function do_semi_labeled(complement_negative : boolean)\n"
            "            returns [...]\n"
            "      {\n"
            "        [] := [read_character()];\n"
            "        slurp_whitespace();\n"
            "\n"
            "        variable result : [...] := [];\n"
            "\n"
            "        if (look_ahead(0) == ']')\n"
            "          {\n"
            "            [] := [read_string(1)];\n"
            "            return result;\n"
            "          };\n"
            "\n"
            "        while (true)\n"
            "          {\n"
            "            immutable the_tag : string | {null} := do_tag();\n"
            "            immutable element :=\n"
            "                    do_any_value(complement_negative);\n"
            "            if (the_tag == null)\n"
            "              {\n"
            "                result ~= [element];\n"
            "              }\n"
            "            else\n"
            "              {\n"
            "                result ~= tag(value := element,\n"
            "                              label := the_tag);\n"
            "              };\n"
            "            slurp_whitespace();\n"
            "\n"
            "            if (look_ahead(0) == ']')\n"
            "              {\n"
            "                [] := [read_string(1)];\n"
            "                return result;\n"
            "              };\n"
            "\n"
            "            immutable comma := read_character();\n"
            "            if (comma != ',')\n"
            "              {\n"
            "                throw(exception_tag_scanf_bad_list_value,\n"
            "                      \"Bad list value in scanf(): missing \" ~\n"
            "                      \"`,' or `]' after list item.\");\n"
            "              };\n"
            "            slurp_whitespace();\n"
            "          };\n"
            "      };\n"
            "\n"
            "    function do_map(complement_negative : boolean) returns\n"
            "            !{} --> !{}\n"
            "      {\n"
            "        [] := [read_character()];\n"
            "        immutable next := read_character();\n"
            "        if (next != '<')\n"
            "          {\n"
            "            throw(exception_tag_scanf_bad_map_value,\n"
            "                  \"Bad map value in scanf(): missing \" ~\n"
            "                  \"second `<' at start of map value.\");\n"
            "          };\n"
            "        slurp_whitespace();\n"
            "\n"
            "        if (forward_is(\">>\"))\n"
            "          {\n"
            "            [] := [read_string(2)];\n"
            "            return <<>>;\n"
            "          };\n"
            "\n"
            "        variable have_key : !{} --> boolean := <<(*-->false)>>;\n"
            "        variable pairs : array[[key : !{}, target : !{}]] :=\n"
            "                [];\n"
            "        variable have_default : boolean := false;\n"
            "        variable default_value;\n"
            "\n"
            "        while (true)\n"
            "          {\n"
            "            immutable left := read_character();\n"
            "            if (left != '(')\n"
            "              {\n"
            "                throw(exception_tag_scanf_bad_map_value,\n"
            "                      \"Bad map value in scanf(): missing \" ~\n"
            "                      \"`(' at start of map item.\");\n"
            "              };\n"
            "            slurp_whitespace();\n"
            "\n"
            "            variable is_star : boolean;\n"
            "            variable key;\n"
            "            if (look_ahead(0) == '*')\n"
            "              {\n"
            "                [] := [read_character()];\n"
            "                is_star := true;\n"
            "                if (have_default)\n"
            "                  {\n"
            "                    throw(exception_tag_scanf_bad_map_value,\n"
            "                          \"Bad map value in scanf(): `*' \" ~\n"
            "                          \"appears more than once.\");\n"
            "                  };\n"
            "                have_default := true;\n"
            "              }\n"
            "            else\n"
            "              {\n"
            "                is_star := false;\n"
            "                key := do_any_value(complement_negative);\n"
            "                if (have_key[key])\n"
            "                  {\n"
            "                    throw(exception_tag_scanf_bad_map_value,\n"
            "                          sprint(\"Bad map value in \" ~\n"
            "                                 \"scanf(): key value `\", key,\n"
            "                                 \"' appears more than \" ~\n"
            "                                 \"once.\"));\n"
            "                  };\n"
            "                have_key[key] := true;\n"
            "              };\n"
            "            slurp_whitespace();\n"
            "\n"
            "            if (!forward_is(\"-->\"))\n"
            "              {\n"
            "                throw(exception_tag_scanf_bad_map_value,\n"
            "                      \"Bad map value in scanf(): missing \" ~\n"
            "                      \"`-->' after map item key.\");\n"
            "              };\n"
            "            [] := [read_string(3)];\n"
            "            slurp_whitespace();\n"
            "\n"
            "            immutable target :=\n"
            "                    do_any_value(complement_negative);\n"
            "            slurp_whitespace();\n"
            "\n"
            "            immutable right := read_character();\n"
            "            if (right != ')')\n"
            "              {\n"
            "                throw(exception_tag_scanf_bad_map_value,\n"
            "                      \"Bad map value in scanf(): missing \" ~\n"
            "                      \"`)' at end of map item.\");\n"
            "              };\n"
            "            slurp_whitespace();\n"
            "\n"
            "            if (is_star)\n"
            "              {\n"
            "                default_value := target;\n"
            "              }\n"
            "            else\n"
            "              {\n"
            "                pairs ~= [[key := key, target := target]];\n"
            "              };\n"
            "\n"
            "            if (forward_is(\">>\"))\n"
            "              {\n"
            "                [] := [read_string(2)];\n"
            "                break;\n"
            "              };\n"
            "\n"
            "            immutable comma := read_character();\n"
            "            if (comma != ',')\n"
            "              {\n"
            "                throw(exception_tag_scanf_bad_map_value,\n"
            "                      \"Bad map value in scanf(): missing \" ~\n"
            "                      \"`,' or `>>' after map item.\");\n"
            "              };\n"
            "            slurp_whitespace();\n"
            "          };\n"
            "\n"
            "        variable result : !{} --> !{} := <<>>;\n"
            "        if (have_default)\n"
            "            result := <<(* --> default_value)>>;;\n"
            "        iterate (pair; pairs)\n"
            "            result[pair.key] := pair.target;;\n"
            "        return result;\n"
            "      };\n"
            "\n"
            "    function do_type(complement_negative : boolean) returns\n"
            "            type !{}\n"
            "      {\n"
            "        [] := [read_character()];\n"
            "        slurp_whitespace();\n"
            "\n"
            "        variable result : type !{} := type {};\n"
            "\n"
            "        if (look_ahead(0) == '}')\n"
            "          {\n"
            "            [] := [read_string(1)];\n"
            "            return result;\n"
            "          };\n"
            "\n"
            "        while (true)\n"
            "          {\n"
            "            immutable element :=\n"
            "                    do_any_value(complement_negative);\n"
            "            result := type result | {element};\n"
            "            slurp_whitespace();\n"
            "\n"
            "            if (look_ahead(0) == '}')\n"
            "              {\n"
            "                [] := [read_string(1)];\n"
            "                return result;\n"
            "              };\n"
            "\n"
            "            immutable comma := read_character();\n"
            "            if (comma != ',')\n"
            "              {\n"
            "                throw(exception_tag_scanf_bad_type_value,\n"
            "                      \"Bad type value in scanf(): missing \" ~\n"
            "                      \"`,' or `}' after list item.\");\n"
            "              };\n"
            "            slurp_whitespace();\n"
            "          };\n"
            "      };\n"
            "\n"
            "    function do_any_value(\n"
            "            complement_negative : boolean := false)\n"
            "      {\n"
            "        procedure test(text : string, result)\n"
            "          {\n"
            "            if (forward_is(text))\n"
            "              {\n"
            "                [] := [read_string(length(text))];\n"
            "                return result from do_any_value;\n"
            "              };\n"
            "          };\n"
            "\n"
            "        test(\"true\", true);\n"
            "        test(\"false\", false);\n"
            "        test(\"null\", null);\n"
            "\n"
            "        switch (look_ahead(0))\n"
            "        case ({'\\\"'})\n"
            "          {\n"
            "            return do_string();\n"
            "          }\n"
            "        case ({'\\\''})\n"
            "          {\n"
            "            return do_character();\n"
            "          }\n"
            "        case ({'@'})\n"
            "          {\n"
            "            return do_regular_expression();\n"
            "          }\n"
            "        case ({'['})\n"
            "          {\n"
            "            return do_semi_labeled(complement_negative);\n"
            "          }\n"
            "        case ({'<'})\n"
            "          {\n"
            "            return do_map(complement_negative);\n"
            "          }\n"
            "        case ({'{'})\n"
            "          {\n"
            "            return do_type(complement_negative);\n"
            "          }\n"
            "        case (!{})\n"
            "          {\n"
            "            return do_fraction(base := 10,\n"
            "                    complement_negative := complement_negative,\n"
            "                    look_for_0x := true, or_point := true);\n"
            "          };\n"
            "      };\n"
            "\n"
            "    function do_percent_base(text : string)\n"
            "      {\n"
            "        switch (text)\n"
            "        case ({\"d\"})\n"
            "          {\n"
            "            return do_integer(base := 10);\n"
            "          }\n"
            "        case ({\"ud\"})\n"
            "          {\n"
            "            return do_integer(base := 10,\n"
            "                              finite_unsigned := true);\n"
            "          }\n"
            "        case ({\"cd\"})\n"
            "          {\n"
            "            return do_integer(base := 10,\n"
            "                              complement_negative := true);\n"
            "          }\n"
            "        case ({\"o\"})\n"
            "          {\n"
            "            return do_integer(base := 8);\n"
            "          }\n"
            "        case ({\"uo\"})\n"
            "          {\n"
            "            return do_integer(base := 8,\n"
            "                              finite_unsigned := true);\n"
            "          }\n"
            "        case ({\"co\"})\n"
            "          {\n"
            "            return do_integer(base := 8,\n"
            "                              complement_negative := true);\n"
            "          }\n"
            "        case ({\"x\"})\n"
            "          {\n"
            "            return do_integer(base := 16);\n"
            "          }\n"
            "        case ({\"ux\"})\n"
            "          {\n"
            "            return do_integer(base := 16,\n"
            "                              finite_unsigned := true);\n"
            "          }\n"
            "        case ({\"cx\"})\n"
            "          {\n"
            "            return do_integer(base := 16,\n"
            "                              complement_negative := true);\n"
            "          }\n"
            "        case ({\"i\"})\n"
            "          {\n"
            "            return do_integer(base := 10, look_for_0x := true);\n"
            "          }\n"
            "        case ({\"ui\"})\n"
            "          {\n"
            "            return do_integer(base := 10,\n"
            "                    finite_unsigned := true,\n"
            "                    look_for_0x := true);\n"
            "          }\n"
            "        case ({\"ci\"})\n"
            "          {\n"
            "            return do_integer(base := 10,\n"
            "                    complement_negative := true,\n"
            "                    look_for_0x := true);\n"
            "          }\n"
            "        case ({\"df\"})\n"
            "          {\n"
            "            return do_point(base := 10);\n"
            "          }\n"
            "        case ({\"cdf\"})\n"
            "          {\n"
            "            return do_point(base := 10,\n"
            "                            complement_negative := true);\n"
            "          }\n"
            "        case ({\"of\"})\n"
            "          {\n"
            "            return do_point(base := 8);\n"
            "          }\n"
            "        case ({\"cof\"})\n"
            "          {\n"
            "            return do_point(base := 8,\n"
            "                            complement_negative := true);\n"
            "          }\n"
            "        case ({\"xf\"})\n"
            "          {\n"
            "            return do_point(base := 16);\n"
            "          }\n"
            "        case ({\"cxf\"})\n"
            "          {\n"
            "            return do_point(base := 16,\n"
            "                            complement_negative := true);\n"
            "          }\n"
            "        case ({\"f\"})\n"
            "          {\n"
            "            return do_point(base := 10, look_for_0x := true);\n"
            "          }\n"
            "        case ({\"cf\"})\n"
            "          {\n"
            "            return do_point(base := 10,\n"
            "                    complement_negative := true,\n"
            "                    look_for_0x := true);\n"
            "          }\n"
            "        case ({\"de\"})\n"
            "          {\n"
            "            return do_exponent(base := 10);\n"
            "          }\n"
            "        case ({\"cde\"})\n"
            "          {\n"
            "            return do_exponent(base := 10,\n"
            "                               complement_negative := true);\n"
            "          }\n"
            "        case ({\"oe\"})\n"
            "          {\n"
            "            return do_exponent(base := 8);\n"
            "          }\n"
            "        case ({\"coe\"})\n"
            "          {\n"
            "            return do_exponent(base := 8,\n"
            "                               complement_negative := true);\n"
            "          }\n"
            "        case ({\"xe\"})\n"
            "          {\n"
            "            return do_exponent(base := 16);\n"
            "          }\n"
            "        case ({\"cxe\"})\n"
            "          {\n"
            "            return do_exponent(base := 16,\n"
            "                               complement_negative := true);\n"
            "          }\n"
            "        case ({\"e\"})\n"
            "          {\n"
            "            return do_exponent(base := 10,\n"
            "                               look_for_0x := true);\n"
            "          }\n"
            "        case ({\"ce\"})\n"
            "          {\n"
            "            return do_exponent(base := 10,\n"
            "                    complement_negative := true,\n"
            "                    look_for_0x := true);\n"
            "          }\n"
            "        case ({\"dg\"})\n"
            "          {\n"
            "            return do_optional_exponent(base := 10);\n"
            "          }\n"
            "        case ({\"cdg\"})\n"
            "          {\n"
            "            return do_optional_exponent(base := 10,\n"
            "                    complement_negative := true);\n"
            "          }\n"
            "        case ({\"og\"})\n"
            "          {\n"
            "            return do_optional_exponent(base := 8);\n"
            "          }\n"
            "        case ({\"cog\"})\n"
            "          {\n"
            "            return do_optional_exponent(base := 8,\n"
            "                    complement_negative := true);\n"
            "          }\n"
            "        case ({\"xg\"})\n"
            "          {\n"
            "            return do_optional_exponent(base := 16);\n"
            "          }\n"
            "        case ({\"cxg\"})\n"
            "          {\n"
            "            return do_optional_exponent(base := 16,\n"
            "                    complement_negative := true);\n"
            "          }\n"
            "        case ({\"g\"})\n"
            "          {\n"
            "            return do_optional_exponent(base := 10,\n"
            "                                        look_for_0x := true);\n"
            "          }\n"
            "        case ({\"cg\"})\n"
            "          {\n"
            "            return do_optional_exponent(base := 10,\n"
            "                    complement_negative := true,\n"
            "                    look_for_0x := true);\n"
            "          }\n"
            "        case ({\"dr\"})\n"
            "          {\n"
            "            return do_fraction(base := 10);\n"
            "          }\n"
            "        case ({\"cdr\"})\n"
            "          {\n"
            "            return do_fraction(base := 10,\n"
            "                               complement_negative := true);\n"
            "          }\n"
            "        case ({\"or\"})\n"
            "          {\n"
            "            return do_fraction(base := 8);\n"
            "          }\n"
            "        case ({\"cor\"})\n"
            "          {\n"
            "            return do_fraction(base := 8,\n"
            "                               complement_negative := true);\n"
            "          }\n"
            "        case ({\"xr\"})\n"
            "          {\n"
            "            return do_fraction(base := 16);\n"
            "          }\n"
            "        case ({\"cxr\"})\n"
            "          {\n"
            "            return do_fraction(base := 16,\n"
            "                               complement_negative := true);\n"
            "          }\n"
            "        case ({\"r\"})\n"
            "          {\n"
            "            return do_fraction(base := 10,\n"
            "                               look_for_0x := true);\n"
            "          }\n"
            "        case ({\"cr\"})\n"
            "          {\n"
            "            return do_fraction(base := 10,\n"
            "                    complement_negative := true,\n"
            "                    look_for_0x := true);\n"
            "          }\n"
            "        case ({\"v\"})\n"
            "          {\n"
            "            return do_any_value();\n"
            "          }\n"
            "        case ({\"cv\"})\n"
            "          {\n"
            "            return do_any_value(complement_negative := true);\n"
            "          }\n"
            "        case (!{})\n"
            "          {\n"
            "            throw(exception_tag_scanf_bad_value_specifier,\n"
            "                  sprint(\"Bad value specifier %\", text,\n"
            "                         \"%.\"));\n"
            "          };\n"
            "      };\n"
            "\n"
            "    function do_percent(text : string) returns [!{}]\n"
            "      {\n"
            "        immutable text_characters : array[character] :=\n"
            "                characters(text);\n"
            "        variable label : string | {null} := find_identifier(\n"
            "                function (position : [0...+oo)) returns\n"
            "                        (character | {end_of_input})\n"
            "          {\n"
            "            return ((position < length(text_characters)) ?\n"
            "                    text_characters[position] : end_of_input);\n"
            "          });\n"
            "        variable base_text : string := text;\n"
            "        if (label != null)\n"
            "          {\n"
            "            immutable label_length := length(label);\n"
            "            if ((label_length < length(text_characters)) &&\n"
            "                (text_characters[label_length] == ':'))\n"
            "              {\n"
            "                base_text := make_string(\n"
            "                        text_characters[(label_length + 1) ...\n"
            "                                        +oo]);\n"
            "              }\n"
            "            else\n"
            "              {\n"
            "                label := null;\n"
            "              };\n"
            "          };\n"
            "        immutable element_value := do_percent_base(base_text);\n"
            "        return ((label == null) ? [element_value] :\n"
            "                tag(element_value, label := label));\n"
            "      };\n"
            "\n"
            "    function do_regular_expression(text : string,\n"
            "            shortest : boolean) returns string\n"
            "      {\n"
            "        procedure do_return()\n"
            "          {\n"
            "            return result from do_regular_expression;\n"
            "          };\n"
            "\n"
            "        procedure done()\n"
            "          {\n"
            "            if (!have_match)\n"
            "              {\n"
            "                throw(\n"
            "              exception_tag_scanf_no_regular_expression_match,\n"
            "                      sprint(\"No match for regular \" ~\n"
            "                             \"expression @\", text, \"@.\"));\n"
            "              };\n"
            "            do_return();\n"
            "          };\n"
            "\n"
            "        immutable matcher := parse_regular_expression(text);\n"
            "        immutable follower : regular_expression_follower :=\n"
            "                create_follower(matcher);\n"
            "        use follower;\n"
            "        variable have_match : boolean := false;\n"
            "        variable result : string := \"\";\n"
            "        variable forward : string := \"\";\n"
            "        while (true)\n"
            "          {\n"
            "            procedure test_for_acceptance()\n"
            "              {\n"
            "                if (is_in_accepting_state())\n"
            "                  {\n"
            "                    result ~= forward;\n"
            "                    [] := [read_string(length(forward))];\n"
            "                    if (shortest)\n"
            "                        do_return();;\n"
            "                    have_match := true;\n"
            "                    forward := \"\";\n"
            "                  };\n"
            "              };\n"
            "\n"
            "            test_for_acceptance();\n"
            "\n"
            "            if (!(more_possible()))\n"
            "                done();;\n"
            "\n"
            "            immutable next := look_ahead(length(forward));\n"
            "            transit(next);\n"
            "\n"
            "            if (next == end_of_input)\n"
            "              {\n"
            "                test_for_acceptance();\n"
            "                done();\n"
            "              };\n"
            "            forward ~= next;\n"
            "          };\n"
            "      };\n"
            "\n"
            "    function specifier_value(bound : character, text : string)\n"
            "            returns [!{}]\n"
            "      {\n"
            "        if (bound == '%')\n"
            "          {\n"
            "            return do_percent(text);\n"
            "          }\n"
            "        else\n"
            "          {\n"
            "            return [do_regular_expression(text := text,\n"
            "                            shortest := (bound == '#'))];\n"
            "          };\n"
            "      };\n"
            "\n"
            "    procedure do_specifier(bound : character, text : string)\n"
            "      {\n"
            "        result ~= specifier_value(bound, text);\n"
            "      };\n"
            "\n"
            "    immutable format_chars : array[character] :=\n"
            "            characters(format);\n"
            "    variable char_num : [0...+oo) := 0;\n"
            "    immutable char_count : [0...+oo) := length(format_chars);\n"
            "    variable result : [...] := [];\n"
            "    while (char_num < char_count)\n"
            "      {\n"
            "        immutable match_char : character :=\n"
            "                format_chars[char_num];\n"
            "        switch (match_char)\n"
            "        case ({'%', '@', '#'})\n"
            "          {\n"
            "            variable end := char_num + 1;\n"
            "            variable escaped : boolean := false;\n"
            "            while (true)\n"
            "              {\n"
            "                if (end == char_count)\n"
            "                  {\n"
            "                    throw(\n"
            "                     exception_tag_scanf_unterminate_specifier,\n"
            "                          sprint(\"Unterminated `\",\n"
            "                                 match_char,\n"
            "                                 \"' specifier found in the\" ~\n"
            "                                 \" format string of a \" ~\n"
            "                                 \"scanf() call.\"));\n"
            "                  };\n"
            "                if (escaped)\n"
            "                  {\n"
            "                    escaped := false;\n"
            "                  }\n"
            "                else\n"
            "                  {\n"
            "                    if (format_chars[end] == '\\\\')\n"
            "                        escaped := true;;\n"
            "                    if (format_chars[end] == match_char)\n"
            "                        break;;\n"
            "                  };\n"
            "                ++end;\n"
            "              };\n"
            "\n"
            "            if (end == char_num + 1)\n"
            "              {\n"
            "                match_one(match_char);\n"
            "              }\n"
            "            else\n"
            "              {\n"
            "                do_specifier(match_char,\n"
            "                        make_string(format_chars[\n"
            "                            (char_num + 1) ... (end - 1)]));\n"
            "              };\n"
            "\n"
            "            char_num := end + 1;\n"
            "          }\n"
            "        case (character)\n"
            "          {\n"
            "            match_one(match_char);\n"
            "            ++char_num;\n"
            "          };\n"
            "      };\n"
            "    return result;\n"
            "  };"),

    ONE_ROUTINE("function file_exists(file_name : string) returns boolean;",
                &file_exists_handler_function, PURE_UNSAFE),

    ONE_ROUTINE("function directory_exists(name : string) returns boolean;",
                &directory_exists_handler_function, PURE_UNSAFE),

    ONE_ROUTINE(
            "function directory_contents(name : string) returns "
            "array[string];", &directory_contents_handler_function,
            PURE_UNSAFE),

    ONE_ROUTINE("procedure remove(file_name : string);",
                &remove_handler_function, PURE_UNSAFE),

    ONE_ROUTINE("procedure rename(old_name : string, new_name : string);",
                &rename_handler_function, PURE_UNSAFE),

    ONE_STATEMENT(
            "procedure print(...)\n"
            "  { internal_call(standard_output.print, arguments); };"),

    ONE_ROUTINE("function sprint(...) returns string;",
                &sprint_handler_function, PURE_SAFE),

    ONE_STATEMENT(
            "procedure printf(format : string, ...)\n"
            "  { internal_call(standard_output.printf, arguments); };"),

    ONE_ROUTINE("function sprintf(format : string, ...) returns string;",
                &sprintf_handler_function, PURE_SAFE),

    ONE_STATEMENT(
            "function read_character() returns (character | {end_of_input})\n"
            "  (standard_input.read_character());"),

    ONE_STATEMENT(
            "function read_string(count : [0...+oo],\n"
            "        minimum : [0...+oo] / [0...count] := count) returns\n"
            "                (string | {end_of_input})\n"
            "  (standard_input.read_string(count, minimum));"),

    ONE_STATEMENT(
            "function read_line() returns (string | {end_of_input})\n"
            "  (standard_input.read_line());"),

    ONE_STATEMENT(
            "function scanf(format : string) returns [...]\n"
            "  (scanf(standard_input, format));"),

    ONE_ROUTINE(
            "function system(: string, capture_standard_out : boolean := "
            "false,\n"
            "                capture_standard_error : boolean := false) "
            "returns\n"
            "        [return_code : integer, standard_out : string,\n"
            "         standard_error : string];", &system_handler_function,
            PURE_UNSAFE),

    ONE_ROUTINE("procedure assert(: boolean);", &assert_handler_function,
                PURE_SAFE),

    ONE_ROUTINE(
            "function why_not_in(the_value : !{}, the_type : type !{})\n"
            "    returns string;", &why_not_in_handler_function, PURE_SAFE),

    ONE_STATEMENT("lock context_switching;"),

    ONE_STATEMENT("hide;"),

    ONE_ROUTINE(
            "procedure initialize_context_switching_lock(\n"
            "        the_lock : {context_switching});",
            &initialize_context_switching_lock_handler_function, PURE_UNSAFE),

    ONE_STATEMENT("initialize_context_switching_lock(context_switching);"),

    ONE_STATEMENT("export;"),

    ONE_STATEMENT(
            "quark enumeration months\n"
            "  {\n"
            "    January, February, March, April, May, June, July, August,\n"
            "    September, October, November, December\n"
            "  };"),

    ONE_STATEMENT(
            "quark enumeration days_of_week\n"
            "  {\n"
            "    Monday, Tuesday, Wednesday, Thursday, Friday, Saturday,\n"
            "    Sunday\n"
            "  };"),

    ONE_STATEMENT(
            "immutable time_type := type\n"
            "  [\n"
            "    year : integer,\n"
            "    month : months,\n"
            "    day_of_month : [1...31],\n"
            "    day_of_week : days_of_week,\n"
            "    hours : [0...24),\n"
            "    minutes : [0...60),\n"
            "    seconds : [0....60),\n"
            "    zone : string | {null},\n"
            "    is_daylight_savings : boolean | {null},\n"
            "    seconds_ahead_of_utc :\n"
            "            [-12 * 60 * 60... 12 * 60 * 60] | {null},\n"
            "    ...\n"
            "  ];"),

    ONE_STATEMENT("hide;"),

    ONE_STATEMENT(
            "immutable month_array :=\n"
            "  [\n"
            "    January, February, March, April, May, June, July, August,\n"
            "    September, October, November, December\n"
            "  ];"),

    ONE_STATEMENT(
            "immutable day_array :=\n"
            "  [\n"
            "    Monday, Tuesday, Wednesday, Thursday, Friday, Saturday,\n"
            "    Sunday\n"
            "  ];"),

    ONE_ROUTINE(
            "function get_time_and_date(: months[12], : days_of_week[7],\n"
            "        is_local : boolean) returns time_type;",
            &get_time_and_date_handler_function, PURE_UNSAFE),

    ONE_STATEMENT("export;"),

    ONE_STATEMENT(
            "function local_time_and_date() returns time_type\n"
            "  (get_time_and_date(month_array, day_array, true));"),

    ONE_STATEMENT(
            "function utc_time_and_date() returns time_type\n"
            "  (get_time_and_date(month_array, day_array, false));"),

    ONE_STATEMENT(
            "function translate(: string, ...) returns string\n"
            "  (arguments[0]);"),

    ONE_STATEMENT(
            "lepton source_region\n"
            "  [\n"
            "    file_name : string,\n"
            "    start_line : [1...+oo) | { 0/0 },\n"
            "    start_column : [1...+oo) | { 0/0 },\n"
            "    end_line : [1...+oo) | { 0/0 },\n"
            "    end_column : [1...+oo) | { 0/0 }\n"
            "  ];"),

    ONE_STATEMENT(
            "lepton exception\n"
            "  [\n"
            "    tag : any_quark,\n"
            "    message : string,\n"
            "    source : source_region,\n"
            "    ...\n"
            "  ];"),

    ONE_ROUTINE(
        "procedure throw(\n"
        "    tag : any_quark,\n"
        "    message : string,\n"
        "    source_file_name : string := \"\",\n"
        "    source_start_line_number : [1...+oo) | { 0/0 } := 0/0,\n"
        "    source_start_column_number : [1...+oo) | { 0/0 } := 0/0,\n"
        "    source_end_line_number : [1...+oo) | { 0/0 } := 0/0,\n"
        "    source_end_column_number : [1...+oo) | { 0/0 } := 0/0,\n"
        "    other : [...] := []\n"
        "  );", &throw_new_handler_function, PURE_SAFE),

    ONE_ROUTINE("procedure throw(to_throw : exception);",
                &throw_old_handler_function, PURE_SAFE),

    ONE_ROUTINE("function current_exceptions() returns array[exception];",
                &current_exceptions_handler_function, PURE_SAFE),

    ONE_ROUTINE("function numerator(base : rational) returns integer;",
                &numerator_handler_function, PURE_SAFE),

    ONE_ROUTINE("function denominator(base : rational) returns (-oo...+oo);",
                &denominator_handler_function, PURE_SAFE),

    ONE_ROUTINE(
            "function power(base : rational, exponent : integer)\n"
            "        returns rational;", &power_handler_function, PURE_SAFE),

    ONE_STATEMENT("hide;"),

    ONE_ROUTINE(
            "procedure set_source_region_key(\n"
            "        source_region_key : {source_region});",
            &set_source_region_key_handler_function, PURE_UNSAFE),

    ONE_STATEMENT("set_source_region_key(source_region);"),

    ONE_ROUTINE("procedure set_exception_key(exception_key : {exception});",
                &set_exception_key_handler_function, PURE_UNSAFE),

    ONE_STATEMENT("set_exception_key(exception);"),

    ONE_ROUTINE("procedure set_standard_library_object(the_object : {this});",
                &set_standard_library_object_handler_function, PURE_UNSAFE),

    ONE_STATEMENT("set_standard_library_object(this);"),

    ONE_ROUTINE(
            "function get_environment(key : string)\n"
            "        returns string | {null} | u8[0...+oo];",
            &get_environment_handler_function, PURE_SAFE),

    ONE_STATEMENT(
            "class get_environment()\n"
            "  {\n"
            "    function operator[](key : string)\n"
            "            returns string | {null} | u8[0...+oo]\n"
            "      (get_environment(key));\n"
            "  };"),

    ONE_STATEMENT("export;"),

    ONE_STATEMENT(
            "immutable environment : interface\n"
            "  [ operator[] :- (string | {null} | u8[0...+oo]) <-- (string)]\n"
            "        := get_environment();"),

#undef DEFINE_EXCEPTION_TAG

#define DEFINE_EXCEPTION_TAG(tag)  \
        { __LINE__, "quark exception_tag_" #tag ";", NULL },

#include "all_exceptions.h"

  };


extern declaration *create_standard_built_ins_class_declaration(void)
  {
    INITIALIZE_SYSTEM_LOCK(getenv_lock, return NULL);
    return create_native_bridge_class_declaration(&(function_list[0]),
            (sizeof(function_list) / sizeof(native_bridge_function_info)),
            __FILE__);
  }

extern void cleanup_standard_built_ins_module(void)
  {
    DESTROY_SYSTEM_LOCK(getenv_lock);
  }

extern void hook_file_pointer_to_object(object *the_object, FILE *fp,
        utf_choice utf_format, char *file_description, jumper *the_jumper)
  {
    fp_call_printer_data *fp_data;

    assert(the_object != NULL);
    assert(fp != NULL);
    assert(file_description != NULL);
    assert(the_jumper != NULL);

    assert(!(object_is_closed(the_object))); /* VERIFICATION NEEDED */
    assert(object_hook(the_object) == NULL);

    fp_data = MALLOC_ONE_OBJECT(fp_call_printer_data);
    if (fp_data == NULL)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    fp_data->fp = fp;
    fp_data->utf_format = utf_format;
    fp_data->file_description = file_description;

    assert(!(object_is_closed(the_object))); /* VERIFICATION NEEDED */
    object_set_hook(the_object, fp_data);
    object_set_hook_cleaner(the_object, &fp_cleaner);
  }

extern void hook_file_pointer_to_object_for_named_file(object *the_object,
        FILE *fp, utf_choice utf_format, const char *file_name,
        jumper *the_jumper)
  {
    string_buffer description_buffer;
    verdict the_verdict;
    int return_value;

    assert(the_object != NULL);
    assert(fp != NULL);
    assert(file_name != NULL);
    assert(the_jumper != NULL);

    the_verdict = string_buffer_init(&description_buffer, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    return_value =
            buffer_printf(&description_buffer, 0, "file \"%s\"", file_name);
    if (return_value < 0)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    hook_file_pointer_to_object(the_object, fp, utf_format,
                                description_buffer.array, the_jumper);
  }

extern value *fp_input_text_read_character_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *object_value;
    value *io_error_value;
    value *end_of_input_value;
    object *the_object;
    fp_call_printer_data *fp_data;
    lepton_key_instance *io_error_key;
    char char_buffer[5];
    verdict the_verdict;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 3);
    object_value = value_component_value(all_arguments_value, 0);
    io_error_value = value_component_value(all_arguments_value, 1);
    end_of_input_value = value_component_value(all_arguments_value, 2);

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    fp_data = fp_data_for_argument(all_arguments_value, 0);
    assert(fp_data != NULL);

    assert(io_error_value != NULL);
    assert(get_value_kind(io_error_value) == VK_LEPTON_KEY);
    io_error_key = value_lepton_key(io_error_value);
    assert(io_error_key != NULL);

    the_verdict = fp_read_character(fp_data, char_buffer, the_object,
                                    io_error_key, the_jumper, location);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;
    if (char_buffer[0] == 0)
      {
        value_add_reference(end_of_input_value);
        return end_of_input_value;
      }

    result = create_character_value(char_buffer);
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
      }
    else
      {
        set_fp_object_eof_and_error_flags_if_needed(the_object, fp_data->fp,
                                                    io_error_key, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            result = NULL;
          }
      }
    return result;
  }

extern value *fp_input_text_read_string_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *object_value;
    value *count_value;
    value *minimum_value;
    value *io_error_value;
    value *end_of_input_value;
    object *the_object;
    fp_call_printer_data *fp_data;
    FILE *fp;
    o_integer count_oi;
    o_integer minimum_oi;
    verdict the_verdict;
    size_t count_size_t;
    size_t minimum_size_t;
    lepton_key_instance *io_error_key;
    char *char_buffer;
    unsigned char *input_buffer;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 5);
    object_value = value_component_value(all_arguments_value, 0);
    count_value = value_component_value(all_arguments_value, 1);
    minimum_value = value_component_value(all_arguments_value, 2);
    io_error_value = value_component_value(all_arguments_value, 3);
    end_of_input_value = value_component_value(all_arguments_value, 4);

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    fp_data = fp_data_for_argument(all_arguments_value, 0);
    assert(fp_data != NULL);
    fp = fp_data->fp;

    assert(count_value != NULL);
    assert(get_value_kind(count_value) == VK_INTEGER);
    count_oi = integer_value_data(count_value);
    assert(!(oi_out_of_memory(count_oi)));

    assert(minimum_value != NULL);
    assert(get_value_kind(minimum_value) == VK_INTEGER);
    minimum_oi = integer_value_data(minimum_value);
    assert(!(oi_out_of_memory(minimum_oi)));

    if (oi_kind(count_oi) == IIK_POSITIVE_INFINITY)
      {
      too_big:
        location_exception(the_jumper, location,
                EXCEPTION_TAG(read_count_too_big),
                "The number of items requested to be read (%I) was too large "
                "for the system memory to handle.", count_oi);
        return NULL;
      }
    assert(oi_kind(count_oi) == IIK_FINITE);
    assert(!(oi_is_negative(count_oi)));
    the_verdict = oi_magnitude_to_size_t(count_oi, &count_size_t);
    if (the_verdict != MISSION_ACCOMPLISHED)
        goto too_big;

    if (count_size_t > (((~(size_t)0) - 1) / 4))
        goto too_big;

    assert(oi_kind(minimum_oi) == IIK_FINITE);
    assert(!(oi_is_negative(minimum_oi)));
    the_verdict = oi_magnitude_to_size_t(minimum_oi, &minimum_size_t);
    assert(the_verdict == MISSION_ACCOMPLISHED);

    assert(io_error_value != NULL);
    assert(get_value_kind(io_error_value) == VK_LEPTON_KEY);
    io_error_key = value_lepton_key(io_error_value);
    assert(io_error_key != NULL);

    if (fp == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(closed_stream_used),
                "A closed I/O stream was used for a read_string() call.");
        return NULL;
      }

    if (ferror(fp))
      {
        set_fp_object_error_info_and_do_exception(the_object, io_error_key,
                the_jumper, location, "I/O error");
        return NULL;
      }

    if (feof(fp))
      {
      do_eof:
        set_fp_object_eof_info(the_object, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return NULL;

        value_add_reference(end_of_input_value);
        return end_of_input_value;
      }

    char_buffer = MALLOC_ARRAY(char, ((count_size_t * 4) + 1));
    if (char_buffer == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    input_buffer = MALLOC_ARRAY(unsigned char,
                                ((count_size_t > 0) ? (count_size_t * 4) : 1));
    if (input_buffer == NULL)
      {
        jumper_do_abort(the_jumper);
        free(char_buffer);
        return NULL;
      }

    switch (fp_data->utf_format)
      {
        case UTF_8:
          {
            size_t chars_left;
            size_t char_position;
            size_t character_num;

            chars_left = 0;
            char_position = 0;

            for (character_num = 0; character_num < count_size_t;
                 ++character_num)
              {
                unsigned char character_bits;

                if (chars_left == 0)
                  {
                    size_t result_code;

                    self_block(jumper_thread(the_jumper), the_jumper,
                               location);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        free(input_buffer);
                        free(char_buffer);
                        return NULL;
                      }

                    result_code = fread(&(input_buffer[char_position]), 1,
                                        count_size_t - character_num, fp);

                    self_unblock(jumper_thread(the_jumper), the_jumper,
                                 location);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        free(input_buffer);
                        free(char_buffer);
                        return NULL;
                      }

                    if (result_code < (count_size_t - character_num))
                      {
                        if (ferror(fp))
                          {
                            free(input_buffer);
                            free(char_buffer);
                            set_fp_object_error_info_and_do_exception(
                                    the_object, io_error_key, the_jumper,
                                    location, "I/O error");
                            return NULL;
                          }

                        assert(feof(fp));

                        if (result_code == 0)
                          {
                            if (character_num >= minimum_size_t)
                              {
                                assert(char_position <= (count_size_t * 4));
                                char_buffer[char_position] = 0;
                                break;
                              }

                            free(input_buffer);
                            free(char_buffer);
                            goto do_eof;
                          }
                      }

                    assert(result_code <= (count_size_t - character_num));
                    chars_left = result_code;
                  }

                char_buffer[char_position] =
                        (char)(input_buffer[char_position]);

                character_bits = input_buffer[char_position];
                if ((character_bits & 0x80) == 0)
                  {
                    ++char_position;
                    --chars_left;
                  }
                else if ((character_bits & 0x40) == 0)
                  {
                    exception_and_fp_error(the_object, io_error_key,
                            the_jumper, location, EXCEPTION_TAG(bad_utf8),
                            "Bad UTF-8 encoding -- a byte (0x%02x) with 0x2 as"
                            " its high two bits was found at the start of a "
                            "character.", (unsigned)character_bits);
                    free(input_buffer);
                    free(char_buffer);
                    return NULL;
                  }
                else
                  {
                    int extra_byte_count;
                    size_t byte_num;
                    exception_error_handler_data *error_data;
                    int valid_count;

                    if ((character_bits & 0x20) == 0)
                      {
                        extra_byte_count = 1;
                      }
                    else if ((character_bits & 0x10) == 0)
                      {
                        extra_byte_count = 2;
                      }
                    else if ((character_bits & 0x08) == 0)
                      {
                        extra_byte_count = 3;
                      }
                    else
                      {
                        exception_and_fp_error(the_object, io_error_key,
                                the_jumper, location, EXCEPTION_TAG(bad_utf8),
                                "Bad UTF-8 encoding -- a byte (0x%02x) with "
                                "0x1f as its high five bits was found at the "
                                "start of a character.",
                                (unsigned)character_bits);
                        free(input_buffer);
                        free(char_buffer);
                        return NULL;
                      }

                    if (chars_left < (extra_byte_count + 1))
                      {
                        size_t additional_count;
                        size_t result_code;

                        additional_count = ((count_size_t - character_num) +
                                            (extra_byte_count - chars_left));

                        self_block(jumper_thread(the_jumper), the_jumper,
                                   location);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            free(input_buffer);
                            free(char_buffer);
                            return NULL;
                          }

                        result_code = fread(
                                &(input_buffer[char_position + chars_left]), 1,
                                additional_count, fp);

                        self_unblock(jumper_thread(the_jumper), the_jumper,
                                     location);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            free(input_buffer);
                            free(char_buffer);
                            return NULL;
                          }

                        if (result_code < additional_count)
                          {
                            if (ferror(fp))
                              {
                                free(input_buffer);
                                free(char_buffer);
                                set_fp_object_error_info_and_do_exception(
                                        the_object, io_error_key, the_jumper,
                                        location, "I/O error");
                                return NULL;
                              }

                            assert(feof(fp));

                            if (result_code <
                                (extra_byte_count + 1) - chars_left)
                              {
                                if (character_num >= minimum_size_t)
                                  {
                                    assert(char_position <=
                                           (count_size_t * 4));
                                    char_buffer[char_position] = 0;
                                    break;
                                  }

                                free(input_buffer);
                                free(char_buffer);
                                goto do_eof;
                              }
                          }

                        assert(result_code <= additional_count);
                        chars_left += result_code;
                        assert(chars_left >= (extra_byte_count + 1));
                      }

                    assert(chars_left >= (extra_byte_count + 1));

                    for (byte_num = 0; byte_num <= extra_byte_count;
                         ++byte_num)
                      {
                        char_buffer[char_position + byte_num] =
                                (char)(input_buffer[char_position + byte_num]);
                      }

                    error_data = create_exception_error_data_with_object(
                            the_jumper, location, the_object, io_error_key);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        free(input_buffer);
                        free(char_buffer);
                        return NULL;
                      }

                    valid_count = validate_utf8_character_with_error_handler(
                            char_buffer, &exception_error_handler, error_data);

                    free(error_data);

                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        assert(valid_count < 0);
                        free(input_buffer);
                        free(char_buffer);
                        return NULL;
                      }

                    assert(valid_count == (extra_byte_count + 1));

                    assert(chars_left >= valid_count);
                    chars_left -= valid_count;
                    char_position += valid_count;
                  }
              }

            free(input_buffer);

            assert(chars_left == 0);
            assert(char_position <= (count_size_t * 4));
            char_buffer[char_position] = 0;

            break;
          }
        case UTF_16_LE:
        case UTF_16_BE:
          {
            size_t chars_left;
            size_t char_position;
            size_t raw_position;
            size_t character_num;

            chars_left = 0;
            char_position = 0;
            raw_position = 0;

            for (character_num = 0; character_num < count_size_t;
                 ++character_num)
              {
                unsigned u16_buffer[2];
                unsigned long code_point;
                size_t character_count;

                if (chars_left == 0)
                  {
                    size_t additional_count;
                    size_t result_code;

                    additional_count = ((count_size_t - character_num) * 2);

                    self_block(jumper_thread(the_jumper), the_jumper,
                               location);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        free(input_buffer);
                        free(char_buffer);
                        return NULL;
                      }

                    result_code = fread(&(input_buffer[raw_position]), 1,
                                        additional_count, fp);

                    self_unblock(jumper_thread(the_jumper), the_jumper,
                                 location);
                    if (!(jumper_flowing_forward(the_jumper)))
                      {
                        free(input_buffer);
                        free(char_buffer);
                        return NULL;
                      }

                    if (result_code < additional_count)
                      {
                        if (ferror(fp))
                          {
                            free(input_buffer);
                            free(char_buffer);
                            set_fp_object_error_info_and_do_exception(
                                    the_object, io_error_key, the_jumper,
                                    location, "I/O error");
                            return NULL;
                          }

                        assert(feof(fp));

                        if (result_code < 2)
                          {
                            if (character_num >= minimum_size_t)
                              {
                                assert(char_position <= (count_size_t * 4));
                                char_buffer[char_position] = 0;
                                break;
                              }

                            free(input_buffer);
                            free(char_buffer);
                            goto do_eof;
                          }
                      }

                    assert(result_code <= additional_count);
                    chars_left = result_code;
                  }

                assert(chars_left >= 2);

                if (fp_data->utf_format == UTF_16_LE)
                  {
                    u16_buffer[0] =
                            ((((unsigned long)input_buffer[raw_position + 1])
                              << 8) |
                             ((unsigned long)input_buffer[raw_position]));
                  }
                else
                  {
                    assert(fp_data->utf_format == UTF_16_BE);
                    u16_buffer[0] =
                            ((((unsigned long)input_buffer[raw_position]) << 8)
                             |
                             ((unsigned long)input_buffer[raw_position + 1]));
                  }

                if ((u16_buffer[0] & 0xfc00) == 0xdc00)
                  {
                    exception_and_fp_error(the_object, io_error_key,
                            the_jumper, location, EXCEPTION_TAG(bad_utf16),
                            "Bad UTF-16 encoding -- a 16-bit block (0x%04lx) "
                            "with 0x37 as its high six bits was found at the "
                            "start of a character.",
                            (unsigned long)(u16_buffer[0]));
                    free(input_buffer);
                    free(char_buffer);
                    return NULL;
                  }

                if ((u16_buffer[0] & 0xfc00) == 0xd800)
                  {
                    if (chars_left < 4)
                      {
                        size_t additional_count;
                        size_t result_code;

                        assert(chars_left == 2);

                        additional_count =
                                ((count_size_t - character_num) * 2);

                        self_block(jumper_thread(the_jumper), the_jumper,
                                   location);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            free(input_buffer);
                            free(char_buffer);
                            return NULL;
                          }

                        result_code = fread(&(input_buffer[raw_position + 2]),
                                            1, additional_count, fp);
                        self_unblock(jumper_thread(the_jumper), the_jumper,
                                     location);
                        if (!(jumper_flowing_forward(the_jumper)))
                          {
                            free(input_buffer);
                            free(char_buffer);
                            return NULL;
                          }

                        if (result_code < additional_count)
                          {
                            if (ferror(fp))
                              {
                                free(input_buffer);
                                free(char_buffer);
                                set_fp_object_error_info_and_do_exception(
                                        the_object, io_error_key, the_jumper,
                                        location, "I/O error");
                                return NULL;
                              }

                            assert(feof(fp));

                            if (result_code < 2)
                              {
                                if (character_num >= minimum_size_t)
                                  {
                                    assert(char_position <=
                                           (count_size_t * 4));
                                    char_buffer[char_position] = 0;
                                    break;
                                  }

                                free(input_buffer);
                                free(char_buffer);
                                goto do_eof;
                              }
                          }

                        assert(result_code <= additional_count);
                        chars_left += additional_count;
                        assert(chars_left >= 4);
                      }

                    assert(chars_left >= 4);

                    if (fp_data->utf_format == UTF_16_LE)
                      {
                        u16_buffer[1] =
                                ((((unsigned long)input_buffer[
                                           raw_position + 3]) << 8) |
                                 ((unsigned long)input_buffer[
                                          raw_position + 2]));
                      }
                    else
                      {
                        assert(fp_data->utf_format == UTF_16_BE);
                        u16_buffer[1] =
                                ((((unsigned long)input_buffer[
                                           raw_position + 2]) << 8) |
                                 ((unsigned long)input_buffer[
                                          raw_position + 3]));
                      }

                    chars_left -= 4;
                    raw_position += 4;
                  }
                else
                  {
                    chars_left -= 2;
                    raw_position += 2;
                  }

                code_point = utf16_to_code_point(u16_buffer);

                if (((code_point >= 0xd800) && (code_point < 0xe000)) ||
                    (code_point >= 0x110000))
                  {
                    exception_and_fp_error(the_object, io_error_key,
                            the_jumper, location, EXCEPTION_TAG(bad_utf16),
                            "Bad UTF-16 character: 0x%08lx.",
                            (unsigned long)code_point);
                    return NULL;
                  }

                character_count = code_point_to_utf8(code_point,
                        &(char_buffer[char_position]));
                assert((character_count >= 1) && (character_count <= 4));

                char_position += character_count;
              }

            free(input_buffer);

            assert(chars_left == 0);
            assert(char_position <= (count_size_t * 4));
            char_buffer[char_position] = 0;

            break;
          }
        case UTF_32_LE:
        case UTF_32_BE:
          {
            size_t result_code;
            size_t char_position;
            size_t character_num;

            self_block(jumper_thread(the_jumper), the_jumper, location);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                free(input_buffer);
                free(char_buffer);
                return NULL;
              }

            result_code = fread(&(input_buffer[0]), 4, count_size_t, fp);

            self_unblock(jumper_thread(the_jumper), the_jumper, location);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                free(input_buffer);
                free(char_buffer);
                return NULL;
              }

            if (result_code < count_size_t)
              {
                if (ferror(fp))
                  {
                    free(input_buffer);
                    free(char_buffer);
                    set_fp_object_error_info_and_do_exception(the_object,
                            io_error_key, the_jumper, location, "I/O error");
                    return NULL;
                  }

                assert(feof(fp));

                if (result_code < minimum_size_t)
                  {
                    free(input_buffer);
                    free(char_buffer);
                    goto do_eof;
                  }
              }

            assert(result_code <= count_size_t);

            char_position = 0;

            for (character_num = 0; character_num < result_code;
                 ++character_num)
              {
                const unsigned char *input_next;
                unsigned long code_point;
                size_t character_count;

                input_next = (input_buffer + (4 * character_num));
                if (fp_data->utf_format == UTF_32_LE)
                  {
                    code_point =
                            ((((unsigned long)input_next[3]) << 24) |
                             (((unsigned long)input_next[2]) << 16) |
                             (((unsigned long)input_next[1]) << 8) |
                             ((unsigned long)input_next[0]));
                  }
                else
                  {
                    assert(fp_data->utf_format == UTF_32_BE);
                    code_point =
                            ((((unsigned long)input_next[0]) << 24) |
                             (((unsigned long)input_next[1]) << 16) |
                             (((unsigned long)input_next[2]) << 8) |
                             ((unsigned long)input_next[3]));
                  }

                if (((code_point >= 0xd800) && (code_point < 0xe000)) ||
                    (code_point >= 0x110000))
                  {
                    exception_and_fp_error(the_object, io_error_key,
                            the_jumper, location, EXCEPTION_TAG(bad_utf32),
                            "Bad UTF-32 character: 0x%08lx.",
                            (unsigned long)code_point);
                    free(input_buffer);
                    free(char_buffer);
                    return NULL;
                  }

                character_count = code_point_to_utf8(code_point,
                        &(char_buffer[char_position]));
                assert((character_count >= 1) && (character_count <= 4));

                char_position += character_count;
                assert(char_position <= (result_code * 4));
              }

            free(input_buffer);

            assert(char_position <= (result_code * 4));
            char_buffer[char_position] = 0;

            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    result = create_string_value(char_buffer);
    free(char_buffer);
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
      }
    else
      {
        set_fp_object_eof_and_error_flags_if_needed(the_object, fp,
                                                    io_error_key, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            result = NULL;
          }
      }
    return result;
  }

extern value *fp_input_text_read_line_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *object_value;
    value *io_error_value;
    value *end_of_input_value;
    object *the_object;
    fp_call_printer_data *fp_data;
    lepton_key_instance *io_error_key;
    char *char_buffer;
    size_t buffer_space;
    size_t position;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 3);
    object_value = value_component_value(all_arguments_value, 0);
    io_error_value = value_component_value(all_arguments_value, 1);
    end_of_input_value = value_component_value(all_arguments_value, 2);

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    fp_data = fp_data_for_argument(all_arguments_value, 0);
    assert(fp_data != NULL);

    assert(io_error_value != NULL);
    assert(get_value_kind(io_error_value) == VK_LEPTON_KEY);
    io_error_key = value_lepton_key(io_error_value);
    assert(io_error_key != NULL);

    buffer_space = 10;
    char_buffer = MALLOC_ARRAY(char, buffer_space);
    if (char_buffer == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }
    position = 0;

    while (TRUE)
      {
        verdict the_verdict;

        if (position + 5 > buffer_space)
          {
            char *new_buffer;

            buffer_space = ((buffer_space * 2) + 3);
            new_buffer = MALLOC_ARRAY(char, buffer_space);
            if (new_buffer == NULL)
              {
                free(char_buffer);
                jumper_do_abort(the_jumper);
                return NULL;
              }
            memcpy(new_buffer, char_buffer, position);
            free(char_buffer);
            char_buffer = new_buffer;
          }

        the_verdict = fp_read_character(fp_data, &(char_buffer[position]),
                the_object, io_error_key, the_jumper, location);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            free(char_buffer);
            return NULL;
          }
        if (char_buffer[position] == 0)
          {
            if (position > 0)
                break;
            free(char_buffer);
            value_add_reference(end_of_input_value);
            return end_of_input_value;
          }
        if (char_buffer[position] == '\n')
          {
            char_buffer[position] = 0;
            break;
          }
        while (char_buffer[position] != 0)
            ++position;
      }

    result = create_string_value(char_buffer);
    free(char_buffer);
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
      }
    else
      {
        set_fp_object_eof_and_error_flags_if_needed(the_object, fp_data->fp,
                                                    io_error_key, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            result = NULL;
          }
      }
    return result;
  }

extern value *fp_flush_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *object_value;
    object *the_object;
    fp_call_printer_data *fp_data;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    object_value = value_component_value(all_arguments_value, 0);

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    fp_data = fp_data_for_argument(all_arguments_value, 0);
    if (fp_data == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(closed_stream_used),
                "A closed I/O stream was used for a flush() call.");
        return NULL;
      }

    fflush(fp_data->fp);

    return NULL;
  }

extern value *fp_close_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *object_value;
    object *the_object;
    fp_call_printer_data *fp_data;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    object_value = value_component_value(all_arguments_value, 0);

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    fp_data = fp_data_for_argument(all_arguments_value, 0);
    if (fp_data == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(closed_stream_used),
                "A closed I/O stream was used for a close() call.");
        return NULL;
      }

    destroy_fp_data(fp_data);
    assert(!(object_is_closed(the_object))); /* VERIFICATION NEEDED */
    object_set_hook(the_object, NULL);
    object_set_hook_cleaner(the_object, NULL);

    return NULL;
  }

extern value *fp_tell_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    fp_call_printer_data *fp_data;
    o_integer result_oi;
    value *result_value;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);

    fp_data = fp_data_for_argument(all_arguments_value, 0);
    if (fp_data == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(closed_stream_used),
                "A closed I/O stream was used for a tell() call.");
        return NULL;
      }

    fp_data->cp_data.jumper = the_jumper;
    fp_data->cp_data.location = location;
    result_oi = oi_tell(fp_data->fp, &tell_error_handler, &(fp_data->cp_data));
    if (oi_out_of_memory(result_oi))
      {
        if (jumper_flowing_forward(the_jumper))
            jumper_do_abort(the_jumper);
        return NULL;
      }

    result_value = create_integer_value(result_oi);
    oi_remove_reference(result_oi);
    if (result_value == NULL)
        jumper_do_abort(the_jumper);
    return result_value;
  }

extern value *fp_seek_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *position_value;
    fp_call_printer_data *fp_data;
    o_integer position_oi;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    position_value = value_component_value(all_arguments_value, 1);

    fp_data = fp_data_for_argument(all_arguments_value, 0);
    if (fp_data == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(closed_stream_used),
                "A closed I/O stream was used for a seek() call.");
        return NULL;
      }

    assert(position_value != NULL);
    assert(get_value_kind(position_value) == VK_INTEGER);
    position_oi = integer_value_data(position_value);
    assert(!(oi_out_of_memory(position_oi)));

    fp_data->cp_data.jumper = the_jumper;
    fp_data->cp_data.location = location;
    oi_seek(fp_data->fp, position_oi, &seek_error_handler,
            &(fp_data->cp_data));

    return NULL;
  }


extern value *fp_seek_end_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    fp_call_printer_data *fp_data;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);

    fp_data = fp_data_for_argument(all_arguments_value, 0);
    if (fp_data == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(closed_stream_used),
                "A closed I/O stream was used for a seek() call.");
        return NULL;
      }

    fp_data->cp_data.jumper = the_jumper;
    fp_data->cp_data.location = location;
    seek_end(fp_data->fp, &seek_error_handler, &(fp_data->cp_data));

    return NULL;
  }

extern value *fp_output_text_print_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *object_value;
    value *io_error_value;
    object *the_object;
    fp_call_printer_data *fp_data;
    FILE *fp;
    lepton_key_instance *io_error_key;
    size_t component_count;
    size_t component_num;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) >= 2);
    object_value = value_component_value(all_arguments_value, 0);
    io_error_value = value_component_value(all_arguments_value, 1);

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    fp_data = fp_data_for_argument(all_arguments_value, 0);
    fp = fp_data->fp;

    assert(io_error_value != NULL);
    assert(get_value_kind(io_error_value) == VK_LEPTON_KEY);
    io_error_key = value_lepton_key(io_error_value);
    assert(io_error_key != NULL);

    if (fp == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(closed_stream_used),
                "A closed I/O stream was used for a print() call.");
        return NULL;
      }

    component_count = value_component_count(all_arguments_value);
    for (component_num = 2; component_num < component_count; ++component_num)
      {
        value *this_value;

        this_value = value_component_value(all_arguments_value, component_num);
        assert(this_value != NULL);

        fp_data->cp_data.verdict = MISSION_ACCOMPLISHED;
        fp_data->cp_data.jumper = the_jumper;
        fp_data->cp_data.location = location;
        if (get_value_kind(this_value) == VK_STRING)
            fp_call_printer(fp_data, "%s", string_value_data(this_value));
        else if (get_value_kind(this_value) == VK_CHARACTER)
            fp_call_printer(fp_data, "%s", character_value_data(this_value));
        else
            fp_call_print_value(this_value, &fp_call_printer, fp_data);
      }

    if (jumper_flowing_forward(the_jumper))
      {
        set_fp_object_error_flag_if_needed(the_object, fp, io_error_key,
                                           the_jumper);
      }

    return NULL;
  }

extern value *fp_output_text_printf_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *object_value;
    value *io_error_value;
    object *the_object;
    fp_call_printer_data *fp_data;
    FILE *fp;
    lepton_key_instance *io_error_key;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) >= 3);
    object_value = value_component_value(all_arguments_value, 0);
    io_error_value = value_component_value(all_arguments_value, 1);

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    fp_data = fp_data_for_argument(all_arguments_value, 0);
    assert(fp_data != NULL);
    fp = fp_data->fp;

    assert(io_error_value != NULL);
    assert(get_value_kind(io_error_value) == VK_LEPTON_KEY);
    io_error_key = value_lepton_key(io_error_value);
    assert(io_error_key != NULL);

    if (fp == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(closed_stream_used),
                "A closed I/O stream was used for a printf() call.");
        return NULL;
      }

    fp_data->cp_data.verdict = MISSION_ACCOMPLISHED;
    fp_data->cp_data.jumper = the_jumper;
    fp_data->cp_data.location = location;

    overload_printf(all_arguments_value, 2, &fp_call_printer, fp_data,
                    &(fp_data->cp_data), &fp_call_print_value);

    if (jumper_flowing_forward(the_jumper))
      {
        set_fp_object_error_flag_if_needed(the_object, fp, io_error_key,
                                           the_jumper);
      }

    return NULL;
  }

extern value *fp_input_bit_read_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *object_value;
    value *bits_value;
    value *count_value;
    value *minimum_value;
    value *io_error_value;
    value *end_of_input_value;
    object *the_object;
    fp_call_printer_data *fp_data;
    FILE *fp;
    o_integer bits_oi;
    o_integer count_oi;
    o_integer minimum_oi;
    o_integer net_bits_oi;
    size_t net_bits_size_t;
    verdict the_verdict;
    lepton_key_instance *io_error_key;
    value *result;
    unsigned char *input_buffer;
    size_t result_code;
    size_t bits_size_t;
    size_t count_size_t;
    size_t minimum_size_t;
    size_t actual_count;
    size_t byte_position;
    size_t bit_position;
    size_t value_num;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 6);
    object_value = value_component_value(all_arguments_value, 0);
    bits_value = value_component_value(all_arguments_value, 1);
    count_value = value_component_value(all_arguments_value, 2);
    minimum_value = value_component_value(all_arguments_value, 3);
    io_error_value = value_component_value(all_arguments_value, 4);
    end_of_input_value = value_component_value(all_arguments_value, 5);

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    fp_data = fp_data_for_argument(all_arguments_value, 0);
    assert(fp_data != NULL);
    fp = fp_data->fp;

    assert(bits_value != NULL);
    assert(get_value_kind(bits_value) == VK_INTEGER);
    bits_oi = integer_value_data(bits_value);
    assert(!(oi_out_of_memory(bits_oi)));

    assert(count_value != NULL);
    assert(get_value_kind(count_value) == VK_INTEGER);
    count_oi = integer_value_data(count_value);
    assert(!(oi_out_of_memory(count_oi)));

    assert(minimum_value != NULL);
    assert(get_value_kind(minimum_value) == VK_INTEGER);
    minimum_oi = integer_value_data(minimum_value);
    assert(!(oi_out_of_memory(minimum_oi)));

    assert((oi_kind(bits_oi) == IIK_FINITE) && !(oi_is_negative(bits_oi)));
    assert((oi_kind(count_oi) == IIK_FINITE) && !(oi_is_negative(count_oi)));
    assert((oi_kind(minimum_oi) == IIK_FINITE) &&
           !(oi_is_negative(minimum_oi)));

    oi_multiply(net_bits_oi, bits_oi, count_oi);
    if (oi_out_of_memory(net_bits_oi))
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    assert((oi_kind(net_bits_oi) == IIK_FINITE) &&
           !(oi_is_negative(net_bits_oi)));
    the_verdict = oi_magnitude_to_size_t(net_bits_oi, &net_bits_size_t);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(read_count_too_big),
                "The total number of bits requested to be read (%I) was too "
                "large for the system memory to handle.", net_bits_oi);
        oi_remove_reference(net_bits_oi);
        return NULL;
      }

    if ((net_bits_size_t % 8) != 0)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(read_bit_count_not_8_divisible),
                "The total number of bits requested to be read (%I) was not a "
                "multiple of 8.", net_bits_oi);
        oi_remove_reference(net_bits_oi);
        return NULL;
      }

    oi_remove_reference(net_bits_oi);

    assert(io_error_value != NULL);
    assert(get_value_kind(io_error_value) == VK_LEPTON_KEY);
    io_error_key = value_lepton_key(io_error_value);
    assert(io_error_key != NULL);

    if (fp == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(closed_stream_used),
                "A closed I/O stream was used for a read() call.");
        return NULL;
      }

    if (ferror(fp))
      {
        set_fp_object_error_info_and_do_exception(the_object, io_error_key,
                the_jumper, location, "I/O error");
        return NULL;
      }

    if (feof(fp))
      {
      do_eof:
        set_fp_object_eof_info(the_object, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return NULL;

        value_add_reference(end_of_input_value);
        return end_of_input_value;
      }

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    if (net_bits_size_t == 0)
        return result;

    input_buffer = MALLOC_ARRAY(unsigned char, (net_bits_size_t / 8));
    if (input_buffer == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    self_block(jumper_thread(the_jumper), the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        free(input_buffer);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    result_code = fread(input_buffer, 1, (net_bits_size_t / 8), fp);

    self_unblock(jumper_thread(the_jumper), the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        free(input_buffer);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    the_verdict = oi_magnitude_to_size_t(bits_oi, &bits_size_t);
    assert(the_verdict == MISSION_ACCOMPLISHED);
    the_verdict = oi_magnitude_to_size_t(count_oi, &count_size_t);
    assert(the_verdict == MISSION_ACCOMPLISHED);
    the_verdict = oi_magnitude_to_size_t(minimum_oi, &minimum_size_t);
    assert(the_verdict == MISSION_ACCOMPLISHED);

    if (result_code < (net_bits_size_t / 8))
      {
        if (ferror(fp))
          {
            free(input_buffer);
            set_fp_object_error_info_and_do_exception(the_object, io_error_key,
                    the_jumper, location, "I/O error");
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        assert(feof(fp));

        assert(net_bits_size_t == (bits_size_t * count_size_t));
        assert(minimum_size_t <= count_size_t);

        if (result_code < ((bits_size_t * minimum_size_t) / 8))
          {
            free(input_buffer);
            value_remove_reference(result, the_jumper);
            goto do_eof;
          }

        actual_count = ((result_code * 8) / bits_size_t);
      }
    else
      {
        actual_count = count_size_t;
      }

    assert(actual_count >= minimum_size_t);

    byte_position = 0;
    bit_position = 0;

    for (value_num = 0; value_num < actual_count; ++value_num)
      {
        o_integer element_oi;
        size_t element_bits;
        size_t bits_remaining;
        value *element_value;
        verdict the_verdict;

        element_oi = oi_null;
        element_bits = 0;
        bits_remaining = bits_size_t;
        assert(bits_remaining > 0);

        while (bits_remaining > 0)
          {
            size_t start_bit_position;
            long int chunk_int;
            o_integer chunk_oi;

            assert(bit_position < 8);
            start_bit_position = bit_position;

            if (bit_position + bits_remaining <= 8)
              {
                if (fp_data->utf_format == UTF_32_LE)
                  {
                    chunk_int = ((input_buffer[byte_position] >> bit_position)
                                 & (0xff >> (8 - bits_remaining)));
                  }
                else
                  {
                    chunk_int = (((input_buffer[byte_position] << bit_position)
                                  & 0xff) >> (8 - bits_remaining));
                  }
                bit_position += bits_remaining;
                bits_remaining = 0;
              }
            else
              {
                if (fp_data->utf_format == UTF_32_LE)
                  {
                    chunk_int = ((input_buffer[byte_position] >> bit_position)
                                 & (0xff >> bit_position));
                  }
                else
                  {
                    chunk_int = (((input_buffer[byte_position] << bit_position)
                                  & 0xff) >> bit_position);
                  }
                assert(bits_remaining > (8 - bit_position));
                bits_remaining -= (8 - bit_position);
                bit_position = 8;
                assert(bits_remaining > 0);
              }

            oi_create_from_long_int(chunk_oi, chunk_int);
            if (oi_out_of_memory(chunk_oi))
              {
                jumper_do_abort(the_jumper);
                if (!(oi_out_of_memory(element_oi)))
                    oi_remove_reference(element_oi);
                free(input_buffer);
                value_remove_reference(result, the_jumper);
                return NULL;
              }

            if (oi_out_of_memory(element_oi))
              {
                element_oi = chunk_oi;
              }
            else
              {
                size_t shift_amount_size_t;
                o_integer shift_amount;
                o_integer to_shift;
                o_integer to_or;
                o_integer shifted;

                assert(bit_position > start_bit_position);
                if (fp_data->utf_format == UTF_32_LE)
                    shift_amount_size_t = element_bits;
                else
                    shift_amount_size_t = bit_position - start_bit_position;
                oi_create_from_size_t(shift_amount, shift_amount_size_t);
                if (oi_out_of_memory(shift_amount))
                  {
                    jumper_do_abort(the_jumper);
                    oi_remove_reference(chunk_oi);
                    oi_remove_reference(element_oi);
                    free(input_buffer);
                    value_remove_reference(result, the_jumper);
                    return NULL;
                  }

                if (fp_data->utf_format == UTF_32_LE)
                  {
                    to_shift = chunk_oi;
                    to_or = element_oi;
                  }
                else
                  {
                    to_shift = element_oi;
                    to_or = chunk_oi;
                  }
                oi_shift_left(shifted, to_shift, shift_amount);
                oi_remove_reference(to_shift);
                oi_remove_reference(shift_amount);
                if (oi_out_of_memory(shifted))
                  {
                    jumper_do_abort(the_jumper);
                    oi_remove_reference(to_or);
                    free(input_buffer);
                    value_remove_reference(result, the_jumper);
                    return NULL;
                  }

                oi_bitwise_or(element_oi, shifted, to_or);
                oi_remove_reference(shifted);
                oi_remove_reference(to_or);
                if (oi_out_of_memory(element_oi))
                  {
                    jumper_do_abort(the_jumper);
                    free(input_buffer);
                    value_remove_reference(result, the_jumper);
                    return NULL;
                  }
              }

            element_bits += bit_position - start_bit_position;

            assert(bit_position <= 8);
            if (bit_position == 8)
              {
                ++byte_position;
                bit_position = 0;
              }
          }

        assert(!(oi_out_of_memory(element_oi)));

        element_value = create_integer_value(element_oi);
        oi_remove_reference(element_oi);
        if (element_value == NULL)
          {
            jumper_do_abort(the_jumper);
            free(input_buffer);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        the_verdict = add_field(result, NULL, element_value);
        value_remove_reference(element_value, the_jumper);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            free(input_buffer);
            value_remove_reference(result, the_jumper);
            return NULL;
          }
      }

    assert(byte_position == ((bits_size_t * actual_count) / 8));
    assert(bit_position == ((bits_size_t * actual_count) % 8));

    free(input_buffer);

    set_fp_object_eof_and_error_flags_if_needed(the_object, fp, io_error_key,
                                                the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(result, the_jumper);
        result = NULL;
      }
    return result;
  }

extern value *fp_is_end_of_input_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    fp_call_printer_data *fp_data;
    FILE *fp;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);

    fp_data = fp_data_for_argument(all_arguments_value, 0);
    assert(fp_data != NULL);
    fp = fp_data->fp;

    assert(fp != NULL);

    if (feof(fp))
        result = create_true_value();
    else
        result = create_false_value();

    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

extern value *fp_output_bit_write_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *object_value;
    value *bits_value;
    value *data_value;
    value *io_error_value;
    object *the_object;
    fp_call_printer_data *fp_data;
    FILE *fp;
    o_integer bits_oi;
    o_integer count_oi;
    o_integer net_bits_oi;
    size_t net_bits_size_t;
    verdict the_verdict;
    size_t bits_size_t;
    size_t count_size_t;
    lepton_key_instance *io_error_key;
    unsigned char *output_buffer;
    size_t byte_position;
    size_t bit_position;
    size_t value_num;
    size_t result_code;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 4);
    object_value = value_component_value(all_arguments_value, 0);
    bits_value = value_component_value(all_arguments_value, 1);
    data_value = value_component_value(all_arguments_value, 2);
    io_error_value = value_component_value(all_arguments_value, 3);

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    fp_data = fp_data_for_argument(all_arguments_value, 0);
    assert(fp_data != NULL);
    fp = fp_data->fp;

    assert(bits_value != NULL);
    assert(get_value_kind(bits_value) == VK_INTEGER);
    bits_oi = integer_value_data(bits_value);
    assert(!(oi_out_of_memory(bits_oi)));

    assert(data_value != NULL);
    switch (get_value_kind(data_value))
      {
        case VK_INTEGER:
            count_oi = oi_one;
            break;
        case VK_SEMI_LABELED_VALUE_LIST:
            oi_create_from_size_t(count_oi, value_component_count(data_value));
            break;
        case VK_MAP:
            count_oi = map_value_length(data_value, "fp_output_bit_write",
                                        the_jumper, location);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(oi_out_of_memory(count_oi));
                return NULL;
              }
            break;
        default:
            assert(FALSE);
            count_oi = oi_null;
      }
    if (oi_out_of_memory(count_oi))
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    assert((oi_kind(bits_oi) == IIK_FINITE) && !(oi_is_negative(bits_oi)));
    assert((oi_kind(count_oi) == IIK_POSITIVE_INFINITY) ||
           ((oi_kind(count_oi) == IIK_FINITE) && !(oi_is_negative(count_oi))));

    oi_multiply(net_bits_oi, bits_oi, count_oi);
    if (oi_out_of_memory(net_bits_oi))
      {
        jumper_do_abort(the_jumper);
        oi_remove_reference(count_oi);
        return NULL;
      }

    assert((oi_kind(net_bits_oi) == IIK_FINITE) &&
           !(oi_is_negative(net_bits_oi)));
    assert(oi_kind(net_bits_oi) == IIK_FINITE);
    the_verdict = oi_magnitude_to_size_t(net_bits_oi, &net_bits_size_t);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(write_count_too_big),
                "The total number of bits requested to be written (%I) was too"
                " large for the system memory to handle.", net_bits_oi);
        oi_remove_reference(net_bits_oi);
        oi_remove_reference(count_oi);
        return NULL;
      }

    the_verdict = oi_magnitude_to_size_t(bits_oi, &bits_size_t);
    assert(the_verdict == MISSION_ACCOMPLISHED);
    the_verdict = oi_magnitude_to_size_t(count_oi, &count_size_t);
    assert(the_verdict == MISSION_ACCOMPLISHED);
    oi_remove_reference(count_oi);

    if ((net_bits_size_t % 8) != 0)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(write_bit_count_not_8_divisible),
                "The total number of bits requested to be written (%I) was not"
                " a multiple of 8.", net_bits_oi);
        oi_remove_reference(net_bits_oi);
        return NULL;
      }

    oi_remove_reference(net_bits_oi);

    assert(io_error_value != NULL);
    assert(get_value_kind(io_error_value) == VK_LEPTON_KEY);
    io_error_key = value_lepton_key(io_error_value);
    assert(io_error_key != NULL);

    if (fp == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(closed_stream_used),
                "A closed I/O stream was used for a write() call.");
        return NULL;
      }

    if (ferror(fp))
      {
        set_fp_object_error_info(the_object, io_error_key, the_jumper,
                                 "I/O error.");
        return NULL;
      }

    if (net_bits_size_t == 0)
        return NULL;

    output_buffer = MALLOC_ARRAY(unsigned char, (net_bits_size_t / 8));
    if (output_buffer == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    byte_position = 0;
    bit_position = 0;

    for (value_num = 0; value_num < count_size_t; ++value_num)
      {
        size_t value_index;
        value *element_value;
        o_integer element_oi;
        size_t bits_remaining;
        o_integer remainder_oi;
        boolean is_zero;

        if (fp_data->utf_format == UTF_32_LE)
            value_index = value_num;
        else
            value_index = (count_size_t - (value_num + 1));

        switch (get_value_kind(data_value))
          {
            case VK_INTEGER:
              {
                assert(value_index == 0);
                element_value = data_value;
                break;
              }
            case VK_SEMI_LABELED_VALUE_LIST:
              {
                element_value = value_component_value(data_value, value_index);
                break;
              }
            case VK_MAP:
              {
                o_integer key_oi;
                value *key_value;
                boolean doubt;

                oi_create_from_size_t(key_oi, value_index);
                if (oi_out_of_memory(key_oi))
                  {
                    jumper_do_abort(the_jumper);
                    free(output_buffer);
                    return NULL;
                  }

                key_value = create_integer_value(key_oi);
                oi_remove_reference(key_oi);
                if (key_value == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    free(output_buffer);
                    return NULL;
                  }

                assert(map_value_all_keys_are_valid(data_value));
                        /* VERIFICATION NEEDED */
                assert(value_is_valid(key_value)); /* VERIFICATION NEEDED */
                element_value = map_value_lookup(data_value, key_value, &doubt,
                                                 location, the_jumper);
                value_remove_reference(key_value, the_jumper);
                assert(!doubt);
                if (element_value == NULL)
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(write_missing_element),
                            "Element %lu missing in the data for a write() "
                            "call.", (unsigned long)value_index);
                    free(output_buffer);
                    return NULL;
                  }

                break;
              }
            default:
              {
                assert(FALSE);
                element_value = NULL;
              }
          }

        assert(element_value != NULL);
        assert(get_value_kind(element_value) == VK_INTEGER);
        element_oi = integer_value_data(element_value);
        assert(!(oi_out_of_memory(element_oi)));
        assert((oi_kind(element_oi) == IIK_POSITIVE_INFINITY) ||
               ((oi_kind(element_oi) == IIK_FINITE) &&
                !(oi_is_negative(element_oi))));

        bits_remaining = bits_size_t;
        assert(bits_remaining > 0);
        remainder_oi = element_oi;
        oi_add_reference(remainder_oi);

        while (bits_remaining > 0)
          {
            size_t chunk_bits;
            o_integer mask_oi;
            o_integer chunk_oi;
            size_t chunk_size_t;
            verdict the_verdict;
            unsigned char new_bits;
            size_t byte_index;
            o_integer shift_amount_oi;
            o_integer new_remainder;

            assert(bit_position < 8);

            if (bit_position + bits_remaining <= 8)
              {
                chunk_bits = bits_remaining;
              }
            else
              {
                chunk_bits = 8 - bit_position;
                assert(bits_remaining > chunk_bits);
              }
            assert(bits_remaining >= chunk_bits);
            assert(chunk_bits <= 8);
            assert(chunk_bits > 0);
            assert(bit_position + chunk_bits <= 8);

            oi_create_from_size_t(mask_oi, 0xff >> (8 - chunk_bits));
            if (oi_out_of_memory(mask_oi))
              {
                jumper_do_abort(the_jumper);
                oi_remove_reference(remainder_oi);
                free(output_buffer);
                return NULL;
              }

            oi_bitwise_and(chunk_oi, remainder_oi, mask_oi);
            oi_remove_reference(mask_oi);
            if (oi_out_of_memory(chunk_oi))
              {
                jumper_do_abort(the_jumper);
                oi_remove_reference(remainder_oi);
                free(output_buffer);
                return NULL;
              }

            the_verdict = oi_magnitude_to_size_t(chunk_oi, &chunk_size_t);
            assert(the_verdict == MISSION_ACCOMPLISHED);
            oi_remove_reference(chunk_oi);

            new_bits = (unsigned char)chunk_size_t;
            new_bits <<= bit_position;

            if (fp_data->utf_format == UTF_32_LE)
                byte_index = byte_position;
            else
                byte_index = ((net_bits_size_t / 8) - (byte_position + 1));

            if (bit_position == 0)
                output_buffer[byte_index] = new_bits;
            else
                output_buffer[byte_index] |= new_bits;

            oi_create_from_size_t(shift_amount_oi, chunk_bits);
            if (oi_out_of_memory(shift_amount_oi))
              {
                jumper_do_abort(the_jumper);
                oi_remove_reference(remainder_oi);
                free(output_buffer);
                return NULL;
              }

            oi_shift_right(new_remainder, remainder_oi, shift_amount_oi);
            oi_remove_reference(remainder_oi);
            oi_remove_reference(shift_amount_oi);
            if (oi_out_of_memory(new_remainder))
              {
                jumper_do_abort(the_jumper);
                free(output_buffer);
                return NULL;
              }
            remainder_oi = new_remainder;

            bit_position += chunk_bits;
            bits_remaining -= chunk_bits;

            assert(bit_position <= 8);
            if (bit_position == 8)
              {
                ++byte_position;
                bit_position = 0;
              }
          }

        is_zero = oi_equal(remainder_oi, oi_zero);
        if (!is_zero)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(write_element_too_big),
                    "In a write() call, Element %lu (%I) has more bits than "
                    "specified per element (%I).", (unsigned long)value_index,
                    element_oi, bits_oi);
            oi_remove_reference(remainder_oi);
            free(output_buffer);
            return NULL;
          }

        oi_remove_reference(remainder_oi);
      }

    assert(byte_position == (net_bits_size_t / 8));
    assert(bit_position == 0);

    result_code = fwrite(output_buffer, 1, (net_bits_size_t / 8), fp);
    free(output_buffer);
    if (result_code < (net_bits_size_t / 8))
      {
        if (ferror(fp))
          {
            set_fp_object_error_info(the_object, io_error_key, the_jumper,
                                     "I/O error.");
          }
        else
          {
            assert(feof(fp));
            set_fp_object_eof_info(the_object, the_jumper);
          }
        return NULL;
      }

    set_fp_object_error_flag_if_needed(the_object, fp, io_error_key,
                                       the_jumper);
    return NULL;
  }


static value *get_integer_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    type *the_type;
    value *result;

    the_type = get_integer_type();
    if (the_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_type_value(the_type);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_rational_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    type *the_type;
    value *result;

    the_type = get_rational_type();
    if (the_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_type_value(the_type);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_string_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    type *the_type;
    value *result;

    the_type = get_string_type();
    if (the_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_type_value(the_type);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_character_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    type *the_type;
    value *result;

    the_type = get_character_type();
    if (the_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_type_value(the_type);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_regular_expression_type_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    type *the_type;
    value *result;

    the_type = get_any_regular_expression_type();
    if (the_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_type_value(the_type);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_any_quark_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    type *the_type;
    value *result;

    the_type = get_any_quark_type();
    if (the_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_type_value(the_type);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_any_lepton_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    type *the_type;
    value *result;

    the_type = get_any_lepton_type();
    if (the_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_type_value(the_type);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_lepton_key_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    type *the_type;
    value *result;

    the_type = get_lepton_key_type();
    if (the_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_type_value(the_type);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_jump_target_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    type *the_type;
    value *result;

    the_type = get_jump_target_type();
    if (the_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_type_value(the_type);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_any_class_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    type *the_type;
    value *result;

    the_type = get_any_class_type();
    if (the_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_type_value(the_type);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_object_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    type *the_type;
    value *result;

    the_type = get_object_type();
    if (the_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_type_value(the_type);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_tagalong_key_type_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    type *the_type;
    value *result;

    the_type = get_tagalong_key_type();
    if (the_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_type_value(the_type);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_any_lock_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    type *the_type;
    value *result;

    the_type = get_lock_type();
    if (the_type == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_type_value(the_type);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_null_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *result;

    result = create_null_value();
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_true_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *result;

    result = create_true_value();
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *get_false_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *result;

    result = create_false_value();
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *characters_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *string_chars;
    value *result;
    const char *follow;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    string_chars = string_for_argument(all_arguments_value, 0);

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    follow = string_chars;

    while (*follow != 0)
      {
        int bytes_in_character;
        char buffer[5];
        value *character_value;
        verdict the_verdict;

        bytes_in_character = validate_utf8_character(follow);

        assert(bytes_in_character >= 1);
        assert(bytes_in_character <= 4);

        memcpy(buffer, follow, bytes_in_character);
        buffer[bytes_in_character] = 0;

        character_value = create_character_value(buffer);
        if (character_value == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        the_verdict = add_field(result, NULL, character_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(character_value, the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        value_remove_reference(character_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        follow += bytes_in_character;
      }

    return result;
  }

static value *make_string_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *array_value;
    size_t element_count;
    string_buffer output_buffer;
    verdict the_verdict;
    size_t element_num;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    array_value = value_component_value(all_arguments_value, 0);

    element_count = array_value_element_count(array_value, "make_string",
                                              the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    the_verdict = string_buffer_init(&output_buffer, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    for (element_num = 0; element_num < element_count; ++element_num)
      {
        value *element_value;
        const char *element_bytes;

        element_value = array_value_element_value(array_value, element_num,
                                                  the_jumper, location);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(element_value == NULL);
            free(output_buffer.array);
            return NULL;
          }

        if (element_value == NULL)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(make_string_undefined),
                    "In make_string(), element %lu of the array is undefined.",
                    (unsigned long)element_num);
            free(output_buffer.array);
            return NULL;
          }

        assert(get_value_kind(element_value) == VK_CHARACTER);
        element_bytes = character_value_data(element_value);
        assert(element_bytes != NULL);
        assert(strlen(element_bytes) ==
               validate_utf8_character(element_bytes));
        assert(*element_bytes != 0);

        do
          {
            verdict the_verdict;

            the_verdict = string_buffer_append(&output_buffer, *element_bytes);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                free(output_buffer.array);
                return NULL;
              }

            ++element_bytes;
          } while (*element_bytes != 0);
      }

    the_verdict = string_buffer_append(&output_buffer, 0);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        free(output_buffer.array);
        return NULL;
      }

    result = create_string_value(output_buffer.array);
    free(output_buffer.array);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *from_utf8_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *array_value;
    size_t element_count;
    size_t element_num;
    char buffer[5];
    exception_error_handler_data *error_data;
    int char_count;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    array_value = value_component_value(all_arguments_value, 0);

    element_count = array_value_element_count(array_value, "from_utf8",
                                              the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    assert(element_count >= 1);
    assert(element_count <= 4);

    for (element_num = 0; element_num < element_count; ++element_num)
      {
        unsigned long element_u32;

        element_u32 = array_value_element_u32(array_value, element_num,
                the_jumper, location, EXCEPTION_TAG(from_utf8_undefined),
                "from_utf8");
        if (!(jumper_flowing_forward(the_jumper)))
            return NULL;

        assert(element_u32 <= 0xff);
        buffer[element_num] = (char)element_u32;
      }

    buffer[element_count] = 0;

    error_data = create_exception_error_data(the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    char_count = validate_utf8_character_with_error_handler(buffer,
            &exception_error_handler, error_data);

    free(error_data);

    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    if (char_count != strlen(buffer))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(from_utf8_more_than_one),
                "In from_utf8(), the argument array contains more than a "
                "single character in UTF-8 format.");
        return NULL;
      }

    result = create_character_value(buffer);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *from_utf16_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *array_value;
    size_t element_count;
    size_t element_num;
    unsigned int u16_buffer[2];
    exception_error_handler_data *error_data;
    char char_buffer[9];
    size_t character_count;
    size_t byte_count;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    array_value = value_component_value(all_arguments_value, 0);

    element_count = array_value_element_count(array_value, "from_utf16",
                                              the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    assert(element_count >= 1);
    assert(element_count <= 2);

    for (element_num = 0; element_num < element_count; ++element_num)
      {
        unsigned long element_u32;

        element_u32 = array_value_element_u32(array_value, element_num,
                the_jumper, location, EXCEPTION_TAG(from_utf16_undefined),
                "from_utf16");
        if (!(jumper_flowing_forward(the_jumper)))
            return NULL;

        assert(element_u32 <= 0xffff);
        u16_buffer[element_num] = (unsigned int)element_u32;
      }

    error_data = create_exception_error_data(the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    convert_utf16_to_utf8(&(u16_buffer[0]), element_count, &(char_buffer[0]),
            &character_count, &byte_count, &exception_error_handler,
            error_data);

    free(error_data);

    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    if (character_count != 1)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(from_utf16_more_than_one),
                "In from_utf16(), the argument array contains more than a "
                "single character in UTF-16 format.");
        return NULL;
      }

    char_buffer[byte_count] = 0;

    result = create_character_value(char_buffer);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *from_utf32_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *u32_value;
    o_integer u32_oi;
    size_t u32_size_t;
    verdict the_verdict;
    char char_buffer[5];
    size_t char_bytes;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    u32_value = value_component_value(all_arguments_value, 0);
    assert(u32_value != NULL);

    assert(get_value_kind(u32_value) == VK_INTEGER);
    u32_oi = integer_value_data(u32_value);
    assert(!(oi_out_of_memory(u32_oi)));
    assert(oi_kind(u32_oi) == IIK_FINITE);
    assert(!(oi_is_negative(u32_oi)));

    the_verdict = oi_magnitude_to_size_t(u32_oi, &u32_size_t);
    assert(the_verdict == MISSION_ACCOMPLISHED);

    assert(u32_size_t <= 0xffffffff);

    char_bytes =
            code_point_to_utf8((unsigned long)u32_size_t, &(char_buffer[0]));

    char_buffer[char_bytes] = 0;

    result = create_character_value(char_buffer);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *to_utf8_character_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *character_value;
    const char *character_chars;
    value *result;
    int bytes_in_character;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    character_value = value_component_value(all_arguments_value, 0);

    assert(get_value_kind(character_value) == VK_CHARACTER);
    character_chars = character_value_data(character_value);
    assert(character_chars != NULL);

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    bytes_in_character = validate_utf8_character(character_chars);

    assert(bytes_in_character >= 1);
    assert(bytes_in_character <= 4);

    while (bytes_in_character > 0)
      {
        o_integer u8_oi;
        value *u8_value;
        verdict the_verdict;

        oi_create_from_size_t(u8_oi, *(unsigned char *)character_chars);
        if (oi_out_of_memory(u8_oi))
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        u8_value = create_integer_value(u8_oi);
        oi_remove_reference(u8_oi);
        if (u8_value == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        the_verdict = add_field(result, NULL, u8_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(u8_value, the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        value_remove_reference(u8_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        --bytes_in_character;
        ++character_chars;
      }

    return result;
  }

static value *to_utf16_character_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *character_value;
    const char *character_chars;
    value *result;
    unsigned long code_point;
    unsigned u16_buffer[2];
    size_t u16s_in_character;
    unsigned *follow_u16s;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    character_value = value_component_value(all_arguments_value, 0);

    assert(get_value_kind(character_value) == VK_CHARACTER);
    character_chars = character_value_data(character_value);
    assert(character_chars != NULL);

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    code_point = utf8_to_code_point(character_chars);

    u16s_in_character = code_point_to_utf16(code_point, &(u16_buffer[0]));

    assert(u16s_in_character >= 1);
    assert(u16s_in_character <= 2);

    follow_u16s = &(u16_buffer[0]);

    while (u16s_in_character > 0)
      {
        o_integer u16_oi;
        value *u16_value;
        verdict the_verdict;

        oi_create_from_size_t(u16_oi, *follow_u16s);
        if (oi_out_of_memory(u16_oi))
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        u16_value = create_integer_value(u16_oi);
        oi_remove_reference(u16_oi);
        if (u16_value == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        the_verdict = add_field(result, NULL, u16_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(u16_value, the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        value_remove_reference(u16_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        --u16s_in_character;
        ++follow_u16s;
      }

    return result;
  }

static value *to_utf32_character_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *character_value;
    const char *character_chars;
    unsigned long code_point;
    o_integer u32_oi;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    character_value = value_component_value(all_arguments_value, 0);

    assert(get_value_kind(character_value) == VK_CHARACTER);
    character_chars = character_value_data(character_value);
    assert(character_chars != NULL);

    code_point = utf8_to_code_point(character_chars);

    oi_create_from_size_t(u32_oi, code_point);
    if (oi_out_of_memory(u32_oi))
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_integer_value(u32_oi);
    oi_remove_reference(u32_oi);
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    return result;
  }

static value *string_from_utf8_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *array_value;
    size_t element_count;
    char *buffer;
    size_t element_num;
    exception_error_handler_data *error_data;
    const char *follow;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    array_value = value_component_value(all_arguments_value, 0);

    element_count = array_value_element_count(array_value, "string_from_utf8",
                                              the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    assert(element_count >= 0);

    buffer = MALLOC_ARRAY(char, element_count + 1);
    if (buffer == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    for (element_num = 0; element_num < element_count; ++element_num)
      {
        unsigned long element_u32;

        element_u32 = array_value_element_u32(array_value, element_num,
                the_jumper, location,
                EXCEPTION_TAG(string_from_utf8_undefined), "string_from_utf8");
        if (!(jumper_flowing_forward(the_jumper)))
          {
            free(buffer);
            return NULL;
          }

        assert(element_u32 <= 0xff);
        buffer[element_num] = (char)element_u32;
      }

    buffer[element_count] = 0;

    error_data = create_exception_error_data(the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        free(buffer);
        return NULL;
      }

    follow = buffer;
    while (*follow != 0)
      {
        int char_count;

        char_count = validate_utf8_character_with_error_handler(buffer,
                &exception_error_handler, error_data);
        if (char_count < 0)
            break;

        assert(char_count > 0);
        follow += char_count;
      }

    free(error_data);

    if (!(jumper_flowing_forward(the_jumper)))
      {
        free(buffer);
        return NULL;
      }

    result = create_string_value(buffer);
    free(buffer);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *string_from_utf16_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *array_value;
    size_t element_count;
    unsigned *u16_buffer;
    size_t element_num;
    char *char_buffer;
    exception_error_handler_data *error_data;
    size_t character_count;
    size_t byte_count;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    array_value = value_component_value(all_arguments_value, 0);

    element_count = array_value_element_count(array_value, "string_from_utf16",
                                              the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    assert(element_count >= 0);

    if (element_count == 0)
      {
        u16_buffer = NULL;
      }
    else
      {
        u16_buffer = MALLOC_ARRAY(unsigned, element_count);
        if (u16_buffer == NULL)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }
      }

    for (element_num = 0; element_num < element_count; ++element_num)
      {
        unsigned long element_u32;

        element_u32 = array_value_element_u32(array_value, element_num,
                the_jumper, location,
                EXCEPTION_TAG(string_from_utf16_undefined),
                "string_from_utf16");
        if (!(jumper_flowing_forward(the_jumper)))
          {
            free(u16_buffer);
            return NULL;
          }

        assert(element_u32 <= 0xffff);
        assert(u16_buffer != NULL);
        u16_buffer[element_num] = (unsigned)element_u32;
      }

    char_buffer = MALLOC_ARRAY(char, (element_count * 4) + 1);
    if (char_buffer == NULL)
      {
        if (u16_buffer != NULL)
            free(u16_buffer);
        jumper_do_abort(the_jumper);
        return NULL;
      }

    error_data = create_exception_error_data(the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        if (u16_buffer != NULL)
            free(u16_buffer);
        free(char_buffer);
        return NULL;
      }

    convert_utf16_to_utf8(&(u16_buffer[0]), element_count, &(char_buffer[0]),
            &character_count, &byte_count, &exception_error_handler,
            error_data);

    free(error_data);

    if (u16_buffer != NULL)
        free(u16_buffer);

    if (!(jumper_flowing_forward(the_jumper)))
      {
        free(char_buffer);
        return NULL;
      }

    char_buffer[byte_count] = 0;

    result = create_string_value(char_buffer);
    free(char_buffer);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *string_from_utf32_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *array_value;
    size_t element_count;
    char *char_buffer;
    size_t char_num;
    size_t element_num;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    array_value = value_component_value(all_arguments_value, 0);

    element_count = array_value_element_count(array_value, "string_from_utf32",
                                              the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    assert(element_count >= 0);

    char_buffer = MALLOC_ARRAY(char, (element_count * 4) + 1);
    if (char_buffer == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    char_num = 0;

    for (element_num = 0; element_num < element_count; ++element_num)
      {
        unsigned long element_u32;
        size_t char_bytes;

        element_u32 = array_value_element_u32(array_value, element_num,
                the_jumper, location,
                EXCEPTION_TAG(string_from_utf32_undefined),
                "string_from_utf32");
        if (!(jumper_flowing_forward(the_jumper)))
          {
            free(char_buffer);
            return NULL;
          }

        assert(element_u32 <= 0xffffffff);
        char_bytes = code_point_to_utf8((unsigned long)element_u32,
                                        &(char_buffer[char_num]));
        assert(char_bytes > 0);
        assert(char_bytes <= 4);
        char_num += char_bytes;
        assert(char_num <= (element_count * 4));
      }

    char_buffer[char_num] = 0;

    result = create_string_value(char_buffer);
    free(char_buffer);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *to_utf8_string_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *string_chars;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    string_chars = string_for_argument(all_arguments_value, 0);

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    while (*string_chars != 0)
      {
        o_integer u8_oi;
        value *u8_value;
        verdict the_verdict;

        oi_create_from_size_t(u8_oi, *(unsigned char *)string_chars);
        if (oi_out_of_memory(u8_oi))
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        u8_value = create_integer_value(u8_oi);
        oi_remove_reference(u8_oi);
        if (u8_value == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        the_verdict = add_field(result, NULL, u8_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(u8_value, the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        value_remove_reference(u8_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        ++string_chars;
      }

    return result;
  }

static value *to_utf16_string_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *string_chars;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    string_chars = string_for_argument(all_arguments_value, 0);

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    while (*string_chars != 0)
      {
        unsigned long code_point;
        unsigned u16_buffer[2];
        size_t u16s_in_character;
        unsigned *follow_u16s;
        int char_count;

        code_point = utf8_to_code_point(string_chars);

        u16s_in_character = code_point_to_utf16(code_point, &(u16_buffer[0]));

        assert(u16s_in_character >= 1);
        assert(u16s_in_character <= 2);

        follow_u16s = &(u16_buffer[0]);

        while (u16s_in_character > 0)
          {
            o_integer u16_oi;
            value *u16_value;
            verdict the_verdict;

            oi_create_from_size_t(u16_oi, *follow_u16s);
            if (oi_out_of_memory(u16_oi))
              {
                jumper_do_abort(the_jumper);
                value_remove_reference(result, the_jumper);
                return NULL;
              }

            u16_value = create_integer_value(u16_oi);
            oi_remove_reference(u16_oi);
            if (u16_value == NULL)
              {
                jumper_do_abort(the_jumper);
                value_remove_reference(result, the_jumper);
                return NULL;
              }

            the_verdict = add_field(result, NULL, u16_value);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                value_remove_reference(u16_value, the_jumper);
                value_remove_reference(result, the_jumper);
                return NULL;
              }

            value_remove_reference(u16_value, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                value_remove_reference(result, the_jumper);
                return NULL;
              }

            --u16s_in_character;
            ++follow_u16s;
          }

        char_count = validate_utf8_character(string_chars);
        assert(char_count > 0);
        string_chars += char_count;
      }

    return result;
  }

static value *to_utf32_string_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *string_chars;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    string_chars = string_for_argument(all_arguments_value, 0);

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    while (*string_chars != 0)
      {
        unsigned long code_point;
        o_integer u32_oi;
        value *u32_value;
        verdict the_verdict;
        int char_count;

        code_point = utf8_to_code_point(string_chars);

        oi_create_from_size_t(u32_oi, code_point);
        if (oi_out_of_memory(u32_oi))
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        u32_value = create_integer_value(u32_oi);
        oi_remove_reference(u32_oi);
        if (u32_value == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        the_verdict = add_field(result, NULL, u32_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(u32_value, the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        value_remove_reference(u32_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        char_count = validate_utf8_character(string_chars);
        assert(char_count > 0);
        string_chars += char_count;
      }

    return result;
  }

static value *delete_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *to_delete_value;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    to_delete_value = value_component_value(all_arguments_value, 0);

    switch(get_value_kind(to_delete_value))
      {
        case VK_SLOT_LOCATION:
          {
            slot_location *the_slot;
            variable_instance *instance;
            variable_declaration *declaration;

            the_slot = slot_location_value_data(to_delete_value);
            assert(the_slot != NULL);

            switch (get_slot_location_kind(the_slot))
              {
                case SLK_VARIABLE:
                    instance = variable_slot_location_variable(the_slot);
                    assert(instance != NULL);
                    break;
                case SLK_LOOKUP:
                case SLK_FIELD:
                case SLK_TAGALONG:
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(delete_variable_component),
                            "In delete(), an attempt was made to delete a "
                            "component of a variable instead of the entire "
                            "variable.");
                    return NULL;
                case SLK_CALL:
                case SLK_PASS:
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(delete_variable_overloaded),
                            "In delete(), an attempt was made to delete "
                            "through an overloaded pointer.");
                    return NULL;
                default:
                    assert(FALSE);
                    return NULL;
              }

            assert(instance != NULL);
            declaration = variable_instance_declaration(instance);
            assert(declaration != NULL);

            if (variable_declaration_automatic_allocation(declaration))
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(delete_variable_automatic),
                        "In delete(), the variable to be deleted was "
                        "automatically allocated.");
                return NULL;
              }

            assert(!(variable_instance_scope_exited(instance))); /* VERIFIED */

            set_variable_instance_scope_exited(instance, the_jumper);

            return NULL;
          }
        case VK_ROUTINE:
          {
            delete_for_routine_instance(routine_value_data(to_delete_value),
                                        the_jumper, location);
            return NULL;
          }
        case VK_ROUTINE_CHAIN:
          {
            routine_instance_chain *chain;

            chain = routine_chain_value_data(to_delete_value);
            assert(chain != NULL);

            delete_for_routine_instance(routine_instance_chain_instance(chain),
                                        the_jumper, location);
            return NULL;
          }
        case VK_TAGALONG_KEY:
          {
            tagalong_key *instance;
            tagalong_declaration *declaration;

            instance = tagalong_key_value_data(to_delete_value);
            assert(instance != NULL);

            declaration = tagalong_key_declaration(instance);
            assert(declaration != NULL);

            if (tagalong_declaration_automatic_allocation(declaration))
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(delete_tagalong_key_automatic),
                        "In delete(), the tagalong key to be deleted was "
                        "automatically allocated.");
                return NULL;
              }

            assert(!(tagalong_key_scope_exited(instance))); /* VERIFIED */

            set_tagalong_key_scope_exited(instance, the_jumper);

            return NULL;
          }
        case VK_LEPTON_KEY:
          {
            lepton_key_instance *instance;
            lepton_key_declaration *declaration;

            instance = value_lepton_key(to_delete_value);
            assert(instance != NULL);

            declaration = lepton_key_instance_declaration(instance);
            assert(declaration != NULL);

            if (lepton_key_declaration_automatic_allocation(declaration))
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(delete_lepton_key_automatic),
                        "In delete(), the lepton key to be deleted was "
                        "automatically allocated.");
                return NULL;
              }

            assert(!(lepton_key_instance_scope_exited(instance)));
                    /* VERIFIED */

            set_lepton_key_instance_scope_exited(instance, the_jumper);

            return NULL;
          }
        case VK_QUARK:
          {
            quark *instance;
            quark_declaration *declaration;
            verdict the_verdict;

            instance = value_quark(to_delete_value);
            assert(instance != NULL);

            declaration = quark_instance_declaration(instance);
            assert(declaration != NULL);

            if (quark_declaration_automatic_allocation(declaration))
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(delete_quark_automatic),
                        "In delete(), the quark to be deleted was "
                        "automatically allocated.");
                return NULL;
              }

            assert(!(quark_scope_exited(instance))); /* VERIFIED */

            the_verdict = set_quark_scope_exited(instance);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                return NULL;
              }

            return NULL;
          }
        case VK_LOCK:
          {
            lock_instance *instance;
            lock_declaration *declaration;

            instance = lock_value_data(to_delete_value);
            assert(instance != NULL);

            declaration = lock_instance_declaration(instance);
            assert(declaration != NULL);

            if (lock_declaration_automatic_allocation(declaration))
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(delete_lock_automatic),
                        "In delete(), the lock to be deleted was automatically"
                        " allocated.");
                return NULL;
              }

            assert(!(lock_instance_scope_exited(instance))); /* VERIFIED */

            set_lock_instance_scope_exited(instance, the_jumper);

            return NULL;
          }
        case VK_OBJECT:
          {
            object *the_object;

            the_object = object_value_data(to_delete_value);
            assert(the_object != NULL);

            assert(!(object_is_closed(the_object))); /* VERIFIED */

            if (!(object_is_complete(the_object)))
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(delete_object_incomplete),
                        "In delete(), an attempt was made to delete an "
                        "incomplete object.");
                return NULL;
              }

            close_object(the_object, the_jumper);

            return NULL;
          }
        default:
          {
            assert(FALSE);
            return NULL;
          }
      }
  }

static value *matches_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *regular_expression_value;
    regular_expression *the_regular_expression;
    const char *string_chars;
    boolean error;
    boolean does_match;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    regular_expression_value = value_component_value(all_arguments_value, 0);

    assert(get_value_kind(regular_expression_value) == VK_REGULAR_EXPRESSION);
    the_regular_expression =
            regular_expression_value_data(regular_expression_value);
    assert(the_regular_expression != NULL);

    string_chars = string_for_argument(all_arguments_value, 1);

    does_match = matches(the_regular_expression, string_chars, TRUE, &error);
    if (error)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    if (does_match)
        result = create_true_value();
    else
        result = create_false_value();

    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *split_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *regular_expression_value;
    regular_expression *the_regular_expression;
    const char *string_chars;
    value *result;
    const char *follow;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    regular_expression_value = value_component_value(all_arguments_value, 0);

    assert(get_value_kind(regular_expression_value) == VK_REGULAR_EXPRESSION);
    the_regular_expression =
            regular_expression_value_data(regular_expression_value);
    assert(the_regular_expression != NULL);

    string_chars = string_for_argument(all_arguments_value, 1);

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    follow = string_chars;

    while (TRUE)
      {
        boolean error;
        size_t match_start;
        size_t match_length;
        boolean match_found;
        char *section_chars;
        value *section_value;
        verdict the_verdict;

        match_found = longest_match(the_regular_expression, follow,
                (follow == string_chars), &match_start, &match_length, &error);
        if (error)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        if (!match_found)
            match_start = strlen(follow);

        section_chars = MALLOC_ARRAY(char, match_start + 1);
        if (section_chars == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        if (match_start > 0)
            memcpy(section_chars, follow, match_start);
        section_chars[match_start] = 0;

        section_value = create_string_value(section_chars);
        free(section_chars);
        if (section_value == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        the_verdict = add_field(result, NULL, section_value);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(section_value, the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        value_remove_reference(section_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        if (!match_found)
            break;

        assert(match_start + match_length <= strlen(follow));
        follow += match_start + match_length;

        if (match_length == 0)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(split_null_match),
                    "In split(), the empty string matched the pattern.");
            value_remove_reference(result, the_jumper);
            return NULL;
          }
      }

    return result;
  }

static value *join_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *array_value;
    const char *joiner_chars;
    size_t joiner_length;
    string_buffer output_buffer;
    size_t element_count;
    verdict the_verdict;
    size_t element_num;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    array_value = value_component_value(all_arguments_value, 0);
    joiner_chars = string_for_argument(all_arguments_value, 1);

    joiner_length = strlen(joiner_chars);

    element_count = array_value_element_count(array_value, "join", the_jumper,
                                              location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    the_verdict = string_buffer_init(&output_buffer, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    for (element_num = 0; element_num < element_count; ++element_num)
      {
        value *string_value;
        const char *string_chars;
        verdict the_verdict;

        if (element_num > 0)
          {
            verdict the_verdict;

            the_verdict = string_buffer_append_array(&output_buffer,
                    joiner_length, joiner_chars);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                free(output_buffer.array);
                return NULL;
              }
          }

        string_value = array_value_element_value(array_value, element_num,
                                                 the_jumper, location);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(string_value == NULL);
            free(output_buffer.array);
            return NULL;
          }

        if (string_value == NULL)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(join_undefined),
                    "In join(), element %lu of the array is undefined.",
                    (unsigned long)element_num);
            free(output_buffer.array);
            return NULL;
          }

        assert(get_value_kind(string_value) == VK_STRING);
        string_chars = string_value_data(string_value);
        assert(string_chars != NULL);

        the_verdict = string_buffer_append_array(&output_buffer,
                strlen(string_chars), string_chars);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            free(output_buffer.array);
            return NULL;
          }
      }

    the_verdict = string_buffer_append(&output_buffer, 0);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        free(output_buffer.array);
        return NULL;
      }

    result = create_string_value(output_buffer.array);
    free(output_buffer.array);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *join_function_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *array_value;
    value *function_value;
    size_t element_count;
    string_buffer output_buffer;
    verdict the_verdict;
    size_t element_num;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    array_value = value_component_value(all_arguments_value, 0);
    function_value = value_component_value(all_arguments_value, 1);

    element_count = array_value_element_count(array_value, "join", the_jumper,
                                              location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    the_verdict = string_buffer_init(&output_buffer, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    for (element_num = 0; element_num < element_count; ++element_num)
      {
        value *string_value;
        const char *string_chars;
        verdict the_verdict;

        if (element_num > 0)
          {
            value *joiner_value;
            const char *joiner_chars;
            verdict the_verdict;

            joiner_value = execute_call_from_arrays(function_value, 0, NULL,
                    NULL, TRUE, the_jumper, location);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(joiner_value == NULL);
                free(output_buffer.array);
                return NULL;
              }

            assert(joiner_value != NULL);

            assert(get_value_kind(joiner_value) == VK_STRING);
            joiner_chars = string_value_data(joiner_value);
            assert(joiner_chars != NULL);

            value_remove_reference(joiner_value, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                jumper_do_abort(the_jumper);
                free(output_buffer.array);
                return NULL;
              }

            the_verdict = string_buffer_append_array(&output_buffer,
                    strlen(joiner_chars), joiner_chars);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                free(output_buffer.array);
                return NULL;
              }
          }

        string_value = array_value_element_value(array_value, element_num,
                                                 the_jumper, location);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(string_value == NULL);
            free(output_buffer.array);
            return NULL;
          }

        if (string_value == NULL)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(join_undefined),
                    "In join(), element %lu of the array is undefined.",
                    (unsigned long)element_num);
            free(output_buffer.array);
            return NULL;
          }

        assert(get_value_kind(string_value) == VK_STRING);
        string_chars = string_value_data(string_value);
        assert(string_chars != NULL);

        the_verdict = string_buffer_append_array(&output_buffer,
                strlen(string_chars), string_chars);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            free(output_buffer.array);
            return NULL;
          }
      }

    if (!(jumper_flowing_forward(the_jumper)))
      {
        free(output_buffer.array);
        return NULL;
      }

    the_verdict = string_buffer_append(&output_buffer, 0);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        free(output_buffer.array);
        return NULL;
      }

    result = create_string_value(output_buffer.array);
    free(output_buffer.array);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *filter_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *regular_expression_value;
    value *array_value;
    regular_expression *the_regular_expression;
    size_t element_count;
    value *result;
    size_t element_num;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    regular_expression_value = value_component_value(all_arguments_value, 0);
    array_value = value_component_value(all_arguments_value, 1);

    assert(get_value_kind(regular_expression_value) == VK_REGULAR_EXPRESSION);
    the_regular_expression =
            regular_expression_value_data(regular_expression_value);
    assert(the_regular_expression != NULL);

    element_count = array_value_element_count(array_value, "filter",
                                              the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    for (element_num = 0; element_num < element_count; ++element_num)
      {
        value *string_value;
        const char *string_chars;
        boolean error;
        boolean does_match;

        string_value = array_value_element_value(array_value, element_num,
                                                 the_jumper, location);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(string_value == NULL);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        if (string_value == NULL)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(filter_undefined),
                    "In filter(), element %lu of the array is undefined.",
                    (unsigned long)element_num);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        assert(get_value_kind(string_value) == VK_STRING);
        string_chars = string_value_data(string_value);
        assert(string_chars != NULL);

        does_match =
                matches(the_regular_expression, string_chars, TRUE, &error);
        if (error)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        if (does_match)
          {
            verdict the_verdict;

            the_verdict = add_field(result, NULL, string_value);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                value_remove_reference(result, the_jumper);
                return NULL;
              }
          }
      }

    return result;
  }

static value *filter_type_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *type_value;
    value *array_value;
    type *the_type;
    size_t element_count;
    value *result;
    size_t element_num;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    type_value = value_component_value(all_arguments_value, 0);
    array_value = value_component_value(all_arguments_value, 1);

    assert(get_value_kind(type_value) == VK_TYPE);
    the_type = type_value_data(type_value);
    assert(the_type != NULL);

    element_count = array_value_element_count(array_value, "filter",
                                              the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    for (element_num = 0; element_num < element_count; ++element_num)
      {
        value *element_value;
        boolean doubt;
        char *why_not;
        boolean in_type;

        element_value = array_value_element_value(array_value, element_num,
                                                  the_jumper, location);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(element_value == NULL);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        if (element_value == NULL)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(filter_undefined),
                    "In filter(), element %lu of the array is undefined.",
                    (unsigned long)element_num);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        assert(type_is_valid(the_type)); /* VERIFICATION NEEDED */
        in_type = value_is_in_type(element_value, the_type, &doubt, &why_not,
                                   location, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        if (doubt)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(filter_doubt),
                    "In filter(), %s was unable to determine whether element "
                    "%lu of the array is in the type because %s.",
                    interpreter_name(), (unsigned long)element_num, why_not);
            free(why_not);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        if (in_type)
          {
            verdict the_verdict;

            the_verdict = add_field(result, NULL, element_value);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                value_remove_reference(result, the_jumper);
                return NULL;
              }
          }
        else
          {
            free(why_not);
          }
      }

    return result;
  }

static value *substitute_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *regular_expression_value;
    regular_expression *the_regular_expression;
    const char *base_chars;
    const char *replacement_chars;
    size_t replacement_char_count;
    string_buffer output_buffer;
    verdict the_verdict;
    const char *follow;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 3);
    regular_expression_value = value_component_value(all_arguments_value, 0);

    assert(get_value_kind(regular_expression_value) == VK_REGULAR_EXPRESSION);
    the_regular_expression =
            regular_expression_value_data(regular_expression_value);
    assert(the_regular_expression != NULL);

    base_chars = string_for_argument(all_arguments_value, 1);

    replacement_chars = string_for_argument(all_arguments_value, 2);

    replacement_char_count = strlen(replacement_chars);

    the_verdict = string_buffer_init(&output_buffer, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    follow = base_chars;

    while (TRUE)
      {
        boolean error;
        size_t match_start;
        size_t match_length;
        boolean match_found;

        match_found = longest_match(the_regular_expression, follow,
                (follow == base_chars), &match_start, &match_length, &error);
        if (error)
          {
            jumper_do_abort(the_jumper);
            free(output_buffer.array);
            return NULL;
          }

        if (!match_found)
            match_start = strlen(follow);

        if (match_start > 0)
          {
            verdict the_verdict;

            the_verdict = string_buffer_append_array(&output_buffer,
                                                     match_start, follow);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                free(output_buffer.array);
                return NULL;
              }
          }

        if (!match_found)
            break;

        assert(match_start + match_length <= strlen(follow));
        follow += match_start + match_length;

        if (match_length == 0)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(substitute_null_match),
                    "In substitute(), the empty string matched the pattern.");
            free(output_buffer.array);
            return NULL;
          }

        if (replacement_char_count > 0)
          {
            verdict the_verdict;

            the_verdict = string_buffer_append_array(&output_buffer,
                    replacement_char_count, replacement_chars);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                free(output_buffer.array);
                return NULL;
              }
          }
      }

    the_verdict = string_buffer_append(&output_buffer, 0);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        free(output_buffer.array);
        return NULL;
      }

    result = create_string_value(output_buffer.array);
    free(output_buffer.array);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *substitute_function_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *regular_expression_value;
    value *function_value;
    regular_expression *the_regular_expression;
    const char *base_chars;
    string_buffer output_buffer;
    verdict the_verdict;
    const char *follow;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 3);
    regular_expression_value = value_component_value(all_arguments_value, 0);
    function_value = value_component_value(all_arguments_value, 2);

    assert(get_value_kind(regular_expression_value) == VK_REGULAR_EXPRESSION);
    the_regular_expression =
            regular_expression_value_data(regular_expression_value);
    assert(the_regular_expression != NULL);

    base_chars = string_for_argument(all_arguments_value, 1);

    the_verdict = string_buffer_init(&output_buffer, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    follow = base_chars;

    while (TRUE)
      {
        boolean error;
        size_t match_start;
        size_t match_length;
        boolean match_found;
        char *to_replace_chars;
        value *to_replace_value;
        value *replacement_value;
        const char *replacement_chars;
        size_t replacement_char_count;

        match_found = longest_match(the_regular_expression, follow,
                (follow == base_chars), &match_start, &match_length, &error);
        if (error)
          {
            jumper_do_abort(the_jumper);
            free(output_buffer.array);
            return NULL;
          }

        if (!match_found)
            match_start = strlen(follow);

        if (match_start > 0)
          {
            verdict the_verdict;

            the_verdict = string_buffer_append_array(&output_buffer,
                                                     match_start, follow);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                free(output_buffer.array);
                return NULL;
              }
          }

        if (!match_found)
            break;

        assert(match_start <= strlen(follow));
        follow += match_start;

        if (match_length == 0)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(substitute_null_match),
                    "In substitute(), the empty string matched the pattern.");
            free(output_buffer.array);
            return NULL;
          }

        to_replace_chars = MALLOC_ARRAY(char, match_length + 1);
        if (to_replace_chars == NULL)
          {
            jumper_do_abort(the_jumper);
            free(output_buffer.array);
            return NULL;
          }

        if (match_length > 0)
            memcpy(to_replace_chars, follow, match_length);
        to_replace_chars[match_length] = 0;

        to_replace_value = create_string_value(to_replace_chars);
        free(to_replace_chars);
        if (to_replace_value == NULL)
          {
            jumper_do_abort(the_jumper);
            free(output_buffer.array);
            return NULL;
          }

        assert(match_length <= strlen(follow));
        follow += match_length;

        replacement_value = execute_call_from_arrays(function_value, 1, NULL,
                &to_replace_value, TRUE, the_jumper, location);
        value_remove_reference(to_replace_value, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            if (replacement_value != NULL)
                value_remove_reference(replacement_value, the_jumper);
            free(output_buffer.array);
            return NULL;
          }

        assert(replacement_value != NULL);

        assert(get_value_kind(replacement_value) == VK_STRING);
        replacement_chars = string_value_data(replacement_value);
        assert(replacement_chars != NULL);

        replacement_char_count = strlen(replacement_chars);

        if (replacement_char_count > 0)
          {
            verdict the_verdict;

            the_verdict = string_buffer_append_array(&output_buffer,
                    replacement_char_count, replacement_chars);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                jumper_do_abort(the_jumper);
                free(output_buffer.array);
                return NULL;
              }
          }
      }

    the_verdict = string_buffer_append(&output_buffer, 0);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        free(output_buffer.array);
        return NULL;
      }

    result = create_string_value(output_buffer.array);
    free(output_buffer.array);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *pattern_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *regular_expression_value;
    regular_expression *the_regular_expression;
    const char *pattern_chars;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    regular_expression_value = value_component_value(all_arguments_value, 0);

    assert(get_value_kind(regular_expression_value) == VK_REGULAR_EXPRESSION);
    the_regular_expression =
            regular_expression_value_data(regular_expression_value);
    assert(the_regular_expression != NULL);

    pattern_chars = regular_expression_pattern(the_regular_expression);
    assert(pattern_chars != NULL);

    result = create_string_value(pattern_chars);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *parse_regular_expression_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *string_chars;
    regular_expression_error error;
    regular_expression *the_regular_expression;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    string_chars = string_for_argument(all_arguments_value, 0);

    the_regular_expression = create_regular_expression_from_pattern(
            string_chars, strlen(string_chars), &error);
    if (the_regular_expression == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(parse_regular_expression_bad_pattern),
                "In parse_regular_expression(): %s",
                string_for_regular_expression_error(error));
        return NULL;
      }

    result = create_regular_expression_value(the_regular_expression);
    regular_expression_remove_reference(the_regular_expression);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *exact_string_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *string_chars;
    regular_expression *the_regular_expression;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    string_chars = string_for_argument(all_arguments_value, 0);

    the_regular_expression =
            create_exact_string_regular_expression(string_chars);
    if (the_regular_expression == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_regular_expression_value(the_regular_expression);
    regular_expression_remove_reference(the_regular_expression);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *exact_character_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *character_value;
    const char *character_chars;
    regular_expression *the_regular_expression;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    character_value = value_component_value(all_arguments_value, 0);

    assert(get_value_kind(character_value) == VK_CHARACTER);
    character_chars = character_value_data(character_value);
    assert(character_chars != NULL);

    the_regular_expression =
            create_exact_string_regular_expression(character_chars);
    if (the_regular_expression == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_regular_expression_value(the_regular_expression);
    regular_expression_remove_reference(the_regular_expression);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *character_set_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *array_value;
    string_aa the_set;
    size_t element_count;
    verdict the_verdict;
    size_t element_num;
    regular_expression *the_regular_expression;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    array_value = value_component_value(all_arguments_value, 0);

    element_count = array_value_element_count(array_value, "character_set",
                                              the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    the_verdict = string_aa_init(&the_set, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    for (element_num = 0; element_num < element_count; ++element_num)
      {
        value *element_value;
        const char *character_chars;
        verdict the_verdict;

        element_value = array_value_element_value(array_value, element_num,
                                                  the_jumper, location);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            assert(element_value == NULL);
            free(the_set.array);
            return NULL;
          }

        if (element_value == NULL)
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(character_set_undefined),
                    "In character_set(), element %lu of the array is "
                    "undefined.", (unsigned long)element_num);
            free(the_set.array);
            return NULL;
          }

        assert(get_value_kind(element_value) == VK_CHARACTER);
        character_chars = character_value_data(element_value);
        assert(character_chars != NULL);

        the_verdict = string_aa_append(&the_set, character_chars);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            free(the_set.array);
            return NULL;
          }
      }

    the_regular_expression = create_character_set_regular_expression(
            the_set.array, the_set.element_count);
    free(the_set.array);
    if (the_regular_expression == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_regular_expression_value(the_regular_expression);
    regular_expression_remove_reference(the_regular_expression);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *character_range_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *lower_value;
    value *upper_value;
    const char *lower_chars;
    const char *upper_chars;
    regular_expression *the_regular_expression;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    lower_value = value_component_value(all_arguments_value, 0);
    upper_value = value_component_value(all_arguments_value, 1);

    assert(get_value_kind(lower_value) == VK_CHARACTER);
    lower_chars = character_value_data(lower_value);
    assert(lower_chars != NULL);

    assert(get_value_kind(upper_value) == VK_CHARACTER);
    upper_chars = character_value_data(upper_value);
    assert(upper_chars != NULL);

    the_regular_expression = create_character_range_regular_expression(
            lower_chars, upper_chars);
    if (the_regular_expression == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_regular_expression_value(the_regular_expression);
    regular_expression_remove_reference(the_regular_expression);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *concatenate_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *left_value;
    value *right_value;
    regular_expression *left_regular_expression;
    regular_expression *right_regular_expression;
    regular_expression *result_regular_expression;
    value *result_value;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    left_value = value_component_value(all_arguments_value, 0);
    right_value = value_component_value(all_arguments_value, 1);

    assert(get_value_kind(left_value) == VK_REGULAR_EXPRESSION);
    left_regular_expression = regular_expression_value_data(left_value);
    assert(left_regular_expression != NULL);

    assert(get_value_kind(right_value) == VK_REGULAR_EXPRESSION);
    right_regular_expression = regular_expression_value_data(right_value);
    assert(right_regular_expression != NULL);

    result_regular_expression = create_concatenation_regular_expression(
            left_regular_expression, right_regular_expression);
    if (result_regular_expression == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result_value = create_regular_expression_value(result_regular_expression);
    regular_expression_remove_reference(result_regular_expression);
    if (result_value == NULL)
        jumper_do_abort(the_jumper);
    return result_value;
  }

static value *or_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *left_value;
    value *right_value;
    regular_expression *left_regular_expression;
    regular_expression *right_regular_expression;
    regular_expression *result_regular_expression;
    value *result_value;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    left_value = value_component_value(all_arguments_value, 0);
    right_value = value_component_value(all_arguments_value, 1);

    assert(get_value_kind(left_value) == VK_REGULAR_EXPRESSION);
    left_regular_expression = regular_expression_value_data(left_value);
    assert(left_regular_expression != NULL);

    assert(get_value_kind(right_value) == VK_REGULAR_EXPRESSION);
    right_regular_expression = regular_expression_value_data(right_value);
    assert(right_regular_expression != NULL);

    result_regular_expression = create_or_regular_expression(
            left_regular_expression, right_regular_expression);
    if (result_regular_expression == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result_value = create_regular_expression_value(result_regular_expression);
    regular_expression_remove_reference(result_regular_expression);
    if (result_value == NULL)
        jumper_do_abort(the_jumper);
    return result_value;
  }

static value *repeat_zero_or_more_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *base_value;
    regular_expression *base_regular_expression;
    regular_expression *result_regular_expression;
    value *result_value;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    base_value = value_component_value(all_arguments_value, 0);

    assert(get_value_kind(base_value) == VK_REGULAR_EXPRESSION);
    base_regular_expression = regular_expression_value_data(base_value);
    assert(base_regular_expression != NULL);

    result_regular_expression = create_repeat_zero_or_more_regular_expression(
            base_regular_expression);
    if (result_regular_expression == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result_value = create_regular_expression_value(result_regular_expression);
    regular_expression_remove_reference(result_regular_expression);
    if (result_value == NULL)
        jumper_do_abort(the_jumper);
    return result_value;
  }

static value *repeat_one_or_more_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *base_value;
    regular_expression *base_regular_expression;
    regular_expression *result_regular_expression;
    value *result_value;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    base_value = value_component_value(all_arguments_value, 0);

    assert(get_value_kind(base_value) == VK_REGULAR_EXPRESSION);
    base_regular_expression = regular_expression_value_data(base_value);
    assert(base_regular_expression != NULL);

    result_regular_expression = create_repeat_one_or_more_regular_expression(
            base_regular_expression);
    if (result_regular_expression == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result_value = create_regular_expression_value(result_regular_expression);
    regular_expression_remove_reference(result_regular_expression);
    if (result_value == NULL)
        jumper_do_abort(the_jumper);
    return result_value;
  }

static value *re_follower_init_hook_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    object *the_object;
    regular_expression *the_regular_expression;
    re_follower *the_follower;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    the_object = object_for_argument(all_arguments_value, 0);
    the_regular_expression =
            regular_expression_for_argument(all_arguments_value, 1);

    assert(!(object_is_closed(the_object))); /* VERIFICATION NEEDED */
    assert(object_hook(the_object) == NULL);

    the_follower = regular_expression_create_follower(the_regular_expression);
    if (the_follower == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    assert(!(object_is_closed(the_object))); /* VERIFICATION NEEDED */
    object_set_hook(the_object, the_follower);
    object_set_hook_cleaner(the_object, &re_follower_cleaner);

    return NULL;
  }

static value *re_follower_transit_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    object *the_object;
    value *character_value;
    re_follower *the_follower;
    verdict the_verdict;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    the_object = object_for_argument(all_arguments_value, 0);
    character_value = value_component_value(all_arguments_value, 1);

    assert(!(object_is_closed(the_object))); /* VERIFICATION NEEDED */
    the_follower = (re_follower *)(object_hook(the_object));
    assert(the_follower != NULL);

    the_verdict = re_follower_transit(the_follower,
                                      character_value_data(character_value));
    if (the_verdict != MISSION_ACCOMPLISHED)
        jumper_do_abort(the_jumper);
    return NULL;
  }

static value *re_follower_end_transit_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    object *the_object;
    re_follower *the_follower;
    verdict the_verdict;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    the_object = object_for_argument(all_arguments_value, 0);

    assert(!(object_is_closed(the_object))); /* VERIFICATION NEEDED */
    the_follower = (re_follower *)(object_hook(the_object));
    assert(the_follower != NULL);

    the_verdict = re_follower_end_transit(the_follower);
    if (the_verdict != MISSION_ACCOMPLISHED)
        jumper_do_abort(the_jumper);
    return NULL;
  }

static value *re_follower_is_in_accepting_state_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    object *the_object;
    re_follower *the_follower;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    the_object = object_for_argument(all_arguments_value, 0);

    assert(!(object_is_closed(the_object))); /* VERIFICATION NEEDED */
    the_follower = (re_follower *)(object_hook(the_object));
    assert(the_follower != NULL);

    if (re_follower_is_in_accepting_state(the_follower))
        result = create_true_value();
    else
        result = create_false_value();

    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *re_follower_more_possible_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    object *the_object;
    re_follower *the_follower;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    the_object = object_for_argument(all_arguments_value, 0);

    assert(!(object_is_closed(the_object))); /* VERIFICATION NEEDED */
    the_follower = (re_follower *)(object_hook(the_object));
    assert(the_follower != NULL);

    if (re_follower_more_possible(the_follower))
        result = create_true_value();
    else
        result = create_false_value();

    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *length_string_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *string_chars;
    o_integer the_oi;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    string_chars = string_for_argument(all_arguments_value, 0);

    oi_create_from_size_t(the_oi, strlen(string_chars));
    if (oi_out_of_memory(the_oi))
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_integer_value(the_oi);
    oi_remove_reference(the_oi);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *length_array_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *array_value;
    o_integer length_oi;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    array_value = value_component_value(all_arguments_value, 0);

    switch (get_value_kind(array_value))
      {
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            oi_create_from_size_t(length_oi,
                                  value_component_count(array_value));
            break;
          }
        case VK_MAP:
          {
            length_oi = map_value_length(array_value, "length", the_jumper,
                                         location);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(oi_out_of_memory(length_oi));
                return NULL;
              }
            break;
          }
        default:
          {
            assert(FALSE);
            length_oi = oi_null;
          }
      }

    if (oi_out_of_memory(length_oi))
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_integer_value(length_oi);
    oi_remove_reference(length_oi);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *tag_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *element_value;
    const char *label;
    value *result;
    verdict the_verdict;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(value_component_count(all_arguments_value) == 2);
    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    element_value = value_component_value(all_arguments_value, 0);
    label = string_for_argument(all_arguments_value, 1);

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    the_verdict = add_field(result, label, element_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

static value *call_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *base_value;
    value *call_with_value;
    semi_labeled_value_list *actuals;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    base_value = value_component_value(all_arguments_value, 0);
    call_with_value = value_component_value(all_arguments_value, 1);

    actuals = create_semi_labeled_value_list();
    if (actuals == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    switch (get_value_kind(call_with_value))
      {
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            size_t component_count;
            size_t component_num;

            component_count = value_component_count(call_with_value);
            for (component_num = 0; component_num < component_count;
                 ++component_num)
              {
                verdict the_verdict;

                the_verdict = append_value_to_semi_labeled_value_list(actuals,
                        value_component_label(call_with_value, component_num),
                        value_component_value(call_with_value, component_num));
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    delete_semi_labeled_value_list(actuals, the_jumper);
                    return NULL;
                  }
              }

            break;
          }
        case VK_MAP:
          {
            size_t element_count;
            size_t element_num;

            element_count = array_value_element_count(call_with_value, "call",
                                                      the_jumper, location);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                delete_semi_labeled_value_list(actuals, the_jumper);
                return NULL;
              }

            for (element_num = 0; element_num < element_count; ++element_num)
              {
                value *element_value;
                verdict the_verdict;

                element_value = array_value_element_value(call_with_value,
                        element_num, the_jumper, location);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    assert(element_value == NULL);
                    delete_semi_labeled_value_list(actuals, the_jumper);
                    return NULL;
                  }

                if (element_value == NULL)
                  {
                    location_exception(the_jumper, location,
                            EXCEPTION_TAG(call_undefined),
                            "In call(), element %lu of the array is "
                            "undefined.", (unsigned long)element_num);
                    delete_semi_labeled_value_list(actuals, the_jumper);
                    return NULL;
                  }

                the_verdict = append_value_to_semi_labeled_value_list(actuals,
                        NULL, element_value);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    delete_semi_labeled_value_list(actuals, the_jumper);
                    return NULL;
                  }
              }

            break;
          }
        default:
          {
            assert(FALSE);
          }
      }

    result = execute_call_from_values(base_value, actuals,
            nearest_routine_expects_return_value(the_context), the_jumper,
            location);
    delete_semi_labeled_value_list(actuals, the_jumper);
    if ((!(jumper_flowing_forward(the_jumper))) && (result != NULL))
      {
        value_remove_reference(result, the_jumper);
        return NULL;
      }
    return result;
  }

static value *internal_call_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    return call_handler_function(all_arguments_value, the_context, the_jumper,
                                 jumper_call_stack_back_site(the_jumper, 1));
  }

static value *return_value_expected_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *return_target_value;
    jump_target *the_target;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    return_target_value = value_component_value(all_arguments_value, 0);

    assert(return_target_value != NULL);
    assert(get_value_kind(return_target_value) == VK_JUMP_TARGET);
    the_target = jump_target_value_data(return_target_value);
    assert(the_target != NULL);

    assert(!(jump_target_scope_exited(the_target))); /* VERIFICATION NEEDED */
    if (nearest_routine_expects_return_value(jump_target_context(the_target)))
        result = create_true_value();
    else
        result = create_false_value();

    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *set_fp_stream_to_standard_input_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *object_value;
    object *the_object;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    object_value = value_component_value(all_arguments_value, 0);

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    hook_file_pointer_to_object(the_object, stdin, UTF_8, "standard input",
                                the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    value_add_reference(object_value);
    return object_value;
  }

static value *set_fp_stream_to_standard_output_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *object_value;
    object *the_object;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    object_value = value_component_value(all_arguments_value, 0);

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    hook_file_pointer_to_object(the_object, stdout, UTF_8, "standard output",
                                the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    value_add_reference(object_value);
    return object_value;
  }

static value *set_fp_stream_to_standard_error_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *object_value;
    object *the_object;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    object_value = value_component_value(all_arguments_value, 0);

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    hook_file_pointer_to_object(the_object, stderr, UTF_8, "standard error",
                                the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    value_add_reference(object_value);
    return object_value;
  }

static value *set_fp_stream_to_named_input_text_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    return set_fp_stream_to_named_text_file_handler_function(
            all_arguments_value, the_context, the_jumper, location, TRUE,
            FALSE);
  }

static value *set_fp_stream_to_named_output_text_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    return set_fp_stream_to_named_text_file_handler_function(
            all_arguments_value, the_context, the_jumper, location, FALSE,
            TRUE);
  }

static value *set_fp_stream_to_named_input_output_text_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    return set_fp_stream_to_named_text_file_handler_function(
            all_arguments_value, the_context, the_jumper, location, TRUE,
            TRUE);
  }

static value *set_fp_stream_to_named_text_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location, boolean input, boolean output)
  {
    value *object_value;
    value *format_value;
    value *io_error_value;
    object *the_object;
    const char *file_name_chars;
    utf_choice utf_format;
    lepton_key_instance *io_error_key;
    boolean append;
    FILE *fp;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) >= 4);
    object_value = value_component_value(all_arguments_value, 0);
    format_value = value_component_value(all_arguments_value, 2);
    io_error_value = value_component_value(all_arguments_value, 3);

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    file_name_chars = string_for_argument(all_arguments_value, 1);

    utf_format = utf_choice_from_value(format_value);

    assert(io_error_value != NULL);
    assert(get_value_kind(io_error_value) == VK_LEPTON_KEY);
    io_error_key = value_lepton_key(io_error_value);
    assert(io_error_key != NULL);

    if (!input)
      {
        assert(value_component_count(all_arguments_value) == 5);
        append = boolean_from_value(
                value_component_value(all_arguments_value, 4));
      }

    fp = fopen(file_name_chars,
            ((utf_format == UTF_8) ?
             (input ? (output ? "r+" : "r") : (append ? "a" : "w")) :
             (input ? (output ? "rb+" : "rb") : (append ? "ab" : "wb"))));
    if (fp == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(file_open_failed),
                "Failed trying to open file \"%s\" for text %s: %s.",
                file_name_chars,
                (input ? (output ? "reading and writing" : "reading") :
                         "writing"), strerror(errno));
        return NULL;
      }

    hook_file_pointer_to_object_for_named_file(the_object, fp, utf_format,
                                               file_name_chars, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    if (input)
      {
        set_fp_object_eof_and_error_flags_if_needed(the_object, fp,
                                                    io_error_key, the_jumper);
      }
    else
      {
        set_fp_object_error_flag_if_needed(the_object, fp, io_error_key,
                                           the_jumper);
      }
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    value_add_reference(object_value);
    return object_value;
  }

static value *set_fp_stream_to_named_input_bit_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    return set_fp_stream_to_named_bit_file_handler_function(
            all_arguments_value, the_context, the_jumper, location, TRUE,
            FALSE);
  }

static value *set_fp_stream_to_named_output_bit_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    return set_fp_stream_to_named_bit_file_handler_function(
            all_arguments_value, the_context, the_jumper, location, FALSE,
            TRUE);
  }

static value *set_fp_stream_to_named_input_output_bit_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    return set_fp_stream_to_named_bit_file_handler_function(
            all_arguments_value, the_context, the_jumper, location, TRUE,
            TRUE);
  }

static value *set_fp_stream_to_named_bit_file_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location, boolean input, boolean output)
  {
    value *object_value;
    value *which_endian_value;
    value *io_error_value;
    object *the_object;
    const char *file_name_chars;
    utf_choice utf_format;
    lepton_key_instance *io_error_key;
    FILE *fp;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 4);
    object_value = value_component_value(all_arguments_value, 0);
    which_endian_value = value_component_value(all_arguments_value, 2);
    io_error_value = value_component_value(all_arguments_value, 3);

    assert(object_value != NULL);
    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    file_name_chars = string_for_argument(all_arguments_value, 1);

    utf_format = utf_choice_from_endian_value(which_endian_value);
    assert((utf_format == UTF_32_LE) || (utf_format == UTF_32_BE));

    assert(io_error_value != NULL);
    assert(get_value_kind(io_error_value) == VK_LEPTON_KEY);
    io_error_key = value_lepton_key(io_error_value);
    assert(io_error_key != NULL);

    fp = fopen(file_name_chars, (input ? (output ? "rb+" : "rb") : "wb"));
    if (fp == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(file_open_failed),
                "Failed trying to open file \"%s\" for bit %s: %s.",
                file_name_chars,
                (input ? (output ? "reading and writing" : "reading") :
                         "writing"), strerror(errno));
        return NULL;
      }

    hook_file_pointer_to_object_for_named_file(the_object, fp, utf_format,
                                               file_name_chars, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    if (input)
      {
        set_fp_object_eof_and_error_flags_if_needed(the_object, fp,
                                                    io_error_key, the_jumper);
      }
    else
      {
        set_fp_object_error_flag_if_needed(the_object, fp, io_error_key,
                                           the_jumper);
      }
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    value_add_reference(object_value);
    return object_value;
  }

static value *file_exists_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *name_chars;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    name_chars = string_for_argument(all_arguments_value, 0);

    if (file_exists(name_chars))
        result = create_true_value();
    else
        result = create_false_value();

    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *directory_exists_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *name_chars;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    name_chars = string_for_argument(all_arguments_value, 0);

    if (directory_exists(name_chars))
        result = create_true_value();
    else
        result = create_false_value();

    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *directory_contents_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *name_chars;
    directory_read_error_handler_data handler_data;
    char **contents_array;
    value *result;
    size_t element_num;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    name_chars = string_for_argument(all_arguments_value, 0);

    handler_data.jumper = the_jumper;
    handler_data.location = location;
    contents_array = directory_contents(name_chars,
            &directory_read_error_handler, &handler_data);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(contents_array == NULL);
        return NULL;
      }
    if (contents_array == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        element_num = 0;
        while (contents_array[element_num] != NULL)
          {
            free(contents_array[element_num]);
            ++element_num;
          }
        free(contents_array);
        return NULL;
      }

    element_num = 0;
    while (TRUE)
      {
        char *element_name;
        value *element_value;
        verdict the_verdict;

        element_name = contents_array[element_num];
        if (element_name == NULL)
            break;

        ++element_num;

        element_value = create_string_value(element_name);
        free(element_name);
        if (element_value == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            while (contents_array[element_num] != NULL)
              {
                free(contents_array[element_num]);
                ++element_num;
              }
            free(contents_array);
            return NULL;
          }

        the_verdict = add_field(result, NULL, element_value);
        value_remove_reference(element_value, the_jumper);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            while (contents_array[element_num] != NULL)
              {
                free(contents_array[element_num]);
                ++element_num;
              }
            free(contents_array);
            return NULL;
          }
      }

    free(contents_array);

    return result;
  }

static value *remove_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *file_name_chars;
    int return_code;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    file_name_chars = string_for_argument(all_arguments_value, 0);

    return_code = remove(file_name_chars);
    if (return_code != 0)
      {
        location_exception(the_jumper, location, EXCEPTION_TAG(remove_failed),
                           "Failed trying to remove \"%s\".", file_name_chars);
      }

    return NULL;
  }

static value *rename_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *old_name_chars;
    const char *new_name_chars;
    int return_code;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    old_name_chars = string_for_argument(all_arguments_value, 0);
    new_name_chars = string_for_argument(all_arguments_value, 1);

    return_code = rename(old_name_chars, new_name_chars);
    if (return_code != 0)
      {
        location_exception(the_jumper, location, EXCEPTION_TAG(rename_failed),
                "Failed trying to rename \"%s\" to \"%s\".", old_name_chars,
                new_name_chars);
      }

    return NULL;
  }

static value *sprint_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    string_buffer output_buffer;
    verdict the_verdict;
    size_t component_count;
    size_t component_num;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);

    the_verdict = string_buffer_init(&output_buffer, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    component_count = value_component_count(all_arguments_value);
    for (component_num = 0; component_num < component_count; ++component_num)
      {
        value *this_value;
        verdict the_verdict;

        this_value = value_component_value(all_arguments_value, component_num);
        assert(this_value != NULL);

        if (get_value_kind(this_value) == VK_STRING)
          {
            const char *data;

            data = string_value_data(this_value);
            assert(data != NULL);

            the_verdict = string_buffer_append_array(&output_buffer,
                                                     strlen(data), data);
            if (the_verdict != MISSION_ACCOMPLISHED)
                jumper_do_abort(the_jumper);
          }
        else if (get_value_kind(this_value) == VK_CHARACTER)
          {
            const char *data;

            data = character_value_data(this_value);
            assert(data != NULL);

            the_verdict = string_buffer_append_array(&output_buffer,
                                                     strlen(data), data);
            if (the_verdict != MISSION_ACCOMPLISHED)
                jumper_do_abort(the_jumper);
          }
        else
          {
            string_printer_data data;

            data.output_buffer = &output_buffer;
            data.cp_data.verdict = MISSION_ACCOMPLISHED;
            data.cp_data.jumper = the_jumper;
            data.cp_data.location = location;
            string_print_value(this_value, &string_printer, &data);
            the_verdict = data.cp_data.verdict;
          }

        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            free(output_buffer.array);
            return NULL;
          }
      }

    the_verdict = string_buffer_append(&output_buffer, 0);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        free(output_buffer.array);
        return NULL;
      }

    result = create_string_value(output_buffer.array);
    free(output_buffer.array);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *sprintf_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    string_buffer output_buffer;
    verdict the_verdict;
    string_printer_data data;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) >= 1);

    the_verdict = string_buffer_init(&output_buffer, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    data.output_buffer = &output_buffer;
    data.cp_data.verdict = MISSION_ACCOMPLISHED;
    data.cp_data.jumper = the_jumper;
    data.cp_data.location = location;
    overload_printf(all_arguments_value, 0, &string_printer, &data,
                    &(data.cp_data), &string_print_value);
    if (data.cp_data.verdict != MISSION_ACCOMPLISHED)
      {
        free(output_buffer.array);
        return NULL;
      }

    the_verdict = string_buffer_append(&output_buffer, 0);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        free(output_buffer.array);
        return NULL;
      }

    result = create_string_value(output_buffer.array);
    free(output_buffer.array);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *system_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *capture_standard_out_value;
    value *capture_standard_error_value;
    const char *command_chars;
    boolean do_capture_standard_out;
    boolean do_capture_standard_error;
    o_integer return_code_oi;
    value *standard_out_value;
    value *standard_error_value;
    value *return_code_value;
    value *result_value;
    verdict the_verdict;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 3);
    capture_standard_out_value = value_component_value(all_arguments_value, 1);
    capture_standard_error_value =
            value_component_value(all_arguments_value, 2);

    command_chars = string_for_argument(all_arguments_value, 0);

    do_capture_standard_out = boolean_from_value(capture_standard_out_value);
    do_capture_standard_error =
            boolean_from_value(capture_standard_error_value);

    fflush(stdout);
    fflush(stderr);

    if ((!do_capture_standard_out) && (!do_capture_standard_error))
      {
        int result_int;

        result_int = system(command_chars);

        oi_create_from_size_t(return_code_oi,
                              ((result_int < 0) ? -result_int : result_int));
        if (oi_out_of_memory(return_code_oi))
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        if (result_int < 0)
          {
            o_integer negative_oi;

            oi_negate(negative_oi, return_code_oi);
            oi_remove_reference(return_code_oi);
            return_code_oi = negative_oi;
            if (oi_out_of_memory(return_code_oi))
              {
                jumper_do_abort(the_jumper);
                return NULL;
              }
          }

        standard_out_value = create_string_value("");
        if (standard_out_value == NULL)
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(return_code_oi);
            return NULL;
          }

        value_add_reference(standard_out_value);
        standard_error_value = standard_out_value;
      }
    else
      {
        char *standard_out_string;
        char *standard_error_string;
        boolean error;
        int result_int;

        result_int = redirected_system(command_chars,
                (do_capture_standard_out ? &standard_out_string : NULL),
                (do_capture_standard_error ? &standard_error_string : NULL),
                &error);
        if (error)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        if (do_capture_standard_out)
            assert(standard_out_string != NULL);
        if (do_capture_standard_error)
            assert(standard_error_string != NULL);

        oi_create_from_size_t(return_code_oi,
                              ((result_int < 0) ? -result_int : result_int));
        if (oi_out_of_memory(return_code_oi))
          {
            jumper_do_abort(the_jumper);
            if (do_capture_standard_out)
                free(standard_out_string);
            if (do_capture_standard_error)
                free(standard_error_string);
            return NULL;
          }

        if (result_int < 0)
          {
            o_integer negative_oi;

            oi_negate(negative_oi, return_code_oi);
            oi_remove_reference(return_code_oi);
            return_code_oi = negative_oi;
            if (oi_out_of_memory(return_code_oi))
              {
                jumper_do_abort(the_jumper);
                if (do_capture_standard_out)
                    free(standard_out_string);
                if (do_capture_standard_error)
                    free(standard_error_string);
                return NULL;
              }
          }

        if (do_capture_standard_out)
            assert(standard_out_string != NULL);
        else
            standard_out_string = "";

        standard_out_value = create_string_value(standard_out_string);
        if (do_capture_standard_out)
            free(standard_out_string);
        if (standard_out_value == NULL)
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(return_code_oi);
            if (do_capture_standard_error)
                free(standard_error_string);
            return NULL;
          }

        if (do_capture_standard_error)
            assert(standard_error_string != NULL);
        else
            standard_error_string = "";

        standard_error_value = create_string_value(standard_error_string);
        if (do_capture_standard_error)
            free(standard_error_string);
        if (standard_error_value == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(standard_out_value, the_jumper);
            oi_remove_reference(return_code_oi);
            return NULL;
          }
      }

    if (!(nearest_routine_expects_return_value(the_context)))
      {
        oi_remove_reference(return_code_oi);
        value_remove_reference(standard_out_value, the_jumper);
        value_remove_reference(standard_error_value, the_jumper);
        return NULL;
      }

    return_code_value = create_integer_value(return_code_oi);
    oi_remove_reference(return_code_oi);
    if (return_code_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(standard_out_value, the_jumper);
        value_remove_reference(standard_error_value, the_jumper);
        return NULL;
      }

    result_value = create_semi_labeled_value_list_value();
    if (result_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(return_code_value, the_jumper);
        value_remove_reference(standard_out_value, the_jumper);
        value_remove_reference(standard_error_value, the_jumper);
        return NULL;
      }

    the_verdict = add_field(result_value, "return_code", return_code_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result_value, the_jumper);
        value_remove_reference(return_code_value, the_jumper);
        value_remove_reference(standard_out_value, the_jumper);
        value_remove_reference(standard_error_value, the_jumper);
        return NULL;
      }

    the_verdict = add_field(result_value, "standard_out", standard_out_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result_value, the_jumper);
        value_remove_reference(return_code_value, the_jumper);
        value_remove_reference(standard_out_value, the_jumper);
        value_remove_reference(standard_error_value, the_jumper);
        return NULL;
      }

    the_verdict =
            add_field(result_value, "standard_error", standard_error_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result_value, the_jumper);
        value_remove_reference(return_code_value, the_jumper);
        value_remove_reference(standard_out_value, the_jumper);
        value_remove_reference(standard_error_value, the_jumper);
        return NULL;
      }

    value_remove_reference(return_code_value, the_jumper);
    value_remove_reference(standard_out_value, the_jumper);
    value_remove_reference(standard_error_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(result_value, the_jumper);
        return NULL;
      }

    return result_value;
  }

static value *assert_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *boolean_value;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    boolean_value = value_component_value(all_arguments_value, 0);

    if (get_value_kind(boolean_value) == VK_TRUE)
        return NULL;

    assert(get_value_kind(boolean_value) == VK_FALSE);
    location_exception(the_jumper, location, EXCEPTION_TAG(assertion_failure),
                       "Assertion failed.");

    return NULL;
  }

static value *why_not_in_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *base_value;
    type *the_type;
    boolean doubt;
    char *why_not;
    boolean in_type;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    base_value = value_component_value(all_arguments_value, 0);
    the_type = type_for_argument(all_arguments_value, 1);

    assert(type_is_valid(the_type)); /* VERIFICATION NEEDED */
    in_type = value_is_in_type(base_value, the_type, &doubt, &why_not,
                               location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return NULL;

    if (!doubt && in_type)
      {
        result = create_string_value("it is");
        if (result == NULL)
            jumper_do_abort(the_jumper);
        return result;
      }

    assert(why_not != NULL);

    result = create_string_value(why_not);
    free(why_not);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static value *initialize_context_switching_lock_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    set_lock_instance_is_context_switching(
            lock_value_data(value_component_value(all_arguments_value, 0)));

    return NULL;
  }

static value *get_time_and_date_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *months_value;
    value *days_value;
    value *is_local_value;
    unsigned long microseconds_unsigned_long;
    time_t the_time_t;
    boolean is_local_boolean;
    struct tm *the_tm;
    value *result;
    o_integer year_oi;
    value *year_value;
    verdict the_verdict;
    value *month_value;
    o_integer day_of_month_oi;
    value *day_of_month_value;
    value *day_of_week_value;
    o_integer hours_oi;
    value *hours_value;
    o_integer minutes_oi;
    value *minutes_value;
    o_integer seconds_oi;
    value *seconds_value;
    boolean know_if_daylight_savings;
    boolean is_daylight_savings;
    boolean know_seconds_ahead_of_utc;
    long seconds_ahead_of_utc;
    const char *zone_string;
    value *zone_value;
    value *is_daylight_savings_value;
    value *seconds_ahead_of_utc_value;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 3);
    months_value = value_component_value(all_arguments_value, 0);
    days_value = value_component_value(all_arguments_value, 1);
    is_local_value = value_component_value(all_arguments_value, 2);

    the_time_t = get_time_with_microseconds(&microseconds_unsigned_long);
    assert(microseconds_unsigned_long < 1000000);

    is_local_boolean = boolean_from_value(is_local_value);

    if (is_local_boolean)
        the_tm = localtime(&the_time_t);
    else
        the_tm = gmtime(&the_time_t);
    assert(the_tm != NULL);

    result = create_semi_labeled_value_list_value();
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    oi_create_from_long_int(year_oi, the_tm->tm_year + 1900);
    if (oi_out_of_memory(year_oi))
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    year_value = create_integer_value(year_oi);
    oi_remove_reference(year_oi);
    if (year_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    the_verdict = add_field(result, "year", year_value);
    value_remove_reference(year_value, the_jumper);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    assert(months_value != NULL);
    assert(get_value_kind(months_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(months_value) == 12);
    assert((the_tm->tm_mon >= 0) && (the_tm->tm_mon < 12));
    month_value = value_component_value(months_value, the_tm->tm_mon);

    assert(month_value != NULL);
    assert(get_value_kind(month_value) == VK_QUARK);

    the_verdict = add_field(result, "month", month_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    oi_create_from_long_int(day_of_month_oi, the_tm->tm_mday);
    if (oi_out_of_memory(day_of_month_oi))
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    day_of_month_value = create_integer_value(day_of_month_oi);
    oi_remove_reference(day_of_month_oi);
    if (day_of_month_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    the_verdict = add_field(result, "day_of_month", day_of_month_value);
    value_remove_reference(day_of_month_value, the_jumper);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    assert(days_value != NULL);
    assert(get_value_kind(days_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(days_value) == 7);
    assert((the_tm->tm_wday >= 0) && (the_tm->tm_wday < 7));
    day_of_week_value =
            value_component_value(days_value, (the_tm->tm_wday + 6) % 7);

    assert(day_of_week_value != NULL);
    assert(get_value_kind(day_of_week_value) == VK_QUARK);

    the_verdict = add_field(result, "day_of_week", day_of_week_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    oi_create_from_long_int(hours_oi, the_tm->tm_hour);
    if (oi_out_of_memory(hours_oi))
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    hours_value = create_integer_value(hours_oi);
    oi_remove_reference(hours_oi);
    if (hours_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    the_verdict = add_field(result, "hours", hours_value);
    value_remove_reference(hours_value, the_jumper);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    oi_create_from_long_int(minutes_oi, the_tm->tm_min);
    if (oi_out_of_memory(minutes_oi))
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    minutes_value = create_integer_value(minutes_oi);
    oi_remove_reference(minutes_oi);
    if (minutes_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    the_verdict = add_field(result, "minutes", minutes_value);
    value_remove_reference(minutes_value, the_jumper);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    oi_create_from_long_int(seconds_oi, the_tm->tm_sec);
    if (oi_out_of_memory(seconds_oi))
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    if (microseconds_unsigned_long == 0)
      {
        seconds_value = create_integer_value(seconds_oi);
        oi_remove_reference(seconds_oi);
      }
    else
      {
        o_integer million_oi;
        o_integer seconds_million;
        o_integer microseconds_oi;
        o_integer microsecond_sum;
        rational *seconds_rational;

        oi_create_from_long_int(million_oi, 1000000);
        if (oi_out_of_memory(million_oi))
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(seconds_oi);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        oi_multiply(seconds_million, seconds_oi, million_oi);
        oi_remove_reference(seconds_oi);
        if (oi_out_of_memory(seconds_million))
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(million_oi);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        oi_create_from_long_int(microseconds_oi, microseconds_unsigned_long);
        if (oi_out_of_memory(microseconds_oi))
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(seconds_million);
            oi_remove_reference(million_oi);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        oi_add(microsecond_sum, microseconds_oi, seconds_million);
        oi_remove_reference(microseconds_oi);
        oi_remove_reference(seconds_million);
        if (oi_out_of_memory(microsecond_sum))
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(million_oi);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        seconds_rational = create_rational(microsecond_sum, million_oi);
        oi_remove_reference(microsecond_sum);
        oi_remove_reference(million_oi);
        if (seconds_rational == NULL)
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }
        assert(!(rational_is_integer(seconds_rational)));

        seconds_value = create_rational_value(seconds_rational);
        rational_remove_reference(seconds_rational);
      }

    if (seconds_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    the_verdict = add_field(result, "seconds", seconds_value);
    value_remove_reference(seconds_value, the_jumper);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    if (is_local_boolean)
      {
        zone_string = time_zone_name_from_tm(the_tm, &know_if_daylight_savings,
                &is_daylight_savings, &know_seconds_ahead_of_utc,
                &seconds_ahead_of_utc);
      }
    else
      {
        know_if_daylight_savings = TRUE;
        is_daylight_savings = FALSE;
        know_seconds_ahead_of_utc = TRUE;
        seconds_ahead_of_utc = 0;
        zone_string = "GMT";
      }

    if (zone_string == NULL)
        zone_value = create_null_value();
    else
        zone_value = create_string_value(zone_string);
    if (zone_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    the_verdict = add_field(result, "zone", zone_value);
    value_remove_reference(zone_value, the_jumper);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    if (!know_if_daylight_savings)
        is_daylight_savings_value = create_null_value();
    else if (is_daylight_savings)
        is_daylight_savings_value = create_true_value();
    else
        is_daylight_savings_value = create_false_value();
    if (is_daylight_savings_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    the_verdict = add_field(result, "is_daylight_savings",
                            is_daylight_savings_value);
    value_remove_reference(is_daylight_savings_value, the_jumper);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    if (!know_seconds_ahead_of_utc)
      {
        seconds_ahead_of_utc_value = create_null_value();
      }
    else
      {
        o_integer seconds_ahead_of_utc_oi;

        oi_create_from_long_int(seconds_ahead_of_utc_oi, seconds_ahead_of_utc);
        if (oi_out_of_memory(seconds_ahead_of_utc_oi))
          {
            jumper_do_abort(the_jumper);
            value_remove_reference(result, the_jumper);
            return NULL;
          }

        seconds_ahead_of_utc_value =
                create_integer_value(seconds_ahead_of_utc_oi);
        oi_remove_reference(seconds_ahead_of_utc_oi);
      }

    if (seconds_ahead_of_utc_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    the_verdict = add_field(result, "seconds_ahead_of_utc",
                            seconds_ahead_of_utc_value);
    value_remove_reference(seconds_ahead_of_utc_value, the_jumper);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

static value *throw_new_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *exception_value;
    value *tag_value;
    verdict the_verdict;
    value *message_value;
    value *source_value;
    value *file_name_value;
    value *start_line_value;
    value *start_column_value;
    value *end_line_value;
    value *end_column_value;
    value *other_value;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 8);

    exception_value = create_lepton_value(jumper_exception_key(the_jumper));
    if (exception_value == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    tag_value = value_component_value(all_arguments_value, 0);
    assert(tag_value != NULL);
    assert(get_value_kind(tag_value) == VK_QUARK);

    the_verdict = add_field(exception_value, "tag", tag_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    message_value = value_component_value(all_arguments_value, 1);
    assert(message_value != NULL);
    assert(get_value_kind(message_value) == VK_STRING);

    the_verdict = add_field(exception_value, "message", message_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    source_value = create_lepton_value(jumper_region_key(the_jumper));
    if (source_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    file_name_value = value_component_value(all_arguments_value, 2);
    assert(file_name_value != NULL);
    assert(get_value_kind(file_name_value) == VK_STRING);

    the_verdict = add_field(source_value, "file_name", file_name_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(source_value, the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    start_line_value = value_component_value(all_arguments_value, 3);
    assert(start_line_value != NULL);
    assert(get_value_kind(start_line_value) == VK_INTEGER);

    the_verdict = add_field(source_value, "start_line", start_line_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(source_value, the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    start_column_value = value_component_value(all_arguments_value, 4);
    assert(start_column_value != NULL);
    assert(get_value_kind(start_column_value) == VK_INTEGER);

    the_verdict = add_field(source_value, "start_column", start_column_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(source_value, the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    end_line_value = value_component_value(all_arguments_value, 5);
    assert(end_line_value != NULL);
    assert(get_value_kind(end_line_value) == VK_INTEGER);

    the_verdict = add_field(source_value, "end_line", end_line_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(source_value, the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    end_column_value = value_component_value(all_arguments_value, 6);
    assert(end_column_value != NULL);
    assert(get_value_kind(end_column_value) == VK_INTEGER);

    the_verdict = add_field(source_value, "end_column", end_column_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(source_value, the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    the_verdict = add_field(exception_value, "source", source_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(source_value, the_jumper);
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    value_remove_reference(source_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    other_value = value_component_value(all_arguments_value, 7);
    assert(other_value != NULL);

    if (!(value_has_only_named_fields(other_value)))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(throw_unnamed_extra),
                "The ``other'' parameter to a throw() call contained an "
                "unnamed field.");
        value_remove_reference(exception_value, the_jumper);
        return NULL;
      }

    jumper_throw_exception(the_jumper, exception_value, other_value);

    value_remove_reference(exception_value, the_jumper);

    return NULL;
  }

static value *throw_old_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *exception_value;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);

    exception_value = value_component_value(all_arguments_value, 0);
    assert(exception_value != NULL);
    assert(get_value_kind(exception_value) == VK_LEPTON);

    jumper_throw_exception(the_jumper, exception_value, NULL);

    value_remove_reference(exception_value, the_jumper);

    return NULL;
  }

static value *current_exceptions_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *result;

    result = jumper_exception_information(the_jumper);

    if (result == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(current_exceptions_no_exception),
                "The current_exceptions() function was called outside the "
                "``catch'' part of a try-catch statement.");
      }
    else
      {
        value_add_reference(result);
      }

    return result;
  }

static value *numerator_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *base_value;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    base_value = value_component_value(all_arguments_value, 0);

    assert(base_value != NULL);
    switch (get_value_kind(base_value))
      {
        case VK_INTEGER:
          {
            value_add_reference(base_value);
            return base_value;
          }
        case VK_RATIONAL:
          {
            value *result_value;

            result_value = create_integer_value(
                    rational_numerator(rational_value_data(base_value)));
            if (result_value == NULL)
                jumper_do_abort(the_jumper);
            return result_value;
          }
        default:
          {
            assert(FALSE);
            return NULL;
          }
      }
  }

static value *denominator_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *base_value;
    o_integer result_oi;
    value *result_value;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    base_value = value_component_value(all_arguments_value, 0);

    assert(base_value != NULL);
    switch (get_value_kind(base_value))
      {
        case VK_INTEGER:
          {
            result_oi = oi_one;
            break;
          }
        case VK_RATIONAL:
          {
            result_oi = rational_denominator(rational_value_data(base_value));
            break;
          }
        default:
          {
            assert(FALSE);
            break;
          }
      }

    result_value = create_integer_value(result_oi);
    if (result_value == NULL)
        jumper_do_abort(the_jumper);
    return result_value;
  }

static value *power_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    value *base_value;
    o_integer exponent;
    o_integer base_numerator;
    o_integer base_denominator;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 2);
    base_value = value_component_value(all_arguments_value, 0);
    exponent = integer_for_argument(all_arguments_value, 1);

    assert(base_value != NULL);
    switch (get_value_kind(base_value))
      {
        case VK_INTEGER:
          {
            base_numerator = integer_value_data(base_value);
            assert(!(oi_out_of_memory(base_numerator)));
            base_denominator = oi_null;
            break;
          }
        case VK_RATIONAL:
          {
            rational *base_rational;

            base_rational = rational_value_data(base_value);
            assert(base_rational != NULL);

            base_numerator = rational_numerator(base_rational);
            assert(!(oi_out_of_memory(base_numerator)));
            base_denominator = rational_denominator(base_rational);
            assert(!(oi_out_of_memory(base_denominator)));
            break;
          }
        default:
          {
            assert(FALSE);
            base_numerator = oi_null;
            base_denominator = oi_null;
          }
      }

    if ((oi_kind(exponent) != IIK_FINITE) || !(oi_is_negative(exponent)))
      {
        o_integer result_numerator;
        o_integer result_denominator;
        rational *result_rational;
        value *result_value;

        result_numerator = oi_power(base_numerator, exponent);
        if (oi_out_of_memory(result_numerator))
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        if (oi_out_of_memory(base_denominator))
          {
            value *result_value;

            result_value = create_integer_value(result_numerator);
            oi_remove_reference(result_numerator);
            if (result_value == NULL)
                jumper_do_abort(the_jumper);
            return result_value;
          }

        result_denominator = oi_power(base_denominator, exponent);
        if (oi_out_of_memory(result_denominator))
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(result_numerator);
            return NULL;
          }

        result_rational =
                create_rational(result_numerator, result_denominator);
        oi_remove_reference(result_numerator);
        oi_remove_reference(result_denominator);
        if (result_rational == NULL)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        if (rational_is_integer(result_rational))
          {
            result_value =
                    create_integer_value(rational_numerator(result_rational));
          }
        else
          {
            result_value = create_rational_value(result_rational);
          }
        rational_remove_reference(result_rational);

        if (result_value == NULL)
            jumper_do_abort(the_jumper);
        return result_value;
      }
    else
      {
        o_integer negated_exponent;
        o_integer result_denominator;
        o_integer result_numerator;
        rational *result_rational;
        value *result_value;

        oi_negate(negated_exponent, exponent);
        if (oi_out_of_memory(negated_exponent))
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        result_denominator = oi_power(base_numerator, negated_exponent);
        if (oi_out_of_memory(result_denominator))
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(negated_exponent);
            return NULL;
          }

        if (oi_out_of_memory(base_denominator))
          {
            result_numerator = oi_one;
            oi_add_reference(result_numerator);
          }
        else
          {
            result_numerator = oi_power(base_denominator, negated_exponent);
          }
        oi_remove_reference(negated_exponent);
        if (oi_out_of_memory(result_numerator))
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(result_denominator);
            return NULL;
          }

        result_rational =
                create_rational(result_numerator, result_denominator);
        oi_remove_reference(result_numerator);
        oi_remove_reference(result_denominator);
        if (result_rational == NULL)
          {
            jumper_do_abort(the_jumper);
            return NULL;
          }

        if (rational_is_integer(result_rational))
          {
            result_value =
                    create_integer_value(rational_numerator(result_rational));
          }
        else
          {
            result_value = create_rational_value(result_rational);
          }
        rational_remove_reference(result_rational);

        if (result_value == NULL)
            jumper_do_abort(the_jumper);
        return result_value;
      }
  }

static value *set_source_region_key_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);

    jumper_set_region_key(the_jumper,
            value_lepton_key(value_component_value(all_arguments_value, 0)));

    return NULL;
  }

static value *set_exception_key_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);

    jumper_set_exception_key(the_jumper,
            value_lepton_key(value_component_value(all_arguments_value, 0)));

    return NULL;
  }

static value *set_standard_library_object_handler_function(
        value *all_arguments_value, context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);

    jumper_set_standard_library_object(the_jumper,
            object_value_data(value_component_value(all_arguments_value, 0)));

    return NULL;
  }

static value *get_environment_handler_function(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    const char *key;
    const char *target_string;
    value *result;

    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(value_component_count(all_arguments_value) == 1);
    key = string_for_argument(all_arguments_value, 0);

    /*
     * The ANSI C 1990 standard allows subsequent calls to getenv to overwrite
     * the string returned by a previous call.  So, to be thread safe, we can't
     * allow another thread to call getenv before we're done with the string it
     * returns.
     */

    GRAB_SYSTEM_LOCK(getenv_lock);

    target_string = getenv(key);

    result = c_string_to_value(target_string);

    RELEASE_SYSTEM_LOCK(getenv_lock);

    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

static exception_error_handler_data *create_exception_error_data(
        jumper *the_jumper, const source_location *location)
  {
    return create_exception_error_data_with_object(the_jumper, location, NULL,
                                                   NULL);
  }

static exception_error_handler_data *create_exception_error_data_with_object(
        jumper *the_jumper, const source_location *location,
        object *the_object, lepton_key_instance *io_error_key)
  {
    exception_error_handler_data *the_data;

    the_data = MALLOC_ONE_OBJECT(exception_error_handler_data);
    if (the_data == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    the_data->jumper = the_jumper;
    the_data->location = location;
    the_data->the_object = the_object;
    the_data->io_error_key = io_error_key;

    return the_data;
  }

static void exception_error_handler(void *data, const char *format, ...)
  {
    exception_error_handler_data *typed_data;
    jumper *the_jumper;
    const source_location *location;
    va_list ap;
    string_buffer buffer;
    verdict the_verdict;
    int return_code;

    assert(data != NULL);

    typed_data = (exception_error_handler_data *)data;
    the_jumper = typed_data->jumper;
    assert(the_jumper != NULL);
    location = typed_data->location;

    assert(format != NULL);

    va_start(ap, format);

    if (typed_data->the_object != NULL)
      {
        vset_fp_object_error_info(typed_data->the_object,
                typed_data->io_error_key, the_jumper, format, ap);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            va_end(ap);
            return;
          }
      }

    the_verdict = string_buffer_init(&buffer, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        va_end(ap);
        return;
      }

    return_code = vinterpreter_zero_buffer_printf(&buffer, 0, format, ap);
    va_end(ap);
    if (return_code < 0)
      {
        jumper_do_abort(the_jumper);
        free(buffer.array);
        return;
      }

    if (strncmp(buffer.array, "Bad UTF-8 encoding",
                strlen("Bad UTF-8 encoding")) == 0)
      {
        location_exception(the_jumper, location, EXCEPTION_TAG(bad_utf8), "%s",
                           buffer.array);
      }
    else
      {
        assert(strncmp(buffer.array, "Bad UTF-16 encoding",
                       strlen("Bad UTF-16 encoding")) == 0);
        location_exception(the_jumper, location, EXCEPTION_TAG(bad_utf16),
                           "%s", buffer.array);
      }

    free(buffer.array);
  }

static size_t array_value_element_count(value *array_value,
        const char *routine_name, jumper *the_jumper,
        const source_location *location)
  {
    assert(array_value != NULL);

    switch (get_value_kind(array_value))
      {
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            return value_component_count(array_value);
          }
        case VK_MAP:
          {
            o_integer result_oi;
            size_t result_size_t;
            verdict the_verdict;
            o_integer comparator;

            result_oi = map_value_length(array_value, routine_name, the_jumper,
                                         location);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                assert(oi_out_of_memory(result_oi));
                return 0;
              }

            the_verdict = oi_magnitude_to_size_t(result_oi, &result_size_t);
            if (the_verdict == MISSION_ACCOMPLISHED)
              {
                oi_remove_reference(result_oi);
                return result_size_t;
              }

            oi_create_from_size_t(comparator, ~(size_t)0);
            if (oi_out_of_memory(comparator))
              {
                jumper_do_abort(the_jumper);
                oi_remove_reference(result_oi);
                return 0;
              }

            if (oi_less_than(comparator, result_oi))
              {
                location_exception(the_jumper, location,
                        EXCEPTION_TAG(array_too_large),
                        "The array argument to %s() was too enormous to be "
                        "handled.", routine_name);
              }
            else
              {
                jumper_do_abort(the_jumper);
              }

            oi_remove_reference(comparator);
            oi_remove_reference(result_oi);

            return 0;
          }
        default:
          {
            assert(FALSE);
            return 0;
          }
      }
  }

static value *array_value_element_value(value *array_value, size_t element_num,
        jumper *the_jumper, const source_location *location)
  {
    assert(array_value != NULL);
    assert(the_jumper != NULL);

    switch (get_value_kind(array_value))
      {
        case VK_SEMI_LABELED_VALUE_LIST:
          {
            return value_component_value(array_value, element_num);
          }
        case VK_MAP:
          {
            o_integer key_oi;
            value *key_value;
            boolean doubt;
            value *element_value;

            oi_create_from_size_t(key_oi, element_num);
            if (oi_out_of_memory(key_oi))
              {
                jumper_do_abort(the_jumper);
                return NULL;
              }

            key_value = create_integer_value(key_oi);
            oi_remove_reference(key_oi);
            if (key_value == NULL)
              {
                jumper_do_abort(the_jumper);
                return NULL;
              }

            assert(map_value_all_keys_are_valid(array_value));
                    /* VERIFICATION NEEDED */
            assert(value_is_valid(key_value)); /* VERIFICATION NEEDED */
            element_value = map_value_lookup(array_value, key_value, &doubt,
                                             location, the_jumper);
            value_remove_reference(key_value, the_jumper);
            if (!(jumper_flowing_forward(the_jumper)))
              {
                if (element_value != NULL)
                    value_remove_reference(element_value, the_jumper);
                return NULL;
              }

            assert(!doubt);

            return element_value;
          }
        default:
          {
            assert(FALSE);
            return NULL;
          }
      }
  }

static unsigned long array_value_element_u32(value *array_value,
        size_t element_num, jumper *the_jumper,
        const source_location *location, static_exception_tag *undefined_tag,
        const char *function_name)
  {
    value *element_value;
    o_integer element_oi;
    size_t element_size_t;
    verdict the_verdict;

    element_value = array_value_element_value(array_value, element_num,
                                              the_jumper, location);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        assert(element_value == NULL);
        return 0;
      }

    if (element_value == NULL)
      {
        location_exception(the_jumper, location, undefined_tag,
                "In %s(), element %lu of the array is undefined.",
                function_name, (unsigned long)element_num);
        return 0;
      }

    assert(get_value_kind(element_value) == VK_INTEGER);
    element_oi = integer_value_data(element_value);
    assert(!(oi_out_of_memory(element_oi)));
    assert(oi_kind(element_oi) == IIK_FINITE);
    assert(!(oi_is_negative(element_oi)));

    the_verdict = oi_magnitude_to_size_t(element_oi, &element_size_t);
    assert(the_verdict == MISSION_ACCOMPLISHED);

    assert(element_size_t <= 0xffffffff);
    return (unsigned long)element_size_t;
  }

static void overload_print_value(value *the_value,
        void (*printer)(void *data, const char *format, ...), void *data,
        call_printer_data *cp_data,
        void (*override)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data))
  {
    value *result_value;
    verdict the_verdict;

    assert(the_value != NULL);
    assert(printer != NULL);
    assert(cp_data != NULL);

    if (cp_data->verdict != MISSION_ACCOMPLISHED)
        return;

    the_verdict = try_overloading(the_value, "sprint", &result_value, 0, NULL,
                                  NULL, cp_data->jumper, cp_data->location);
    if (the_verdict == MISSION_ACCOMPLISHED)
      {
        if (!(jumper_flowing_forward(cp_data->jumper)))
          {
            cp_data->verdict = MISSION_FAILED;
            assert(result_value == NULL);
          }
        else
          {
            if (get_value_kind(result_value) == VK_STRING)
              {
                (*printer)(data, "%s", string_value_data(result_value));
              }
            else
              {
                location_exception(cp_data->jumper, cp_data->location,
                        EXCEPTION_TAG(sprint_not_string),
                        "When printing an object with a sprint() routine, the "
                        "sprint() routine returned a value that wasn't a "
                        "string.");
                cp_data->verdict = MISSION_FAILED;
              }

            value_remove_reference(result_value, cp_data->jumper);
            if (!(jumper_flowing_forward(cp_data->jumper)))
                cp_data->verdict = MISSION_FAILED;
          }
        return;
      }

    assert(value_is_valid(the_value)); /* VERIFICATION NEEDED */
    print_value_with_override(the_value, printer, data, override);
  }

static void overload_printf(value *all_arguments_value,
        size_t format_argument_number,
        void (*printer)(void *data, const char *format, ...), void *data,
        call_printer_data *cp_data,
        void (*override)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data))
  {
    size_t max_argument_num;
    size_t next_argument_num;
    const char *format_chars;

    assert(all_arguments_value != NULL);
    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(format_argument_number <
           value_component_count(all_arguments_value));
    assert(printer != NULL);
    assert(cp_data != NULL);
    assert(override != NULL);

    max_argument_num = (value_component_count(all_arguments_value) -
                        (format_argument_number + 1));
    next_argument_num = 1;
    format_chars =
            string_for_argument(all_arguments_value, format_argument_number);

    while (*format_chars != 0)
      {
        const char *close;
        value *to_convert;
        boolean flag_minus;
        boolean flag_plus;
        boolean flag_space;
        boolean flag_octothorp;
        boolean flag_zero;
        o_integer width;
        o_integer precision;
        value *call_value;

        if (*format_chars != '%')
          {
            const char *end;

            end = format_chars;

            while ((*end != 0) && (*end != '%'))
                ++end;

            (*printer)(data, "%.*s", (int)(end - format_chars), format_chars);

            format_chars = end;

            continue;
          }

        ++format_chars;

        if (*format_chars == '%')
          {
            (*printer)(data, "%%");
            ++format_chars;
            continue;
          }

        close = format_chars;
        while (*close != '%')
          {
            if (*close == 0)
              {
                location_exception(cp_data->jumper, cp_data->location,
                        EXCEPTION_TAG(printf_unclosed_specifier),
                        "When doing formatted printing, a percent sign (`%%') "
                        "was found without a matching closing percent sign.");
                return;
              }
            ++close;
          }

        if ((*format_chars >= '0') && (*format_chars <= '9'))
          {
            const char *follow;

            follow = format_chars + 1;
            while ((*follow >= '0') && (*follow <= '9'))
                ++follow;

            if (*follow == ':')
              {
                next_argument_num = 0;
                while ((*format_chars >= '0') && (*format_chars <= '9'))
                  {
                    size_t digit;

                    digit = (*format_chars - '0');
                    if (next_argument_num > (max_argument_num - digit) / 10)
                      {
                        location_exception(cp_data->jumper, cp_data->location,
                                EXCEPTION_TAG(printf_too_few_arguments),
                                "When doing formatted printing, not enough "
                                "arguments were supplied.");
                        return;
                      }
                    next_argument_num = ((next_argument_num * 10) + digit);
                    ++format_chars;
                  }

                if (next_argument_num == 0)
                  {
                    location_exception(cp_data->jumper, cp_data->location,
                            EXCEPTION_TAG(printf_zero_argument),
                            "When doing formatted printing, a conversion "
                            "specifier specified zero as an argument number.");
                    return;
                  }

                format_chars = follow + 1;
              }
          }
        else if (((*format_chars >= 'a') && (*format_chars <= 'z')) ||
                 ((*format_chars >= 'A') && (*format_chars <= 'Z')) ||
                 (*format_chars == '_'))
          {
            size_t id_length;

            id_length = identifier_length(format_chars);

            if (format_chars[id_length] == ':')
              {
                for (next_argument_num = 1;
                     next_argument_num <= max_argument_num;
                     ++next_argument_num)
                  {
                    const char *label;

                    label = value_component_label(all_arguments_value,
                            next_argument_num + format_argument_number);
                    if ((label != NULL) &&
                        (strncmp(label, format_chars, id_length) == 0) &&
                        (strlen(label) == id_length))
                      {
                        break;
                      }
                  }

                if (next_argument_num > max_argument_num)
                  {
                    location_exception(cp_data->jumper, cp_data->location,
                            EXCEPTION_TAG(printf_bad_argument_name),
                            "When doing formatted printing, a conversion "
                            "specifier specified an argument named `%.*s', but"
                            " no such argument was found.", (int)id_length,
                            format_chars);
                    return;
                  }

                format_chars += id_length + 1;
              }
          }

        if (next_argument_num > max_argument_num)
          {
            assert(next_argument_num == (max_argument_num + 1));
            location_exception(cp_data->jumper, cp_data->location,
                    EXCEPTION_TAG(printf_too_few_arguments),
                    "When doing formatted printing, not enough arguments were "
                    "supplied.");
            return;
          }

        to_convert = value_component_value(all_arguments_value,
                next_argument_num + format_argument_number);
        assert(to_convert != NULL);

        ++next_argument_num;

        flag_minus = FALSE;
        flag_plus = FALSE;
        flag_space = FALSE;
        flag_octothorp = FALSE;
        flag_zero = FALSE;

        while (TRUE)
          {
            switch (*format_chars)
              {
                case '-':
                    flag_minus = TRUE;
                    break;
                case '+':
                    flag_plus = TRUE;
                    break;
                case ' ':
                    flag_space = TRUE;
                    break;
                case '#':
                    flag_octothorp = TRUE;
                    break;
                case '0':
                    flag_zero = TRUE;
                    break;
                default:
                    goto done;
              }

            ++format_chars;
          }
      done:

        if (flag_plus)
            flag_space = FALSE;
        if (flag_minus)
            flag_zero = FALSE;

        if ((*format_chars >= '0') && (*format_chars <= '9'))
          {
            const char *follow;

            follow = format_chars + 1;
            while ((*follow >= '0') && (*follow <= '9'))
                ++follow;

            oi_create_from_decimal_ascii(width, follow - format_chars,
                                         format_chars, FALSE);
            if (oi_out_of_memory(width))
              {
                jumper_do_abort(cp_data->jumper);
                return;
              }

            format_chars = follow;
          }
        else
          {
            width = oi_null;
          }

        if ((*format_chars == '.') && (format_chars[1] >= '0') &&
            (format_chars[1] <= '9'))
          {
            const char *follow;

            ++format_chars;
            follow = format_chars + 1;
            while ((*follow >= '0') && (*follow <= '9'))
                ++follow;

            oi_create_from_decimal_ascii(precision, follow - format_chars,
                                         format_chars, FALSE);
            if (oi_out_of_memory(precision))
              {
                jumper_do_abort(cp_data->jumper);
                if (!(oi_out_of_memory(width)))
                    oi_remove_reference(width);
                return;
              }

            format_chars = follow;
          }
        else
          {
            precision = oi_null;
          }

        call_value = try_formatting_call(to_convert, format_chars,
                close - format_chars, flag_plus, flag_space, flag_octothorp,
                flag_zero ? width : oi_null, precision, cp_data->jumper,
                cp_data->location);
        if (!(jumper_flowing_forward(cp_data->jumper)))
          {
            assert(call_value == NULL);
            if (!(oi_out_of_memory(precision)))
                oi_remove_reference(precision);
            if (!(oi_out_of_memory(width)))
                oi_remove_reference(width);
            return;
          }

        if (call_value != NULL)
          {
            if (get_value_kind(call_value) != VK_STRING)
              {
                location_exception(cp_data->jumper, cp_data->location,
                        EXCEPTION_TAG(printf_sprint_not_string),
                        "When doing formatted printing, the result of a field "
                        "sprint() was not a string value.");
                value_remove_reference(call_value, cp_data->jumper);
                if (!(oi_out_of_memory(precision)))
                    oi_remove_reference(precision);
                if (!(oi_out_of_memory(width)))
                    oi_remove_reference(width);
                return;
              }

            if (oi_out_of_memory(width))
              {
                (*printer)(data, "%s", string_value_data(call_value));
              }
            else
              {
                print_with_width(printer, data, string_value_data(call_value),
                        width, flag_minus, FALSE, 0, '0', cp_data->jumper);
                if (!(jumper_flowing_forward(cp_data->jumper)))
                  {
                    value_remove_reference(call_value, cp_data->jumper);
                    if (!(oi_out_of_memory(precision)))
                        oi_remove_reference(precision);
                    if (!(oi_out_of_memory(width)))
                        oi_remove_reference(width);
                    return;
                  }
              }

            value_remove_reference(call_value, cp_data->jumper);
            assert(jumper_flowing_forward(cp_data->jumper));
          }
        else
          {
            if (oi_out_of_memory(width))
              {
                print_value_with_formatting(to_convert, format_chars,
                        close - format_chars, printer, data, cp_data, override,
                        flag_plus, flag_space, flag_octothorp, NULL, NULL,
                        NULL, precision, cp_data->jumper);
              }
            else
              {
                string_buffer output_buffer;
                verdict the_verdict;
                string_printer_data sp_data;
                boolean have_zeros_position;
                size_t zeros_position;
                char zeros_character;

                the_verdict = string_buffer_init(&output_buffer, 10);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(cp_data->jumper);
                    if (!(oi_out_of_memory(precision)))
                        oi_remove_reference(precision);
                    if (!(oi_out_of_memory(width)))
                        oi_remove_reference(width);
                    return;
                  }

                sp_data.output_buffer = &output_buffer;
                sp_data.cp_data.verdict = MISSION_ACCOMPLISHED;
                sp_data.cp_data.jumper = cp_data->jumper;
                sp_data.cp_data.location = cp_data->location;
                print_value_with_formatting(to_convert, format_chars,
                        close - format_chars, &string_printer, &sp_data,
                        &(sp_data.cp_data), &string_print_value, flag_plus,
                        flag_space, flag_octothorp, &have_zeros_position,
                        &zeros_position, &zeros_character, precision,
                        cp_data->jumper);
                if (sp_data.cp_data.verdict != MISSION_ACCOMPLISHED)
                  {
                    free(output_buffer.array);
                    if (!(oi_out_of_memory(precision)))
                        oi_remove_reference(precision);
                    if (!(oi_out_of_memory(width)))
                        oi_remove_reference(width);
                    return;
                  }

                the_verdict = string_buffer_append(&output_buffer, 0);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(cp_data->jumper);
                    free(output_buffer.array);
                    if (!(oi_out_of_memory(precision)))
                        oi_remove_reference(precision);
                    if (!(oi_out_of_memory(width)))
                        oi_remove_reference(width);
                    return;
                  }

                print_with_width(printer, data, output_buffer.array, width,
                        flag_minus, flag_zero && have_zeros_position,
                        zeros_position, zeros_character, cp_data->jumper);
                free(output_buffer.array);
                if (!(jumper_flowing_forward(cp_data->jumper)))
                  {
                    if (!(oi_out_of_memory(precision)))
                        oi_remove_reference(precision);
                    if (!(oi_out_of_memory(width)))
                        oi_remove_reference(width);
                    return;
                  }
              }
          }

        if (!(oi_out_of_memory(precision)))
            oi_remove_reference(precision);

        if (!(oi_out_of_memory(width)))
            oi_remove_reference(width);

        format_chars = close + 1;
      }
  }

static value *try_formatting_call(value *to_convert, const char *format,
        size_t format_length, boolean flag_plus, boolean flag_space,
        boolean flag_octothorp, o_integer zero_width, o_integer precision,
        jumper *the_jumper, const source_location *location)
  {
    value *call_base;
    semi_labeled_value_list *actuals;
    size_t flag_num;
    value *result;

    assert(to_convert != NULL);
    assert(format != NULL);
    assert(the_jumper != NULL);

    switch (get_value_kind(to_convert))
      {
        case VK_SEMI_LABELED_VALUE_LIST:
        case VK_SEMI_LABELED_MULTI_SET:
        case VK_LEPTON:
          {
            call_base = value_get_field("sprint", to_convert);
            if (call_base == NULL)
                return NULL;
            value_add_reference(call_base);
            break;
          }
        case VK_OBJECT:
          {
            object *base_object;
            size_t field_num;

            base_object = object_value_data(to_convert);
            assert(base_object != NULL);

            assert(!(object_is_closed(base_object))); /* VERIFICATION NEEDED */
            field_num = object_field_lookup(base_object, "sprint");
            if (field_num >= object_field_count(base_object))
                return NULL;

            assert(!(object_is_closed(base_object))); /* VERIFICATION NEEDED */
            call_base = object_field_read_value(base_object, field_num,
                                                location, the_jumper);
            if (call_base == NULL)
                return NULL;

            break;
          }
        default:
          {
            return NULL;
          }
      }

    switch (get_value_kind(call_base))
      {
        case VK_ROUTINE:
        case VK_ROUTINE_CHAIN:
            break;
        default:
            value_remove_reference(call_base, the_jumper);
            return NULL;
      }

    actuals = create_semi_labeled_value_list();
    if (actuals == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(call_base, the_jumper);
        return NULL;
      }

    if ((format_length != 0) &&
        ((format_length != 1) || (strncmp(format, "v", 1) != 0)))
      {
        char *buffer;
        value *specifier_value;
        verdict the_verdict;

        buffer = MALLOC_ARRAY(char, format_length + 1);
        if (buffer == NULL)
          {
            jumper_do_abort(the_jumper);
            delete_semi_labeled_value_list(actuals, the_jumper);
            value_remove_reference(call_base, the_jumper);
            return NULL;
          }
        memcpy(buffer, format, format_length);
        buffer[format_length] = 0;

        specifier_value = create_string_value(buffer);
        free(buffer);
        if (specifier_value == NULL)
          {
            jumper_do_abort(the_jumper);
            delete_semi_labeled_value_list(actuals, the_jumper);
            value_remove_reference(call_base, the_jumper);
            return NULL;
          }

        the_verdict = append_value_to_semi_labeled_value_list(actuals, NULL,
                                                              specifier_value);
        value_remove_reference(specifier_value, the_jumper);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            delete_semi_labeled_value_list(actuals, the_jumper);
            value_remove_reference(call_base, the_jumper);
            return NULL;
          }
      }

    for (flag_num = 0; flag_num < 3; ++flag_num)
      {
        boolean flag_is_set;
        const char *flag_name;
        value *true_value;
        verdict the_verdict;

        switch (flag_num)
          {
            case 0:
                flag_is_set = flag_plus;
                flag_name = "plus";
                break;
            case 1:
                flag_is_set = flag_space;
                flag_name = "space";
                break;
            case 2:
                flag_is_set = flag_octothorp;
                flag_name = "hash";
                break;
            default:
                assert(FALSE);
                flag_is_set = FALSE;
                flag_name = NULL;
          }

        if (!flag_is_set)
            continue;

        true_value = create_true_value();
        if (true_value == NULL)
          {
            jumper_do_abort(the_jumper);
            delete_semi_labeled_value_list(actuals, the_jumper);
            value_remove_reference(call_base, the_jumper);
            return NULL;
          }

        the_verdict = append_value_to_semi_labeled_value_list(actuals,
                flag_name, true_value);
        value_remove_reference(true_value, the_jumper);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            delete_semi_labeled_value_list(actuals, the_jumper);
            value_remove_reference(call_base, the_jumper);
            return NULL;
          }
      }

    if (!(oi_out_of_memory(zero_width)))
      {
        value *width_value;
        verdict the_verdict;

        width_value = create_integer_value(zero_width);
        if (width_value == NULL)
          {
            jumper_do_abort(the_jumper);
            delete_semi_labeled_value_list(actuals, the_jumper);
            value_remove_reference(call_base, the_jumper);
            return NULL;
          }

        the_verdict = append_value_to_semi_labeled_value_list(actuals,
                "zero_width", width_value);
        value_remove_reference(width_value, the_jumper);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            delete_semi_labeled_value_list(actuals, the_jumper);
            value_remove_reference(call_base, the_jumper);
            return NULL;
          }
      }

    if (!(oi_out_of_memory(precision)))
      {
        value *precision_value;
        verdict the_verdict;

        precision_value = create_integer_value(precision);
        if (precision_value == NULL)
          {
            jumper_do_abort(the_jumper);
            delete_semi_labeled_value_list(actuals, the_jumper);
            value_remove_reference(call_base, the_jumper);
            return NULL;
          }

        the_verdict = append_value_to_semi_labeled_value_list(actuals,
                "precision", precision_value);
        value_remove_reference(precision_value, the_jumper);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            delete_semi_labeled_value_list(actuals, the_jumper);
            value_remove_reference(call_base, the_jumper);
            return NULL;
          }
      }

    result = execute_call_from_values(call_base, actuals, TRUE, the_jumper,
                                      location);
    delete_semi_labeled_value_list(actuals, the_jumper);
    value_remove_reference(call_base, the_jumper);
    return result;
  }

static void print_with_width(
        void (*printer)(void *data, const char *format, ...), void *data,
        const char *to_print, o_integer width, boolean flag_minus,
        boolean use_zeros, size_t zeros_position, char zeros_character,
        jumper *the_jumper)
  {
    assert(printer != NULL);
    assert(to_print != NULL);
    assert(!(oi_out_of_memory(width)));
    assert(the_jumper != NULL);

    if (flag_minus)
      {
        assert(!use_zeros);
        (*printer)(data, "%s", to_print);
      }
    else if (use_zeros)
      {
        (*printer)(data, "%.*s", (int)zeros_position, to_print);
      }

    do_padding(printer, data, width, utf8_string_character_count(to_print),
               (use_zeros ? zeros_character : ' '), the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return;

    if (use_zeros)
      {
        assert(!flag_minus);
        (*printer)(data, "%s", &(to_print[zeros_position]));
      }
    else if (!flag_minus)
      {
        (*printer)(data, "%s", to_print);
      }
  }

static void do_padding(void (*printer)(void *data, const char *format, ...),
        void *data, o_integer width, size_t characters_used,
        char pad_character, jumper *the_jumper)
  {
    o_integer used_count;
    o_integer pad_count;
    o_integer block_oi;
    char padding[PRINTF_PAD_BLOCK_SIZE];
    size_t character_num;
    size_t pad_remainder;
    verdict the_verdict;

    assert(printer != NULL);
    assert(!(oi_out_of_memory(width)));
    assert(the_jumper != NULL);

    oi_create_from_size_t(used_count, characters_used);
    if (oi_out_of_memory(used_count))
      {
        jumper_do_abort(the_jumper);
        return;
      }

    if (!(oi_less_than(used_count, width)))
      {
        oi_remove_reference(used_count);
        return;
      }

    oi_subtract(pad_count, width, used_count);
    oi_remove_reference(used_count);
    if (oi_out_of_memory(pad_count))
      {
        jumper_do_abort(the_jumper);
        return;
      }

    oi_create_from_size_t(block_oi, PRINTF_PAD_BLOCK_SIZE);
    if (oi_out_of_memory(block_oi))
      {
        jumper_do_abort(the_jumper);
        oi_remove_reference(pad_count);
        return;
      }

    for (character_num = 0; character_num < PRINTF_PAD_BLOCK_SIZE;
         ++character_num)
      {
        padding[character_num] = pad_character;
      }

    while (oi_less_than(block_oi, pad_count))
      {
        o_integer new_count;

        assert(PRINTF_PAD_BLOCK_SIZE <= INT_MAX);
        (*printer)(data, "%.*s", (int)PRINTF_PAD_BLOCK_SIZE, &(padding[0]));

        oi_subtract(new_count, pad_count, block_oi);
        oi_remove_reference(pad_count);
        if (oi_out_of_memory(new_count))
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(block_oi);
            return;
          }

        pad_count = new_count;
      }

    oi_remove_reference(block_oi);

    the_verdict = oi_magnitude_to_size_t(pad_count, &pad_remainder);
    assert(the_verdict == MISSION_ACCOMPLISHED);
    oi_remove_reference(pad_count);

    assert(pad_remainder <= PRINTF_PAD_BLOCK_SIZE);
    assert(PRINTF_PAD_BLOCK_SIZE <= INT_MAX);
    (*printer)(data, "%.*s", (int)pad_remainder, &(padding[0]));
  }

static void print_value_with_formatting(value *to_convert, const char *format,
        size_t format_length,
        void (*printer)(void *data, const char *format, ...), void *data,
        call_printer_data *cp_data,
        void (*override)(value *the_value,
                void (*printer)(void *data, const char *format, ...),
                void *data), boolean flag_plus, boolean flag_space,
        boolean flag_octothorp, boolean *have_zeros_position,
        size_t *zeros_position, char *zeros_character, o_integer precision,
        jumper *the_jumper)
  {
    assert(to_convert != NULL);
    assert(format != NULL);
    assert(printer != NULL);
    assert(cp_data != NULL);
    assert(override != NULL);
    assert(the_jumper != NULL);

    if (have_zeros_position != NULL)
        *have_zeros_position = FALSE;
    if (zeros_position != NULL)
        *zeros_position = 0;
    if (zeros_character != NULL)
        *zeros_character = '0';

    if ((format_length == 1) && (strncmp(format, "s", 1) == 0))
      {
        const char *raw_string;

        switch (get_value_kind(to_convert))
          {
            case VK_STRING:
              {
                raw_string = string_value_data(to_convert);
                break;
              }
            case VK_CHARACTER:
              {
                raw_string = character_value_data(to_convert);
                break;
              }
            default:
              {
                location_exception(the_jumper, cp_data->location,
                        EXCEPTION_TAG(printf_not_string),
                        "When doing formatted printing, a string conversion "
                        "specification was used with a non-string, "
                        "non-character value.");
                return;
              }
          }

        if (oi_out_of_memory(precision))
          {
            (*printer)(data, "%s", raw_string);
          }
        else
          {
            size_t count;

            count = utf8_string_character_count(raw_string);
            if (!(oi_less_than_size_t(precision, count)))
              {
                (*printer)(data, "%s", raw_string);
              }
            else
              {
                size_t precision_size_t;
                verdict the_verdict;
                const char *follow;
                size_t num;
                char *buffer;

                the_verdict =
                        oi_magnitude_to_size_t(precision, &precision_size_t);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    return;
                  }

                follow = raw_string;
                for (num = 0; num < precision_size_t; ++num)
                    follow += validate_utf8_character(follow);

                buffer = MALLOC_ARRAY(char, (follow - raw_string) + 1);
                if (buffer == NULL)
                  {
                    jumper_do_abort(the_jumper);
                    return;
                  }

                memcpy(buffer, raw_string, (follow - raw_string));
                buffer[follow - raw_string] = 0;
                (*printer)(data, "%s", buffer);
                free(buffer);
              }
          }
      }
    else if ((format_length == 1) &&
             ((strncmp(format, "d", 1) == 0) ||
              (strncmp(format, "i", 1) == 0)))
      {
        print_integer(to_convert, 10, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE,
                cp_data->location, the_jumper);
      }
    else if ((format_length == 1) && (strncmp(format, "o", 1) == 0))
      {
        print_integer(to_convert, 8, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE,
                cp_data->location, the_jumper);
      }
    else if ((format_length == 1) && (strncmp(format, "x", 1) == 0))
      {
        print_integer(to_convert, 16, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE,
                cp_data->location, the_jumper);
      }
    else if ((format_length == 1) && (strncmp(format, "X", 1) == 0))
      {
        print_integer(to_convert, 16, TRUE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE,
                cp_data->location, the_jumper);
      }
    else if ((format_length == 2) &&
             ((strncmp(format, "cd", 2) == 0) ||
              (strncmp(format, "ci", 2) == 0)))
      {
        print_integer(to_convert, 10, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE,
                cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "co", 2) == 0))
      {
        print_integer(to_convert, 8, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE,
                cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "cx", 2) == 0))
      {
        print_integer(to_convert, 16, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE,
                cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "cX", 2) == 0))
      {
        print_integer(to_convert, 16, TRUE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE,
                cp_data->location, the_jumper);
      }
    else if ((format_length == 1) && (strncmp(format, "f", 1) == 0))
      {
        print_rational(to_convert, 10, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, TRUE, FALSE,
                0, cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "of", 2) == 0))
      {
        print_rational(to_convert, 8, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, TRUE, FALSE,
                0, cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "xf", 2) == 0))
      {
        print_rational(to_convert, 16, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, TRUE, FALSE,
                0, cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "Xf", 2) == 0))
      {
        print_rational(to_convert, 16, TRUE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, TRUE, FALSE,
                0, cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "cf", 2) == 0))
      {
        print_rational(to_convert, 10, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, TRUE, FALSE,
                0, cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "cof", 3) == 0))
      {
        print_rational(to_convert, 8, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, TRUE, FALSE,
                0, cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "cxf", 3) == 0))
      {
        print_rational(to_convert, 16, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, TRUE, FALSE,
                0, cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "cXf", 3) == 0))
      {
        print_rational(to_convert, 16, TRUE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, TRUE, FALSE,
                0, cp_data->location, the_jumper);
      }
    else if ((format_length == 1) && (strncmp(format, "e", 1) == 0))
      {
        print_rational(to_convert, 10, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, FALSE, TRUE,
                'e', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "oe", 2) == 0))
      {
        print_rational(to_convert, 8, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, FALSE, TRUE,
                'e', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "xe", 2) == 0))
      {
        print_rational(to_convert, 16, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, FALSE, TRUE,
                'x', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "Xe", 2) == 0))
      {
        print_rational(to_convert, 16, TRUE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, FALSE, TRUE,
                'x', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "ce", 2) == 0))
      {
        print_rational(to_convert, 10, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, FALSE, TRUE,
                'e', cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "coe", 3) == 0))
      {
        print_rational(to_convert, 8, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, FALSE, TRUE,
                'e', cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "cxe", 3) == 0))
      {
        print_rational(to_convert, 16, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, FALSE, TRUE,
                'x', cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "cXe", 3) == 0))
      {
        print_rational(to_convert, 16, TRUE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, FALSE, TRUE,
                'x', cp_data->location, the_jumper);
      }
    else if ((format_length == 1) && (strncmp(format, "E", 1) == 0))
      {
        print_rational(to_convert, 10, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, FALSE, TRUE,
                'E', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "oE", 2) == 0))
      {
        print_rational(to_convert, 8, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, FALSE, TRUE,
                'E', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "xE", 2) == 0))
      {
        print_rational(to_convert, 16, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, FALSE, TRUE,
                'X', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "XE", 2) == 0))
      {
        print_rational(to_convert, 16, TRUE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, FALSE, TRUE,
                'X', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "cE", 2) == 0))
      {
        print_rational(to_convert, 10, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, FALSE, TRUE,
                'E', cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "coE", 3) == 0))
      {
        print_rational(to_convert, 8, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, FALSE, TRUE,
                'E', cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "cxE", 3) == 0))
      {
        print_rational(to_convert, 16, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, FALSE, TRUE,
                'X', cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "cXE", 3) == 0))
      {
        print_rational(to_convert, 16, TRUE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, FALSE, TRUE,
                'X', cp_data->location, the_jumper);
      }
    else if ((format_length == 1) && (strncmp(format, "g", 1) == 0))
      {
        print_rational(to_convert, 10, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, TRUE, TRUE,
                'e', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "og", 2) == 0))
      {
        print_rational(to_convert, 8, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, TRUE, TRUE,
                'e', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "xg", 2) == 0))
      {
        print_rational(to_convert, 16, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, TRUE, TRUE,
                'x', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "Xg", 2) == 0))
      {
        print_rational(to_convert, 16, TRUE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, TRUE, TRUE,
                'x', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "cg", 2) == 0))
      {
        print_rational(to_convert, 10, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, TRUE, TRUE,
                'e', cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "cog", 3) == 0))
      {
        print_rational(to_convert, 8, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, TRUE, TRUE,
                'e', cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "cxg", 3) == 0))
      {
        print_rational(to_convert, 16, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, TRUE, TRUE,
                'x', cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "cXg", 3) == 0))
      {
        print_rational(to_convert, 16, TRUE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, TRUE, TRUE,
                'x', cp_data->location, the_jumper);
      }
    else if ((format_length == 1) && (strncmp(format, "G", 1) == 0))
      {
        print_rational(to_convert, 10, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, TRUE, TRUE,
                'E', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "oG", 2) == 0))
      {
        print_rational(to_convert, 8, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, TRUE, TRUE,
                'E', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "xG", 2) == 0))
      {
        print_rational(to_convert, 16, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, TRUE, TRUE,
                'X', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "XG", 2) == 0))
      {
        print_rational(to_convert, 16, TRUE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, FALSE, TRUE, TRUE,
                'X', cp_data->location, the_jumper);
      }
    else if ((format_length == 2) && (strncmp(format, "cG", 2) == 0))
      {
        print_rational(to_convert, 10, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, TRUE, TRUE,
                'E', cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "coG", 3) == 0))
      {
        print_rational(to_convert, 8, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, TRUE, TRUE,
                'E', cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "cxG", 3) == 0))
      {
        print_rational(to_convert, 16, FALSE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, TRUE, TRUE,
                'X', cp_data->location, the_jumper);
      }
    else if ((format_length == 3) && (strncmp(format, "cXG", 3) == 0))
      {
        print_rational(to_convert, 16, TRUE, printer, data, flag_plus,
                flag_space, flag_octothorp, have_zeros_position,
                zeros_position, zeros_character, precision, TRUE, TRUE, TRUE,
                'X', cp_data->location, the_jumper);
      }
    else if (((format_length == 1) && (strncmp(format, "v", 1) == 0)) ||
             (format_length == 0))
      {
        if (oi_out_of_memory(precision))
          {
            overload_print_value(to_convert, printer, data, cp_data, override);
          }
        else
          {
            size_t precision_size_t;
            verdict the_verdict;

            the_verdict = oi_magnitude_to_size_t(precision, &precision_size_t);
            if (the_verdict != MISSION_ACCOMPLISHED)
              {
                overload_print_value(to_convert, printer, data, cp_data,
                                     override);
              }
            else
              {
                string_buffer output_buffer;
                verdict the_verdict;
                string_printer_data sp_data;

                the_verdict = string_buffer_init(&output_buffer, 10);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    return;
                  }

                sp_data.output_buffer = &output_buffer;
                sp_data.cp_data.verdict = MISSION_ACCOMPLISHED;
                sp_data.cp_data.jumper = the_jumper;
                sp_data.cp_data.location = cp_data->location;
                string_print_value(to_convert, &string_printer, &sp_data);
                if (sp_data.cp_data.verdict != MISSION_ACCOMPLISHED)
                  {
                    free(output_buffer.array);
                    return;
                  }

                the_verdict = string_buffer_append(&output_buffer, 0);
                if (the_verdict != MISSION_ACCOMPLISHED)
                  {
                    jumper_do_abort(the_jumper);
                    free(output_buffer.array);
                    return;
                  }

                if (utf8_string_character_count(output_buffer.array) >
                    precision_size_t)
                  {
                    output_buffer.array[
                            bytes_for_utf8_character_count(output_buffer.array,
                                    precision_size_t)] = 0;
                  }

                (*printer)(data, "%s", output_buffer.array);
                free(output_buffer.array);
              }
          }
      }
    else
      {
        location_exception(the_jumper, cp_data->location,
                EXCEPTION_TAG(printf_bad_specifier),
                "When doing formatted printing, format specification string "
                "`%.*s' was not understood.", (int)format_length, format);
      }
  }

static void print_integer(value *to_convert, size_t base, boolean use_capitals,
        void (*printer)(void *data, const char *format, ...), void *data,
        boolean flag_plus, boolean flag_space, boolean flag_octothorp,
        boolean *have_zeros_position, size_t *zeros_position,
        char *zeros_character, o_integer precision, boolean complement,
        const source_location *location, jumper *the_jumper)
  {
    o_integer to_convert_oi;

    assert(to_convert != NULL);
    assert((base == 10) || (base == 8) || (base == 16));
    assert(printer != NULL);
    assert(the_jumper != NULL);

    if (get_value_kind(to_convert) != VK_INTEGER)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(printf_not_integer),
                "When doing formatted printing, an integer conversion "
                "specification was used with a non-integer value.");
        return;
      }

    to_convert_oi = integer_value_data(to_convert);
    assert(!(oi_out_of_memory(to_convert_oi)));

    do_print_oi(to_convert_oi, base, use_capitals, printer, data, flag_plus,
            flag_space, flag_octothorp, have_zeros_position, zeros_position,
            zeros_character, precision, complement, NULL, the_jumper);
  }

static void do_print_oi(o_integer to_convert, size_t base,
        boolean use_capitals,
        void (*printer)(void *data, const char *format, ...), void *data,
        boolean flag_plus, boolean flag_space, boolean flag_octothorp,
        boolean *have_zeros_position, size_t *zeros_position,
        char *zeros_character, o_integer precision, boolean complement,
        size_t *real_digit_count, jumper *the_jumper)
  {
    boolean is_negative;
    o_integer to_use_oi;
    boolean own_to_use;
    char pad_character;
    boolean extra_octal_flag_character;
    char *digits;
    size_t digit_count;

    assert(!(oi_out_of_memory(to_convert)));
    assert((base == 10) || (base == 8) || (base == 16));
    assert(printer != NULL);
    assert(the_jumper != NULL);

    switch (oi_kind(to_convert))
      {
        case IIK_FINITE:
            break;
        case IIK_POSITIVE_INFINITY:
            (*printer)(data, "+oo");
            if (real_digit_count != NULL)
                *real_digit_count = 0;
            return;
        case IIK_NEGATIVE_INFINITY:
            (*printer)(data, "-oo");
            if (real_digit_count != NULL)
                *real_digit_count = 0;
            return;
        case IIK_UNSIGNED_INFINITY:
            (*printer)(data, "1/0");
            if (real_digit_count != NULL)
                *real_digit_count = 0;
            return;
        case IIK_ZERO_ZERO:
            (*printer)(data, "0/0");
            if (real_digit_count != NULL)
                *real_digit_count = 0;
            return;
        default:
            assert(FALSE);
      }

    if (have_zeros_position != NULL)
        *have_zeros_position = TRUE;
    if (zeros_position != NULL)
        *zeros_position = 0;

    is_negative = oi_is_negative(to_convert);

    to_use_oi = to_convert;
    own_to_use = FALSE;

    if (is_negative)
      {
        (*printer)(data, "-");
        if (zeros_position != NULL)
            ++(*zeros_position);

        if (base != 10)
          {
            if (!complement)
              {
                oi_negate(to_use_oi, to_convert);
                if (oi_out_of_memory(to_use_oi))
                  {
                    jumper_do_abort(the_jumper);
                    return;
                  }
                is_negative = FALSE;
                own_to_use = TRUE;
              }
          }
        else
          {
            if (complement)
              {
                oi_add(to_use_oi, to_convert, oi_one);
                if (oi_out_of_memory(to_use_oi))
                  {
                    jumper_do_abort(the_jumper);
                    return;
                  }
                own_to_use = TRUE;
              }
          }
      }
    else if (flag_plus)
      {
        (*printer)(data, "+");
        if (zeros_position != NULL)
            ++(*zeros_position);
      }
    else if (flag_space)
      {
        (*printer)(data, " ");
        if (zeros_position != NULL)
            ++(*zeros_position);
      }

    if (flag_octothorp && (base == 16))
      {
        (*printer)(data, (use_capitals ? "0X" : "0x"));
        if (zeros_position != NULL)
            (*zeros_position) += 2;
      }

    pad_character = '0';
    extra_octal_flag_character = FALSE;

    if (base == 10)
      {
        verdict the_verdict;

        the_verdict = oi_decimal_digit_count(to_use_oi, &digit_count);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            if (own_to_use)
                oi_remove_reference(to_use_oi);
            return;
          }

        digits = MALLOC_ARRAY(char, digit_count + 1);
        if (digits == NULL)
          {
            jumper_do_abort(the_jumper);
            if (own_to_use)
                oi_remove_reference(to_use_oi);
            return;
          }

        the_verdict = oi_write_decimal_digits(to_use_oi, digits);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            free(digits);
            if (own_to_use)
                oi_remove_reference(to_use_oi);
            return;
          }

        if (is_negative && complement)
          {
            size_t digit_num;

            pad_character = '9';

            for (digit_num = 0; digit_num < digit_count; ++digit_num)
              {
                assert((digits[digit_num] >= '0') &&
                       (digits[digit_num] <= '9'));
                digits[digit_num] = ('9' - (digits[digit_num] - '0'));
              }
          }
      }
    else if (base == 16)
      {
        verdict the_verdict;

        if (is_negative)
            pad_character = (use_capitals ? 'F' : 'f');

        the_verdict = oi_hex_digits(to_use_oi, &digit_count);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            if (own_to_use)
                oi_remove_reference(to_use_oi);
            return;
          }

        digits = MALLOC_ARRAY(char, digit_count + 1);
        if (digits == NULL)
          {
            jumper_do_abort(the_jumper);
            if (own_to_use)
                oi_remove_reference(to_use_oi);
            return;
          }

        the_verdict = oi_write_hex_digits(to_use_oi, digits, use_capitals);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            free(digits);
            if (own_to_use)
                oi_remove_reference(to_use_oi);
            return;
          }
      }
    else
      {
        size_t hex_digit_count;
        verdict the_verdict;
        size_t block_count;
        char *octal_write_location;
        boolean non_zero_seen;
        size_t block_num;

        assert(base == 8);

        if (is_negative)
            pad_character = '7';

        the_verdict = oi_hex_digits(to_use_oi, &hex_digit_count);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            if (own_to_use)
                oi_remove_reference(to_use_oi);
            return;
          }

        block_count = ((hex_digit_count + 2) / 3);
        digit_count = (block_count * 4);
        assert(digit_count >= hex_digit_count);
        digits = MALLOC_ARRAY(char, digit_count + 2);
        if (digits == NULL)
          {
            jumper_do_abort(the_jumper);
            if (own_to_use)
                oi_remove_reference(to_use_oi);
            return;
          }

        the_verdict = oi_write_hex_digits(to_use_oi,
                &(digits[digit_count - hex_digit_count]), FALSE);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            free(digits);
            if (own_to_use)
                oi_remove_reference(to_use_oi);
            return;
          }

        octal_write_location = &(digits[0]);
        non_zero_seen = FALSE;

        for (block_num = 0; block_num < block_count; ++block_num)
          {
            size_t block_hex_end;
            unsigned long bits;
            size_t octal_digit_num;

            block_hex_end = ((block_count - (block_num + 1)) * 3);
            assert(block_hex_end <= ((block_count - 1) * 3));

            assert((block_hex_end + 1) <= hex_digit_count);
            assert(&(digits[digit_count - (block_hex_end + 1)]) >=
                   octal_write_location);
            bits = hex_digit_to_bits(
                    digits[digit_count - (block_hex_end + 1)]);

            if ((block_hex_end + 2) <= hex_digit_count)
              {
                assert(&(digits[digit_count - (block_hex_end + 2)]) >=
                       octal_write_location);
                bits |= (hex_digit_to_bits(
                                 digits[digit_count - (block_hex_end + 2)]) <<
                         4);
              }
            else if (is_negative)
              {
                bits |= (((unsigned long)0xf) << 4);
              }

            if ((block_hex_end + 3) <= hex_digit_count)
              {
                assert(&(digits[digit_count - (block_hex_end + 3)]) >=
                       octal_write_location);
                bits |= (hex_digit_to_bits(
                                 digits[digit_count - (block_hex_end + 3)]) <<
                         8);
              }
            else if (is_negative)
              {
                assert(&(digits[digit_count - (block_hex_end + 3)]) >=
                       octal_write_location);
                bits |= (((unsigned long)0xf) << 8);
              }

            for (octal_digit_num = 0; octal_digit_num < 4; ++octal_digit_num)
              {
                unsigned long digit_bits;

                digit_bits =
                        ((bits >> ((4 - (octal_digit_num + 1)) * 3)) & 0x7);
                if ((!non_zero_seen) &&
                    (digit_bits != (is_negative ? 0x7 : 0x0)))
                  {
                    non_zero_seen = TRUE;
                    if (flag_octothorp)
                      {
                        *octal_write_location = (is_negative ? '7' : '0');
                        ++octal_write_location;
                        assert((octal_write_location - digits) <
                               (digit_count + 1));
                        extra_octal_flag_character = TRUE;
                      }
                  }
                if (non_zero_seen)
                  {
                    *octal_write_location = (digit_bits + '0');
                    ++octal_write_location;
                    assert((octal_write_location - digits) <
                           (digit_count + 1));
                  }
              }
          }

        assert((octal_write_location - digits) < (digit_count + 1));
        digit_count = (octal_write_location - digits);
      }

    if (own_to_use)
        oi_remove_reference(to_use_oi);

    digits[digit_count] = 0;

    if (!(oi_out_of_memory(precision)))
      {
        do_padding(printer, data, precision, digit_count, pad_character,
                   the_jumper);
      }
    else if (digit_count == 0)
      {
        (*printer)(data, "%c", pad_character);
      }

    assert((!extra_octal_flag_character) || (digit_count >= 1));

    if (real_digit_count != NULL)
        *real_digit_count = digit_count - (extra_octal_flag_character ? 1 : 0);

    (*printer)(data, "%s", digits);
    free(digits);

    if (zeros_character != NULL)
        *zeros_character = pad_character;
  }

static void print_rational(value *to_convert, size_t base,
        boolean use_capitals,
        void (*printer)(void *data, const char *format, ...), void *data,
        boolean flag_plus, boolean flag_space, boolean flag_octothorp,
        boolean *have_zeros_position, size_t *zeros_position,
        char *zeros_character, o_integer precision, boolean complement,
        boolean decimal_point_format_allowed,
        boolean scientific_notation_allowed, char exponent_character,
        const source_location *location, jumper *the_jumper)
  {
    rational *whole_rational;
    o_integer new_precision;
    boolean own_new_precision;
    rational *mantissa;
    o_integer exponent;
    boolean use_scientific_notation;

    assert(to_convert != NULL);
    assert((base == 10) || (base == 8) || (base == 16));
    assert(printer != NULL);
    assert(the_jumper != NULL);

    switch (get_value_kind(to_convert))
      {
        case VK_INTEGER:
          {
            o_integer to_convert_oi;

            to_convert_oi = integer_value_data(to_convert);
            assert(!(oi_out_of_memory(to_convert_oi)));

            whole_rational = create_rational(to_convert_oi, oi_one);
            if (whole_rational == NULL)
              {
                jumper_do_abort(the_jumper);
                return;
              }

            break;
          }
        case VK_RATIONAL:
          {
            whole_rational = rational_value_data(to_convert);
            assert(whole_rational != NULL);
            rational_add_reference(whole_rational);

            break;
          }
        default:
          {
            location_exception(the_jumper, location,
                    EXCEPTION_TAG(printf_not_rational),
                    "When doing formatted printing, a rational conversion "
                    "specification was used with a non-rational value.");
            return;
          }
      }

    assert(whole_rational != NULL);

    if (rational_is_integer(whole_rational) &&
        (oi_kind(rational_numerator(whole_rational)) != IIK_FINITE))
      {
        do_print_oi(rational_numerator(whole_rational), base, use_capitals,
                printer, data, flag_plus, flag_space, flag_octothorp,
                have_zeros_position, zeros_position, zeros_character,
                precision, complement, NULL, the_jumper);

        rational_remove_reference(whole_rational);
        return;
      }

    if (!scientific_notation_allowed)
      {
        assert(decimal_point_format_allowed);

        print_in_decimal_point_format(whole_rational, base, use_capitals,
                printer, data, flag_plus, flag_space, flag_octothorp,
                have_zeros_position, zeros_position, zeros_character,
                precision, complement, !flag_octothorp, FALSE, FALSE, NULL,
                the_jumper, location);

        rational_remove_reference(whole_rational);
        return;
      }

    new_precision = precision;
    own_new_precision = FALSE;
    if (decimal_point_format_allowed)
      {
        boolean switch_to_one;

        if (oi_out_of_memory(precision))
            switch_to_one = TRUE;
        else
            switch_to_one = oi_equal(precision, oi_zero);

        if (switch_to_one)
          {
            new_precision = oi_one;
            if (oi_out_of_memory(new_precision))
              {
                jumper_do_abort(the_jumper);
                rational_remove_reference(whole_rational);
                return;
              }
            oi_add_reference(new_precision);
            own_new_precision = TRUE;
          }
      }

    mantissa = find_mantissa_and_exponent(whole_rational, base, &exponent);
    if (mantissa == NULL)
      {
        jumper_do_abort(the_jumper);
        if (own_new_precision)
            oi_remove_reference(new_precision);
        rational_remove_reference(whole_rational);
        return;
      }
    assert(!(oi_out_of_memory(exponent)));

    if (!decimal_point_format_allowed)
      {
        use_scientific_notation = TRUE;
      }
    else
      {
        o_integer minus_5_oi;
        boolean less_than_minus_4;
        boolean doubt;

        oi_create_from_long_int(minus_5_oi, -5);
        if (oi_out_of_memory(minus_5_oi))
          {
            jumper_do_abort(the_jumper);
            rational_remove_reference(mantissa);
            oi_remove_reference(exponent);
            if (own_new_precision)
                oi_remove_reference(new_precision);
            rational_remove_reference(whole_rational);
            return;
          }

        less_than_minus_4 = !(oi_less_than(minus_5_oi, exponent));
        doubt = oi_equal(minus_5_oi, exponent);
        oi_remove_reference(minus_5_oi);

        if (less_than_minus_4)
          {
            use_scientific_notation = TRUE;
          }
        else
          {
            assert(!doubt);
            use_scientific_notation = !(oi_less_than(exponent, new_precision));
            doubt = TRUE;
          }

        if (doubt)
          {
            o_integer munged_precision;
            boolean overflow;

            oi_subtract(munged_precision, new_precision, oi_one);
            if (oi_out_of_memory(munged_precision))
              {
                jumper_do_abort(the_jumper);
                rational_remove_reference(mantissa);
                oi_remove_reference(exponent);
                if (own_new_precision)
                    oi_remove_reference(new_precision);
                rational_remove_reference(whole_rational);
                return;
              }

            if (less_than_minus_4 || (oi_equal(exponent, munged_precision)))
              {
                assert(oi_kind(munged_precision) == IIK_FINITE);
                assert(!(oi_is_negative(munged_precision)));
                print_in_decimal_point_format(mantissa, base, use_capitals,
                        &null_printer, NULL, flag_plus, flag_space,
                        flag_octothorp, NULL, NULL, zeros_character,
                        munged_precision, complement, !flag_octothorp,
                        !flag_octothorp, TRUE, &overflow, the_jumper,
                        location);
                oi_remove_reference(munged_precision);
                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    jumper_do_abort(the_jumper);
                    rational_remove_reference(mantissa);
                    oi_remove_reference(exponent);
                    if (own_new_precision)
                        oi_remove_reference(new_precision);
                    rational_remove_reference(whole_rational);
                    return;
                  }

                if (overflow)
                  {
                    use_scientific_notation = !use_scientific_notation;
                  }
              }
            else
              {
                oi_remove_reference(munged_precision);
              }
          }
      }

    if (!use_scientific_notation)
      {
        o_integer diff;
        o_integer munged_precision;
        boolean overflow;

        rational_remove_reference(mantissa);

        oi_subtract(diff, new_precision, exponent);
        oi_remove_reference(exponent);
        if (oi_out_of_memory(diff))
          {
            jumper_do_abort(the_jumper);
            if (own_new_precision)
                oi_remove_reference(new_precision);
            rational_remove_reference(whole_rational);
            return;
          }

        oi_subtract(munged_precision, diff, oi_one);
        oi_remove_reference(diff);
        if (oi_out_of_memory(munged_precision))
          {
            jumper_do_abort(the_jumper);
            if (own_new_precision)
                oi_remove_reference(new_precision);
            rational_remove_reference(whole_rational);
            return;
          }
        assert(oi_kind(munged_precision) == IIK_FINITE);
        assert(!(oi_is_negative(munged_precision)));

        print_in_decimal_point_format(whole_rational, base, use_capitals,
                printer, data, flag_plus, flag_space, flag_octothorp,
                have_zeros_position, zeros_position, zeros_character,
                munged_precision, complement, !flag_octothorp, !flag_octothorp,
                FALSE, &overflow, the_jumper, location);

        oi_remove_reference(munged_precision);
        rational_remove_reference(whole_rational);
      }
    else
      {
        boolean overflow;
        o_integer two_oi;

        rational_remove_reference(whole_rational);

        if (decimal_point_format_allowed)
          {
            o_integer munged_precision;

            oi_subtract(munged_precision, new_precision, oi_one);
            if (own_new_precision)
                oi_remove_reference(new_precision);
            if (oi_out_of_memory(munged_precision))
              {
                jumper_do_abort(the_jumper);
                oi_remove_reference(exponent);
                rational_remove_reference(mantissa);
                return;
              }
            assert(oi_kind(munged_precision) == IIK_FINITE);
            assert(!(oi_is_negative(munged_precision)));
            new_precision = munged_precision;
            own_new_precision = TRUE;
          }

        print_in_decimal_point_format(mantissa, base, use_capitals, printer,
                data, flag_plus, flag_space, flag_octothorp,
                have_zeros_position, zeros_position, zeros_character,
                new_precision, complement, !flag_octothorp,
                (decimal_point_format_allowed && !flag_octothorp), TRUE,
                &overflow, the_jumper, location);
        rational_remove_reference(mantissa);
        if (!(jumper_flowing_forward(the_jumper)))
          {
            oi_remove_reference(exponent);
            if (own_new_precision)
                oi_remove_reference(new_precision);
            return;
          }

        if (overflow)
          {
            o_integer new_exponent;

            oi_add(new_exponent, exponent, oi_one);
            oi_remove_reference(exponent);
            if (oi_out_of_memory(new_exponent))
              {
                jumper_do_abort(the_jumper);
                if (own_new_precision)
                    oi_remove_reference(new_precision);
                return;
              }
            exponent = new_exponent;
          }

        (*printer)(data, "%c", exponent_character);

        oi_create_from_size_t(two_oi, 2);
        if (oi_out_of_memory(two_oi))
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(exponent);
            if (own_new_precision)
                oi_remove_reference(new_precision);
            return;
          }

        do_print_oi(exponent, base, use_capitals, printer, data, TRUE, FALSE,
                FALSE, NULL, NULL, NULL, two_oi, complement, NULL, the_jumper);
        oi_remove_reference(two_oi);
        oi_remove_reference(exponent);
      }

    if (own_new_precision)
        oi_remove_reference(new_precision);
  }

static void print_in_decimal_point_format(rational *to_print, size_t base,
        boolean use_capitals,
        void (*printer)(void *data, const char *format, ...), void *data,
        boolean flag_plus, boolean flag_space, boolean flag_octothorp,
        boolean *have_zeros_position, size_t *zeros_position,
        char *zeros_character, o_integer precision, boolean complement,
        boolean omit_trailing_point, boolean omit_trailing_zeros,
        boolean exactly_one_pre_point, boolean *overflow, jumper *the_jumper,
        const source_location *location)
  {
    o_integer new_precision;
    o_integer precision_plus_one;
    o_integer base_oi;
    o_integer factor;
    o_integer start_numerator;
    o_integer start_denominator;
    o_integer scaled_denominator;
    o_integer scaled_numerator;
    long int signed_half_base;
    o_integer half_base;
    o_integer to_add;
    o_integer rounded;
    o_integer remainder;
    o_integer final;
    string_buffer output_buffer;
    verdict the_verdict;
    string_printer_data sp_data;
    size_t real_digit_count;
    size_t precision_size_t;
    size_t characters_before_point;
    char save_character;

    assert(to_print != NULL);
    assert((base == 10) || (base == 8) || (base == 16));
    assert(printer != NULL);
    assert(the_jumper != NULL);

    if (overflow != NULL)
        *overflow = FALSE;

    if (!(oi_out_of_memory(precision)))
      {
        new_precision = precision;
        oi_add_reference(new_precision);
      }
    else
      {
        oi_create_from_size_t(new_precision, 6);
        if (oi_out_of_memory(new_precision))
          {
            jumper_do_abort(the_jumper);
            return;
          }
      }

    oi_add(precision_plus_one, new_precision, oi_one);
    oi_remove_reference(new_precision);
    if (oi_out_of_memory(precision_plus_one))
      {
        jumper_do_abort(the_jumper);
        return;
      }

    oi_create_from_size_t(base_oi, base);
    if (oi_out_of_memory(base_oi))
      {
        jumper_do_abort(the_jumper);
        oi_remove_reference(precision_plus_one);
        return;
      }

    factor = oi_power(base_oi, precision_plus_one);
    if (oi_out_of_memory(factor))
      {
        jumper_do_abort(the_jumper);
        oi_remove_reference(base_oi);
        oi_remove_reference(precision_plus_one);
        return;
      }

    start_numerator = rational_numerator(to_print);
    assert(!(oi_out_of_memory(start_numerator)));
    assert(oi_kind(start_numerator) == IIK_FINITE);

    start_denominator = rational_denominator(to_print);
    assert(!(oi_out_of_memory(start_denominator)));
    assert(oi_kind(start_denominator) == IIK_FINITE);

    oi_multiply(scaled_denominator, start_denominator, base_oi);
    oi_remove_reference(base_oi);
    if (oi_out_of_memory(scaled_denominator))
      {
        jumper_do_abort(the_jumper);
        oi_remove_reference(factor);
        oi_remove_reference(precision_plus_one);
        return;
      }

    oi_multiply(scaled_numerator, start_numerator, factor);
    oi_remove_reference(factor);
    if (oi_out_of_memory(scaled_denominator))
      {
        jumper_do_abort(the_jumper);
        oi_remove_reference(scaled_denominator);
        oi_remove_reference(precision_plus_one);
        return;
      }

    signed_half_base = (long int)(base / 2);
    if (oi_is_negative(start_numerator))
        signed_half_base = -signed_half_base;

    oi_create_from_long_int(half_base, signed_half_base);
    if (oi_out_of_memory(half_base))
      {
        jumper_do_abort(the_jumper);
        oi_remove_reference(scaled_numerator);
        oi_remove_reference(scaled_denominator);
        oi_remove_reference(precision_plus_one);
        return;
      }

    oi_multiply(to_add, half_base, start_denominator);
    oi_remove_reference(half_base);
    if (oi_out_of_memory(to_add))
      {
        jumper_do_abort(the_jumper);
        oi_remove_reference(scaled_numerator);
        oi_remove_reference(scaled_denominator);
        oi_remove_reference(precision_plus_one);
        return;
      }

    oi_add(rounded, scaled_numerator, to_add);
    oi_remove_reference(scaled_numerator);
    oi_remove_reference(to_add);
    if (oi_out_of_memory(rounded))
      {
        jumper_do_abort(the_jumper);
        oi_remove_reference(scaled_denominator);
        oi_remove_reference(precision_plus_one);
        return;
      }

    oi_divide(final, rounded, scaled_denominator, &remainder);
    oi_remove_reference(rounded);
    oi_remove_reference(scaled_denominator);
    if (oi_out_of_memory(final))
      {
        jumper_do_abort(the_jumper);
        oi_remove_reference(precision_plus_one);
        return;
      }

    if (complement && oi_is_negative(final) && oi_equal(remainder, oi_zero))
      {
        o_integer new_final;

        oi_add(new_final, final, oi_one);
        oi_remove_reference(final);
        if (oi_out_of_memory(new_final))
          {
            jumper_do_abort(the_jumper);
            oi_remove_reference(remainder);
            oi_remove_reference(precision_plus_one);
            return;
          }

        final = new_final;
      }

    oi_remove_reference(remainder);

    the_verdict = string_buffer_init(&output_buffer, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        oi_remove_reference(final);
        oi_remove_reference(precision_plus_one);
        return;
      }

    sp_data.output_buffer = &output_buffer;
    sp_data.cp_data.verdict = MISSION_ACCOMPLISHED;
    sp_data.cp_data.jumper = the_jumper;
    sp_data.cp_data.location = location;
    do_print_oi(final, base, use_capitals, &string_printer, &sp_data,
            flag_plus, flag_space, flag_octothorp, have_zeros_position,
            zeros_position, zeros_character, precision_plus_one, complement,
            &real_digit_count, the_jumper);
    oi_remove_reference(final);
    oi_remove_reference(precision_plus_one);
    if (sp_data.cp_data.verdict != MISSION_ACCOMPLISHED)
      {
        free(output_buffer.array);
        return;
      }

    the_verdict = string_buffer_append(&output_buffer, 0);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        free(output_buffer.array);
        return;
      }

    if (!(oi_out_of_memory(precision)))
      {
        verdict the_verdict;

        the_verdict = oi_magnitude_to_size_t(precision, &precision_size_t);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            jumper_do_abort(the_jumper);
            free(output_buffer.array);
            return;
          }
      }
    else
      {
        precision_size_t = 6;
      }

    assert(output_buffer.element_count >= (precision_size_t + 2));

    if (exactly_one_pre_point && (real_digit_count > precision_size_t + 1))
      {
        --(output_buffer.element_count);
        output_buffer.array[output_buffer.element_count - 1] = 0;
        if (overflow != NULL)
            *overflow = TRUE;
      }

    characters_before_point =
            (output_buffer.element_count - (precision_size_t + 1));

    if (omit_trailing_zeros)
      {
        size_t end_position;

        end_position = (output_buffer.element_count - 1);
        while ((end_position > characters_before_point) &&
               (output_buffer.array[end_position - 1] == '0'))
          {
            --end_position;
          }
        output_buffer.array[end_position] = 0;
      }

    save_character = output_buffer.array[characters_before_point];
    output_buffer.array[characters_before_point] = 0;
    (*printer)(data, "%s", output_buffer.array);
    output_buffer.array[characters_before_point] = save_character;

    if ((!omit_trailing_point) || (save_character != 0))
        (*printer)(data, ".");

    (*printer)(data, "%s", &(output_buffer.array[characters_before_point]));
    free(output_buffer.array);
  }

static void string_print_value(value *the_value,
        void (*printer)(void *data, const char *format, ...), void *data)
  {
    string_printer_data *sp_data;
    call_printer_data *cp_data;

    assert(the_value != NULL);
    assert(printer != NULL);
    assert(data != NULL);

    sp_data = (string_printer_data *)data;
    cp_data = &(sp_data->cp_data);

    overload_print_value(the_value, printer, data, cp_data,
                         &string_print_value);
  }

static void string_printer(void *data, const char *format, ...)
  {
    string_printer_data *sp_data;
    va_list ap;
    string_buffer *output_buffer;
    size_t element_count;
    int return_value;

    assert(data != NULL);

    sp_data = (string_printer_data *)data;

    if (sp_data->cp_data.verdict != MISSION_ACCOMPLISHED)
        return;

    va_start(ap, format);

    output_buffer = sp_data->output_buffer;
    assert(output_buffer != NULL);
    element_count = output_buffer->element_count;

    return_value = vbuffer_printf(output_buffer, element_count, format, ap);

    if (return_value < 0)
      {
        sp_data->cp_data.verdict = MISSION_FAILED;
        jumper_do_abort(sp_data->cp_data.jumper);
      }
    else
      {
        assert(output_buffer->element_count ==
               (element_count + return_value + 1));
        assert(output_buffer->array[element_count + return_value] == 0);
        output_buffer->element_count = (element_count + return_value);
      }

    va_end(ap);
  }

static void fp_call_print_value(value *the_value,
        void (*printer)(void *data, const char *format, ...), void *data)
  {
    fp_call_printer_data *fp_data;
    call_printer_data *cp_data;

    assert(the_value != NULL);
    assert(printer != NULL);
    assert(data != NULL);

    fp_data = (fp_call_printer_data *)data;
    cp_data = &(fp_data->cp_data);

    overload_print_value(the_value, printer, data, cp_data,
                         &fp_call_print_value);
  }

static void fp_call_printer(void *data, const char *format, ...)
  {
    fp_call_printer_data *fp_data;
    va_list ap;
    int return_value;

    assert(data != NULL);

    fp_data = (fp_call_printer_data *)data;

    if (fp_data->cp_data.verdict != MISSION_ACCOMPLISHED)
        return;

    va_start(ap, format);

    if (fp_data->utf_format == UTF_8)
      {
        return_value = vfprintf(fp_data->fp, format, ap);

        if (return_value < 0)
          {
            fp_data->cp_data.verdict = MISSION_FAILED;
            location_exception(fp_data->cp_data.jumper,
                    fp_data->cp_data.location, EXCEPTION_TAG(file_io_failure),
                    "Failed trying to write to %s: %s.",
                    fp_data->file_description, strerror(errno));
          }
      }
    else
      {
        string_buffer output_buffer;
        verdict the_verdict;

        the_verdict = string_buffer_init(&output_buffer, 10);
        if (the_verdict != MISSION_ACCOMPLISHED)
          {
            va_end(ap);
            fp_data->cp_data.verdict = MISSION_FAILED;
            jumper_do_abort(fp_data->cp_data.jumper);
            return;
          }

        return_value = vbuffer_printf(&output_buffer, 0, format, ap);
        if (return_value < 0)
          {
            va_end(ap);
            free(output_buffer.array);
            fp_data->cp_data.verdict = MISSION_FAILED;
            jumper_do_abort(fp_data->cp_data.jumper);
            return;
          }

        if ((fp_data->utf_format == UTF_16_LE) ||
            (fp_data->utf_format == UTF_16_BE))
          {
            unsigned *u16_buffer;
            const char *follow_u8;
            unsigned *follow_u16;
            size_t unit_count;
            unsigned char *u8_buffer;
            size_t unit_num;
            size_t units_written;

            u16_buffer =
                    MALLOC_ARRAY(unsigned, output_buffer.element_count + 1);
            if (u16_buffer == NULL)
              {
                va_end(ap);
                free(output_buffer.array);
                fp_data->cp_data.verdict = MISSION_FAILED;
                jumper_do_abort(fp_data->cp_data.jumper);
                return;
              }

            follow_u8 = output_buffer.array;
            follow_u16 = u16_buffer;

            while (*follow_u8 != 0)
              {
                int u8_count;
                unsigned long code_point;
                size_t u16_count;

                u8_count = validate_utf8_character(follow_u8);
                assert(u8_count > 0);
                assert((follow_u8 - output_buffer.array) + u8_count <=
                       output_buffer.element_count);

                code_point = utf8_to_code_point(follow_u8);
                u16_count = code_point_to_utf16(code_point, follow_u16);
                assert(u16_count > 0);
                assert((follow_u16 - u16_buffer) + u16_count <=
                       output_buffer.element_count);

                follow_u8 += u8_count;
                follow_u16 += u16_count;
              }

            unit_count = follow_u16 - u16_buffer;
            u8_buffer = MALLOC_ARRAY(unsigned char,
                    ((unit_count > 0) ? (unit_count * 2) : 1));
            if (u8_buffer == NULL)
              {
                va_end(ap);
                free(u16_buffer);
                free(output_buffer.array);
                fp_data->cp_data.verdict = MISSION_FAILED;
                jumper_do_abort(fp_data->cp_data.jumper);
                return;
              }

            for (unit_num = 0; unit_num < unit_count; ++unit_num)
              {
                if (fp_data->utf_format == UTF_16_LE)
                  {
                    u8_buffer[(unit_num * 2) + 0] =
                            (u16_buffer[unit_num] & 0xff);
                    u8_buffer[(unit_num * 2) + 1] =
                            (u16_buffer[unit_num] >> 8);
                  }
                else
                  {
                    assert(fp_data->utf_format == UTF_16_BE);
                    u8_buffer[(unit_num * 2) + 0] =
                            (u16_buffer[unit_num] >> 8);
                    u8_buffer[(unit_num * 2) + 1] =
                            (u16_buffer[unit_num] & 0xff);
                  }
              }

            free(u16_buffer);

            units_written = fwrite(u8_buffer, 2, unit_count, fp_data->fp);
            free(u8_buffer);
            if (units_written < unit_count)
              {
                fp_data->cp_data.verdict = MISSION_FAILED;
                location_exception(fp_data->cp_data.jumper,
                        fp_data->cp_data.location,
                        EXCEPTION_TAG(file_io_failure),
                        "Failed trying to write to %s: %s.",
                        fp_data->file_description, strerror(errno));
              }
          }
        else
          {
            unsigned long *u32_buffer;
            const char *follow_u8;
            unsigned long *follow_u32;
            size_t unit_count;
            unsigned char *u8_buffer;
            size_t unit_num;
            size_t units_written;

            assert((fp_data->utf_format == UTF_32_LE) ||
                   (fp_data->utf_format == UTF_32_BE));

            u32_buffer = MALLOC_ARRAY(unsigned long,
                                      output_buffer.element_count + 1);
            if (u32_buffer == NULL)
              {
                va_end(ap);
                free(output_buffer.array);
                fp_data->cp_data.verdict = MISSION_FAILED;
                jumper_do_abort(fp_data->cp_data.jumper);
                return;
              }

            follow_u8 = output_buffer.array;
            follow_u32 = u32_buffer;

            while (*follow_u8 != 0)
              {
                int u8_count;

                u8_count = validate_utf8_character(follow_u8);
                assert(u8_count > 0);
                assert((follow_u8 - output_buffer.array) + u8_count <=
                       output_buffer.element_count);

                *follow_u32 = utf8_to_code_point(follow_u8); 
                assert((follow_u32 - u32_buffer) + 1 <=
                       output_buffer.element_count);

                follow_u8 += u8_count;
                ++follow_u32;
              }

            unit_count = follow_u32 - u32_buffer;
            u8_buffer = MALLOC_ARRAY(unsigned char,
                    ((unit_count > 0) ? (unit_count * 4) : 1));
            if (u8_buffer == NULL)
              {
                va_end(ap);
                free(u32_buffer);
                free(output_buffer.array);
                fp_data->cp_data.verdict = MISSION_FAILED;
                jumper_do_abort(fp_data->cp_data.jumper);
                return;
              }

            for (unit_num = 0; unit_num < unit_count; ++unit_num)
              {
                if (fp_data->utf_format == UTF_32_LE)
                  {
                    u8_buffer[(unit_num * 4) + 0] =
                            ((u32_buffer[unit_num] >> 0) & 0xff);
                    u8_buffer[(unit_num * 4) + 1] =
                            ((u32_buffer[unit_num] >> 8) & 0xff);
                    u8_buffer[(unit_num * 4) + 2] =
                            ((u32_buffer[unit_num] >> 16) & 0xff);
                    u8_buffer[(unit_num * 4) + 3] =
                            ((u32_buffer[unit_num] >> 24) & 0xff);
                  }
                else
                  {
                    assert(fp_data->utf_format == UTF_32_BE);
                    u8_buffer[(unit_num * 4) + 0] =
                            ((u32_buffer[unit_num] >> 24) & 0xff);
                    u8_buffer[(unit_num * 4) + 1] =
                            ((u32_buffer[unit_num] >> 16) & 0xff);
                    u8_buffer[(unit_num * 4) + 2] =
                            ((u32_buffer[unit_num] >> 8) & 0xff);
                    u8_buffer[(unit_num * 4) + 3] =
                            ((u32_buffer[unit_num] >> 0) & 0xff);
                  }
              }

            free(u32_buffer);

            units_written = fwrite(u8_buffer, 4, unit_count, fp_data->fp);
            free(u8_buffer);
            if (units_written < unit_count)
              {
                fp_data->cp_data.verdict = MISSION_FAILED;
                location_exception(fp_data->cp_data.jumper,
                        fp_data->cp_data.location,
                        EXCEPTION_TAG(file_io_failure),
                        "Failed trying to write to %s: %s.",
                        fp_data->file_description, strerror(errno));
              }
          }

        free(output_buffer.array);
      }

    va_end(ap);
  }

static void null_printer(void *data, const char *format, ...)
  {
    /* empty */
  }

static utf_choice utf_choice_from_value(value *format_value)
  {
    quark *the_quark;
    quark_declaration *declaration;
    const char *quark_name;

    assert(format_value != NULL);

    assert(get_value_kind(format_value) == VK_QUARK);
    the_quark = value_quark(format_value);
    assert(the_quark != NULL);

    declaration = quark_instance_declaration(the_quark);
    assert(declaration != NULL);

    quark_name = quark_declaration_name(declaration);
    assert(quark_name != NULL);

    if (strcmp(quark_name, "utf_8") == 0)
        return UTF_8;
    else if (strcmp(quark_name, "utf_16_le") == 0)
        return UTF_16_LE;
    else if (strcmp(quark_name, "utf_16_be") == 0)
        return UTF_16_BE;
    else if (strcmp(quark_name, "utf_32_le") == 0)
        return UTF_32_LE;
    else if (strcmp(quark_name, "utf_32_be") == 0)
        return UTF_32_BE;

    assert(FALSE);
    return UTF_8;
  }

static utf_choice utf_choice_from_endian_value(value *which_endian_value)
  {
    quark *the_quark;
    quark_declaration *declaration;
    const char *quark_name;

    assert(which_endian_value != NULL);

    assert(get_value_kind(which_endian_value) == VK_QUARK);
    the_quark = value_quark(which_endian_value);
    assert(the_quark != NULL);

    declaration = quark_instance_declaration(the_quark);
    assert(declaration != NULL);

    quark_name = quark_declaration_name(declaration);
    assert(quark_name != NULL);

    if (strcmp(quark_name, "little_endian") == 0)
        return UTF_32_LE;
    else if (strcmp(quark_name, "big_endian") == 0)
        return UTF_32_BE;

    assert(FALSE);
    return UTF_32_LE;
  }

static boolean boolean_from_value(value *the_value)
  {
    assert(the_value != NULL);

    if (get_value_kind(the_value) == VK_TRUE)
        return TRUE;
    else if (get_value_kind(the_value) == VK_FALSE)
        return FALSE;

    assert(FALSE);
    return FALSE;
  }

static value *vcreate_io_error_value(lepton_key_instance *io_error_key,
        jumper *the_jumper, const char *message, va_list ap)
  {
    string_buffer message_buffer;
    verdict the_verdict;
    int return_code;
    value *result;
    value *message_value;

    the_verdict = string_buffer_init(&message_buffer, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    return_code = vbuffer_printf(&message_buffer, 0, message, ap);

    if (return_code < 0)
      {
        jumper_do_abort(the_jumper);
        free(message_buffer.array);
        return NULL;
      }

    result = create_lepton_value(io_error_key);
    if (result == NULL)
      {
        jumper_do_abort(the_jumper);
        free(message_buffer.array);
        return NULL;
      }

    message_value = create_string_value(message_buffer.array);
    free(message_buffer.array);
    if (message_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    the_verdict = add_field(result, "message", message_value);
    value_remove_reference(message_value, the_jumper);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(result, the_jumper);
        return NULL;
      }

    return result;
  }

static void set_fp_object_error_flag_if_needed(object *the_object, FILE *fp,
        lepton_key_instance *io_error_key, jumper *the_jumper)
  {
    assert(the_object != NULL);
    assert(fp != NULL);
    assert(the_jumper != NULL);

    if (ferror(fp))
      {
        set_fp_object_error_info(the_object, io_error_key, the_jumper,
                                 "I/O error.");
      }
  }

static void set_fp_object_eof_and_error_flags_if_needed(object *the_object,
        FILE *fp, lepton_key_instance *io_error_key, jumper *the_jumper)
  {
    assert(the_object != NULL);
    assert(fp != NULL);
    assert(the_jumper != NULL);

    if (feof(fp))
      {
        set_fp_object_eof_info(the_object, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return;
      }

    set_fp_object_error_flag_if_needed(the_object, fp, io_error_key,
                                       the_jumper);
  }

static void object_set_variable_field(object *the_object,
        const char *field_name, value *new_value, jumper *the_jumper)
  {
    size_t field_num;
    variable_instance *instance;

    assert(the_object != NULL);
    assert(field_name != NULL);
    assert(new_value != NULL);
    assert(the_jumper != NULL);

    assert(!(object_is_closed(the_object))); /* VERIFICATION NEEDED */
    field_num = object_field_lookup(the_object, field_name);
    assert(field_num < object_field_count(the_object));

    assert(!(object_is_closed(the_object))); /* VERIFICATION NEEDED */
    assert(object_field_is_variable(the_object, field_num));
    instance = object_field_variable(the_object, field_num);
    assert(instance != NULL);

    assert(!(variable_instance_scope_exited(instance)));
            /* VERIFICATION NEEDED */
    set_variable_instance_value(instance, new_value, the_jumper);
  }

static void set_fp_object_error_info(object *the_object,
        lepton_key_instance *io_error_key, jumper *the_jumper,
        const char *format, ...)
  {
    va_list ap;

    va_start(ap, format);
    vset_fp_object_error_info(the_object, io_error_key, the_jumper, format,
                              ap);
    va_end(ap);
  }

static void set_fp_object_error_info_and_do_exception(object *the_object,
        lepton_key_instance *io_error_key, jumper *the_jumper,
        const source_location *location, const char *format, ...)
  {
    va_list ap;
    value *error_value;

    assert(the_object != NULL);
    assert(io_error_key != NULL);
    assert(the_jumper != NULL);
    assert(format != NULL);

    va_start(ap, format);
    error_value = vset_fp_object_error_info_return_error_value(the_object,
            io_error_key, the_jumper, format, ap);
    va_end(ap);

    if (error_value == NULL)
      {
        assert(!(jumper_flowing_forward(the_jumper)));
        return;
      }

    assert(jumper_flowing_forward(the_jumper));

    location_exception(the_jumper, location, EXCEPTION_TAG(file_io_failure),
            "%s", string_value_data(value_get_field("message", error_value)));
    value_remove_reference(error_value, the_jumper);
  }

static void vset_fp_object_error_info(object *the_object,
        lepton_key_instance *io_error_key, jumper *the_jumper,
        const char *format, va_list ap)
  {
    value *result;

    result = vset_fp_object_error_info_return_error_value(the_object,
            io_error_key, the_jumper, format, ap);
    if (result != NULL)
        value_remove_reference(result, the_jumper);
  }

static value *vset_fp_object_error_info_return_error_value(object *the_object,
        lepton_key_instance *io_error_key, jumper *the_jumper,
        const char *format, va_list ap)
  {
    value *error_value;
    value *true_value;

    error_value = vcreate_io_error_value(io_error_key, the_jumper, format, ap);
    if (error_value == NULL)
      {
        assert(!(jumper_flowing_forward(the_jumper)));
        return NULL;
      }

    object_set_variable_field(the_object, "which_error", error_value,
                              the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(error_value, the_jumper);
        return NULL;
      }

    true_value = create_true_value();
    if (true_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(error_value, the_jumper);
        return NULL;
      }

    object_set_variable_field(the_object, "have_error", true_value,
                              the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(error_value, the_jumper);
        return NULL;
      }

    object_set_variable_field(the_object, "is_end_of_input", true_value,
                              the_jumper);
    value_remove_reference(true_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(error_value, the_jumper);
        return NULL;
      }

    return error_value;
  }

static void set_fp_object_eof_info(object *the_object, jumper *the_jumper)
  {
    value *true_value;

    true_value = create_true_value();
    if (true_value == NULL)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    object_set_variable_field(the_object, "is_end_of_input", true_value,
                              the_jumper);
    value_remove_reference(true_value, the_jumper);
  }

static void exception_and_fp_error(object *the_object,
        lepton_key_instance *io_error_key, jumper *the_jumper,
        const source_location *location, static_exception_tag *tag,
        const char *format, ...)
  {
    va_list ap;

    va_start(ap, format);
    vset_fp_object_error_info(the_object, io_error_key, the_jumper, format,
                              ap);
    if (jumper_flowing_forward(the_jumper))
        vlocation_exception(the_jumper, location, tag, format, ap);
    va_end(ap);
  }

static void tell_error_handler(void *data, const char *format, ...)
  {
    call_printer_data *cp_data;
    va_list ap;

    assert(data != NULL);
    assert(format != NULL);

    cp_data = (call_printer_data *)data;

    va_start(ap, format);
    vlocation_exception(cp_data->jumper, cp_data->location,
                        EXCEPTION_TAG(file_tell_failed), format, ap);
    va_end(ap);
  }

static void seek_error_handler(void *data, const char *format, ...)
  {
    call_printer_data *cp_data;
    va_list ap;

    assert(data != NULL);
    assert(format != NULL);

    cp_data = (call_printer_data *)data;

    va_start(ap, format);
    vlocation_exception(cp_data->jumper, cp_data->location,
                        EXCEPTION_TAG(file_seek_failed), format, ap);
    va_end(ap);
  }

static fp_call_printer_data *fp_data_for_argument(value *all_arguments_value,
                                                  size_t argument_number)
  {
    value *object_value;
    object *the_object;

    assert(all_arguments_value != NULL);
    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(argument_number < value_component_count(all_arguments_value));

    object_value = value_component_value(all_arguments_value, argument_number);
    assert(object_value != NULL);

    assert(get_value_kind(object_value) == VK_OBJECT);
    the_object = object_value_data(object_value);
    assert(the_object != NULL);

    assert(!(object_is_closed(the_object))); /* VERIFICATION NEEDED */
    return (fp_call_printer_data *)(object_hook(the_object));
  }

static const char *string_for_argument(value *all_arguments_value,
                                       size_t argument_number)
  {
    value *string_value;
    const char *string_chars;

    assert(all_arguments_value != NULL);
    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(argument_number < value_component_count(all_arguments_value));

    string_value = value_component_value(all_arguments_value, argument_number);
    assert(string_value != NULL);

    assert(get_value_kind(string_value) == VK_STRING);
    string_chars = string_value_data(string_value);
    assert(string_chars != NULL);

    return string_chars;
  }

static o_integer integer_for_argument(value *all_arguments_value,
                                      size_t argument_number)
  {
    value *integer_value;
    o_integer result;

    assert(all_arguments_value != NULL);
    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(argument_number < value_component_count(all_arguments_value));

    integer_value =
            value_component_value(all_arguments_value, argument_number);
    assert(integer_value != NULL);

    assert(get_value_kind(integer_value) == VK_INTEGER);
    result = integer_value_data(integer_value);
    assert(!(oi_out_of_memory(result)));

    return result;
  }

static object *object_for_argument(value *all_arguments_value,
                                   size_t argument_number)
  {
    value *object_value;
    object *result;

    assert(all_arguments_value != NULL);
    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(argument_number < value_component_count(all_arguments_value));

    object_value = value_component_value(all_arguments_value, argument_number);
    assert(object_value != NULL);

    assert(get_value_kind(object_value) == VK_OBJECT);
    result = object_value_data(object_value);
    assert(result != NULL);

    return result;
  }

static regular_expression *regular_expression_for_argument(
        value *all_arguments_value, size_t argument_number)
  {
    value *regular_expression_value;
    regular_expression *result;

    assert(all_arguments_value != NULL);
    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(argument_number < value_component_count(all_arguments_value));

    regular_expression_value =
            value_component_value(all_arguments_value, argument_number);
    assert(regular_expression_value != NULL);

    assert(get_value_kind(regular_expression_value) == VK_REGULAR_EXPRESSION);
    result = regular_expression_value_data(regular_expression_value);
    assert(result != NULL);

    return result;
  }

static type *type_for_argument(value *all_arguments_value,
                               size_t argument_number)
  {
    value *type_value;
    type *result;

    assert(all_arguments_value != NULL);
    assert(get_value_kind(all_arguments_value) == VK_SEMI_LABELED_VALUE_LIST);
    assert(argument_number < value_component_count(all_arguments_value));

    type_value = value_component_value(all_arguments_value, argument_number);
    assert(type_value != NULL);

    assert(get_value_kind(type_value) == VK_TYPE);
    result = type_value_data(type_value);
    assert(result != NULL);

    return result;
  }

static unsigned long hex_digit_to_bits(char digit)
  {
    if ((digit >= '0') && (digit <= '9'))
        return digit - '0';
    else if ((digit >= 'a') && (digit <= 'f'))
        return (digit - 'a') + 0xa;
    else if ((digit >= 'A') && (digit <= 'F'))
        return (digit - 'A') + 0xa;
    assert(FALSE);
    return 0;
  }

static o_integer oi_power(o_integer base, o_integer exponent)
  {
    boolean zero_exponent;
    o_integer new_base;
    o_integer two_oi;
    o_integer new_exponent;
    o_integer remainder;
    boolean remainder_non_zero;
    o_integer next_power;
    o_integer result;

    assert(!(oi_out_of_memory(base)));
    assert(!(oi_out_of_memory(exponent)));

    switch (oi_kind(base))
      {
        case IIK_FINITE:
          {
            if (oi_equal(base, oi_one))
              {
                oi_add_reference(oi_one);
                return oi_one;
              }

            switch (oi_kind(exponent))
              {
                case IIK_FINITE:
                  {
                    break;
                  }
                case IIK_POSITIVE_INFINITY:
                  {
                    o_integer minus_one_oi;
                    boolean is_zero;

                    if (oi_less_than(oi_one, base))
                      {
                        oi_add_reference(oi_positive_infinity);
                        return oi_positive_infinity;
                      }

                    oi_negate(minus_one_oi, oi_one);
                    if (oi_out_of_memory(minus_one_oi));
                        return oi_null;

                    is_zero = oi_less_than(minus_one_oi, base);
                    oi_remove_reference(minus_one_oi);
                    if (is_zero)
                      {
                        oi_add_reference(oi_zero);
                        return oi_zero;
                      }
                    else
                      {
                        oi_add_reference(oi_zero_zero);
                        return oi_zero_zero;
                      }
                  }
                case IIK_NEGATIVE_INFINITY:
                  {
                    o_integer minus_one_oi;
                    boolean is_zero;
                    boolean is_plus_infinity;

                    if (oi_less_than(oi_one, base))
                      {
                        oi_add_reference(oi_zero);
                        return oi_zero;
                      }

                    oi_negate(minus_one_oi, oi_one);
                    if (oi_out_of_memory(minus_one_oi));
                        return oi_null;

                    is_zero = oi_less_than(base, minus_one_oi);
                    oi_remove_reference(minus_one_oi);
                    if (is_zero)
                      {
                        oi_add_reference(oi_zero);
                        return oi_zero;
                      }

                    is_plus_infinity = oi_less_than(oi_zero, base);
                    if (is_plus_infinity)
                      {
                        oi_add_reference(oi_positive_infinity);
                        return oi_positive_infinity;
                      }
                    else
                      {
                        oi_add_reference(oi_zero_zero);
                        return oi_zero_zero;
                      }
                  }
                case IIK_UNSIGNED_INFINITY:
                  {
                    oi_add_reference(oi_zero_zero);
                    return oi_zero_zero;
                  }
                case IIK_ZERO_ZERO:
                  {
                    oi_add_reference(oi_zero_zero);
                    return oi_zero_zero;
                  }
                default:
                  {
                    assert(FALSE);
                  }
              }
            break;
          }
        case IIK_POSITIVE_INFINITY:
          {
            switch (oi_kind(exponent))
              {
                case IIK_FINITE:
                  {
                    if (oi_is_negative(exponent))
                      {
                        oi_add_reference(oi_zero);
                        return oi_zero;
                      }

                    if (oi_equal(oi_zero, exponent))
                      {
                        oi_add_reference(oi_zero_zero);
                        return oi_zero_zero;
                      }

                    oi_add_reference(oi_positive_infinity);
                    return oi_positive_infinity;
                  }
                case IIK_POSITIVE_INFINITY:
                  {
                    oi_add_reference(oi_positive_infinity);
                    return oi_positive_infinity;
                  }
                case IIK_NEGATIVE_INFINITY:
                  {
                    oi_add_reference(oi_zero);
                    return oi_zero;
                  }
                case IIK_UNSIGNED_INFINITY:
                  {
                    oi_add_reference(oi_zero_zero);
                    return oi_zero_zero;
                  }
                case IIK_ZERO_ZERO:
                  {
                    oi_add_reference(oi_zero_zero);
                    return oi_zero_zero;
                  }
                default:
                  {
                    assert(FALSE);
                  }
              }
          }
        case IIK_NEGATIVE_INFINITY:
          {
            switch (oi_kind(exponent))
              {
                case IIK_FINITE:
                  {
                    o_integer two_oi;
                    o_integer division;
                    o_integer remainder;
                    boolean even;

                    if (oi_is_negative(exponent))
                      {
                        oi_add_reference(oi_zero);
                        return oi_zero;
                      }

                    if (oi_equal(oi_zero, exponent))
                      {
                        oi_add_reference(oi_zero_zero);
                        return oi_zero_zero;
                      }

                    oi_create_from_size_t(two_oi, 2);
                    if (oi_out_of_memory(two_oi))
                        return oi_null;

                    oi_divide(division, exponent, two_oi, &remainder);
                    oi_remove_reference(two_oi);
                    if (oi_out_of_memory(division))
                        return oi_null;
                    oi_remove_reference(division);

                    even = oi_equal(remainder, oi_zero);
                    oi_remove_reference(remainder);
                    if (even)
                      {
                        oi_add_reference(oi_positive_infinity);
                        return oi_positive_infinity;
                      }
                    else
                      {
                        oi_add_reference(oi_negative_infinity);
                        return oi_negative_infinity;
                      }
                  }
                case IIK_POSITIVE_INFINITY:
                  {
                    oi_add_reference(oi_zero_zero);
                    return oi_zero_zero;
                  }
                case IIK_NEGATIVE_INFINITY:
                  {
                    oi_add_reference(oi_zero);
                    return oi_zero;
                  }
                case IIK_UNSIGNED_INFINITY:
                  {
                    oi_add_reference(oi_zero_zero);
                    return oi_zero_zero;
                  }
                case IIK_ZERO_ZERO:
                  {
                    oi_add_reference(oi_zero_zero);
                    return oi_zero_zero;
                  }
                default:
                  {
                    assert(FALSE);
                  }
              }
          }
        case IIK_UNSIGNED_INFINITY:
          {
            switch (oi_kind(exponent))
              {
                case IIK_FINITE:
                  {
                    o_integer two_oi;
                    o_integer division;
                    o_integer remainder;
                    boolean even;

                    if (oi_is_negative(exponent))
                      {
                        oi_add_reference(oi_zero);
                        return oi_zero;
                      }

                    if (oi_equal(oi_zero, exponent))
                      {
                        oi_add_reference(oi_zero_zero);
                        return oi_zero_zero;
                      }

                    oi_create_from_size_t(two_oi, 2);
                    if (oi_out_of_memory(two_oi))
                        return oi_null;

                    oi_divide(division, exponent, two_oi, &remainder);
                    oi_remove_reference(two_oi);
                    if (oi_out_of_memory(division))
                        return oi_null;
                    oi_remove_reference(division);

                    even = oi_equal(remainder, oi_zero);
                    oi_remove_reference(remainder);
                    if (even)
                      {
                        oi_add_reference(oi_positive_infinity);
                        return oi_positive_infinity;
                      }
                    else
                      {
                        oi_add_reference(oi_unsigned_infinity);
                        return oi_unsigned_infinity;
                      }
                  }
                case IIK_POSITIVE_INFINITY:
                  {
                    oi_add_reference(oi_zero_zero);
                    return oi_zero_zero;
                  }
                case IIK_NEGATIVE_INFINITY:
                  {
                    oi_add_reference(oi_zero);
                    return oi_zero;
                  }
                case IIK_UNSIGNED_INFINITY:
                  {
                    oi_add_reference(oi_zero_zero);
                    return oi_zero_zero;
                  }
                case IIK_ZERO_ZERO:
                  {
                    oi_add_reference(oi_zero_zero);
                    return oi_zero_zero;
                  }
                default:
                  {
                    assert(FALSE);
                  }
              }
          }
        case IIK_ZERO_ZERO:
          {
            oi_add_reference(base);
            return base;
          }
        default:
          {
            assert(FALSE);
          }
      }

    assert(!(oi_is_negative(exponent)));

    if (oi_equal(base, oi_zero))
      {
        oi_add_reference(oi_zero);
        return oi_zero;
      }

    zero_exponent = oi_equal(exponent, oi_zero);
    if (zero_exponent)
      {
        oi_add_reference(oi_one);
        return oi_one;
      }

    oi_multiply(new_base, base, base);
    if (oi_out_of_memory(new_base))
        return oi_null;

    oi_create_from_size_t(two_oi, 2);
    if (oi_out_of_memory(two_oi))
      {
        oi_remove_reference(new_base);
        return oi_null;
      }

    oi_divide(new_exponent, exponent, two_oi, &remainder);
    oi_remove_reference(two_oi);
    if (oi_out_of_memory(new_exponent))
      {
        oi_remove_reference(new_base);
        return oi_null;
      }

    remainder_non_zero = !(oi_equal(remainder, oi_zero));
    oi_remove_reference(remainder);

    next_power = oi_power(new_base, new_exponent);
    oi_remove_reference(new_base);
    oi_remove_reference(new_exponent);
    if (oi_out_of_memory(next_power))
        return oi_null;

    if (remainder_non_zero)
      {
        oi_multiply(result, next_power, base);
        oi_remove_reference(next_power);
        if (oi_out_of_memory(result))
            return oi_null;
      }
    else
      {
        result = next_power;
      }

    return result;
  }

static rational *find_mantissa_and_exponent(rational *whole_rational,
                                            size_t base, o_integer *exponent)
  {
    size_t numerator_count;
    size_t denominator_count;
    o_integer to_multiply;
    o_integer exponent_magnitude;
    o_integer base_oi;
    o_integer factor;
    o_integer product;
    rational *mantissa;
    boolean negative_mantissa;
    o_integer signed_base;

    assert(whole_rational != NULL);
    assert((base == 10) || (base == 8) || (base == 16));
    assert(exponent != NULL);

    if (oi_equal(rational_numerator(whole_rational), oi_zero))
      {
        mantissa = whole_rational;
        rational_add_reference(whole_rational);
        *exponent = oi_zero;
        oi_add_reference(oi_zero);
        return mantissa;
      }

    numerator_count =
            approximate_digit_count(rational_numerator(whole_rational), base);
    denominator_count = approximate_digit_count(
            rational_denominator(whole_rational), base);
    if (numerator_count >= denominator_count)
      {
        to_multiply = rational_denominator(whole_rational);
        oi_create_from_size_t(exponent_magnitude,
                              numerator_count - denominator_count);
      }
    else
      {
        to_multiply = rational_numerator(whole_rational);
        oi_create_from_size_t(exponent_magnitude,
                              denominator_count - numerator_count);
      }
    if (oi_out_of_memory(exponent_magnitude))
        return NULL;

    oi_create_from_size_t(base_oi, base);
    if (oi_out_of_memory(base_oi))
      {
        oi_remove_reference(exponent_magnitude);
        return NULL;
      }

    factor = oi_power(base_oi, exponent_magnitude);
    if (oi_out_of_memory(factor))
      {
        oi_remove_reference(exponent_magnitude);
        oi_remove_reference(base_oi);
        return NULL;
      }

    oi_multiply(product, to_multiply, factor);
    oi_remove_reference(factor);
    if (oi_out_of_memory(product))
      {
        oi_remove_reference(exponent_magnitude);
        oi_remove_reference(base_oi);
        return NULL;
      }

    if (numerator_count >= denominator_count)
      {
        mantissa =
                create_rational(rational_numerator(whole_rational), product);
      }
    else
      {
        mantissa =
                create_rational(product, rational_denominator(whole_rational));
      }
    oi_remove_reference(product);
    if (mantissa == NULL)
      {
        oi_remove_reference(exponent_magnitude);
        oi_remove_reference(base_oi);
        return NULL;
      }

    if (numerator_count >= denominator_count)
      {
        *exponent = exponent_magnitude;
      }
    else
      {
        oi_negate(*exponent, exponent_magnitude);
        oi_remove_reference(exponent_magnitude);
        if (oi_out_of_memory(*exponent))
          {
            rational_remove_reference(mantissa);
            oi_remove_reference(base_oi);
            return NULL;
          }
      }

    assert(oi_kind(rational_numerator(whole_rational)) == IIK_FINITE);
    negative_mantissa = oi_is_negative(rational_numerator(whole_rational));

    oi_create_from_long_int(signed_base,
            negative_mantissa ? -(long int)base : (long int)base);
    if (oi_out_of_memory(signed_base))
      {
        oi_remove_reference(*exponent);
        rational_remove_reference(mantissa);
        oi_remove_reference(base_oi);
        return NULL;
      }

    while (TRUE)
      {
        o_integer div;
        o_integer remainder;
        boolean is_zero;
        o_integer new_numerator;
        rational *new_mantissa;
        o_integer new_exponent;

        oi_divide(div, rational_numerator(mantissa),
                  rational_denominator(mantissa), &remainder);
        if (oi_out_of_memory(div))
          {
            oi_remove_reference(signed_base);
            oi_remove_reference(*exponent);
            rational_remove_reference(mantissa);
            oi_remove_reference(base_oi);
            return NULL;
          }

        oi_remove_reference(remainder);

        is_zero = oi_equal(div, oi_zero);
        oi_remove_reference(div);
        if (!is_zero)
            break;

        oi_multiply(new_numerator, rational_numerator(mantissa), base_oi);
        if (oi_out_of_memory(new_numerator))
          {
            oi_remove_reference(signed_base);
            oi_remove_reference(*exponent);
            rational_remove_reference(mantissa);
            oi_remove_reference(base_oi);
            return NULL;
          }

        new_mantissa =
                create_rational(new_numerator, rational_denominator(mantissa));
        oi_remove_reference(new_numerator);
        if (new_mantissa == NULL)
          {
            oi_remove_reference(signed_base);
            oi_remove_reference(*exponent);
            rational_remove_reference(mantissa);
            oi_remove_reference(base_oi);
            return NULL;
          }

        rational_remove_reference(mantissa);
        mantissa = new_mantissa;

        oi_subtract(new_exponent, *exponent, oi_one);
        if (oi_out_of_memory(new_exponent))
          {
            oi_remove_reference(signed_base);
            oi_remove_reference(*exponent);
            rational_remove_reference(mantissa);
            oi_remove_reference(base_oi);
            return NULL;
          }

        oi_remove_reference(*exponent);
        *exponent = new_exponent;
      }

    while (TRUE)
      {
        o_integer div;
        o_integer remainder;
        boolean done;
        o_integer new_denominator;
        rational *new_mantissa;
        o_integer new_exponent;

        oi_divide(div, rational_numerator(mantissa),
                  rational_denominator(mantissa), &remainder);
        if (oi_out_of_memory(div))
          {
            oi_remove_reference(signed_base);
            oi_remove_reference(*exponent);
            rational_remove_reference(mantissa);
            oi_remove_reference(base_oi);
            return NULL;
          }
        oi_remove_reference(remainder);

        if (negative_mantissa)
            done = oi_less_than(signed_base, div);
        else
            done = oi_less_than(div, signed_base);
        oi_remove_reference(div);
        if (done)
            break;

        oi_multiply(new_denominator, rational_denominator(mantissa), base_oi);
        if (oi_out_of_memory(new_denominator))
          {
            oi_remove_reference(signed_base);
            oi_remove_reference(*exponent);
            rational_remove_reference(mantissa);
            oi_remove_reference(base_oi);
            return NULL;
          }

        new_mantissa =
                create_rational(rational_numerator(mantissa), new_denominator);
        oi_remove_reference(new_denominator);
        if (new_mantissa == NULL)
          {
            oi_remove_reference(signed_base);
            oi_remove_reference(*exponent);
            rational_remove_reference(mantissa);
            oi_remove_reference(base_oi);
            return NULL;
          }

        rational_remove_reference(mantissa);
        mantissa = new_mantissa;

        oi_add(new_exponent, *exponent, oi_one);
        if (oi_out_of_memory(new_exponent))
          {
            oi_remove_reference(signed_base);
            oi_remove_reference(*exponent);
            rational_remove_reference(mantissa);
            oi_remove_reference(base_oi);
            return NULL;
          }

        oi_remove_reference(*exponent);
        *exponent = new_exponent;
      }

    oi_remove_reference(signed_base);
    oi_remove_reference(base_oi);
    return mantissa;
  }

static size_t approximate_digit_count(o_integer the_oi, size_t base)
  {
    assert(!(oi_out_of_memory(the_oi)));
    assert(oi_kind(the_oi) == IIK_FINITE);
    assert((base == 10) || (base == 8) || (base == 16));

    switch (base)
      {
        case 10:
          {
            size_t result;
            verdict the_verdict;

            the_verdict = oi_decimal_digit_count(the_oi, &result);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return 0;
            return result;
          }
        case 8:
        case 16:
          {
            size_t result;
            verdict the_verdict;

            the_verdict = oi_hex_digits(the_oi, &result);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return 0;
            if (base == 16)
                return result;
            return (result * 4) / 3;
          }
        default:
          {
            assert(FALSE);
            return 0;
          }
      }
  }

static size_t identifier_length(const char *string)
  {
    const char *follow;

    while (((*string < 'a') || (*string > 'z')) &&
           ((*string < 'A') || (*string > 'Z')) && (*string != '_'))
      {
        return 0;
      }

    if (strncmp(string, "operator", strlen("operator")) == 0)
      {
        const char *punctuation;
        size_t result;

        result = strlen("operator");
        punctuation = string + result;
        switch (*punctuation)
          {
            case '(':
                if (punctuation[1] == ')')
                    return result + 2;
                break;
            case '[':
                if (punctuation[1] == ']')
                    return result + 2;
                break;
            case ':':
                if (punctuation[1] == ':')
                    return result + 2;
                break;
            case '*':
                return result + 1;
            case '/':
                if ((punctuation[1] == ':') && (punctuation[2] == ':'))
                    return result + 3;
                else
                    return result + 1;
            case '%':
                return result + 1;
            case '+':
                return result + 1;
            case '-':
                if (punctuation[1] == '>')
                    return result + 2;
                else
                    return result + 1;
            case '<':
                if (punctuation[1] == '<')
                    return result + 2;
                else if (punctuation[1] == '=')
                    return result + 2;
                else
                    return result + 1;
            case '>':
                if (punctuation[1] == '>')
                    return result + 2;
                else if (punctuation[1] == '=')
                    return result + 2;
                else
                    return result + 1;
            case '&':
                return result + 1;
            case '^':
                return result + 1;
            case '|':
                return result + 1;
            case '=':
                if (punctuation[1] == '=')
                    return result + 2;
                break;
            case '!':
                if (punctuation[1] == '=')
                    return result + 2;
                else
                    return result + 1;
            case '~':
                return result + 1;
            default:
                break;
          }
      }

    follow = string + 1;

    while (((*follow >= '0') && (*follow <= '9')) ||
           ((*follow >= 'a') && (*follow <= 'z')) ||
           ((*follow >= 'A') && (*follow <= 'Z')) || (*follow == '_'))
      {
        ++follow;
      }

    return (follow - string);
  }

static o_integer map_value_length(value *array_value, const char *routine_name,
        jumper *the_jumper, const source_location *location)
  {
    o_integer min;
    o_integer max;
    boolean min_doubt;
    boolean max_doubt;
    o_integer difference;
    o_integer result;

    assert(array_value != NULL);
    assert(routine_name != NULL);
    assert(the_jumper != NULL);

    assert(get_value_kind(array_value) == VK_MAP);

    map_value_integer_key_bounds(array_value, &min, &max, &min_doubt,
                                 &max_doubt, location, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
        return oi_null;

    if (min_doubt || max_doubt)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(array_length_indeterminate),
                "In %s(), %s was unable to determine the length of the array.",
                routine_name, interpreter_name());
        if (!(oi_out_of_memory(min)))
            oi_remove_reference(min);
        if (!(oi_out_of_memory(max)))
            oi_remove_reference(max);
        return oi_null;
      }

    assert(!(oi_out_of_memory(min)));
    assert(!(oi_out_of_memory(max)));

    if (oi_less_than(max, min))
      {
        oi_remove_reference(min);
        oi_remove_reference(max);
        return oi_zero;
      }

    oi_subtract(difference, max, min);
    oi_remove_reference(max);
    oi_remove_reference(min);
    if (oi_out_of_memory(difference))
      {
        jumper_do_abort(the_jumper);
        return oi_null;
      }

    oi_add(result, difference, oi_one);
    oi_remove_reference(difference);

    return result;
  }

static void directory_read_error_handler(void *data, const char *format, ...)
  {
    directory_read_error_handler_data *typed_data;
    va_list ap;

    assert(data != NULL);
    assert(format != NULL);

    typed_data = (directory_read_error_handler_data *)data;

    va_start(ap, format);
    vlocation_exception(typed_data->jumper, typed_data->location,
                        EXCEPTION_TAG(directory_read_failed), format, ap);
    va_end(ap);
  }

static void delete_for_routine_instance(routine_instance *instance,
        jumper *the_jumper, const source_location *location)
  {
    routine_declaration *declaration;

    assert(instance != NULL);
    assert(the_jumper != NULL);

    declaration = routine_instance_declaration(instance);
    assert(declaration != NULL);

    if (routine_declaration_automatic_allocation(declaration))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(delete_routine_automatic),
                "In delete(), the routine to be deleted was automatically "
                "allocated.");
        return;
      }

    if (routine_instance_is_active(instance))
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(delete_routine_active),
                "In delete(), the routine to be deleted was executing at the "
                "point an attempt was being made to delete it.");
        return;
      }

    assert(!(routine_instance_scope_exited(instance)));

    routine_instance_set_scope_exited(instance, the_jumper);
  }

static verdict fp_read_character(fp_call_printer_data *fp_data,
        char *char_buffer, object *the_object,
        lepton_key_instance *io_error_key, jumper *the_jumper,
        const source_location *location)
  {
    FILE *fp;

    assert(fp_data != NULL);
    assert(char_buffer != NULL);
    assert(the_object != NULL);
    assert(io_error_key != NULL);

    fp = fp_data->fp;

    if (fp == NULL)
      {
        location_exception(the_jumper, location,
                EXCEPTION_TAG(closed_stream_used),
                "A closed I/O stream was used for a read_character() call.");
        return MISSION_FAILED;
      }

    if (ferror(fp))
      {
        set_fp_object_error_info_and_do_exception(the_object, io_error_key,
                the_jumper, location, "I/O error");
        return MISSION_FAILED;
      }

    if (feof(fp))
      {
      do_eof:
        set_fp_object_eof_info(the_object, the_jumper);
        if (!(jumper_flowing_forward(the_jumper)))
            return MISSION_FAILED;

        char_buffer[0] = 0;
        return MISSION_ACCOMPLISHED;
      }

    switch (fp_data->utf_format)
      {
        case UTF_8:
          {
            int input_character;
            unsigned char character_bits;

            self_block(jumper_thread(the_jumper), the_jumper, location);
            if (!(jumper_flowing_forward(the_jumper)))
                return MISSION_FAILED;

            input_character = fgetc(fp);

            self_unblock(jumper_thread(the_jumper), the_jumper, location);
            if (!(jumper_flowing_forward(the_jumper)))
                return MISSION_FAILED;

            if (input_character == EOF)
                goto do_eof;

            char_buffer[0] = (char)input_character;

            character_bits = ((unsigned char)input_character);
            if ((character_bits & 0x80) == 0)
              {
                char_buffer[1] = 0;
              }
            else if ((character_bits & 0x40) == 0)
              {
                exception_and_fp_error(the_object, io_error_key, the_jumper,
                        location, EXCEPTION_TAG(bad_utf8),
                        "Bad UTF-8 encoding -- a byte (0x%02x) with 0x2 as its"
                        " high two bits was found at the start of a "
                        "character.", (unsigned)character_bits);
                return MISSION_FAILED;
              }
            else
              {
                int extra_byte_count;
                int extra_byte_num;
                exception_error_handler_data *error_data;
                int valid_count;

                if ((character_bits & 0x20) == 0)
                  {
                    extra_byte_count = 1;
                  }
                else if ((character_bits & 0x10) == 0)
                  {
                    extra_byte_count = 2;
                  }
                else if ((character_bits & 0x08) == 0)
                  {
                    extra_byte_count = 3;
                  }
                else
                  {
                    exception_and_fp_error(the_object, io_error_key,
                            the_jumper, location, EXCEPTION_TAG(bad_utf8),
                            "Bad UTF-8 encoding -- a byte (0x%02x) with 0x1f "
                            "as its high five bits was found at the start of a"
                            " character.", (unsigned)character_bits);
                    return MISSION_FAILED;
                  }

                for (extra_byte_num = 0; extra_byte_num < extra_byte_count;
                     ++extra_byte_num)
                  {
                    int next_input_character;

                    self_block(jumper_thread(the_jumper), the_jumper,
                               location);
                    if (!(jumper_flowing_forward(the_jumper)))
                        return MISSION_FAILED;

                    next_input_character = fgetc(fp);

                    self_unblock(jumper_thread(the_jumper), the_jumper,
                                 location);
                    if (!(jumper_flowing_forward(the_jumper)))
                        return MISSION_FAILED;

                    if (next_input_character == EOF)
                      {
                        exception_and_fp_error(the_object, io_error_key,
                                the_jumper, location, EXCEPTION_TAG(bad_utf8),
                                "End of file encountered in the middle of a "
                                "UTF-8 character.");
                        return MISSION_FAILED;
                      }

                    char_buffer[1 + extra_byte_num] =
                            (char)next_input_character;
                  }

                char_buffer[1 + extra_byte_count] = 0;

                error_data = create_exception_error_data_with_object(
                        the_jumper, location, the_object, io_error_key);
                if (!(jumper_flowing_forward(the_jumper)))
                    return MISSION_FAILED;

                valid_count = validate_utf8_character_with_error_handler(
                        char_buffer, &exception_error_handler, error_data);

                free(error_data);

                if (!(jumper_flowing_forward(the_jumper)))
                  {
                    assert(valid_count < 0);
                    return MISSION_FAILED;
                  }

                assert(valid_count == (extra_byte_count + 1));
              }

            break;
          }
        case UTF_16_LE:
        case UTF_16_BE:
          {
            unsigned char input_buffer[2];
            size_t result_code;
            unsigned u16_buffer[2];
            unsigned long code_point;
            size_t character_count;

            self_block(jumper_thread(the_jumper), the_jumper, location);
            if (!(jumper_flowing_forward(the_jumper)))
                return MISSION_FAILED;

            result_code = fread(&(input_buffer[0]), 2, 1, fp);

            self_unblock(jumper_thread(the_jumper), the_jumper, location);
            if (!(jumper_flowing_forward(the_jumper)))
                return MISSION_FAILED;

            if (result_code < 1)
              {
                if (ferror(fp))
                  {
                    set_fp_object_error_info_and_do_exception(the_object,
                            io_error_key, the_jumper, location, "I/O error");
                    return MISSION_FAILED;
                  }

                assert(feof(fp));

                if (result_code == 0)
                    goto do_eof;

                exception_and_fp_error(the_object, io_error_key, the_jumper,
                        location, EXCEPTION_TAG(bad_utf16),
                        "End of file encountered in the middle of a UTF-16 "
                        "character.");
                return MISSION_FAILED;
              }

            assert(result_code == 1);

            if (fp_data->utf_format == UTF_16_LE)
              {
                u16_buffer[0] =
                        ((((unsigned long)input_buffer[1]) << 8) |
                         ((unsigned long)input_buffer[0]));
              }
            else
              {
                assert(fp_data->utf_format == UTF_16_BE);
                u16_buffer[0] =
                        ((((unsigned long)input_buffer[0]) << 8) |
                         ((unsigned long)input_buffer[1]));
              }

            if ((u16_buffer[0] & 0xfc00) == 0xdc00)
              {
                exception_and_fp_error(the_object, io_error_key, the_jumper,
                        location, EXCEPTION_TAG(bad_utf16),
                        "Bad UTF-16 encoding -- a 16-bit block (0x%04lx) with "
                        "0x37 as its high six bits was found at the start of a"
                        " character.", (unsigned long)(u16_buffer[0]));
                return MISSION_FAILED;
              }

            if ((u16_buffer[0] & 0xfc00) == 0xd800)
              {
                self_block(jumper_thread(the_jumper), the_jumper, location);
                if (!(jumper_flowing_forward(the_jumper)))
                    return MISSION_FAILED;

                result_code = fread(&(input_buffer[0]), 2, 1, fp);

                self_unblock(jumper_thread(the_jumper), the_jumper, location);
                if (!(jumper_flowing_forward(the_jumper)))
                    return MISSION_FAILED;

                if (result_code < 1)
                  {
                    if (ferror(fp))
                      {
                        set_fp_object_error_info_and_do_exception(the_object,
                                io_error_key, the_jumper, location,
                                "I/O error");
                        return MISSION_FAILED;
                      }

                    assert(feof(fp));

                    if (result_code == 0)
                        goto do_eof;

                    exception_and_fp_error(the_object, io_error_key,
                            the_jumper, location, EXCEPTION_TAG(bad_utf16),
                            "End of file encountered in the middle of a UTF-16"
                            " character.");
                    return MISSION_FAILED;
                  }

                assert(result_code == 1);

                if (fp_data->utf_format == UTF_16_LE)
                  {
                    u16_buffer[1] =
                            ((((unsigned long)input_buffer[1]) << 8) |
                             ((unsigned long)input_buffer[0]));
                  }
                else
                  {
                    assert(fp_data->utf_format == UTF_16_BE);
                    u16_buffer[1] =
                            ((((unsigned long)input_buffer[0]) << 8) |
                             ((unsigned long)input_buffer[1]));
                  }
              }

            code_point = utf16_to_code_point(u16_buffer);

            if (((code_point >= 0xd800) && (code_point < 0xe000)) ||
                (code_point >= 0x110000))
              {
                exception_and_fp_error(the_object, io_error_key, the_jumper,
                        location, EXCEPTION_TAG(bad_utf16),
                        "Bad UTF-16 character: 0x%08lx.",
                        (unsigned long)code_point);
                return MISSION_FAILED;
              }

            character_count =
                    code_point_to_utf8(code_point, &(char_buffer[0]));
            assert((character_count >= 1) && (character_count <= 4));

            char_buffer[character_count] = 0;

            break;
          }
        case UTF_32_LE:
        case UTF_32_BE:
          {
            unsigned char input_buffer[4];
            size_t result_code;
            unsigned long code_point;
            size_t character_count;

            self_block(jumper_thread(the_jumper), the_jumper, location);
            if (!(jumper_flowing_forward(the_jumper)))
                return MISSION_FAILED;

            result_code = fread(&(input_buffer[0]), 4, 1, fp);

            self_unblock(jumper_thread(the_jumper), the_jumper, location);
            if (!(jumper_flowing_forward(the_jumper)))
                return MISSION_FAILED;

            if (result_code < 1)
              {
                if (ferror(fp))
                  {
                    set_fp_object_error_info_and_do_exception(the_object,
                            io_error_key, the_jumper, location, "I/O error");
                    return MISSION_FAILED;
                  }

                assert(feof(fp));

                if (result_code == 0)
                    goto do_eof;

                exception_and_fp_error(the_object, io_error_key, the_jumper,
                        location, EXCEPTION_TAG(bad_utf32),
                        "End of file encountered in the middle of a UTF-32 "
                        "character.");
                return MISSION_FAILED;
              }

            assert(result_code == 1);

            if (fp_data->utf_format == UTF_32_LE)
              {
                code_point =
                        ((((unsigned long)input_buffer[3]) << 24) |
                         (((unsigned long)input_buffer[2]) << 16) |
                         (((unsigned long)input_buffer[1]) << 8) |
                         ((unsigned long)input_buffer[0]));
              }
            else
              {
                assert(fp_data->utf_format == UTF_32_BE);
                code_point =
                        ((((unsigned long)input_buffer[0]) << 24) |
                         (((unsigned long)input_buffer[1]) << 16) |
                         (((unsigned long)input_buffer[2]) << 8) |
                         ((unsigned long)input_buffer[3]));
              }

            if (((code_point >= 0xd800) && (code_point < 0xe000)) ||
                (code_point >= 0x110000))
              {
                exception_and_fp_error(the_object, io_error_key, the_jumper,
                        location, EXCEPTION_TAG(bad_utf32),
                        "Bad UTF-32 character: 0x%08lx.",
                        (unsigned long)code_point);
                return MISSION_FAILED;
              }

            character_count =
                    code_point_to_utf8(code_point, &(char_buffer[0]));
            assert((character_count >= 1) && (character_count <= 4));

            char_buffer[character_count] = 0;

            break;
          }
        default:
          {
            assert(FALSE);
            return MISSION_FAILED;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

static void destroy_fp_data(fp_call_printer_data *fp_data)
  {
    if ((fp_data->fp != stdout) && (fp_data->fp != stdin) &&
        (fp_data->fp != stderr))
      {
        fclose(fp_data->fp);
        free(fp_data->file_description);
      }

    free(fp_data);
  }

static void re_follower_cleaner(void *hook, jumper *the_jumper)
  {
    re_follower *the_follower;

    the_follower = (re_follower *)hook;
    assert(the_follower != NULL);
    delete_re_follower(the_follower);
  }

static void fp_cleaner(void *hook, jumper *the_jumper)
  {
    fp_call_printer_data *fp_data;

    fp_data = (fp_call_printer_data *)hook;
    assert(fp_data != NULL);
    destroy_fp_data(fp_data);
  }
