#include <kernel/types.h>
#include <kernel/timer.h>
#include "ata.h"




struct Ata ata_drive[MAX_ATA_DRIVES];

bool ata_isr14_enabled = FALSE;
bool ata_isr15_enabled = FALSE;

struct ISRHandler *ata_isr14_handler;
struct ISRHandler *ata_isr15_handler;

int32 ata_isr14_signal;
int32 ata_isr15_signal;
int32 ata_alarm_signal;

int ata_init_error;
int ata_pid;

struct MsgPort *ata_msgport;

int ata_error = 0;			/* Needed or not,  only for reset ?? */

struct Alarm ata_alarm;
bool ata_needs_reset;

bool use_interrupts;

uint8 *ata_buffer;

volatile uint8 ata_stored_status;	/* status after interrupt, saved
									so as to prevent clearing further
									interrupts.  May want to be per channel */

struct Timer ata_timer;
int32 ata_timer_signal;
struct TimeVal ata_timer_tv = {0,500000};


volatile int ata_isr14_cnt = 0;
volatile int ata_isr15_cnt = 0;
