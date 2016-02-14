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
 *
 */

size_t FatFileRead (struct FatNode *node, void *buf, size_t count, off_t offset, struct FSReq *fsreq)
{
	struct FatSB *fsb;
	uint32 cluster;
	uint32 nbytes_read;
	uint32 transfer_nbytes;
	uint32 sector;
	uint32 cluster_offset;
	uint32 sector_offset;
	uint8 *dst;
	
	
	fsb = node->fsb;
	
	KASSERT (buf >= (void *)0x1000);
	KASSERT (fsb != (void *)0x1000);		
		
	if (offset >= node->dirent.size)
		count = 0;
	else if (count < (node->dirent.size - offset))
		count = count;
	else
		count = node->dirent.size - offset;
	
	dst = buf;
	nbytes_read = 0;
	
	while (nbytes_read < count)
	{	
		/*
		 * Fix, assuming Cluster is 512 bytes.
		 * Should also read nclusters to see if they're contiguous.
		 */
		 
		if (FindCluster (fsb, node, offset, &cluster) != 0)
			break;
		
		cluster_offset = offset % (fsb->bpb.sectors_per_cluster * 512);
		sector = ClusterToSector(fsb, cluster) + (cluster_offset / 512);
		
		
		sector_offset = offset % 512;
		
		transfer_nbytes = (512 < (count - nbytes_read)) ? 512 : (count - nbytes_read);
		transfer_nbytes = (transfer_nbytes < (512 - sector_offset))
						? transfer_nbytes : (512 - sector_offset);
		
		
		if (ReadBlocks (fsb, fsreq->as, dst, sector, sector_offset, transfer_nbytes) != 0)
			break;
		
		
		dst += transfer_nbytes;
		nbytes_read += transfer_nbytes;
		offset += transfer_nbytes;
	}
	
	FatSetTime (fsb, &node->dirent, ST_ATIME);
	
	return nbytes_read;
}



		
/*
 * FatFileWrite()
 */
 
size_t FatFileWrite (struct FatNode *node, void *buf, size_t count, off_t offset, struct FSReq *fsreq)
{
	struct FatSB *fsb;
	uint32 cluster;
	uint32 sector;
	uint32 cluster_offset;
	uint32 transfer_nbytes;
	uint32 sector_offset;
	uint8 *src;
	uint32 nbytes_written;
	uint32 nbytes_to_clear;

		
	fsb = node->fsb;
	
	if (fsb->write_protect == TRUE)
	{
		SetError (EROFS);
		return -1;
	}
	
	
	/* Clear from end of file to end of last cluster */
	
	if (offset > node->dirent.size)
	{
		nbytes_to_clear = (fsb->bpb.sectors_per_cluster * 512) - (node->dirent.size % (fsb->bpb.sectors_per_cluster * 512));
		
		if (FindCluster (fsb, node, node->dirent.size, &cluster) == 0)
		{	
			cluster_offset = offset % (fsb->bpb.sectors_per_cluster * 512);
			sector = ClusterToSector(fsb, cluster) + (cluster_offset / 512);
			sector_offset = offset % 512;
			
			if (WriteBlocks (fsb, fsreq->as, NULL, sector, sector_offset, nbytes_to_clear, 0) != 0)
				return -1;
		}
	}

	
	
	src = buf;
	nbytes_written = 0;

	while (nbytes_written < count)
	{
		while (FindCluster (fsb, node, offset, &cluster) != 0)
		{
			if (AppendCluster (node, &cluster) != 0)
				return nbytes_written;
		}
		
		cluster_offset = offset % (fsb->bpb.sectors_per_cluster * 512);
		sector = ClusterToSector(fsb, cluster) + (cluster_offset / 512);
		sector_offset = offset % 512;
		
		transfer_nbytes = (512 < (count - nbytes_written)) ? 512 : (count - nbytes_written);
		transfer_nbytes = (transfer_nbytes < (512 - sector_offset))
							? transfer_nbytes : (512 - sector_offset);
				
		if (WriteBlocks (fsb, fsreq->as, src, sector, sector_offset, transfer_nbytes, 0) != 0)
			return nbytes_written;
		
		
					
		src += transfer_nbytes;
		offset += transfer_nbytes;
		nbytes_written += transfer_nbytes;		
	}



	
	
	if (offset > node->dirent.size)
		node->dirent.size = offset;
		
	FatSetTime (fsb, &node->dirent, ST_MTIME | ST_CTIME);
	
	/* FIXME:  Really need a FatFlushNode() */
	
	
	return nbytes_written;
}











