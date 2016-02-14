/*----------------------------------------------------------------------------
Keyboard interrupt handler demo code.
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: March 2, 2007
This code is public domain (no copyright).
You can do whatever you want with it.
----------------------------------------------------------------------------*/
#include <stdio.h> /* NULL, stdout, setbuf(), printf() */
#include <dos.h>

#if defined(__TURBOC__)
#define	INTERRUPT		interrupt
#define	HOOK(vect,num,handler)	\
	(vect) = getvect(num);	\
	setvect(num, handler)
#define	UNHOOK(vect,num)	setvect(num, vect)
typedef void interrupt (*vector_t)(void);

#elif defined(__WATCOMC__)
#include <conio.h> /* inp(), outp() */
#define	inportb(P)		inp(P)
#define	outportb(P,V)		outp(P,V)
#define	INTERRUPT		__interrupt
#define	HOOK(vect,num,handler)		\
	(vect) = _dos_getvect(num);	\
	_dos_setvect(num, handler)
#define	UNHOOK(vect,num)	_dos_setvect(num, vect)
typedef void __interrupt (*vector_t)(void);

#elif defined(__DJGPP__)
#include <go32.h> /* _my_cs() */
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
typedef struct {
	_go32_dpmi_seginfo old_v, new_v;
} vector_t;

/* lock all memory, to prevent it being swapped or paged out */
int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;

#else
#error Sorry, unsupported compiler
#endif

static int g_scancode = -1;
/*****************************************************************************
In an OS, this routine should put the scancode into a queue and signal
a foreground task (using a wake_up() function, e.g.) to remove the
scancode from the queue and process it.

The scancode produced when a key is pressed (the 'make' code) is different
from the scancode produced when a key is released (the 'break' code).
The scancode values also depend on the 'scancode set'. Only scancode set 2
is widely supported and free of bugs. The 8042 keyboard controller chip
is usually programmed to convert set 2 scancodes to set 1.
*****************************************************************************/
static void INTERRUPT kbd_irq(void)
{
/* reading I/O port 60h gets scancode
ands clears the IRQ 1 interrupt at 8042 keyboard controller chip */
	g_scancode = inportb(0x60);
/* now clear the interrupt at the 8259 interrupt controller chip */
	outportb(0x20, 0x20);
}
/*****************************************************************************
*****************************************************************************/
#define	VECT_NUM	9

int main(void)
{
	vector_t old_handler;
	int code;

/* turn off stdout line buffering */
	setbuf(stdout, NULL);
/* install interrupt handler */
	HOOK(old_handler, VECT_NUM, kbd_irq);
/* get make and break codes and display them until Esc is pressed */
	while(1)
	{
		code = g_scancode;
		if(code == -1)
			continue;
		g_scancode = -1;
/* make code for 'Esc' key in scancode sets 1, 2, and 3: */
		if(code == 0x01 || code == 0x76 || code == 0x08)
			break;
		printf("%02X  ", code);
	}
/* restore old keyboard interrupt handler */
	UNHOOK(old_handler, VECT_NUM);
	return 0;
}
