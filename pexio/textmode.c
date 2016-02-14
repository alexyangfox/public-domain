#include "kernel.h"

void cls()
{
	setcur(0, 0);
	memsetw((unsigned short*) 0xB8000, ((unsigned char) 0x20 | 0x0700), 2000);
}

void setcur(unsigned char x, unsigned char y)
{
	CursorPos.x = x;
	CursorPos.y = y;

	unsigned int index = (80 * y) + x;

	outportb(0x3D4, 14);
	outportb(0x3D5, index >> 8);
	outportb(0x3D4, 15);
	outportb(0x3D5, index);
}

void putch(const char c)
{
	if(c == 0x00)
	{
		return;
	}
	
	if(c == (unsigned char) '\n')
	{
		setcur(0, CursorPos.y + 1);
		return;
	}

	if(c == (unsigned char) '\b')
	{
		if(CursorPos.x == 0)
		{
			if(CursorPos.y >= 8)
			{
				setcur(79, (CursorPos.y - 1));
			}
			else
			{
				return;
			}
		}
		else
		{
			setcur((CursorPos.x - 1), CursorPos.y);
		}

		unsigned short location = 80 * (unsigned short) CursorPos.y;
		location += (unsigned short) CursorPos.x;

		memsetw(((unsigned short*) 0xB8000 + location), 0, 1);

		return;
	}
	
	unsigned short location = 80 * (unsigned short) CursorPos.y;
	location += (unsigned short) CursorPos.x;

	unsigned short character = c | CursorPos.colour;
	
	memsetw(((unsigned short*) 0xB8000 + location), character, 1);

	setcur(CursorPos.x + 1, CursorPos.y);

	if(CursorPos.x >= 80)
	{
		setcur(0, CursorPos.y + 1);
	}
}

void puts(const char *s)
{
	size_t len = strlen(s);
	
	for(int i = 0; i < len; i++)
	{
		putch(s[i]);
	}
}
