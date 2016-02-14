#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/device.h>
#include <kernel/timer.h>
#include <kernel/sync.h>
#include <kernel/block.h>
#include <kernel/callback.h>




/*
 * Constants
 */


#define CMD_LOOKUP						0
#define CMD_CREATE						1
#define CMD_DELETE						2
#define CMD_RENAME						4

#define FAT_TIME_CREATE					0
#define FAT_TIME_MODIFY					1
#define FAT_TIME_ACCESS					2

#define TYPE_FAT12						0
#define TYPE_FAT16						1
#define TYPE_FAT32						2

#define ATTR_READ_ONLY					0x01
#define ATTR_HIDDEN						0x02
#define ATTR_SYSTEM						0x04
#define ATTR_VOLUME_ID					0x08
#define ATTR_DIRECTORY					0x10
#define ATTR_ARCHIVE					0x20
#define ATTR_LONG_FILENAME				(ATTR_READ_ONLY | ATTR_HIDDEN |	ATTR_SYSTEM | ATTR_VOLUME_ID)

#define CLUSTER_FREE					0x00000000
#define CLUSTER_ALLOC_MIN				0x00000001
#define CLUSTER_ALLOC_MAX				0x0ffffff6
#define CLUSTER_BAD						0x0ffffff7
#define CLUSTER_EOC						0x0fffffff

#define FAT12_CLUSTER_FREE				0x00000000
#define FAT12_CLUSTER_ALLOC_MIN			0x00000001
#define FAT12_CLUSTER_ALLOC_MAX			0x00000ff6
#define FAT12_CLUSTER_BAD				0x00000ff7
#define FAT12_CLUSTER_EOC_MIN			0x00000ff8
#define FAT12_CLUSTER_EOC_MAX			0x00000fff
#define FAT12_CLUSTER_EOC				0x00000fff

#define FAT16_CLUSTER_FREE				0x00000000
#define FAT16_CLUSTER_ALLOC_MIN			0x00000001
#define FAT16_CLUSTER_ALLOC_MAX			0x0000fff6
#define FAT16_CLUSTER_BAD				0x0000fff7
#define FAT16_CLUSTER_EOC_MIN			0x0000fff8
#define FAT16_CLUSTER_EOC_MAX			0x0000ffff
#define FAT16_CLUSTER_EOC				0x0000ffff

#define FAT32_CLUSTER_FREE				0x00000000
#define FAT32_CLUSTER_ALLOC_MIN			0x00000001
#define FAT32_CLUSTER_ALLOC_MAX			0x0ffffff6
#define FAT32_CLUSTER_BAD				0x0ffffff7
#define FAT32_CLUSTER_EOC_MIN			0x0ffffff8
#define FAT32_CLUSTER_EOC_MAX			0x0fffffff
#define FAT32_CLUSTER_EOC				0x0fffffff

#define DIRENTRY_FREE					0x00
#define DIRENTRY_DELETED				0xe5
#define DIRENTRY_LONG					0xe5

#define FAT32_RESVD_SECTORS						32
#define FAT16_ROOT_DIR_ENTRIES  				512
#define FAT32_BOOT_SECTOR_BACKUP_SECTOR_START	6
#define FAT32_BOOT_SECTOR_BACKUP_SECTOR_CNT		3
#define BPB_EXT_OFFSET							36
#define FAT16_BOOTCODE_START 					0x3e
#define FAT32_BOOTCODE_START 					0x5a
#define SIZEOF_FAT32_BOOTCODE   				134
#define SIZEOF_FAT16_BOOTCODE   				134

#define FSINFO_LEAD_SIG	0x41615252
#define FSINFO_STRUC_SIG	0x61417272
#define FSINFO_TRAIL_SIG	0xaa550000

#define FAT_DIRENTRY_SZ							32


#define ST_CTIME								(1<<0)
#define ST_MTIME								(1<<1)
#define ST_ATIME								(1<<2)




/*
 * Structures
 */
 
struct FatBPB 
{
	uint8	jump[3];
	char	oem_name[8];
	uint16	bytes_per_sector;
	uint8	sectors_per_cluster;
	uint16	reserved_sectors_cnt;
	uint8	fat_cnt;
	uint16	root_entries_cnt;
	uint16	total_sectors_cnt16;
	uint8	media_type;
	uint16	sectors_per_fat16;
	uint16	sectors_per_track;
	uint16	heads_per_cylinder;
	uint32	hidden_sectors_cnt;
	uint32	total_sectors_cnt32;
  
} __attribute__ (( __packed__ ));




/*
 *
 */

struct FatBPB_16Ext
{
	uint8	drv_num;
	uint8	reserved1;
	uint8	boot_sig;
	uint32	volume_id;
	uint8	volume_label[11];
	uint8	filesystem_type[8];

} __attribute__ (( __packed__ ));




/*
 *
 */

struct FatBPB_32Ext
{
  	uint32	sectors_per_fat32;	
  	uint16	ext_flags;	
  	uint16	fs_version;
  	uint32	root_cluster;
  	uint16	fs_info;
  	uint16	boot_sector_backup;
  	uint32	reserved[12];
  	uint8	drv_num;
  	uint8	reserved1;
	uint8	boot_sig;
	uint32	volume_id;
	uint8	volume_label[11];
	uint8	filesystem_type[8];
	
} __attribute__ (( __packed__ ));
			
			
			
		

struct FatFSInfo
{
	uint32 lead_sig;
	uint32 reserved1[120];
	uint32 struc_sig;
	uint32 free_cnt;
	uint32 next_free;
	uint32 reserved2[3];
	uint32 trail_sig;

} __attribute__ (( __packed__ ));




/*
 *
 */

struct FatDirEntry
{
  unsigned char		name[8];
  unsigned char		extension[3];
  uint8		attributes;
  uint8		reserved;
  uint8		creation_time_sec_tenths;
  uint16	creation_time_2secs;
  uint16	creation_date;
  uint16	last_access_date;
  uint16	first_cluster_hi;

  uint16	last_write_time;
  uint16	last_write_date;

  uint16	first_cluster_lo;
  uint32	size;
}  __attribute__ (( __packed__ ));




/*
 *
 */

struct FatNode
{
	struct FatSB *fsb;
	LIST_ENTRY(FatNode) node_entry;
	uint32 dirent_sector;
	uint32 dirent_offset;
	struct FatDirEntry dirent;
	
	uint32 hint_cluster;			/* Seek hint, hint_cluster = 0 for invalid hint */
	uint32 hint_offset;
	
	LIST (FatFilp) filp_list;
	int32 reference_cnt;
};




/*
 *
 */

struct FatSB
{
	/* struct Mutex mutex; */
	
	int pid;
	
	struct MsgPort *msgport;
	int init_error;

	int fat_type;
	int reference_cnt;  /* Prevents unmounting/formatting > 0 */

	struct FatBPB bpb;
	struct FatBPB_16Ext bpb16;
	struct FatBPB_32Ext bpb32;
	struct FatFSInfo fsi;
	
	int diskchange_signal;
	int flush_signal;
	
	struct Device *device;
	void *unitp;
	
	struct MountEnviron *me;
	
	struct Mount *device_mount;
		
	uint32 features;
	
	/* Where is it initialized? */
	uint32 total_sectors;
	
	struct Buf *buf;
	
	LIST (FatNode) node_list;  		/* What?  list of open files/dirs? */
	
	LIST (FatFilp) active_filp_list;
	LIST (FatFilp) invalid_filp_list;
	
/*	int inhibited; */
	int validated;					/* Format() could ignore validated state? (except not-present) */
	bool write_protect;				/* Needs checking/setting */
	int removable;
	int writable;
	
	bool fsinfo_valid;
		
	int partition_start;			/* Partition start/end */
	int partition_size;
	uint32 total_sectors_cnt;
	
	uint32 sectors_per_fat;			/* ????????????? */
	uint32 data_sectors;			/* ???????????? Used or not ? */
	uint32 cluster_cnt;				/* ???????????? Used or not ? */
	
	uint32 first_data_sector;		/* Computed at BPB validation */
	uint32 root_dir_sectors;		/* Computed at BPB validation */
	uint32 first_root_dir_sector;	/* Computed at BPB validation */
	uint32 start_search_cluster;
	
	struct FatNode root_node;
	
	uint32 search_start_cluster; 	 /* ????????? free fat entries? */
	uint32 last_cluster;         	 /* ????????? last fat entry,  not is FatBPB ?? */
	
	struct Callback callback;
};





/*
 *
 */

struct FatFilp
{
	struct Device *device;
	struct FatNode *node;
	struct FatSB *fsb;
	
	uint32 offset;
	
	LIST_ENTRY(FatFilp) fsb_filp_entry;
	LIST_ENTRY(FatFilp) node_filp_entry;
	
	int invalid;
	
	int reference_cnt;
	int append_mode;
};




/*
 *
 */

struct FatLookup
{
	uint32 cmd;

	struct FatSB *fsb;
	char *pathname;
	
	char *last_component;
	off_t dirent_offset;
	struct FatNode *node;
	struct FatNode *parent;
};




/*
 * Fat Formatting tables
 */

struct Fat12BPBSpec
{ 
   uint16 bytes_per_sector;			/* sector size */ 
   uint8 sectors_per_cluster;		/* sectors per cluster */ 
   uint16 reserved_sectors_cnt;		/* reserved sectors */ 
   uint8 fat_cnt;					/* FATs */ 
   uint16 root_entries_cnt;			/* root directory entries */ 
   uint16 total_sectors_cnt16;		/* total sectors */ 
   uint8 media_type;				/* media descriptor */ 
   uint16 sectors_per_fat16;		/* sectors per FAT */ 
   uint16 sectors_per_track;		/* sectors per track */ 
   uint16 heads_per_cylinder;		/* drive heads */ 
};




/*
 *
 */

struct FatDskSzToSecPerClus
{
	uint32 disk_size;
	uint32 sectors_per_cluster;
};















/*
 * Global variables
 */
extern struct Device fat_device;


extern struct Fat12BPBSpec fat12_bpb[];
extern struct FatDskSzToSecPerClus dsksz_to_spc_fat16[];
extern struct FatDskSzToSecPerClus dsksz_to_spc_fat32[];


extern char fat_no_name_label[11];
extern char fat12_filesystem_type_str[8];
extern char fat16_filesystem_type_str[8];
extern char fat32_filesystem_type_str[8];		

extern uint8 fat32_bootcode[];
extern uint8 fat16_bootcode[];




/*
 *
 */
 
int fat_init (void *elf);
void *fat_expunge (void);
int fat_opendevice (int unit, void *ioreq, uint32 flags);
int fat_closedevice (void *ioreq);
void fat_beginio (void *ioreq);
int fat_abortio (void *ioreq);



 
/*
 *
 */
 
int32 FatTask (void *arg);
void FatTaskInit (struct FatSB *fsb);
void FatTaskFini (struct FatSB *fsb);

void FatDoInhibit (struct FSReq *fsreq);
void FatDoUninhibit (struct FSReq *fsreq);

void FatDoOpen (struct FSReq *fsreq);
void FatDoClose (struct FSReq *fsreq);
void FatDoDup (struct FSReq *fsreq);
void FatDoRead (struct FSReq *fsreq);
void FatDoWrite (struct FSReq *fsreq);
void FatDoSeek (struct FSReq *fsreq);
void FatDoOpendir (struct FSReq *fsreq);
void FatDoReaddir (struct FSReq *fsreq);
void FatDoRewinddir (struct FSReq *fsreq);
void FatDoFstat (struct FSReq *fsreq);
void FatDoStat (struct FSReq *fsreq);
void FatDoTruncate (struct FSReq *fsreq);
void FatDoMkdir (struct FSReq *fsreq);
void FatDoRmdir (struct FSReq *fsreq);
void FatDoUnlink (struct FSReq *fsreq);
void FatDoFormat (struct FSReq *fsreq);
void FatDoRename (struct FSReq *fsreq);
void FatDoChmod (struct FSReq *fsreq);
void FatDoChown (struct FSReq *fsreq);




/*
 *
 */

int FatLookup (struct FatLookup *lookup);
struct FatNode *FatSearchDir (struct FatSB *fsb, struct FatNode *dirnode, char *component);
bool CompareDirEntry (struct FatDirEntry *dirent, char *comp);

int FatDirEntryToASCIIZ (char *pathbuf, struct FatDirEntry *dirent);
int FatASCIIZToDirEntry (struct FatDirEntry *dirent, char *filename);
int FatIsDosName (char *component);




/*
 *
 */

struct FatNode *FindNode (struct FatSB *fsb, uint32 sector, uint32 offset);
void InitRootNode (struct FatSB *fsb);
struct FatNode *AllocNode (struct FatSB *fsb, struct FatDirEntry *dirent, uint32 sector, uint32 offset);
void FreeNode (struct FatSB *fsb, struct FatNode *node);




/*
 */

int ReadFATEntry (struct FatSB *fsb, uint32 cluster, uint32 *r_value);
int WriteFATEntry (struct FatSB *fatsb, uint32 cluster, uint32 value);

int AppendCluster (struct FatNode *node, uint32 *r_cluster);
int FindFreeCluster (struct FatSB *fsb, uint32 *r_cluster);
int FindCluster (struct FatSB *fsb, struct FatNode *node, off_t offset, uint32 *r_cluster);
int FindLastCluster (struct FatNode *node, uint32 *r_cluster);

uint32 GetFirstCluster (struct FatSB *fsb, struct FatDirEntry *dirent);
void SetFirstCluster (struct FatSB *fsb, struct FatDirEntry *dirent, uint32 cluster);

void FreeClusters (struct FatSB *fatsb, uint32 first_cluster);
uint32 ClusterToSector (struct FatSB *fatsb, uint32 cluster);




/*
 */

err32 ReadSector (struct FatSB *fsb, void **buf, int32 sector);
err32 WriteSector (struct FatSB *fsb, int32 sector, int type);

err32 ReadBlocks (struct FatSB *fsb, struct AddressSpace *as, void *buf, uint32 block, uint32 offset, uint32 nbytes);
err32 WriteBlocks (struct FatSB *fsb, struct AddressSpace *as, void *buf, uint32 block, uint32 offset, uint32 nbytes, int type);
err32 GetBlock (struct FatSB *fsb, uint32 block, void **buf);
err32 PutBlock (struct FatSB *fsb, uint32 block, int type);

int DoReadBlock (struct FatSB *fsb, void *addr, uint32 block);
int DoWriteBlock (struct FatSB *fsb, void *addr, uint32 block);
void FatInvalidateCache (struct FatSB *fsb);;




/*
 */

struct FatNode *FatCreateDir (struct FatSB *fsb, struct FatNode *parent, char *name);
struct FatNode *FatCreateFile (struct FatSB *fsb, struct FatNode *parent, char *name);

int FatDeleteDir (struct FatSB *fsb, struct FatNode *parent, struct FatNode *node);
int FatDeleteFile (struct FatSB *fsb, struct FatNode *parent, struct FatNode *node);
int IsDirEmpty (struct FatNode *node);

int FatTruncateFile (struct FatNode *node, size_t size);
int FatExtendFile (struct FatNode *node, size_t length);




/*
 */

void FatToEpochTime (struct TimeVal *tv, uint16 date, uint16 time, uint8 milliten);
void EpochToFatTime (struct TimeVal *tv, uint16 *date, uint16 *time, uint8 *milliten);
void FatSetTime (struct FatSB *fsb, struct FatDirEntry *dirent, uint32 update);




/*
 */

int FlushDirent (struct FatSB *fsb, struct FatNode *node);
int FlushSB (struct FatSB *fsb);
void FlushFSInfo (struct FatSB *fsb);


int FatIsValid (struct FatSB *fsb, struct FSReq *fsreq);
int FatRevalidate (struct FatSB *fsb, int skip_validation);
void FatInvalidate (struct FatSB *fsb);
int FatDoMediaPresent (struct FatSB *fsb, int *media_state);
int FatValidateBPB (struct FatSB *fsb, uint32 sector_cnt);
int FatStatDevice (struct FatSB *fsb, struct BlockDeviceStat *stat);



int FatFormat (struct FatSB *fsb, char *label, uint32 flags, uint32 cluster_size);
int InitializeFatSB (struct FatSB *fsb, struct FatDirEntry *label_dirent, uint32 flags, uint32 cluster_size);
int FatEraseDisk (struct FatSB *fsb, uint32 flags);
int FatWriteBootRecord (struct FatSB *fsb);
int FatInitFATs (struct FatSB *fsb);
int FatInitRootDirectory (struct FatSB *fsb, struct FatDirEntry *label_dirent);

void FatPrecalculateFSBValues (struct FatSB *fsb);




/*
 */

size_t FatFileRead (struct FatNode *node, void *buf, size_t count, off_t offset, struct FSReq *fsreq);
size_t FatFileWrite (struct FatNode *node, void *buf, size_t count, off_t offset, struct FSReq *fsreq);
size_t FatInternalWriteFile (struct FatNode *node, void *buf, size_t count, off_t offset, struct FSReq *fsreq);

int FatDirRead (struct FatNode *node, void *buf, off_t offset, uint32 *r_sector, uint32 *r_sector_offset);

int FatCreateDirEntry (struct FatNode *parent, struct FatDirEntry *dirent, uint32 *r_sector, uint32 *r_sector_offset);
void FatDeleteDirEntry (struct FatSB *fsb, uint32 sector, uint32 sector_offset);




/*
 * **** Identical ?
 */

void FileOffsetToSectorOffset (struct FatNode *node, off_t file_offset, uint32 *r_sector, uint32 *r_sec_offset);
int CalcDirentLocation (struct FatNode *node, off_t offset, int32 *node_sector, int32 *node_offset);


int ClearCluster (struct FatSB *fsb, uint32 cluster);




void FatAddCallback (struct FatSB *fsb);
void FatRemCallback (struct FatSB *fsb);
void FatDiskChangeCallback (void *arg);


int CreateVolumeMount (struct FatSB *fsb);
int RemoveVolumeMount (struct FatSB *fsb);
int FatLabelToASCIIZ (struct FatSB *fsb, char *dst);
int ASCIIZToFatLabel (char *dst, char *src);
char *CreateFatLabel (char *src, int n);


