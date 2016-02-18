/*----------------------------------------------------------------------------
TASKS (=ADDRESS SPACE + KERNEL THREAD + I/O)

EXPORTS:
int create_task(void);
void destroy_task(task_t *t);
----------------------------------------------------------------------------*/
#include <string.h> /* NULL, memcpy(), memset() */
#include <errno.h> /* ERR_... */
#include "_krnl.h"

/* from ASPACE.C */
aspace_t *create_aspace(void);
void destroy_aspace(aspace_t *as);

/* from ELF.C */
int load_elf_exec(char *image, unsigned *entry, aspace_t *as);

/* from PECOFF.C */
int load_pe_exec(char *image, unsigned *entry, aspace_t *as);

/* from COFF.C */
int load_coff_exec(char *image, unsigned *entry, aspace_t *as);

/* from MM.C */
void *kmalloc(unsigned size);
void kfree(void *blk);

/* from THREADS.C */
task_t *create_thread(unsigned init_eip, int priority);
void destroy_thread(task_t *t);

/* from KSTART.S */
void to_user(void);

/* from KRNL.LD */
extern char g_code[];

#define	USER_STACK_SIZE	8192
#define	USER_BASE	0x400000
/*****************************************************************************
*****************************************************************************/
int create_task(char *image)
{
	unsigned entry, lowest, highest, i, uvirt_to_kvirt;
	uregs_t *uregs;
	kregs_t *kregs;
	aspace_t *as;
	sect_t *sect;
	task_t *t;
	int err;

/* STEP 1: create address space */
	as = create_aspace();
	if(as == NULL)
	{
		kprintf("create_task: can't create address space\n");
		return -ERR_NO_MEM;
	}
/* STEP 2: validate executable file, get file/memory sections
try loading as COFF */
	err = load_coff_exec(image, &entry, as);
	if(err)
	{
/* try loading as Win32 PE COFF */
		err = load_pe_exec(image, &entry, as);
		if(err)
		{
/* try loading as ELF */
			err = load_elf_exec(image, &entry, as);
			if(err)
			{
//				kprintf("create_task: module is not a valid "
	//				"executable file\n");
				destroy_aspace(as);
				return +1;
			}
		}
	}
/* STEP 3: scan sections... */
	lowest = -1u;
	highest = 0;
	for(i = 0; i < as->num_sects; i++)
	{
		sect = as->sect + i;
/* ...check if section is in userland (USER_BASE ... g_code) */
		if(sect->adr < USER_BASE)
		{
			kprintf("create_task: section %u at adr "
				"0x%X, must be at or above 0x%X\n",
				i, sect->adr, USER_BASE);
			destroy_aspace(as);
			return -ERR_EXEC;
		}
		if(sect->adr + sect->size >= (unsigned)g_code)
		{
			kprintf("create_task: section %u at adr "
				"0x%X, must be below 0x%p\n",
				i, sect->adr + sect->size, g_code);
			destroy_aspace(as);
			return -ERR_EXEC;
		}
/* ...find lowest and highest virtual addresses */
		if(sect->adr < lowest)
			lowest = sect->adr;
		if(sect->adr + sect->size > highest)
			highest = sect->adr + sect->size;
	}
	as->virt_adr = lowest;
// xxx - separate sect_t for stack
// xxx - add sect_t for heap, too
/* convert highest virtual address to size */
	as->size = (highest - lowest) + USER_STACK_SIZE;
/* STEP 4: allocate memory for user task, including user stack */
// xxx - not for demand-load paging
	as->user_mem = (char *)kmalloc(as->size);
	if(as->user_mem == NULL)
	{
		kprintf("create_task: out of memory)\n");
		destroy_aspace(as); /* this does kfree(as->user_mem); */
		return -ERR_NO_MEM;
	}
	uvirt_to_kvirt = (unsigned)as->user_mem - as->virt_adr;
/* STEP 5: load or zero file/memory sections, as appropriate */
// xxx - not for demand-load paging
	for(i = 0; i < as->num_sects; i++)
	{
		sect = as->sect + i;
		if(sect->flags & SF_LOAD)
			memcpy((char *)sect->adr + uvirt_to_kvirt,
				image + sect->offset, sect->size);
		else if(sect->flags & SF_ZERO)
			memset((char *)sect->adr + uvirt_to_kvirt,
				0, sect->size);
	}
/* STEP 6: create thread */
	t = create_thread(entry, 0);
	if(t == NULL)
	{
		kprintf("create_task: no task_t's left\n");
		destroy_aspace(as);
		return -ERR_NO_MEM;// xxx
	}
	t->as = as;
/* STEP 7: re-do the kernel stack for exit to ring 3 task */
	t->kstack = t->kstack_mem + KRNL_STACK_SIZE -
		sizeof(uregs_t) - sizeof(kregs_t);
/* set user-mode (ring 3) regs */
	uregs = (uregs_t *)(t->kstack + sizeof(kregs_t));
	uregs->ds = uregs->es = uregs->user_ss = uregs->fs =
		uregs->gs = USER_DS;
	uregs->cs = USER_CS;
	uregs->eip = entry;
	uregs->eflags = 0x200; /* enable interrupts */
	uregs->user_esp = as->virt_adr + as->size;
/* set kernel-mode (ring 0) regs such that initial task-switch
goes first to switch_to(), then to to_user(), then to ring 3 task */
	kregs = (kregs_t *)t->kstack;
	kregs->eip = (unsigned)to_user;
	kregs->eflags = 0; /* interrupts off until ring 3 */
// xxx - alloc page dir, copy, init
/* STEP 8: allocate virtual console
This is now done by the task startup code (USTART.S);
which calls open(/dev/con) */
#if 0
	t->vc = create_vc();
	if(t->vc == NULL)
	{
		kprintf("create_task: no VCs left\n");
		destroy_aspace(as);
		destroy_thread(t);
		return -1;
	}
#endif
	return 0;
}
/*****************************************************************************
*****************************************************************************/
void destroy_task(task_t *t)
{
	unsigned i;
	file_t *f;

/* close open files; if any */
	for(i = 0; i < NUM_FILES; i++)
	{
		f = t->files[i];
		if(f == NULL || f->ops == NULL ||
			f->ops->close == NULL)
				continue;
/* xxx - this causes a reboot...
		(void)f->ops->close(f); */
		kfree(f);
	}
/* destroy address space; if any */
	if(t->as != NULL)
		destroy_aspace(t->as);
	destroy_thread(t);
}
