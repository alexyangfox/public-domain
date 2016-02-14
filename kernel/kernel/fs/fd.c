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




 /* Only needed for Open, Creat and OpenDir */
 
int AllocFD (struct Process *proc, int base)
{
	int fd;
	
	for (fd = base; fd < MAX_FD; fd++)
	{
		if (proc->filedesc[fd] == NULL)
		{
			proc->close_on_exec[fd] = FALSE;
			return fd;
		}
	}
	
	SetError (EMFILE);
	return -1;	
}




void FreeFD (struct Process *proc, int fd)
{
	proc->filedesc[fd] = NULL;
}




void *FDtoFilp (int fd)
{
	void *filp;
	
	if (fd >= 0 && fd < MAX_FD)
	{
		filp = current_process->filedesc[fd];
	}
	else
	{
		KPRINTF ("FILP out of range");
		SetError (EBADF);
		filp = NULL;
	}
	
	return filp;
}



