/* file "native_bridge_include_test.c" */

/*
 *  This file is part of a test of the native bridge interface of SalmonEye.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stdio.h>
#include "../native_bridge.h"
#include "../source_location.h"

static value *demo_handler(value *all_arguments_value, context *the_context,
        jumper *the_jumper, const source_location *location);

native_bridge_function_info table[3] =
  {
    ONE_STATEMENT(
            "print(\"This line came from \" ~\n"
            "      \"`native_bridge_include_test.c'.\\n\");"),
    ONE_STATEMENT(
            "print(\"This file was dynamically loaded using the native bridge "
            "interface.\\n\");"),
    ONE_ROUTINE("procedure demo(...);", &demo_handler, PURE_UNSAFE)
  };

extern native_bridge_function_info *salmon_native_bridge_statement_table()
  {
    printf("statement table generator called.\n");
    return &(table[0]);
  }

extern size_t salmon_native_bridge_statement_count()
  {
    printf("statement count generator called.\n");
    return 3;
  }

extern const char *salmon_native_bridge_source_file_name()
  {
    printf("source file name generator called.\n");
    return "native_bridge_include_test.c";
  }

static value *demo_handler(value *all_arguments_value, context *the_context,
                           jumper *the_jumper, const source_location *location)
  {
    printf("demo() was called.\n");
    printf("arguments value kind: %d.\n",
           (int)(get_value_kind(all_arguments_value)));
    interpreter_zero_fprintf(stdout, "arguments value: %U.\n",
                             all_arguments_value);
    return NULL;
  }
