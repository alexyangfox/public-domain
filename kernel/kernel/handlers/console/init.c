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

void ConsoleTaskInit (void)
{
	int t, c;
	int x, y;
	struct MountEnviron *me;
	
	
	console_pid = GetPID();
	
	console_msgport = CreateMsgPort();
	
	if (console_msgport == NULL)
	{
		con_init_error = -1;
		KSignal (GetPPID(), SIG_INIT);
		Exit(-1);
	}
	
	
	
	for (t=0; t < MAX_CONSOLES; t++)
	{
		console[t].device = &con_device;
		console[t].reference_cnt = 0;
		
		console[t].width = 80;
		console[t].height = 25;
		
		console[t].column = 0;
		console[t].row = 0;
				
		console[t].current_attr = 0x07;
		
		console[t].raw_write_buffer_offset = 0;
		console[t].raw_read_buffer_offset = 0;
		console[t].raw_buffer_free = RAW_BUFFER_SZ;
		
		console[t].input_read_offset = 0;
		console[t].input_buffer_offset = 0;
		console[t].line_data_ready = FALSE;
		
		
		/* Initialise termios */
		
		console[t].termios.c_iflag = ICRNL;		/* Input */
		console[t].termios.c_oflag = ONLCR;		/* Output */
		console[t].termios.c_cflag = CS8;		/* Control */
		console[t].termios.c_lflag = ECHO | ECHONL | ICANON; /* Local */
		
		for (c=0; c < NCCS; c++)
			console[t].termios.c_cc[c] = 0;
		
		console[t].termios.c_cc[VEOF] = 0x04;	
		console[t].termios.c_cc[VEOL] = 0x08;
		console[t].termios.c_cc[VERASE] = '\b';
		console[t].termios.c_cc[VKILL] = 0x40;
		
		
				
		InitMsgQueue (&console[t].fsreq_queue);
		
		for (y=0; y< 25; y++)
		{
			for (x=0; x<80; x++)
			{
				console[t].display_buffer[x][y] = ' ';
				console[t].attr_buffer[x][y] = 0x07;
			}
		}
	}
	
	current_console = &console[0];
	

	keyboard_signal = AllocSignal();
	
	con_isr_handler = ISRHandlerInsert (IRQ_KEYBOARD,
							ConISRHandler, NULL);
		
	if (con_isr_handler == NULL)
	{
		con_init_error = -1;
		KSignal (GetPPID(), SIG_INIT);
		Exit(-1);
	}
	
	
	InitKeyboard();
	
	
	
	if ((me = AllocMountEnviron()) != NULL)
	{
		me->mount_name[0] = 'c';
		me->mount_name[1] = 'o';
		me->mount_name[2] = 'n';
		me->mount_name[3] = '\0';
		
		StrLCpy (me->handler_name, "con.handler", MOUNTENVIRON_STR_MAX + 1);
		StrLCpy (me->device_name, "", MOUNTENVIRON_STR_MAX + 1);
		StrLCpy (me->startup_args, "", MOUNTENVIRON_STR_MAX + 1);
		
		me->handler_unit = 0;
		me->handler_flags = 0;
		
		me->device_unit = 0;
		me->device_flags = 0;
					
		me->block_size = 0;
					
		me->partition_start = 0;
		me->partition_end = 0;
					
		me->buffer_cnt = 0;
		me->boot_priority = 0;
		me->baud = 0;
	
		AddBootMountEnviron (me);

		con_init_error = 0;
		KSignal (GetPPID(), SIG_INIT);
	
	}
	else
	{
		con_init_error = -1;
		KSignal (GetPPID(), SIG_INIT);
		Exit(-1);
	}
}




/*
 *
 */
 
void ConsoleTaskFini (void)
{
	ISRHandlerRemove (con_isr_handler);
	FreeSignal (keyboard_signal);
	DeleteMsgPort (console_msgport);
	Exit(0);
}
