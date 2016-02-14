#include <kernel/types.h>
#include <kernel/i386/i386.h>
#include <kernel/device.h>
#include <kernel/sync.h>
#include <kernel/proc.h>
#include <kernel/msg.h>
#include <kernel/vm.h>
#include <kernel/timer.h>
#include <kernel/dbg.h>
#include <kernel/fs.h>
#include <kernel/error.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include "console.h"




/*
 *
 */

int32 ConsoleTask (void *arg)
{
	struct FSReq *fsreq;
	struct Msg *msg;
	uint32 signals;
	struct ConFilp *filp;
	struct Console *con;
	int t;
	int exit_loop;
	int poll_loop;
	

	ConsoleTaskInit();

	while (1)
	{
		poll_loop = 0;
	
		signals = KWait ((1 << keyboard_signal) | (1 << console_msgport->signal) | SIGF_TERM);
		
		SetError (0);
		
		
		if (signals & (1 << keyboard_signal))
		{
			poll_loop = ConDoKeyboard();
		}
				
		if (signals & (1 << console_msgport->signal))
		{
			while ((msg = GetMsg (console_msgport)) != NULL)
			{
				fsreq = (struct FSReq *)msg;
				
				switch (fsreq->cmd)
				{
					case FS_CMD_OPEN:
						ConDoOpen (fsreq);
						break;
						
					case FS_CMD_CLOSE:
						ConDoClose (fsreq);
						break;
						
					case FS_CMD_DUP:
						ConDoDup (fsreq);
						break;
						
					case FS_CMD_READ:
						filp = (struct ConFilp *) fsreq->filp;
						con = filp->console;
						
						poll_loop = 1;
						
						QueueMsg (&con->fsreq_queue, &fsreq->msg);
						break;
						
					case FS_CMD_WRITE:
						filp = (struct ConFilp *) fsreq->filp;
						con = filp->console;

						poll_loop = 1;
						
						QueueMsg (&con->fsreq_queue, &fsreq->msg);
						break;
						
					case FS_CMD_TCGETATTR:
						ConDoTcgetattr(fsreq);
						break;
						
					case FS_CMD_TCSETATTR:
						ConDoTcsetattr(fsreq);
						break;
						
					case FS_CMD_IOCTL:
						ConDoIoctl(fsreq);
						break;
						
					case FS_CMD_STAT:						
					case FS_CMD_FSTAT:
						ConDoStat(fsreq);
						break;
					
					case FS_CMD_SYNC:
					case FS_CMD_FSYNC:
						fsreq->error = 0;
						fsreq->rc = 0;
						ReplyMsg (&fsreq->msg);
						break;

					case FS_CMD_ISATTY:
						ConDoIsatty(fsreq);
						break;
					
					case FS_CMD_TCSETPGRP:
						ConDoTcsetpgrp(fsreq);
						break;
						
					case FS_CMD_TCGETPGRP:
						ConDoTcgetpgrp(fsreq);
						break;

					case CMD_ABORT:
						ConDoAbortIO (fsreq);
						break;

					
					default:
					{
						KPRINTF ("CONSOLE Unknown command = %d", fsreq->cmd);
						KPANIC ("CONSOLE");
						
						fsreq->error = ENOSYS;
						fsreq->rc = -1;
						ReplyMsg (&fsreq->msg);
						break;
					}
				}
			}
		}	
		
		/* Swap loops around,  do away with exit_loop,  not sure of the purpose of
			exit loop,  what if there was another pending request? */
		
		if (poll_loop == 1)
		{
			for (t=0; t<MAX_CONSOLES; t++)
			{
				exit_loop = 0;
				
				while (exit_loop == 0 && (msg = MsgQueueHead (&console[t].fsreq_queue)) != NULL)
				{
					fsreq = (struct FSReq *)msg;
									
					switch (fsreq->cmd)
					{
						case FS_CMD_READ:
							exit_loop = ConDoRead (fsreq);
							break;
							
						case FS_CMD_WRITE:
							exit_loop = 0;
							ConDoWrite (fsreq);
							break;
							
						default:
							KPANIC ("Unknown queued message");
					}	
				}
			}
		}
		
		if (signals & SIGF_TERM)
		{
			ConsoleTaskFini();
		}
	}
}




/*
 * ConDoOpen();
 *
 * FIX: Consider allowing a process group to open only one console at a time
 * Or remove /con/0 /con/1  and /con (to access current pgrp console).
 *
 * Instead Open(/con) by process group leader creates a new terminal.
 * Future calls to Open(/con) open the same terminal.   If all processes
 * in the group close (/con) then it becomes free for other process groups?
 *
 */

void ConDoOpen (struct FSReq *fsreq)
{
	struct ConFilp *filp;
	int t;
	int x,y;
	int c;
	
	
	KLogToScreenDisable(); 
		
	if (*fsreq->path != '\0')
	{
		t = AtoI (fsreq->path);
	
		
		if (t >= 0 && t <= MAX_CONSOLES)
		{
			if (console[t].reference_cnt == 0)
			{
				if ((filp = KMalloc (sizeof (struct ConFilp))) != NULL)
				{
					filp->console = &console[t];
					filp->device = &con_device;
					filp->reference_cnt = 1;
					
					console[t].pgrp = fsreq->proc->pgrp;
					
					console_filp_reference_cnt ++;
					console[t].reference_cnt = 1;
												
					console[t].column = 0;
					console[t].row = 0;
			
					console[t].current_attr = 0x07;
					console[t].bold = FALSE;
					console[t].blink = FALSE;				
					console[t].reverse = FALSE;				
					console[t].bg_color = 0;
					console[t].fg_color = 7;
					
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
					
					RefreshDisplay (&console[t], 0, 0, console[t].width, console[t].height);
					
					fsreq->filp = filp;
					fsreq->error = 0;
					fsreq->rc = 0;
					ReplyMsg (&fsreq->msg);
					return;
				}
			}
		}
	}
	else
	{
		/* Find matching console/pgrp   What if more than one matches???? */
		/* FIX: See header comments */
		
		for (t=0; t < MAX_CONSOLES; t++)
		{
			if (fsreq->proc->pgrp == console[t].pgrp)
			{
				if ((filp = KMalloc (sizeof (struct ConFilp))) != NULL)
				{
					filp->console = &console[t];
					filp->device = &con_device;
					filp->reference_cnt = 1;
					
					console_filp_reference_cnt ++;
					console[t].reference_cnt ++;
					
					RefreshDisplay (&console[t], 0, 0, console[t].width, console[t].height);
					
					KPRINTF ("Opened /con");
					
					fsreq->filp = filp;
					fsreq->error = 0;
					fsreq->rc = 0;
					ReplyMsg (&fsreq->msg);
					return;
				}
			}
		}
	}
	
	

	fsreq->filp = NULL;
	fsreq->error = EBUSY;
	fsreq->rc = -1;
	ReplyMsg (&fsreq->msg);
}




/*
 * ConDoClose();
 */

void ConDoClose (struct FSReq *fsreq)
{
	struct ConFilp *filp;
	struct Console *con;
	
	filp = fsreq->filp;
	con = filp->console;
	
	filp->reference_cnt--;
	con->reference_cnt--;
	console_filp_reference_cnt --;
	
	if (filp->reference_cnt == 0)
		KFree (filp);
		
	fsreq->error = 0;
	fsreq->rc = 0;
	ReplyMsg (&fsreq->msg);
}




/*
 * ConDoDup();
 */

void ConDoDup (struct FSReq *fsreq)
{
	struct ConFilp *filp;
	struct Console *con;
	
	

	
	filp = fsreq->filp;
	con = filp->console;
	
	filp->reference_cnt ++;
	con->reference_cnt ++;
	console_filp_reference_cnt ++;
	
	fsreq->filp2 = filp;
	fsreq->error = 0;
	fsreq->rc = 0;
	
	ReplyMsg (&fsreq->msg);
}




/*
 *
 */

int ConDoRead (struct FSReq *fsreq)
{
	struct ConFilp *filp;
	struct Console *con;
	int ch;
	int return_sz;

	
	if (fsreq->count == 0)
	{
		KPANIC ("console zero read count");
	}
	
	filp = (struct ConFilp *) fsreq->filp;
	con = filp->console;


	KASSERT (con->reference_cnt > 0 && con->reference_cnt < 10000);
	KASSERT (filp->reference_cnt > 0 && filp->reference_cnt < 10000);	



	if ((con->termios.c_lflag & ICANON) == 0)
	{
		while ((ch = GetRawChar (con)) != -1)
		{	
			if (fsreq->count == 0)
				return_sz = 0;
			else
			{
				if (con->termios.c_lflag & ECHO)
					ConOutChar (con, ch);
					
				return_sz = 1;
			}
			
			CopyOut (fsreq->as, fsreq->buf, &ch, return_sz);

			DequeueMsg (&con->fsreq_queue);
			
			fsreq->nbytes_transferred = return_sz;
			fsreq->error = 0;
			fsreq->rc = 0;
			ReplyMsg (&fsreq->msg);
			return 0;
		}
	}
	else
	{
		if (con->line_data_ready == TRUE)
		{
			if ((con->input_read_offset + fsreq->count) > con->input_buffer_offset)
				return_sz = con->input_buffer_offset - con->input_read_offset;
			else
				return_sz = fsreq->count;
			
			CopyOut (fsreq->as, fsreq->buf, &con->input_buffer[con->input_read_offset], return_sz);
			
			con->input_read_offset += return_sz;
			
			if (con->input_read_offset == con->input_buffer_offset)
			{
				con->input_buffer_offset = 0;
				con->line_data_ready = FALSE;
			}
			
			
			DequeueMsg (&con->fsreq_queue);
			
			fsreq->nbytes_transferred = return_sz;
			fsreq->error = 0;
			fsreq->rc = 0;
			ReplyMsg (&fsreq->msg);
			return 0;
		}
		else
		{
			while ((ch = GetRawChar (con)) != -1)
			{
				if (ch == '\n')
				{
					con->line_data_ready = TRUE;
				
					if (con->input_buffer_offset >= INPUT_BUFFER_SZ)
						con->input_buffer_offset = INPUT_BUFFER_SZ-1;
					
					con->input_buffer[con->input_buffer_offset] = ch;
					con->input_buffer_offset++;
					
					con->input_read_offset = 0;
					
					if (con->input_buffer_offset < fsreq->count)
						return_sz = con->input_buffer_offset;
					else
						return_sz = fsreq->count;

					CopyOut (fsreq->as, fsreq->buf, &con->input_buffer, return_sz);
					
					con->input_read_offset += return_sz;
					
					if (con->input_read_offset == con->input_buffer_offset)
					{
						con->input_buffer_offset = 0;
						con->line_data_ready = FALSE;
					}
					
					if (con->termios.c_lflag & ECHONL)
						ConOutChar (con, ch);
					
					DequeueMsg (&con->fsreq_queue);
					
					fsreq->nbytes_transferred = return_sz;
					fsreq->error = 0;
					fsreq->rc = 0;
					ReplyMsg (&fsreq->msg);
					return 0;
				}
				else if (ch == '\b')
				{
					if (con->input_buffer_offset > 0)
					{
						con->input_buffer_offset --;
		
						ConOutChar (con, '\b');
						ConOutChar (con, ' ');
						ConOutChar (con, '\b');
					}
				}
				else if (ch == ASCII_EOT)
				{
					if (con->termios.c_lflag & ECHONL)
						ConOutChar (con, '\n');

					con->input_read_offset = 0;
					con->input_buffer_offset = 0;
					con->line_data_ready = FALSE;

					DequeueMsg (&con->fsreq_queue);
						
					fsreq->nbytes_transferred = 0;
					fsreq->error = 0;
					fsreq->rc = 0;
					ReplyMsg (&fsreq->msg);
					return 0;
				}
				else if (ch != 0 && con->input_buffer_offset < INPUT_BUFFER_SZ-1)
				{
					con->input_buffer[con->input_buffer_offset] = ch;
					con->input_buffer_offset++;
					
					if (ch == '\033')
						ch = '^';
					else if (ch == '\t')
						ch = ' ';
					
					if (con->termios.c_lflag & ECHO)
						ConOutChar (con, ch);
				}
			}
		}
	}
	
	return -1;
}







/*
 * ConDoWrite();
 */
 
#define WRITE_BUF_SZ	256

void ConDoWrite (struct FSReq *fsreq)
{
	char write_buf[WRITE_BUF_SZ];
	int remaining, len;
	char *src;
	struct ConFilp *filp;
	struct Console *con;
	int t;
	

	filp = (struct ConFilp *) fsreq->filp;
	con = filp->console;
	
	remaining = fsreq->count;
	src = fsreq->buf;

	while (remaining > 0)
	{
		if (remaining > WRITE_BUF_SZ)
			len = WRITE_BUF_SZ;
		else
			len = remaining; 
		
		CopyIn (fsreq->as, write_buf, src, len);
		
		for (t=0; t < len; t++)
		{
			ConOutChar (con, write_buf[t]);
		}
	
		remaining -= len;
		src += len;
	}
	
	DequeueMsg (&con->fsreq_queue);
		
	fsreq->nbytes_transferred = fsreq->count;
	fsreq->error = 0;
	fsreq->rc = 0;
	ReplyMsg (&fsreq->msg);
}




/*
 *
 */

void ConDoTcgetattr (struct FSReq *fsreq)
{
	struct ConFilp *filp;
	struct Console *con;
	

	filp = (struct ConFilp *) fsreq->filp;
	con = filp->console;
		
	CopyOut (fsreq->as, fsreq->termios, &con->termios, sizeof (struct Termios));
	
	fsreq->error = 0;
	fsreq->rc = 0;
	ReplyMsg (&fsreq->msg);
}




/*
 * ConDoTcsetattr()
 *
 * FIX: add actions to specify when/what should be modified/flushed.
 */
 
void ConDoTcsetattr (struct FSReq *fsreq)
{
	struct ConFilp *filp;
	struct Console *con;
	
	
	filp = (struct ConFilp *) fsreq->filp;
	con = filp->console;
	
	CopyIn (fsreq->as, &con->termios, fsreq->termios, sizeof (struct Termios));
			
	fsreq->error = 0;
	fsreq->rc = 0;
	ReplyMsg (&fsreq->msg);
}





/*
 *
 */

void ConDoIoctl (struct FSReq *fsreq)
{
	switch (fsreq->ioctl_request)
	{
		case IOCTL_CON_SETMAP:
			fsreq->rc = SetKeymap ((uint32)fsreq->ioctl_arg);
			fsreq->error = 0;
			break;
			
		default:
			fsreq->rc = -1;
			fsreq->error = EINVAL;
	}

	ReplyMsg(&fsreq->msg);
}




/*
 *
 */

void ConDoIsatty (struct FSReq *fsreq)
{
	fsreq->error = 0;
	fsreq->rc = 1;
	ReplyMsg (&fsreq->msg);
}




/*
 *
 */

void ConDoStat (struct FSReq *fsreq)
{
	struct Stat *stat = fsreq->stat;
	
	stat->st_mode = S_IFCHR  | (S_IRWXU | S_IRWXG | S_IRWXO);
		
	stat->st_nlink = 1;
	stat->st_uid = 1;
	stat->st_gid = 1;
	stat->st_rdev = 2;
	stat->st_size = 256;
	stat->st_atime = 10;
	stat->st_mtime = 11;
	stat->st_ctime = 12;
	stat->st_blocks = 256;
	
	fsreq->error = 0;
	fsreq->rc = 0;
	ReplyMsg (&fsreq->msg);
}




/*
 *
 */

void ConDoTcsetpgrp (struct FSReq *fsreq)
{
	struct ConFilp *filp;
	struct Console *con;

	filp = (struct ConFilp *) fsreq->filp;
	con = filp->console;

	con->pgrp = fsreq->foreground_grp;
	
	fsreq->error = 0;
	fsreq->rc = 0;
	ReplyMsg (&fsreq->msg);
}




/*
 *
 */

void ConDoTcgetpgrp (struct FSReq *fsreq)
{
	struct ConFilp *filp;
	struct Console *con;

	filp = (struct ConFilp *) fsreq->filp;
	con = filp->console;

	fsreq->foreground_grp = con->pgrp;
	
	fsreq->error = 0;
	fsreq->rc = 0;
	ReplyMsg (&fsreq->msg);
}




/*
 *
 */

void ConDoAbortIO (struct FSReq *abortreq)
{
	struct Msg *msg, *next;
	struct FSReq *fsreq;
	int t;
	
	
	KPRINTF ("ConDoAbortIO()");
	
	for (t=0; t< MAX_CONSOLES; t++)
	{
		msg = MsgQueueHead(&console[t].fsreq_queue);
		
		while (msg != NULL)
		{	
			next = MsgQueueNext (msg);
			
			fsreq = (struct FSReq *)msg;
			
			if (fsreq == (struct FSReq *)(abortreq->abort_ioreq))
			{
				/* FIX: fsreq->nbytes_transferred = return_sz; */
				
				console[t].input_buffer_offset = 0;
				console[t].line_data_ready = FALSE;
				
				MsgQueueRemove (&console[t].fsreq_queue, msg);
				
				fsreq->nbytes_transferred = 0;
				fsreq->error = EINTR;
				fsreq->rc = -1;            /* FIX: -1 or 0 ????? */
				ReplyMsg (&fsreq->msg);
				
				abortreq->error = 0;
				abortreq->rc = 0;
				ReplyMsg (&abortreq->msg);
				
				KPRINTF ("ConDoAbortIO() SUCCESS");
				return;
			}
			
			msg = next;
		}
	}
	
	KPRINTF ("ConDoAbortIO() ERROR");
	
	abortreq->error = 0;
	abortreq->rc = -1;
	ReplyMsg (&abortreq->msg);
}



