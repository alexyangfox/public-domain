#include <kernel/types.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/lists.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include "fat.h"




/*
 * Returned EEXISTS for some reason even though it succeed in creating directory.
 * Wrapper function problem?  Or errno not being set/cleared?
 */

struct FatNode *FatCreateDir (struct FatSB *fsb, struct FatNode *parent, char *name)
{
	struct FatDirEntry dot, dotdot;
	struct FatDirEntry dirent;
	uint32 sector;
	uint32 sector_offset;
	struct FatNode *node;	
	int t;
	
	KPRINTF ("FatCreateDir()");

	if (fsb->write_protect == TRUE)
	{
		SetError (EROFS);
		return NULL;
	}

	if (FatASCIIZToDirEntry (&dirent, name) != 0)
	{
		SetError (EINVAL);
		return NULL;
	}
		
	dirent.attributes = ATTR_DIRECTORY;
	dirent.reserved = 0;
	dirent.size = 0;
	FatSetTime (fsb, &dirent, ST_CTIME | ST_ATIME | ST_MTIME);
	SetFirstCluster (fsb, &dirent, CLUSTER_EOC);
	
			
	if (FatCreateDirEntry (parent, &dirent, &sector, &sector_offset) == 1)
	{
		if ((node = AllocNode (fsb, &dirent, sector, sector_offset)) != NULL)
		{
			dot.attributes = ATTR_DIRECTORY;
			dot.reserved = 0;
			dot.first_cluster_hi = 0;
			dot.first_cluster_lo = 0;
			dot.size = 0;
			FatSetTime (fsb, &dot, ST_CTIME | ST_ATIME | ST_MTIME);
			SetFirstCluster (fsb, &dot, GetFirstCluster (fsb, &node->dirent));
			
			dot.name[0] = '.';
			for (t=1; t<8; t++)
				dot.name[t] = ' ';
			for (t=0; t<3; t++)
				dot.extension[t] = ' ';
			
			
			
			dotdot.attributes = ATTR_DIRECTORY;
			dotdot.reserved = 0;
			dotdot.first_cluster_hi = 0;
			dotdot.first_cluster_lo = 0;
			dotdot.size = 0;
			FatSetTime (fsb, &dotdot, ST_CTIME | ST_ATIME | ST_MTIME);
			
			if (parent == &fsb->root_node)
				SetFirstCluster (fsb, &dotdot, 0);
			else
				SetFirstCluster (fsb, &dotdot, GetFirstCluster (fsb, &parent->dirent));

			dotdot.name[0] = '.';
			dotdot.name[1] = '.';
			for (t=2; t<8; t++)
				dotdot.name[t] = ' ';
			for (t=0; t<3; t++)
				dotdot.extension[t] = ' ';

			
			if (FatCreateDirEntry (node, &dot, NULL, NULL) == 1 && FatCreateDirEntry (node, &dotdot, NULL, NULL) == 1)
		 	{
				FatSetTime (fsb, &parent->dirent, ST_CTIME | ST_ATIME | ST_MTIME);
				FlushDirent (fsb, parent);
				return node;
			}

			FreeClusters (fsb, GetFirstCluster (fsb, &node->dirent));
			FreeNode (fsb, node);
		}
				
		FatDeleteDirEntry (fsb, sector, sector_offset);
		
	}
		
	return NULL;
}




/*
 *
 * If write fails, clear cached dirent
 */
 
struct FatNode *FatCreateFile (struct FatSB *fsb, struct FatNode *parent, char *name)
{
	uint32 sector;
	uint32 sector_offset;
	struct FatDirEntry dirent;
	struct FatNode *node;	

	KPRINTF ("FatCreateFile()");
	
	if (fsb->write_protect == TRUE)
	{
		SetError (EROFS);
		return NULL;
	}

	if (FatASCIIZToDirEntry (&dirent, name) != 0)
	{
		SetError (EINVAL);
		return NULL;
	}

		
	dirent.attributes = 0;
	dirent.reserved = 0;
	dirent.first_cluster_hi = 0;
	dirent.first_cluster_lo = 0;
	dirent.size = 0;
	FatSetTime (fsb, &dirent, ST_CTIME | ST_ATIME | ST_MTIME);
	SetFirstCluster (fsb, &dirent, CLUSTER_EOC);
	
	
	if (FatCreateDirEntry (parent, &dirent, &sector, &sector_offset) == 1)
	{
		if ((node = AllocNode (fsb, &dirent, sector, sector_offset)) != NULL)
		{
			FatSetTime (fsb, &parent->dirent, ST_CTIME | ST_ATIME | ST_MTIME);
			FlushDirent (fsb, parent);
			return node;
		}
		
		FatDeleteDirEntry (fsb, sector, sector_offset);
	}
		
	return NULL;
}








