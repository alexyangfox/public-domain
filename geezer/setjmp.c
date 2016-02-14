/*----------------------------------------------------------------------------
Cooperative multitasking with setjmp() and longjmp()
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: Mar 20, 2003
This code is public domain (no copyright).
You can do whatever you want with it.


20 Mar, 2003: modified to work with Linux + glibc 2

22 Mar, 2002: modified to work with Watcom C

3 Jan, 2002: modified to work with Linux (libc 5/glibc 1, anyway)

7 May, 2001: malloc() and related functions will not work with
this code, at least, not with Turbo C. The problem is in the
__sbrk() function, which fails if the stack pointer (SP) is
less than the break value.
----------------------------------------------------------------------------*/
#include <setjmp.h> /* jmp_buf, setjmp(), longjmp() */
#include <stdio.h> /* puts() */

/* 'state' is the name of the field in the task_t struct, defined below */
#if defined(__TURBOC__)
#include <conio.h> /* kbhit(), getch() */
#define	JMPBUF_IP	state[0].j_ip
#define	JMPBUF_SP	state[0].j_sp

#elif defined(__DJGPP__)
#include <conio.h> /* kbhit(), getch() */
#define	JMPBUF_IP	state[0].__eip
#define	JMPBUF_SP	state[0].__esp

#elif defined(linux)
#if __GLIBC__==1
#define	JMPBUF_IP	state[0].__pc
#define	JMPBUF_SP	state[0].__sp
#elif __GLIBC__==2
#define	JMPBUF_IP	state[0].__jmpbuf[JB_PC]
#define	JMPBUF_SP	state[0].__jmpbuf[JB_SP]
#else
#error Sorry; unsupported version of GNU libc
#endif
static int kbhit(void);
static int getch(void);

#elif defined(__WATCOMC__)
#include <conio.h> /* kbhit(), getch() */
#if defined(__386__)
#define	JMPBUF_IP	state[6]
#define	JMPBUF_SP	state[7]
#else
#define	JMPBUF_IP	state[10]
#define	JMPBUF_SP	state[6]
#endif

#else
#error Sorry, unsupported compiler, OS, or C library
#endif

#define	NUM_TASKS	2
#define	STACK_SIZE	512

typedef struct
{
	jmp_buf state;
} task_t;

static task_t g_tasks[NUM_TASKS + 1];
/*****************************************************************************
g_tasks[0]		state of first task
	...
g_tasks[NUM_TASKS - 1]	state of last task
g_tasks[NUM_TASKS]	jmp_buf state to return to main()
*****************************************************************************/
static void schedule(void)
{
	static unsigned current = NUM_TASKS;
/**/
	unsigned prev;

	prev = current;
/* round-robin switch to next task */
	current++;
	if(current >= NUM_TASKS)
		current = 0;
/* return to main() if key pressed */
	if(kbhit())
		current = NUM_TASKS;
/* save old task state
setjmp() returning nonzero means we came here through hyperspace
from the longjmp() below -- just return */
	if(setjmp(g_tasks[prev].state) != 0)
		return;
/* load new task state */
	longjmp(g_tasks[current].state, 1);
}
/*****************************************************************************
*****************************************************************************/
#define	WAIT	0xFFFFFL

static void wait(void)
{
	unsigned long wait;

	for(wait = WAIT; wait != 0; wait--)
		/* nothing */;
}
/*****************************************************************************
*****************************************************************************/
static void task0(void)
{
	puts("hello from task 0\n");
	while(1)
	{
		schedule(); /* yield() */
		puts("task 0\n");
		wait();
	}
}
/*****************************************************************************
*****************************************************************************/
static void task1(void)
{
	puts("\tgreetz from task 1\n");
	while(1)
	{
		schedule(); /* yield() */
		puts("\ttask 1\n");
		wait();
	}
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	static char stacks[NUM_TASKS][STACK_SIZE];
	volatile unsigned i;
/**/
	unsigned adr;

	for(i = 0; i < NUM_TASKS; i++)
	{
/* set all registers of task state */
		(void)setjmp(g_tasks[i].state);
		adr = (unsigned)(stacks[i] + STACK_SIZE);
/* set SP of task state */
		g_tasks[i].JMPBUF_SP = adr;
	}
/* set IP of task state */
	g_tasks[0].JMPBUF_IP = (unsigned)task0;
	g_tasks[1].JMPBUF_IP = (unsigned)task1;
/* this does not return until a key is pressed */
	schedule();
/* eat keystroke */
	if(getch() == 0)
		(void)getch();
	return 0;
}
/*----------------------------------------------------------------------------
kbhit() and getch() for Linux/UNIX
----------------------------------------------------------------------------*/
#if defined(linux)
#include <sys/time.h> /* struct timeval, select() */
/* ICANON, ECHO, TCSANOW, struct termios */
#include <termios.h> /* tcgetattr(), tcsetattr() */
#include <stdlib.h> /* atexit(), exit() */
#include <unistd.h> /* read() */

static struct termios g_old_kbd_mode;
/*****************************************************************************
*****************************************************************************/
static void cooked(void)
{
	tcsetattr(0, TCSANOW, &g_old_kbd_mode);
}
/*****************************************************************************
*****************************************************************************/
static void raw(void)
{
	static char init;
/**/
	struct termios new_kbd_mode;

/* put keyboard (stdin, actually) in raw, unbuffered mode */
	if(init)
		return;
	tcgetattr(0, &g_old_kbd_mode);
	memcpy(&new_kbd_mode, &g_old_kbd_mode, sizeof(struct termios));
	new_kbd_mode.c_lflag &= ~(ICANON | ECHO);
	new_kbd_mode.c_cc[VTIME] = 0;
	new_kbd_mode.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_kbd_mode);
/* when we exit, go back to normal, "cooked" mode */
	atexit(cooked);

	init = 1;
}
/*****************************************************************************
*****************************************************************************/
static int kbhit(void)
{
	struct timeval timeout;
	fd_set read_handles;
	int status;

	raw();
/* check stdin (fd 0) for activity */
	FD_ZERO(&read_handles);
	FD_SET(0, &read_handles);
	timeout.tv_sec = timeout.tv_usec = 0;
	status = select(1, &read_handles, NULL, NULL, &timeout);
	if(status < 0)
	{
		printf("select() failed in kbhit()\n");
		exit(1);
	}
	return status;
}
/*****************************************************************************
*****************************************************************************/
static int getch(void)
{
	unsigned char temp;

	raw();
/* stdin = fd 0 */
	if(read(0, &temp, 1) != 1)
		return 0;
	return temp;
}
#endif
