#include <kernel/types.h>
#include <kernel/i386/i386.h>
#include <kernel/device.h>
#include <kernel/sync.h>
#include <kernel/proc.h>
#include <kernel/msg.h>
#include <kernel/vm.h>
#include <kernel/kmalloc.h>
#include <kernel/timer.h>
#include <kernel/utility.h>
#include <kernel/dbg.h>
#include "ata.h"




/*
 *
 */

void AtaTaskInit (void)
{
	KPRINTF ("AtaTaskInit()");

	ata_pid = GetPID();
	
	if ((ata_buffer = KMalloc (ATA_BUFFER_SZ)) != NULL)
	{
		if ((ata_timer_signal = AllocSignal()) != -1)
		{
			if ((ata_alarm_signal = AllocSignal()) != -1)
			{
				if ((ata_msgport = CreateMsgPort()) != NULL)
				{
					if (AtaInitUnits() == 0)
					{
						ata_init_error = 0;
						KSignal (GetPPID(), SIG_INIT);
						return;
					}
					
					DeleteMsgPort (ata_msgport);
				}
				
				FreeSignal (ata_alarm_signal);
			}

			FreeSignal (ata_timer_signal);
		}
				
		KFree (ata_buffer);
	}
	
	ata_init_error = -1;	
	KSignal (GetPPID(), SIG_INIT);
	
	Exit(-1);
}



/*
 *
 */

void AtaTaskFini (void)
{
	if (ata_isr14_enabled == TRUE)
	{
		ISRHandlerRemove (ata_isr14_handler);
		FreeSignal (ata_isr14_signal);
	}
	
	if (ata_isr15_enabled == TRUE)
	{
		ISRHandlerRemove (ata_isr15_handler);
		FreeSignal (ata_isr15_signal);
	}	
			
	DeleteMsgPort (ata_msgport);
	FreeSignal (ata_alarm_signal);
	Exit (0);
}




uint8 ata_mbr[512];

/*
 *
 */

int AtaInitUnits (void)
{
	int t;
	uint32 size;
	struct MountEnviron *me;
	struct Partition *partition_table;
	int p, hd_cnt, cd_cnt;
	uint16 general_config;

	
	KPRINTF ("AtaInitUnits()");
	
	use_interrupts = FALSE;

	for (t=0; t<4; t++)
	{
		ata_drive[t].enabled = FALSE;
		ata_drive[t].config = ATA_CONFIG_NONE;
		ata_drive[t].reference_cnt = 0;
		
		
		switch (t)
		{
			case 0:
				ata_drive[t].ldhpref = LDH_DEFAULT | (0 << 4);
				break;
			
			case 1:
				ata_drive[t].ldhpref = LDH_DEFAULT | (1 << 4);
				break;
			
			case 2:
				ata_drive[t].ldhpref = LDH_DEFAULT | (0 << 4);
				break;
				
			case 3:
				ata_drive[t].ldhpref = LDH_DEFAULT | (1 << 4);
				break;
		}
		
		
		LIST_INIT (&ata_drive[t].callback_list);
		
		if (t <2)
			ata_drive[t].base = REG_BASE0;
		else
			ata_drive[t].base = REG_BASE1;
	}
	
	ata_drive[0].device[0] = &ata_drive[0];
	ata_drive[0].device[1] = &ata_drive[1];	
	
	ata_drive[1].device[0] = &ata_drive[0];
	ata_drive[1].device[1] = &ata_drive[1];	
	
	ata_drive[2].device[0] = &ata_drive[2];
	ata_drive[2].device[1] = &ata_drive[3];	
	
	ata_drive[3].device[0] = &ata_drive[2];
	ata_drive[3].device[1] = &ata_drive[3];	
	
	AtaDetect (&ata_drive[0], &ata_drive[1]);
	AtaDetect (&ata_drive[2], &ata_drive[3]);
	
	
	
	
	if (ata_drive[0].config != ATA_CONFIG_UNKNOWN ||
		ata_drive[1].config != ATA_CONFIG_UNKNOWN)
	{

		if ((ata_isr14_signal = AllocSignal()) != -1)
		{
			if ((ata_isr14_handler = ISRHandlerInsert (14, AtaISRHandler, NULL)) != NULL)
			{
				ata_isr14_enabled = TRUE;
				ata_drive[0].isr_signal = ata_isr14_signal;
				ata_drive[1].isr_signal = ata_isr14_signal;
			}
			else
				FreeSignal (ata_isr14_signal);
		}	
		
	}
	
	if (ata_drive[2].config != ATA_CONFIG_UNKNOWN ||
		ata_drive[3].config != ATA_CONFIG_UNKNOWN)
	{
		if ((ata_isr15_signal = AllocSignal()) != -1)
		{
			if ((ata_isr15_handler = ISRHandlerInsert (15, AtaISRHandler, NULL)) != NULL)
			{
				ata_isr15_enabled = TRUE;
				ata_drive[2].isr_signal = ata_isr15_signal;
				ata_drive[3].isr_signal = ata_isr15_signal;
			}
			else
				FreeSignal (ata_isr15_signal);
		}	

	}

	use_interrupts = TRUE;

	for (t=0; t<4; t++)
	{
		if (ata_drive[t].config == ATA_CONFIG_ATA)
		{
			KPRINTF ("ATA Drive (%d) is ATA", t);
		
			if (AtaIdentify (&ata_drive[t], ata_buffer) == 0)
			{
				ata_drive[t].pcylinders = id_word(1);
				ata_drive[t].pheads = id_word(3);
				ata_drive[t].psectors = id_word(6);
			
				size = (uint32) ata_drive[t].pcylinders
								* ata_drive[t].pheads
								* ata_drive[t].psectors;
		
				if ((id_byte(49)[1] & 0x02) && size > 512L*1024*2)
				{
					ata_drive[t].ldhpref |= LDH_LBA;
					size = id_longword(60);
				}
			
				ata_drive[t].sector_sz = 512;
				ata_drive[t].size = size;
				ata_drive[t].disk_state = ATA_MEDIA_PRESENT;
				ata_drive[t].diskchange_cnt = 0;
				

				ata_drive[t].enabled = FALSE;
			}
		}
		else if (ata_drive[t].config == ATA_CONFIG_ATAPI)
		{
			KPRINTF ("ata_isr15_cnt = %d", ata_isr15_cnt);

		
			KPRINTF ("ATA Drive (%d) is ATAPI, RESETTING", t);

			AtapiReset (&ata_drive[t]);
			
			KPRINTF ("ATAPI RESET complete");
			KPRINTF ("ata_isr15_cnt = %d", ata_isr15_cnt);

			
			if (AtapiIdentify (&ata_drive[t], ata_buffer) == 0)
			{
				KPRINTF ("Atapi Identify OK on ATAPI device");
				KPRINTF ("ata_isr15_cnt = %d", ata_isr15_cnt);
			
				general_config = id_word (0);
						
				ata_drive[t].atapi_protocol_type = (general_config & 0xC000) >> 14;
				ata_drive[t].atapi_type  = (general_config & 0x1F00) >> 8;
				ata_drive[t].atapi_removable = (general_config & 0x0080) >> 7;
				ata_drive[t].atapi_cmd_drq_type = (general_config & 0x0060) >> 5;
				
				ata_drive[t].atapi_cmd_packet_size = ((general_config & 0x0001) == 1) ? 16 : 12;
				
				
				KPRINTF ("protocol_type = %d", ata_drive[t].atapi_protocol_type);
				KPRINTF ("atapi_type = %d", ata_drive[t].atapi_type);
				KPRINTF ("packet_size = %d", ata_drive[t].atapi_cmd_packet_size);
				
				
				if (ata_drive[t].atapi_protocol_type == ATAPI_PROTOCOL_ATAPI
							&& ata_drive[t].atapi_type == ATAPI_TYPE_CDROM
							&& ata_drive[t].atapi_cmd_packet_size == 12)
				{
					KPRINTF ("ATAPI CDROM DETECTED");
					
					ata_drive[t].sector_sz = 2048;
					ata_drive[t].size = 0;
					ata_drive[t].enabled = TRUE;
					ata_drive[t].disk_state = AtapiMediaPresent (&ata_drive[t]);
					ata_drive[t].diskchange_cnt = 0;
				}
			}
		}
		else
		{
			KPRINTF ("ATA Drive (%d) is UNKNOWN", t);
		}
	}
	
	
	
	hd_cnt = 0;
	cd_cnt = 0;
	
	
	for (t=0; t<4; t++)
	{
		if (ata_drive[t].enabled == TRUE)
		{
			if (ata_drive[t].config == ATA_CONFIG_ATA)
			{
				AtaReadSectors (&ata_drive[t], 1, 0,  &ata_mbr, &kernel_as);
				
				partition_table = (struct Partition *)(ata_mbr + 0x1be);
											
				for (p = 0; p < 4; p++)
				{
					if (partition_table[p].type == 0 || partition_table[p].size == 0)
						continue;
					
					if ((me = AllocMountEnviron()) != NULL)
					{
						me->mount_name[0] = 'h';
						me->mount_name[1] = 'd';
						me->mount_name[2] = '0' + (hd_cnt % 10);
						me->mount_name[3] = '\0';
						hd_cnt++;
						
						StrLCpy (me->handler_name, "fat.handler", MOUNTENVIRON_STR_MAX + 1);
						StrLCpy (me->device_name, "ata.device", MOUNTENVIRON_STR_MAX + 1);
						StrLCpy (me->startup_args, "", MOUNTENVIRON_STR_MAX + 1);
						
						me->handler_unit = 0;
						me->handler_flags = 0;
						
						me->device_unit = t;
						me->device_flags = 0;
					
						me->block_size = 512;
						
						me->partition_start = partition_table[p].start_lba;
						me->partition_end = me->partition_start + partition_table[p].size;
						
						me->buffer_cnt = 512;
						
						me->boot_priority = 2;
						
						me->baud = 0;
		
						me->removable = FALSE;
						me->writable = TRUE;
						
						me->writethru_critical = 1;
						me->writeback_delay = 1;
	
						me->max_transfer = 0x10000;
	
						me->control_flags = 0;
						
						
						AddBootMountEnviron (me);
					}
				}
			}
			else if (ata_drive[t].config == ATA_CONFIG_ATAPI)
			{
				if ((me = AllocMountEnviron()) != NULL)
				{
					me->mount_name[0] = 'c';
					me->mount_name[1] = 'd';
					me->mount_name[2] = '0' + (cd_cnt % 10);
					me->mount_name[3] = '\0';
					cd_cnt++;
									
					StrLCpy (me->handler_name, "cd.handler", MOUNTENVIRON_STR_MAX + 1);
					StrLCpy (me->device_name, "ata.device", MOUNTENVIRON_STR_MAX + 1);
					StrLCpy (me->startup_args, "", MOUNTENVIRON_STR_MAX + 1);
										
					me->handler_unit = 0;
					me->handler_flags = 0;
					
					me->device_unit = t;
					me->device_flags = 0;
					
					me->block_size = 2048;
					
					me->partition_start = 0;
					me->partition_end = 0;
					
					me->buffer_cnt = 128;
					
					me->boot_priority = 1;
					me->baud = 0;
	
					me->removable = TRUE;
					me->writable = FALSE;
					
					me->writethru_critical = 0;
					me->writeback_delay = 0;
	
					me->max_transfer = 0x10000;
					me->control_flags = 0;
								
					AddBootMountEnviron (me);
				}
			}
		}
	}
	
	return 0;
}


