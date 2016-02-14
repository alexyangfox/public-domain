/*----------------------------------------------------------------------------
PC realtime clock interrupt demo
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: March 2, 2007
This code is public domain (no copyright).
You can do whatever you want with it.
----------------------------------------------------------------------------*/
#include <stdio.h> /* printf() */
#include <conio.h> /* kbhit(), getch() */
/* getvect(), setvect(), peekb(), pokeb() */
#include <dos.h> /* outportb(), inportb(), enable(), disable() */

#if defined(__TURBOC__)
#define	INTERRUPT		interrupt
#define	HOOK(vect,num,handler)	\
	(vect) = getvect(num);	\
	setvect(num, handler)
#define	UNHOOK(vect,num)	setvect(num, vect)
/* "BLINK" is defined in Turbo C CONIO.H, it seems */
#define	_BLINK			pokeb(0xB800, 0, peekb(0xB800, 0) + 1)
typedef void interrupt (*vector_t)(void);

#elif defined(__WATCOMC__)
#include <conio.h> /* inp(), outp() */
#define	inportb(P)	inp(P)
#define	outportb(P,V)	outp(P,V)
#define	disable		_disable
#define	enable		_enable
#define	INTERRUPT		__interrupt
#define	HOOK(vect,num,handler)	\
	(vect) = _dos_getvect(num);\
	_dos_setvect(num, handler)
#define	UNHOOK(vect,num)	_dos_setvect(num, vect)
#if defined(__386__)
/* this works with CauseWay DOS extender only: */
#define	_BLINK			*(unsigned char *)0xB8000L += 1
#else
#define	_BLINK			*(unsigned char far *)MK_FP(0xB800, 0) += 1
#endif
typedef void __interrupt (*vector_t)(void);

#elif defined(__DJGPP__)
#include <sys/farptr.h> /* _farpeekb(), _farpokeb() */
#include <go32.h> /* _my_cs(), _dos_ds */
#include <dpmi.h> /* _go32_dpmi..., __dpmi... */
#include <crt0.h> /* _CRT0_FLAG_LOCK_MEMORY */
#define	INTERRUPT			/* nothing */
#define	HOOK(vect,num,handler)						\
	_go32_dpmi_get_protected_mode_interrupt_vector(num, &vect.old_v);\
	vect.new_v.pm_selector = _my_cs();				\
	vect.new_v.pm_offset = (unsigned long)handler;			\
	_go32_dpmi_allocate_iret_wrapper(&vect.new_v);			\
	_go32_dpmi_set_protected_mode_interrupt_vector(num, &vect.new_v);
#define	UNHOOK(vect,num)						\
	_go32_dpmi_set_protected_mode_interrupt_vector(num, &vect.old_v);\
	_go32_dpmi_free_iret_wrapper(&vect.new_v);
#define	_BLINK								\
	_farpokeb(_dos_ds, 0xB8000, _farpeekb(_dos_ds, 0xB8000) + 1)
typedef struct {
	_go32_dpmi_seginfo old_v, new_v;
} vector_t;

/* lock all memory, to prevent it being swapped or paged out */
int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;

#else
#error Sorry, unsupported compiler
#endif

static volatile unsigned long g_ticks;
/*****************************************************************************
*****************************************************************************/
static void INTERRUPT rtc_irq(void)
{
	unsigned i;

	g_ticks++;
/* increment char in upper left corner of screen on every timer tick */
	_BLINK;
/* save contents of I/O port 0x70 */
	i = inportb(0x70);
/* acknowledge IRQ 8 at the RTC by reading register C */
	outportb(0x70, 0x0C);
	(void)inportb(0x71);
	outportb(0x70, i);
/* acknowledge IRQ 8 at the PICs */
	outportb(0xA0, 0x20);
	outportb(0x20, 0x20);
}
/*****************************************************************************
*****************************************************************************/
/* IRQ 8 vector (with default BIOS settings of the PICs) */
#define	VECT_NUM	112

int main(void)
{
	vector_t old_handler;
	unsigned i;

	printf("Periodic interrupt test...\n");
/* install interrupt handler */
	HOOK(old_handler,VECT_NUM, rtc_irq);
/* enable IRQ 8 at the PIC */
	outportb(0xA1, inportb(0xA1) & ~0x01);
/* set periodic interrupt rate */
	disable();
	outportb(0x70, 0x0A);
	i = inportb(0x71) & ~0x0F;
#if 1
	outportb(0x71, i | 0x06);	/* 1024 Hz */
#else
	outportb(0x71, i | 0x0F);	/* 2 Hz */
#endif
/* enable IRQ 8 at the RTC */
	outportb(0x70, 0x0B);
	outportb(0x71, inportb(0x71) | 0x40);
/* "Make sure to read this register after programming the interrupt
control register (status B), or you won't get any interrupts."
My PC doesn't need this. */
//	outportb(0x70, 0x0C);
//	(void)inportb(0x71);
	enable();
/* exit loop if key pressed (in case timer doesn't work :) */
	while(!kbhit())
	{
		if(g_ticks > 1024)
			break;
	}
	disable();
/* disable IRQ 8 at the PIC */
	outportb(0xA1, inportb(0xA1) | 0x01);
/* disable IRQ 8 at the RTC */
	outportb(0x70, 0x0B);
	outportb(0x71, inportb(0x71) & ~0x40);
/* restore old handler */
	UNHOOK(old_handler, VECT_NUM);
	enable();
/* eat keystroke */
	if(kbhit())
	{
		if(getch() == 0)
			(void)getch();
	}
	printf("...done\n");
	return 0;
}
