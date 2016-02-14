#include <kernel/types.h>
#include <kernel/i386/i386.h>
#include <kernel/proc.h>
#include <kernel/config.h>
#include <kernel/utility.h>
#include <kernel/kmalloc.h>
#include <kernel/dbg.h>

extern void RescheduleResume;
extern void KProcStart (void);
extern void UProcStart (void);
extern void ForkStart (void);












/*
 * Rename to InitFpu() ??????
 *
 * Merge into ArchInitFork/KProc/Exec etc.
 */

int ArchAllocProcess (struct Process *proc)
{
	union I387_State *fpu;
	
	if ((fpu = KMalloc (sizeof (union I387_State))) != NULL)
	{
		/* FIX: User different ones for each */
		
		if (cpu_fxsr)
		{
			MemSet (&fpu->fxstate, 0, sizeof (struct I387_FXSave));
		
			/* Remove zeros */
			
			fpu->fxstate.control_word = 0x037f;
			fpu->fxstate.status_word = 0x0000;
			fpu->fxstate.tag_word = 0x0000;      /* Why is this not 0xffff */
			fpu->fxstate.mxcsr = 0;
			fpu->fxstate.mxcsr_mask = cpu_mxcsr_mask;
		}
		else
		{
			MemSet (&fpu->fstate, 0, sizeof (struct I387_FSave));
		
			fpu->fstate.control_word = 0x037f;
			fpu->fstate.status_word = 0x0000;
			fpu->fstate.tag_word = 0xffff;
		}
		
		proc->archproc.fpu_state = (void *)fpu;
		return 0;
	}

	return -1;
}




/*
 *
 */
 
void ArchFreeProcess (struct Process *proc)
{
	fpu_state_owner = NULL;
	KFree (proc->archproc.fpu_state);
}




/*
 *
 */

int ArchInitKProc (struct Process *proc, int32 (*func)(void *arg), void *arg)
{
	uint32 *esp;
	union I387_State *fpu;
	
	proc->archproc.resume_eip = 0;
	proc->archproc.resume_esp = 0;


	esp = (uint32 *)((vm_addr)proc->kernel_stack + KERNEL_STACK_SZ);
	
	*--esp = (uint32)arg;
	*--esp = (uint32)func;
	*--esp = 0x00000000;		/* return address of KProcStart */
	*--esp = (uint32)KProcStart;
	*--esp = EFLG_IF;
	*--esp = 0;					/* Pusha */
	*--esp = 0;					/* Pusha */
	*--esp = 0;					/* Pusha */
	*--esp = 0;					/* Pusha */
	*--esp = 0;					/* Pusha */
	*--esp = 0;					/* Pusha */
	*--esp = 0;					/* Pusha */
	*--esp = 0;					/* Pusha */
	*--esp = (uint32)&RescheduleResume;  /* ResumptionPoint */
	
	
	fpu = proc->archproc.fpu_state;
	
	if (cpu_fxsr)
	{
		fpu->fxstate.control_word = 0x037f;
		fpu->fxstate.status_word = 0x0000;
		fpu->fxstate.tag_word = 0x0000;
		fpu->fxstate.fpu_ip_offset = 0x00000000;
		fpu->fxstate.fpu_ip_selector = 0x0000;
		fpu->fxstate.fpu_opcode = 0x00000000;
		fpu->fxstate.fpu_operand_offset = 0x00000000;
		fpu->fxstate.fpu_operand_selector = 0x0000;
		fpu->fxstate.mxcsr = 0;
		fpu->fxstate.mxcsr_mask = cpu_mxcsr_mask;
	}
	else
	{
		fpu->fstate.control_word = 0x037f;
		fpu->fstate.status_word = 0x0000;
		fpu->fstate.tag_word = 0xffff;
		fpu->fstate.fpu_ip_offset = 0x00000000;
		fpu->fstate.fpu_ip_selector = 0x0000;
		fpu->fstate.fpu_opcode = 0x00000000;
		fpu->fstate.fpu_operand_offset = 0x00000000;
		fpu->fstate.fpu_operand_selector = 0x0000;
	}

	
	
	proc->archproc.stack_pointer = (vm_addr)esp;
	return 0;
}





/*
 *
 */

int ArchInitUProc (struct Process *proc, vm_addr entry_point, vm_addr stack_pointer,
					vm_addr argv, int argc, vm_addr env, int envc)
{
	uint32 *esp;
	union I387_State *fpu;
	
	KPRINTF ("ArchInitUProc");
	
	
	/* Really should create a double register state  EntryExit_State + Reschedule_State
		Currently only Reschedule_State below */
	
	proc->archproc.resume_eip = 0;
	proc->archproc.resume_esp = 0;
	
	
	/* Entry/Exit State */
	esp = (uint32 *)((vm_addr)proc->kernel_stack + KERNEL_STACK_SZ);
	
	*--esp = (uint32)PL3_DATA_SEGMENT;
	*--esp = (uint32)stack_pointer;
	*--esp = EFLG_IF;
	*--esp = (uint32)PL3_CODE_SEGMENT;
	*--esp = (uint32)entry_point;
	
	
	/* Reschedule State */
	
	*--esp = (uint32)UProcStart;
	*--esp = EFLG_IF;
	*--esp = 0;					/* Push eax */
	*--esp = argv;				/* Push ecx */
	*--esp = argc;				/* Push edx */
	*--esp = 0;					/* Push ebx */
	*--esp = 0;					/* Push temp */
	*--esp = 0;					/* Push ebp */
	*--esp = env;				/* Push esi */
	*--esp = envc;				/* Push edi */
	*--esp = (uint32)&RescheduleResume;  /* ResumptionPoint */
	
	proc->archproc.stack_pointer = (vm_addr)esp;
	
	fpu = proc->archproc.fpu_state;
	
	if (cpu_fxsr)
	{
		fpu->fxstate.control_word = 0x037f;
		fpu->fxstate.status_word = 0x0000;
		fpu->fxstate.tag_word = 0x0000;
		fpu->fxstate.fpu_ip_offset = 0x00000000;
		fpu->fxstate.fpu_ip_selector = 0x0000;
		fpu->fxstate.fpu_opcode = 0x00000000;
		fpu->fxstate.fpu_operand_offset = 0x00000000;
		fpu->fxstate.fpu_operand_selector = 0x0000;
		fpu->fxstate.mxcsr = 0;
		fpu->fxstate.mxcsr_mask = cpu_mxcsr_mask;
	}
	else
	{
		fpu->fstate.control_word = 0x037f;
		fpu->fstate.status_word = 0x0000;
		fpu->fstate.tag_word = 0xffff;
		fpu->fstate.fpu_ip_offset = 0x00000000;
		fpu->fstate.fpu_ip_selector = 0x0000;
		fpu->fstate.fpu_opcode = 0x00000000;
		fpu->fstate.fpu_operand_offset = 0x00000000;
		fpu->fstate.fpu_operand_selector = 0x0000;
	}

	
	
	
	
	
	return 0;
}




/*
 * 
 */

void ArchPickProc (struct Process *proc)
{
	if (fpu_state_owner != NULL)
	{
		if (cpu_fxsr)
			asm ("fxsave (%0)" : : "r" (fpu_state_owner->archproc.fpu_state) : "memory" );
		else
			asm ("fnsave (%0)" : : "r" (fpu_state_owner->archproc.fpu_state) : "memory" );
	}
	
	fpu_state_owner = current_process;
	
	if (cpu_fxsr)
		asm ("fxrstor (%0)" : : "r" (current_process->archproc.fpu_state) : "memory" );
	else
		asm ("frstor (%0)" : : "r" (current_process->archproc.fpu_state) : "memory" );
	
	tss.esp0 = ((vm_addr)proc->kernel_stack + KERNEL_STACK_SZ);

	PmapSwitch (&proc->user_as->pmap);
}




/*
 *
 */

int ArchInitFork (struct Process *proc)
{
	uint32 *esp;
	vm_addr entry_point;
	vm_addr stack_pointer;
	uint32 *src_esp;
	int t;

	entry_point =  *(vm_addr *)((vm_addr)current_process->kernel_stack + KERNEL_STACK_SZ - 20);
	stack_pointer = *(vm_addr *)((vm_addr)current_process->kernel_stack + KERNEL_STACK_SZ - 8);
	
	proc->archproc.resume_eip = 0;
	proc->archproc.resume_esp = 0;
	
	esp = (uint32 *)((vm_addr)proc->kernel_stack + KERNEL_STACK_SZ);
	
	*--esp = (uint32)PL3_DATA_SEGMENT;
	*--esp = (uint32)stack_pointer;
	*--esp = EFLG_IF;
	*--esp = (uint32)PL3_CODE_SEGMENT;
	*--esp = (uint32)entry_point;
	*--esp = (uint32)ForkStart;
	*--esp = EFLG_IF;

	src_esp = (uint32 *)((vm_addr)current_process->kernel_stack + KERNEL_STACK_SZ - 28);
	
	for (t=0; t<8; t++)
	{
		*--esp = *--src_esp;
	}
	
	
	*--esp = (uint32)&RescheduleResume;  /* ResumptionPoint */
	proc->archproc.stack_pointer = (vm_addr)esp;
	

	if (cpu_fxsr)
		asm ("fxsave (%0)" : : "r" (current_process->archproc.fpu_state) : "memory" );
	else
		asm ("fnsave (%0)" : : "r" (current_process->archproc.fpu_state) : "memory" );

	MemCpy (proc->archproc.fpu_state, current_process->archproc.fpu_state, sizeof (union I387_State));
	
	
	
	return 0;
}









int ArchInitExec (struct Process *proc, vm_addr entry_point, vm_addr stack_pointer,
					vm_addr argv, int argc, vm_addr env, int envc)
{
	uint32 *esp;
	union I387_State *fpu;
		
	proc->archproc.resume_eip = 0;
	proc->archproc.resume_esp = 0;
	
		
	esp = (uint32 *)((vm_addr)proc->kernel_stack + KERNEL_STACK_SZ);
	
	*--esp = (uint32)PL3_DATA_SEGMENT;
	*--esp = (uint32)stack_pointer;
	*--esp = EFLG_IF;
	*--esp = (uint32)PL3_CODE_SEGMENT;
	*--esp = (uint32)entry_point;
	
	*--esp = 0; /* error-code */
	*--esp = 0; /* exception  */
	*--esp = 0; /* interrupt  */
	
	--esp; /* ds */
	--esp; /* es */
	--esp; /* fs */
	--esp; /* gs */
	
	*--esp = 0xdeadbeef;	/* ebp */
	*--esp = envc;			/* edi */
	*--esp = env;			/* esi */
	*--esp = argc;			/* edx */
	*--esp = argv;			/* ecx */
	*--esp = 0xcafefeed;	/* ebx */

	fpu = current_process->archproc.fpu_state;

	if (cpu_fxsr)
	{
		fpu->fxstate.control_word = 0x037f;
		fpu->fxstate.status_word = 0x0000;
		fpu->fxstate.tag_word = 0x0000;
		fpu->fxstate.fpu_ip_offset = 0x00000000;
		fpu->fxstate.fpu_ip_selector = 0x0000;
		fpu->fxstate.fpu_opcode = 0x00000000;
		fpu->fxstate.fpu_operand_offset = 0x00000000;
		fpu->fxstate.fpu_operand_selector = 0x0000;
		fpu->fxstate.mxcsr = 0;
		fpu->fxstate.mxcsr_mask = cpu_mxcsr_mask;
	}
	else
	{
		fpu->fstate.control_word = 0x037f;
		fpu->fstate.status_word = 0x0000;
		fpu->fstate.tag_word = 0xffff;
		fpu->fstate.fpu_ip_offset = 0x00000000;
		fpu->fstate.fpu_ip_selector = 0x0000;
		fpu->fstate.fpu_opcode = 0x00000000;
		fpu->fstate.fpu_operand_offset = 0x00000000;
		fpu->fstate.fpu_operand_selector = 0x0000;
	}
	
	if (cpu_fxsr)
		asm ("fxrstor (%0)" : : "r" (current_process->archproc.fpu_state) : "memory" );
	else
		asm ("frstor (%0)" : : "r" (current_process->archproc.fpu_state) : "memory" );	
	

	return 0;
}



