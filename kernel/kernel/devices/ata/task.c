#include <kernel/types.h>
#include <kernel/i386/i386.h>
#include <kernel/device.h>
#include <kernel/sync.h>
#include <kernel/proc.h>
#include <kernel/msg.h>
#include <kernel/vm.h>
#include <kernel/timer.h>
#include <kernel/dbg.h>
#include <kernel/error.h>
#include "ata.h"




/*
 * AtaTask()
 *
 * Main function of ATA driver.
 *
 * FIXME:  Timer structure has timer_expired flag, so sort of redundant in Floppy structure ????????
 */

int32 AtaTask (void *arg)
{	
	struct BlkReq *blkreq;
	struct Msg *msg;
	uint32 signals;
	int t;
	
	AtaTaskInit();

	SetTimer (&ata_timer, TIMER_TYPE_RELATIVE,
		&ata_timer_tv, &AtaTimerCallout, NULL);
		
	while (1)
	{
		signals = KWait (1 << ata_msgport->signal | SIGF_TERM | 1 << ata_timer_signal);
		
		SetError (0);
		
		if (signals & (1 << ata_msgport->signal))
		{
			while ((msg = GetMsg (ata_msgport)) != NULL)
			{
				blkreq = (struct BlkReq *)msg;
				
				switch (blkreq->cmd)
				{	
					case BLK_CMD_READ:
						DoAtaRead (blkreq);
						break;

					case BLK_CMD_WRITE:
						DoAtaWrite (blkreq);
						break;
					
					case BLK_CMD_MEDIA_PRESENT:
						DoAtaMediaPresent (blkreq);
						break;

					case BLK_CMD_ADD_CALLBACK:
						DoAtaAddCallback (blkreq);
						break;
					
					case BLK_CMD_REM_CALLBACK:
						DoAtaRemCallback (blkreq);
						break;

					case BLK_CMD_SCSI:
						DoAtaSCSICommand (blkreq);
						break;

					default:
						blkreq->error = IOERR_NOCMD;
						blkreq->rc = -1;
				}
				
				ReplyMsg (msg);
			}
		}
		
		
		if (signals & (1 << ata_timer_signal))
		{
			for (t=0; t< MAX_ATA_DRIVES; t++)
			{							
				if (ata_drive[t].enabled == TRUE && ata_drive[t].config == ATA_CONFIG_ATAPI)
				{
					/* FIXME:  FOr this and the floppy drive.  We need to detect THE CHANGE,
						not just if present or not, could change from PRESENT TO PRESENT */
				
				
					if (ata_drive[t].disk_state == ATA_MEDIA_PRESENT)
					{
						if (AtapiMediaPresent(&ata_drive[t]) == ATA_MEDIA_NOT_PRESENT)
						{
							KPRINTF ("************ REMOVAL DETECTED");
						
							ata_drive[t].disk_state = ATA_MEDIA_NOT_PRESENT;
							ata_drive[t].diskchange_cnt ++;
							
							AtaCallbacks(t);
						}
						
						
					}
					else if (ata_drive[t].disk_state == ATA_MEDIA_NOT_PRESENT)
					{
						if (AtapiMediaPresent(&ata_drive[t]) == ATA_MEDIA_PRESENT)
						{
							KPRINTF ("**** INSERTION DETECTED");
						
							ata_drive[t].disk_state = ATA_MEDIA_PRESENT;
							ata_drive[t].diskchange_cnt ++;

							AtaCallbacks(t);
						}
					}				
				}
			}

			SetTimer (&ata_timer, TIMER_TYPE_RELATIVE,
						&ata_timer_tv, &AtaTimerCallout, NULL);
		}

		
		
		
		if (signals & SIGF_TERM)
		{
			AtaTaskFini();
		}
	}
}



/*
 *
 */

void AtaCallbacks (int unit)
{
	struct Callback *callback;
	
	callback = LIST_HEAD (&ata_drive[unit].callback_list);
	
	while (callback != NULL)
	{
		callback->callback(callback->arg);
	
		callback = LIST_NEXT (callback, callback_entry);
	}
}




/*
 *
 */

void AtaTimerCallout (struct Timer *timer, void *arg)
{
	KSignal (ata_pid, ata_timer_signal);
}





/*
 * ata_drive[0].partition_table[0].start_lba + 
 */

void DoAtaRead (struct BlkReq *blkreq)
{
	int rc;
	uint8 *buf;
	int32 sector;
	int32 sectors_remaining;
	int32 sectors_to_transfer;
	struct Ata *ata;
	
	
	ata = blkreq->unitp;
	sector = blkreq->sector;
	sectors_remaining = blkreq->nsectors;
	buf = blkreq->buf;
	rc = 0;
	
	while (rc == 0 && sectors_remaining > 0)
	{
		sectors_to_transfer = (sectors_remaining < ATA_BUFFER_SZ / ata->sector_sz) ?
								sectors_remaining : ATA_BUFFER_SZ / ata->sector_sz;

		if (ata->config == ATA_CONFIG_ATA)
		{
			rc = AtaReadSectors (blkreq->unitp, sectors_to_transfer,
												sector, buf, blkreq->as);
		}
		else if (ata->config == ATA_CONFIG_ATAPI)
		{
			rc = AtapiReadSectors (blkreq->unitp, sectors_to_transfer,
												sector, buf, blkreq->as);
		}
		else
		{
			SetError (EIO);
			rc = -1;
		}
		
		
		sectors_remaining -= sectors_to_transfer;
		sector += sectors_to_transfer;
		buf += sectors_to_transfer * ata->sector_sz;
	
	}
	
	blkreq->error = GetError();
	blkreq->rc = rc;
}





/*
 * ata_drive[0].partition_table[0].start_lba + 
 */

void DoAtaWrite (struct BlkReq *blkreq)
{
	int rc;
	uint8 *buf;
	int32 sector;
	int32 sectors_remaining;
	int32 sectors_to_transfer;
	struct Ata *ata;
	
	
	ata = blkreq->unitp;	
	sector = blkreq->sector;
	sectors_remaining = blkreq->nsectors;
	buf = blkreq->buf;
	rc = 0;
	
	while (rc == 0 && sectors_remaining > 0)
	{
		sectors_to_transfer = (sectors_remaining < ATA_BUFFER_SZ / ata->sector_sz) ?
								sectors_remaining : ATA_BUFFER_SZ / ata->sector_sz;


		if (ata->config == ATA_CONFIG_ATA)
		{
			rc = AtaWriteSectors (blkreq->unitp, sectors_to_transfer,
												sector, buf, blkreq->as);

		}
		else if (ata->config == ATA_CONFIG_ATAPI)
		{
			rc = AtapiWriteSectors (blkreq->unitp, sectors_to_transfer,
												sector, buf, blkreq->as);
		}
		else
		{
			SetError (EIO);
			rc = -1;
		}
											

		sectors_remaining -= sectors_to_transfer;
		sector += sectors_to_transfer;
		buf += sectors_to_transfer * ata->sector_sz;
	}

	
	blkreq->error = GetError();
	blkreq->rc = rc;
}




/*
 * Maybe alter FS code to avoid calling this.
 */

void DoAtaMediaPresent (struct BlkReq *blkreq)
{
	struct BlockDeviceStat *stat;
	struct Ata *ata;

	KPRINTF ("DoAtaMediaPresent()");

	stat = blkreq->stat;
	ata = blkreq->unitp;
	
	
	if (ata->config == ATA_CONFIG_ATA)
	{
		stat->total_sectors = ata->size;
		stat->media_state = MEDIA_INSERTED;	
		stat->write_protect = FALSE;
		blkreq->error = 0;
		blkreq->rc = 0;
	}
	else if (ata->config == ATA_CONFIG_ATAPI)
	{
		if (AtapiMediaPresent (ata) == ATA_MEDIA_PRESENT)
		{
			KPRINTF ("MEDIA_PRESENT");
			
			stat->total_sectors = 0;
			stat->media_state = MEDIA_INSERTED;
			stat->write_protect = TRUE;
		}
		else
		{
			KPRINTF ("MEDIA_REMOVED");
		
			stat->total_sectors = 0;
			stat->media_state = MEDIA_REMOVED;
			stat->write_protect = TRUE;
		}
		
		blkreq->error = 0;		
		blkreq->rc = 0;
	}
	else
	{
		blkreq->error = EIO;
		blkreq->rc = -1;
	}
}






/*
 *
 */

void DoAtaAddCallback (struct BlkReq *blkreq)
{
	struct Ata *ata;
	
	ata = blkreq->unitp;
	LIST_ADD_TAIL (&ata->callback_list, blkreq->callback, callback_entry);
	blkreq->error = 0;
	blkreq->rc = 0;
}




/*
 *
 */

void DoAtaRemCallback (struct BlkReq *blkreq)
{
	struct Ata *ata;
	
	ata = blkreq->unitp;
	LIST_REM_ENTRY (&ata->callback_list, blkreq->callback, callback_entry);

	blkreq->error = 0;
	blkreq->rc = 0;
}




/*
 *
 */

void DoAtaSCSICommand (struct BlkReq *blkreq)
{
	struct Ata *ata;
	
	
	ata = blkreq->unitp;
			
	if (ata->config == ATA_CONFIG_ATAPI)
	{
		blkreq->rc = AtapiPacket (ata, blkreq->cmd_packet_addr, blkreq->cmd_packet_nbytes,
					blkreq->data_buf_addr, blkreq->data_nbytes, blkreq->dir, blkreq->as);

		blkreq->error = GetError();
	}
	else
	{
		blkreq->error = EIO;
		blkreq->rc = -1;
	}
}

