#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/i386/i386.h>
#include <kernel/dbg.h>
#include <kernel/utility.h>
#include <kernel/proc.h>
#include <kernel/config.h>
#include "v86.h"




/*
 * V86Spawn();
 */

struct Process *V86Spawn (void)
{
	struct Process *newproc;

	
	if ((newproc = AllocProcess (PROC_TYPE_KERNEL)) != NULL)
	{
		newproc->priority = 0;
		newproc->uid = 0;
		newproc->gid = 0;
		newproc->euid = 0;
		newproc->egid = 0;
		newproc->pgrp = 0;
		newproc->umask = 0;
	
		StrLCpy (newproc->exe_name, "v86", PROC_NAME_SZ);
	
		if (CreateV86AddressSpace (&newproc->as) == 0)
		{	
			newproc->user_as = &newproc->as;
			
			if (0 == ArchInitV86Proc (newproc))
			{	
				DisableInterrupts();
				newproc->state = PROC_STATE_READY;
				SchedReady (newproc);
				Reschedule();
				EnableInterrupts();
				
				return newproc;
			}
			
			FreeAddressSpace (&newproc->as);
		}
		FreeProcess (newproc);
	}
	
	return NULL;
}




/*
 *
 */

int CreateV86AddressSpace (struct AddressSpace *as)
{
	MutexLock (&vm_mutex);
	
	if (PmapV86Init(as) == TRUE)
	{
		LIST_INIT (&as->sorted_memregion_list);
		LIST_INIT (&as->free_memregion_list);
		as->hint = NULL;
		as->active = TRUE;
	
		MutexUnlock (&vm_mutex);
		return 0;
	}
		
	MutexUnlock (&vm_mutex);
	return -1;
}




/*
 *
 */

bool PmapV86Init (struct AddressSpace *as)
{
	uint32 *kernel_pd;
	uint32 *pd;
	uint32 *pt;
	uint32 *src_pt;
	uint32 pde_idx;
	uint32 pte_idx;
	vm_addr pa;
	
	
	kernel_pd = kernel_as.pmap.page_directory;
	
	if ((pd = PmapAllocPagetable (&as->pmap, PMAP_WIRED)) != NULL)
	{
		if ((pt = PmapAllocPagetable (&as->pmap, PMAP_WIRED)) != NULL)
		{
			as->pmap.page_directory = pd;

			for (pte_idx = 0, pa = 0x00000000; pte_idx < 256; pte_idx++, pa += PAGE_SIZE)
				*(pt + pte_idx) = (pa & PTE_ADDR_MASK) | PG_USER | PG_PRESENT | PG_READWRITE;
			
			src_pt = (uint32 *)(*(kernel_pd + 0) & PDE_ADDR_MASK);
			
			for (pte_idx = 256; pte_idx < 1024; pte_idx++)
				*(pt + pte_idx) = *(src_pt + pte_idx);

			*(pd + 0) = ((uint32)pt & PDE_ADDR_MASK) | PG_USER | PG_READWRITE | PG_PRESENT;

			for (pde_idx = VM_KERNEL_PDE_BASE +1; pde_idx < VM_KERNEL_PDE_CEILING; pde_idx++)
				*(pd + pde_idx) = *(kernel_pd + pde_idx);
			
			return TRUE;
		}
		
		PmapFreePagetable (&as->pmap, pd);
	}

	return FALSE;
}




/*
 * ArchInitV86Proc();
 */

int ArchInitV86Proc (struct Process *proc)
{
	uint32 *esp;
	union I387_State *fpu;
	

	proc->archproc.resume_eip = 0;
	proc->archproc.resume_esp = 0;
	
	esp = (uint32 *)((vm_addr)proc->kernel_stack + KERNEL_STACK_SZ);
	
	*--esp = 0x1000;   /* v86 gs */
	*--esp = 0x1000;   /* v86 fs */
	*--esp = 0x1000;   /* v86 es */
	*--esp = 0x1000;   /* v86 ds */
	
	*--esp = (uint32)PL3_DATA_SEGMENT;
	*--esp = (uint32)0xdeadf00d;
	*--esp = EFLG_IF;
	*--esp = (uint32)PL3_CODE_SEGMENT;
	*--esp = (uint32)0x1ee7f001;
	
	/* Interrupt/Exception codes */
	
	*--esp = 0;
	*--esp = 0;
	*--esp = 0;	
	
	/* Segment registers here */

	*--esp = 0;
	*--esp = 0;
	*--esp = 0;
	*--esp = 0;
	
	/* General Registers here */

	*--esp = 0xdead0000;	/* ebp */
	*--esp = 0xdead0001;	/* edi */
	*--esp = 0xdead0002;	/* esi */
	*--esp = 0xdead0003;	/* edx */
	*--esp = 0xdead0004;	/* ecx */
	*--esp = 0xdead0005;	/* ebx */
	*--esp = 0xdead0006;	/* eax */
	
	
		
	/* Reschedule State */
		
	*--esp = (uint32)V86ProcStart;     /* V86ProcStart */
	*--esp = EFLG_IF;
	*--esp = 0;				/* Push eax */
	*--esp = 0;				/* Push ecx */
	*--esp = 0;				/* Push edx */
	*--esp = 0;				/* Push ebx */
	*--esp = 0;				/* Push temp */   
	*--esp = 0;				/* Push ebp */
	*--esp = 0;				/* Push esi */
	*--esp = 0;				/* Push edi */
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

