#include <kernel/types.h>
#include <kernel/i386/i386.h>
#include <kernel/device.h>
#include <kernel/sync.h>
#include <kernel/proc.h>
#include <kernel/msg.h>
#include <kernel/vm.h>
#include <kernel/timer.h>
#include <kernel/dbg.h>
#include <kernel/error.h>
#include <kernel/kmalloc.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/utility.h>
#include <kernel/buffers.h>
#include <kernel/block.h>
#include "fat.h"




/*
 *
 */

int32 FatTask (void *arg)
{
	struct FSReq *fsreq;
	struct Msg *msg;
	uint32 signals;
	struct FatSB *fsb = arg;
	struct Alarm flush_alarm;

	FatTaskInit (fsb);
	
	if (fsb->buf->writeback_delay > 0)
		KAlarmSet (&flush_alarm, fsb->buf->writeback_delay, 0, fsb->flush_signal);

	while (1)
	{
		signals = KWait ((1 << fsb->msgport->signal) | (1 << fsb->flush_signal)
							| (1 << fsb->diskchange_signal) | SIGF_TERM);
		
		SetError (0);
		
		if (signals & (1 << fsb->diskchange_signal))
		{
			KPRINTF ("Diskchange Signal");

			FatInvalidate (fsb);
			FatRevalidate (fsb, 0);
		}
		
		if (signals & (1 << fsb->flush_signal))
		{
			SyncBuf (fsb->buf);
			
			if (fsb->buf->writeback_delay > 0)
				KAlarmSet (&flush_alarm, fsb->buf->writeback_delay, 0, fsb->flush_signal);
		}
		
		if (signals & (1 << fsb->msgport->signal))
		{
			while ((msg = GetMsg (fsb->msgport)) != NULL)
			{
				fsreq = (struct FSReq *)msg;
				
								
				switch (fsreq->cmd)
				{
					case FS_CMD_INHIBIT:
						FatDoInhibit (fsreq);
						break;
						
					case FS_CMD_UNINHIBIT:
						FatDoUninhibit (fsreq);
						break;
					
					/*
					case FS_CMD_RELABEL:
						FatDoRelabel (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					*/
					
					case FS_CMD_OPEN:
						FatDoOpen (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
						
					case FS_CMD_CLOSE:
					case FS_CMD_CLOSEDIR:
						FatDoClose (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_DUP:
						FatDoDup (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_READ:
						FatDoRead (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_WRITE:
						FatDoWrite (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
						
					case FS_CMD_LSEEK:
						FatDoSeek (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_OPENDIR:
						FatDoOpendir (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_READDIR:
						FatDoReaddir (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_REWINDDIR:
						FatDoRewinddir (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_FSTAT:
						FatDoFstat (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_STAT:
						FatDoStat (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_TRUNCATE:
						FatDoTruncate (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_MKDIR:
						FatDoMkdir (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_UNLINK:
						FatDoUnlink (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
						
					case FS_CMD_RMDIR:
						FatDoRmdir (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					case FS_CMD_ISATTY:
						fsreq->error = 0;
						fsreq->rc = 0;
						ReplyMsg (&fsreq->msg);
						break;
				
					case FS_CMD_FORMAT:
						FatDoFormat (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
				
					case FS_CMD_RENAME:
						FatDoRename (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
				
					case FS_CMD_CHMOD:
						FatDoChmod (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
				
					case FS_CMD_CHOWN:
						FatDoChown (fsreq);
						ReplyMsg (&fsreq->msg);
						break;
					
					default:
					{
						fsreq->error = ENOSYS;
						fsreq->rc = -1;
						KPANIC ("FAT ENOSYS");
					}
				}
			}
		}			

		if (signals & SIGF_TERM)
		{
			FatTaskFini(fsb);
		}
	}
}




/*
 *
 */

void FatTaskInit (struct FatSB *fsb)
{
	fsb->pid = GetPID();
	
	if ((fsb->diskchange_signal = AllocSignal()) != -1)
	{
		if ((fsb->flush_signal = AllocSignal()) != -1)
		{
			if ((fsb->msgport = CreateMsgPort()) != NULL)
			{
				FatAddCallback (fsb);
									
				FatRevalidate (fsb, 0);
				fsb->init_error = 0;
				KSignal (GetPPID(), SIG_INIT);
				return;
			}

			FreeSignal (fsb->flush_signal);
		}
		FreeSignal (fsb->diskchange_signal);
	}
	
	fsb->init_error = -1;
	KSignal (GetPPID(), SIG_INIT);
	Exit(-1);
}




/*
 *
 */

void FatTaskFini (struct FatSB *fsb)
{
	KPRINTF ("FatTaskFini()");
	
	
	SyncBuf (fsb->buf);
	FatInvalidate (fsb);
	FatRemCallback (fsb);
	DeleteMsgPort (fsb->msgport);
	FreeSignal (fsb->diskchange_signal);

	KPRINTF ("FatTaskFini()+");
	Exit(0);
}




/*
 *
 */

void FatAddCallback (struct FatSB *fsb)
{
	struct BlkReq blkreq;
	
	fsb->callback.arg = fsb;
	fsb->callback.callback = &FatDiskChangeCallback;

	blkreq.as = &kernel_as;
	blkreq.device = fsb->device;
	blkreq.unitp = fsb->unitp;
	blkreq.callback = &fsb->callback;
	blkreq.cmd = BLK_CMD_ADD_CALLBACK;

	DoIO (&blkreq, NULL);
}




/*
 *
 */

void FatRemCallback (struct FatSB *fsb)
{
	struct BlkReq blkreq;
	
	blkreq.as = &kernel_as;
	blkreq.device = fsb->device;
	blkreq.unitp = fsb->unitp;
	blkreq.callback = &fsb->callback;
	blkreq.cmd = BLK_CMD_REM_CALLBACK;
	DoIO (&blkreq, NULL);
}




/*
 * FatDiskChangeCallback();
 */

void FatDiskChangeCallback (void *arg)
{
	struct FatSB *fsb = arg;
	
	KSignal (fsb->pid, fsb->diskchange_signal);
}




/*
 * FatDoInhibit();
 */

void FatDoInhibit (struct FSReq *fsreq)
{
	struct FatSB *fsb;

	fsb = fsreq->unitp;
	FatInvalidate (fsb);

}




/*
 * FatDoUninhibit();
 */

void FatDoUninhibit (struct FSReq *fsreq)
{
	struct FatSB *fsb;

	fsb = fsreq->unitp;
	FatRevalidate (fsb, 0);
}




/*
 * FatDoOpen();
 */

void FatDoOpen (struct FSReq *fsreq)
{
	struct FatSB *fsb;
	struct FatNode *node;
	struct FatFilp *filp;
	struct FatLookup lookup;
	

	fsb = fsreq->unitp;
	

	if (FatIsValid (fsb, fsreq) == -1)
		return;
	
	
	if ((filp = KMalloc (sizeof (struct FatFilp))) != NULL)
	{
		lookup.fsb = fsb;
		lookup.pathname = fsreq->path;
		lookup.cmd = CMD_CREATE;
		
		if (FatLookup (&lookup) == 0)
		{
			node = lookup.node;
			
			if (node != NULL && ((fsreq->oflags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL)))
			{
				FreeNode (fsb, lookup.parent);
				FreeNode (fsb, node);
				fsreq->filp = NULL;
				fsreq->error = EEXIST;
				fsreq->rc = -1;
				KFree (filp);
				return;
			}
			
			if (node == NULL && (fsreq->oflags & O_CREAT))
			{
				node = FatCreateFile (fsb, lookup.parent, lookup.last_component);
			}
			
			FreeNode (fsb, lookup.parent);				
			
			if (node != NULL)
			{					
				if (node->dirent.attributes & ATTR_DIRECTORY)
				{
					FreeNode (fsb, node);
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
						if (FatTruncateFile (node, 0) != 0)
						{
							FreeNode (fsb, node);
							fsreq->filp = NULL;
							fsreq->error = GetError();
							fsreq->rc = -1;
							KFree (filp);
							return;
						}
					}
					
					
					if (fsreq->oflags & O_APPEND)
						filp->append_mode = TRUE;
					else
						filp->append_mode = FALSE;
					
					filp->device = &fat_device;
					filp->node = node;
					filp->fsb = fsb;
					filp->offset = 0;
					filp->reference_cnt = 1;	
					filp->invalid = 0;
					
					LIST_ADD_TAIL (&node->filp_list, filp, node_filp_entry);
					LIST_ADD_TAIL (&fsb->active_filp_list, filp, fsb_filp_entry);
					
					fsb->reference_cnt ++;
					
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
 * FatDoClose();
 */

void FatDoClose (struct FSReq *fsreq)
{
	struct FatFilp *filp;
	struct FatNode *node;
	struct FatSB *fsb;
	
	filp = fsreq->filp;
	node = filp->node;
	fsb = filp->fsb;
	
	filp->reference_cnt --;
	fsb->reference_cnt --;
	
	if (filp->reference_cnt == 0)
	{
		LIST_REM_ENTRY (&node->filp_list, filp, node_filp_entry);
		
		if (filp->invalid == 0)
		{
			LIST_REM_ENTRY (&fsb->active_filp_list, filp, fsb_filp_entry);
		}
		else
		{
			LIST_REM_ENTRY (&fsb->invalid_filp_list, filp, fsb_filp_entry);
		}
		
		KFree (filp);
	}
	
	FreeNode (fsb, node);
	
	fsreq->error = 0;
	fsreq->rc = 0;
}




/*
 * FatDoDup();
 */

void FatDoDup (struct FSReq *fsreq)
{
	struct FatFilp *filp;
	struct FatNode *node;
	struct FatSB *fsb;

	filp = fsreq->filp;
	node = filp->node;
	fsb = filp->fsb;
	
	if (FatIsValid (fsb, fsreq) == -1)
		return;
		
	filp->reference_cnt++;
	node->reference_cnt++;
	fsb->reference_cnt++;
		
	fsreq->filp2 = filp;
	fsreq->error = 0;
	fsreq->rc = 0;
}




/*
 * FatDoSeek();
 */

void FatDoSeek (struct FSReq *fsreq)
{
	struct FatFilp *filp;
	struct FatSB *fsb;
	struct FatNode *node;


	filp = fsreq->filp;
	fsb = filp->fsb;
	node = filp->node;
		
	if (FatIsValid (fsb, fsreq) == -1)
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
		filp->offset = node->dirent.size + fsreq->offset;
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
 * FatDoRead();
 *
 * Ensure we are dealing with a file
 */

void FatDoRead (struct FSReq *fsreq)
{
	struct FatFilp *filp;
	struct FatSB *fsb;
	struct FatNode *node;
	int32 nbytes_read;
	

	filp = fsreq->filp;
	fsb = filp->fsb;
	node = filp->node;

	if (FatIsValid (fsb, fsreq) == -1)
		return;
	
	if ((node->dirent.attributes & ATTR_DIRECTORY) == 0)
	{
		nbytes_read = FatFileRead (node, fsreq->buf, fsreq->count, filp->offset, fsreq);
		
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
 * FatDoWrite();
 * Ensure we are dealing with a file
 */

void FatDoWrite (struct FSReq *fsreq)
{
	struct FatFilp *filp;
	struct FatSB *fsb;
	struct FatNode *node;
	int32 nbytes_written;
	

	filp = fsreq->filp;
	fsb = filp->fsb;
	node = filp->node;


	if (FatIsValid (fsb, fsreq) == -1)
		return;
			
	if ((node->dirent.attributes & ATTR_DIRECTORY) == 0)
	{
		if (filp->append_mode == TRUE)
			nbytes_written = FatFileWrite (node, fsreq->buf, fsreq->count,
								node->dirent.size, fsreq);
		else
			nbytes_written = FatFileWrite (node, fsreq->buf, fsreq->count,
											filp->offset, fsreq);
		
		if (nbytes_written != -1)
			filp->offset += nbytes_written;
	}
	else
	{
		SetError (EISDIR);
		nbytes_written = -1;
	}
	
	fsreq->error = GetError();
	fsreq->nbytes_transferred = nbytes_written;
	fsreq->rc = 0;
}




/*
 * FatDoOpendir();
 */

void FatDoOpendir (struct FSReq *fsreq)
{
	struct FatSB *fsb;
	struct FatNode *node;
	struct FatFilp *filp;
	struct FatLookup lookup;
	
	KPRINTF ("FatOpendir()");
	
	fsb = fsreq->unitp;


	if (FatIsValid (fsb, fsreq) == -1)
		return;
		
	if ((filp = KMalloc (sizeof (struct FatFilp))) != NULL)
	{
		lookup.fsb = fsb;
		lookup.pathname = fsreq->path;
		lookup.cmd = CMD_LOOKUP;
		
		if (FatLookup (&lookup) == 0)
		{
			node = lookup.node;
			
			KASSERT (node != NULL);
			
			if ((node->dirent.attributes & ATTR_DIRECTORY) != 0)
			{
				filp->device = &fat_device;
				
				filp->node = node;
				filp->fsb = fsb;
				filp->offset = 0;
				filp->reference_cnt = 1;	
				filp->invalid = 0;
				
				LIST_ADD_TAIL (&node->filp_list, filp, node_filp_entry);
				LIST_ADD_TAIL (&fsb->active_filp_list, filp, fsb_filp_entry);
				
				fsb->reference_cnt ++;
				
				fsreq->filp = filp;
				fsreq->error = 0;
				fsreq->rc = 0;
				return;
			}
			else
			{
				KPRINTF ("FatOpendir() err1");
					
				FreeNode (fsb, node);
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

void FatDoReaddir (struct FSReq *fsreq)
{
	struct FatFilp *filp;
	struct FatSB *fsb;
	struct FatNode *node;
	struct FatDirEntry fdirent;
	struct Dirent *dirent;
	char asciiz_name[16];
	int len;
	int rc;
	char *c;


	filp = fsreq->filp;
	dirent = fsreq->dirent;
	fsb = filp->fsb;
	node = filp->node;

	if (FatIsValid (fsb, fsreq) == -1)
		return;
	
	if (node->dirent.attributes & ATTR_DIRECTORY)
	{
		while (1)
		{
			rc = FatDirRead (node, &fdirent, filp->offset, NULL, NULL);
					
			if (rc == 1)
			{
				filp->offset ++;
				
				if (fdirent.name[0] == DIRENTRY_FREE)
				{
					fsreq->dirent = NULL;
					fsreq->error = 0;
					fsreq->rc = -1;
					break;
				}
				else if (fdirent.name[0] != DIRENTRY_DELETED && !(fdirent.attributes & ATTR_VOLUME_ID))
				{
					len = FatDirEntryToASCIIZ (asciiz_name, &fdirent);
					
					c = asciiz_name;
		
					while (*c != '\0')
					{
						if (*c >= 'A' && *c <= 'Z')
							*c += ('a' - 'A');
						
						c++;
					}
					
					CopyOut (fsreq->as, &dirent->d_name, asciiz_name, len + 1);
					fsreq->error = 0;
					fsreq->rc = 0;
					break;
				}
			}
			else
			{	
				fsreq->dirent = NULL;
				fsreq->error = 0;
				fsreq->rc = -1;	
				break;	
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

void FatDoRewinddir (struct FSReq *fsreq)
{
	struct FatFilp *filp;
	struct FatSB *fsb;
	struct FatNode *node;


	filp = fsreq->filp;
	fsb = filp->fsb;
	node = filp->node;
	
	if (FatIsValid (fsb, fsreq) == -1)
		return;
		
	if (node->dirent.attributes & ATTR_DIRECTORY)
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

void FatDoFstat (struct FSReq *fsreq)
{
	struct FatFilp *filp;
	struct FatSB *fsb;
	struct FatNode *node;
	struct Stat *stat;
	struct TimeVal tv;
	
	
	filp = fsreq->filp;
	fsb = filp->fsb;
	node = filp->node;
	stat = fsreq->stat;


	if (FatIsValid (fsb, fsreq) == -1)
		return;
	
	stat->st_dev = (uint32)fsb; /* FIX: Was fsb, should be mount or dev */
	stat->st_ino = (uint32)node; /* Always zero, makes everything appear as same file? CAT BREAKS */
	
	if (node->dirent.attributes & ATTR_DIRECTORY)
		stat->st_mode = S_IFDIR  | (S_IRWXU | S_IRWXG | S_IRWXO);
	else
		stat->st_mode = S_IFREG  | (S_IRWXU | S_IRWXG | S_IRWXO);
		
	stat->st_nlink = 1;
	stat->st_uid = 1;
	stat->st_gid = 1;
	stat->st_rdev = 2;
	stat->st_size = node->dirent.size;

	FatToEpochTime (&tv, node->dirent.last_access_date, 0,0);
	stat->st_atime = tv.seconds;
	FatToEpochTime (&tv, node->dirent.last_write_date, node->dirent.last_write_time,0);
	stat->st_mtime = tv.seconds;
	FatToEpochTime (&tv, node->dirent.creation_date, node->dirent.creation_time_2secs, node->dirent.creation_time_sec_tenths);
	stat->st_ctime = tv.seconds;	

	stat->st_blocks = 0;
		
	fsreq->error = 0;
	fsreq->rc = 0;
}




/*
 * FatDoStat();
 */

void FatDoStat (struct FSReq *fsreq)
{
	struct FatSB *fsb;
	struct FatNode *node;
	struct Stat *stat;
	struct FatLookup lookup;
	struct TimeVal tv;
	

	fsb = fsreq->unitp;
	stat = fsreq->stat;	
	

	if (FatIsValid (fsb, fsreq) == -1)
		return;
		
	lookup.fsb = fsb;
	lookup.pathname = fsreq->path;
	lookup.cmd = CMD_LOOKUP;
	
	
	if (FatLookup (&lookup) == 0)
	{
		node = lookup.node;
		
		stat->st_dev = (uint32)fsb;  /* FIX: See above */
		stat->st_ino = (uint32)node;  /* Always zero, makes everything appear as same file? */
		
		if (node->dirent.attributes & ATTR_DIRECTORY)
			stat->st_mode = S_IFDIR  | (S_IRWXU | S_IRWXG | S_IRWXO);
		else
			stat->st_mode = S_IFREG  | (S_IRWXU | S_IRWXG | S_IRWXO);
		
		stat->st_nlink = 1;
		stat->st_uid = 1;
		stat->st_gid = 1;
		stat->st_rdev = 2;
		stat->st_size = node->dirent.size;
		
		
		
		FatToEpochTime (&tv, node->dirent.last_access_date, 0,0);
		stat->st_atime = tv.seconds;
		FatToEpochTime (&tv, node->dirent.last_write_date, node->dirent.last_write_time,0);
		stat->st_mtime = tv.seconds;
		FatToEpochTime (&tv, node->dirent.creation_date, node->dirent.creation_time_2secs, node->dirent.creation_time_sec_tenths);
		stat->st_ctime = tv.seconds;	
		
		
		stat->st_blocks = 0;
		
		
		FreeNode (fsb, node);
		fsreq->error = 0;
		fsreq->rc = 0;
		return;
	}
		
	
	fsreq->rc = -1;
}




/*
 * FatDoTruncate();
 *
 * How does seek position change on truncation?
 */
 

void FatDoTruncate (struct FSReq *fsreq)
{
	struct FatFilp *filp;
	struct FatNode *node;
	struct FatSB *fsb;
	int32 size;
	
	
	filp = fsreq->filp;
	node = filp->node;
	size = fsreq->size;
	fsb = filp->fsb;


	if (FatIsValid (fsb, fsreq) == -1)
		return;
	
	if (size == node->dirent.size)
	{
		fsreq->rc = 0;
		fsreq->error = 0;
	}
	else if (size < 0)
	{
		fsreq->rc = -1;
		fsreq->error = EINVAL;
	}
	else if (size < node->dirent.size)
		fsreq->rc = FatTruncateFile (node, size);
	else
		fsreq->rc = FatExtendFile (node, size);
	
	if (fsreq->rc == 0)
		fsreq->error = 0;
	else
		fsreq->error = GetError();
		
}




/*
 *
 */

void FatDoMkdir (struct FSReq *fsreq)
{
	struct FatSB *fsb;
	struct FatLookup lookup;
	struct FatNode *node;
	
	
	fsb = fsreq->unitp;
	

	if (FatIsValid (fsb, fsreq) == -1)
		return;
		
	lookup.fsb = fsb;
	lookup.pathname = fsreq->path;
	lookup.cmd = CMD_CREATE;
					
	if (FatLookup (&lookup) == 0)
	{
		if (lookup.node == NULL)
		{
			if ((node = FatCreateDir (fsb, lookup.parent, lookup.last_component)) != NULL)
			{
				FreeNode (fsb, node);
				FreeNode (fsb, lookup.parent);
				fsreq->error = 0;
				fsreq->rc = 0;
				return;
			}
			else
			{
				FreeNode (fsb, lookup.parent);
				fsreq->error = GetError();
				fsreq->rc = -1;
				return;
			}
		}
		else
		{
			FreeNode (fsb, lookup.node);
			FreeNode (fsb, lookup.parent);
			fsreq->error = EEXIST;
		}
	}
	
	fsreq->rc = -1;
}




/*
 *
 */

void FatDoUnlink (struct FSReq *fsreq)
{
	struct FatSB *fsb;
	struct FatLookup lookup;
	
		
	fsb = fsreq->unitp;

	if (FatIsValid (fsb, fsreq) == -1)
		return;

	lookup.fsb = fsb;
	lookup.pathname = fsreq->path;
	lookup.cmd = CMD_DELETE;
					
	if (FatLookup (&lookup) == 0)
	{
		if (lookup.node != NULL)
		{
			if (FatDeleteFile (fsb, lookup.parent, lookup.node) == 0)
			{
				FreeNode (fsb, lookup.parent);
				
				fsreq->error = 0;
				fsreq->rc = 0;
				return;
			}
		}
		
		FreeNode (fsb, lookup.parent);
	}
	else
		fsreq->error = ENOENT;
	
	fsreq->error = GetError();
	fsreq->rc = -1;
}




/*
 *
 */

void FatDoRmdir (struct FSReq *fsreq)
{
	struct FatSB *fsb;
	struct FatLookup lookup;
		
	
	fsb = fsreq->unitp;

	if (FatIsValid (fsb, fsreq) == -1)
		return;

	lookup.fsb = fsb;
	lookup.pathname = fsreq->path;
	lookup.cmd = CMD_DELETE;
					
	if (FatLookup (&lookup) == 0)
	{
		if (FatDeleteDir (fsb, lookup.parent, lookup.node) == 0)
		{
			FreeNode (fsb, lookup.parent);
			
			fsreq->error = 0;
			fsreq->rc = 0;
			return;
		}
		
		FreeNode (fsb, lookup.parent);
	}
		
	fsreq->error = GetError();
	fsreq->rc = -1;
}




/*
 *
 */

void FatDoFormat (struct FSReq *fsreq)
{
	struct FatSB *fsb;
		
	
	fsb = fsreq->unitp;
	
	if (FatIsValid (fsb, fsreq) == -1)
		return;
	
	if (fsb->reference_cnt == 0)
	{
		if (FatFormat (fsb, fsreq->format_label, fsreq->format_flags, fsreq->format_cluster_sz) == 0)
		{
			fsreq->error = 0;
			fsreq->rc = 0;
			return;
		}
	}
	
	
	fsreq->error = EINVAL;
	fsreq->rc = -1;
}




/*
 * FatDoRename();
 *
 * I assume outer wrapper checks the source and dst pathnames are on the same
 * drive?
 *
 * FIXME:  Needs to be finished/is not working.
 */

void FatDoRename (struct FSReq *fsreq)
{
	struct FatSB *fsb;
	struct FatLookup lookup_src, lookup_dst;
	

	fsb = fsreq->unitp;
	
	if (FatIsValid (fsb, fsreq) == -1)
		return;
		
	lookup_src.fsb = fsb;
	lookup_src.pathname = fsreq->path;
	lookup_src.cmd = CMD_RENAME;
	
	lookup_dst.fsb = fsb;
	lookup_dst.pathname = fsreq->path2;
	lookup_dst.cmd = CMD_RENAME;

	/* FIXME: Are nodes opened and incremented?
		Should FAT handler allow rename of open files */

	if (FatLookup (&lookup_src) == 0)
	{
		if (FatLookup (&lookup_dst) == 0)
		{
			if (lookup_src.node->reference_cnt > 1)
			{
				if (lookup_dst.parent == lookup_src.parent)
				{
					if (StrCmp (lookup_dst.last_component, lookup_src.last_component) == 0)
					{
						fsreq->rc = 0;
						fsreq->error = 0;
					}
					else
					{
						FatASCIIZToDirEntry (&lookup_src.node->dirent, lookup_dst.last_component);
						FatSetTime (fsb, &lookup_src.node->dirent, ST_MTIME);
						FlushDirent (fsb, lookup_src.node);
						
						fsreq->rc = 0;
						fsreq->error = 0;
					}
				}
				else
				{
					/* Fully Qualified Case */
					fsreq->rc = -1;
					fsreq->error = EINVAL;
				}
			}
			else
			{
				/* File already open */
				fsreq->rc = -1;
				fsreq->error = EINVAL;		/* Change the error message */
			}
			
			if (lookup_dst.node != NULL)
				FreeNode (fsb, lookup_dst.node);
			
			if (lookup_dst.parent != NULL)
				FreeNode (fsb, lookup_dst.parent);
		}
		
		if (lookup_src.node != NULL)
			FreeNode (fsb, lookup_src.node);
		
		if (lookup_src.parent != NULL)
			FreeNode (fsb, lookup_src.parent);
	}
	else
	{
		fsreq->error = GetError();
		fsreq->rc = -1;
	}	
}




/*
 *
 */

void FatDoChmod (struct FSReq *fsreq)
{
	struct FatSB *fsb;
	
	fsb = fsreq->unitp;
	
	
	if (FatIsValid (fsb, fsreq) == -1)
		return;
	
	fsreq->error = 0;
	fsreq->rc = 0;
}




/*
 *
 */

void FatDoChown (struct FSReq *fsreq)
{
	struct FatSB *fsb;
	
	fsb = fsreq->unitp;

	if (FatIsValid (fsb, fsreq) == -1)
		return;

	fsreq->error = 0;
	fsreq->rc = 0;
}

