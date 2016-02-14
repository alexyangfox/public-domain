#ifndef KERNEL_BLOCK_H
#define KERNEL_BLOCK_H

#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/sync.h>
#include <kernel/msg.h>
#include <kernel/config.h>
#include <kernel/callback.h>
#include <kernel/iov.h>




/*
 *
 */

struct BlkReq
{
	struct Msg msg;
	struct Device *device;
	void *unitp;
	uint32 flags;
	
	int32 cmd;
	int rc;
	int error;     /* IOERR Error state */
	struct AddressSpace *as;
	struct StdIOReq *abort_ioreq;
	
	/* ---- Unique block-device data ---- */
	
	void *buf;
	uint32 sector;
	uint32 nsectors;
	uint32 size; /* Is this for returning size of disk? */


	/* ATAPI Packet/SCSI command fields */
	
	uint8 *cmd_packet_addr;
	int cmd_packet_nbytes;
	void *data_buf_addr;
	int data_nbytes;
	int dir;
	uint32 lba;

	/* ----------------------------------- */

	struct BlockDeviceStat *stat;
	struct Callback *callback;
};




/*
 * struct BlockDeviceStat;
 */

struct BlockDeviceStat
{
	int media_state;
	int diskchange_cnt;
	int write_protect;
	uint32 features;
	uint32 total_sectors;
	int class_type;
	struct TimeVal ctime;
};




/*
 *
 */

#define SECTOR_SZ				512




/* 
 * stat->media_state
 */

#define MEDIA_INSERTED			1
#define MEDIA_REMOVED			0




/*
 * struct Partition
 */

struct Partition
{
	uint8 state;			
	uint8 start_head;
	uint16 start_cylsec;
	uint8 type;
	uint8 end_head;
	uint16 end_cylsec;
	uint32 start_lba;
	uint32 size;			/* In sectors? */
} __attribute__ (( __packed__ ));





/*
 *
 */
 
#define BLK_CMD_READ                3
#define BLK_CMD_WRITE               4
#define BLK_CMD_FLUSH               5
#define BLK_CMD_MEDIA_PRESENT       6
#define BLK_CMD_MEDIA_REVALIDATE  	7	/* Maybe same as getgeometry ? */
#define BLK_CMD_MEDIA_LOCK          8	/* For CD-ROMs etc where media change can be controlled */
#define BLK_CMD_MEDIA_UNLOCK        9

#define BLK_CMD_ADD_CALLBACK		10
#define BLK_CMD_REM_CALLBACK		11

#define BLK_CMD_SCSI				12



/*
 * CD-ROM error codes
 */

#define CDERR_NOSENSE			0
#define CDERR_RECOVERED			1
#define CDERR_NOT_READY			2
#define CDERR_MEDIUM			3
#define CDERR_HARDWARE			4
#define CDERR_ILLEGAL			5
#define CDERR_ATTENTION			6
#define CDERR_PROTECT			7
#define CDERR_ABORT				11
#define CDERR_MISCOMPARE		14








#endif
