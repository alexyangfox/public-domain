/* file "test_printf_formatting_driver.c" */

/*
 *  This file contains the implementation of a driver to test
 *  "print_formatting.c" using the tests in "print_test_basic.h".
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <float.h>
#include "diagnostic.h"
#include "print_formatting.h"
#include "memory_allocation_test.h"
#include "print_formatting/floating_point_plug_in.h"
#include "print_formatting/floating_point_output_caller.h"
#include "print_formatting/floating_point_output_conversion.h"
#include "print_formatting/sprintf_floating_point_conversion.h"
#include "print_formatting/division_floating_point_conversion.h"
#include "print_formatting/pointer_plug_in.h"
#include "print_formatting/sprintf_pointer_conversion.h"
#include "print_formatting/bytewise_hex_pointer_conversion.h"


/*
 *      Implementation
 *
 *  The tests of the print_formatting module are split into two pieces, with
 *  this file containing the driver code but not a specific list of tests and
 *  the specific tests in a header file.  This allows the same tests to be used
 *  by other drivers to test other code that implements printf()-based
 *  functionality.  In particular, I've written another driver that uses the
 *  sprintf() function to do all the same tests.  This allows me to check my
 *  interpretation of the standard, as embodied in my print_formatting module
 *  and the tests, against another implementation -- the sprintf() function on
 *  the host running the test.  The sprintf() version could also be used to
 *  test a new implementation of sprintf() or for other purposes in the future.
 *
 *
 *      History
 *
 *  This code was written by me, Chris Wilson, in 2008 and placed in the public
 *  domain at that time.  It's entirely new code and isn't based on anything I
 *  or anyone else has written in the past.
 *
 *
 *      Legal Issues
 *
 *  I've written this code from scratch, without using or refering to any other
 *  code, on my own equipment and not for hire for anyone else, so I have full
 *  legal rights to place it in the public domain.
 *
 *  I've chosen to put this software in the public domain rather than
 *  copyrighting it and using the FSF's GPL or a Berkeley-style ``vanity''
 *  license because my personal opinion is that making it public domain
 *  maximizes its potential usefulness to others.  Anyone can feel free to use
 *  it for any purpose, including with their own proprietary code or with GPL
 *  code, without fear of intellectual property issues.  I have no desire to
 *  stop anyone else from making money on this code or getting any other
 *  advantages they can from it.
 *
 *  I do request that anyone who finds this software useful let me know about
 *  it.  You can drop me e-mail at "Chris Wilson" <chris@chriswilson.info> to
 *  let me know how you are using it and what is good and bad about it for you.
 *  Bug reports are also appreciated.  Also, if you release a product or
 *  software package of some sort that includes this software, I would like you
 *  to give me credit in the documentation as appropriate for the importance of
 *  my code in your product.  These are requests, not requirements, so you are
 *  not legally bound to do them, they're just a nice way to show appreciation.
 *
 *  Note that even though this software is public domain and there are no
 *  copyright laws that limit what you can do with it, other laws may apply.
 *  For example, if you lie and claim that you wrote this code when you did
 *  not, or you claim that I endorse a product of yours when I do not, that
 *  could be fraud and you could be legally liable.
 *
 *  There is absolutely no warranty for this software!  I am warning you now
 *  that it may or may not work.  It may have bugs that cause you a lot of
 *  problems.  I disclaim any implied warranties for merchantability or fitness
 *  for a particular purpose.  The fact that I have written some documentation
 *  on what I intended this software for should not be taken as any kind of
 *  warranty that it will actually behave that way.  I am providing this
 *  software as-is in the hope that it will be useful.
 *
 *          Chris Wilson, 2004, 2008
 */


#define TEST_FP_CONVERTER_MANTISSA_DIGITS 200

/*
 *  The maximum number of exponent digits we might encounter is the greater of
 *  the number of decimal digits in LDBL_MAX_10_EXP and the number of decimal
 *  digits in (LDBL_MIN_10_EXP - 1).  We can be conservative about the number
 *  of decimal digits in either of these two values by taking the number of
 *  bytes needed to represent each of the constants, multiplying that by 3
 *  (since each byte adds a factor of 256, which is less than 10^3) and adding
 *  2 (one for the first digit, which may be a zero, and one in case
 *  subtracting one bumps up the digit count).  And adding these two values
 *  gives a conservative bound on the maximum of them.
 */
#define MAX_EXPONENT_DIGITS \
        (((sizeof(LDBL_MAX_10_EXP) + sizeof(LDBL_MIN_10_EXP)) * 3) + 4)

/*
 *  The TEST_FP_CONVERTER_BUFFER_SPACE macros specifies how many characters are
 *  to be used for the buffer in the test floating-point converter.  The
 *  converter uses sprintf to print a floating-point value into this buffer in
 *  exponential format.  So the buffer will contain an optional sign character,
 *  then one mantissa character, then a decimal point, then more mantissa
 *  digits, then "e" or "E", then the sign of the exponent, then the exponent
 *  digits, then a terminating null character.  So the total number of
 *  characters used will be the number of mantissa digits plus the number of
 *  exponent digits plus 4 or 5 (4 for the decimal point, "e" or "E", exponent
 *  sign, and terminating null, plus zero or one for the mantissa sign).  The
 *  number of mantissa digits will be TEST_FP_CONVERTER_MANTISSA_DIGITS and the
 *  number of exponent digits will be no more than MAX_EXPONENT_DIGITS, so
 *  we're safe if we allocate (TEST_FP_CONVERTER_MANTISSA_DIGITS +
 *  MAX_EXPONENT_DIGITS + 5) characters for the buffer.
 */
#define TEST_FP_CONVERTER_BUFFER_SPACE \
        (TEST_FP_CONVERTER_MANTISSA_DIGITS + MAX_EXPONENT_DIGITS + 5)


typedef struct test_fp_converter_data
  {
    boolean do_rounding;
    boolean send_ascii_exponent;
    boolean find_trailing_zeros;
  } test_fp_converter_data;

typedef struct local_test_info
  {
    const char *expected_result;
    size_t current_position;
    boolean found_mismatch;
    boolean do_output_failure;
    size_t failure_after;
    boolean do_memory_failure;
    boolean errors_ok;
  } local_test_info;

typedef struct special_diagnostic_handler_info
  {
    boolean in_prefix;
    const char *format;
    const char *expected_result;
    const char *fp_algorithm_name;
    const char *pointer_algorithm_name;
    void (*old_diagnostic_handler)(void *data, diagnostic_kind kind,
            boolean has_message_number, size_t message_number,
            const char *file_name, boolean has_line_number, size_t line_number,
            boolean has_column_number, size_t column_number,
            boolean has_character_count, size_t character_count,
            const char *format, va_list arg);
    void *old_handler_data;
  } special_diagnostic_handler_info;

typedef struct floating_point_output_module_test_specification
  {
    size_t expected_output_character_count;
    verdict (*conversion_function)(void *function_data,
            floating_point_output_control *output_control,
            size_t requested_mantissa_digit_count,
            boolean care_about_trailing_zero_count, void *value_data,
            boolean mantissa_is_negative);
    void *function_data;
    void *value_data;
    boolean mantissa_is_negative;
    size_t requested_mantissa_digit_count;
    size_t precision;
    boolean print_space_if_positive;
    boolean print_plus_sign_if_positive;
    char exponent_marker_character;
    boolean suppress_trailing_zeros;
    boolean fixed_number_of_digits_after_decimal_point;
    boolean decimal_point_use_decided;
    boolean print_decimal_point;
    boolean exponent_notation_use_decided;
    size_t negative_exponent_limit_for_exponent_notation;
    boolean use_exponent_notation;
    char conversion_type_specification_character;
    size_t minimum_width;
    padding_kind padding_specification;
    verdict (*character_output_function)(void *data, char output_character);
    void *character_output_data;
    boolean rounding_done_early;
  } floating_point_output_module_test_specification;

typedef enum trailing_zero_notify_policy
  {
    TZNP_NEVER, TZNP_AT_START, TZNP_AT_ZERO_START, TZNP_AT_END,
    TZNP_SPECIFIC_POINT
  } trailing_zero_notify_policy;

typedef struct direct_floating_point_output_module_test_function_data
  {
    boolean early_by_size_t;
    boolean late_by_size_t;
    boolean send_unspecified_trailing_zeros;
    boolean send_specified_trailing_zeros;
    trailing_zero_notify_policy notify_of_trailing_zeros_policy;
    size_t zero_notify_after_mantissa_digit_count;
    boolean do_failure;
    size_t failure_after_digits;
  } direct_floating_point_output_module_test_function_data;

typedef struct direct_floating_point_output_module_test_value_data
  {
    const char *mantissa_digits;
    const char *early_exponent_digits;
    const char *late_exponent_digits;
    boolean early_exponent_is_negative;
    boolean late_exponent_is_negative;
  } direct_floating_point_output_module_test_value_data;


static local_test_info *current_local_info = NULL;
static special_diagnostic_handler_info *current_handler_info = NULL;
static size_t error_count = 0;


static void do_tests(const char *fp_algorithm_name,
        const char *pointer_algorithm_name,
        boolean test_memory_allocation_errors);
static void do_one_test(const char *format, ...);
static void test_n_expectation(local_test_info *local_info,
        const char *variable_name, long actual_value, long expected_value);
static void setup_for_output_test(local_test_info *local_info);
static verdict character_function(void *data, char output_character);
static void check_output(local_test_info *local_info, verdict the_verdict);
static void diagnostic_show_any_character(char character);
static void local_diagnostic_handler(void *data, diagnostic_kind kind,
        boolean has_message_number, size_t message_number,
        const char *file_name, boolean has_line_number, size_t line_number,
        boolean has_column_number, size_t column_number,
        boolean has_character_count, size_t character_count,
        const char *format, va_list arg);
static void reinitialize_print_formatting(void);
static void set_and_check_floating_point_conversion_plug_in(
        floating_point_plug_in_function_type *plug_in_function,
        void *plug_in_data, boolean plug_in_rounding_done_in_plug_in);
static void set_and_check_pointer_conversion_plug_in(
        pointer_plug_in_function_type *plug_in_function, void *plug_in_data,
        size_t max_output_characters);
static verdict test_fp_converter_function(void *plug_in_data,
        floating_point_output_control *output_control,
        size_t requested_mantissa_digit_count,
        boolean care_about_trailing_zero_count,
        floating_point_type_kind type_kind, void *value_data,
        boolean mantissa_is_negative);
static void use_test_fp_converter(boolean do_rounding,
        boolean send_ascii_exponent, boolean find_trailing_zeros,
        char *fp_algorithm_name_buffer);
static void do_direct_floating_point_output_module_tests(
        boolean test_memory_allocation_errors);
static verdict do_one_direct_floating_point_output_module_test(
        floating_point_output_module_test_specification *specification);
verdict direct_floating_point_output_module_test_conversion_function(
        void *function_data, floating_point_output_control *output_control,
        size_t requested_mantissa_digit_count,
        boolean care_about_trailing_zero_count, void *value_data,
        boolean mantissa_is_negative);


extern void test_code_point(const char *point)
  {
    fprintf(stdout, "Code point %s reached.\n", point);
  }

extern int main(int argc, char **argv)
  {
    boolean show_memory_allocation_count;
    const char *pass_specifier;
    boolean test_memory_allocation_errors;
    void *bytewise_hex_pointer_conversion_data;
    char fp_algorithm_name[100];

    show_memory_allocation_count = FALSE;

    if ((argc == 3) && (strcmp(argv[2], "show") == 0))
      {
        show_memory_allocation_count = TRUE;
      }
    else if (argc == 3)
      {
        const char *follow;
        size_t limit;

        follow = argv[2];
        limit = 0;
        while (*follow != 0)
          {
            if ((*follow < '0') || (*follow > '9'))
              {
                basic_error("Usage: %s {<memory-allocation-limit>}", argv[0]);
                return 1;
              }
            if (((limit * 10) + (*follow - '0')) <= ~(size_t)0)
                limit = ((limit * 10) + (*follow - '0'));
            else
                limit = ~(size_t)0;
            ++follow;
          }

        set_memory_allocation_limit(limit);
      }
    else if (argc != 2)
      {
        basic_error("Usage: %s <pass-number> {<memory-allocation-limit>}",
                    argv[0]);
        return 1;
      }

    pass_specifier = argv[1];

    if (strcmp(pass_specifier, "1") == 0)
      {
        test_memory_allocation_errors = FALSE;
      }
    else if (strcmp(pass_specifier, "2") == 0)
      {
        test_memory_allocation_errors = TRUE;
      }
    else if (strcmp(pass_specifier, "3") == 0)
      {
        verdict the_verdict;
        pointer_plug_in_function_type *default_pointer_plug_in;
        void *plug_in_data;
        size_t max_output_characters;

        the_verdict = get_pointer_conversion_plug_in(&default_pointer_plug_in,
                &plug_in_data, &max_output_characters);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return 1;

        the_verdict = get_pointer_conversion_plug_in(&default_pointer_plug_in,
                &plug_in_data, &max_output_characters);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return 1;

        if (show_memory_allocation_count)
          {
            basic_notice(
                    "There were a total of %lu memory allocations in this "
                    "run.", (unsigned long)get_memory_allocation_count());
          }
        return 0;
      }
    else
      {
        basic_error("`%s' is a bad pass number -- it must be 1 through 3",
                    pass_specifier);
        return 1;
      }

    bytewise_hex_pointer_conversion_data =
            create_bytewise_hex_pointer_conversion_data();
    if (bytewise_hex_pointer_conversion_data == NULL)
        return 1;

    sprintf(fp_algorithm_name, "default");

    do_tests(fp_algorithm_name, "default", test_memory_allocation_errors);

    set_and_check_pointer_conversion_plug_in(
            &do_bytewise_hex_pointer_conversion,
            bytewise_hex_pointer_conversion_data,
            bytewise_hex_pointer_conversion_max_output_characters());

    set_and_check_floating_point_conversion_plug_in(
            &do_division_floating_point_conversion, NULL, FALSE);
    sprintf(fp_algorithm_name, "division");

    do_tests(fp_algorithm_name, "bytewise_hex", test_memory_allocation_errors);

    set_and_check_floating_point_conversion_plug_in(
            &do_sprintf_floating_point_conversion, NULL, TRUE);
    sprintf(fp_algorithm_name, "sprintf");

    do_tests(fp_algorithm_name, "bytewise_hex", test_memory_allocation_errors);

    set_and_check_pointer_conversion_plug_in(&do_sprintf_pointer_conversion,
            NULL, sprintf_pointer_conversion_max_output_characters());

    destroy_bytewise_hex_pointer_conversion_data(
            bytewise_hex_pointer_conversion_data);

    set_and_check_floating_point_conversion_plug_in(
            &do_division_floating_point_conversion, NULL, FALSE);
    sprintf(fp_algorithm_name, "division");

    do_tests(fp_algorithm_name, "sprintf", test_memory_allocation_errors);

    set_and_check_floating_point_conversion_plug_in(
            &do_sprintf_floating_point_conversion, NULL, TRUE);
    sprintf(fp_algorithm_name, "sprintf");

    do_tests(fp_algorithm_name, "sprintf", test_memory_allocation_errors);

    use_test_fp_converter(FALSE, FALSE, FALSE, fp_algorithm_name);

    do_tests(fp_algorithm_name, "sprintf", test_memory_allocation_errors);

    use_test_fp_converter(TRUE, FALSE, FALSE, fp_algorithm_name);

    do_tests(fp_algorithm_name, "sprintf", test_memory_allocation_errors);

    use_test_fp_converter(FALSE, TRUE, FALSE, fp_algorithm_name);

    do_tests(fp_algorithm_name, "sprintf", test_memory_allocation_errors);

    use_test_fp_converter(TRUE, TRUE, FALSE, fp_algorithm_name);

    do_tests(fp_algorithm_name, "sprintf", test_memory_allocation_errors);

    use_test_fp_converter(FALSE, FALSE, TRUE, fp_algorithm_name);

    do_tests(fp_algorithm_name, "sprintf", test_memory_allocation_errors);

    use_test_fp_converter(TRUE, FALSE, TRUE, fp_algorithm_name);

    do_tests(fp_algorithm_name, "sprintf", test_memory_allocation_errors);

    use_test_fp_converter(FALSE, TRUE, TRUE, fp_algorithm_name);

    do_tests(fp_algorithm_name, "sprintf", test_memory_allocation_errors);

    use_test_fp_converter(TRUE, TRUE, TRUE, fp_algorithm_name);

    do_tests(fp_algorithm_name, "sprintf", test_memory_allocation_errors);

    set_and_check_pointer_conversion_plug_in(
            &do_bytewise_hex_pointer_conversion,
            bytewise_hex_pointer_conversion_data,
            bytewise_hex_pointer_conversion_max_output_characters() + 200);

    do_tests(fp_algorithm_name, "bytewise_hex_plus_200",
             test_memory_allocation_errors);

    set_and_check_pointer_conversion_plug_in(
            &do_bytewise_hex_pointer_conversion,
            bytewise_hex_pointer_conversion_data,
            bytewise_hex_pointer_conversion_max_output_characters() + 400);

    do_tests(fp_algorithm_name, "bytewise_hex_plus_400",
             test_memory_allocation_errors);

    set_and_check_pointer_conversion_plug_in(
            &do_bytewise_hex_pointer_conversion, NULL,
            bytewise_hex_pointer_conversion_max_output_characters());

    do_tests(fp_algorithm_name, "broken_bytewise_hex",
             test_memory_allocation_errors);

    do_direct_floating_point_output_module_tests(
            test_memory_allocation_errors);

    if (error_count == 0)
      {
        basic_notice("All do_print_formatting() tests passed.");
        if (show_memory_allocation_count)
          {
            basic_notice(
                    "There were a total of %lu memory allocations in this "
                    "run.", (unsigned long)get_memory_allocation_count());
          }
        return 0;
      }
    else
      {
        basic_notice(
                "The do_print_formatting() tests encountered %lu failure%s.",
                (unsigned long)error_count, ((error_count == 1) ? "" : "s"));
        return 1;
      }
  }


static void do_tests(const char *fp_algorithm_name,
        const char *pointer_algorithm_name,
        boolean test_memory_allocation_errors)
  {
#define PRINT_TEST(args, expected_result_constant) \
        handler_info.expected_result = (expected_result_constant); \
        RUN_TEST(__FILE__, __LINE__, do_one_test args, \
                 (expected_result_constant), FALSE) \
        handler_info.expected_result = NULL;
#define RUN_TEST(FILE, LINE, call, expected_result_constant, \
                 always_do_failure) \
      { \
        boolean save_always_do_failure; \
        size_t start_allocation_count; \
        size_t end_allocation_count; \
        size_t saved_failure_count; \
 \
        save_always_do_failure = (always_do_failure); \
        set_diagnostic_source_file_name(FILE); \
        set_diagnostic_source_line_number(LINE); \
        local_info.expected_result = (expected_result_constant); \
 \
        /* Do a test where all the attempts to send a character by the code \
         * being tested succeed. */ \
        local_info.do_output_failure = save_always_do_failure; \
        local_info.do_memory_failure = FALSE; \
        start_allocation_count = get_memory_allocation_count(); \
        call; \
        end_allocation_count = get_memory_allocation_count(); \
 \
        /* Now, test the error handling of the code being tested by \
         * simulating I/O errors.  Run the test once for each character \
         * position in the output string, testing an I/O error in each \
         * position. */ \
        local_info.do_output_failure = TRUE; \
        if (!save_always_do_failure) \
            local_info.failure_after = strlen(expected_result_constant); \
        saved_failure_count = local_info.failure_after; \
        while (local_info.failure_after > 0) \
          { \
            --(local_info.failure_after); \
            call; \
          } \
        local_info.do_output_failure = save_always_do_failure; \
        local_info.failure_after = saved_failure_count; \
 \
        /* Next, test the handling of memory allocation errors. */ \
        if (test_memory_allocation_errors) \
          { \
            boolean save_errors_ok; \
            size_t fail_after_allocation_count; \
 \
            local_info.do_output_failure = save_always_do_failure; \
            local_info.do_memory_failure = TRUE; \
            save_errors_ok = local_info.errors_ok; \
            local_info.errors_ok = TRUE; \
            fail_after_allocation_count = \
                    (end_allocation_count - start_allocation_count); \
            while (fail_after_allocation_count > 0) \
              { \
                --fail_after_allocation_count; \
                reinitialize_print_formatting(); \
                set_memory_allocation_limit(fail_after_allocation_count); \
                call; \
              } \
            clear_memory_allocation_limit(); \
            local_info.errors_ok = save_errors_ok; \
            local_info.do_memory_failure = FALSE; \
          } \
 \
        local_info.expected_result = NULL; \
        unset_diagnostic_source_line_number(); \
        unset_diagnostic_source_file_name(); \
      }

#define PRINT_N_TEST(args, expected_result_constant, declarations, \
                     expectations) \
      { \
        declarations; \
 \
        PRINT_TEST(args, (expected_result_constant)); \
 \
        set_diagnostic_source_file_name(__FILE__); \
        set_diagnostic_source_line_number(__LINE__); \
 \
        expectations; \
 \
        unset_diagnostic_source_line_number(); \
        unset_diagnostic_source_file_name(); \
      }

#define EXPECT(variable, value) \
    test_n_expectation(&local_info, #variable, (long)variable, (long)value)

    local_test_info local_info;
    special_diagnostic_handler_info handler_info;

    local_info.expected_result = NULL;

    current_local_info = &local_info;

    handler_info.in_prefix = FALSE;
    handler_info.format = NULL;
    handler_info.expected_result = NULL;
    handler_info.fp_algorithm_name = fp_algorithm_name;
    handler_info.pointer_algorithm_name = pointer_algorithm_name;
    handler_info.old_diagnostic_handler =
            get_diagnostic_handler(&handler_info.old_handler_data);

    current_handler_info = &handler_info;

    set_diagnostic_handler(&handler_info, &local_diagnostic_handler);

    if ((pointer_algorithm_name != NULL) &&
        (strcmp(pointer_algorithm_name, "broken_bytewise_hex") == 0))
      {
        local_info.errors_ok = TRUE;
        PRINT_TEST(("%p", NULL), "");
        goto cleanup;
      }

    local_info.errors_ok = FALSE;

#include "print_test_basic.h"
#include "print_test_basic_n.h"
#include "print_test_non_standard.h"

    if ((fp_algorithm_name != NULL) &&
        (strcmp(fp_algorithm_name, "division") != 0))
      {
#include "print_test_fp_precision.h"
      }

    local_info.errors_ok = TRUE;

#include "print_test_error.h"

    if ((fp_algorithm_name != NULL) &&
        (strcmp(fp_algorithm_name, "sprintf") == 0))
      {
#include "print_test_sprintf_error.h"
      }

    if ((pointer_algorithm_name != NULL) &&
        (strncmp(pointer_algorithm_name, "bytewise_hex",
                 strlen("bytewise_hex")) == 0))
      {
#include "print_test_bytewise_hex_error.h"
      }

  cleanup:
    current_local_info = NULL;
    current_handler_info = NULL;

    set_diagnostic_handler(handler_info.old_handler_data,
                           handler_info.old_diagnostic_handler);
  }

static void do_one_test(const char *format, ...)
  {
    local_test_info *local_info;
    special_diagnostic_handler_info *handler_info;
    va_list ap;
    verdict the_verdict;

    local_info = current_local_info;
    handler_info = current_handler_info;

    assert(local_info != NULL);
    assert(handler_info != NULL);
    assert(local_info->expected_result != NULL);

    handler_info->format = format;

    va_start(ap, format);

    setup_for_output_test(local_info);

    the_verdict =
            do_print_formatting(local_info, &character_function, format, ap);

    handler_info->format = NULL;

    check_output(local_info, the_verdict);

    va_end(ap);
  }

static void test_n_expectation(local_test_info *local_info,
        const char *variable_name, long actual_value, long expected_value)
  {
    assert(local_info != NULL);

    if ((actual_value != expected_value) && (!(local_info->do_memory_failure)))
      {
        basic_error(
                "Mismatch: variable `%s' was expected to end up with the value"
                " %ld, but it actually ended up with the value %ld.",
                variable_name, expected_value, actual_value);
        ++error_count;
        local_info->found_mismatch = TRUE;
      }
  }

static void setup_for_output_test(local_test_info *local_info)
  {
    assert(local_info != NULL);

    local_info->found_mismatch = FALSE;
    local_info->current_position = 0;
  }

static verdict character_function(void *data, char output_character)
  {
    local_test_info *local_info;
    size_t test_position;
    char expected_character;

    local_info = (local_test_info *)data;
    assert(local_info != NULL);
    assert(local_info->expected_result != NULL);

    test_position = local_info->current_position;
    expected_character =
            local_info->expected_result[local_info->current_position];
    if (expected_character != 0)
        ++(local_info->current_position);
    if (expected_character != output_character)
      {
        open_error();

        diagnostic_text(
                "Mismatch: expected ");
        diagnostic_show_any_character(expected_character);
        diagnostic_text(", but found ");
        diagnostic_show_any_character(output_character);
        diagnostic_text(" in column %lu of `%s'.",
                (unsigned long)test_position, local_info->expected_result);

        close_diagnostic();

        ++error_count;
        local_info->found_mismatch = TRUE;
        return MISSION_FAILED;
      }

    if (local_info->do_output_failure)
      {
        if (test_position == local_info->failure_after)
            return MISSION_FAILED;
        if (test_position > local_info->failure_after)
          {
            basic_error(
                    "The code wrote another character after getting an error "
                    "code on a previous character write.");
            ++error_count;
            return MISSION_FAILED;
          }
      }

    return MISSION_ACCOMPLISHED;
  }

static void check_output(local_test_info *local_info, verdict the_verdict)
  {
    assert(local_info != NULL);

    if ((!(local_info->do_output_failure)) &&
        (!(local_info->do_memory_failure)))
      {
        if ((!(local_info->found_mismatch)) &&
            (local_info->expected_result[local_info->current_position] != 0))
          {
            basic_error("Mismatch: output done at column %lu of `%s'.",
                    local_info->current_position, local_info->expected_result);
            ++error_count;
          }

        if ((the_verdict != MISSION_ACCOMPLISHED) &&
            (!(local_info->errors_ok)))
          {
            basic_error(
                    "An error code was returned when it should not have "
                    "been.");
            ++error_count;
          }
      }
    else
      {
        if (the_verdict == MISSION_ACCOMPLISHED)
          {
            basic_error(
                    "No error code was returned when it should have been.");
            ++error_count;
          }
      }
  }

static void diagnostic_show_any_character(char character)
  {
    if (character == 0)
        diagnostic_text("end-of-string");
    else if (isprint(character))
        diagnostic_text("`%c'", (int)character);
    else if (character == '\a')
        diagnostic_text("\\a");
    else if (character == '\b')
        diagnostic_text("\\b");
    else if (character == '\f')
        diagnostic_text("\\f");
    else if (character == '\n')
        diagnostic_text("\\n");
    else if (character == '\r')
        diagnostic_text("\\r");
    else if (character == '\t')
        diagnostic_text("\\t");
    else if (character == '\v')
        diagnostic_text("\\v");
    else
        diagnostic_text("\\%03o", (int)(unsigned char)character);
  }

static void local_diagnostic_handler(void *data, diagnostic_kind kind,
        boolean has_message_number, size_t message_number,
        const char *file_name, boolean has_line_number, size_t line_number,
        boolean has_column_number, size_t column_number,
        boolean has_character_count, size_t character_count,
        const char *format, va_list arg)
  {
    special_diagnostic_handler_info *handler_info;

    assert(data != NULL);

    handler_info = (special_diagnostic_handler_info *)data;

    if ((!(handler_info->in_prefix)) && (handler_info->format != NULL) &&
        (handler_info->expected_result != NULL) &&
        (handler_info->fp_algorithm_name != NULL) &&
        (handler_info->pointer_algorithm_name != NULL))
      {
        handler_info->in_prefix = TRUE;
        basic_notice(
                "The following diagnostic refers to a test case with a format "
                "of `%s' and an expected result of `%s', and executed with "
                "`%s' as the floating-point conversion algorithm and `%s' as "
                "the pointer conversion algorithm.", handler_info->format,
                handler_info->expected_result, handler_info->fp_algorithm_name,
                handler_info->pointer_algorithm_name);
        handler_info->in_prefix = FALSE;
      }

    (*handler_info->old_diagnostic_handler)(handler_info->old_handler_data,
            kind, has_message_number, message_number, file_name,
            has_line_number, line_number, has_column_number, column_number,
            has_character_count, character_count, format, arg);
  }

static void reinitialize_print_formatting(void)
  {
    verdict the_verdict;
    floating_point_plug_in_function_type *fp_plug_in_function;
    void *fp_plug_in_data;
    boolean fp_rounding_done_in_plug_in;
    pointer_plug_in_function_type *pointer_plug_in_function;
    void *pointer_plug_in_data;
    size_t pointer_max_output_characters;

    the_verdict = get_floating_point_conversion_plug_in(&fp_plug_in_function,
            &fp_plug_in_data, &fp_rounding_done_in_plug_in);
    assert(the_verdict == MISSION_ACCOMPLISHED);
    the_verdict = get_pointer_conversion_plug_in(&pointer_plug_in_function,
            &pointer_plug_in_data, &pointer_max_output_characters);
    assert(the_verdict == MISSION_ACCOMPLISHED);

    uninitialize_print_formatting();

    set_and_check_floating_point_conversion_plug_in(fp_plug_in_function,
            fp_plug_in_data, fp_rounding_done_in_plug_in);
    set_and_check_pointer_conversion_plug_in(pointer_plug_in_function,
            pointer_plug_in_data, pointer_max_output_characters);
  }

static void set_and_check_floating_point_conversion_plug_in(
        floating_point_plug_in_function_type *plug_in_function,
        void *plug_in_data, boolean plug_in_rounding_done_in_plug_in)
  {
    verdict the_verdict;
    floating_point_plug_in_function_type *test_plug_in_function;
    void *test_plug_in_data;
    boolean test_plug_in_rounding_done_in_plug_in;

    the_verdict = set_floating_point_conversion_plug_in(plug_in_function,
            plug_in_data, plug_in_rounding_done_in_plug_in);
    assert(the_verdict == MISSION_ACCOMPLISHED);

    the_verdict = get_floating_point_conversion_plug_in(&test_plug_in_function,
            &test_plug_in_data, &test_plug_in_rounding_done_in_plug_in);
    assert(the_verdict == MISSION_ACCOMPLISHED);
    assert(test_plug_in_function == plug_in_function);
    assert(test_plug_in_data == plug_in_data);
    assert(test_plug_in_rounding_done_in_plug_in ==
           plug_in_rounding_done_in_plug_in);
  }

static void set_and_check_pointer_conversion_plug_in(
        pointer_plug_in_function_type *plug_in_function, void *plug_in_data,
        size_t max_output_characters)
  {
    verdict the_verdict;
    pointer_plug_in_function_type *test_plug_in_function;
    void *test_plug_in_data;
    size_t test_max_output_characters;

    the_verdict = set_pointer_conversion_plug_in(plug_in_function,
            plug_in_data, max_output_characters);
    assert(the_verdict == MISSION_ACCOMPLISHED);

    the_verdict = get_pointer_conversion_plug_in(&test_plug_in_function,
            &test_plug_in_data, &test_max_output_characters);
    assert(the_verdict == MISSION_ACCOMPLISHED);
    assert(test_plug_in_function == plug_in_function);
    assert(test_plug_in_data == plug_in_data);
    assert(test_max_output_characters == max_output_characters);
  }

static verdict test_fp_converter_function(void *plug_in_data,
        floating_point_output_control *output_control,
        size_t requested_mantissa_digit_count,
        boolean care_about_trailing_zero_count,
        floating_point_type_kind type_kind, void *value_data,
        boolean mantissa_is_negative)
  {
    test_fp_converter_data *converter_data;
    size_t pass_num;
    char buffer[TEST_FP_CONVERTER_BUFFER_SPACE];
    char *start_mantissa;
    boolean exponent_is_negative;
    char *start_exponent;
    size_t digit_num;

    assert(plug_in_data != NULL);
    assert(value_data != NULL);

    converter_data = (test_fp_converter_data *)plug_in_data;

    for (pass_num = 0; pass_num < 2; ++pass_num)
      {
        switch (type_kind)
          {
            case FPTK_FLOAT:
              {
                float float_value;

                float_value = *(float *)value_data;
                sprintf(buffer, "%+.*e",
                        (int)(TEST_FP_CONVERTER_MANTISSA_DIGITS - 1),
                        float_value);
                break;
              }
            case FPTK_DOUBLE:
              {
                double double_value;

                double_value = *(double *)value_data;
                sprintf(buffer, "%+.*e",
                        (int)(TEST_FP_CONVERTER_MANTISSA_DIGITS - 1),
                        double_value);
                break;
              }
            case FPTK_LONG_DOUBLE:
              {
                long double long_double_value;

                long_double_value = *(long double *)value_data;
                sprintf(buffer, "%+.*Le",
                        (int)(TEST_FP_CONVERTER_MANTISSA_DIGITS - 1),
                        long_double_value);
                break;
              }
            default:
              {
                assert(FALSE);
              }
          }

        if (buffer[0] == '+')
          {
            assert(!mantissa_is_negative);
          }
        else if (buffer[0] == '-')
          {
            assert(mantissa_is_negative);
          }
        else
          {
            return special_non_numeric_value(output_control, buffer);
          }

        if ((buffer[1] < '0') || (buffer[1] > '9'))
          {
            return special_non_numeric_value(output_control, buffer);
          }

        assert(buffer[2] == '.');
        buffer[2] = buffer[1];
        buffer[1] = '0';
        start_mantissa = &(buffer[2]);

        assert(buffer[TEST_FP_CONVERTER_MANTISSA_DIGITS + 2] == 'e');

        switch (buffer[TEST_FP_CONVERTER_MANTISSA_DIGITS + 3])
          {
            case '+':
                exponent_is_negative = FALSE;
                break;
            case '-':
                exponent_is_negative = TRUE;
                break;
            default:
                assert(FALSE);
          }

        start_exponent = &(buffer[TEST_FP_CONVERTER_MANTISSA_DIGITS + 4]);
        while (*start_exponent == '0')
            ++start_exponent;

        if (converter_data->do_rounding &&
            (requested_mantissa_digit_count <
             TEST_FP_CONVERTER_MANTISSA_DIGITS))
          {
            char test_char;

            test_char = start_mantissa[requested_mantissa_digit_count];
            assert((test_char >= '0') && (test_char <= '9'));
            if (test_char >= '5')
              {
                char *follow;

                follow = &(start_mantissa[requested_mantissa_digit_count]);
                while (TRUE)
                  {
                    assert(follow > &(buffer[0]));
                    --follow;
                    test_char = *follow;
                    assert((test_char >= '0') && (test_char <= '9'));
                    if (test_char < '9')
                      {
                        *follow = test_char + 1;
                        if (follow < start_mantissa)
                          {
                            char *follow_exponent;

                            assert(follow + 1 == start_mantissa);
                            assert(test_char == '0');
                            start_mantissa = follow;

                            follow_exponent = start_exponent;
                            while (*follow_exponent != 0)
                              {
                                assert((*follow_exponent >= '0') &&
                                       (*follow_exponent <= '9'));
                                ++follow_exponent;
                              }

                            while (TRUE)
                              {
                                char test_exponent_char;

                                if (follow_exponent == start_exponent)
                                  {
                                    assert(!exponent_is_negative);
                                    --start_exponent;
                                    *start_exponent = '1';
                                    break;
                                  }

                                --follow_exponent;
                                test_exponent_char = *follow_exponent;
                                assert((test_exponent_char >= '0') &&
                                       (test_exponent_char <= '9'));
                                if (exponent_is_negative)
                                  {
                                    if (test_exponent_char == '0')
                                      {
                                        *follow_exponent = '9';
                                      }
                                    else
                                      {
                                        *follow_exponent =
                                                (test_exponent_char - 1);
                                        while (*start_exponent == '0')
                                            ++start_exponent;
                                        if (*start_exponent == 0)
                                            exponent_is_negative = FALSE;
                                        break;
                                      }
                                  }
                                else
                                  {
                                    if (test_exponent_char == '9')
                                      {
                                        *follow_exponent = '0';
                                      }
                                    else
                                      {
                                        *follow_exponent =
                                                (test_exponent_char + 1);
                                        break;
                                      }
                                  }
                              }
                          }
                        break;
                      }
                    *follow = '0';
                  }
              }
          }

        if (pass_num == 1)
            break;
        if (converter_data->send_ascii_exponent)
          {
            const char *follow_exponent;
            size_t exponent_digit_count;
            verdict the_verdict;

            follow_exponent = start_exponent;
            while (*follow_exponent != 0)
                ++follow_exponent;
            exponent_digit_count = (follow_exponent - start_exponent);
            the_verdict = early_exponent_by_digits(output_control,
                    mantissa_is_negative, exponent_is_negative,
                    exponent_digit_count, start_exponent,
                    &requested_mantissa_digit_count);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;
          }
        else
          {
            const char *follow_exponent;
            size_t exponent_magnitude;
            verdict the_verdict;

            follow_exponent = start_exponent;
            exponent_magnitude = 0;
            while (*follow_exponent != 0)
              {
                char test_char;

                exponent_magnitude *= 10;
                test_char = *follow_exponent;
                assert((test_char >= '0') && (test_char <= '9'));
                exponent_magnitude += test_char - '0';
                ++follow_exponent;
              }
            the_verdict = early_exponent_by_size_t(output_control,
                    mantissa_is_negative, exponent_is_negative,
                    exponent_magnitude, &requested_mantissa_digit_count);
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;
          }
      }

    if (converter_data->find_trailing_zeros && care_about_trailing_zero_count)
      {
        size_t digit_num;
        verdict the_verdict;

        if (requested_mantissa_digit_count > TEST_FP_CONVERTER_MANTISSA_DIGITS)
            digit_num = TEST_FP_CONVERTER_MANTISSA_DIGITS;
        else
            digit_num = requested_mantissa_digit_count;

        while ((digit_num > 0) && (start_mantissa[digit_num - 1] == '0'))
            --digit_num;

        the_verdict = notify_of_mantissa_trailing_zero_count(output_control,
                requested_mantissa_digit_count - digit_num);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
        requested_mantissa_digit_count = digit_num;
      }

    for (digit_num = 0; digit_num < requested_mantissa_digit_count;
         ++digit_num)
      {
        char this_digit;
        verdict the_verdict;

        if (digit_num < TEST_FP_CONVERTER_MANTISSA_DIGITS)
            this_digit = start_mantissa[digit_num];
        else
            this_digit = '0';
        assert((this_digit >= '0') && (this_digit <= '9'));
        the_verdict = handle_mantissa_digit(output_control, this_digit);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    if (converter_data->send_ascii_exponent)
      {
        const char *follow_exponent;
        size_t exponent_digit_count;

        follow_exponent = start_exponent;
        while (*follow_exponent != 0)
            ++follow_exponent;
        exponent_digit_count = (follow_exponent - start_exponent);
        return late_exponent_by_digits(output_control, exponent_is_negative,
                                       exponent_digit_count, start_exponent);
      }
    else
      {
        const char *follow_exponent;
        size_t exponent_magnitude;

        follow_exponent = start_exponent;
        exponent_magnitude = 0;
        while (*follow_exponent != 0)
          {
            char test_char;

            exponent_magnitude *= 10;
            test_char = *follow_exponent;
            assert((test_char >= '0') && (test_char <= '9'));
            exponent_magnitude += test_char - '0';
            ++follow_exponent;
          }
        return late_exponent_by_size_t(output_control, exponent_is_negative,
                                       exponent_magnitude);
      }
  }

static void use_test_fp_converter(boolean do_rounding,
        boolean send_ascii_exponent, boolean find_trailing_zeros,
        char *fp_algorithm_name_buffer)
  {
    static test_fp_converter_data converter_data;

    converter_data.do_rounding = do_rounding;
    converter_data.send_ascii_exponent = send_ascii_exponent;
    converter_data.find_trailing_zeros = find_trailing_zeros;
    set_and_check_floating_point_conversion_plug_in(
            &test_fp_converter_function, &converter_data, do_rounding);
    sprintf(fp_algorithm_name_buffer, "test - %s, %s, %s",
            (send_ascii_exponent ? "ASCII exponent" : "size_t exponent"),
            (do_rounding ? "rounding" : "no rounding"),
            (find_trailing_zeros ? "trailing zero counting" :
                                   "no trailing zero counting"));
  }

static void do_direct_floating_point_output_module_tests(
        boolean test_memory_allocation_errors)
  {
#define DO_TEST(mantissa_digit_constant, exponent_constant, \
                expected_result_constant) \
      { \
        const char *exponent_follow; \
        boolean exponent_is_negative; \
 \
        exponent_follow = (exponent_constant); \
        if (*exponent_follow == '+') \
          { \
            exponent_is_negative = FALSE; \
            ++exponent_follow; \
          } \
        else if (*exponent_follow == '-') \
          { \
            exponent_is_negative = TRUE; \
            ++exponent_follow; \
          } \
        else \
          { \
            exponent_is_negative = FALSE; \
          } \
        value_data.mantissa_digits = (mantissa_digit_constant); \
        value_data.early_exponent_digits = exponent_follow; \
        value_data.late_exponent_digits = exponent_follow; \
        value_data.early_exponent_is_negative = exponent_is_negative; \
        value_data.late_exponent_is_negative = exponent_is_negative; \
        specification.expected_output_character_count = \
                strlen(expected_result_constant); \
        RUN_TEST(__FILE__, __LINE__, \
          { \
            verdict the_verdict; \
 \
            setup_for_output_test(&local_info); \
            the_verdict = do_one_direct_floating_point_output_module_test( \
                    &specification); \
            check_output(&local_info, the_verdict); \
          } \
        , (expected_result_constant), local_info.do_output_failure) \
      }

    local_test_info local_info;
    floating_point_output_module_test_specification specification;
    direct_floating_point_output_module_test_function_data function_data;
    direct_floating_point_output_module_test_value_data value_data;
    char buffer[100];

    local_info.errors_ok = FALSE;
    local_info.do_output_failure = FALSE;

    specification.conversion_function =
            &direct_floating_point_output_module_test_conversion_function;
    specification.function_data = &function_data;
    specification.value_data = &value_data;
    specification.mantissa_is_negative = FALSE;
    specification.requested_mantissa_digit_count = 15;
    specification.precision = 506;
    specification.print_space_if_positive = FALSE;
    specification.print_plus_sign_if_positive = FALSE;
    specification.exponent_marker_character = 'E';
    specification.suppress_trailing_zeros = FALSE;
    specification.fixed_number_of_digits_after_decimal_point = FALSE;
    specification.decimal_point_use_decided = FALSE;
    specification.print_decimal_point = TRUE;
    specification.exponent_notation_use_decided = TRUE;
    specification.negative_exponent_limit_for_exponent_notation = 6;
    specification.use_exponent_notation = TRUE;
    specification.conversion_type_specification_character = 'q';
    specification.minimum_width = 10;
    specification.padding_specification = PK_LEFT_SPACE_PADDING;
    specification.character_output_function = &character_function;
    specification.character_output_data = &local_info;
    specification.rounding_done_early = TRUE;

    function_data.early_by_size_t = FALSE;
    function_data.late_by_size_t = FALSE;
    function_data.send_unspecified_trailing_zeros = FALSE;
    function_data.send_specified_trailing_zeros = TRUE;
    function_data.notify_of_trailing_zeros_policy = TZNP_NEVER;
    function_data.zero_notify_after_mantissa_digit_count = 0;
    function_data.do_failure = FALSE;
    function_data.failure_after_digits = 0;

    DO_TEST("1", "0", "1.00000000000000E+00");
    function_data.notify_of_trailing_zeros_policy = TZNP_AT_START;
    DO_TEST("1", "0", "1.00000000000000E+00");
    specification.suppress_trailing_zeros = TRUE;
    specification.decimal_point_use_decided = TRUE;
    DO_TEST("1", "0", "    1.E+00");
    DO_TEST("11", "0", "   1.1E+00");
    DO_TEST("11", "100000000000000000000000", "1.1E+100000000000000000000000");
    function_data.do_failure = TRUE;
    function_data.failure_after_digits = 20;
    DO_TEST("11", "100000000000000000000000", "1.1E+100000000000000000000000");
    function_data.failure_after_digits = 1;
    local_info.errors_ok = TRUE;
    DO_TEST("11", "100000000000000000000000", "");
    local_info.errors_ok = FALSE;
    function_data.do_failure = FALSE;
    specification.negative_exponent_limit_for_exponent_notation = ~(size_t)0;
    specification.exponent_notation_use_decided = FALSE;
    DO_TEST("11", "-100000000000000000000000",
            "1.1E-100000000000000000000000");
    DO_TEST("11", "100000000000000000000000", "1.1E+100000000000000000000000");
    specification.exponent_notation_use_decided = TRUE;
    specification.use_exponent_notation = FALSE;
    local_info.errors_ok = TRUE;
    local_info.do_output_failure = TRUE;
    local_info.failure_after = 20;
    specification.padding_specification = PK_NO_PADDING;
    DO_TEST("11", "-100000000000000000000000", "0.0000000000000000000");
    specification.requested_mantissa_digit_count = 6;
    sprintf(buffer, "-%lu", (unsigned long)(~(size_t)0) - 7);
    DO_TEST("11", buffer, "0.0000000000000000000");
    sprintf(buffer, "-%lu", (unsigned long)(~(size_t)0) - 6);
    DO_TEST("11", buffer, "0.0000000000000000000");
    sprintf(buffer, "-%lu", (unsigned long)(~(size_t)0) - 5);
    DO_TEST("11", buffer, "0.0000000000000000000");
    sprintf(buffer, "-%lu", (unsigned long)(~(size_t)0) - 4);
    DO_TEST("11", buffer, "0.0000000000000000000");
    sprintf(buffer, "-%lu", (unsigned long)(~(size_t)0) - 3);
    DO_TEST("11", buffer, "0.0000000000000000000");
    sprintf(buffer, "-%lu", (unsigned long)(~(size_t)0) - 2);
    DO_TEST("11", buffer, "0.0000000000000000000");
    sprintf(buffer, "-%lu", (unsigned long)(~(size_t)0) - 1);
    DO_TEST("11", buffer, "0.0000000000000000000");
    sprintf(buffer, "-%lu", (unsigned long)~(size_t)0);
    DO_TEST("11", buffer, "0.0000000000000000000");
    specification.requested_mantissa_digit_count = 15;
    specification.padding_specification = PK_LEFT_SPACE_PADDING;
    local_info.do_output_failure = FALSE;
    local_info.failure_after = 0;
    local_info.errors_ok = FALSE;
    specification.fixed_number_of_digits_after_decimal_point = TRUE;
    specification.requested_mantissa_digit_count = 5;
    specification.minimum_width = 1;
    DO_TEST("11", "-2", "0.011");
    DO_TEST("111", "-2", "0.0111");
    DO_TEST("1111", "-2", "0.0111");
    specification.minimum_width = 14;
    DO_TEST("11", "-2", "         0.011");
    DO_TEST("111", "-2", "        0.0111");
    DO_TEST("1111", "-2", "        0.0111");
    specification.fixed_number_of_digits_after_decimal_point = FALSE;
    specification.minimum_width = 1;
    DO_TEST("11", "-2", "0.011");
    DO_TEST("111", "-2", "0.0111");
    DO_TEST("1111", "-2", "0.01111");
    specification.minimum_width = 14;
    DO_TEST("11", "-2", "         0.011");
    DO_TEST("111", "-2", "        0.0111");
    DO_TEST("1111", "-2", "       0.01111");
    specification.suppress_trailing_zeros = FALSE;
    specification.minimum_width = 1;
    DO_TEST("11", "-2", "0.011000");
    DO_TEST("111", "-2", "0.011100");
    DO_TEST("1111", "-2", "0.011110");
    specification.minimum_width = 14;
    DO_TEST("11", "-2", "      0.011000");
    DO_TEST("111", "-2", "      0.011100");
    DO_TEST("1111", "-2", "      0.011110");
    DO_TEST("1111", "-3", "     0.0011110");
    DO_TEST("1111", "-4", "    0.00011110");
    DO_TEST("1111", "-5", "   0.000011110");
    DO_TEST("1111", "-6", "  0.0000011110");
    DO_TEST("1111", "-7", " 0.00000011110");
    DO_TEST("1111", "-8", "0.000000011110");
    specification.fixed_number_of_digits_after_decimal_point = TRUE;
    specification.requested_mantissa_digit_count = 5;
    specification.exponent_notation_use_decided = FALSE;
    DO_TEST("11", "-2", "        0.0110");
    DO_TEST("111", "-2", "        0.0111");
    DO_TEST("1111", "-2", "        0.0111");
    DO_TEST("1111", "-3", "        0.0011");
    DO_TEST("1111", "-4", "        0.0001");
    DO_TEST("1111", "-5", "    1.1110E-05");
    DO_TEST("1111", "-6", "    1.1110E-06");
    DO_TEST("1111", "-7", "    1.1110E-07");
    DO_TEST("1111", "-8", "    1.1110E-08");
    DO_TEST("1111", "-9", "    1.1110E-09");
    DO_TEST("1111", "-10", "    1.1110E-10");
    DO_TEST("999", "-2", "        0.0999");
    DO_TEST("9999", "-2", "        0.0999");
    DO_TEST("9999", "-3", "        0.0099");
    DO_TEST("9999", "-4", "        0.0009");
    DO_TEST("9999", "-5", "    9.9990E-05");
    DO_TEST("9999", "-6", "    9.9990E-06");
    DO_TEST("9999", "-7", "    9.9990E-07");
    DO_TEST("9999", "-8", "    9.9990E-08");
    DO_TEST("9999", "-9", "    9.9990E-09");
    DO_TEST("9999", "-10", "    9.9990E-10");
    DO_TEST("9999", "-11", "    9.9990E-11");
    specification.negative_exponent_limit_for_exponent_notation = 5;
    DO_TEST("11", "-2", "        0.0110");
    DO_TEST("111", "-2", "        0.0111");
    DO_TEST("1111", "-2", "        0.0111");
    DO_TEST("1111", "-3", "        0.0011");
    DO_TEST("1111", "-4", "        0.0001");
    DO_TEST("1111", "-5", "    1.1110E-05");
    DO_TEST("1111", "-6", "    1.1110E-06");
    DO_TEST("1111", "-7", "    1.1110E-07");
    DO_TEST("1111", "-8", "    1.1110E-08");
    DO_TEST("1111", "-9", "    1.1110E-09");
    DO_TEST("1111", "-10", "    1.1110E-10");
    specification.requested_mantissa_digit_count = 3;
    DO_TEST("11", "-2", "          0.01");
    DO_TEST("111", "-2", "          0.01");
    DO_TEST("1111", "-2", "          0.01");
    DO_TEST("1111", "-3", "          0.00");
    DO_TEST("1111", "-4", "          0.00");
    DO_TEST("1111", "-5", "      1.11E-05");
    DO_TEST("1111", "-6", "      1.11E-06");
    DO_TEST("1111", "-7", "      1.11E-07");
    DO_TEST("1111", "-8", "      1.11E-08");
    DO_TEST("1111", "-9", "      1.11E-09");
    DO_TEST("1111", "-10", "      1.11E-10");
    specification.rounding_done_early = FALSE;
    DO_TEST("11", "-2", "          0.01");
    DO_TEST("111", "-2", "          0.01");
    DO_TEST("1111", "-2", "          0.01");
    DO_TEST("1111", "-3", "          0.00");
    DO_TEST("1111", "-4", "          0.00");
    DO_TEST("1111", "-5", "      1.11E-05");
    DO_TEST("1111", "-6", "      1.11E-06");
    DO_TEST("1111", "-7", "      1.11E-07");
    DO_TEST("1111", "-8", "      1.11E-08");
    DO_TEST("1111", "-9", "      1.11E-09");
    DO_TEST("1111", "-10", "      1.11E-10");
    specification.requested_mantissa_digit_count = 5;
    DO_TEST("999", "-2", "        0.0999");
    DO_TEST("9999", "-2", "        0.1000");
    DO_TEST("9999", "-3", "        0.0100");
    DO_TEST("9999", "-4", "        0.0010");
    DO_TEST("9999", "-5", "    9.9990E-05");
    DO_TEST("99999", "-5", "    9.9999E-05");
    DO_TEST("999999", "-5", "        0.0001");
    DO_TEST("9999", "-6", "    9.9990E-06");
    DO_TEST("99999", "-6", "    9.9999E-06");
    DO_TEST("999999", "-6", "    1.0000E-05");
    DO_TEST("9999", "-7", "    9.9990E-07");
    DO_TEST("99999", "-7", "    9.9999E-07");
    DO_TEST("999999", "-7", "    1.0000E-06");
    DO_TEST("9999", "-8", "    9.9990E-08");
    DO_TEST("99999", "-8", "    9.9999E-08");
    DO_TEST("999999", "-8", "    1.0000E-07");
    DO_TEST("9999", "-9", "    9.9990E-09");
    DO_TEST("99999", "-9", "    9.9999E-09");
    DO_TEST("999999", "-9", "    1.0000E-08");
    DO_TEST("9999", "-10", "    9.9990E-10");
    DO_TEST("99999", "-10", "    9.9999E-10");
    DO_TEST("999999", "-10", "    1.0000E-09");
    DO_TEST("9999", "-11", "    9.9990E-11");
    DO_TEST("99999", "-11", "    9.9999E-11");
    DO_TEST("999999", "-11", "    1.0000E-10");
    specification.padding_specification = PK_NO_PADDING;
    DO_TEST("999", "-2", "0.0999");
    DO_TEST("9999", "-2", "0.1000");
    DO_TEST("9999", "-3", "0.0100");
    DO_TEST("9999", "-4", "0.0010");
    DO_TEST("9999", "-5", "9.9990E-05");
    DO_TEST("99999", "-5", "9.9999E-05");
    DO_TEST("999999", "-5", "0.0001");
    DO_TEST("9999", "-6", "9.9990E-06");
    DO_TEST("99999", "-6", "9.9999E-06");
    DO_TEST("999999", "-6", "1.0000E-05");
    DO_TEST("9999", "-7", "9.9990E-07");
    DO_TEST("99999", "-7", "9.9999E-07");
    DO_TEST("999999", "-7", "1.0000E-06");
    DO_TEST("9999", "-8", "9.9990E-08");
    DO_TEST("99999", "-8", "9.9999E-08");
    DO_TEST("999999", "-8", "1.0000E-07");
    DO_TEST("9999", "-9", "9.9990E-09");
    DO_TEST("99999", "-9", "9.9999E-09");
    DO_TEST("999999", "-9", "1.0000E-08");
    DO_TEST("9999", "-10", "9.9990E-10");
    DO_TEST("99999", "-10", "9.9999E-10");
    DO_TEST("999999", "-10", "1.0000E-09");
    DO_TEST("9999", "-11", "9.9990E-11");
    DO_TEST("99999", "-11", "9.9999E-11");
    DO_TEST("999999", "-11", "1.0000E-10");
  }

static verdict do_one_direct_floating_point_output_module_test(
        floating_point_output_module_test_specification *specification)
  {
    verdict actual_verdict;
    size_t actual_output_character_count;

    assert(specification != NULL);

    actual_verdict = do_floating_point_output(
            specification->conversion_function, specification->function_data,
            specification->value_data, specification->mantissa_is_negative,
            specification->requested_mantissa_digit_count,
            specification->precision, specification->print_space_if_positive,
            specification->print_plus_sign_if_positive,
            specification->exponent_marker_character,
            specification->suppress_trailing_zeros,
            specification->fixed_number_of_digits_after_decimal_point,
            specification->decimal_point_use_decided,
            specification->print_decimal_point,
            specification->exponent_notation_use_decided,
            specification->negative_exponent_limit_for_exponent_notation,
            specification->use_exponent_notation,
            specification->conversion_type_specification_character,
            specification->minimum_width, specification->padding_specification,
            specification->character_output_function,
            specification->character_output_data,
            specification->rounding_done_early,
            &actual_output_character_count);

    if ((actual_verdict == MISSION_ACCOMPLISHED) &&
        (actual_output_character_count !=
         specification->expected_output_character_count))
      {
        basic_error(
                "The do_floating_point_output() function was expected to say "
                "it had sent out %lu characters, but it actually said it had "
                "sent out %lu characters.",
                (unsigned long)
                        (specification->expected_output_character_count),
                (unsigned long)actual_output_character_count);
        ++error_count;
      }

    return actual_verdict;
  }

verdict direct_floating_point_output_module_test_conversion_function(
        void *function_data, floating_point_output_control *output_control,
        size_t requested_mantissa_digit_count,
        boolean care_about_trailing_zero_count, void *value_data,
        boolean mantissa_is_negative)
  {
    direct_floating_point_output_module_test_function_data *
            typed_function_data;
    direct_floating_point_output_module_test_value_data *typed_value_data;
    const char *mantissa_digits;
    const char *follow_mantissa;
    const char *end_mantissa_digits;
    size_t specified_trailing_zero_count;
    const char *exponent_digits;
    boolean use_size_t;
    size_t exponent_size_t;
    size_t mantissa_digits_remaining;
    size_t total_trailing_zero_count;
    size_t zero_notify_plus_one;
    size_t fail_digits_plus_one;

    assert(function_data != NULL);
    assert(value_data != NULL);

    typed_function_data =
            (direct_floating_point_output_module_test_function_data *)
                    function_data;
    typed_value_data =
            (direct_floating_point_output_module_test_value_data *)value_data;

    mantissa_digits = typed_value_data->mantissa_digits;
    assert(mantissa_digits != NULL);

    for (follow_mantissa = mantissa_digits; *follow_mantissa != 0;
         ++follow_mantissa)
      {
        if ((*follow_mantissa < '0') || (*follow_mantissa > '9' + 1))
            return special_non_numeric_value(output_control, mantissa_digits);
      }

    end_mantissa_digits = follow_mantissa;

    if ((!(typed_function_data->send_specified_trailing_zeros)) ||
        (typed_function_data->notify_of_trailing_zeros_policy != TZNP_NEVER))
      {
        while ((follow_mantissa > mantissa_digits) &&
               (*(follow_mantissa - 1) == '0'))
          {
            --follow_mantissa;
          }
        specified_trailing_zero_count =
                (end_mantissa_digits - follow_mantissa);
        if (!(typed_function_data->send_specified_trailing_zeros))
            end_mantissa_digits = follow_mantissa;
      }

    exponent_digits = typed_value_data->early_exponent_digits;
    assert(exponent_digits != NULL);

    if (typed_function_data->early_by_size_t)
      {
        const char *exponent_digits;
        const char *follow_exponent;

        exponent_size_t = 0;
        use_size_t = TRUE;
        for (follow_exponent = exponent_digits; *follow_exponent != 0;
             ++follow_exponent)
          {
            char digit;

            digit = *follow_exponent;
            if ((digit < '0') || (digit > '9'))
              {
                basic_error("Exponent contains non-digit characters.");
                return MISSION_FAILED;
              }

            if (exponent_size_t > (((~(size_t)0) - (digit - '0')) / 10))
              {
                use_size_t = FALSE;
                break;
              }
            exponent_size_t = ((exponent_size_t * 10) + (digit - '0'));
          }
      }
    else
      {
        use_size_t = FALSE;
      }

    mantissa_digits_remaining = requested_mantissa_digit_count;

    if (use_size_t)
      {
        verdict the_verdict;

        the_verdict = early_exponent_by_size_t(output_control,
                mantissa_is_negative,
                typed_value_data->early_exponent_is_negative, exponent_size_t,
                &mantissa_digits_remaining);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }
    else
      {
        const char *follow_exponent;
        verdict the_verdict;

        for (follow_exponent = exponent_digits; *follow_exponent != 0;
             ++follow_exponent)
          {
            char digit;

            digit = *follow_exponent;
            if ((digit < '0') || (digit > '9'))
              {
                basic_error("Exponent contains non-digit characters.");
                return MISSION_FAILED;
              }
          }

        the_verdict = early_exponent_by_digits(output_control,
                mantissa_is_negative,
                typed_value_data->early_exponent_is_negative,
                follow_exponent - exponent_digits, exponent_digits,
                &mantissa_digits_remaining);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    switch (typed_function_data->notify_of_trailing_zeros_policy)
      {
        case TZNP_NEVER:
            zero_notify_plus_one = 0;
            break;
        case TZNP_AT_START:
            zero_notify_plus_one = 1;
            break;
        case TZNP_AT_ZERO_START:
            zero_notify_plus_one = (end_mantissa_digits - mantissa_digits);
            if (typed_function_data->send_specified_trailing_zeros)
              {
                assert(zero_notify_plus_one >= specified_trailing_zero_count);
                zero_notify_plus_one -= specified_trailing_zero_count;
              }
            if (zero_notify_plus_one < ~(size_t)0)
                ++zero_notify_plus_one;
            break;
        case TZNP_AT_END:
            zero_notify_plus_one = mantissa_digits_remaining;
            if (zero_notify_plus_one < ~(size_t)0)
                ++zero_notify_plus_one;
            break;
        case TZNP_SPECIFIC_POINT:
            zero_notify_plus_one =
                    typed_function_data->
                            zero_notify_after_mantissa_digit_count;
            if (zero_notify_plus_one < ~(size_t)0)
                ++zero_notify_plus_one;
            break;
        default:
            assert(FALSE);
            break;
      }

    if (zero_notify_plus_one > 0)
      {
        size_t total_specified_digits;

        total_specified_digits = end_mantissa_digits - mantissa_digits;
        assert(specified_trailing_zero_count <= total_specified_digits);
        if (mantissa_digits_remaining > total_specified_digits)
          {
            total_trailing_zero_count =
                    (specified_trailing_zero_count +
                     (mantissa_digits_remaining - total_specified_digits));
          }
        else
          {
            size_t truncated_digits;

            truncated_digits =
                    (total_specified_digits - mantissa_digits_remaining);
            if (truncated_digits <= specified_trailing_zero_count)
              {
                total_trailing_zero_count =
                        (specified_trailing_zero_count - truncated_digits);
              }
            else
              {
                total_trailing_zero_count = 0;
              }
          }
      }

    if (typed_function_data->do_failure)
      {
        fail_digits_plus_one = typed_function_data->failure_after_digits;
        if (fail_digits_plus_one < ~(size_t)0)
            ++fail_digits_plus_one;
      }
    else
      {
        fail_digits_plus_one = 0;
      }

    for (follow_mantissa = mantissa_digits;
         ((follow_mantissa != end_mantissa_digits) &&
          (mantissa_digits_remaining > 0));
         ++follow_mantissa, --mantissa_digits_remaining)
      {
        verdict the_verdict;

        if (zero_notify_plus_one > 0)
          {
            --zero_notify_plus_one;
            if (zero_notify_plus_one == 0)
              {
                verdict the_verdict;

                the_verdict = notify_of_mantissa_trailing_zero_count(
                        output_control, total_trailing_zero_count);
                if (the_verdict != MISSION_ACCOMPLISHED)
                    return the_verdict;
              }
          }

        if (fail_digits_plus_one > 0)
          {
            --fail_digits_plus_one;
            if (fail_digits_plus_one == 0)
                return MISSION_FAILED;
          }

        the_verdict = handle_mantissa_digit(output_control, *follow_mantissa);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    if (typed_function_data->send_unspecified_trailing_zeros)
      {
        for (; mantissa_digits_remaining > 0; --mantissa_digits_remaining)
          {
            verdict the_verdict;

            if (zero_notify_plus_one > 0)
              {
                --zero_notify_plus_one;
                if (zero_notify_plus_one == 0)
                  {
                    verdict the_verdict;

                    the_verdict = notify_of_mantissa_trailing_zero_count(
                            output_control, total_trailing_zero_count);
                    if (the_verdict != MISSION_ACCOMPLISHED)
                        return the_verdict;
                  }
              }

            if (fail_digits_plus_one > 0)
              {
                --fail_digits_plus_one;
                if (fail_digits_plus_one == 0)
                    return MISSION_FAILED;
              }

            the_verdict = handle_mantissa_digit(output_control, '0');
            if (the_verdict != MISSION_ACCOMPLISHED)
                return the_verdict;
          }
      }

    if (zero_notify_plus_one > 0)
      {
        verdict the_verdict;

        the_verdict = notify_of_mantissa_trailing_zero_count(
                output_control, total_trailing_zero_count);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    exponent_digits = typed_value_data->late_exponent_digits;
    assert(exponent_digits != NULL);

    if (typed_function_data->late_by_size_t)
      {
        const char *exponent_digits;
        const char *follow_exponent;

        exponent_size_t = 0;
        use_size_t = TRUE;
        for (follow_exponent = exponent_digits; *follow_exponent != 0;
             ++follow_exponent)
          {
            char digit;

            digit = *follow_exponent;
            if ((digit < '0') || (digit > '9'))
              {
                basic_error("Exponent contains non-digit characters.");
                return MISSION_FAILED;
              }

            if (exponent_size_t > (((~(size_t)0) - (digit - '0')) / 10))
              {
                use_size_t = FALSE;
                break;
              }
            exponent_size_t = ((exponent_size_t * 10) + (digit - '0'));
          }
      }
    else
      {
        use_size_t = FALSE;
      }

    if (use_size_t)
      {
        verdict the_verdict;

        the_verdict = late_exponent_by_size_t(output_control,
                typed_value_data->late_exponent_is_negative, exponent_size_t);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }
    else
      {
        const char *follow_exponent;
        verdict the_verdict;

        for (follow_exponent = exponent_digits; *follow_exponent != 0;
             ++follow_exponent)
          {
            char digit;

            digit = *follow_exponent;
            if ((digit < '0') || (digit > '9'))
              {
                basic_error("Exponent contains non-digit characters.");
                return MISSION_FAILED;
              }
          }

        the_verdict = late_exponent_by_digits(output_control,
                typed_value_data->late_exponent_is_negative,
                follow_exponent - exponent_digits, exponent_digits);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return the_verdict;
      }

    return MISSION_ACCOMPLISHED;
  }
