#include <kernel/types.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/lists.h>
#include <kernel/error.h>
#include <kernel/buffers.h>
#include <kernel/dbg.h>
#include "fat.h"




/* FatDirRead();
 * offset is idx of direntry in directory
 *
 * Check calling code uses right values/increments filp->offset correctly.
 *
 * DirRead - Only read a single Dirent ?????
 * DirWrite - Writes a single Dirent
 *
 *
 * FIX:  returns 1, 0 and -1  confusing
 * 
 */

int FatDirRead (struct FatNode *node, void *buf, off_t offset, uint32 *r_sector, uint32 *r_sector_offset)
{
	struct FatSB *fsb;
	uint32 cluster;
	uint32 sector;
	uint32 cluster_offset;
	uint32 sector_offset;


	fsb = node->fsb;
				
	if (node == &fsb->root_node && (fsb->fat_type == TYPE_FAT12 || fsb->fat_type == TYPE_FAT16))
	{
		if (offset < fsb->bpb.root_entries_cnt)
		{
			sector = fsb->bpb.reserved_sectors_cnt +
				(fsb->bpb.fat_cnt * fsb->sectors_per_fat) + ((offset * FAT_DIRENTRY_SZ)/512);
			sector_offset = (offset * FAT_DIRENTRY_SZ) % 512;
			
			
			if (ReadBlocks (fsb, &kernel_as, buf, sector, sector_offset, FAT_DIRENTRY_SZ) == 0)
			{
				if (r_sector != NULL)
					*r_sector = sector;
				if (r_sector_offset != NULL)
					*r_sector_offset = sector_offset;

				FatSetTime (fsb, &node->dirent, ST_ATIME);
				 
				return 1;
			}
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if (FindCluster (fsb, node, offset * FAT_DIRENTRY_SZ, &cluster) == 0)
		{
			cluster_offset = (offset * FAT_DIRENTRY_SZ) % (fsb->bpb.sectors_per_cluster * 512);
			sector = ClusterToSector(fsb, cluster) + (cluster_offset / 512);
			sector_offset = (offset * FAT_DIRENTRY_SZ) % 512;
		
			if (ReadBlocks (fsb, &kernel_as, buf, sector, sector_offset, FAT_DIRENTRY_SZ) == 0)
			{
				if (r_sector != NULL)
					*r_sector = sector;
				if (r_sector_offset != NULL)
					*r_sector_offset = sector_offset;
				
				FatSetTime (fsb, &node->dirent, ST_ATIME);
				 
				return 1;
			}
			else
				return -1;
		}
		else
			return 0;
	}
	
	return -1;
}





/*
 *
 */

int FatCreateDirEntry (struct FatNode *parent, struct FatDirEntry *dirent, uint32 *r_sector, uint32 *r_sector_offset)
{
	struct FatSB *fsb;
	uint32 cluster;
	uint32 sector;
	uint32 cluster_offset;
	uint32 sector_offset;
	uint32 offset;
	struct FatDirEntry current_dirent;
	
	
	fsb = parent->fsb;
	offset = 0;
		
	if (parent == &fsb->root_node && (fsb->fat_type == TYPE_FAT12 || fsb->fat_type == TYPE_FAT16))
	{
		while (offset < fsb->bpb.root_entries_cnt)
		{
			sector = fsb->bpb.reserved_sectors_cnt +
				(fsb->bpb.fat_cnt * fsb->sectors_per_fat) + ((offset * FAT_DIRENTRY_SZ)/512);
			sector_offset = (offset * FAT_DIRENTRY_SZ) % 512;
			
			if (ReadBlocks (fsb, &kernel_as, &current_dirent, sector, sector_offset, FAT_DIRENTRY_SZ) == 0)
			{
				if (current_dirent.name[0] == DIRENTRY_FREE || current_dirent.name[0] == DIRENTRY_DELETED)
				{
					WriteBlocks (fsb, &kernel_as, dirent, sector, sector_offset, FAT_DIRENTRY_SZ, BUF_IMMED);
					
					if (r_sector != NULL)
						*r_sector = sector;
					if (r_sector_offset != NULL)
						*r_sector_offset = sector_offset;

					return 1;
				}
			}
			else
			{
				return -1;
			}
			
			offset++;
		}
		
		return 0;		
	}
	else
	{
		while (FindCluster (fsb, parent, offset * FAT_DIRENTRY_SZ, &cluster) == 0)
		{
			cluster_offset = (offset * FAT_DIRENTRY_SZ) % (fsb->bpb.sectors_per_cluster * 512);
			sector = ClusterToSector(fsb, cluster) + (cluster_offset / 512);
			sector_offset = (offset * FAT_DIRENTRY_SZ) % 512;
			
			
			if (ReadBlocks (fsb, &kernel_as, &current_dirent, sector, sector_offset, FAT_DIRENTRY_SZ) == 0)
			{
				if (current_dirent.name[0] == DIRENTRY_FREE || current_dirent.name[0] == DIRENTRY_DELETED)
				{
					WriteBlocks (fsb, &kernel_as, dirent, sector, sector_offset, FAT_DIRENTRY_SZ, BUF_IMMED);
					
					if (r_sector != NULL)
						*r_sector = sector;
					if (r_sector_offset != NULL)
						*r_sector_offset = sector_offset;

					return 1;
				}
				
			}
			else
			{
				return -1;
			}
				
			offset ++;
		}
		
		
		if (AppendCluster (parent, &cluster) == 0)
		{
			ClearCluster (fsb, cluster);
			
			sector = ClusterToSector(fsb, cluster);
			sector_offset = 0;
			
			WriteBlocks (fsb, &kernel_as, dirent, sector, sector_offset, FAT_DIRENTRY_SZ, BUF_IMMED);

			if (r_sector != NULL)
				*r_sector = sector;
			if (r_sector_offset != NULL)
				*r_sector_offset = sector_offset;
			
			return 1;
		}
		else
		{
			return 0;
		}
	}
	
	return -1;
}




/*
 *
 */

void FatDeleteDirEntry (struct FatSB *fsb, uint32 sector, uint32 sector_offset)
{
	struct FatDirEntry dirent;
	
	
	if (ReadBlocks (fsb, &kernel_as, &dirent, sector, sector_offset, FAT_DIRENTRY_SZ) == 0)
	{
		dirent.name[0] = DIRENTRY_DELETED;

		WriteBlocks (fsb, &kernel_as, &dirent, sector, sector_offset, FAT_DIRENTRY_SZ, BUF_IMMED);
	}
}
















