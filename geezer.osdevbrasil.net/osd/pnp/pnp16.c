/*****************************************************************************
16-bit PnP hardware detection that uses the PnP BIOS

Ripped off from Craig Hart's Pascal code at
http://members.hyperlink.net.au/~chart

This code is public domain (no copyright).
You can do whatever you want with it.

PnP in a nutshell:
- PnP lets you
  1. detect and identify devices
  2. determine the hardware resources they require (I/O ports, IRQs,
     DMAs, memory)
  3. (for some devices) soft-set the hardware resources used by the device.

- Newer motherboards have a PnP BIOS that supports 16-bit code like this.

- Some new motherboards may also have a 32-bit PnP BIOS.
  Like the PCI BIOS, the 32-bit PnP BIOS is a BIOS32 "client".

- You can ignore the BIOS and do PnP hardware probing, but this won't
  tell you about "legacy" PnP devices, e.g. AT motherboard peripherals
  like the timer chip. These devices have PnP IDs like "PNP0xxx".
*****************************************************************************/
#include <string.h>
#include <stdio.h>

/* #include <stdint.h>
these assume sizeof(short)==2 and sizeof(long)==4 */
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;

static uint16_t g_pnp_bios_ds;
static int far (*g_pnp_bios_entry)(unsigned fn, ...);

/********************************* TURBO C **********************************/
#if defined(__TURBOC__)
#include <dos.h> /* peekb(), peek() */

#define	peekw(S,O)	peek(S,O)

/******************************** WATCOM C **********************************/
#elif defined(__WATCOMC__)

#if defined(__386__)
#error This is a 16-bit program. Compile with WCC, not WCC386.
#else
#include <dos.h> /* MK_FP() */
#define	peekb(S,O)	*(unsigned char far *)MK_FP(S,O)
#define	peekw(S,O)	*(unsigned short far *)MK_FP(S,O)
#endif

#else
#error Not Turbo C, not Watcom C. Sorry.
#endif
/*****************************************************************************
*****************************************************************************/
static int fn0(uint16_t far *node_size, uint16_t far *num_nodes)
{
	return g_pnp_bios_entry(0, num_nodes, node_size, g_pnp_bios_ds);
}
/*****************************************************************************
*****************************************************************************/
static int fn1(uint8_t far *done, uint8_t far *node)
{
	enum
	{
		THIS_BOOT = 1, NEXT_BOOT = 2
	} boot_type = THIS_BOOT;

	return g_pnp_bios_entry(1, done, node, boot_type, g_pnp_bios_ds);
}
/*****************************************************************************
*****************************************************************************/
#define BPERL		16	/* byte/line for dump */

static void dump(uint8_t *data, unsigned count)
{
	uint8_t byte1, byte2;

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
				putchar('.');
			else
				putchar(data[byte2]);
		}
		printf("\n");
		data += BPERL;
	}
}
/*****************************************************************************
opens Craig Hart's PNPID.TXT file, searches it for the 5-character 'id',
prints the corresponding human-readable device name
*****************************************************************************/
#define	MAX	128

static char *identify(char *id)
{
/* Turbo C++ 1.0 seems to "lose" anything declared 'static const'
	static const char *idfile_name = "pnpid.txt"; */
	static char *idfile_name = "pnpid.txt";
	static unsigned long file_len;
	static char init, entry[MAX];
	static FILE *idfile = NULL;
/**/
	long where, delta;
	int temp;

	if(!init)
	{
		init = 1;
		idfile = fopen(idfile_name, "r");
		if(idfile == NULL)
			printf("\t*** can't open PNP ID file '%s'\n",
				idfile_name);
		else
		{
			fseek(idfile, 0, SEEK_END);
			file_len = ftell(idfile);
		}
	}
	if(idfile == NULL)
		return NULL;
/* start in middle of file */
	where = delta = file_len >> 1;
	do
	{
/* binary-search up or down */
		delta >>= 1;
		fseek(idfile, where, SEEK_SET);
/* advance to newline */
		do temp = fgetc(idfile);
		while(temp != '\n');
/* read entry */
		fgets(entry, MAX, idfile);
/* compare IDs */
		temp = strncmp(entry, id, 7);
		if(temp > 0)
			where -= delta;
		else if(temp == 0)
		{
/* remove trailing newline */
			entry[strlen(entry) - 1] = '\0';
			return entry + 8;
		}
		else /*if(temp < 0)*/
			where += delta;
	} while(delta != 0);
	return NULL;
}
/*****************************************************************************
converts 5-character "compressed ASCII" PnP ID at src
to 7-character normal ASCII at dst
*****************************************************************************/
static void decode_id(char *dst, uint8_t *src)
{
	uint16_t temp;

	temp = src[2];
	temp <<= 8;
	temp |= src[3];
	sprintf(dst + 3, "%04X", temp);

	temp = src[0];
	temp <<= 8;
	temp |= src[1];
	dst[2] = '@' + (temp & 0x1F);
	temp >>= 5;
	dst[1] = '@' + (temp & 0x1F);
	temp >>= 5;
	dst[0] = '@' + (temp & 0x1F);
}
/*****************************************************************************
dumps resource (IRQ, DMA, I/O, memory) requirements block of a PnP device
see also  pnp_read_one_resource() in PNP.C
*****************************************************************************/
static void dump_res(uint8_t *data)
{
	uint16_t temp1, len, temp2;
	char buf[16], comma;
	uint8_t name;

	while(1)
	{
/* small item */
		if((data[0] & 0x80) == 0)
		{
			temp1 = data[0];
			temp1 >>= 3;
			name = (temp1 & 0x0F);
			len = (data[0] & 0x07);
			switch(name)
			{
			case 1:
				temp1 = data[1];
				temp1 >>= 4;
				printf("\tPnP version %u.%u\n",
					temp1 + '0', (data[1] & 0x0F) + '0');
				break;
			case 2:
				decode_id(buf, data + 1);
				printf("\tlogical device id %s\n", buf);
				break;
			case 3:
				decode_id(buf, data + 1);
				printf("\tcompatible device id %s\n", buf);
				break;
			case 4:
				printf("\tIRQ");
				comma = ' ';
				temp1 = data[2];
				temp1 <<= 8;
				temp1 |= data[1];
				goto FOO;
			case 5:
				printf("\tDMA");
				comma = ' ';
				temp1 = data[1];
FOO:
				for(temp2 = 0; temp1 != 0; temp1 >>= 1, temp2++)
				{
					if(temp1 & 1)
					{
						printf("%c%u", comma, temp2);
						comma = ',';
					}
				}
				printf("\n");
				break;
//			case 6:start dependent function
//			case 7:end dependent function
			case 8:
				temp1 = data[3];
				temp1 <<= 8;
				temp1 |= data[2];

				temp2 = data[5];
				temp2 <<= 8;
				temp2 |= data[4];
				for(; temp1 <= temp2; temp1 += data[6])
				{
					printf("\tI/O %03X-%03X\n",
						temp1, temp1 + data[7] - 1);
					if(data[6] == 0)
						break;
				}
/* 8-bit ISA cards decode only I/O address lines 0-9 (?) */
				if((data[1] & 0x01) == 0)
					printf("    *** WARNING: 10-bit "
						"I/O address\n");
				break;
			case 9:
				printf("\t(fixed location I/O range)\n");
				break;
			case 14:
				printf("\tvendor-specific\n");
				break;
/* END tag */
			case 15:
				return;
/* 10-13 */
			default:
				printf("\treserved\n");
				break;
			}
			data++;
		}
/* large item */
		else
		{
			name = (data[0] & 0x7F);
			len = data[2];
			len <<= 8;
			len |= data[1];
			switch(name)
			{
			case 1:
				printf("\t(24-bit memory range)\n");
				break;
			case 2:
				printf("\tANSI string '%s'\n", data + 1);
				break;
			case 3:
				printf("\t(Unicode string)\n");
				break;
			case 4:
				printf("\tvendor-specific\n");
				break;
			case 5:
				printf("\t(32-bit memory range)\n");
				break;
			case 6:
			{
				uint32_t start, size;

				start = data[7];
				start <<= 8;
				start |= data[6];
				start <<= 8;
				start |= data[5];
				start <<= 8;
				start |= data[4];

				size = data[11];
				size <<= 8;
				size |= data[10];
				size <<= 8;
				size |= data[9];
				size <<= 8;
				size |= data[8];
				printf("\tmem %08lX-%08lX\n",
					start, start + size - 1);
				break;
			}
			default:
				printf("\treserved\n");
				break;
			}
			data += 3;
		}
		data += len;
	}
}
/*****************************************************************************
*****************************************************************************/
static int do_pnp(uint16_t pnp_seg, uint16_t pnp_off)
{
	uint16_t entry_off, entry_seg, node_size, num_nodes;
	uint8_t pdev, dev, buf[128];
	char id[16], *name;
	int temp;

/* get data segment and real-mode entry point */
	printf("found, version is %u.%u\n",
		peekb(pnp_seg, pnp_off + 4) >> 4,
		peekb(pnp_seg, pnp_off + 4) & 0x0F);
/*dump(MK_FP(pnp_seg, pnp_off), 33);*/
	entry_off = peekw(pnp_seg, pnp_off + 13);
	entry_seg = peekw(pnp_seg, pnp_off + 15);
	g_pnp_bios_ds = peekw(pnp_seg, pnp_off + 27);
	printf("PnP BIOS entry point is %04X:%04X, "
		"data segment is %04X\n",
		entry_seg, entry_off, g_pnp_bios_ds);
	g_pnp_bios_entry = MK_FP(entry_seg, entry_off);
/* get node size and count */
	temp = fn0(&node_size, &num_nodes);
	if(temp != 0)
	{
		printf("PnP function 0 returned %d\n", temp);
		return -1;
	}
	num_nodes &= 0xFF;
	printf("node_size = %u, num_nodes = %u\n", node_size, num_nodes);
/* read nodes */
	pdev = dev = 0;
	do
	{
		temp = fn1(&dev, buf);
		if(temp != 0)
		{
			printf("PnP function 1 returned %d\n", temp);
			return -1;
		}
/* bytes 3-6 are the device ID */
		decode_id(id, buf + 3);
		name = identify(id);
		if(name != NULL)
			printf("device %2u: %s, '%s'\n", pdev, id, name);
		else
			printf("device %2u: %s\n", pdev, id);
/* resource info starts at byte 12, I guess */
		dump_res(buf + 12);
//dump(buf, node_size);
		pdev = dev;
	} while(dev != 0xFF);
	return 0;
}
/*****************************************************************************
plucked from FARCALL.LST of Ralf Brown's interrupt list

offset	bytes	what
 0	4	signature "$PnP"
 4	1	version (packed BCD: high nybble=major, low=minor)
 5	1	length of this structure
 6	2	control fields
		  b15-b2 : reserved
		  b1-b0  : event notification (00=none, 01=polling,
			10=async/interrupt)
 8	1	checksum (sum of all bytes in structure including
			this one should equal 0)
 9	4	physical adr of event notification flag, if polling
0D      2	real-mode entry point: offset
0F      2	real-mode entry point: segment
11      2	16-bit pmode entry point: offset
13      4	16-bit pmode entry point: code segment base
17      4	OEM device identifier
1B      2	real-mode data segment
1D      4	16-bit pmode data segment base
21
*****************************************************************************/
int main(void)
{
	uint16_t scan_seg = 0xF000, scan_off;

/* look for PnP BIOS */
	printf("scanning for 16-bit PnP BIOS...");
	for(scan_off = 0; scan_off < 0xFFF0; scan_off += 16)
	{
		uint8_t size, check, temp;

		if(peekb(scan_seg, scan_off + 0) != '$')
			continue;
		if(peekb(scan_seg, scan_off + 1) != 'P')
			continue;
		if(peekb(scan_seg, scan_off + 2) != 'n')
			continue;
		if(peekb(scan_seg, scan_off + 3) != 'P')
			continue;
/* verify checksum */
		size = peekb(scan_seg, scan_off + 5);
		check = 0;
		for(temp = 0; temp < size; temp++)
			check += peekb(scan_seg, scan_off + temp);
/* found it! */
		if(check == 0)
		{
			(void)do_pnp(scan_seg, scan_off);
			return 0;
		}
	}
	printf("not found\n");
	return 1;
}
