#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/kmalloc.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include <kernel/resident.h>
#include <kernel/block.h>
#include <kernel/buffers.h>
#include "iso9660.h"



struct Device cd_device =
{
	{NULL, NULL},
	"cd.handler",
	1,
	0,
	{0, {NULL, NULL}},
	&cd_init,
	&cd_expunge,
	&cd_opendevice,
	&cd_closedevice,
	&cd_beginio,
	&cd_abortio
};

struct Resident cd_resident =
{
	RESIDENT_MAGIC1,
	RESIDENT_MAGIC2,
	RESIDENT_MAGIC3,
	RESIDENT_MAGIC4,
	&cd_resident,
	RFLG_AUTOINIT,
	10,
	RTYPE_DEVICE,
	10,
	{NULL, NULL},
	"cd.handler",
	"id",
	&cd_init,
	&cd_device,
	NULL
};


static void *elf_header;



/*
 */

int cd_init (void *elf)
{
	KPRINTF ("cd_init()");

	elf_header = elf;
	AddDevice (&cd_device);
	
	return 0;
}




/*
 */

void *cd_expunge (void)
{
	KPRINTF ("cd_expunge()");

	RemDevice (&cd_device);
	return elf_header;
}




/*
 * Use opendevice to mount the drive?  Pass pointer to block device
 * and partition offset.
 *
 * Or OpenDevice just does initialization, other command creates/frees fsbs?
 * **** ONLY CALLED BY MOUNT MANAGER TASK.
 */

int cd_opendevice (int unit, void *ioreq, uint32 flags)
{
	struct FSReq *fsreq = ioreq;
	struct CDSB *cdsb;
	struct MountEnviron *me;
	struct BlkReq blkreq;
	
	
	KPRINTF ("cd_opendevice ()");
	
	
	if ((cdsb = KMalloc (sizeof (struct CDSB))) != NULL)
	{
		me = fsreq->me;
		
		
		if (OpenDevice (me->device_name, me->device_unit, &blkreq, me->device_flags) == 0)
		{	
			/* FIXME:  Shouldn't need it (also in fat handler) */
			MemSet (cdsb, 0, sizeof (struct CDSB)); /* Need it as cdsb->validated isn't initialized */
			
			cdsb->device = blkreq.device;
			cdsb->unitp = blkreq.unitp;
			cdsb->me = fsreq->me;
			cdsb->reference_cnt = 0;
							
			LIST_INIT (&cdsb->node_list);
			LIST_INIT (&cdsb->active_filp_list);
			LIST_INIT (&cdsb->invalid_filp_list);
			
			cdsb->root_node.flags = ISO_DIRECTORY;
			cdsb->root_node.cdsb = cdsb;
			cdsb->root_node.reference_cnt = 0;
			
			if ((cdsb->buf = CreateBuf (cdsb->device, cdsb->unitp,
							me->buffer_cnt, me->block_size,
							me->partition_start, me->partition_end,
							me->writethru_critical, me->writeback_delay,
							me->max_transfer)) != NULL)
			{
				if (KSpawn (CDTask, cdsb, 10, "cd.handler") != -1)
				{		
					KWait (SIGF_INIT);
					
					RWWriteLock (&mountlist_rwlock);
					cd_device.reference_cnt ++;
					
					cdsb->device_mount = MakeMount (cdsb->me, &cd_device, cdsb);
					AddMount (cdsb->device_mount);
					
					RWUnlock (&mountlist_rwlock);

					fsreq->unitp = cdsb;
					fsreq->error = 0;
					fsreq->rc = 0;
					return 0;
				}
			}
		
			CloseDevice (&blkreq);
		}
		
		KFree (cdsb);
	}
	
	fsreq->error = IOERR_OPENFAIL;
	fsreq->rc = -1;
	
	KPANIC ("CD OpenDevice FAIL");
	return -1;
}




/*
 * cd_closedevice();
 *
 * Stop worker task, close cache handle and free resources.
 */

int cd_closedevice (void *ioreq)
{
	struct FSReq *fsreq = ioreq;
	struct BlkReq blkreq;
	struct CDSB *cdsb;

	KPRINTF ("cd_closedevice()");
	
	cdsb = fsreq->unitp;

	RWWriteLock (&mountlist_rwlock);
	
	if (cdsb->reference_cnt == 0)
	{
		KSignal (cdsb->pid, SIG_TERM);
		WaitPid (cdsb->pid, NULL, 0);
		
		blkreq.device = cdsb->device;
		blkreq.unitp = cdsb->unitp;
		
		CloseDevice (&blkreq);
	
		cd_device.reference_cnt --;	
		RemMount (cdsb->device_mount);

		FreeBuf (cdsb->buf);
		KFree (cdsb);
		
		fsreq->error = 0;
		fsreq->rc = 0;
	}
	else
	{
		fsreq->error = EBUSY;
		fsreq->rc = -1;
	}
	
	RWUnlock (&mountlist_rwlock);
		
	KPRINTF ("cd_closedevice() +");
	
	return 0;
}




/*
 *
 */

void cd_beginio (void *ioreq)
{
	struct FSReq *fsreq = ioreq;
	struct CDSB *cdsb;
	struct CDFilp *filp;	
	
	
	fsreq->flags &= ~IOF_QUICK;

	switch (fsreq->cmd)
	{
		case FS_CMD_OPEN:
		case FS_CMD_OPENDIR:
		case FS_CMD_STAT:
		case FS_CMD_MKDIR:
		case FS_CMD_UNLINK:
		case FS_CMD_RMDIR:
		case FS_CMD_FORMAT:
		case FS_CMD_RENAME:
		case FS_CMD_INHIBIT:
		case FS_CMD_UNINHIBIT:
			cdsb = fsreq->unitp;
			PutMsg (cdsb->msgport, &fsreq->msg);
			break;

		case FS_CMD_CLOSE:
		case FS_CMD_CLOSEDIR:
		case FS_CMD_DUP:
		case FS_CMD_READ:
		case FS_CMD_WRITE:
		case FS_CMD_LSEEK:
		case FS_CMD_READDIR:
		case FS_CMD_REWINDDIR:
		case FS_CMD_ISATTY:
		case FS_CMD_CHMOD:
		case FS_CMD_CHOWN:
		case FS_CMD_TRUNCATE:
		case FS_CMD_FSTAT:
			filp = fsreq->filp;
			cdsb = filp->cdsb;
			PutMsg (cdsb->msgport, &fsreq->msg);
			break;
			
		default:
			fsreq->flags |= IOF_QUICK;
			fsreq->rc = -1;
			fsreq->error = ENOSYS;
		
	}
}




/*
 *
 */

int cd_abortio (void *ioreq)
{
	return 0;
}
