#ifndef KERNEL_USIGNAL_H
#define KERNEL_USIGNAL_H

#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/i386/i386.h>




/*
 * 
 */

typedef unsigned long sigset_t;
typedef int	sig_atomic_t;			/* Atomic entity type (ANSI) */



/*
 * stack_t
 */

typedef struct __stack
{
	void     *ss_sp;
	size_t    ss_size;
	int       ss_flags;
} stack_t;




/*
 * siginfo_t.sigval
 */

union sigval
{
	int	sival_int;    /* Integer signal value */
	void *sival_ptr;    /* Pointer signal value */
};




/*
 * siginfo_t
 */

typedef struct
{
	int si_signo;				/* Signal number */
	int si_code;				/* Cause of the signal */
	union sigval si_value;		/* Signal value */
} siginfo_t; 




/*
 * siginfo_t.si_code values
 */

#define SI_USER    1    /* Sent by a user. kill(), abort(), etc */
#define SI_QUEUE   2    /* Sent by sigqueue() */
#define SI_TIMER   3    /* Sent by expiration of a timer_settime() timer */
#define SI_ASYNCIO 4    /* Indicates completion of asycnhronous IO */
#define SI_MESGQ   5    /* Indicates arrival of a message at an empty queue */




/*
 * struct SigAction
 */

typedef void (*_sig_func_ptr)();

struct sigaction
{
	int			sa_flags;       /* Special flags to affect behavior of signal */
	sigset_t	sa_mask;        /* Signals to be blocked during handler */

	union
	{
		_sig_func_ptr _handler;  /* SIG_DFL, SIG_IGN, or pointer to a function */
		void (*_sigaction)( int, siginfo_t *, void * );
  	} _signal_handlers;
};




/*
 * Aliases to entries in the sigaction._signal_handlers union
 */
 
#define sa_handler    _signal_handlers._handler
#define sa_sigaction  _signal_handlers._sigaction




/*
 * Signal handler actions and error
 */

#define SIG_DFL ((_sig_func_ptr)0)	/* Default action */
#define SIG_IGN ((_sig_func_ptr)1)	/* Ignore action */
#define SIG_ERR ((_sig_func_ptr)-1)	/* Error return */




/*
 * sigaction.sa_flags
 */
 
#define SA_NOCLDSTOP (1<<0)
#define SA_RESETHAND (1<<1)
#define SA_SIGINFO   (1<<2)
#define SA_NODEFER   (1<<3)




/*
 *
 */
 
#define SIG_SETMASK 0	/* set mask with sigprocmask() */
#define SIG_BLOCK 1		/* set of signals to block */
#define SIG_UNBLOCK 2	/* set of signals to, well, unblock */




/*
 * Exit modes and return codes for Join()
 */

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define SIGNAL_EXIT_MASK(w)		(w & 0x000000ff)
#define EXIT_STATUS(w) (w<<8)


#define WNOHANG 1
#define WUNTRACED 2

#define WIFEXITED(w)	(((w) & 0xff) == 0)
#define WIFSIGNALED(w)	(((w) & 0x7f) > 0 && (((w) & 0x7f) < 0x7f))
#define WIFSTOPPED(w)	(((w) & 0xff) == 0x7f)
#define WEXITSTATUS(w)	(((w) >> 8) & 0xff)
#define WTERMSIG(w)		((w) & 0x7f)
#define WSTOPSIG		WEXITSTATUS




/*
 * User signal numbers
 */

#define	SIGHUP		1	/* hangup */
#define	SIGINT		2	/* interrupt */
#define	SIGQUIT		3	/* quit */
#define	SIGILL		4	/* # illegal instruction (not reset when caught) */
#define	SIGTRAP		5	/* # trace trap (not reset when caught) */
#define	SIGIOT		6	/* # IOT instruction */
#define	SIGABRT		6	/* used by abort, replace SIGIOT in the future */
#define	SIGEMT		7	/* # EMT instruction */
#define	SIGFPE		8	/* # floating point exception */
#define	SIGKILL		9	/* kill (cannot be caught or ignored) */
#define	SIGBUS		10	/* # bus error */
#define	SIGSEGV		11	/* # segmentation violation */
#define	SIGSYS		12	/* bad argument to system call */
#define	SIGPIPE		13	/* write on a pipe with no one to read it */
#define	SIGALRM		14	/* alarm clock */
#define	SIGTERM		15	/* software termination signal from kill */
#define	SIGURG		16	/* urgent condition on IO channel */
#define	SIGSTOP		17	/* sendable stop signal not from tty */
#define	SIGTSTP		18	/* stop signal from tty */
#define	SIGCONT		19	/* continue a stopped process */
#define	SIGCHLD		20	/* to parent on child stop or exit */
#define	SIGCLD		20	/* System V name for SIGCHLD */
#define	SIGTTIN		21	/* to readers pgrp upon background tty read */
#define	SIGTTOU		22	/* like TTIN for output if (tp->t_local&LTOSTOP) */
#define	SIGIO		23	/* input/output possible signal */
#define	SIGPOLL		23	/* System V name for SIGIO */
#define	SIGXCPU		24	/* exceeded CPU time limit */
#define	SIGXFSZ		25	/* exceeded file size limit */
#define	SIGVTALRM	26	/* virtual time alarm */
#define	SIGPROF		27	/* profiling time alarm */
#define	SIGWINCH	28	/* window changed */
#define	SIGLOST		29	/* resource lost (eg, record-lock lost) */
#define	SIGUSR1		30	/* user defined signal 1 */
#define	SIGUSR2		31	/* user defined signal 2 */
#define SIGPWR		32  /* power failure */

#define NSIG		33	/* Number of defined signals 1-32 */




/* Signal flag bits */

#define	SIGFHUP		(1<<0)	/* hangup */
#define	SIGFINT		(1<<1)	/* interrupt */
#define	SIGFQUIT	(1<<2)	/* quit */
#define	SIGFILL		(1<<3)	/* illegal instruction (not reset when caught) */
#define	SIGFTRAP	(1<<4)	/* trace trap (not reset when caught) */
#define	SIGFIOT		(1<<5)	/* IOT instruction */
#define	SIGFABRT	(1<<5)	/* used by abort, replace SIGIOT in the future */
#define	SIGFEMT		(1<<6)	/* EMT instruction */
#define	SIGFFPE		(1<<7)	/* floating point exception */
#define	SIGFKILL	(1<<8)	/* kill (cannot be caught or ignored) */
#define	SIGFBUS		(1<<9)	/* bus error */
#define	SIGFSEGV	(1<<10)	/* segmentation violation */
#define	SIGFSYS		(1<<11)	/* bad argument to system call */
#define	SIGFPIPE	(1<<12)	/* write on a pipe with no one to read it */
#define	SIGFALRM	(1<<13)	/* alarm clock */
#define	SIGFTERM	(1<<14)	/* software termination signal from kill */
#define	SIGFURG		(1<<15)	/* urgent condition on IO channel */
#define	SIGFSTOP	(1<<16)	/* sendable stop signal not from tty */
#define	SIGFTSTP	(1<<17)	/* stop signal from tty */
#define	SIGFCONT	(1<<18)	/* continue a stopped process */
#define	SIGFCHLD	(1<<19)	/* to parent on child stop or exit */
#define	SIGFCLD		(1<<19)	/* System V name for SIGCHLD */
#define	SIGFTTIN	(1<<20)	/* to readers pgrp upon background tty read */
#define	SIGFTTOU	(1<<21)	/* like TTIN for output if (tp->t_local&LTOSTOP) */
#define	SIGFIO		(1<<22)	/* input/output possible signal */
#define	SIGFPOLL	(1<<22)	/* System V name for SIGIO */
#define	SIGFXCPU	(1<<23)	/* exceeded CPU time limit */
#define	SIGFXFSZ	(1<<24)	/* exceeded file size limit */
#define	SIGFVTALRM	(1<<25)	/* virtual time alarm */
#define	SIGFPROF	(1<<26)	/* profiling time alarm */
#define	SIGFWINCH	(1<<27)	/* window changed */
#define	SIGFLOST	(1<<28)	/* resource lost (eg, record-lock lost) */
#define	SIGFUSR1	(1<<29)	/* user defined signal 1 */
#define	SIGFUSR2	(1<<30)	/* user defined signal 2 */
#define SIGFPWR		(1<<31)  /* power failure */




/*
 * Macros for manipulating signals
 */
 
#define SIGBIT(sig)	(1UL<<(sig-1))




/*
 * Bitmap masks of some signal properties
 */
 
#define SIGCANTMASK (SIGFKILL | SIGFSTOP)
#define STOPSIGMASK (SIGFSTOP) | SIGFTSTP | SIGFTTIN | SIGFTTOU)
#define CONTSIGMASK (SIGFCONT)
#define SYNCSIGMASK (SIGFILL | SIGFTRAP | SIGFBUS | SIGFFPE | SIGFSEGV)




/*
 * Flags used in the sigprop[] array
 */
 
#define SP_KILL     (1<<0)
#define SP_CORE     (1<<1)
#define SP_NORESET  (1<<2)
#define SP_CANTMASK (1<<3)
#define SP_CONT     (1<<4)
#define SP_STOP     (1<<5)
#define SP_TTYSTOP  (1<<6)




/*
 * User/nix signal state of a Process.
 */

struct UserSignals
{
	_sig_func_ptr handler[NSIG-1];      /* Action for each signal, func, SIG_DFL, SIG_IGN */
	sigset_t handler_mask[NSIG-1];      /* Mask to add to current sig_mask during handler.*/
	
	siginfo_t siginfo_data[NSIG-1];
	
	sigset_t sig_info;                /* bitmap of signals that want SA_SIGINFO args. */
	
	sigset_t sig_mask;                /* Current signal mask */
	sigset_t sig_pending;             /* bitmap of signals awaiting delivery */
	
	sigset_t sig_resethand;           /* Unreliable signals */
	sigset_t sig_nodefer;             /* Don't automatically mask delivered signal */
	
	sigset_t sigsuspend_oldmask;
	bool use_sigsuspend_mask;
	
	void (*trampoline)(void);
};





/*
 * Variables
 */

extern const uint32 sigprop[NSIG];

/*
 * Prototypes
 */

void USigSetTrampoline (void (*func)(void));

void USigInit (struct Process *dst);
void USigFork (struct Process *src, struct Process *dst);
void USigExec (struct Process *dst);


int UKill (int pid, int signal);

int USigAction (int how, const struct sigaction *act, struct sigaction *oact);
int USigSuspend (const sigset_t *mask);

int USigProcMask (int how, const sigset_t *set, sigset_t *oset);
int USigPending (sigset_t *set);

void DoUSignalDefault (int sig);

int PickUSignal (uint32 sigbits);




#endif
