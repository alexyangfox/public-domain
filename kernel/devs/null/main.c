#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include <kernel/utility.h>
#include <kernel/resident.h>
#include "null.h"



struct Device null_device =
{
	{NULL, NULL},
	"null.ko",
	1,
	0,
	{0, {NULL, NULL}},
	&null_init,
	&null_expunge,
	&null_opendevice,
	&null_closedevice,
	&null_beginio,
	&null_abortio
};

struct Resident null_resident =
{
	RESIDENT_MAGIC1,
	RESIDENT_MAGIC2,
	RESIDENT_MAGIC3,
	RESIDENT_MAGIC4,
	&null_resident,
	RFLG_AUTOINIT,
	10,
	RTYPE_DEVICE,
	10,
	{NULL, NULL},
	"null.handler",
	"id",
	&null_init,
	&null_device,
	NULL
};




/*
 */

static void *elf_header;
struct NullFilp null_filp;
int null_filp_reference_cnt;
struct Mount *null_mount;
struct Mutex null_mutex;


/*
 */

int null_init (void *elf)
{
	struct MountEnviron *me;
	
	
	KPRINTF ("null_init()");

	elf_header = elf;
	AddDevice (&null_device);

	MutexInit (&null_mutex);
	
	return 0;
}




/*
 */

void *null_expunge (void)
{
	RemDevice (&null_device);
	return elf_header;
}




/*
 *
 */

int null_opendevice (int unit, void *ioreq, uint32 flags)
{
	struct FSReq *fsreq = ioreq;
	
	
	RWWriteLock (&mountlist_rwlock);
		
	if (null_device.reference_cnt == 0)
	{
		null_device.reference_cnt = 1;
		null_filp_reference_cnt = 0;
		
		null_filp.device = &null_device; /* Init here and in CMD_OPEN ???????? */
		
		null_mount = MakeMount (fsreq->me, &null_device, NULL);
		AddMount (null_mount);
	
		fsreq->device = &null_device;
		fsreq->unitp = NULL;
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

int null_closedevice (void *ioreq)
{
	struct FSReq *fsreq = ioreq;
	
	KPRINTF ("null_closedevice()");
	
	RWWriteLock (&mountlist_rwlock);
	
	if (null_filp_reference_cnt == 0)
	{
		null_device.reference_cnt = 0;
		RemMount (null_mount);
	
		fsreq->error =0;
		fsreq->rc = 0;
	}	
	else
	{
		fsreq->error = EBUSY;
		fsreq->rc = -1;
	}
	

	RWUnlock (&mountlist_rwlock);
	
	KPRINTF ("null_closedevice()+");
	return fsreq->rc;
}




/*
 * beginio();
 */

void null_beginio (void *ioreq)
{
	struct FSReq *fsreq = ioreq;

	KPRINTF ("null_beginio()");

	fsreq->flags |= IOF_QUICK;

	MutexLock (&null_mutex);

	switch (fsreq->cmd)
	{

		case FS_CMD_OPEN:
			null_filp_reference_cnt ++;
			
			null_filp.device = &null_device;
			
			fsreq->filp = &null_filp;
			fsreq->error = 0;
			fsreq->rc = 0;
			break;

		case FS_CMD_PIPE:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;
					
		case FS_CMD_CLOSE:
			null_filp_reference_cnt --;
			
			fsreq->error = 0;
			fsreq->rc = 0;
			break;
			
		case FS_CMD_DUP:
			null_filp_reference_cnt ++;
			
			fsreq->filp2 = &null_filp;
			fsreq->error = 0;
			fsreq->rc = 0;
			break;
		
		case FS_CMD_READ:
			fsreq->error = 0;
			fsreq->nbytes_transferred = 0;
			fsreq->rc = 0;
			break;

		case FS_CMD_WRITE:
			fsreq->error = 0;
			fsreq->nbytes_transferred = fsreq->count;
			fsreq->rc = 0;
			break;

		case FS_CMD_LSEEK:
			fsreq->error = ENOTBLK;
			fsreq->position = -1;
			break;
			
		case FS_CMD_UNLINK:
			fsreq->error = ENOTBLK;
			fsreq->rc = -1;
			break;

		case FS_CMD_RENAME:
			fsreq->error = ENOTBLK;
			fsreq->rc = -1;
			break;

		case FS_CMD_FTRUNCATE:
			fsreq->error = ENOTBLK;
			fsreq->rc = -1;
			break;

		case FS_CMD_FSTAT:
		{
			struct Stat *stat;
			stat = fsreq->stat;

			stat->st_mode = S_IFCHR  | (S_IRWXU | S_IRWXG | S_IRWXO);
			stat->st_nlink = 1;
			stat->st_uid = 1;
			stat->st_gid = 1;
			stat->st_rdev = 2;
			stat->st_size = 0;
			stat->st_atime = 0;
			stat->st_mtime = 0;
			stat->st_ctime = 0;
			stat->st_blocks = 0;
			
			fsreq->error = 0;
			fsreq->rc = 0;
			break;
		}
		

		case FS_CMD_STAT:
		{
			struct Stat *stat;
			stat = fsreq->stat;
				
			stat->st_mode = S_IFCHR  | (S_IRWXU | S_IRWXG | S_IRWXO);
			stat->st_nlink = 1;
			stat->st_uid = 1;
			stat->st_gid = 1;
			stat->st_rdev = 2;
			stat->st_size = 0;
			stat->st_atime = 0;
			stat->st_mtime = 0;
			stat->st_ctime = 0;
			stat->st_blocks = 0;
			
			fsreq->error = 0;
			fsreq->rc = 0;
			break;
		}
		
		
		case FS_CMD_FSTATFS:
			fsreq->error = ENOTBLK;
			fsreq->rc = -1;
			break;

		case FS_CMD_ISATTY:
			fsreq->error = 0;
			fsreq->rc = 0;
			break;

		case FS_CMD_FSYNC:
			fsreq->error = 0;
			fsreq->rc = 0;
			break;
			
		case FS_CMD_SYNC:
			fsreq->error = 0;
			fsreq->rc = 0;
			break;

		case FS_CMD_MKDIR:
			fsreq->error = ENOTBLK;
			fsreq->rc = -1;
			break;
		
		case FS_CMD_RMDIR:
			fsreq->error = ENOTBLK;
			fsreq->rc = -1;
			break;

		case FS_CMD_OPENDIR:
			fsreq->error = ENOTBLK;
			fsreq->rc = -1;
			break;

		case FS_CMD_CLOSEDIR:
			fsreq->error = ENOTBLK;
			fsreq->rc = -1;
			break;

		case FS_CMD_READDIR:
			fsreq->error = ENOTBLK;
			fsreq->rc = -1;
			break;

		case FS_CMD_REWINDDIR:
			fsreq->error = ENOTBLK;
			fsreq->rc = -1;
			break;

		case FS_CMD_TCGETATTR:
			fsreq->error = ENOTTY;
			fsreq->rc = -1;
			break;

		case FS_CMD_TCSETATTR:
			fsreq->error = ENOTTY;
			fsreq->rc = -1;
			break;

		case FS_CMD_IOCTL:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;
		
		default:
		{
			KPANIC ("NULL Unknown command");
		}
	}
	
	
	MutexUnlock (&null_mutex);
}







int null_abortio (void *ioreq)
{
	return 0;
}






