/*****************************************************************************
DOS VGA/VBE graphics demo for 16-bit compiler (Turbo C; 16-bit Watcom C)
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: July 14, 2004
This code is public domain (no copyright).
You can do whatever you want with it.

EXPORTS:
extern img_t g_fb;

void set_plane(unsigned p);
void set_bank(unsigned b);
int get_modes(void);
int set_graphics_mode(unsigned wd, unsigned ht,
		unsigned depth, unsigned flags);
*****************************************************************************/
#include <stdlib.h> /* malloc(), realloc() */
#include <string.h> /* strcpy() */
#include <stdio.h> /* printf() */
/* union REGS, int86(), FP_SEG(), FP_OFF(), MK_FP() */
#include <dos.h> /* outport(), outportb(), inportb() */

#if defined(__TURBOC__)
/* nothing */
#elif defined(__WATCOMC__)
#if defined(__386__)
#error Compile with 16-bit compiler (WCC.EXE)
#endif
#include <conio.h>
#define	inportb(P)	inp(P)
#define	outportb(P,V)	outp(P,V)
#define	outport(P,V)	outpw(P,V)
#else
#error Unsupported compiler
#endif

#define	HUGE	huge

#if 0
#include <stdint.h> /* uint8_t, uint16_t, uint32_t */
#else
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
#endif

/* "class" for in-memory bitmap or framebuffer */
typedef struct
{
	unsigned wd, ht;
	unsigned long bytes_per_row;
	unsigned char HUGE *raster;
} img_t;

#define VGA_SEQ_INDEX	0x3C4
#define VGA_SEQ_DATA	0x3C5
#define VGA_GC_INDEX 	0x3CE
#define VGA_GC_DATA 	0x3CF
#define VGA_CRTC_INDEX	0x3D4
#define VGA_CRTC_DATA	0x3D5

/* video mode */
typedef struct
{
	unsigned mode_num, depth, flags;
	unsigned long bytes_per_row;
	unsigned wd, ht;
} mode_t;

#pragma pack(1)
typedef struct
{
	char sig[4];
	uint8_t ver_minor;
	uint8_t ver_major;
	char far *oem_name;
	uint32_t capabilities;	 /* b1=1 for non-VGA board */
	uint16_t far *mode_list;
	uint16_t vid_mem_size; /* in units of 64K */
	char reserved1[236];
	char reserved2[256];
} vbe_info_t;

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
	uint16_t bytes_per_row;
/* OEM modes and VBE 1.2 only: */
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
/* VBE 1.2 only */
	uint8_t red_width;
	uint8_t red_shift;
	uint8_t green_width;
	uint8_t green_shift;
	uint8_t blue_width;
	uint8_t blue_shift;
	char reserved3[3];
	uint32_t lfb_adr;
	char reserved4[212];
} vbe_mode_t;

img_t g_fb;

static mode_t *g_modes;
static unsigned g_num_modes;
static unsigned g_use_win_a, g_gran_per_64k;
/*****************************************************************************
Selects one of four video memory planes (for 16-color EGA/VGA/SVGA modes)
*****************************************************************************/
void set_plane(unsigned p)
{
	unsigned pmask;

/* use only bottom two bits of 'p' */
	p &= 3;
	pmask = 1 << p;
/* set read plane
	outportb(VGA_GC_INDEX, 4);
	outportb(VGA_GC_DATA, p); */
	outport(VGA_GC_INDEX, (p << 8) | 0x04);
/* set write plane
	outportb(VGA_SEQ_INDEX, 2);
	outportb(VGA_SEQ_DATA, pmask); */
	outport(VGA_SEQ_INDEX, (pmask << 8) | 0x02);
}
/*****************************************************************************
'Bank' is always 64K, though INT 10h AX=4F05h uses 'granules'
One of my video boards has 64K granules; the other has 4K granules.
*****************************************************************************/
void set_bank(unsigned b)
{
	static unsigned curr_bank = -1u;
/**/
	union REGS regs;

	if(b == curr_bank)
		return;
	curr_bank = b;
	regs.x.ax = 0x4F05;
/* g_use_win_a and g_gran_per_64k set by INT 10h AX=4F01h */
	regs.x.bx = g_use_win_a ? 0x0000 : 0x0001;
	regs.x.dx = b * g_gran_per_64k;
	int86(0x10, &regs, &regs);
}
/*****************************************************************************
*****************************************************************************/
int get_modes(void)
{
	static const mode_t vga_modes[] =
	{
		{
			0x04, 2, 0, 80, 320, 200
		}, {
/* modes 0x0D and up don't work on CGA */
			0x0D, 4, 1, 40, 320, 200
		}, {
/* modes 0x11 and up don't work on CGA or EGA */
			0x11, 1, 0, 80, 640, 480
		}, {
			0x12, 4, 1, 80, 640, 480
		}, {
			0x13, 8, 0, 320, 320, 200
		}
	};
/* big structures; too much stack for a 16-bit program. Make these static */
	static vbe_info_t vbe;
	static vbe_mode_t mode;
/**/
	struct SREGS sregs;
	union REGS regs;
	unsigned long mem;
	mode_t *new_modes;
	unsigned i;

/* start list of supported video modes with VGA modes */
	g_modes = (mode_t *)malloc(sizeof(vga_modes));
	if(g_modes == NULL)
MEM:	{
		printf("Out of memory\n");
		return -1;
	}
	g_num_modes = sizeof(vga_modes) / sizeof(vga_modes[0]);
	for(i = 0; i < g_num_modes; i++)
		g_modes[i] = vga_modes[i]; /* structure copy */
/* detect VESA video BIOS extensions (VBE) */
	sregs.es = FP_SEG(&vbe);
	regs.x.di = FP_OFF(&vbe);
	regs.x.ax = 0x4F00;
	strcpy(vbe.sig, "VBE2");
	int86x(0x10, &regs, &regs, &sregs);
	if(regs.x.ax != 0x004F)
	{
printf("No VESA BIOS extensions (VBE) in this PC\n");
		goto NO_VBE;
	}
printf("VBE version %u.%u, %uK video RAM, OEM name '%Fs'\n",
 vbe.ver_major, vbe.ver_minor,
 vbe.vid_mem_size * 64, vbe.oem_name);
/* want VBE 1.2 for extra mode info in mode_info_t */
	if(vbe.ver_major < 1 ||
		(vbe.ver_major == 1 && vbe.ver_minor < 2))
	{
printf("VBE 1.2+ or better required\n");
		goto NO_VBE;
	}
/* walk mode list */
	for(; *vbe.mode_list != 0xFFFF; vbe.mode_list++)
	{
/* get mode info */
		sregs.es = FP_SEG(&mode);
		regs.x.di = FP_OFF(&mode);
		regs.x.cx = *vbe.mode_list;
		regs.x.ax = 0x4F01;
		int86x(0x10, &regs, &regs, &sregs);
		if(regs.x.ax != 0x004F)
NOT:		{
printf("Unsupported video mode 0x%X (%ux%ux%u)\n",
 *vbe.mode_list, mode.wd, mode.ht, mode.depth);
			continue;
		}
/* sanity checking: mode must exist... */
		if((mode.mode_attrib & 0x0001) == 0)
			goto NOT;
/* ...ignore text modes */
		if((mode.mode_attrib & 0x0010) == 0)
		{
printf("Text -- ");
			goto NOT;
		}
/* ...must have enough video RAM for mode */
		mem = (unsigned long)mode.bytes_per_row * mode.ht;
		if(mem > vbe.vid_mem_size * 65536L)
		{
printf("Not enough video RAM (got %uK, need %luK) -- ",
 mem >> 10, vbe.vid_mem_size * 64);
			goto NOT;
		}
/* ...banked framebuffer must exist */
		if(mode.mode_attrib & 0x0040)
		{
printf("No banked framebuffer (\?\?\?) -- ");
			goto NOT;
		}
/* ...at least one of the two windows must exist
and be both readable and writable */
		if(mode.win_a_attrib != 0x0007 &&
			mode.win_b_attrib != 0x0007)
				goto NOT;
/* ...the only planar modes supported are 4-plane modes */
		if(mode.planes != 1)
		{
			if(mode.planes != 4 || mode.depth != 4)
			{
printf("%u planes -- ", mode.planes);
				goto NOT;
			}
/* ...planar modes that need >64K (e.g. 1024x768x4) not supported */
			if((unsigned long)mode.bytes_per_row *
				mode.ht >= 65536uL)
			{
printf("Hi-res planar -- ");
				goto NOT;
			}
		}
/* all checks passed -- add mode to global list of supported modes */
		new_modes = (mode_t *)realloc(g_modes,
			(g_num_modes + 1) * sizeof(mode_t));
		if(new_modes == NULL)
			goto MEM;
		g_modes = new_modes;
/* */
		g_modes[g_num_modes].mode_num = *vbe.mode_list;
		if(mode.planes == 4)
		{
			g_modes[g_num_modes].depth = 4;
			g_modes[g_num_modes].flags = 0x0001;
		}
		else
		{
			g_modes[g_num_modes].depth = mode.depth;
			g_modes[g_num_modes].flags = 0;
		}
		g_modes[g_num_modes].bytes_per_row = mode.bytes_per_row;
		g_modes[g_num_modes].wd = mode.wd;
		g_modes[g_num_modes].ht = mode.ht;
		g_num_modes++;
	}
NO_VBE:
/* dump list of supported graphics modes */
printf("Mode W    H    D  Flags\n");
printf("---- ---- ---- -- -----\n");
for(i = 0; i < g_num_modes; i++)
{
 printf("%3Xh ", g_modes[i].mode_num);
 printf("%4u ", g_modes[i].wd);
 printf("%4u ", g_modes[i].ht);
 printf("%2u ", g_modes[i].depth);
 printf("%4Xh\n", g_modes[i].flags);
}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int set_graphics_mode(unsigned wd, unsigned ht,
		unsigned depth, unsigned flags)
{
	static vbe_mode_t mode;
/**/
	struct SREGS sregs;
	union REGS regs;
	unsigned i;

/* special-case text mode (depth==0) */
	if(depth == 0)
	{
		regs.x.ax = 0x0003; /* 80x25 text */
		int86(0x10, &regs, &regs);
		return 0;
	}
/* one-time init */
	if(g_num_modes == 0)
	{
		if(get_modes())
			return -1;
	}
/* find supported mode with identical wd, ht, depth, and flags */
	for(i = 0; i < g_num_modes; i++)
	{
		if(g_modes[i].depth == depth && g_modes[i].flags == flags &&
			g_modes[i].wd == wd && g_modes[i].ht == ht)
				break;
	}
	if(i >= g_num_modes)
	{
		printf("Unsupported graphics mode: %ux%ux%u, flags=0x%X\n",
			wd, ht, depth, flags);
		return -1;
	}
	g_fb.wd = wd;
	g_fb.ht = ht;
	g_fb.bytes_per_row = g_modes[i].bytes_per_row;
/* set graphics mode */
	if(g_modes[i].mode_num < 0x100)
	{
		regs.x.ax = g_modes[i].mode_num;
		int86(0x10, &regs, &regs);
		if(depth == 2)
		{
/* to make CGA graphics work like other graphics modes... */
/* 1) turn off screwy CGA addressing */
			outportb(VGA_CRTC_INDEX, 0x17);
			outportb(VGA_CRTC_DATA, inportb(VGA_CRTC_DATA) | 1);
/* 2) turn off doublescan */
			outportb(VGA_CRTC_INDEX, 9);
			outportb(VGA_CRTC_DATA, inportb(VGA_CRTC_DATA) & ~0x80);
/* 3) move the framebuffer from B800:0000 to A000:0000 */
			outportb(VGA_GC_INDEX, 6);
			outportb(VGA_GC_DATA, inportb(VGA_GC_INDEX) & ~0x0C);
		}
		g_fb.raster = MK_FP(0xA000, 0);
	}
	else
	{
/* get mode info (again) to get framebuffer address */
		sregs.es = FP_SEG(&mode);
		regs.x.di = FP_OFF(&mode);
		regs.x.cx = g_modes[i].mode_num;
		regs.x.ax = 0x4F01;
		int86x(0x10, &regs, &regs, &sregs);
		if(regs.x.ax != 0x004F)
ERR:		{
printf("Unsupported video mode 0x%X (%ux%ux%u) (can't happen!)\n",
 g_modes[i].mode_num, mode.wd, mode.ht, mode.depth);
			return -1;
		}
		if(mode.win_a_attrib == 0x0007)
		{
			g_use_win_a = 1;
			g_fb.raster = MK_FP(mode.win_a_seg, 0);
		}
		else
		{
			g_use_win_a = 0;
			g_fb.raster = MK_FP(mode.win_b_seg, 0);
		}
		g_gran_per_64k = 64 / mode.k_per_gran;
/* set the mode! */
		regs.x.ax = 0x4F02;
		regs.x.bx = g_modes[i].mode_num;
		int86(0x10, &regs, &regs);
		if(regs.x.ax != 0x004F)
			goto ERR;
	}
	return 0;
}
/*****************************************************************************
DEMO ROUTINES
*****************************************************************************/
#include <conio.h>

/* IMPORTS */
void set_plane(unsigned p);
void set_bank(unsigned b);
int get_modes(void);
int set_graphics_mode(unsigned wd, unsigned ht,
		unsigned depth, unsigned flags);
/*****************************************************************************
*****************************************************************************/
static void draw_pixel_1(unsigned x, unsigned y, uint32_t c)
{
	unsigned char HUGE *ptr, mask, val;

	ptr = g_fb.raster + g_fb.bytes_per_row * y + x / 8;
	mask = 0x80 >> (x & 7);
	val = (c & 0x01) * 0xFF;
	*ptr = (*ptr & ~mask) | (val & mask);
}
/*****************************************************************************
*****************************************************************************/
static void draw_pixel_2(unsigned x, unsigned y, uint32_t c)
{
	unsigned char HUGE *ptr, mask, val;

	ptr = g_fb.raster + g_fb.bytes_per_row * y + x / 4;
	mask = 0xC0 >> ((x & 3) * 2);
	val = (c & 0x03) * 0x55;
	*ptr = (*ptr & ~mask) | (val & mask);
}
/*****************************************************************************
*****************************************************************************/
static void draw_pixel_4p(unsigned x, unsigned y, uint32_t c)
{
	unsigned char HUGE *ptr, mask, pmask, p;

	ptr = g_fb.raster + g_fb.bytes_per_row * y + x / 8;
	mask = 0x80 >> (x & 7);
	pmask = 0x01;
	for(p = 0; p < 4; p++)
	{
		set_plane(p);
		if(pmask & c)
			*ptr |= mask;
		else
			*ptr &= ~mask;
		pmask <<= 1;
	}
}
/*****************************************************************************
*****************************************************************************/
static void draw_pixel_8(unsigned x, unsigned y, uint32_t c)
{
	g_fb.raster[g_fb.bytes_per_row * y + x] = c;
}
/*****************************************************************************
*****************************************************************************/
static void draw_pixel_8b(unsigned x, unsigned y, uint32_t c)
{
	unsigned long offset;

	offset = g_fb.bytes_per_row * y + x;
	set_bank((unsigned)(offset >> 16));
	g_fb.raster[offset & 0xFFFF] = c;
}
/*****************************************************************************
*****************************************************************************/
static void draw_pixel_16b(unsigned x, unsigned y, uint32_t c)
{
	unsigned long offset;

	offset = g_fb.bytes_per_row * y + x * 2;
	set_bank((unsigned)(offset >> 16));
	*(uint16_t HUGE *)(g_fb.raster + (offset & 0xFFFF)) = c;
}
/*****************************************************************************
*****************************************************************************/
static void draw_pixel_24b(unsigned x, unsigned y, uint32_t c)
{
	unsigned long offset;
	unsigned char HUGE *ptr;

	offset = g_fb.bytes_per_row * y + x * 4;
	set_bank((unsigned)(offset >> 16));
	ptr = g_fb.raster + (offset & 0xFFFF);
	ptr[0] = c;
	ptr[1] = c >> 8;
	ptr[2] = c >> 16;
}
/*****************************************************************************
*****************************************************************************/
static void draw_pixel_32b(unsigned x, unsigned y, uint32_t c)
{
	unsigned long offset;

	offset = g_fb.bytes_per_row * y + x * 4;
	set_bank((unsigned)(offset >> 16));
	*(uint32_t HUGE *)(g_fb.raster + (offset & 0xFFFF)) = c;
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	void (*draw_pixel)(unsigned x, unsigned y, uint32_t c);
	unsigned i;

	if(get_modes())
		return 1;
/* 1-bit (monochrome) */
	if(set_graphics_mode(640, 480, 1, 0) == 0)
	{
/* draw an 'X' */
		draw_pixel = draw_pixel_1;
		for(i = 0; i < g_fb.ht; i++)
		{
			draw_pixel((g_fb.wd - g_fb.ht) / 2 + i, i, 1);
			draw_pixel((g_fb.wd + g_fb.ht) / 2 - i, i, 1);
		}
		getch();
	}
/* 2-bit (4-color) */
	if(set_graphics_mode(320, 200, 2, 0) == 0)
	{
		draw_pixel = draw_pixel_2;
		for(i = 0; i < g_fb.ht; i++)
		{
			draw_pixel((g_fb.wd - g_fb.ht) / 2 + i, i, 1);
			draw_pixel((g_fb.wd + g_fb.ht) / 2 - i, i, 2);
		}
		getch();
	}
/* 4-plane (16-color) */
	if(set_graphics_mode(640, 480, 4, 0x0001) == 0)
	{
		draw_pixel = draw_pixel_4p;
		for(i = 0; i < g_fb.ht; i++)
		{
			draw_pixel((g_fb.wd - g_fb.ht) / 2 + i, i, 1);
			draw_pixel((g_fb.wd + g_fb.ht) / 2 - i, i, 2);
		}
		getch();
	}
/* 8-bit (256-color) -- linear framebuffer */
	if(set_graphics_mode(320, 200, 8, 0) == 0)
	{
		draw_pixel = draw_pixel_8;
		for(i = 0; i < g_fb.ht; i++)
		{
			draw_pixel((g_fb.wd - g_fb.ht) / 2 + i, i, 3);
			draw_pixel((g_fb.wd + g_fb.ht) / 2 - i, i, 4);
		}
		getch();
	}
/* 8-bit (256-color) -- banked framebuffer */
	if(set_graphics_mode(640, 480, 8, 0) == 0)
	{
		draw_pixel = draw_pixel_8b;
		for(i = 0; i < g_fb.ht; i++)
		{
			draw_pixel((g_fb.wd - g_fb.ht) / 2 + i, i, 5);
			draw_pixel((g_fb.wd + g_fb.ht) / 2 - i, i, 6);
		}
		getch();
	}
/* 16-bit banked framebuffer */
	if(set_graphics_mode(640, 480, 16, 0) == 0)
	{
		draw_pixel = draw_pixel_16b;
		for(i = 0; i < g_fb.ht; i++)
		{
			draw_pixel((g_fb.wd - g_fb.ht) / 2 + i, i,
				(0x15 << 10) | (0x15 << 5) | 0x15); /* 7 */
			draw_pixel((g_fb.wd + g_fb.ht) / 2 - i, i,
				(0x0A << 10) | (0x0A << 5) | 0x1F); /* 9 */
		}
		getch();
	}
/* 24-bit banked framebuffer */
	if(set_graphics_mode(640, 480, 24, 0) == 0)
	{
		draw_pixel = draw_pixel_24b;
		for(i = 0; i < g_fb.ht; i++)
		{
			draw_pixel((g_fb.wd - g_fb.ht) / 2 + i, i,
				0x0055FF55L);			/* 10 */
			draw_pixel((g_fb.wd + g_fb.ht) / 2 - i, i,
				0x0055FFFFL);			/* 11 */
		}
		getch();
	}
/* 32-bit banked framebuffer */
	if(set_graphics_mode(640, 400, 32, 0) == 0)
	{
		draw_pixel = draw_pixel_32b;
		for(i = 0; i < g_fb.ht; i++)
		{
			draw_pixel((g_fb.wd - g_fb.ht) / 2 + i, i,
				0x00FF5555L);			/* 12 */
			draw_pixel((g_fb.wd + g_fb.ht) / 2 - i, i,
				0x00FF55FFL);			/* 13 */
		}
		getch();
	}

	(void)set_graphics_mode(0, 0, 0, 0);
	return 0;
}
