#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include <kernel/timer.h>
#include <kernel/i386/i386.h>




/*
 * DispatchISR();
 *
 * Interrupt handlers may only alter variables and KSignal() processes to
 * awaken them.  They may NOT create or disable timers due to the possibility
 * of an interrupt occuring during soft clock processing which may be traversing
 * a list of timers.
 */
 
void DispatchISR (uint32 isr_idx)
{
	struct ISRHandler *isr_handler;
	int32 status;

	
	if (isr_idx == ISR_TIMER_IDX)
	{
		HardClockProcessing();
	}
	
	EnableInterrupts();
		
	isr_handler = LIST_HEAD(&isr_handler_list[isr_idx]);
	status = -1;
		
	while (status == -1 && isr_handler != NULL)
	{
		status = isr_handler->func (isr_idx, isr_handler->arg);
		isr_handler = LIST_NEXT(isr_handler, isr_handler_entry);
	}
	
	DisableInterrupts();
}

