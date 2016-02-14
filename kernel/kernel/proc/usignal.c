#include <kernel/types.h>
#include <kernel/usignal.h>
#include <kernel/i386/ucontext.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/timer.h>
#include <kernel/utility.h>
#include <kernel/error.h>




/*
 * Array defining the default actions and properties of each usignal.
 */

const uint32 sigprop[NSIG] = 
{
	0,								/* FIX: Added dummy 1st Oct */
	SP_KILL,                        /* 1  [ 0] SIGHUP    */
	SP_KILL,                        /* 2  [ 1] SIGINT    */
	SP_KILL|SP_CORE,                /* 3  [ 2] SIGQUIT   */
	SP_KILL|SP_CORE|SP_NORESET,     /* 4  [ 3] SIGILL    */
	SP_KILL|SP_CORE|SP_NORESET,     /* 5  [ 4] SIGTRAP   */
	SP_KILL|SP_CORE,                /* 6  [ 5] SIGABRT   */
	SP_KILL|SP_CORE,                /* 7  [ 6] SIGEMT    */
	SP_KILL|SP_CORE,                /* 8  [ 7] SIGFPE    */
	SP_KILL|SP_CANTMASK,            /* 9  [ 8] SIGKILL   */
	SP_KILL|SP_CORE,                /* 10 [ 9] SIGBUS    */
	SP_KILL|SP_CORE,                /* 11 [10] SIGSEGV   */
	SP_KILL|SP_CORE,                /* 12 [11] SIGSYS    */
	SP_KILL,                        /* 13 [12] SIGPIPE   */
	SP_KILL,                        /* 14 [13] SIGALRM   */
	SP_KILL,                        /* 15 [14] SIGTERM   */
	0,                              /* 16 [15] SIGURG    */
	SP_STOP|SP_CANTMASK,            /* 17 [16] SIGSTOP   */
	SP_STOP|SP_TTYSTOP,             /* 18 [17] SIGTSTP   */
	SP_CONT,                        /* 19 [18] SIGCONT   */
	0,                              /* 20 [19] SIGCHLD   */
	SP_STOP|SP_TTYSTOP,             /* 21 [20] SIGTTIN   */
	SP_STOP|SP_TTYSTOP,             /* 22 [21] SIGTTOU   */
	0,                              /* 23 [22] SIGIO     */
	SP_KILL,                        /* 24 [23] SIGXCPU   */
	SP_KILL,                        /* 25 [24] SIGXFSZ   */
	SP_KILL,                        /* 26 [25] SIGVTALRM */
	SP_KILL,                        /* 27 [26] SIGPROF   */
	0,					            /* 28 [27] SIGWINCH  */
	0,						        /* 29 [28] SIGINFO   */
	SP_KILL,						/* 30 [29] SIGUSR1   */
	SP_KILL,						/* 31 [30] SIGUSR2   */
	SP_NORESET                      /* 32 [31] SIGPWR    */
};




/*
 * USigFork()
 *
 * Called by DoFork()
 */
 
void USigFork (struct Process *src, struct Process *dst)
{
	int t;
	
	for (t=0; t<NSIG-1; t++)
	{
		dst->usignal.handler[t] = src->usignal.handler[t];
		dst->usignal.handler_mask[t] = src->usignal.handler_mask[t];
		
		dst->usignal.siginfo_data[t].si_signo = 0;
		dst->usignal.siginfo_data[t].si_code = 0;
		dst->usignal.siginfo_data[t].si_value.sival_int = 0;
	}
	

	dst->usignal.sig_info      = src->usignal.sig_info;
	dst->usignal.sig_mask      = src->usignal.sig_mask;
	dst->usignal.sig_pending   = 0;
	dst->usignal.sig_resethand = src->usignal.sig_resethand;
	dst->usignal.sig_nodefer   = src->usignal.sig_nodefer;
	dst->usignal.trampoline    = src->usignal.trampoline;
	
	dst->usignal.sigsuspend_oldmask = 0;
	dst->usignal.use_sigsuspend_mask = FALSE;
	
	/* Clear proc->pending_signal SIGF_USER */
}




/*
 * USigClear()
 *
 * Called by KCreateProcess()
 */

void USigInit (struct Process *dst)
{
	int t;
	
	for (t=0; t<NSIG-1; t++)
	{
		dst->usignal.handler[t] = SIG_DFL;
		dst->usignal.handler_mask[t] = 0x00000000;
		
		dst->usignal.siginfo_data[t].si_signo = 0;
		dst->usignal.siginfo_data[t].si_code = 0;
		dst->usignal.siginfo_data[t].si_value.sival_int = 0;
	}
	
	dst->usignal.sig_info      = 0x00000000;
	dst->usignal.sig_mask      = 0x00000000;
	dst->usignal.sig_pending   = 0x00000000;
	dst->usignal.sig_resethand = 0x00000000;
	dst->usignal.sig_nodefer   = 0x00000000;
	dst->usignal.trampoline    = NULL;
	
	
	dst->usignal.sigsuspend_oldmask = 0;
	dst->usignal.use_sigsuspend_mask = FALSE;
	
	/* Clear proc->pending_signal SIGF_USER */
}



void USigExec (struct Process *dst)
{
	int t;
	
	for (t=0; t<NSIG-1; t++)
	{
		if (dst->usignal.handler[t] != SIG_IGN)
			dst->usignal.handler[t] = SIG_DFL;

		dst->usignal.handler_mask[t] = 0x00000000;
		
		dst->usignal.siginfo_data[t].si_signo = 0;
		dst->usignal.siginfo_data[t].si_code = 0;
		dst->usignal.siginfo_data[t].si_value.sival_int = 0;
	}
	
	dst->usignal.sig_info      = 0x00000000;
	dst->usignal.sig_resethand = 0x00000000;
	dst->usignal.sig_nodefer   = 0x00000000;
	dst->usignal.trampoline    = NULL;


	dst->usignal.sigsuspend_oldmask = 0;
	dst->usignal.use_sigsuspend_mask = FALSE;
	
	
	
	/* No, they remain the same.  Only when you execve(2) a new process they will 
	be reset to the default action.  But ignored signals (SIG_IGN) remain 
	ignored.  Also, the signal mask remains the same.
	*/
	
/*	if (dst->sig_pending & dst->sig) */
	

	/* Any pending signals set SIGF_USER */
}



/*
 *
 */
 
void USigSetTrampoline (void (*func)(void))
{
	KPRINTF ("USigSetTrampoline()");
	current_process->usignal.trampoline = func;
}




/*
 *
 */

int USigAction (int signal, const struct sigaction *act_in, struct sigaction *oact_out)
{
	struct sigaction act, oact;
	
	KPRINTF ("USigAction()");

	if (signal > 0 || signal < NSIG)
	{
		if (oact_out != NULL)
		{
			if (current_process->usignal.sig_info &= SIGBIT(signal))
				oact.sa_flags |= SA_SIGINFO;
				
			oact._signal_handlers._handler = (void *) current_process->usignal.handler[signal-1];
			oact.sa_mask = current_process->usignal.handler_mask[signal-1];
	
			CopyOut (current_process->user_as, oact_out, &oact, sizeof (struct sigaction));
		}
		
		if (act_in != NULL)
		{
			if (CopyIn (current_process->user_as, &act, act_in, sizeof (struct sigaction)) == 0)
			{
				if (act.sa_flags & SA_SIGINFO)
					current_process->usignal.sig_info |= SIGBIT(signal);
				
				if (act.sa_flags & SA_NODEFER)
					current_process->usignal.sig_nodefer |= SIGBIT(signal);
	
				if (act.sa_flags & SA_RESETHAND)
					current_process->usignal.sig_resethand |= SIGBIT(signal);
				
				if (signal != SIGKILL && signal != SIGSTOP)
				{
					current_process->usignal.handler[signal-1] = (void *) act._signal_handlers._handler;
					current_process->usignal.handler_mask[signal-1] = act.sa_mask;
					current_process->usignal.handler_mask[signal-1] &= ~(SIGFKILL | SIGFSTOP);
				}
				else
				{
					SetError (EINVAL);
					return -1;
				}
			}
			else
				return -1;
		}
		
		return 0;
	}
	else
	{
		SetError (EINVAL);
		return -1;
	}	
}




/*
 *
 */

int UKill (int pid, int signal)
{
	struct Process *proc;
	int rc = 0;	
	int t;

	KPRINTF ("UKill (pid = %d, sig = %d)", pid, signal);

	MutexLock (&proc_mutex);
	

	if (signal == 0)
	{
		rc = -1;
	}
	else if (signal < 0 || signal >= NSIG)
	{
		rc = -1;
	}
	else if (pid == 0)
	{
	}
	else if (pid > 0)
	{
		proc = PIDtoProc (pid);

		if (proc->state != PROC_STATE_UNALLOC)
		{
			DisableInterrupts();
			
			if (proc->usignal.handler[signal-1] != SIG_IGN)
			{
				proc->usignal.sig_pending |= SIGBIT (signal);
				proc->pending_signals |= SIGF_USER;
		
				proc->usignal.siginfo_data[signal-1].si_signo = signal;
				proc->usignal.siginfo_data[signal-1].si_code = 0;
				proc->usignal.siginfo_data[signal-1].si_value.sival_int = 0;

				if (proc->state == PROC_STATE_WAIT_BLOCKED && (proc->waiting_for_signals & proc->pending_signals))
				{				
					proc->state = PROC_STATE_READY;
					SchedReady (proc);
					Reschedule();
				}
			}
							
			EnableInterrupts();
		}
		else
		{
			rc = -1;
			SetError (EINVAL);
		}
	}
	else
	{
		pid = -pid;
		
		for (t=0;t < process_cnt; t++)
		{
			proc = &process[t];

			DisableInterrupts();

			if (proc->state != PROC_STATE_UNALLOC)
			{
				if (proc->pgrp == pid) /* FIX: WAS && proc->pid != pid) */
				{
					if (proc->usignal.handler[signal-1] != SIG_IGN)
					{
						proc->usignal.sig_pending |= SIGBIT(signal);
						proc->pending_signals |= SIGF_USER;
	
						proc->usignal.siginfo_data[signal-1].si_signo = signal;
						proc->usignal.siginfo_data[signal-1].si_code = 0;
						proc->usignal.siginfo_data[signal-1].si_value.sival_int = 0;
						
						if (proc->state == PROC_STATE_WAIT_BLOCKED && (proc->waiting_for_signals & proc->pending_signals))
						{	
							proc->state = PROC_STATE_READY;
							SchedReady (proc);
							Reschedule();
						}
					}
				}
			}
			
			EnableInterrupts();
		}
	}

		
	MutexUnlock (&proc_mutex);
		
	return rc;
}




/*
 * USigSuspend();
 *
 * Waits for a user-signal to occur,  mask_in controls which signals are to be
 * accepted (0) or ignored (1).  The previous state of the signal mask is stored
 * in usignal.sigsuspend_oldmask and a flag, use_sigsuspend_mask is set to indicate
 * that we are returning from USigSuspend().
 *
 * A call to DeliverUserSignals() occurs before all system calls complete.  This
 * checks the use_sigsuspend_mask find out if we are returning from a USigSuspend()
 * system call.  If we are returning from USigSuspend() it restores the the sig_mask
 * to sigsuspend_oldmask _after_ choosing what signal to deliver based on the current
 * mask that was set in USigSuspend().
 */

int USigSuspend (const sigset_t *mask_in)
{
	sigset_t mask;
	uint32 intstate;
	
	KPRINTF ("USigSuspend()");
	
	if (CopyIn (current_process->user_as, &mask, mask_in, sizeof (sigset_t)) == 0)
	{
		intstate = DisableInterrupts();

		current_process->usignal.sigsuspend_oldmask = current_process->usignal.sig_mask;
		current_process->usignal.use_sigsuspend_mask = TRUE;
		
		current_process->usignal.sig_mask = mask;

		if ((current_process->usignal.sig_pending & ~current_process->usignal.sig_mask) == 0)
		{
			KWait (SIGF_USER);
		}
		
		RestoreInterrupts (intstate);	
	}
	else
	{
		SetError (EINVAL);
	}
	
	return -1;
}




/*
 *
 */

int USigProcMask (int how, const sigset_t *set_in, sigset_t *oset_out)
{
	sigset_t set;
	
	KPRINTF ("USigProcMask()");
	
	if (oset_out != NULL)
		CopyOut (current_process->user_as, oset_out, &current_process->usignal.sig_mask, sizeof (sigset_t));

	if (set_in != NULL)
	{
		if (CopyIn (current_process->user_as, &set, set_in,	sizeof (sigset_t)) == 0)
		{
			switch (how)
			{
				case SIG_SETMASK:
					current_process->usignal.sig_mask = set;
					return 0;
				
				case SIG_BLOCK:
					current_process->usignal.sig_mask |= set;
					return 0;
				
				case SIG_UNBLOCK:
					current_process->usignal.sig_mask &= ~set;
					return 0;
				
				default:
					return -1;
			}
		}
		else
			return -1;
	}
	
	return 0;
}




/*
 *
 */

int USigPending (sigset_t *set_out)
{
	sigset_t set;
	
	KPRINTF ("USigPending()");
		
	set = current_process->usignal.sig_pending & ~current_process->usignal.sig_mask;
	
	
	KPRINTF ("%#010x = USigPending()", set);
	
	CopyOut (current_process->user_as, set_out, &set, sizeof (sigset_t));
	
	return 0;
}




/*
 * DoUSignalDefault (int sig)
 */
 
void DoUSignalDefault (int sig)
{
	if (sigprop[sig] & SP_KILL)
	{
		KPRINTF ("DoSignalDefault() KILL **************************");
		USigExit (sig);
	}
	else
	{
		KPRINTF ("DoSignalDefault() IGNORE ************************");
		return;
	}
}






/*
 * KillUserProcesses();
 */
 
void KillUserProcesses (void)
{
	struct Process *proc;
	int t;

	KPRINTF ("KillUserProcesses() cnt = %d", user_process_cnt);


	MutexLock (&proc_mutex);

	for (t=0;t < process_cnt; t++)
	{
		proc = &process[t];
	
		DisableInterrupts();
	
		if (proc->state != PROC_STATE_UNALLOC && proc->type == PROC_TYPE_USER)
		{
			proc->usignal.sig_pending |= SIGBIT (SIGKILL);
			proc->pending_signals |= SIGF_USER;

			proc->usignal.siginfo_data[SIGKILL-1].si_signo = SIGKILL;
			proc->usignal.siginfo_data[SIGKILL-1].si_code = 0;
			proc->usignal.siginfo_data[SIGKILL-1].si_value.sival_int = 0;
			
			if (proc->state == PROC_STATE_WAIT_BLOCKED && (proc->waiting_for_signals & proc->pending_signals))
			{	
				proc->state = PROC_STATE_READY;
				SchedReady (proc);
				Reschedule();
			}
		}
		
		EnableInterrupts();
	}
	
	
		
	MutexUnlock (&proc_mutex);
}




