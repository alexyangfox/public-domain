/* file "unimplemented_thread.c" */

/*
 *  This file implements a rump thread module through the Salmon native bridge
 *  interface for systems that don't support multi-threading.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <stddef.h>
#include <assert.h>
#include "../native_bridge.h"
#include "../source_location.h"
#include "../jumper.h"


static value *commit_suicide_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location);


static native_bridge_function_info table[] =
  {
    ONE_ROUTINE("procedure commit_suicide();", &commit_suicide_handler,
                PURE_UNSAFE)
  };


extern native_bridge_function_info *salmon_native_bridge_statement_table(void)
  {
    return &(table[0]);
  }

extern size_t salmon_native_bridge_statement_count(void)
  {
    return (sizeof(table) / sizeof(native_bridge_function_info));
  }

extern const char *salmon_native_bridge_source_file_name(void)
  {
    return __FILE__;
  }


static value *commit_suicide_handler(value *all_arguments_value,
        context *the_context, jumper *the_jumper,
        const source_location *location)
  {
    assert(all_arguments_value != NULL);
    assert(the_context != NULL);
    assert(the_jumper != NULL);

    assert(jumper_flowing_forward(the_jumper));
    jumper_do_abort(the_jumper);

    return NULL;
  }
