/*****************************************************************************
Keyboard driver/interrupt handler demo code.
Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: ?
This code is public domain (no copyright).
You can do whatever you want with it.

Scancode set 3 is very nice:
- one-byte make code for each key
- break code is the make code preceded by F0h
- each key can be programmed individually to repeat or not,
  and to generate a break code or not
Unfortunately, not all keyboards support set 3.

Scancodes set 2 and 1 are crap:
- keys have scancodes of different lengths: 01 for Esc,
  E038 for right Alt, and E11D45E19DC5 for Pause in set 1
- some keys produce different scancodes depending on
  the internal num lock state of the keyboard:
  Insert, Home, PageUp, Delete, End, PageDown,
  the arrow keys, and the Windows and Menu keys
- the 8042 keyboard controller chip does optional translation
  of set 2 scancodes (AT) to set 1 (XT), so that's one more
  thing to worry about

To give you an idea of what a mess this is,
look at the set 2 scancodes for the Insert key:

8042		internal
AT-to-XT	Num Lock	make		repeat		break
conversion	state		code		code		code
----------	--------	-----		------		-----
off		off		E070		E070		E0F070
off		ON		E012E070	E070		E0F070E0F012
ON		off		E052		E052		E0D2
ON		ON		E02AE052	E052		E0D2E0AA

Naturally, only set 2 seems to be supported by all keyboards.

xxx - trouble with this code: when keyboard internal num lock is on,
the E0AA or E0F012 appears BEFORE the NEXT MAKE code,
instead of AFTER the CURRENT BREAK code
*****************************************************************************/
#include <stdio.h> /* printf(), putchar(), setbuf() */
#include <dos.h> /* inportb(), outportb() */

/* this will change if you reprogram the 8259 chips to route
IRQs to non-reserved INTs (a pmode OS should do this) */
#define	KBD_VECT_NUM	9

/********************************* TURBO C **********************************/
#if defined(__TURBOC__)
#include <dos.h> /* struct REGPACK, intr(), getvect(), setvect() */

#define	R_AX		r_ax

#define	trap(N,R)	intr(N,R)

typedef struct REGPACK regs_t;

#ifdef __cplusplus
typedef void interrupt (*vector_t)(...);
#else
typedef void interrupt (*vector_t)();
#endif

#define	INTERRUPT			interrupt

#define	save_vector(vec, num)		vec = getvect(num)
#define	install_handler(vec, num, fn)	setvect(num, (vector_t)fn)
#define	restore_vector(vec, num)	setvect(num, vec)

/********************************* DJGPP ************************************/
#elif defined(__DJGPP__)
#include <go32.h> /* _my_cs() */
#include <dpmi.h> /* _go32_dpmi..., __dpmi... */
#include <crt0.h> /* _CRT0_FLAG_LOCK_MEMORY */

typedef struct
{
	_go32_dpmi_seginfo old_v, new_v;
} vector_t;

#define	INTERRUPT			/* nothing */

#define	save_vector(vec, num)						\
	_go32_dpmi_get_protected_mode_interrupt_vector(num, &vec.old_v)

#define	install_handler(vec, num, fn)					\
	vec.new_v.pm_selector = _my_cs();				\
	vec.new_v.pm_offset = (unsigned long)fn;			\
	_go32_dpmi_allocate_iret_wrapper(&vec.new_v);			\
	_go32_dpmi_set_protected_mode_interrupt_vector(num, &vec.new_v);

#define	restore_vector(vec, num)					\
	_go32_dpmi_set_protected_mode_interrupt_vector(num, &vec.old_v);\
	_go32_dpmi_free_iret_wrapper(&vec.new_v);

/* lock all memory, to prevent it being swapped or paged out */
int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;

#define	R_AX		x.ax

#define	trap(N,R)	__dpmi_int(N,R)

typedef __dpmi_regs	regs_t;

/******************************** WATCOM C **********************************/
#elif defined(__WATCOMC__)
#include <conio.h> /* outp(), inp() */
#include <dos.h> /* dos_getvect(), _dos_setvect() */

#ifdef __cplusplus
typedef void __interrupt (*vector_t)(...);
#else
typedef void __interrupt (*vector_t)();
#endif

#define	INTERRUPT			__interrupt

#define	save_vector(vec, num)		vec = _dos_getvect(num)
#define	install_handler(vec, num, fn)	_dos_setvect(num, (vector_t)fn)
#define	restore_vector(vec, num)	_dos_setvect(num, vec)

#define	inportb(P)	inp(P)
#define	outportb(P,V)	outp(P,V)

#define	R_AX		w.ax

/* WARNING: for 32-bit code, unused fields of regs_t
must be zeroed before using this macro */
#define	trap(N,R)	intr(N,R)

typedef union REGPACK	regs_t;

#else
#error Not Turbo C, not DJGPP, not Watcom C. Sorry.
#endif
/*----------------------------------------------------------------------------
CIRCULAR QUEUES
----------------------------------------------------------------------------*/
typedef struct
{
	unsigned char *data;
	unsigned size, in_ptr, out_ptr;
} queue_t;
/*****************************************************************************
*****************************************************************************/
static int inq(queue_t *q, unsigned data)
{
	unsigned temp;

	temp = q->in_ptr + 1;
	if(temp >= q->size)
		temp = 0;
/* if in_ptr reaches out_ptr, the queue is full */
	if(temp == q->out_ptr)
		return -1;
	q->data[q->in_ptr] = data;
	q->in_ptr = temp;
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int deq(queue_t *q, unsigned char *data)
{
/* if out_ptr reaches in_ptr, the queue is empty */
	if(q->out_ptr == q->in_ptr)
		return -1;
	*data = q->data[q->out_ptr++];
	if(q->out_ptr >= q->size)
		q->out_ptr = 0;
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int empty(queue_t *q)
{
	return q->out_ptr == q->in_ptr;
}
/*----------------------------------------------------------------------------
LOW-LEVEL KEYBOARD DRIVER
----------------------------------------------------------------------------*/
#define	BUF_SIZE	64

static unsigned char g_kbd_buf[BUF_SIZE];
static queue_t g_queue =
{
	g_kbd_buf, BUF_SIZE, 0, 0
};
/*****************************************************************************
*****************************************************************************/
static void INTERRUPT kbd_irq(void)
{
	unsigned scan_code;

/* read I/O port 60h to reset interrupt at 8042 keyboard controller chip */
	scan_code = inportb(0x60);
/* put scancode in queue */
	(void)inq(&g_queue, scan_code);
/* reset interrupt at 8259 interrupt controller chip */
	outportb(0x20, 0x20);
}
/*****************************************************************************
*****************************************************************************/
static int read_kbd(void)
{
	unsigned long timeout;
	unsigned stat, data;

	for(timeout = 500000L; timeout != 0; timeout--)
	{
		stat = inportb(0x64);
/* loop until 8042 output buffer full */
		if(stat & 0x01)
		{
			data = inportb(0x60);
/* loop if parity error or receive timeout */
			if((stat & 0xC0) == 0)
				return data;
		}
	}
/*	printf("read_kbd: timeout\n"); */
	return -1;
}
/*****************************************************************************
*****************************************************************************/
static void write_kbd(unsigned adr, unsigned data)
{
	unsigned long timeout;
	unsigned stat;

	for(timeout = 500000L; timeout != 0; timeout--)
	{
		stat = inportb(0x64);
/* loop until 8042 input buffer empty */
		if((stat & 0x02) == 0)
			break;
	}
	if(timeout == 0)
	{
		printf("write_kbd: timeout\n");
		return;
	}
	outportb(adr, data);
}
/*****************************************************************************
*****************************************************************************/
static int write_kbd_await_ack(unsigned val)
{
	int got;

	write_kbd(0x60, val);
	got = read_kbd();
	if(got != 0xFA)
	{
		printf("write_kbd_await_ack: expected "
			"acknowledge (0xFA), got 0x%02X\n", got);
		return -1;
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int init_kbd(unsigned ss, unsigned typematic, unsigned xlat)
{
	printf("flushing keyboard output\n");
	while(read_kbd() != -1)
		/* nothing */;
/* disable keyboard before programming it */
	printf("disabling keyboard controller\n");
	if(write_kbd_await_ack(0xF5) != 0)
		return -1;
/* disable PS/2 mouse, set SYS bit, and Enable Keyboard Interrupt... */
	write_kbd(0x64, 0x60);
/* ...and either disable or enable AT-to-XT keystroke conversion */
	write_kbd(0x60, xlat ? 0x65 : 0x25);
/* program desired scancode set */
	printf("programming scancode set %u\n", ss);
	if(write_kbd_await_ack(0xF0) != 0)
		return -1;
	if(write_kbd_await_ack(ss) != 0)
		return -1;
/* we want all keys to return both a make code (when pressed)
and a break code (when released -- scancode set 3 only) */
	if(ss == 3)
	{
		printf("making all keys make-break\n");
		if(write_kbd_await_ack(0xFA) != 0)
			return -1;
	}
/* set typematic delay and rate */
	printf("setting fast typematic mode\n");
	if(write_kbd_await_ack(0xF3) != 0)
		return -1;
	if(write_kbd_await_ack(typematic) != 0)
		return -1;
/* enable keyboard */
	printf("enabling keyboard controller\n");
	if(write_kbd_await_ack(0xF4) != 0)
		return -1;
	return 0;
}
/*----------------------------------------------------------------------------
SCANCODE CONVERSION
----------------------------------------------------------------------------*/
/* "ASCII" values for non-ASCII keys. All of these are user-defined.
function keys: */
#define	KEY_F1		0x80
#define	KEY_F2		(KEY_F1 + 1)
#define	KEY_F3		(KEY_F2 + 1)
#define	KEY_F4		(KEY_F3 + 1)
#define	KEY_F5		(KEY_F4 + 1)
#define	KEY_F6		(KEY_F5 + 1)
#define	KEY_F7		(KEY_F6 + 1)
#define	KEY_F8		(KEY_F7 + 1)
#define	KEY_F9		(KEY_F8 + 1)
#define	KEY_F10		(KEY_F9 + 1)
#define	KEY_F11		(KEY_F10 + 1)
#define	KEY_F12		(KEY_F11 + 1)
/* cursor keys */
#define	KEY_INS		0x90
#define	KEY_DEL		(KEY_INS + 1)
#define	KEY_HOME	(KEY_DEL + 1)
#define	KEY_END		(KEY_HOME + 1)
#define	KEY_PGUP	(KEY_END + 1)
#define	KEY_PGDN	(KEY_PGUP + 1)
#define	KEY_LFT		(KEY_PGDN + 1)
#define	KEY_UP		(KEY_LFT + 1)
#define	KEY_DN		(KEY_UP + 1)
#define	KEY_RT		(KEY_DN + 1)
/* print screen/sys rq and pause/break */
#define	KEY_PRNT	(KEY_RT + 1)
#define	KEY_PAUSE	(KEY_PRNT + 1)
/* these return a value but they could also act as additional meta keys */
#define	KEY_LWIN	(KEY_PAUSE + 1)
#define	KEY_RWIN	(KEY_LWIN + 1)
#define	KEY_MENU	(KEY_RWIN + 1)

/* "meta bits"
0x0100 is reserved for non-ASCII keys, so start with 0x200 */
#define	KBD_META_ALT	0x0200	/* Alt is pressed */
#define	KBD_META_CTRL	0x0400	/* Ctrl is pressed */
#define	KBD_META_SHIFT	0x0800	/* Shift is pressed */
#define	KBD_META_ANY	(KBD_META_ALT | KBD_META_CTRL | KBD_META_SHIFT)
#define	KBD_META_CAPS	0x1000	/* CapsLock is on */
#define	KBD_META_NUM	0x2000	/* NumLock is on */
#define	KBD_META_SCRL	0x4000	/* ScrollLock is on */
/*****************************************************************************
*****************************************************************************/
#define	RAW1_LEFT_CTRL		0x1D
#define	RAW1_LEFT_SHIFT		0x2A
#define	RAW1_CAPS_LOCK		0x3A
#define	RAW1_LEFT_ALT		0x38
#define	RAW1_RIGHT_ALT		0x38	/* same as left */
#define	RAW1_RIGHT_CTRL		0x1D	/* same as left */
#define	RAW1_RIGHT_SHIFT	0x36
#define	RAW1_SCROLL_LOCK	0x46
#define	RAW1_NUM_LOCK		0x45
#define	RAW1_DEL		0x53

static int set1_scancode_to_ascii(unsigned code)
{
	static const unsigned char map[] =
	{
/* 00 */0,	0x1B,	'1',	'2',	'3',	'4',	'5',	'6',
/* 08 */'7',	'8',	'9',	'0',	'-',	'=',	'\b',	'\t',
/* 10 */'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
/* 1Dh is left Ctrl */
/* 18 */'o',	'p',	'[',	']',	'\n',	0,	'a',	's',
/* 20 */'d',	'f',	'g',	'h',	'j',	'k',	'l',	';',
/* 2Ah is left Shift */
/* 28 */'\'',	'`',	0,	'\\',	'z',	'x',	'c',	'v',
/* 36h is right Shift */
/* 30 */'b',	'n',	'm',	',',	'.',	'/',	0,	0,
/* 38h is left Alt, 3Ah is Caps Lock */
/* 38 */0,	' ',	0,	KEY_F1,	KEY_F2,	KEY_F3,	KEY_F4,	KEY_F5,
/* 45h is Num Lock, 46h is Scroll Lock */
/* 40 */KEY_F6,	KEY_F7,	KEY_F8,	KEY_F9,	KEY_F10,0,	0,	KEY_HOME,
/* 48 */KEY_UP,	KEY_PGUP,'-',	KEY_LFT,'5',	KEY_RT,	'+',	KEY_END,
/* 50 */KEY_DN,	KEY_PGDN,KEY_INS,KEY_DEL,0,	0,	0,	KEY_F11,
/* 58 */KEY_F12
	};
	static const unsigned char shift_map[] =
	{
/* 00 */0,	0x1B,	'!',	'@',	'#',	'$',	'%',	'^',
/* 08 */'&',	'*',	'(',	')',	'_',	'+',	'\b',	'\t',
/* 10 */'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
/* 1Dh is left Ctrl */
/* 18 */'O',	'P',	'{',	'}',	'\n',	0,	'A',	'S',
/* 20 */'D',	'F',	'G',	'H',	'J',	'K',	'L',	':',
/* 2Ah is left Shift */
/* 28 */'"',	'~',	0,	'|',	'Z',	'X',	'C',	'V',
/* 36h is right Shift */
/* 30 */'B',	'N',	'M',	'<',	'>',	'?',	0,	0,
/* 38h is left Alt, 3Ah is Caps Lock */
/* 38 */0,	' ',	0,	KEY_F1,	KEY_F2,	KEY_F3,	KEY_F4,	KEY_F5,
/* 45h is Num Lock, 46h is Scroll Lock */
/* 40 */KEY_F6,	KEY_F7,	KEY_F8,	KEY_F9,	KEY_F10,0,	0,	KEY_HOME,
/* 48 */KEY_UP,	KEY_PGUP,'-',	KEY_LFT,'5',	KEY_RT,	'+',	KEY_END,
/* 50 */KEY_DN,	KEY_PGDN,KEY_INS,KEY_DEL,0,	0,	0,	KEY_F11,
/* 58 */KEY_F12
	};
	static unsigned saw_break_code, kbd_status;
/**/
	unsigned temp;

/* check for break code (i.e. a key is released) */
	if(code >= 0x80)
	{
		saw_break_code = 1;
		code &= 0x7F;
	}
/* the only break codes we're interested in are Shift, Ctrl, Alt */
	if(saw_break_code)
	{
		if(code == RAW1_LEFT_ALT || code == RAW1_RIGHT_ALT)
			kbd_status &= ~KBD_META_ALT;
		else if(code == RAW1_LEFT_CTRL || code == RAW1_RIGHT_CTRL)
			kbd_status &= ~KBD_META_CTRL;
		else if(code == RAW1_LEFT_SHIFT || code == RAW1_RIGHT_SHIFT)
			kbd_status &= ~KBD_META_SHIFT;
		saw_break_code = 0;
		return -1;
	}
/* it's a make code: check the "meta" keys, as above */
	if(code == RAW1_LEFT_ALT || code == RAW1_RIGHT_ALT)
	{
		kbd_status |= KBD_META_ALT;
		return -1;
	}
	if(code == RAW1_LEFT_CTRL || code == RAW1_RIGHT_CTRL)
	{
		kbd_status |= KBD_META_CTRL;
		return -1;
	}
	if(code == RAW1_LEFT_SHIFT || code == RAW1_RIGHT_SHIFT)
	{
		kbd_status |= KBD_META_SHIFT;
		return -1;
	}
/* Scroll Lock, Num Lock, and Caps Lock set the LEDs. These keys
have on-off (toggle or XOR) action, instead of momentary action */
	if(code == RAW1_SCROLL_LOCK)
	{
		kbd_status ^= KBD_META_SCRL;
		goto LEDS;
	}
	if(code == RAW1_NUM_LOCK)
	{
		kbd_status ^= KBD_META_NUM;
		goto LEDS;
	}
	if(code == RAW1_CAPS_LOCK)
	{
		kbd_status ^= KBD_META_CAPS;
LEDS:		write_kbd(0x60, 0xED);	/* "set LEDs" command */
		temp = 0;
		if(kbd_status & KBD_META_SCRL)
			temp |= 1;
		if(kbd_status & KBD_META_NUM)
			temp |= 2;
		if(kbd_status & KBD_META_CAPS)
			temp |= 4;
		write_kbd(0x60, temp);	/* bottom 3 bits set LEDs */
		return -1;
	}
/* no conversion if Alt pressed */
	if(kbd_status & KBD_META_ALT)
		return code;
/* convert A-Z[\]^_ to control chars */
	if(kbd_status & KBD_META_CTRL)
	{
		if(code >= sizeof(map) / sizeof(map[0]))
			return -1;
		temp = map[code];
		if(temp >= 'a' && temp <= 'z')
			return temp - 'a';
		if(temp >= '[' && temp <= '_')
			return temp - '[' + 0x1B;
		return -1;
	}
/* convert raw scancode to ASCII */
	if(kbd_status & KBD_META_SHIFT)
	{
/* ignore invalid scan codes */
		if(code >= sizeof(shift_map) / sizeof(shift_map[0]))
			return -1;
		temp = shift_map[code];
/* defective keyboard? non-US keyboard? more than 104 keys? */
		if(temp == 0)
			return -1;
/* caps lock? */
		if(kbd_status & KBD_META_CAPS)
		{
			if(temp >= 'A' && temp <= 'Z')
				temp = map[code];
		}
	}
	else
	{
		if(code >= sizeof(map) / sizeof(map[0]))
			return -1;
		temp = map[code];
		if(temp == 0)
			return -1;
		if(kbd_status & KBD_META_CAPS)
		{
			if(temp >= 'a' && temp <= 'z')
				temp = shift_map[code];
		}
	}
	return temp;
}
/*****************************************************************************
*****************************************************************************/
#define	RAW3_LEFT_CTRL		0x11
#define	RAW3_LEFT_SHIFT		0x12
#define	RAW3_CAPS_LOCK		0x14
#define	RAW3_LEFT_ALT		0x19
#define	RAW3_RIGHT_ALT		0x39
#define	RAW3_RIGHT_CTRL		0x58
#define	RAW3_RIGHT_SHIFT	0x59
#define	RAW3_SCROLL_LOCK	0x5F
#define	RAW3_NUM_LOCK		0x76
#define	RAW3_DEL		0x64

static int set3_scancode_to_ascii(unsigned code)
{
	static const unsigned char map[] =
	{
/* 00 */0,	0,	0,	0,	0,	0,	0,	KEY_F1,
/* 08 */0x1B,	0,	0,	0,	0,	0x09,	'~',	KEY_F2,
/* 11 is left Ctrl; 12 is left Shift; 14 is CapsLock */
/* 10 */0,	0,	0,	0,	0,	'q',	'!',	KEY_F3,
/* 19 is left Alt */
/* 18 */0,	0,	'z',	's',	'a',	'w',	'@',	KEY_F4,
/* 20 */0,	'c',	'x',	'd',	'e',	'$',	'#',	KEY_F5,
/* 28 */0,	' ',	'v',	'f',	't',	'r',	'%',	KEY_F6,
/* 30 */0,	'n',	'b',	'h',	'g',	'y',	'^',	KEY_F7,
/* 39 is right Alt */
/* 38 */0,	0,	'm',	'j',	'u',	'&',	'*',	KEY_F8,
/* 40 */0,	'<',	'k',	'i',	'o',	')',	'(',	KEY_F9,
/* 48 */0,	'>',	'?',	'l',	':',	'p',	'_',	KEY_F10,
/* 50 */0,	0,	'"',	0,	'{',	'+',	KEY_F11,KEY_PRNT,
/* 58 is right Ctrl; 59 is right Shift; 5F is Scroll Lock */
/* 58 */0,	0,	0x0D,	'}',	'|',	0,	KEY_F12,0,
/* 60 */KEY_DN,	KEY_LFT,KEY_PAUSE,KEY_UP,KEY_DEL,KEY_END,0x08,	KEY_INS,
/* 68 */0,	'1',	KEY_RT,	'4',	'7',	KEY_PGDN,KEY_HOME,KEY_PGUP,
/* 76 is Num Lock */
/* 70 */'0',	'.',	'2',	'5',	'6',	'8',	0,	'/',
/* 78 */0,	0x0D,	'3',	0,	'+',	'9',	'*',	0,
/* 80 */0,	0,	0,	0,	'-',	0,	0,	0,
/* 88 */0,	0,	0,	KEY_LWIN,KEY_RWIN,KEY_MENU,0,	0
	};
	static const unsigned char shift_map[] =
	{
/* 00 */0,	0,	0,	0,	0,	0,	0,	KEY_F1,
/* 08 */0x1B,	0,	0,	0,	0,	0x09,	'`',	KEY_F2,
/* 10 */0,	0,	0,	0,	0,	'Q',	'1',	KEY_F3,
/* 18 */0,	0,	'Z',	'S',	'A',	'W',	'2',	KEY_F4,
/* 20 */0,	'C',	'X',	'D',	'E',	'4',	'3',	KEY_F5,
/* 28 */0,	' ',	'V',	'F',	'T',	'R',	'5',	KEY_F6,
/* 30 */0,	'N',	'B',	'H',	'G',	'Y',	'6',	KEY_F7,
/* 38 */0,	0,	'M',	'J',	'U',	'7',	'8',	KEY_F8,
/* 40 */0,	',',	'K',	'I',	'O',	'0',	'9',	KEY_F9,
/* 48 */0,	'.',	'/',	'L',	';',	'P',	'-',	KEY_F10,
/* 50 */0,	0,	'\'',	0,	'[',	'=',	KEY_F11,KEY_PRNT,
/* 58 */0,	0,	0x0D,	']',	'\\',	0,	KEY_F12,0,
/* 60 */KEY_DN,	KEY_LFT,KEY_PAUSE,KEY_UP,KEY_DEL,KEY_END,0x08,	KEY_INS,
/* 68 */0,	KEY_END,KEY_RT,	KEY_LFT,KEY_HOME,KEY_PGDN,KEY_HOME,KEY_PGUP,
/* 70 */KEY_INS,KEY_DEL,KEY_DN,	'5',	KEY_RT,	KEY_UP,	0,	'/',
/* 78 */0,	0x0D,	KEY_PGDN,0,	'+',	KEY_PGUP,'*',	0,
/* 80 */0,	0,	0,	0,	'-',	0,	0,	0,
/* 88 */0,	0,	0,	KEY_LWIN,KEY_RWIN,KEY_MENU,0,	0
	};
	static unsigned saw_break_code, kbd_status;
/**/
	unsigned temp;

/* check for break code (i.e. a key is released) */
	if(code == 0xF0)
	{
		saw_break_code = 1;
		return -1;
	}
/* the only break codes we're interested in are Shift, Ctrl, Alt */
	if(saw_break_code)
	{
		if(code == RAW3_LEFT_ALT || code == RAW3_RIGHT_ALT)
			kbd_status &= ~KBD_META_ALT;
		else if(code == RAW3_LEFT_CTRL || code == RAW3_RIGHT_CTRL)
			kbd_status &= ~KBD_META_CTRL;
		else if(code == RAW3_LEFT_SHIFT || code == RAW3_RIGHT_SHIFT)
			kbd_status &= ~KBD_META_SHIFT;
		saw_break_code = 0;
		return -1;
	}
/* it's a make code: check the "meta" keys, as above */
	if(code == RAW3_LEFT_ALT || code == RAW3_RIGHT_ALT)
	{
		kbd_status |= KBD_META_ALT;
		return -1;
	}
	if(code == RAW3_LEFT_CTRL || code == RAW3_RIGHT_CTRL)
	{
		kbd_status |= KBD_META_CTRL;
		return -1;
	}
	if(code == RAW3_LEFT_SHIFT || code == RAW3_RIGHT_SHIFT)
	{
		kbd_status |= KBD_META_SHIFT;
		return -1;
	}
/* Scroll Lock, Num Lock, and Caps Lock set the LEDs. These keys
have on-off (toggle or XOR) action, instead of momentary action */
	if(code == RAW3_SCROLL_LOCK)
	{
		kbd_status ^= KBD_META_SCRL;
		goto LEDS;
	}
	if(code == RAW3_NUM_LOCK)
	{
		kbd_status ^= KBD_META_NUM;
		goto LEDS;
	}
	if(code == RAW3_CAPS_LOCK)
	{
		kbd_status ^= KBD_META_CAPS;
LEDS:		write_kbd(0x60, 0xED);	/* "set LEDs" command */
		temp = 0;
		if(kbd_status & KBD_META_SCRL)
			temp |= 1;
		if(kbd_status & KBD_META_NUM)
			temp |= 2;
		if(kbd_status & KBD_META_CAPS)
			temp |= 4;
		write_kbd(0x60, temp);	/* bottom 3 bits set LEDs */
		return -1;
	}
/* no conversion if Alt pressed */
	if(kbd_status & KBD_META_ALT)
		return code;
/* convert A-Z[\]^_ to control chars */
	if(kbd_status & KBD_META_CTRL)
	{
		if(code >= sizeof(map) / sizeof(map[0]))
			return -1;
		temp = map[code];
		if(temp >= 'a' && temp <= 'z')
			return temp - 'a';
		if(temp >= '[' && temp <= '_')
			return temp - '[' + 0x1B;
		return -1;
	}
/* convert raw scancode to ASCII */
	if(kbd_status & KBD_META_SHIFT)
	{
/* ignore invalid scan codes */
		if(code >= sizeof(shift_map) / sizeof(shift_map[0]))
			return -1;
		temp = shift_map[code];
/* defective keyboard? non-US keyboard? more than 104 keys? */
		if(temp == 0)
			return -1;
/* caps lock? */
		if(kbd_status & KBD_META_CAPS)
		{
			if(temp >= 'A' && temp <= 'Z')
				temp = map[code];
		}
	}
	else
	{
		if(code >= sizeof(map) / sizeof(map[0]))
			return -1;
		temp = map[code];
		if(temp == 0)
			return -1;
		if(kbd_status & KBD_META_CAPS)
		{
			if(temp >= 'a' && temp <= 'z')
				temp = shift_map[code];
		}
	}
	return temp;
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
#if 0
	static const unsigned esc_make_code[] =
	{
		0 /* there is no scancode set 0 */, 0x01, 0x76, 0x08
	};
/**/
#endif
	unsigned windows, ss = 1;
	unsigned char scancode;
	vector_t old_vector;
	regs_t regs;
	int c;

	if(ss < 1 || ss > 3)
	{
		printf("scancode set (%u) must be 1-3\n", ss);
		return 1;
	}
/* can't change scancode set if Windows running */
	regs.R_AX = 0x1600;
	trap(0x2F, &regs);
	regs.R_AX &= 0xFF;
	if(regs.R_AX != 0 && regs.R_AX != 0x80)
	{
		printf("Windows detected, can not change scancode set\n");
		windows = 1;
/* actually, the AT keyboard uses scancode set 2 by default, but the
8042 controller is programmed to convert the scancodes to set 1 */
		ss = 1;
	}
	else
		windows = 0;
/* install interrupt handler */
	save_vector(old_vector, KBD_VECT_NUM);
	install_handler(old_vector, KBD_VECT_NUM, kbd_irq);
/* change scancode set, set SLOW repeat/LONG delay, turn off translation */
	if(!windows)
	{
		if(init_kbd(ss, 0x7F, 0) != 0)
			goto END;
	}
	printf("press Esc to end\n");
/* turn off Watcom C and DJGPP line buffering */
	setbuf(stdout, NULL);
/* main loop */
	do
	{
/* wait for keyboard interrupt to put scancode in queue */
		while(empty(&g_queue))
			/* nothing */;
/* get scancode */
		if(deq(&g_queue, &scancode) < 0)
			break; /* should not happen */
/* process scancodes into ASCII and display them */
#if 0
		switch(ss)
		{
			case 1:
				c = set1_scancode_to_ascii(scancode);
				break;
			case 3:
				c = set3_scancode_to_ascii(scancode);
				break;
			default:
				c = -1;
				break;
		}
		if(c != -1)
			putchar(c);
/* display raw scancodes in hex */
#else
		printf("%02X  ", scancode);
#endif
/* until Esc pressed
	} while(scancode != esc_make_code[ss]); */
	} while(scancode != 1 && scancode != 0x76 && scancode != 8);
	printf("\n");
END:
/* set scancode set 1, set FAST repeat/SHORT delay, no AT-to-XT translation */
	if(!windows)
		init_kbd(1, 0, 0);
	restore_vector(old_vector, KBD_VECT_NUM);
	return 0;
}
