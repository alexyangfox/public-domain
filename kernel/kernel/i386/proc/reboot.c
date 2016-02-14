#include <kernel/types.h>
#include <kernel/i386/i386.h>
#include <kernel/proc.h>
#include <kernel/config.h>
#include <kernel/utility.h>
#include <kernel/kmalloc.h>
#include <kernel/dbg.h>
#include <kernel/error.h>
#include <kernel/reboot.h>


/*
 * Reboot();
 */

int Reboot (int how)
{
	reboot_requested = TRUE;
	reboot_how = how;
	
	KPRINTF ("Reboot (%d)", how);
	
	/* FIXME: Check for root user */	
	
	KSignal (root_pid, SIG_TERM);

	return 0;
}





