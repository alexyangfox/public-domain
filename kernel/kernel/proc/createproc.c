#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/fs.h>
#include <kernel/kmalloc.h>
#include <kernel/i386/elf.h>
#include <kernel/i386/i386.h>
#include <kernel/utility.h>
#include <kernel/error.h>




/*
 * CreateProcess();
 */

int CreateProcess (char *filename, char **argv, char **env)
{
	int fd;
	struct Process *newproc;
	vm_addr entry_point;
	vm_addr user_stack;
	struct ArgInfo ai;
	struct AddressSpace *old_as;
	
	
	if ((newproc = AllocProcess (PROC_TYPE_USER)) != NULL)
	{
		SetProcessName (newproc, filename);
		
		KASSERT (*newproc->exe_name != '\0');
		
		if (0 == FSCreateProcess (current_process, newproc, current_process->current_dir, NO_DUP_FD))
		{
			if ((fd = Open (filename, O_RDONLY, 0)) != -1)
			{
				if (CheckELFHeaders(fd) == 0)
				{
					if (CreateArgEnv (&ai, argv, env) == 0)
					{
						KPRINTF ("CreateArgEnv() +");
						
						if (CreateAddressSpace (&newproc->as) == 0)
						{	
							KPRINTF ("CreateAddressSpace() +");
						
							newproc->user_as = &newproc->as;
							old_as = SwitchAddressSpace (newproc->user_as);
						
						
							if (PopulateAddressSpace (newproc, fd,
												&entry_point, &user_stack) == 0)
							{
								KPRINTF ("PopulateAddressSpace() +");
							
								Close (fd);
								
								if (CopyArgEnvToUserSpace (&ai, newproc->user_as) == 0)
								{
									KPRINTF ("CopyArgEnvToUserSpace() +");
								
									FreeArgEnv(&ai);
									
									USigInit (newproc);
									
									if (ArchInitUProc (newproc, entry_point, user_stack, 
												(vm_addr)ai.uargv, ai.argc,
												(vm_addr)ai.uenv, ai.envc) == 0)
									{		
										
										SwitchAddressSpace (old_as);
										
										DisableInterrupts();
										newproc->state = PROC_STATE_READY;
										SchedReady (newproc);
										Reschedule();
										EnableInterrupts();
										
										return newproc->pid;
									}
								}
							}
						
							SwitchAddressSpace (old_as);
						
							KPRINTF ("FreeAddressSpace()");
							FreeAddressSpace (&newproc->as);
						}

						KPRINTF ("FreeArgEnv()");										
						FreeArgEnv (&ai);
					}
				}
			
				Close (fd);			
			}
			
			
			KPRINTF ("FSExitProcess()");
			FSExitProcess (newproc);
		}
		
		KPRINTF ("FreeProcess()");
		FreeProcess (newproc);
	}
	
	
	return -1;
}


