/*----------------------------------------------------------------------------
VBE video mode information utility and graphics demo routines
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
This code is public domain (no copyright).
You can do whatever you want with it.
Release date: January 31, 2008

Compile with Turbo C or 16-bit Watcom C.
----------------------------------------------------------------------------*/
/* struct SREGS, union REGS, FP_SEG(), FP_OFF(), int86(), int86x() */
#include <dos.h>

#if 0
/* C99 fixed-width types */
#include <stdint.h>
#else
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
#endif
/*----------------------------------------------------------------------------
GRAPHICS DEMO CODE
The VBE mode-set code is below.
If that's all you want, then delete this demo code.
----------------------------------------------------------------------------*/
#include <conio.h> /* getch() */

#define	C_BLACK		g_std_colors[g_color_model][0]
#define	C_BLUE		g_std_colors[g_color_model][1]
#define	C_GREEN		g_std_colors[g_color_model][2]
#define	C_CYAN		g_std_colors[g_color_model][3]
#define	C_RED		g_std_colors[g_color_model][4]
#define	C_MAGENTA	g_std_colors[g_color_model][5]
#define	C_BROWN		g_std_colors[g_color_model][6]
#define	C_LIGHTGRAY	g_std_colors[g_color_model][7]
/*
finish filling out g_std_colors[] if you want to use these:
#define	C_DARKGRAY	g_std_colors[g_color_model][8]
#define	C_LIGHTBLUE	g_std_colors[g_color_model][9]
#define	C_LIGHTGREEN	g_std_colors[g_color_model][10]
#define	C_LIGHTCYAN	g_std_colors[g_color_model][11]
#define	C_LIGHTRED	g_std_colors[g_color_model][12]
#define	C_LIGHTMAGENTA	g_std_colors[g_color_model][13]
#define	C_YELLOW	g_std_colors[g_color_model][14]
#define	C_WHITE		g_std_colors[g_color_model][15]
*/

typedef unsigned long	color_t;

static unsigned g_wd, g_ht;
/* for a generalized 16-bit graphics library, you probably want a
bitmap object with a 'huge' pointer to the raster (to support
in-memory bitmaps larger than 64K). Using 'huge' here will also
eliminate the 'Conversion may lose significant digits' warnings
from Turbo C. 'huge' is completely broken in Turbo C++ 3.0, however. */
#define	HUGE	far
static unsigned char HUGE *g_raster;
static unsigned long g_bytes_per_row;
/* for banked framebuffer: */
static unsigned g_use_win_a, g_gran_per_64k;

static unsigned g_color_model;

/* 4 "color models"; 8 "standard" colors */
static color_t g_std_colors[4][8] =
{
/* g_color_model=0: palette color (monochrome, 16-color, and 256-color) */
	{
		/*0, 1, 2, 3, 4, 5, 6, 7,*/ 8, 9, 10, 11, 12, 13, 14, 15
	}, {
/* g_color_model=1: 15-bit color (5-5-5; True Color) */
#undef RGB2V
#define	RGB2V(R,G,B)			\
	(((((R) >> 3) & 0x1F) << 10) |	\
	 ((((G) >> 3) & 0x1F) <<  5) |	\
	 ((((B) >> 3) & 0x1F) <<  0))
		RGB2V(0,   0,   0),  	RGB2V(0,   0,   0xFF),
		RGB2V(0,   0xFF,0),	RGB2V(0,   0xFF,0xFF),
		RGB2V(0xFF,0,   0),	RGB2V(0xFF,0,   0xFF),
		RGB2V(0xFF,0xFF,0),	RGB2V(0xFF,0xFF,0xFF)
	}, {
/* g_color_model=2: 16-bit color (5-6-5; True Color) */
#undef RGB2V
#define	RGB2V(R,G,B)			\
	(((((R) >> 3) & 0x1F) << 11) |	\
	 ((((G) >> 2) & 0x3F) <<  5) |	\
	 ((((B) >> 3) & 0x1F) <<  0))
		RGB2V(0,   0,   0),  	RGB2V(0,   0,   0xFF),
		RGB2V(0,   0xFF,0),	RGB2V(0,   0xFF,0xFF),
		RGB2V(0xFF,0,   0),	RGB2V(0xFF,0,   0xFF),
		RGB2V(0xFF,0xFF,0),	RGB2V(0xFF,0xFF,0xFF)
	}, {
/* g_color_model=3: 24- or 32-bit color (High Color) */
#undef RGB2V
#define	RGB2V(R,G,B)			\
		((unsigned long)(R) << 16) | ((G) << 8) | (B)
		RGB2V(0,   0,   0),  	RGB2V(0,   0,   0xFF),
		RGB2V(0,   0xFF,0),	RGB2V(0,   0xFF,0xFF),
		RGB2V(0xFF,0,   0),	RGB2V(0xFF,0,   0xFF),
		RGB2V(0xFF,0xFF,0),	RGB2V(0xFF,0xFF,0xFF)
	}
};
/*****************************************************************************
*****************************************************************************/
#if defined(__WATCOMC__)
#include <conio.h> /* outp() */
#define	outportb(P,V)	outp(P,V)
#endif

#define VGA_SEQ_INDEX		0x3C4
#define VGA_SEQ_DATA		0x3C5
#define VGA_GC_INDEX 		0x3CE
#define VGA_GC_DATA 		0x3CF

static void set_plane(unsigned p)
{
	unsigned pmask;

	p &= 3;
	pmask = 1 << p;
	outportb(VGA_GC_INDEX, 4);
	outportb(VGA_GC_DATA, p);
	outportb(VGA_SEQ_INDEX, 2);
	outportb(VGA_SEQ_DATA, pmask);
}
/*****************************************************************************
*****************************************************************************/
static void set_bank(unsigned b)
{
	static unsigned curr_bank = -1u;
/**/
	union REGS regs;

	if(b == curr_bank)
		return;
	curr_bank = b;
	regs.x.ax = 0x4F05;
/* g_use_win_a and g_gran_per_64k were set by INT 10h AX=4F01h */
	regs.x.bx = g_use_win_a ? 0x0000 : 0x0001;
	regs.x.dx = b * g_gran_per_64k;
	int86(0x10, &regs, &regs);
}
/*****************************************************************************
Note: This function does not support 4-plane framebuffer with banks
i.e. VBE modes 0x104 (1024x768x4) or 0x106 (1280x1024x4)
*****************************************************************************/
static void write_pixel_4p(unsigned x, unsigned y, color_t c)
{
	unsigned char HUGE *ptr, mask, pmask, p;

	if(x >= g_wd || y >= g_ht)
		return;
	ptr = &g_raster[g_bytes_per_row * y + x / 8];
	mask = 0x80 >> (x & 7);
	pmask = 1;
	for(p = 0; p < 4; p++)
	{
		set_plane(p);
		if(pmask & c)
			ptr[0] |= mask;
		else
			ptr[0] &= ~mask;
		pmask <<= 1;
	}
}
/*****************************************************************************
this routine can be used for VGA mode 13h since the framebuffer
is smaller than 64K in that mode (i.e. it's 'linear')
*****************************************************************************/
#undef BPP
#define BPP 1
static void write_pixel_8(unsigned x, unsigned y, color_t c)
{
	unsigned long offset;
	uint8_t HUGE *ptr;

	if(x >= g_wd || y >= g_ht)
		return;
	offset = g_bytes_per_row * y + x * BPP;
	ptr = &g_raster[offset];
	*ptr = c;
}
/*****************************************************************************
8-bit color, banked framebuffer
*****************************************************************************/
static void write_pixel_8b(unsigned x, unsigned y, color_t c)
{
	unsigned long offset;
	uint8_t HUGE *ptr;

	if(x >= g_wd || y >= g_ht)
		return;
	offset = g_bytes_per_row * y + x * BPP;
	ptr = &g_raster[offset & 0xFFFF];
	offset >>= 16;
	set_bank((unsigned)offset);
	*ptr = c;
}
/*****************************************************************************
these two functions also work for 15-bit color,
but the color value is different

			Little-endian 16-bit value:
			b15......b12.........b8.b7.......b4..........b0
16-bit color (5-6-5):	R4 R3 R2 R1 R0 G5 G4 G3 G2 G1 G0 B4 B3 B2 B1 B0
15-bit color (5-5-5):	-  R4 R3 R2 R1 R0 G4 G3 G2 G1 G0 B4 B3 B2 B1 B0
*****************************************************************************/
#undef BPP
#define BPP 2
static void write_pixel_16(unsigned x, unsigned y, color_t c)
{
	unsigned long offset;
	uint16_t HUGE *ptr;

	if(x >= g_wd || y >= g_ht)
		return;
	offset = g_bytes_per_row * y + x * BPP;
	ptr = (uint16_t HUGE *)(&g_raster[offset]);
	*ptr = c;
}
/*****************************************************************************
*****************************************************************************/
static void write_pixel_16b(unsigned x, unsigned y, color_t c)
{
	unsigned long offset;
	uint16_t HUGE *ptr;

	if(x >= g_wd || y >= g_ht)
		return;
	offset = g_bytes_per_row * y + x * BPP;
	ptr = (uint16_t HUGE *)&g_raster[offset & 0xFFFF];
	offset >>= 16;
	set_bank((unsigned)offset);
	*ptr = c;
}
/*****************************************************************************
*****************************************************************************/
#undef BPP
#define BPP 3
static void write_pixel_24(unsigned x, unsigned y, color_t c)
{
	unsigned long offset;
	uint8_t HUGE *ptr;

	if(x >= g_wd || y >= g_ht)
		return;
	offset = g_bytes_per_row * y + x * BPP;
	ptr = (uint8_t HUGE *)(&g_raster[offset]);
	ptr[0] = c;	c >>= 8;
	ptr[1] = c;	c >>= 8;
	ptr[2] = c;
}
/*****************************************************************************
*****************************************************************************/
static void write_pixel_24b(unsigned x, unsigned y, color_t c)
{
	unsigned long offset;
	uint8_t HUGE *ptr;

	if(x >= g_wd || y >= g_ht)
		return;
	offset = g_bytes_per_row * y + x * BPP;
	ptr = (uint8_t HUGE *)(&g_raster[offset & 0xFFFF]);
	offset >>= 16;
	set_bank((unsigned)offset);
	ptr[0] = c;	c >>= 8;
	ptr[1] = c;	c >>= 8;
	ptr[2] = c;
}
/*****************************************************************************
*****************************************************************************/
#undef BPP
#define BPP 4
static void write_pixel_32(unsigned x, unsigned y, color_t c)
{
	unsigned long offset;
	uint32_t HUGE *ptr;

	if(x >= g_wd || y >= g_ht)
		return;
	offset = g_bytes_per_row * y + x * BPP;
	ptr = (uint32_t HUGE *)(&g_raster[offset]);
	*ptr = c;
}
/*****************************************************************************
*****************************************************************************/
static void write_pixel_32b(unsigned x, unsigned y, color_t c)
{
	unsigned long offset;
	uint32_t HUGE *ptr;

	if(x >= g_wd || y >= g_ht)
		return;
	offset = g_bytes_per_row * y + x * BPP;
	ptr = (uint32_t HUGE *)(&g_raster[offset & 0xFFFF]);
	offset >>= 16;
	set_bank((unsigned)offset);
	*ptr = c;
}
/*****************************************************************************
g_write_pixel must be set before calling draw_x()
*****************************************************************************/
static void (*g_write_pixel)(unsigned x, unsigned y, color_t c);

static void draw_x(unsigned mode_num)
{
	union REGS regs;
	unsigned i;

/* switch to VBE graphics mode */
	regs.x.ax = 0x4F02;
	regs.x.bx = mode_num;
	int86(0x10, &regs, &regs);
/* draw an 'X' */
	for(i = 0; i < g_ht; i++)
	{
		g_write_pixel((g_wd - g_ht) / 2 + i, i, C_BLUE);
		g_write_pixel((g_ht + g_wd) / 2 - i, i, C_RED);
	}
/* wait for key pressed */
	if(getch() == 0)
		(void)getch();
/* go back to text mode */
	regs.x.ax = 0x0003;
	int86(0x10, &regs, &regs);
}
/*----------------------------------------------------------------------------
VBE CODE
----------------------------------------------------------------------------*/
#include <string.h> /* strcpy() */
#include <stdio.h> /* printf(), sscanf() */

/* structure used by INT 10h AX=4F00h */
#pragma pack(1)
typedef struct
{
	char sig[4];
	uint8_t ver_minor;
	uint8_t ver_major;
	char HUGE *oem_name;
	uint32_t capabilities;	 /* b1=1 for non-VGA board */
	uint16_t HUGE *mode_list;
	uint16_t vid_mem_size; /* in units of 64K */
	char reserved1[492];
} vbe_info_t;

/* structure used by INT 10h AX=4F01h */
#pragma pack(1)
typedef struct
{
	uint16_t mode_attrib;	 /* b5=1 for non-VGA mode */
	uint8_t win_a_attrib;
	uint8_t win_b_attrib;
	uint16_t k_per_gran;
	uint16_t win_size;
	uint16_t win_a_seg;
	uint16_t win_b_seg;
	char reserved1[4];
/* this is not always the expected value;
rounded up to the next power of 2 for some video boards: */
	uint16_t bytes_per_row;
/* OEM modes and VBE 1.2+ only: */
	uint16_t wd;
	uint16_t ht;
	uint8_t char_wd;
	uint8_t char_ht;
	uint8_t planes;
	uint8_t depth;
	uint8_t banks;
	uint8_t memory_model;
	uint8_t k_per_bank;
	uint8_t num_pages;	 /* ? */
	char reserved2;
/* VBE 1.2+ only */
	uint8_t red_width;
	uint8_t red_shift;
	uint8_t green_width;
	uint8_t green_shift;
	uint8_t blue_width;
	uint8_t blue_shift;
	char reserved3[3];
	uint32_t lfb_adr;	/* linear framebuffer */
	char reserved4[212];
} mode_info_t;
/*****************************************************************************
this function is needed by the graphics demo code; above
*****************************************************************************/
static int get_mode_info(mode_info_t *info, unsigned mode_num)
{
	if((info->mode_attrib & 0x10) == 0)
	{
		printf("Mode 0x%X is a text mode -- can't "
			"demo graphics\n", mode_num);
		return -1;
	}
	g_wd		= info->wd;
	g_ht		= info->ht;
	g_bytes_per_row = info->bytes_per_row;
	g_gran_per_64k	= 64 / info->k_per_gran;
/* if framebuffer is larger than 64K,
real-mode code like this ALWAYS uses banks */
	if((info->win_a_attrib & 7) == 7)
	{
		g_raster = MK_FP(info->win_a_seg, 0);
		g_use_win_a = 1;
	}
	else if((info->win_b_attrib & 7) == 7)
	{
		g_raster = MK_FP(info->win_b_seg, 0);
		g_use_win_a = 0;
	}
	switch(info->depth)
	{
	case 4:
		g_write_pixel = write_pixel_4p;
		g_color_model = 0;
		break;
	case 8:
		g_write_pixel = write_pixel_8b;
		g_color_model = 0;
		break;
	case 15:
		g_write_pixel = write_pixel_16b;
		g_color_model = 1;
		break;
	case 16:
		g_write_pixel = write_pixel_16b;
		g_color_model = 2;
		break;
	case 24:
		g_write_pixel = write_pixel_24b;
		g_color_model = 3;
		break;
	case 32:
		g_write_pixel = write_pixel_32b;
		g_color_model = 3;
		break;
	default:
		printf("This software does not support %u-"
			"bit graphics\n", info->depth);
		return -1;
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	mode_info_t mode_info;
	vbe_info_t vbe_info;
	uint16_t HUGE *mnp; /* mode number pointer */
	struct SREGS sregs;
	union REGS regs;
	unsigned rmode;	/* user-requested mode; from command line */

	if(arg_c == 1)
		rmode = 0;
	else if(arg_c == 2)
	{
		sscanf(arg_v[1], "%x", &rmode);
		if(rmode < 0x100)
		{
			printf("Illegal VBE mode number 0x%X\n", rmode);
			return 1;
		}
	}
	else
	{
		printf("Argument must be hex number of supported VBE mode\n"
			"Use 'vbe' with no arguments to list modes\n");
		return 1;
	}
/* detect VBE and get info */
	strcpy(vbe_info.sig, "VBE2");
	regs.x.ax = 0x4F00;
	regs.x.di = FP_OFF(&vbe_info);
	sregs.es = FP_SEG(&vbe_info);
	int86x(0x10, &regs, &regs, &sregs);
	if(regs.x.ax != 0x004F)
	{
		printf("INT 10h AX=4F00h failed (no VBE?)\n");
		return 1;
	}
/* display VBE info */
	printf("VBE version %u.%u, %uK video memory\n",
		vbe_info.ver_major, vbe_info.ver_minor,
		vbe_info.vid_mem_size * 64);
	printf("OEM name: %Fs\n", vbe_info.oem_name);
	printf("Video board is");
	if(vbe_info.capabilities & 0x02)
		printf(" NOT");
	printf(" register-compatible with VGA\n");
/* walk mode list */
	printf(	"                             Banked   Linear\n"
		"Mode                    Gran frame-   frame-\n"
		"num-                    -ule buffer   buffer\n"
		"ber  Width Height Depth size address  address\n"
		"---- ----- ------ ----- ---- -------- --------\n");
	for(mnp = vbe_info.mode_list; *mnp != 0xFFFF; mnp++)
	{
		unsigned mode_num;
		unsigned long mem;

/* try getting info for LFB mode */
		mode_num = *mnp | 0x4000;
		regs.x.ax = 0x4F01;
		regs.x.cx = mode_num;
		regs.x.di = FP_OFF(&mode_info);
		sregs.es = FP_SEG(&mode_info);
		int86x(0x10, &regs, &regs, &sregs);
		if(regs.x.ax != 0x004F)
		{
/* that didn't work; try banked FB */
			mode_num = *mnp;
			regs.x.ax = 0x4F01;
			regs.x.cx = mode_num;
			regs.x.di = FP_OFF(&mode_info);
			sregs.es = FP_SEG(&mode_info);
			int86x(0x10, &regs, &regs, &sregs);
			if(regs.x.ax != 0x004F)
			{
				printf("INT 10h AX=4F01h CX=%04Xh failed\n",
					*mnp);
				continue;
			}
		}
/* mode not supported (probably because there's not enough video memory) */
		if((mode_info.mode_attrib & 0x0001) == 0)
			continue;
/* verify enough video memory */
		mem = (unsigned long)mode_info.bytes_per_row * mode_info.ht;
		if(mem > vbe_info.vid_mem_size * 0x10000L)
			continue;
/* display info about mode */
		printf("%4X %5u %6u %5u", mode_num, mode_info.wd,
			mode_info.ht, mode_info.depth);
/* bank-switching supported? */
		if(mode_info.mode_attrib & 0x40)
			printf(" ----");
		else
			printf(" %4u", mode_info.k_per_gran);
/* banked framebuffer must exist, be readable, and be writable */
		if((mode_info.win_a_attrib & 7) == 7)
			printf(" %8lX", mode_info.win_a_seg * 16uL);
		else if((mode_info.win_b_attrib & 7) == 7)
			printf(" %8lX", mode_info.win_b_seg * 16uL);
		else
			printf(" ????????");
/* LFB */
		if(mode_info.mode_attrib & 0x80)
			printf(" %8lX", mode_info.lfb_adr);
		else
			printf(" --------");
		printf("\n");
/* if this is the mode we want to demo, store info */
		if(mode_num == rmode)
		{
			if(get_mode_info(&mode_info, rmode))
				rmode = 0;
		}
	}
/* user specified an unsupported mode */
	if(rmode != 0 && g_wd == 0 && g_ht == 0)
	{
		printf("Mode 0x%X not supported\n", rmode);
		rmode = 0;
	}
/* switch to graphics mode, draw an 'X', quit when key pressed */
	if(rmode != 0)
		draw_x(rmode);
	return 0;
}
