#include <kernel/types.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/lists.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include "fat.h"



/*
 *
 */

int FatTruncateFile (struct FatNode *node, size_t size)
{
	uint32 cluster;
	struct FatSB *fsb;
	
	fsb = node->fsb;
	
	
	if (fsb->write_protect == TRUE)
	{
		SetError (EROFS);
		return -1;
	}
		
	
	if (size == 0)
	{
		cluster = GetFirstCluster (node->fsb, &node->dirent);
		
		if (cluster != CLUSTER_EOC || cluster != CLUSTER_BAD);
		{	
			FreeClusters (node->fsb, cluster);
			
			SetFirstCluster (node->fsb, &node->dirent, CLUSTER_EOC);
		
			node->dirent.size = 0;
			FatSetTime (fsb, &node->dirent, ST_MTIME);
		
			node->hint_cluster = 0;
			node->hint_offset = 0;
		
			return FlushDirent (fsb, node);
		}
	}
	else
	{
		if (FindCluster (fsb, node, size, &cluster) == 0)
		{
			if ((size % (fsb->bpb.sectors_per_cluster * 512)) == 0)
			{				
				if (FindCluster (fsb, node, size -1, &cluster) == 0)
				{
					FreeClusters (fsb, cluster);
					node->dirent.size = size;
					FatSetTime (fsb, &node->dirent, ST_MTIME);					
					
					node->hint_cluster = 0;
					node->hint_offset = 0;
							
					return FlushDirent (fsb, node);
				}
			}
			else
			{
				FreeClusters (fsb, cluster);
				node->dirent.size = size;
				
				node->hint_cluster = 0;
				node->hint_offset = 0;			

				FatSetTime (fsb, &node->dirent, ST_MTIME);
				return FlushDirent (fsb, node);
			}
		}
	}
	
	
	return -1;
}




/*
 * Can't replace with GrowFile, as that doesn't clear sectors
 * or alter the file length.
 */

int FatExtendFile (struct FatNode *node, size_t length)
{
	struct FatSB *fsb;
	
	fsb = node->fsb;

	if (fsb->write_protect == TRUE)
	{
		SetError (EROFS);
		return -1;
	}

	return FatFileWrite (node, NULL, 0, length, 0);
}




