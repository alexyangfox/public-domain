#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/timer.h>
#include <kernel/sync.h>
#include <kernel/device.h>




/*
 *
 */

LIST_DEFINE (DeviceList) device_list;

int iomanager_init_error;
int iomanager_pid;

struct MsgPort *iomanager_msgport;
