#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/vm.h>
#include <kernel/dbg.h>




/*
 * GetPageSize();
 */

int GetPageSize (void)
{
	return PAGE_SIZE;
}




/*
 * ObtainPageframe();
 */


struct Pageframe *ObtainPageframe (uint32 va)
{
	struct Pageframe *pf;

	KASSERT (va != 0);

	pf = LIST_HEAD (&unused_pageframe_list);
			
	if ((pf = LIST_HEAD (&unused_pageframe_list)) != NULL)
	{
		LIST_REM_HEAD (&unused_pageframe_list, unused_entry);
		pf->virtual_addr = va;
		pf->state = PF_ACTIVE;
		free_pageframe_cnt --;
		return pf;
	}
	
	return NULL;
}



/*
 * ReleasePageframe();
 */

void ReleasePageframe (struct Pageframe *pf)
{
	LIST_ADD_HEAD (&unused_pageframe_list, pf, unused_entry);
	pf->virtual_addr = 0;
	pf->state = PF_FREE;
	free_pageframe_cnt ++;
}




/*
 *
 */

int AllocPageframes (struct MemRegion *mr)
{
	struct AddressSpace *as;
	struct Pageframe *pf;
	vm_addr va;
	
	as = mr->as;
	
	for (va = mr->base_addr; va < mr->ceiling_addr; va += PAGE_SIZE)
	{
		pf = ObtainPageframe (va);
				
		if (pf != NULL)
		{
			as->page_cnt++;
			pf->mr = mr;
			pf->virtual_addr = va;
			LIST_ADD_TAIL (&mr->pageframe_list, pf, memregion_entry);
		}
		else
		{
			while((pf = LIST_HEAD (&mr->pageframe_list)) != NULL)
			{			
				as->page_cnt--;
				LIST_REM_HEAD (&mr->pageframe_list, memregion_entry);
				ReleasePageframe (pf);
			}
		
			return -1;
		}
	}

	return 0;
}




/*
 *
 */

int AllocDupPageframes (struct MemRegion *mr_dst, struct MemRegion *mr_src)
{
	struct Pageframe *pf_dst, *pf_src;
	struct AddressSpace *as_src, *as_dst;
	
	as_src = mr_src->as;
	as_dst = mr_dst->as;
	
	
	pf_src = LIST_HEAD(&mr_src->pageframe_list);
	
	while (pf_src != NULL)
	{
		pf_dst = ObtainPageframe (pf_src->virtual_addr);
		
		if (pf_dst != NULL)
		{
			as_dst->page_cnt++;
			pf_dst->mr = mr_dst;
			pf_dst->virtual_addr = pf_src->virtual_addr;
			LIST_ADD_TAIL (&mr_dst->pageframe_list, pf_dst, memregion_entry);
		}
		else
		{
			while((pf_dst = LIST_HEAD (&mr_dst->pageframe_list)) != NULL)
			{			
				as_dst->page_cnt--;
				LIST_REM_HEAD (&mr_dst->pageframe_list, memregion_entry);
				ReleasePageframe (pf_dst);
			}
		
			KPANIC ("AllocDupPagframes() fail");
			return -1;
		}
		
		
		pf_src = LIST_NEXT (pf_src, memregion_entry);
	}
	
	return 0;
}





/*
 *
 */

void FreePageframes (struct MemRegion *mr)
{
	struct Pageframe *pf;
	struct AddressSpace *as;
	
	as = mr->as;
	
	while((pf = LIST_HEAD (&mr->pageframe_list)) != NULL)
	{			
		as->page_cnt--;
		LIST_REM_HEAD (&mr->pageframe_list, memregion_entry);
		ReleasePageframe (pf);
	}
}




/*
 * FindPageframe();
 */

struct Pageframe *FindPageframe (struct MemRegion *mr, vm_addr addr)
{
	struct Pageframe *pf;
	
	pf = LIST_HEAD (&mr->pageframe_list);
	
	while (pf != NULL)
	{
		if (addr == pf->virtual_addr)
			return pf;
		
		pf = LIST_NEXT (pf, memregion_entry);
	}
	
	return NULL;
}



/*
 *
 */
 
void InsertPageframe (struct MemRegion *mr, struct Pageframe *pf)
{
	struct Pageframe *next_pf;
	
	next_pf = LIST_HEAD (&mr->pageframe_list);
	
	if (next_pf == NULL)
	{
		LIST_ADD_HEAD (&mr->pageframe_list, pf, memregion_entry);
		return;
	}
	else
	{
		while (next_pf != NULL)
		{
			if (next_pf->virtual_addr > pf->virtual_addr)
			{
				LIST_INSERT_BEFORE (&mr->pageframe_list, next_pf, pf, memregion_entry);
				return;
			}
			
			next_pf = LIST_NEXT (next_pf, memregion_entry);
		}
	}
	
	LIST_ADD_TAIL (&mr->pageframe_list, pf, memregion_entry);
}


