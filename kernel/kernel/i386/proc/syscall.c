#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include <kernel/timer.h>
#include <kernel/i386/i386.h>




/*
 * Move to syscalls.c ?????????
 * Pass syscall_idx??? Complicates the stack.
 */
	
int SyscallUnknown (void)
{
	UKill (GetPID(), SIGSYS);
	SetError (ENOSYS);
	return -1;
}


