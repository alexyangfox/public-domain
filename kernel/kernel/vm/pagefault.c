#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/vm.h>
#include <kernel/dbg.h>




/*
 * int PageFault (vm_addr addr, int direction, int priv)
 *
 * Page faults at a kernel address should never happen.  All kernel
 * pages that are allocated are immediately mapped into the page tables.
 *
 * **** Set EFAULT here ??????
 */

int PageFault (vm_addr addr, int direction, int privilege)
{
	int rc;
	struct MemRegion *mr = NULL;
	struct Pageframe *pf;
	
	
	addr = ALIGN_DOWN (addr, PAGE_SIZE);
	
	MutexLock (&vm_mutex);
	
	if (addr >= VM_KERNEL_BASE && addr <= VM_KERNEL_CEILING && privilege == 0)
	{
		rc = -1;
	}
	else if (addr >= VM_USER_BASE && addr <= VM_USER_CEILING)
	{
		mr = MemRegionFindSorted (current_process->user_as, addr);
		
		if (mr != NULL)
		{
			if (mr->type != MR_TYPE_FREE)
			{
				pf = FindPageframe (mr, addr);
				
				if (pf == NULL)
				{
					pf = ObtainPageframe (addr);
					
					if (pf != NULL)
					{
						InsertPageframe (mr, pf);
						
						KASSERT (pf->virtual_addr != 0);
						
						PmapEnter (&mr->as->pmap, pf->virtual_addr, pf->physical_addr, VM_PROT_READWRITE);
						MemSet ((void *) pf->virtual_addr, 0, PAGE_SIZE);
						PmapProtect (&mr->as->pmap, pf->virtual_addr, mr->prot);

						rc = 0;
					}
					else
					{
						rc = -1;
					}
				}
				else
				{			
					if (direction == PF_DIR_WRITE)
					{
						if (mr->prot & VM_PROT_WRITE)
						{
							PmapEnter (&mr->as->pmap, pf->virtual_addr, pf->physical_addr, mr->prot);
							rc = 0;
						}
						else
						{
							rc = -1;
						}
					}
					else
					{
						PmapEnter (&mr->as->pmap, pf->virtual_addr, pf->physical_addr, mr->prot);
						rc = 0;
					}	
				}
			}
			else
			{
				rc = -1;
			}
		}
		else
			rc = -1;
	
		PmapFlushTLBs();
	}
	else
		rc = -1;
	
	
	MutexUnlock (&vm_mutex);
	
	return rc;
}



