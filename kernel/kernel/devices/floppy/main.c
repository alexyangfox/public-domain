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
 * Timer structure has timer_expired flag, so sort of redundant in Floppy 
 */

int32 FloppyTask (void *arg)
{	
	struct BlkReq *blkreq;
	struct Msg *msg;
	uint32 signals;
	uint8 dir;
	uint32 sense_status;
	int t;

		
	FloppyTaskInit();
	
	
	SetTimer (&floppy_timer, TIMER_TYPE_RELATIVE,
				&floppy_motor_off_tv, &FloppyTimerCallout, NULL);
	
	while (1)
	{
		signals = KWait ((1 << floppy_msgport->signal) | (1 << floppy_timer_signal) | SIGF_TERM);
		
		SetError (0);
		
		if (signals & (1 << floppy_msgport->signal))
		{
			while ((msg = GetMsg (floppy_msgport)) != NULL)
			{
				blkreq = (struct BlkReq *)msg;
				
				switch (blkreq->cmd)
				{						
					case BLK_CMD_READ:
						FloppyCmdReadWrite (blkreq);
						break;

					case BLK_CMD_WRITE:
						FloppyCmdReadWrite (blkreq);
						break;

					case BLK_CMD_MEDIA_PRESENT:
						FloppyCmdMediaPresent (blkreq);
						break;
						
					case BLK_CMD_ADD_CALLBACK:
						FloppyCmdAddCallback (blkreq);
						break;
					
					case BLK_CMD_REM_CALLBACK:
						FloppyCmdRemCallback (blkreq);
						break;

					default:
						blkreq->error = IOERR_NOCMD;
						blkreq->rc = -1;
				}
				
				ReplyMsg (msg);
			}
		}
		
		
		if (signals & (1 << floppy_timer_signal))
		{
			/* Maybe when timer fires,  check motor of each drive
				if active.  decrement counters of active drives, if zero
				then turn motor off */
			
			if (floppy_needs_reset)
				FloppyReset();
			
			for (t=0; t< MAX_FLOPPY_DRIVES; t++)
			{							
				if (floppy_drive[t].drive_enabled == TRUE)
				{
					if (floppy_drive[t].disk_state == FLOPPY_PRESENT)
					{
				/*		KPRINTF ("Testing removal"); */
				
						StartMotor(t, FALSE);
						dir = InByte (FDC_DIR);
						
						if (floppy_drive[t].motor_off_delay != 0)
							floppy_drive[t].motor_off_delay --;
						else
							StopMotor (t);
						
						if (dir & 0x80)
						{
							KPRINTF ("************ REMOVAL DETECTED");
						
							floppy_drive[t].disk_state = FLOPPY_NOT_PRESENT;
							floppy_drive[t].diskchange_cnt ++;
							
							if (t == floppy_buffer_unit)
								floppy_buffer_unit = -1;
							
							FloppyCallbacks(t);
						}
					}
					else if (floppy_drive[t].disk_state == FLOPPY_NOT_PRESENT)
					{
/*						KPRINTF ("Testing insertion"); */
						
						StartMotor(t, FALSE);
						FloppySeek (t, 0, 0);
						FloppySeek (t, 0, 1);
						dir = InByte (FDC_DIR);
						
						if (FloppySenseDrive (t, &sense_status) == 0)
						{
							if (sense_status & ST3_WR_PROTECT)
								floppy_drive[t].write_protect = TRUE;
							else
								floppy_drive[t].write_protect = FALSE;
						}
						
						StopMotor (t);
						
						if ((dir & 0x80) == 0)
						{
							KPRINTF ("**** INSERTION DETECTED");
						
							floppy_drive[t].disk_state = FLOPPY_PRESENT;
							floppy_drive[t].diskchange_cnt ++;

							FloppyCallbacks(t);
						}
					}				
				}
			}

			SetTimer (&floppy_timer, TIMER_TYPE_RELATIVE,
						&floppy_motor_off_tv, &FloppyTimerCallout, NULL);
		}

		if (signals & SIGF_TERM)
		{
			FloppyTaskFini();
		}
	}
}




/*
 *
 */

void FloppyCallbacks (int unit)
{
	struct Callback *callback;
	
	callback = LIST_HEAD (&floppy_drive[unit].callback_list);
	
	while (callback != NULL)
	{
		callback->callback(callback->arg);
	
		callback = LIST_NEXT (callback, callback_entry);
	}
}




/*
 */

void FloppyTimerCallout (struct Timer *timer, void *arg)
{
	KSignal (floppy_pid, floppy_timer_signal);
}




/*
 * FloppyDoReadWrite()
 *
 * ** Need to multi-sector reads/writes, -1 for invalid buffer and
 * double-sided track reading, ie. 36-sector tracks
 *
 * Need some sort of wrapper to handle the multisector reads writes
 * so they can span the tracks.  Also ensure no read/write past last
 * sector.
 */

void FloppyCmdReadWrite (struct BlkReq *blkreq)
{
	int unit;
	uint32 c, h, s;
	uint32 heads;
	uint32 sectors_per_track;
	int sectors_remaining;
	int sectors_remaining_in_track;
	int sectors_to_transfer;
	int sector;
	struct Floppy *fp;
	uint8 *buf;
	int rc;
	
		
	fp = blkreq->unitp;
	unit = fp->unit;
		
	if (floppy_drive[unit].error != 0 || floppy_drive[unit].disk_state == FLOPPY_NOT_PRESENT)
	{
		blkreq->error = IOERR_NO_MEDIA;
		blkreq->rc = -1;
		return;
	}
	
	
	heads = 2;
	sectors_per_track = nr_sectors[floppy_drive[unit].density];
	sector = blkreq->sector;
	sectors_remaining = blkreq->nsectors;
	buf = blkreq->buf;
	rc = 0;
		
	
	while (rc == 0 && sectors_remaining > 0)
	{
		c = sector / (heads * sectors_per_track);
		h = (sector % (heads * sectors_per_track)) / sectors_per_track;
		s = (sector % (heads * sectors_per_track)) % sectors_per_track;
		
		sectors_remaining_in_track = sectors_per_track - s;
		
		sectors_to_transfer = (sectors_remaining < sectors_remaining_in_track) ?
								sectors_remaining : sectors_remaining_in_track;
		
		rc = FloppyDoReadWrite(blkreq->cmd, blkreq->as, buf,
										sectors_to_transfer, unit, c, h, s);
		
		sectors_remaining -= sectors_to_transfer;
		sector += sectors_to_transfer;
		buf += sectors_to_transfer * 512;
	}


	blkreq->error = GetError();
	
	if (rc == 0)
	{
		floppy_drive[unit].motor_off_delay = 1;
		blkreq->rc = 0;
	}
	else
		blkreq->rc = -1;
}


	

/*
 *
 */
	
int FloppyDoReadWrite (int cmd, struct AddressSpace *as, void *buf,
							int sector_cnt, int unit, int c, int h, int s)
{
	uint8 dir;
	uint8 *transfer_addr;
	uint32 transfer_start_sector;
	uint32 transfer_sector_cnt;
	uint8 *src, *dst;
	int32 error_cnt;
	uint8 cmdbuf[3];
	int32 error;
	int32 dma_mode;
	int32 opcode;
	uint32 heads;
	uint32 sectors_per_track;
	
	
		
	if (cmd == BLK_CMD_READ)
	{
		if (unit == floppy_buffer_unit && c == floppy_buffer_c && h == floppy_buffer_h)
		{
			src = floppy_buffer + (s * 512);
			dst = buf;
			
			CopyOut (as, dst, src, sector_cnt * 512);
			return 0;
		}
		else
		{
			transfer_addr = floppy_buffer;
			transfer_start_sector = 1;
			transfer_sector_cnt = nr_sectors[floppy_drive[unit].density]; /* FIXME: Read both sides */
			
			dma_mode = DMA_READ;
			opcode = FDC_READ;
		}
	}
	else
	{
		src = buf;
		dst = floppy_buffer + (s * 512);
		
		CopyIn (as, dst, src, sector_cnt * 512);
		
		if (floppy_buffer_unit != unit || floppy_buffer_c != c || floppy_buffer_h != h)
			floppy_buffer_unit = -1;
						
		transfer_addr = dst;
		transfer_start_sector = s+1;  /* FIXME: s+1 ??????? or 1 (if read whole track) */
		transfer_sector_cnt = sector_cnt;
		
		dma_mode = DMA_WRITE;
		opcode = FDC_WRITE;
	}
	
		
	floppy_d = floppy_drive[unit].density;
	
	OutByte (FDC_RATE, rate[floppy_d]);
	FloppyReadyDMA (dma_mode, unit, transfer_sector_cnt, transfer_addr);
	StartMotor (unit, TRUE);
		
	dir = InByte (FDC_DIR);
						
	if (dir & 0x80)
	{
		SetError (IOERR_NO_MEDIA);
		return -1;
	}
		
	for (error_cnt = 5; error_cnt > 0; error_cnt--)
	{
		if (floppy_needs_reset)
			FloppyReset();
					
		if (current_spec1 != spec1[floppy_d])
		{
			cmdbuf[0] = FDC_SPECIFY;
			cmdbuf[1] = current_spec1 = spec1[floppy_d];
			cmdbuf[2] = SPEC2;
			
			error = FloppyCommand (cmdbuf, 3);
			
			if (error != 0)
				continue;
		}
		
		error = FloppySeek (unit, h, c);
		
		if (error == 0)
			error = FloppyTransfer (opcode, unit, c, h,
							transfer_start_sector, transfer_sector_cnt);
		
		if (error == 0)
			break;	/* if successful, exit loop */
		
		if (error == IOERR_WRITE_PROTECT)
			break;	/* retries won't help */
	}



	if (error == 0)
	{
		if (cmd == BLK_CMD_READ)
		{
			floppy_buffer_unit = unit;
			floppy_buffer_c = c;
			floppy_buffer_h = h;
			
			src = floppy_buffer + (s * 512);
			dst = buf;
							
			CopyOut (as, dst, src, sector_cnt * 512);
		}

		return 0;
	}
	else
	{
		floppy_buffer_unit = -1;
		floppy_drive[unit].error = -1;

		return -1;

	}
}




/*
 * FloppyCmdMediaPresent();
 */

void FloppyCmdMediaPresent (struct BlkReq *blkreq)
{
	struct BlockDeviceStat *stat;
	int unit;
	struct Floppy *fp;
	
	fp = blkreq->unitp;
	stat = blkreq->stat;
	unit = fp->unit;
	
	stat->media_state = floppy_drive[unit].disk_state;
	stat->diskchange_cnt = floppy_drive[unit].diskchange_cnt;
	stat->total_sectors = nr_blocks[floppy_drive[unit].density];
	stat->write_protect = floppy_drive[unit].write_protect;
	
	blkreq->error = 0;
	blkreq->rc = 0;
}




/*
 *
 */

void FloppyCmdAddCallback (struct BlkReq *blkreq)
{
	struct Floppy *fd;
	
	fd = blkreq->unitp;
	LIST_ADD_TAIL (&fd->callback_list, blkreq->callback, callback_entry);
	blkreq->error = 0;
	blkreq->rc = 0;
}




/*
 *
 */

void FloppyCmdRemCallback (struct BlkReq *blkreq)
{
	struct Floppy *fd;
	
	fd = blkreq->unitp;
	LIST_REM_ENTRY (&fd->callback_list, blkreq->callback, callback_entry);

	blkreq->error = 0;
	blkreq->rc = 0;
}



