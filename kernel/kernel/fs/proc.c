#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/sync.h>
#include <kernel/fs.h>
#include <kernel/device.h>
#include <kernel/kmalloc.h>
#include <kernel/proc.h>
#include <kernel/utility.h>
#include <kernel/dbg.h>




/*
 * FSCreateProcess();
 */

int FSCreateProcess (struct Process *oldproc, struct Process *newproc, char *cdir, bool dup_fd)
{
	int t;
	int error = 0;
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	
	
	newproc->reply_port.signal = SIG_FS;
	newproc->reply_port.pid = (newproc - process) + 1;
	LIST_INIT (&newproc->reply_port.msg_list);
	
	
	if ((newproc->current_dir = KMalloc (MAX_PATHNAME_SZ)) != NULL)
	{
		if (cdir == NULL)
			for (t=0; t<MAX_PATHNAME_SZ; t++)
				newproc->current_dir[t] = '\0';
		else
			for (t=0; t<MAX_PATHNAME_SZ; t++)
				newproc->current_dir[t] = oldproc->current_dir[t];
		
		for (t=0; t < MAX_FD; t++)
		{
			newproc->filedesc[t] = NULL;
			newproc->close_on_exec[t] = FALSE;
		}
		
			
		if (dup_fd == DUP_FD)
		{
			for (t=0; t < MAX_FD && error == 0; t++)
			{
				if ((filp = oldproc->filedesc[t]) != NULL)
				{
					device = (struct Device *) *(void **)filp;
					
					
					/* FIXME: Why would device == NULL ? */
					
					/* FIXME: MAYBE,  no need for RWLock in dup() */
					
					if (device != NULL)
					{
						fsreq.device = device;
						fsreq.unitp = NULL;
						fsreq.cmd = FS_CMD_DUP;
						fsreq.filp = filp;
						DoIO (&fsreq, NULL);
						SetError (fsreq.error);
						error = fsreq.rc;
					}
					

					if (error == -1)
					{
						newproc->filedesc[t] = NULL;
						newproc->close_on_exec[t] = FALSE;
						break;
					}
					else
					{
						newproc->filedesc[t] = fsreq.filp2;
						newproc->close_on_exec[t] = oldproc->close_on_exec[t];
					}
				}
				else
					newproc->filedesc[t] = NULL;
			}
		
			if (error == 0)
			{
				return 0;
			}
			
			
			/* The error path, close FDs, current dir etc */
			
			for (t=0; t < MAX_FD; t++)
			{
				filp = newproc->filedesc[t];

				if (filp != NULL)
				{
					device = (struct Device *) *(void **)filp;
					
					/* FIXME: Why would device == NULL ? */
					
					RWReadLock (&mountlist_rwlock);
					
					if (device != NULL)
					{
						fsreq.device = device;
						fsreq.unitp = NULL;
						fsreq.cmd = FS_CMD_CLOSE;
						fsreq.filp = newproc->filedesc[t];
						DoIO (&fsreq, NULL);
						SetError (fsreq.error);
					}
					
					RWUnlock (&mountlist_rwlock);
					
					newproc->filedesc[t] = NULL;
				}
			}
		}
		else
		{
			KPRINTF ("End CreateProcFS success  NODUP NODUP");
			/* Maybe should return 0 here ?????????????  */
			return 0;
		}
				
		KFree (newproc->current_dir);
		newproc->current_dir = NULL;
	}
	else
		error = -1;
	
	
	return error;
}






/*
 * FSExitProcess();
 */

void FSExitProcess (struct Process *process)
{
	int t;
	void *filp;
	struct Device *device;
	struct FSReq fsreq;		

		
	for (t=0; t < MAX_FD; t++)
	{
		if ((filp = process->filedesc[t]) != NULL)
		{
			device = (struct Device *) *(void **)filp;
			
			/* FIXME: Why would device == NULL ? */
					
			RWReadLock (&mountlist_rwlock);

			if (device != NULL)
			{
				fsreq.device = device;
				fsreq.unitp = NULL;
				fsreq.cmd = FS_CMD_CLOSE;
				fsreq.filp = process->filedesc[t];
				DoIO (&fsreq, NULL);
				SetError (fsreq.error);
			}
			
			RWUnlock (&mountlist_rwlock);
					
			process->filedesc[t] = NULL;
		}
	}
}







/*
 * FSCloseOnExec();
 */

void FSCloseOnExec(struct Process *process)
{
	int t;
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	
		
	for (t=0; t < MAX_FD; t++)
	{
		if ((filp = process->filedesc[t]) != NULL && process->close_on_exec[t] == TRUE)
		{
			device = (struct Device *) *(void **)filp;
			
			RWReadLock (&mountlist_rwlock);
			
			/* FIXME: Why would device == NULL ? */
			
			if (device != NULL)
			{
				fsreq.device = device;
				fsreq.unitp = NULL;
				fsreq.cmd = FS_CMD_CLOSE;
				fsreq.filp = process->filedesc[t];
				DoIO (&fsreq, NULL);
				SetError (fsreq.error);
			}
			
			process->filedesc[t] = NULL;
			process->close_on_exec[t] = FALSE;

			RWUnlock (&mountlist_rwlock);
		}
	}
}
