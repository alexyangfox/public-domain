/*----------------------------------------------------------------------------
TEXT VIDEO ROUTINES

EXPORTS:
extern console_t *g_curr_vc;

void blink(void);
void select_vc(unsigned which_con);
void putch_help(console_t *con, unsigned c);
void putch(unsigned c);
console_t *create_vc(void);
void destroy_vc(console_t *con);
void init_video(void);
----------------------------------------------------------------------------*/
#include <string.h> /* NULL, memcpy(), memsetw() */
#include <stdint.h> /* uint8_t, uint16_t, uintptr_t */
#include <system.h> /* outportb(), inportb() */
#include <ctype.h> /* isdigit() */
#include "_krnl.h" /* KBD_BUF_SIZE, console_t, panic(), kprintf() */

/* IMPORTS
from MM.C */
void *kmalloc(unsigned size);
void *kcalloc(unsigned size);
void kfree(void *blk);

#define	NUM_VCS		12	/* 12 'F' keys on keyboard (F1-F12), so... */
#define	CONSOLE_MAGIC	0x37A1
#define	VGA_MISC_READ	0x3CC

static unsigned char g_vc0_buf[KBD_BUF_SIZE];
static console_t g_vc0 =
{
	CONSOLE_MAGIC,		/* .magic */
	{
		g_vc0_buf,	/* .keystrokes.data */
		KBD_BUF_SIZE	/* .keystrokes.size */
/* no need to initialize .keystrokes.in_ptr, or .keystrokes.out_ptr */
	},
	0,			/* .fb_adr -- see init_video() */
	0x01			/* .attrib (blue on black) */
/* no need to initialize .esc, .csr_x, .csr_y, .esc1, .esc2, .esc3,
.save_x, .save_y, or .wait_queue */
};
/* set g_curr_vc = VC #0 */
console_t *g_curr_vc = &g_vc0;
/* you can have as many VCs as you want,
but only NUM_VCS VCs can be displayed */
static console_t *g_vcs[NUM_VCS] =
{
	&g_vc0
};
static unsigned g_crtc_io_adr;
static uint16_t *g_vga_fb_adr;
static unsigned char g_vc_width, g_vc_height;
/*****************************************************************************
*****************************************************************************/
void blink(void)
{
	(*(uint8_t *)g_vga_fb_adr)++;
}
/*****************************************************************************
*****************************************************************************/
static void scroll(console_t *con)
{
	unsigned blank, i;
	uint16_t *fb_adr;

	if(con->magic != CONSOLE_MAGIC)
		panic("scroll: bad VC 0x%p", con);
	blank = 0x20 | ((unsigned)con->attrib << 8);
	if(con == g_curr_vc)
		fb_adr = g_vga_fb_adr;
	else
		fb_adr = con->fb_adr;
/* scroll up */
	if(con->csr_y >= g_vc_height)
	{
		i = con->csr_y - g_vc_height + 1;
		memcpy(fb_adr, fb_adr + i * g_vc_width,
			(g_vc_height - i) * g_vc_width * 2);
/* erase the bottom line of the screen */
		memsetw(fb_adr + (g_vc_height - i) * g_vc_width,
			blank, g_vc_width);
		con->csr_y = g_vc_height - i;
	}
}
/*****************************************************************************
*****************************************************************************/
static void erase(console_t *con, unsigned count)
{
	uint16_t *where;
	unsigned blank;

	if(con->magic != CONSOLE_MAGIC)
		panic("erase: bad VC 0x%p", con);
	if(con == g_curr_vc)
		where = g_vga_fb_adr;
	else
		where = con->fb_adr;
	where += (con->csr_y * g_vc_width + con->csr_x);
	blank = 0x20 | ((unsigned)con->attrib << 8);
	memsetw(where, blank, count);
}
/*****************************************************************************
*****************************************************************************/
static void set_attrib(console_t *con, unsigned att)
{
	static const unsigned ansi_to_vga[] =
	{
		0, 4, 2, 6, 1, 5, 3, 7
	};
/**/
	unsigned new_att;

	if(con->magic != CONSOLE_MAGIC)
		panic("set_attrib: bad VC 0x%p", con);
	new_att = con->attrib;
/* "All attributes off" */
	if(att == 0)
		new_att = 7;
/* bold */
	else if(att == 1)
		new_att |= 0x08;
/* reverse video */
	else if(att == 7)
		new_att = (new_att & 0x88) | ((new_att & 0x07) << 4) |
			((new_att & 0x70) >> 4);
/* set foreground color */
	else if(att >= 30 && att <= 37)
	{
		att = ansi_to_vga[att - 30];
		new_att = (new_att & ~0x07) | att;
	}
/* set background color */
	else if(att >= 40 && att <= 47)
	{
		att = ansi_to_vga[att - 40] << 4;
		new_att = (new_att & ~0x70) | att;
	}
	con->attrib = new_att;
}
/*****************************************************************************
*****************************************************************************/
static void move_csr(void)
{
	unsigned i;

	i = g_curr_vc->csr_y * g_vc_width + g_curr_vc->csr_x;
	outportb(g_crtc_io_adr + 0, 14);
	outportb(g_crtc_io_adr + 1, i >> 8);
	outportb(g_crtc_io_adr + 0, 15);
	outportb(g_crtc_io_adr + 1, i);
}
/*****************************************************************************
*****************************************************************************/
void select_vc(unsigned which_vc)
{
	console_t *con;

	if(which_vc >= NUM_VCS)
		return;
	con = g_vcs[which_vc];
/* do not select inactive VC */
	if(con == NULL)
		return;
/* copy screen contents of current VC to off-screen memory */
	if(g_curr_vc->fb_adr != NULL)
		memcpy(g_curr_vc->fb_adr, g_vga_fb_adr,
			g_vc_width * g_vc_height * 2);
/* copy off-screen memory of new VC to screen */
	if(con->fb_adr != NULL)
		memcpy(g_vga_fb_adr, con->fb_adr,
			g_vc_width * g_vc_height * 2);
/* switch VCs */
	g_curr_vc = con;
	move_csr();
}
/*****************************************************************************
*****************************************************************************/
static int do_vt(console_t *con, unsigned c)
{
	if(con->magic != CONSOLE_MAGIC)
		panic("do_vt: bad VC 0x%p", con);
/* state machine to handle the escape sequences */
	switch(con->esc)
	{
/* await 'ESC' */
	case 0:
		if(c == 0x1B)
		{
			con->esc++;
			return 1; /* "I handled it" */
		}
		break;
/* got 'ESC' -- await '[' */
	case 1:
		if(c == '[')
		{
			con->esc1 = 0;
			con->esc++;
			return 1;
		}
		break;
/* got 'ESC[' -- await digit or 'm' or 's' or 'u' or 'H' or 'J' or 'K'
xxx - support ESC[A ESC[B ESC[C ESC[D */
	case 2:
		if(isdigit(c))
		{
			con->esc1 = c - '0';
			con->esc++;
			return 1;
		}
/* ESC[m -- reset color scheme to default */
		else if(c == 'm')
		{
			set_attrib(con, 0);
			con->esc = 0;
			return 1;
		}
/* ESC[s -- save cursor position */
		else if(c == 's')
		{
			con->save_x = con->csr_x;
			con->save_y = con->csr_y;
			con->esc = 0;
			return 1;
		}
/* ESC[u -- restore saved cursor position */
		else if(c == 'u')
		{
			if(con->save_x >= 0)
			{
				con->csr_x = con->save_x;
				con->csr_y = con->save_y;
				con->save_x = -1;
			}
			con->esc = 0;
			return 1;
		}
/* ESC[H -- home cursor (does not clear screen) */
		else if(c == 'H')
		{
			con->csr_x = con->csr_y = 0;
			con->esc = 0;
			return 1;
		}
/* ESC[J -- erase to end of screen, including character under cursor.
Does not move cursor. */
		else if(c == 'J')
		{
			erase(con, (g_vc_height - con->csr_y) * g_vc_width
				- con->csr_x);
			return 1;
		}
/* ESC[K -- erase to end of line, including character under cursor.
Does not move cursor. */
		else if(c == 'K')
		{
			erase(con, g_vc_width - con->csr_x);
			return 1;
		}
		break;
/* got 'ESC[num1' -- collect digits until ';' or 'J' or 'm'
or 'A' or 'B' or 'C' or 'D' */
	case 3:
		if(isdigit(c))
		{
			con->esc1 = con->esc1 * 10 + c - '0';
			return 1;
		}
		else if(c == ';')
		{
			con->esc2 = 0;
			con->esc++;
			return 1;
		}
/* ESC[2J -- clear screen */
		else if(c == 'J')
		{
			if(con->esc1 == 2)
			{
				con->csr_x = con->csr_y = 0;
				erase(con, g_vc_height * g_vc_width);
				con->esc = 0;
				return 1;
			}
		}
/* ESC[num1m -- set attribute num1 */
		else if(c == 'm')
		{
			set_attrib(con, con->esc1);
			con->esc = 0;
			return 1;
		}
/* ESC[num1A -- move cursor up num1 rows */
		else if(c == 'A')
		{
			if(con->esc1 > con->csr_y)
				con->csr_y = 0;
			else
				con->csr_y -= con->esc1;
			con->esc = 0;
			return 1;
		}
/* ESC[num1B -- move cursor down num1 rows */
		else if(c == 'B')
		{
			if(con->esc1 >= con->csr_y + g_vc_height)
				con->csr_y = g_vc_height - 1;
			else
				con->csr_y += con->esc1;
			con->esc = 0;
			return 1;
		}
/* ESC[num1C -- move cursor right num1 columns */
		else if(c == 'C')
		{
			if(con->esc1 >= con->csr_x + g_vc_width)
				con->csr_x = g_vc_width - 1;
			else
				con->csr_x += con->esc1;
			con->esc = 0;
			return 1;
		}
/* ESC[num1D -- move cursor left num1 columns */
		else if(c == 'D')
		{
			if(con->esc1 > con->csr_x)
				con->csr_x = 0;
			else
				con->csr_x -= con->esc1;
			con->esc = 0;
			return 1;
		}
		break;
/* got 'ESC[num1;' -- collect digits until ';' or 'H' or 'f' or 'm' */
	case 4:
		if(isdigit(c))
		{
			con->esc2 = con->esc2 * 10 + c - '0';
			return 1;
		}
		else if(c == ';')
		{
			con->esc3 = 0;
			con->esc++;
			return 1;
		}
/* ESC[num1;num2H or ESC[num1;num2f -- move cursor to num1,num2 */
		else if(c == 'H' || c == 'f')
		{
			if(con->esc2 < 1)
				con->csr_x = 0;
			else if(con->esc2 > g_vc_width)
				con->csr_x = g_vc_width - 1;
			else
				con->csr_x = con->esc2 - 1;
			if(con->esc1 < 1)
				con->csr_y = 0;
			else if(con->esc1 > g_vc_height)
				con->csr_y = g_vc_height - 1;
			else
				con->csr_y = con->esc1 - 1;
			con->esc = 0;
			return 1;
		}
/* ESC[num1;num2m -- set attributes num1,num2 */
		else if(c == 'm')
		{
			set_attrib(con, con->esc1);
			set_attrib(con, con->esc2);
			con->esc = 0;
			return 1;
		}
		break;
/* got 'ESC[num1;num2;num3' -- collect digits until 'm' */
	case 5:
		if(isdigit(c))
		{
			con->esc3 = con->esc3 * 10 + c - '0';
			return 1;
		}
/* ESC[num1;num2;num3m -- set attributes num1,num2,num3 */
		else if(c == 'm')
		{
			set_attrib(con, con->esc1);
			set_attrib(con, con->esc2);
			set_attrib(con, con->esc3);
			con->esc = 0;
			return 1;
		}
		break;
/* invalid state; reset
	default:
		con->esc = 0;
		break; */
	}
	con->esc = 0;
	return 0; /* "YOU handle it" */
}
/*****************************************************************************
*****************************************************************************/
void putch_help(console_t *con, unsigned c)
{
	if(con->magic != CONSOLE_MAGIC)
		panic("putch_help: bad VC 0x%p", con);
/* handle ANSI/VT escape sequences */
	if(!do_vt(con, c))
	{
/* backspace */
		if(c == 0x08)
		{
			if(con->csr_x != 0)
				con->csr_x--;
		}
/* tab */
		else if(c == 0x09)
			con->csr_x = (con->csr_x + 8) & ~(8 - 1);
/* carriage return */
		else if(c == '\r')	/* 0x0D */
			con->csr_x = 0;
/* line feed */
//		else if(c == '\n')	/* 0x0A */
//			con->csr_y++;
/* CR/LF */
		else if(c == '\n')	/* ### - 0x0A again */
		{
			con->csr_x = 0;
			con->csr_y++;
		}
/* printable ASCII; store in text framebuffer */
		else if(c >= ' ')
		{
			uint16_t *where;
			unsigned att;

			if(con == g_curr_vc)
				where = g_vga_fb_adr;
			else
				where = con->fb_adr;
			where += (con->csr_y * g_vc_width + con->csr_x);
			att = (unsigned)con->attrib << 8;
			*where = (c | att);
			con->csr_x++;
		}
	}
/* check cursor position; scroll if necessary */
	if(con->csr_x >= g_vc_width)
	{
		con->csr_x = 0;
		con->csr_y++;
	}
	scroll(con);
/* move hardware cursor only if the VC we're writing is the current VC */
	if(g_curr_vc == con)
		move_csr();
}
/*****************************************************************************
*****************************************************************************/
void putch(unsigned c)
{
/* all kernel messages to VC #0 */
//	putch_help(g_vcs + 0, c);
/* all kernel messages to current VC */
	putch_help(g_curr_vc, c);
}
/*****************************************************************************
*****************************************************************************/
console_t *create_vc(void)
{
	static unsigned fg_color = 2;
/**/
	console_t *con;
	unsigned i;

/* If we're calling create_vc(), then kmalloc() must now be available.
Before allocating memory for a new VC, allocate memory for VC #0 */
	if(g_vc0.fb_adr == NULL)
	{
		g_vc0.fb_adr = (uint16_t *)kmalloc(
			g_vc_width * g_vc_height * 2);
		if(g_vc0.fb_adr == NULL)
			return NULL; /* panic? */
	}
/* allocate and zero console_t */
	con = (console_t *)kcalloc(sizeof(console_t));
	if(con == NULL)
		return NULL;
/* init */
	con->fb_adr = (uint16_t *)kmalloc(g_vc_width * g_vc_height * 2);
	if(con->fb_adr == NULL)
	{
		kfree(con);
		return NULL;
	}
	con->keystrokes.data = kmalloc(KBD_BUF_SIZE);
	if(con->keystrokes.data == NULL)
	{
		kfree(con->fb_adr);
		kfree(con);
		return NULL;
	}
	con->keystrokes.size = KBD_BUF_SIZE;
/* use a different foreground color for each VC */
	con->attrib = fg_color;
	fg_color++;
	if(fg_color > 7)
		fg_color = 1;
	con->magic = CONSOLE_MAGIC;
	erase(con, g_vc_height * g_vc_width);
/* Insert into global list. If there are more than NUM_VCS virtual consoles,
some of them will not be accessible from the keyboard. */
	for(i = 0; i < NUM_VCS; i++)
	{
		if(g_vcs[i] == NULL)
		{
			g_vcs[i] = con;
			break;
		}
	}
	return con;
}
/*****************************************************************************
*****************************************************************************/
void destroy_vc(console_t *con)
{
	unsigned i;

	if(con->magic != CONSOLE_MAGIC)
		panic("destroy_vc: bad VC 0x%p", con);
	kfree(con->fb_adr);
	kfree(con->keystrokes.data);
/* remove from global list */
	for(i = 0; i < NUM_VCS; i++)
	{
		if(g_vcs[i] == con)
		{
			g_vcs[i] = NULL;
			break;
		}
	}
	memset(con, 0, sizeof(console_t));
	kfree(con);
}
/*****************************************************************************
*****************************************************************************/
DISCARDABLE_CODE(void init_video(void))
{
	uintptr_t vga_fb_adr; /* physical address of framebuffer */

/* check for monochrome or color VGA emulation
I don't think new SVGA boards support mono emulation */
	if(inportb(VGA_MISC_READ) & 0x01)
	{
		vga_fb_adr = 0xB8000L;
		g_crtc_io_adr = 0x3D4;
	}
	else
	{
		vga_fb_adr = 0xB0000L;
		g_crtc_io_adr = 0x3B4;
	}
	g_vga_fb_adr = (uint16_t *) /* virtual address of FB */
		(vga_fb_adr - g_kvirt_to_phys);
/* read text-mode screen size from BIOS data segment */
	g_vc_width = *(uint16_t *)(0x44A - g_kvirt_to_phys);
	g_vc_height = *(uint8_t *)(0x484 - g_kvirt_to_phys) + 1;
/* g_vcs[0].fb_adr = NULL. We can't allocate memory for VC #0 because
kmalloc() is not yet available (we haven't called init_mm() yet).
However, as long as g_curr_vc == &g_vcs[0], then g_vcs[0].fb_adr
is not used. (Really! The code in this file is carefully written
to achieve this.) Allocate memory for VC #0 later. */
	kprintf("\x1B[2J"
		"  init_video: %s emulation, %u x %u, framebuffer at "
		"0x%X\n", (g_crtc_io_adr == 0x3D4) ? "color" : "mono",
		g_vc_width, g_vc_height, vga_fb_adr);
}
