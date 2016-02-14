#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/timer.h>


void KTimedWaitCallout (struct Timer *timer, void *arg);



/*
 * AllocSignal()
 *
 * Ensure we don't allocate a reserved signal, rather than
 * setting them as allocated in KSpawn().
 */
 
int32 AllocSignal (void)
{
	int32 signal;
	
	for (signal = MIN_SIGNAL; signal < MAX_SIGNAL; signal++)
	{
		if (((1<<signal) & (current_process->free_signals)) != 0)
		{
			current_process->free_signals &= ~(1<<signal);
			
			return signal;
		}
	}

	return -1;
}




/*
 * FreeSignal();
 *
 * Ensure we don't free a reserved signal.
 */

void FreeSignal (int32 signal)
{
	if (signal >= MIN_SIGNAL && signal < MAX_SIGNAL)
		current_process->free_signals |= (1<<signal);
}




/*
 * KSignal();
 *
 * Usable within interrupt handlers.  If KSignal() is called within an
 * interrupt handler Reschedule() is not immediately called,  instead
 * reschedule_request is set by SchedReady() so that PickProc() can
 * choose a new process at the end of interrupt processing.
 *
 * What if a task waits on a signal and a timer,  any problems?
 *
 */
 
void KSignal (int pid, int32 signal)
{
	uint32 interrupt_state;
	struct Process *proc;
	
	proc = process + pid - 1;
		
	interrupt_state = DisableInterrupts();

	if (proc->state != PROC_STATE_UNALLOC)
	{
		proc->pending_signals |= (1<<signal);
	
		if (proc->state == PROC_STATE_WAIT_BLOCKED
				&& (proc->waiting_for_signals & proc->pending_signals))
		{
			proc->state = PROC_STATE_READY;
			SchedReady (proc);
		
			if (isr_depth == 0)
				Reschedule();
		}
	}
	
	RestoreInterrupts (interrupt_state);
}




/*
 * KSetSignal();  Behaves as GetSignals() when newsigs = mask = 0
 *
 *
 * FIX : This is buggy, really should wake up WAITING process
 */
 
uint32 KSetSignals (uint32 newsigs, uint32 mask)
{
	uint32 old_signals;

	DisableInterrupts();
	
	old_signals = current_process->pending_signals;
	current_process->pending_signals = (newsigs & mask) | (current_process->pending_signals & ~mask);

	EnableInterrupts();

	return old_signals;
}




/*
 * KWait();
 */

uint32 KWait (uint32 mask)
{
	uint32 signals;
	uint32 int_state;
	
	int_state = DisableInterrupts();
	
	current_process->waiting_for_signals = mask;
	
	signals = current_process->waiting_for_signals & current_process->pending_signals;
	
	if (signals == 0)
	{
		current_process->state = PROC_STATE_WAIT_BLOCKED;
		SchedUnready (current_process);
		Reschedule();
		
		signals = current_process->waiting_for_signals & current_process->pending_signals;
	}
	
	current_process->pending_signals &= ~(signals & ~(SIGF_NO_CLEAR_WAIT));
	
	RestoreInterrupts (int_state);
	
	return signals;
}




/*
 * KTimedWait();
 *
 * More or less the same as KWait(), except it has a timeout.
 * returns a bitmap of masked signals received.  There is no way
 * of detecting whether a timeout occured.  A timeout is not
 * guaranteed to return with 0x00000000 signal bitmap,  it ia
 * possible that after the timer wakens the task but before the
 * signals bits a retrieved that another task may set the pending
 * signal bits.
 *
 * Could set the proc->error to ETIMEDOUT
 */

uint32 KTimedWait (uint32 mask, struct TimeVal *tv)
{
	struct Timer timer;
	uint32 signals;
	
	DisableInterrupts();
	
	current_process->waiting_for_signals = mask;
	
	signals = current_process->waiting_for_signals & current_process->pending_signals;
	
	if (signals == 0)
	{
		SetTimer (&timer, TIMER_TYPE_RELATIVE, tv, &KTimedWaitCallout, NULL);

		current_process->state = PROC_STATE_WAIT_BLOCKED;		
		SchedUnready (current_process);
		Reschedule();
		
		CancelTimer (&timer);
			
		signals = current_process->waiting_for_signals & current_process->pending_signals;
	}
	
	current_process->pending_signals &= ~(signals & ~(SIGF_NO_CLEAR_WAIT));
	
	EnableInterrupts();
		
	return signals;
}




/*
 * KTimedWaitCallout();
 *
 * Awaken the KTimedWait() process.  Let it cancel the timer.
 */

void KTimedWaitCallout (struct Timer *timer, void *arg)
{
	if (timer->process->state == PROC_STATE_WAIT_BLOCKED)
	{
		timer->process->state = PROC_STATE_READY;
		SchedReady (timer->process);
	}
}




