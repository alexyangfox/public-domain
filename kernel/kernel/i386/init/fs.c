#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/lists.h>
#include <kernel/dbg.h>
#include <kernel/vm.h>
#include <kernel/config.h>
#include <kernel/sync.h>
#include <kernel/timer.h>
#include <kernel/i386/i386.h>
#include <kernel/i386/init.h>
#include <kernel/utility.h>



/*
 *
 */
 
void InitFS (void)
{
	MutexInit (&mount_mutex);

	LIST_INIT (&mount_list);
	RWInit (&mountlist_rwlock);

	LIST_INIT (&root_filp_list);
	MutexInit (&root_mutex);
}





/*
 *
 */
 
void FiniFS (void)
{
}
