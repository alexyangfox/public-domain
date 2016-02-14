#include <kernel/types.h>
#include <kernel/i386/i386.h>
#include <kernel/i386/ucontext.h>
#include <kernel/proc.h>
#include <kernel/usignal.h>
#include <kernel/vm.h>
#include <kernel/dbg.h>




/*
 * SigReturn();
 */

void SigReturn (struct sigframe *u_sigframe, struct ContextState *state)
{
	struct sigframe sigframe;
	
	if (CopyIn (current_process->user_as, &sigframe, u_sigframe, sizeof (struct sigframe)) == 0)
	{		
		state->eax = sigframe.sf_uc.uc_mcontext.eax;
		state->ebx = sigframe.sf_uc.uc_mcontext.ebx;
		state->ecx = sigframe.sf_uc.uc_mcontext.ecx;
		state->edx = sigframe.sf_uc.uc_mcontext.edx;
		state->esi = sigframe.sf_uc.uc_mcontext.esi;
		state->edi = sigframe.sf_uc.uc_mcontext.edi;
		state->ebp = sigframe.sf_uc.uc_mcontext.ebp;
		
		state->gs = PL3_DATA_SEGMENT;
		state->fs = PL3_DATA_SEGMENT;
		state->es = PL3_DATA_SEGMENT;
		state->ds = PL3_DATA_SEGMENT;
		
		state->return_eip    = sigframe.sf_uc.uc_mcontext.eip;
		state->return_cs     = PL3_CODE_SEGMENT;
		
		state->return_eflags = (sigframe.sf_uc.uc_mcontext.eflags & ~EFLAGS_SYSTEM_MASK)
								| (state->return_eflags & EFLAGS_SYSTEM_MASK)
								| EFLG_IF;
		
		state->return_esp    = sigframe.sf_uc.uc_mcontext.esp;
		state->return_ss     = PL3_DATA_SEGMENT;
		
		
		
		
		/* We are always saving/changing FPU state on task switches, no deferring
			But code in PickProc() appears to defer.
		*/
		
		
		
		
		if (cpu_fxsr)
		{
			MemCpy (&current_process->archproc.fpu_state->fxstate,
				&sigframe.sf_uc.uc_mcontext.fpstate, sizeof (struct I387_FXSave));

			asm ("fxrstor (%0)" : : "r" (current_process->archproc.fpu_state) : "memory" );
		}
		else
		{
		MemCpy (&current_process->archproc.fpu_state->fstate,
				&sigframe.sf_uc.uc_mcontext.fpstate, sizeof (struct I387_FSave));
			asm ("frstor (%0)" : : "r" (current_process->archproc.fpu_state) : "memory" );
		}
				
		current_process->usignal.sig_mask = sigframe.sf_uc.uc_sigmask;
	}
	else
	{
		USigExit (SIGSEGV);
	}
}



/*
 * DeliverUserSignals();
 *
 * Called just before returning from a syscall, interrupt or exception by the
 * wrappers in the stubs.s file.  Only called if we are returning to PL3 and not
 * to virtual-86 mode.
 *
 * Interrupts are disabled for the duration of this function, though it should
 * only need slight modification to enable interrupts for most of it.
 *
 * USigExit() is responsible for enabling interrupts if a signal cannot be
 * delivered or it's default action is to kill the process.
 *
 * If we are returning from USigSuspend() then the process's current sigmask
 * will have been changed and the old mask saved.  We use the current sigmask
 * to determine what signal to deliver then reset it to the mask used prior
 * to calling USigSuspend().  See USigSuspend() for details.
 */

void DeliverUserSignals (struct ContextState *state)
{
	struct sigframe sigframe;
	struct sigframe *u_sigframe;
	uint32 safe_signals;
	uint32 sync_signals;
	uint32 old_mask;
	int sig;
	
	
	
	sync_signals = current_process->usignal.sig_pending & SYNCSIGMASK;

	if (sync_signals != 0 && (sync_signals & current_process->usignal.sig_mask) != 0)
	{
		sig = PickUSignal (sync_signals & ~current_process->usignal.sig_mask);
		USigExit (sig);
	}
		
	safe_signals = current_process->usignal.sig_pending & ~current_process->usignal.sig_mask;
		
	if (safe_signals)
	{	
		if (current_process->usignal.use_sigsuspend_mask == TRUE)
		{
			current_process->usignal.sig_mask = current_process->usignal.sigsuspend_oldmask;
			current_process->usignal.use_sigsuspend_mask = FALSE;
		}
			
		if (safe_signals & SIGFKILL)
		{
			USigExit (SIGKILL);	
		}
		else if (safe_signals & SIGFCONT)
		{
			KPRINTF ("Delivering SIGFCONT");
		}
		else
		{
			sig = PickUSignal (safe_signals);
				
			current_process->usignal.sig_pending &= ~SIGBIT(sig);
			
			old_mask = current_process->usignal.sig_mask;
			
			if (current_process->usignal.sig_nodefer & SIGBIT(sig))
				current_process->usignal.sig_mask |= ~current_process->usignal.handler_mask[sig-1];
			else
				current_process->usignal.sig_mask |= SIGBIT(sig) | ~current_process->usignal.handler_mask[sig-1];
			
			
			if (current_process->usignal.trampoline == NULL)
			{
				DoUSignalDefault (sig);
			}
			else if (current_process->usignal.handler[sig-1] == SIG_DFL)
			{			
				DoUSignalDefault (sig);
				
				current_process->usignal.sig_mask = old_mask;
			}
			else if (current_process->usignal.handler[sig-1] == SIG_IGN)
			{
				current_process->usignal.sig_mask = old_mask;

				if (current_process->usignal.sig_resethand & SIGBIT(sig))
					current_process->usignal.handler[sig-1] = SIG_DFL;
			}
			else
			{
				sigframe.sf_uc.uc_mcontext.eax = state->eax;
				sigframe.sf_uc.uc_mcontext.ebx = state->ebx;
				sigframe.sf_uc.uc_mcontext.ecx = state->ecx;
				sigframe.sf_uc.uc_mcontext.edx = state->edx;
				sigframe.sf_uc.uc_mcontext.esi = state->esi;
				sigframe.sf_uc.uc_mcontext.edi = state->edi;
				sigframe.sf_uc.uc_mcontext.ebp = state->ebp;
				
				sigframe.sf_uc.uc_mcontext.ds = PL3_DATA_SEGMENT;
				sigframe.sf_uc.uc_mcontext.es = PL3_DATA_SEGMENT;
				sigframe.sf_uc.uc_mcontext.fs = PL3_DATA_SEGMENT;
				sigframe.sf_uc.uc_mcontext.gs = PL3_DATA_SEGMENT;
				
				sigframe.sf_uc.uc_mcontext.eip    = state->return_eip;
				sigframe.sf_uc.uc_mcontext.cs     = PL3_CODE_SEGMENT;
				sigframe.sf_uc.uc_mcontext.eflags = state->return_eflags & ~EFLAGS_SYSTEM_MASK;
				sigframe.sf_uc.uc_mcontext.esp    = state->return_esp;
				sigframe.sf_uc.uc_mcontext.ss     = PL3_DATA_SEGMENT;
				
				sigframe.sf_uc.uc_sigmask = old_mask;
				
				
				if (cpu_fxsr)
				{
					asm ("fxsave (%0)" : : "r" (current_process->archproc.fpu_state) : "memory" );
				
					MemCpy (&sigframe.sf_uc.uc_mcontext.fpstate,
							&current_process->archproc.fpu_state->fxstate,
							 sizeof (struct I387_FXSave));

					MemSet (&current_process->archproc.fpu_state->fxstate, 0, sizeof (struct I387_FXSave));
				
					current_process->archproc.fpu_state->fxstate.control_word = 0x037f;
					current_process->archproc.fpu_state->fxstate.status_word = 0x0000;
					current_process->archproc.fpu_state->fxstate.tag_word = 0x0000;
					current_process->archproc.fpu_state->fxstate.mxcsr = 0;
					current_process->archproc.fpu_state->fxstate.mxcsr_mask = cpu_mxcsr_mask;
				
					asm ("fxrstor (%0)" : : "r" (current_process->archproc.fpu_state) : "memory" );
				}
				else
				{
					asm ("fnsave (%0)" : : "r" (current_process->archproc.fpu_state) : "memory" );
					
					MemCpy (&sigframe.sf_uc.uc_mcontext.fpstate,
							&current_process->archproc.fpu_state->fstate,
							 sizeof (struct I387_FSave));

					MemSet (&current_process->archproc.fpu_state->fstate, 0, sizeof (struct I387_FSave));

					current_process->archproc.fpu_state->fstate.control_word = 0x037f;
					current_process->archproc.fpu_state->fstate.status_word = 0x0000;
					current_process->archproc.fpu_state->fstate.tag_word = 0xffff;
					
					asm ("frstor (%0)" : : "r" (current_process->archproc.fpu_state) : "memory" );
				}
								
				
				u_sigframe = (struct sigframe *) ALIGN_DOWN (state->return_esp - sizeof (struct sigframe), 16);
				sigframe.sf_signum = sig;
				
				if (current_process->usignal.sig_info & SIGBIT(sig))
					sigframe.sf_siginfo = &u_sigframe->sf_si;
				else
					sigframe.sf_siginfo = NULL;
				
				sigframe.sf_ucontext = &u_sigframe->sf_uc;
				sigframe.sf_ahu.sf_action = current_process->usignal.handler[sig-1];
												
				if (current_process->usignal.sig_resethand & SIGBIT(sig))
					current_process->usignal.handler[sig-1] = SIG_DFL;

				if (CopyOut (current_process->user_as, u_sigframe, &sigframe, sizeof (struct sigframe)) == 0)
				{		
					state->return_esp = (uint32) u_sigframe;
					state->return_eip = (uint32) current_process->usignal.trampoline;
				}
				else
				{
					USigExit (SIGSEGV);
				}
			}
		}
	}
}




/*
 * PickUSignal();
 */

int PickUSignal (uint32 sigbits)
{
	int sig;

	for (sig = 0; sig < 32; sig++)
		if ((1<<sig) & sigbits)
			break;

	return sig + 1;
}





