/* Routines that must be linked into the core for lisp to work. */
/* $Header: undefineds.c,v 1.3 94/10/25 00:20:38 ram Exp $ */

#ifdef sun
#ifndef MACH
#if !defined(SUNOS) && !defined(SOLARIS)
#define SUNOS
#endif
#endif
#endif

typedef int func();

extern func
#include "undefineds.h"
;

func *reference_random_symbols[] = {
#include "undefineds.h"
};
