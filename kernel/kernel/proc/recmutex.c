#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/sync.h>




/*
 * RecMutexInit();
 */

void RecMutexInit (struct RecMutex *mutex)
{
	mutex->locked = FALSE;
	mutex->owner = NULL;
	mutex->recursion_cnt = 0;
	LIST_INIT (&mutex->blocked_list);
}




/*
 * RecMutexTryLock();
 */

int RecMutexTryLock (struct RecMutex *mutex)
{
	int status;
	
	if (current_process == NULL)
		return 0;
	
	DisableInterrupts();
	
	if (mutex->locked == FALSE)
	{
		mutex->locked = TRUE;
		mutex->owner = current_process;
		mutex->recursion_cnt = 1;
		status = 0;
	}
	else
	{
		if (mutex->owner == current_process)
		{
			mutex->recursion_cnt ++;
			status = 0;
		}
		else		
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

void RecMutexLock (struct RecMutex *mutex)
{
	if (current_process == NULL)
		return;
	
	DisableInterrupts();
	
	if (mutex->locked == FALSE)
	{
		mutex->locked = TRUE;

		mutex->owner = current_process;
		mutex->recursion_cnt = 1;
	}
	else
	{
		if (mutex->owner == current_process)
		{
			mutex->recursion_cnt ++;
		}
		else	
		{
			LIST_ADD_TAIL (&mutex->blocked_list, current_process, blocked_entry);
			current_process->state = PROC_STATE_MUTEX_BLOCKED;
			SchedUnready (current_process);
			Reschedule();
		}
	}
	
	EnableInterrupts();
}




/*
 * MutexUnlock();
 */

void RecMutexUnlock (struct RecMutex *mutex)
{
	struct Process *proc;
	
	if (current_process == NULL)
		return;
	
	DisableInterrupts();
	
	if (mutex->locked == TRUE)
	{
		if (mutex->owner == current_process)
			mutex->recursion_cnt --;
		
		
		if (mutex->recursion_cnt == 0)
		{
			proc = LIST_HEAD(&mutex->blocked_list);
			
			if (proc != NULL)
			{
				mutex->locked = TRUE;
				mutex->owner = proc;
				
				LIST_REM_HEAD (&mutex->blocked_list, blocked_entry);
				proc->state = PROC_STATE_READY;
				SchedReady (proc);
				Reschedule();
			}
			else
			{
				mutex->owner = NULL;
				mutex->locked = FALSE;
			}
		}
	}

	EnableInterrupts();
}







