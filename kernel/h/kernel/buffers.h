#ifndef KERNEL_BUFFERS_H
#define KERNEL_BUFFERS_H

#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/block.h>
#include <kernel/callback.h>



#define BUF_HASH_CNT			64



struct Buf
{
	struct Device *device;
	void *unitp;
		
	uint32 buffer_cnt;
	uint32 block_size;
	uint32 lba_start;
	uint32 lba_end;
	int writethru_critical;
	uint32 writeback_delay;
	uint32 max_transfer;
	
	
	struct Blk *blk_table;
	void *blk_mem;
	
	
	LIST (Blk) lru_list;
	LIST (Blk) dirty_list;
	LIST (Blk) free_list;
	LIST (Blk) hash_list[BUF_HASH_CNT];
};




/*
 * struct Blk
 */

struct Blk
{
	uint32 sector;
	int32 dirty;
	struct Buf *buf;
	void *mem;
	bool in_use;
	
	LIST_ENTRY (Blk) lru_entry;
	LIST_ENTRY (Blk) dirty_entry;
	LIST_ENTRY (Blk) free_entry;
	LIST_ENTRY (Blk) hash_entry;
};



/*
 * Write mode flags
 */

#define BUF_IMMED				(1<<0)
#define BUF_ONESHOT				(1<<1)



/*
 * Prototypes
 */

struct Buf *CreateBuf (struct Device *device, void *unitp, uint32 buffer_cnt, uint32 block_size,
						uint32 lba_start, uint32 lba_end, int writethru_critical, uint32 writeback_delay,
						uint32 max_transfer);

void FreeBuf (struct Buf *buf);
void SyncBuf (struct Buf *buf);
void InvalidateBuf (struct Buf *buf);

int BufReadBlocks (struct Buf *buf, void *addr, struct AddressSpace *as,
						uint32 block, uint32 offset, uint32 nbytes);
int BufWriteBlocks (struct Buf *buf, void *addr, struct AddressSpace *as,
						uint32 block, uint32 offset, uint32 nbytes, int mode);

struct Blk *BufGetBlock (struct Buf *buf, uint32 block);
struct Blk *BufPutBlock (struct Buf *buf, struct Blk *blk, int mode);







#endif


