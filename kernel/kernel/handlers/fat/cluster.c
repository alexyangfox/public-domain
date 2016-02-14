#include <kernel/types.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/lists.h>
#include <kernel/buffers.h>
#include "fat.h"


/*
 * ReadFATEntry();
 *
 * Modify to use 32-bit values for args and result.
 *
 * Things that call ReadEntry WriteEntry need better error checking.
 *
 * Use multiple FATs
 */
 
int ReadFATEntry (struct FatSB *fsb, uint32 cluster, uint32 *r_value)
{
	int32 sector, sector_offset;
	uint32 fat_offset, alignment;
	uint16 word_value;
	uint32 long_value;
	int f;
	uint32 fat_sz;
	
	
	for (f=0; f < fsb->bpb.fat_cnt; f++)
	{
		if (fsb->fat_type == TYPE_FAT12)
		{
			fat_sz = fsb->bpb.sectors_per_fat16;
			fat_offset = cluster + cluster/2;
			alignment = cluster % 2;
			
			sector = fsb->bpb.reserved_sectors_cnt + (fat_offset / 512) + (f * fat_sz);
			sector_offset = fat_offset % 512;
			
			
			if (ReadBlocks (fsb, &kernel_as, &word_value, sector, sector_offset, 2) != 0)
				break;
			
			if (alignment == 1)
				word_value = word_value >> 4;
			else
				word_value = word_value & 0x0fff;
				
			if (word_value >= FAT12_CLUSTER_EOC_MIN && word_value <= FAT12_CLUSTER_EOC_MAX)
				*r_value = CLUSTER_EOC;
			else if (word_value == FAT12_CLUSTER_BAD)
				*r_value = CLUSTER_BAD;
			else
				*r_value = word_value;
			
			return 0;
		}
		else if (fsb->fat_type == TYPE_FAT16)
		{
			fat_sz = fsb->bpb.sectors_per_fat16;
			fat_offset = cluster * 2;
						
			sector = fsb->bpb.reserved_sectors_cnt + (fat_offset / 512) + (f * fat_sz);
			sector_offset = fat_offset % 512;
			
			
			if (ReadBlocks (fsb, &kernel_as, &word_value, sector, sector_offset, 2) != 0)
				break;
						
			if (word_value >= FAT16_CLUSTER_EOC_MIN)
				*r_value = CLUSTER_EOC;
			else if (word_value == FAT16_CLUSTER_BAD)
				*r_value = CLUSTER_BAD;
			else
				*r_value = word_value;
			
			return 0;
		}
		else
		{
			fat_sz = fsb->bpb32.sectors_per_fat32;
			fat_offset = cluster * 4;
			
			sector = fsb->bpb.reserved_sectors_cnt + (fat_offset / 512)  + (f * fat_sz);
			sector_offset = fat_offset % 512;
			
			
			if (ReadBlocks (fsb, &kernel_as, &long_value, sector, sector_offset, 4) != 0)
				break;
									
			if (long_value >= FAT32_CLUSTER_EOC_MIN && long_value <= FAT32_CLUSTER_EOC_MAX)
				*r_value = CLUSTER_EOC;
			else if (long_value == FAT32_CLUSTER_BAD)
				*r_value = CLUSTER_BAD;
			else
				*r_value = long_value;
			
			return 0;
		}
	}
	
	return -1;
}




/*
 * WriteFATEntry();
 */
 
int WriteFATEntry (struct FatSB *fsb, uint32 cluster, uint32 value)
{
	uint16 word_value;
	uint32 long_value;
	int32 sector, sector_offset;
	uint32 fat_offset, alignment;
	int f;
	int fat_written_cnt = 0;
	uint32 fat_sz;
	
	for (f=0; f < fsb->bpb.fat_cnt; f++)
	{
	 	if (fsb->fat_type == TYPE_FAT12)
		{
			/* FIXME: Do a check if cluster == 0 or cluster == 1, special case clusters */
			
			if (value == CLUSTER_EOC)
				value = FAT12_CLUSTER_EOC;
			else if (value == CLUSTER_BAD)
				value = FAT12_CLUSTER_BAD;
			
			
			fat_sz = fsb->bpb.sectors_per_fat16;
			fat_offset = cluster + cluster/2;
			alignment = cluster % 2;
			
			sector = fsb->bpb.reserved_sectors_cnt + (fat_offset / 512)  + (f * fat_sz);
			sector_offset = fat_offset % 512;
		
			
			if (ReadBlocks (fsb, &kernel_as, &word_value, sector, sector_offset, 2) != 0)
				break;
				
			if (alignment == 1)
				word_value = ((value << 4) & 0xfff0) | (word_value & 0x000f);
			else
				word_value = (value & 0x0fff) | (word_value & 0xf000);
			
			if (WriteBlocks (fsb, &kernel_as, &word_value, sector, sector_offset, 2, BUF_IMMED) != 0)
				break;
						
			fat_written_cnt ++;
		}
		else if (fsb->fat_type == TYPE_FAT16) 
		{
			/* FIXME: Do a check if cluster == 0 or cluster == 1, special case clusters */
			
			if (value == CLUSTER_EOC)
				value = FAT16_CLUSTER_EOC;
			else if (value == CLUSTER_BAD)
				value = FAT16_CLUSTER_BAD;
			
			fat_sz = fsb->bpb.sectors_per_fat16;
			fat_offset = cluster * 2;
			
			sector = fsb->bpb.reserved_sectors_cnt + (fat_offset / 512)  + (f * fat_sz);
			sector_offset = fat_offset % 512;
			
			word_value = value;
			
			if (WriteBlocks (fsb, &kernel_as, &word_value, sector, sector_offset, 2, BUF_IMMED) != 0)
				break;
			
			fat_written_cnt ++;
		}
		else
		{
			/* FIXME: Do a check if cluster == 0 or cluster == 1, special case clusters */
		
			if (value == CLUSTER_EOC)
				value = FAT32_CLUSTER_EOC;
			else if (value == CLUSTER_BAD)
				value = FAT32_CLUSTER_BAD;
			
			fat_sz = fsb->bpb32.sectors_per_fat32;
			fat_offset = cluster;
			
			sector = fsb->bpb.reserved_sectors_cnt + (fat_offset / 512)  + (f * fat_sz);
			sector_offset = fat_offset % 512;
			
			long_value = value;
			
			if (WriteBlocks (fsb, &kernel_as, &long_value, sector, sector_offset, 4, BUF_IMMED) != 0)
				break;
			
			fat_written_cnt ++;		
		}
	}
	
	
	/* FIXME:  really need primary FAT table to be written to consider it a success */

	if (fat_written_cnt == 0)
		return -1;
	else
		return 0;
}










/*
 * AppendCluster();
 *
 * Clearing the directory cluster may fail, but we carry on regardless
 * 
 */

int AppendCluster (struct FatNode *node, uint32 *r_cluster)
{
	struct FatSB *fsb;
	uint32 last_cluster;
	uint32 cluster;
	
	
	fsb = node->fsb;
		
	if (FindLastCluster(node, &last_cluster) == 0)
	{
		if (FindFreeCluster(fsb, &cluster) == 0)
		{
			if (GetFirstCluster (fsb, &node->dirent) == CLUSTER_EOC)
			{
				SetFirstCluster (fsb, &node->dirent, cluster);
				FlushDirent (fsb, node);
			}
			else
			{
				WriteFATEntry (fsb, last_cluster, cluster);
			}
			
			*r_cluster = cluster;
			return 0;		
		}
	}

	return -1;
}




/*
 * FindLastCluster();
 *
 * Or return -1 on error and return CLUSTER_EOC when there is no first cluster.
 * What about CLUSTER_BAD ?????
 */

int FindLastCluster (struct FatNode *node, uint32 *r_cluster)
{
	struct FatSB *fsb;
	uint32 cluster, next_cluster;

	
	fsb = node->fsb;
	
	
	if (GetFirstCluster (fsb, &node->dirent) == CLUSTER_EOC)
	{
		*r_cluster = CLUSTER_EOC;
		return 0;
	}
	else
	{
		if (node->hint_cluster != 0)
			cluster = node->hint_cluster;
		else
			cluster = GetFirstCluster (fsb, &node->dirent);
		
				
		if (ReadFATEntry (fsb, cluster, &next_cluster) != 0)
			return -1;

		while (next_cluster >= CLUSTER_ALLOC_MIN && next_cluster <= CLUSTER_ALLOC_MAX)
		{	
			cluster = next_cluster;
			
			node->hint_offset += fsb->bpb.sectors_per_cluster * 512;
			node->hint_cluster = cluster;
			
			if (ReadFATEntry (fsb, cluster, &next_cluster) != 0)
				return -1;
		}
						
		*r_cluster = cluster;
		
		return 0;
	}
}




/*
 *
 */

int FindCluster (struct FatSB *fsb, struct FatNode *node, off_t offset, uint32 *r_cluster)
{
	uint32 cluster_size;
	uint32 temp_offset;
	uint32 cluster;
	
	
	cluster_size = fsb->bpb.sectors_per_cluster * 512;
	offset = (offset / cluster_size) * cluster_size;

	if (node->hint_cluster != 0 && offset == node->hint_offset)
	{
		*r_cluster = node->hint_cluster;
		return 0;
	}
	
	if (node->hint_cluster != 0 && offset > node->hint_offset)
	{
		temp_offset = node->hint_offset;
		cluster = node->hint_cluster;
	}
	else
	{
		temp_offset = 0;
		cluster = GetFirstCluster (fsb, &node->dirent);
	}

	
	
	
	while (temp_offset < offset)
	{
		if (cluster < CLUSTER_ALLOC_MIN || cluster > CLUSTER_ALLOC_MAX)
		{	
			return -1;
		}

		if (ReadFATEntry (fsb, cluster, &cluster) != 0)
			return -1;
		
		temp_offset += cluster_size;
	}
	
	if (cluster < CLUSTER_ALLOC_MIN || cluster > CLUSTER_ALLOC_MAX)
	{
		return -1;
	}
	else
	{
		*r_cluster = cluster;
		node->hint_offset = offset;
		node->hint_cluster = cluster;
		return 0;
	}
}














/*
 * GetFirstCluster();
 */

uint32 GetFirstCluster (struct FatSB *fsb, struct FatDirEntry *dirent)
{
	uint32 cluster;
	
	if (fsb->fat_type == TYPE_FAT12)
	{
		cluster = dirent->first_cluster_lo;
		
		if (cluster >= FAT12_CLUSTER_EOC_MIN && cluster <= FAT12_CLUSTER_EOC_MAX)
			return CLUSTER_EOC;
		else if(cluster == FAT12_CLUSTER_BAD)
			return CLUSTER_BAD;
		else
			return cluster;
	}
	else if (fsb->fat_type == TYPE_FAT16)
	{
		cluster = dirent->first_cluster_lo;

		if (cluster >= FAT16_CLUSTER_EOC_MIN && cluster <= FAT16_CLUSTER_EOC_MAX)
			return CLUSTER_EOC;
		else if(cluster == FAT16_CLUSTER_BAD)
			return CLUSTER_BAD;
		else
			return cluster;
	}
	else
	{
		cluster = (dirent->first_cluster_hi<<16) + dirent->first_cluster_lo;
		
		if (cluster >= FAT32_CLUSTER_EOC_MIN && cluster <= FAT32_CLUSTER_EOC_MAX)
			return CLUSTER_EOC;
		else if(cluster == FAT32_CLUSTER_BAD)
			return CLUSTER_BAD;
		else
			return cluster;
	}
}




/*
 * SetFirstCluster();
 *
 * ** ** Write to disk
 */

void SetFirstCluster (struct FatSB *fsb, struct FatDirEntry *dirent, uint32 cluster)
{
	if (fsb->fat_type == TYPE_FAT12)
	{
		if (cluster == CLUSTER_EOC)
			cluster = FAT12_CLUSTER_EOC;
		else if (cluster == CLUSTER_BAD)
			cluster = FAT12_CLUSTER_BAD;
	
		dirent->first_cluster_hi = 0;
		dirent->first_cluster_lo = cluster & 0x00000fff;
	}
	else if (fsb->fat_type == TYPE_FAT16)
	{
		if (cluster == CLUSTER_EOC)
			cluster = FAT16_CLUSTER_EOC;
		else if (cluster == CLUSTER_BAD)
			cluster = FAT16_CLUSTER_BAD;
		
		dirent->first_cluster_hi = 0;
		dirent->first_cluster_lo = cluster & 0x0000ffff;
	}
	else
	{
		if (cluster == CLUSTER_EOC)
			cluster = FAT32_CLUSTER_EOC;
		else if (cluster == CLUSTER_BAD)
			cluster = FAT32_CLUSTER_BAD;
		
		dirent->first_cluster_hi = (cluster >> 16) & 0x0000ffff;
		dirent->first_cluster_lo = cluster & 0x0000ffff;
	}
}




/*
 * FindFreeCluster();
 */

int FindFreeCluster (struct FatSB *fsb, uint32 *r_cluster)
{
	uint32 cluster;
	uint32 value;
	
	
	for (cluster = fsb->start_search_cluster; cluster < fsb->cluster_cnt; cluster ++)
	{
		if (ReadFATEntry (fsb, cluster, &value) == 0)
		{
			if (value == CLUSTER_FREE)
			{
				if (WriteFATEntry (fsb, cluster, CLUSTER_EOC) == 0)
				{
					*r_cluster = cluster;
					fsb->start_search_cluster = cluster;
					
					if (fsb->fsinfo_valid)
					{
						fsb->fsi.next_free = cluster;
						fsb->fsi.free_cnt --;
						FlushFSInfo (fsb);
					}
					
					return 0;
				}
			}
		}
	}
	

	for (cluster = 2; cluster < fsb->start_search_cluster; cluster ++)
	{
		if (ReadFATEntry (fsb, cluster, &value) == 0)
		{
			if (value == CLUSTER_FREE)
			{
				if (WriteFATEntry (fsb, cluster, CLUSTER_EOC) == 0)
				{
					*r_cluster = cluster;
					fsb->start_search_cluster = cluster;
					
					if (fsb->fsinfo_valid)
					{
						fsb->fsi.next_free = cluster;
						fsb->fsi.free_cnt --;
						FlushFSInfo (fsb);
					}
					
					return 0;
				}
			}
		}
	}


	return -1;
}




/*
 * FreeClusters();
 *
 * Modify so that it takes a node as a parameter,  only free upto official
 * filesize cluster.  But remember we may not be freeing clusters starting
 * from the beginning of a file.
 */

void FreeClusters (struct FatSB *fsb, uint32 first_cluster)
{
	uint32 cluster, next_cluster;
	
	cluster = first_cluster;
	
	while (cluster >= CLUSTER_ALLOC_MIN && cluster <= CLUSTER_ALLOC_MAX)
	{
		if (ReadFATEntry (fsb, cluster, &next_cluster) != 0)
			return;
		
		if (WriteFATEntry (fsb, cluster, CLUSTER_FREE) != 0)
			return;

		cluster = next_cluster;
		
		if (fsb->fsinfo_valid)
			fsb->fsi.free_cnt ++;
	}
	
	FlushFSInfo (fsb);
}




/*
 * ClusterToSector();
 */

uint32 ClusterToSector (struct FatSB *fsb, uint32 cluster)
{
	uint32 sector;
	
	sector = ((cluster - 2) * fsb->bpb.sectors_per_cluster) + fsb->first_data_sector;
	
	return sector;
}




/*
 * FIX  Should return an error???????  Or is that only possible with buggy code
 *
 * Usually used during Lookup to find the sector/offset of a Dirent so it
 * assumes the file length-cluster is valid at that point.
 */

void FileOffsetToSectorOffset (struct FatNode *node, off_t file_offset,
									uint32 *r_sector, uint32 *r_sec_offset)
{
	uint32 cluster;
	struct FatSB *fsb;
	uint32 base_sector;
	uint32 cluster_offset;
	uint32 sector_in_cluster;
	
	
	fsb = node->fsb;
	
	if (node == &fsb->root_node && (fsb->fat_type == TYPE_FAT12 || fsb->fat_type == TYPE_FAT16))
	{
		*r_sector = fsb->bpb.reserved_sectors_cnt +
				(fsb->bpb.fat_cnt * fsb->sectors_per_fat) + (file_offset/512);
	
		*r_sec_offset = file_offset % 512;
	}
	else if (FindCluster (fsb, node, file_offset, &cluster) == 0)
	{
		base_sector = ClusterToSector(fsb, cluster);
		
		cluster_offset = file_offset % (fsb->bpb.sectors_per_cluster * 512);
		sector_in_cluster = cluster_offset / 512;
		
		*r_sector = base_sector + sector_in_cluster;
		*r_sec_offset = file_offset % 512;
	}
	else
	{
		KPANIC ("File Offset to Sector Offset FAILURE, shouldn't happen");
	}
}




/*
 * ClearCluster();
 *
 * Attempts to clear a cluster, returns 0 on success.
 */
 
int ClearCluster (struct FatSB *fsb, uint32 cluster)
{
	uint32 sector;
	int c;
	uint8 clear_sector[512];
		
	MemSet (clear_sector, 0, 512);
			
	sector = ClusterToSector (fsb, cluster);
				
	for (c=0; c < fsb->bpb.sectors_per_cluster; c++)
	{
		if (WriteBlocks (fsb, &kernel_as, clear_sector, sector +c, 0, 512, BUF_IMMED) != 0)
			return -1;
	}
	
	return 0;
}




