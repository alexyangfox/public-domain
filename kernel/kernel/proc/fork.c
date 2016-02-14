#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/fs.h>
#include <kernel/vm.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>


/*
 * ** FIX: Will need floating point state duplicated
 * in ArchInitFork().  Clear FPU state in ArchInitExec()
 */

int Fork (void)
{
	struct Process *newproc;
	
	
	KPRINTF ("Fork()");
	
	if ((newproc = AllocProcess(PROC_TYPE_USER)) != NULL)
	{
		StrLCpy (newproc->exe_name, current_process->exe_name, PROC_NAME_SZ);
		
		if (*newproc->exe_name == '\0')
		{
			KPRINTF ("Fork() exe_name == 0");
			while(1) KWait(0);
		}
		
		
		if (DuplicateAddressSpace (&current_process->as, &newproc->as) == 0)
		{
			newproc->user_as = &newproc->as;
						
			if (0 == FSCreateProcess (current_process, newproc, current_process->current_dir, DUP_FD))
			{
				USigFork (current_process, newproc);
				
				if (ArchInitFork (newproc) == 0)
				{
					DisableInterrupts();
					newproc->state = PROC_STATE_READY;
					newproc->priority = 0;
					SchedReady (newproc);
					Reschedule();
					EnableInterrupts();
					
					return newproc->pid;
				}
				
				FSExitProcess (newproc);
			}
			
			FreeAddressSpace (&newproc->as);
		}
		
		FreeProcess (newproc);
	}
	
	
	return -1;
}
