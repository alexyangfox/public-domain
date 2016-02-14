/*----------------------------------------------------------------------------
Polled serial output for PC
Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: Oct 13, 2003
This code is public domain (no copyright).
You can do whatever you want with it.

EXPORTS:
void ser_putch(unsigned c);
----------------------------------------------------------------------------*/
#include <dos.h> /* inportb(), outportb() */
/*****************************************************************************
*****************************************************************************/
void ser_putch(unsigned c)
{
	static unsigned io_adr;
/**/

	if(io_adr == 0)
	{
		io_adr = 0x3F8;	/* 3F8=COM1, 2F8=COM2, 3E8=COM3, 2E8=COM4 */
		outportb(io_adr + 3, 0x80);
/* 115200 baud */
		outportb(io_adr + 0, 1);
		outportb(io_adr + 1, 0);
/* 8N1 */
		outportb(io_adr + 3, 0x03);
/* all interrupts disabled */
		outportb(io_adr + 1, 0);
/* turn off FIFO, if any */
		outportb(io_adr + 2, 0);
/* loopback off, interrupts (Out2) off, Out1/RTS/DTR off */
		outportb(io_adr + 4, 0);
	}
/* wait for transmitter ready */
	while((inportb(io_adr + 5) & 0x40) == 0)
		/* nothing */;
/* send char */
	outportb(io_adr + 0, c);
}
