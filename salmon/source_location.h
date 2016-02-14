/* file "source_location.h" */

/*
 *  This file contains the interface to the source_location module.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#ifndef SOURCE_LOCATION_H
#define SOURCE_LOCATION_H

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include "c_foundations/buffer_print.h"


typedef struct file_name_holder file_name_holder;

typedef struct source_location
  {
    const char *file_name;
    size_t start_line_number;
    size_t start_column_number;
    size_t end_line_number;
    size_t end_column_number;
    file_name_holder *holder;
  } source_location;


#include "jumper.h"
#include "value.h"
#include "lepton_key_instance.h"


extern void set_source_location(source_location *target,
                                const source_location *source);
extern void set_location_start(source_location *target,
                               const source_location *source);
extern void set_location_end(source_location *target,
                             const source_location *source);

extern void source_location_add_reference(source_location *location);
extern void source_location_remove_reference(source_location *location);

extern void location_error(const source_location *location, const char *format,
                           ...);
extern void location_warning(const source_location *location,
                             const char *format, ...);
extern void location_notice(const source_location *location,
                            const char *format, ...);
extern void vlocation_error(const source_location *location,
                            const char *format, va_list arg);
extern void vlocation_warning(const source_location *location,
                              const char *format, va_list arg);
extern void vlocation_notice(const source_location *location,
                             const char *format, va_list arg);

extern int interpreter_zero_buffer_printf(string_buffer *buffer,
        size_t start_character, const char *format, ...);
extern int vinterpreter_zero_buffer_printf(string_buffer *buffer,
        size_t start_character, const char *format, va_list arg);
extern void interpreter_zero_fprintf(FILE *fp, const char *format, ...);
extern void vinterpreter_zero_fprintf(FILE *fp, const char *format,
                                      va_list arg);
extern char *allocate_printf(const char *format, ...);
extern char *vallocate_printf(const char *format, va_list ap);

extern value *create_source_location_value(const source_location *location,
                                           jumper *the_jumper);
extern value *create_source_location_value_with_key(
        const source_location *location, jumper *the_jumper,
        lepton_key_instance *region_key);

extern void print_jump_target(
        void (*text_out)(void *data, const char *format, ...), void *data,
        boolean is_upper, jump_target *target);
extern void print_object(void (*text_out)(void *data, const char *format, ...),
                         void *data, boolean is_upper, object *the_object);

extern file_name_holder *create_file_name_holder(const char *file_name);
extern const char *file_name_holder_name(file_name_holder *holder);
extern void file_name_holder_add_reference(file_name_holder *holder);
extern void file_name_holder_remove_reference(file_name_holder *holder);


#endif /* SOURCE_LOCATION_H */
