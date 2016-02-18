/*----------------------------------------------------------------------------
KEYBOARD ROUTINES

EXPORTS:
int deq(queue_t *q, unsigned char *data);
int inq(queue_t *q, unsigned char data);
void keyboard_bh(void);
void keyboard_irq(void);
----------------------------------------------------------------------------*/
#include <system.h> /* disable(), outportb(), inportb() */
#include <string.h> /* NULL */
#include <conio.h> /* KEY_nnn */
#include "_krnl.h" /* console_t, wait_queue_t, queue_t */

/* IMPORTS
from VIDEO.C */
extern console_t *g_curr_vc;

void select_vc(unsigned which_vc);

/* from THREADS.C */
void wake_up(wait_queue_t *queue);
int sleep_on(wait_queue_t *queue, unsigned *timeout);

/* from KSTART.S */
void halt(void);

/* "raw" set 1 scancodes from PC keyboard. Keyboard info here:
http://my.execpc.com/~geezer/osd/kbd */
#define	RAW1_LEFT_CTRL		0x1D
#define	RAW1_RIGHT_CTRL		0x1D	/* same as left */
#define	RAW1_LEFT_SHIFT		0x2A
#define	RAW1_RIGHT_SHIFT	0x36
#define	RAW1_LEFT_ALT		0x38
#define	RAW1_RIGHT_ALT		0x38	/* same as left */
#define	RAW1_CAPS_LOCK		0x3A
#define	RAW1_F1			0x3B
#define	RAW1_F2			0x3C
#define	RAW1_F3			0x3D
#define	RAW1_F4			0x3E
#define	RAW1_F5			0x3F
#define	RAW1_F6			0x40
#define	RAW1_F7			0x41
#define	RAW1_F8			0x42
#define	RAW1_F9			0x43
#define	RAW1_F10		0x44
#define	RAW1_NUM_LOCK		0x45
#define	RAW1_SCROLL_LOCK	0x46
#define	RAW1_DEL		0x53
#define	RAW1_F11		0x57
#define	RAW1_F12		0x58

static unsigned char g_buf[KBD_BUF_SIZE];
static queue_t g_kbd_queue =
{
	g_buf,		/* .data */
	KBD_BUF_SIZE	/* .size */
/* no need to initialize .in_ptr, .out_ptr */
};
static wait_queue_t g_kbd_wait;
/*****************************************************************************
On my Pentium system, A20 must be enabled for reboot() to work.
When I used this function in a real-mode kernel, it gave me
exception 6 (invalid instruction) instead of rebooting.
*****************************************************************************/
static void reboot(void)
{
	unsigned i;

	disable();
/* flush the keyboard controller */
	do
	{
		i = inportb(0x64);
		if(i & 0x01)
		{
			(void)inportb(0x60);
			continue;
		}
	} while(i & 0x02);
/* pulse the CPU reset line */
	outportb(0x64, 0xFE);
/* ...and if that didn't work, just halt */
	halt();
}
/*****************************************************************************
returns 0 and sets 'data' if byte available,
else returns -1
*****************************************************************************/
int deq(queue_t *q, unsigned char *data)
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
returns 0 if byte stored in queue,
else returns -1 if queue is full
*****************************************************************************/
int inq(queue_t *q, unsigned char data)
{
	unsigned i;

	i = q->in_ptr + 1;
	if(i >= q->size)
		i = 0;
/* if in_ptr reaches out_ptr, the queue is full */
	if(i == q->out_ptr)
		return -1;
	q->data[q->in_ptr] = data;
	q->in_ptr = i;
	return 0;
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
	if(timeout != 0)
		outportb(adr, data);
}
/*****************************************************************************
*****************************************************************************/
static unsigned convert(unsigned key)
{
	static const unsigned char set1_map[] =
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
/* 48 */KEY_UP,	KEY_PGUP,'-',	KEY_LEFT,'5',	KEY_RIGHT,'+',	KEY_END,
/* 50 */KEY_DOWN,KEY_PGDN,KEY_INS,KEY_DEL,0,	0,	0,	KEY_F11,
/* 58 */KEY_F12
	};
	static const unsigned char shift1_map[]=
	{
/* 00 */0,	0x1B,	'!',	'@',	'#',	'$',	'%',	'^',
/* 08 */'&',	'*',	'(',	')',	'_',	'+',	'\b',	'\t',
/* 10 */'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
/* 1Dh is left Ctrl */
/* 18 */'O',	'P',	'{',	'}',	'\n',	0,	'A',	'S',
/* 20 */'D',	'F',	'G',	'H',	'J',	'K',	'L',	':',
/* 2Ah is left Shift */
/* 28 */'\"',	'~',	0,	'|',	'Z',	'X',	'C',	'V',
/* 36h is right Shift */
/* 30 */'B',	'N',	'M',	'<',	'>',	'?',	0,	0,
/* 38h is left Alt, 3Ah is Caps Lock */
/* 38 */0,	' ',	0,	KEY_F1,	KEY_F2,	KEY_F3,	KEY_F4,	KEY_F5,
/* 45h is Num Lock, 46h is Scroll Lock */
/* 40 */KEY_F6,	KEY_F7,	KEY_F8,	KEY_F9,	KEY_F10,0,	0,	KEY_HOME,
/* 48 */KEY_UP,	KEY_PGUP,'-',	KEY_LEFT,'5',	KEY_RIGHT,'+',	KEY_END,
/* 50 */KEY_DOWN,KEY_PGDN,KEY_INS,KEY_DEL,0,	0,	0,	KEY_F11,
/* 58 */KEY_F12
	};
	static unsigned kbd_status;
/**/
	unsigned i;

/* check for break key (i.e. a key is released) */
	if(key >= 0x80)
	{
		key &= 0x7F;
/* the only break codes we're interested in are Shift, Ctrl, Alt */
		if(key == RAW1_LEFT_ALT || key == RAW1_RIGHT_ALT)
			kbd_status &= ~KBD_META_ALT;
		else if(key == RAW1_LEFT_CTRL || key == RAW1_RIGHT_CTRL)
			kbd_status &= ~KBD_META_CTRL;
		else if(key == RAW1_LEFT_SHIFT || key == RAW1_RIGHT_SHIFT)
			kbd_status &= ~KBD_META_SHIFT;
		return 0;
	}
/* it's a make key (key preseed): check the "meta" keys, as above */
	if(key == RAW1_LEFT_ALT || key == RAW1_RIGHT_ALT)
	{
		kbd_status |= KBD_META_ALT;
		return 0;
	}
	if(key == RAW1_LEFT_CTRL || key == RAW1_RIGHT_CTRL)
	{
		kbd_status |= KBD_META_CTRL;
		return 0;
	}
	if(key == RAW1_LEFT_SHIFT || key == RAW1_RIGHT_SHIFT)
	{
		kbd_status |= KBD_META_SHIFT;
		return 0;
	}
/* Scroll Lock, Num Lock, and Caps Lock set the LEDs. These keys
have on-off (toggle or XOR) action, instead of momentary action */
	if(key == RAW1_SCROLL_LOCK)
	{
		kbd_status ^= KBD_META_SCRL;
		goto LEDS;
	}
	if(key == RAW1_NUM_LOCK)
	{
		kbd_status ^= KBD_META_NUM;
		goto LEDS;
	}
	if(key == RAW1_CAPS_LOCK)
	{
		kbd_status ^= KBD_META_CAPS;
LEDS:		write_kbd(0x60, 0xED);	/* "set LEDs" command */
		i = 0;
		if(kbd_status & KBD_META_SCRL)
			i |= 1;
		if(kbd_status & KBD_META_NUM)
			i |= 2;
		if(kbd_status & KBD_META_CAPS)
			i |= 4;
		write_kbd(0x60, i);	/* bottom 3 bits set LEDs */
		return 0;
	}
/* now that we've tested for CTRL and ALT,
we can handle the three-finger salute */
	if((kbd_status & KBD_META_CTRL) &&
		(kbd_status & KBD_META_ALT) && key == RAW1_DEL)
	{
		kprintf("\n""\x1B[42;37;1m""*** rebooting!");
		reboot();
	}
/* ignore invalid scan codes */
	if(key >= sizeof(set1_map) / sizeof(set1_map[0]))
		return 0;
/* handle Ctrl+A-Z */
	if(kbd_status & KBD_META_CTRL)
	{
		i = set1_map[key];
		if(i < 'a' || i > 'z')
			return 0;
		return i - 'a' + 1;
	}
/* handle Shift */
	if(kbd_status & KBD_META_SHIFT)
		i = shift1_map[key];
	else
		i = set1_map[key];
	return i;
}
/*****************************************************************************
*****************************************************************************/
void keyboard_irq(void)
{
	unsigned char code;

	code = inportb(0x60);
	/*if(*/inq(&g_kbd_queue, code)/*)
		full queue; beep or something */;
/* wake up the bottom-half handler */
	wake_up(&g_kbd_wait);
}
/*****************************************************************************
bottom-half keyboard interrupt handler
*****************************************************************************/
void keyboard_bh(void)
{
	static char alt;
/**/
	unsigned char key, i = 0;

	while(1)
	{
		do
		{
/* empty queue? */
			if(g_kbd_queue.out_ptr == g_kbd_queue.in_ptr)
/* sleep on it until keyboard_irq() puts something in it */
				sleep_on(&g_kbd_wait, NULL);
/* get raw set-1 scancode */
		} while(deq(&g_kbd_queue, &key));
/* if it's F1, F2 etc. switch to the appropriate virtual console */
		switch(key)
		{
		case RAW1_LEFT_ALT: /* same as RAW1_RIGHT_ALT */
			alt = 1;
			goto HANDLE;
		case RAW1_LEFT_ALT | 0x80:
			alt = 0;
			goto HANDLE;
		case RAW1_F1:
			i = 0;
			goto SWITCH_VC;
		case RAW1_F2:
			i = 1;
			goto SWITCH_VC;
		case RAW1_F3:
			i = 2;
			goto SWITCH_VC;
		case RAW1_F4:
			i = 3;
			goto SWITCH_VC;
		case RAW1_F5:
			i = 4;
			goto SWITCH_VC;
		case RAW1_F6:
			i = 5;
			goto SWITCH_VC;
		case RAW1_F7:
			i = 6;
			goto SWITCH_VC;
		case RAW1_F8:
			i = 7;
			goto SWITCH_VC;
		case RAW1_F9:
			i = 8;
			goto SWITCH_VC;
		case RAW1_F10:
			i = 9;
			goto SWITCH_VC;
		case RAW1_F11:
			i = 10;
			goto SWITCH_VC;
		case RAW1_F12:
			i = 11;
SWITCH_VC:
			if(alt)
				select_vc(i);
			else
				goto HANDLE;
			break;
		default:
HANDLE:
/* convert scancode to ASCII */
			i = convert(key);
			if(i != 0)
			{
/* put scancode into per-VC queue */
				/*if(*/inq(&g_curr_vc->keystrokes, i) /*:
					full queue; beep or something */;
/* wake up the task running in this VC */
				wake_up(&g_curr_vc->wait);
			}
			break;
		}
	}
}
