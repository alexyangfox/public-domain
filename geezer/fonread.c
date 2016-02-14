/*----------------------------------------------------------------------------
Code to dump or display characters in .FON bitmap font file
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: Apr 7, 2005
This code is public domain (no copyright).
You can do whatever you want with it.

Sources: various files from www.wotsit.org

.FON files are actually Win16 DLLs. They typically contain:
1. a DOS .EXE stub which, if you attempt to "run" the DLL
   from plain DOS, displays a message like
	This program cannot be run in DOS mode
2. a Win16 New Executable (NE) header
3. a Win16 resource table
4. a FONTDIR resource
5. one or more FONT resources
6. a VERSION resource

The FONT resources contain the actual fonts. Note that the bitmaps
are stored in column-major order, i.e. for the following bitmap:

	........ ....	00 00
	.....**. ....	06 00
	....*..* ....	09 00
	...*.... *...	10 80
	..*..... .*..	20 40
	..*..... .*..	20 40
	..*..... .*..	20 40
	..****** **..	3F C0
	..*..... .*..	20 40
	..*..... .*..	20 40
	..*..... .*..	20 40
	........ ....	00 00
	........ ....	00 00
	........ ....	00 00

the data is stored in the .FON file as
	00 06 09 10 20 20 20 3F 20 20 20 00 00 00
	00 00 00 80 40 40 40 C0 40 40 40 00 00 00

Proportional spacing data is stored in a separate "character table"
----------------------------------------------------------------------------*/
/* NULL, FILE, fopen(), ftell(), fseek() */
#include <stdio.h> /* fread(), fgetc(), fclose(), printf() */
/*****************************************************************************
portability stuff to make display() work with different DOS compilers
*****************************************************************************/
#include <string.h> /* memset() */
#include <conio.h> /* getch() */
#include <dos.h> /* MK_FP(), union REGS, int86(), pokeb() */

#if defined(__TURBOC__)
/* pokeb() already defined in DOS.H */

#elif defined(__WATCOMC__)
/* I used the CauseWay DOS extender. YMMV. */
#if defined(__386__)
#define	pokeb(S,O,V)	*(unsigned char *)(16L * (S) + (O)) = V
#else
#define	pokeb(S,O,V)	*(unsigned char far *)MK_FP(S,O) = V
#endif

#elif defined(__DJGPP__)

#include <sys/farptr.h> /* _farpokeb() */
#include <go32.h> /* _dos_ds */
#include <dpmi.h> /* __dpmi_regs, __dpmi_int() */

#define	pokeb(S,O,V)	_farpokeb(_dos_ds, 16L * (S) + (O), V)

#else
#error Sorry, unsupported compiler
#endif

static char g_graphics;
static unsigned g_x_off, g_y_off;
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
DOS routine to display character 'i' from font on screen
'off' is the file offset of the font resource within Win16 .EXE file 'in'
*****************************************************************************/
static void display(FILE *in, unsigned long off, unsigned i)
{
	unsigned ht, wd, wd_bytes, x, y;
	unsigned long curr_pos;
	unsigned char buf[32];

/* remember file position */
	curr_pos = ftell(in);
/* seek to font resource and read part of it */
	fseek(in, off + 66, SEEK_SET);
	(void)fread(buf, 1, 30, in);
	if(read_le16(buf + 0) != 0) /* scalable font? */
	{
		printf("*** Not a bitmap font\n");
		fseek(in, curr_pos, SEEK_SET);
		return;
	}
	ht = read_le16(buf + 22);	/* pixheight */
	if(i >= buf[29])		/* first_char */
		i -= buf[29];
/* seek to character table and read i'th entry */
	fseek(in, off + 118 + 4 * i, SEEK_SET);
	(void)fread(buf, 1, 4, in);
	wd = read_le16(buf + 0);
	wd_bytes = (wd + 7) / 8; /* convert to bytes */
/* seek to bitmaps */
	fseek(in, off + read_le16(buf + 2), SEEK_SET);
/* switch to graphics mode */
	if(!g_graphics)
	{
		union REGS regs;

#if defined(__WATCOMC__)&&defined(__386__)
		memset(&regs, 0, sizeof(regs));
		regs.w.ax = 0x0012;
		int386(0x10, &regs, &regs);
#else
		regs.x.ax = 0x0012;
		int86(0x10, &regs, &regs);
#endif
		g_graphics = 1;
	}
	for(x = 0; x < wd_bytes; x++)
	{
		for(y = 0; y < ht; y++)
		{
			i = fgetc(in);
			pokeb(0xA000, 80 * (y + g_y_off) + x + g_x_off, i);
		}
	}
	g_x_off += (wd_bytes + 1);
	fseek(in, curr_pos, SEEK_SET);
}
/*****************************************************************************
*****************************************************************************/
#define BPERL		16	/* byte/line for dump */

static void dump(unsigned char *data, unsigned count)
{
	unsigned char byte1, byte2;

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
Seek And Dump ASCII
seek to offset 'off' of file 'in', read and display 'size' characters
*****************************************************************************/
static void sada(FILE *in, unsigned long off, unsigned size)
{
	static unsigned char buf[128];
/**/
	unsigned long curr_pos;

	curr_pos = ftell(in);
	fseek(in, off, SEEK_SET);
	if(size > sizeof(buf))
		size = sizeof(buf);
	fread(buf, 1, size, in);
	printf("%-*.*s", size, size, buf);
	fseek(in, curr_pos, SEEK_SET);
}
/*****************************************************************************
Seek And Dump Hex
seek to offset 'off' of file 'in', do ASCII/hex dump of 'size' bytes
*****************************************************************************/
static void sadh(FILE *in, unsigned long off, unsigned size)
{
	static unsigned char buf[128];
/**/
	unsigned long curr_pos;

	curr_pos = ftell(in);
	fseek(in, off, SEEK_SET);
	if(size > sizeof(buf))
		size = sizeof(buf);
	fread(buf, 1, size, in);
	dump(buf, size);
	fseek(in, curr_pos, SEEK_SET);
}
/*****************************************************************************
display info about font resource at file offset 'off' in Win16 .EXE file 'in'

.FNT file format, as encapsulated (as a Win16 resource)
inside a .FON file:

offset	size	description
------	----	-----------
0	2	version (00 02 for Win2.x, 00 03 for Win3.x)
2	4	resource/file size
6	60	copyright string
66	2	font type (b0=vector font)
68	2	points
70	2	vertical resolution (in dots-per-inch, or dpi)
72	2	horizontal resolution (in dots-per-inch, or dpi)
74	2	ascent (distance from top of character cell to baseline)
76	2(?)	internal leading (lines of pixheight used for accent marks)
78	2(?)	external leading (additional lines requested for accents)
80	1	b0=italic
81	1	b0=underline
82	1	b0=strikeout
83	2	weight (400="regular")
85	1	charset (details?)
86	2	pixwidth (non-zero for monospaced fonts only)
88	2       pixheight (total height of character cell)
90	1	pitch and family
		b0=variable pitch (same as proportional spacing?)
		b7-b4=family:
			b0-b4	family		description
			-----	------		-------------------
			0	DONTCARE	(or don't know)
			1	ROMAN		proportional spaced
			2	SWISS		proportional spaced
			3	MODERN		fixed-pitch
			4	SCRIPT		-
			5	DECORATIVE	-
91	2	average width (same as pixwidth for fixed-pitch
			fonts, else width of "X")
93	2	max width (same as pixwidth for fixed-pitch fonts)
95	1	first char in font (usually 32 for space)
96	1	last char in font (usually 255)
97	1	default char in font (used to display invalid chars)
98	1	break char (for word breaks)
99	2	width of font bitmap; in bytes (always even)
101	4	offset of string containing device name (0=none)
105	4	offset of string containing face name
109	4	(?)
113	4	offset of font bitmap
117	1	(reserved)

(Windows 2.x .FNT:)
118	-	character table (4 bytes per entry)

(Windows 3.x .FNT:)
118	4	flags: b0=fixed-pitch font, b1=proportional-spaced font,
			b2=ABC fixed font, b3=ABC proportional font,
			b4=monochrome font, b5=16-color font,
			b6=256-color font, b7=RGB font
122	2	A-space (?)
124	2	B-space (?)
126	2	C-space (?)
128	4	offset of color table (color fonts only)
132	16	(reserved)
148	-	character table (6 bytes per entry)
*****************************************************************************/
static void dump_fon(FILE *in, unsigned long res_offset, unsigned long size)
{
	static unsigned char buf[118];
/**/
	unsigned long curr_pos, k;
	unsigned i;

	curr_pos = ftell(in);
	fseek(in, res_offset, SEEK_SET);
	if(fread(buf, 1, sizeof(buf), in) != sizeof(buf))
	{
		printf("Error reading file\n");
		fseek(in, curr_pos, SEEK_SET);
		return;
	}
//printf("\t%-60.60s\n", buf + 6); /* copyright */
	i = read_le16(buf + 66);
	if(i != 0)
	{
		printf("Not a bitmap font\n");
		fseek(in, curr_pos, SEEK_SET);
		return;
	}
printf(".FNT version: %u.%u\n", buf[1], buf[0]);
printf("\tpoints:%u,", read_le16(buf + 68));
printf(" vert res:%u,", read_le16(buf + 70));
printf(" horiz res:%u,", read_le16(buf + 72));
printf(" ascent:%u\n", read_le16(buf + 74));
printf("\tinternal leading:%u,", read_le16(buf + 76));
printf(" external leading:%u\n", read_le16(buf + 78));
printf("\titalic:%s,", buf[80] ? "YES" : "NO");
printf(" underline:%s,", buf[81] ? "YES" : "NO");
printf(" strikeout:%s\n", buf[82] ? "YES" : "NO");
printf("\tweight:%u,", read_le16(buf + 83));
printf(" charset:%u", buf[85]);
printf(" pixwidth:%u,", read_le16(buf + 86));
printf(" pixheight:%u\n", read_le16(buf + 88));
printf("\tpitch:0x%02X,", buf[90]);
printf(" avg width:%u,", read_le16(buf + 91));
printf(" max width:%u\n", read_le16(buf + 93));
printf("\tfirst char:%u,", buf[95]);
printf(" last char:%u,", buf[96]);
printf(" default char:%u,", buf[97]);
printf(" break char:%u\n", buf[95] + buf[98]);
printf("\twidth in bytes:%u, ", read_le16(buf + 99));

k = read_le32(buf + 101);
printf("device:");
if(k == 0) printf("NONE"); else sada(in, k + res_offset, 16);
printf(",");

k = read_le32(buf + 105);
printf(" face:");
if(k == 0) printf("NONE"); else sada(in, k + res_offset, 16);
printf("\n");

k = read_le32(buf + 113);
// xxx - is the byte count correct here?
printf("\tbitmaps: 0x%X bytes at file offset 0x%lX\n",
 (buf[96] - buf[95]) * read_le16(buf + 88), k + res_offset);

/* xxx - version char table is at offset 118 for version 2 .FNT,
offset 148 for version 3 */
printf("\tchar table: 0x%X bytes at file offset 0x%lX\n",
 (buf[96] - buf[95]) * 4, res_offset + 118);

	fseek(in, curr_pos, SEEK_SET);
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	unsigned long new_exe_offset, shift, res_size, res_offset;
	unsigned res_type, res_count, i;
	unsigned char buf[64];
	char *in_name;
	FILE *in;

/* check args */
	if(arg_c != 2)
	{
		printf("Dumps/displays .FON files\n");
		return 1;
	}
	in_name = arg_v[1];
/* open input file */
	in = fopen(in_name, "rb");
	if(in == NULL)
	{
		printf("Error: can't open input file '%s'\n", in_name);
		return 2;
	}
/* validate DOS .EXE header */
	if(fread(buf, 1, 64, in) != 64 || buf[0] != 'M' || buf[1] != 'Z')
	{
		printf("Error: file '%s' is not a .FON file "
			"(no DOS .EXE header)\n", in_name);
		fclose(in);
		return 3;
	}
/* validate Win16 .EXE header */
	i = read_le16(buf + 8);	/* DOS header size, in 16-byte paragraphs */
	new_exe_offset = read_le32(buf + 60);
	if(i < 4 || new_exe_offset == 0)
NOT:	{
		printf("Error: file '%s' is not a .FON file "
			"(no Win16 NE header)\n", in_name);
		fclose(in);
		return 3;
	}
	fseek(in, new_exe_offset, SEEK_SET);
	if(fread(buf, 1, 38, in) != 38 || buf[0] != 'N' || buf[1] != 'E')
		goto NOT;
/* find resource table and seek to it */
	i = read_le16(buf + 36);
	if(i == 0)
	{
		printf("Error: file '%s' is not a .FON file "
			"(no Win16 resource table)\n", in_name);
		fclose(in);
		return 3;
	}
	fseek(in, i + new_exe_offset, SEEK_SET);
/* read rscAlignShift */
	if(fread(buf, 1, 2, in) != 2)
ERR:	{
		printf("Error reading file '%s'\n", in_name);
		fclose(in);
		return 4;
	}
	shift = read_le16(buf + 0);
	shift = 1 << shift;
	while(1)
	{
/* read rscEndTypes (=0) or rscTypes[0].rdTypeId */
		if(fread(buf, 1, 2, in) != 2)
			goto ERR;
		res_type = read_le16(buf + 0);
/* 0x8007=FONTDIR, 0x8008=FONT, 0x8010=VERSION */
//printf("rdTypeId=0x%X, ", res_type);
		if(res_type == 0)
			break;
/* read rtResourceCount and rtReserved */
		if(fread(buf + 4, 1, 6, in) != 6)
			goto ERR;
		res_count = read_le16(buf + 4);
//printf("rtResourceCount=%u\n", res_count);
		if(res_type != 0x8008)
		{
			fseek(in, res_count * 12, SEEK_CUR);
			continue;
		}
		for(i = 0; i < res_count; i++)
		{
/* read rtNameInfo[] */
			if(fread(buf + 10, 1, 12, in) != 12)
				goto ERR;
			res_offset = read_le16(buf + 10) * shift;
			res_size = read_le16(buf + 12) * shift;
//printf("%u: 0x%lX bytes at file offset 0x%lX\n", i, res_size, res_offset);
/* dump info about font resource */
			dump_fon(in, res_offset, res_size);
/* display characters from the font */
			display(in, res_offset, 'H');
			display(in, res_offset, 'e');
			display(in, res_offset, 'l');
			display(in, res_offset, 'l');
			display(in, res_offset, 'o');
			display(in, res_offset, '!');
/* advance "cursor" to start of next line */
			g_x_off = 0;
			g_y_off += 30;
		}
	}
/* await key pressed, then go back to text mode */
	if(g_graphics)
	{
		union REGS regs;

		getch();
#if defined(__WATCOMC__)&&defined(__386__)
		regs.w.ax = 0x0003;
		int386(0x10, &regs, &regs);
#else
		regs.x.ax = 3;
		int86(0x10, &regs, &regs);
#endif
	}
	fclose(in);
	return 0;
}
