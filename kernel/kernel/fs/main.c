#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/sync.h>
#include <kernel/fs.h>
#include <kernel/device.h>
#include <kernel/kmalloc.h>
#include <kernel/proc.h>
#include <kernel/utility.h>
#include <kernel/error.h>
#include <kernel/dbg.h>



/*
 * NOTE:  May need to use RWLock inside dup() and dup2().  However as at least
 * one reference_cnt will be held on the target filesystem there is no need
 * to use RWLock().
 */
 
 

/*
 * Mount();
 *
 * ***** Should the handler get to keep the mountenviron??????? 
 *
 */

int Mount (struct MountEnviron *me_usr)
{	
	struct MountEnviron *me;
	struct FSReq fsreq;
	int rc;
	
	
	KPRINTF ("Mount()");

	MutexLock (&mount_mutex);

	if ((me = AllocMountEnviron()) != NULL)
	{	
		if (CopyIn (current_process->user_as, me, me_usr, sizeof (struct MountEnviron)) == 0)
		{
			if ((StrLen (me->mount_name) != 0) && (StrLen (me->handler_name) != 0))
			{
				KPRINTF ("mount = %s", me->mount_name);
				KPRINTF ("handler = %s", me->handler_name);
				
				fsreq.me = me;
												
				rc = OpenDevice (me->handler_name, me->handler_unit, &fsreq, me->handler_flags);
				
				if (rc == 0)
				{
					MutexUnlock (&mount_mutex);
					return 0;
				}
			}
			else
				SetError (EINVAL);
		}
		
		KFree (me);
	}


	MutexUnlock (&mount_mutex);

	return -1;
}




/*
 * Unmount();
 */

int Unmount (char *mountname)
{
	struct Mount *mount;
	int rc = -1;
	struct FSReq fsreq;
	char mountname2[MAX_PATHNAME_SZ];


	MutexLock (&mount_mutex);

	KPRINTF ("Unmount()");

	if (CopyInStr (current_process->user_as, mountname2, mountname, MAX_PATHNAME_SZ) == 0)
	{
		KPRINTF ("Unmount str = (%s)", mountname2);
		
		/*
		 * FIXME:  FECK,  need mountlist locked,  otherwise multiple mounts/unmounts
		 * could occur at same time.
		 */
		
		if ((mount = FindMount (mountname2)) != NULL)
		{
			if (mount->type == MOUNT_DEVICE)
			{
				fsreq.device = mount->handler;
				fsreq.unitp = mount->unitp;
				fsreq.as = &kernel_as;
						
				rc = CloseDevice (&fsreq);
				
				if (rc != 0)
					SetError (fsreq.error);
			}
			else
			{
				SetError (ENODEV);
				KPRINTF ("******** NOT A MOUNT_DEVICE");
			}
		}
	}

	MutexUnlock (&mount_mutex);
	
	
	KPRINTF ("Unmount() = %d", rc);
	
	return rc;
}



/*
 *
 */

int MountInfo (char *mount, struct StatFS *buf)
{
	KPRINTF ("MountInfo()");

	RWReadLock (&mountlist_rwlock);
	RWUnlock (&mountlist_rwlock);

	return -1;
}




/*
 * Open();
 */

int Open (char *pathname, int oflags, uint32 mode)
{
	struct PathInfo pi;
	struct Mount *mount;
	int fd = -1;
	struct FSReq fsreq;

	KPRINTF("Open()");


	RWReadLock (&mountlist_rwlock);

	if (CreatePathInfo (&pi, pathname, 0) == 0)
	{
		if ((mount = FindMount (pi.mountname)) != NULL)
		{
			if ((fd = AllocFD (current_process, 0)) != -1)
			{	
				fsreq.device = mount->handler;
				fsreq.unitp = mount->unitp;
				fsreq.cmd = FS_CMD_OPEN;
				fsreq.path = pi.pathname;
				fsreq.oflags = oflags;
				fsreq.mode = mode & ~current_process->umask;
				fsreq.as = current_process->user_as;
				fsreq.proc = current_process;

				DoIO (&fsreq, NULL);

				if (fsreq.rc == 0)
				{
					current_process->filedesc[fd] = fsreq.filp;
				}
				else
				{
					SetError (fsreq.error);
					FreeFD (current_process, fd);
					fd = -1;
				}
			}
			
		}
		
		FreePathInfo (&pi);
	}


	RWUnlock (&mountlist_rwlock);	
	
	return fd;
}




/*
 * Close();
 */

int Close (int fd)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int rc = -1;
	

	RWReadLock (&mountlist_rwlock);
		
	if ((filp = FDtoFilp(fd)) != NULL) 
	{
		device = (struct Device *) *(void **)filp;
		
		if (device != NULL)
		{
			fsreq.device = device;
			fsreq.unitp = NULL;
			fsreq.cmd = FS_CMD_CLOSE;
			fsreq.filp = filp;
			fsreq.as = current_process->user_as;
			fsreq.proc = current_process;
			
			DoIO (&fsreq, NULL);
			
			FreeFD (current_process, fd);
			
			SetError (fsreq.error);
			rc = fsreq.rc;
		}
		else
		{
			SetError (EIO);
			rc = -1;
		}
	}		
	
	
	RWUnlock (&mountlist_rwlock);
		
	return rc;
}




/*
 * Unlink();
 */

int Unlink (char *pathname)
{
	struct PathInfo pi;
	struct Mount *mount;
	struct FSReq fsreq;
	int rc = -1;
	
	
	KPRINTF("Unlink()");

	
	RWReadLock (&mountlist_rwlock);
	
	if (CreatePathInfo (&pi, pathname, 0) == 0)
	{
		if ((mount = FindMount (pi.mountname)) != NULL)
		{
			fsreq.device = mount->handler;
			fsreq.unitp = mount->unitp;
			fsreq.cmd = FS_CMD_UNLINK;
			fsreq.path = pi.pathname;
			fsreq.as = current_process->user_as;
			fsreq.proc = current_process;
			
			DoIO (&fsreq, NULL);
			
			SetError (fsreq.error);
			rc = fsreq.rc;
		}
		
		FreePathInfo (&pi);
	}
	

	RWUnlock (&mountlist_rwlock);
	
	return rc;
}




/*
 * Pipe();
 */

int Pipe (int fd_ptr[2])
{
	int fd[2];
	struct Mount *mount;
	struct FSReq fsreq;
	int rc;
	
	

	RWReadLock (&mountlist_rwlock);
		
	if ((mount = FindMount ("pipe")) != NULL)
	{
		if ((fd[0] = AllocFD (current_process, 0)) != -1)
		{
			if ((fd[1] = AllocFD (current_process, fd[0]+1)) != -1)
			{
				CopyOut (current_process->user_as, fd_ptr, fd, sizeof (fd));
			
				fsreq.device = mount->handler;
				fsreq.unitp = mount->unitp;
				fsreq.cmd = FS_CMD_PIPE;
				fsreq.as = current_process->user_as;
				fsreq.proc = current_process;
				
				DoIO (&fsreq, NULL);
				
				SetError (fsreq.error);
				rc = fsreq.rc;
				
				if (rc == 0)
				{
					current_process->filedesc[fd[0]] = fsreq.filp;
					current_process->filedesc[fd[1]] = fsreq.filp2;
					RWUnlock (&mountlist_rwlock);
					return 0;
				}
				
				FreeFD (current_process, fd[1]);
			}

			FreeFD (current_process, fd[0]);
		}
	}
	else
	{
		KPANIC ("PIPE not found");
	}
	
	RWUnlock (&mountlist_rwlock);
	
	return -1;
}










/*
 * Dup();
 */

int Dup (int oldfd)
{
	int newfd;
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int rc=0;	
	
	
	if ((filp = FDtoFilp (oldfd)) != NULL)
	{
		device = (struct Device *) *(void **)filp;
		
		if ((newfd = AllocFD (current_process, 0)) != -1)
		{
			fsreq.device = device;
			fsreq.unitp = NULL;
			fsreq.cmd = FS_CMD_DUP;
			fsreq.filp = filp;
			fsreq.as = current_process->user_as;
			fsreq.proc = current_process;

			DoIO (&fsreq, NULL);
			SetError (fsreq.error);
			rc = fsreq.rc;
			
			if (rc == 0)
			{
				current_process->filedesc[newfd] = fsreq.filp2;
				rc = newfd;
			}
			else
				FreeFD (current_process, newfd);
		}
		else
			rc = -1;
	}
	else
		rc = -1;
		
		
	return rc;
}




/*
 * Dup2();
 */

int Dup2 (int oldfd, int newfd)
{
	void *filp, *newfilp;
	struct Device *device, *newdevice;
	struct FSReq fsreq;
	int rc;
	
	
 	if ((filp = FDtoFilp (oldfd)) != NULL) 
	{
		if (oldfd == newfd)
		{
			rc = newfd;
		}
		else
		{
			if ((newfilp = FDtoFilp (newfd)) != NULL)
			{
				newdevice = (struct Device *) *(void **)newfilp;
				
				fsreq.device = newdevice;
				fsreq.unitp = NULL;
				fsreq.cmd = FS_CMD_CLOSE;
				fsreq.filp = newfilp;
				fsreq.proc = current_process;
				
				DoIO (&fsreq, NULL);
				SetError (fsreq.error);

				newfilp = NULL;
				current_process->filedesc[newfd] = NULL;
			}
		
			
			device = (struct Device *) *(void **)filp;
			
			fsreq.device = device;
			fsreq.unitp = NULL;
			fsreq.cmd = FS_CMD_DUP;
			fsreq.filp = filp;
			fsreq.proc = current_process;
			
			DoIO (&fsreq, NULL);
			SetError (fsreq.error);
			
			if (fsreq.rc == 0)
			{
				current_process->filedesc[newfd] = fsreq.filp2;
				rc = newfd;
			}
			else
				rc = -1;
		}
	}
	else
		rc = -1;	

	
	return rc;
}




/*
 * Ftruncate();
 */

int Ftruncate (int fd, off_t length)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int rc;
	
	
	if ((filp = FDtoFilp(fd)) != NULL)
	{
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_FTRUNCATE;
		fsreq.filp = filp;
		fsreq.size = length;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;

		DoIO (&fsreq, NULL);
		SetError (fsreq.error);
		rc = fsreq.rc;
	}
	else
		rc = -1;	
	
	return rc;
}




/*
 * Fstat();
 */

int Fstat (int fd, struct Stat *buf)
{
	struct Stat stat;
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int rc;
	
		
	if ((filp = FDtoFilp(fd)) != NULL)
	{
		device = (struct Device *) *(void **)filp;
		
		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_FSTAT;
		fsreq.filp = filp;
		fsreq.stat = &stat;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;

		DoIO (&fsreq, NULL);
		SetError (fsreq.error);
		rc = fsreq.rc;
		
		if (rc == 0)
			if (CopyOut (current_process->user_as, buf, &stat, sizeof (struct Stat)) != 0)
				rc = -1;
	}
	else
		rc = -1;	
	
	return rc;
}




/*
 * Fstatvfs();
 */

int Fstatvfs (int fd, struct StatVFS *buf)
{
	void *filp;
	struct StatVFS statvfs;
	struct Device *device;
	struct FSReq fsreq;
	int rc;
	
	
	if ((filp = FDtoFilp(fd)) != NULL)
	{
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_FSTATFS;
		fsreq.filp = filp;
		fsreq.statfs = &statvfs;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;

		DoIO (&fsreq, NULL);
		SetError (fsreq.error);
		rc = fsreq.rc;
		
		if (rc == 0)
			if (CopyOut (current_process->user_as, buf, &statvfs, sizeof (struct StatVFS)) != 0)
				rc = -1;
	}
	else
		rc = -1;	
		
	return rc;
}




/*
 * Stat();
 *
 */

int Stat (char *pathname, struct Stat *buf)
{
	struct Stat stat;
	struct PathInfo pi;
	struct Mount *mount;
	struct FSReq fsreq;
	int rc = -1;
	
	
	RWReadLock (&mountlist_rwlock);
	
	if (CreatePathInfo (&pi, pathname, 0) == 0)
	{
		if ((mount = FindMount (pi.mountname)) != NULL)
		{
			fsreq.device = mount->handler;
			fsreq.unitp = mount->unitp;
			fsreq.cmd = FS_CMD_STAT;
			fsreq.path = pi.pathname;
			fsreq.stat = &stat;
			fsreq.as = current_process->user_as;
			fsreq.proc = current_process;
			
			DoIO (&fsreq, NULL);
			SetError (fsreq.error);
			rc = fsreq.rc;
			
			if (rc == 0)
				if (CopyOut (current_process->user_as, buf, &stat, sizeof (struct Stat)) != 0)
					rc = -1;
		}

		FreePathInfo (&pi);
	}

	RWUnlock (&mountlist_rwlock);
	
	return rc;
}	




/*
 * Isatty();
 */

int Isatty (int fd)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int rc;

	
	if ((filp = FDtoFilp(fd)) != NULL)
	{
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_ISATTY;
		fsreq.filp = filp;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;
		
		DoIO (&fsreq, NULL);
		SetError (fsreq.error);
		rc = fsreq.rc;
	}
	else
		rc = -1;
	
	return rc;
}




/*
 * Fsync();
 */

int Fsync (int fd)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int rc;


	KPRINTF ("Fsync()");	
	
	
	if ((filp = FDtoFilp(fd)) != NULL)
	{	
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_FSYNC;
		fsreq.filp = filp;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;
		
		DoIO (&fsreq, NULL);
		SetError (fsreq.error);
		rc = fsreq.rc;
	}
	else
		rc = -1;	

	return rc;
}




/*
 * Sync();
 *
 * FIXME: Use the mount_unmount_mutex.
 * Readdir each dos_device then sync by name
 */

int Sync (void)
{
	struct Mount *mount;
	struct FSReq fsreq;
	

	KPRINTF ("Sync()");
		

	RWReadLock (&mountlist_rwlock);


	mount = LIST_HEAD (&mount_list);
	
	while (mount != NULL)
	{
		if (mount->handler != NULL)
		{
			fsreq.device = mount->handler;
			fsreq.unitp = mount->unitp;
			fsreq.cmd = FS_CMD_SYNC;
			fsreq.as = current_process->user_as;
			fsreq.proc = current_process;
			
			DoIO (&fsreq, NULL);
		}
		
		mount = LIST_NEXT (mount, mount_list_entry);
	}

	RWUnlock (&mountlist_rwlock);
	
	return 0;
}




/*
 * Read();
 */
		
ssize_t Read (int fd, void *buf, size_t count)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	ssize_t sz;


	if (count == 0)
		return 0;
		
	if ((filp = FDtoFilp(fd)) != NULL)
	{
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_READ;
		fsreq.filp = filp;
		fsreq.buf = buf;
		fsreq.count = count;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;
		
		DoIOAbortable (&fsreq, NULL, SIGF_USER);
		SetError (fsreq.error);
	
		if (fsreq.rc == 0)
			sz = fsreq.nbytes_transferred;
		else
			sz = -1;
	}
	else
	{
		sz = -1;
	}
		
	
	return sz;
}




/*
 * Write();
 */

ssize_t Write (int fd, void *buf, size_t count)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	ssize_t sz;


	if (count == 0)
		return 0;
	
	
	if ((filp = FDtoFilp(fd)) != NULL)
	{	
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_WRITE;
		fsreq.filp = filp;
		fsreq.buf = buf;
		fsreq.count = count;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;
		
		DoIOAbortable (&fsreq, NULL, SIGF_USER);
		SetError (fsreq.error);
	
		if (fsreq.rc == 0)
			sz = fsreq.nbytes_transferred;
		else
			sz = -1;
	}
	else
		sz = -1;	

	
	return sz;
}




/*
 * Seek();
 */

off_t Seek (int fd, off_t offset, int whence)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	off_t position;


		
	if ((filp = FDtoFilp(fd)) != NULL)
	{
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_LSEEK;
		fsreq.filp = filp;
		fsreq.offset = offset;
		fsreq.whence = whence;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;
		
		DoIO (&fsreq, NULL);
		SetError (fsreq.error);
		
		if (fsreq.rc == 0)
			position = fsreq.position;
		else
			position = -1;
	}
	else
		position = -1;

	
	return position;
}




/*
 * Getcwd();
 *
 */

char *Getcwd (char *buf, int sz)
{
	int len;
	char *rbuf;
	

	KPRINTF ("Getcwd()");
	
	MutexLock (&proc_mutex);
	
	len = StrLen (current_process->current_dir);	
	
	if (sz >= len + 1)
	{
		CopyOut (current_process->user_as, buf, current_process->current_dir, len+1);
		rbuf = buf;
	}
	else
	{
		SetError (ERANGE);
		rbuf = NULL;
	}

	MutexUnlock (&proc_mutex);
	
	return rbuf;
}




/*
 * Chdir(); **** 5
 */

int Chdir (char *pathname)
{
	struct Stat stat;
	struct PathInfo pi;
	struct Mount *mount;
	struct FSReq fsreq;
	int rc = -1;
	char pathname2[MAX_PATHNAME_SZ];
	

	KPRINTF ("Chdir()");

	RWReadLock (&mountlist_rwlock);

	if (CopyInStr (current_process->user_as, pathname2, pathname, MAX_PATHNAME_SZ) == 0)
	{
		if (CreatePathInfo (&pi, pathname, KEEP_CANON) == 0)
		{
			if ((mount = FindMount (pi.mountname)) != NULL)
			{
				fsreq.device = mount->handler;
				fsreq.unitp = mount->unitp;
				fsreq.cmd = FS_CMD_STAT;
				fsreq.path = pi.pathname;
				fsreq.stat = &stat;
				fsreq.as = current_process->user_as;
				fsreq.proc = current_process;
				
				DoIO (&fsreq, NULL);
				SetError (fsreq.error);
				rc = fsreq.rc;
				
				if (rc == 0)
				{
					MutexLock (&proc_mutex);
				
					if (S_ISDIR(stat.st_mode) != 0)
					{
						StrLCpy (current_process->current_dir, pi.canon, MAX_PATHNAME_SZ);
						rc = 0;
					}
					else
					{
						SetError (ENOTDIR);
						rc = -1;
					}
					
					MutexUnlock (&proc_mutex);
				}
			}
		
			FreePathInfo (&pi);
		}
	}
			
	RWUnlock (&mountlist_rwlock);
	
	return rc;
}




/*
 * Mkdir();
 */
	
int Mkdir (char *pathname, mode_t mode)
{
	struct PathInfo pi;
	struct Mount *mount;
	struct FSReq fsreq;
	int rc = -1;
	

	KPRINTF("Mkdir()");


	RWReadLock (&mountlist_rwlock);

	if (CreatePathInfo (&pi, pathname, 0) == 0)
	{
		if ((mount = FindMount (pi.mountname)) != NULL)
		{
			fsreq.device = mount->handler;
			fsreq.unitp = mount->unitp;
			fsreq.cmd = FS_CMD_MKDIR;
			fsreq.path = pi.pathname;
			fsreq.mode = mode & ~current_process->umask;
			fsreq.as = current_process->user_as;
			fsreq.proc = current_process;
			
			DoIO (&fsreq, NULL);
			SetError (fsreq.error);
			rc = fsreq.rc;
		}
					
		FreePathInfo (&pi);
	}


	RWUnlock (&mountlist_rwlock);

	return rc;
}




/*
 * Rmdir();  **** 7
 */

int Rmdir (char *pathname)
{
	struct PathInfo pi;
	struct Mount *mount;
	struct FSReq fsreq;
	int rc = -1;
	
	
	KPRINTF("Rmdir()");
	
	RWReadLock (&mountlist_rwlock);
	
	if (CreatePathInfo (&pi, pathname, 0) == 0)
	{
		if ((mount = FindMount (pi.mountname)) != NULL)
		{
			fsreq.device = mount->handler;
			fsreq.unitp = mount->unitp;
			fsreq.cmd = FS_CMD_RMDIR;
			fsreq.path = pi.pathname;
			fsreq.as = current_process->user_as;
			fsreq.proc = current_process;
			
			DoIO (&fsreq, NULL);
			SetError (fsreq.error);
			rc = fsreq.rc;
		}

		FreePathInfo (&pi);
	}

	RWUnlock (&mountlist_rwlock);

	return rc;
}



/*
 * Opendir();
 */

int Opendir (char *pathname)
{
	struct PathInfo pi;
	struct Mount *mount;
	struct FSReq fsreq;
	int fd = -1;


	KPRINTF("Opendir()");
	

	RWReadLock (&mountlist_rwlock);

	if (CreatePathInfo (&pi, pathname, 0) == 0)
	{
		if ((mount = FindMount (pi.mountname)) != NULL)
		{
			if ((fd = AllocFD (current_process, 0)) != -1)
			{
				fsreq.device = mount->handler;
				fsreq.unitp = mount->unitp;
				fsreq.cmd = FS_CMD_OPENDIR;
				fsreq.path = pi.pathname;
				fsreq.as = current_process->user_as;
				fsreq.proc = current_process;
				
				DoIO (&fsreq, NULL);
				
				if (fsreq.rc == 0)
				{
					current_process->filedesc[fd] = fsreq.filp;
					SetError (0);
				}
				else
				{
					FreeFD (current_process, fd);
					fd = -1;
					SetError (fsreq.error);
				}
			}
		}
		
		FreePathInfo (&pi);
	}	

	RWUnlock (&mountlist_rwlock);
	
	return fd;
}




/*
 * Closedir();
 */

int Closedir (int fd)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int rc;


	KPRINTF("Closedir()");

	RWReadLock (&mountlist_rwlock);

 	if ((filp = FDtoFilp(fd)) != NULL) 
	{
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_CLOSEDIR;
		fsreq.filp = filp;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;
		
		DoIO (&fsreq, NULL);
		SetError (fsreq.error);
		rc = fsreq.rc;
		
		FreeFD (current_process, fd);
	}
	else
		rc = -1;

	RWUnlock (&mountlist_rwlock);

	
	return rc;
}




/*
 * Readdir();
 */

int Readdir (int fd, struct Dirent *dirent)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int rc;
	

	KPRINTF("Readdir()");
	
	
 	if ((filp = FDtoFilp(fd)) != NULL) 
	{
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_READDIR;
		fsreq.filp = filp;
		fsreq.dirent = dirent;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;
		
		DoIO (&fsreq, NULL);
		SetError (fsreq.error);
		rc = fsreq.rc;
	}
	else
	{
		rc = -1;
	}
	

	return rc;
}




/*
 * Rewinddir();
 */
 
int Rewinddir (int fd)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int rc;

	KPRINTF("Rewinddir()");
		
	
 	if ((filp = FDtoFilp(fd)) != NULL) 
	{
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_REWINDDIR;
		fsreq.filp = filp;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;
		
		DoIO (&fsreq, NULL);
		SetError (fsreq.error);
		rc = fsreq.rc;
	}
	else
		rc = -1;
	
	
	return rc;
}




/*
 * Tcgetattr();
 */

int Tcgetattr (int fd, struct Termios *termios)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int rc;
	
	
	KPRINTF ("Tcgetattr()");	
		
	if ((filp = FDtoFilp(fd)) != NULL)
	{	
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_TCGETATTR;
		fsreq.filp = filp;
		fsreq.termios = termios;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;
		
		DoIO (&fsreq, NULL);
		SetError (fsreq.error);
		rc = fsreq.rc;
	}
	else
		rc = -1;	

	
	return rc;
}




/*
 * Tcsetattr();
 */

int Tcsetattr (int fd, int action, struct Termios *termios)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int rc;


	KPRINTF ("Tcsetattr()");	
		
	if ((filp = FDtoFilp(fd)) != NULL)
	{	
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_TCSETATTR;
		fsreq.filp = filp;
		fsreq.termios_action = action;
		fsreq.termios = termios;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;
		
		DoIO (&fsreq, NULL);
		SetError (fsreq.error);
		rc = fsreq.rc;
	}
	else
		rc = -1;	

	
	return rc;
}




/*
 * Fcntl();
 */

int Fcntl (int fd, int cmd, int arg)
{
	int rc;
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int newfd;


	KPRINTF("Fcntl()");
		
	if ((filp = FDtoFilp(fd)) != NULL)
	{
		device = (struct Device *) *(void **)filp;
		
		switch (cmd)
		{
			case F_DUPFD:	/* Duplicate fildes */
			
				KPRINTF ("Fcntl() DUPFD");
			
				if ((newfd = AllocFD (current_process, arg)) != -1)
				{
					fsreq.device = device;
					fsreq.unitp = NULL;
					fsreq.cmd = FS_CMD_DUP;
					fsreq.filp = filp;
					fsreq.as = current_process->user_as;
					fsreq.proc = current_process;
					
					DoIO (&fsreq, NULL);
					SetError (fsreq.error);
					rc = fsreq.rc;

					if (rc == 0)
					{
						current_process->filedesc[newfd] = fsreq.filp2;
						rc = newfd;
					}
					else
					{
						FreeFD (current_process, newfd);
						rc = -1;
					}
				}
				else
					rc = -1;
					
				break;
			
			case F_GETFD:	/* Get fildes flags */
				KPRINTF ("Fcntl() GETFD");
			
				if (fd >= 0 && fd < MAX_FD)
				{
					if (current_process->close_on_exec[fd] == TRUE)
						rc = FD_CLOEXEC;
					else
						rc = 0;
				}
				else
				{
					SetError (EBADF);
					rc = -1;
				}
				break;
				
			
			case F_SETFD:	/* Get fildes flags */
				KPRINTF ("Fcntl() SETFD");
				
				if (fd >= 0 && fd < MAX_FD)
				{
					if (arg & FD_CLOEXEC)
						current_process->close_on_exec[fd] = TRUE;
					else
						current_process->close_on_exec[fd] = FALSE;
					
					rc = arg;
				}
				else
				{
					SetError (EBADF);
					rc = -1;
				}
			
				break;
						
			case F_GETFL:	/* Get file flags */
				SetError (ENOSYS);
				rc = -1;
				break;
				
			case F_SETFL:	/* Set file flags */
				SetError (ENOSYS);
				rc = -1;
				break;
				
			default:
				SetError (ENOSYS);
				rc = -1;
				break;
		}
	}
	else
		rc = -1;
	
	
	return rc;
}




/*
 * Access();
 */

int Access (char *pathname, int amode)
{
	struct PathInfo pi;
	struct Stat stat;
	struct Mount *mount;
	struct FSReq fsreq;
	int rc = -1;
	
	
	RWReadLock (&mountlist_rwlock);
	
	if (CreatePathInfo (&pi, pathname, 0) == 0)
	{
		if ((mount = FindMount (pi.mountname)) != NULL)
		{
			fsreq.device = mount->handler;
			fsreq.unitp = mount->unitp;
			fsreq.cmd = FS_CMD_STAT;
			fsreq.path = pi.pathname;
			fsreq.stat = &stat;
			fsreq.as = current_process->user_as;
			fsreq.proc = current_process;
	
			DoIO (&fsreq, NULL);
			SetError (fsreq.error);
			rc = fsreq.rc;
		}
			
		FreePathInfo (&pi);
	}

	RWUnlock (&mountlist_rwlock);
	
	return rc;
}




/*
 * Ioctl();
 */

int Ioctl (int fd, int request, uintptr_t arg)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int rc;
	
	
	if ((filp = FDtoFilp(fd)) != NULL)
	{	
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_IOCTL;
		fsreq.filp = filp;
		fsreq.ioctl_request = request;
		fsreq.ioctl_arg = (void *)arg;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;
		
		DoIO (&fsreq, NULL);
		SetError (fsreq.error);
		rc = fsreq.rc;
	}
	else
		rc = -1;	

	return rc;
}




/*
 * Umask();
 */

mode_t Umask (mode_t cmask)
{
	mode_t old_cmask;
	
	old_cmask = current_process->umask;
	current_process->umask = cmask;
	
	return old_cmask;
}





/*
 * Format();
 */

int Format (char *mount_name, char *label, uint32 flags, uint32 cluster_size)
{
	struct Mount *mount;
	int rc = -1;
	struct FSReq fsreq;
	char *mount_name2;
	char *label2;	
	
	KPRINTF ("Format()");
	

	RWReadLock (&mountlist_rwlock);
	
	if ((mount_name2 = KMalloc(MAX_PATHNAME_SZ)) != NULL)
	{
		if ((label2 = KMalloc(MAX_PATHNAME_SZ)) != NULL)
		{
			if (CopyInStr (current_process->user_as, mount_name2, mount_name, MAX_PATHNAME_SZ) == 0)
			{
				if (CopyInStr (current_process->user_as, label2, label, MAX_PATHNAME_SZ) == 0)
				{
					if ((mount = FindMount (mount_name2)) != NULL)
					{
						fsreq.device = mount->handler;
						fsreq.unitp = mount->unitp;
						fsreq.cmd = FS_CMD_FORMAT;
						fsreq.format_label = label2;
						fsreq.format_flags = flags;
						fsreq.format_cluster_sz = cluster_size;
						fsreq.as = current_process->user_as;
						fsreq.proc = current_process;
						
						DoIO (&fsreq, NULL);
						rc = fsreq.rc;
													
						if (fsreq.rc != 0)
							SetError (fsreq.error);
					}
				}
			}			
			
			KFree (label2);
		}
		
		KFree (mount_name2);
	}
	
	RWUnlock (&mountlist_rwlock);
	
	return rc;
}


/*
 *
 */

int Relabel (char *mount, char *newlabel)
{
	SetError (ENOSYS);
	return -1;
}




/*
 * Rename();
 */

int Rename (char *oldpath, char *newpath)
{
	struct PathInfo pi_old, pi_new;
	struct Mount *mount;
	struct FSReq fsreq;
	int rc = -1;


	KPRINTF ("Rename()");
	

	RWReadLock (&mountlist_rwlock);

	if (CreatePathInfo (&pi_old, oldpath, 0) == 0)
	{
		if (CreatePathInfo (&pi_new, newpath, 0) == 0)
		{
			if ((mount = FindMount (pi_old.mountname)) != NULL)
			{
				if (StrCmp (pi_old.mountname, pi_new.mountname) == 0)
				{
					if (StrCmp (pi_old.pathname, pi_new.pathname) == 0)
						rc = 0;
					else
					{
						fsreq.device = mount->handler;
						fsreq.unitp = mount->unitp;
						fsreq.cmd = FS_CMD_RENAME;
						fsreq.path = pi_old.pathname;
						fsreq.path2 = pi_new.pathname;
						fsreq.as = current_process->user_as;
						fsreq.proc = current_process;

						DoIO (&fsreq, NULL);
						SetError (fsreq.error);
						rc = fsreq.rc;
					}
				}
			}
			
			FreePathInfo (&pi_new);
		}
	
		FreePathInfo (&pi_old);
	}

	RWUnlock (&mountlist_rwlock);
	
	return rc;
}




/*
 * Tcsetpgrp();
 */

int Tcsetpgrp (int fd, int foreground_grp)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int rc;
		
		
	if ((filp = FDtoFilp(fd)) != NULL)
	{	
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_TCSETPGRP;
		fsreq.filp = filp;
		fsreq.foreground_grp = foreground_grp;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;
		
		DoIO (&fsreq, NULL);
		SetError (fsreq.error);
		rc = fsreq.rc;
	}
	else
		rc = -1;	
	
	
	return rc;
}



/*
 * Tcgetpgrp();
 */

int Tcgetpgrp (int fd)
{
	void *filp;
	struct Device *device;
	struct FSReq fsreq;
	int pgrp;

	
	if ((filp = FDtoFilp(fd)) != NULL)
	{	
		device = (struct Device *) *(void **)filp;

		fsreq.device = device;
		fsreq.unitp = NULL;
		fsreq.cmd = FS_CMD_TCGETPGRP;
		fsreq.filp = filp;
		fsreq.as = current_process->user_as;
		fsreq.proc = current_process;
		
		DoIO (&fsreq, NULL);
		SetError (fsreq.error);
		
		if (fsreq.rc == 0)
			pgrp = fsreq.foreground_grp;
		else
			pgrp = -1;
	}
	else
		pgrp = -1;	
	
	
	return pgrp;
}




/*
 * Chmod();
 */
 
int Chmod (char *pathname, mode_t mode)
{
	struct PathInfo pi;
	struct Mount *mount;
	struct FSReq fsreq;
	int rc = -1;
	
	
	RWReadLock (&mountlist_rwlock);
	
	if (CreatePathInfo (&pi, pathname, 0) == 0)
	{
		if ((mount = FindMount (pi.mountname)) != NULL)
		{
			fsreq.device = mount->handler;
			fsreq.unitp = mount->unitp;
			fsreq.cmd = FS_CMD_CHOWN;
			fsreq.path = pi.pathname;
			fsreq.mode = mode;
			fsreq.as = current_process->user_as;
			fsreq.proc = current_process;
			
			DoIO (&fsreq, NULL);
			SetError (fsreq.error);
			rc = fsreq.rc;
		}
		
		FreePathInfo (&pi);
	}

	RWUnlock (&mountlist_rwlock);
	
	return rc;
}




/*
 * Chown();
 */
 
int Chown (char *pathname, uid_t owner, gid_t group)
{
	struct PathInfo pi;
	struct Mount *mount;
	struct FSReq fsreq;
	int rc = -1;
	

	RWReadLock (&mountlist_rwlock);
		
	if (CreatePathInfo (&pi, pathname, 0) == 0)
	{
		if ((mount = FindMount (pi.mountname)) != NULL)
		{
			fsreq.device = mount->handler;
			fsreq.unitp = mount->unitp;
			fsreq.cmd = FS_CMD_CHOWN;
			fsreq.path = pi.pathname;
			fsreq.owner = owner;
			fsreq.group = group;
			fsreq.as = current_process->user_as;
			fsreq.proc = current_process;
			
			DoIO (&fsreq, NULL);
			SetError (fsreq.error);
			rc = fsreq.rc;
		}
		
		FreePathInfo (&pi);
	}
	

	RWUnlock (&mountlist_rwlock);

	return rc;
}




/*
 *
 */

int Inhibit (char *pathname)
{
	struct PathInfo pi;
	struct Mount *mount;
	struct FSReq fsreq;
	int rc = -1;
	
	
	KPRINTF ("Inhibit()");
	
	RWReadLock (&mountlist_rwlock);
	
	if (CreatePathInfo (&pi, pathname, 0) == 0)
	{
		if ((mount = FindMount (pi.mountname)) != NULL)
		{
			fsreq.device = mount->handler;
			fsreq.unitp = mount->unitp;
			fsreq.cmd = FS_CMD_INHIBIT;
			fsreq.as = current_process->user_as;
			fsreq.proc = current_process;
			
			DoIO (&fsreq, NULL);
			SetError (fsreq.error);
			rc = fsreq.rc;
		}
			
		FreePathInfo (&pi);
	}

	RWUnlock (&mountlist_rwlock);
	
	return rc;
}




/*
 *
 */

int Uninhibit (char *pathname)
{
	struct PathInfo pi;
	struct Mount *mount;
	struct FSReq fsreq;
	int rc = -1;
	
	
	KPRINTF ("Inhibit()");
	
	RWReadLock (&mountlist_rwlock);
	
	if (CreatePathInfo (&pi, pathname, 0) == 0)
	{
		if ((mount = FindMount (pi.mountname)) != NULL)
		{
			fsreq.device = mount->handler;
			fsreq.unitp = mount->unitp;
			fsreq.cmd = FS_CMD_UNINHIBIT;
			fsreq.as = current_process->user_as;
			fsreq.proc = current_process;
	
			DoIO (&fsreq, NULL);
			SetError (fsreq.error);
			rc = fsreq.rc;
		}
			
		FreePathInfo (&pi);
	}

	RWUnlock (&mountlist_rwlock);
	
	return rc;
}




/*
 * SetAssign();
 */

int SetAssign (char *alias, char *path)
{
	char *alias_c, *path_c;
	struct Mount *assign;
	int rc;


	RWWriteLock (&mountlist_rwlock);
		
	if ((alias_c = AllocPathname (alias)) != NULL)
	{
		if (*path != '\0')
		{
			if ((path_c = CanonPathname (path)) != NULL)
			{
				if ((assign = FindAssign (alias_c)) == NULL)
				{
					if (AllocAssign (alias_c, path_c) == 0)
						rc = 0;
					else
						rc = -1;
				}
				else
				{
					FreeAssign (assign);
					
					if (AllocAssign (alias_c, path_c) == 0)
						rc = 0;
					else
						rc = -1;
				}
				
				KFree (path_c);
			}
			else
			{
				SetError (EINVAL);
				rc = -1;
			}
		}
		else
		{
			if ((assign = FindAssign (alias_c)) != NULL)
			{
				FreeAssign (assign);
				rc = 0;
			}
			else
			{
				SetError (ENOENT);
				rc = -1;
			}
		}

		KFree (alias_c);
	}
	else
		rc = -1;

	RWUnlock (&mountlist_rwlock);

	return rc;
}




/*
 * GetAssign();
 *
 * FIXME:  Add code to read path that assign points to.
 */

int GetAssign (int idx, char *alias, char *path)
{
	int rc;
	
		
	RWReadLock (&mountlist_rwlock);
	
	SetError (0);
	rc = -1;
	
	RWUnlock (&mountlist_rwlock);
	
	return rc;
}
