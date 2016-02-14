#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/config.h>
#include <kernel/kmalloc.h>
#include <kernel/i386/i386.h>
#include <kernel/i386/multiboot.h>
#include <kernel/i386/init.h>




/* Move prototypes out of header into here?
	Or give each file its own "" header for private prototypes
	and maybe private variables?
 */

uint32 CalcHeapObjectCnt (uint32 min, uint32 divisor, uint32 max);
vm_addr GetMemoryCeiling (void);
struct MemoryMap *GetNextMMEntry(struct MemoryMap *prev_mm_entry);
void SetupModulesMem(void);
void SetupFloppyDMA (void);
void SetupBootHeap (void);
void *BootAlloc (uint32 sz);
void InitPageframes (void);
void InitPagetables(void);
void InitMemRegions(void);
void InitKMapPhys (uint32 base, uint32 ceiling, uint32 prot);
void InitKMapResv (uint32 base, uint32 ceiling);
void EnablePaging(void);




/*
 * InitVM();
 *
 * Called by Init() in i386/init/main.c
 *
 * Initializes the VM subsystem.  Allocates kernel tables for processes
 * and VM structures.  Initializes the VM tables to identity map the kernel
 * and tables, assuming the kernel is linked to start at 1MB.
 *
 * Paging is enabled at the end of this function.  Processes are initialized
 * elsewhere.
 */

void InitVM (void)
{
	/* Based on the total physical memory in the system, calculate suitable
	 * values for the number pageframes structures, pagetables (and pmap desc),
	 * memregions and processes 
	 */

	mem_ceiling = GetMemoryCeiling();
	
	pageframe_cnt = mem_ceiling / PAGE_SIZE;
		
	pagetable_cnt = CalcHeapObjectCnt (PAGETABLE_MIN_CNT,
									PAGETABLE_DIVISOR_CNT,
									PAGETABLE_MAX_CNT);
	
	memregion_cnt = CalcHeapObjectCnt (MEMREGION_MIN_CNT,
									MEMREGION_DIVISOR_CNT,
									MEMREGION_MAX_CNT);
	
	process_cnt   = CalcHeapObjectCnt (PROCESS_MIN_CNT,
									PROCESS_DIVISOR_CNT,
									PROCESS_MAX_CNT);



	/* Determine the addresses of the kernel's text, data and bss sections */

	realmode_base    = 0x00001000;
	realmode_ceiling = 0x00100000;
	text_base        = ALIGN_DOWN ((vm_addr)&_stext, PAGE_SIZE);
	text_ceiling     = ALIGN_UP   ((vm_addr)&_etext, PAGE_SIZE);
	data_base        = ALIGN_DOWN ((vm_addr)&_sdata, PAGE_SIZE);
	data_ceiling     = ALIGN_UP   ((vm_addr)&_ebss,  PAGE_SIZE);
	
	
	SetupModulesMem();
	SetupFloppyDMA();  /* Floppy DMA allocated above modules or multiboot ceiling */
	SetupBootHeap();   /* Set the start of the "System Table Heap" above the
						* Floppy DMA ceiling. */	
	
	/* Allocate memory for the "System Table Heaps" */
	
	
	process    = BootAlloc (process_cnt   * sizeof (struct Process));
	memregion  = BootAlloc (memregion_cnt * sizeof (struct MemRegion));
	pageframe  = BootAlloc (pageframe_cnt * sizeof (struct Pageframe));
	pmapdesc   = BootAlloc (pagetable_cnt * sizeof (struct PmapDesc));
	pagetable  = BootAlloc (pagetable_cnt * PAGE_SIZE);

	interrupt_stack = (vm_addr) BootAlloc (INTERRUPT_STACK_SZ);
	interrupt_stack_ceiling = interrupt_stack + INTERRUPT_STACK_SZ;
		
	
	heap_base        = ALIGN_DOWN ((vm_addr)boot_heap_base, PAGE_SIZE);
	heap_ceiling     = ALIGN_UP   ((vm_addr)boot_heap_ptr, PAGE_SIZE);
		
	KPRINTF ("heap_base = %#010x", heap_base);
	KPRINTF ("heap_ceiling = %#010x", heap_ceiling);
	
		
	InitPageframes();
	InitPagetables();
	InitMemRegions();
	
	
	
	/* Create the Memregions and initialise the pagetables */

	InitKMapResv (0, realmode_base);
	InitKMapPhys (realmode_base, realmode_ceiling, VM_PROT_READWRITE);
	InitKMapResv (realmode_ceiling, text_base);
 	InitKMapPhys (text_base, text_ceiling, VM_PROT_READEXEC);
	InitKMapResv (text_ceiling, data_base);
	InitKMapPhys (data_base, data_ceiling, VM_PROT_READWRITE);
	
	if (modules_cnt == 0)
	{
		InitKMapResv (data_ceiling, floppy_dma_base);
	}
	else
	{
		InitKMapResv (data_ceiling, modules_base);
		InitKMapPhys (modules_base, modules_ceiling, VM_PROT_READ);
		InitKMapResv (modules_ceiling, floppy_dma_base);
	}
	
	InitKMapPhys (floppy_dma_base, floppy_dma_ceiling, VM_PROT_READWRITE);
	
	/* FIX: Possible gap in created MemRegion list between floppy memory
	   and the kernel heap IF the kernel heap moved above 15-16MB hole.
	   Not a problem, this area should be marked as free */
	
	InitKMapPhys (heap_base, heap_ceiling, VM_PROT_READWRITE);

	
	/* Pages above the kernel arrays are reserved upto 4MB, this is so
	 * that no allocation/freeing occurs below 4MB which allows the v86 task
	 * to have a unique pagetable for the first 4MB.  The reserved pages
	 * between the heap and 4MB are removed from the free(unused) page list.
	 * This means it won't work on systems with less than 4MB.
	 *
	 * FIX: Create function similar to InitKMapResv() that doesn't remove
	 * pages from the free list.
	 */
		
	if (heap_ceiling < 0x00400000)
	{
		InitKMapResv (heap_ceiling, 0x00400000);
	}
		
	if (cpu_is_i386 == FALSE)
	{
		SetCR0 (GetCR0() | CR0_WP);
		KPRINTF ("Write-Protect Enabled!");
	}
	
	
	if (cpu_gpe == TRUE)
	{
		SetCR4 (GetCR4() | CR4_PGE);
		KPRINTF ("Global Pages Enabled!");
	}

	
	/* Pagetables are initialized, safe to enable paging */
	
	EnablePaging();
	MutexInit (&vm_mutex);
	
	/* Allocate buffer used by VM to copy pages when forking */
	vm_temp_buf = KMap (VM_TEMP_BUF_SZ, VM_PROT_READWRITE);
	
	KPRINTF("Free Mem = %d k", (free_pageframe_cnt * PAGE_SIZE)/1024);
	
}




/*
 * CalcHeapObjectCnt();
 *
 * Used for calculating the ammount of a given kernel structure should
 * be allocated.
 */

uint32 CalcHeapObjectCnt (uint32 min, uint32 divisor, uint32 max)
{
	uint32 cnt;
	
	cnt = pageframe_cnt / divisor;
	
	if (cnt < min)
		cnt = min;
	
	if (cnt > max)
		cnt = max;

	return cnt;		
}




/* 
 * GetMemoryCeiling();
 *
 * Scans the memory list provided by the MultibootInfo structure.  Determines
 * the address of the highest page in the system.  A Pageframe structure is
 * allocated for each page based on the return value of this function.
 */

vm_addr GetMemoryCeiling (void)
{
	uint64 mem_ceiling = 0;
	uint64 new_ceiling;
	struct MemoryMap *mmap_ceiling;
	struct MemoryMap *mm;

	/* Need to ignore mem above 3/4GB, truncate value */

	if (mbi->flags & MI_MMAP_VALID)
	{
		mmap_ceiling = (struct MemoryMap *)((uint8 *)mbi->mmap_addr + mbi->mmap_length);
		mm = (struct MemoryMap *)mbi->mmap_addr;

		while (mm < mmap_ceiling)
		{
			new_ceiling = mm->base_addr + mm->length;

			if (new_ceiling > mem_ceiling && mm->type == 1)
				mem_ceiling = new_ceiling;

			mm = (struct MemoryMap *)((uint8 *)mm + mm->size + 4);
		}
	}
	else if (mbi->flags & MI_MEM_VALID)
		mem_ceiling = (mbi->mem_upper + 1024)*1024;
	else
		KPANIC("No multiboot memory map");
	
	return mem_ceiling;
}







/*
 * GetNextMMEntry();
 *
 * Advances to the next MemoryMap structure in the MultibootInfo's memory map.
 */

struct MemoryMap *GetNextMMEntry(struct MemoryMap *prev_mm_entry)
{
	struct MemoryMap *mm_entry;
	struct MemoryMap *mmap_ceiling;

	/* Need to ignore mem above 3/4GB, truncate */

	if (mbi->flags & MI_MMAP_VALID)
	{
		mmap_ceiling = (struct MemoryMap *)((uint32)mbi->mmap_addr + mbi->mmap_length);
	
		if (prev_mm_entry == NULL)
			mm_entry = (struct MemoryMap *)mbi->mmap_addr;
		else if (prev_mm_entry < mmap_ceiling)
			mm_entry = (struct MemoryMap *)((uint32)prev_mm_entry + prev_mm_entry->size + 4);
		else
			mm_entry = NULL;

		return mm_entry; 
	}
	else
		return NULL;
}




/*
 * SetupModulesMem();
 *
 * Reserves and calculates the base and ceiling of the area of memory holding the
 * modules loaded by GRUB, sets modules_base, modules_ceiling and modules_cnt.
 */

void SetupModulesMem(void)
{
	vm_addr new_ceiling;
	struct MultibootModule *mod;
	uint32 t;
	
	KPRINTF ("SetupModulesMem");

	
	if (mbi->flags & MI_MODS_VALID && mbi->mods_addr != NULL && mbi->mods_count != 0)
	{
		mod = (struct MultibootModule *)(mbi->mods_addr);

		modules_base = mod->mod_start;
		modules_ceiling = 0;
		modules_cnt = mbi->mods_count;
		
		
		for (t=0; t < modules_cnt; t++)
		{
			KPRINTF ("mod->name = %s", mod->mod_name);
			
			new_ceiling = (vm_addr)(mod->mod_end);
			if (new_ceiling > modules_ceiling) modules_ceiling = new_ceiling;

			mod++;
		}
		
		
		
		modules_base = ALIGN_DOWN(modules_base, PAGE_SIZE);
		modules_ceiling = ALIGN_UP(modules_ceiling, PAGE_SIZE);
	}
	else
	{
		modules_base = 0;
		modules_ceiling = 0;
		modules_cnt = 0;
		
	}
}





/*
 * SetupFloppyDMA()
 *
 * Reserves and calculates an area of memory for floppy DMA above the
 * modules memory.  Floppy dma buffer is 64k aligned.
 */

void SetupFloppyDMA (void)
{
	if (modules_cnt == 0)
		floppy_dma_base = ALIGN_UP(data_ceiling, 0x10000);
	else
		floppy_dma_base = ALIGN_UP(modules_ceiling, 0x10000);
	
	floppy_dma_ceiling = floppy_dma_base + FLOPPY_DMA_SZ;
	
}

 
/*
 * SetupBootHeap();
 *
 * Search for a suitable location for the boot heap.
 */

void SetupBootHeap (void)
{
	struct MemoryMap *mm;
	vm_addr req_base_addr;
	

	if (mbi->flags & MI_MMAP_VALID)
	{
		if (mem_ceiling >= MIN_MEM_FOR_16MB_BASE)
			req_base_addr = 0x1000000;
		else
			req_base_addr = floppy_dma_ceiling;
		
		boot_heap_base = req_base_addr;
		boot_heap_ptr = boot_heap_base;
		
		mm = NULL;
			
		while ((mm = GetNextMMEntry(mm)) != NULL)
		{
			if (mm->type != MB_MEM_AVAIL)
				continue;
		
			if (mm->base_addr <= req_base_addr  &&
					req_base_addr < mm->base_addr + mm->length)
			{
				boot_heap_ceiling = mm->base_addr + mm->length;
				break;
			}
		}
	}
	else if (mbi->flags & MI_MEM_VALID)
	{
		if (mbi->mem_upper * 1024 >= MIN_MEM_FOR_16MB_BASE)
			boot_heap_base = 0x1000000;
		else
			boot_heap_base = floppy_dma_ceiling;

		boot_heap_ceiling = mbi->mem_upper * 1024;
	}
	else
	{
		KPANIC("No multiboot memory map");
	}
}




/*
 * BootAlloc();
 *
 * Allocate an area of physical/virtal memory for a kernel array.
 */

void *BootAlloc (uint32 sz)
{
	void *p;
	
	p = (void *)boot_heap_ptr;
	boot_heap_ptr += ALIGN_UP (sz, PAGE_SIZE);

	if (boot_heap_ptr > boot_heap_ceiling)
	{
		KPANIC("Out of boot memory");
	}

	return p;
}





/*
 * InitPageframes();
 *
 * Initialize the array of Pageframe structures, one for each page of physical
 * memory.  Examines the Multiboot memory map and marks some pages as reserved.
 * Adds other pages to the unused_pageframe_list (the list of free pageframes).
 *
 * Pages of the kernel, modules and the system table heap are reserved later
 * by InitKMapPhys() and InitKMapResv().
 */

void InitPageframes (void)
{
	struct MemoryMap *mm;
	struct Pageframe *pf;
	uint32 t;
	vm_addr addr, base;
	vm_size size;

	free_pageframe_cnt = 0;

	LIST_INIT (&unused_pageframe_list);
	
	for (t=0; t<pageframe_cnt; t++)
	{
		(pageframe + t)->state = PF_RESERVED;
		(pageframe + t)->physical_addr = t*PAGE_SIZE;
	}
	
	mm = NULL;
	
	if (mbi->flags & MI_MMAP_VALID)
	{
		while (NULL != (mm = GetNextMMEntry (mm)))
		{
			if (mm->type != MB_MEM_AVAIL)
				continue;
			
			base = ALIGN_UP (mm->base_addr, PAGE_SIZE);
			size = ALIGN_DOWN (mm->length, PAGE_SIZE);
			
			for (addr = base; addr < (base + size); addr += PAGE_SIZE)
			{
				pf = pageframe + (addr/PAGE_SIZE);
		
				LIST_ADD_TAIL (&unused_pageframe_list, pf, unused_entry);
				pf->state = PF_FREE;
				pf->physical_addr = addr;
				pf->virtual_addr = 0;
				free_pageframe_cnt++;
			}
		}
	}
	else if (mbi->flags & MI_MEM_VALID)
	{
		base = ALIGN_UP (0, PAGE_SIZE);
		size = ALIGN_DOWN ((mbi->mem_lower * 1024), PAGE_SIZE);
		
		for (addr = base; addr < (base + size); addr += PAGE_SIZE)
		{
			pf = pageframe + (addr/PAGE_SIZE);
			
			LIST_ADD_TAIL (&unused_pageframe_list, pf, unused_entry);
			pf->state = PF_FREE;
			pf->physical_addr = addr;
			pf->virtual_addr = 0;
			free_pageframe_cnt++;
		}

		base = ALIGN_UP (0x00100000, PAGE_SIZE);
		size = ALIGN_DOWN ((mbi->mem_upper * 1024), PAGE_SIZE);

		for (addr = base; addr < (base + size); addr += PAGE_SIZE)
		{
			pf = pageframe + (addr/PAGE_SIZE);
			
			LIST_ADD_TAIL (&unused_pageframe_list, pf, unused_entry);
			pf->state = PF_FREE;
			pf->physical_addr = addr;
			pf->virtual_addr = 0;
			free_pageframe_cnt++;
		}
	}
	else
	{
		KPANIC ("No multiboot map");
	}
}




/*
 * InitPagetables()
 *
 * Allocates the page directory and pagetables needed by the kernel and
 * clears them.  Initializes the page directory to point to the page tables
 * but does not set the page table entries.
 *
 * The kernel, modules and heap are mapped by InitKMapPhys() and InitKMapResv().
 *
 */

void InitPagetables(void)
{
	uint32 t;
	uint32 pde_idx;
	uint32 *pd;
	uint32 *pt;
	
	KPRINTF ("InitPagetables()");
		
	free_pagetable_cnt = pagetable_cnt;
	LIST_INIT (&lru_pmapdesc_list);
	
	for (t=0; t < pagetable_cnt; t++)
	{
		LIST_ADD_TAIL (&lru_pmapdesc_list, (pmapdesc + t), lru_entry);
		(pmapdesc+t)->addr = pagetable + (t * (PAGE_SIZE / sizeof(uint32)));
		(pmapdesc+t)->parent_pdesc = NULL;
		(pmapdesc+t)->reference_cnt = 0;
		(pmapdesc+t)->pmap = NULL;
		(pmapdesc+t)->wired = 0;
	}
	
	lru_pagetable_cnt = pagetable_cnt;
	
	pd = PmapAllocPagetable (&kernel_as.pmap, PMAP_WIRED);
	kernel_as.pmap.page_directory = pd;
	df_tss.cr3 = (uint32)kernel_as.pmap.page_directory;
	
	
	for (pde_idx = VM_KERNEL_PDE_BASE; pde_idx < VM_KERNEL_PDE_CEILING; pde_idx ++)
	{
		pt = PmapAllocPagetable (&kernel_as.pmap, PMAP_WIRED);
		
		if (cpu_gpe == TRUE)
			*(pd + pde_idx) = ((uint32)pt & PDE_ADDR_MASK) | PG_READWRITE | PG_PRESENT | PG_GLOBAL;
		else
			*(pd + pde_idx) = ((uint32)pt & PDE_ADDR_MASK) | PG_READWRITE | PG_PRESENT;
	}


	KPRINTF ("InitPagetables() END");
}




/*
 * InitMemRegions()
 * 
 * Initialise the alloc_memregion_list and the lists belonging to the kernel
 * AddressSpace structure.  Map the necessary physical regions into the
 * kernel's address space.
 */

void InitMemRegions(void)
{
	struct MemRegion *mr;
	uint32 t;
	
	free_memregion_cnt = memregion_cnt;
	LIST_INIT (&unused_memregion_list);
	
	for (t=0; t<memregion_cnt; t++)
	{
		LIST_ADD_TAIL (&unused_memregion_list, (memregion + t), unused_entry);
	}
	
	LIST_INIT (&kernel_as.sorted_memregion_list);
	LIST_INIT (&kernel_as.free_memregion_list);
	kernel_as.hint = NULL;


	mr = LIST_HEAD (&unused_memregion_list);
	LIST_REM_HEAD (&unused_memregion_list, unused_entry);
	free_memregion_cnt--;
	
	LIST_ADD_HEAD (&kernel_as.free_memregion_list, mr, free_entry);
	LIST_ADD_HEAD (&kernel_as.sorted_memregion_list, mr, sorted_entry);
	
	mr->as = &kernel_as;
	mr->type = MR_TYPE_FREE;
	mr->base_addr = VM_KERNEL_BASE;
	mr->ceiling_addr = VM_KERNEL_CEILING;
	mr->prot  = VM_PROT_NONE;
	mr->flags = MR_FLAGS_NONE;
	mr->pageframe_hint = NULL;
}




/*
 * InitKMapPhys()
 *
 * Used to map regions of the kernel and system table heap.
 *
 * Creates a MemRegion for this memory and marks it as MR_TYPE_PHYS,
 * i.e physical mapped memory, not anonymous memory.
 * 
 * Removes the underlying Pageframes from the free memory pool but doesn't
 * attach them to the MemRegion.  In other words, the pages are reserved.
 *
 * Finally maps the pages into the kernel's pagetables using PmapKEnter(). 
 */

void InitKMapPhys (uint32 base, uint32 ceiling, uint32 prot)
{
	vm_addr va;
	vm_size sz;
	struct Pageframe *pf;
	struct MemRegion *mr;
	
	
	if (base == ceiling)
		return;
		
	sz = ceiling - base;

	if ((mr = MemRegionCreate (&kernel_as, base, sz,
				MR_FLAGS_FIXED, prot, MR_TYPE_PHYS)) != NULL)
	{
		for (va = mr->base_addr; va < mr->ceiling_addr; va += PAGE_SIZE)
		{
			pf = pageframe + (va / PAGE_SIZE);
			
			if (pf->state != PF_RESERVED)
			{
				LIST_REM_ENTRY (&unused_pageframe_list, pf, unused_entry);
				pf->state = PF_RESERVED;
				free_pageframe_cnt--;
			}
			
			PmapKEnter (va, va, prot);
		}
		
		mr->pageframe_hint = NULL;
	}
	else
	{
		KPANIC("InitKMapPhys() failure");
	}
}




/*
 * InitKMapPhys();
 *
 * Similar to InitKMapPhys() above,  creates a MemRegion and removes
 * the underlying Pageframes from the free list but doesn't map them
 * into the kernels pagetables.
 *
 * Used for reserving areas of the physical/virtual address space.
 */

void InitKMapResv (uint32 base, uint32 ceiling)
{
	vm_addr va;
	vm_size sz;
	struct Pageframe *pf;
	struct MemRegion *mr;
	
	if (base == ceiling)
		return;
	
			
	sz = ceiling - base;
		
	if ((mr = MemRegionCreate (&kernel_as, base, sz,
				MR_FLAGS_FIXED, VM_PROT_NONE, MR_TYPE_RESERVED)) != NULL)
	{
		for (va = mr->base_addr; va < mr->ceiling_addr; va += PAGE_SIZE)
		{	
			pf = pageframe + (va / PAGE_SIZE);
					
			if (pf->state != PF_RESERVED)
			{
				LIST_REM_ENTRY (&unused_pageframe_list, pf, unused_entry);
				pf->state = PF_RESERVED;
				free_pageframe_cnt--;
			}

			PmapKRemove (va);
		}

		mr->pageframe_hint = NULL;
	}
	else
	{
		KPANIC("InitKMapResv() failure");
	}
}




/*
 * EnablePaging()
 */

void EnablePaging (void)
{
	uint32 cr0;
	
	SetCR3 ((uint32)kernel_as.pmap.page_directory);

	cr0 = GetCR0();

	SetCR0((cr0 | CR0_PG | CR0_WP));
	
	PmapFlushKernelTLBs();
}


