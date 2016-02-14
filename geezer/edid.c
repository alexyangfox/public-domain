/*----------------------------------------------------------------------------
Program to use INT 10h AX=4F15h to read EDIDs via DDC and analyze them
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer/
This code is public domain (no copyright).
You can do whatever you want with it.

This is a DOS program. Compile this program with Turbo C or 16-bit Watcom C.

Dec 2, 2008
- Initial release

EDID		struct
version	  year	size
-------	  ----	------
1.0	  1994	128
1.1	  1996	128
1.2,1.3   2000	128
2.0	  ?     256

References:
- VESA VBE/DDC Standard, Version 1.1, November 18, 1999
- Ralf Brown's interrupt list; entry for INT 10h AX=4F15h
- Wikipedia entry for 'EDID'
- John Fremlin's get-edid
----------------------------------------------------------------------------*/
#include <string.h> /* memcmp() */
#include <stdio.h> /* printf(), putchar() */
#include <math.h> /* sqrt() */
#include <dos.h> /* union REGS, struct SREGS, FP_SEG(), FP_OFF(), int86x() */
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
static unsigned read_le16(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] + buf[1] * 0x100;
}
/*****************************************************************************
CAUTION: the 'u' after '0x100' is significant
*****************************************************************************/
static unsigned long read_le32(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] +	buf[1] * 0x100u +
		buf[2] * 0x10000L + buf[3] * 0x1000000L;
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	unsigned verbose = 1, i, j, need_comma;
	unsigned char buf[128], *s;
	struct SREGS sregs;
	union REGS regs;

	memset(&regs, 0, sizeof(regs));
	regs.x.ax = 0x4F15;
	regs.h.bl = 0;
/* CX="controller unit number (00=primary controller)"
Presumably, this is for multihead system (multiple monitors) */
	regs.x.cx = 0;
/* spec says these must be zeroed, too */
	regs.x.di = sregs.es = 0;
	int86x(0x10, &regs, &regs, &sregs);
	if(regs.x.ax != 0x004F)
	{
		printf("INT 10h AX=4F15h BL=0 failed (BIOS doesn't do DDC)\n");
		return 1;
	}
	if(verbose)
	{
		printf("DDC transfer takes %u second(s) per block\n",
			regs.h.bh);
		printf("DDC1 %s supported\n",
			(regs.h.bl & 0x01) ? "is" : "is NOT");
		printf("DDC2 %s supported\n",
			(regs.h.bl & 0x02) ? "is" : "is NOT");
		printf("Monitor %s blanked during transfers\n",
			(regs.h.bl & 0x04) ? "is" : "is NOT");
	}
	regs.x.ax = 0x4F15;
	regs.h.bl = 1;
	regs.x.cx = 0;
/* EDID block number: */
	regs.x.dx = 0;
	sregs.es = FP_SEG(buf);
	regs.x.di = FP_OFF(buf);
	int86x(0x10, &regs, &regs, &sregs);
	if(regs.x.ax != 0x004F)
	{
		printf("INT 10h AX=4F15h BL=1 failed (can't read EDID)\n");
		return 1;
	}
/* the PDF spec I have provides no information about the EDIDs themselves;
only the INT 10h interface */
	j = 0;
	for(i = 0; i < 128; i++)
		j += buf[i];
	if(j & 0xFF)
		printf("Warning: EDID checksum failure\n");
/* bytes 0-7: signature */
	if(memcmp(&buf[0], "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8)
		&& memcmp(&buf[0], "\x00\xFF\xFF\xFF\xFF\xFF\xFF\x00", 8))
	{
		printf("Warning: invalid signature bytes at start of EDID:\n");
		dump(buf, 8);
	}
	if(verbose)
	{
/* bytes 8-9: 3-letter manufacturer code
		i = read_le16(&buf[8]);
why is this big-endian?? */
		i = buf[8] * 256 + buf[9];
		buf[2] = '@' + (i & 0x1F);
		i >>= 5;
		buf[1] = '@' + (i & 0x1F);
		i >>= 5;
		buf[0] = '@' + (i & 0x1F);
		printf("Manufacturer: %c%c%c\n", buf[0], buf[1], buf[2]);
/* bytes 10-11: 16-bit manufacturer-specific model number */
		printf("EDID ID code (monitor model): 0x%X\n",
			read_le16(&buf[10]));
/* bytes 12-15: 32-bit serial number. ### - has to be adjusted
for various manufacturer codes; see Ralf Brown's list */
		printf("Serial number: %lu\n", read_le32(&buf[12]));
/* bytes 16-17: year and week of manufacture */
		printf("Monitor was built in week %u of year %u\n",
			buf[16], buf[17] + 1990);
	}
/* bytes 18-19: EDID version */
	printf("EDID version %u.%u\n", buf[18], buf[19]);
	if(verbose)
	{
/* byte 20: input type */
		printf("Input type: ");
		printf((buf[20] & 0x80) ? "digital" : "analog");
		i = buf[20];
		i >>= 5;
		i &= 3;
		{
			static const char *voltage[] =
			{
				"0.700V/0.300V (1.00 Vpp)",
				"0.714V/0.286V",
				"0.100V/0.400V",
				"reserved"
			};
			printf(", %s", voltage[i]);
		}
		if(buf[20] & 0x04)
			printf(", sync-on-green");
		if(buf[20] & 0x02)
			printf(", composite sync");
		if(buf[20] & 0x01)
			printf(", separate sync");
		putchar('\n');
/* bytes 21-22: monitor size */
		printf("Monitor size: %u cm horiz, %u cm vert, %4.1f in "
			"diagonal\n", buf[21], buf[22],
			sqrt(buf[21] * buf[21] + buf[22] * buf[22]) / 2.54);
/* byte 23: monitor gamma */
		printf("Gamma: %4.2f\n", 1.0 + buf[23] / 100.0);
	}
/* byte 24: DPMS */
#undef MSG
#define	MSG(X)				\
	{				\
		if(need_comma)		\
			putchar(',');	\
		printf(X);		\
		need_comma = 1;		\
	}
	printf("DPMS:");
	need_comma = 0;
	if(buf[24] & 0x80)
		MSG(" standby");
	if(buf[24] & 0x40)
		MSG(" suspend");
	if(buf[24] & 0x80)
		MSG(" active off");
	if(buf[24] & 0x08)
		MSG(" non-RGB monitor");
	putchar('\n');

/* ### - bytes 25-34: chroma. Probably useful if you get anal
about color rendering (e.g. ICC) */

/* bytes 35-36: "established" timings */
	printf("'Established' timings: ");
	need_comma = 0;
	if(buf[35] & 0x80)
		MSG(" 800x600@60Hz");
	if(buf[35] & 0x40)
		MSG(" 800x600@56Hz");
	if(buf[35] & 0x20)
		MSG(" 640x480@75Hz");
	if(buf[35] & 0x10)
		MSG(" 640x480@72Hz");
	if(buf[35] & 0x08)
		MSG(" 640x480@67Hz (Mac II)");
	if(buf[35] & 0x04)
		MSG(" 640x480@60Hz");
	if(buf[35] & 0x02)
		MSG(" 720x400@88Hz (XGA2)");
	if(buf[35] & 0x01)
		MSG(" 720x400@70Hz");
	if(buf[36] & 0x80)
		MSG(" 1280x1024@75Hz");
	if(buf[36] & 0x40)
		MSG(" 1024x768@75Hz");
	if(buf[36] & 0x20)
		MSG(" 1024x768@70Hz");
	if(buf[36] & 0x10)
		MSG(" 1024x768@60Hz");
	if(buf[36] & 0x08)
		MSG(" 1024x768@87Hz interlaced");
	if(buf[36] & 0x04)
		MSG(" 832x624@75Hz (Mac II)");
	if(buf[36] & 0x02)
		MSG(" 800x600@75Hz");
	if(buf[36] & 0x01)
		MSG(" 800x600@72Hz");
	putchar('\n');

/* xxx - byte 37: "reserved" timing */

/* bytes 38-53: eight 2-byte "standard" timings */
	printf("'Standard' timings:");
	need_comma = 0;
	for(i = 0; i < 8; i++)
	{
		unsigned hres, vsync, vres;

		s = &buf[38 + i * 2];
		if((s[0] == 0 && s[1] == 0) || (s[0] == 1 && s[1] == 1))
			continue;
/* horizontal resolution */
		hres = (s[0] + 31) * 8;
		j    =  s[1];
/* vsync frequency & aspect ratio */
		vsync = (j & 0x3F) + 60;
		j >>= 6;
		if(j == 0)
			vres = 0;
		else if(j == 1)
			vres = (hres * 3) / 4;
		else if(j == 2)
			vres = (hres * 4) / 5;
		else if(j == 3)
			vres = (hres * 9) / 16;
		if(need_comma)
			putchar(',');
		printf(" %ux%u@%uHz", hres, vres, vsync);
		need_comma = 1;
	}
	putchar('\n');
/* bytes 54-125: four 18-byte "detailed" timings or text IDs */
	for(i = 0; i < 4; i++)
	{
		s = &buf[54 + i * 18];
		if(s[0] == 0 && s[1] == 0 && s[2] == 0)
		{
/* ...oh, and monitor limits, too: */
			if(s[3] == 0xFD)
			{
				printf("Supported freq: "
					"%u<=vsync<=%u (Hz), "
					"%u<=hsync<=%u (kHz)",
					s[5], s[6], s[7], s[8]);
/* these are from John Fremlin's get-edid */
				if(s[9] != 0xFF)
					printf(", dotclock<=%u (MHz)",
						s[9] * 10);
				if(s[10])
					printf(", GTF");
				putchar('\n');
			}
			else if(verbose)
			{
#undef MSG
#define MSG(X)							\
	{							\
		printf(X);					\
		for(j = 5; j <= 18; j++)			\
		{						\
			if(s[j] == '\x0A' || s[j] == '\0')	\
				break;				\
			putchar(s[j]);				\
		}						\
		putchar('\n');					\
	}
				if(s[3] == 0xFF)
					MSG("Serial number: ")
				else if(s[3] == 0xFE)
					MSG("Vendor name: ")
				else if(s[3] == 0xFC)
					MSG("Model name: ")
/* 0xFB= color point data (Wikipedia entry for 'EDID' has details),
0xFA=standard timing data (same format as above),
0xF9=undefined, 0xF8=defined by manufacturer */
				else
				{
					MSG("Text of unknown type: ")
					dump(s, 18);
				}
			}
		}
		else
		{
			unsigned hs_start, hs_wd, vs_start, vs_wd;
			unsigned hdisp, vdisp, htot, vtot;
			double clock;

#define LN2MSB(X)	(((X) << 8) & 0x0F00)	/*  Low Nibble to MSB */
#define HN2MSB(X)	(((X) << 4) & 0x0F00)	/* High Nibble to MSB */
/* Ralf Brown's list ("Format of Detailed Timing Description", "Table 00133")
got bytes 0-1 wrong. Also, I got more information about this struct
from John Fremlin's get-edid and Wikipedia.
bytes 0-1: pixel clock */
			clock = read_le16(&s[0]) * 10000.0;
/* bytes 2,4: horizontal displayed */
			hdisp = HN2MSB(s[4]) | s[2];
/* bytes 3,4: horizontal blanking */
			htot = hdisp + (LN2MSB(s[4]) | s[3]);
/* bytes 5,7: vertical displayed */
			vdisp = HN2MSB(s[7]) | s[5];
/* bytes 6,7: vertical blanking */
			vtot = vdisp + (LN2MSB(s[7]) | s[6]);
/* bytes 8, 11: start of horizontal sync (w.r.t. start of blanking)
DAMN, this is ugly... */
			hs_start = ((s[11] << 2) & 0x0300) | s[8];
/* bytes 9, 11: horizontal sync width */
			hs_wd    = ((s[11] << 4) & 0x0300) | s[9];
/* bytes 10,11: start of vertical sync (w.r.t. start of blanking) */
			vs_start = ((s[11] << 2) & 0x0030) | (s[10] >> 4);
/* bytes 10,11: vertical sync width */
			vs_wd    = ((s[11] << 4) & 0x0030) | (s[10] & 0x0F);
			printf("'Detailed' timing: clock=%5.1f MHz\n",
				clock / 1000000.0);
			printf("\tHorizontal: %4u %4u %4u %4u\n",
				hdisp, hdisp + hs_start,
				hdisp + hs_start + hs_wd, htot);
			printf("\tVertical:   %4u %4u %4u %4u\n",
				vdisp, vdisp + vs_start,
				vdisp + vs_start + vs_wd, vtot);
/* byte 12: horizontal image size (mm)
byte 13: vertical image size (mm)
byte 14: "horizontal image size 2 / vertical image size 2"
byte 15: horizontal border (in pixels)
byte 16: vertical border (in lines)
byte 17: flags/"type of display" */
#undef MSG
#define	MSG(X)				\
	{				\
		if(need_comma)		\
			putchar(',');	\
		printf(X);		\
		need_comma = 1;		\
	}
			printf("\tFlags:");
			need_comma = 0;
			if(s[17] & 0x80)
				MSG(" interlaced");
/* according to Ralf Brown's list, b6-b5 are for stereoscopic display.
b4-b3: 0x00=analog composite sync, 0x08=bipolar analog composite sync,
0x10=digital composite sync, 0x18=digital separate sync */
			if((s[17] & 0x18) == 0x18)
			{
				if(s[17] & 0x04)
					MSG(" +hsync")
				else
					MSG(" -hsync");
				if(s[17] & 0x02)
					MSG(" +vsync")
				else
					MSG(" -vsync");
			}
			else
			{
/* xxx - 'serrate' is also from Ralf Brown's list -- what is it?
				if(s[17] & 0x04)
					MSG(" serrate"); */
				if((s[17] & 0x02) == 0)
					MSG(" sync-on-green");
				/* else sync-on-RGB */
			}
			putchar('\n');
			printf("\thsync=%4.1f kHz, vsync=%4.1f Hz\n",
				clock / htot / 1000.0, clock / htot / vtot);
		}
	}
/* byte 126: 0 for EDID 1.1, number of extension blocks for EDID 1.3 */
	printf("byte 126=%u\n", buf[126]);
	return 0;
}
