#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/lists.h>
#include <kernel/dbg.h>
#include <kernel/vm.h>
#include <kernel/config.h>
#include <kernel/sync.h>
#include <kernel/timer.h>
#include <kernel/i386/i386.h>
#include <kernel/i386/init.h>
#include <kernel/utility.h>


void InitRoot (void);
int32 IdleTask (void *arg);
bool init_proc_complete = FALSE;




/*
 *
 */

void InitProc (void)
{
	uint32 t;
	
	MutexInit (&proc_mutex);
	MutexInit (&loader_mutex);
	
	for (t=0;t<16;t++)
	{
		LIST_INIT (&isr_handler_list[t]);
	}
	

	for (t=0;t<256;t++)
	{
		CIRCLEQ_INIT (&sched_queue[t]);
	}
	
	LIST_INIT (&free_process_list)
	
	
	for (t=0; t<process_cnt; t++)
	{
		LIST_ADD_TAIL (&free_process_list, (process + t), free_entry);
		process[t].pid = t + 1;
		process[t].parent = NULL;
		process[t].state = PROC_STATE_UNALLOC;
	}
	
	
	for (t=0; t<JIFFIES_PER_SECOND; t++)
	{
		LIST_INIT(&timing_wheel[t]);
	}
	
	softclock_seconds = hardclock_seconds = GetBootTime();
	softclock_jiffies = hardclock_jiffies = 0;
	
	InitRoot();
	init_proc_complete = TRUE;
	
	idle_pid = KSpawn (&IdleTask, NULL, -127, "idle");
	
	
}



	


/*
 *
 */

void InitRoot (void)
{
	int t;
	uint32 cr0;
	
	root_process = LIST_HEAD (&free_process_list);
	LIST_REM_HEAD (&free_process_list, free_entry);

	root_process->kernel_stack = (vm_addr)&root_stack;
	


	root_pid = root_process->pid;
	
	KASSERT (root_pid != 0);
	
	root_process->exe_name = &root_process_name[0];

	root_process->reply_port.signal = SIG_FS;
	root_process->reply_port.pid = root_pid;
	LIST_INIT (&root_process->reply_port.msg_list);
	
	root_process->current_dir = root_current_dir;
		
	for (t=0; t<MAX_PATHNAME_SZ; t++)
		root_process->current_dir[t] = '\0';

	for (t=0; t < MAX_FD; t++)
		root_process->filedesc[t] = NULL;
	
	
	/* May not have proper alignment */
		
	MemSet (&root_fpu_state, 0, sizeof (union I387_State));
	
	KPRINTF ("MemSet() fpu state");
	
	root_process->archproc.fpu_state = (void *)&root_fpu_state;
	root_process->archproc.resume_esp = (vm_addr)NULL;
	root_process->archproc.resume_eip = (vm_addr)NULL;

	
	fpu_state_owner = NULL;
	cr0 = GetCR0();
	SetCR0(cr0 & ~CR0_TS);
	
	if (cpu_fxsr)
	{
		KPRINTF ("FPU has fast save/restore");
	
		asm ("fxsave (%0)" : : "r" ((uint32)(&root_fpu_state.fxstate)) : "memory" );
		
		cpu_mxcsr_mask = root_fpu_state.fxstate.mxcsr_mask;
		
		if (cpu_mxcsr_mask == 0)
			cpu_mxcsr_mask = MXCSR_MASK;
		
		root_fpu_state.fxstate.mxcsr_mask = cpu_mxcsr_mask;
		
		
		root_fpu_state.fxstate.control_word = 0x037f;
		root_fpu_state.fxstate.status_word = 0x0000;
		root_fpu_state.fxstate.tag_word = 0x0000;
		root_fpu_state.fxstate.fpu_ip_offset = 0x00000000;
		root_fpu_state.fxstate.fpu_ip_selector = 0x0000;
		root_fpu_state.fxstate.fpu_opcode = 0x00000000;
		root_fpu_state.fxstate.fpu_operand_offset = 0x00000000;
		root_fpu_state.fxstate.fpu_operand_selector = 0x0000;
		root_fpu_state.fxstate.mxcsr = 0;
		
		
		asm ("fxrstor (%0)" : : "r" ((uint32)(&root_fpu_state.fxstate)) : "memory" );
	}
	else
	{
		asm ("fnsave (%0)" : : "r" ((uint32)(&root_fpu_state.fstate)) : "memory" );
		
		root_fpu_state.fstate.control_word = 0x037f;
		root_fpu_state.fstate.status_word = 0x0000;
		root_fpu_state.fstate.tag_word = 0xffff;
		root_fpu_state.fstate.fpu_ip_offset = 0x00000000;
		root_fpu_state.fstate.fpu_ip_selector = 0x0000;
		root_fpu_state.fstate.fpu_opcode = 0x00000000;
		root_fpu_state.fstate.fpu_operand_offset = 0x00000000;
		root_fpu_state.fstate.fpu_operand_selector = 0x0000;

		asm ("frstor (%0)" : : "r" ((uint32)(&root_fpu_state.fstate)) : "memory" );		
	}
	
	
	/* Change the FPU stuff */
		
	
	
	PmapSwitch (&kernel_as.pmap);
		
	CreateNullAddressSpace(&root_process->as);
	root_process->user_as = &kernel_as;
	
	root_process->parent = root_process;
	LIST_INIT (&root_process->child_list);
	
	root_process->uid = 0;
	root_process->gid = 0;
	root_process->euid = 0;
	root_process->egid = 0;
	root_process->umask = 0;
	
	root_process->type = PROC_TYPE_KERNEL;
	root_process->pgrp = root_process->pid;
	root_process->pgrp_reference_cnt = 1;
	
	root_process->priority = 0;
	root_process->state = PROC_STATE_INIT;
	root_process->preempt_cnt = 0;
	root_process->quanta_used = 0;
	root_process->status = 0;
	root_process->free_signals = ~SIGNALS_RESERVED;
	root_process->pending_signals = 0;
	root_process->waiting_for_signals = 0;
	
	USigInit (root_process);
	
	current_process = root_process;
	current_process->state = PROC_STATE_READY;
	SchedReady (current_process);
	current_process->state = PROC_STATE_RUNNING;
	EnableInterrupts();	
}



/*
 *
 */

int32 IdleTask (void *arg)
{
	while (1)
	{
		asm ("hlt;"); 
	}
}



/*
 *
 */

void FiniProc (void)
{
}
