#include <kernel/types.h>
#include <kernel/timer.h>
#include "floppy.h"





int gap[NT] = {0x1b,  0x2a};		/* Gap size */
int rate[NT] = {0x00,  0x02};	/* Data rate 0=500, 1=200 */
int nr_sectors[NT] = {18, 9};	/* sectors_per_track */
int nr_blocks[NT] = {2880, 1440};/* total sectors on disk */
int nr_cylinders[NT] = {80, 80};	/* cylinders on disk */
struct TimeVal mtr_start[NT] = {{1,0}, {1,0}}; /* Motor spin up wait */
char spec1[NT] = {0xaf, 0xdf};	/* step rate etc */


/* ????????????????? Got to set this properly */
int floppy_d;			/* diskette/drive combination  DENSITY Set in FloppyDoReadWrite ??? */
							/* Used as an index into above arrays?   Why is it not stored
								in the floppy structure?
								*/
int current_spec1;
bool floppy_needs_reset = TRUE;
int32 floppy_busy;

struct ISRHandler *floppy_isr_handler;
int32 floppy_isr_signal;

struct Alarm floppy_alarm;
int32 floppy_alarm_signal;

struct Floppy floppy_drive[2];

uint8 *floppy_buffer;
int32 floppy_buffer_unit;
int32 floppy_buffer_c;
int32 floppy_buffer_h;

int floppy_pid;
int floppy_init_error;

/* Move to global variables */
struct TimeVal floppy_motor_off_tv = {3,0};

struct Timer floppy_timer;
int32 floppy_timer_signal;

struct MsgPort *floppy_msgport;






