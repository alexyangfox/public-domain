#include <kernel/types.h>
#include <kernel/i386/i386.h>
#include <kernel/device.h>
#include <kernel/sync.h>
#include <kernel/proc.h>
#include <kernel/msg.h>
#include <kernel/vm.h>
#include <kernel/timer.h>
#include <kernel/error.h>
#include <kernel/kmalloc.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/utility.h>
#include <kernel/buffers.h>
#include "iso9660.h"




/*
 *
 */

int32 CDTask (void *arg)
{
	struct FSReq *fsreq;
	struct Msg *msg;
	uint32 signals;
	struct CDSB *cdsb = arg;
	

	CDTaskInit (cdsb);

	while (1)
	{
		signals = KWait ((1 << cdsb->msgport->signal) | (1 << cdsb->diskchange_signal) | SIGF_TERM);
		
		SetError (0);
		
		if (signals & (1 << cdsb->diskchange_signal))
		{
			KPRINTF ("CD Diskchange Signal");

			CDInvalidate (cdsb);
			CDRevalidate (cdsb, 0);
		}
		
		if (signals & (1 << cdsb->msgport->signal))
		{		
			while ((msg = GetMsg (cdsb->msgport)) != NULL)
			{
				fsreq = (struct FSReq *)msg;
				
								
				switch (fsreq->cmd)
				{
					case FS_CMD_INHIBIT:
						CDDoInhibit (fsreq);
						break;
						
					case FS_CMD_UNINHIBIT:
						CDDoUninhibit (fsreq);
						break;
					
					
					/*
					case FS_CMD_RELABEL:
						fsreq->error = EROFS;
						fsreq->rc = -1;
						ReplyMsg (&fsreq->msg);
						break;
					*/
					
					
					case FS_CMD_OPEN:
						CDDoOpen (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
						
					case FS_CMD_CLOSE:
					case FS_CMD_CLOSEDIR:
						CDDoClose (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_DUP:
						CDDoDup (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_READ:
						CDDoRead (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_WRITE:
						fsreq->error = EROFS;
						fsreq->rc = -1;
						ReplyMsg (&fsreq->msg);
						break;
						
					case FS_CMD_LSEEK:
						CDDoSeek (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_OPENDIR:
						CDDoOpendir (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_READDIR:
						CDDoReaddir (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_REWINDDIR:
						CDDoRewinddir (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_FSTAT:
						CDDoFstat (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_STAT:
						CDDoStat (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_TRUNCATE:
						fsreq->error = EROFS;
						fsreq->rc = -1;
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_MKDIR:
						fsreq->error = EROFS;
						fsreq->rc = -1;
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_UNLINK:
						fsreq->error = EROFS;
						fsreq->rc = -1;
						ReplyMsg (&fsreq->msg);
						break;
						
					case FS_CMD_RMDIR:
						fsreq->error = EROFS;
						fsreq->rc = -1;
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_ISATTY:
						fsreq->error = 0;
						fsreq->rc = 0;
						ReplyMsg (&fsreq->msg);
						break;
				
					case FS_CMD_FORMAT:
						fsreq->error = EROFS;
						fsreq->rc = -1;
						ReplyMsg (&fsreq->msg);
						break;
				
					case FS_CMD_RENAME:
						fsreq->error = EROFS;
						fsreq->rc = -1;
						ReplyMsg (&fsreq->msg);
						break;
				
					case FS_CMD_CHMOD:
						fsreq->error = EROFS;
						fsreq->rc = -1;
						ReplyMsg (&fsreq->msg);
						break;
				
					case FS_CMD_CHOWN:
						fsreq->error = EROFS;
						fsreq->rc = -1;
						ReplyMsg (&fsreq->msg);
						break;
					
					default:
					{
						fsreq->error = ENOSYS;
						fsreq->rc = -1;
						KPANIC ("CD ENOSYS");
					}
				}
			}
		}			

		if (signals & SIGF_TERM)
		{
			CDTaskFini(cdsb);
		}
	}
}




/*
 *
 */

void CDTaskInit (struct CDSB *cdsb)
{
	KPRINTF ("CDTaskInit()");

	cdsb->pid = GetPID();
	
	if ((cdsb->diskchange_signal = AllocSignal()) != -1)
	{
		if ((cdsb->msgport = CreateMsgPort()) != NULL)
		{
			CDAddCallback (cdsb);
			
			CDRevalidate (cdsb, 0);
			
			cdsb->init_error = 0;
			KSignal (GetPPID(), SIG_INIT);
			return;
		}

		FreeSignal (cdsb->diskchange_signal);
	}
	
	cdsb->init_error = -1;
	KSignal (GetPPID(), SIG_INIT);
	Exit(-1);
}




/*
 *
 */

void CDTaskFini (struct CDSB *cdsb)
{
	KPRINTF ("CDTaskFini()");

	CDInvalidate(cdsb);
	CDRemCallback(cdsb);
	DeleteMsgPort(cdsb->msgport);
	FreeSignal(cdsb->diskchange_signal);

	Exit(0);
}




/*
 *
 */

void CDAddCallback (struct CDSB *cdsb)
{
	struct BlkReq blkreq;
	
	cdsb->callback.arg = cdsb;
	cdsb->callback.callback = &CDDiskChangeCallback;

	blkreq.as = &kernel_as;
	blkreq.device = cdsb->device;
	blkreq.unitp = cdsb->unitp;
	blkreq.callback = &cdsb->callback;
	blkreq.cmd = BLK_CMD_ADD_CALLBACK;

	DoIO (&blkreq, NULL);
}




/*
 *
 */

void CDRemCallback (struct CDSB *cdsb)
{
	struct BlkReq blkreq;
	
	blkreq.as = &kernel_as;
	blkreq.device = cdsb->device;
	blkreq.unitp = cdsb->unitp;
	blkreq.callback = &cdsb->callback;
	blkreq.cmd = BLK_CMD_REM_CALLBACK;
	DoIO (&blkreq, NULL);
}




/*
 * CDDiskChangeCallback();
 */

void CDDiskChangeCallback (void *arg)
{
	struct CDSB *cdsb = arg;
	
	KPRINTF ("CDDiskChangeCallback()");
	
	KSignal (cdsb->pid, cdsb->diskchange_signal);
}




/*
 * CDDoInhibit();
 */

void CDDoInhibit (struct FSReq *fsreq)
{
	struct CDSB *cdsb;

	cdsb = fsreq->unitp;
	CDInvalidate (cdsb);
}




/*
 * CDDoUninhibit();
 */

void CDDoUninhibit (struct FSReq *fsreq)
{
	struct CDSB *cdsb;

	cdsb = fsreq->unitp;
	CDRevalidate (cdsb, 0);
}




/*
 * CDDoOpen();
 */

void CDDoOpen (struct FSReq *fsreq)
{
	struct CDSB *cdsb;
	struct CDNode *node;
	struct CDFilp *filp;
	struct CDLookup lookup;


	cdsb = fsreq->unitp;
	
	if (CDIsValid (cdsb, fsreq) == -1)
		return;
		
	if ((filp = KMalloc (sizeof (struct CDFilp))) != NULL)
	{
		lookup.cdsb = cdsb;
		lookup.pathname = fsreq->path;
		
		if (CDLookup (&lookup) == 0)
		{
			node = lookup.node;
							
			if (node == NULL && (fsreq->oflags & O_CREAT))
			{
				CDFreeNode (cdsb, node);
				fsreq->filp = NULL;
				fsreq->error = EROFS;
				fsreq->rc = -1;
				KFree (filp);
				return;
			}
			
			
			if (node != NULL)
			{					
				if (node->flags & ISO_DIRECTORY)
				{
					CDFreeNode (cdsb, node);
					fsreq->filp = NULL;
					fsreq->error = EISDIR;
					fsreq->rc = -1;
					KFree (filp);
					return;
				}
				else
				{
					if (fsreq->oflags & O_TRUNC)
					{
						CDFreeNode (cdsb, node);
						fsreq->filp = NULL;
						fsreq->error = GetError();
						fsreq->rc = -1;
						KFree (filp);
						return;
					}
											
					filp->device = &cd_device;
					filp->node = node;
					filp->cdsb = cdsb;
					filp->offset = 0;
					filp->reference_cnt = 1;	
					filp->invalid = 0;
					
					LIST_ADD_TAIL (&node->filp_list, filp, node_filp_entry);
					LIST_ADD_TAIL (&cdsb->active_filp_list, filp, cdsb_filp_entry);
					
					cdsb->reference_cnt ++;
					
					fsreq->filp = filp;
					fsreq->error = 0;
					fsreq->rc = 0;
					return;
				}
			}
		}
		else
		{
			SetError (ENOENT);
		}
		
		KFree (filp);
	}
		
	fsreq->filp = NULL;
	fsreq->error = GetError();
	fsreq->rc = -1;

}




/*
 * CDDoClose();
 */

void CDDoClose (struct FSReq *fsreq)
{
	struct CDFilp *filp;
	struct CDNode *node;
	struct CDSB *cdsb;
	
		
	filp = fsreq->filp;
	node = filp->node;
	cdsb = filp->cdsb;
	
	filp->reference_cnt --;
	cdsb->reference_cnt --;
	
	if (filp->reference_cnt == 0)
	{
		LIST_REM_ENTRY (&node->filp_list, filp, node_filp_entry);
		
		if (filp->invalid == 0)
		{
			LIST_REM_ENTRY (&cdsb->active_filp_list, filp, cdsb_filp_entry);
		}
		else
		{
			LIST_REM_ENTRY (&cdsb->invalid_filp_list, filp, cdsb_filp_entry);
		}
		
		KFree (filp);
	}
	
	CDFreeNode (cdsb, node);
	
	fsreq->error = 0;
	fsreq->rc = 0;
}




/*
 * CDDoDup();
 */

void CDDoDup (struct FSReq *fsreq)
{
	struct CDFilp *filp;
	struct CDNode *node;
	struct CDSB *cdsb;

	
	filp = fsreq->filp;
	node = filp->node;
	cdsb = filp->cdsb;
	
	if (CDIsValid (cdsb, fsreq) == -1)
		return;
	
	filp->reference_cnt++;
	node->reference_cnt++;
	cdsb->reference_cnt++;
		
	fsreq->filp2 = filp;
	fsreq->error = 0;
	fsreq->rc = 0;
}




/*
 * CDDoSeek();
 */

void CDDoSeek (struct FSReq *fsreq)
{
	struct CDFilp *filp;
	struct CDSB *cdsb;
	struct CDNode *node;


	filp = fsreq->filp;
	cdsb = filp->cdsb;
	node = filp->node;
		
	if (CDIsValid (cdsb, fsreq) == -1)
		return;

	if (fsreq->whence == SEEK_SET)
	{
		filp->offset = fsreq->offset;
		fsreq->position = filp->offset;
		fsreq->error = 0;
		fsreq->rc = 0;
	}
	else if (fsreq->whence == SEEK_CUR)
	{
		filp->offset += fsreq->offset;
		fsreq->position = filp->offset;
		fsreq->error = 0;
		fsreq->rc = 0;
	}
	else if (fsreq->whence == SEEK_END)
	{
		filp->offset = node->size + fsreq->offset;
		fsreq->position = filp->offset;
		fsreq->error = 0;
		fsreq->rc = 0;
	}
	else
	{
		fsreq->position = 0;
		fsreq->error = EINVAL;
		fsreq->rc = -1;
	}
}




/*
 * CDDoRead();
 *
 * Ensure we are dealing with a file
 */

void CDDoRead (struct FSReq *fsreq)
{
	struct CDFilp *filp;
	struct CDSB *cdsb;
	struct CDNode *node;
	int32 nbytes_read;


	filp = fsreq->filp;
	cdsb = filp->cdsb;
	node = filp->node;

	if (CDIsValid (cdsb, fsreq) == -1)
		return;
	
	if ((node->flags & ISO_DIRECTORY) == 0)
	{
		nbytes_read = CDFileRead (node, fsreq->buf, fsreq->count, filp->offset, fsreq);
		
		if (nbytes_read != -1)
			filp->offset += nbytes_read;
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
 * CDDoOpendir();
 *
 * Somehow merge with Open().
 */

void CDDoOpendir (struct FSReq *fsreq)
{
	struct CDSB *cdsb;
	struct CDNode *node;
	struct CDFilp *filp;
	struct CDLookup lookup;
	

	cdsb = fsreq->unitp;

	if (CDIsValid (cdsb, fsreq) == -1)
		return;
		
	if ((filp = KMalloc (sizeof (struct CDFilp))) != NULL)
	{
		lookup.cdsb = cdsb;
		lookup.pathname = fsreq->path;
		
		if (CDLookup (&lookup) == 0)
		{
			node = lookup.node;
			
			KASSERT (node != NULL);
			
			if ((node->flags & ISO_DIRECTORY) != 0)
			{
				filp->device = &cd_device;
				filp->node = node;
				filp->cdsb = cdsb;
				filp->offset = 0;
				filp->reference_cnt = 1;	
				filp->invalid = 0;
				
				LIST_ADD_TAIL (&node->filp_list, filp, node_filp_entry);
				LIST_ADD_TAIL (&cdsb->active_filp_list, filp, cdsb_filp_entry);
				
				cdsb->reference_cnt ++;
				
				fsreq->filp = filp;
				fsreq->error = 0;
				fsreq->rc = 0;
				return;
			}
			else
			{
				CDFreeNode (cdsb, node);
				SetError(ENOTDIR);
			}
		}
		else
		{
			KPRINTF ("No such directory!!");
			SetError (ENOENT);
		}
		
		KFree (filp);
	}
			
	fsreq->filp = NULL;
	fsreq->error = GetError();
	fsreq->rc = -1;
}




/*
 * Read a single dirent at a time
 *
 * Ensure we are dealing with a directory
 */

void CDDoReaddir (struct FSReq *fsreq)
{
	struct CDFilp *filp;
	struct CDSB *cdsb;
	struct CDNode *node;
	struct Dirent *dirent;
	struct Dirent temp_dirent;
	int rc;
	off_t offset, new_offset;
	
	

	filp = fsreq->filp;
	dirent = fsreq->dirent;
	cdsb = filp->cdsb;
	node = filp->node;
	offset = filp->offset;
	
	if (CDIsValid (cdsb, fsreq) == -1)
		return;
	
	if (node->flags & ISO_DIRECTORY)
	{
		while (1)
		{
			/* FIXME,  dirents are variable length */
			
			rc = CDDirRead (node, &temp_dirent, NULL, offset, &new_offset);
			
			
						
			if (rc == 1)
			{
				filp->offset = new_offset;

				CopyOut (fsreq->as, dirent, &temp_dirent, sizeof (struct Dirent));
				
				
				fsreq->error = 0;
				fsreq->rc = 0;
				break;
			}
			else if (rc == 0)
			{	
				
				fsreq->dirent = NULL;
				fsreq->error = 0;
				fsreq->rc = -1;	
				break;	
			}
			else
			{
				fsreq->dirent = NULL;
				fsreq->error = EIO;
				fsreq->rc = -1;
			}
		}
	}
	else
	{
		fsreq->dirent = NULL;
		fsreq->error = ENOTDIR;
		fsreq->rc = -1;
	}

}




/*
 * Ensure it is a directory
 */

void CDDoRewinddir (struct FSReq *fsreq)
{
	struct CDFilp *filp;
	struct CDSB *cdsb;
	struct CDNode *node;

	

	filp = fsreq->filp;
	cdsb = filp->cdsb;
	node = filp->node;
	
	if (CDIsValid (cdsb, fsreq) == -1)
		return;
		
	if (node->flags & ISO_DIRECTORY)
	{
		filp->offset = 0;
		fsreq->error = 0;
		fsreq->rc = 0;
	}
	else
	{
		fsreq->error = ENOTDIR;
		fsreq->rc = -1;
	}
}




/*
 *
 */

void CDDoFstat (struct FSReq *fsreq)
{
	struct CDFilp *filp;
	struct CDSB *cdsb;
	struct CDNode *node;
	struct Stat *stat;
	
	
	filp = fsreq->filp;
	cdsb = filp->cdsb;
	node = filp->node;
	stat = fsreq->stat;

	if (CDIsValid (cdsb, fsreq) == -1)
		return;
	
	stat->st_dev = (uint32)cdsb; /* FIX: Was fsb, should be mount or dev */
	stat->st_ino = node->extent_start; /* Always zero, makes everything appear as same file? CAT BREAKS */

	if (node->flags & ISO_DIRECTORY)
		stat->st_mode = S_IFDIR  | (S_IRWXU | S_IRWXG | S_IRWXO);
	else
		stat->st_mode = S_IFREG  | (S_IRWXU | S_IRWXG | S_IRWXO);

	/* FIXME: */		
	stat->st_nlink = 1;
	stat->st_uid = 1;
	stat->st_gid = 1;
	stat->st_rdev = 2;
	stat->st_size = node->size;

	stat->st_atime = 0;
	stat->st_mtime = 0;
	stat->st_ctime = 0;
	stat->st_blocks = 0;

	
	
	fsreq->error = 0;
	fsreq->rc = 0;
}




/*
 * CDDoStat();
 */

void CDDoStat (struct FSReq *fsreq)
{
	struct CDSB *cdsb;
	struct CDNode *node;
	struct Stat *stat;
	struct CDLookup lookup;
	
	

	cdsb = fsreq->unitp;
	stat = fsreq->stat;	
	

	if (CDIsValid (cdsb, fsreq) == -1)
		return;
		
	lookup.cdsb = cdsb;
	lookup.pathname = fsreq->path;
	
	if (CDLookup (&lookup) == 0)
	{
		node = lookup.node;
		
		stat->st_dev = (uint32)cdsb;  /* FIX: See above */
		stat->st_ino = node->extent_start;  /* Always zero, makes everything appear as same file? */

			
		if (node->flags & ISO_DIRECTORY)
			stat->st_mode = S_IFDIR  | (S_IRWXU | S_IRWXG | S_IRWXO);
		else
			stat->st_mode = S_IFREG  | (S_IRWXU | S_IRWXG | S_IRWXO);
	
		/* FIXME: */				
		stat->st_nlink = 1;
		stat->st_uid = 1;
		stat->st_gid = 1;
		stat->st_rdev = 2;
		stat->st_size = node->size;

		stat->st_atime = 0;
		stat->st_mtime = 0;
		stat->st_ctime = 0;
		stat->st_blocks = 0;
		
		
		CDFreeNode (cdsb, node);
		fsreq->error = 0;
		fsreq->rc = 0;
		return;
	}
	else
	{
		fsreq->error = ENOENT;
	}
	
	fsreq->rc = -1;
}



