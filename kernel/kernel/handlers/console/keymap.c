#include <kernel/types.h>
#include "console.h"
#include "keymap.h"










/*
 * Unshifted keymap
 */

unsigned char unsh_kmap[NR_SCAN_CODES] =
{

/* 00 - 0F,  0 - 15 */

	0,
	27,               /* ESC */
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	'0',
	'-',
	'=',
	'\b',             /* <- Backspace */
	'\t',             /* Tab */

/* 10 - 1F,  16 - 31 */
	
	'q',
	'w',
	'e',
	'r',
	't',
	'y',
	'u',
	'i',
	'o',
	'p',
	'[',
	']',
	'\n',             /* Enter (cr) */
	0,                /* Left-CTRL */
	'a',
	's',
	
/* 20 - 2F,  32 - 47 */
 
	'd',
	'f',
	'g',
	'h',
	'j',
	'k',
	'l',
	';',
	39,               /* (') ' or @ */
	96,               /* (`) ` or ¬ */
	0,                /* Left Shift */
	'#',
	'z',
	'x',
	'c',
	'v',

/* 30 - 3F,  48 - 63 */
	
	'b',
	'n',
	'm',
	',',
	'.',
	'/',
	0,                /* Right Shift */
	'*',
	0,                /* Left-Alt */
	' ',              /* Space bar */
	0,                /* Caps Lock */
	EXT_CH_F1,           /* F1  */
	EXT_CH_F2,           /* F2  */
	EXT_CH_F3,           /* F3  */
	EXT_CH_F4,           /* F4  */
	EXT_CH_F5,           /* F5  */

/* 40 - 4F,  64 - 79 */
	
	EXT_CH_F6,           /* F6  */
	EXT_CH_F7,           /* F7  */
	EXT_CH_F8,           /* F8  */
	EXT_CH_F9,           /* F9  */
	EXT_CH_F10,          /* F10 */
	0,                /* Num Lock */
	0,                /* Scroll Lock */
	EXT_CH_HOME,         /* Keypad 7 / Home */
	EXT_CH_UP,           /* Keypad 8 / Up */
	EXT_CH_PGUP,         /* Keypad 9 / PgUp */
	'-',              /* Keypad - */
	EXT_CH_LEFT,         /* Keypad 4 / Left */
	'5',              /* Keypad 5 */
	EXT_CH_RIGHT,        /* Keypad 6 / Right */
	'+',              /* Keypad + */
	EXT_CH_END,          /* Keypad 1 / End */

/* 50 - 5F,  80 - 95 */
	
	EXT_CH_DOWN,         /* Keypad 2 / Down */
	EXT_CH_PGDN,         /* Keypad 3 / PgDn */
	0,                /* Keypad 0 / Ins */ 
	127,              /* Delete */
	0,                /* Alt-Sysreq >84 keyboard */
	0,                /* Uncommon key */
	'\\',             /* (\)  \ / | */
	EXT_CH_F11,          /* F11 */
	EXT_CH_F12,          /* F12 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,

/* 60 - 6F,  96 - 111 */

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

/* 70 - 7F,  112 - 127 */

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};




/*
 * shifted keymap
 */

unsigned char sh_kmap[NR_SCAN_CODES] =
{

/* 00 - 0F,  1 - 15 */

	0,
	27,               /* ESC */
	'!',
	34,               /* Double-Quote */
	'£',
	'$',
	'%',
	'^',
	'&',
	'*',
	'(',
	')',
	'_',
	'+',
	'\b',             /* <- Backspace */
	'\t',             /* Tab */

/* 10 - 1F,  16 - 31 */

	'Q',
	'W',
	'E',
	'R',
	'T',
	'Y',
	'U',
	'I',
	'O',
	'P',
	'{',
	'}',
	'\n',                /* Enter 13 */
	0,                /* Left-CTRL */
	'A',
	'S',

/* 20 - 2F,  32 - 47 */
	
	'D',
	'F',
	'G',
	'H',
	'J',
	'K',
	'L',
	':',
	'@',              /* (@) ' or @ */
	'¬',              /* (¬) ` or ¬ */
	0,                /* Left Shift */
	'~', 
	'Z',
	'X',
	'C',
	'V',

/* 30 - 3F,  48 - 63 */

	'B',
	'N',
	'M',
	'<',
	'>',
	'?',
	0,                /* Right Shift */
	'*',
	0,                /* Left-Alt */
	' ',              /* Space bar */
	0,                /* Caps Lock */
	EXT_CH_F1,           /* F1  */
	EXT_CH_F2,           /* F2  */
	EXT_CH_F3,           /* F3  */
	EXT_CH_F4,           /* F4  */
	EXT_CH_F5,           /* F5  */

/* 40 - 4F,  64 - 79 */
	
	EXT_CH_F6,           /* F6  */
	EXT_CH_F7,           /* F7  */
	EXT_CH_F8,           /* F8  */
	EXT_CH_F9,           /* F9  */
	EXT_CH_F10,          /* F10 */
	0,                /* Num Lock */
	0,                /* Scroll Lock */
	'7',              /* Keypad 7 / Home */
	'8',              /* Keypad 8 / Up */
	'9',              /* Keypad 9 / PgUp */
	'-',              /* Keypad - */
	'4',              /* Keypad 4 / Left */
	'5',              /* Keypad 5 */
	'6',              /* Keypad 6 / Right */
	'+',              /* Keypad + */
	'1',              /* Keypad 1 / End */

/* 50 - 5F,  80 - 95 */
	
	'2',              /* Keypad 2 / Down */
	'3',              /* Keypad 3 / PgDn */
	'0',              /* Keypad 0 / Ins */ 
	127,              /* Delete */
	0,                /* Alt-Sysreq >84 keyboard */
	0,                /* Uncommon key */
	'|',              /* (|)  \ or |    Unlabelled key in some countries */
	EXT_CH_F11,          /* F11 */
	EXT_CH_F12,          /* F12 */
	0,
	0,
	0,
	0,
	0,
	0,
	0,

/* 60 - 6F,  96 - 111 */

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,

/* 70 - 7F,  112 - 127 */

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};





/*
 * alt-key keymap
 */

unsigned char alt_kmap[NR_SCAN_CODES] =
{

/* 00 - 0F,  0 - 15 */

	0,
	27,               /* ESC */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	'\b',             /* <- Backspace */
	'\t',             /* Tab */
	
/* 10 - 1F,  16 - 31 */
		
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	
/* 20 - 2F,  32 - 47 */
	
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	
/* 30 - 3F,  48 - 63 */
	
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	
/* 40 - 4F,  64 - 79 */
	
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	
/* 50 - 5F,  80 - 95 */	
	
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	
/* 60 - 6F,  96 - 111 */	
	
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	
/* 70 - 7F,  112 - 127 */	
	
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

};






