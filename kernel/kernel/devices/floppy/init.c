#include <kernel/types.h>
#include <kernel/i386/i386.h>
#include <kernel/device.h>
#include <kernel/sync.h>
#include <kernel/proc.h>
#include <kernel/msg.h>
#include <kernel/vm.h>
#include <kernel/timer.h>
#include <kernel/utility.h>
#include <kernel/dbg.h>
#include "floppy.h"




/*
 *
 */

void FloppyTaskInit (void)
{
	floppy_buffer = (uint8 *)floppy_dma_base;
	floppy_buffer_unit = -1;
	floppy_buffer_c = -1;
	
	
	KPRINTF ("FloppyTaskInit()");
		
	if ((floppy_timer_signal = AllocSignal()) != -1)
	{
		if ((floppy_isr_signal = AllocSignal()) != -1)
		{
			if ((floppy_alarm_signal = AllocSignal()) != -1)
			{
				if ((floppy_msgport = CreateMsgPort()) != NULL)
				{
					if ((floppy_isr_handler = ISRHandlerInsert (6, FloppyISRHandler, NULL)) != NULL)
					{
						FloppyInitUnits();
						FloppyReset();
												
						floppy_init_error = 0;
						KSignal (GetPPID(), SIG_INIT);
						return;
					}
							
					DeleteMsgPort (floppy_msgport);
				}
			
				FreeSignal (floppy_alarm_signal);
			}
			
			FreeSignal (floppy_isr_signal);
		}
	
		FreeSignal (floppy_timer_signal);
	}
	
	KPRINTF ("FloppyInit() failed");

	floppy_init_error = -1;	
	KSignal (GetPPID(), SIG_INIT);
	Exit(-1);
}




/*
 *
 */
 
void FloppyTaskFini (void)
{
	int t;
	
	KPRINTF ("FloppyTaskFini()");
	
	CancelTimer (&floppy_timer);
	
	for (t=0; t< MAX_FLOPPY_DRIVES; t++)
	{							
		if (floppy_drive[t].drive_enabled == TRUE)
			StopMotor (t);
	}
	
	ISRHandlerRemove (floppy_isr_handler);
	DeleteMsgPort (floppy_msgport);
	FreeSignal (floppy_alarm_signal);
	FreeSignal (floppy_isr_signal);
	FreeSignal (floppy_timer_signal);
	
	Exit(0);
}




/*
 *
 */

int FloppyInitUnits (void)
{
	uint8 floppy_cmos;
	int t;
	struct MountEnviron *me;
	char mount_name[4];
	uint8 dir;
	uint32 sense_status;

	KPRINTF ("FloppyInitUnits()");

	
	floppy_cmos = CMOS_READ (CMOS_FLOPPY_REG);
	
	switch ((floppy_cmos & 0xf0)>>4)
	{
		case CMOS_FLOPPY_1440:
			floppy_drive[0].density = 0;
			floppy_drive[0].drive_enabled = TRUE;
			break;
		
		case CMOS_FLOPPY_720:
			floppy_drive[0].density = 1;
			floppy_drive[0].drive_enabled = TRUE;
			break;
		
			floppy_drive[0].drive_enabled = FALSE;
	}
	
	switch (floppy_cmos & 0x0f)
	{
		case CMOS_FLOPPY_1440:
			floppy_drive[1].density = 0;
			floppy_drive[1].drive_enabled = TRUE;
			break;
			
		case CMOS_FLOPPY_720:
			floppy_drive[1].density = 1;
			floppy_drive[1].drive_enabled = TRUE;
			break;
			
			floppy_drive[1].drive_enabled = FALSE;
	}
	
	
	floppy_drive[0].unit = 0;
	floppy_drive[1].unit = 1;
	floppy_drive[0].reference_cnt = 0;
	floppy_drive[1].reference_cnt = 0;
	floppy_drive[0].calibration = UNCALIBRATED;
	floppy_drive[1].calibration = UNCALIBRATED;
	floppy_drive[0].disk_state = FLOPPY_NOT_PRESENT;
	floppy_drive[1].disk_state = FLOPPY_NOT_PRESENT;
	floppy_drive[0].motor_off_delay = 0;
	floppy_drive[1].motor_off_delay = 0;
	
	LIST_INIT (&floppy_drive[0].callback_list);
	LIST_INIT (&floppy_drive[1].callback_list);
	
	floppy_pid = GetPID();
	
	InitTimer (&floppy_timer);
	
	FloppyReset();     /* Why does this fail here ?????? */
	
	for (t=0; t<2; t++)
	{
		if (floppy_drive[t].drive_enabled == TRUE)
		{
			floppy_drive[t].diskchange_cnt = 0;
				
			StartMotor(t, FALSE);
			dir = InByte (FDC_DIR);
							
			FloppySeek (t, 0, 0);
			FloppySeek (t, 0, 1);
			dir = InByte (FDC_DIR);
			
			if ((dir & 0x80) == 0)
			{
				KPRINTF ("****** FLOPPY PRESENT at INIT, unit = %d", t);
			
				floppy_drive[t].disk_state = FLOPPY_PRESENT;
								
				if (FloppySenseDrive (t, &sense_status) == 0)
				{
					if (sense_status & ST3_WR_PROTECT)
						floppy_drive[t].write_protect = TRUE;
					else
						floppy_drive[t].write_protect = FALSE;
				}
				else
					floppy_drive[t].write_protect = TRUE;
			}					
			else
			{
				KPRINTF ("****** FLOPPY _NOT_ PRESENT at INIT");
				
				floppy_drive[t].disk_state = FLOPPY_NOT_PRESENT;
				floppy_drive[t].write_protect = TRUE;
			}
			
			StopMotor(t);
		}
	}
	
	
	
		
	
	
	mount_name[0] = 'f';
	mount_name[1] = 'd';
	
	for (t=0; t<2; t++)
	{
		if (floppy_drive[t].drive_enabled == TRUE)
		{			
			if ((me = AllocMountEnviron()) != NULL)
			{
				me->mount_name[0] = 'f';
				me->mount_name[1] = 'd';
				me->mount_name[2] = '0' + (t % 10);
				me->mount_name[3] = '\0';
								
				StrLCpy (me->handler_name, "fat.handler", MOUNTENVIRON_STR_MAX + 1);
				StrLCpy (me->device_name, "floppy.device", MOUNTENVIRON_STR_MAX + 1);
				StrLCpy (me->startup_args, "", MOUNTENVIRON_STR_MAX + 1);
											
				me->handler_unit = 0;
				me->handler_flags = 0;
				
				me->device_unit = t;
				me->device_flags = 0;
				
				me->block_size = 512;
				
				me->partition_start = 0;
				me->partition_end = 0;
				
				me->buffer_cnt = 1;
				me->boot_priority = 1;
				me->baud = 0;
				
				me->removable = TRUE;
				me->writable = TRUE;						
				
				me->writethru_critical = 1;
				me->writeback_delay = 0;
	
				me->max_transfer = 0x10000;

				me->control_flags = 0;
				
				AddBootMountEnviron (me);
			}
		}
	}
		
	return 0;
}





