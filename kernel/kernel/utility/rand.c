#include <stdarg.h>
#include <kernel/types.h>
#include <kernel/dbg.h>


static int32 seed = 1;


/*
 * Rand()
 */

int32 Rand (void)
{
	uint32 hi, lo;
	
	lo = 16807 * (seed & 0xffff);
	hi = 16807 * (seed >> 16);
	lo += (hi & 0x7fff)	<< 16;
	lo += hi >> 15;
	
	if (lo > 0x7fffffff) lo -= 0x7fffffff;
	
	return (seed = (int32)lo);
}



