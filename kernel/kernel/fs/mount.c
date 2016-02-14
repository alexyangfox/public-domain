#include <kernel/types.h>
#include <kernel/sync.h>
#include <kernel/fs.h>
#include <kernel/kmalloc.h>
#include <kernel/device.h>
#include <kernel/utility.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/dbg.h>






/*
 * MountBootDevices();
 *
 * FIXME:  Mount code should be moved into the /fs directory,  but it is sort
 * of related to device/io initialization so was placed here.
 *
 */

void MountBootDevices (void)
{
	struct FSReq fsreq;
	struct BootEntry *be;
	struct MountEnviron *me;
	int rc;
	

	KPRINTF ("MountBootDevices()");

	MutexLock (&mount_mutex);
	
	be = LIST_HEAD (&bootentry_list);
	
	while (be != NULL)
	{
		me = be->me;

		KPRINTF ("Mounting... (%s)", me->mount_name);

		fsreq.me = be->me;
		rc = OpenDevice (me->handler_name, me->handler_unit, &fsreq, me->handler_flags);
		
		be = LIST_NEXT (be, boot_entry);
	}
	
	MutexUnlock (&mount_mutex);
}




/*
 *
 */

void UnmountAll (void)
{
	struct Mount *mount, *next;
	struct FSReq fsreq;

	MutexLock (&mount_mutex);
	
	mount = LIST_HEAD(&mount_list);
		
	while (mount != NULL)
	{
		next = LIST_NEXT(mount, mount_list_entry);
		
		if (mount->type == MOUNT_DEVICE)
		{
			KPRINTF ("Unmounting (%s)", mount->name);
			fsreq.device = mount->handler;
			fsreq.unitp = mount->unitp;
			fsreq.as = &kernel_as;
			CloseDevice (&fsreq);
		}
		mount = next;
	}
	
	MutexUnlock (&mount_mutex);
}




/*
 * DetermineBootMount();
 */

int DetermineBootMount (void)
{
	char boot_device[255];

	KPRINTF ("**** Setting Boot Device");
	
	StrLCpy (boot_device, "/", 255);
	StrLCat (boot_device, cfg_boot_prefix, 255);
	
	
	
	SetAssign ("sys", boot_device);
		
	KPRINTF ("**** Boot device set to (%s)", boot_device);
	

	return 0;
}





/*
 * MakeMountEnviron();
 */

struct MountEnviron *AllocMountEnviron (void)
{
	/* Initialize with dummy values, set strings to '\0'; */

	struct MountEnviron *me;
	
	if ((me = KMalloc (sizeof (struct MountEnviron))) != NULL)
	{
		me->mount_name[0] = '\0';
		me->handler_name[0] = '\0';
		me->device_name[0] = '\0';
		me->startup_args[0] = '\0';
		
		return me;
	}
	
	return NULL;
};






/*
 * AddBootMountEnviron();
 */

int AddBootMountEnviron (struct MountEnviron *me)
{
	struct BootEntry *be, *new_be;
	
	
	be = LIST_HEAD (&bootentry_list);
	
	while (be != NULL)
	{
		if (be->me->boot_priority < me->boot_priority)
		{
			if ((new_be = KMalloc (sizeof (struct BootEntry))) != NULL)
			{
				new_be->me = me;
								
				LIST_INSERT_BEFORE (&bootentry_list, be, new_be, boot_entry);
				return 0;
			}
			else
				return -1;
		}
	
		be = LIST_NEXT (be, boot_entry);
	}
	
	
	
	if ((new_be = KMalloc (sizeof (struct BootEntry))) != NULL)
	{
		new_be->me = me;

		LIST_ADD_TAIL (&bootentry_list, new_be, boot_entry);
		return 0;
	}
	else
		return -1;
}




/*
 * MakeMount();
 *
 * Allocates a mount structure and populates its fields.
 */

struct Mount *MakeMount (struct MountEnviron *me, struct Device *device, void *unitp)
{
	struct Mount *mount;
	
	if ((mount = KMalloc (sizeof (struct Mount))) != NULL)
	{
		mount->type = MOUNT_DEVICE;
		mount->name = me->mount_name;
		mount->handler = device;
		mount->unitp = unitp;
		mount->pathname = NULL;

		mount->me = me;
	}
	
	return mount;
};




/*
 * FindMount();
 *
 * Finds a mount,  caller is responsible for locking mount list.
 *
 * UPDATES:
 *
 * Replaced error ENODEV with ENOENT to allow execlp/execvp to work.
 */

struct Mount *FindMount (char *name)
{
	struct Mount *mount;
	
	if ((StrCmp ("", name)) == 0)
		return &root_mount;
	
	mount = LIST_HEAD (&mount_list);
	
	while (mount != NULL)
	{
		if ((StrCmp (mount->name, name)) == 0)
			return mount;
		
		mount = LIST_NEXT (mount, mount_list_entry);
	}
	
	SetError (ENOENT);
	return NULL;
}




/*
 * AddMount();
 *
 * Adds a mount to the mount list.
 *
 * FIXME:  Need to see if mount exists first,  need to return error.
 */

void AddMount (struct Mount *mount)
{
	LIST_ADD_TAIL (&mount_list, mount, mount_list_entry);
}




/*
 * RemMount();
 *
 * Removes a mount to the mount list.
 */

void RemMount (struct Mount *mount)
{
	struct RootFilp *filp;
	
	filp = LIST_HEAD (&root_filp_list);
	
	while (filp != NULL)
	{
		if (filp->seek_mount == mount)
		{
			filp->seek_mount = LIST_NEXT (mount, mount_list_entry);
		}
		
		filp = LIST_NEXT (filp, filp_entry);
	}
	
	LIST_REM_ENTRY (&mount_list, mount, mount_list_entry);
}



