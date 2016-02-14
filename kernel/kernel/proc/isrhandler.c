#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>




void FPUSwitchState(void);




/*
 * ISRHandlerInsert();
 */

struct ISRHandler *ISRHandlerInsert (int32 irq, int32 (*func) (int32 irq_idx, void *arg), void *arg)
{
	struct ISRHandler *isr;
	
	KPRINTF ("ISRHandlerInsert()");
	
	isr = KMalloc (sizeof (struct ISRHandler));
	
	if (isr != NULL)
	{
		isr->func = func;
		isr->arg = arg;
		isr->irq = irq;
				
		DisableInterrupts();

		LIST_ADD_TAIL (&isr_handler_list[irq], isr, isr_handler_entry);
			
		EnableInterrupts();

		return isr;
	}
	
	return NULL;
}




/*
 * ISRHandlerRemove();
 */

int ISRHandlerRemove (struct ISRHandler *isr)
{
	LIST_REM_ENTRY (&isr_handler_list[isr->irq], isr, isr_handler_entry);
	KFree (isr);
	
	return 0;
}




