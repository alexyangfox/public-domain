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
 * FloppyISRHandler();
 */
 
int32 FloppyISRHandler (int32 isr_idx, void *arg)
{
	floppy_busy = BSY_WAKEN;
	
	KSignal (floppy_pid, floppy_isr_signal);
	return 0;
}




/*
 * WaitForFloppyInterrupt();
 *
 * FIXME: Add timeout
 */

int32 WaitForFloppyInterrupt (void)
{
	uint32 signals;
	
	KAlarmSet (&floppy_alarm, 3, 0, floppy_alarm_signal);

	do
	{
		signals = KWait ((1<<floppy_isr_signal) | (1 << floppy_alarm_signal));

		if (signals & (1 << floppy_alarm_signal))
		{
			KAlarmCancel (&floppy_alarm);
			
			floppy_needs_reset = TRUE;
			return IOERR_TIMEOUT;
		}	
	
	}while (floppy_busy == BSY_IO);


	KAlarmCancel (&floppy_alarm);

	return 0;
}




/*
 * StartMotor();    
 */

void StartMotor (int32 unit, bool wait)
{
	uint8 motor_status;
	bool motor_needs_spinup;
	
	if (floppy_drive[unit].motor_active != TRUE)
		motor_needs_spinup = TRUE;
	else
		motor_needs_spinup = FALSE;
	
		
	floppy_drive[unit].motor_active = TRUE;
	motor_status = 0;
	
	if (floppy_drive[0].motor_active == TRUE)
		motor_status |= 0x10;
		
	if (floppy_drive[1].motor_active == TRUE)
		motor_status |= 0x20;
		
	OutByte (DOR, motor_status | (unit & 0x01) | DOR_DMA | ENABLE_INT);
	
	if (motor_needs_spinup == TRUE && wait == TRUE)
		KSleep (&mtr_start[floppy_drive[unit].density]);
}




/*
 * StopMotor();
 */

void StopMotor (int32 unit)
{
	uint8 motor_status;
	
	
	floppy_drive[unit].motor_active = FALSE;
		
	motor_status = 0;
	
	if (floppy_drive[0].motor_active == TRUE)
		motor_status |= 0x10;

	if (floppy_drive[1].motor_active == TRUE)
		motor_status |= 0x20;

	
	OutByte (DOR, (motor_status | ENABLE_INT));
}




/*
 * FloppySendByte();
 * FIXME: Might want to disable preemption
 */

int32 FloppySendByte (uint8 val)
{
	uint8 status;
	struct TimeVal tv_start, tv_end, tv_timeout, tv_now;

	
	tv_timeout.seconds = 0;
	tv_timeout.microseconds = 500000;
		
	KGetTimeOfDay (&tv_start);
	AddTime (&tv_start, &tv_timeout, &tv_end);

	while (TRUE)
	{
		status = InByte (FDC_STATUS);
		status &= (MASTER | DIRECTION);
		
		if (status == MASTER)
		{
			OutByte (FDC_DATA, val);
			return 0;
		}
		
		KGetTimeOfDay (&tv_now);
		
		if (CompareTime (&tv_end, &tv_now) == 1)
			break;
	} 

	KPRINTF ("***** FloppySendByte() TIMED OUT");
	
	floppy_needs_reset = TRUE;
	return IOERR_TIMEOUT;
}




/*
 * FloppyGetByte();
 * FIXME: Might want to disable preemption
 */

int32 FloppyGetByte (uint8 *val)
{
	uint8 status;
	struct TimeVal tv_start, tv_end, tv_timeout, tv_now;

	
	tv_timeout.seconds = 0;
	tv_timeout.microseconds = 500000;
		
	KGetTimeOfDay (&tv_start);
	AddTime (&tv_start, &tv_timeout, &tv_end);

	while (1)
	{
		status = InByte (FDC_STATUS);
		status &= (MASTER | DIRECTION);
		
		if (status == (MASTER | DIRECTION))
		{
			*val = InByte (FDC_DATA);
			return 0;
		}
		
		KGetTimeOfDay (&tv_now);
		
		if (CompareTime (&tv_end, &tv_now) == 1)
			break;
	} 
	
	KPRINTF ("***** FloppyGetByte() TIMED OUT");
	
	floppy_needs_reset = TRUE;
	return IOERR_TIMEOUT;
}




/*
 * FloppyCommand();
 */

int32 FloppyCommand (uint8 *cmd, int32 nbytes)
{
	int32 t;
	int32 error;
	

	for (t=0; t<nbytes; t++)
	{
		if ((error = FloppySendByte (cmd[t])) != 0)
			return error;
	}

	return 0;
}




/*
 * FloppyGetResult();
 * Could replace with pointer to result buffer, handle null pointer destination.
 */

int32 FloppyGetResult (int32 unit, int32 nbytes)
{
	int32 error;
	int32 t;
	uint8 status;
		
	for (t = 0; t < nbytes; t ++)
	{
		error = FloppyGetByte (&floppy_drive[unit].result[t]);

		if (error != 0)
			return error;
		
		status = InByte(FDC_STATUS);

		if ((status & CTL_BUSY) != CTL_BUSY)
			return 0;
	}
	
	return 0;
}




/*
 * FloppyReset();
 */
 
void FloppyReset (void)
{
	KPRINTF ("FloppyReset()");

	floppy_needs_reset = FALSE;
	floppy_drive[0].calibration = UNCALIBRATED;
	floppy_drive[1].calibration = UNCALIBRATED;

		
	/* Reset */
		
	DisableInterrupts();
	OutByte (DOR, 0);		
	OutByte(DOR, ENABLE_INT);
	EnableInterrupts();
	
	do 
	{
		WaitForFloppyInterrupt();
	} while (floppy_busy == BSY_IO);


	/* Probe FDC to make it return status and flush the results */
	
	FloppySendByte(FDC_SENSE_INT);
	FloppyGetResult(0, 2);
	FloppySendByte(FDC_SENSE_INT);
	FloppyGetResult(0, 2);
	FloppySendByte(FDC_SENSE_INT);
	FloppyGetResult(0, 2);
	FloppySendByte(FDC_SENSE_INT);	
	FloppyGetResult(0, 2);

	/* Set FDC drive parameters. */
	
	FloppySendByte (FDC_SPECIFY);
	FloppySendByte (current_spec1=spec1[floppy_d]);
	FloppySendByte (SPEC2);
}




/*
 * FloppyReadID();
 *
 * Unused.
 */

int32 FloppyReadID (int32 unit, uint32 head)
{
	int error;
	uint8 cmd[2];

	
	if (floppy_drive[unit].calibration == UNCALIBRATED ||
			floppy_drive[unit].motor_active == FALSE)
		return(IOERR_NOTSPECIFIED);


	cmd[0] = FDC_READ_ID;
	cmd[1] = (head << 2) | unit;
	FloppyCommand(cmd, 2);
	
	if (floppy_needs_reset)
		return IOERR_NOTSPECIFIED;
	
	if (WaitForFloppyInterrupt() != 0)
		return IOERR_TIMEOUT;
	
	error = FloppyGetResult(unit, 7);
	
	
	if (error != 0)
		return(error);
		
	if ( (floppy_drive[unit].result[ST1] & BAD_SECTOR) || (floppy_drive[unit].result[ST2] & BAD_CYL) )
		floppy_drive[unit].calibration = UNCALIBRATED;
		
	if ((floppy_drive[unit].result[ST0] & ST0_BITS) != TRANS_ST0)
		return IOERR_NOTSPECIFIED;
	
	if (floppy_drive[unit].result[ST1] | floppy_drive[unit].result[ST2])
		return IOERR_NOTSPECIFIED;

	return 0;
}




/*
 * FloppySeek();
 *
 * Called when we need to change to the correct cylinder,  motor
 * will already have been started,  we never call this seperately
 * from a read/write command.  Will call it for CMD_MEDIA_PRESENT test.
 */

int32 FloppySeek (int32 unit, int32 head, int32 cylinder)
{
	int error;
	uint8 cmd[3];
	
	
	if (floppy_drive[unit].calibration == UNCALIBRATED)
		if (FloppyRecalibrate(unit) != 0)
			return IOERR_SEEK;
	
	if (floppy_drive[unit].curcyl == cylinder)
		return 0;
		
	cmd[0] = FDC_SEEK;
	cmd[1] = (head << 2) | unit;
	cmd[2] = cylinder;
	
	if (FloppyCommand(cmd, 3) != 0)
		return IOERR_SEEK;

	if (WaitForFloppyInterrupt() != 0)
		return IOERR_TIMEOUT;
	
	FloppySendByte(FDC_SENSE_INT);
	error = FloppyGetResult(unit, 2);
		
	if (error != 0
			|| (floppy_drive[unit].result[ST0] & ST0_BITS) != SEEK_ST0
			|| floppy_drive[unit].result[ST1] != cylinder)
		return IOERR_SEEK;

	floppy_drive[unit].curcyl = cylinder;

	/* Would let head settle here if we were doing a Format command */
	/* Why not any other command? */
	
	KSleep2 (0, 15000);
	
	return 0;
}




/*
 * FloppyRecalibrate();
 */

int32 FloppyRecalibrate (int32 unit)
{
	int32 error;
	uint8 cmd[2];


	KPRINTF ("FloppyRecalibrate()");
	
	StartMotor (unit, TRUE);
	
	cmd[0] = FDC_RECALIBRATE;
	cmd[1] = unit;
	
	error = FloppyCommand (cmd, 2);
	
	if (error != 0)
		return IOERR_NOTSPECIFIED;
	
	if (floppy_needs_reset == TRUE)
		return IOERR_NOTSPECIFIED;
	
	if (WaitForFloppyInterrupt() != 0)
		return IOERR_TIMEOUT;
	

	floppy_drive[unit].curcyl = -1;
	
	
	FloppySendByte (FDC_SENSE_INT);
	error = FloppyGetResult(unit, 2);
			
	if (error != 0
		|| (floppy_drive[unit].result[ST0] & ST0_BITS) != SEEK_ST0
		|| floppy_drive[unit].result[ST_PCN] !=0)
	{
		floppy_needs_reset = TRUE;
		floppy_drive[unit].calibration = UNCALIBRATED;
		return IOERR_NOTSPECIFIED;
	}
	else
	{
		floppy_drive[unit].calibration = CALIBRATED;
		floppy_drive[unit].curcyl = floppy_drive[unit].result[ST_PCN];
		return 0;
	}
}




/*
 * FloppyReadyDMA();
 */

void FloppyReadyDMA(int mode, int32 unit, int sector_cnt, void *buf)
{
	uint8 lo_byte, mid_byte, hi_byte;
	uint32 dma_buf;
	
	
	dma_buf = (uint32)buf;

	lo_byte = (uint8)(dma_buf & 0xff);
	mid_byte = (uint8)((dma_buf >> 8) & 0xff);
	hi_byte = (uint8)((dma_buf >> 16) & 0xff);
    
    DisableInterrupts();

    OutByte (DMA_FLIPFLOP, 0);
    OutByte (DMA_MODE, mode);
    OutByte (DMA_ADDR, lo_byte);
    OutByte (DMA_ADDR, mid_byte);
    OutByte (DMA_TOP, hi_byte);
    
	OutByte (DMA_COUNT, ((512 * sector_cnt) - 1) & 0xff);
    OutByte (DMA_COUNT, (((512 * sector_cnt) - 1) >> 8) & 0xff);
    OutByte (DMA_INIT, 2);

	
}




/*
 * FloppyTransfer();
 */
 
int32 FloppyTransfer (int32 opcode, int32 unit, int32 cylinder, int32 head,
							int32 start_sector, int32 sector_cnt)
{
	int32 error;
	uint8 cmd[9];
	

	if (floppy_drive[unit].calibration == UNCALIBRATED)
		return IOERR_NOTSPECIFIED;
	
	if (floppy_drive[unit].motor_active == FALSE)
		return IOERR_NOTSPECIFIED;
	
	cmd[0] = opcode;
	cmd[1] = (head << 2) | unit;
	cmd[2] = cylinder;
	cmd[3] = head;
	cmd[4] = start_sector;
	cmd[5] = 2;
	cmd[6] = sector_cnt;
	cmd[7] = gap[floppy_d];
	cmd[8] = DTL;

	FloppyCommand (cmd, 9);

	if (floppy_needs_reset)
		return IOERR_NOTSPECIFIED;
	
	if ((error = WaitForFloppyInterrupt()) != 0)
		return IOERR_TIMEOUT;
	
	if ((error = FloppyGetResult (unit, 7)) != 0)
		return error;
		
	if ( (floppy_drive[unit].result[ST1] & BAD_SECTOR) || (floppy_drive[unit].result[ST2] & BAD_CYL) )
	{
		floppy_drive[unit].calibration = UNCALIBRATED;
		return IOERR_NOTSPECIFIED;
	}

	if (floppy_drive[unit].result[ST1] & WRITE_PROTECT)
		return IOERR_WRITE_PROTECT;
	
	if ((floppy_drive[unit].result[ST0] & ST0_BITS) != TRANS_ST0)
		return IOERR_NOTSPECIFIED;
	
		
	if (floppy_drive[unit].result[ST1] | floppy_drive[unit].result[ST2])
		return IOERR_NOTSPECIFIED;

	return 0;
}










/*
 * FloppySenseDrive();
 *
 * Called when we need to change to the correct cylinder,  motor
 * will already have been started,  we never call this seperately
 * from a read/write command.  Will call it for CMD_MEDIA_PRESENT test.
 */

int32 FloppySenseDrive (int32 unit, uint32 *status)
{
	int error;
	uint8 cmd[2];
	
	cmd[0] = FDC_SENSE_DRV;
	cmd[1] = unit;
	
	if (FloppyCommand(cmd, 2) != 0)
		return IOERR_SEEK;

	error = FloppyGetResult(unit, 1);
	
	if (error == 0)
		*status = floppy_drive[unit].result[0];
	
	return error;
}










