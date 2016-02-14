/*----------------------------------------------------------------------------
Timer interrupt handler demo code.
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: March 2, 2007
This code is public domain (no copyright).
You can do whatever you want with it.
----------------------------------------------------------------------------*/
#include <conio.h> /* kbhit(), getch() */
#include <dos.h>

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
#define	inportb(P)		inp(P)
#define	outportb(P,V)		outp(P,V)
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
#define	INTERRUPT		/* nothing */
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
/*****************************************************************************
*****************************************************************************/
static void INTERRUPT timer_irq(void)
{
	_BLINK;
/* clear the interrupt at the 8259 interrupt controller chip */
	outportb(0x20, 0x20);
}
/*****************************************************************************
*****************************************************************************/
#define	VECT_NUM	8
#define HZ      	100

int main(void)
{
	static const unsigned short divisor = 1193182L / HZ;
/**/
	vector_t old_handler;

/* install interrupt handler */
	HOOK(old_handler, VECT_NUM, timer_irq);
/* program 8253 timer chip to run at frequency HZ */
	outportb(0x43, 0x36);	/* channel 0, LSB/MSB, mode 3, binary */
	outportb(0x40, divisor & 0xFF);	/* LSB */
	outportb(0x40, divisor >> 8);	/* MSB */
/* wait for key pressed */
	while(!kbhit())
		/* nothing */;
/* restore old timer interrupt handler */
	UNHOOK(old_handler, VECT_NUM);
/* program 8253 timer chip to run at default rate (18.2 Hz) */
	outportb(0x43, 0x36);
	outportb(0x40, 0xFF);
	outportb(0x40, 0xFF);
/* consume keypress */
	if(getch() == 0)
		(void)getch();
	return 0;
}
