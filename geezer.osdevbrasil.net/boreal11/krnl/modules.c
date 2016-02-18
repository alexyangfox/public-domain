/*----------------------------------------------------------------------------
KERNEL MODULES

EXPORTS:
int load_module(char *image);
----------------------------------------------------------------------------*/
#include <string.h> /* NULL */
#include "_krnl.h" /* task_t, console_t, kprintf() */

/* IMPORTS
from ELF.C */
int load_elf_reloc(char *image, unsigned *entry);

/* DJGPP COFF and Win32 PE COFF .o files are nearly indistinguishable,
but not identical (relocations work differently). We support only
one of these file types at a time: */
#if defined(__WIN32__)
/* from PECOFF.C */
int load_pe_reloc(char *image, unsigned *entry);
#else
/* from COFF.C */
int load_coff_reloc(char *image, unsigned *entry);
#endif

/* from THREADS.C */
task_t *create_thread(unsigned init_eip, int priority);
void destroy_thread(task_t *t);
/*****************************************************************************
*****************************************************************************/
int load_module(char *image)
{
	unsigned entry;
	task_t *t;
	int err;

/* try loading as COFF */
#if defined(__WIN32__)
	err = load_pe_reloc(image, &entry);
#else
	err = load_coff_reloc(image, &entry);
#endif
	if(err != 0)
	{
/* try loading as ELF */
		err = load_elf_reloc(image, &entry);
		if(err != 0)
		{
/*			kprintf("load_module: module is not a valid "
				"relocatable file\n"); */
			return +1;
		}
	}
	t = create_thread(entry, 0);
	if(t == NULL)
	{
		kprintf("load_module: no task_t's left\n");
		return -1;
	}
#if 0
	t->vc = create_vc();
	if(t->vc == NULL)
	{
		destroy_thread(t);
		kprintf("load_module: no virtual consoles left\n");
		return -1;
	}
#endif
	return 0;
}
