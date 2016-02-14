#include <kernel/types.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/lists.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include <kernel/buffers.h>
#include "iso9660.h"




/*
 *
 */

size_t CDFileRead (struct CDNode *node, void *dst, size_t count, off_t offset, struct FSReq *fsreq)
{
	struct CDSB *cdsb;
	uint32 sector;
	uint32 sector_offset;
	
	cdsb = node->cdsb;
	
	if (offset >= node->size)
		count = 0;
	else if (count < (node->size - offset))
		count = count;
	else
		count = node->size - offset;
	
	
	sector_offset = offset % 2048;
	sector = node->extent_start + offset / 2048;
			
	if (BufReadBlocks (cdsb->buf, dst, fsreq->as, sector, sector_offset, count) == 0)
		return count;
	else
		return 0;
}





