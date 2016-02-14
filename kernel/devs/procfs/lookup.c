#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/kmalloc.h>
#include <kernel/dbg.h>
#include <kernel/utility.h>
#include "procfs.h"



/* Maybe dummy nodes

	root_node
	proc_node   - all files share single dummy node?
	mem_node
	
	etc, etc
*/

int ProcFSLookup (struct ProcfsLookup *lookup)
{
	char *component, *first_component, *final_component;
	int pid;
	

	first_component = lookup->pathname;

	component = first_component;
	final_component = component;
	
	while (*component != '\0')
	{
		final_component = component;
		component = Advance (component);
	}

	component = first_component;


	if (*first_component == '\0')
	{	
		lookup->isdir = 1;
		lookup->proc = NULL;
		return 0;
	}
	else
	{	
		if (component == final_component)
		{	
			pid = AtoI (component);
			
			if (pid > 0 && pid <= process_cnt)
			{
				lookup->isdir = 0;
				lookup->proc = PIDtoProc(pid);
				return 0;
			}
			else
			{
				SetError (ENOENT);
				return -1;
			}
		}
		else
		{
			SetError (ENOENT);
			return -1;
		}
	}
}
