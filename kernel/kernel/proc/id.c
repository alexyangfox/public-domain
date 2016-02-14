#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/fs.h>
#include <kernel/kmalloc.h>
#include <kernel/i386/elf.h>
#include <kernel/i386/i386.h>
#include <kernel/error.h>




/*
 *
 */

struct Process *PIDtoProc (int pid)
{
	if (pid > 0 && pid <= process_cnt)
		return process + pid - 1;
	else
		return NULL;
}




/*
 *
 */

int GetPID (void)
{
	return current_process->pid;
}




/*
 *
 */

int GetPPID (void)
{
	int ppid;
		
	DisableInterrupts();
	ppid = (current_process->parent - process) + 1;
	EnableInterrupts();

	return ppid;
}




/*
 *
 */

int GetUID (void)
{
	return current_process->uid;
}




/*
 *
 */

int SetUID (int uid)
{
	current_process->uid = uid;
	return 0;
}




/*
 *
 */

int GetEUID (void)
{
	return current_process->uid;
}




/*
 *
 */

int GetGID (void)
{
	return current_process->gid;
}




/*
 *
 */

int SetGID (int gid)
{
	current_process->gid = gid;
	return 0;
}




/*
 *
 */

int GetEGID (void)
{
	return current_process->gid;
}












/*
 *
 */

int SetPGID (int pid, int pgrp)
{
	SetError (ENOSYS);
	return -1;
}




/*
 *
 */

int GetPGID (int pid)
{
	SetError (ENOSYS);
	return -1;
}




/*
 *
 */

int GetPGRP(void)
{
	return current_process->pgrp;
}



/*
 * SetPGRP();   Really SysV style setpgrp().
 * 
 * Set the PGRP of the current process to the PID, forming a new process_group.
 * FreePGRPProcessStruct() is responsible for freeing the old process group,
 * if the reference_cnt is 0.
 */
 
int SetPGRP (void)
{
	struct Process *pgrp_proc;
	
	
	if (current_process->pgrp != current_process->pid)
	{
		MutexLock (&proc_mutex);
		
		/* Free the current pgrp */
		
		
		pgrp_proc = PIDtoProc (current_process->pgrp);
		pgrp_proc->pgrp_reference_cnt --;
		
		if (pgrp_proc != pgrp_proc && pgrp_proc->pgrp_reference_cnt == 0)
		{
			pgrp_proc->state = PROC_STATE_UNALLOC;
			LIST_ADD_HEAD (&free_process_list, pgrp_proc, free_entry); 
		}

			
		current_process->pgrp = current_process->pid;
						
		MutexUnlock (&proc_mutex);
				
		
		/* Wake up WaitPID parent regardless of what it was waiting for */
		
		DisableInterrupts();
				
		if (current_process->parent->state == PROC_STATE_WAITPID)
		{
			current_process->parent->state = PROC_STATE_READY;
			SchedReady (current_process->parent);
			Reschedule();
		}
		
		EnableInterrupts();
		
		return 0;
	}
	else
	{
		SetError(EPERM);
		return -1;
	}
}



