#ifndef KERNEL_I386_UCONTEXT_H
#define KERNEL_I386_UCONTEXT_H

#include <kernel/types.h>
#include <kernel/usignal.h>




/*
 * mcontext_t : i386/87 register state
 */

#define _MC_FPFMT_NODEV         0x10000 /* device not present or configured */
#define _MC_FPFMT_387           0x10001
#define _MC_FPFMT_XMM           0x10002

typedef struct
{
	uint32 eax;
	uint32 ebx;
	uint32 ecx;
	uint32 edx;
	uint32 esi;
	uint32 edi;
	uint32 ebp;
	
	uint32 gs;		
	uint32 fs;
	uint32 es;
	uint32 ds;
	
	uint32 trap;
	
	uint32 eip;
	uint32 cs;
	uint32 eflags;
	uint32 esp;
	uint32 ss;
		
	uint32 fpformat;
	
	uint32 __pad[2];
		
	uint32  fpstate[128] __attribute__ ((aligned(16)));
	
} mcontext_t;	/* 592 bytes */




/*
 * ucontext_t
 */

#define UCF_SWAPPED     0x00000001      /* Used by swapcontext(3). */

typedef struct __ucontext
{
	mcontext_t	uc_mcontext;
	
	sigset_t	uc_sigmask;
	stack_t		uc_stack;
	int			uc_flags;
	struct __ucontext *uc_link;

} ucontext_t;
 



/*
 *
 */
 
typedef void __sighandler_t (int);
typedef	void __siginfohandler_t (int, siginfo_t *, void *);




/*
 * struct sigframe
 */

struct sigframe
{
	int          sf_signum;
	siginfo_t    *sf_siginfo;     /* pointer to sf_si */
	ucontext_t   *sf_ucontext;    /* pointer to sf_uc */

	union
    {
		__siginfohandler_t      *sf_action;
		__sighandler_t          *sf_handler;
	} sf_ahu;
    
    ucontext_t      sf_uc;          /* = *sf_ucontext */
    siginfo_t       sf_si;          /* = *sf_siginfo (SA_SIGINFO case) */
};




/*
 * Prototypes
 */

void SigReturn (struct sigframe *sigframe, struct ContextState *state);
void DeliverUserSignals (struct ContextState *state);





#endif
