#include <kernel/types.h>
#include <kernel/i386/i386.h>
#include <kernel/device.h>
#include <kernel/sync.h>
#include <kernel/proc.h>
#include <kernel/msg.h>
#include <kernel/vm.h>
#include <kernel/timer.h>
#include <kernel/dbg.h>
#include <kernel/resident.h>
#include "floppy.h"


struct Device floppy_device =
{
	{NULL, NULL},
	"floppy.device",
	1,
	0,
	{0, {NULL, NULL}},
	&floppy_init,
	&floppy_expunge,
	&floppy_opendevice,
	&floppy_closedevice,
	&floppy_beginio,
	&floppy_abortio
};

struct Resident floppy_resident =
{
	RESIDENT_MAGIC1,
	RESIDENT_MAGIC2,
	RESIDENT_MAGIC3,
	RESIDENT_MAGIC4,
	&floppy_resident,
	RFLG_AUTOINIT,
	10,
	RTYPE_DEVICE,
	10,
	{NULL, NULL},
	"floppy.device",
	"id",
	&floppy_init,
	&floppy_device,
	NULL
};


static void *elf_header;




/*
 * floppy_init();
 */

int floppy_init (void *elf)
{	
	KPRINTF ("floppy_init()");
	
	elf_header = elf;
	
	
	if ((floppy_pid = KSpawn (FloppyTask, NULL, 2, "floppy.driver")) != -1)
	{
		KWait (SIGF_INIT);

		if (floppy_init_error == 0)
		{
			AddDevice (&floppy_device);
			return 0;
		}
		
		WaitPid (floppy_pid, NULL, 0);
	}

	return -1;
}





/*
 *
 */

void *floppy_expunge (void)
{
	KPRINTF ("floppy_expunge()");

	if (floppy_pid != -1)
	{
		KSignal (floppy_pid, SIG_TERM);
		WaitPid (floppy_pid, NULL, 0);
		RemDevice (&floppy_device);
	}
		
	return elf_header;
}




/*
 *
 */
 
int floppy_opendevice (int unit, void *ioreq, uint32 flags)
{
	struct BlkReq *blkreq = ioreq;
	
	
	if (unit <= MAX_FLOPPY_DRIVES)
	{
		floppy_drive[unit].reference_cnt++;
		
		blkreq->device = &floppy_device;
		blkreq->unitp = &floppy_drive[unit];
		blkreq->error = 0;
		blkreq->rc = 0;
		return 0;
	}
	else
	{
		blkreq->error = IOERR_OPENFAIL;
		blkreq->rc = -1;
		return -1;
	}
}




/*
 *
 */

int floppy_closedevice (void *ioreq)
{
	struct BlkReq *blkreq = ioreq;
	struct Floppy *fd;
	
	KPRINTF ("floppy_closedevice()");
	
	fd = blkreq->unitp;
	
	fd->reference_cnt--;
	
	blkreq->error = 0;
	blkreq->rc = 0;
		
	return 0;
}




/*
 *
 */
 
void floppy_beginio (void *ioreq)
{
	struct BlkReq *blkreq = ioreq;
	
	blkreq->flags &= ~IOF_QUICK;
	PutMsg (floppy_msgport, &blkreq->msg); 
}




/*
 *
 */
 
int floppy_abortio (void *ioreq)
{
	return 0;
}


