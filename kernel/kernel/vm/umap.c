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

vm_addr UMap (vm_addr addr, vm_size len, uint32 prot, uint32 flags)
{
	struct MemRegion *mr;
	struct AddressSpace *as;
	uint32 mr_flags;
	vm_addr raddr;
	
	KASSERT (current_process->user_as != &kernel_as);
	KASSERT (len != 0);
	
	addr = ALIGN_DOWN (addr, PAGE_SIZE);
	len = ALIGN_UP (len, PAGE_SIZE);
	as = current_process->user_as;
	
	mr_flags = 0;
	
	if (flags & MAP_FIXED)
		mr_flags |= MR_FLAGS_FIXED;
	
	
	MutexLock (&vm_mutex);

	if ((mr = MemRegionCreate (as, addr, len, mr_flags, prot, MR_TYPE_ANON)) != NULL)
		raddr = mr->base_addr;
	else
		raddr = MAP_FAILED;
		
	MutexUnlock (&vm_mutex);

	return raddr;
}


			

/*
 *
 */

int UUnmap (vm_offset addr)
{
	struct MemRegion *mr;
	struct AddressSpace *as;
	struct Pageframe *pf;
	int rc;
	
	
	KASSERT (current_process->user_as != &kernel_as);
	KASSERT ((addr & 0x00000fff) == 0);
	
	MutexLock (&vm_mutex);
		
	addr = ALIGN_DOWN (addr, PAGE_SIZE);
	
	as = current_process->user_as;
	mr = MemRegionFindSorted (as, addr);
								
	if (mr != NULL && mr->type != MR_TYPE_FREE)
	{
		pf = LIST_HEAD (&mr->pageframe_list);
		
		while (pf != NULL)
		{		
			PmapRemove (&as->pmap, pf->virtual_addr);
			ReleasePageframe(pf);
			
			LIST_REM_HEAD (&mr->pageframe_list, memregion_entry);
			pf = LIST_HEAD (&mr->pageframe_list);
		}
		
		PmapFlushTLBs();
		
		MemRegionDelete (as, mr);
		
		rc = 0;
	}
	else
		rc = -1;
		
	MutexUnlock (&vm_mutex);
	return rc;
}




/*
 *
 */

int UProtect (vm_addr addr, uint32 prot)
{
	struct MemRegion *mr;
	struct AddressSpace *as;
	struct Pageframe *pf;
	int rc;
	
	
	KASSERT (current_process->user_as != &kernel_as);

	MutexLock (&vm_mutex);
	
	addr = ALIGN_DOWN (addr, PAGE_SIZE);
		
	as = current_process->user_as;
	mr = MemRegionFindSorted (as, addr);

	if (mr != NULL && mr->type != MR_TYPE_FREE)
	{
		if (mr->prot != prot)
		{
			mr->prot = prot;
		
			pf = LIST_HEAD (&mr->pageframe_list);
		
			while (pf != NULL)
			{
				PmapProtect (&as->pmap, pf->virtual_addr, prot);
			
				pf = LIST_NEXT (pf, memregion_entry);
			}
		}
		
		rc = 0;
	}
	else
		rc = -1;
	
	PmapFlushTLBs();
	
	MutexUnlock (&vm_mutex);

	return rc;
}




/*
 *
 */

vm_size USizeOfMap (vm_addr addr)
{
	struct MemRegion *mr;
	struct AddressSpace *as;
	vm_size sz;
	

	KASSERT (current_process->user_as != &kernel_as);

	MutexLock (&vm_mutex);
	
	as = current_process->user_as;
	mr = MemRegionFindSorted (as, addr);
	
	if (mr != NULL)
		sz = mr->ceiling_addr - mr->base_addr;
	else
		sz = 0;
	
	
	MutexUnlock (&vm_mutex);
	
	return sz;
}



