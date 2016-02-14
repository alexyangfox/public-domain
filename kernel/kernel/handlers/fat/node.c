#include <kernel/types.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/lists.h>
#include <kernel/buffers.h>
#include "fat.h"




/*
 * ObtainNode();
 *
 * HAven't a clue, assume searches incore list for a given node based on
 * cluster and offset.  Then copies dirent to the node if the node is new.
 * Strange.
 */

struct FatNode *FindNode (struct FatSB *fsb, uint32 sector, uint32 offset)
{
	struct FatNode *node;
	
	
	node = LIST_HEAD (&fsb->node_list);
	
	while (node != NULL)
	{
		if (node->dirent_sector == sector && node->dirent_offset == offset)
		{
			KASSERT (node->reference_cnt > 0);
			
			node->reference_cnt ++;
			
			break;
		}
		
		node = LIST_NEXT(node, node_entry);
	}
	
	return node;
}




/*
 *
 */

void InitRootNode (struct FatSB *fsb)
{
	struct FatNode *node;
	
	
	node = &fsb->root_node;
	
	MemSet (&node->dirent, 0, sizeof (struct FatDirEntry));

	node->dirent.attributes = ATTR_DIRECTORY;

	FatSetTime (fsb, &node->dirent, ST_ATIME | ST_MTIME | ST_CTIME);

	node->fsb = fsb;
	node->reference_cnt = 1;
	node->dirent_sector = 0;
	node->dirent_offset = 0;
		
	node->hint_cluster = 0;
	node->hint_offset = 0;
		
	LIST_INIT (&node->filp_list);
}



/*
 * AllocNode();
 */

struct FatNode *AllocNode (struct FatSB *fsb, struct FatDirEntry *dirent, uint32 sector, uint32 offset)
{
	struct FatNode *node;
	
	
	if ((node = KMalloc (sizeof (struct FatNode))) != NULL)
	{
		MemCpy (&node->dirent, dirent, sizeof (struct FatDirEntry));

		node->fsb = fsb;
		node->reference_cnt = 1;
		node->dirent_sector = sector;
		node->dirent_offset = offset;
		
		node->hint_cluster = 0;
		node->hint_offset = 0;
		
		LIST_ADD_TAIL (&fsb->node_list, node, node_entry);
		LIST_INIT (&node->filp_list);
	}
	
	return node;
}




/*
 * FreeNode();
 */

void FreeNode (struct FatSB *fsb, struct FatNode *node)
{
	node->reference_cnt--;
	
	if (node == &fsb->root_node)
	{
		return;
	}
	
	FlushDirent (fsb, node);
	
	if (node->reference_cnt == 0)
	{	
		LIST_REM_ENTRY (&fsb->node_list, node, node_entry);
		KFree (node);
	}
}




/*
 * FlushNode();
 */

int FlushDirent (struct FatSB *fsb, struct FatNode *node)
{
	fsb = node->fsb;
		
	if (fsb->validated == FALSE || node == &fsb->root_node || fsb->write_protect == TRUE)
		return 0;

	WriteBlocks (fsb, &kernel_as, &node->dirent, node->dirent_sector, node->dirent_offset, sizeof (struct FatDirEntry), BUF_IMMED);
	
	return 0;
}




/*
 * Not needed anywhere
 */

int FlushFSB (struct FatSB *fsb)
{
	return 0;
}




/*
 * Optimize write of FSInfo
 */

void FlushFSInfo (struct FatSB *fsb)
{
}




