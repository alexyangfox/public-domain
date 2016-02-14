#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/fs.h>
#include <kernel/kmalloc.h>
#include <kernel/error.h>




/*
 * static prototypes
 */

static void DoExit (int32 status);




/*
 * Exit();
 */

void Exit (int status)
{
	DoExit (EXIT_STATUS(status));
}




/*
 *
 */

void USigExit (int sig)
{
	EnableInterrupts();
	
	KPRINTF ("USigExit()");
	DoExit (sig);
}




/*
 * DoExit();
 *
 */

static void DoExit (int32 status)
{
	struct Process *parent;
	struct Process *child;
	int pid;
	int pgrp;
	int child_cnt;
	
	
	KPRINTF ("DoExit (%d)", status);
	
	
	pid = GetPID();
	pgrp = GetPGRP();
	
	KASSERT (pgrp > 0);
	
	current_process->status = status;
	child_cnt = 0;
	
		
	SwitchAddressSpace (&kernel_as);
	FreeAddressSpace (&current_process->as);
	UAlarm(0);
	FSExitProcess (current_process);
	
	if (current_process->pid == current_process->pgrp)
		UKill (-pgrp, SIGHUP);
	
	
	MutexLock (&proc_mutex);

	parent = current_process->parent;
	
	KASSERT (parent != NULL);
	KASSERT (parent->state != PROC_STATE_UNALLOC);
		
	while ((child = LIST_HEAD (&current_process->child_list)) != NULL)
	{
		child_cnt ++;
		
		DisableInterrupts();
		
		child->parent = root_process;		
		LIST_REM_ENTRY (&current_process->child_list, child, child_entry);
		LIST_ADD_TAIL (&root_process->child_list, child, child_entry);

		EnableInterrupts();
	}
	
	
	if (child_cnt != 0)
	{
		root_process->pending_signals |= (SIGF_CHLD | SIGF_USER);
		root_process->usignal.sig_pending |= SIGFCHLD;

		DisableInterrupts();

		if (root_process->state == PROC_STATE_WAITPID
			&& (root_process->waiting_for == pid
				|| root_process->waiting_for == -pgrp
				|| root_process->waiting_for == -1))
		{
			root_process->state = PROC_STATE_READY;
			SchedReady (root_process);
			Reschedule();
		}
		else if (root_process->state == PROC_STATE_WAIT_BLOCKED
			&& (root_process->waiting_for_signals & root_process->pending_signals))
		{
			root_process->state = PROC_STATE_READY;
			SchedReady (root_process);
			Reschedule();
		}
		
		EnableInterrupts();
	}
		
	MutexUnlock(&proc_mutex);

	
	DisableInterrupts();
	
	parent->pending_signals |= (SIGF_CHLD | SIGF_USER);
	parent->usignal.sig_pending |= SIGFCHLD;
	
	parent->usignal.siginfo_data[SIGCHLD-1].si_signo = SIGCHLD;
	parent->usignal.siginfo_data[SIGCHLD-1].si_code = 0;
	parent->usignal.siginfo_data[SIGCHLD-1].si_value.sival_int = 0;
	
	
	if (parent->state == PROC_STATE_WAITPID
		&& (parent->waiting_for == pid || parent->waiting_for == -pgrp || parent->waiting_for == -1))
	{
		parent->state = PROC_STATE_READY;
		SchedReady (parent);
	}
	else if (parent->state == PROC_STATE_WAIT_BLOCKED
		&& (parent->waiting_for_signals & parent->pending_signals))
	{
		parent->state = PROC_STATE_READY;
		SchedReady (parent);
	}

	current_process->state = PROC_STATE_ZOMBIE;	
	SchedUnready (current_process);
	Reschedule();
}



