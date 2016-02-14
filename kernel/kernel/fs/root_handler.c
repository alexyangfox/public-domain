#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/sync.h>
#include <kernel/fs.h>
#include <kernel/resident.h>
#include <kernel/dbg.h>

int root_init (void *elf);
void *root_expunge (void);
void root_beginio (void *ioreq);
int root_abortio (void *ioreq);



/* The Root filesystem handler used for obtaining the directory list of mounts */


struct Device root_handler =
{
	{NULL, NULL},
	"root.handler",
	1,
	0,
	{0, {NULL, NULL}},
	&root_init,
	&root_expunge,
	NULL,
	NULL,
	&root_beginio,
	&root_abortio
};

struct Resident root_resident =
{
	RESIDENT_MAGIC1,
	RESIDENT_MAGIC2,
	RESIDENT_MAGIC3,
	RESIDENT_MAGIC4,
	&root_resident,
	RFLG_AUTOINIT,
	10,
	RTYPE_DEVICE,
	10,
	{NULL, NULL},
	"root.handler",
	"id",
	&root_init,
	&root_handler,
	NULL
};




/*
 */

int root_init (void *elf)
{
	root_mount.handler = &root_handler;
	
	return 0;
}




/*
 */

void *root_expunge (void)
{
	return NULL;
}




/*
 *
 */

void root_beginio (void *ioreq)
{
	struct FSReq *fsreq = ioreq;
	struct RootFilp *filp;
	struct Mount *mount;
	int len;
	
	KPRINTF ("root_beginio()");
	
	fsreq->flags |= IOF_QUICK;
	
	switch (fsreq->cmd)
	{
		case FS_CMD_OPEN:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;
		
		case FS_CMD_PIPE:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;

		case FS_CMD_CLOSE:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;


		case FS_CMD_DUP:		/* Might want to dup() the handle? */
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;

		case FS_CMD_READ:
			fsreq->error = ENOSYS;
			fsreq->nbytes_transferred = -1;
			fsreq->rc = -1;
			break;

		case FS_CMD_WRITE:
			fsreq->error = ENOSYS;
			fsreq->nbytes_transferred = -1;
			fsreq->rc = -1;
			break;

		case FS_CMD_LSEEK:
			fsreq->error = ENOSYS;
			fsreq->position = -1;
			break;

		case FS_CMD_UNLINK:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;

		case FS_CMD_RENAME:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;

		case FS_CMD_FTRUNCATE:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;

		case FS_CMD_FSTAT:
			fsreq->stat->st_mode = S_IFDIR  | (S_IRUSR | S_IRGRP | S_IROTH);
			fsreq->stat->st_nlink = 1; /* ??? 0 ??? */
			fsreq->stat->st_uid = 1;
			fsreq->stat->st_gid = 1;
			fsreq->stat->st_rdev = 5;	
			fsreq->stat->st_size = 0;
			fsreq->stat->st_atime = 0;
			fsreq->stat->st_mtime = 0;
			fsreq->stat->st_ctime = 0;
			fsreq->stat->st_blocks = 0;
			fsreq->error = 0;
			fsreq->rc = 0;
			break;

		case FS_CMD_STAT:
			fsreq->stat->st_mode = S_IFDIR  | (S_IRUSR | S_IRGRP | S_IROTH);
			fsreq->stat->st_nlink = 1; /* ??? 0 ??? */
			fsreq->stat->st_uid = 1;
			fsreq->stat->st_gid = 1;
			fsreq->stat->st_rdev = 5;	
			fsreq->stat->st_size = 0;
			fsreq->stat->st_atime = 0;
			fsreq->stat->st_mtime = 0;
			fsreq->stat->st_ctime = 0;
			fsreq->stat->st_blocks = 0;
			fsreq->error = 0;
			fsreq->rc = 0;
			break;

		case FS_CMD_FSTATFS:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;

		case FS_CMD_FSYNC:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;

		case FS_CMD_SYNC:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;

		case FS_CMD_MKDIR:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;
			
		case FS_CMD_RMDIR:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;
		
			
		case FS_CMD_OPENDIR:
		{
			if ((filp = KMalloc (sizeof (struct RootFilp))) != NULL)
			{
				filp->device = &root_handler;
				filp->seek_mount = LIST_HEAD (&mount_list);
				
				LIST_ADD_TAIL (&root_filp_list, filp, filp_entry);
								
				fsreq->filp = filp;
				fsreq->device = &root_handler;  
				fsreq->error = 0;
				fsreq->rc = 0;
			}
			else
			{
				fsreq->error = ENOSYS;
				fsreq->rc = -1;
			}
			break;
		}
			
		case FS_CMD_CLOSEDIR:
		{
			filp = fsreq->filp;
			
			LIST_REM_ENTRY (&root_filp_list, filp, filp_entry);
			KFree (filp);
			fsreq->error = 0;
			fsreq->rc = 0;
			break;
		}
			
		case FS_CMD_READDIR:
			filp = fsreq->filp;
			
			mount = filp->seek_mount;
						
			if (mount != NULL)
			{
				len = StrLen (mount->name);	
			
				if (len + 1 <= NAME_MAX)
				{
					CopyOut (fsreq->as, &fsreq->dirent->d_name, mount->name, len + 1);
		
					filp->seek_mount = LIST_NEXT (mount, mount_list_entry);

					fsreq->error = 0;
					fsreq->rc = 0;
				}
				else
				{
					fsreq->dirent = NULL;
					fsreq->error = ENAMETOOLONG;
					fsreq->rc = -1;
				}
			}
			else
			{
				fsreq->dirent = NULL;
				fsreq->error = 0;
				fsreq->rc = -1;
			}
			break;
			
		
		case FS_CMD_REWINDDIR:
			filp = fsreq->filp;
			filp->seek_mount = LIST_HEAD (&mount_list);
			
			fsreq->error = 0;
			fsreq->rc = 0;
			break;
			
		case FS_CMD_ISATTY:
			fsreq->error = 0;
			fsreq->rc = 0;
			break;

		case FS_CMD_TCGETATTR:
			fsreq->error = ENOTTY;
			fsreq->rc = -1;
			break;

		case FS_CMD_TCSETATTR:
			fsreq->error = ENOTTY;
			fsreq->rc = -1;
			break;

		case FS_CMD_IOCTL:
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;
	
		default:
		{
			KPANIC ("ROOT Unknown command");
			fsreq->error = ENOSYS;
			fsreq->rc = -1;
			break;
		}
	}
}




/*
 * AbortIO();
 *
 * All io-requests to root_handler are in quick-mode,  so no need
 * to abort an io-request.
 */

int root_abortio (void *ioreq)
{
	return 0;
}



