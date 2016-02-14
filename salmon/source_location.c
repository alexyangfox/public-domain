/* file "source_location.c" */

/*
 *  This file contains the implementation of the source_location module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include "c_foundations/diagnostic.h"
#include "c_foundations/memory_allocation.h"
#include "c_foundations/buffer_print.h"
#include "source_location.h"
#include "declaration.h"
#include "variable_declaration.h"
#include "routine_declaration.h"
#include "o_integer.h"
#include "object.h"
#include "value.h"
#include "validator.h"
#include "platform_dependent.h"


struct file_name_holder
  {
    char *file_name;
    DECLARE_SYSTEM_LOCK(reference_lock);
    size_t reference_count;
  };

typedef struct
  {
    string_buffer *buffer;
    size_t position;
    int result;
  } buffer_print_data;


DECLARE_SYSTEM_LOCK(diagnostic_lock);
static boolean initialized = FALSE;


static void setup_for_location(const source_location *location);
static void clear_for_location(const source_location *location);
static void vinterpreter_zero_diagnostic_text(const char *format, va_list arg);
static void vdiagnostic_text_out(void *data, const char *format, va_list arg);
static void diagnostic_text_out(void *data, const char *format, ...);
static void vinterpreter_zero_do_formatting(const char *format, va_list arg,
        void (*vtext_out)(void *data, const char *format, va_list arg),
        void (*text_out)(void *data, const char *format, ...), void *data);
static void vbuffer_text_out(void *data, const char *format, va_list arg);
static void buffer_text_out(void *data, const char *format, ...);
static void vfile_text_out(void *data, const char *format, va_list arg);
static void file_text_out(void *data, const char *format, ...);
static void show_declaration(
        void (*text_out)(void *data, const char *format, ...), void *data,
        boolean is_upper, declaration *the_declaration);
static void show_location_if_available(
        void (*text_out)(void *data, const char *format, ...), void *data,
        const source_location *location, const char *prefix,
        const char *suffix);
static void add_source_position_field(value *base, const char *field_name,
                                      size_t position, jumper *the_jumper);
static value *source_position_to_value(size_t position);
static void write_oi(o_integer the_oi,
        void (*text_out)(void *data, const char *format, ...), void *data);


extern void set_source_location(source_location *target,
                                const source_location *source)
  {
    assert(target != NULL);

    if (source == NULL)
      {
        target->file_name = NULL;
        target->start_line_number = 0;
        target->start_column_number = 0;
        target->end_line_number = 0;
        target->end_column_number = 0;
        target->holder = NULL;
      }
    else
      {
        target->file_name = source->file_name;
        target->start_line_number = source->start_line_number;
        target->start_column_number = source->start_column_number;
        target->end_line_number = source->end_line_number;
        target->end_column_number = source->end_column_number;
        target->holder = source->holder;
        if (source->file_name != NULL)
          {
            assert(target->holder != NULL);
            file_name_holder_add_reference(target->holder);
          }
        else
          {
            assert(target->holder == NULL);
          }
      }
  }

extern void set_location_start(source_location *target,
                               const source_location *source)
  {
    assert(target != NULL);

    if (source != NULL)
      {
        if (target->file_name == NULL)
          {
            target->file_name = source->file_name;
            target->holder = source->holder;
            if (source->file_name != NULL)
              {
                assert(target->holder != NULL);
                file_name_holder_add_reference(target->holder);
              }
            else
              {
                assert(target->holder == NULL);
              }
          }
        target->start_line_number = source->start_line_number;
        target->start_column_number = source->start_column_number;
        if (target->end_line_number == 0)
            target->end_line_number = source->end_line_number;
        if (target->end_column_number == 0)
            target->end_column_number = source->end_column_number;
      }
  }

extern void set_location_end(source_location *target,
                             const source_location *source)
  {
    assert(target != NULL);

    if (source != NULL)
      {
        if (target->file_name == NULL)
          {
            target->file_name = source->file_name;
            target->holder = source->holder;
            if (source->file_name != NULL)
              {
                assert(target->holder != NULL);
                file_name_holder_add_reference(target->holder);
              }
            else
              {
                assert(target->holder == NULL);
              }
          }
        if (target->start_line_number == 0)
            target->start_line_number = source->start_line_number;
        if (target->start_column_number == 0)
            target->start_column_number = source->start_column_number;
        target->end_line_number = source->end_line_number;
        target->end_column_number = source->end_column_number;
      }
  }

extern void source_location_add_reference(source_location *location)
  {
    if ((location != NULL) && (location->holder != NULL))
        file_name_holder_add_reference(location->holder);
  }

extern void source_location_remove_reference(source_location *location)
  {
    if ((location != NULL) && (location->holder != NULL))
        file_name_holder_remove_reference(location->holder);
  }

extern void location_error(const source_location *location, const char *format,
                           ...)
  {
    va_list ap;

    va_start(ap, format);
    vlocation_error(location, format, ap);
    va_end(ap);
  }

extern void location_warning(const source_location *location,
                             const char *format, ...)
  {
    va_list ap;

    va_start(ap, format);
    vlocation_warning(location, format, ap);
    va_end(ap);
  }

extern void location_notice(const source_location *location,
                            const char *format, ...)
  {
    va_list ap;

    va_start(ap, format);
    vlocation_notice(location, format, ap);
    va_end(ap);
  }

extern void vlocation_error(const source_location *location,
                            const char *format, va_list arg)
  {
    setup_for_location(location);
    open_error();
    vinterpreter_zero_diagnostic_text(format, arg);
    close_diagnostic();
    clear_for_location(location);
  }

extern void vlocation_warning(const source_location *location,
                              const char *format, va_list arg)
  {
    setup_for_location(location);
    open_warning();
    vinterpreter_zero_diagnostic_text(format, arg);
    close_diagnostic();
    clear_for_location(location);
  }

extern void vlocation_notice(const source_location *location,
                             const char *format, va_list arg)
  {
    setup_for_location(location);
    open_notice();
    vinterpreter_zero_diagnostic_text(format, arg);
    close_diagnostic();
    clear_for_location(location);
  }

extern int interpreter_zero_buffer_printf(string_buffer *buffer,
        size_t start_character, const char *format, ...)
  {
    va_list ap;
    int result;

    va_start(ap, format);
    result = vinterpreter_zero_buffer_printf(buffer, start_character, format,
                                             ap);
    va_end(ap);
    return result;
  }

extern int vinterpreter_zero_buffer_printf(string_buffer *buffer,
        size_t start_character, const char *format, va_list arg)
  {
    buffer_print_data data;

    data.buffer = buffer;
    data.position = start_character;
    data.result = 0;

    vinterpreter_zero_do_formatting(format, arg, &vbuffer_text_out,
                                    &buffer_text_out, &data);

    return data.result;
  }

extern void interpreter_zero_fprintf(FILE *fp, const char *format, ...)
  {
    va_list ap;

    va_start(ap, format);
    vinterpreter_zero_fprintf(fp, format, ap);
    va_end(ap);
  }

extern void vinterpreter_zero_fprintf(FILE *fp, const char *format,
                                      va_list arg)
  {
    assert(fp != NULL);
    assert(format != NULL);

    vinterpreter_zero_do_formatting(format, arg, &vfile_text_out,
                                    &file_text_out, fp);
  }

extern char *allocate_printf(const char *format, ...)
  {
    va_list ap;
    char *result;

    assert(format != NULL);

    va_start(ap, format);
    result = vallocate_printf(format, ap);
    va_end(ap);
    return result;
  }

extern char *vallocate_printf(const char *format, va_list ap)
  {
    string_buffer buffer;
    verdict the_verdict;
    int print_result;

    assert(format != NULL);

    the_verdict = string_buffer_init(&buffer, 10);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return NULL;

    print_result = vinterpreter_zero_buffer_printf(&buffer, 0, format, ap);
    if (print_result < 0)
      {
        free(buffer.array);
        return NULL;
      }

    return buffer.array;
  }

extern value *create_source_location_value(const source_location *location,
                                           jumper *the_jumper)
  {
    lepton_key_instance *region_key;
    value *result;

    assert(the_jumper != NULL);

    region_key = jumper_region_key(the_jumper);
    if (region_key == NULL)
        return NULL;

    result = create_source_location_value_with_key(location, the_jumper,
                                                   region_key);

    return result;
  }

extern value *create_source_location_value_with_key(
        const source_location *location, jumper *the_jumper,
        lepton_key_instance *region_key)
  {
    const char *source_file_name;
    value *file_name_value;
    value *region_value;
    verdict the_verdict;

    source_file_name = ((location == NULL) ? "" : location->file_name);
    if (source_file_name == NULL)
        source_file_name = "";

    file_name_value = create_string_value(source_file_name);
    if (file_name_value == NULL)
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    region_value = create_lepton_value(region_key);
    if (region_value == NULL)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(file_name_value, the_jumper);
        return NULL;
      }

    the_verdict = add_field(region_value, "file_name", file_name_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(region_value, the_jumper);
        value_remove_reference(file_name_value, the_jumper);
        return NULL;
      }

    value_remove_reference(file_name_value, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(region_value, the_jumper);
        return NULL;
      }

    add_source_position_field(region_value, "start_line",
            (location == NULL) ? 0 : location->start_line_number, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(region_value, the_jumper);
        return NULL;
      }

    add_source_position_field(region_value, "start_column",
            (location == NULL) ? 0 : location->start_column_number,
            the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(region_value, the_jumper);
        return NULL;
      }

    add_source_position_field(region_value, "end_line",
            (location == NULL) ? 0 : location->end_line_number, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(region_value, the_jumper);
        return NULL;
      }

    add_source_position_field(region_value, "end_column",
            (location == NULL) ? 0 : location->end_column_number, the_jumper);
    if (!(jumper_flowing_forward(the_jumper)))
      {
        value_remove_reference(region_value, the_jumper);
        return NULL;
      }

    return region_value;
  }

extern void print_jump_target(
        void (*text_out)(void *data, const char *format, ...), void *data,
        boolean is_upper, jump_target *target)
  {
    assert(text_out != NULL);
    assert(target != NULL);

    text_out(data, "%sump target ", (is_upper ? "J" : "j"));

    switch (get_jump_target_kind(target))
      {
        case JTK_LABEL:
          {
            statement *label_statement;

            label_statement = label_jump_target_label_statement(target);
            assert(label_statement != NULL);

            text_out(data, "`%s'", label_statement_name(label_statement));
            show_location_if_available(text_out, data,
                    get_statement_location(label_statement), " (", ")");

            break;
          }
        case JTK_ROUTINE_RETURN:
          {
            text_out(data, "for return from ");
            show_declaration(text_out, data, FALSE,
                    routine_declaration_declaration(
                            routine_return_jump_target_routine_declaration(
                                    target)));
            break;
          }
        case JTK_TOP_LEVEL_RETURN:
          {
            text_out(data, "for return from the top level");
            break;
          }
        case JTK_LOOP_CONTINUE:
          {
            text_out(data, "for loop continue");
            show_location_if_available(text_out, data,
                    loop_continue_jump_target_loop_location(target), " (",
                    ")");
            break;
          }
        case JTK_LOOP_BREAK:
          {
            text_out(data, "for loop break");
            show_location_if_available(text_out, data,
                    loop_break_jump_target_loop_location(target), " (", ")");
            break;
          }
        case JTK_BLOCK_EXPRESSION_RETURN:
          {
            text_out(data, "for return from a block expression");
            break;
          }
        case JTK_TRY_CATCH_CATCH:
          {
            text_out(data, "for catch of a try-catch statement");
            break;
          }
        default:
          {
            assert(FALSE);
          }
      }
  }

extern void print_object(void (*text_out)(void *data, const char *format, ...),
                         void *data, boolean is_upper, object *the_object)
  {
    assert(text_out != NULL);
    assert(the_object != NULL);

    assert(!(object_is_closed(the_object))); /* VERIFICATION NEEDED */

    text_out(data, "%sbject based on ", (is_upper ? "O" : "o"));
    show_declaration(text_out, data, FALSE,
            routine_declaration_declaration(routine_instance_declaration(
                    object_class(the_object))));
  }

extern file_name_holder *create_file_name_holder(const char *file_name)
  {
    file_name_holder *result;
    char *name_copy;

    assert(file_name != NULL);

    result = MALLOC_ONE_OBJECT(file_name_holder);
    if (result == NULL)
        return NULL;

    name_copy = MALLOC_ARRAY(char, strlen(file_name) + 1);
    if (name_copy == NULL)
      {
        free(result);
        return NULL;
      }

    strcpy(name_copy, file_name);

    result->file_name = name_copy;

    INITIALIZE_SYSTEM_LOCK(result->reference_lock,
            if (name_copy != NULL)
                free(name_copy);
            free(result);
            return NULL);

    result->reference_count = 1;

    return result;
  }

extern const char *file_name_holder_name(file_name_holder *holder)
  {
    assert(holder != NULL);

    return holder->file_name;
  }

extern void file_name_holder_add_reference(file_name_holder *holder)
  {
    assert(holder != NULL);

    GRAB_SYSTEM_LOCK(holder->reference_lock);
    assert(holder->reference_count > 0);
    ++(holder->reference_count);
    RELEASE_SYSTEM_LOCK(holder->reference_lock);
  }

extern void file_name_holder_remove_reference(file_name_holder *holder)
  {
    size_t new_reference_count;

    assert(holder != NULL);

    GRAB_SYSTEM_LOCK(holder->reference_lock);
    assert(holder->reference_count > 0);
    --(holder->reference_count);
    new_reference_count = holder->reference_count;
    RELEASE_SYSTEM_LOCK(holder->reference_lock);

    if (new_reference_count > 0)
        return;

    free(holder->file_name);

    DESTROY_SYSTEM_LOCK(holder->reference_lock);

    free(holder);
  }


static void setup_for_location(const source_location *location)
  {
    if (!initialized)
      {
        INITIALIZE_SYSTEM_LOCK(diagnostic_lock, assert(FALSE));
        initialized = TRUE;
      }

    GRAB_SYSTEM_LOCK(diagnostic_lock);

    if (location == NULL)
        return;

    if (location->file_name != NULL)
        set_diagnostic_source_file_name(location->file_name);
    if (location->start_line_number != 0)
        set_diagnostic_source_line_number(location->start_line_number);
    if (location->start_column_number != 0)
        set_diagnostic_source_column_number(location->start_column_number);
  }

static void clear_for_location(const source_location *location)
  {
    if (location != NULL)
      {
        if (location->file_name != NULL)
            unset_diagnostic_source_file_name();
        if (location->start_line_number != 0)
            unset_diagnostic_source_line_number();
        if (location->start_column_number != 0)
            unset_diagnostic_source_column_number();
      }

    RELEASE_SYSTEM_LOCK(diagnostic_lock);
  }

static void vinterpreter_zero_diagnostic_text(const char *format, va_list arg)
  {
    vinterpreter_zero_do_formatting(format, arg, &vdiagnostic_text_out,
                                    &diagnostic_text_out, NULL);
  }

static void vdiagnostic_text_out(void *data, const char *format, va_list arg)
  {
    assert(data == NULL);
    assert(format != NULL);

    vdiagnostic_text(format, arg);
  }

static void diagnostic_text_out(void *data, const char *format, ...)
  {
    va_list ap;

    assert(data == NULL);
    assert(format != NULL);

    va_start(ap, format);
    vdiagnostic_text(format, ap);
    va_end(ap);
  }

static void vinterpreter_zero_do_formatting(const char *format, va_list arg,
        void (*vtext_out)(void *data, const char *format, va_list arg),
        void (*text_out)(void *data, const char *format, ...), void *data)
  {
    char *copy;
    char *plain_start;

    assert(format != NULL);

    copy = MALLOC_ARRAY(char, strlen(format) + 1);
    if (copy == NULL)
      {
        vtext_out(data, format, arg);
        return;
      }

    strcpy(copy, format);

    plain_start = copy;
    while (*plain_start != 0)
      {
        char *plain_end;
        char *conversion_end;
        boolean is_long;
        boolean is_short;
        boolean is_long_double;
        size_t star_count;

        plain_end = plain_start;
        while ((*plain_end != 0) && (*plain_end != '%'))
            ++plain_end;

        if (*plain_end == 0)
          {
            vtext_out(data, plain_start, arg);
            break;
          }

        assert(*plain_end == '%');
        *plain_end = 0;
        text_out(data, plain_start);
        *plain_end = '%';

        conversion_end = plain_end + 1;
        is_long = FALSE;
        is_short = FALSE;
        is_long_double = FALSE;
        star_count = 0;
        while (TRUE)
          {
            assert(*conversion_end != 0);

            switch (*conversion_end)
              {
                case '%':
                  {
                    assert(conversion_end == plain_end + 1);
                    assert(star_count == 0);
                    text_out(data, "%%");
                    break;
                  }
                case 'h':
                  {
                    is_short = TRUE;
                    ++conversion_end;
                    continue;
                  }
                case 'l':
                  {
                    is_long = TRUE;
                    ++conversion_end;
                    continue;
                  }
                case 'L':
                  {
                    is_long_double = TRUE;
                    ++conversion_end;
                    continue;
                  }
                case 'd':
                case 'i':
                case 'o':
                case 'u':
                case 'x':
                case 'X':
                case 'f':
                case 'e':
                case 'E':
                case 'g':
                case 'G':
                case 'c':
                case 's':
                case 'p':
                case 'n':
                  {
                    char save;

#define do_argument(type) \
  { \
    if (star_count == 0) \
      { \
        text_out(data, plain_end, va_arg(arg, type)); \
      } \
    else if (star_count == 1) \
      { \
        int int0 = va_arg(arg, int); \
        text_out(data, plain_end, int0, va_arg(arg, type)); \
      } \
    else if (star_count == 2) \
      { \
        int int0 = va_arg(arg, int); \
        int int1 = va_arg(arg, int); \
        text_out(data, plain_end, int0, int1, va_arg(arg, type)); \
      } \
    else \
      { \
        assert(FALSE); \
      } \
  }

                    save = *(conversion_end + 1);
                    *(conversion_end + 1) = 0;

                    switch (*conversion_end)
                      {
                        case 'd':
                        case 'i':
                            if (is_long)
                                do_argument(long)
                            else
                                do_argument(int)
                            break;
                        case 'o':
                        case 'u':
                        case 'x':
                        case 'X':
                            if (is_long)
                                do_argument(unsigned long)
                            else
                                do_argument(unsigned int)
                            break;
                        case 'f':
                        case 'e':
                        case 'E':
                        case 'g':
                        case 'G':
                            if (is_long_double)
                                do_argument(long double)
                            else
                                do_argument(double)
                            break;
                        case 'c':
                            do_argument(int)
                            break;
                        case 's':
                            do_argument(char *)
                            break;
                        case 'p':
                            do_argument(void *)
                            break;
                        case 'n':
                            if (is_long)
                                do_argument(long *)
                            else if (is_short)
                                do_argument(short *)
                            else
                                do_argument(int *)
                            break;
                        default:
                            assert(FALSE);
                            break;
                      }
                    *(conversion_end + 1) = save;
                    break;
                  }
                case 'a':
                case 'A':
                  {
                    declaration *the_declaration;

                    the_declaration = va_arg(arg, declaration *);
                    assert(the_declaration != NULL);

                    show_declaration(text_out, data, (*conversion_end == 'A'),
                                     the_declaration);

                    break;
                  }
                case 'v':
                case 'V':
                  {
                    variable_declaration *declaration;

                    declaration = va_arg(arg, variable_declaration *);
                    assert(declaration != NULL);

                    show_declaration(text_out, data, (*conversion_end == 'V'),
                            variable_declaration_declaration(declaration));

                    break;
                  }
                case 'r':
                case 'R':
                  {
                    routine_declaration *declaration;

                    declaration = va_arg(arg, routine_declaration *);
                    assert(declaration != NULL);

                    show_declaration(text_out, data, (*conversion_end == 'R'),
                            routine_declaration_declaration(declaration));

                    break;
                  }
                case 'k':
                case 'K':
                  {
                    lock_instance *instance;
                    lock_declaration *declaration;

                    instance = va_arg(arg, lock_instance *);
                    assert(instance != NULL);

                    declaration = lock_instance_declaration(instance);
                    assert(declaration != NULL);

                    show_declaration(text_out, data, (*conversion_end == 'K'),
                            lock_declaration_declaration(declaration));

                    break;
                  }
                case 'I':
                  {
                    write_oi(va_arg(arg, o_integer), text_out, data);
                    break;
                  }
                case 'Y':
                  {
                    rational *the_rational;

                    the_rational = va_arg(arg, rational *);
                    write_oi(rational_numerator(the_rational), text_out, data);
                    if (!(oi_equal(rational_denominator(the_rational),
                                   oi_one)))
                      {
                        text_out(data, "/");
                        write_oi(rational_denominator(the_rational), text_out,
                                 data);
                      }
                    break;
                  }
                case 'U':
                  {
                    value *the_value;

                    the_value = va_arg(arg, value *);
                    assert(the_value != NULL);

                    print_value(the_value, text_out, data);

                    break;
                  }
                case 't':
                  {
                    type *the_type;

                    the_type = va_arg(arg, type *);
                    assert(the_type != NULL);

                    assert(type_is_valid(the_type)); /* VERIFICATION NEEDED */
                    print_type(the_type, text_out, data, TEPP_TOP);

                    break;
                  }
                case 'j':
                case 'J':
                  {
                    jump_target *target;

                    target = va_arg(arg, jump_target *);
                    assert(target != NULL);

                    print_jump_target(text_out, data, (*conversion_end == 'J'),
                                      target);

                    break;
                  }
                case 'b':
                case 'B':
                  {
                    object *the_object;

                    the_object = va_arg(arg, object *);
                    assert(the_object != NULL);

                    print_object(text_out, data, (*conversion_end == 'B'),
                                 the_object);

                    break;
                  }
                case 'Z':
                  {
                    const char *prefix;
                    const source_location *location;
                    const char *suffix;

                    prefix = va_arg(arg, const char *);
                    location = va_arg(arg, const source_location *);
                    suffix = va_arg(arg, const char *);

                    show_location_if_available(text_out, data, location,
                                               prefix, suffix);

                    break;
                  }
                case 'q':
                  {
                    validator *the_validator;

                    the_validator = va_arg(arg, validator *);
                    assert(the_validator != NULL);

                    print_validator(text_out, data, the_validator);

                    break;
                  }
                case '*':
                  {
                    ++star_count;
                    assert(star_count <= 2);
                    ++conversion_end;
                    continue;
                  }
                default:
                  {
                    ++conversion_end;
                    continue;
                  }
              }
            break;
          }
        plain_start = conversion_end + 1;
      }

    free(copy);
  }

static void vbuffer_text_out(void *data, const char *format, va_list arg)
  {
    buffer_print_data *buffer_data;
    int local_result;

    assert(data != NULL);
    assert(format != NULL);

    buffer_data = (buffer_print_data *)data;

    if (buffer_data->result < 0)
        return;

    local_result = vbuffer_printf(buffer_data->buffer, buffer_data->position,
                                  format, arg);
    if (local_result < 0)
      {
        buffer_data->result = local_result;
      }
    else
      {
        buffer_data->result += local_result;
        buffer_data->position += local_result;
      }
  }

static void buffer_text_out(void *data, const char *format, ...)
  {
    va_list ap;

    assert(data != NULL);
    assert(format != NULL);

    va_start(ap, format);
    vbuffer_text_out(data, format, ap);
    va_end(ap);
  }

static void vfile_text_out(void *data, const char *format, va_list arg)
  {
    FILE *fp;

    assert(data != NULL);
    assert(format != NULL);

    fp = (FILE *)data;

    vfprintf(fp, format, arg);
  }

static void file_text_out(void *data, const char *format, ...)
  {
    va_list ap;

    assert(data != NULL);
    assert(format != NULL);

    va_start(ap, format);
    vfile_text_out(data, format, ap);
    va_end(ap);
  }

static void show_declaration(
        void (*text_out)(void *data, const char *format, ...), void *data,
        boolean is_upper, declaration *the_declaration)
  {
    const char *name;
    const char *lower_tag;
    const char *upper_tag;
    const source_location *location;

    name = declaration_name(the_declaration);

    switch (declaration_kind(the_declaration))
      {
        case NK_VARIABLE:
            if (variable_declaration_is_immutable(
                        declaration_variable_declaration(the_declaration)))
              {
                lower_tag = "immutable";
                upper_tag = "Immutable";
              }
            else
              {
                lower_tag = "variable";
                upper_tag = "Variable";
              }
            break;
        case NK_ROUTINE:
            lower_tag = "routine";
            upper_tag = "Routine";
            break;
        case NK_TAGALONG:
            lower_tag = "tagalong";
            upper_tag = "Tagalong";
            break;
        case NK_LEPTON_KEY:
            lower_tag = "lepton key";
            upper_tag = "Lepton key";
            break;
        case NK_QUARK:
            lower_tag = "quark";
            upper_tag = "Quark";
            break;
        case NK_LOCK:
            lower_tag = "lock";
            upper_tag = "Lock";
            break;
        default:
            assert(FALSE);
            lower_tag = NULL;
            upper_tag = NULL;
            break;
      }

    location = get_declaration_location(the_declaration);

    if (name == NULL)
      {
        if (is_upper)
            text_out(data, "A");
        else
            text_out(data, "a");
        text_out(data, "nonymous %s", lower_tag);
      }
    else
      {
        if (is_upper)
            text_out(data, "%s `%s'", upper_tag, name);
        else
            text_out(data, "%s `%s'", lower_tag, name);
      }

    show_location_if_available(text_out, data, location, " (declared at ",
                               ")");
  }

static void show_location_if_available(
        void (*text_out)(void *data, const char *format, ...), void *data,
        const source_location *location, const char *prefix,
        const char *suffix)
  {
    if ((location != NULL) && (location->start_line_number != 0))
      {
        text_out(data, prefix);

        if (location->file_name != NULL)
            text_out(data, "\"%s\":", location->file_name);
        else
            text_out(data, "line ");

        text_out(data, "%lu", (unsigned long)(location->start_line_number));

        if (location->start_column_number > 0)
          {
            text_out(data, ":%lu",
                    (unsigned long)(location->start_column_number));
          }

        text_out(data, suffix);
      }
  }

static void add_source_position_field(value *base, const char *field_name,
                                      size_t position, jumper *the_jumper)
  {
    value *new_value;
    verdict the_verdict;

    new_value = source_position_to_value(position);
    if (new_value == NULL)
      {
        jumper_do_abort(the_jumper);
        return;
      }

    the_verdict = add_field(base, field_name, new_value);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        jumper_do_abort(the_jumper);
        value_remove_reference(new_value, the_jumper);
        return;
      }

    value_remove_reference(new_value, the_jumper);
  }

static value *source_position_to_value(size_t position)
  {
    o_integer the_oi;
    value *result;

    if (position == 0)
      {
        the_oi = oi_zero_zero;
        oi_add_reference(the_oi);
      }
    else
      {
        oi_create_from_size_t(the_oi, position);
      }

    if (oi_out_of_memory(the_oi))
        return NULL;

    result = create_integer_value(the_oi);
    oi_remove_reference(the_oi);
    return result;
  }

static void write_oi(o_integer the_oi,
        void (*text_out)(void *data, const char *format, ...), void *data)
  {
    verdict the_verdict;
    size_t digit_count;
    char *digit_buffer;

    assert(!(oi_out_of_memory(the_oi)));

    switch (oi_kind(the_oi))
      {
        case IIK_FINITE:
            break;
        case IIK_POSITIVE_INFINITY:
            text_out(data, "+oo");
            return;
        case IIK_NEGATIVE_INFINITY:
            text_out(data, "-oo");
            return;
        case IIK_UNSIGNED_INFINITY:
            text_out(data, "oo");
            return;
        case IIK_ZERO_ZERO:
            text_out(data, "0/0");
            return;
        default:
            assert(FALSE);
      }

    if (oi_is_negative(the_oi))
        text_out(data, "-");

    the_verdict = oi_decimal_digit_count(the_oi, &digit_count);
    if (the_verdict != MISSION_ACCOMPLISHED)
        return;

    if (digit_count == 0)
      {
        text_out(data, "0");
        return;
      }

    digit_buffer = MALLOC_ARRAY(char, digit_count + 1);
    if (digit_buffer == NULL)
        return;

    the_verdict = oi_write_decimal_digits(the_oi, digit_buffer);
    if (the_verdict != MISSION_ACCOMPLISHED)
      {
        free(digit_buffer);
        return;
      }

    digit_buffer[digit_count] = 0;

    text_out(data, "%s", digit_buffer);

    free(digit_buffer);
  }
