#include "kernel.h"

int ticks = 0; /* Keeps count of the ticks of the PIT */

unsigned char *memset(unsigned char *dest, unsigned char val, size_t count)
{
	unsigned char *ret = (unsigned char*) dest;
	while(count-- != 0)
	{
		*dest++ = val;
	}

	return ret;
}

unsigned short *memsetw(unsigned short *dest, unsigned short val, size_t count)
{
	unsigned short *ret = (unsigned short*) dest;
	while(count-- != 0)
	{
		*dest++ = val;
	}

	return ret;
}

void main(void *mbd, unsigned int magic)
{
        /* Pointer to multiboot data structure, get 'magic' number */
	cls();
	
	CursorPos.colour = 0x0F00;
	puts("[Reloading GDT...");
	reloadGDT();
	CursorPos.colour = 0x0A00;
	puts("Done!");
	CursorPos.colour = 0x0F00;
	puts("]\n");
	
	puts("[Reloading IDT...");
	setupIDT();
	CursorPos.colour = 0x0A00;
	puts("Done!");
	CursorPos.colour = 0x0F00;
	puts("]\n");

	puts("[Setting PIT to 100 Hz...");
	SetTimerHz(100);
	CursorPos.colour = 0x0A00;
	puts("Done!");
	CursorPos.colour = 0x0F00;
	puts("]\n");

	puts("[Remapping PICs...");
	remapPIC();
	CursorPos.colour = 0x0A00;
	puts("Done!");
	CursorPos.colour = 0x0F00;
	puts("]\n");

	puts("[Masking IRQs (PIC1: 0xFD, PIC2: 0xFF)...");
	outportb(0x21, 0xFD);
	outportb(0xA1, 0xFF);
	CursorPos.colour = 0x0A00;
	puts("Done!");
	CursorPos.colour = 0x0F00;
	puts("]\n");

	puts("[Enabling interrupts...");
	EnableInterrupts();
	CursorPos.colour = 0x0A00;
	puts("Done!");
	CursorPos.colour = 0x0F00;
	puts("]\n");

	CursorPos.colour = 0x0700;

	CursorPos.colour = 0x1000;
	for(int i = 0; i < 80; i++)
	{
		puts(" ");
	}
	CursorPos.colour = 0x0700;
	
	/* Idle loop */
        while(1 == 1)
	{
		HaltProcessor();
	}
}

void SetTimerHz(int hz)
{
	#define PIT_CMD 0x43
	#define CHL0_DAT 0x40
	int divisor = 1193180 / hz;
	int lsbdiv = divisor & 0xFF;
	int msbdiv = (divisor >> 8) & 0xFF;
	
	outportb(PIT_CMD, 0x34);
	/* 0x36 - 0011 0110
	   Bits 7-6 - SC
	   	Select counter - 00
		Counter 0 is connected to the PIC and fires IRQ0
	   Bits 5-4 - RW
	   	Read/write - 11
		01 is read LSB, 10 is read MSB, 11 is read LSB then MSB
	   Bits 3-1 - M
	   	Mode - 011
		X11 is mode 3 - square wave - and X should be 0 for future
		compatibility
	   Bits 0 - BCD
	   	Binary Coded Decimal
		0 is off
	*/

	wait();
	outportb(CHL0_DAT, lsbdiv);
	wait();
	outportb(CHL0_DAT, msbdiv);
	wait();
}

void wait()
{
	for(int i = 0; i < 6; i++) outportb(0x80, 0x00);
}
