#ifndef KERNEL_PROC_H
#define KERNEL_PROC_H

#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/sync.h>
#include <kernel/vm.h>
#include <kernel/msg.h>
#include <kernel/timer.h>
#include <kernel/usignal.h>
#include <kernel/fs.h>
#include <kernel/arch.h>




/* 
 * Number of file descriptors and maximum exec() argument size.
 */
 
#define MAX_FD				32
#define ARG_MAX				0x10000

#define PROC_NAME_SZ		64




/*
 * Duplicate file descriptor during fork().
 */

#define DUP_FD				1
#define NO_DUP_FD			0




/*
 * Kernel-Mode Synchronous Signals
 */


#define SIG_USER			0  
#define SIG_CHLD			1   /* Child exiting */
#define SIG_FS				2
#define SIG_INIT			3
#define SIG_ALARM			4
#define SIG_ABORTIO			5
#define SIG_TERM			6

#define SIGF_USER			(1 << SIG_USER)  /* Terminated ? */
#define SIGF_CHLD			(1 << SIG_CHLD)  /* Terminated ? */
#define SIGF_FS				(1 << SIG_FS)
#define SIGF_INIT			(1 << SIG_INIT)
#define SIGF_ALARM			(1 << SIG_ALARM)
#define SIGF_ABORTIO		(1 << SIG_ABORTIO)
#define SIGF_TERM			(1 << SIG_TERM)

#define SIGNALS_RESERVED	0x000000ff
#define MIN_SIGNAL			8
#define MAX_SIGNAL			32

#define SIGF_NO_CLEAR_WAIT	(SIGF_CHLD | SIGF_ALARM) /* FIXME: add SIGF_USER ? */





/*
 * Process states
 */

#define PROC_STATE_UNALLOC				0
#define PROC_STATE_ZOMBIE				1
#define PROC_STATE_RUNNING				2
#define PROC_STATE_READY				3
#define PROC_STATE_INIT					4
#define PROC_STATE_WAIT_BLOCKED			5
#define PROC_STATE_MUTEX_BLOCKED		6
#define PROC_STATE_COND_BLOCKED			7
#define PROC_STATE_RWREAD_BLOCKED		8
#define PROC_STATE_RWWRITE_BLOCKED		9
#define PROC_STATE_SLEEP				10
#define PROC_STATE_WAITPID				11
#define PROC_STATE_PGRPZOMBIE			12


/*
 * Process types
 */
 
#define PROC_TYPE_KERNEL				0
#define PROC_TYPE_USER					1

/*
 * Priority limits, currently unused.
 */

#define PROC_PRI_IDLE					-128
#define PROC_PRI_MIN_USER				-127
#define PROC_PRI_MAX_USER				0
#define PROC_PRI_MIN_KERNEL				1
#define PROC_PRI_MAX_KERNEL				127





/*
 * struct Process
 */

struct Process
{
	struct ArchProc archproc;
	
	struct AddressSpace *user_as;
	struct AddressSpace as;
	vm_addr kernel_stack;
			
	int pid;
	int uid;
	int gid;	
	int euid;
	int egid;
	
	int pgrp;       /* pgrp = pid if session leader */
	int pgrp_reference_cnt;
	
	LIST_ENTRY (Process) free_entry;

	struct Process *parent;

	int waiting_for;      /* waitpid() waiting for 
							 0 = not waiting ??????? or invalid
						     >0 = pid,  <-1 = pgrp
						  */

	int child_cnt;
	LIST (Process) child_list;
	LIST_ENTRY (Process) child_entry;
	
	char *exe_name;
	
	LIST_ENTRY (Process) sched_entry;		/* run-queue */
	LIST_ENTRY (Process) blocked_entry;		/* Mutex/Cond/RWlock blocked list */
	
	uint32 priority;
	uint32 quanta_used;
	
	uint32 type;
	uint32 state;
	uint32 preempt_cnt;
	int32 status;					/* exit_status int16 ??? */

	struct UserSignals usignal;		/* User signals */
	
	uint32 free_signals;			/* Kernel signals */
	uint32 pending_signals;
	uint32 waiting_for_signals;

	struct Timer ualarm_timer;

	int umask;
	char *current_dir;
	void *filedesc[MAX_FD];
	bool close_on_exec[MAX_FD];
	
	struct MsgPort reply_port;
	int error;						/* Kernel errno, passed to user-space at end of syscall */
};




/*
 * struct ISRHandler
 */

struct ISRHandler
{
	LIST_ENTRY (ISRHandler) isr_handler_entry;
	int32 (*func) (int32 irq_idx, void *arg);
	void *arg;
	int32 irq;
};




/*
 * Simple memory-manager in user-space for ELF structures and suchlike.
 */

struct UMallocState
{
	uint8 *base;
	uint32 size;
};




/*
 * Manages arguments and environment passed to new process/exec.
 */

struct ArgInfo
{
	void *current_pos;
	int nbytes_free;
	void *buf;
	char **argv;
	char **env;
	int argc;
	int envc;
	vm_addr ubase;
	char **uargv;
	char **uenv;
};




/*
 * Global process variables
 */

extern struct Mutex proc_mutex;
extern struct Mutex loader_mutex;

extern uint32 process_cnt;				/* Maximum number of process slots */
extern struct Process *process;       /* rename to proc_table */
extern uint32 isr_depth;
extern uint32 interrupt_stack;
extern uint32 interrupt_stack_ceiling;
extern struct Process *current_process;
extern bool reschedule_request;

extern LIST_DECLARE (ProcessList, Process) free_process_list;
extern LIST_DECLARE (ISRHandlerList, ISRHandler) isr_handler_list[16];
extern CIRCLEQ_DECLARE (SchedQueue, Process) sched_queue[256];

extern uint32 sched_queue_bitmap[8];

extern uint32 user_process_cnt;


/*
 * Main process management functions
 */

int KSpawn (int32 (*func)(void *arg), void *arg, int32 priority, char *name);
int CreateProcess (char *filename, char **argv, char **env);
int Fork (void);
int Exec (char *filename, char **argv, char **env);
int WaitPid (int pid, int32 *status, int options);
void Exit (int status);
void USigExit (int sig);
void Yield (void);
int32 ChangePriority (int32 priority);




/*
 * KSignal, Kernel synchronous Amiga-like signals 
 */

void KSignal (int pid, int32 event);
uint32 KSetSignals (uint32 newsigs, uint32 mask);
uint32 KWait (uint32 mask);
uint32 KTimedWait (uint32 mask, struct TimeVal *tv);
int32 AllocSignal (void);
void FreeSignal (int32 signal);




/*
 * Interrupt handlers
 */

struct ISRHandler *ISRHandlerInsert (int32 irq, int32 (*func) (int32 isr_idx, void *arg), void *arg);
int ISRHandlerRemove (struct ISRHandler *isr);




/*
 * In-Kernel 'errno' (proc->error) handling.  'errno' returned to user-space at end of syscall.
 */

void SetError (int error);
int GetError (void);




/*
 * Scheduling functions
 */


void Reschedule (void);
void PickProc(void);
void SchedReady (struct Process *proc);
void SchedUnready (struct Process *proc);
void DisablePreempt(void);
void EnablePreempt(void);




/*
 * Internal process creation and loading functions
 */

struct Process *AllocProcess (int type);
void FreeProcess (struct Process *proc);
void FreePGRPProcessStruct (struct Process *pgrp_proc);
int SetProcessName (struct Process *proc, char *filename);
int CheckELFHeaders (int fd);
int PopulateAddressSpace (struct Process *newproc, int fd, vm_addr *entry_point, vm_addr *stack_pointer);
							



/*
 * Architecture-specific process initialization and scheduling
 */
			 
int ArchAllocProcess (struct Process *process);
void ArchFreeProcess (struct Process *process);
void ArchPickProc (struct Process *proc);
int ArchInitKProc (struct Process *proc, int32 (*func)(void *arg), void *arg);
int ArchInitUProc (struct Process *proc, vm_addr entry_point, vm_addr stack_pointer, vm_addr argv, int argc, vm_addr env, int envc);
int ArchInitFork (struct Process *newproc);
int ArchInitExec (struct Process *proc, vm_addr entry_point, vm_addr stack_pointer,	vm_addr argv, int argc, vm_addr env, int envc);




/*
 * Arguments and environemt passed to new process/exec
 */

int CreateArgEnv (struct ArgInfo *ai, char **argv, char **env);
void FreeArgEnv (struct ArgInfo *ai);
int CopyArgEnvToUserSpace (struct ArgInfo *ai, struct AddressSpace *new_as);




/*
 * ID functions
 */

struct Process *PIDtoProc (int pid);    /* FIX: Convert to MACRO */
int GetPID (void);
int GetPPID (void);
int GetUID (void);
int SetUID (int uid);
int GetEUID (void);
int GetGID (void);
int SetGID (int gid);
int GetEGID (void);
int SetPGID (int pid, int pgrp);
int GetPGID (int pid);
int GetPGRP (void);
int SetPGRP (void);




#endif

