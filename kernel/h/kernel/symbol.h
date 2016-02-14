#ifndef KERNEL_SYMBOL_H
#define KERNEL_SYMBOL_H

#include <kernel/types.h>
#include <kernel/lists.h>




/*
 *
 */

struct KSym
{
	char *name;
	void *addr;
};




/*
 *
 */

struct KSym *FindKernelSymbol (char *name);






    
#endif
