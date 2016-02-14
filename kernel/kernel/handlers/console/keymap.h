#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/fs.h>



/* Real Modifier keycodes */
#define CAPS_DOWN     58
#define SHIFTL_DOWN   42
#define SHIFTR_DOWN   54
#define SHIFTL_UP     170
#define SHIFTR_UP     182
#define ALT_DOWN      56
#define ALT_UP        184
#define CTRL_DOWN     29
#define CTRL_UP       157

#define NUMLOCK_DOWN   0x45 
#define SCROLLOCK_DOWN 0x46  




/*
 *
 */

#define SC_NUMERIC_BEGIN    0x47
#define SC_NUMERIC_END      0x52




/*
 * Keycodes converted to ASCII values above 128
 */

#define EXT_CH_F1		  128
#define EXT_CH_F2		  129
#define EXT_CH_F3		  130
#define EXT_CH_F4		  131
#define EXT_CH_F5		  132
#define EXT_CH_F6		  133
#define EXT_CH_F7         134
#define EXT_CH_F8         135
#define EXT_CH_F9         136
#define EXT_CH_F10        137
#define EXT_CH_F11        138
#define EXT_CH_F12        139

#define EXT_CH_UP		  140
#define EXT_CH_DOWN       141
#define EXT_CH_LEFT       142
#define EXT_CH_RIGHT      143
#define EXT_CH_PGUP       144
#define EXT_CH_PGDN       145
#define EXT_CH_HOME       146
#define EXT_CH_END        147

#define NR_SCAN_CODES	  128


extern unsigned char unsh_kmap[NR_SCAN_CODES];
extern unsigned char sh_kmap[NR_SCAN_CODES];
extern unsigned char alt_kmap[NR_SCAN_CODES];

