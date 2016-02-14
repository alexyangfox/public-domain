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


void AtapiTester (int n);



/*
 * WTF is a sense_key ???????????   Also need CHECK_CONDITION error status of Packet command
 *
 * P54 for ATAPI register map,  not sure what/where it is?
 *
 * CHECK is bit in ATA status register.
 * SENSE KEY is 4 bits in ATA error register
 * Sense Keys in Table 140, p183
 */
 
int AtapiMediaPresent (struct Ata *ata)
{
	uint8 cmd[12] = {0,0,0,0,  0,0,0,0,  0,0,0,0};
	uint8 sense;
		
	
	if (AtapiPacket (ata, cmd, 12, NULL, 0, -1, &kernel_as) != 0)
	{
		sense = InByte (ata->base + REG_ERROR) >> 4;
		return ATA_MEDIA_NOT_PRESENT;
	}
	else
	{
		return ATA_MEDIA_PRESENT;
	}
}




/*
 * Really should use 512-byte sector reads.
 * ATAPIPacket should read into kernel buffer max 64k,  then copy 512-byte sectors to kernel/user space.
 */

int AtapiReadSectors (struct Ata *ata, int sector_cnt, uint32 block, void *buf, struct AddressSpace *as)
{
	int rc;
	uint8 cmd[12];
	uint8 sense;
	int t;
	
	
	/* *** Wrapper needs to handle 512-byte/ 2048-byte sectors */
	
	/* Need to round up sector count then divide by CD_ROM_SECTOR_SZ/512 */
	/* Similar for starting block EXCEPT Need to be careful with odd starting offsets */
	
	/* Maybe we should avoid cache and read direct from disc */
	/* Maybe remove Cache driver????????? */
	
	/* Need to return device sector_size */
		
	cmd[0] = 0x28;
	cmd[1] = 0;
	cmd[2] = (block & 0xff000000) >> 24;
	cmd[3] = (block & 0x00ff0000) >> 16;
	cmd[4] = (block & 0x0000ff00) >> 8;
	cmd[5] = (block & 0x000000ff);
	cmd[6] = 0;	
	cmd[7] = (sector_cnt & 0xff00) >> 8;
	cmd[8] = (sector_cnt & 0x00ff);
	cmd[9] = 0;	
	cmd[10] = 0;	
	cmd[11] = 0;	
	
	for (t=0; t<MAX_ATA_TRIES; t++)
	{
		if (AtapiPacket (ata, cmd, 12, buf, sector_cnt * 2048, ATAPI_DIR_READ, as) == 0)
		{	
			return 0;
		}
	}
	
	
	
	KPRINTF ("AtapiReadSectors() FAIL");
	return -1;
}




/*
 *
 */

int AtapiWriteSectors (struct Ata *ata, int sector_cnt, uint32 block, void *buf, struct AddressSpace *as)
{
	return -1;
}




/*
 *
 */

int AtapiIdentify (struct Ata *ata, void *buf)
{	
	int t;
	struct command cmd;
	int rc;

	
	KPRINTF ("AtapiIdentify()");
		
	cmd.precomp =0;
	cmd.count = 1;
	cmd.sector = 0;
	cmd.cyl_lo = 0;
	cmd.cyl_hi = 0;
	cmd.ldh     = ata->ldhpref & 0x10;
	cmd.command = ATA_CMD_IDENTIFY_PACKET_DEVICE;
				
	if (AtaPIODataIn (ata, &cmd, buf, 1) == 0)
		rc = 0;
	else
		rc = -1;
	
	return rc;
}




/*
 *
 */

int AtapiPacket (struct Ata *ata, uint8 *cmd_packet_addr, int cmd_packet_nbytes,
					void *data_buf_addr, int data_nbytes, int dir, struct AddressSpace *as)
{
	uint32 transfer_nbytes, transfer_nwords;
	int error = 0;
	uint8 status;
		
	
	KSetSignals (1 << ata->isr_signal, 0);
	
	if (cmd_packet_nbytes != 12 && cmd_packet_nbytes != 16)
	{
		SetError (EIO);
		return -1;
	}
	
	/* Select Drive */
	
 	if (AtaWaitFor (ata, STATUS_BSY | STATUS_DRQ, 0) != 0)  /* Added DRQ */
		return -1;
		
	
 	OutByte (ata->base + REG_LDH, ata->ldhpref);
	DELAY400NS;
	
	if (AtaWaitFor (ata, STATUS_BSY | STATUS_DRQ, 0) != 0)
		return -1;
	
	/* Set up all registers except COMMAND register */
		
	OutByte (ata->base + REG_CTL, 0);
	OutByte (ata->base + REG_PRECOMP, 0);
	OutByte (ata->base + REG_COUNT,   0);
	OutByte (ata->base + REG_SECTOR,  0);
	OutByte (ata->base + REG_CYL_LO,  data_nbytes & 0xff);
	OutByte (ata->base + REG_CYL_HI,  (data_nbytes >> 8) & 0xff);
	
	/* Set up COMMAND register */
	
	OutByte (ata->base + REG_COMMAND, ATA_CMD_PACKET);
	DELAY400NS;
	
	
	/* Transfer command packet */
	/* Poll Alternate Status for BSY=0. */

	while (1)
	{
		status = InByte (ata->base + REG_ASTAT);      /* poll for not busy */
	
		if ((status & STATUS_BSY) == 0)
			break;
	}
		
	if (error == 0)
	{
		/* Read the primary status register and the other ATAPI registers. */
		
		status = InByte (ata->base + REG_STATUS);
		
		/* check status: must have BSY=0, DRQ=1 now */
		
		if ((status & (STATUS_BSY | STATUS_DRQ | STATUS_ERR )) != STATUS_DRQ)
		{
			error = 52;
			dir = -1;   /* command done */
		}
		else
		{
			/* Transfer the command packet */
			
			pio_outsw (ata->base + REG_DATA, cmd_packet_addr, cmd_packet_nbytes);
			DELAY400NS;    /* delay so device can get the status updated */
		}
	}
   
   
       
   	/* Data transfer loop */

	while (error == 0)
	{
		/* Wait for interrupt -or- wait for not BUSY -or- wait for time out. */

		error = AtaIntrWait(ata);
		
		if (error != 0)
		{
			dir = -1;   /* command done */
			break;
		}


		/* Using interrupts so get the status read by the interrupt handler. */
		/* Could a second interrupt be occurring that changes ata_stored_status ? */
		
		status = ata_stored_status;
		

 		/* FIXME: Moved error test after ata_stored_status to test above code.
 			If there was a time out error, exit the data transfer loop. */
	
		/* Exit the read data loop if the device indicates this is the
			end of the command. */
		
		if ((status & (STATUS_BSY | STATUS_DRQ)) == 0)
		{
			dir = -1;   /* command done */
			break;
		}


		/* The device must want to transfer data...
			check status: must have BSY=0, DRQ=1 now. */
		
		if ((status & (STATUS_BSY | STATUS_DRQ)) != STATUS_DRQ)
		{
			error = 55;
			dir = -1;   /* command done */
			break;
		}


		/* get the byte count, check for zero... */
		
		transfer_nbytes = (InByte (ata->base + REG_CYL_HI) << 8) | InByte (ata->base + REG_CYL_LO);

				
		if (transfer_nbytes < 1)
		{
			error = 59;
			dir = -1;   /* command done */
			break;
		}
		
		
		/* transfer the data and update the i/o buffer address
			and the number of bytes transfered. */
		
		transfer_nwords = (transfer_nbytes >> 1) + (transfer_nbytes & 0x0001);
		
		
		if (dir == ATAPI_DIR_READ)
		{
			pio_insw (ata->base + REG_DATA, ata_buffer, transfer_nwords * 2);
			CopyOut (as, data_buf_addr, ata_buffer, transfer_nwords * 2);
		}
		else if (dir == ATAPI_DIR_WRITE)
		{
			CopyIn (as, ata_buffer, data_buf_addr, transfer_nwords * 2);
			pio_outsw (ata->base + REG_DATA, ata_buffer, transfer_nwords * 2);
		}
		
		
		data_buf_addr += transfer_nbytes;
		DELAY400NS;    /* delay so device can get the status updated */
	}





	/* End of command...
		Wait for interrupt or poll for BSY=0,
		but don't do this if there was any error or if this
		was a commmand that did not transfer data. */
	
	if (error == 0 && dir >= 0)
	{
		error = AtaIntrWait(ata);
	}

	
	/* Final status check, only if no previous error. */
	
	if (error == 0)
	{
		/* Using interrupts so get the status read by the interrupt handler. */
		
		status = ata_stored_status;
				
		/* check for any error. */
		
		if (status & (STATUS_BSY | STATUS_DRQ | STATUS_ERR))
		{
			error = 58;
		}
	}


	if (error != 0)
	{	
		/* FIXME:  Couldn't use the status variable holding the ASR from interrupt handler.
			Had to read status register again,  reported different values, not sure why */
		
		/* FIXME Maybe this status is why it is not working? */
				
		return -1;
	}
	else
	{
		return 0;
	}
}




void AtapiTester (int n)
{
	DELAY400NS;
}




void AtapiReset (struct Ata *ata)
{

  // Start the command by setting the Command register.  The drive
   // should immediately set BUSY status.


	OutByte (ata->base + REG_LDH,  ata->ldhpref);
	OutByte (ata->base + REG_COMMAND,  0x08);

   // Waste some time by reading the alternate status a few times.
   // This gives the drive time to set BUSY in the status register on
   // really fast systems.  If we don't do this, a slow drive on a fast
   // system may not set BUSY fast enough and we would think it had
   // completed the command when it really had not even started the
   // command yet.

   DELAY400NS;
   DELAY400NS;
   
   //    This is a Dev Reset command (cmd=0x08) then
   //    there should be no interrupt.  So we must
   //    poll for BSY=0.

	
	
	AtaWaitFor (ata, STATUS_BSY, 0);
	
	KSetSignals (1 << ata->isr_signal, 0);
}
