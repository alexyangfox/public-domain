#include <kernel/types.h>
#include <kernel/error.h>
#include <kernel/fs.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include "console.h"
#include <kernel/resident.h>




struct Device con_device =
{
	{NULL, NULL},
	"con.handler",
	1,						/* version */
	0,						/* reference_cnt */
	{0, {NULL, NULL}},		/* mutex */
	&con_init,
	&con_expunge,
	&con_opendevice,
	&con_closedevice,
	&con_beginio,
	&con_abortio
};


struct Resident con_resident =
{
	RESIDENT_MAGIC1,
	RESIDENT_MAGIC2,
	RESIDENT_MAGIC3,
	RESIDENT_MAGIC4,
	&con_resident,
	RFLG_AUTOINIT,
	10,
	RTYPE_DEVICE,
	10,
	{NULL, NULL},
	"con.handler",
	"id",
	&con_init,
	&con_device,
	NULL
};


static void *elf_header;




/*
 */

int con_init (void *elf)
{
	KPRINTF ("con_init()");
	
	elf_header = elf;
	
	
	/* FIXME:  Maybe call adddevice() here,  return pointer to device? */
	
	if (KSpawn (ConsoleTask, NULL, 0, "con.driver") != -1)
	{		
		KWait (SIGF_INIT);
		AddDevice (&con_device);	
		return 0;
	}
	else
		return -1;
}




/*
 */

void *con_expunge (void)
{
	KPRINTF ("con_expunge()");
	
	KSignal (console_pid, SIG_TERM);
	WaitPid (console_pid, NULL, 0);
	
	RemDevice (&con_device);
	
	return elf_header;
}



/*
 *
 */

int con_opendevice (int unit, void *ioreq, uint32 flags)
{
	struct FSReq *fsreq = ioreq;


	RWWriteLock (&mountlist_rwlock);
	
	if (con_device.reference_cnt == 0)			/* FIXME: Convert to con_driver->reference_cnt */
	{
		con_device.reference_cnt = 1;			/* Number of successful OpenDevice() calls */
		console_filp_reference_cnt = 0;
		
		fsreq->device = &con_device;
		fsreq->unitp = NULL;			/* Only a single unit for /con */
		fsreq->error = 0;
		fsreq->rc = 0;
		
		
		/* FIXME: check mount_name does not exist */
		
		console_mount = MakeMount (fsreq->me, &con_device, NULL);
		AddMount (console_mount);
	}
	else
	{
		fsreq->device = &con_device;
		fsreq->unitp = NULL;
		fsreq->error = EBUSY;
		fsreq->rc = -1;
	}

	RWUnlock (&mountlist_rwlock);
	
	return fsreq->rc;
}




/*
 *
 */

int con_closedevice (void *ioreq)
{
	struct FSReq *fsreq = ioreq;
	
	KPRINTF ("con_closedevice()");


	RWWriteLock (&mountlist_rwlock);

	if (console_filp_reference_cnt == 0)
	{
		RemMount (console_mount);
		con_device.reference_cnt = 0;
		
		fsreq->error = 0;
		fsreq->rc = 0;
	}	
	else
	{
		fsreq->error = EBUSY;
		fsreq->rc = -1;
	}

	RWUnlock (&mountlist_rwlock);

	return fsreq->rc;
}




/*
 *
 */

void con_beginio (void *ioreq)
{
	struct FSReq *fsreq = ioreq;
		
	fsreq->flags &= ~IOF_QUICK;
	PutMsg (console_msgport, &fsreq->msg);
}




/*
 *
 */

int con_abortio (void *ioreq)
{
	struct FSReq *fsreq = ioreq;
	
	KPRINTF ("con->AbortIO!");


	fsreq->flags &= ~IOF_QUICK;
	PutMsg (console_msgport, &fsreq->msg);

	return 0;
}
