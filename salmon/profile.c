/* file "profile.c" */

/*
 *  This file contains the implementation of the profile module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include "c_foundations/basic.h"
#include "c_foundations/memory_allocation.h"
#include "profile.h"
#include "routine_declaration.h"
#include "declaration.h"
#include "platform_dependent.h"
#include "unicode.h"


AUTO_ARRAY(routine_declaration_aa, routine_declaration *);


static boolean initialized = FALSE;
static routine_declaration_aa all_routines;
static CLOCK_T start_time;
static CLOCK_T total_time;


static void show_all_stats(FILE *fp, size_t max_call_count_digits,
        int precision, int max_net_width, int max_local_width);
static void show_routine_stats(FILE *fp, routine_declaration *routine,
        size_t max_call_count_digits, int precision, int max_net_width,
        int max_local_width);
static void center_output(FILE *fp, int width, const char *text);
static int routine_declaration_net_time_order(const void *raw_left,
                                              const void *raw_right);
static int routine_declaration_local_time_order(const void *raw_left,
                                                const void *raw_right);
static int routine_declaration_timeless_order(routine_declaration *left,
                                              routine_declaration *right);


extern verdict init_profile_module(void)
  {
    verdict result;

    assert(!initialized);
    GET_CLOCK(start_time);
    result = routine_declaration_aa_init(&all_routines, 10);
    if (result == MISSION_ACCOMPLISHED)
        initialized = TRUE;
    return result;
  }

extern void cleanup_profile_module(void)
  {
    assert(initialized);
    free(all_routines.array);
    initialized = FALSE;
  }

extern void profile_register_routine(routine_declaration *routine)
  {
    o_integer count;

    assert(initialized);

    count = routine_declaration_call_count(routine);
    if (oi_equal(count, oi_one))
        routine_declaration_aa_append(&all_routines, routine);
    oi_remove_reference(count);
  }

extern void dump_profile_listing(FILE *fp)
  {
    CLOCK_T end_time;
    double resolution_seconds;
    char buffer[100];
    int precision;
    size_t max_call_count_digits;
    int max_net_width;
    int max_local_width;
    size_t routine_num;

    assert(initialized);

    GET_CLOCK(end_time);
    CLOCK_DIFF(total_time, start_time, end_time);

    CLOCK_RESOLUTION_SECONDS(resolution_seconds);
    sprintf(buffer, "%.1e", resolution_seconds);
    assert((buffer[0] >= '0') && (buffer[0] <= '9'));
    assert(buffer[1] == '.');
    assert((buffer[2] >= '0') && (buffer[2] <= '9'));
    assert(buffer[3] == 'e');
    if (buffer[4] == '+')
        precision = 0;
    else
        sscanf(&(buffer[5]), "%d", &precision);
    if (buffer[2] != '0')
        ++precision;

    fprintf(fp, "\n");
    fprintf(fp, "        Profiling Information\n");
    fprintf(fp, "        =====================\n");
    fprintf(fp, "\n");
    fprintf(fp, "    Total run time: %.*f seconds.\n", precision,
            CLOCK_TO_DOUBLE_SECONDS(total_time));
    fprintf(fp, "    Timed using ");
    PRINT_CLOCK_SOURCE(fp);
    fprintf(fp, ".\n");
    fprintf(fp, "\n");

    max_call_count_digits = 4;
    max_net_width = strlen("seconds");
    max_local_width = strlen("seconds");
    for (routine_num = 0; routine_num < all_routines.element_count;
         ++routine_num)
      {
        routine_declaration *routine;
        o_integer count;
        size_t count_digits;
        verdict the_verdict;
        int test_width;

        routine = all_routines.array[routine_num];
        count = routine_declaration_call_count(routine);
        the_verdict = oi_decimal_digit_count(count, &count_digits);
        oi_remove_reference(count);
        if (the_verdict != MISSION_ACCOMPLISHED)
            return;
        if (count_digits > max_call_count_digits)
            max_call_count_digits = count_digits;

        test_width = sprintf(buffer, "%.*f", precision,
                CLOCK_TO_DOUBLE_SECONDS(
                        routine_declaration_local_time(routine)));
        if (test_width > max_local_width)
            max_local_width = test_width;

        test_width = sprintf(buffer, "%.*f", precision,
                CLOCK_TO_DOUBLE_SECONDS(
                        routine_declaration_net_time(routine)));
        if (test_width > max_net_width)
            max_net_width = test_width;
      }
    ++max_net_width;
    ++max_local_width;

    fprintf(fp, "    Sorted by Net Time\n");
    fprintf(fp, "    ------------------\n");
    fprintf(fp, "\n");

    qsort(all_routines.array, all_routines.element_count,
          sizeof(routine_declaration *), routine_declaration_net_time_order);

    show_all_stats(fp, max_call_count_digits, precision, max_net_width,
                   max_local_width);

    fprintf(fp, "\n");
    fprintf(fp, "    Sorted by Local Time\n");
    fprintf(fp, "    --------------------\n");
    fprintf(fp, "\n");

    qsort(all_routines.array, all_routines.element_count,
          sizeof(routine_declaration *), routine_declaration_local_time_order);

    show_all_stats(fp, max_call_count_digits, precision, max_net_width,
                   max_local_width);
  }


static void show_all_stats(FILE *fp, size_t max_call_count_digits,
        int precision, int max_net_width, int max_local_width)
  {
    size_t routine_num;

    fprintf(fp, "%*s", (int)(max_call_count_digits - 4), "");
    fprintf(fp, "%s", "       local     net   ");
    center_output(fp, max_local_width, "local");
    center_output(fp, max_net_width, "net");
    fprintf(fp, "\n");
    fprintf(fp, "%*s", (int)(max_call_count_digits - 4), "");
    fprintf(fp, "%s", "Calls   time %   time %");
    center_output(fp, max_local_width, "seconds");
    center_output(fp, max_net_width, "seconds");
    fprintf(fp, "%s\n", "  name");

    for (routine_num = 0; routine_num < all_routines.element_count;
         ++routine_num)
      {
        show_routine_stats(fp, all_routines.array[routine_num],
                max_call_count_digits, precision, max_net_width,
                max_local_width);
      }
  }

static void show_routine_stats(FILE *fp, routine_declaration *routine,
        size_t max_call_count_digits, int precision, int max_net_width,
        int max_local_width)
  {
    o_integer count;
    size_t count_digits;
    verdict the_verdict;
    const char *name;
    const source_location *location;

    assert(fp != NULL);
    assert(routine != NULL);

    count = routine_declaration_call_count(routine);
    the_verdict = oi_decimal_digit_count(count, &count_digits);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        oi_remove_reference(count);
        return;
      }
    while (count_digits < max_call_count_digits)
      {
        fprintf(fp, " ");
        ++count_digits;
      }
    interpreter_zero_fprintf(fp, "%I", count);
    oi_remove_reference(count);

    fprintf(fp, "%9.3f%%",
            ((CLOCK_TO_DOUBLE_SECONDS(routine_declaration_local_time(routine))
              * 100) / CLOCK_TO_DOUBLE_SECONDS(total_time)));
    fprintf(fp, "%8.3f%%",
            ((CLOCK_TO_DOUBLE_SECONDS(routine_declaration_net_time(routine)) *
              100) / CLOCK_TO_DOUBLE_SECONDS(total_time)));
    fprintf(fp, "%*.*f", max_local_width, precision,
            CLOCK_TO_DOUBLE_SECONDS(routine_declaration_local_time(routine)));
    fprintf(fp, "%*.*f", max_net_width, precision,
            CLOCK_TO_DOUBLE_SECONDS(routine_declaration_net_time(routine)));

    name = routine_declaration_name(routine);
    fprintf(fp, " %s", ((name == NULL) ? "<anonymous>" : name));

    location =
            get_declaration_location(routine_declaration_declaration(routine));
    assert(location != NULL);
    fprintf(fp, " [");
    if (location->file_name == NULL)
        fprintf(fp, "???");
    else
        fprintf(fp, "\"%s\"", location->file_name);
    fprintf(fp, ":");
    if (location->start_line_number > 0)
        fprintf(fp, "%lu", (unsigned long)(location->start_line_number));
    fprintf(fp, ":");
    if (location->start_column_number > 0)
        fprintf(fp, "%lu", (unsigned long)(location->start_column_number));
    fprintf(fp, "]");

    fprintf(fp, "\n");
  }

static void center_output(FILE *fp, int width, const char *text)
  {
    int text_length;
    char buffer[100];

    text_length = strlen(text);
    if (text_length >= width)
        fprintf(fp, "%s", text);
    sprintf(buffer, "%*s", -(text_length + ((width - text_length) / 2)), text);
    fprintf(fp, "%*s", width, buffer);
  }

static int routine_declaration_net_time_order(const void *raw_left,
                                              const void *raw_right)
  {
    routine_declaration **left;
    routine_declaration **right;
    double left_seconds;
    double right_seconds;

    left = (routine_declaration **)raw_left;
    right = (routine_declaration **)raw_right;

    assert(left != NULL);
    assert(right != NULL);
    assert(*left != NULL);
    assert(*right != NULL);

    left_seconds =
            CLOCK_TO_DOUBLE_SECONDS(routine_declaration_net_time(*left));
    right_seconds =
            CLOCK_TO_DOUBLE_SECONDS(routine_declaration_net_time(*right));

    if (left_seconds < right_seconds)
        return 1;
    if (left_seconds > right_seconds)
        return -1;

    return routine_declaration_timeless_order(*left, *right);
  }

static int routine_declaration_local_time_order(const void *raw_left,
                                                const void *raw_right)
  {
    routine_declaration **left;
    routine_declaration **right;
    double left_seconds;
    double right_seconds;

    left = (routine_declaration **)raw_left;
    right = (routine_declaration **)raw_right;

    assert(left != NULL);
    assert(right != NULL);
    assert(*left != NULL);
    assert(*right != NULL);

    left_seconds =
            CLOCK_TO_DOUBLE_SECONDS(routine_declaration_local_time(*left));
    right_seconds =
            CLOCK_TO_DOUBLE_SECONDS(routine_declaration_local_time(*right));

    if (left_seconds < right_seconds)
        return 1;
    if (left_seconds > right_seconds)
        return -1;

    return routine_declaration_timeless_order(*left, *right);
  }

static int routine_declaration_timeless_order(routine_declaration *left,
                                              routine_declaration *right)
  {
    o_integer left_count;
    o_integer right_count;
    int count_order;
    const char *left_name;
    const char *right_name;
    const source_location *left_location;
    const source_location *right_location;
    const char *left_file;
    const char *right_file;

    assert(left != NULL);
    assert(right != NULL);

    left_count = routine_declaration_call_count(left);
    right_count = routine_declaration_call_count(right);
    if (oi_equal(left_count, right_count))
        count_order = 0;
    else
        count_order = (oi_less_than(left_count, right_count) ? 1 : -1);
    oi_remove_reference(left_count);
    oi_remove_reference(right_count);
    if (count_order != 0)
        return count_order;

    left_name = routine_declaration_name(left);
    right_name = routine_declaration_name(right);

    if (left_name != NULL)
      {
        if (right_name != NULL)
          {
            int name_order;

            name_order = utf8_string_lexicographical_order_by_code_point(
                    left_name, right_name);
            if (name_order != 0)
                return name_order;
          }
        else
          {
            return -1;
          }
      }
    else
      {
        if (right_name != NULL)
            return 1;
      }

    left_location =
            get_declaration_location(routine_declaration_declaration(left));
    right_location =
            get_declaration_location(routine_declaration_declaration(right));
    assert(left_location != NULL);
    assert(right_location != NULL);

    left_file = left_location->file_name;
    right_file = right_location->file_name;

    if (left_file != NULL)
      {
        if (right_file != NULL)
          {
            if (string_is_utf8(left_file) && string_is_utf8(right_file))
              {
                int file_order;

                file_order = utf8_string_lexicographical_order_by_code_point(
                        left_file, right_file);
                if (file_order != 0)
                    return file_order;
              }
            else
              {
                const char *follow_left;
                const char *follow_right;

                follow_left = left_file;
                follow_right = right_file;
                while (TRUE)
                  {
                    if (*follow_left == 0)
                      {
                        if (*follow_right == 0)
                            break;
                        return -1;
                      }
                    if (*follow_right == 0)
                        return 1;
                    if (*follow_left != *follow_right)
                        return ((*follow_left < *follow_right) ? -1 : 1);
                    ++follow_left;
                    ++follow_right;
                  }
              }
          }
        else
          {
            return -1;
          }
      }
    else
      {
        if (right_file != NULL)
          {
            return 1;
          }
      }

    if (left_location->start_line_number != right_location->start_line_number)
      {
        return ((left_location->start_line_number <
                 right_location->start_line_number) ? -1 : 1);
      }

    if (left_location->start_column_number !=
        right_location->start_column_number)
      {
        return ((left_location->start_column_number <
                 right_location->start_column_number) ? -1 : 1);
      }

    if (left_location->end_line_number != right_location->end_line_number)
      {
        return ((left_location->end_line_number <
                 right_location->end_line_number) ? -1 : 1);
      }

    if (left_location->end_column_number != right_location->end_column_number)
      {
        return ((left_location->end_column_number <
                 right_location->end_column_number) ? -1 : 1);
      }

    if (left < right)
        return 1;
    if (left > right)
        return -1;
    else
        return 0;
  }
