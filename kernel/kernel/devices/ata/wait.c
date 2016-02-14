#include <kernel/types.h>
#include <kernel/i386/i386.h>
#include <kernel/device.h>
#include <kernel/sync.h>
#include <kernel/proc.h>
#include <kernel/msg.h>
#include <kernel/vm.h>
#include <kernel/timer.h>
#include <kernel/dbg.h>
#include "ata.h"





/*
 * Busy Loop wait routine,  where is it used?
 * Only before a command?
 *
 * Check for timeout here ? 
 */

int AtaWaitFor (struct Ata *ata, uint8 mask, uint8 value)
{
	DELAY400NS;
		
	while (1)
	{
		ata_stored_status = InByte (ata->base + REG_STATUS);
		
		if ((ata_stored_status & mask) == value)
			return 0;
		
		/*  FIXME: Removed ATA interrupt timeout 
		if (KAlarmCheck (&ata_alarm) != 0)
		{
			ata_needs_reset = TRUE;
			return -1;
		}
		*/
	}
}




/*
 * Interrupt wait routine
 */
 
int AtaIntrWait (struct Ata *ata)
{
	int rc;
	
	
	do
	{
		KWait ((1 << ata->isr_signal));
	} while ((ata_stored_status = InByte (ata->base + REG_STATUS)) & STATUS_BSY);
	
		
	if ((ata_stored_status & (STATUS_BSY | STATUS_WF | STATUS_ERR)) == 0)
		rc = 0;
	else
		rc = -1;		/* any other error */
	
	return rc;
}




/*
 * AtaISRHandler();
 *
 * FIXME: How do we determine source of interrupt if the PCI interrupt line is shared between
 * different devices?
 * Also how is the level-triggered interrupt line of a PCI ATA controller cleared?
 */
 
int32 AtaISRHandler (int32 isr_idx, void *arg)
{
	if (isr_idx == 14)
	{
		ata_stored_status = InByte (REG_BASE0 + REG_STATUS);
		ata_isr14_cnt ++;
		KSignal (ata_pid, ata_isr14_signal);
	}
	else if (isr_idx == 15)
	{
		ata_stored_status = InByte (REG_BASE1 + REG_STATUS);
		ata_isr15_cnt ++;
		KSignal (ata_pid, ata_isr15_signal);
	}
	return 0;
}







/*
 * AtaAtapiDelay();
 * Delay for 110ms
 */

void AtaAtapiDelay (struct Ata *ata)
{
 	struct TimeVal tv;
 	
 	/*
	if (ata->config != ATA_CONFIG_ATAPI)
		return;
	*/
	
	tv.seconds = 0;
	tv.microseconds = 110000;
	KSleep(&tv);
}


