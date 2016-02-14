#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/error.h>
#include <kernel/dbg.h>
#include <kernel/timer.h>
#include <kernel/i386/i386.h>




/*
 *
 */

void PrintException (struct ContextState *es, char *str, int panic);



/*
 * Double-Fault handler.  Runs on it's own TSS and stack.  Reads the
 * contents of the main TSS and prints to screen before it panics.
 */

void DoubleFaultHandler (void)
{
	KPRINTF("return_cs     = %#010x   return_eip = %#010x", tss.cs, tss.eip);
	KPRINTF("return_ss     = %#010x   return_esp = %#010x", tss.ss, tss.esp);
	KPRINTF("return_eflags = %#010x", tss.eflags);
	KPRINTF("EAX = %#010x   ECX = %#010x", tss.eax, tss.ecx);
	KPRINTF("EDX = %#010x   EBX = %#010x", tss.edx, tss.ebx);
	KPRINTF("EBP = %#010x   ", tss.ebp);
	KPRINTF("ESI = %#010x   EDI = %#010x", tss.esi, tss.edi);
	KPRINTF ("EXCEPTION : DOUBLE FAULT");
	KPanic();

	while (1);
}




/*
 * DispatchException();
 *
 * if Exception in kernel then
 *    if Exception == PAGEFAULT && .......
 *		  Restore to Catch point
 *	  else
 *		  Kernel Panic()
 * else if exception == GP && umode = v86
 *     call v86_handler
 * else
 *     Convert to Signal + SigInfo
 */

void DispatchException (struct ContextState *es)
{
	int privilege_level;
	int pf_direction;
	vm_addr pf_addr;
	
		
	if (isr_depth > 0)
		PrintException (es, "Exception in interrupt Handler", TRUE);


	if (es->return_eflags & EFLG_VM)
	{
		KPANIC ("V86 Exception");
	
		switch (es->exception)
		{
			case EXCEPTION_GP:
				EnableInterrupts();
				V86GPHandler (es);
				DisableInterrupts();
				break;
			
			default:
				PrintException (es, "#GP in V86", TRUE); 
			
		}
	}
	else if ((privilege_level = es->return_cs & 0x03) == 0)
	{	
		switch (es->exception)
		{
			case EXCEPTION_PF:
				pf_addr = GetCR2();
				
				if ((es->error_code & PF_ERRORCODE_DIRECTION) == 0)
					pf_direction = PF_DIR_READ;
				else
					pf_direction = PF_DIR_WRITE;
				
				EnableInterrupts();
					
				if (PageFault (pf_addr, pf_direction, privilege_level) != 0)
				{
					if (current_process->archproc.resume_eip != 0 &&
						current_process->archproc.resume_esp != 0)
					{
						es->return_eip = current_process->archproc.resume_eip;
						es->return_esp = current_process->archproc.resume_esp;
						current_process->archproc.resume_eip = 0;
						current_process->archproc.resume_esp = 0;
					}
					else
					{
						PrintException (es, "#PF in Kernel", TRUE);
					}
				}
				
				DisableInterrupts();
				
				break;
				
			default:
				PrintException (es, "Exception in Kernel", TRUE);
				
		}
	}
	else
	{
		switch (es->exception)
		{
			case EXCEPTION_DE:	/* Divide Error */
				current_process->usignal.sig_pending |= SIGFFPE;
				current_process->usignal.siginfo_data[SIGFPE-1].si_signo = SIGFPE;
				current_process->usignal.siginfo_data[SIGFPE-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGFPE-1].si_value.sival_int = 0;
				break;
			case EXCEPTION_DB:  /* Debug */
				current_process->usignal.sig_pending |= SIGFTRAP;
				current_process->usignal.siginfo_data[SIGTRAP-1].si_signo = SIGTRAP;
				current_process->usignal.siginfo_data[SIGTRAP-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGTRAP-1].si_value.sival_int = 0;
				break;
			case EXCEPTION_NMI: /* Non-Maskable Interupt */
				break;
			case EXCEPTION_BP:  /* Breakpoint */
				current_process->usignal.sig_pending |= SIGFTRAP;
				current_process->usignal.siginfo_data[SIGTRAP-1].si_signo = SIGTRAP;
				current_process->usignal.siginfo_data[SIGTRAP-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGTRAP-1].si_value.sival_int = 0;
				break;
			case EXCEPTION_OF:  /* Overflow */
				current_process->usignal.sig_pending |= SIGFSEGV;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_signo = SIGSEGV;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_value.sival_int = 0;
				break;
			case EXCEPTION_BR:  /* Boundary-Range Exceeded */
				current_process->usignal.sig_pending |= SIGFSEGV;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_signo = SIGSEGV;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_value.sival_int = 0;
				break;
			case EXCEPTION_UD:  /* Undefined-Opcode */
				current_process->usignal.sig_pending |= SIGFILL;
				current_process->usignal.siginfo_data[SIGILL-1].si_signo = SIGILL;
				current_process->usignal.siginfo_data[SIGILL-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGILL-1].si_value.sival_int = 0;
				break;
			case EXCEPTION_NM:  /* Device-Not Available */
				current_process->usignal.sig_pending |= SIGFSEGV;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_signo = SIGSEGV;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_value.sival_int = 0;
				break;
			case EXCEPTION_DF:  /* Double-Fault */
				KPANIC ("DF");				
				break;
			case EXCEPTION_CSO: /* Reserved : Coprocessor Segment Overrun */
				current_process->usignal.sig_pending |= SIGFFPE;
				current_process->usignal.siginfo_data[SIGFPE-1].si_signo = SIGFPE;
				current_process->usignal.siginfo_data[SIGFPE-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGFPE-1].si_value.sival_int = 0;
				break;
			case EXCEPTION_TS:  /* Invalid TSS */
				current_process->usignal.sig_pending |= SIGFSEGV;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_signo = SIGSEGV;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_value.sival_int = 0;
				break;
			case EXCEPTION_NP:  /* Segment / Not Present */
				current_process->usignal.sig_pending |= SIGFBUS;
				current_process->usignal.siginfo_data[SIGBUS-1].si_signo = SIGBUS;
				current_process->usignal.siginfo_data[SIGBUS-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGBUS-1].si_value.sival_int = 0;
				break;
			case EXCEPTION_SS:  /* Stack Segment */
				current_process->usignal.sig_pending |= SIGFBUS;
				current_process->usignal.siginfo_data[SIGBUS-1].si_signo = SIGBUS;
				current_process->usignal.siginfo_data[SIGBUS-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGBUS-1].si_value.sival_int = 0;
				break;
			case EXCEPTION_GP:  /* General Protection */
				current_process->usignal.sig_pending |= SIGFSEGV;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_signo = SIGSEGV;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_value.sival_int = 0;
				break;
			case EXCEPTION_PF:
				pf_addr = GetCR2();
			
				if ((es->error_code & PF_ERRORCODE_DIRECTION) == 0)
					pf_direction = PF_DIR_READ;
				else
					pf_direction = PF_DIR_WRITE;
				
				EnableInterrupts();
					
				if (PageFault (pf_addr, pf_direction, privilege_level) != 0)
				{
					current_process->usignal.sig_pending |= SIGFSEGV;
					current_process->usignal.siginfo_data[SIGSEGV-1].si_signo = SIGSEGV;
					current_process->usignal.siginfo_data[SIGSEGV-1].si_code = 0;
					current_process->usignal.siginfo_data[SIGSEGV-1].si_value.sival_int = 0;
					
					PrintException (es, "Page Fault in PL3", FALSE);
				}
			
				DisableInterrupts();
			
				break;
			case EXCEPTION_RESV: /* Reserved */
				break;
			case EXCEPTION_MF:  /* Math Fault */
				current_process->usignal.sig_pending |= SIGFFPE;
				current_process->usignal.siginfo_data[SIGFPE-1].si_signo = SIGFPE;
				current_process->usignal.siginfo_data[SIGFPE-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGFPE-1].si_value.sival_int = 0;
				break;
			case EXCEPTION_AC:  /* Alignment Check */
				current_process->usignal.sig_pending |= SIGFSEGV;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_signo = SIGSEGV;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_value.sival_int = 0;
				break;
			case EXCEPTION_MC:  /* Machine Check */
				break;
			case EXCEPTION_XF:  /* Extended Math Fault */
				current_process->usignal.sig_pending |= SIGFFPE;
				current_process->usignal.siginfo_data[SIGFPE-1].si_signo = SIGFPE;
				current_process->usignal.siginfo_data[SIGFPE-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGFPE-1].si_value.sival_int = 0;
				break;
			default:            /* Unknown Exception */
				current_process->usignal.sig_pending |= SIGFSEGV;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_signo = SIGSEGV;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_code = 0;
				current_process->usignal.siginfo_data[SIGSEGV-1].si_value.sival_int = 0;
				break;
		}
	}
}








/*
 * Strings for exceptions
 */

char *exception_string[32] =
{
	"DE", "DB",  "NMI",	"BP", "OF", "BR", "UD", "FPU",
	"DF", "CSO", "TS",	"NP", "SS", "GP", "PF", "RESV",
	"MF", "AC",  "MC",  "XF", "20", "21", "22", "23",
	"24", "25",  "26",  "27", "28", "29", "30", "31"
};
	



/*
 * Only do Exit() if the exception instruction was in user-space and it was a user-process
 * Could also check eip is between user-space bounds?
 *
 * Otherwise it is an exception in the kernel (either kproc or uproc)
 */

void PrintException (struct ContextState *es, char *str, int panic)
{
	int privilege;
	
	privilege = es->return_cs & 0x03;
		
	if (privilege != 0)
	{
		KPRINTF("return_ss     = %#010x   return_esp = %#010x", es->return_ss, es->return_esp);
	}

	KPRINTF("return_eflags = %#010x          cr2 = %#010x", es->return_eflags, GetCR2());
	KPRINTF("return_cs     = %#010x   return_eip = %#010x", es->return_cs, es->return_eip);
	KPRINTF("error_code    = %#010x    exception = %#010x", es->error_code, es->exception);
	
	KPRINTF("EAX = %#010x   ECX = %#010x", es->eax, es->ecx);
	KPRINTF("EDX = %#010x   EBX = %#010x", es->edx, es->ebx);
	KPRINTF("EBP = %#010x   ", es->ebp);
	KPRINTF("ESI = %#010x   EDI = %#010x", es->esi, es->edi);
	
	KPRINTF ("EXCEPTION = %d #%s name = %s",  es->exception, exception_string[es->exception % 32],
				current_process->exe_name);
	
	KPRINTF ("pid = %d, Name = %s", current_process->pid, current_process->exe_name);
	
	if (panic == TRUE)
		KPANIC (str);
}





