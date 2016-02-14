/*----------------------------------------------------------------------------
VBE video mode information utility
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
This code is public domain (no copyright).
You can do whatever you want with it.
Release date: January 31, 2008

Compile with DJGPP or 32-bit Watcom C.
----------------------------------------------------------------------------*/
/* union REGS, outportb(), int386() */
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
#include <conio.h> /* getch(). For Watcom C: outp() */

#if defined(__WATCOMC__)
#if !defined(__386__)
#error 32-bit program -- compile with WCC386.EXE
#endif
#define	outportb(P,V)	outp(P,V)
#endif

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
#define	HUGE	/* nothing */
static unsigned char HUGE *g_raster;
static unsigned long g_bytes_per_row;
/* for banked framebuffer: */
static unsigned g_use_win_a, g_gran_per_64k;
/* for demo code: */
static void (*g_write_pixel)(unsigned x, unsigned y, color_t c);

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
	regs.w.ax = 0x4F05;
/* g_use_win_a and g_gran_per_64k were set by INT 10h AX=4F01h */
	regs.w.bx = g_use_win_a ? 0x0000 : 0x0001;
	regs.w.dx = b * g_gran_per_64k;
	int386(0x10, &regs, &regs);
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
this routine can be used for VGA mode 13h
since the framebuffer is smaller than 64K in that mode
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
*****************************************************************************/
static void draw_x(unsigned mode_num)
{
	union REGS regs;
	unsigned i;

/* switch to VBE graphics mode */
	regs.w.ax = 0x4F02;
	regs.w.bx = mode_num;
	int386(0x10, &regs, &regs);
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
	regs.w.ax = 0x0003;
	int386(0x10, &regs, &regs);
}
/*----------------------------------------------------------------------------
VBE CODE
----------------------------------------------------------------------------*/
#include <string.h> /* strcpy */
#include <stdio.h> /* printf(), sscanf() */

/* structure used by INT 10h AX=4F00h */
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
} mode_info_t;

#if defined(__DJGPP__)
/* __djgpp_conventional_base, __djgpp_nearptr_enable() */
#include <sys/nearptr.h>
#include <crt0.h> /* _CRT0_FLAG_NEARPTR, _crt0_startup_flags */
#include <dpmi.h> /* __dpmi_... */

#elif defined(__WATCOMC__)
#include <dos.h> /* union REGS, struct SREGS, int386(), int386x() */

/* These work with CauseWay DOS extender only: */
#define	dosmemget(off,n,buf)		memcpy(buf, (void *)(off), n)
#define	dosmemput(buf,n,off)		memcpy((void *)(off), buf, n)
#define	__djgpp_conventional_base	0

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

typedef struct
{
	unsigned long size, address;
} __dpmi_meminfo;
/*****************************************************************************
*****************************************************************************/
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
/*****************************************************************************
*****************************************************************************/
int __dpmi_physical_address_mapping(__dpmi_meminfo *info)
{
	union REGS regs;

	memset(&regs, 0, sizeof(regs));
	regs.w.ax = 0x0800;
	regs.w.bx = info->address >> 16;
	regs.w.cx = info->address;
	regs.w.si = info->size >> 16;
	regs.w.di = info->size;
	int386(0x31, &regs, &regs);
	info->address = ((unsigned long)regs.w.bx << 16) | regs.w.cx;
	return regs.w.cflag ? -1 : 0;
}
#endif
/*****************************************************************************
*****************************************************************************/
static int rm_int_es_di(void *buf, unsigned buf_size,
		__dpmi_regs *regs, unsigned int_num)
{
	int seg, sel;

/* allocate conventional memory */
	seg = __dpmi_allocate_dos_memory((buf_size + 15) / 16, &sel);
	if(seg == -1)
	{
		printf("Error: can't allocate conventional memory\n");
		return -1;
	}
/* copy buffer to conventional memory */
	dosmemput(buf, buf_size, seg * 16);
/* point real-mode ES and DI to buffer and do interrupt */
	regs->x.es = seg;
	regs->x.di = 0;
	__dpmi_int(int_num, regs);
/* copy conventional memory back to buffer */
	dosmemget(seg * 16, buf_size, buf);
/* free conventional memory buffer */
	__dpmi_free_dos_memory(sel);
	return 0;
}
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
/* linear framebuffer */
	if(info->mode_attrib & 0x80)
	{
		__dpmi_meminfo mem_info;

/* ...map it into the virtual address space */
		mem_info.size = info->ht * (unsigned long)info->bytes_per_row;
		mem_info.address = info->lfb_adr;
		if(__dpmi_physical_address_mapping(&mem_info) == -1)
		{
			printf("Error mapping linear framebuffer "
				"for video mode 0x%X\n", mode_num);
			return -1;
		}
		g_raster = (unsigned char HUGE *)(mem_info.address +
			__djgpp_conventional_base);
		switch(info->depth)
		{
		case 4:
			g_write_pixel = write_pixel_4p;
			g_color_model = 0;
			break;
		case 8:
			g_write_pixel = write_pixel_8;
			g_color_model = 0;
			break;
		case 15:
			g_write_pixel = write_pixel_16;
			g_color_model = 1;
			break;
		case 16:
			g_write_pixel = write_pixel_16;
			g_color_model = 2;
			break;
		case 24:
			g_write_pixel = write_pixel_24;
			g_color_model = 3;
			break;
		case 32:
			g_write_pixel = write_pixel_32;
			g_color_model = 3;
			break;
		default:
			printf("This software does not support %u-"
				"bit graphics\n", info->depth);
			return -1;
		}
	}
/* banked framebuffer */
	else
	{
		if((info->win_a_attrib & 7) == 7)
		{
			g_raster = (unsigned char HUGE *)
				(info->win_a_seg * 16uL +
				__djgpp_conventional_base);
			g_use_win_a = 1;
		}
		else if((info->win_b_attrib & 7) == 7)
		{
			g_raster = (unsigned char HUGE *)
				(info->win_b_seg * 16uL +
				__djgpp_conventional_base);
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
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	uint16_t mode_list[128];
	mode_info_t mode_info;
	vbe_info_t vbe_info;
	unsigned long linear;
	unsigned rmode, i;
	__dpmi_regs regs;

#if defined(__DJGPP__)
/* turn off data segment limit, for nearptr access */
	if(!(_crt0_startup_flags & _CRT0_FLAG_NEARPTR))
	{
		if(!__djgpp_nearptr_enable())
		{
			printf("Error: can not enable near pointer "
				"access (WinNT/2k/XP?)\n");
			return 1;
		}
	}
#endif
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
memset(&regs, 0, sizeof(regs)); /* stupid Watcom C... */
	regs.x.ax = 0x4F00;
	if(rm_int_es_di(&vbe_info, sizeof(vbe_info), &regs, 0x10))
		return 1;
	if(regs.x.ax != 0x004F)
	{
		printf("INT 10h AX=4F00h failed (no VBE?)\n");
		return 1;
	}
/* display VBE info */
	printf("VBE version %u.%u, %uK video memory\n",
		vbe_info.ver_major, vbe_info.ver_minor,
		vbe_info.vid_mem_size * 64);
/* OEM name */
	{
		unsigned char buf[256];

		linear = vbe_info.oem_name_off +
			vbe_info.oem_name_seg * 16uL;
		dosmemget(linear, sizeof(buf), buf);
		printf("OEM name: %s\n", buf);
	}
	printf("Video board is");
	if(vbe_info.capabilities & 0x02)
		printf(" NOT");
	printf(" register-compatible with VGA\n");
/* list of supported mode numbers */
	linear = vbe_info.mode_list_off + vbe_info.mode_list_seg * 16uL;
	dosmemget(linear, sizeof(mode_list), mode_list);
/* walk mode list */
	printf(	"                             Banked   Linear\n"
		"Mode                    Gran frame-   frame-\n"
		"num-                    -ule buffer   buffer\n"
		"ber  Width Height Depth size address  address\n"
		"---- ----- ------ ----- ---- -------- --------\n");
	for(i = 0; i < sizeof(mode_list) / sizeof(mode_list[0]); i++)
	{
		unsigned mode_num;
		unsigned long mem;

		mode_num = mode_list[i];
		if(mode_num == 0xFFFF)
			break;
/* try getting info for LFB mode */
		mode_num |= 0x4000;
		regs.x.cx = mode_num;
		regs.x.ax = 0x4F01;
		if(rm_int_es_di(&mode_info, sizeof(mode_info), &regs, 0x10))
			continue;
/* that didn't work; try banked FB */
		if(regs.x.ax != 0x004F)
		{
			mode_num &= ~0x4000;
			regs.x.cx = mode_num;
			regs.x.ax = 0x4F01;
			if(rm_int_es_di(&mode_info, sizeof(mode_info),
				&regs, 0x10))
					continue;
			if(regs.x.ax != 0x004F)
			{
				printf("INT 10h AX=4F01h CX=%04Xh failed\n",
					mode_num);
				continue;
			}
		}
/* mode not supported (probably because there's not enough video memory) */
		if((mode_info.mode_attrib & 0x01) == 0)
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
