#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/sync.h>
#include <kernel/arch.h>
#include <kernel/vm.h>
#include <kernel/proc.h>
#include <kernel/utility.h>
#include <kernel/dbg.h>




/*
 *
 */

void PmapKEnterRegion (struct MemRegion *mr)
{
	struct Pageframe *pf;
	vm_addr va, pa;

	pa = mr->phys_base_addr;
	va = mr->base_addr;
	
	
	if (mr->type == MR_TYPE_ANON)
	{
		pf = LIST_HEAD(&mr->pageframe_list);
		
		while (pf != NULL)
		{
			PmapKEnter (pf->virtual_addr, pf->physical_addr, mr->prot);
			pf = LIST_NEXT(pf, memregion_entry);
		}
		
	}
	else if (mr->type == MR_TYPE_PHYS)
	{
		for (va = mr->base_addr; va < mr->ceiling_addr; va += PAGE_SIZE)
		{
			PmapKEnter (va, pa, mr->prot); 
			pa += PAGE_SIZE;
		}
	}
	
	/* FIX: Not needed when mapping? */
	PmapFlushKernelTLBs();
}






/*
 * Might remove this.
 */

void PmapKRemoveRegion (struct MemRegion *mr)
{
	vm_addr va;

	for (va = mr->base_addr; va < mr->ceiling_addr; va += PAGE_SIZE)
	{
		PmapKRemove (va); 
	}
	
	PmapFlushKernelTLBs();
}






/*
 *
 */

int PmapEnterRegion (struct MemRegion *mr)
{
	struct Pageframe *pf;
	
	
	pf = LIST_HEAD(&mr->pageframe_list);
	
	while (pf != NULL)
	{
		if (PmapEnter (&mr->as->pmap, pf->virtual_addr, pf->physical_addr, mr->prot) == FALSE)
		{
			PmapFlushTLBs();
			return -1;
		}
		
		pf = LIST_NEXT(pf, memregion_entry);
	}
		
	PmapFlushTLBs();
	return 0;

}



/*
 *
 */

void PmapRemoveRegion (struct MemRegion *mr)
{
	vm_addr va;
	
	KPRINTF ("PmapRemoveRegion()");
	
	for (va = mr->base_addr; va < mr->ceiling_addr; va += PAGE_SIZE)
	{
		PmapRemove (&mr->as->pmap, va); 
	}
	
	/* FIX:  Why Full flush of all/kernel TLBs? */
	PmapFlushTLBs();
}







