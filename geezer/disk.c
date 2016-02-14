/*----------------------------------------------------------------------------
Sector-level disk I/O code for DOS, using Turbo C or 16-bit Watcom C
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: Feb 9, 2006
This code is public domain (no copyright).
You can do whatever you want with it.
----------------------------------------------------------------------------*/
#include <string.h> /* memset() */
#include <stdio.h> /* printf() */
/* union REGS, struct SREGS, int86(), int86x(), peekb(), peek() */
#include <dos.h> /* FP_SEG(), FP_OFF() */
#if 0
/* C99 fixed-width types */
#include <stdint.h>
#else
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
#endif

#define _DISK_RESET     0
#define _DISK_READ      2
#define _DISK_WRITE     3

#if defined(__TURBOC__)
#define	peekw(S,O)	peek(S,O)

#elif defined(__WATCOMC__)
#if defined(__386__)
#error 16-bit program -- compile with WCC.EXE
#endif
#define	peekb(S,O)	*(uint8_t far *)MK_FP(S, O)
#define	peekw(S,O)	*(uint16_t far *)MK_FP(S, O)

#else
#error Sorry, unsupported compilier
#endif

/* these work for little-endian CPU only (like x86) */
#define	READ_LE16(X)	*(uint16_t *)(X)
#define	READ_LE32(X)	*(uint32_t *)(X)

typedef struct
{
	unsigned char int13_drive_num;
	unsigned char heads, sectors;
	int use_lba : 1;
} disk_t;
/*****************************************************************************
compiler	biosdisk()	_bios_disk()
---------------	----------	------------
Turbo C++ 1.0	yes		NO
Turbo C++ 3.0	yes		yes
Borland C++ 3.1	yes		yes
Watcom C	NO		yes

bah -- we'll write our own biosdisk()
*****************************************************************************/
static int chs_biosdisk(int cmd, int drive, int head, int track,
		int sector, int nsects, void *buf)
{
	unsigned tries, err;
	struct SREGS sregs;
	union REGS regs;

/* set up registers for INT 13h AH=02/03h */
	sregs.es = FP_SEG(buf);
	regs.h.dh = head;
	regs.h.dl = drive;
	regs.h.ch = track;
	regs.h.cl = ((track >> 2) & 0xC0) | sector;
	regs.x.bx = FP_OFF(buf);
/* make 3 attempts */
	for(tries = 3; tries != 0; tries--)
	{
		if(cmd == _DISK_RESET)
			regs.h.ah = 0x00;
/*		else if(cmd == _DISK_STATUS)
			regs.h.ah = 0x01; */
		else if(cmd == _DISK_READ)
			regs.h.ah = 0x02;
		else if(cmd == _DISK_WRITE)
			regs.h.ah = 0x03;
/*		else if(cmd == _DISK_VERIFY)
			regs.h.ah = 0x04;
		else if(cmd == _DISK_FORMAT)
			regs.h.ah = 0x05; */
		else
			return 1; /* invalid command */
		regs.h.al = nsects;
		int86x(0x13, &regs, &regs, &sregs);
		err = regs.h.ah;
		if(err == 0)
			return 0;
/* reset disk */
		regs.h.ah = 0;
		int86x(0x13, &regs, &regs, &sregs);
	}
printf("chs_biosdisk: error 0x%02X\n", err);
	return err;
}
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
printf("lba_biosdisk: error 0x%02X\n", err);
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
//printf("LBA %lu -> CHS=%u:%u:%u\n", lba, cyl, head, sector);
	return chs_biosdisk(_DISK_READ, disk->int13_drive_num,
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
		printf("Error in get_hd_geometry: hard drive 0x%02X "
			"does not exist\n", disk->int13_drive_num);
		return -1;
	}
/* use LBA if drive and BIOS support it */
	regs.h.ah = 0x41;
	regs.x.bx = 0x55AA;
	regs.h.dl = disk->int13_drive_num;
	int86(0x13, &regs, &regs);
	if(!regs.x.cflag && regs.x.bx == 0xAA55)
	{
		disk->use_lba = 1;
printf("get_hd_geometry: hard drive 0x%02X using LBA\n",
 disk->int13_drive_num);
		return 0;
	}
	disk->use_lba = 0;
/* get geometry from BIOS */
	regs.h.ah = 0x08;
	regs.h.dl = disk->int13_drive_num;
	int86(0x13, &regs, &regs);
	if(regs.x.cflag)
	{
		printf("Error in get_hd_geometry: can't get geometry "
			"for hard drive 0x%02X\n", disk->int13_drive_num);
		return -1;
	}
	disk->heads = regs.h.dh + 1;
	disk->sectors = regs.h.cl & 0x3F;
printf("get_hd_geometry: hard drive 0x%02X using CHS=?:%u:%u (from "
 "INT 13h AH=08h)\n", disk->int13_drive_num, disk->heads, disk->sectors);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int is_fat_bootsector(unsigned char *buf)
{
	int i, ok = 1;

/* must start with 16-bit JMP or 8-bit JMP plus NOP */
	if(buf[0] == 0xE9)
		/* OK */;
	else if(buf[0] == 0xEB && buf[2] == 0x90)
		/* OK */;
	else
		ok = 0;
	i = buf[13];
/* sectors/cluster must be a power of 2 */
	if(i == 0 || ((i - 1) & i) != 0)
		ok = 0;
/* number of sectors must be valid */
	i = READ_LE16(buf + 24);
	if(i == 0 || i > 63)
		ok = 0;
/* number of heads must be valid */
	i = READ_LE16(buf + 26);
	if(i == 0 || i > 255)
		ok = 0;
	return ok;
}
/*****************************************************************************
*****************************************************************************/
static int get_fd_geometry(disk_t *disk)
{
	static unsigned char buf[512];
/**/
	unsigned i;

/* count floppy drives */
	i = peekw(0x40, 0x10);
	if(i & 1)
		i = (i >> 6) & 0x03;
	else
ERR:	{
		printf("Error in get_fd_geometry: floppy drive 0x%02X "
			"does not exist\n", disk->int13_drive_num);
		return -1;
	}
/* make sure floppy drive exists */
	if(disk->int13_drive_num > i)
		goto ERR;
/* read sector 0 */
	if(chs_biosdisk(_DISK_READ, disk->int13_drive_num, 0, 0, 1, 1, buf))
		return -1;
/* test if FAT bootsector */
	if(is_fat_bootsector(buf))
	{
		disk->heads = READ_LE16(buf + 26);
		disk->sectors = READ_LE16(buf + 24);
printf("get_fd_geometry: floppy drive 0x%02X using CHS=?:%u:%u (from FAT "
 "bootsector)\n", disk->int13_drive_num, disk->heads, disk->sectors);
	}
	else
	{
		printf("probing floppy geometry, please wait...\n");
/* probe sectors-per-track value */
		for(i = 1; i < 40; i++)
		{
			if(chs_biosdisk(_DISK_READ, disk->int13_drive_num,
				0, 0, i, 1, buf))
					break;
		}
		i--;
/* ALL floppies have 2 heads -- right? */
		disk->heads = 2;
		disk->sectors = i;
printf("get_fd_geometry: floppy drive 0x%02X using CHS=?:%u:%u "
 "(probed)\n", disk->int13_drive_num, disk->heads, disk->sectors);
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
#define BPERL		16	/* byte/line for dump */

void dump(void *data_p, unsigned count)
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
int main(void)
{
	static unsigned char mbr[512], boot[512];
/**/
	unsigned long part_start;
	unsigned char *ptab;
	disk_t disk;
	unsigned i;

/* first floppy drive */
	disk.int13_drive_num = 0;
/* get geometry */
	if(get_fd_geometry(&disk) ||
/* read sector 0 (the bootsector). Floppies ALWAYS use CHS. */
		chs_biosdisk(_DISK_READ, disk.int13_drive_num,
			0, 0, 1, 1, boot))
				printf("\t(no disk in the floppy drive?)\n");
/* dump it */
	else
		dump(boot, 64);

/* first hard drive */
	disk.int13_drive_num = 0x80;
/* test if BIOS supports LBA. If not, get drive CHS geometry */
	if(get_hd_geometry(&disk))
		return 1;
/* read sector 0 (the MBR/partition table) */
	printf("\nReading master boot record/partition table "
		"(sector 0 of disk)...\n");
	if(read_sector(&disk, mbr, 0))
		return 1;
/* dump partition table */
	dump(&mbr[446], 64);
/* for each primary partition... */
	for(i = 0; i < 4; i++)
	{
		ptab = &mbr[446 + 16 * i];
/* check if it exists */
		if(ptab[4] == 0)
			continue;
/* read bootsector */
		part_start = READ_LE32(&ptab[8]);
		printf("\nReading boot sector (sector 0 of partition, "
			"sector %lu of disk)\n", part_start);
		if(read_sector(&disk, boot, part_start))
			return 1;
/* dump bootsector */
		dump(boot, 64);
	}
	return 0;
}
