/*****************************************************************************
ATA (IDE) hard drive detection code
Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: May 13, 2003
This code is public domain (no copyright).
You can do whatever you want with it.

This code based on Phoenix document
"Autotyping ATA Disk Drives", version 0.9 (July 26, 1995)

This code builds with Turbo C, DJGPP, or 16- or 32-bit Watcom C
It will NOT work inside a Windows DOS box
Not tested with ATAPI drives (e.g. IDE CD-ROMs)
*****************************************************************************/
#include <stdio.h> /* printf() */
#include <dos.h> /* inportb(), delay() */

#if defined(__WATCOMC__)
#include <conio.h>
#define	inportb(P)	inp(P)
#define	outportb(P,V)	outp(P,V)
#endif
/*****************************************************************************
*****************************************************************************/
static void msleep(unsigned milliseconds)
{
	delay(milliseconds);
}
/*****************************************************************************
*****************************************************************************/
static void nsleep(unsigned nanoseconds)
{
}
/*****************************************************************************
*****************************************************************************/
static void detect_drives(unsigned io_adr)
{
	static /*const*/ char *msg[] =
	{
		"Unknown error", "OK", "Formatter device error",
		"Sector buffer error", "ECC circuitry error",
		"Controller microprocessor errror"
	};
/**/
	unsigned timeout, byte0, byte1;

printf("Interface 0x%03X:\n", io_adr);
/* check for 0xFF; the value of a floating bus
"debounce" this value for a period of 20 ms */
	for(timeout = 20; timeout != 0; timeout--)
	{
		if(inportb(io_adr + 7) != 0xFF)
			break;
		msleep(1);
	}
/* probe cylinder registers to make absolutely sure there's no drive */
	if(timeout == 0)
	{
printf("floating bus; probing cylinder registers...\n");
		outportb(io_adr + 4, 0x55);
		outportb(io_adr + 5, 0xAA);
		byte0 = inportb(io_adr + 4);
/* if drive is present, next inportb() will read 0xAA. May read 0xAA
even if no drive (because of bus capacitance), so force 0x55 onto bus */
		outportb(0x80, 0x55);
		byte1 = inportb(io_adr + 5);
		if(byte0 != 0x55 || byte1 != 0xAA)
		{
printf("floating bus; no drive(s) present\n");
			return;
		}
	}
/* wait (xxx - how long?) until BSY=0 */
	for(timeout = 5000; timeout != 0; timeout--)
	{
		if((inportb(io_adr + 7) & 0x80) == 0)
			break;
		msleep(1);
	}
	if(timeout == 0)
	{
printf("BSY != 0; no drive(s) present\n");
		return;
	}
/* Execute Drive Diagnostics */
	outportb(io_adr + 7, 0x90);
	nsleep(400);
/* ...can take up to 5 seconds
I think you can use interrupts here, if you want */
	for(timeout = 5000; timeout != 0; timeout--)
	{
		if((inportb(io_adr + 7) & 0x80) == 0)
			break;
		msleep(1);
	}
	if(timeout == 0)
	{
printf("Execute Drive Diagnostics did not complete; no drive(s) present\n");
		return;
	}
	byte0 = inportb(io_adr + 1);	/* b0-b6=master diagnostic code */
	byte1 = byte0 & 0x7F;		/* b7=1 if slave error */
	if(byte1 >= sizeof(msg) / sizeof(msg[0]))
		byte1 = 0;
printf("  master drive: %s\n", msg[byte1]);
printf("  slave  drive: ");
	if(byte0 < 0x80)
printf("NONE\n");
	else
	{
		outportb(io_adr + 6, 0xB0); /* select slave */
		nsleep(400);
		byte0 = inportb(io_adr + 1); /* read slave diagnostic */
		byte1 = byte0 & 0x7F;
		if(byte1 >= sizeof(msg) / sizeof(msg[0]))
			byte1 = 0;
printf("%s\n", msg[byte1]);
	}
/* at this point, according to the Phoenix docs, you should
use the ATA "Identify Device" command to get the following info:
- Maximum sector count for Read Multiple and Write Multiple commands
- LBA capability	- CHS geometry
- Fastest PIO mode	- Fastest DMA mode

I would soft-reset the drive(s), then test the registers to see
if we've got ATA or ATAPI drives, and use the appropriate version
of "Identify Device" */
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	detect_drives(0x1F0);
	detect_drives(0x170);
	return 0;
}
