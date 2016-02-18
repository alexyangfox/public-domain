/*----------------------------------------------------------------------------
KERNEL THREAD CREATION, DELETION, SCHEDULING

EXPORTS:
extern task_t *g_curr_task;

int sleep_on(wait_queue_t *queue, unsigned *timeout);
void wake_up(wait_queue_t *queue);
void timer_irq(void);
task_t *create_thread(unsigned init_eip, int priority);
void destroy_thread(task_t *t);
void init_threads(void);
void _exit(int code);
----------------------------------------------------------------------------*/
#include <string.h> /* NULL */
#include <system.h> /* outportb() */
#include <os.h> /* O_RDONLY, O_WRONLY, open() */
#include "_krnl.h"

/* IMPORTS
from USTART.S (for TETRIS.C) */
int uentry(void);

/* from TASKS.C */
void destroy_task(task_t *t);

/* from KBD.C */
void keyboard_bh(void);

/* from MM.C */
void *kmalloc(unsigned size);
void kfree(void *ptr);

/* from KSTART.S */
void switch_to(task_t *t);

#define	THREAD_MAGIC	0x1F9D	/* must be < 0x8000 */
#define	NUM_TASKS	15

static wait_queue_t g_task_died;
static task_t g_tasks[NUM_TASKS] =
{
/* thread #0 (idle thread) */
	{
		0,		/* .kstack (dummy value) */
		0,		/* .as (dummy value) */
		THREAD_MAGIC,	/* .magic */
		TS_RUNNABLE,	/* .status */
		-1		/* .priority (-1 for idle thread) */
/* no need to initialize .exit_code, .timeout, .kstack_mem,
.prev, .next, .files */
	}
};
/* set g_curr_task = idle thread */
task_t *g_curr_task = g_tasks + 0;
/*****************************************************************************
*****************************************************************************/
static void schedule(void)
{
	static unsigned current;
/**/
	int i, p = -127;
	task_t *t;

	for(i = 0; i < NUM_TASKS; i++)
	{
		t = g_tasks + i;
/* test all in-use threads to see if any were corrupted */
		if(t->status == TS_UNUSED)
			continue;
		if(t->magic != THREAD_MAGIC)
			panic("schedule: bad thread 0x%p", t);
/* find runnable thread with highest priority */
		if(t->status != TS_RUNNABLE)
			continue;
		if(t->priority > p)
			p = t->priority;
	}
/* find next thread beyond current with this priority */
	for(i = 0; i < NUM_TASKS * 2; i++)
	{
		current++;
		if(current >= NUM_TASKS)
			current = 0;
		t = g_tasks + current;
		if(t->status == TS_RUNNABLE && t->priority >= p)
			break;
	}
	if(i >= NUM_TASKS * 2)
		panic("schedule: infinite loop! (no runnable threads?)");
/* switch threads */
	switch_to(t);
}
/*****************************************************************************
*****************************************************************************/
int sleep_on(wait_queue_t *queue, unsigned *timeout)
{
	task_t *prev;

/* mark thread blocked */
	g_curr_task->status = TS_BLOCKED;
/* splice into wait queue at queue->tail */
	prev = queue->tail;
	queue->tail = g_curr_task;
	if(prev == NULL)
	{
		queue->head = g_curr_task;
		g_curr_task->prev = NULL;
	}
	else
	{
		g_curr_task->prev = prev;
		prev->next = g_curr_task;
	}
	g_curr_task->next = NULL;
/* set the timeout, if there is one */
	if(timeout != NULL)
		g_curr_task->timeout = *timeout;
	else
		g_curr_task->timeout = 0;
/* go do something else until something wakes us */
	schedule();
/* now: why did we return? */
	if(timeout != NULL)
	{
		*timeout = g_curr_task->timeout;
/* there was a timeout, so timer_irq() awoke us. Return -1 */
		if(*timeout == 0)
			return -1;
	}
/* someone called wake_up(), making us TS_RUNNABLE again. Return 0 */
	return 0;
}
/*****************************************************************************
*****************************************************************************/
void wake_up(wait_queue_t *queue)
{
	task_t *thread, *next;

/* make sure queue is not empty */
	thread = queue->head;
	if(thread == NULL)
		return;
/* mark head thread in queue runnable */
	thread->status = TS_RUNNABLE;
/* remove head thread from queue */
	next = thread->next;
	queue->head = next;
	if(next != NULL)
		next->prev = NULL;
	else
		queue->tail = NULL;
}
/*****************************************************************************
*****************************************************************************/
void timer_irq(void)
{
	unsigned i, new_time;

/* decrement timeouts for threads that have them */
	for(i = 0; i < NUM_TASKS; i++)
	{
		new_time = g_tasks[i].timeout;
		if(new_time == 0)
			continue;
/* number of milliseconds per timer IRQ */
		new_time -= (unsigned)(1000 / HZ);
		if(new_time > g_tasks[i].timeout)
			new_time = 0;	/* underflow */
		g_tasks[i].timeout = new_time;
/* blocked thread timed out? make it runnable
xxx - should do wake_up() here, but timer_irq() doesn't know
where the appropriate wait queue is */
		if(new_time == 0 && g_tasks[i].status == TS_BLOCKED)
			g_tasks[i].status = TS_RUNNABLE;
	}
/* switch threads */
	schedule();
}
/*****************************************************************************
*****************************************************************************/
task_t *create_thread(unsigned init_eip, int priority)
{
	kregs_t *kregs;
	task_t *t;

#if 1
	unsigned i;

	for(i = 0; i < NUM_TASKS; i++)
	{
		if(g_tasks[i].status == TS_UNUSED)
			break;
	}
	if(i >= NUM_TASKS)
		return NULL;
	t = g_tasks + i;
#else
/* allocate memory for task_t. This is commented out because
dynamically-allocated thread_t's are not yet supported */
	t = (task_t *)kcalloc(sizeof(task_t));
	if(t == NULL)
		return NULL;
#endif
/* allocate kernel stack */
	t->kstack_mem = kmalloc(KRNL_STACK_SIZE);
	if(t->kstack_mem == NULL)
	{
		kfree(t);
		return NULL;
	}
/* that worked; now init various task_t fields */
	t->status = TS_RUNNABLE;
	t->priority = priority;
	t->magic = THREAD_MAGIC;
/* push init_eip on the kernel stack, to be popped by first switch_to() */
	t->kstack = t->kstack_mem + KRNL_STACK_SIZE - sizeof(kregs_t);
	kregs = (kregs_t *)t->kstack;
/* init vital ring 0 registers */
	kregs->eip = init_eip;
	kregs->eflags = 0x200; /* for kernel thread; enable interrupts */
	return t;
}
/*****************************************************************************
*****************************************************************************/
void destroy_thread(task_t *t)
{
	if(t->magic != THREAD_MAGIC)
		panic("destroy_thread: bad thread 0x%p", t);
	kfree(t->kstack_mem);
	memset(t, 0, sizeof(task_t)); /* t->status = 0 = TS_UNUSED */
/*	kfree(t); for dynamically-allocated thread_t's */
}
/*****************************************************************************
*****************************************************************************/
static void dead_task_reaper(void)
{
	unsigned i;

	while(1)
	{
		sleep_on(&g_task_died, NULL); /* no timeout */
		for(i = 0; i < NUM_TASKS; i++)
		{
			if(g_tasks[i].status != TS_ZOMBIE)
				continue;
kprintf("dead_task_reaper: recovering task %u\n", i);
			destroy_task(g_tasks + i);
		}
	}
}
/*****************************************************************************
*****************************************************************************/
DISCARDABLE_CODE(void init_threads(void))
{
/* Tetris linked into the kernel:
	static unsigned ip[] =
	{
		(unsigned)uentry
	}; */
/**/
	unsigned num_threads, i;
	task_t *t;

	num_threads = 1; /* idle thread already created statically */
	kprintf("init_threads: ");
/* create keyboard bottom-half handler */
	t = create_thread((unsigned)keyboard_bh, +1);
	if(t == NULL)
		panic("can't create keyboard bottom-half handler");
	num_threads++;
/* create dead task reaper */
	t = create_thread((unsigned)dead_task_reaper, +1);
	if(t == NULL)
		kprintf("can't create dead task reaper\n");
	num_threads++;
#if 0
/* Borland and Watcom C use OMF- (.OBJ)-format relocatable files,
which my module code does not support... */
#if defined(__GNUC__)
/* create static (linked-in) threads */
	for(i = 0; i < sizeof(ip) / sizeof(ip[0]); i++)
	{
		t = create_thread((unsigned)ip[i], 0);
		if(t == NULL)
		{
			kprintf("Can't create static thread %u "
				"(no task_t's left)\n", i);
			break;
		}
		num_threads++;
	}
#endif
#endif
	kprintf("\tstarted %u static kernel thread(s)\n", num_threads);
}
/*****************************************************************************
*****************************************************************************/
void _exit(int code)
{
	g_curr_task->exit_code = code;
	g_curr_task->status = TS_ZOMBIE;
	wake_up(&g_task_died);
	schedule();
}
