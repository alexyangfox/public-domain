#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/kmalloc.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include <kernel/resident.h>
#include <kernel/buffers.h>
#include <kernel/block.h>
#include "fat.h"



struct Device fat_device =
{
	{NULL, NULL},
	"fat.handler",
	1,
	0,
	{0, {NULL, NULL}},
	&fat_init,
	&fat_expunge,
	&fat_opendevice,
	&fat_closedevice,
	&fat_beginio,
	&fat_abortio
};

struct Resident fat_resident =
{
	RESIDENT_MAGIC1,
	RESIDENT_MAGIC2,
	RESIDENT_MAGIC3,
	RESIDENT_MAGIC4,
	&fat_resident,
	RFLG_AUTOINIT,
	10,
	RTYPE_DEVICE,
	10,
	{NULL, NULL},
	"fat.handler",
	"id",
	&fat_init,
	&fat_device,
	NULL
};



static void *elf_header;



/*
 */

int fat_init (void *elf)
{
	KPRINTF ("fat_init()");

	elf_header = elf;
	AddDevice (&fat_device);

	return 0;
}




/*
 */

void *fat_expunge (void)
{
	KPRINTF ("fat_expunge()");

	RemDevice (&fat_device);
	return elf_header;
}




/*
 * Use opendevice to mount the drive?  Pass pointer to block device
 * and partition offset.
 *
 * Or OpenDevice just does initialization, other command creates/frees fsbs?
 * **** ONLY CALLED BY MOUNT MANAGER TASK.
 */

int fat_opendevice (int unit, void *ioreq, uint32 flags)
{
	struct FSReq *fsreq = ioreq;
	struct FatSB *fsb;
	struct BlkReq blkreq;
	struct MountEnviron *me;
	
	
	KPRINTF ("fat_opendevice ()");
	
	
	if ((fsb = KMalloc (sizeof (struct FatSB))) != NULL)
	{
		me = fsreq->me;
		
		if (OpenDevice (me->device_name, me->device_unit, &blkreq, me->device_flags) == 0)
		{
			MemSet (fsb, 0, sizeof (struct FatSB));
			
			fsb->device = blkreq.device;
			fsb->unitp = blkreq.unitp;
			
			fsb->me = fsreq->me;

			LIST_INIT (&fsb->node_list);
			
			LIST_INIT (&fsb->active_filp_list);
			LIST_INIT (&fsb->invalid_filp_list);
			
			fsb->root_node.dirent.attributes = ATTR_DIRECTORY;
			fsb->root_node.fsb = fsb;
			fsb->root_node.reference_cnt = 0;
			fsb->root_node.dirent_sector = 0;
			fsb->root_node.dirent_offset = 0;
			fsb->root_node.hint_cluster = 0;
			fsb->root_node.hint_offset = 0;
			
			
			LIST_INIT (&fsb->root_node.filp_list);

			FatSetTime (fsb, &fsb->root_node.dirent, ST_ATIME | ST_MTIME | ST_CTIME);

			fsb->partition_start = me->partition_start;
			fsb->partition_size = me->partition_end - me->partition_start;
			fsb->removable = me->removable;
			fsb->writable = me->writable;
			fsb->validated = FALSE;

			
			if ((fsb->buf = CreateBuf (fsb->device, fsb->unitp,
							me->buffer_cnt, me->block_size,
							me->partition_start, me->partition_end,
							me->writethru_critical, me->writeback_delay,
							me->max_transfer)) != NULL)
			{		
				if (KSpawn (FatTask, fsb, 10, "fat.handler") != -1)
				{		
					KWait (SIGF_INIT);
					
					/* FIXME: check mount_name does not exist */
									
					RWWriteLock (&mountlist_rwlock);
	
					fat_device.reference_cnt ++;
					fsb->device_mount = MakeMount (fsb->me, &fat_device, fsb);
					AddMount (fsb->device_mount);
					
					RWUnlock (&mountlist_rwlock);
		
					fsreq->unitp = fsb;
					fsreq->error = 0;
					fsreq->rc = 0;
					return 0;
				}
				
				FreeBuf (fsb->buf);
			}
						
			CloseDevice (&blkreq);
		}
		
		KFree (fsb);
	}
	
	fsreq->error = IOERR_OPENFAIL;
	fsreq->rc = -1;
	return -1;
}




/*
 * fat_closedevice();
 *
 * Stop worker task, close cache handle and free resources.
 */

int fat_closedevice (void *ioreq)
{
	struct FSReq *fsreq = ioreq;
	struct BlkReq blkreq;
	struct FatSB *fsb;

	KPRINTF ("fat_closedevice()");
	
	fsb = fsreq->unitp;
	



	RWWriteLock (&mountlist_rwlock);
	
	if (fsb->reference_cnt == 0)
	{
		KSignal (fsb->pid, SIG_TERM);
		WaitPid (fsb->pid, NULL, 0);
		
		blkreq.device = fsb->device;
		blkreq.unitp = fsb->unitp;
		CloseDevice (&blkreq);

		fat_device.reference_cnt --;		
		RemMount (fsb->device_mount);
			
		FreeBuf (fsb->buf);			
		KFree (fsb);
	
		fsreq->error = 0;
		fsreq->rc = 0;
	}
	else
	{
		fsreq->error = EBUSY;
		fsreq->rc = -1;
	}
	
	RWUnlock (&mountlist_rwlock);
	

	KPRINTF ("fat_closedevice() +");
	
	return 0;
}




/*
 *
 */

void fat_beginio (void *ioreq)
{
	struct FSReq *fsreq = ioreq;
	struct FatSB *fsb;
	struct FatFilp *filp;	
	

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
			fsb = fsreq->unitp;
			PutMsg (fsb->msgport, &fsreq->msg);
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
			fsb = filp->fsb;
			PutMsg (fsb->msgport, &fsreq->msg);
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

int fat_abortio (void *ioreq)
{
	return 0;
}
