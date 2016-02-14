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

extern void ISRResumptionPoint;
extern void RescheduleResume;




/*
 * PickProc();
 *
 * Picks the highest priority runnable process and sets it to
 * the current process.  Called by Reschedule() and the interrupt handler.
 *
 * 256 Priority queues, each round-robin,  highest priority runs.
 * Priority is between -128 to + 127.  A bitmap of 8 longwords is used
 * to keep track of queues with running threads on.  The bitmap is scanned
 * to find the highest priority.  After 4 timer ticks the queue of the
 * current process is rotated round-robin style.
 *
 * reschedule_request is set by SchedReady() or SchedUnready(), useful in
 * interrupt handlers that call KSignal() or timer callbacks that can't
 * call Reschedule() immediately.
 *
 * Each process maintains a preempt_cnt of DisablePreempt() which
 * disables preemption.  Only when zero will a reschedule occur.  This may
 * be remove as it is not used anywhere.
 */

void PickProc (void)
{
	int32 bm;
	int32 q;
	

	if (reschedule_request == TRUE && current_process->preempt_cnt == 0)
	{
		if (current_process->state == PROC_STATE_RUNNING)
			current_process->state = PROC_STATE_READY;
		
		/*if (current_process->quanta_used > 4)
		{*/
			CIRCLEQ_FORWARD (&sched_queue[current_process->priority+128], sched_entry);
			current_process->quanta_used = 0;
		
		/*}*/
		
		
		current_process = NULL;
		
		for (bm=7; bm >= 0 && current_process == NULL; bm--)
		{
			for (q=31; q >= 0 && current_process == NULL; q--)
			{
				if (sched_queue_bitmap[bm] & (1<<q))
				{
					current_process = CIRCLEQ_HEAD (&sched_queue[(bm*32)+q]);
					current_process->state = PROC_STATE_RUNNING;
				}
			}
		}

	
		ArchPickProc (current_process);
		
		reschedule_request = FALSE;
	}
}




/*
 * SchedReady();
 *
 * Add process to a ready queue based on its priority.
 *
 * Example:
 *
 * DisableInterrupts();
 * proc->state = PROC_STATE_READY;
 * SchedReady(proc);
 * Reschedule();
 * DisableInterrupts();
 *
 * If Reschedule() picks a new process then the interrupt
 * state of that process is used.
 */

void SchedReady (struct Process *proc)
{
	int32 bm; 
	int32 q;
	int32 unsigned_pri;
	
	
	unsigned_pri = (int32)proc->priority + 128;
	bm = unsigned_pri / 32;
	q = unsigned_pri % 32;
		
	reschedule_request = TRUE;
	proc->quanta_used = 0;

	CIRCLEQ_ADD_TAIL (&sched_queue[unsigned_pri], proc, sched_entry);
	sched_queue_bitmap[bm] |= (1<<q);
}




/*
 * SchedUnready();
 *
 * Removes process from the ready queue.  See SchedReady() above.
 */

void SchedUnready (struct Process *proc)
{
	int32 bm; 
	int32 q;
	int32 unsigned_pri;
	

	unsigned_pri = (int32)proc->priority + 128;
	bm = unsigned_pri / 32;
	q = unsigned_pri % 32;
		
	reschedule_request = TRUE;
	proc->quanta_used = 0;
	
	CIRCLEQ_REM_ENTRY (&sched_queue[unsigned_pri], proc, sched_entry);
	
	if (CIRCLEQ_HEAD (&sched_queue[unsigned_pri]) == NULL)
		sched_queue_bitmap[bm] &= ~(1<<q);
}




/*
 * DisablePreempt();
 *
 * Disable preemption,  not used anywhere, may remove.
 */

void DisablePreempt(void)
{
	if (current_process == NULL)
		return;
	
	current_process->preempt_cnt++;
}




/*
 * EnablePreempt();
 *
 * Enable preemption,  not used anywhere, may remove.
 */

void EnablePreempt(void)
{
	if (current_process == NULL)
		return;
	
	current_process->preempt_cnt--;

	if (reschedule_request == TRUE && current_process->preempt_cnt == 0)
	{
		DisableInterrupts();
		Reschedule();
		EnableInterrupts();
	}
}




/*
 * EnablePreemptNoResched();
 */

void EnablePreemptNoResched(void)
{
	current_process->preempt_cnt--;
}




/*
 * Yield();
 */

void Yield (void)
{
	DisableInterrupts();
	Reschedule();
	EnableInterrupts();
}




/*
 * ChangePriority();
 */

int32 ChangePriority (int32 priority)
{
	int32 old_priority;
	DisableInterrupts();
	
	 old_priority = current_process->priority;
	
	/* FIX: Remove from actual priority queue, ,move to new priority queue */
	/* Clear quanta_used ? */	
	
	EnableInterrupts();	
	return old_priority;
}




