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
 * Identify, Reset, ReadSectors, WriteSectors in this file??????
 */

int AtaIdentify (struct Ata *ata, void *buf)
{
	int t;
	struct command cmd;


	for (t=0; t<MAX_ATA_TRIES; t++)
	{
		cmd.ldh     = ata->ldhpref;
		cmd.command = ATA_CMD_IDENTIFY;
				
		if (AtaPIODataIn (ata, &cmd, buf, 1) == 0)
			return 0;
		else if (ata_needs_reset == TRUE)
		{
			if (AtaReset (ata, 0) != 0)
				return -1;
		}
	}
	
	return -1;
}




/* 
 * AtaReadSectors()
 */

int AtaReadSectors (struct Ata *ata, int sector_cnt, uint32 block, void *buf, struct AddressSpace *as)
{
	int t;
	int rc;
	struct command cmd;
	uint32 cylinder, head, sector;
	

	for (t=0; t<MAX_ATA_TRIES; t++)
	{
		if (sector_cnt == 0)
			return -1;
			
		if(ata->ldhpref & LDH_LBA)
		{
			cmd.sector  = (block >>  0) & 0x000000ff;
			cmd.cyl_lo  = (block >>  8) & 0x000000ff;
			cmd.cyl_hi  = (block >> 16) & 0x000000ff;
			cmd.ldh     = ata->ldhpref | ((block >> 24) & 0x0000000f);
		}
		else
		{	
			cylinder = block / (ata->pheads * ata->psectors);
			head = (block % (ata->pheads * ata->psectors)) / ata->psectors;
			sector = block % ata->psectors;
			cmd.sector  = sector + 1;
			cmd.cyl_lo  = cylinder & 0x000000ff;
			cmd.cyl_hi  = (cylinder >> 8) & 0x000000ff;
			cmd.ldh     = ata->ldhpref | head;
		}
		
		if (sector_cnt == 256)
			cmd.count = 0;
		else
			cmd.count = sector_cnt;
			
		cmd.precomp = 0;
		cmd.command = ATA_CMD_READ;
		
		KAlarmSet (&ata_alarm, ATA_TIMEOUT, 0, 1 << ata_alarm_signal);

		rc = AtaPIODataIn (ata, &cmd, ata_buffer, sector_cnt);
		
		CopyOut(as, buf, ata_buffer, sector_cnt * 512);
		

		KAlarmCancel(&ata_alarm);
		
		if (rc == 0)
			return 0;
		else if (ata_needs_reset == TRUE)
		{
			if (AtaReset (ata, 0) != 0)
				return -1;
		}
	}
	
	return -1;
}




/* 
 * AtaWriteSectors()
 * 
 * FIXME:  Ram and ATA read/write assumes data goes to kernel/cache, direct
 * from pio registers.  Need to use buffer and copyout like floppy if going
 * direct to user-space.
 */

int AtaWriteSectors (struct Ata *ata, int sector_cnt, uint32 block, void *buf, struct AddressSpace *as)
{
	int t;
	int rc;
	struct command cmd;
	uint32 cylinder, head, sector;

	for (t=0; t<MAX_ATA_TRIES; t++)
	{
		if (sector_cnt == 0)
			return -1;
			
		if(ata->ldhpref & LDH_LBA)
		{
			cmd.sector  = (block >>  0) & 0x000000ff;
			cmd.cyl_lo  = (block >>  8) & 0x000000ff;
			cmd.cyl_hi  = (block >> 16) & 0x000000ff;
			cmd.ldh     = ata->ldhpref | ((block >> 24) & 0x0000000f);
		}
		else
		{	
			cylinder = block / (ata->pheads * ata->psectors);
			head = (block % (ata->pheads * ata->psectors)) / ata->psectors;
			sector = block % ata->psectors;
			cmd.sector  = sector + 1;
			cmd.cyl_lo  = cylinder & 0x000000ff;
			cmd.cyl_hi  = (cylinder >> 8) & 0x000000ff;
			cmd.ldh     = ata->ldhpref | head;
		}
		
		if (sector_cnt == 256)
			cmd.count = 0;
		else
			cmd.count = sector_cnt;
		
		
		cmd.precomp = 0;
		cmd.command = ATA_CMD_WRITE;
		
		
		KAlarmSet (&ata_alarm, ATA_TIMEOUT, 0, 1 << ata_alarm_signal);
		
		CopyIn(as, ata_buffer, buf, sector_cnt * 512);
		
		rc = AtaPIODataOut (ata, &cmd, ata_buffer, sector_cnt);
				
		KAlarmCancel(&ata_alarm);
		
		if (rc == 0)
			return 0;
		else if (ata_needs_reset == TRUE)
		{
			if (AtaReset (ata, 0) != 0)
				return -1;
		}
	}

	return -1;
}




/*
 * PIO Data In protocol
 * See Page 64 of ATA-2 doc  d0948r4c-ata-2.pdf
 */

int AtaPIODataIn (struct Ata *ata, struct command *cmd, void *buf, int cnt)
{
	KSetSignals (1 << ata->isr_signal, 0);
	
	
	/* a) */
 	
 	if (AtaWaitFor (ata, STATUS_BSY, 0) != 0)
		return -1;
	
	/* b) */	
	OutByte (ata->base + REG_LDH, cmd->ldh);

	/* c) */
	if (AtaWaitFor (ata, STATUS_BSY | STATUS_DRQ, 0) != 0)
		return -1;

	/* d) */
	
	OutByte (ata->base + REG_CTL, ata->pheads >= 8 ? CTL_HD15 : 0);
	OutByte (ata->base + REG_PRECOMP, cmd->precomp);
	OutByte (ata->base + REG_COUNT,   cmd->count);
	OutByte (ata->base + REG_SECTOR,  cmd->sector);
	OutByte (ata->base + REG_CYL_LO,  cmd->cyl_lo);
	OutByte (ata->base + REG_CYL_HI,  cmd->cyl_hi);
	
	
	/* e) */
	OutByte (ata->base + REG_COMMAND, cmd->command);
	
	DELAY400NS;
	
	/* f) */
	
	while (cnt != 0) 
	{
		/* g) h) */
		
		if (AtaIntrWait (ata) != 0)
			return -1;
		
		if (AtaWaitFor (ata, STATUS_DRQ, STATUS_DRQ) != 0)
			return -1;
	
		/* i) */
		
		pio_insw (ata->base + REG_DATA, buf, SECTOR_SIZE);
		buf += SECTOR_SIZE;
		cnt --;
	}
	
	return 0;
}






/*
 * PIO Data Out protocol
 * See Page of ATA-2 doc  d0948r4c-ata-2.pdf
 */

int AtaPIODataOut (struct Ata *ata, struct command *cmd, void *buf, int cnt)
{ 	
	KSetSignals (1 << ata->isr_signal, 0);
	
	
 	/* a) */
 	if (AtaWaitFor (ata, STATUS_BSY, 0) != 0)
		return -1;

 	/* b) */	
	OutByte (ata->base + REG_LDH, cmd->ldh);

 	/* c) */
	if (AtaWaitFor (ata, STATUS_BSY | STATUS_DRQ, 0) != 0)
		return -1;
	
	/* d) */
	OutByte (ata->base + REG_CTL, ata->pheads >= 8 ? CTL_HD15 : 0);
	OutByte (ata->base + REG_PRECOMP, cmd->precomp);
	OutByte (ata->base + REG_COUNT,   cmd->count);
	OutByte (ata->base + REG_SECTOR,  cmd->sector);
	OutByte (ata->base + REG_CYL_LO,  cmd->cyl_lo);
	OutByte (ata->base + REG_CYL_HI,  cmd->cyl_hi);

	/* e) */
	OutByte (ata->base + REG_COMMAND, cmd->command);

	DELAY400NS;

	/* f) */
	
	while (cnt != 0) 
	{
		/* g) h) */

		if (AtaWaitFor (ata, STATUS_DRQ, STATUS_DRQ) != 0)
			return -1;

		/* i) */
		
		pio_outsw (ata->base + REG_DATA, buf, SECTOR_SIZE);
		buf += SECTOR_SIZE;
	
		if (AtaIntrWait (ata) != 0)
			return -1;
		
		cnt --;
	}
	
	return 0;
}




/* 
 * AtaReset()
 */

int AtaReset (struct Ata *ata, int skip)
{
	uint8 sc;
	uint8 sn;
	uint8 status;
	uint8 ctl;
	struct Ata *ata0, *ata1;

	KPRINTF ("AtaReset()");

	ata0 = ata->device[0];
	ata1 = ata->device[1];
	ata_error = 0;
	
	
	/* Initialise the timeout */
	
	KAlarmSet (&ata_alarm, ATA_TIMEOUT, 0, 1 << ata_alarm_signal);
	
	
	/* Set and then reset the soft reset bit in the Device Control register.
	 * This causes device 0 be selected.
	 */
	
	/*
	if (!skip)
	{*/	
		
		ctl = /* FIXME: CTL_HD15 | */ (use_interrupts ? 0 : CTL_NIEN);

		OutByte (ata0->base + REG_CTL, ctl | CTL_RESET);
		DELAY400NS;
		OutByte (ata0->base + REG_CTL, ctl);
		DELAY400NS;
	
	/*
	}
	*/
	
	
	
	
	/* If there is a device 0, wait for device 0 to set BSY=0.
	 */
	
	if (ata0->config != ATA_CONFIG_NONE)
	{
		/* FIXME:  AtaAtapiDelay (ata0); */
		
		while (1)
		{
			status = InByte (ata0->base + REG_STATUS);
			
			if ((status & STATUS_BSY) == 0)
				break;
			
			
			if (KAlarmCheck(&ata_alarm) != 0)  /* Does it have a value ? */
			{
				ata_error = 2;       /* error code */
				break;
			}
		}
	}
	
		
	/* If there is a device 1, wait until device 1 allows register access.
	 */
	
	if (ata1->config != ATA_CONFIG_NONE)
	{
		/* AtaAtapiDelay (ata1); */

		while (1)
		{
			OutByte(ata1->base + REG_LDH, (1<<4));
			DELAY400NS;
			sc = InByte (ata1->base + REG_COUNT);
			sn = InByte (ata1->base + REG_SECTOR);
			
			if ((sc == 0x01) && (sn == 0x01))
				break;
			
			
			if (KAlarmCheck (&ata_alarm) != 0)
			{
				ata_error = 2;       /* error code */
				break;
			}
		}
	
		/* Now check if drive 1 set BSY=0.
		 */
	
		if (ata_error == 0)
		{
			if (InByte (ata1->base + REG_STATUS) & STATUS_BSY)
			{
				ata_error = 3;
			}
		}
	}
	
	/* We are done but now we must select the device the caller
	 * requested. This will cause
	 * the correct data to be returned in reg_cmd_info.
	 */

   OutByte (ata1->base + REG_LDH, 0<<4);
   DELAY400NS;
   
   
	if (ata0->config != ATA_CONFIG_NONE)
	{
		OutByte (ata0->base + REG_LDH, 0<<4);
		DELAY400NS;
	}
   
	if (ata1->config != ATA_CONFIG_NONE)
	{
		OutByte (ata0->base + REG_LDH, 1<<4);
		DELAY400NS;
	}
	
	
	
	KAlarmCancel (&ata_alarm);


	KSetSignals (1 << ata0->isr_signal, 0);

	ata_needs_reset = FALSE;
	
	if (ata_error == 0)
		return 0;
	else
		return -1;
	
}




/*
 * See Landis ataioreg.c   reg_config();
 */
 
int AtaDetect (struct Ata *ata0, struct Ata *ata1)
{
	uint8 sc;
	uint8 sn;
	uint8 cl;
	uint8 ch;
	uint8 st;
	uint8 ctl;
	
	KPRINTF ("AtaDetect()");
	
	/* assume there are no devices
	 */
	
	ata0->config = ATA_CONFIG_NONE;
	ata1->config = ATA_CONFIG_NONE;
	
	
	/* set up Device Control register
	 */
	
	ctl = CTL_HD15 | (use_interrupts ? 0 : CTL_NIEN); 
	OutByte (ata0->base + REG_CTL, ctl);
	
	/* lets see if there is a device 0
	 */
	
	OutByte (ata0->base + REG_LDH, (0 << 4));
	DELAY400NS;
	OutByte (ata0->base + REG_COUNT, 0x55);
	OutByte (ata0->base + REG_SECTOR, 0xaa);
	OutByte (ata0->base + REG_COUNT, 0xaa);
	OutByte (ata0->base + REG_SECTOR, 0x55);
	OutByte (ata0->base + REG_COUNT, 0x55);
	OutByte (ata0->base + REG_SECTOR, 0xaa);
	sc = InByte (ata0->base + REG_COUNT);
	sn = InByte (ata0->base + REG_SECTOR);
	
	KPRINTF ("ata0 - sc = %#02x, sn=%#02x", sc, sn);
	
	if ((sc == 0x55) && (sn == 0xaa))
	   ata0->config = ATA_CONFIG_UNKNOWN;  /* Means there might be something */
	
	/* Lets see if there is a device 1
	 */
	
	OutByte(ata1->base + REG_LDH, (1<<4));
	DELAY400NS;
	OutByte (ata1->base + REG_COUNT, 0x55);
	OutByte (ata1->base + REG_SECTOR, 0xaa);
	OutByte (ata1->base + REG_COUNT, 0xaa);
	OutByte (ata1->base + REG_SECTOR, 0x55);
	OutByte (ata1->base + REG_COUNT, 0x55);
	OutByte (ata1->base + REG_SECTOR, 0xaa);
	sc = InByte (ata1->base + REG_COUNT);
	sn = InByte (ata1->base + REG_SECTOR);
	
	KPRINTF ("ata1 - sc = %#02x, sn=%#02x", sc, sn);
	
	if ((sc == 0x55) && (sn == 0xaa))
	   ata1->config = ATA_CONFIG_UNKNOWN;
	
	
	/* Now we think we know which devices, if any are there,
	 * so lets try a soft reset (ignoring any errors).
	 */
			
	OutByte (ata0->base + REG_LDH, (0<<4));
	DELAY400NS;
	AtaReset (ata0, 0);
	
		
	/* lets check device 0 again, is device 0 really there?
	 * is it ATA or ATAPI?
	 */
	
	OutByte (ata0->base + REG_LDH, (0<<4));
	DELAY400NS;
	sc = InByte (ata0->base + REG_COUNT);
	sn = InByte (ata0->base + REG_SECTOR);
	
	KPRINTF ("ata0 retest - sc = %#02x, sn=%#02x", sc, sn);
		
	if ((sc == 0x01) && (sn == 0x01))
	{
	   ata0->config = ATA_CONFIG_UNKNOWN;
	   st = InByte (ata0->base + REG_STATUS);
	   cl = InByte (ata0->base + REG_CYL_LO);
	   ch = InByte (ata0->base + REG_CYL_HI);

		KPRINTF ("**** st = %#02x, cl=%#02x, ch=%#02x", st, cl, ch);
	   
	   if ( ((cl == 0x14) && (ch == 0xeb))          /* PATAPI */
	        || (( cl == 0x69 ) && ( ch == 0x96)) )  /* SATAPI */
	   {
	      ata0->config = ATA_CONFIG_ATAPI;
	   }
	   else if ((st != 0)
	        && ((( cl == 0x00) && (ch == 0x00))     /* PATA */
	        || ((cl == 0x3c) && (ch == 0xc3))) )    /* SATA */
	   {
	      ata0->config = ATA_CONFIG_ATA;
	   }
	}
	
	/* lets check device 1 again, is device 1 really there?
	 * is it ATA or ATAPI?
	 */
	
	OutByte (ata1->base + REG_LDH, (1<<4));
	DELAY400NS;
	sc = InByte (ata1->base + REG_COUNT);
	sn = InByte (ata1->base + REG_SECTOR);

	KPRINTF ("ata1 retest - sc = %#02x, sn=%#02x", sc, sn);
	
	
	if ((sc == 0x01) && (sn == 0x01))
	{
	   ata1->config = ATA_CONFIG_UNKNOWN;

	   st = InByte (ata1->base + REG_STATUS);
	   cl = InByte (ata1->base + REG_CYL_LO);
	   ch = InByte (ata1->base + REG_CYL_HI);
	   
   		KPRINTF ("**** st = %#02x, cl=%#02x, ch=%#02x", st, cl, ch);
	   
	   if ( ((cl == 0x14) && (ch == 0xeb))          /* PATAPI */
	        || ((cl == 0x69) && (ch == 0x96)))      /* SATAPI */
	   {
	      ata1->config = ATA_CONFIG_ATAPI;
	   }
	   else if ( (st != 0)
	        && (((cl == 0x00) && (ch == 0x00))      /* PATA */
	        || ((cl == 0x3c) && (ch == 0xc3))))     /* SATA */
	   {
	      ata1->config = ATA_CONFIG_ATA;
	   }
	}
	

	/* FIX:  Added this extra reset, otherwise the ata status register
		remains busy */
	
	DELAY400NS;
	AtaReset (ata0, 0);
	DELAY400NS;
		
	
	if (ata0->config != ATA_CONFIG_NONE || ata1->config != ATA_CONFIG_NONE)
		return 0;
	else
		return -1;
}
