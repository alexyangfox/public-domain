/*----------------------------------------------------------------------------
DEBUG ROUTINES

EXPORTS:
void dump(char *data, unsigned count);
void dump_regs(uregs_t *regs);
----------------------------------------------------------------------------*/
#include <string.h> /* NULL */
#include "_krnl.h" /* EBP_MAGIC, uregs_t, kprintf() */

/* IMPORTS
from THREADS.C */
extern task_t *g_curr_task;

/* from KSTART.S
unsigned *get_page_dir(void);
unsigned get_page_fault_adr(void); */

/* from PAGING.C
int address_mapped(unsigned virt); */
/*****************************************************************************
dumps 'count' bytes at 'data' as both hexadecimal and ASCII values
*****************************************************************************/
#define BPERL		16	/* byte/line for dump */

void dump(void *data_p, unsigned count)
{
	unsigned char *data = (unsigned char *)data_p;
	unsigned byte1, byte2;

	while(count != 0)
	{
		for(byte1 = 0; byte1 < BPERL; byte1++)
		{
			if(count == 0)
				break;
			kprintf("%02X ", data[byte1]);
			count--;
		}
		kprintf("\t");
		for(byte2 = 0; byte2 < byte1; byte2++)
		{
			if(data[byte2] < ' ')
				kprintf(".");
			else
				kprintf("%c", data[byte2]);
		}
		kprintf("\n");
		data += BPERL;
	}
}
/*****************************************************************************
*****************************************************************************/
void dump_regs(uregs_t *regs)
{
	kprintf("EDI=%08X    ESI=%08X    EBP=%08X    ESP=%08X    EBX=%08X\n",
		regs->edi, regs->esi, regs->ebp, regs->esp, regs->ebx);
	kprintf("EDX=%08X    ECX=%08X    EAX=%08X     DS=%08X     ES=%08X\n",
		regs->edx, regs->ecx, regs->eax, regs->ds, regs->es);
	kprintf(" FS=%08X     GS=%08X intnum=%08X  error=%08X    EIP=%08X\n",
		regs->fs, regs->gs, regs->which_int, regs->err_code,
		regs->eip);
	kprintf(" CS=%08X EFLAGS=%08X   uESP=%08X    uSS=%08X\n",
		regs->cs, regs->eflags, regs->user_esp, regs->user_ss);
}
