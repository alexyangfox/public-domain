#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/timer.h>
#include <kernel/usignal.h>
#include <kernel/dbg.h>





/*
 *
 */

int USleep (uint32 seconds, uint32 microseconds)
{
	struct TimeVal tv;
	
	tv.seconds = seconds;
	tv.microseconds = microseconds;
	
	KSleep (&tv);
	
	return 0;
}



/*
 * UAlarm(0) cancels alarm
 *
 * Need to initialise timer in AllocProcess().
 * InitTimer();
 */

unsigned int UAlarm (unsigned int seconds)
{
	struct TimeVal tv;
	unsigned int remaining;


	if (current_process->ualarm_timer.expired == FALSE)
		remaining = current_process->ualarm_timer.expiration_seconds;
	else
		remaining = 0;
	

	CancelTimer (&current_process->ualarm_timer);
		
	current_process->usignal.sig_pending &= ~SIGFALRM;
	if ((current_process->usignal.sig_pending & current_process->usignal.sig_mask) == 0)
		current_process->pending_signals &= ~SIGF_USER;
	
	
	if (seconds > 0)
	{
		tv.seconds = seconds;
		tv.microseconds = 0;
	
		SetTimer (&current_process->ualarm_timer, TIMER_TYPE_RELATIVE, &tv,
					&UAlarmCallout, current_process);
	}
	
	return remaining;	
}




/*
 *
 */

void UAlarmCallout (struct Timer *timer, void *arg)
{
	struct Process *proc = (struct Process *)arg;
	int pid;
	
	pid = (proc - process) + 1;
	
	if (proc->usignal.handler[SIGALRM-1] != SIG_IGN)
	{
		proc->usignal.sig_pending |= SIGFALRM;
		KSignal (pid, SIG_USER);
	}
}


