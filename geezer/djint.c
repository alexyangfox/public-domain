/*----------------------------------------------------------------------------
Software and hardware interrupt handler demo using DJGPP
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: June 1, 2004
This code is public domain (no copyright).
You can do whatever you want with it.
----------------------------------------------------------------------------*/
#include <sys/farptr.h> /* _farpokeb(), _farpeekb() */
#include <stdio.h> /* printf() */
#include <conio.h> /* getch() */
#include <go32.h> /* _dos_ds, _my_cs() */
#include <dpmi.h> /* _go32_dpmi... */
#include <crt0.h> /* _CRT0_FLAG_LOCK_MEMORY */
#include <dos.h> /* inportb(), outportb() */

#define	SYSCALL_INT	0x32

typedef struct
{
	unsigned int_num;
	char irq_was_enabled_at_8259;
	_go32_dpmi_seginfo old_vector, new_vector;
} vector_t;

/* forward declarations */
static void hook_8259(vector_t *v);
static void unhook_8259(vector_t *v);
/*****************************************************************************
*****************************************************************************/
void hook_int(vector_t *v, unsigned int_num, void (*handler)())
{
	v->int_num = int_num;
/* for hardware interrupts, enable IRQ at 8259 chips */
	hook_8259(v);
/* save address of current interrupt handler */
	_go32_dpmi_get_protected_mode_interrupt_vector(int_num,
		&v->old_vector);
/* create IRET wrapper for handler */
	v->new_vector.pm_selector = _my_cs();
	v->new_vector.pm_offset = (unsigned long)handler;
	_go32_dpmi_allocate_iret_wrapper(&v->new_vector);
/* install new handler */
	_go32_dpmi_set_protected_mode_interrupt_vector(int_num,
		&v->new_vector);
}
/*****************************************************************************
*****************************************************************************/
void unhook_int(vector_t *v)
{
	unhook_8259(v);
/* restore old interrupt handler */
	_go32_dpmi_set_protected_mode_interrupt_vector(v->int_num,
		&v->old_vector);
/* de-allocate IRET wrapper */
	_go32_dpmi_free_iret_wrapper(&v->new_vector);
}
/*****************************************************************************
save 8259 interrupt enable state for this IRQ,
then enable IRQ at 8259 chips
*****************************************************************************/
static void hook_8259(vector_t *v)
{
	unsigned irq_num, mask = 1, io_adr;

/* INT 8-15 = IRQ 0-7 */
	if(v->int_num >= 8 && v->int_num <= 15)
	{
/* convert int_num to irq_num... */
		irq_num = v->int_num - 8;
/* ...then to enable/disable mask value and 8259 chip I/O address */
		mask <<= irq_num;
		io_adr = 0x21; /* first 8259 chip */
	}
/* INT 0x70-0x77 = IRQ 8-15 */
	else if(v->int_num >= 0x70 && v->int_num < 0x77)
	{
		irq_num = v->int_num - 0x70;
		mask <<= irq_num;
		io_adr = 0xA1; /* second 8259 chip */
	}
	else
		return;
/* save current IRQ enable state
(the interrupt enable bit is ACTIVE LOW) */
	v->irq_was_enabled_at_8259 = ((inportb(io_adr) & mask) == 0);
/* enable IRQ at 8259 chip */
	outportb(io_adr, inportb(io_adr) & ~mask);
/* for IRQ 8-15, enable cascade bit in first 8259 chip */
	if(io_adr == 0xA1)
		outportb(0x21, inportb(0x21) & ~4);
}
/*****************************************************************************
restore 8259 interrupt enable state for this IRQ
*****************************************************************************/
static void unhook_8259(vector_t *v)
{
	unsigned irq_num, mask = 1, io_adr;

	if(v->int_num >= 8 && v->int_num <= 15)
	{
		irq_num = v->int_num - 8;
		mask <<= irq_num;
		io_adr = 0x21;
	}
	else if(v->int_num >= 0x70 && v->int_num < 0x77)
	{
		irq_num = v->int_num - 0x70;
		mask <<= irq_num;
		io_adr = 0xA1;
	}
	else
		return;
/* restore original state of 8259 chip */
	if(v->irq_was_enabled_at_8259)
		outportb(io_adr, inportb(io_adr) & ~mask);
	else
		outportb(io_adr, inportb(io_adr) | mask);
}
/*****************************************************************************
user-defined software interrupt handler
*****************************************************************************/
static void syscall_int(__dpmi_regs *regs)
{
	static char farewell[] = "Goodbye";
/**/

/* passing data into software interrupt handler via registers */
	printf("Registers: '%c%c%c%c'    (should be 'BEER')\n",
		regs->x.ax, regs->x.bx, regs->x.cx, regs->x.dx);
/* passing pointer into software interrupt handler */
	printf("Greeting:  '%s'  (should be 'Hello!')\n",
		(char *)regs->d.edi);
/* passing pointer out of software interrupt handler */
	regs->d.edi = (int)farewell;
}
/*****************************************************************************
hardware interrupt handler
*****************************************************************************/
static void timer_irq(void)
{
	_farpokeb(_dos_ds, 0xB8000, _farpeekb(_dos_ds, 0xB8000) + 1);
	outportb(0x20, 0x20);
}
/*****************************************************************************
*****************************************************************************/
#define	VECT	0x32

int main(void)
{
	vector_t v;
	char *s;

/* install software interrupt handler and test */
	hook_int(&v, SYSCALL_INT, syscall_int);
	__asm__ __volatile__(
		"int %1\n"
		: "=D"(s)
		: "i"(SYSCALL_INT), "a"('B'), "b"('E'), "c"('E'), "d"('R'),
			"D"((unsigned)"Hello!"));
	unhook_int(&v);
	printf("Farewell:  '%s' (should be 'Goodbye')\n", s);
/* install hardware interrupt handler and test */
	printf("Timer interrupt test; press Esc to quit\n");
	hook_int(&v, 8, timer_irq);
	while(getch() != 27)
		/* nothing */;
	unhook_int(&v);
	return 0;
}
