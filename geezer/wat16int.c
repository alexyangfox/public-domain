/*----------------------------------------------------------------------------
Software and hardware interrupt handler demo using 16-bit Watcom C
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: June 1, 2004
This code is public domain (no copyright).
You can do whatever you want with it.
----------------------------------------------------------------------------*/
#include <string.h> /* _fstrcpy() */
#include <stdio.h> /* printf() */
#include <conio.h> /* kbhit(), getch() */
#include <conio.h> /* inp(), outp() */
#include <dos.h> /* union INTPACK, MK_FP(), _dos_getvect(), _dos_setvect() */

#define	SYSCALL_INT	0x32

typedef struct
{
	unsigned int_num;
	char irq_was_enabled_at_8259;
	void __interrupt (*old_vector)();
} vector_t;

/* forward declarations */
static void hook_8259(vector_t *v);
static void unhook_8259(vector_t *v);
/*****************************************************************************
*****************************************************************************/
void hook_int(vector_t *v, unsigned int_num, void __interrupt (*handler)())
{
	v->int_num = int_num;
/* for hardware interrupts, enable IRQ at 8259 chips */
	hook_8259(v);
/* save address of current interrupt handler */
	v->old_vector = _dos_getvect(int_num);
/* install new handler */
	_dos_setvect(int_num, handler);
}
/*****************************************************************************
*****************************************************************************/
void unhook_int(vector_t *v)
{
	unhook_8259(v);
/* restore old interrupt handler */
	_dos_setvect(v->int_num, v->old_vector);
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
	v->irq_was_enabled_at_8259 = ((inp(io_adr) & mask) == 0);
/* enable IRQ at 8259 chip */
	outp(io_adr, inp(io_adr) & ~mask);
/* for IRQ 8-15, enable cascade bit in first 8259 chip */
	if(io_adr == 0xA1)
		outp(0x21, inp(0x21) & ~4);
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
		outp(io_adr, inp(io_adr) & ~mask);
	else
		outp(io_adr, inp(io_adr) | mask);
}
/*****************************************************************************
*****************************************************************************/
static void __interrupt syscall_int(union INTPACK r)
{
	static char farewell[] = "Goodbye";
/**/

/* passing data into software interrupt handler via registers */
	printf("Registers: '%c%c%c%c'    (should be 'BEER')\n",
		r.w.ax, r.w.bx, r.w.cx, r.w.dx);
/* passing pointer into software interrupt handler */
	printf("Greeting:  '%s'  (should be 'Hello!')\n", r.w.di);
/* passing pointer out of software interrupt handler */
	r.w.di = (int)farewell;
}
/*****************************************************************************
*****************************************************************************/
static void __interrupt timer_irq(void)
{
	(*(unsigned char far *)MK_FP(0xB800, 0))++;
	outp(0x20, 0x20);
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	union REGPACK regs;
	vector_t v;

/* install software interrupt handler and test */
	hook_int(&v, SYSCALL_INT, syscall_int);
	regs.w.ax = 'B';
	regs.w.bx = 'E';
	regs.w.cx = 'E';
	regs.w.dx = 'R';
	regs.w.di = (unsigned)"Hello!";
	intr(SYSCALL_INT, &regs);
	unhook_int(&v);
	printf("Farewell:  '%s' (should be 'Goodbye')\n", regs.w.di);
/* install hardware interrupt handler and test */
	printf("Timer interrupt test; press Esc to quit\n");
	hook_int(&v, 8, timer_irq);
	while(getch() != 27)
		/* nothing */;
	unhook_int(&v);
	return 0;
}
