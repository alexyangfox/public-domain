#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/sync.h>
#include <kernel/resident.h>
#include <kernel/utility.h>
#include "pipe.h"




struct Device pipe_device =
{
	{NULL, NULL},
	"pipe.ko",
	1,
	0,
	{0, {NULL, NULL}},
	&pipe_init,
	&pipe_expunge,
	&pipe_opendevice,
	&pipe_closedevice,
	&pipe_beginio,
	&pipe_abortio
};

struct Resident pipe_resident =
{
	RESIDENT_MAGIC1,
	RESIDENT_MAGIC2,
	RESIDENT_MAGIC3,
	RESIDENT_MAGIC4,
	&pipe_resident,
	RFLG_AUTOINIT,
	10,
	RTYPE_DEVICE,
	10,
	{NULL, NULL},
	"pipe.handler",
	"id",
	&pipe_init,
	&pipe_device,
	NULL
};






/*
 */
 
static void *elf_header;
struct Mutex pipe_mutex;
bool pipe_mounted;
int pipe_cnt;
struct Mount *pipe_mount;



/*
 *
 */

int pipe_init (void *elf)
{
	KPRINTF ("pipe_init()");

	elf_header = elf;
	AddDevice (&pipe_device);

	MutexInit (&pipe_mutex);

	return 0;
}




/*
 *
 */

void *pipe_expunge (void)
{
	RemDevice (&pipe_device);
	return elf_header;
}




/*
 *
 */

int pipe_opendevice (int unit, void *ioreq, uint32 flags)
{
	struct FSReq *fsreq = ioreq;

	
	RWWriteLock (&mountlist_rwlock);
	
	if (pipe_device.reference_cnt == 0)
	{
		pipe_device.reference_cnt = 1;
		
		pipe_cnt = 0;
		
		pipe_mount = MakeMount (fsreq->me, &pipe_device, NULL);
		AddMount (pipe_mount);
		
		fsreq->device = &pipe_device;
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

int pipe_closedevice (void *ioreq)
{
	struct FSReq *fsreq = ioreq;

	RWWriteLock (&mountlist_rwlock);
	
	if (pipe_cnt == 0)
	{
		pipe_device.reference_cnt = 0;
		RemMount (pipe_mount);
		
		pipe_mounted = FALSE;
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

void pipe_beginio (void *ioreq)
{
	struct FSReq *fsreq = ioreq;
	
	
	fsreq->flags |= IOF_QUICK;
	
	switch (fsreq->cmd)
	{
		case FS_CMD_OPEN:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;
			

		case FS_CMD_PIPE:
		{
			struct PipeFilp *filp1, *filp2;
			struct Pipe *pipe;

			if ((pipe = KMalloc (sizeof (struct Pipe))) != NULL)
			{				
				if ((pipe->buf = (uint8 *)KMalloc (PIPE_BUFFER_SZ)) != NULL)
				{
					if ((filp1 = KMalloc (sizeof (struct PipeFilp))) != NULL)
					{
						if ((filp2 = KMalloc (sizeof (struct PipeFilp))) != NULL)
						{
							MutexLock (&pipe_mutex);
							pipe_cnt ++;
							
							pipe->device = &pipe_device;
							pipe->reader_cnt = 1;
							pipe->writer_cnt = 1;
							
							pipe->read_sz = 0;
							pipe->write_sz = PIPE_BUFFER_SZ;
							pipe->w_pos = 0;
							pipe->r_pos = 0;
												
							MutexInit (&pipe->mutex);
							CondInit (&pipe->reader_cond);
							CondInit (&pipe->writer_cond);
							
							filp1->device = &pipe_device;
							filp1->pipe = pipe;
							filp1->is_writer = FALSE;
							filp1->reference_cnt = 1;
							
							filp2->device = &pipe_device;
							filp2->pipe = pipe;
							filp2->is_writer = TRUE;
							filp2->reference_cnt = 1;
						
							MutexUnlock (&pipe_mutex);					
												
							fsreq->filp = filp1;
							fsreq->filp2 = filp2;
							fsreq->error = 0;
							fsreq->rc = 0;
							break;
						}
						
						KFree (filp1);
					}
					
					KFree (pipe->buf);			
				}
				
				KFree (pipe);
			}
			
			fsreq->error = ENOMEM;
			fsreq->rc = -1;
			break;
		}	
			
		case FS_CMD_CLOSE:
		{
			struct PipeFilp *filp;
			struct Pipe *pipe;

			KPRINTF ("PIPE CLOSE");

			filp = fsreq->filp;
			pipe = filp->pipe;
			
			MutexLock (&pipe->mutex);

			filp->reference_cnt --;
						
			if (filp->is_writer == TRUE)
				pipe->writer_cnt --;
			else
				pipe->reader_cnt --;
			
			if (pipe->writer_cnt == 0)
				CondBroadcast (&pipe->reader_cond);
			
			if (pipe->reader_cnt == 0)
				CondBroadcast (&pipe->writer_cond);
			
			if (filp->reference_cnt == 0)
				KFree (filp);
			
			if (pipe->writer_cnt == 0 && pipe->reader_cnt == 0)
			{
				KPRINTF ("Pipe Close() - Freeing pipe pipe_buf = %#010x", pipe->buf);
			
				KFree (pipe->buf);
				KFree (pipe);
				pipe_cnt --;
			}

			MutexUnlock (&pipe->mutex);
			
			fsreq->error = 0;
			fsreq->rc = 0;
			break;
		}
			
		case FS_CMD_DUP:
		{
			struct PipeFilp *filp;
			struct Pipe *pipe;
			
			filp = fsreq->filp;
			pipe = filp->pipe;
		
			MutexLock (&pipe->mutex);
				
			filp->reference_cnt++;
			
			if (filp->is_writer == TRUE)
				pipe->writer_cnt++;
			else
				pipe->reader_cnt++;
				
			fsreq->filp2 = filp;
			
			MutexUnlock (&pipe->mutex);

			fsreq->error = 0;
			fsreq->rc = 0;
			break;
		}
			
		case FS_CMD_READ:
		{
			struct PipeFilp *filp;
			struct Pipe *pipe;
			uint8 *dst;
			size_t temp_count;
			size_t count;
			
			KPRINTF ("PIPE READ");
			
			filp = fsreq->filp;
			pipe = filp->pipe;
			dst = fsreq->buf;
			count = fsreq->count;
			
			MutexLock (&pipe->mutex);
		
		
			while (pipe->read_sz == 0 && pipe->writer_cnt > 0)
				CondWait (&pipe->reader_cond, &pipe->mutex);
		
			if (pipe->read_sz > 0)
			{
				if (count > pipe->read_sz)
					count = pipe->read_sz;
				
				if ((pipe->r_pos + count) <= PIPE_BUFFER_SZ)
				{
					KPRINTF ("CopyOut() pipe");
					if (CopyOut (fsreq->as, dst, pipe->buf + pipe->r_pos, count) == 0)
					{
						pipe->r_pos = (pipe->r_pos + count) % PIPE_BUFFER_SZ;
						pipe->read_sz -= count;
						pipe->write_sz += count;
						
						fsreq->error = 0;
						fsreq->nbytes_transferred = count;
						fsreq->rc = 0;
					}
					else
					{
						fsreq->error = EIO;
						fsreq->nbytes_transferred = -1;
						fsreq->rc = -1;
					}
				}
				else
				{
					temp_count = PIPE_BUFFER_SZ - pipe->r_pos;
					
					KPRINTF ("CopyOut() pipe");
					if (CopyOut (fsreq->as, dst, pipe->buf + pipe->r_pos, temp_count) == 0)
					{
						dst = fsreq->buf + temp_count;
						temp_count = count - temp_count;

						KPRINTF ("CopyOut() pipe");
						if (CopyOut (fsreq->as, dst, pipe->buf, temp_count) == 0)
						{
							pipe->r_pos = (pipe->r_pos + count) % PIPE_BUFFER_SZ;
							pipe->read_sz -= count;
							pipe->write_sz += count;
							
							fsreq->error = 0;
							fsreq->nbytes_transferred = count;
							fsreq->rc = 0;
						}
						else
						{
							fsreq->error = EIO;
							fsreq->nbytes_transferred = -1;
							fsreq->rc = -1;
						}
					}
					else
					{
						fsreq->error = EIO;
						fsreq->nbytes_transferred = -1;
						fsreq->rc = -1;
					}
				}
						
				if (pipe->read_sz > 0)
					CondSignal (&pipe->reader_cond);
				
				if (pipe->write_sz >= PIPE_BUF)
					CondSignal (&pipe->writer_cond);
			}
			else
			{
				fsreq->error = 0;
				fsreq->nbytes_transferred = 0;
				fsreq->rc = 0;
			}
			
			
			
			MutexUnlock (&pipe->mutex);

			KPRINTF ("pipe - read - done");
			
			break;
		}			
		
		
		case FS_CMD_WRITE:
		{
			struct PipeFilp *filp;
			struct Pipe *pipe;
			int remaining;
			int nbytes_to_copy;
			int exit = 0;
			uint8 *src;
			uint8 *temp_src;
			size_t temp_count;
			size_t count;
			
			KPRINTF ("PIPE WRITE");
			
			count = fsreq->count;			
			filp = fsreq->filp;
			pipe = filp->pipe;
			remaining = count;
			src = fsreq->buf;

					
			while (remaining > 0 && exit == 0)
			{
				MutexLock (&pipe->mutex);

				while (pipe->write_sz < PIPE_BUF && pipe->reader_cnt > 0)
					CondWait (&pipe->writer_cond, &pipe->mutex);
				
				if (pipe->reader_cnt > 0)
				{
					if (remaining < pipe->write_sz)
						nbytes_to_copy = remaining;
					else
						nbytes_to_copy = pipe->write_sz;
			
						
					if ((pipe->w_pos + nbytes_to_copy) <= PIPE_BUFFER_SZ)
					{
						if (CopyIn (fsreq->as, pipe->buf + pipe->w_pos, src, nbytes_to_copy) == 0)
						{
							pipe->w_pos = (pipe->w_pos + nbytes_to_copy) % PIPE_BUFFER_SZ;
							pipe->read_sz += nbytes_to_copy;
							pipe->write_sz -= nbytes_to_copy;
							src += nbytes_to_copy;
							remaining -= nbytes_to_copy;
						}
						else
						{
							SetError (EIO);
							exit = -1;	
						}
					}
					else
					{
						temp_count = PIPE_BUFFER_SZ - pipe->w_pos;
						
						if (CopyIn (fsreq->as, pipe->buf + pipe->w_pos, src, temp_count) == 0)
						{
							temp_src = src + temp_count;
							temp_count = nbytes_to_copy - temp_count;
						
							if (CopyIn (fsreq->as, pipe->buf, temp_src, temp_count) == 0)
							{
								pipe->w_pos = (pipe->w_pos + nbytes_to_copy) % PIPE_BUFFER_SZ;	
								pipe->read_sz += nbytes_to_copy;
								pipe->write_sz -= nbytes_to_copy;
								src += nbytes_to_copy;
								remaining -= nbytes_to_copy;
							}
							else
							{
								SetError (EIO);
								exit = -1;
							}
						}
						else
						{
							SetError (EIO);
							exit = -1;
						}
					}
					
					KPRINTF ("pipe - condsignal reader");
					
					CondSignal (&pipe->reader_cond);
					

					if (pipe->write_sz >= PIPE_BUF)
					{
						KPRINTF ("pipe - condsignal writer");
						CondSignal (&pipe->writer_cond);
					}
				}
				else
				{
					exit = -1;
					
				/* FIXME:   UKill (current_process->pid, SIGPIPE); */
					SetError (EPIPE);
				}
				
				MutexUnlock (&pipe->mutex);
			}
			

			fsreq->error = GetError();
			fsreq->rc = exit;
			fsreq->nbytes_transferred = count - remaining;
			
			
			KPRINTF ("pipe - write - done");
			
			break;
		}
			
			


		case FS_CMD_LSEEK:
		case FS_CMD_UNLINK:
		case FS_CMD_RENAME:
		case FS_CMD_FTRUNCATE:
			fsreq->error = ENOTBLK;
			fsreq->rc = -1;
			break;

		
		case FS_CMD_FSTAT:
		{
			struct Stat *stat;
			struct PipeFilp *pfilp;
			
			pfilp = fsreq->filp;
			stat = fsreq->stat;
				
			if (pfilp->is_writer == TRUE)
				stat->st_mode = S_IFIFO  | (S_IWUSR | S_IWGRP | S_IWOTH);
			else
				stat->st_mode = S_IFIFO  | (S_IRUSR | S_IRGRP | S_IROTH);
				
			stat->st_nlink = 1;
			stat->st_uid = 0;
			stat->st_gid = 0;
			stat->st_rdev = 5;	/* FIXME: Maybe use address of Mount structure? or some other variable */
			stat->st_size = 0;
			stat->st_atime = 0;
			stat->st_mtime = 0;	/* pipe creation times */
			stat->st_ctime = 0;
			stat->st_blocks = 0;
			stat->st_ino = 0;
			
			fsreq->error = 0;
			fsreq->rc = 0;
			break;
		}	
			
		case FS_CMD_STAT:
		{
			struct Stat *stat;
			
			stat = fsreq->stat;
			stat->st_mode = S_IFIFO | (S_IRWXU | S_IRWXG | S_IRWXO);
			stat->st_nlink = 1;
			stat->st_uid = 0;
			stat->st_gid = 0;
			stat->st_rdev = 5;	/* Maybe use address of Mount structure? or some other variable */
			stat->st_size = 0;
			stat->st_atime = 0;
			stat->st_mtime = 0;  /* Pipe handler creation times */
			stat->st_ctime = 0;
			stat->st_blocks = 0;
			stat->st_ino = 0;
						
			fsreq->error = 0;
			fsreq->rc = 0;
			break;
		}

		case FS_CMD_FSTATFS:
			fsreq->error = ENOTBLK;
			fsreq->rc = -1;
			break;


		case FS_CMD_FSYNC:
		case FS_CMD_SYNC:
			fsreq->error = 0;
			fsreq->rc = 0;
			break;
			

		case FS_CMD_MKDIR:
		case FS_CMD_RMDIR:
		case FS_CMD_OPENDIR:
		case FS_CMD_CLOSEDIR:
		case FS_CMD_READDIR:
		case FS_CMD_REWINDDIR:
			fsreq->error = ENOTBLK;
			fsreq->rc = -1;
			break;

			
		case FS_CMD_ISATTY:
			fsreq->error = 0;
			fsreq->rc = 0;
			break;
						

		case FS_CMD_TCGETATTR:
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
			KPANIC ("PIPE Unknown command");
		}
	}
	
	
}





/*
 *
 */
 
int pipe_abortio (void *ioreq)
{
	/* lock mutex */
	
	/* WONT WORK, Pipe uses QUICK-IO so will be blocked
		in DoIO().  SIG_USER will be sent to the process
		doing the pipe-read/pipe-write.
		
		IS ABORT EVEN NECESSARY? PROBABLY.
		
		ALSO, UNCONNECTED is the SIGPIPE needs to be
		set to writers when there are no-readers 
		
		*/
		
		
			
	/* unlock mutex */

	return 0;
}
