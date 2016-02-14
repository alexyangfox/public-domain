#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/error.h>


void SetError (int error)
{
	current_process->error = error;
}



int GetError (void)
{
	return current_process->error;
}
