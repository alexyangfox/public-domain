#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/fs.h>
#include <kernel/lists.h>
#include <kernel/sync.h>
#include <kernel/utility.h>
#include <kernel/kmalloc.h>
#include <kernel/dbg.h>
#include <kernel/proc.h>
#include <kernel/resident.h>
#include "io.h"




/*
 * OpenDevice();
 *
 * Sends a message to the IOManager task to open a device driver.
 * IOManager then calls device's opendevice() function which runs
 * on the context of the IO Manager task.
 *
 * All threads created by the device, either in dev->init() or
 * dev->opendevice() are descendants of the IOManager task.
 *
 * device->opendevice() may make further calls to opendevice(),
 * in this case it executes the nested opendevice() on the IOManager
 * task.
 *
 * During a mount() command OpenDevice() is called with the mount_list
 * writer locked.  This allows filesystem handlers to call AddMount() upon
 * successfully mounting the device.
 *
 * Need some way of flagging devices as being at the the base of nested
 * OpenDevice().  Then replace/remove UnmountAll().
 * 
 */

int OpenDevice (char *name, int unit, void *ioreq, uint32 flags)
{
	void *elf;
	struct Device *device;
	struct StdIOReq *stdreq = ioreq;
	struct IOMReq iomreq;
	struct Resident *resident;
	int rc;
	
	
	if (GetPID() != iomanager_pid)
	{
		/* Send message to IO Manager task */
	
		iomreq.device = &iomanager_device;
		iomreq.unitp = NULL;
		iomreq.as = &kernel_as;
		iomreq.cmd = IOM_CMD_OPENDEVICE;
		iomreq.od_name = name;
		iomreq.od_unit = unit;
		iomreq.od_ioreq = ioreq;
		iomreq.od_flags = flags;
		
		DoIO (&iomreq, NULL);
		
		if (iomreq.rc != 0);
			SetError (iomreq.error);
		
		return iomreq.rc;
	}
	else
	{
		/* Execute nested opendevice() on IOManager task context */
	
				
		if ((device = FindDevice (name)) != NULL)
		{
			stdreq->device = device;
			rc = device->opendevice (unit, ioreq, flags);
			return rc;
		}
		else
		{
			/* Use absolute or /sys/dev prefix for pathname */		
		
			if ((elf = LoadDevice (name)) != NULL)
			{
				if ((resident = FindElfResident (elf)) != NULL)
				{
					if (resident->type == RTYPE_DEVICE &&
						resident->flags & RFLG_AUTOINIT && resident->init(elf) != -1)
					{
						if ((device = resident->data) != NULL)
						{
							stdreq->device = device;
							rc = device->opendevice (unit, ioreq, flags);
							return rc;
						}
					}
				}
				
				UnloadDevice (elf);
			}
		}
	}
	
	return -1;
}




/*
 * CloseDevice();
 *
 * Might not be a simple as above if we want to remove a device
 * from the filesystem.
 *
 * The problem is that threads may still be executing code in
 * the filesystem.  Also Volume label handling is awkward, assigns
 * are messy.
 *
 * Perhaps a reader-writer lock around open/close/mount/unmount
 * commands?
 *
 * mount/unmount is a writer lock, open/close/stat etc use reader
 * locks.  Other FS calls use no locks.
 * 
 * Remove volume labels?   Only assigns and mounts?
 * But have a GetVolumeLabel()/Relabel() functions?
 *
 * During an unmount() command CloseDevice() is called with the mount_list
 * writer locked.  This allows filesystem handlers to call RemMount() upon
 * successfully unmounting the device.
 *
 *
 */

int CloseDevice (void *ioreq)
{
	struct StdIOReq *stdreq = ioreq;
	struct Device *device;
	struct IOMReq iomreq;
	void *elf;
	int rc;
	
	
	if (GetPID() != iomanager_pid)
	{
		iomreq.device = &iomanager_device;
		iomreq.unitp = NULL;
		iomreq.as = &kernel_as;
		iomreq.cmd = IOM_CMD_CLOSEDEVICE;
		iomreq.cd_ioreq = ioreq;
		
		DoIO (&iomreq, NULL);
		
		if (iomreq.rc != 0);
			SetError (iomreq.error);
		
		KPRINTF ("iomreq.rc = %d", iomreq.rc);
		return iomreq.rc;
	}
	else
	{
		device = stdreq->device;
	
		rc = device->closedevice(stdreq);

		if (device->reference_cnt == 0)
		{
			elf = device->expunge();
		
			if (elf != NULL)
				UnloadDevice (elf);
		}
		
		return rc;
	}
}




/*
 *
 */

struct Device *FindDevice (char *name)
{
	struct Device *device;

	KPRINTF ("FindDevice (%s)", name);
	
	device = LIST_HEAD (&device_list);
	
	while (device != NULL)
	{
		if (StrCmp (name, device->name) == 0)
			break;
		
		device = LIST_NEXT (device, device_entry);
	}	
	
	return device;
}



