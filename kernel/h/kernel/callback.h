#ifndef KERNEL_CALLBACK_H
#define KERNEL_CALLBACK_H

#include <kernel/types.h>
#include <kernel/lists.h>




/*
 *
 */

struct Callback
{
	LIST_ENTRY (Callback) callback_entry;
	void *arg;
	void (*callback)(void *arg);
};




    
#endif
