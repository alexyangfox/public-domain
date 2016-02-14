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
#include "console.h"


int con_init_error;
int console_pid;

int console_filp_reference_cnt = 0;

struct Console *current_console;
struct Console console[MAX_CONSOLES];
struct MsgPort *console_msgport;

int32 keyboard_signal;
struct ISRHandler *con_isr_handler;

struct Mount *console_mount;

