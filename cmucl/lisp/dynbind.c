/*
 * $Header: dynbind.c,v 1.1 92/07/28 20:14:22 wlott Exp $
 * 
 * Support for dynamic binding from C.
 */

#include "lisp.h"
#include "internals.h"
#include "globals.h"
#include "dynbind.h"

#ifdef ibmrt
#define GetBSP() ((struct binding *)SymbolValue(BINDING_STACK_POINTER))
#define SetBSP(value) SetSymbolValue(BINDING_STACK_POINTER, (lispobj)(value))
#else
#define GetBSP() ((struct binding *)current_binding_stack_pointer)
#define SetBSP(value) (current_binding_stack_pointer=(lispobj *)(value))
#endif

void bind_variable(lispobj symbol, lispobj value)
{
	lispobj old_value;
	struct binding *binding;

	old_value = SymbolValue(symbol);
	binding = GetBSP();
	SetBSP(binding+1);

	binding->value = old_value;
	binding->symbol = symbol;
	SetSymbolValue(symbol, value);
}

void unbind(void)
{
	struct binding *binding;
	lispobj symbol;
	
	binding = GetBSP() - 1;
		
	symbol = binding->symbol;

	SetSymbolValue(symbol, binding->value);

	binding->symbol = 0;

	SetBSP(binding);
}

void unbind_to_here(lispobj *bsp)
{
    struct binding *target = (struct binding *)bsp;
    struct binding *binding = GetBSP();
    lispobj symbol;

    while (target < binding) {
	binding--;

	symbol = binding->symbol;

	if (symbol) {
	    SetSymbolValue(symbol, binding->value);
	    binding->symbol = 0;
	}

    }
    SetBSP(binding);
}
