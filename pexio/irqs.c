#include "kernel.h"

unsigned char keymap[89] = 
{
	0,
	0x1B,
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	'0',
	'-',
	'=',
	'\b',
	'\t',
	'q',
	'w',
	'e',
	'r',
	't',
	'y',
	'u',
	'i',
	'o',
	'p',
	'[',
	']',
	'\n',
	0,
	'a',
	's',
	'd',
	'f',
	'g',
	'h',
	'j',
	'k',
	'l',
	';',
	'\'',
	'`',
	0,
	'\\',
	'z',
	'x',
	'c',
	'v',
	'b',
	'n',
	'm',
	',',
	'.',
	'/',
	0,
	'*',
	0,
	' ',
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	'-',
	0,
	0,
	0,
	'+',
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

unsigned char keymapshift[89] = 
{
	0,
	0x1B,
	'!',
	'\"',
	'$',
	'$',
	'%',
	'^',
	'&',
	'*',
	'(',
	')',
	'_',
	'+',
	'\b',
	'\t',
	'Q',
	'W',
	'E',
	'R',
	'T',
	'Y',
	'U',
	'I',
	'O',
	'P',
	'{',
	'}',
	'\n',
	0,
	'A',
	'S',
	'D',
	'F',
	'G',
	'H',
	'J',
	'K',
	'L',
	':',
	'@',
	'`',
	0,
	'|',
	'Z',
	'X',
	'C',
	'V',
	'B',
	'N',
	'M',
	'<',
	'>',
	'?',
	0,
	'*',
	0,
	' ',
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	'-',
	0,
	0,
	0,
	'+',
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
};

unsigned char keystatus = 0x00; /* Bit 7 - Left Shift
				   Bit 6 - Right Shift
				   Bit 5 - Left Ctrl
				   Bit 4 - Right Ctrl
				   Bit 3 - Scroll lock
				   Bit 2 - Num lock
				   Bit 1 - 0
				   Bit 0 - 0 */

void CIRQ0()
{
	ticks++;
	/* int x = CursorPos.x;
	int y = CursorPos.y;
	setcur(46, 24);
	char buf[10];
	puts("0x");
	puts(itoa((ticks), buf, 10));
	puts(" ticks have passed...");
	setcur(x, y); */
		
	outportb(0x20, 0x20); /* EOI */
}

void CIRQ1()
{
	unsigned char scancode = inportb(0x60);
	/* TODO: Replace the following with a select..case */
	
	if(scancode & 0x80)
	{
		if( (scancode == 0xAA) || (scancode == 0xB6) )
		{
			if(scancode == 0xAA)
			{
				keystatus -= 128;
			}
			else
			{
				keystatus -= 64;
			}
		}
		
		outportb(0x20, 0x20);
		return;
	}

	switch(scancode)
	{
		case 0x2A:
			keystatus |= 0x80;
			break;
		case 0x36:
			keystatus |= 0x40;
			break;
		case 0x47:
			setcur(0, CursorPos.y);
			break;
		case 0x4F:
			setcur(79, CursorPos.y);
			break;
		case 0x4B:
			if(CursorPos.x > 0)
			{
				setcur((CursorPos.x - 1), CursorPos.y);
			}
			else
			{
				setcur(79, (CursorPos.y - 1));
			}
			break;
		case 0x4D:
			if(CursorPos.x < 79)
			{
				setcur((CursorPos.x + 1), CursorPos.y);
			}
			else
			{
				setcur(0, (CursorPos.y + 1));
			}
			break;
		case 0x48:
			if(CursorPos.y >= 8)
			{
				setcur(CursorPos.x, (CursorPos.y - 1));
			}
			break;
		case 0x50:
			if(CursorPos.y <= 23)
			{
				setcur(CursorPos.x, (CursorPos.y + 1));
			}
			break;
		default:
			if( (keystatus & 0x80) || (keystatus & 0x40) )
			{
				putch(keymapshift[scancode]);
			}
			else
			{
				putch(keymap[scancode]);
			}
			break;
	}
		
	outportb(0x20, 0x20); /* EOI */
}

void CIRQ7()
{
	/* This IRQ is raised by the PIC if during the interrupt sequence,
	   IRn is not raised for the correct length of time:
	   	1. IRn raised high - IRRn set in PIC
		2. PIC evaluates - sends INT to CPU if required
		3. CPU drops INTA-bar low to acknowledge
		4. PIC receives INTA-bar low
	   The 'correct length of time' being defined as step 4 here */
	wait();
	outportb(0x20, 0x20); /* EOI */
}

