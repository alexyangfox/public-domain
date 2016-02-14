/*----------------------------------------------------------------------------
Simple hard disk analyzer/troubleshooter
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
This code is public domain (no copyright).
You can do whatever you want with it.
Compile with Turbo C or Borland C for DOS.

April  8, 2008
- Initial release
----------------------------------------------------------------------------*/
#include <stdarg.h> /* va_list, va_start(), va_end() */
#include <string.h> /* memset() */
#include <stdio.h> /* vsprintf(), printf() */
#include <conio.h> /* textattr(), cputs(), clrscr() */
#include <bios.h> /* _DISK_..., biosdisk() */
/* union REGS, struct SREGS, FP_SEG(), FP_OFF(), peekb(), int86x(), int86() */
#include <dos.h>
#if 0
/* C99 fixed-width types */
#include <stdint.h>
#else
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
#endif

#if 0
#define	DEBUG(X)	X
#else
#define	DEBUG(X)	/* nothing */
#endif

#if defined(__TURBOC__)
#if __TURBOC__<0x300	/* Turbo C++ 1.01 */
#define _DISK_RESET     0   /* controller hard reset */
#define _DISK_STATUS    1   /* status of last operation */
#define _DISK_READ      2   /* read sectors */
#define _DISK_WRITE     3   /* write sectors */
#endif
#endif

typedef struct
{
	unsigned char int13_drive_num;
	unsigned char heads, sectors;
	int use_lba : 1;
} disk_t;
/*----------------------------------------------------------------------------
MISC. ROUTINES
----------------------------------------------------------------------------*/
/*****************************************************************************
printf() in bold text
*****************************************************************************/
static void bprintf(const char *fmt, ...)
{
	char buf[256];
	va_list args;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	textattr(0x0F);
	cputs(buf);
	textattr(0x07);
	putchar('\r'); /* cputs() did only '\n' */
}
/*****************************************************************************
*****************************************************************************/
#define BPERL		16	/* byte/line for dump */

static void dump(void *data_p, unsigned count)
{
	unsigned char *data = (unsigned char *)data_p;
	unsigned byte1, byte2;

	while(count != 0)
	{
		for(byte1 = 0; byte1 < BPERL; byte1++)
		{
			if(count == 0)
				break;
			printf("%02X ", data[byte1]);
			count--;
		}
		printf("\t");
		for(byte2 = 0; byte2 < byte1; byte2++)
		{
			if(data[byte2] < ' ')
				printf(".");
			else
				printf("%c", data[byte2]);
		}
		printf("\n");
		data += BPERL;
	}
}
/*****************************************************************************
*****************************************************************************/
static void do_indent(unsigned indent)
{
	for(; indent != 0; indent--)
		putchar('-');
}
/*****************************************************************************
*****************************************************************************/
static unsigned long read_le32(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

/* warning: the 'u' after 0x100 is significant */
	return buf[0] + buf[1] * 0x100u + buf[2] * 0x10000L
		+ buf[3] * 0x1000000L;
}
/*****************************************************************************
*****************************************************************************/
static unsigned read_le16(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] + buf[1] * 0x100;
}
/*----------------------------------------------------------------------------
SECTOR-LEVEL DISK ACCESS
----------------------------------------------------------------------------*/
/*****************************************************************************
*****************************************************************************/
static int lba_biosdisk(int cmd, int drive, unsigned long lba,
		int nsects, void *buf)
{
/* INT 13h AH=42h/AH=43h command packet: */
#pragma pack(1)
	struct
	{
		uint8_t  pkt_len;
		uint8_t  res0;
		uint8_t  nsects;
		uint8_t  res1;
		uint16_t buf_off;
		uint16_t buf_seg;
		uint32_t lba31_0;
		uint32_t lba63_32;
	} lba_cmd_pkt;
	unsigned tries, err;
	struct SREGS sregs;
	union REGS regs;

/* fill out the INT 13h AH=4xh command packet */
	memset(&lba_cmd_pkt, 0, sizeof(lba_cmd_pkt));
	lba_cmd_pkt.pkt_len = sizeof(lba_cmd_pkt);
	lba_cmd_pkt.nsects = nsects;
	lba_cmd_pkt.buf_off = FP_OFF(buf);
	lba_cmd_pkt.buf_seg = FP_SEG(buf);
	lba_cmd_pkt.lba31_0 = lba;
/* set up registers for INT 13h AH=4xh */
	sregs.ds = FP_SEG(&lba_cmd_pkt);
	regs.x.si = FP_OFF(&lba_cmd_pkt);
	regs.h.dl = drive;
/* make 3 attempts */
	for(tries = 3; tries != 0; tries--)
	{
		if(cmd == _DISK_RESET)
			regs.h.ah = 0x00;
/*		else if(cmd == _DISK_STATUS)
			regs.h.ah = 0x01; */
		else if(cmd == _DISK_READ)
			regs.h.ah = 0x42;
		else if(cmd == _DISK_WRITE)
			regs.h.ah = 0x43;
/*		else if(cmd == _DISK_VERIFY)
			regs.h.ah = 0x04;
		else if(cmd == _DISK_FORMAT)
			regs.h.ah = 0x05; */
		else
			return 1; /* invalid command */
		int86x(0x13, &regs, &regs, &sregs);
		if(!regs.x.cflag)
			return 0;
		err = regs.h.ah;
/* reset disk */
		regs.h.ah = 0;
		int86x(0x13, &regs, &regs, &sregs);
	}
	DEBUG(printf("lba_biosdisk: error 0x%02X\n", err);)
	return err;
}
/*****************************************************************************
*****************************************************************************/
static int read_sector(disk_t *disk, unsigned char *buf, unsigned long lba)
{
	unsigned sector, head, cyl;

	if(disk->use_lba)
		return lba_biosdisk(_DISK_READ,
			disk->int13_drive_num, lba, 1, buf);
/* convert LBA to CHS */
	sector = (unsigned)(lba % disk->sectors) + 1;
	head = (unsigned)((lba / disk->sectors) % disk->heads);
	cyl = (unsigned)((lba / disk->sectors) / disk->heads);
	DEBUG(printf("read_sector: LBA %lu -> CHS=%u:%u:%u\n",
		lba, cyl, head, sector);)
	return biosdisk(_DISK_READ, disk->int13_drive_num,
		head, cyl, sector, 1, buf);
}
/*****************************************************************************
*****************************************************************************/
static int get_hd_geometry(disk_t *disk)
{
	union REGS regs;

/* make sure hard drive exists */
	if(disk->int13_drive_num - 0x80 >= peekb(0x40, 0x75))
	{
		bprintf("*** Error: hard drive 0x%02X does not exist\n",
			disk->int13_drive_num);
		return -1;
	}
/* get geometry from BIOS */
	regs.h.ah = 0x08;
	regs.h.dl = disk->int13_drive_num;
	int86(0x13, &regs, &regs);
	if(regs.x.cflag)
	{
		bprintf("*** Error can't get CHS geometry for hard "
			"drive 0x%02X\n", disk->int13_drive_num);
		return -1;
	}
	disk->heads = regs.h.dh + 1;
	disk->sectors = regs.h.cl & 0x3F;
/* use LBA if drive and BIOS support it */
	regs.h.ah = 0x41;
	regs.x.bx = 0x55AA;
	regs.h.dl = disk->int13_drive_num;
	int86(0x13, &regs, &regs);
	if(!regs.x.cflag && regs.x.bx == 0xAA55)
	{
		disk->use_lba = 1;
		DEBUG(printf("get_hd_geometry: hard drive 0x%02X "
			"using LBA\n", disk->int13_drive_num);)
		return 0;
	}
	DEBUG(printf("get_hd_geometry: hard drive 0x%02X using "
		"CHS=?:%u:%u (from INT 13h AH=08h)\n",
		disk->int13_drive_num, disk->heads, disk->sectors);)
	disk->use_lba = 0;
	return 0;
}
/*----------------------------------------------------------------------------
FAT (DOS) FILESYSTEM
----------------------------------------------------------------------------*/
/*****************************************************************************
*****************************************************************************/
int is_fat_bootsector(unsigned char *buf)
{
	unsigned i;
	int good = 1;

	DEBUG(printf("is_fat_bootsector:\n");)
/* must start with 16-bit JMP or 8-bit JMP plus NOP */
	if(buf[0] == 0xE9)
		/* OK */;
	else if(buf[0] == 0xEB && buf[2] == 0x90)
		/* OK */;
	else
	{
		DEBUG(printf("\tMissing JMP/NOP\n");)
		good = 0;
	}
/* what it says */
	i = buf[13];
	if(i == 0 || ((i - 1) & i) != 0)
	{
		DEBUG(printf("\tSectors per cluster (%u) "
			"is not a power of 2\n", i);)
		good = 0;
	}
/* must have sensible CHS geometry */
	i = read_le16(buf + 24);
	if(i == 0 || i > 63)
	{
		DEBUG(printf("\tInvalid number of sectors (%u)\n", i);)
/* old Mobius disk fails here - MTOOLS fuckup?
		good = 0; */
	}
	i = read_le16(buf + 26);
	if(i == 0 || i > 255)
	{
		DEBUG(printf("\tInvalid number of heads (%u)\n", i);)
		good = 0;
	}
	return good;
}
/*****************************************************************************
offsets in this table changed to be offsets into bootsector:

Format of BIOS Parameter Block:
Offset	Size	Description	(Table 01663)
 0Bh	WORD	number of bytes per sector
 0Dh	BYTE	number of sectors per cluster
 0Eh	WORD	number of reserved sectors at start of disk
 10h	BYTE	number of FATs
 11h	WORD	number of entries in root directory
 13h	WORD	total number of sectors
		for DOS 4.0+, set to zero if partition >32M, then set DWORD at
		  15h to actual number of sectors
 15h	BYTE	media ID byte (see #01356)
 16h	WORD	number of sectors per FAT
---DOS 3.0+ ---
 18h	WORD	number of sectors per track
 1Ah	WORD	number of heads
 1Ch	DWORD	number of hidden sectors
---DOS 4.0+ ---
 20h	DWORD	total number of sectors if word at 08h contains zero
 24h  6 BYTEs	???
 2Ah	WORD	number of cylinders
 2Ch	BYTE	device type
 2Dh	WORD	device attributes (removable or not, etc)
SeeAlso: #01395,#01664
*****************************************************************************/
#define	FAT_DIRENT_SIZE	32

static int analyze_fat_partition(disk_t *disk, unsigned long lba,
		unsigned long size, unsigned indent)
{
	unsigned long num_sects;
	unsigned char buf[512];
	int rv = 0;

	do_indent(indent);
	printf("FAT partition at LBA sector #%lu: ", lba);
/* read first sector of partition */
	if(read_sector(disk, buf, lba))
	{
		bprintf("\n*** Error: can't read partition\n");
		rv = -1;
	}
	else
	{
		if(!is_fat_bootsector(buf))
		{
			bprintf("\n*** Error: partition does not "
				"contain valid FAT filesystem\n");
			rv = -1;
		}
		else
		{
			num_sects = read_le16(&buf[19]);
			if(num_sects == 0)
				num_sects = read_le32(&buf[32]);
			printf("%lu sectors\n", num_sects);
			rv = 0;
			if(num_sects > size)
			{
				bprintf("*** Error: filesystem extends "
					"beyond end of partition\n");
				rv = -1;
			}
		}
	}
	return rv;
}
/*----------------------------------------------------------------------------
EXT2 (LINUX) FILESYSTEM
----------------------------------------------------------------------------*/
/*****************************************************************************
*****************************************************************************/
static int analyze_ext2_partition(disk_t *disk, unsigned long lba,
		unsigned long size, unsigned indent)
{
	unsigned sectors_per_block;
	unsigned long num_sects;
	unsigned char buf[512];
	int rv = 0;

	do_indent(indent);
	printf("ext2 partition at LBA sector #%lu:\n", lba);
/* read THIRD sector of partition (superblock starts at offset 1024
into partition; regardless of ext2 block size) */
	if(read_sector(disk, buf, lba + 2))
	{
		bprintf("*** Error: can't read partition\n");
		rv = -1;
	}
	else
	{
		if(read_le16(&buf[56]) != 0xEF53)
		{
			bprintf("*** Error: partition does not contain "
				"valid ext2 filesystem\n");
			rv = -1;
		}
/* minimum ext2 block size is 1K (2 sectors) */
		sectors_per_block = (unsigned)(2 << read_le32(&buf[24]));
		num_sects = read_le32(buf + 4) * sectors_per_block;
		do_indent(indent + 2);
		printf("%lu sectors\n", num_sects);
		if(num_sects > size)
		{
			bprintf("*** Error: filesystem extends "
				"beyond end of partition\n");
			rv = -1;
		}
	}
	return rv;
}
/*----------------------------------------------------------------------------
MAIN ROUTINES
----------------------------------------------------------------------------*/
/*****************************************************************************
*****************************************************************************/
static int analyze_partition_table(disk_t *disk, unsigned long lba,
		unsigned indent)
{
	unsigned char buf[512], *p;
	unsigned i;
	int rv = 0;

	do_indent(indent);
	printf("Partition table at LBA sector #%lu:\n", lba);
/* read partition table */
	if(read_sector(disk, buf, lba))
	{
		bprintf("*** Error: can't read partition table\n");
		rv = -1;
	}
	else
	{
		indent += 2;
/* scan the four entries */
		for(i = 0; i < 4; i++)
		{
			unsigned long size, lba1, lba2, lchs1, lchs2;
			unsigned c1, h1, s1, c2, h2, s2;

/* point to entry in partition table */
			p = &buf[446 + 16 * i];
/* partition type 0 means none defined */
			if(p[4] == 0)
				continue;
			do_indent(indent);
			printf("Partition table entry %u: type=0x%02X\n",
				i + 1, p[4]);
/* CHS start of partition */
			c1 = p[2] & 0xC0;
			c1 <<= 2;
			c1 |= p[3];
			h1 = p[1];
			s1 = p[2] & 0x3F;
/* LBA value calculated from CHS values */
			lchs1 = ((c1 * (unsigned long)disk->heads) + h1) *
				(unsigned long)disk->sectors + s1 - 1;
/* LBA start of partition
LBA start/end values (NOT THE CHS VALUES, though) in extended partition
table are relative to start of extended partition, so add 'lba' here */
			lba1  = read_le32(&p[8]) + lba;
			do_indent(indent + 2);
			printf("Start: LBA=%10lu, CHS=%4u:%3u:%3u, LBA "
				"from CHS=%10lu\n", lba1, c1, h1, s1, lchs1);
			if(lba1 != lchs1)
				bprintf("Warning: LBA != CHS\n");
/* CHS end of partition */
			c2 = p[6] & 0xC0;
			c2 <<= 2;
			c2 |= p[7];
			h2 = p[5];
			s2 = p[6] & 0x3F;
/* LBA value calculated from CHS values */
			lchs2 = ((c2 * (unsigned long)disk->heads) + h2) *
				(unsigned long)disk->sectors + s2 - 1;
/* LBA partition size and end of partition */
			size = read_le32(&p[12]);
			lba2 = lba1 + size - 1;
			do_indent(indent + 2);
			printf("  End: LBA=%10lu, CHS=%4u:%3u:%3u, LBA "
				"from CHS=%10lu\n", lba2, c2, h2, s2, lchs2);
			if(lba2 != lchs2)
				bprintf("Warning: LBA != CHS\n");
/* now check the corresponding partition */
			switch(p[4])
			{
/* extended partitions */
			case 5:	/* normal */
			case 15: /* w/ LBA (eh?) */
			case 30: /* hidden w/ LBA */
				if(analyze_partition_table(disk, lba1,
					indent + 2))
						rv = -1;
				break;
#if 0
/* NTFS */
			case 7: /* ...other */
			case 23: /* hidden NTFS */
				break;
#endif
/* FAT */
			case 6: /* FAT16 */
			case 11: /* FAT32 */
			case 12: /* FAT32 w/ LBA (I don't understand this) */
			case 14: /* FAT16 w/ LBA */
			case 27: /* hidden FAT32 */
			case 28: /* hidden FAT32 w/ LBA */
				if(analyze_fat_partition(disk, lba1,
					size, indent + 2))
						rv = -1;
				break;
			case 0x83:
				if(analyze_ext2_partition(disk, lba1,
					size, indent + 2))
						rv = -1;
				break;
			}
		}
	}
	return rv;
}
/*****************************************************************************
Format of partition record:
Offset	Size	Description	(Table 00651)
 00h	BYTE	boot indicator (80h = active partition)
 01h	BYTE	partition start head
 02h	BYTE	partition start sector (bits 0-5)
 03h	BYTE	partition start track (bits 8,9 in bits 6,7 of sector)
 04h	BYTE	operating system indicator (see #00652)
 05h	BYTE	partition end head
 06h	BYTE	partition end sector (bits 0-5)
 07h	BYTE	partition end track (bits 8,9 in bits 6,7 of sector)
 08h	DWORD	sectors preceding partition
 0Ch	DWORD	length of partition in sectors
SeeAlso: #00650
*****************************************************************************/
int main(void)
{
	disk_t disk;
	unsigned i;

	clrscr();
	for(i = 0; i < peekb(0x40, 0x75); i++)
	{
		disk.int13_drive_num = i + 0x80;
/* test if BIOS supports LBA; and get CHS geometry for hard disk */
		if(get_hd_geometry(&disk))
			continue;
		printf("Drive 0x%02X: %u heads, %u sectors/track\n",
			disk.int13_drive_num, disk.heads, disk.sectors);
		if(analyze_partition_table(&disk, 0, 0))
			bprintf("Errors found on drive!\n\n");
		else
			printf("No problems found\n\n");
	}
	return 0;
}
