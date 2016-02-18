/*----------------------------------------------------------------------------
MAIN KERNEL

EXPORTS:
void kprintf(const char *fmt, ...);
void panic(const char *fmt, ...);
void fault(uregs_t *regs);
void kmain(void);
----------------------------------------------------------------------------*/
#include <_syscall.h> /* SYSCALL_INT */
#include <_printf.h> /* do_printf() */
#include <mltiboot.h>
#include <stdarg.h> /* va_list, va_start(), va_end() */
#include <string.h> /* NULL */
#include <system.h> /* outportb(), disable(), enable() */
#include "_krnl.h"

/* IMPORTS
from VIDEO.C */
void blink(void);
void putch(unsigned c);
void init_video(void);

/* from KBD.C */
void keyboard_irq(void);

/* from DEBUG.C */
void dump(void *data_p, unsigned count);
void dump_regs(uregs_t *regs);

/* from MM.C */
void init_mm(void);

/* from TIME.C */
void rtc_irq(void);

/* from SYSCALLS.C */
void syscall(uregs_t *regs);

/* from THREADS.C */
extern task_t *g_curr_task;

void timer_irq(void);
void init_threads(void);
void _exit(int exit_code);

/* from KSTART.S */
extern mboot_info_t *g_mboot_info;

void halt(void);

/* from SERIAL.C */
void serial_irq3(void);
void serial_irq4(void);
int init_serial(void);

/* from PAGING.C
int init_paging(void);
void discard_mem(void);
int page_fault(uregs_t *regs); */

/* from MODULES.C */
int load_module(char *image);

/* from TASKS.C */
int create_task(char *image);
/*****************************************************************************
*****************************************************************************/
static int kprintf_help(unsigned c, void **ptr_UNUSED)
{
	putch(c);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
void kprintf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	(void)do_printf(fmt, args, kprintf_help, NULL);
	va_end(args);
}
/*****************************************************************************
*****************************************************************************/
void panic(const char *fmt, ...)
{
	va_list args;

	disable(); /* interrupts off */
	kprintf("panic: ");
	va_start(args, fmt);
	do_printf(fmt, args, kprintf_help, NULL);
	halt();
}
/*****************************************************************************
*****************************************************************************/
void fault(uregs_t *regs)
{
	static const char *msg[] =
	{
/* 0-7 */
		"divide error", "debug exception", "NMI", "INT3",
		"INTO", "BOUND exception", "invalid opcode", "no coprocessor",
/* 8-0x0F */
		"double fault", "coprocessor segment overrun",
			"bad TSS", "segment not present",
		"stack fault", "GPF", "page fault", "??",
/* 0x10-0x17 */
		"coprocessor error", "alignment check", "??", "??",
		"??", "??", "??", "??",
/* 0x18-0x1F */
		"??", "??", "??", "??",
		"??", "??", "??", "??",
/* 0x20-0x27 */
		"syscall", "??", "??", "??",
		"??", "??", "??", "??",
/* 0x28-0x2F */
		"IRQ0", "IRQ1", "IRQ2", "IRQ3",
		"IRQ4", "IRQ5", "IRQ6", "IRQ7",
/* 0x30-0x37 */
		"IRQ8", "IRQ9", "IRQ10", "IRQ11",
		"IRQ12", "IRQ13", "IRQ14", "IRQ15",
	};
/**/
	const char *s;

	switch(regs->which_int)
	{
/* page fault
	case 0x0E:
		if(page_fault(regs))
			goto ERR;
		break; */
/* timer (IRQ 0) */
	case 0x28:
		blink();
/* Reset hardware interrupt at (master) 8259 interrupt controller chip.
IRQs remain masked at the CPU. */
		outportb(0x20, 0x20);
		timer_irq();
		break;
/* keyboard (IRQ 1) */
	case 0x29:
		keyboard_irq();
		outportb(0x20, 0x20);
		break;
/* serial (IRQ 3) */
	case 0x2B:
		serial_irq3();
		outportb(0x20, 0x20);
		break;
/* serial (IRQ 4) */
	case 0x2C:
		serial_irq4();
		outportb(0x20, 0x20);
		break;
/* realtime clock (IRQ 8) */
	case 0x30:
		rtc_irq();
		outportb(0xA0, 0x20);
		outportb(0x20, 0x20);
		break;
/* syscall (INT 30h) */
	case SYSCALL_INT:
		syscall(regs);
		break;
/* anything else */
	default:
/*ERR:*/
		if(regs->which_int <= sizeof(msg) / sizeof(char *))
			s = msg[regs->which_int];
		else
			s = "??";
		dump_regs(regs);
/* uh-oh */
		if((regs->cs & 3) == 0)
		{
			panic("Exception #%u (%s) in kernel mode",
				regs->which_int, s);
		}
/* else user mode fault: kill the task */
		kprintf("Exception #%u (%s) in user mode; task 0x%p "
			"killed\n", regs->which_int, s, g_curr_task);
		_exit(-1);
		break;
	}
}
/*****************************************************************************
*****************************************************************************/
DISCARDABLE_CODE(static void init_8259s(void))
{
	outportb(0x20, 0x11); /* ICW1: edge-triggered IRQs... */
	outportb(0xA0, 0x11); /* ...cascade mode, need ICW4 */

	outportb(0x21, 0x28); /* ICW2: route IRQs 0...7 to INTs 28h...2Fh */
	outportb(0xA1, 0x30); /* ...IRQs 8...15 to INTs 30h...37h */

	outportb(0x21, 0x04); /* ICW3 master: slave 8259 is on IRQ2 */
	outportb(0xA1, 0x02); /* ICW3 slave: I am slave on master's IRQ2 */

	outportb(0x21, 0x01); /* ICW4 */
	outportb(0xA1, 0x01);
/* enable IRQ0 (timer), IRQ1 (keyboard), IRQ2 (cascade),
IRQs 3 and 4 (serial)... */
	outportb(0x21, ~0x1F);
/* ...and IRQ8 (realtime clock) */
	outportb(0xA1, ~0x01);
}
/*****************************************************************************
*****************************************************************************/
DISCARDABLE_CODE(static void init_8253(void))
{
/* I can remember the NTSC TV color burst frequency,
but not the PC peripheral clock. Fortunately, they are related: */
	static const unsigned short period = (3579545L / 3) / HZ;
/**/

/* reprogram the 8253 timer chip to run at 'HZ', instead of 18 Hz */
	outportb(0x43, 0x36);	/* channel 0, LSB/MSB, mode 3, binary */
	outportb(0x40, period & 0xFF);	/* LSB */
	outportb(0x40, period >> 8);	/* MSB */
}
/*****************************************************************************
use kmain() instead of main() to avoid:
	warning: return type of `main' is not `int'
*****************************************************************************/
void kmain(void)
{
	unsigned i, num_tasks, num_mods;
	mboot_mod_t *mod;
	char *image;

/* get video working */
	init_video();
/* display banner */
	kprintf("\x1B[31m""B"	"\x1B[32m""o"	"\x1B[33m""r"
		"\x1B[34m""e"	"\x1B[35m""a"	"\x1B[36m""l"
		"\x1B[31;1m""i"	"\x1B[37;0m""s"
		" OS release 11, by Chris Giese "
		"<geezer@execpc.com>\n");
#if defined(__GNUC__)
#if __GNUC__>=3
	kprintf("Kernel built with GCC version %u.%u.%u\n",
		__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#else
	kprintf("Kernel built with GCC version %u.%u\n",
		__GNUC__, __GNUC_MINOR__);
#endif
#endif
/* init this, init that */
	init_mm();
	init_8259s();
	init_8253();
	init_serial();
	init_threads();
/* modules can be executable files (tasks) or relocatable (kernel modules) */
	if(!(g_mboot_info->flags & MBF_MODS))
		kprintf("bootloader did not set Multiboot module fields\n");
	else
	{
		kprintf("Processing %u boot module(s)...\n",
			g_mboot_info->num_mods);
		num_tasks = num_mods = 0;
		for(i = 0; i < g_mboot_info->num_mods; i++)
		{
			mod = (mboot_mod_t *)(g_mboot_info->mods_adr -
				g_kvirt_to_phys) + i;
/*			image = (char *)(mod->cmd_line - g_kvirt_to_phys);
kprintf("module %u: name=%s, start_adr=0x%lX, end_adr=0x%lX\n",
 i, image, mod->start_adr, mod->end_adr); */
			image = (char *)(mod->start_adr - g_kvirt_to_phys);
			if(create_task(image) == 0)
				num_tasks++;
			else
			{
				if(load_module(image) == 0)
					num_mods++;
				else
					kprintf("\tinvalid module %u\n", i);
			}
		}
		kprintf("\tstarted %u task(s); loaded %u kernel module(s)\n",
			num_tasks, num_mods);
	}
/*	err = init_paging();
	if(err)
	{
		kprintf("init_paging returned %d\n", err);
		halt();
	} */
/* done with init, can now discard init-only code/data.
This feature requires paging.
	discard_mem(); */
	kprintf("Press Alt+F1, Alt+F2, etc. to select virtual console, "
		"Ctrl+Alt+Del to reboot\n");
/* enabling timer interrupt starts the scheduler */
	enable();
	while(1)
		halt(); /* halt until interrupt, that is... */
}
