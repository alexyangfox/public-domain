#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/proc.h>
#include <kernel/vm.h>
#include <kernel/dbg.h>
#include <kernel/error.h>
#include <kernel/kmalloc.h>
#include <kernel/proc.h>
#include <kernel/buffers.h>



/*
 * A replacement for the global cache implemented as a driver, cache.device.
 * This is a per-mount fixed-size cache invoked by filesystem handler tasks such
 * as fat.handler and cdrom.handler.
 *
 * Currently unused by handlers,  above handlers need modifying.
 */

static int BufReadSector (struct Buf *buf, struct Blk *blk, uint32 sector);
static int BufWriteSector (struct Buf *buf, struct Blk *blk, int mode);



/*
 * FIXME:  Need option to override write-mode here.  Force BUF_IMMED always.
 *
 * Replace args with MountEnviron ???????????
 */

struct Buf *CreateBuf (struct Device *device, void *unitp, uint32 buffer_cnt, uint32 block_size,
						uint32 lba_start, uint32 lba_end, int writethru_critical, uint32 writeback_delay,
						uint32 max_transfer)
{
	struct Buf *buf;
	int t;
	
	
	if (buffer_cnt == 0 || block_size < 512)
		return NULL;
		
	if ((buf = KMalloc (sizeof (struct Buf))) != NULL)
	{
		if ((buf->blk_table = KMalloc (sizeof (struct Blk) * buffer_cnt)) != NULL)
		{
			if ((buf->blk_mem = KMalloc (buffer_cnt * block_size)) != NULL)
			{
				buf->device = device;
				buf->unitp = unitp;
				
				buf->buffer_cnt = buffer_cnt;
				buf->block_size = block_size;
				buf->lba_start = lba_start;
				buf->lba_end = lba_end;
				buf->writethru_critical = writethru_critical;
				buf->writeback_delay = writeback_delay;
				buf->max_transfer = max_transfer;
				
				LIST_INIT (&buf->lru_list);
				LIST_INIT (&buf->dirty_list);
				LIST_INIT (&buf->free_list);
				
								
				for (t=0; t < BUF_HASH_CNT; t++)
				{
					LIST_INIT (&buf->hash_list[t]);
				}
					
				for (t=0; t < buffer_cnt; t++)
				{
					buf->blk_table[t].buf = buf;
					buf->blk_table[t].sector = -1;
					buf->blk_table[t].dirty = FALSE;
					buf->blk_table[t].mem = (uint8 *)buf->blk_mem + (t * block_size);
					buf->blk_table[t].in_use = FALSE;
													
					LIST_ADD_TAIL(&buf->free_list, &buf->blk_table[t], free_entry);
				}
				
				return buf;
			}
		}
	}
	
	return NULL;
}




/*
 *
 */

void FreeBuf (struct Buf *buf)
{
	KFree (buf->blk_mem);
	KFree (buf->blk_table);
	KFree (buf);
}




/*
 * SyncBuf();
 *
 * Flush all dirty blocks out to disk.  Intended to be called by the handler task
 * in response to a periodic timer of several seconds.
 */

void SyncBuf (struct Buf *buf)
{
	struct Blk *blk;
	
	while ((blk = LIST_HEAD (&buf->dirty_list)) != NULL)
	{
		blk->dirty = FALSE;
		LIST_REM_HEAD (&buf->dirty_list, dirty_entry);
							
		BufWriteSector (buf, blk, BUF_IMMED);
	}
}




/*
 *
 */

void InvalidateBuf (struct Buf *buf)
{
	struct Blk *blk;
	int hash_idx;
	int t;
	
	
	for (t=0; t < buf->buffer_cnt; t++)
	{
		blk = &buf->blk_table[t];
		
		if (blk->in_use == TRUE)
		{
			hash_idx = blk->sector % BUF_HASH_CNT;
		
			LIST_REM_ENTRY (&buf->lru_list, blk, lru_entry);
			LIST_REM_ENTRY (&buf->hash_list[hash_idx], blk, hash_entry);
			LIST_ADD_HEAD (&buf->free_list, blk, free_entry);
			
			blk->in_use = FALSE;
			blk->dirty = FALSE;
			blk->sector = 0;
		}
	}
}




/*
 * BufReadBlocks();
 */

int BufReadBlocks (struct Buf *buf, void *addr, struct AddressSpace *as,
						uint32 block, uint32 offset, uint32 nbytes)
{
	struct Blk *blk;
	uint32 nbytes_to_read;
	uint32 remaining_in_block;
	void *cache_buf;
	int rc = 0;
	
	
	/* Need to limit read upto end of media */

	while (nbytes > 0)
	{
		remaining_in_block = buf->block_size - offset;
		
		nbytes_to_read = (nbytes < remaining_in_block) ? nbytes : remaining_in_block;
		
		if ((blk = BufGetBlock (buf, block)) == NULL)
		{
			KPRINTF ("BufGetBlock(block = %d) failed", block);
			rc = -1;
			break;
		}
		
		cache_buf = blk->mem;
	
		if (CopyOut (as, addr, (uint8 *)cache_buf + offset, nbytes_to_read) != 0)
		{
			KPRINTF ("Buf  CopyOut() FAIL");
			
			rc = -1;
			break;
		}
		
		nbytes -= nbytes_to_read;
		addr += nbytes_to_read;
					
		block++;
		offset = 0;
	}
		
	return rc;
}




/*
 * WriteBlocks();
 */

int BufWriteBlocks (struct Buf *buf, void *addr, struct AddressSpace *as,
						uint32 block, uint32 offset, uint32 nbytes, int mode)

{
	struct Blk *blk;
	uint32 nbytes_to_write;
	uint32 remaining_in_block;
	void *cache_buf;
	int rc = 0;
	
	
	while (nbytes > 0)
	{
		remaining_in_block = buf->block_size - offset;
		
		nbytes_to_write = (nbytes < remaining_in_block) ? nbytes : remaining_in_block;
		
		
		if ((blk = BufGetBlock (buf, block)) == NULL)
		{
			rc = -1;
			break;
		}
		
		cache_buf = blk->mem;
		
		
		if (addr != NULL)
		{
			if (CopyIn (as, (uint8 *)cache_buf + offset, addr, nbytes_to_write) != 0)
			{
				rc = -1;
				break;
			}
		}
		else
		{
			MemSet ((uint8 *)cache_buf + offset, 0, nbytes_to_write); 
		}
		
		if (BufPutBlock (buf, blk, mode) == NULL)
		{
			rc = -1;
			break;
		}
		
		nbytes -= nbytes_to_write;
		addr += nbytes_to_write;
		block++;
		offset = 0;
	}

	return rc;
}




/*
 * GetBlock();
 */

struct Blk *BufGetBlock (struct Buf *buf, uint32 block)
{
	struct Blk *blk;
	uint32 hash_idx;
	int rc;
	
	/* Need to limit read upto end of media, final block may be partial,
	 * simply use disksize%blocksize for final block? */
		
		
	hash_idx = block % BUF_HASH_CNT;

	blk = LIST_HEAD (&buf->hash_list[hash_idx]);

		
	while (blk != NULL)
	{
		KASSERT (blk->sector != -1);
		
		if (blk->sector == block)
			break;
		
		blk = LIST_NEXT (blk, hash_entry);
	}
	
			
	if (blk == NULL)
	{
		blk = LIST_HEAD (&buf->free_list);
		
				
		if (blk == NULL)
		{
			blk = LIST_TAIL (&buf->lru_list);
			
			KASSERT (blk->sector != -1);
			
			if (blk == NULL)
			{
				KPANIC ("fb not found");
				return NULL;
			}
			
							
			if (blk->dirty == TRUE)
			{
				blk->dirty = FALSE;
				LIST_REM_ENTRY (&buf->dirty_list, blk, dirty_entry);
							
				if (BufWriteSector (buf, blk, BUF_IMMED) != 0)
				{
					KPANIC ("BufWriteBlock failure");
					return NULL;
				}
			}
			
						
			hash_idx = blk->sector % BUF_HASH_CNT;
			
			LIST_REM_ENTRY (&buf->lru_list, blk, lru_entry);
			LIST_REM_ENTRY (&buf->hash_list[hash_idx], blk, hash_entry);
			
			blk->in_use = FALSE;
			LIST_ADD_HEAD (&buf->free_list, blk, free_entry);
		}

		
		LIST_REM_HEAD (&buf->free_list, free_entry);
		blk->in_use = TRUE;
		
		LIST_ADD_HEAD (&buf->lru_list, blk, lru_entry);
		blk->sector = block;
		blk->dirty = FALSE;
		blk->buf = buf;
		
				
		hash_idx = blk->sector % BUF_HASH_CNT;
		LIST_ADD_HEAD (&buf->hash_list[hash_idx], blk, hash_entry);
		
				
		rc = BufReadSector (buf, blk, block);
				
		if (rc == 0)
		{
			return blk;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		LIST_REM_ENTRY(&buf->lru_list, blk, lru_entry);
		LIST_ADD_HEAD(&buf->lru_list, blk, lru_entry);
						
		return blk;
	}
	
}




/*
 * PutBlock();
 *
 * "mode" determines where on the LRU list the block is written and if it
 * should be written immediately.
 *
 * Values inside "buf" determine whether a block is written to disk or left
 * for the caller to write blocks to disk by periodically calling SyncBuf().
 *
 * If writeback_delay == 0    write EVERYTHING IMMEDIATELY
 * If writethru_critical == 1  write BUF_IMMED blocks immediately.
 */


struct Blk *BufPutBlock (struct Buf *buf, struct Blk *blk, int mode)
{
	int rc;

	if (mode & BUF_ONESHOT)
	{
		LIST_REM_ENTRY (&buf->lru_list, blk, lru_entry)
		LIST_ADD_TAIL (&buf->lru_list, blk, lru_entry);
	}
	
	

	if (mode & BUF_IMMED)
	{
		if (blk->dirty == TRUE)
		{	
			blk->dirty = FALSE;
			LIST_REM_ENTRY (&buf->dirty_list, blk, dirty_entry);
		}
	
		if (buf->writethru_critical != 0)
		{
			rc = BufWriteSector (buf, blk, BUF_IMMED);
		
			if (rc == 0)
			{
				return blk;
			}
			else if (rc == IOERR_MEDIA_CHANGE || rc == IOERR_NO_MEDIA)
			{
				SetError (ENODEV);
				return NULL;
			}
			else
			{
				SetError (EIO);
				return NULL;
			}
		}
		
		return blk;
	}
	else
	{
		/* Mark as dirty and place on tail of dirty list */
				
		if (blk->dirty != TRUE)
		{
			blk->dirty = TRUE;
			LIST_ADD_TAIL (&buf->dirty_list, blk, dirty_entry);
		}
		
		if (buf->writeback_delay == 0)
		{
			rc = BufWriteSector (buf, blk, BUF_IMMED);
		
			if (rc == 0)
			{
				return blk;
			}
			else if (rc == IOERR_MEDIA_CHANGE || rc == IOERR_NO_MEDIA)
			{
				SetError (ENODEV);
				return NULL;
			}
			else
			{
				SetError (EIO);
				return NULL;
			}
		}
				
		return blk;
	}
}




/*
 *
 */

static int BufReadSector (struct Buf *buf, struct Blk *blk, uint32 sector)
{
	struct BlkReq blkreq;
	
	
	blkreq.device = buf->device;
	blkreq.unitp = buf->unitp;
	
	blkreq.sector = sector;
	blkreq.nsectors = 1;
	blkreq.buf = blk->mem;
		
	blkreq.cmd = BLK_CMD_READ;
	blkreq.as = &kernel_as;
	blkreq.rc = 0;
	
	DoIO (&blkreq, NULL);
	
	if (blkreq.rc != 0)
	{
		if (blkreq.error == IOERR_NO_MEDIA)
		{
			KPRINTF ("********* NO MEDIA");
			SetError (ENODEV);
		}
		else
		{
			KPRINTF ("********* IO ERROR");
			SetError (EIO);
		}
	}
	return blkreq.rc;
}




/*
 *
 */

static int BufWriteSector (struct Buf *buf, struct Blk *blk, int mode)
{		
	struct BlkReq blkreq;
	
	blkreq.device = buf->device;
	blkreq.unitp = buf->unitp;
	
	blkreq.sector = blk->sector;
	blkreq.nsectors = 1;
	blkreq.buf = blk->mem;
		
	blkreq.cmd = BLK_CMD_WRITE;
	blkreq.as = &kernel_as;
	blkreq.rc = 0;
	
	DoIO (&blkreq, NULL);
	return 0;
}



