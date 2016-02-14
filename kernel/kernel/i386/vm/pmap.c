#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/i386/i386.h>
#include <kernel/dbg.h>
#include <kernel/utility.h>
#include <kernel/proc.h>




/*
 * For debugging
 */
 
int PmapFindPTE (vm_addr va)
{
	uint32 *pd, *pt;
	uint32 pde_idx, pte_idx;
		
	pde_idx = (va >> PDE_SHIFT) & PDE_MASK;
	pte_idx = (va >> PTE_SHIFT) & PTE_MASK;
	
	
	pd = (uint32 *)(GetCR3() & PDE_ADDR_MASK);
	
		
	if ((*(pd + pde_idx) & PG_PRESENT) == 0) 
	{
		KPRINTF ("PDE not present");
		KPRINTF ("PDE = %#010x", *(pd + pde_idx));
	}
	else
	{
		pt = (uint32 *)(*(pd + pde_idx) & PDE_ADDR_MASK);
		
		KPRINTF ("PDE = %#010x", *(pd + pde_idx));
		KPRINTF ("PT = %#010x", pt);
		KPRINTF ("PTE = %#010x", *(pt + pte_idx));
	}
	
	while (1);
	
	return 0;
}




/*
 *  PmapCreate();
 */
 
bool PmapInit (struct AddressSpace *as)
{
	uint32 *kernel_pd;
	uint32 *pd;
	uint32 pde_idx;
	struct PmapDesc *pd_desc;
	
	
	kernel_pd = kernel_as.pmap.page_directory;
	
	as->pmap.page_directory = NULL;
	LIST_INIT (&as->pmap.pmapdesc_list);
	
	
	if ((pd = PmapAllocPagetable (&as->pmap, PMAP_WIRED)) != NULL)
	{
		as->pmap.page_directory = pd;
				
		for (pde_idx = VM_KERNEL_PDE_BASE; pde_idx < VM_KERNEL_PDE_CEILING; pde_idx++)
			*(pd + pde_idx) = *(kernel_pd + pde_idx);
				
		pd_desc = pmapdesc + (((vm_addr)pd - (vm_addr)pagetable)/PAGE_SIZE);
		pd_desc->pde_idx = 0;
		pd_desc->parent_pdesc = NULL;
		pd_desc->reference_cnt = 0;
		
		return TRUE;
	}

	KPRINTF ("Pmap failed to allocate page directory");

	return FALSE;
}



/*
 * PmapDestroy();
 */

void PmapDestroy (struct AddressSpace *as)
{
	uint32 *pd;
	uint32 *pt;
	uint32 pde_idx;
	
	
	pd = as->pmap.page_directory;
	
	for (pde_idx = VM_USER_PDE_BASE; pde_idx < VM_USER_PDE_CEILING; pde_idx ++)
	{
		if ((*(pd + pde_idx) & PG_PRESENT) != 0)
		{
			pt = (uint32 *)(*(pd + pde_idx) & PDE_ADDR_MASK);
			PmapFreePagetable (&as->pmap, pt);
		}
	}
	
	PmapFreePagetable (&as->pmap, pd);
}




/*
 *  PmapKEnter();
 */

void PmapKEnter (vm_offset va, vm_offset pa, uint32 prot)
{
	uint32 *pd, *pt;
	uint32 pde_idx, pte_idx;
	uint32 pde_bits;
	
	
	KASSERT (VM_KERNEL_BASE <= va && va < VM_KERNEL_CEILING);
	
	pde_idx = (va >> PDE_SHIFT) & PDE_MASK;
	pte_idx = (va >> PTE_SHIFT) & PTE_MASK;
	
	pd = kernel_as.pmap.page_directory;
	pt = (uint32 *)((*(pd + pde_idx)) & PDE_ADDR_MASK);

	pde_bits = PG_PRESENT;
		
	if (cpu_gpe == TRUE)
		pde_bits |= PG_GLOBAL;
		
	if (prot & VM_PROT_WRITE)
		pde_bits |= PG_READWRITE;
	
	*(pt + pte_idx) = (pa & PTE_ADDR_MASK) | pde_bits;
}




/*
 *  PmapKProtect();
 */

void PmapKProtect (vm_offset va, uint32 prot)
{
	uint32 *pd, *pt;
	uint32 pde_idx, pte_idx;
	uint32 pa;
	uint32 pde_bits;
	
	
	KASSERT (VM_KERNEL_BASE <= va && va < VM_KERNEL_CEILING);
	
	pde_idx = (va >> PDE_SHIFT) & PDE_MASK;
	pte_idx = (va >> PTE_SHIFT) & PTE_MASK;
	
	pd = kernel_as.pmap.page_directory;
	pt = (uint32 *)(*(pd + pde_idx) & PDE_ADDR_MASK);
	
	pa = *(pt + pte_idx) & PTE_ADDR_MASK;
		
	pde_bits = PG_PRESENT;
		
	if (cpu_gpe == TRUE)
		pde_bits |= PG_GLOBAL;
		
	if (prot & VM_PROT_WRITE)
		pde_bits |= PG_READWRITE;
	
	*(pt + pte_idx) = (pa & PTE_ADDR_MASK) | pde_bits;
}




/*
 *  PmapKRemove();
 */
 
void PmapKRemove (vm_offset va)
{
	uint32 *pd, *pt;
	uint32 pde_idx, pte_idx;
	
	
	KASSERT (VM_KERNEL_BASE <= va && va < VM_KERNEL_CEILING);
	
	pde_idx = (va >> PDE_SHIFT) & PDE_MASK;
	pte_idx = (va >> PTE_SHIFT) & PTE_MASK;
	
	pd = kernel_as.pmap.page_directory;
	pt = (uint32 *)(*(pd + pde_idx) & PDE_ADDR_MASK);
	
	*(pt + pte_idx) = 0 & ~PG_PRESENT;
}




/*
 *  PmapEnter ();
 *
 * Implement LRU pagetable code or not?
 */


bool PmapEnter (struct Pmap *pmap, vm_offset va, vm_offset pa, uint32 prot)
{
	uint32 *pd, *pt;
	uint32 pde_idx, pte_idx;
	struct PmapDesc *pt_desc, *pd_desc;
	uint32 pde_bits;
	
	
	KASSERT (VM_USER_BASE <= va && va < VM_USER_CEILING);
	
	pde_idx = (va >> PDE_SHIFT) & PDE_MASK;
	pte_idx = (va >> PTE_SHIFT) & PTE_MASK;
	
	
	pd = pmap->page_directory;
		
	if ((*(pd + pde_idx) & PG_PRESENT) == 0) 
	{
		if ((pt = PmapAllocPagetable (pmap, PMAP_THROWAWAY)) == NULL)
			return FALSE;
		
		*(pd + pde_idx) = ((uint32)pt & PDE_ADDR_MASK) | PG_USER | PG_READWRITE | PG_PRESENT;
		
		pd_desc = pmapdesc + (((vm_addr)pd - (vm_addr)pagetable)/PAGE_SIZE);
		pd_desc->reference_cnt ++;
		
		pt_desc = pmapdesc + (((vm_addr)pt - (vm_addr)pagetable)/PAGE_SIZE);
		pt_desc->pde_idx = pde_idx;
		pt_desc->parent_pdesc = pd_desc;
	}
	else
	{
		pt = (uint32 *)(*(pd + pde_idx) & PDE_ADDR_MASK);
		pt_desc = pmapdesc + (((vm_addr)pt - (vm_addr)pagetable)/PAGE_SIZE);
	}
	
	
	pt_desc->reference_cnt ++;
	
	pde_bits = PG_USER | PG_PRESENT;
	
	if (prot & VM_PROT_WRITE)
		pde_bits |= PG_READWRITE;
	
	*(pt + pte_idx) = (pa & PTE_ADDR_MASK) | pde_bits;
	
	return TRUE;
}




/*
 * PmapProtect();
 */

bool PmapProtect (struct Pmap *pmap, vm_offset va, uint32 prot)
{
	uint32 *pd, *pt;
	uint32 pde_idx, pte_idx;
	uint32 pa;
	uint32 pde_bits;
	

	KASSERT (VM_USER_BASE <= va && va < VM_USER_CEILING);
	
	pde_idx = (va >> PDE_SHIFT) & PDE_MASK;
	pte_idx = (va >> PTE_SHIFT) & PTE_MASK;
	
	pd = pmap->page_directory;
	
	if ((*(pd + pde_idx) & PG_PRESENT)) 
	{
		pt = (uint32 *)(*(pd + pde_idx) & PDE_ADDR_MASK);
		
		if ((*(pt + pte_idx) & PG_PRESENT)) 
		{
			pa = *(pt + pte_idx) & PTE_ADDR_MASK;
			
			pde_bits = PG_PRESENT | PG_USER;
						
			if (prot & VM_PROT_WRITE)
				pde_bits |= PG_READWRITE;
			
			*(pt + pte_idx) = (pa & PTE_ADDR_MASK) | pde_bits;
			
			return TRUE;
		}
	}
		
	return FALSE;
}




/*
 *  PmapRemove ();
 */

bool PmapRemove (struct Pmap *pmap, vm_offset va)
{
	uint32 *pd, *pt;
	uint32 pde_idx, pte_idx;
	struct PmapDesc *pt_desc;


	KASSERT (VM_USER_BASE <= va && va < VM_USER_CEILING);
	
	pde_idx = (va >> PDE_SHIFT) & PDE_MASK;
	pte_idx = (va >> PTE_SHIFT) & PTE_MASK;
	
	pd = pmap->page_directory;
	
	if ((*(pd + pde_idx) & PG_PRESENT)) 
	{
		pt = (uint32 *)(*(pd + pde_idx) & PDE_ADDR_MASK);
		
		if ((*(pt + pte_idx) & PG_PRESENT)) 
		{
			*(pt + pte_idx) = 0 & ~PG_PRESENT;
						
			pt_desc = pmapdesc + (((vm_addr)pt - (vm_addr)pagetable)/PAGE_SIZE);
			pt_desc->reference_cnt --;
			
			if (pt_desc->reference_cnt == 0)
			{
				*(pd + pde_idx) = 0 & ~PG_PRESENT;
				PmapFreePagetable (pmap, pt);
			}
			
			return TRUE;
		}
	}
		
	return FALSE;
}




/*
 * PmapAllocPagetable();   LRU free list
 *
 */
 
uint32 *PmapAllocPagetable (struct Pmap *pmap, bool wired)
{
	struct PmapDesc *pdesc, *parent_pdesc;
	uint32 *addr;
	uint32 *pd;
	uint32 pde_idx;
	
		
	pdesc = LIST_TAIL (&lru_pmapdesc_list);
		
	if (pdesc != NULL)
	{
		LIST_REM_TAIL (&lru_pmapdesc_list, lru_entry);
		LIST_ADD_HEAD (&pmap->pmapdesc_list, pdesc, pmap_entry);
				
		if (wired == 0)
		{
			LIST_ADD_HEAD (&lru_pmapdesc_list, pdesc, lru_entry);
		}
		else
		{
			lru_pagetable_cnt --;
		}

			
		parent_pdesc = pdesc->parent_pdesc;
	

		if (parent_pdesc != NULL)
		{
			pd = parent_pdesc->addr;
			pde_idx = pdesc->pde_idx;
			*(pd + pde_idx) = 0 & ~PG_PRESENT;
			parent_pdesc->reference_cnt--;
			PmapFlushKernelTLBs();
		}

		pdesc->reference_cnt = 0;
		pdesc->pmap = pmap;
		pdesc->wired = wired;
		
		addr = (uint32*)(pdesc->addr);
		
		MemSet (addr, 0, PAGE_SIZE);
		

		return pdesc->addr;
	}
	
	KPANIC ("PmapAllocPagetable() - out of pagetables");
	
	return NULL;
}




/*
 * PmapAllocPagetable();
 */

void PmapFreePagetable (struct Pmap *pmap, uint32 *pt)
{
	struct PmapDesc *pdesc;
	
	pdesc = pmapdesc + (((vm_addr)pt - (vm_addr)pagetable)/PAGE_SIZE);
	
	LIST_REM_ENTRY (&pmap->pmapdesc_list, pdesc, pmap_entry);
		
	if (pdesc->wired == 1)
	{
		lru_pagetable_cnt ++;
		LIST_ADD_TAIL (&lru_pmapdesc_list, pdesc, lru_entry);
	}
	
	pdesc->parent_pdesc = NULL;
	pdesc->pde_idx = 0;
	pdesc->reference_cnt = 0;
	pdesc->pmap = NULL;
	pdesc->wired = 0;
}




/*
 * PmapFlushUserTLBs()
 */

void PmapFlushTLBs (void)
{
	SetCR3 (GetCR3());
}




/*
 * PmapFlushKernelTLBs()
 */

void PmapFlushKernelTLBs (void)
{
	if (cpu_gpe == TRUE)
		SetCR4 (GetCR4() & ~CR4_PGE);
		
	SetCR3 (GetCR3());
	
	if (cpu_gpe == TRUE)
		SetCR4 (GetCR4() | CR4_PGE);
}




/*
 * PmapSwitch();
 */

void PmapSwitch (struct Pmap *pmap)
{
	KASSERT (pmap != NULL);
	
	SetCR3 ((uint32)pmap->page_directory);
}




/*
 * Used mostly when loading user-mode code/read-only data.
 * The I386 processors allow the kernel to any user-mode
 * page even if it is marked as READ_ONLY.
 *
 * Later processors have a Write-Protect bit in the CR0
 * register that causes a page fault whenever the kernel
 * attempts to write to a read-only page.
 *
 * WriteProtectDisable()/WriteProtectEnable() are wrapped
 * around sections of the process loader and UMap() 
 * ClearPages() code.  This may be replaced with calls
 * to UProtect() instead.
 */

void WriteProtectEnable (void)
{
	if (cpu_is_i386 == FALSE)
		SetCR0 (GetCR0() | CR0_WP);
}




/*
 *
 */

void WriteProtectDisable (void)
{
	if (cpu_is_i386 == FALSE)
		SetCR0 (GetCR0() & ~CR0_WP);
}







