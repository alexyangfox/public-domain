/*****************************************************************************
PnP demo for Turbo C or 16-bit Watcom C, using 16-bit PnP BIOS

Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: Feb 23, 2005
This code is public domain (no copyright).
You can do whatever you want with it.
*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <dos.h>
#if 0
/* C99 fixed-width types */
#include <stdint.h>
#else
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
#endif

static uint16_t g_pnp_ds;
static int far (*g_pnp_entry)(uint16_t fn, ...);
/*****************************************************************************
*****************************************************************************/
static int pnp_detect(void)
{
	unsigned char far *adr, csum;
	unsigned offset, len, i;

	printf("16-bit PnP BIOS...");
/* the spec says it's in this range, aligned on a 16-byte boundary */
	for(offset = 0; offset < 0xFFF0; offset += 16)
	{
		adr = (unsigned char far *)MK_FP(0xF000, offset);
/* bytes 0-3: signature */
		if(adr[0] != '$' || adr[1] != 'P' ||
			adr[2] != 'n' || adr[3] != 'P')
				continue;
/* byte 5: structure len */
		len = adr[5];
		if(len == 0)
			continue;
/* byte 8: checksum (checksum of entire structure must be 0) */
		csum = 0;
		for(i = 0; i < len; i++)
			csum += adr[i];
		if(csum)
			continue;
/* found it! */
		goto FOUND;
	}
	printf("not found\n");
	return -1;
FOUND:
/* byte 4: PnP version */
	i = adr[4];
	printf("version %u.%u, ", i >> 4, i & 0x0F);
/* store entry point */
	g_pnp_entry = MK_FP(
		*(uint16_t far *)(adr + 15),
		*(uint16_t far *)(adr + 13));
/* store DS value */
	g_pnp_ds = *(uint16_t far *)(adr + 27);
	printf("entry point at %Fp, DS=%04X\n", g_pnp_entry, g_pnp_ds);
	return 0;
}
/*****************************************************************************
PnP BIOS function #0
*****************************************************************************/
static int pnp_count_nodes(uint8_t far *num_nodes, uint16_t far *node_size)
{
	static const uint16_t fn = 0;
/**/

	return g_pnp_entry(fn, num_nodes, node_size, g_pnp_ds);
}
/*****************************************************************************
PnP BIOS function #1
*****************************************************************************/
static int pnp_get_node(uint8_t far *node_num, uint8_t far *node)
{
/* b0=get dynamic (current) node value,
b1=get static node value (value for next boot) */
	static const uint16_t boot = 0x01;
	static const uint16_t fn = 1;
/**/

	return g_pnp_entry(fn, node_num, node, boot, g_pnp_ds);
}
/*****************************************************************************
PnP BIOS function #2

Note that Table F0028 in Ralf Brown's list is wrong.
The real-mode C prototype for this function is not
	int (*entry)(uint16_t fn, uint8_t far *node_num,
		uint8_t far *node, uint16_t boot, uint16_t pnp_ds);
but rather:
	int (*entry)(uint16_t fn, uint16_t node_num,
		uint8_t far *node, uint16_t boot, uint16_t pnp_ds);
*****************************************************************************/
static int pnp_set_node(uint16_t node_num, uint8_t far *node)
{
/* b0=get dynamic (current) node value,
b1=get static node value (value for next boot)
Unlike pnp_get_node(), both bits can be set here. */
	static const uint16_t boot = 0x01;
	static const uint16_t fn = 2;
/**/

	return g_pnp_entry(fn, node_num, node, boot, g_pnp_ds);
}
/*****************************************************************************
converts 4-character "compressed ASCII" PnP ID at src
to 7-character normal ASCII at dst
*****************************************************************************/
static void decode_id(char *dst, unsigned char *src)
{
	unsigned i;

/* grrr...why are these values Big Endian??? */
	i = src[2];
	i <<= 8;
	i |= src[3];
	sprintf(dst + 3, "%04X", i);

	i = src[0];
	i <<= 8;
	i |= src[1];
	dst[2] = '@' + (i & 0x1F);
	i >>= 5;
	dst[1] = '@' + (i & 0x1F);
	i >>= 5;
	dst[0] = '@' + (i & 0x1F);
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
xxx - rename this function
*****************************************************************************/
static int foo(unsigned char *buf)
{
	unsigned i, j, rec_len, tag, tag_len;

	rec_len = *(uint16_t *)(buf + 0);
/* skip 12-byte header and tags describing current resource usage,
then dump _POSSIBLE_ resource assignments */
#if 1
	for(i = 12; i < rec_len; )
	{
		if(buf[i] & 0x80)
		{
			tag = 0;
			tag_len = *(uint16_t *)(buf + i + 1);
			i += 3;
		}
		else
		{
			tag = (buf[i] >> 3) & 0x0F;
			tag_len = buf[i] & 0x07;
			i++;
		}
		i += tag_len;
		if(tag == 15)
			goto OK;
	}
	printf("*** error finding device resources\n");
	return -1;
OK:
#else
/* dump _CURRENT_ resource assignments */
	i = 12;
#endif
//dump(buf + i, rec_len - i);
/* process possible resource assignments */
	while(i < rec_len)
	{
/* large items (larger than 7 bytes) */
		if(buf[i] & 0x80)
		{
			tag = buf[i] & 0x7F;
			tag_len = *(uint16_t *)(buf + i + 1);
			i += 3;
/* 6: 32-bit memory range */
			if(tag == 6 && tag_len == 9)
			{
				unsigned long lo, size;

				lo = *(uint32_t *)(buf + i + 1);
				size = *(uint32_t *)(buf + i + 5);
printf("mem=0x%lX-", lo);
printf("0x%lX\n", lo + size - 1);
			}
			else
			{
				printf("*** unrecognized large item %u "
					"(len=%u)\n", tag, tag_len);
				return -1;
			}
		}
/* small items */
		else
		{
			tag = (buf[i] >> 3) & 0x0F;
			tag_len = buf[i] & 0x07;
			i++;
/* 4: IRQ */
			if(tag == 4 && tag_len == 2)
			{
				j = *(uint16_t *)(buf + i);
printf("IRQ mask=0x%04X\n", j);
			}
/* 5: DMA */
			else if(tag == 5 && tag_len == 2)
			{
				j = *(uint16_t *)(buf + i);
printf("DMA mask=0x%04X\n", j);
			}
/* 6: begin dependent function */
			else if(tag == 6 && tag_len == 0)
			{
printf("begin dependent function\n");
			}
/* 7: end depdendent function */
			else if(tag == 7 && tag_len == 0)
			{
printf("end dependent function\n");
			}
/* 8: I/O range */
			else if(tag == 8 && tag_len == 7)
			{
				unsigned lo, hi, incr, size;

				lo = *(uint16_t *)(buf + i + 1);
				hi = *(uint16_t *)(buf + i + 3);
				incr = buf[i + 5];
				size = buf[i + 6];
if(buf[i + 0] & 0x01)
	printf("10-bit ");
printf("I/O=0x%03X-", lo);
printf("0x%03X, ", hi);
printf("align=%u, ", incr);
printf("count=%u\n", size);
			}
/* 15: end of resource list */
			else if(tag == 15)
			{
				return 0;
			}
			else
			{
				printf("*** unrecognized small item %u "
					"(len=%u)\n", tag, tag_len);
				return -1;
			}
		}
		i += tag_len;
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	static unsigned char buf[256];
/**/
	uint8_t num_nodes, node_num;
	uint16_t node_size;
	int i;

/* detect 16-bit PnP BIOS */
	if(pnp_detect())
		return 1;
/* get number and maximum size of PnP device nodes */
	i = pnp_count_nodes(&num_nodes, &node_size);
	if(i)
ERR:	{
		printf("PnP error 0x%X\n", i);
		return 1;
	}
	printf("%u nodes, max %u bytes each\n", num_nodes, node_size);
/* get nodes and display PnP IDs */
#if 1
	for(node_num = 0; node_num != 0xFF; )
	{
		printf("device #%u: ", node_num);
		i = pnp_get_node(&node_num, buf);
		if(i)
			goto ERR;
		decode_id(buf + 128, buf + 3);
		printf("%s\n", buf + 128);
		(void)foo(buf);
	}
#else
/* node #9 on my PC is the serial port */
	node_num = 9;
	i = pnp_get_node(&node_num, buf);
	if(i)
		goto ERR;
	dump(buf, *(uint16_t *)(buf + 0));
/* change the IRQ. The 3-byte tag indicating the current IRQ starts at
(buf + 12) on my system -- this will not be the case for other PCs */
	if(buf[13] == 0x10)	/* IRQ 4 */
		buf[13] = 0x08; /* IRQ 3 */
	else if(buf[13] == 0x08)/* IRQ 4 */
		buf[13] = 0x10;	/* IRQ 3 */
	else
	{
		printf("serial port does not use IRQ 3 nor IRQ 4\n");
		return 1;
	}
/* write it back. This works from a Win95 DOS box but the IRQ
conflict may cause your serial mouse to stop responding :) */
	node_num = 9;
	i = pnp_set_node(node_num, buf);
	if(i)
		goto ERR;
/* verify */
	i = pnp_get_node(&node_num, buf);
	if(i)
		goto ERR;
	dump(buf, *(uint16_t *)(buf + 0));
#endif
	return 0;
}
