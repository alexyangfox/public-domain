#include <kernel/types.h>
#include <kernel/utility.h>
#include <kernel/device.h>
#include <kernel/dbg.h>
#include <kernel/kmalloc.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/error.h>
#include <kernel/buffers.h>
#include <kernel/block.h>
#include "fat.h"



uint8 fat_sector_buffer[512];



/*
 * ReadBlocks();
 *
 * ****** real_block = block + fsb->partition_start 
 */

err32 ReadBlocks (struct FatSB *fsb, struct AddressSpace *as, void *addr, uint32 block, uint32 offset, uint32 nbytes)
{
	return BufReadBlocks (fsb->buf, addr, as, fsb->partition_start + block, offset, nbytes);
		
	/* FIXME: Invalidate FSB if read fails */
}




/*
 * WriteBlocks();
 */

err32 WriteBlocks (struct FatSB *fsb, struct AddressSpace *as, void *addr, uint32 block, uint32 offset, uint32 nbytes, int type)
{
	return BufWriteBlocks (fsb->buf, addr, as, fsb->partition_start + block, offset, nbytes, type);
}



void FatInvalidateCache(struct FatSB *fsb)
{
	InvalidateBuf (fsb->buf);
	
	/* FIXME: Invalidate FSB if read fails */
}


