#ifndef KERNEL_REBOOT_H
#define KERNEL_REBOOT_H

#include <kernel/types.h>
#include <kernel/lists.h>




/*
 * Reboot() options
 */

#define KOS_REBOOT_HALT		0
#define KOS_REBOOT_REBOOT	1




/*
 * Variables
 */

extern int reboot_how;
extern bool reboot_requested;


/*
 * Prototypes
 */

int Reboot (int how);





#endif

