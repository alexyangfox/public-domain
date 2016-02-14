#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/sync.h>




void CondTimedWaitCallout (struct Timer *timer, void *arg);




/*
 * Rename to Cond
 */

void CondInit (struct Cond *cond)
{
	LIST_INIT (&cond->blocked_list);
}




/*
 * CondWait();
 */

void CondWait (struct Cond *cond, struct Mutex *mutex)
{
	struct Process *proc;
	
	if (current_process == NULL)
		return;
	
	DisableInterrupts();
	
	/* Release Mutex */
	
	if (mutex->locked == TRUE)   
	{
		proc = LIST_HEAD (&mutex->blocked_list);
		
		if (proc != NULL)
		{
			LIST_REM_HEAD (&mutex->blocked_list, blocked_entry);
			proc->state = PROC_STATE_READY;
			SchedReady (proc);
		}
		else
		{
			mutex->locked = FALSE;
		}
	}
	else
	{
		KPANIC ("CondWait() on a free mutex");
	}
	
	
	/* Block current process on Condvar */
	
	LIST_ADD_TAIL (&cond->blocked_list, current_process, blocked_entry);
	current_process->state = PROC_STATE_COND_BLOCKED;
	SchedUnready (current_process);
	Reschedule();
	
	
	/* Now acquire mutex */
	
	if (mutex->locked == TRUE)
	{
		LIST_ADD_TAIL (&mutex->blocked_list, current_process, blocked_entry);
		current_process->state = PROC_STATE_MUTEX_BLOCKED;
		SchedUnready (current_process);
		Reschedule();
	}
	else
	{
		mutex->locked = TRUE;
	}
	
	
	
	EnableInterrupts();
}




/*
 * CondSignal();
 */

void CondSignal (struct Cond *cond)
{
	struct Process *proc;
	
	if (current_process == NULL)
		return;
	
	
	DisableInterrupts();
	
	proc = LIST_HEAD (&cond->blocked_list);
	
	if (proc != NULL)
	{
		LIST_REM_HEAD (&cond->blocked_list, blocked_entry);
		proc->state = PROC_STATE_READY;
		SchedReady (proc);
		Reschedule();
	}

	EnableInterrupts();
}




/*
 * CondBroadcast();
 */

void CondBroadcast (struct Cond *cond)
{
	struct Process *proc;
	
	if (current_process == NULL)
		return;
	
	do
	{
		DisableInterrupts();
	
		proc = LIST_HEAD (&cond->blocked_list);
	
		if (proc != NULL)
		{
			LIST_REM_HEAD (&cond->blocked_list, blocked_entry);
			
			proc->state = PROC_STATE_READY;
			SchedReady (proc);
			Reschedule();
		}
		
		proc = LIST_HEAD (&cond->blocked_list);
		EnableInterrupts();

	} while (proc != NULL);
}




/*
 * CondTimedWait();
 *
 * If a timeout occurs cond = NULL,  Hope I don't need to
 * set it to a volatile variable and that it doesn't cache
 * the contents after the sleep().
 */

int CondTimedWait (volatile struct Cond *cond, struct Mutex *mutex, struct TimeVal *tv)
{
	struct Process *proc;
	struct Timer timer;
	
	if (current_process == NULL)
		return 0;
	
	DisableInterrupts();
	
	if (mutex->locked == TRUE)   
	{
		/* Release Mutex */

		proc = LIST_HEAD (&mutex->blocked_list);
		
		if (proc != NULL)
		{
			LIST_REM_HEAD (&mutex->blocked_list, blocked_entry);
			proc->state = PROC_STATE_READY;
			SchedReady (proc);
		}
		else
		{
			mutex->locked = FALSE;
		}
	}
	else
	{
		KPANIC ("CondTimedWait() on a free mutex");
	}
	
	
	/* Block current process on Condvar */

	LIST_ADD_TAIL (&cond->blocked_list, current_process, blocked_entry);
		
	SetTimer (&timer, TIMER_TYPE_RELATIVE, tv, &CondTimedWaitCallout, &cond);
	current_process->state = PROC_STATE_COND_BLOCKED;
	SchedUnready (current_process);
	Reschedule();

	CancelTimer (&timer);

	/* Now acquire mutex */
	
	if (mutex->locked == TRUE)
	{
		LIST_ADD_TAIL (&mutex->blocked_list, current_process, blocked_entry);
		current_process->state = PROC_STATE_MUTEX_BLOCKED;
		SchedUnready (current_process);
		Reschedule();
	}
	else
	{
		mutex->locked = TRUE;
	}
	
	
	EnableInterrupts();
	
	if (cond == NULL)
		return -1;
	else
		return 0;
}




/*
 * CondTimedWaitCallout();
 * 
 * Wake up the CondTimedWait() process.  'arg' contains a pointer
 * to the struct Cond *cond argument of CondTimedWait().  This
 * is cleared to indicate that a timeout occured.	
 */

void CondTimedWaitCallout (struct Timer *timer, void *arg)
{
	struct Process *proc;
	struct Cond *cond;
	
	cond = *(struct Cond **)arg;
	proc = timer->process;
		
	if (proc->state == PROC_STATE_COND_BLOCKED)
	{
		*(struct Cond **)arg = NULL;

		LIST_REM_HEAD (&cond->blocked_list, blocked_entry);
		proc->state = PROC_STATE_READY;
		SchedReady (proc);
		Reschedule();
	}
}
