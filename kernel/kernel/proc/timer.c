#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/timer.h>
#include <kernel/dbg.h>




/*
 *
 */

void HardClockProcessing(void)
{
	current_process->quanta_used++;
	reschedule_request = TRUE;
	
	hardclock_jiffies ++;
	
	if (hardclock_jiffies == JIFFIES_PER_SECOND)
	{
		hardclock_jiffies = 0;
		hardclock_seconds ++;
	}
}



/*
 * SoftClockProcessing();
 *
 * The reason we don't convert relative timers to an absolute expiration is so that we can
 * allow hardclock to be changed,  either forward or backward without affecting relative
 * timers.
 *
 * Something in the timer caused an exception,  which kept causing an exception.
 * Don't know why, don't know how it didn't run out of stack.  Should detect nested
 * exceptions.  I assume it was the timer,  may have been a bug in console.
 *
 *
 * Bug continues,  occasionally timer doesn't expire when created with KTimedWait(),
 * hardclock/softclock not initialized correctly?
 *
 * We changed from (softclock_jiffies <= hardclock_jiffies to)
 * (softclock_jiffies < hardclock_jiffies) otherwise softclock_jiffies gets ahead of
 * harclock_jiffies.  Thought that solved an earlier timer bug but now it is back.
 */

void SoftClockProcessing (void)
{
	struct Timer *timer, *next_timer;	
	
	while (softclock_seconds < hardclock_seconds || softclock_jiffies < hardclock_jiffies)
	{
		EnableInterrupts();
	
		timer = LIST_HEAD (&timing_wheel[softclock_jiffies]);
		
		while (timer != NULL)
		{
			next_timer = LIST_NEXT (timer, timer_entry);
			
			switch (timer->type)
			{
				case TIMER_TYPE_RELATIVE:
					if (timer->expiration_seconds == 0)
					{
						LIST_REM_ENTRY(&timing_wheel[softclock_jiffies], timer, timer_entry);
						timer->expired = TRUE;
						timer->callout (timer, timer->arg);
						
						timer->callout = NULL;
					}
					else
						timer->expiration_seconds--;
					
					break;
				
				case TIMER_TYPE_ABSOLUTE:
					if (timer->expiration_seconds == softclock_seconds)
					{
						LIST_REM_ENTRY(&timing_wheel[softclock_jiffies], timer, timer_entry);
						timer->expired = TRUE;
						timer->callout (timer, timer->arg);
					}
					
					break;
					
					
				default:
				{
					DisableInterrupts();
					KPRINTF("s=%d, j = %d", timer->expiration_seconds, timer->expiration_jiffies);
					KPRINTF("sc=%d, scj = %d", softclock_seconds, softclock_jiffies);
					KPANIC("Unknown Timer Type");
				}
			}
			
			
			timer = next_timer;
		}
			
		
			
		softclock_jiffies ++;
		
		if (softclock_jiffies == JIFFIES_PER_SECOND)
		{
			softclock_jiffies = 0;
			softclock_seconds ++;
		}
		
		DisableInterrupts();
	}
}




/*
 * EnqueueTimer();
 */

int EnqueueTimer (struct Timer *timer)
{
	LIST_ADD_TAIL(&timing_wheel[timer->expiration_jiffies], timer, timer_entry);
	return 0;
}




/*
 * DequeueTimer();
 */

int DequeueTimer (struct Timer *timer)
{
	LIST_REM_ENTRY(&timing_wheel[timer->expiration_jiffies], timer, timer_entry);
	return 0;
}




/*
 * SetTimeOfDay();
 */
 
int SetTimeOfDay (struct TimeVal *tv)
{
	DisableInterrupts();
	hardclock_seconds = tv->seconds;
	softclock_seconds = tv->seconds;
	EnableInterrupts();
	
	return 0;
}




/*
 *
 */
 
int KGetTimeOfDay (struct TimeVal *tv)
{
	volatile uint32 prev_seconds, prev_jiffies;
	volatile uint32 new_seconds, new_jiffies;
	
	do
	{
		prev_seconds = hardclock_seconds;
		prev_jiffies = hardclock_jiffies;

		new_seconds = hardclock_seconds;
		new_jiffies = hardclock_jiffies;
	} while (new_seconds != prev_seconds && new_jiffies != prev_jiffies);

	tv->seconds = new_seconds;
	tv->microseconds = new_jiffies;
	
	return 0;
}




/*
 * UGetTimeOfDay();
 */

int UGetTimeOfDay (struct TimeVal *tv_out)
{
	struct TimeVal tv;

	volatile uint32 prev_seconds, prev_jiffies;
	volatile uint32 new_seconds, new_jiffies;
	
	do
	{
		prev_seconds = hardclock_seconds;
		prev_jiffies = hardclock_jiffies;

		new_seconds = hardclock_seconds;
		new_jiffies = hardclock_jiffies;
	} while (new_seconds != prev_seconds && new_jiffies != prev_jiffies);

	tv.seconds = new_seconds;
	tv.microseconds = new_jiffies;
	
	return CopyOut (current_process->user_as, tv_out, &tv, sizeof (struct TimeVal));
}





/*
 * SetTimer();
 * 
 * Still unsure why,  might want to check.
 * Expiration jiffies should be added to the current hardclock_jiffies;
 *
 * otherwise we end up always placing 0 jiffies in the first bucket.
 * This might explain what we are seeing.
 *
 * Has to be due to way we count down expiration_seconds and jiffies
 * For relative timers expiration_seconds and expiration_jiffies are
 * relative and are counted down.
 */

void SetTimer (struct Timer *timer, int32 type, struct TimeVal *tv, void (*callout)(struct Timer *timer, void *arg), void *arg)
{
	uint32 interrupt_state;
	
	interrupt_state = DisableInterrupts();
	
	if (type == TIMER_TYPE_RELATIVE)
	{
		timer->expiration_seconds = tv->seconds + (tv->microseconds/1000000);
		timer->expiration_jiffies = (((tv->microseconds % 1000000)
								/ MICROSECONDS_PER_JIFFY) + hardclock_jiffies)
								% JIFFIES_PER_SECOND;
	}
	else
	{
		timer->expiration_seconds = tv->seconds + (tv->microseconds/1000000);
		timer->expiration_jiffies = (((tv->microseconds % 1000000)
								/ MICROSECONDS_PER_JIFFY))
								% JIFFIES_PER_SECOND;
	}
		
	timer->type = type;
	timer->process = current_process;
	timer->callout = callout;
	timer->arg = arg;
	timer->expired = FALSE;
	
	EnqueueTimer (timer);
	
	RestoreInterrupts (interrupt_state);
}




/*
 * CancelTimer(); 
 */

void CancelTimer (struct Timer *timer)
{
	uint32 interrupt_state;
	
	interrupt_state = DisableInterrupts();
	
	if (timer->expired == FALSE)
		DequeueTimer (timer);
	
	RestoreInterrupts (interrupt_state);
}



/*
 *
 */

void InitTimer (struct Timer *timer)
{
	timer->expired = TRUE;
	timer->expiration_seconds = 0;
	timer->expiration_jiffies = 0;
}



/*
 *
 */

int KSleep (struct TimeVal *tv)
{
	struct Timer timer;
	
	DisableInterrupts();
	
	SetTimer (&timer, TIMER_TYPE_RELATIVE, tv, &KSleepCallout, NULL);
	
	current_process->state = PROC_STATE_SLEEP;
	SchedUnready (current_process);
	Reschedule();
		
	EnableInterrupts();
	return 0;
}




/*
 *
 */

int KSleep2 (uint32 seconds, uint32 microseconds)
{
	struct TimeVal tv;
	
	tv.seconds = seconds;
	tv.microseconds = microseconds;
	
	return KSleep (&tv);
}




/*
 *
 */

void KSleepCallout (struct Timer *timer, void *arg)
{
	timer->process->state = PROC_STATE_READY;
	SchedReady (timer->process);
}




/*
 * Always relative ?????
 */

int KAlarmSet (struct Alarm *alarm, uint32 seconds, uint32 microseconds, int signal)
{
	struct TimeVal tv;
		
	tv.seconds = seconds;
	tv.microseconds = microseconds;
	
	alarm->signal = signal;
	alarm->pid = GetPID();
	alarm->timedout = FALSE;
	SetTimer (&alarm->timer, TIMER_TYPE_RELATIVE, &tv, &KAlarmCallout, alarm);
	

	return 0;
}




/*
 *
 */
 
int KAlarmCheck (struct Alarm *alarm)
{
	if (alarm->timedout == FALSE)
		return 0;
	else
		return -1;
}




/*
 *
 */
 
int KAlarmCancel (struct Alarm *alarm)
{
	CancelTimer (&alarm->timer);
	
	KSetSignals (0, 1 << alarm->signal);
	
	return 0;
}




/*
 *
 */

void KAlarmCallout (struct Timer *timer, void *arg)
{
	struct Alarm *alarm = (struct Alarm *) arg;
		
	alarm->timedout = TRUE;
	KSignal (alarm->pid, alarm->signal);
}




