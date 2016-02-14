/* $Header: globals.c,v 1.1 92/07/28 20:14:29 wlott Exp $ */

/* Variables everybody needs to look at or frob on. */

#include <stdio.h>

#include "lisp.h"
#include "internals.h"
#include "globals.h"

int foreign_function_call_active;

lispobj *current_control_stack_pointer;
lispobj *current_control_frame_pointer;
#ifndef BINDING_STACK_POINTER
lispobj *current_binding_stack_pointer;
#endif

lispobj *read_only_space;
lispobj *static_space;
lispobj *dynamic_0_space;
lispobj *dynamic_1_space;
lispobj *control_stack;
lispobj *binding_stack;

lispobj *current_dynamic_space;
#ifndef ALLOCATION_POINTER
lispobj *current_dynamic_space_free_pointer;
#endif
#ifndef INTERNAL_GC_TRIGGER
lispobj *current_auto_gc_trigger;
#endif

void globals_init(void)
{
    /* Space, stack, and free pointer vars are initialized by
       validate() and coreparse(). */

#ifndef INTERNAL_GC_TRIGGER
    /* No GC trigger yet */
    current_auto_gc_trigger = NULL;
#endif

    /* Set foreign function call active. */
    foreign_function_call_active = 1;

    /* Initialize the current lisp state. */
    current_control_stack_pointer = control_stack;
    current_control_frame_pointer = (lispobj *)0;
#ifndef BINDING_STACK_POINTER
    current_binding_stack_pointer = binding_stack;
#endif
}
