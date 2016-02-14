/*****************************************************************************
PC realtime clock interrupt demo

Chris Giese <geezer@execpc.com>, http://www.execpc.com/~geezer

This code is public domain (no copyright).
You can do whatever you want with it.


demo:
(DONE)	- read time and date from RTC, with polling delay for UIP
- same as above, with IRQ 8-driven delay for UIP
- polled alarm
- IRQ 8 alarm
(DONE)	- constant-frequency IRQs (2 Hz ... 1024 Hz)
*****************************************************************************/
/* [_dos_]getvect(), [_dos_]setvect(), pokeb(), pokeb() */
#include <dos.h> /* outportb(), inportb(), [_]enable(), [_]disable() */

/********************************* TURBO C **********************************/
#if defined(__TURBOC__)

#ifdef __cplusplus
typedef void interrupt (*vector_t)(...);
#else
typedef void interrupt (*vector_t)();
#endif

#define	INTERRUPT			interrupt

#define	save_vector(vec, num)		vec = getvect(num)
#define	install_handler(vec, num, fn)	setvect(num, (vector_t)fn)
#define	restore_vector(vec, num)	setvect(num, vec)

/********************************* DJGPP ************************************/
#elif defined(__DJGPP__)
#include <crt0.h> /* _CRT0_FLAG_NEARPTR, _crt0_startup_flags */
#include <go32.h> /* _my_cs() */
#include <dpmi.h> /* _go32_dpmi..., __dpmi... */

//#define NEARPTR 1

/* near pointers; not supported in Windows NT/2k/XP DOS box
Must call __djgpp_nearptr_enable() before using these functions */
#if defined(NEARPTR)
#include <sys/nearptr.h> /* __djgpp_conventional_base, __djgpp_nearptr_enable() */
#include <stdio.h> /* printf() */

#define	peekb(S,O)	*(unsigned char *)(16uL * (S) + (O) + \
				__djgpp_conventional_base)
#define	pokeb(S,O,V)	*(unsigned char *)(16uL * (S) + (O) + \
				__djgpp_conventional_base)=(V)
/* far pointers */
#else
#include <sys/farptr.h> /* _farp[eek|oke][b|w]() */
#include <go32.h> /* _dos_ds */

#define	peekb(S,O)	_farpeekb(_dos_ds, 16uL * (S) + (O))
#define	pokeb(S,O,V)	_farpokeb(_dos_ds, 16uL * (S) + (O), V)
#endif

typedef struct
{
	_go32_dpmi_seginfo old_v, new_v;
} vector_t;

#define	INTERRUPT			/* nothing */

#define	save_vector(vec, num)						\
	_go32_dpmi_get_protected_mode_interrupt_vector(num, &vec.old_v)

#define	install_handler(vec, num, fn)					\
	vec.new_v.pm_selector = _my_cs();				\
	vec.new_v.pm_offset = (unsigned long)fn;			\
	_go32_dpmi_allocate_iret_wrapper(&vec.new_v);			\
	_go32_dpmi_set_protected_mode_interrupt_vector(num, &vec.new_v);

#define	restore_vector(vec, num)					\
	_go32_dpmi_set_protected_mode_interrupt_vector(num, &vec.old_v);\
	_go32_dpmi_free_iret_wrapper(&vec.new_v);

/* lock all memory, to prevent it being swapped or paged out */
int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;

/******************************** WATCOM C **********************************/
#elif defined(__WATCOMC__)
#include <conio.h> /* inp(), outp() */

#if defined(__386__)
/* CauseWay DOS extender only */
#define	peekb(S,O)	*(unsigned char *)(16uL * (S) + (O))
#define	pokeb(S,O,V)	*(unsigned char *)(16uL * (S) + (O))=(V)
#else
#include <dos.h> /* MK_FP() */
#define	peekb(S,O)	*(unsigned char far *)MK_FP(S,O)
#define	pokeb(S,O,V)	*(unsigned char far *)MK_FP(S,O)=(V)
#endif

#define	inportb(P)	inp(P)
#define	outportb(P,V)	outp(P,V)

#define	disable()	_disable()
#define	enable()	_enable()

#ifdef __cplusplus
typedef void __interrupt (*vector_t)(...);
#else
typedef void __interrupt (*vector_t)();
#endif

#define	INTERRUPT			__interrupt

#define	save_vector(vec, num)		vec = _dos_getvect(num)
#define	install_handler(vec, num, fn)	_dos_setvect(num, (vector_t)fn)
#define	restore_vector(vec, num)	_dos_setvect(num, vec)

#else
#error Not Turbo C, not DJGPP, not Watcom C. Sorry.
#endif

const unsigned g_irq_to_vect[] =
{
	8, 9, 10, 11, 12, 13, 14, 15,
	112, 113, 114, 115, 116, 117, 118, 119
};
/*****************************************************************************
*****************************************************************************/
void enable_irq_at_8259(unsigned irq)
{
	if(irq >= 16)
		return;
	if(irq >= 8)
	{
		outportb(0x21, inportb(0x21) & ~0x04);
		irq -= 8;
		irq = 1 << irq;
		outportb(0xA1, inportb(0xA1) & ~irq);
		return;
	}
	irq = 1 << irq;
	outportb(0x21, inportb(0x21) & ~irq);
}
/*****************************************************************************
*****************************************************************************/
void disable_irq_at_8259(unsigned irq)
{
	if(irq >= 16)
		return;
	if(irq >= 8)
	{
/* no! leave cascade (2nd 8259 chip) enabled!
		outportb(0x21, inportb(0x21) | 0x04); */
		irq -= 8;
		irq = 1 << irq;
		outportb(0xA1, inportb(0xA1) | irq);
		return;
	}
	irq = 1 << irq;
	outportb(0x21, inportb(0x21) | irq);
}
/*****************************************************************************
*****************************************************************************/
#include <stdio.h> /* printf() */
#include <conio.h> /* kbhit(), getch() */

static volatile unsigned long g_ticks;
/*****************************************************************************
*****************************************************************************/
static void INTERRUPT rtc_irq(void)
{
	g_ticks++;
/* increment char in upper left corner of screen on every timer tick */
	pokeb(0xB800, 0, peekb(0xB800, 0) + 1);
/* acknowledge IRQ8 at the RTC by reading register C */
	outportb(0x70, 0x0C);
	(void)inportb(0x71);
/* acknowledge IRQ8 at the 8259 interrupt controller chips */
	outportb(0xA0, 0x20);
	outportb(0x20, 0x20);
}
/*****************************************************************************
*****************************************************************************/
static unsigned read_cmos_binary(unsigned reg)
{
/* b7 disables NMI; leave it as you found it */
	outportb(0x70, (inportb(0x70) & 0x80) | (reg & 0x7F));
	return inportb(0x71);
}
/*****************************************************************************
*****************************************************************************/
static unsigned read_cmos_bcd(unsigned reg)
{
	unsigned high_digit, low_digit;

/* b7 disables NMI; leave it as you found it */
	outportb(0x70, (inportb(0x70) & 0x80) | (reg & 0x7F));
	high_digit = low_digit = inportb(0x71);
/* convert from BCD to binary */
	high_digit >>= 4;
	high_digit &= 0x0F;
	low_digit &= 0x0F;
	return 10 * high_digit + low_digit;
}
/*****************************************************************************
*****************************************************************************/
static void get_time_and_display(void)
{
	unsigned sec, min, hr, mo, day, yr;

	sec = read_cmos_bcd(0);
	min = read_cmos_bcd(2);
	hr = read_cmos_bcd(4);
	day = read_cmos_bcd(7);
	mo = read_cmos_bcd(8);
	yr = read_cmos_bcd(9);
	printf("Time is %02u:%02u:%02u, date is %u/%u/%u\n",
		hr, min, sec, mo, day, yr);
}
/*****************************************************************************
*****************************************************************************/
#define	RTC_IRQ		8

static void periodic_int(void)
{
	vector_t old_vector;
	unsigned temp;

/* install interrupt handler */
	printf("Periodic interrupt test...\n");
	save_vector(old_vector, g_irq_to_vect[RTC_IRQ]);
	install_handler(old_vector, g_irq_to_vect[RTC_IRQ], rtc_irq);
	enable_irq_at_8259(RTC_IRQ);
/* set periodic interrupt rate */
	disable();
	outportb(0x70, 0x0A);
	temp = inportb(0x71) & ~0x0F;
	outportb(0x71, temp | 0x06);	/* 1024 Hz */
/*	outportb(0x71, temp | 0x0F);	/* 2 Hz */
/* enable IRQ8 at the RTC */
	outportb(0x70, 0x0B);
	temp = inportb(0x71);
	outportb(0x71, temp | 0x40);
/* "Make sure to read this register after programming the interrupt
control register (status B), or you won't get any interrupts."
    My PC doesn't need this. YMMV. */
/*	outportb(0x70, 0x0C);
	(void)inportb(0x71); */
	enable();
/* exit loop if key pressed (in case timer doesn't work :) */
	while(!kbhit())
	{
		if(g_ticks > 1024)
			break;
	}
	disable_irq_at_8259(RTC_IRQ);
/* disable IRQ8 at the RTC */
	disable();
	outportb(0x70, 0x0B);
	temp = inportb(0x71);
	outportb(0x71, temp & ~0x40);
/* restore old handler */
	restore_vector(old_vector, g_irq_to_vect[RTC_IRQ]);
	enable();
/* eat keystroke */
	if(kbhit())
	{
		if(getch() == 0)
			(void)getch();
	}
	printf("...done\n");
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
//printf("register A = 0x%X\n", read_cmos_binary(0x0A));
//return 0;
#if defined(__DJGPP__)&&defined(NEARPTR)
	if(!(_crt0_startup_flags & _CRT0_FLAG_NEARPTR))
	{
		if(!__djgpp_nearptr_enable())
		{
			printf("Could not enable nearptr access "
				"(Windows NT/2k/XP?)\n");
			return 1;
		}
	}
#endif
/* GET TIME FROM RTC, USING POLLING
wait for update-in-progress, if any, to finish. This polling
can take nearly 2 milliseconds if RTC chip uses 32.768 Khz crystal */
	while(read_cmos_binary(10) & 0x80)
		/* nothing */;
	get_time_and_display();
/* PERIODIC INTERRUPT TEST */
	periodic_int();
	return 0;
}
