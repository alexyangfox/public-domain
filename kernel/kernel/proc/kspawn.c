#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/fs.h>
#include <kernel/kmalloc.h>
#include <kernel/i386/elf.h>
#include <kernel/i386/i386.h>
#include <kernel/utility.h>


/*
 * KSpawn();
 */

int KSpawn (int32 (*func)(void *arg), void *arg, int32 priority, char *name)
{
	struct Process *newproc;
	
	KPRINTF ("KSpawn()");
		
	if ((newproc = AllocProcess(PROC_TYPE_KERNEL)) != NULL)
	{
		StrLCpy (newproc->exe_name, name, PROC_NAME_SZ);
			
		KASSERT (*newproc->exe_name != '\0');
			
		if (0 == FSCreateProcess (current_process, newproc, current_process->current_dir, NO_DUP_FD))
		{
			if (CreateNullAddressSpace (&newproc->as) == 0)
			{	
				newproc->user_as = &kernel_as;
					
				if (0 == ArchInitKProc (newproc, func, arg))
				{	
					DisableInterrupts();
					newproc->priority = priority;
					newproc->state = PROC_STATE_READY;
					SchedReady (newproc);
					Reschedule();
					EnableInterrupts();
					
					return newproc->pid;
				}
				
				FreeAddressSpace (&newproc->as);
			}
			
			FSExitProcess (newproc);
		}
		
		FreeProcess (newproc);
	}

	
	return -1;
}








