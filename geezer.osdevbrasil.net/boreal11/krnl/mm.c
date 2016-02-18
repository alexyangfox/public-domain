/*----------------------------------------------------------------------------
KERNEL MEMORY MANAGEMENT

EXPORTS:
void init_mm(void);
void *kmalloc(unsigned size);
void *kcalloc(unsigned size);
void kfree(void *blk);
void *krealloc(void *blk, unsigned size);
----------------------------------------------------------------------------*/
#include <mltiboot.h> /* mboot_info_t, mboot_mod_t */
#include <string.h> /* NULL, memset(), memcpy() */
#include "_krnl.h" /* kprintf(), panic() */

/* IMPORTS
from KRNL.LD */
extern char g_code[], g_d_code[], g_data[];
extern char g_d_data[], g_bss[], g_d_bss[], g_end[];

/* from KSTART.S */
extern mboot_info_t *g_mboot_info;

static char *g_heap, *g_max_heap;
/*****************************************************************************
bogo-malloc (TM)
*****************************************************************************/
void *kmalloc(unsigned size)
{
	void *rv = g_heap;

	if(g_heap + size > g_max_heap)
		return NULL;
	g_heap += size;
	return rv;
}
/*****************************************************************************
*****************************************************************************/
void *kcalloc(unsigned size)
{
	void *rv;

	rv = kmalloc(size);
	if(rv != NULL)
		memset(rv, 0, size);
	return rv;
}
/*****************************************************************************
Java garbage collector :)
*****************************************************************************/
void kfree(void *blk_UNUSED)
{
}
/*****************************************************************************
*****************************************************************************/
void *krealloc(void *blk, unsigned size)
{
	void *rv;

	rv = kmalloc(size);
	if(rv == NULL)
		return rv;
/* xxx - size of this copy is size of the NEW block. If new size > old size,
a memory protection fault may occur while reading blk. */
	memcpy(rv, blk, size);
	kfree(blk);
	return rv;
}
/*****************************************************************************
PHYSICAL MEMORY MAP:

top of free RAM		    --->+-------+ 0x01000000 (16 meg; typ. RAM size)
 = top of kernel heap		|	|
 = top of extended memory	|	|
 = top of ALL memory		| free	|
				| (heap)|
bottom of free RAM	    --->+-------+
 = bottom of kernel heap	|	|
 = top of kernel modules	|	|
				| mods	|
bottom of kernel modules    --->+-------+
 = top of kernel		|	|
				| kern	|
bottom of kernel	    --->+-------+ 0x00100000 (1 meg)
 = bottom of extended memory	|	|
 = top of conventional memory	|	|
				|	|
				| conv	|
bottom of conventional memory ->+-------+ 0
 = bottom of ALL memory

Conventional memory is a "junkyard" of ROMs, non-existent upper memory,
and reserved memory areas (IVT, BDA, EBDA, Multiboot system info, etc.),
so this kernel ignores conventional memory completely.
*****************************************************************************/
DISCARDABLE_CODE(void init_mm(void))
{
	unsigned keep, discard, heap_start, i;
	mboot_mod_t *mod;

	kprintf("init_mm: ");
	if((g_mboot_info->flags & MBF_MEMORY) == 0)
		panic("bootloader did not set memory flags");
/* yes; GRUB stores these values in Kbytes... */
	kprintf("%luK conventional memory, %luK extended memory,\n",
		g_mboot_info->conv_mem, g_mboot_info->ext_mem);
/* find memory ranges used by modules
these are PHYSICAL addresses */
	heap_start = 0;
	if(g_mboot_info->flags & MBF_MODS)
	{
		for(i = 0; i < g_mboot_info->num_mods; i++)
		{
			mod = (mboot_mod_t *)(g_mboot_info->mods_adr -
				g_kvirt_to_phys) + i;
			if(mod->end_adr > heap_start)
				heap_start = mod->end_adr;
		}
	}
/* now heap_start = highest physical address of free RAM */
	heap_start -= g_kvirt_to_phys;
/* now heap_start = highest VIRTUAL address of free RAM
check against end of kernel, in case modules are loaded below kernel */
	if((unsigned)g_end > heap_start)
		heap_start = (unsigned)g_end;
/* heap_start = VIRTUAL address of start of kernel heap */
	g_heap = (char *)heap_start;
/* figure out size of kernel heap
Start with total memory size in K */
	i = 1024 + g_mboot_info->ext_mem;
/* convert to bytes. Result is highest valid physical address for RAM */
	i *= 1024;
/* convert to highest RAM virtual address */
	i -= g_kvirt_to_phys;
/* convert to */

	g_max_heap = g_heap + ((1024 + g_mboot_info->ext_mem) * 1024 -
		g_kvirt_to_phys) - heap_start;


	kprintf("\t%uK kernel heap at 0x%p\n",
		(g_max_heap - g_heap) / 1024, g_heap);
/* */
	kprintf("Kernel virtual address=0x%p, physical address=0x%p\n",
		g_d_code, g_d_code + g_kvirt_to_phys);
	keep = (g_d_data - g_code) + (g_d_bss - g_data) + (g_end - g_bss);
	discard = (g_code - g_d_code) + (g_data - g_d_data) + (g_bss - g_d_bss);
/* Addresses are either 'unsigned' (GCC 2.x) or 'unsigned long' (GCC 3.x)
Sigh. Use Harry Potter mode: cast the hell out of everything. */
	kprintf("Kernel memory:     code    data     bss   TOTAL\n"
		  "\tTOTAL\t%7u %7u %7u %7u\n",
		(unsigned)(g_d_data - g_d_code),(unsigned)(g_d_bss - g_d_data),
		(unsigned)(g_end - g_d_bss),	(unsigned)(keep + discard));
	kprintf("\tDISCARD\t%7u %7u %7u %7u\n",
		(unsigned)(g_code - g_d_code),	(unsigned)(g_data - g_d_data),
		(unsigned)(g_bss - g_d_bss),	discard);
	kprintf(   "\tKEEP\t%7u %7u %7u %7u\t(all values in bytes)\n",
		(unsigned)(g_d_data - g_code),	(unsigned)(g_d_bss - g_data),
		(unsigned)(g_end - g_bss),	keep);
}
