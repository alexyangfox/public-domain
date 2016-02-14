/*****************************************************************************
Scans PC upper memory area for BIOS ROM extensions
	(ROMs other than the motherboard ROM).
Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: ?
This code is public domain (no copyright).
You can do whatever you want with it.

xxx - ROM header offset + 0x1A -> PnP Expansion Header,
for BIOS Boot Spec
*****************************************************************************/
#include <stdio.h>

/********************************* TURBO C **********************************/
#if defined(__TURBOC__)
#include <dos.h> /* peekb() */

/********************************* DJGPP ************************************/
#elif defined(__DJGPP__)

//#define NEARPTR 1

/* near pointers; not supported in Windows NT/2k/XP DOS box
Must call __djgpp_nearptr_enable() before using these functions */
#if defined(NEARPTR)
#include <sys/nearptr.h> /* __djgpp_conventional_base, __djgpp_nearptr_enable() */
#include <stdio.h> /* printf() */
#include <crt0.h> /* _CRT0_FLAG_NEARPTR, _crt0_startup_flags */

#define	peekb(S,O)	*(unsigned char *)(16uL * (S) + (O) + \
				__djgpp_conventional_base)
/* far pointers */
#else
#include <sys/farptr.h> /* _farp[eek|oke][b|w]() */
#include <go32.h> /* _dos_ds */

#define	peekb(S,O)	_farpeekb(_dos_ds, 16uL * (S) + (O))
#endif

/******************************** WATCOM C **********************************/
#elif defined(__WATCOMC__)

#if defined(__386__)
/* CauseWay DOS extender only */
#define	peekb(S,O)	*(unsigned char *)(16uL * (S) + (O))
#else
#include <dos.h> /* MK_FP() */
#define	peekb(S,O)	*(unsigned char far *)MK_FP(S,O)
#endif

#else
#error Not Turbo C, not DJGPP, not Watcom C. Sorry.
#endif
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	unsigned long size, temp;
	unsigned char checksum;
	unsigned short seg;

/* start at C000:0000, check every 2K for BIOS extension signature bytes */
	for(seg = 0xC000; seg != 0; seg += 2048)
	{
		if(peekb(seg, 0) != 0x55)
			continue;
/* Turbo C peekb() returns type 'char'; causes sign-extension problems */
		if((unsigned char)peekb(seg, 1) != 0xAA)
			continue;
/* get size of BIOS extension */
		size = peekb(seg, 2);
		size <<= 9;
/* do checksum
xxx - this doesn't work if size >= 64K */
		checksum = 0;
		for(temp = 0; temp < size; temp++)
			checksum += peekb(seg, temp);
		if(checksum != 0)
			continue;
		printf("BIOS extension ROM of size %lu bytes found at "
			"%04X:0000\n", size, seg);
	}
	return 0;
}
