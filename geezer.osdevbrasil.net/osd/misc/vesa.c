/*****************************************************************************
This puts an Energy-Star compatible monitor to sleep by
suppressing both the horizontal and vertical sync pulses.

This code is public domain (no copyright).
You can do whatever you want with it.
*****************************************************************************/
#include <conio.h> /* getch() */
#include <stdio.h> /* printf() */
#include <dos.h> /* inportb(), outportb() */

#define	VGA_CRTC_INDEX	0x3D4 /* color emulation */
#define	VGA_CRTC_DATA	(VGA_CRTC_INDEX + 1)
/********************************* TURBO C **********************************/
#if defined(__TURBOC__)

/********************************* DJGPP ************************************/
#elif defined(__DJGPP__)

/******************************** WATCOM C **********************************/
#elif defined(__WATCOMC__)
#include <conio.h> /* inp(), outp(), inpw(), outpw() */

#define	inportb(P)	inp(P)
#define	outportb(P,V)	outp(P,V)

#else
#error Not Turbo C, not DJGPP, not Watcom C. Sorry.
#endif
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	unsigned char reg4, reg5, reg7, reg10, reg11;
/* Does this code depend on the video chip used? Maybe; maybe not.
It seems to work, no matter what video mode I'm in.
The registers that I modify are VGA-compatible.

regs_t regs;

memset(&regs, 0, sizeof(regs));
regs.R_AX = 0x4F02;
regs.R_BX = 0x105;
trap(0x10, &regs); */

	printf("1. Press a key for VESA screen-blanking\n"
		"2. Wait up to 15 seconds for screen to blank\n"
		"3. Press a key to unblank screen\n"
		"4. Wait up to 15 seconds for screen to be restored\n");
	getch();
/* unlock CRTC registers */
	outportb(VGA_CRTC_INDEX, 0x03);
	outportb(VGA_CRTC_DATA, inportb(VGA_CRTC_DATA) | 0x80);
	outportb(VGA_CRTC_INDEX, 0x11);
	outportb(VGA_CRTC_DATA, inportb(VGA_CRTC_DATA) & ~0x80);
/* SAVE REGISTERS */
	outportb(VGA_CRTC_INDEX, 0x04);
	reg4 = inportb(VGA_CRTC_DATA);

	outportb(VGA_CRTC_INDEX, 0x05);
	reg5 = inportb(VGA_CRTC_DATA);

	outportb(VGA_CRTC_INDEX, 0x07);
	reg7 = inportb(VGA_CRTC_DATA);

	outportb(VGA_CRTC_INDEX, 0x10);
	reg10 = inportb(VGA_CRTC_DATA);

	outportb(VGA_CRTC_INDEX, 0x11);
	reg11 = inportb(VGA_CRTC_DATA);
/* SET REGISTERS
...start horiz retrace=max */
	outportb(VGA_CRTC_INDEX, 0x04);
	outportb(VGA_CRTC_DATA, 0xFF);
/* ...end_horiz_retrace=0 */
	outportb(VGA_CRTC_INDEX, 0x05);
	outportb(VGA_CRTC_DATA, 0);
/* ...start_vert_retrace=max */
	outportb(VGA_CRTC_INDEX, 0x10);
	outportb(VGA_CRTC_DATA, 0xFF);

	outportb(VGA_CRTC_INDEX, 0x07);
	outportb(VGA_CRTC_DATA, inportb(VGA_CRTC_DATA) | 0x84);
/* ...end_vert_retrace=0 */
	outportb(VGA_CRTC_INDEX, 0x11);
	outportb(VGA_CRTC_DATA, inportb(VGA_CRTC_DATA) & ~0x0F);

	getch();
/* RESTORE OLD REGISTERS */
	outportb(VGA_CRTC_INDEX, 0x04);
	outportb(VGA_CRTC_DATA, reg4);

	outportb(VGA_CRTC_INDEX, 0x05);
	outportb(VGA_CRTC_DATA, reg5);

	outportb(VGA_CRTC_INDEX, 0x07);
	outportb(VGA_CRTC_DATA, reg7);

	outportb(VGA_CRTC_INDEX, 0x10);
	outportb(VGA_CRTC_DATA, reg10);

	outportb(VGA_CRTC_INDEX, 0x11);
	outportb(VGA_CRTC_DATA, reg11);

	return 0;
}
