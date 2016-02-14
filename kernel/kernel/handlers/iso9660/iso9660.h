#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/device.h>
#include <kernel/timer.h>
#include <kernel/sync.h>
#include <kernel/callback.h>
#include <kernel/block.h>




/*
 *
 */

struct CDLookup
{
	struct CDSB *cdsb;
	char *pathname;
	
	char *last_component;
	off_t dirent_offset;
	struct CDNode *node;
	struct CDNode *parent;
};




/*
 * Constants
 */


#define ST_CTIME								(1<<0)
#define ST_MTIME								(1<<1)
#define ST_ATIME								(1<<2)




/*
 *
 */

struct TOCHeader
{
	uint8 length[2];
	uint8 first_track;
	uint8 last_track;
};




/*
 *
 */

struct TOCEntry
{
	uint8 resv1;
	uint8 flags;
	uint8 track;
	uint8 resv2;
	uint8 lba_addr[4];
};




/*
 * ISO Volume Descriptor types
 */

#define ISO_VD_BOOT				0
#define ISO_VD_PRIMARY			1
#define ISO_VD_SUPPLEMENTAL		2
#define ISO_VD_PARTITION		3
#define ISO_VD_END				255




/*
 * ISO Primary Volume Descriptor
 */

struct ISOVolDesc
{
	uint8 type[1];						// 711
	uint8 id[5];						// magic
	uint8 version[1];					// 711
	uint8 flags[1];						//
	uint8 system_id[32];				// achar
	uint8 volume_id[32];				// dchar
	uint8 unused1[8];					// 
	uint8 volume_size[8];				// 733
	uint8 escape_sequences[32];			//
	uint8 volume_set_size[4];			// 723
	uint8 volume_sequence_number[4];	// 723
	uint8 block_size[4];				// 723
	uint8 path_table_size[8];			// 733
	uint8 lpath_table[4];				// 731
	uint8 optional_lpath_table[4];		// 731
	uint8 mpath_table[4];				// 732
	uint8 optional_mpath_table[4];		// 732
	uint8 root_directory_entry[34];		// 9.1
	uint8 volume_set[128];				// dchar
	uint8 publisher[128];				// achar
	uint8 preparer[128];				// achar
	uint8 application[128];				// achar
	uint8 notice[37];					// 7.5 dchar
	uint8 abstract[37];					// 7.5 dchar
	uint8 biblio[37];					// 7.5 dchar
	uint8 cdate[17];					// 8.4.26.1
	uint8 mdate[17];					// 8.4.26.1
	uint8 xdate[17];					// 8.4.26.1
	uint8 edate[17];					// 8.4.26.1
	uint8 fsvers[1];					// 711
};




/*
 * ISO Directory Record
 */

struct ISODirEntry
{
	uint8 length[1];		// 711
	uint8 xlength[1];		// 711
	uint8 extent[8];		// 733
	uint8 size[8];			// 733
	uint8 date[7];			// 711[7]
	uint8 flags[1];			//
	uint8 unitsize[1];		// 711
	uint8 gapsize[1];		// 711
	uint8 volseqnum[4];		// 723
	uint8 name_len[1];		// 711
	uint8 name[0];
};




/*
 * ISODirEntry flags
 */

#define ISO_EXISTENCE		(1<<0)
#define ISO_DIRECTORY		(1<<1)   
#define ISO_ASSOCIATED		(1<<2)   
#define ISO_RECORD			(1<<3)   
#define ISO_PROTECTION		(1<<4)   
#define ISO_DRESERVED1		(1<<5)   
#define ISO_DRESERVED2		(1<<6)  
#define ISO_MULTIEXTENT		(1<<7)




/*
 * ISO type conversion inline functions
 */

static inline int Iso711 (uint8 *v);
static inline int Iso712 (uint8 *v);
static inline int Iso721 (uint8 *v);
static inline int Iso722 (uint8 *v);
static inline int Iso723 (uint8 *v);
static inline int Iso731 (uint8 *v);
static inline int Iso732 (uint8 *v);
static inline int Iso733 (uint8 *v);


static inline int Iso711 (uint8 *v)	{return v[0];}
static inline int Iso712 (uint8 *v)	{return (char) v[0];}
static inline int Iso721 (uint8 *v)	{return v[0] | ((char) v[1] << 8);}
static inline int Iso722 (uint8 *v)	{return ((char) v[0] << 8) | v[1];}
static inline int Iso723 (uint8 *v)	{return v[0] | ((char) v[1] << 8);}
static inline int Iso731 (uint8 *v)	{return v[0] | (v[1] << 8) | (v[2] << 16) | (v[3] << 24);}
static inline int Iso732 (uint8 *v)	{return (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];}
static inline int Iso733 (uint8 *v)	{return v[0] | (v[1] << 8) | (v[2] << 16) | (v[3] << 24);}



/*
 *
 */

struct CDNode
{
	struct CDSB *cdsb;
	LIST_ENTRY(CDNode) node_entry;

	uint8 flags;
	int32 size;
	int32 extent_start;
	struct TimeVal date;
	
	LIST (CDFilp) filp_list;
	int32 reference_cnt;
};




/*
 * CD/disc superblock structure
 */
 
struct CDSB
{
	/* struct Mutex mutex; */
	
	int pid;
	
	struct MsgPort *msgport;
	int init_error;
	int reference_cnt;  /* Prevents unmounting/formatting > 0 */

	int diskchange_signal;
	
	struct Device *device;
	void *unitp;
	
	struct Buf *buf;
	
	struct MountEnviron *me;
	
	struct Mount *device_mount;
		
	uint32 features;
	
	struct Callback callback;
	
	struct ISOVolDesc pvd;
	struct CDNode root_node;
	
	LIST (CDNode) node_list;  		/* What?  list of open files/dirs? */
	
	LIST (CDFilp) active_filp_list;
	LIST (CDFilp) invalid_filp_list;
	
	int validated;
};




/*
 *
 */

struct CDFilp
{
	struct Device *device;
	struct CDNode *node;
	struct CDSB *cdsb;
	
	uint32 offset;
	
	LIST_ENTRY(CDFilp) cdsb_filp_entry;
	LIST_ENTRY(CDFilp) node_filp_entry;
	
	int invalid;
	
	int reference_cnt;
	int append_mode;
};




/*
 * Global variables
 */

extern struct Device cd_device;




/*
 *
 */
 
int cd_init (void *elf);
void *cd_expunge (void);
int cd_opendevice (int unit, void *ioreq, uint32 flags);
int cd_closedevice (void *ioreq);
void cd_beginio (void *ioreq);
int cd_abortio (void *ioreq);




/*
 *
 */
 
int32 CDTask (void *arg);
void CDTaskInit (struct CDSB *fsb);
void CDTaskFini (struct CDSB *fsb);

void CDDoInhibit (struct FSReq *fsreq);
void CDDoUninhibit (struct FSReq *fsreq);

void CDDoOpen (struct FSReq *fsreq);
void CDDoClose (struct FSReq *fsreq);
void CDDoDup (struct FSReq *fsreq);
void CDDoRead (struct FSReq *fsreq);
void CDDoWrite (struct FSReq *fsreq);
void CDDoSeek (struct FSReq *fsreq);
void CDDoOpendir (struct FSReq *fsreq);
void CDDoReaddir (struct FSReq *fsreq);
void CDDoRewinddir (struct FSReq *fsreq);
void CDDoFstat (struct FSReq *fsreq);
void CDDoStat (struct FSReq *fsreq);





void CDAddCallback (struct CDSB *cdsb);
void CDRemCallback (struct CDSB *cdsb);
void CDDiskChangeCallback (void *arg);

int CDIsValid (struct CDSB *cdsb, struct FSReq *fsreq);
int CDRevalidate (struct CDSB *cdsb, int skip_validation);
void CDInvalidate (struct CDSB *cdsb);
int CDStatDevice (struct CDSB *cdsb, struct BlockDeviceStat *stat);
int CDReadTOC (struct CDSB *cdsb, void *cd_toc);
bool CDIsDataTrack (void *cd_toc, int track);
int CDValidateVolDesc (struct CDSB *cdsb, void *cd_buffer);




int CDLookup (struct CDLookup *lookup);
struct CDNode *CDSearchDir (struct CDSB *cdsb, struct CDNode *cdnode, char *component);
bool CDCompareDirEntry (struct Dirent *dirent, char *component);


struct CDNode *CDFindNode (struct CDSB *cdsb, struct ISODirEntry *idir);
void CDInitRootNode (struct CDSB *cdsb, struct ISODirEntry *root_idir);
struct CDNode *CDAllocNode (struct CDSB *cdsb, struct ISODirEntry *idir);
void CDFreeNode (struct CDSB *cdsb, struct CDNode *node);

int CDDirRead (struct CDNode *node, struct Dirent *dirent, struct ISODirEntry *idir, off_t offset, off_t *new_offset);
int ISODirEntryToASCIIZ (char *dst, char *src, int max_len);

size_t CDFileRead (struct CDNode *node, void *buf, size_t count, off_t offset, struct FSReq *fsreq);




