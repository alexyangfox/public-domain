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
#include "ata.h"




struct Device ata_device =
{
	{NULL, NULL},
	"ata.device",
	1,
	0,
	{0, {NULL, NULL}},
	&ata_init,
	&ata_expunge,
	&ata_opendevice,
	&ata_closedevice,
	&ata_beginio,
	&ata_abortio
};

struct Resident ata_resident =
{
	RESIDENT_MAGIC1,
	RESIDENT_MAGIC2,
	RESIDENT_MAGIC3,
	RESIDENT_MAGIC4,
	&ata_resident,
	RFLG_AUTOINIT,
	10,
	RTYPE_DEVICE,
	10,
	{NULL, NULL},
	"ata.device",
	"id",
	&ata_init,
	&ata_device,
	NULL
};

static void *elf_header;




/*
 * d_init();
 */

int ata_init (void *elf)
{
	elf_header = elf;
	

	if ((ata_pid = KSpawn (AtaTask, NULL, 20, "ata.driver")) != -1)
	{		
		KWait (SIGF_INIT);
		
		if (ata_init_error == 0)
		{
			AddDevice (&ata_device);
			return 0;
		}

		WaitPid (ata_pid, NULL, 0);
	}

	return -1;
}




/*
 *
 */

void *ata_expunge (void)
{
	KPRINTF ("ata_expunge()");

	KSignal (ata_pid, SIG_TERM);
	WaitPid (ata_pid, NULL, 0);

	RemDevice (&ata_device);
		
	return elf_header;
}




/*
 *
 */
 
int ata_opendevice (int unit, void *ioreq, uint32 flags)
{
	struct BlkReq *blkreq = ioreq;
	
	
	if (unit <= MAX_ATA_DRIVES)
	{
		ata_drive[unit].reference_cnt++;
		
		blkreq->device = &ata_device;
		blkreq->unitp = &ata_drive[unit];
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

int ata_closedevice (void *ioreq)
{
	struct BlkReq *blkreq = ioreq;
	struct Ata *ata;
	
	ata = blkreq->unitp;
	ata->reference_cnt--;
	
	blkreq->error = 0;
	blkreq->rc = 0;
	
	return 0;
}




/*
 *
 */
 
void ata_beginio (void *ioreq)
{
	struct BlkReq *blkreq = ioreq;
	
	blkreq->flags &= ~IOF_QUICK;
	PutMsg (ata_msgport, &blkreq->msg); 
}




/*
 *
 */
 
int ata_abortio (void *ioreq)
{
	return 0;
}


