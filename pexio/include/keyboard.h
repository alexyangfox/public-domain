#ifndef KEYBOARD_H
#define KEYBOARD_H

extern unsigned char keymap[89];
extern unsigned char keymapshift[89];
extern unsigned char keystatus;	/* Bit 7 - Left Shift
				 	   Bit 6 - Right Shift
				   	   Bit 5 - Left Ctrl
					   Bit 4 - Right Ctrl
					   Bit 3 - Scroll lock
					   Bit 2 - Num lock
					   Bit 1 - 0
					   Bit 0 - 0 */

#endif
