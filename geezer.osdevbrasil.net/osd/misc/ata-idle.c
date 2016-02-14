/*****************************************************************************
spin down an IDE hard drive, to conserve power

This code is public domain (no copyright).
You can do whatever you want with it.

	Time to
Drive	return to		Power
state	ACTIVE state		consumption
-----	------------		-----------
ACTIVE	zero			highest
IDLE	less than STANDBY(?)	less than ACTIVE
STANDBY	up to 30 seconds	less than IDLE
SLEEP	(must reset drive)	lowest

Both idle and standby can use a timer built into the drive
(IDLE/STANDBY commands instead of IDLE IMMEDIATE/STANDBY IMMEDIATE).

SLEEP state can only be exited by resetting the drive
*****************************************************************************/
#include <conio.h> /* getch() */
#include <stdio.h> /* printf() */
#include <dos.h> /* inportb(), outportb() */

/* ATA register file (offsets from 0x1F0 or 0x170) */
#define	ATA_REG_FEAT		1	/* write: feature reg */
#define	ATA_REG_ERROR	ATA_REG_FEAT	/* read: error */

#define	ATA_REG_DRVHD		6	/* drive select; head */
#define	ATA_REG_CMD		7	/* write: drive command */
#define	ATA_REG_STATUS		7	/* read: status and error flags */

/* ATA command bytes */
#define	ATA_CMD_STANDBY_IMMED	0xE0	/* or 0x94 */
#define	ATA_CMD_IDLE_IMMED	0xE1	/* or 0x95 */
#define	ATA_CMD_STANDBY		0xE2	/* or 0x96 */
#define	ATA_CMD_IDLE		0xE3	/* or 0x97 */
#define	ATA_CMD_SLEEP		0xE6	/* or 0x99 */

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
static void nsleep(unsigned nanosecs)
{
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
/* 0x1F0 for primary interface (1st & 2nd drives),
0x170 for secondary interface (3rd and 4th drives) */
	unsigned short ioadr = 0x1F0, timeout, temp;
/* which unit on this interface: 0xA0 for master, 0xB0 for slave */
	unsigned char unit = 0xA0, stat = 0;

/* select master/slave */
	outportb(ioadr + ATA_REG_DRVHD, unit);
/* Hale Landis: ATA-4 needs delay after drive select/head reg written */
	nsleep(400);
/* issue idle/standby/sleep command */
//	outportb(ioadr + ATA_REG_CMD, ATA_CMD_IDLE_IMMED);
	outportb(ioadr + ATA_REG_CMD, ATA_CMD_STANDBY_IMMED);
//	outportb(ioadr + ATA_REG_CMD, ATA_CMD_SLEEP);
	nsleep(400);
/* await completion of command
wait up to 30 seconds for status =
BUSY=0  READY=?  DF=?  DSC=?		DRQ=?  CORR=?  IDX=?  ERR=? */
	for(timeout = 30000; timeout != 0; timeout--)
	{
		stat = inportb(ioadr + ATA_REG_STATUS);
		if((stat & 0x80) == 0)
			break;
		delay(1);
	}
	if(stat & 1)/* ERR */
	{
		temp = inportb(ioadr + ATA_REG_ERROR);
printf("uh-oh: drive status=0x%02X, error=0x%02X\n", stat, temp);
		if(temp & 4)/* ABRT */
			printf("power-saving command not supported "
				"by drive\n");
		else
			printf("unknown error\n");
		return 1;
	}
	else
	{
		printf("Drive is now in low-power state. Press a key "
			"to continue.\n(The drive may need up to 30 "
			"seconds to awaken)\n");
		getch();
	}
	return 0;
}
