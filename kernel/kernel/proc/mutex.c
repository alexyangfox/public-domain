#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/sync.h>



/*
 * MutexInit();
 */

void MutexInit (struct Mutex *mutex)
{
	mutex->locked = FALSE;
	LIST_INIT (&mutex->blocked_list);
}




/*
 * MutexTryLock();
 */

int MutexTryLock (struct Mutex *mutex)
{
	int status;
	
	if (current_process == NULL)
		return 0;
	
	DisableInterrupts();
	
	if (mutex->locked == FALSE)
	{
		mutex->locked = TRUE;
		status = 0;
	}
	else
	{
		status = -1;
	}
	
	EnableInterrupts();
	return status;
}




/*
 * MutexLock(mutex, struct Timeval *tm);  ?
 *
 * Maybe mtx.flags can control whether timeouts or recursion allowed 
 *
 *
 *
 */

void MutexLock (struct Mutex *mutex)
{
	if (current_process == NULL)
		return;
	
	DisableInterrupts();
	
	if (mutex->locked == FALSE)
	{
		mutex->locked = TRUE;
	}
	else
	{
		LIST_ADD_TAIL (&mutex->blocked_list, current_process, blocked_entry);
		current_process->state = PROC_STATE_MUTEX_BLOCKED;
		SchedUnready (current_process);
		Reschedule();
	}
	
	EnableInterrupts();
}




/*
 * MutexUnlock();
 */

void MutexUnlock (struct Mutex *mutex)
{
	struct Process *proc;
	
	if (current_process == NULL)
		return;
	
	DisableInterrupts();
	
	if (mutex->locked == TRUE)
	{
		proc = LIST_HEAD(&mutex->blocked_list);
		
		if (proc != NULL)
		{
			mutex->locked = TRUE;
			
			LIST_REM_HEAD (&mutex->blocked_list, blocked_entry);
			
			proc->state = PROC_STATE_READY;
			SchedReady (proc);
			Reschedule();
		}
		else
		{
			mutex->locked = FALSE;
		}
	}

	EnableInterrupts();
}







