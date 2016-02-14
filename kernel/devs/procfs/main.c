#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/kmalloc.h>
#include <kernel/dbg.h>
#include <kernel/utility.h>
#include <kernel/resident.h>
#include "procfs.h"




struct Device procfs_device =
{
	{NULL, NULL},
	"procfs.ko",
	1,
	0,
	{0, {NULL, NULL}},
	&procfs_init,
	&procfs_expunge,
	&procfs_opendevice,
	&procfs_closedevice,
	&procfs_beginio,
	&procfs_abortio
};

struct Resident procfs_resident =
{
	RESIDENT_MAGIC1,
	RESIDENT_MAGIC2,
	RESIDENT_MAGIC3,
	RESIDENT_MAGIC4,
	&procfs_resident,
	RFLG_AUTOINIT,
	10,
	RTYPE_DEVICE,
	10,
	{NULL, NULL},
	"procfs.handler",
	"id",
	&procfs_init,
	&procfs_device,
	NULL
};




/*
 */

static void *elf_header;
struct Mutex procfs_mutex;
int procfs_filp_reference_cnt;
struct Mount *procfs_mount;



/*
 */

int procfs_init (void *elf)
{
	KPRINTF ("procfs_init()");

	elf_header = elf;
	AddDevice (&procfs_device);

	MutexInit (&procfs_mutex);
	
	return 0;
}




/*
 *
 */

void *procfs_expunge (void)
{
	RemDevice (&procfs_device);
	return elf_header;
}




/*
 *
 */

int procfs_opendevice (int unit, void *ioreq, uint32 flags)
{
	struct FSReq *fsreq = ioreq;
	
	
	RWWriteLock (&mountlist_rwlock);

	if (procfs_device.reference_cnt == 0)
	{
		procfs_device.reference_cnt ++;
		procfs_filp_reference_cnt = 0;
		
		fsreq->device = &procfs_device;
		fsreq->unitp = NULL;
		fsreq->error = 0;
		fsreq->rc = 0;
		
		procfs_mount = MakeMount (fsreq->me, &procfs_device, NULL);
		AddMount (procfs_mount);
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

int procfs_closedevice (void *ioreq)
{
	struct FSReq *fsreq = ioreq;
	

	RWWriteLock (&mountlist_rwlock);
		
	if (procfs_filp_reference_cnt == 0)
	{
		procfs_device.reference_cnt --;
		RemMount (procfs_mount);
		
		fsreq->error =0;
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
 * beginio();
 */

void procfs_beginio (void *ioreq)
{
	struct FSReq *fsreq = ioreq;

	MutexLock (&procfs_mutex);

	SetError (0);

	fsreq->flags |= IOF_QUICK;

	switch (fsreq->cmd)
	{
		case FS_CMD_OPEN:
			ProcFSOpen (fsreq);
			break;

		case FS_CMD_PIPE:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;
					
		case FS_CMD_CLOSE:
			ProcFSClose (fsreq);
			break;
			
		case FS_CMD_DUP:
			ProcFSDup (fsreq);
			break;
		
		case FS_CMD_READ:
			ProcFSRead (fsreq);
			break;

		case FS_CMD_WRITE:
			fsreq->error = EBADF;
			fsreq->rc = -1;
			break;

		case FS_CMD_LSEEK:
			fsreq->error = ENOTBLK;
			fsreq->rc = -1;
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
			ProcFSFStat (fsreq);
			break;
				
		case FS_CMD_STAT:
			ProcFSStat(fsreq);
			break;
		
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
			ProcFSOpenDir (fsreq);
			break;

		case FS_CMD_CLOSEDIR:
			ProcFSClose (fsreq);
			break;

		case FS_CMD_READDIR:
			ProcFSReadDir (fsreq);
			break;

		case FS_CMD_REWINDDIR:
			ProcFSRewindDir (fsreq);
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
			KPANIC ("ProcFS Unknown command");
		}
	}

	MutexUnlock (&procfs_mutex);
}




/*
 *
 */

int procfs_abortio (void *ioreq)
{
	return 0;
}



char procfs_state[12] = {'u', 'z', 'R', 'r', 'i', 'w', 'm', 'c', '<', '>', 's', 'W'};


/*
 *
 */

void ProcFSOpen (struct FSReq *fsreq)
{
	struct ProcfsFilp *filp;
	struct Process *proc;
	struct ProcfsLookup lookup;
	int ppid;
	char state_ch;
	int state;
	

	if (fsreq->oflags & O_CREAT)
	{
		fsreq->error = EPERM;
		fsreq->rc = -1;
		return;
	}
	
	
	lookup.pathname = fsreq->path;
	
	if (ProcFSLookup (&lookup) == 0)
	{
		if (lookup.isdir == 0)
		{
			if ((filp = KMalloc (sizeof (struct ProcfsFilp))) != NULL)
			{
				MutexLock(&proc_mutex);
				
				proc = lookup.proc;
								
				KASSERT (proc != NULL);
				
				KASSERT (proc >= process && proc <= (process + process_cnt));
				
				if (proc->state != PROC_STATE_UNALLOC)
				{
					if (proc->parent == NULL)
						ppid = 0;
					else
						ppid = proc->parent->pid;
					
					state = proc->state;
					
					if (state < 0 || state > 11)
						state_ch = '*';
					else
						state_ch = procfs_state[state];
										
					filp->device = &procfs_device;
					
					filp->isdir = FALSE;
					filp->fpos = 0;
					filp->reference_cnt = 1;
				
					procfs_filp_reference_cnt ++;
				
					MutexUnlock(&proc_mutex);
				
					fsreq->filp = filp;
					fsreq->error = 0;
					fsreq->rc = 0;
					return;
				}
				else
				{
					MutexUnlock(&proc_mutex);
					SetError (ENOENT);
				}
				
				KFree (filp);
			}
			else
				SetError (ENOMEM);
		}
		else
			SetError(EISDIR);
	}
	
	fsreq->filp = NULL;
	fsreq->error = GetError();
	fsreq->rc = -1;
}



/*
 *
 */

void ProcFSDup (struct FSReq *fsreq)
{
	struct ProcfsFilp *filp;
	
	filp = fsreq->filp;
	
	filp->reference_cnt ++;
	
	procfs_filp_reference_cnt ++;
	
	fsreq->filp2 = &filp;
	fsreq->error = 0;
	fsreq->rc = 0;
}




/*
 *
 */

void ProcFSClose (struct FSReq *fsreq)
{
	struct ProcfsFilp *filp;
	
	filp = fsreq->filp;
	
	filp->reference_cnt --;

	procfs_filp_reference_cnt --;
	
	if (filp->reference_cnt == 0)
		KFree (filp);
	
	fsreq->error = 0;
	fsreq->rc = 0;
}




/*
 *
 */

void ProcFSOpenDir (struct FSReq *fsreq)
{
	struct ProcfsFilp *filp;
	struct ProcfsLookup lookup;
	
	
	lookup.pathname = fsreq->path;
	
	if (ProcFSLookup (&lookup) == 0)
	{
		if (lookup.isdir == 1)
		{
			if ((filp = KMalloc (sizeof (struct ProcfsFilp))) != NULL)
			{
				filp->device = &procfs_device;
					
				filp->isdir = TRUE;
				filp->fpos = 0;
				filp->reference_cnt = 1;
				
				fsreq->filp = filp;
				fsreq->error = 0;
				fsreq->rc = 0;
				
				procfs_filp_reference_cnt ++;
				
				return;
			}
			else
				SetError (ENOMEM);
		}
		else
			SetError(EISDIR);
	}
	
	fsreq->filp = NULL;
	fsreq->error = GetError();
	fsreq->rc = -1;
}




/*
 *
 */

void ProcFSReadDir (struct FSReq *fsreq)
{
	struct ProcfsFilp *filp;
	int pid;
	int len;
	struct Dirent *dirent;
	
	
	filp = fsreq->filp;
	dirent = fsreq->dirent;
	
	
	MutexLock (&proc_mutex);
	
	if (filp->isdir == 1)
	{
		while (filp->fpos < process_cnt)
		{
			if ((process + filp->fpos)->state != PROC_STATE_UNALLOC)
			{
				pid = (process + filp->fpos)->pid;
				Snprintf (filp->buf, PROCFS_STR_SZ, "%d", pid);
				
				MutexUnlock (&proc_mutex);
				
				filp->fpos++;
							
				len = StrLen (filp->buf);
				
				CopyOut (current_process->user_as, &dirent->d_name, filp->buf, len + 1);
				
				fsreq->error = 0;
				fsreq->rc = 0;
				return;
			}
			
			filp->fpos++;
		}
	
	
		fsreq->dirent = NULL;
		fsreq->error = 0;
		fsreq->rc = -1;	
	}
	else
	{
		fsreq->dirent = NULL;
		fsreq->error = ENOTDIR;
		fsreq->rc = -1;
	}
	
	MutexUnlock (&proc_mutex);
}




/*
 *
 */

void ProcFSRewindDir (struct FSReq *fsreq)
{
	struct ProcfsFilp *filp;
	
		
	filp = fsreq->filp;
	filp->fpos = 0;
	fsreq->error = 0;
	fsreq->rc = 0;
	
}




/*
 *
 */

void ProcFSRead (struct FSReq *fsreq)
{
	struct ProcfsFilp *filp;
	int len;
	int nbytes_read;
	
	
	filp = fsreq->filp;
	
	if (filp->isdir == 0)
	{
		len = StrLen (filp->buf);
		
		if (filp->fpos >= (len + 1))
		{
			nbytes_read = 0;
		}
		else
		{
			nbytes_read = len + 1 - filp->fpos;
			
			if (fsreq->count < nbytes_read)
				nbytes_read = fsreq->count;

			KPRINTF ("CopyOut() proc");
			CopyOut (current_process->user_as, fsreq->buf, filp->buf + filp->fpos, nbytes_read);
			
			filp->fpos += nbytes_read;
		}
	}
	else
	{
		SetError (EISDIR);
		nbytes_read = -1;
	}

	
	fsreq->error = GetError();
	fsreq->nbytes_transferred = nbytes_read;
	fsreq->rc = 0;
}




/*
 *
 */

void ProcFSFStat (struct FSReq *fsreq)
{
	struct ProcfsFilp *filp;
	struct Stat *stat;
	
	
	filp = fsreq->filp;
	stat = fsreq->stat;
		
	if (filp->isdir == 1)
		stat->st_mode = S_IFDIR | (S_IRWXU | S_IRWXG | S_IRWXO);
	else
		stat->st_mode = S_IFCHR | (S_IRWXU | S_IRWXG | S_IRWXO);
		
	stat->st_nlink = 1;
	stat->st_uid = 0;
	stat->st_gid = 0;
	stat->st_rdev = 7;	/* Maybe use address of Mount structure? or some other variable */
	stat->st_size = 0;
	stat->st_atime = 0;
	stat->st_mtime = 0;  /* Pipe handler creation times */
	stat->st_ctime = 0;
	stat->st_blocks = 0;
	stat->st_ino = 0;
				
	fsreq->error = 0;
	fsreq->rc = 0;
}




/*
 *
 */

void ProcFSStat (struct FSReq *fsreq)
{
	struct ProcfsLookup lookup;
	struct Stat *stat;
	
	KPRINTF ("ProcFSStat()");
	
	lookup.pathname = fsreq->path;
	
	if (ProcFSLookup (&lookup) == 0)
	{
		stat = fsreq->stat;
		
		if (lookup.isdir == 1)
			stat->st_mode = S_IFDIR | (S_IRWXU | S_IRWXG | S_IRWXO);
		else
			stat->st_mode = S_IFCHR | (S_IRWXU | S_IRWXG | S_IRWXO);
			
		stat->st_nlink = 1;
		stat->st_uid = 0;
		stat->st_gid = 0;
		stat->st_rdev = 7;	/* Maybe use address of Mount structure? or some other variable */
		stat->st_size = 0;
		stat->st_atime = 0;
		stat->st_mtime = 0;  /* Pipe handler creation times */
		stat->st_ctime = 0;
		stat->st_blocks = 0;
		stat->st_ino = 0;
					
		fsreq->error = 0;
		fsreq->rc = 0;
		return;
	}
	
	fsreq->error = GetError();
	fsreq->rc = -1;
}

