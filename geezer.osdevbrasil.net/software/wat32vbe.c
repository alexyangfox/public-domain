/*****************************************************************************
DOS VGA/VBE graphics demo for 32-bit Watcom C; using CauseWay DOS extender
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
#include <string.h> /* strcpy() */
#include <stdlib.h> /* malloc(), realloc() */
#include <conio.h> /* outportw(), outportb(), inportb() */
#include <stdio.h> /* printf() */
#include <dos.h>

#if defined(__WATCOMC__)
#if !defined(__386__)
#error Compile with 32-bit compiler (WCC386.EXE)
#endif
#else
#error Compile this file with 32-bit Watcom C
#endif

#define	HUGE	/* nothing */

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
/*	char far *oem_name; */
	uint16_t oem_name_off, oem_name_seg;

	uint32_t capabilities;	 /* b1=1 for non-VGA board */
/*	uint16_t far *mode_list; */
	uint16_t mode_list_off, mode_list_seg;

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

#pragma pack(1)
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

typedef struct
{
	unsigned long size, address;
} __dpmi_meminfo;

img_t g_fb;

mode_t *g_modes;
unsigned g_num_modes;
unsigned g_use_win_a, g_gran_per_64k;
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
	outp(VGA_GC_INDEX, 4);
	outp(VGA_GC_DATA, p); */
	outpw(VGA_GC_INDEX, (p << 8) | 0x04);
/* set write plane
	outp(VGA_SEQ_INDEX, 2);
	outp(VGA_SEQ_DATA, pmask); */
	outpw(VGA_SEQ_INDEX, (pmask << 8) | 0x02);
}
/*****************************************************************************
'Bank' is always 64K, though INT 10h AX=4F05h uses 'granules'
One of my video boards has 64K granules; the other has 4K granules.
*****************************************************************************/
void set_bank(unsigned b)
{
        unsigned curr_bank = -1u;
/**/
	union REGS regs;

	if(b == curr_bank)
		return;
	curr_bank = b;
	regs.w.ax = 0x4F05;
/* g_use_win_a and g_gran_per_64k set by INT 10h AX=4F01h */
	regs.w.bx = g_use_win_a ? 0x0000 : 0x0001;
	regs.w.dx = b * g_gran_per_64k;
	int386(0x10, &regs, &regs);
}
/*****************************************************************************
*****************************************************************************/
int __dpmi_physical_address_mapping(__dpmi_meminfo *info)
{
	union REGS regs;

	regs.w.ax = 0x0800;
	regs.w.bx = info->address >> 16;
	regs.w.cx = info->address;
	regs.w.si = info->size >> 16;
	regs.w.di = info->size;
	int386(0x31, &regs, &regs);
	info->address = ((unsigned long)regs.w.bx << 16) | regs.w.cx;
	return regs.w.cflag ? -1 : 0;
}
/*****************************************************************************
*****************************************************************************/
int __dpmi_allocate_dos_memory(unsigned paragraphs, int *sel)
{
	union REGS regs;

	regs.w.ax = 0x0100;
	regs.w.bx = paragraphs;
	int386(0x31, &regs, &regs);
	*sel = regs.w.dx;
	return regs.w.cflag ? -1 : regs.w.ax;
}
/*****************************************************************************
*****************************************************************************/
int __dpmi_simulate_real_mode_interrupt(
		unsigned int_num, __dpmi_regs *rm_regs)
{
	struct SREGS sregs;
	union REGS regs;

	memset(&sregs, 0, sizeof(sregs));
	memset(&regs, 0, sizeof(regs));
	sregs.es = FP_SEG(rm_regs);
	regs.x.edi = FP_OFF(rm_regs);
	regs.x.eax = 0x0300;
	regs.x.ebx = int_num;	/* AH=flags=0 */
	regs.x.ecx = 0;		/* number of words to copy between stacks */
	int386x(0x31, &regs, &regs, &sregs);
	return regs.w.cflag ? -1 : 0;
}
/*****************************************************************************
*****************************************************************************/
#define	NEARPTR(S,O)	(char *)((S) * 16uL + (O))

int g_conv_mem_seg, g_conv_mem_sel;

int get_modes(void)
{
        const mode_t vga_modes[] =
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
/**/
	vbe_info_t *vbe;
	vbe_mode_t *mode;
	__dpmi_regs regs;
	unsigned long mem;
	mode_t *new_modes;
	unsigned i, m;

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
/* allocate buffers in conventional memory */
	i = sizeof(vbe_info_t) + sizeof(vbe_mode_t);
	g_conv_mem_seg = __dpmi_allocate_dos_memory(
		(i + 15) / 16, &g_conv_mem_sel);
	if(g_conv_mem_seg == -1)
	{
		printf("Can't allocate conventional memory\n");
		return -1;
	}
	mode = (vbe_mode_t *)NEARPTR(g_conv_mem_seg, 0);
	vbe = (vbe_info_t *)NEARPTR(g_conv_mem_seg, sizeof(vbe_mode_t));
/* detect VESA video BIOS extensions (VBE) */
	memset(&regs, 0, sizeof(regs)); /* YES; you need this! */
	regs.x.ax = 0x4F00;
	strcpy(vbe->sig, "VBE2");
/* ES:DI=real-mode address of 'vbe' */
	regs.x.es = g_conv_mem_seg;
	regs.x.di = sizeof(vbe_mode_t);
	__dpmi_simulate_real_mode_interrupt(0x10, &regs);
	if(regs.x.ax != 0x004F)
	{
printf("No VESA BIOS extensions (VBE) in this PC\n");
		goto NO_VBE;
	}
printf("VBE version %u.%u, %uK video RAM, OEM name '%s'\n",
 vbe->ver_major, vbe->ver_minor, vbe->vid_mem_size * 64,
 NEARPTR(vbe->oem_name_seg, vbe->oem_name_off));
/* want VBE 1.2 for extra mode info in mode_info_t */
	if(vbe->ver_major < 1 ||
		(vbe->ver_major == 1 && vbe->ver_minor < 2))
	{
printf("VBE 1.2+ or better required\n");
		goto NO_VBE;
	}
/* walk mode list */
	while(1)
	{
		m = *(uint16_t *)NEARPTR(
			vbe->mode_list_seg, vbe->mode_list_off);
		if(m == 0xFFFF)
			break;
		vbe->mode_list_off += 2;
/* get mode info */
		memset(&regs, 0, sizeof(regs));
		regs.x.cx = m;
		regs.x.ax = 0x4F01;
/* ES:DI=real-mode address of 'mode' */
		regs.x.es = g_conv_mem_seg;
		regs.x.di = 0;
		__dpmi_simulate_real_mode_interrupt(0x10, &regs);
		if(regs.x.ax != 0x004F)
NOT:		{
printf("Unsupported video mode 0x%X (%ux%ux%u)\n",
 m, mode->wd, mode->ht, mode->depth);
			continue;
		}
/* sanity checking: mode must exist... */
		if((mode->mode_attrib & 0x0001) == 0)
			goto NOT;
/* ...ignore text modes */
		if((mode->mode_attrib & 0x0010) == 0)
		{
printf("Text -- ");
			goto NOT;
		}
/* ...must have enough video RAM for mode */
		mem = (unsigned long)mode->bytes_per_row * mode->ht;
		if(mem > vbe->vid_mem_size * 65536L)
		{
printf("Not enough video RAM (got %luK, need %uK) -- ",
 mem >> 10, vbe->vid_mem_size * 64);
			goto NOT;
		}
/* ...either linear framebuffer (LFB) must exist... */
		if(mode->mode_attrib & 0x0080)
			/* nothing */;
/* ...or banked framebuffer must exist */
		else
		{
			if(mode->mode_attrib & 0x0040)
			{
printf("No banked framebuffer (\?\?\?) -- ");
				goto NOT;
			}
/* ...at least one of the two windows must exist,
be readable, and be writable */
			if(mode->win_a_attrib != 0x0007 &&
				mode->win_b_attrib != 0x0007)
					goto NOT;
		}
/* ...the only planar modes supported are 4-plane modes */
		if(mode->planes != 1)
		{
			if(mode->planes != 4 || mode->depth != 4)
			{
printf("%u planes -- ", mode->planes);
				goto NOT;
			}
/* ...planar modes that need >64K (e.g. 1024x768x4) not supported */
			if((unsigned long)mode->bytes_per_row *
				mode->ht >= 65536uL)
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
		g_modes[g_num_modes].mode_num = m;
		if(mode->planes == 4)
		{
			g_modes[g_num_modes].depth = 4;
			g_modes[g_num_modes].flags = 0x0001;
		}
		else
		{
			g_modes[g_num_modes].depth = mode->depth;
			g_modes[g_num_modes].flags = 0;
		}
		g_modes[g_num_modes].bytes_per_row = mode->bytes_per_row;
		g_modes[g_num_modes].wd = mode->wd;
		g_modes[g_num_modes].ht = mode->ht;
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
char g_lfb; /* xxx - hack */

int set_graphics_mode(unsigned wd, unsigned ht,
		unsigned depth, unsigned flags)
{
	__dpmi_regs dpmi_regs;
	union REGS regs;
	vbe_mode_t *mode;
	unsigned i, m;

/* special-case text mode (depth==0) */
	if(depth == 0)
	{
		regs.w.ax = 0x0003; /* 80x25 text */
		int386(0x10, &regs, &regs);
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
	m = g_modes[i].mode_num;
/* SET VGA GRAPHICS MODE */
	if(m < 0x100)
	{
		regs.w.ax = m;
		int386(0x10, &regs, &regs);
		if(depth == 2)
		{
/* to make CGA graphics work like other graphics modes... */
/* 1) turn off screwy CGA addressing */
			outp(VGA_CRTC_INDEX, 0x17);
			outp(VGA_CRTC_DATA, inp(VGA_CRTC_DATA) | 1);
/* 2) turn off doublescan */
			outp(VGA_CRTC_INDEX, 9);
			outp(VGA_CRTC_DATA, inp(VGA_CRTC_DATA) & ~0x80);
/* 3) move the framebuffer from B800:0000 to A000:0000 */
			outp(VGA_GC_INDEX, 6);
			outp(VGA_GC_DATA, inp(VGA_GC_INDEX) & ~0x0C);
		}
		g_fb.raster = NEARPTR(0xA000, 0);
		g_lfb = 0;
	}
/* (RE)GET VBE MODE INFO */
	else
	{
		mode = (vbe_mode_t *)NEARPTR(g_conv_mem_seg, 0);
		memset(&dpmi_regs, 0, sizeof(dpmi_regs));
		dpmi_regs.x.cx = m;
		dpmi_regs.x.ax = 0x4F01;
/* ES:DI=real-mode address of 'mode' */
		dpmi_regs.x.es = g_conv_mem_seg;
		dpmi_regs.x.di = 0;
		__dpmi_simulate_real_mode_interrupt(0x10, &dpmi_regs);
		if(dpmi_regs.x.ax != 0x004F)
ERR:		{
printf("Unsupported video mode 0x%X (%ux%ux%u) (can't happen!)\n",
 m, mode->wd, mode->ht, mode->depth);
			return -1;
		}
/* SET VBE GRAPHICS MODE USING LINEAR FRAMEBUFFER (LFB) */
		if(mode->mode_attrib & 0x0080)
		{
			__dpmi_meminfo mem_info;

			mem_info.size = mode->bytes_per_row * mode->ht;
			mem_info.address = mode->lfb_adr;
			if(__dpmi_physical_address_mapping(&mem_info) == -1)
			{
				printf("Could not map linear framebuffer\n");
				return -1;
			}
			g_fb.raster = (unsigned char HUGE *)mem_info.address;
/* enable LFB when setting mode */
			m |= 0x4000;
			g_lfb = 1;
		}
/* SET VBE GRAPHICS MODE USING BANKED FRAMEBUFFER */
		else
		{
			if(mode->mode_attrib & 0x0040)
				goto ERR;
			if(mode->win_a_attrib == 0x0007)
			{
				g_use_win_a = 1;
				g_fb.raster = NEARPTR(mode->win_a_seg, 0);
			}
			else
			{
				g_use_win_a = 0;
				g_fb.raster = NEARPTR(mode->win_b_seg, 0);
			}
			g_gran_per_64k = 64 / mode->k_per_gran;
			g_lfb = 0;
		}
/* set the mode! */
		regs.w.ax = 0x4F02;
		regs.w.bx = m;
		int386(0x10, &regs, &regs);
		if(regs.w.ax != 0x004F)
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
void draw_pixel_1(unsigned x, unsigned y, uint32_t c)
{
	unsigned char HUGE *ptr, mask, val;

	ptr = g_fb.raster + g_fb.bytes_per_row * y + x / 8;
	mask = 0x80 >> (x & 7);
	val = (c & 0x01) * 0xFF;
	*ptr = (*ptr & ~mask) | (val & mask);
}
/*****************************************************************************
*****************************************************************************/
void draw_pixel_2(unsigned x, unsigned y, uint32_t c)
{
	unsigned char HUGE *ptr, mask, val;

	ptr = g_fb.raster + g_fb.bytes_per_row * y + x / 4;
	mask = 0xC0 >> ((x & 3) * 2);
	val = (c & 0x03) * 0x55;
	*ptr = (*ptr & ~mask) | (val & mask);
}
/*****************************************************************************
*****************************************************************************/
void draw_pixel_4p(unsigned x, unsigned y, uint32_t c)
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
void draw_pixel_8(unsigned x, unsigned y, uint32_t c)
{
	g_fb.raster[g_fb.bytes_per_row * y + x] = c;
}
/*****************************************************************************
*****************************************************************************/
void draw_pixel_8b(unsigned x, unsigned y, uint32_t c)
{
	unsigned long offset;

	offset = g_fb.bytes_per_row * y + x;
	set_bank((unsigned)(offset >> 16));
	g_fb.raster[offset & 0xFFFF] = c;
}
/*****************************************************************************
*****************************************************************************/
void draw_pixel_16(unsigned x, unsigned y, uint32_t c)
{
	*(uint16_t HUGE *)(g_fb.raster + g_fb.bytes_per_row * y + x * 2) = c;
}
/*****************************************************************************
*****************************************************************************/
void draw_pixel_16b(unsigned x, unsigned y, uint32_t c)
{
	unsigned long offset;

	offset = g_fb.bytes_per_row * y + x * 2;
	set_bank((unsigned)(offset >> 16));
	*(uint16_t HUGE *)(g_fb.raster + (offset & 0xFFFF)) = c;
}
/*****************************************************************************
*****************************************************************************/
void draw_pixel_24(unsigned x, unsigned y, uint32_t c)
{
	unsigned char HUGE *ptr;

	ptr = g_fb.raster + g_fb.bytes_per_row * y + x * 3;
	ptr[0] = c;
	ptr[1] = c >> 8;
	ptr[2] = c >> 16;
}
/*****************************************************************************
*****************************************************************************/
void draw_pixel_24b(unsigned x, unsigned y, uint32_t c)
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
void draw_pixel_32(unsigned x, unsigned y, uint32_t c)
{
	*(uint32_t HUGE *)(g_fb.raster + g_fb.bytes_per_row * y + x * 4) = c;
}
/*****************************************************************************
*****************************************************************************/
void draw_pixel_32b(unsigned x, unsigned y, uint32_t c)
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
/* 8-bit (256-color) -- banked OR linear framebuffer */
	if(set_graphics_mode(640, 480, 8, 0) == 0)
	{
		draw_pixel = g_lfb ? draw_pixel_8 : draw_pixel_8b;
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
		draw_pixel = g_lfb ? draw_pixel_16 : draw_pixel_16b;
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
		draw_pixel = g_lfb ? draw_pixel_24 : draw_pixel_24b;
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
		draw_pixel = g_lfb ? draw_pixel_32 : draw_pixel_32b;
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
