#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/fs.h>
#include <kernel/lists.h>
#include <kernel/sync.h>
#include <kernel/utility.h>
#include <kernel/kmalloc.h>
#include <kernel/dbg.h>
#include <kernel/proc.h>




/*
 * AddDevice();
 */

int AddDevice (struct Device *device)
{
	/* FIXME: Check for similar device name */

	LIST_ADD_TAIL (&device_list, device, device_entry);
	
	return 0;
}




/*
 * RemDevice();
 */

int RemDevice (struct Device *device)
{
	LIST_REM_ENTRY (&device_list, device, device_entry);

	return 0;
}


