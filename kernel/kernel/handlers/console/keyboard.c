#include <kernel/types.h>
#include <kernel/i386/i386.h>
#include <kernel/device.h>
#include <kernel/sync.h>
#include <kernel/proc.h>
#include <kernel/msg.h>
#include <kernel/vm.h>
#include <kernel/timer.h>
#include <kernel/dbg.h>
#include <kernel/error.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include "console.h"
#include "keymap.h"



/*
 *
 */

void Reboot (int reboot_mode);




/*
 * AT keyboard ports and flags
 */

#define KEYBD		0x60
#define PORT_B      0x61
#define KBIT		0x80
#define KB_COMMAND	0x64
#define KB_GATE_A20	0x02
#define KB_PULSE_OUTPUT	0xf0
#define KB_RESET	0x01
#define KB_STATUS	0x64
#define KB_ACK		0xfa
#define KB_IN_BUSY	0x02
#define KB_OUT_BUSY	0x01
#define KB_LED_CODE	0xed




/*
 *
 */

static int shift_state = 0;
static int alt_state = 0;
static int caps_state = 0;
static int ctrl_state = 0;
static int numlock_state = 0;
static int real_shift_state = 0;




/*
 *
 */

int SetKeymap (int km)
{
	return 0;
}




/*
 * ConDoKeyboard();
 */

int ConDoKeyboard (void)
{
	int32 scan_code;
	unsigned char ext_ch;
	
	while ((scan_code = GetScanCode()) != -1)
	{
		if (CheckModifierKeys (scan_code) == 1)
		{
			SetKeyboardLEDs();
			continue;
		}
		
		if (scan_code >= 128)
			continue;
			
				
		if (SC_NUMERIC_BEGIN <= scan_code && scan_code <= SC_NUMERIC_END)
		{
			if (numlock_state == 0)
				ext_ch = unsh_kmap[scan_code];
			else
				ext_ch = sh_kmap[scan_code];
		}
		else
		{
			if (shift_state == 0)
				ext_ch = unsh_kmap[scan_code];
			else
				ext_ch = sh_kmap[scan_code];
	
			if (caps_state == 1)
			{
				if (shift_state == 0)
					real_shift_state = 1;
				else
					real_shift_state = 0;
			}
			else
				real_shift_state = shift_state;
		
			if (real_shift_state == 1)
				ext_ch = sh_kmap[scan_code];
			else
				ext_ch = unsh_kmap[scan_code];
					
			if (alt_state == 1)
				ext_ch = alt_kmap[scan_code];
		}
		
		
		if (CheckFunctionKeys (ext_ch) == 1)
			continue;
			
		if (CheckDebuggerKeys (ext_ch) == 1)
			continue;
				
		if (CheckCtrlAltDelKeys (ext_ch) == 1)
			continue;
		
		
		if (current_console->reference_cnt > 0)
			AddToRawCircularBuffer (current_console, ext_ch);
	}
	
		
	if (current_console->raw_buffer_free == RAW_BUFFER_SZ)
		return 0;
	else
		return 1;
}




/*
 * ConKeyboardInterrupt();
 */

int32 ConISRHandler (int32 isr_idx, void *arg)
{
	KSignal (console_pid, keyboard_signal);
	
	return 0;
}




/*
 *
 */

void InitKeyboard (void)
{
	SetKeyboardLEDs();
	GetScanCode();
}




/*
 *
 */

void SetKeyboardLEDs (void)
{
	uint8 led_bits = 0;
	
	if (caps_state == 1)
		led_bits |= (1<<2);

	if (numlock_state)
		led_bits |= (1<<1);

	
	OutByte (KEYBD, KB_LED_CODE);
	
	while (InByte(KB_STATUS) == KB_IN_BUSY);
	
	OutByte (KEYBD, led_bits);
}




/*
 */

int32 GetScanCode (void)
{
	int32 code;
	uint8 status;
	uint8 val;
	

	status = InByte(KB_STATUS);

	if (status & KB_OUT_BUSY)
	{
		code = InByte(KEYBD);
		val = InByte(PORT_B);
		OutByte(PORT_B, val | KBIT);
		OutByte(PORT_B, val);
	}
	else
		code = -1;
	
	return code;
}




/*
 * Add Num-lock/num-lock handling here and in AddToCircularBuffer().
 */

int CheckModifierKeys (int32 scan_code)
{
	switch (scan_code)
	{
		case CAPS_DOWN:
			caps_state = (caps_state == 1) ? 0 : 1;
			return 1;

		case NUMLOCK_DOWN:
			numlock_state = (numlock_state == 1) ? 0 : 1;
			return 1;
		
		case SHIFTL_DOWN:
			shift_state = 1;
			return 1;
			
		case SHIFTL_UP:
			shift_state = 0;
			return 1;
						
		case SHIFTR_DOWN:
			shift_state = 1;
			return 1;
						
		case SHIFTR_UP:
			shift_state = 0;
			return 1;			

		case ALT_DOWN:
			alt_state = 1;
			return 1;
								
		case ALT_UP:
			alt_state = 0;
			return 1;
							
		case CTRL_DOWN:
			ctrl_state = 1;
			return 1;
					
		case CTRL_UP:
			ctrl_state = 0;
			return 1;
	}
	
	return 0;
}



void KConDebug(int32 idx);
void KConDebugProc(void);



/*
 * FIX: Move debugger into seperate file 
 */
 
int CheckFunctionKeys (int32 ext_ch)
{
	int n;
	
	if (EXT_CH_F1 <= ext_ch  && ext_ch <= EXT_CH_F12)
	{
		n = ext_ch - EXT_CH_F1;
	
		current_console = &console[n];
		RefreshDisplay (current_console, 0, 0, current_console->width, current_console->height);
		RefreshCursor (current_console);
		return 1;
	}
	
	return 0;
}




/*
 * CheckDebuggerKeys();
 */

int CheckDebuggerKeys (int32 ext_ch)
{
	uint32 signals;
	int scan_code;
	
	if (ext_ch == 27 && shift_state == 1)
	{
		KConDebug (0);
		
		while (1)
		{
			signals = KWait (1 << keyboard_signal);
			
			if (signals & (1 << keyboard_signal))
			{
				while ((scan_code = GetScanCode()) != -1)
				{
					if (CheckModifierKeys (scan_code) == 1)
						continue;
					
					if (scan_code >= 128)
						continue;
					
					ext_ch = unsh_kmap[scan_code]; 
					
					switch (ext_ch)
					{
						case 27:
							RefreshDisplay (current_console, 0, 0, current_console->width, current_console->height);
							RefreshCursor (current_console);
							return 1;
							
						case EXT_CH_DOWN:
							KConDebug (1);
							break;
						
						case EXT_CH_UP:
							KConDebug (-1);
					}
				}
			}
		}
	}
	
	return 0;
}




/*
 * CheckCtrlAltDelKeys();
 *
 * FIX: Needs to be called somewhere.  Maybe display a shutdown/reset menu.
 */

int CheckCtrlAltDelKeys (int32 ext_ch)
{
	if (ctrl_state == 1 && alt_state == 1 && ext_ch == 127)
	{
		Reboot(1);    /* Where? ASM stubs ?????? */
		while (1);
	}
	
	return 0;
}




/*
 *
 */

void AddToRawCircularBuffer (struct Console *con, int ext_ch)
{
	char ch[4];
	int ch_cnt;
	int c;
	

	switch (ext_ch)
	{
		case EXT_CH_UP:
			ch[0] = '\033';
			ch[1] = '[';
			ch[2] = 'A';
			ch_cnt = 3;
			break;
			
		case EXT_CH_DOWN:
			ch[0] = '\033';
			ch[1] = '[';
			ch[2] = 'B';
			ch_cnt = 3;
			break;
			
		case EXT_CH_LEFT:
			ch[0] = '\033';
			ch[1] = '[';
			ch[2] = 'D';
			ch_cnt = 3;
			break;
					
		case EXT_CH_RIGHT:
			ch[0] = '\033';
			ch[1] = '[';
			ch[2] = 'C';
			ch_cnt = 3;
			break;
		
		case EXT_CH_PGUP:
			ch[0] = '\033';
			ch[1] = '[';
			ch[2] = 'V';
			ch_cnt = 3;
			break;

		case EXT_CH_PGDN:
			ch[0] = '\033';
			ch[1] = '[';
			ch[2] = 'U';
			ch_cnt = 3;
			break;

		case EXT_CH_HOME:
			ch[0] = '\033';
			ch[1] = '[';
			ch[2] = 'H';
			ch_cnt = 3;
			break;

		case EXT_CH_END:
			ch[0] = '\033';
			ch[1] = '[';
			ch[2] = '8';
			ch[3] = '~';
			ch_cnt = 4;
			break;


	
		default:
			if (ctrl_state == 1)
			{
				switch (ext_ch)
				{
					case 'i':
						ch[0] = 0x09;
						ch_cnt = 1;
						break;

					case 'd':
						ch[0] = ASCII_EOT;
						ch_cnt = 1;
						break;
					
					case 'c':
						UKill (-con->pgrp, SIGINT);
						ch_cnt = 0;
						break;
					
					default:
						ch_cnt = 0;
						
				}
			}
			else
			{
				if (ext_ch <= 127)
				{
					ch[0] = ext_ch;
					ch_cnt = 1;
				}
				else
					ch_cnt = 0;
			}
	}
	
	
	
	
	
	
	
	
	
	/* Not checking for key release, adding 2 chars to a buffer at a time */
	/* *** Need read_offset and write_offset in buffer */
	
	if (con->raw_buffer_free >= ch_cnt)
	{
		for (c=0; c < ch_cnt; c++)
		{
			con->raw_buffer[con->raw_write_buffer_offset] = ch[c];
			con->raw_write_buffer_offset++;
			con->raw_write_buffer_offset %= RAW_BUFFER_SZ;
			con->raw_buffer_free --;
		}
	}
}




/*
 *
 */

int GetRawChar(struct Console *con)
{
	char ch;
	
	if (con->raw_buffer_free < RAW_BUFFER_SZ)
	{
		ch = con->raw_buffer[con->raw_read_buffer_offset];
		con->raw_read_buffer_offset++;
		con->raw_read_buffer_offset %= RAW_BUFFER_SZ;
		con->raw_buffer_free++;
		return ch;
	}
	
	return -1;
}

