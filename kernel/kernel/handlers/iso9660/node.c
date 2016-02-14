#include <kernel/types.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/lists.h>
#include <kernel/buffers.h>
#include "iso9660.h"




/*
 */

struct CDNode *CDFindNode (struct CDSB *cdsb, struct ISODirEntry *idir)
{
	struct CDNode *node;
	
		
	node = LIST_HEAD (&cdsb->node_list);
	
	while (node != NULL)
	{
		if (node->extent_start == Iso733(idir->extent))
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

void CDInitRootNode (struct CDSB *cdsb, struct ISODirEntry *root_idir)
{
	struct CDNode *node;
	
		
	node = &cdsb->root_node;
	
	node->extent_start = Iso733(root_idir->extent);
	node->size = Iso733(root_idir->size);
	node->flags = ISO_DIRECTORY;

	/*	CDSetTime (cdsb, node, idir, ST_ATIME | ST_MTIME | ST_CTIME); */
	
	node->cdsb = cdsb;
	node->reference_cnt = 1;
		
	LIST_INIT (&node->filp_list);
}



/*
 * AllocNode();
 */

struct CDNode *CDAllocNode (struct CDSB *cdsb, struct ISODirEntry *idir)
{
	struct CDNode *node;
	
		
	if ((node = KMalloc (sizeof (struct CDNode))) != NULL)
	{
		node->extent_start = Iso733 (idir->extent);
		node->size = Iso733 (idir->size);
		node->flags = idir->flags[0];
		
		node->cdsb = cdsb;
		node->reference_cnt = 1;

	/*	CDSetTime (cdsb, node, idir, ST_ATIME | ST_MTIME | ST_CTIME); */
		
		LIST_ADD_TAIL (&cdsb->node_list, node, node_entry);
		LIST_INIT (&node->filp_list);
	}
	
	return node;
}




/*
 * CDFreeNode();
 */

void CDFreeNode (struct CDSB *cdsb, struct CDNode *node)
{
	node->reference_cnt--;

		
	if (node == &cdsb->root_node)
	{
		return;
	}
	
	if (node->reference_cnt == 0)
	{	
		LIST_REM_ENTRY (&cdsb->node_list, node, node_entry);
		KFree (node);
	}
}



