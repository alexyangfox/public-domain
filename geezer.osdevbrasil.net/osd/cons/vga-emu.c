/*****************************************************************************
Determines if VGA board is set for monochrome or color emulation.
Uses 3 different algorithms.

This code is public domain (no copyright).
You can do whatever you want with it.
*****************************************************************************/
#include <stdlib.h> /* atoi() */
#include <stdio.h> /* printf() */
//#include "../port.c" /* inportb(), peekw() */

/********************************* TURBO C **********************************/
#if defined(__TURBOC__)
#include <dos.h> /* inportb(), peek() */

#define	peekw(S,O)	peek(S,O)

/********************************* DJGPP ************************************/
#elif defined(__DJGPP__)
#include <crt0.h> /* _CRT0_FLAG_LOCK_MEMORY */
#include <dos.h> /* inportb() */

//#define NEARPTR 1

/* near pointers; not supported in Windows NT/2k/XP DOS box
Must call __djgpp_nearptr_enable() before using these functions */
#if defined(NEARPTR)
#include <sys/nearptr.h> /* __djgpp_conventional_base, __djgpp_nearptr_enable() */
#include <stdio.h> /* printf() */
#include <crt0.h> /* _CRT0_FLAG_NEARPTR, _crt0_startup_flags */

#define	peekw(S,O)	*(unsigned short *)(16uL * (S) + (O) + \
				__djgpp_conventional_base)
/* far pointers */
#else
#include <sys/farptr.h> /* _farpeekw() */
#include <go32.h> /* _dos_ds */

#define	peekw(S,O)	_farpeekw(_dos_ds, 16uL * (S) + (O))
#endif

/******************************** WATCOM C **********************************/
#elif defined(__WATCOMC__)
#include <conio.h> /* inp() */

#if defined(__386__)
/* CauseWay DOS extender only */
#define	peekw(S,O)	*(unsigned short *)(16uL * (S) + (O))
#else
#include <dos.h> /* MK_FP() */

#define	peekw(S,O)	*(unsigned short far *)MK_FP(S,O)
#endif

#define	inportb(P)	inp(P)

#else
#error Not Turbo C, not DJGPP, not Watcom C. Sorry.
#endif


static unsigned short g_crtc_base_adr;
/*****************************************************************************
	Pentium	 486	Bochs
method	color 	color 	(color)	mono
------	-------	-----	-------	-------
1	pass	pass	pass	UNTESTED
2	pass	pass	pass	UNTESTED
3	pass	pass	pass	UNTESTED
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	int method;

#if defined(__DJGPP__)&&defined(NEARPTR)
	if(!(_crt0_startup_flags & _CRT0_FLAG_NEARPTR))
	{
		if(!__djgpp_nearptr_enable())
		{
			printf("Could not enable nearptr access "
				"(Windows NT/2k/XP?)\nUn-define NEARPTR "
				"in source code and re-compile\n");
			return 1;
		}
	}
#endif
	if(arg_c < 2)
	{
		printf("attempt to detect monochrome/color VGA emulation "
			"using one of three methods\n"
			"specify 1, 2, or 3 on the command line\n");
		return 1;
	}
	method = atoi(arg_v[1]);
	switch(method)
	{
	case 1:
/* this method cobbled from info in Finn Thoegersen's VGADOC4 */
#define	VGA_MISC_READ	0x3CC

		if((inportb(VGA_MISC_READ) & 0x01) == 0)
			g_crtc_base_adr = 0x3B4;	/* mono */
		else
			g_crtc_base_adr = 0x3D4;	/* color */
		break;
	case 2:
/* I forgot where this came from:
"The word at low memory address 0040:0063 (or 0000:0463) contains the
I/O address of the CRTC which can be used to determine whether the video
system is colour or monochrome. A value of 3B4 hex indicates monochrome."
(I presume 3D4 hex means color; my Pentium system has that value at 0463.) */
		g_crtc_base_adr = peekw(0x40, 0x63);
		break;
	case 3:
/* Dark Fiber's method, from the OS FAQ
http://www.osdev.org/osfaq2/

from MEMORY.LST of Ralf Brown's Interrupt List
0040:0010 is Installed Hardware word, b5:b4 indicate video hardware:
	00 EGA,VGA,PGA, or other with on-board video BIOS
	01 40x25 CGA color
	10 80x25 CGA color
	11 80x25 mono text

whoa, this won't work with DJGPP -- OK, I will make a slight change here
		if((*(unsigned short *)0x410 & 30) == 0x30) */
		if((peekw(0x40, 0x10) & 30) == 0x30)
			g_crtc_base_adr = 0x3B4;	/* mono */
		else
			g_crtc_base_adr = 0x3D4;	/* color */
		break;
	default:
		printf("didn't find 1, 2, or 3 on the command line, sorry\n");
		return 1;
	}
/* what've we got? */
	if(g_crtc_base_adr < 0x3C0)
		printf("MONOCHROME emulation detected\n");
	else
		printf("color emulation detected\n");
	return 0;
}
