/*----------------------------------------------------------------------------
CONSOLE CHARACTER DEVICE

EXPORTS:
int open_con(file_t *f);
----------------------------------------------------------------------------*/
#include <system.h> /* disable(), restore_flags() */
#include <string.h> /* NULL */
#include <os.h> /* O_RDONLY */
#include "_krnl.h"

/* IMPORTS
from THREADS.C */
extern task_t *g_curr_task;

int sleep_on(wait_queue_t *queue, unsigned *timeout);

/* from KBD.C */
int deq(queue_t *q, unsigned char *data);

/* from VIDEO.C */
void putch_help(console_t *con, unsigned c);
console_t *create_vc(void);
void destroy_vc(console_t *con);
/*****************************************************************************
*****************************************************************************/
static int close_con(file_t *f)
{
	destroy_vc((console_t *)f->data);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int write_con(file_t *f, unsigned char *buf, unsigned len)
{
	console_t *con;
	unsigned i;

	con = (console_t *)f->data;
	for(i = 0; i < len; i++)
	{
		putch_help(con, *buf);
		buf++;
	}
	return len;
}
/*****************************************************************************
*****************************************************************************/
static int read_con(file_t *f, unsigned char *buf, unsigned len)
{
	unsigned i, empty, flags;
	console_t *con;

	con = (console_t *)f->data;
	for(i = 0; i < len; i++)
	{
		do
		{
/* atomic test for empty queue */
			flags = disable();
			empty = (con->keystrokes.out_ptr ==
				con->keystrokes.in_ptr);
			restore_flags(flags);
/* if empty, wait on it forever (no timeout) */
			if(empty)
				sleep_on(&con->wait, NULL);
		} while(deq(&con->keystrokes, buf));
		buf++;
	}
	return len;
}
/*****************************************************************************
*****************************************************************************/
static int select_con(file_t *f, unsigned access, unsigned *timeout)
{
	unsigned flags, empty;
	console_t *con;
	int rv;

	con = (console_t *)f->data;
	while(1)
	{
/* console is always ready to write */
		rv = 0x02;
/* atomic test for empty queue */
		flags = disable();
		empty = (con->keystrokes.out_ptr ==
			con->keystrokes.in_ptr);
		restore_flags(flags);
/* check if ready to read */
		if(!empty)
			rv |= 0x01;
/* ready to read or write; return */
		rv &= access;
		if(rv)
			break;
/* not ready to read or write, but no timeout, so return */
		if(timeout == NULL)
			break;
/* not ready; wait... */
		if(sleep_on(&con->wait, timeout))
			break;
	}
	return rv;
}
/*****************************************************************************
xxx - de-couple console output and input
*****************************************************************************/
int open_con(file_t *f)
{
	static const ops_t ops =
	{
		close_con, write_con, read_con, select_con
	};
static void *pf;
/**/
if(f->access & O_RDONLY) {
pf =	f->data = (void *)create_vc();
	if(f->data == NULL)
		return -1;
/* if opening for output (writing), use the same file handle
that was returned by the last call to open_con() */
} else f->data = pf;
	f->ops = &ops;
	return 0;
}
