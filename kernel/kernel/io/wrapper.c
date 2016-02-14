#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/fs.h>
#include <kernel/lists.h>
#include <kernel/sync.h>
#include <kernel/utility.h>
#include <kernel/kmalloc.h>
#include <kernel/dbg.h>
#include <kernel/proc.h>
#include "io.h"

 


/*
 */
 
struct Device iomanager_device =
{
	{NULL, NULL},
	"iomanager.device",
	1,
	0,
	{0, {NULL, NULL}},
	NULL,
	NULL,
	NULL,
	NULL,
	&iomanager_beginio,
	&iomanager_abortio
};




/*
 * 
 */

int InitIOManager (void)
{
	KPRINTF ("iomanager_init()");
	
	if (KSpawn (IOManagerTask, NULL, 20, "iomanager_task") != -1)
	{		
		KWait (SIGF_INIT);
		return 0;
	}
	else
		return -1;
}




/*
 * 
 */

int FiniIOManager (void)
{
	KSignal (iomanager_pid, SIG_TERM);
	WaitPid (iomanager_pid, NULL, 0);
	return 0;
}




/*
 *
 */
 
void iomanager_beginio (void *ioreq)
{
	struct IOMReq *iomreq = ioreq;
			
	iomreq->flags &= ~IOF_QUICK;
	PutMsg (iomanager_msgport, &iomreq->msg); 
}




/*
 *
 */
 
int iomanager_abortio (void *ioreq)
{
	return 0;
}


