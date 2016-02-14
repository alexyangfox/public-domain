/*****************************************************************************
LBA version of biosdisk() for DJGPP
Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: Jan 2, 2004
This code is public domain (no copyright).
You can do whatever you want with it.
*****************************************************************************/
#include <stdio.h> /* printf() */
#include <bios.h> /* _DISK_... */
#include <dpmi.h> /* __dpmi_regs, __dpmi_int() */
#include <go32.h> /* _go32_info_block, __tb, dosmemget(), dosmemput() */
#include <string.h>
#define	BPS	512	/* bytes per sector for disk */

typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned long		uint32_t;
typedef unsigned long long	uint64_t;
/*****************************************************************************
*****************************************************************************/
int lba_biosdisk(unsigned int13_drive_num, int cmd, unsigned long lba,
		unsigned nsects, void *buf)
{
/* INT 13h AH=42h/AH=43h command packet: */
	struct
	{
		uint8_t  packet_len	__attribute__((packed));
		uint8_t  reserved1	__attribute__((packed));
		uint8_t  nsects		__attribute__((packed));
		uint8_t  reserved2	__attribute__((packed));
		uint16_t buf_offset	__attribute__((packed));
		uint16_t buf_segment	__attribute__((packed));
		uint64_t lba		__attribute__((packed));
	} lba_cmd_pkt;
	unsigned tries, err = 0;
	__dpmi_regs regs;

	if(cmd != _DISK_READ && cmd != _DISK_WRITE)
		return 0x100;
/* make sure the DJGPP transfer buffer
(in conventional memory) is big enough */
	if(BPS * nsects + sizeof(lba_cmd_pkt) >
		_go32_info_block.size_of_transfer_buffer)
			return 0x100;
/* make sure drive and BIOS support LBA. Note that Win95 DOS box
emulates INT 13h AH=4x if they are not present in the BIOS. */
	regs.x.bx = 0x55AA;
	regs.h.dl = int13_drive_num;
	regs.h.ah = 0x41;
	__dpmi_int(0x13, &regs);
	if(regs.x.flags & 0x0001) /* carry bit (CY) is set */
		return 0x100;
/* fill in the INT 13h AH=4xh command packet */
	memset(&lba_cmd_pkt, 0, sizeof(lba_cmd_pkt));
	lba_cmd_pkt.packet_len = sizeof(lba_cmd_pkt);
	lba_cmd_pkt.nsects = nsects;
/* use start of transfer buffer
for data transferred by BIOS disk I/O... */
	lba_cmd_pkt.buf_offset = 0;
	lba_cmd_pkt.buf_segment = __tb >> 4;
	lba_cmd_pkt.lba = lba;
/* ...use end of transfer buffer for the command packet itself */
	dosmemput(&lba_cmd_pkt, sizeof(lba_cmd_pkt), __tb + BPS * nsects);
/* fill in registers for INT 13h AH=4xh */
	regs.x.ds = (__tb + BPS * nsects) >> 4;
	regs.x.si = (__tb + BPS * nsects) & 0x0F;
	regs.h.dl = int13_drive_num;
/* if writing, copy the data to conventional memory now */
	if(cmd == _DISK_WRITE)
		dosmemput(buf, BPS * nsects, __tb);
/* make 3 attempts */
	for(tries = 3; tries != 0; tries--)
	{
		regs.h.ah = (cmd == _DISK_READ) ? 0x42 : 0x43;
		__dpmi_int(0x13, &regs);
		err = regs.h.ah;
		if((regs.x.flags & 0x0001) == 0)
		{
/* if reading, copy the data from conventional memory now */
			if(cmd == _DISK_READ)
				dosmemget(__tb, BPS * nsects, buf);
			return 0;
		}
/* reset disk */
		regs.h.ah = 0;
		__dpmi_int(0x13, &regs);
	}
	return err;
}
/*****************************************************************************
demo routines
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
	char boot[BPS], mbr[BPS], *pte; /* partition table entry */
	unsigned i, drive = 0x80;
	unsigned long j;

/* read sector 0 of drive (the partition table; or MBR)
int13_drive_num = 0x80 for drive C: */
	if(lba_biosdisk(drive, _DISK_READ, 0, 1, mbr))
	{
		printf("Error reading partition table on drive 0x%02X\n",
			drive);
		return 1;
	}
	printf("Dump of partition table on drive 0x%02X:\n", drive);
	dump(mbr + 446, 64);
	for(i = 0; i < 4; i++)
	{
		pte = mbr + 446 + 16 * i;
/* skip partition if size == 0 */
		j = *(uint32_t *)(pte + 12);
		if(j == 0)
			continue;
/* read sector 0 of partition (the boot sector) */
		j = *(uint32_t *)(pte + 8);
		if(lba_biosdisk(drive, _DISK_READ, j, 1, boot))
		{
			printf("Error reading bootsector of drive 0x%02X, "
				"partition %u\n", drive, i);
			continue;
		}
		printf("Dump of bootsector for drive 0x%02X, "
			"partition %u:\n", drive, i);
		dump(boot, 64);
	}
	return 0;
}

