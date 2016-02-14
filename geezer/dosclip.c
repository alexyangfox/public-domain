/*----------------------------------------------------------------------------
DOS access to Windows clipboard
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: September 16, 2008
This code is public domain (no copyright).
You can do whatever you want with it.

Compile with Turbo C or 16-bit Watcom C
----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	static const char to[] = "Hello";
/**/
	struct SREGS sregs;
	union REGS regs;
	char *from;

/* identify WINOLDAP version */
	regs.x.ax = 0x1700;
	int86(0x2F, &regs, &regs);
	if(regs.x.ax == 0x1700)
	{
		printf("Can't access Windows clipboard\n");
		return 1;
	}
/* open clipboard (ignore return value) */
	regs.x.ax = 0x1701;
	int86(0x2F, &regs, &regs);
/* check if any text already in clipboard */
	regs.x.ax = 0x1704;
	regs.x.dx = 0x01;
	int86(0x2F, &regs, &regs);
/* bigger than 64K? forget it... */
	if(regs.x.dx != 0)
		printf("Text in Windows clipboard is too big to display\n");
	else if(regs.x.ax == 0)
		printf("No text in Windows clipboard\n");
	else
	{
		from = malloc(regs.x.ax);
		if(from == NULL)
			printf("Not enough memory to read Windows clipboard\n");
		else
		{
			regs.x.ax = 0x1705;
			regs.x.dx = 0x01;
			regs.x.bx = FP_OFF(from);
			sregs.es = FP_SEG(from);
			int86x(0x2F, &regs, &regs, &sregs);
			if(regs.x.ax == 0)
				printf("Error reading Windows clipboard\n");
			else
				printf("Text in Windows clipboard is:\n%s\n",
					from);
			free(from);
		}
	}
/* set clipboard data */
	printf("Writing to Windows clipboard...\n");
	regs.x.ax = 0x1703;
/* 1=text, 2=bitmap, 3=.WMF...see Ralf Brown's list for more: */
	regs.x.dx = 0x01;
	regs.x.bx = FP_OFF(to);
	sregs.es = FP_SEG(to);
	regs.x.si = 0;
	regs.x.cx = sizeof(to);
	int86x(0x2F, &regs, &regs, &sregs);
	if(regs.x.ax == 0)
		printf("Error writing Windows clipboard\n");
/* close clipboard */
	regs.x.ax = 0x1708;
	int86(0x2F, &regs, &regs);
	return 0;
}
