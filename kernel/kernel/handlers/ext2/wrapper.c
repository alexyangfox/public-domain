#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/kmalloc.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include "ext2.h"




/*
 */

DEVICE_CTOR (ext2_init, ext2_ctor);
DEVICE_DTOR (ext2_exit, ext2_dtor);

struct Mutex ext2_mutex;




/*
 */

int ext2_init (void)
{
	
	ext2_handler.dclass = DCLASS_FILESYSTEM;
	ext2_handler.name = "ext2";
	ext2_handler.opendevice = &ext2_opendevice;
	ext2_handler.closedevice = &ext2_closedevice;
	ext2_handler.beginio = &ext2_beginio;
	ext2_handler.abortio = &ext2_abortio;
	ext2_handler.unit_data = NULL;
	
	AddDevice (&ext2_handler);

	MutexInit (&ext2_mutex);

	return 0;
}




/*
 */

int ext2_exit (void)
{
	return 0;
}




/*
 * Use opendevice to mount the drive?  Pass pointer to block device
 * and partition offset.
 *
 * Or OpenDevice just does initialization, other command creates/frees fsbs?
 */

int ext2_opendevice (void *ioreq, uint32 flags)
{
	struct FSReq *fsreq = ioreq;
	struct Ext2SB *fsb;
	struct Mount *mount;
	
	
	KPRINTF ("ext2_opendevice ()");
	
	if ((fsb = KMalloc (sizeof (struct ext2SB))) != NULL)
	{
		MutexInit (&fsb->mutex);

		if (InitCache(fsb, 512, BUF_IMMED) == 0)
		{
			mount = fsreq->mount;
			
			fsb->device = mount->blkdev->blk_device;
			fsb->features = mount->blkdev->features;
			fsb->validated = FALSE;
			
			fsb->root_node.dirent.attributes = ATTR_DIRECTORY;
			fsb->root_node.fsb = fsb;
			fsb->reference_cnt = 0;
			LIST_INIT (&fsb->node_list);
				
			fsb->partition_start = mount->partition_start;
			fsb->partition_size = mount->partition_end - mount->partition_start;
			
			if (fsb->features & BLK_FF_REMOVABLE)
				fsb->removable = TRUE;
			else
				fsb->removable = FALSE;

			mount->data = fsb;
	
			/* Need to validate partition here */	
			/* FreeCache/Free on failure */
						
			
			if (ext2ValidateBPB (fsb, fsb->partition_size) == 0)
			{
				return 0;
			}
			
			FreeCache (fsb);
		}

		KFree (fsb);
	}
	
	fsreq->error = ENOMEM;
	fsreq->rc = -1;
	
	KPRINTF ("ext2 OpenDevice FAIL");
	return -1;
}




/*
 * Close device, invalidate filps and fsb.  Let last close remove the fsb?
 */

int ext2_closedevice (void *ioreq)
{
	struct FSReq *fsreq = ioreq;
	struct Ext2SB *fsb;
	struct Mount *mount;
	
	
	mount = fsreq->mount;
	fsb = mount->data;
	
	if (fsb->reference_cnt == 0)
	{
		KFree (fsb);
		fsreq->error = 0;
		fsreq->rc = 0;
		return 0;
	}
	else
	{
		fsreq->error = EBUSY;
		fsreq->rc = -1;
		return -1;
	}
}




/*
 *
 */

void ext2_beginio (void *ioreq)
{
	struct FSReq *fsreq = ioreq;
	
	KPRINTF ("ext2_beginio");
	
	fsreq->flags |= IOF_QUICK;
		
	MutexLock (&ext2_mutex);
	
	switch (fsreq->cmd)
	{
		case FS_CMD_OPEN:
			ext2DoOpen (fsreq);
			break;
			
		case FS_CMD_CLOSE:
		case FS_CMD_CLOSEDIR:
			ext2DoClose (fsreq);
			break;
		
		case FS_CMD_DUP:
			ext2DoDup (fsreq);
			break;
		
		case FS_CMD_READ:
			ext2DoRead (fsreq);
			break;
		
		case FS_CMD_WRITE:
			ext2DoWrite (fsreq);
			break;
			
		case FS_CMD_LSEEK:
			ext2DoSeek (fsreq);
			break;
		
		case FS_CMD_OPENDIR:
			ext2DoOpendir (fsreq);
			break;
		
		case FS_CMD_READDIR:
			ext2DoReaddir (fsreq);
			break;
		
		case FS_CMD_REWINDDIR:
			ext2DoRewinddir (fsreq);
			break;
		
		case FS_CMD_FSTAT:
			ext2DoFstat (fsreq);
			break;
		
		case FS_CMD_STAT:
			ext2DoStat (fsreq);
			break;
		
		case FS_CMD_TRUNCATE:
			ext2DoTruncate (fsreq);
			break;
		
		case FS_CMD_MKDIR:
			ext2DoMkdir (fsreq);
			break;
		
		case FS_CMD_UNLINK:
			ext2DoUnlink (fsreq);
			break;
			
		case FS_CMD_RMDIR:
			ext2DoRmdir (fsreq);
			break;
		
		case FS_CMD_ISATTY:
			fsreq->error = 0;
			fsreq->rc = 0;
			break;

		case FS_CMD_FORMAT:
			ext2DoFormat (fsreq);
			break;

		case FS_CMD_RENAME:
			ext2DoRename (fsreq);
			break;

		case FS_CMD_CHMOD:
			ext2DoChmod (fsreq);
			break;

		case FS_CMD_CHOWN:
			ext2DoChown (fsreq);
			break;
		
		default:
		{
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			KPANIC ("ext2 ENOSYS");
		}
	}
	
	
	MutexUnlock (&ext2_mutex);
}




/*
 *
 */

int ext2_abortio (void *ioreq)
{
	return 0;
}
