#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/lists.h>
#include <kernel/dbg.h>
#include <kernel/config.h>




/*
 * MemRegionFindFree();
 */
 
struct MemRegion *MemRegionFindFree (struct AddressSpace *as, vm_addr addr)
{
	struct MemRegion *mr;
	
	if (as->hint != NULL
		&& as->hint->base_addr <= addr
		&& addr < as->hint->ceiling_addr)
		return as->hint;
	

	mr = LIST_HEAD (&as->free_memregion_list);
	
	while (mr != NULL)
	{
		if (mr->base_addr <= addr && addr < mr->ceiling_addr)
			break;
		
		mr = LIST_NEXT (mr, free_entry);
	}

	if (mr != NULL)
		as->hint = mr;
	
	return mr;
}




/*
 * MemRegionFindSorted();
 */
 
struct MemRegion *MemRegionFindSorted (struct AddressSpace *as, vm_addr addr)
{
	struct MemRegion *mr;
	
	if (as->hint != NULL
		&& as->hint->base_addr <= addr
		&& addr < as->hint->ceiling_addr)
		return as->hint;
	
	
	mr = LIST_HEAD (&as->sorted_memregion_list);

	while (mr != NULL)
	{
		if (mr->base_addr <= addr && addr < mr->ceiling_addr)
			break;
		
		mr = LIST_NEXT (mr, sorted_entry);
	}

	if (mr != NULL)
		as->hint = mr;

	return mr;
}

















/*
 * MemRegionCreate();
 */

struct MemRegion *MemRegionCreate (struct AddressSpace *as, vm_offset addr,
								vm_size size, uint32 flags, uint32 prot, uint32 type)
{
	struct MemRegion *mr, *mrbase, *mrtail;
	vm_offset aligned_base_addr;
	
	
	if ((flags & MR_FLAGS_FIXED) == MR_FLAGS_FIXED)
	{
		if ((mr = MemRegionFindFree(as, addr)) != NULL)
		{
			if (((addr + size) > mr->ceiling_addr) || (mr->type != MR_TYPE_FREE))
			{
				mr = NULL;
			}
		}
	}
	else
	{
		mr = LIST_HEAD (&as->free_memregion_list);
		
		while (mr != NULL)
		{
			aligned_base_addr = ALIGN_UP (mr->base_addr, MMAP_DEFAULT_ALIGN);
			
			if (mr->type == MR_TYPE_FREE &&
					mr->base_addr <= aligned_base_addr &&
						 aligned_base_addr < mr->ceiling_addr &&
							size <= (mr->ceiling_addr - aligned_base_addr))
			{
				addr = aligned_base_addr;
				break;
			}

			mr = LIST_NEXT (mr, free_entry);
		}
	}
	
	
	
	
	if (mr != NULL)
	{
		/* Allocate base and tail MemRegions beforehand */
		
		if ((mrbase = LIST_HEAD (&unused_memregion_list)) != NULL)
		{
			LIST_REM_HEAD (&unused_memregion_list, unused_entry);
			free_memregion_cnt --;

			if ((mrtail = LIST_HEAD (&unused_memregion_list)) != NULL)
			{
				LIST_REM_HEAD (&unused_memregion_list, unused_entry);
				free_memregion_cnt --;
				
				
				if (mr->base_addr < addr)
				{
					/* Keep and initialise mrbase */
					
					LIST_ADD_HEAD (&as->free_memregion_list, mrbase, free_entry);
					LIST_INSERT_BEFORE (&as->sorted_memregion_list, mr, mrbase, sorted_entry);
					mrbase->base_addr = mr->base_addr;
					mrbase->ceiling_addr = addr;

					mrbase->as = as;
					mrbase->type = MR_TYPE_FREE;
					mrbase->prot  = VM_PROT_NONE;
					mrbase->flags = MR_FLAGS_NONE;
					mrbase->pageframe_hint = NULL;

					LIST_INIT (&mrbase->pageframe_list);
				}
				else
				{
					/* Do not need mrbase */
					
					LIST_ADD_HEAD (&unused_memregion_list, mrbase, unused_entry);
					free_memregion_cnt ++;
				}
						
				if (addr + size < mr->ceiling_addr)
				{
					/* Keep and initialise mrtail */

					LIST_ADD_HEAD (&as->free_memregion_list, mrtail, free_entry);
					LIST_INSERT_AFTER (&as->sorted_memregion_list, mr, mrtail, sorted_entry);
					mrtail->base_addr = addr + size;
					mrtail->ceiling_addr = mr->ceiling_addr;

					mrbase->as = as;
					mrtail->type  = MR_TYPE_FREE;
					mrtail->prot  = VM_PROT_NONE;
					mrtail->flags = MR_FLAGS_NONE;
					mrtail->pageframe_hint = NULL;

					LIST_INIT (&mrtail->pageframe_list);
				}
				else
				{
					/* Do not need mrtail */
					
					LIST_ADD_HEAD (&unused_memregion_list, mrtail, unused_entry);
					free_memregion_cnt ++;
				}
				
				/* Initialise new mr */
				
				LIST_REM_ENTRY (&as->free_memregion_list, mr, free_entry);
				
				mr->base_addr = addr;
				mr->ceiling_addr = addr + size;
				
				mr->as = as;
				mr->type = type;
				mr->prot = prot;
				mr->flags = flags;
				mr->pageframe_hint = NULL;
						
				LIST_INIT (&mr->pageframe_list);
				
				return mr;
			}

			LIST_ADD_HEAD (&unused_memregion_list, mrbase, unused_entry);
			free_memregion_cnt ++;
		}
	}
	
	return NULL;
}




/* 
 * MemRegionDelete();
 */

void MemRegionDelete (struct AddressSpace *as, struct MemRegion *mr)
{
	struct MemRegion *mr_prev, *mr_next;

	if (as->hint == mr)
		as->hint = NULL;

		
	mr_prev = LIST_PREV (mr, sorted_entry);
	mr_next = LIST_NEXT (mr, sorted_entry);
	
	 
	if (mr_prev != NULL && mr_prev->type == MR_TYPE_FREE)
	{
		/* mr_prev is on AS Free list, destroy MR and extend prev_mr */
		
		mr_prev->ceiling_addr = mr->ceiling_addr;

		mr->as = NULL;
		mr->type = MR_TYPE_UNALLOCATED;
		LIST_REM_ENTRY (&as->sorted_memregion_list, mr, sorted_entry);
		LIST_ADD_HEAD (&unused_memregion_list, mr, unused_entry);

		free_memregion_cnt ++;

		mr = mr_prev;
	}
	else
	{
		mr->type  = MR_TYPE_FREE;
		mr->prot  = VM_PROT_NONE;
		mr->flags = MR_FLAGS_NONE;
		LIST_ADD_HEAD (&as->free_memregion_list, mr, free_entry);
	}
	
	
	if (mr_next != NULL && mr_next->type == MR_TYPE_FREE)
	{	
		/* mr_next is on AS free list, destroy mr_next and extend mr */
		
		mr->ceiling_addr = mr_next->ceiling_addr;

		mr_next->as = NULL;
		mr_next->type = MR_TYPE_UNALLOCATED;
		LIST_REM_ENTRY (&as->free_memregion_list, mr_next, free_entry);
		LIST_REM_ENTRY (&as->sorted_memregion_list, mr_next, sorted_entry);
		LIST_ADD_HEAD (&unused_memregion_list, mr_next, unused_entry);

		free_memregion_cnt ++;
	}
}
