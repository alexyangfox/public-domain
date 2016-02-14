/*----------------------------------------------------------------------------
Sector-level disk I/O code for DOS, using DJGPP or 32-bit Watcom C
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: Feb 9, 2006
This code is public domain (no copyright).
You can do whatever you want with it.
----------------------------------------------------------------------------*/
#include <string.h> /* memset() */
#include <stdio.h> /* printf() */
#if 0
/* C99 fixed-width types */
#include <stdint.h>
#else
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
#endif

#define	BPS			512	/* bytes per sector for disk */

#define _DISK_RESET     	0
#define _DISK_READ      	2
#define _DISK_WRITE     	3

#if defined(__DJGPP__)
#include <sys/farptr.h> /* _farpeekb(), _farpeekw() */
/* __dpmi_allocate_dos_memory(), __dpmi_free_dos_memory() */
#include <dpmi.h> /* __dpmi_regs, __dpmi_int() */
#include <go32.h> /* _dos_ds, dosmemput(), dosmemget() */
#define	peekb(S,O)		_farpeekb(_dos_ds, (S) * 16uL + (O))
#define	peekw(S,O)		_farpeekw(_dos_ds, (S) * 16uL + (O))

#elif defined(__WATCOMC__)
#if !defined(__386__)
#error 32-bit program -- compile with WCC386.EXE
#endif
#include <dos.h> /* union REGS, struct SREGS, int386(), int386x() */
/* These work with CauseWay DOS extender only: */
#define	dosmemput(D,N,S)	memcpy(D, (void *)(S), N)
#define	dosmemget(S,N,D)	memcpy(D, (void *)(S), N)
#define	peekb(S,O)		*(uint8_t *)((S) * 16uL + (O))
#define	peekw(S,O)		*(uint16_t *)((S) * 16uL + (O))

/* same layout as DJGPP */
typedef union
{
	struct
	{
		uint32_t edi;
		uint32_t esi;
		uint32_t ebp;
		uint32_t res;
		uint32_t ebx;
		uint32_t edx;
		uint32_t ecx;
		uint32_t eax;
	} d;
	struct
	{
		uint16_t di, res_di;
		uint16_t si, res_si;
		uint16_t bp, res_bp;
		uint16_t res[2];
		uint16_t bx, res_bx;
		uint16_t dx, res_dx;
		uint16_t cx, res_cx;
		uint16_t ax, res_ax;
		uint16_t flags, es, ds, fs, gs;
		uint16_t ip, cs, sp, ss;
	} x;
	struct
	{
		uint8_t res[16];
		uint8_t bl, bh, res_bx[2];
		uint8_t dl, dh, res_dx[2];
		uint8_t cl, ch, res_cx[2];
		uint8_t al, ah, res_ax[2];
	} h;
} __dpmi_regs;

#else
#error Sorry, unsupported compiler
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

/* Conventional Memory Buffer */
static unsigned g_cmb_size;
static unsigned long g_cmb_adr;
/*****************************************************************************
*****************************************************************************/
#if defined(__WATCOMC__)
int __dpmi_int(unsigned int_num, __dpmi_regs *rm_regs)
{
	struct SREGS sregs;
	union REGS regs;

	memset(&sregs, 0, sizeof(sregs));
	memset(&regs, 0, sizeof(regs));
	sregs.es = FP_SEG(rm_regs);
	regs.x.edi = FP_OFF(rm_regs);
	regs.x.eax = 0x0300;	/* simulate real-mode interrupt */
	regs.x.ebx = int_num;	/* AH=flags=0 */
	regs.x.ecx = 0;		/* number of words to copy between stacks */
	int386x(0x31, &regs, &regs, &sregs);
	return regs.w.cflag ? -1 : 0;
}
/*****************************************************************************
*****************************************************************************/
int __dpmi_allocate_dos_memory(unsigned paragraphs, int *sel)
{
	union REGS regs;

	memset(&regs, 0, sizeof(regs));
	regs.w.ax = 0x0100;
	regs.w.bx = paragraphs;
	int386(0x31, &regs, &regs);
	*sel = regs.w.dx;
	return regs.w.cflag ? -1 : regs.w.ax;
}
/*****************************************************************************
*****************************************************************************/
int __dpmi_free_dos_memory(int sel)
{
	union REGS regs;

	memset(&regs, 0, sizeof(regs));
	regs.w.ax = 0x0101;
	regs.w.dx = sel;
	int386(0x31, &regs, &regs);
	return regs.w.cflag ? -1 : regs.w.ax;
}
#endif
/*****************************************************************************
*****************************************************************************/
static int chs_biosdisk(int cmd, int drive, int head, int track,
		int sector, int nsects, void *buf)
{
	unsigned tries, err;
	__dpmi_regs regs;

/* make sure the conventional memory buffer is big enough */
	if(BPS * nsects > g_cmb_size)
		return 1;
/* set up registers for INT 13h AH=02/03h */
	memset(&regs, 0, sizeof(regs));
	regs.x.es = (g_cmb_adr >> 4);
	regs.h.dh = head;
	regs.h.dl = drive;
	regs.h.ch = track;
	regs.h.cl = ((track >> 2) & 0xC0) | sector;
	regs.x.bx = (g_cmb_adr & 0x0F);
/* if writing, copy the data to conventional memory now */
	if(cmd == _DISK_WRITE)
		dosmemput(buf, BPS * nsects, g_cmb_adr);
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
		__dpmi_int(0x13, &regs);
		err = regs.h.ah;
		if(err == 0)
		{
/* if reading, copy the data from conventional memory now */
			if(cmd == _DISK_READ)
				dosmemget(g_cmb_adr, BPS * nsects, buf);
			return 0;
		}
/* reset disk */
		regs.h.ah = 0;
		__dpmi_int(0x13, &regs);
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
	unsigned tries, err = 0;
	__dpmi_regs regs;

/* make sure the conventional memory buffer is big enough */
	if(BPS * nsects + sizeof(lba_cmd_pkt) > g_cmb_size)
		return 1;
/* fill out the INT 13h AH=4xh command packet */
	memset(&lba_cmd_pkt, 0, sizeof(lba_cmd_pkt));
	lba_cmd_pkt.pkt_len = sizeof(lba_cmd_pkt);
	lba_cmd_pkt.nsects = nsects;
/* use start of conventional memory buffer
for data transferred by BIOS disk I/O... */
	lba_cmd_pkt.buf_off = (g_cmb_adr + 0) & 0x0F;
	lba_cmd_pkt.buf_seg = (g_cmb_adr + 0) >> 4;
	lba_cmd_pkt.lba31_0 = lba;
/* ...use end of conventional memory buffer for the command packet itself */
	dosmemput(&lba_cmd_pkt, sizeof(lba_cmd_pkt),
		g_cmb_adr + BPS * nsects);
/* set up registers for INT 13h AH=4xh */
	memset(&regs, 0, sizeof(regs));
	regs.x.si = (g_cmb_adr + BPS * nsects) & 0x0F;
	regs.x.ds = (g_cmb_adr + BPS * nsects) >> 4;
	regs.h.dl = drive;
/* if writing, copy the data to conventional memory now */
	if(cmd == _DISK_WRITE)
		dosmemput(buf, BPS * nsects, g_cmb_adr);
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
		__dpmi_int(0x13, &regs);
		err = regs.h.ah;
		if((regs.x.flags & 0x0001) == 0)
		{
/* if reading, copy the data from conventional memory now */
			if(cmd == _DISK_READ)
				dosmemget(g_cmb_adr, BPS * nsects, buf);
			return 0;
		}
/* reset disk */
		regs.h.ah = 0;
		__dpmi_int(0x13, &regs);
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
	__dpmi_regs regs;

/* make sure hard drive exists */
	if(disk->int13_drive_num - 0x80 >= peekb(0x40, 0x75))
	{
		printf("Error in get_hd_geometry: hard drive 0x%02X "
			"does not exist\n", disk->int13_drive_num);
		return -1;
	}
/* use LBA if drive and BIOS support it */
	memset(&regs, 0, sizeof(regs));
	regs.h.ah = 0x41;
	regs.x.bx = 0x55AA;
	regs.h.dl = disk->int13_drive_num;
	__dpmi_int(0x13, &regs);
	if(!(regs.x.flags & 0x0001) && regs.x.bx == 0xAA55)
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
	__dpmi_int(0x13, &regs);
	if(regs.x.flags & 0x0001)
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
	int i, conv_mem_sel;
	unsigned size;
	disk_t disk;

/* allocate conventional memory buffer 22 sectors in size */
	g_cmb_size = BPS * 22;
	size = (g_cmb_size + 15) / 16;
	i = __dpmi_allocate_dos_memory(size, &conv_mem_sel);
	if(i == -1)
	{
		printf("Error: can't allocate conventional memory\n");
		return 1;
	}
	g_cmb_adr = i * 16;
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
		{
/* free conventional memory buffer */
			__dpmi_free_dos_memory(conv_mem_sel);
			return 1;
		}
/* dump bootsector */
		dump(boot, 64);
	}
/* free conventional memory buffer */
	__dpmi_free_dos_memory(conv_mem_sel);
	return 0;
}
