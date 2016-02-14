#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/timer.h>
#include <kernel/sync.h>
#include <kernel/device.h>




/*
 *
 */

struct Mutex proc_mutex;
struct Mutex loader_mutex;

uint32 process_cnt;
struct Process *process;
uint32 isr_depth;
struct Process *current_process;

uint32 interrupt_stack;
uint32 interrupt_stack_ceiling;
bool reschedule_request;

volatile uint32 hardclock_seconds, hardclock_jiffies;
volatile uint32 softclock_seconds, softclock_jiffies;
LIST_DEFINE (TimingWheelList) timing_wheel[JIFFIES_PER_SECOND];


LIST_DEFINE (ProcessList) free_process_list;
LIST_DEFINE (ISRHandlerList) isr_handler_list[16];


CIRCLEQ_DEFINE (SchedQueue) sched_queue[256];
uint32 sched_queue_bitmap[8];

uint32 user_process_cnt;

