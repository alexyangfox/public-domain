#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/fs.h>


#define ASCII_EOT      4
#define ASCII_LF       13

		

#define CON_MAX_WIDTH		80
#define CON_MAX_HEIGHT		25

#define MAX_CONSOLES		12
#define ONSCREEN_BUFFER_SZ	4096
#define INPUT_BUFFER_SZ		256
#define RAW_BUFFER_SZ		64
#define DISPLAY_BUFFER_SZ	(CON_MAX_WIDTH * (CON_MAX_HEIGHT+1))


#define SCROLL_DIR_UP	1
#define SCROLL_DIR_DOWN	-1

#define MAX_ESC_PARMS	6

#define MODE_COOKED 0
#define MODE_RAW	1

#define IOCTL_CON_SETMAP		0


#define WRITE_BUF_SZ	256




/*
 *
 */

struct ConFilp
{
	struct Device *device;
	struct Console *console;
	int reference_cnt;
};




/*
 *
 */
 
struct Console
{
	struct Device *device;
	
	int pgrp;    /* Old variable, is it used? */
	
	int reference_cnt;
		
	int width;
	int height;
	int row;
	int column;
	
	char current_attr;
	bool bold;
	bool blink;
	bool reverse;
	uint8 fg_color;
	uint8 bg_color;
	
	
	struct Termios termios;
		
	int line_buffer_x;		
	int line_buffer_y; /* Negative indicate start is off the screen */
	int line_buffer_cursor_pos;
	
	struct MsgQueue fsreq_queue;
	
	int esc_state;
	char esc_intro;
	int esc_parm_idx;
	int esc_parmv[MAX_ESC_PARMS];
	
	
	char display_buffer[80][25];
	char attr_buffer[80][25];
	
	int raw_read_buffer_offset;
	int raw_write_buffer_offset;
	int raw_buffer_free;

	int input_buffer_offset;
	int input_read_offset;
	bool line_data_ready;
	
	char raw_buffer[RAW_BUFFER_SZ];
	char input_buffer[INPUT_BUFFER_SZ];
};




/*
 *
 */

extern struct Device con_device;
 
extern int con_init_error;
extern int console_pid;

extern int console_filp_reference_cnt;
extern struct Console *current_console;
extern struct Console console[MAX_CONSOLES];
extern struct MsgPort *console_msgport;
extern int32 keyboard_signal;
extern struct ISRHandler *con_isr_handler;
extern struct Mount *console_mount;


/*
 *
 */
 
int con_init (void *elf);
void *con_expunge (void);
int con_opendevice (int unit, void *ioreq, uint32 flags);
int con_closedevice (void *ioreq);
void con_beginio (void *ioreq);
int con_abortio (void *ioreq);




/*
 *
 */

int32 ConsoleTask (void *arg);
void ConsoleTaskInit (void);
void ConsoleTaskFini (void);

void ConDoMount (struct FSReq *fsreq);
void ConDoUnmount (struct FSReq *fsreq);
void ConDoOpen (struct FSReq *fsreq);
void ConDoClose (struct FSReq *fsreq);
void ConDoDup (struct FSReq *fsreq);
int ConDoRead (struct FSReq *fsreq);
void ConDoWrite (struct FSReq *fsreq);
void ConDoTcgetattr (struct FSReq *fsreq);
void ConDoTcsetattr (struct FSReq *fsreq);
void ConDoIoctl (struct FSReq *fsreq);
void ConDoIsatty (struct FSReq *fsreq);
void ConDoStat (struct FSReq *fsreq);
void ConDoTcsetpgrp (struct FSReq *fsreq);
void ConDoTcgetpgrp (struct FSReq *fsreq);
void ConDoAbortIO (struct FSReq *abortreq);

void InitKeyboard (void);
int SetKeymap (int km);
void SetKeyboardLEDs (void);
int32 GetScanCode (void);
char ScancodeToASCII (int scancode);

int ConDoKeyboard (void);
int32 ConISRHandler (int32 isr_idx, void *arg);
int CheckModifierKeys (int32 scan_code);
int CheckFunctionKeys (int32 scan_code);
int CheckDebuggerKeys (int32 scan_code);
int CheckCtrlAltDelKeys (int32 scan_code);

void AddToRawCircularBuffer (struct Console *con, int scan_code);
int GetRawChar(struct Console *con);


void ConOutChar (struct Console *con, char ch);
void ParseEscape (struct Console *con, char ch);
void DoEscape (struct Console *con, char ch);
void DoEscapeAttrs (struct Console *con);
void ConMoveTo (struct Console *con, int x, int y);
void ConScrollScreen (struct Console *con, int direction);
void ConBell (void);
void ConPrintChar (struct Console *con, char ch);
void ConClearChars (struct Console *con, int x, int y, int n);
void ConInsertChars (struct Console *con, int x, int y, int n);
void ConDeleteChars (struct Console *con, int x, int y, int n);

void RefreshDisplay (struct Console *con, int ox, int oy, int width, int height);
void RefreshChar (struct Console *con, int ox, int oy);
void RefreshCursor (struct Console *con);



