#ifndef KERNEL_FS_H
#define KERNEL_FS_H

#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/sync.h>
#include <kernel/termios.h>
#include <kernel/timer.h>
#include <kernel/msg.h>
#include <kernel/config.h>




/* Constants
 */

#define FORMATF_QUICK			(1<<0)		/* Quick format */

#define	O_RDONLY	0x0001		/* +1 == FREAD */
#define	O_WRONLY	0x0002		/* +1 == FWRITE */
#define	O_RDWR		0x0003		/* +1 == FREAD|FWRITE */

#define	O_APPEND	0x0008
#define O_CREAT		0x0200
#define	O_TRUNC		0x0400
#define	O_EXCL		0x0800

#define	O_SYNC		0x2000
#define	O_NONBLOCK	0x4000	
#define	O_NOCTTY	0x8000

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2 

#define MAX_PATHNAME_SZ	1024

#define MOUNTENVIRON_STR_MAX 255
#define NAME_MAX 255

#define FM_MEDIA_PRESENT	1     /* FindMount(), return only mount with media-present */




/*
 * struct Stat
 */

struct Stat 
{
  dev_t		st_dev;
  ino_t		st_ino;
  mode_t	st_mode;
  nlink_t	st_nlink;
  uid_t		st_uid;
  gid_t		st_gid;
  dev_t		st_rdev;
  off_t		st_size;
  time_t	st_atime;
  time_t	st_mtime;
  time_t	st_ctime;
  unsigned long  st_blocks;
  unsigned long st_blksize;
};



#define	_IFMT		0170000	/* type of file */
#define	_IFDIR		0040000	/* directory */
#define	_IFCHR		0020000	/* character special */
#define	_IFBLK		0060000	/* block special */
#define	_IFREG		0100000	/* regular */
#define	_IFLNK		0120000	/* symbolic link */
#define	_IFSOCK		0140000	/* socket */
#define	_IFIFO		0010000	/* fifo */

#define S_BLKSIZE  	512		/* size of a block */

#define	S_ISUID		0004000	/* set user id on execution */
#define	S_ISGID		0002000	/* set group id on execution */

#ifndef	_POSIX_SOURCE
#define	S_ISVTX		0001000	/* save swapped text even after use */
#define	S_IREAD		0000400	/* read permission, owner */
#define	S_IWRITE 	0000200	/* write permission, owner */
#define	S_IEXEC		0000100	/* execute/search permission, owner */
#define	S_ENFMT 	0002000	/* enforcement-mode locking */
#define	S_IFMT		_IFMT
#define	S_IFDIR		_IFDIR
#define	S_IFCHR		_IFCHR
#define	S_IFBLK		_IFBLK
#define	S_IFREG		_IFREG
#define	S_IFLNK		_IFLNK
#define	S_IFSOCK	_IFSOCK
#define	S_IFIFO		_IFIFO
#endif	/* !_POSIX_SOURCE */


#define	S_IRWXU 	(S_IRUSR | S_IWUSR | S_IXUSR)
#define	S_IRUSR		0400	/* read permission, owner */
#define	S_IWUSR		0200	/* write permission, owner */
#define	S_IXUSR		0100	/* execute/search permission, owner */
#define	S_IRWXG		(S_IRGRP | S_IWGRP | S_IXGRP)
#define	S_IRGRP		0040	/* read permission, group */
#define	S_IWGRP		0020	/* write permission, grougroup */
#define	S_IXGRP 	0010	/* execute/search permission, group */
#define	S_IRWXO		(S_IROTH | S_IWOTH | S_IXOTH)
#define	S_IROTH		0004	/* read permission, other */
#define	S_IWOTH		0002	/* write permission, other */
#define	S_IXOTH 	0001	/* execute/search permission, other */

#define UMASK_DFL   0022    /* 777 -> u rwx g r-x o r-x, 666 -> u rw- g r-- o r-- */

#define	S_ISBLK(m)	(((m)&_IFMT) == _IFBLK)
#define	S_ISCHR(m)	(((m)&_IFMT) == _IFCHR)
#define	S_ISDIR(m)	(((m)&_IFMT) == _IFDIR)
#define	S_ISFIFO(m)	(((m)&_IFMT) == _IFIFO)
#define	S_ISREG(m)	(((m)&_IFMT) == _IFREG)
#define	S_ISLNK(m)	(((m)&_IFMT) == _IFLNK)
#define	S_ISSOCK(m)	(((m)&_IFMT) == _IFSOCK)







/* XXX close on exec request; must match UF_EXCLOSE in user.h */
#define	FD_CLOEXEC	1	/* posix */

/* fcntl(2) requests */
#define	F_DUPFD		0	/* Duplicate fildes */
#define	F_GETFD		1	/* Get fildes flags (close on exec) */
#define	F_SETFD		2	/* Set fildes flags (close on exec) */
#define	F_GETFL		3	/* Get file flags */
#define	F_SETFL		4	/* Set file flags */
#define	F_GETOWN 	5	/* Get owner - for ASYNC */
#define	F_SETOWN 	6	/* Set owner - for ASYNC */
#define	F_GETLK  	7	/* Get record-locking information */
#define	F_SETLK  	8	/* Set or Clear a record-lock (Non-Blocking) */
#define	F_SETLKW 	9	/* Set or Clear a record-lock (Blocking) */
#define	F_RGETLK 	10	/* Test a remote lock to see if it is blocked */
#define	F_RSETLK 	11	/* Set or unlock a remote lock */
#define	F_CNVT 		12	/* Convert a fhandle to an open fd */
#define	F_RSETLKW 	13	/* Set or Clear remote record-lock(Blocking) */




/*
 * struct StatVFS
 */

struct StatVFS
{
	unsigned long f_bsize;    /* File system block size */
	unsigned long f_frsize;   /* Fundamental file system block size */
	fsblkcnt_t    f_blocks;   /* Num blocks in units of f_frsize */
	fsblkcnt_t    f_bfree;    /* Num free blocks */
	fsblkcnt_t    f_bavail;   /* Num free blocks for non-privileged process */
	fsfilcnt_t    f_files;    /* Total number of file serial numbers */ 
	fsfilcnt_t    f_ffree;    /* Total number of free file serial numbers */ 
	fsfilcnt_t    f_favail;   /* ... for non-privileged process */ 
	unsigned long f_fsid;     /* File system ID */ 
	unsigned long f_flag;     /* Bit mask of f_flag values */  
	unsigned long f_namemax ; /* Maximum filename length */ 
};



/*
 * struct StatFS
 */

typedef struct { long val[2]; } fsid_t;

#define MFSNAMELEN   15 /* length of fs type name, not inc. nul */
#define MNAMELEN     90 /* length of buffer for returned name */

struct StatFS
{
	short   f_otype;    /* type of file system (reserved: zero) */
	short   f_oflags;   /* copy of mount flags (reserved: zero) */
	long    f_bsize;    /* fundamental file system block size */
	long    f_iosize;   /* optimal transfer block size */
	long    f_blocks;   /* total data blocks in file system */
	long    f_bfree;    /* free blocks in fs */
	long    f_bavail;   /* free blocks avail to non-superuser */
	long    f_files;    /* total file nodes in file system */
	long    f_ffree;    /* free file nodes in fs */
	fsid_t  f_fsid;     /* file system id (super-user only) */
	uid_t   f_owner;    /* user that mounted the file system */
	short   f_reserved1;        /* reserved for future use */
	short   f_type;     /* type of file system (reserved) */
	long    f_flags;    /* copy of mount flags (reserved) */
	long    f_reserved2[2];     /* reserved for future use */
	char    f_fstypename[MFSNAMELEN]; /* fs type name */
	char    f_mntonname[MNAMELEN];    /* directory on which mounted */
	char    f_mntfromname[MNAMELEN];  /* mounted file system */
	char    f_reserved3;        /* reserved for future use */
	long    f_reserved4[4];     /* reserved for future use */
	
	short	f_kostype;			/* Type of mount, either device, volume or assign */
};




/*
 * struct Dirent
 */

struct Dirent {
	long d_ino;						/* inode value */
	char d_name[NAME_MAX + 1];		/* filename */
};




/*
 * PathInfo
 */

struct PathInfo
{
	char *buffer;
	char *mountname;
	char *pathname;
	char *canon;
};




/*
 * CreatePathInfo()  KEEP_CANON 
 */

#define KEEP_CANON			1




/*
 *  FSReq
 */

struct FSReq
{
	struct Msg msg;
	struct Device *device;
	void *unitp;
	uint32 flags;
	
	int32 cmd;
	int rc;
	int error;
		
	struct AddressSpace *as;

	struct StdIOReq *abort_ioreq;
	
	/* Filesystem dependent arguments and results */

	void *filp;
	void *filp2; /* Doesn't need to be a pointer now ?????????????? */

	char *path;
	char *path2;
	
	int oflags;
	mode_t mode;

	/* Not used yet,  perhaps pointer to struct Process or struct IDs
		would be better */
	
	int foreground_grp;   /* These are for chown, chmod, setpgrp */
	uid_t owner;          /* Not for actual permission checks */
	gid_t group;          /* Also need to pass ** umask ** */
	
	
	/* Just added, to get current uid, gid, pgrp, umask etc, etc */
	/* ** Set in all outer-wrappers? */
	
	struct Process *proc;
			
	off_t size;
	off_t position;
	off_t offset;
	int whence;
	
	struct Stat *stat;
	struct StatVFS *statfs;

	ssize_t nbytes_transferred;
	void *buf;
	size_t count;

	struct Dirent *dirent;
	
	int termios_action;
	struct Termios *termios;
	
	int ioctl_request;
	void *ioctl_arg;
	
	char *format_label;
	uint32 format_flags;
	uint32 format_cluster_sz;

	struct MountEnviron *me;
};




/*
 *
 */
 
struct BootEntry
{
	LIST_ENTRY (BootEntry) boot_entry;
	struct MountEnviron *me;
};




/*
 * struct MountEnviron;
 */

struct MountEnviron
{
	char handler_name [MOUNTENVIRON_STR_MAX + 1];
	char device_name [MOUNTENVIRON_STR_MAX + 1];
	char mount_name [MOUNTENVIRON_STR_MAX + 1];
	char startup_args [MOUNTENVIRON_STR_MAX + 1];
	
	uint32 handler_unit;
	uint32 handler_flags;
		
	uint32 device_unit;
	uint32 device_flags;

    uint32 block_size;
    
    uint32 partition_start;
    uint32 partition_end;
    uint32 reserved_blocks;
    
	uint32 buffer_cnt;
	
	int boot_priority;
	
	uint32 baud;

	int removable;
	int writable;
	
	int writethru_critical;
	uint32 writeback_delay;
	
	uint32 max_transfer;
	
	uint32 control_flags;
};




/*
 * MAY Need to contain the FSReq from the OpenDevice (handler.name)
 */
 
struct Mount
{
	LIST_ENTRY (Mount) mount_list_entry;
	int type;
		
	char *name;
	struct Device *handler;
	void *unitp;					 /* Superblock pointer */	
	
	char *pathname;				/* For assigns */
	
	struct MountEnviron *me;
};


#define MOUNT_DEVICE		0
#define MOUNT_ASSIGN		1




/*
 *
 */

struct RootFilp
{
	struct Device *device;
	struct Mount *seek_mount;
	LIST_ENTRY (RootFilp) filp_entry;

};




/*
 *
 */

#define FS_CMD_OPEN			50
#define FS_CMD_CLOSE		51
#define FS_CMD_UNLINK		52
#define FS_CMD_DUP			53
#define FS_CMD_RENAME		54
#define FS_CMD_TRUNCATE		55
#define FS_CMD_FTRUNCATE	56
#define FS_CMD_STAT			57
#define FS_CMD_FSTAT		58
#define FS_CMD_FSTATFS		59
#define FS_CMD_ISATTY		60
#define FS_CMD_FSYNC		61
#define FS_CMD_SYNC			62
#define FS_CMD_READ			63
#define FS_CMD_WRITE		64
#define FS_CMD_KREAD		65
#define FS_CMD_KWRITE		66
#define FS_CMD_LSEEK		67
#define FS_CMD_CHDIR		68
#define FS_CMD_MKDIR		69
#define FS_CMD_RMDIR		70
#define FS_CMD_OPENDIR		71
#define FS_CMD_CLOSEDIR		72
#define FS_CMD_READDIR		73
#define FS_CMD_REWINDDIR	74
#define FS_CMD_FORMAT		75
#define FS_CMD_IOCTL		76
#define FS_CMD_TCGETATTR	77
#define FS_CMD_TCSETATTR	78
#define FS_CMD_PIPE			79
#define FS_CMD_TCSETPGRP	80
#define FS_CMD_TCGETPGRP	81
#define FS_CMD_CHMOD		82
#define FS_CMD_CHOWN		83
#define FS_CMD_INHIBIT		84
#define FS_CMD_UNINHIBIT	85

#define FS_CMD_ABORT		99




/*
 * Globals
 */

extern LIST_DECLARE (BootEntryList, BootEntry) bootentry_list;
extern LIST_DECLARE (MountList, Mount) mount_list;

extern struct Mutex mount_mutex;
extern struct RWLock mountlist_rwlock;

extern struct Mount root_mount;
extern struct Mutex root_mutex;         /* Needed or not? */
extern struct Device root_handler;

extern LIST_DECLARE (RootFilpList, RootFilp) root_filp_list;

extern char *cfg_boot_prefix;
extern int cfg_boot_verbose;



/*
 * Prototypes
 */

int MountAll (void);
int DetermineBootMount (void);

struct MountEnviron *AllocMountEnviron (void);
int AddBootMountEnviron (struct MountEnviron *me);

struct Mount *MakeMount (struct MountEnviron *me, struct Device *device, void *unitp);
struct Mount *FindMount (char *name);

int CheckMediaPresence (struct Mount *mount);

int DoMount (struct Mount *mount);
void DoUnmount (struct Mount *mount);

void AddMount (struct Mount *mount);
void RemMount (struct Mount *mount);




void StartFilesystem (void);
void FinishFilesystem (void);
void InitDevices (void);
void InitHandlers (void);
void InitMounts (void);
int FSCreateProcess (struct Process *oldproc, struct Process *newproc, char *cdir, bool dup_fd);
void FSExitProcess (struct Process *process);
void FSCloseOnExec (struct Process *process);

int AllocFD (struct Process *process, int base);
void FreeFD (struct Process *process, int fd);
void *FDtoFilp (int fd);


int CreatePathInfo (struct PathInfo *pi, char *pathname, bool keep_canon);
void FreePathInfo (struct PathInfo *pi);


char *TranslatePathnameAliases (char *canon);
struct Mount *FindAssign (char *alias);
int32 InitPathInfo (struct PathInfo *pi);
char *AllocPathname (char *src);
int ValidAlias (char *alias);
char *CanonPathname (char *pathname);


int CompareComponent (char *a, char *b);
int DeleteLastComponent (char *canon);
int AppendComponent (char *canon, char *component);
int AppendPath (char *buf, char *remaining);
char *Advance (char *component);
int CompareComponent (char *s, char *t);
int CompareAlias (char *alias, char *pathname);

struct Mount *AllocAssign (char *alias, char *path);
void FreeAssign (struct Mount *assign);




/*
 * Main Filesystem kernel interface
 */


int Mount (struct MountEnviron *me);
int Unmount (char *mount_name);
int MountInfo (char *mount, struct StatFS *buf);

int Open (char *pathname, int flags, uint32 mode);
int Creat (char *pathname, uint32 mode);
int Close (int fd);
int Unlink (char *pathname);

int Dup (int oldfd);
int Dup2 (int oldfd, int newfd);
int Rename (char *oldpath, char *newpath);

int Ftruncate (int fd, off_t length);

int Isatty (int fd);
int Fstat (int fd, struct Stat *buf);
int Fstatvfs (int fd, struct StatVFS *buf);

int Fsync (int fd);
int Sync (void);
		
ssize_t Read (int fd, void *buf, size_t count);
ssize_t Write (int fd, void *buf, size_t count);
off_t   Seek (int fd, off_t offset, int whence);

char *Getcwd (char *buf, int buf_sz);
int Chdir (char *path);

int Mkdir (char *pathname, mode_t mode);
int Rmdir (char *pathname);
	
int Opendir (char *name);
int Closedir (int fd);
int Readdir (int fd, struct Dirent *dirent);
int Rewinddir (int fd);

int	Tcgetattr (int fd, struct Termios *);
int	Tcsetattr (int fd, int action, struct Termios *);

int Pipe (int *fd);
int Fcntl (int fd, int cmd, int arg);

int SetAssign (char *alias, char *path);
int GetAssign (int idx, char *alias, char *path);
int Stat (char *pathname, struct Stat *buf);
int Access (char *pathname, int amode);
int Ioctl (int fd, int request, uintptr_t arg);
mode_t Umask (mode_t cmask);

int Format (char *mount, char *label, uint32 flags, uint32 cluster_size);
int Relabel (char *mount, char *newlabel);

int Tcsetpgrp (int fd, int foreground_grp);
int Tcgetpgrp (int fd);

int Chmod (char *path, mode_t mode);
int Chown (char *path, uid_t owner, gid_t group);

int Inhibit (char *pathname);
int Uninhibit (char *pathname);




#endif
