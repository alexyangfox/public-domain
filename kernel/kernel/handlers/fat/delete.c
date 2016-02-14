#include <kernel/types.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/lists.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include "fat.h"




/*
 * Need parent directory so as to update time fields
 */

int FatDeleteDir (struct FatSB *fsb, struct FatNode *parent, struct FatNode *node)
{
	if (fsb->write_protect == TRUE)
	{
		SetError (EROFS);
		return -1;
	}
	
	
	if (node != &fsb->root_node)
	{
		if (node->dirent.attributes & ATTR_DIRECTORY)
		{
			if (node->reference_cnt == 1)
			{			
				if (IsDirEmpty (node) == 0)
				{
					FreeClusters (fsb, GetFirstCluster (fsb, &node->dirent));
					node->dirent.name[0] = DIRENTRY_DELETED;

					FatSetTime (fsb, &parent->dirent, ST_CTIME | ST_MTIME);
					FlushDirent (fsb, parent);
					FlushDirent (fsb, node);
					FreeNode (fsb, node);
					return 0;
				}
				else
					SetError(EEXIST);
			}
			else
				SetError (EBUSY);
		}
		else
			SetError (ENOTDIR);
	}
	else
		SetError (EINVAL);
	
	FreeNode (fsb, node);
	return -1;
}




/*
 * Don't forget to free parent
 */
 
int FatDeleteFile (struct FatSB *fsb, struct FatNode *parent, struct FatNode *node)
{
	if (fsb->write_protect == TRUE)
	{
		SetError (EROFS);
		return -1;
	}
	
	
	if ((node->dirent.attributes & ATTR_DIRECTORY) == 0)
	{
		if (node->reference_cnt == 1)
		{
			FreeClusters (fsb, GetFirstCluster (fsb, &node->dirent));
			node->dirent.name[0] = DIRENTRY_DELETED;

			FatSetTime (fsb, &parent->dirent, ST_CTIME | ST_MTIME);
			FlushDirent (fsb, parent);
			FlushDirent (fsb, node);
			FreeNode (fsb, node);
			return 0;
		}
		else
			SetError (EBUSY);
	}
	else
		SetError (EISDIR);
	
	FreeNode (fsb, node);
	return -1;
}




/*
 *
 */

int IsDirEmpty (struct FatNode *node)
{
	struct FatSB *fsb;
	struct FatDirEntry dirent;
	char asciiz_name[16];
	int rc;
	off_t offset;
	
	
	fsb = node->fsb;
	offset = 0;
	
	while ((rc = FatDirRead (node, &dirent, offset, NULL, NULL)) == 1)
	{
		offset ++;
		
		if (dirent.name[0] == DIRENTRY_FREE)
			return 0;
		else if (dirent.name[0] == DIRENTRY_DELETED)
			continue;
		else if (dirent.attributes & ATTR_VOLUME_ID)
			continue;
		else
		{
			FatDirEntryToASCIIZ (asciiz_name, &dirent);
			
			if (!((StrCmp ("..", asciiz_name) == 0) || (StrCmp (".", asciiz_name) == 0)))
				return -1;
		}
	}
		
	return rc;
}
