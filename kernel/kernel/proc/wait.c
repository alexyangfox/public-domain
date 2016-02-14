#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/fs.h>
#include <kernel/kmalloc.h>
#include <kernel/error.h>
#include <kernel/lists.h>



void UpdateSigChld (struct Process *proc);




/*
 * WaitPid();
 */

int WaitPid (int pid, int32 *status, int options)
{
	int waiting_for;
	int r_pid;
	struct Process *child;
	struct Process *terminated_proc;
	int matching_cnt;


	KPRINTF ("WaitPid (%d, %#010x, %d)", pid, status, options);


	if (pid == 0)
		waiting_for = -current_process->pgrp;
	else
		waiting_for = pid;

	
	while(1)
	{
		matching_cnt = 0;
		terminated_proc = NULL;
		
		MutexLock (&proc_mutex);
		
		child = LIST_HEAD (&current_process->child_list);
	
		while (child != NULL)
		{
			if (waiting_for > 0 && child->pid != waiting_for)
			{
				child = LIST_NEXT (child, child_entry);
				continue;
			}
								
			if (waiting_for < -1 && child->pgrp != -waiting_for)
			{
				child = LIST_NEXT (child, child_entry);
				continue;
			}
				
			if (child->state == PROC_STATE_ZOMBIE)
				terminated_proc = child;
					
			matching_cnt ++;
						
			child = LIST_NEXT (child, child_entry);
		}

		MutexUnlock (&proc_mutex);
		
		
		
		if (matching_cnt != 0)
		{
			if (terminated_proc != NULL)
			{
				r_pid = terminated_proc->pid;
				
				
				if (status != NULL)
					CopyOut (current_process->user_as, status, &terminated_proc->status, sizeof (int));
				
				FreeProcess (terminated_proc);
				UpdateSigChld (current_process);
				
				return r_pid;
			}	
			else
			{
				if ((options & WNOHANG) == 0)
				{
					DisableInterrupts();
					current_process->waiting_for = waiting_for;
					current_process->state = PROC_STATE_WAITPID;
					SchedUnready(current_process);
					Reschedule();
					EnableInterrupts();
					continue;
				}
				else
				{
					SetError (ECHILD);
					return 0;
				}
			}
		}
		else
		{
			SetError (ECHILD);
			return -1;
		}
	}
}




/*
 *
 */

void UpdateSigChld (struct Process *proc)
{
	if (LIST_HEAD(&proc->child_list) == NULL)
	{
		DisableInterrupts();
	
		proc->pending_signals &= ~SIGF_CHLD;
		
		proc->usignal.sig_pending &= ~SIGFCHLD;
			
		if ((proc->usignal.sig_pending 
			& ~proc->usignal.sig_mask) == 0)
		{
			proc->pending_signals &= ~SIGF_USER;
		}

		EnableInterrupts();
	}
	
	/* Raise signal/awaken if new child added? */
}
