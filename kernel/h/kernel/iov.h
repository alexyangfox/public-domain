#ifndef KERNEL_DEVICE_H
#define KERNEL_DEVICE_H

#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/msg.h>
#include <kernel/proc.h>




/*
 *
 */

struct IOVec
{
	void *base;
	size_t len;
};





    
#endif
