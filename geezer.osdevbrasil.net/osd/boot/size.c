/*****************************************************************************
Code to probe floppy geometry. For Turbo C, DJGPP, or Watcom C.
Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: ?
This code is public domain (no copyright).
You can do whatever you want with it.

NOTE: the biosdisk() function in DJGPP version 2.03 won't read
more than 18 sectors at once. It will fail if you use it to
read 21 sectors at once (i.e. read an entire track from a
floppy disk with 21 sectors per track).
*****************************************************************************/
#include <stdio.h> /* printf() */
#include <bios.h> /* _DISK_..., struct diskinfo_t, _bios_disk() */

#if 0
#define	DEBUG(X)	X
#else
#define	DEBUG(X)
#endif

/* this assumes sizeof(short)==2 */
#define	LE16(X)	*(unsigned short *)(X)

/********************************* TURBO C **********************************/
#if defined(__TURBOC__)
#include <dos.h> /* struct REGPACK, intr() */

#define	R_AX		r_ax
#define	R_BX		r_bx
#define	R_CX		r_cx
#define	R_DX		r_dx
#define	R_FLAGS		r_flags

#define	trap(N,R)	intr(N,R)

typedef struct REGPACK regs_t;

/* xxx - I'm not sure of the Turbo C version where these were
introduced. They are present in Turbo C++ 3.0 (__TURBOC__ == 0x401)
but not in Turbo C++ 1.0 (__TURBOC__ == 0x296) */
#if __TURBOC__<0x300
#include <dos.h> /* struct SREGS, union REGS, int86x() */

#define _DISK_RESET     0   /* controller hard reset */
#define _DISK_STATUS    1   /* status of last operation */
#define _DISK_READ      2   /* read sectors */
#define _DISK_WRITE     3   /* write sectors */
#define _DISK_VERIFY    4   /* verify sectors */
#define _DISK_FORMAT    5   /* format track */

struct diskinfo_t
{
	unsigned drive, head, track, sector, nsectors;
	void far *buffer;
};

unsigned _bios_disk(unsigned cmd, struct diskinfo_t *info)
{
	struct SREGS sregs;
	union REGS regs;

/* biosdisk() returns the 8-bit error code left in register AH by
the call to INT 13h. It does NOT return a combined, 16-bit error
code + number of sectors transferred, as described in the Turbo C
online help.

	return biosdisk(cmd, info->drive, info->head, info->track,
		info->sector, info->nsectors, info->buffer);
*/
	regs.h.ah = cmd;
	regs.h.al = info->nsectors;
	regs.x.bx = FP_OFF(info->buffer);
	regs.h.ch = info->track;
	regs.h.cl = (info->track / 256) * 64 + (info->sector & 0x3F);
	regs.h.dh = info->head;
	regs.h.dl = info->drive;
	sregs.es = FP_SEG(info->buffer);
	int86x(0x13, &regs, &regs, &sregs);
	return regs.x.ax;
}
#endif

/********************************* DJGPP ************************************/
#elif defined(__DJGPP__)
#include <dpmi.h> /* __dpmi... */

#define	R_AX		x.ax
#define	R_BX		x.bx
#define	R_CX		x.cx
#define	R_DX		x.dx
#define	R_FLAGS		x.flags

#define	trap(N,R)	__dpmi_int(N,R)

typedef __dpmi_regs regs_t;

/******************************** WATCOM C **********************************/
#elif defined(__WATCOMC__)
#include <dos.h> /* union REGPACK, intr() */

#define	R_AX		w.ax
#define	R_BX		w.bx
#define	R_CX		w.cx
#define	R_DX		w.dx
#define	R_FLAGS		w.flags

/* WARNING: for 32-bit code, unused fields of regs_t
must be zeroed before using this macro */
#define	trap(N,R)	intr(N,R)

typedef union REGPACK	regs_t;

#else
#error Not Turbo C, not DJGPP, not Watcom C. Sorry.
#endif
/*****************************************************************************
linear search to zero in on actual sectors-per-track value
seems to be faster than binary search
*****************************************************************************/
static void probe1(unsigned drive)
{
	unsigned tries, sects, err;
	unsigned char buffer[512];
	struct diskinfo_t cmd;

	printf("probing floppy geometry; please wait a few seconds...\n");
AGAIN:
	cmd.drive = drive;
	cmd.head = 0;
	cmd.track = 0;
	cmd.nsectors = 1;
	cmd.buffer = buffer;
	for(sects = 6; sects < 60; sects++)
	{
		cmd.sector = sects;
		for(tries = 3; tries != 0; tries--)
		{
			DEBUG(printf("sects=%d\n", sects);)
			err = _bios_disk(_DISK_READ, &cmd);
			err >>= 8;
			if(err == 6)
			{
				DEBUG(printf("disk change, restarting...\n");)
				goto AGAIN;
			}
			else if(err == 0)
				break;
			DEBUG(printf("\terror 0x%X\n", err);)
			(void)_bios_disk(_DISK_RESET, &cmd);
		}
		if(err != 0)
			break;
	}
	sects--;
/* reading sector 0 is a more effective way to reset the drive
than using INT 13h AH=0 */
	cmd.sector = 1;
	(void)_bios_disk(_DISK_READ, &cmd);
	printf("\tCHS=80:2:%u\n", sects);
}
/*****************************************************************************
get floppy geometry with INT 13h AH=08h (unreliable for floppies)
*****************************************************************************/
static void probe2(unsigned drive)
{
	unsigned temp;
	regs_t regs;

	printf("calling INT 13h AH=08 to get floppy geometry...\n");
memset(&regs, 0, sizeof(regs));
	regs.R_AX = 0x0800;
	regs.R_DX = drive;
	trap(0x13, &regs);
	if(regs.R_FLAGS & 0x0001)
		printf("\terror (CY bit set)\n");
	else
	{
		printf("\tdrive type=0x%X\n", regs.R_BX & 0xFF);
		printf("\tmax number of drives=%u\n", regs.R_DX & 0xFF);
		regs.R_DX >>= 8;
		temp = (regs.R_CX & 0xC0) * 4 + (regs.R_CX / 256);
		temp++;
		regs.R_DX++;
		printf("\tCHS=%u:%u:%u\n", temp, regs.R_DX,
			regs.R_CX & 0x3F);
	}
}
/*****************************************************************************
*****************************************************************************/
static int lg2(unsigned arg)
{
	unsigned char log;

	for(log = 0; log < 16; log++)
	{
		if(arg & 1)
		{
			arg >>= 1;
			return (arg != 0) ? -1 : log;
		}
		arg >>= 1;
	}
	return -1;
}
/*****************************************************************************
*****************************************************************************/
static int check_if_fat_bootsector(unsigned char *buf)
{
	unsigned short temp;
	int bad = 0;

	DEBUG(printf("check_if_fat_bootsector:\n");)
/* must start with 16-bit JMP or 8-bit JMP plus NOP */
	if(buf[0] == 0xE9)
		/* OK */;
	else if(buf[0] == 0xEB && buf[2] == 0x90)
		/* OK */;
	else
	{
		DEBUG(printf("\tMissing JMP/NOP\n");)
		bad = 1;
	}
	temp = buf[13];
	if(lg2(temp) < 0)
	{
		DEBUG(printf("\tSectors per cluster (%u) "
			"is not a power of 2\n", temp);)
		bad = 1;
	}
	temp = buf[16];
	temp = LE16(buf + 24);
	if(temp == 0 || temp > 63)
	{
		DEBUG(printf("\tInvalid number of sectors (%u)\n", temp);)
		bad = 1;
	}
	temp = LE16(buf + 26);
	if(temp == 0 || temp > 255)
	{
		DEBUG(printf("\tInvalid number of heads (%u)\n", temp);)
		bad = 1;
	}
	return bad;
}
/*****************************************************************************
read sector 0 and get geometry from the BIOS parameter block
(FAT filesystem only)
*****************************************************************************/
static void probe3(unsigned drive)
{
	unsigned char buffer[512];
	struct diskinfo_t cmd;
	unsigned tries;

	cmd.drive = drive;
	cmd.head = 0;
	cmd.track = 0;
	cmd.sector = 1;
	cmd.nsectors = 1;
	cmd.buffer = buffer;
	printf("reading sectors-per-track from sector 0 BPB...\n");
	for(tries = 3; tries != 0; tries--)
	{
		if(_bios_disk(_DISK_READ, &cmd) == 0x0001)
		{
			unsigned c, h, s;

			if(check_if_fat_bootsector(buffer) != 0)
			{
				printf("Not a FAT disk; no BPB\n");
				return;
			}
			s = LE16(buffer + 0x18);
			h = LE16(buffer + 0x1A);
/* hmmmm, calculation of cylinders fails with my 1.68 meg disk.
Sectors-per-track was set to 21, but superformat or mkdosfs left
total sectors=2880 (=18*80*2), instead of setting it to 3360 (=21*80*2) */
			c = LE16(buffer + 0x13) / s / h;
			printf("\tCHS=%u:%u:%u\n", c, h, s);
			break;
		}
		(void)_bios_disk(_DISK_RESET, &cmd);
	}
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	unsigned drive = 0; /* A: */

	probe1(drive);
	probe2(drive);
	probe3(drive);
	return 0;
}
