#ifndef __TL__KRNL_H
#define	__TL__KRNL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <_gcc.h> /* __PRINTF0__ */
#include <stdint.h> /* uint16_t */

/* EBP is initialized to this value when a thread or task starts.
dump_stack_frames() should stop when it sees this value */
#define	EBP_MAGIC	0xCF95BB6DuL

/* we reprogram the 8253 timer chip (IRQ 0) to run at this frequency */
#define	HZ		100

/* what it says... */
#define	KRNL_STACK_SIZE	8192

/* *** WARNING ***
These are selector values; and must agree with the GDT layout
in the kernel assembly-language code (at label "gdt:" in KSTART.S) */
#define	KERNEL_CS	0x10
#define	KERNEL_DS	0x18
#define	USER_CS		(0x20 + 3) /* ring 3 (user privilege) */
#define	USER_DS		(0x28 + 3) /* ring 3 (user privilege) */

/* discardable (run-once) kernel code and discardable (use-once) data */
#if defined(__GNUC__)
#define	DISCARDABLE_CODE(X)				\
	X __attribute__((section (".dtext")));		\
	X

#define	DISCARDABLE_DATA(X)				\
	extern X __attribute__((section (".ddata")));	\
	X
#else
#define	DISCARDABLE_CODE(X)	X
#define	DISCARDABLE_DATA(X)	X
#endif

/***************************** VIRTUAL CONSOLES *****************************/

/* size of global and per-VC keyboard queues */
#define	KBD_BUF_SIZE	64

typedef struct
{
	struct _task *head, *tail;
} wait_queue_t;

typedef struct	/* circular queue */
{
	unsigned char *data;
	unsigned size, in_ptr, out_ptr;
} queue_t;

typedef struct
{
	unsigned magic;	/* magic value; for validation */
/* virtual console input */
	queue_t keystrokes;
/* virtual console output */
	uint16_t *fb_adr;
	unsigned char attrib, esc, csr_x, csr_y, esc1, esc2, esc3;
/* saved cursor position for ESC[s and ESC[u
old_x==-1 means cursor position not saved (no ESC[s yet) */
	signed char save_x;
	unsigned char save_y;
/* wait queue */
	wait_queue_t wait;
} console_t;

/****************************** ADDRESS SPACES ******************************/

/* flags used to describe memory segments */
#define	SF_ZERO		0x10	/* BSS; zero before use */
#define	SF_LOAD		0x08	/* load from file */
#define	SF_READ		0x04	/* readable */
#define	SF_WRITE	0x02	/* writable */
#define	SF_EXEC		0x01	/* executable */
#define	SF_BSS		(SF_ZERO | SF_READ | SF_WRITE)

typedef struct
{
	unsigned adr, size, flags;
	unsigned long offset;
} sect_t;

/* *** WARNING ***
code in to_user() in KSTART.S must agree with the layout of this struct */
typedef struct
{
	char *user_mem;	/* kmalloc()ed memory for task */
	unsigned virt_adr; /* lowest virtual address of task */
	unsigned size;	/* total size of task */

	unsigned magic, num_sects;
	sect_t *sect;
} aspace_t;

/********************************** FILES ***********************************/

typedef unsigned long	off_t;

typedef struct
{
	off_t off;
	unsigned access;
	const struct _ops *ops;
	void *data; /* instance data */
} file_t;

typedef struct _ops
{
	int (*close)(file_t *file);
	int (*write)(file_t *file, unsigned char *buf, unsigned len);
	int (*read)(file_t *file, unsigned char *buf, unsigned want);
	int (*select)(file_t *file, unsigned access, unsigned *timeout);
} ops_t;

/********************************** TASKS ***********************************/

#define	NUM_FILES	20

/* *** WARNING ***
kstack must be first in this struct, as must be second */
typedef struct _task
{
	char *kstack;	/* current value of kernel (ring 0) ESP */
	aspace_t *as;
/**/
	unsigned magic; /* magic value; for validation */
	enum
	{
		TS_UNUSED = 0, TS_RUNNABLE, TS_BLOCKED, TS_ZOMBIE
	} status;
	int priority, exit_code;
	unsigned timeout;
	char *kstack_mem; /* kmalloc()ed kernel stack */
	struct _task *prev, *next;
	file_t *files[NUM_FILES];
} task_t;

/* registers pushed on the stack by exceptions or interrupts
that switch from user privilege to kernel privilege
	*** WARNING ***
The layout of this struct must agree with the order in which
registers are pushed and popped in the assembly-language
interrupt handler code. */
typedef struct
{
	unsigned edi, esi, ebp, esp, ebx, edx, ecx, eax; /* PUSHA/POP */
	unsigned ds, es, fs, gs;
	unsigned which_int, err_code;
	unsigned eip, cs, eflags, user_esp, user_ss; /* INT nn/IRET */
} uregs_t;

/* registers pushed on the stack by switch_to(); the asm routine
that swaps kernel stacks
	*** WARNING ***
The layout of this struct must agree with the asm switch_to() code */
typedef struct
{
	unsigned ebx, esi, edi, ebp; /* callee-save */
	unsigned eip, eflags;	/* CALL/IRET */
} kregs_t;

/******************************** SERIAL PORT *******************************/

typedef struct
{
	queue_t rx, tx;
/* number of: interrupts, receive interrupts, transmit interrupts */
	unsigned int_count, rx_count, tx_count;
/* number of: framing errors, parity errors, overrun errors */
	unsigned ferr_count, perr_count, oerr_count;
	unsigned fifo_size;
/* hardware resources */
	unsigned io_adr;
/* wait queue */
	wait_queue_t wait;
} serial_t;

/****************************** GLOBAL FUNCTIONS ****************************/

void kprintf(const char *fmt, ...) __PRINTF0__;
void panic(const char *fmt, ...) __PRINTF0__;

#if 1
/* using segment-based address translation:
from KSTART.S */
extern unsigned g_kvirt_to_phys;
#else
/* using paging:
hopefully this will be optimized away */
#define	g_kvirt_to_phys	0
#endif

#ifdef __cplusplus
}
#endif

#endif
