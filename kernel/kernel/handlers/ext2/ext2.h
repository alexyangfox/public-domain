#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/device.h>
#include <kernel/timer.h>
#include <kernel/sync.h>




/*
 * Constants
 */


#define CMD_LOOKUP						0
#define CMD_CREATE						1
#define CMD_DELETE						2
#define CMD_RENAME						4




/*
 *
 */

struct Ext2Node
{
	struct Ext2SB *sb;
	LIST_ENTRY(Ext2Node) node_entry;
};





/*
 *
 */

struct Ext2
{
	struct Ext2SB sb;
	
	uint32	blocksize;
	uint32	nblock;
	uint32	ngroup;
	uint32	inospergroup;
	uint32	blockspergroup;
	uint32	inosperblock;
	uint32	groupaddr;
	uint32	descperblock;
	uint32	firstblock;

	
	
	
	int reference_cnt;  /* Prevents unmounting/formatting > 0 */

	struct Device *device;
	uint32 features;
	uint32 total_sectors;
		
	LIST (Ext2Node) node_list;
	
	struct Mutex mutex;

	int validated;					/* Format() could ignore validated state? (except not-present) */
	bool write_protect;				/* Needs checking/setting */
	bool removable;
	bool fsinfo_valid;
		
	int partition_start;			/* Partition start/end */
	int partition_size;
	uint32 total_sectors_cnt;

	uint32 sectors_per_fat;			/* ????????????? */
	uint32 data_sectors;			/* ???????????? Used or not ? */
	uint32 cluster_cnt;				/* ???????????? Used or not ? */

	struct Ext2Node root_node;
	
	int cache_type;
	int32 buffer_cnt;
	struct Ext2Buffer *buffer;
	LIST (Ext2Buffer) lru_buf_list;
	LIST (Ext2Buffer) free_buf_list;
	LIST (Ext2Buffer) hash_buf_list[MAX_CACHE_HASH];
};





/*
 *
 */

struct Ext2Filp
{
	struct Mount *mount;
	struct Ext2Node *node;
	struct Ext2SB *sb;
	
	uint32 offset;
	LIST_ENTRY(Ext2Filp) filp_entry;
	int reference_cnt;
	int append_mode;
};




/*
 *
 */

struct Ext2Lookup
{
	uint32 cmd;

	struct Ext2SB *sb;
	char *pathname;
	
	char *last_component;

	struct Ext2Node *node;
	struct Ext2Node *parent;
};






/* Ext2 constants */

#define MIN_DIRENT_SIZE		8
#define	GROUP_SIZE			32
#define INODE_SIZE			128

#define NAMELEN				255
#define DIRLEN(namlen)		(((namlen)+8+3)&~3)

#define	BYTESPERSEC			512
#define SBOFF				1024
#define SBSIZE				1024

#define SUPERMAGIC			0xEF53
#define MINBLOCKSIZE		1024
#define MAXBLOCKSIZE		4096
#define ROOTINODE			2
#define FIRSTINODE			11
#define VALIDFS				0x0001
#define ERRORFS				0x0002

#define NDIRBLOCKS			12
#define INDBLOCK			NDIRBLOCKS
#define DINDBLOCK			(INDBLOCK+1)
#define TINDBLOCK			(DINDBLOCK+1)
#define NBLOCKS				(TINDBLOCK+1)


/* Permissions in Inode.mode */
 
#define IEXEC = 00100,
#define IWRITE = 0200,
#define IREAD = 0400,
#define ISVTX = 01000,
#define ISGID = 02000,
#define ISUID = 04000,


/* Type in Inode.mode */

#define IFMT = 0170000,
#define IFIFO = 0010000,
#define IFCHR = 0020000,
#define IFDIR = 0040000,
#define IFBLK = 0060000,
#define IFREG = 0100000,
#define IFLNK = 0120000,
#define IFSOCK = 0140000,
#define IFWHT = 0160000




/*
 * Super block
 */

struct Ext2SB
{
	uint32	ninode;			/* Inodes count */
	uint32	nblock;			/* Blocks count */
	uint32	rblockcount;	/* Reserved blocks count */
	uint32	freeblockcount;	/* Free blocks count */
	uint32	freeinodecount;	/* Free inodes count */
	uint32	firstdatablock;	/* First Data Block */
	uint32	logblocksize;	/* Block size */
	uint32	logfragsize;	/* Fragment size */
	uint32	blockspergroup;	/* Blocks per group */
	uint32	fragpergroup;	/* Fragments per group */
	uint32	inospergroup;	/* Inodes per group */
	uint32	mtime;			/* Mount time */
	uint32	wtime;			/* Write time */
	uint16	mntcount;		/* Mount count */
	uint16	maxmntcount;	/* Maximal mount count */
	uint16	magic;			/* Magic signature */
	uint16	state;			/* File system state */
	uint16	errors;			/* Behaviour when detecting errors */
	uint16	pad;
	uint32	lastcheck;		/* time of last check */
	uint32	checkinterval;	/* max. time between checks */
	uint32	creatoros;		/* OS */
	uint32	revlevel;		/* Revision level */
	uint16	defresuid;		/* Default uid for reserved blocks */
	uint16	defresgid;		/* Default gid for reserved blocks */
	uint32	reserved[235];	/* Padding to the end of the block */
};




/*
 * Block group
 */

struct Group
{
	uint32	bitblock;		/* Blocks bitmap block */
	uint32	inodebitblock;	/* Inodes bitmap block */
	uint32	inodeaddr;		/* Inodes table block */
	uint16	freeblockscount;/* Free blocks count */
	uint16	freeinodescount;/* Free inodes count */
	uint16	useddirscount;	/* Directories count */
};




/*
 * Inode
 */

struct Inode
{
	uint16	mode;			/* File mode */
	uint16	uid;			/* Owner Uid */
	uint32	size;			/* Size in bytes */
	uint32	atime;			/* Access time */
	uint32	ctime;			/* Creation time */
	uint32	mtime;			/* Modification time */
	uint32	dtime;			/* Deletion Time */
	uint16	gid;			/* Group Id */
	uint16	nlink;			/* Links count */
	uint32	nblock;			/* Blocks count */
	uint32	flags;			/* File flags */
	uint32	block[NBLOCKS];	/* Pointers to blocks */
	uint32	version;		/* File version (for NFS) */
	uint32	fileacl;		/* File ACL */
	uint32	diracl;			/* Directory ACL or high size bits */
	uint32	faddr;			/* Fragment address */
};




/*
 * Directory entry
 */
struct Dirent
{
	uint32	ino;		/* Inode number */
	uint16	reclen;		/* Directory entry length */
	uint8	namlen;		/* Name length */
	char	*name;		/* File name */
};















/*
 * Global variables
 */

extern struct Device ext2_handler;




/*
 *
 */

int ext2_init (void);
int ext2_exit (void);
int ext2_opendevice (void *ioreq, uint32 flags);
int ext2_closedevice (void *ioreq);
void ext2_beginio (void *ioreq);
int ext2_abortio (void *ioreq);

