/*****************************************************************************
checks for FAT- (DOS-) format disk, prints FAT info
compiles with Turbo C, 16-bit Watcom C, or DJGPP

This code is public domain (no copyright).
You can do whatever you want with it.
*****************************************************************************/
#include <stdlib.h> /* atoi() */
#include <stdio.h> /* printf(), sscanf() */
#include <bios.h> /* _DISK_nnn, struct diskinfo_t, _bios_disk() */

/* these assume little-endian CPU like x86 */
#define	LE16(X)	*(uint16_t *)(X)
#define	LE32(X)	*(uint32_t *)(X)

/* #include <stdint.h>
these assume sizeof(short)==2 and sizeof(long)==4 */
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
/*****************************************************************************
*****************************************************************************/
#if defined(__TURBOC__)
#if __TURBOC__<0x0300

#define _DISK_RESET     0
#define _DISK_READ      2

struct diskinfo_t
{
	unsigned drive, head, track, sector, nsectors;
	void far *buffer;
};

int _bios_disk(int cmd, struct diskinfo_t *i)
{
	return biosdisk(cmd, i->drive, i->head, i->track, i->sector,
		i->nsectors, (void *)i->buffer);
}

#endif
#endif
/*****************************************************************************
*****************************************************************************/
static int read_sector(unsigned drive, unsigned cyl, unsigned head,
		unsigned sect, unsigned char *buf)
{
	struct diskinfo_t cmd;
	unsigned tries, err;

	cmd.drive = drive;
	cmd.head = head;
	cmd.track = cyl;
	cmd.sector = sect;
	cmd.nsectors = 1;
	cmd.buffer = buf;
	for(tries = 3; tries != 0; tries--)
	{
		err = _bios_disk(_DISK_READ, &cmd);
		err >>= 8;
		if(err == 0)
			return 0;
		_bios_disk(_DISK_RESET, &cmd);
	}
	printf("error 0x%02X reading drive\n", err);
	return -1;
}
/*****************************************************************************
*****************************************************************************/
static int lg2(unsigned arg)
{
	unsigned log;

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
	unsigned temp;
	int bad = 0;

	if(buf[0] == 0xE9)
		/* OK */;
	else if(buf[0] == 0xEB && buf[2] == 0x90)
		/* OK */;
	else
	{
		printf("Missing JMP/NOP\n");
		bad = 1;
	}
/* check other stuff */
	temp = buf[13];
	if(lg2(temp) < 0)
	{
		printf("Sectors per cluster (%u) is not a power of 2\n",
			temp);
		bad = 1;
	}
	temp = buf[16];
/* very few disks have only 1 FAT, but it's valid */
	if(temp != 1 && temp != 2)
	{
		printf("Invalid number of FATs (%u)\n", temp);
		bad = 1;
	}
	temp = LE16(buf + 24);
/* can't check against dev->sects because dev->sects may not yet be set */
	if(temp == 0 || temp > 63)
	{
		printf("Invalid number of sectors (%u)\n", temp);
		bad = 1;
	}
	temp = LE16(buf + 26);
/* can't check against dev->heads because dev->heads may not yet be set */
	if(temp == 0 || temp > 255)
	{
		printf("Invalid number of heads (%u)\n", temp);
		bad = 1;
	}
	return bad;
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	unsigned num_fats, spc, bps, nre, fs, bs, rs;
	unsigned drive, part = 0;
	unsigned char buf[512];
	unsigned long ts, ds;

	if(arg_c < 2)
		drive = 0;
	else
	{
		sscanf(arg_v[1], "%X", &drive);
		if(drive != 0 && drive != 1 && drive != 0x80 && drive != 0x81)
		{
			printf("Bad INT 13h drive number 0x%X "
				"(must be 0, 1, 0x80, or 0x81)\n", drive);
			return 1;
		}
		if(arg_c < 3)
			part = 0;
		else
		{
			part = atoi(arg_v[2]);
			if(part > 3)
			{
				printf("Bad primary partition number %u "
					"(must be < 4)\n", part);
				return 1;
			}
		}
	}
	if(drive >= 0x80)
		printf("partition %u on ", part);
	printf("drive 0x%02X:\n\n", drive);
/* read floppy bootsector or hard disk MBR/partition table */
	if(read_sector(drive, 0, 0, 1, buf) != 0)
		return 1;
	if(drive >= 0x80)
	{
		unsigned char *ptab_rec, head, sect;
		unsigned short cyl;

/* point to partition table entry */
		ptab_rec = buf + 446 + 16 * part;
/* make sure it's FAT16 */
		if(ptab_rec[4] != 6)
			goto NOT;
/* get starting CHS of partition */
		cyl = ptab_rec[3];
		head = ptab_rec[1];
		sect = ptab_rec[2] & 0x3F;
		if(ptab_rec[2] & 0x40)
			cyl |= 0x100;
		if(ptab_rec[2] & 0x80)
			cyl |= 0x200;
/* read zeroth sector of partition (the hard disk bootsector) */
		if(read_sector(drive, cyl, head, sect, buf) != 0)
			return 1;
	}
	if(check_if_fat_bootsector(buf))
NOT:	{
		printf("not a FAT volume\n");
		return 1;
	}
	bps = LE16(buf + 11);
	printf("Disk geometry:\n");
	printf("  Bytes per sector = %u\n", bps);
	printf("  Number of heads = %u\n", LE16(buf + 26));
	printf("  Sectors per track = %u\n", LE16(buf + 24));
/* */
	spc = buf[13];
	printf("FAT info:\n");
	printf("  Sectors per cluster = %u\n", spc);
	num_fats = buf[16];
	printf("  Number of FATs = %u\n", num_fats);
	nre = LE16(buf + 17);
	printf("  Number of root dir entries = %u\n", nre);
	fs = LE16(buf + 22);
	printf("  Sectors per FAT = %u\n", fs);
	fs *= num_fats;
	printf("  Number of hidden sectors = %lu (partition start)\n",
		LE32(buf + 28));
/* */
	ts = LE16(buf + 19);
	if(ts == 0)
		ts = LE32(buf + 32);
	bs = LE16(buf + 14);
	rs = (nre * 32 + bps - 1) / bps;
/* */
	ds = ts - bs - rs - fs;
	while(ds % spc != 0)
		ds--;

	printf("  Total clusters = %lu ", ds / spc);
	if(ds / spc < 4085)
		printf("(FAT12)\n");
	else if(ds / spc < 65525u)
		printf("(FAT16)\n");
	else
		printf("(FAT32)\n");
	printf("  Sectors: %u boot, %u root, %u FAT, %lu data, %lu wasted, "
		"%lu total\n\n",
		bs,			/* reserved/boot sectors */
		rs,			/* root dir sectors */
		fs,			/* FAT sectors */
		ds,			/* data sectors */
		ts - bs - rs - fs - ds,	/* wasted sectors */
		ts);			/* total sectors */
	return 0;
}
