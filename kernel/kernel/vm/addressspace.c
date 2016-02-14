#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/vm.h>
#include <kernel/proc.h>
#include <kernel/utility.h>
#include <kernel/dbg.h>




/*
 * SwitchAddressSpace();
 */

struct AddressSpace *SwitchAddressSpace (struct AddressSpace *as)
{
	struct AddressSpace *old_as;
	uint32 interrupt_state;
	
	KASSERT (as != NULL);
	
	interrupt_state = DisableInterrupts();
	
	old_as = current_process->user_as;
	
	if (as != old_as)
	{
		current_process->user_as = as;
		PmapSwitch (&as->pmap);
	}
	
	RestoreInterrupts (interrupt_state);
	
	return old_as;
}




/*
 *  CreateAddressSpace ();
 *
 * Initialise a vanilla address space with no content,  single free MemRegion.
 *	Set proc->as_valid = TRUE  (in proc code)  Unless we use proc->type.
 *
 */
 
int CreateAddressSpace (struct AddressSpace *as)
{
	struct MemRegion *mr;
	
	KPRINTF ("CreateAddressSpace()");
	
	MutexLock (&vm_mutex);
		
	if (PmapInit(as) == TRUE)
	{
		LIST_INIT (&as->sorted_memregion_list);
		LIST_INIT (&as->free_memregion_list);
		as->hint = NULL;
		
		as->page_cnt = 0;
		
		if ((mr = LIST_HEAD (&unused_memregion_list)) != NULL)
		{
			LIST_REM_HEAD (&unused_memregion_list, unused_entry);
			LIST_ADD_HEAD (&as->free_memregion_list, mr, free_entry);
			LIST_ADD_HEAD (&as->sorted_memregion_list, mr, sorted_entry);
			free_memregion_cnt --;
			
			LIST_INIT (&mr->pageframe_list);
			
			mr->as = as;
			mr->type = MR_TYPE_FREE;
			mr->prot = VM_PROT_NONE;
			mr->flags = MR_FLAGS_NONE;

			mr->base_addr = VM_USER_BASE;
			mr->ceiling_addr = VM_USER_CEILING;
		
			
			as->active = TRUE;
		
			MutexUnlock (&vm_mutex);
			return 0;
		}
		
		PmapDestroy(as);
	}

	as->active = FALSE;

	KPRINTF ("Failed to allocate address-space");

	MutexUnlock (&vm_mutex);
	return -1;
}




/*
 *
 */
 
int CreateNullAddressSpace (struct AddressSpace *as)
{
	as->active = FALSE;
	as->page_cnt = 0;
	return 0;
}




/*
 * FreeAddressSpace ();
 *
 * Must be in a different address-space, such as kernel's safe AS.
 *
 * Set proc->as_valid = FALSE  (in proc code)
 *
 * Destroy the Pmap
 * Release all pageframes
 * Return all MemRegions to alloc_pool
 */
 
void FreeAddressSpace (struct AddressSpace *as)
{
	struct MemRegion *mr;
	struct Pageframe *pf;
	
		
	if (as->active == FALSE)
		return;
	
	MutexLock (&vm_mutex);

	while ((mr = LIST_HEAD (&as->sorted_memregion_list)) != NULL)
	{
		while ((pf = LIST_HEAD (&mr->pageframe_list)) != NULL)
		{
			LIST_REM_HEAD (&mr->pageframe_list, memregion_entry);
			pf->virtual_addr = 0;
			pf->state = PF_FREE;
			LIST_ADD_HEAD (&unused_pageframe_list, pf, unused_entry);
						
			free_pageframe_cnt ++;
		}
		
		LIST_REM_ENTRY (&as->sorted_memregion_list, mr, sorted_entry);
		LIST_ADD_HEAD (&unused_memregion_list, mr, unused_entry);
		free_memregion_cnt ++;
	}

	PmapDestroy (as);

	as->active = FALSE;
	
	MutexUnlock (&vm_mutex);
	
	
}






/*
 * Allocate address-space, memregions and pageframes.
 * unlock vm
 *
 * Copy the contents of each 'anonymous' allocated page from the
 * source address space to the destination address space via a
 * kernel buffer.
 *
 * Could then implement basic Copy-On-Write with on Pageframes
 * and no pv_entry structures.
 */
	
int DuplicateAddressSpace (struct AddressSpace *src_as, struct AddressSpace *dst_as)
{
	struct MemRegion *mr;
	struct Pageframe *pf;
	int error = 0;
	uint32 prot;
	struct AddressSpace *old_as;
	
	
	if (AllocDupAddressSpace (src_as, dst_as) == 0)
	{
		mr = LIST_HEAD (&dst_as->sorted_memregion_list);
		
		while (mr != NULL && error == 0)
		{
			if (mr->type == MR_TYPE_ANON)
			{	
				prot = mr->prot;
				
				old_as = SwitchAddressSpace (dst_as);
				UProtect (mr->base_addr, VM_PROT_READWRITE);
				SwitchAddressSpace (old_as);
				
				/* IMPROVEMENT:
				 *
				 * Try to copy upto 64k (VM_TEMP_BUF_SZ) worth of linear
				 * pages at a time, reduces number of task switches
				 * and concentrates on inner loops
				 */
				
				pf = LIST_HEAD (&mr->pageframe_list);
				
				while (pf != NULL && error == 0)
				{
					error = CopyIn (src_as, (void *)vm_temp_buf, (void *)pf->virtual_addr, PAGE_SIZE);

					if (error == 0)
						error = CopyOut (dst_as, (void *)pf->virtual_addr, (void *)vm_temp_buf, PAGE_SIZE);
					
					pf = LIST_NEXT (pf, memregion_entry);
				}
				
				
				old_as = SwitchAddressSpace (dst_as);
				UProtect (mr->base_addr, prot);
				SwitchAddressSpace (old_as);
			}
			
			mr = LIST_NEXT (mr, sorted_entry);
		}
		
		if (error != 0)
			FreeAddressSpace (dst_as);
	}
	else
		error = -1;

	KASSERT (error != -1);
	
	return error;
}
	
	


/*
 * AllocDupAddressSpace();
 *
 * Allocates the memregion structures and pageframes to create a duplicate
 * address space.  The actual copying of page contents is done by
 * DuplicateAddressSpace().
 */

int AllocDupAddressSpace (struct AddressSpace *src_as, struct AddressSpace *dst_as)
{	
	struct MemRegion *src_mr, *dst_mr;
	int error = 0;
	
	
	MutexLock (&vm_mutex);
		
	if (PmapInit (dst_as) == TRUE)
	{
		LIST_INIT (&dst_as->sorted_memregion_list);
		LIST_INIT (&dst_as->free_memregion_list);
		dst_as->hint = NULL;
		dst_as->page_cnt = src_as->page_cnt;
		
		src_mr = LIST_HEAD (&current_process->user_as->sorted_memregion_list);
		
		while (src_mr != NULL && error == 0)
		{
			if ((dst_mr = LIST_HEAD (&unused_memregion_list)) != NULL)
			{
				LIST_REM_HEAD (&unused_memregion_list, unused_entry);
				LIST_ADD_TAIL (&dst_as->sorted_memregion_list, dst_mr, sorted_entry);
				free_memregion_cnt--;
								
				dst_mr->base_addr = src_mr->base_addr;
				dst_mr->ceiling_addr = src_mr->ceiling_addr;
				dst_mr->as    = dst_as;
				dst_mr->type  = src_mr->type;
				dst_mr->prot  = src_mr->prot;
				dst_mr->flags = src_mr->flags;
				dst_mr->pageframe_hint = NULL;
				LIST_INIT (&dst_mr->pageframe_list);
				
				if (dst_mr->type == MR_TYPE_ANON)
				{
					if (AllocDupPageframes (dst_mr, src_mr) != 0)
					{
						error = -1;
					}
				}
				else if (dst_mr->type == MR_TYPE_FREE)
				{
					LIST_ADD_TAIL (&dst_as->free_memregion_list, dst_mr, free_entry);
				}
			}
			else
			{
				error = -1;
			}
			
			src_mr = LIST_NEXT (src_mr, sorted_entry);
		}
	}
	else
	{
		error = -1;
	}
	
	MutexUnlock (&vm_mutex);

	KASSERT (error != -1);

	return error;
}




