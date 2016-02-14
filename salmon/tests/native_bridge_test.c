/* file "native_bridge_test.c" */

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


extern verdict salmoneye_plugin_initialize()
  {
    printf("Native bridge test initialization called.\n");
    return MISSION_ACCOMPLISHED;
  }

extern value *demo_handler(value *all_arguments_value, context *the_context,
                           jumper *the_jumper, const source_location *location)
  {
    printf("demo() was called.\n");
    printf("arguments value kind: %d.\n",
           (int)(get_value_kind(all_arguments_value)));
    interpreter_zero_fprintf(stdout, "arguments value: %U.\n",
                             all_arguments_value);
    return NULL;
  }

extern value *plus_handler(value *all_arguments_value, context *the_context,
                           jumper *the_jumper, const source_location *location)
  {
    o_integer left_oi;
    o_integer right_oi;
    o_integer result_oi;
    value *result;

    left_oi =
            integer_value_data(value_component_value(all_arguments_value, 0));
    right_oi =
            integer_value_data(value_component_value(all_arguments_value, 1));
    oi_add(result_oi, left_oi, right_oi);
    if (oi_out_of_memory(result_oi))
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_integer_value(result_oi);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }

extern value *return_three_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    o_integer result_oi;
    value *result;

    oi_create_from_long_int(result_oi, 3);
    if (oi_out_of_memory(result_oi))
      {
        jumper_do_abort(the_jumper);
        return NULL;
      }

    result = create_integer_value(result_oi);
    if (result == NULL)
        jumper_do_abort(the_jumper);
    return result;
  }
