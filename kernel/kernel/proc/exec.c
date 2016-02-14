#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/fs.h>
#include <kernel/vm.h>
#include <kernel/kmalloc.h>
#include <kernel/error.h>
#include <kernel/utility.h>




/*
 *
 */

int Exec (char *filename, char **argv, char **env)
{
	int fd;
	vm_addr entry_point;
	vm_addr stack_pointer;
	struct ArgInfo ai;
	bool non_returnable = FALSE;
	bool not_exec = FALSE;
	
	KPRINTF ("Exec()");
	
	
	
	if ((fd = Open (filename, O_RDONLY, 0)) != -1)
	{
		if (CheckELFHeaders(fd) == 0)
		{
			if (CreateArgEnv (&ai, argv, env) == 0)
			{
				SetProcessName (current_process, filename);
			
				KASSERT (*current_process->exe_name != '\0');
			
				SwitchAddressSpace (&kernel_as);
				FreeAddressSpace (&current_process->as);
				
				non_returnable = TRUE;
				
				if (CreateAddressSpace (&current_process->as) == 0)
				{
					SwitchAddressSpace (&current_process->as);
					
					if (PopulateAddressSpace (current_process, fd, &entry_point, &stack_pointer) == 0)
					{
						Close (fd);
				
						if (CopyArgEnvToUserSpace (&ai, current_process->user_as) == 0)
						{
							FreeArgEnv(&ai);
							
							current_process->error = 0;
																			
							FSCloseOnExec (current_process);
							USigExec (current_process);
							ArchInitExec (current_process, entry_point, stack_pointer,
											(vm_addr)ai.uargv, ai.argc,
											(vm_addr)ai.uenv, ai.envc);
							
							return 0;
						}
				
					}
				
					FreeAddressSpace (&current_process->as);
				}
				
				FreeArgEnv (&ai);
			}
		}
		else
		{
			not_exec = TRUE;
		}

		Close (fd);
	}
	
	
	if (non_returnable == TRUE)
	{
		USigExit (SIGSEGV);
	}
	else
	{
		if (not_exec == TRUE)
			SetError(ENOEXEC);
	}
	
	return -1;
}



