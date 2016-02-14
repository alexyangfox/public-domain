#include <kernel/types.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/lists.h>
#include <kernel/error.h>
#include <kernel/buffers.h>
#include <kernel/dbg.h>
#include "fat.h"





/*
 * The FSB will contain the partition size even if the BPB
 * is not a valid FAT.
 * Need a bit to indicate whether the disk is in a valid state or not on return.
 */

int FatFormat (struct FatSB *fsb, char *label, uint32 flags, uint32 cluster_size)
{	
	struct FatDirEntry label_dirent;


	KPRINTF ("FatFormat()");
	

	if (fsb->write_protect == TRUE)
	{
		SetError (EROFS);
		return -1;
	}
	
	
	
	if (label == NULL)
	{
		if (FatASCIIZToDirEntry (&label_dirent, "NO NAME") != 0)
			return -1;
	}
	else if (FatASCIIZToDirEntry (&label_dirent, label) != 0)
	{
		return -1;
	}
	else
	{
		SetError (EINVAL);
		return -1;
	}
	
	
	/* Might crash here if invalidated */
	FatInvalidate (fsb);
	

	if (InitializeFatSB (fsb, &label_dirent, flags, cluster_size) == 0)
	{
		KPRINTF ("InitializeFatSB OK");
	
		if (FatEraseDisk (fsb, flags) == 0)
		{
			KPRINTF ("FatEraseDisk OK");
		
			if (FatWriteBootRecord (fsb) == 0)	/* Can include the backups */
			{
				KPRINTF ("FatWriteBootRecord OK");
			
				if (FatInitFATs (fsb) == 0)
				{
					KPRINTF ("FatInitFATs OK");
				
					if (FatInitRootDirectory (fsb, &label_dirent) == 0)
					{
						KPRINTF ("FatInitRootDirectory OK");
					
						FatRevalidate (fsb, 0);
						
						return 0;
					}
				}
			}
		}
	}
	
	SetError (ENOSYS);
	return -1;
}




/*
 *
 */

int InitializeFatSB (struct FatSB *fsb, struct FatDirEntry *label_dirent, uint32 flags, uint32 cluster_size)
{
	int s, t;
	uint32 root_dir_sectors, root_ent_cnt, bytes_per_sector, reserved_sectors_cnt;
	uint32 tmp1, tmp2, sectors_per_fat;
	uint32 sectors_per_cluster;
	int fat_cnt;
	struct TimeVal tv;
	uint32 data_sector_cnt;
	char *fat_str;
	int fat_type;
	
	
	KPRINTF ("InitializeFatSB partition_size = %d", fsb->partition_size);
	
	
	
	
	KGetTimeOfDay(&tv);
	
	if (fsb->partition_size <= 8400)
	{
		for (t=0; t<8; t++)
		{
			if (fat12_bpb[t].total_sectors_cnt16 == fsb->partition_size)
			{
				fsb->fat_type = TYPE_FAT12;
				
				fsb->bpb.jump[0] = 0xeb;
				fsb->bpb.jump[1] = FAT16_BOOTCODE_START - 2;
				fsb->bpb.jump[2] = 0x90;
				fsb->bpb.oem_name[0] = 'M';
				fsb->bpb.oem_name[1] = 'S';
				fsb->bpb.oem_name[2] = 'W';				
				fsb->bpb.oem_name[3] = 'I';
				fsb->bpb.oem_name[4] = 'N';
				fsb->bpb.oem_name[5] = '4';
				fsb->bpb.oem_name[6] = '.';
				fsb->bpb.oem_name[7] = '1';
				
				fsb->bpb.bytes_per_sector		= fat12_bpb[t].bytes_per_sector;
				fsb->bpb.sectors_per_cluster		= fat12_bpb[t].sectors_per_cluster;
				fsb->bpb.reserved_sectors_cnt	= fat12_bpb[t].reserved_sectors_cnt;
				fsb->bpb.fat_cnt					= fat12_bpb[t].fat_cnt;
				fsb->bpb.root_entries_cnt		= fat12_bpb[t].root_entries_cnt;
				fsb->bpb.media_type				= fat12_bpb[t].media_type;
				fsb->bpb.sectors_per_fat16		= fat12_bpb[t].sectors_per_fat16;
				fsb->bpb.sectors_per_track		= fat12_bpb[t].sectors_per_track;
				fsb->bpb.heads_per_cylinder		= fat12_bpb[t].heads_per_cylinder;
				fsb->bpb.hidden_sectors_cnt = 0;
				
				if (fsb->partition_size < 0x10000)
				{
					fsb->bpb.total_sectors_cnt16 = fat12_bpb[t].total_sectors_cnt16;
					fsb->bpb.total_sectors_cnt32 = 0;
				}
				else
				{
					fsb->bpb.total_sectors_cnt16 = 0;
					fsb->bpb.total_sectors_cnt32 = fat12_bpb[t].total_sectors_cnt16;
				}
				
				fsb->bpb16.drv_num = 0x00;  /* Or check mount? */
				fsb->bpb16.reserved1 = 0;
				fsb->bpb16.boot_sig	= 0x29;
				
				fsb->bpb16.volume_id = tv.seconds;
				
				for (s=0; s<8; s++)
					fsb->bpb16.volume_label[s] = label_dirent->name[s];
				
				for (s=0; s<3; s++)
					fsb->bpb16.volume_label[8+s] = label_dirent->extension[s];
				
				fat_str = "FAT12   ";
				for (s=0;s <8; s++)
					fsb->bpb16.filesystem_type[s] = fat_str[s];
				
				fsb->start_search_cluster = 2;
				
				break;
			}
		}
		
		if (t==8)
		{
			KPRINTF ("UNKNOWN FLOPPY SIZE = %d", fsb->partition_size);
			return -1;
		}
	}
	else if (fsb->partition_size > 8400 && fsb->partition_size <= 1048576)
	{
		fsb->fat_type = TYPE_FAT16;
		
		fat_cnt = 2;
		root_ent_cnt = 512;
		bytes_per_sector = 512;
		reserved_sectors_cnt = 1;
		sectors_per_cluster = 0;
		
		for (t=0; dsksz_to_spc_fat16[t].disk_size != 0xffffffff; t++)
		{
			if (fsb->partition_size <= dsksz_to_spc_fat16[t].disk_size)
				sectors_per_cluster = dsksz_to_spc_fat16[t].sectors_per_cluster;
		}
		
		if (sectors_per_cluster == 0)
			return -1;
				
		root_dir_sectors = ((root_ent_cnt * 32) + (bytes_per_sector - 1)) / bytes_per_sector;
		tmp1 = fsb->partition_size - (reserved_sectors_cnt + root_dir_sectors);
		tmp2 = (256 * sectors_per_cluster) + fat_cnt;
		sectors_per_fat = (tmp1 + (tmp2 - 1)) / tmp2;

		fsb->bpb.jump[0] = 0xeb;
		fsb->bpb.jump[1] = FAT16_BOOTCODE_START - 2;
		fsb->bpb.jump[2] = 0x90;
		fsb->bpb.oem_name[0] = 'M';
		fsb->bpb.oem_name[1] = 'S';
		fsb->bpb.oem_name[2] = 'W';				
		fsb->bpb.oem_name[3] = 'I';
		fsb->bpb.oem_name[4] = 'N';
		fsb->bpb.oem_name[5] = '4';
		fsb->bpb.oem_name[6] = '.';
		fsb->bpb.oem_name[7] = '1';
		
		fsb->bpb.bytes_per_sector		= bytes_per_sector;
		fsb->bpb.sectors_per_cluster	= sectors_per_cluster;
		fsb->bpb.reserved_sectors_cnt	= reserved_sectors_cnt;
		fsb->bpb.fat_cnt				= fat_cnt;
		fsb->bpb.root_entries_cnt		= 512;
		fsb->bpb.total_sectors_cnt16	= fsb->partition_size;
		fsb->bpb.media_type				= (fsb->removable == TRUE) ? 0xf0 : 0xf8;
		fsb->bpb.sectors_per_fat16		= sectors_per_fat;
		fsb->bpb.sectors_per_track		= 0;
		fsb->bpb.heads_per_cylinder		= 0;
		fsb->bpb.hidden_sectors_cnt		= 0;
		
		if (fsb->partition_size < 0x10000)
		{
			fsb->bpb.total_sectors_cnt16 = fsb->partition_size;
			fsb->bpb.total_sectors_cnt32 = 0;
		}
		else
		{
			fsb->bpb.total_sectors_cnt16 = 0;
			fsb->bpb.total_sectors_cnt32 = fsb->partition_size;
		}

		
		fsb->bpb16.drv_num = 0x00;
		fsb->bpb16.reserved1 = 0;
		fsb->bpb16.boot_sig	= 0x29;
		fsb->bpb16.volume_id = tv.seconds;
		
		/* Can be "NO NAME" if label = NULL,  need to test that in outer wrapper */
		
		for (t=0; t<8; t++)
			fsb->bpb16.volume_label[t] = label_dirent->name[t];
	
		for (t=0; t<3; t++)
			fsb->bpb16.volume_label[8+t] = label_dirent->extension[t];
		
		fat_str = "FAT16   ";
		for (t=0;t <8; t++)
			fsb->bpb16.filesystem_type[t] = fat_str[t];
			
		fsb->start_search_cluster = 2;
	}
	else
	{
		fsb->fat_type = TYPE_FAT32;
		
		fat_cnt = 2;
		root_ent_cnt = 0;
		bytes_per_sector = 512;
		reserved_sectors_cnt = 32;
		sectors_per_cluster = 0;
		
		for (t=0; dsksz_to_spc_fat32[t].disk_size != 0xffffffff; t++)
		{
			if (fsb->partition_size <= dsksz_to_spc_fat32[t].disk_size)
				sectors_per_cluster = dsksz_to_spc_fat32[t].sectors_per_cluster;
		}
		
		if (sectors_per_cluster == 0)
			return -1;
		
		root_dir_sectors = ((root_ent_cnt * 32) + (bytes_per_sector - 1)) / bytes_per_sector;
		tmp1 = fsb->partition_size - (reserved_sectors_cnt + root_dir_sectors);
		tmp2 = ((256 * sectors_per_cluster) + fat_cnt)/2;
		sectors_per_fat = (tmp1 + (tmp2 - 1)) / tmp2;
		
				
		fsb->bpb.jump[0] = 0xeb;
		fsb->bpb.jump[1] = FAT32_BOOTCODE_START - 2;
		fsb->bpb.jump[2] = 0x90;
		fsb->bpb.oem_name[0] = 'M';
		fsb->bpb.oem_name[1] = 'S';
		fsb->bpb.oem_name[2] = 'W';				
		fsb->bpb.oem_name[3] = 'I';
		fsb->bpb.oem_name[4] = 'N';
		fsb->bpb.oem_name[5] = '4';
		fsb->bpb.oem_name[6] = '.';
		fsb->bpb.oem_name[7] = '1';
		
		fsb->bpb.bytes_per_sector		= bytes_per_sector;
		fsb->bpb.sectors_per_cluster	= sectors_per_cluster;
		fsb->bpb.reserved_sectors_cnt	= reserved_sectors_cnt;
		fsb->bpb.fat_cnt				= fat_cnt;
		fsb->bpb.root_entries_cnt		= 0;
		fsb->bpb.total_sectors_cnt16	= 0;
		fsb->bpb.media_type				= (fsb->removable == TRUE) ? 0xf0 : 0xf8;
		fsb->bpb.sectors_per_fat16		= 0;
		fsb->bpb.sectors_per_track		= 0;
		fsb->bpb.heads_per_cylinder		= 0;
		fsb->bpb.hidden_sectors_cnt		= 0;
		fsb->bpb.total_sectors_cnt32	= fsb->partition_size;
		
		
		
		fsb->bpb32.sectors_per_fat32	= sectors_per_fat;	
		fsb->bpb32.ext_flags			= 0x00;	
		fsb->bpb32.fs_version           = 0x00;
		fsb->bpb32.root_cluster         = 2;    /* Had better clear/initialize it AND FAT entry */
		fsb->bpb32.fs_info				= 1;
		fsb->bpb32.boot_sector_backup	= 6;
		
		for (t=0; t< 12; t++)
			fsb->bpb32.reserved[t]		= 0;
			
		fsb->bpb32.drv_num				= 0x00;
		fsb->bpb32.reserved1			= 0x00;
		fsb->bpb32.boot_sig				= 0x29;
		fsb->bpb32.volume_id			= tv.seconds;
		
		
		for (t=0; t<8; t++)
			fsb->bpb32.volume_label[t] = label_dirent->name[t];
	
		for (t=0; t<3; t++)
			fsb->bpb32.volume_label[8+t] = label_dirent->extension[t];
		
		fat_str = "FAT32   ";
		for (t=0;t <8; t++)
			fsb->bpb32.filesystem_type[t] = fat_str[t];
		
		
		
		fsb->fsi.lead_sig = FSINFO_LEAD_SIG;
		
		
		for (t=0; t< 120;t++)
			fsb->fsi.reserved1[t] = 0;
		
		
		fsb->fsi.struc_sig = FSINFO_STRUC_SIG;
		
			
		root_dir_sectors = ((fsb->bpb.root_entries_cnt * 32) + (bytes_per_sector - 1)) / bytes_per_sector;
		data_sector_cnt = fsb->partition_size - (fsb->bpb.reserved_sectors_cnt + (fsb->bpb.fat_cnt * fsb->sectors_per_fat) + root_dir_sectors);
		fsb->fsi.free_cnt = data_sector_cnt / fsb->bpb.sectors_per_cluster - 1;
		fsb->fsi.next_free = 2;
		fsb->start_search_cluster = fsb->fsi.next_free;
		
				
		for (t=0; t<3; t++)
			fsb->fsi.reserved2[t] = 0;
		
		fsb->fsi.trail_sig = FSINFO_TRAIL_SIG;
		fsb->fsinfo_valid = TRUE;
	}

	
	fsb->reference_cnt = 0;
	LIST_INIT (&fsb->node_list);
	
	fat_type = fsb->fat_type;
	FatPrecalculateFSBValues (fsb);

	KASSERT (fat_type == fsb->fat_type);
						
	return 0;
}




/*
 *
 */

int FatEraseDisk (struct FatSB *fsb, uint32 flags)
{
	uint32 t, sectors;
	uint32 fat_sz;
	uint32 root_dir_sectors = 0;
	uint8 temp_sector[512];
	
	
	KPRINTF ("FatEraseDisk()");


	MemSet (temp_sector, 0, 512);
	
	if (fsb->fat_type == TYPE_FAT32)
		fat_sz = fsb->bpb32.sectors_per_fat32;
	else
		fat_sz = fsb->bpb.sectors_per_fat16;
	
	root_dir_sectors = ((fsb->bpb.root_entries_cnt * 32) + (fsb->bpb.bytes_per_sector - 1)) / fsb->bpb.bytes_per_sector;
	
	
		
	if (flags & FORMATF_QUICK)
	{
		sectors = fsb->bpb.reserved_sectors_cnt + (fsb->bpb.fat_cnt * fat_sz)
					+ root_dir_sectors;
		
		if (fsb->fat_type == TYPE_FAT32)
			sectors += fsb->bpb.sectors_per_cluster;
	}
	else
		sectors = fsb->partition_size;
	
	
	for (t=0; t<sectors; t++)
	{
		KPRINTF ("Erasing sector %d", t);
		
		WriteBlocks (fsb, &kernel_as, temp_sector, t, 0, 512, BUF_IMMED);
	}
	
	
	return 0;
}




/*
 *
 */

int FatWriteBootRecord (struct FatSB *fsb)
{
	uint8 temp_sector[512];
	uint16 signature;
	int t;
	
	KPRINTF ("FatWriteBootRecord()");
	
	ReadBlocks (fsb, &kernel_as, temp_sector, 0, 0, 512);

	MemCpy (temp_sector, &fsb->bpb, sizeof (struct FatBPB));
	
	
	if (fsb->fat_type == TYPE_FAT32)
	{
		MemCpy (temp_sector + BPB_EXT_OFFSET, &fsb->bpb32, sizeof (struct FatBPB_32Ext));
		MemCpy (temp_sector + FAT32_BOOTCODE_START, fat32_bootcode, SIZEOF_FAT32_BOOTCODE);
	}
	else
	{
		MemCpy (temp_sector + BPB_EXT_OFFSET, &fsb->bpb16, sizeof (struct FatBPB_16Ext));
		MemCpy (temp_sector + FAT16_BOOTCODE_START, fat16_bootcode, SIZEOF_FAT16_BOOTCODE);
	}
	
	*(temp_sector + 510) = 0x55;
	*(temp_sector + 511) = 0xaa;	

	WriteBlocks (fsb, &kernel_as, temp_sector, 0, 0, 512, BUF_IMMED);
	
	
	
	
	
	
	if (fsb->fat_type == TYPE_FAT32)
	{		
		WriteBlocks (fsb, &kernel_as, &fsb->fsi, 1, 0, sizeof (struct FatFSInfo), BUF_IMMED);
			
		signature = 0xaa55;
				
		WriteBlocks (fsb, &kernel_as, &signature, 2, 510, 2, BUF_IMMED);
		
		
		for (t=0; t<3; t++)
		{			
			ReadBlocks (fsb, &kernel_as, temp_sector, t, 0, 512);
			WriteBlocks (fsb, &kernel_as, temp_sector, 6+t, 0, 512, BUF_IMMED);
		}
	}
	
	return 0;
}




/*
 * Fill the first two entries with ?????????
 * Media descriptor???????
 *
 * Third entry for FAT32 is the root dir EOC
 */

int FatInitFATs (struct FatSB *fsb)
{
	uint32 media_byte_value;
	
	if (fsb->fat_type == TYPE_FAT12)
		media_byte_value = fsb->bpb.media_type | 0x00000f00;
	else if (fsb->fat_type == TYPE_FAT16)
		media_byte_value = fsb->bpb.media_type | 0x0000ff00;
	else
		media_byte_value = fsb->bpb.media_type | 0xffffff00;
	
	WriteFATEntry (fsb, 0, media_byte_value);
	WriteFATEntry (fsb, 1, CLUSTER_EOC);
	
	if (fsb->fat_type == TYPE_FAT32)
		WriteFATEntry (fsb, 2, CLUSTER_EOC);
	
	
	return 0;
}




/* Need to allocate a cluster for the Fat32 case as
 * WriteFile will update the first_cluster in the FatNode
 * but not the root_dir_cluster in the bpb32 and won't flush
 * the bpb32 to disk
 */
 
 /* Or call AllocCluster() and write the value to the BPB32 ? */
	/* Will need to set entry to CLUSTER_EOC in the FAT in any case */
	/* It should end up using cluster 2, in which case no need to
		flush BPB32 again, Unless we write the boot record afterward.
		We'd have to update fsinfo as there'll be 1 cluster less. */


/* AWOOGA AWOOGA FIXME:  root_node->first_cluster needs to be set to CLUSTER_EOC
	to enable it to expand with CreateDirEntry() */

	

int FatInitRootDirectory (struct FatSB *fsb, struct FatDirEntry *label_dirent)
{
	struct FatDirEntry label;
	int t;
	
		
	for (t=0; t<8; t++)
		label.name[t] = label_dirent->name[t];
	
	for (t=0; t<3; t++)
		label.extension[t] = label_dirent->extension[t];
	
	
	label.attributes = ATTR_VOLUME_ID;
	label.reserved = 0;
	label.creation_time_sec_tenths = 0;
	label.creation_time_2secs = 0;
	label.creation_date = 0;
	label.last_access_date = 0;
	label.first_cluster_hi = 0;
	label.last_write_time = 0;
	label.last_write_date = 0;
	label.first_cluster_lo = 0;
	label.size = 0;
	
	FatSetTime (fsb, &label, FAT_TIME_CREATE);
	
	FatCreateDirEntry (&fsb->root_node, &label, NULL, NULL);
	
	return 0;
}



