#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/dbg.h>




/*
 * DoKMap();
 */
 
vm_offset KMap (vm_size len, uint32 prot)
{
	struct MemRegion *mr;

	KASSERT (len != 0);

	
	len = ALIGN_UP (len, PAGE_SIZE);
	
	MutexLock (&vm_mutex);
	
	if ((mr = MemRegionCreate (&kernel_as, 0, len, MR_FLAGS_NONE, prot, MR_TYPE_ANON)) != NULL)
	{
		mr->pageframe_hint = NULL;
		
		if (AllocPageframes (mr) == 0)
		{
			PmapKEnterRegion (mr);
			
			MutexUnlock (&vm_mutex);
			return mr->base_addr;
		}
		
		MemRegionDelete (&kernel_as, mr);
	}
	
	KPRINTF ("KMap() FAILED");
	
	MutexUnlock (&vm_mutex);
	return MAP_FAILED;
}




/*
 * DoKMapPhys();
 */

vm_offset KMapPhys (vm_addr pa, vm_size len, uint32 prot)
{
	struct MemRegion *mr;
	

	len = ALIGN_UP (len, PAGE_SIZE);
	
	if ((pa % PAGE_SIZE) != 0)
		return MAP_FAILED;
	
	MutexLock (&vm_mutex);
				
	if ((mr = MemRegionCreate (&kernel_as, 0, len, MR_FLAGS_NONE, prot, MR_TYPE_PHYS)) != NULL)
	{
		mr->phys_base_addr = pa;
	
		PmapKEnterRegion (mr);
		
		MutexUnlock (&vm_mutex);
		return mr->base_addr;
	}	

	MutexUnlock (&vm_mutex);
	return MAP_FAILED;
}




/*
 * DoKMapProtect();
 */

vm_offset KMapProtect (vm_addr addr, uint32 prot)
{
	struct MemRegion *mr;

	MutexLock (&vm_mutex);
			
	mr = MemRegionFindSorted (&kernel_as, addr);

	if (mr != NULL && mr->type != MR_TYPE_FREE)
	{
		if (mr->prot != prot)
		{
			mr->prot = prot;
			PmapKEnterRegion (mr);
		}
		
		MutexUnlock (&vm_mutex);
		return mr->base_addr;
	}	
	
	MutexUnlock (&vm_mutex);
	return MAP_FAILED;
}




/*
 * DoKUnmap()
 */
 
void KUnmap (vm_offset addr)
{
	struct MemRegion *mr;
	
	KASSERT ((addr & 0x00000fff) == 0);
	
	MutexLock (&vm_mutex);

	mr = MemRegionFindSorted (&kernel_as, addr);

	KASSERT (mr != NULL);
				
	if (mr != NULL)
	{
		if (mr->type == MR_TYPE_ANON)
		{
			PmapKRemoveRegion (mr);
			FreePageframes (mr);
		}
		else if (mr->type == MR_TYPE_PHYS)
		{
			PmapKRemoveRegion (mr);
		}
		
		
		MemRegionDelete (&kernel_as, mr);
	}
	
	
	MutexUnlock (&vm_mutex);
}



