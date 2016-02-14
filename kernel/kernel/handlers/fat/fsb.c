#include <kernel/types.h>
#include <kernel/utility.h>
#include <kernel/dbg.h>
#include <kernel/fs.h>
#include <kernel/proc.h>
#include <kernel/device.h>
#include <kernel/error.h>
#include "fat.h"




void FatPrecalculateFSBValues (struct FatSB *fsb)
{
	fsb->root_dir_sectors = ((fsb->bpb.root_entries_cnt * sizeof (struct FatDirEntry))
								+ (512 - 1)) / 512;
	
	if (fsb->bpb.sectors_per_fat16 != 0)
		fsb->sectors_per_fat = fsb->bpb.sectors_per_fat16;
	else
		fsb->sectors_per_fat = fsb->bpb32.sectors_per_fat32;
	
	if (fsb->bpb.total_sectors_cnt16 != 0)
		fsb->total_sectors_cnt = fsb->bpb.total_sectors_cnt16;
	else
		fsb->total_sectors_cnt = fsb->bpb.total_sectors_cnt32;
	
	
	fsb->first_data_sector = fsb->bpb.reserved_sectors_cnt
				+ (fsb->bpb.fat_cnt * fsb->sectors_per_fat)
				+ fsb->root_dir_sectors;
	
	
	fsb->data_sectors = fsb->total_sectors_cnt - (fsb->bpb.reserved_sectors_cnt
								+ (fsb->bpb.fat_cnt * fsb->sectors_per_fat)
								+ fsb->root_dir_sectors);
	
	
	fsb->cluster_cnt = fsb->data_sectors / fsb->bpb.sectors_per_cluster;
	
	
	if (fsb->cluster_cnt < 4085)
		fsb->fat_type = TYPE_FAT12;
	else if (fsb->cluster_cnt < 65525)
		fsb->fat_type = TYPE_FAT16;
	else
		fsb->fat_type = TYPE_FAT32;
	
	
	
	/*
	if (fsb->fat_type = TYPE_FAT32 && fsb->bpb32.fsinfo != 0)
	{
		fsb->fsinfo_valid = TRUE;
	}
	
		
	if (fsb->blkreq.feature_flags & FEATUREF_REMOVABLE)
		fsb->removable = TRUE
	else
		fsb->removable = FALSE;
	
	if (fsb->blkreq.write_protect)
		fsb->write_protect = TRUE;
	else
		fsb->write_protect = FALSE;
	*/
	
	
	if (fsb->fsinfo_valid)
		fsb->start_search_cluster = fsb->fsi.next_free;
	else
		fsb->start_search_cluster = 2;
	
	
}



