/*----------------------------------------------------------------------------
SYSCALLS

EXPORTS:
int open(const char *path, unsigned access);
int close(unsigned handle);
int write(unsigned handle, void *buf_p, unsigned len);
int read(unsigned handle, void *buf_p, unsigned want);
int select(int handle, unsigned access, unsigned *timeout);
void syscall(uregs_t *regs);
int lookup(char *sym_name, unsigned *adr, char underbars);
----------------------------------------------------------------------------*/
#include <_syscall.h> /* SYS_... */
#include <system.h> /* enable(), restore_flags() */
#include <string.h> /* NULL, strcmp(), strncmp() */
#include <stdlib.h> /* atoi() */
#include <errno.h> /* ERR_... */
#include <ctype.h> /* isdigit() */
#include <time.h> /* time_t, time() */
#include "_krnl.h" /* task_t, wait_queue_t, console_t, queue_t, uregs_t */

/* IMPORTS
from THREADS.C */
extern task_t *g_curr_task;

int sleep_on(wait_queue_t *queue, unsigned *timeout);
void _exit(int exit_code);

/* from MM.C */
void *kcalloc(unsigned size);
void kfree(void *blk);

/* from CONDEV.C */
int open_con(file_t *f);

/* from SERDEV.C */
int open_ser(file_t *f, unsigned unit);

/* from ZERODEV.C */
int open_zero(file_t *f);

/* from TIME.C */
time_t time(time_t *timer);
/*****************************************************************************
*****************************************************************************/
int open(const char *path, unsigned access)
{
	int i, err;
	file_t *f;

/* find free file_t pointer in current task_t */
	for(i = 0; i < NUM_FILES; i++)
	{
		if(g_curr_task->files[i] == NULL)
			break;
	}
	if(i >= NUM_FILES)
		return -ERR_NO_FILES;
/* allocate file_t */
	f = (file_t *)kcalloc(sizeof(file_t));
	if(f == NULL)
		return -ERR_NO_MEM;
/* store access flags */
	f->access = access;
/* check if opening device:
...console */
	if(!strcmp(path, "/dev/con"))
		err = open_con(f);
/* ...serial port */
	else if(strlen(path) == 9 &&
		!strncmp(path, "/dev/ser", 8) &&
			isdigit(path[8]))
				err = open_ser(f, path[8] - '0');
/* ...NULL/ZERO device */
	else if(!strcmp(path, "/dev/zero"))
		err = open_zero(f);
/* no files or directories yet :( */
	else
	{
		if(strlen(path) != 9)
			kprintf("error: path is not 9 chars long\n");
		else
			kprintf("OK: path is 9 chars long\n");
		if(strncmp(path, "/dev/ser", 8))
			kprintf("error: path doesn't start with '/dev/ser'\n");
		else
			kprintf("OK: path starts with '/dev/ser'\n");
		if(!isdigit(path[8]))
			kprintf("error: path doesn't end with digit\n");
		else
			kprintf("OK: path ends with digit\n");

		kfree(f);
		return -ERR_NOSUCH_FILE;
	}
/* error: free file_t and return negative value */
	if(err < 0)
	{
		kfree(f);
		return err;
	}
/* success: store file_t in task_t and return non-negative handle */
	g_curr_task->files[i] = f;
	return i;
}
/*****************************************************************************
*****************************************************************************/
int close(unsigned handle)
{
	file_t *f;
	int rv;

	if(handle >= NUM_FILES)
		return -ERR_BAD_ARG;
	f = g_curr_task->files[handle];
	if(f == NULL)
		return -ERR_NOT_OPEN;
	if(f->ops == NULL || f->ops->close == NULL)
		return -1;
	rv = f->ops->close(f);
	kfree(f);
	g_curr_task->files[handle] = NULL;
	return rv;
}
/*****************************************************************************
*****************************************************************************/
int write(unsigned handle, void *buf, unsigned len)
{
	file_t *f;

	if(handle >= NUM_FILES)
		return -ERR_BAD_ARG;
	f = g_curr_task->files[handle];
	if(f == NULL)
		return -ERR_NOT_OPEN;
	if(f->ops == NULL || f->ops->write == NULL)
		return -1;
	return f->ops->write(f, buf, len);
}
/*****************************************************************************
*****************************************************************************/
int read(unsigned handle, void *buf, unsigned len)
{
	file_t *f;

	if(handle >= NUM_FILES)
		return -ERR_BAD_ARG;
	f = g_curr_task->files[handle];
	if(f == NULL)
		return -ERR_NOT_OPEN;
	if(f->ops == NULL || f->ops->read == NULL)
		return -1;
	return f->ops->read(f, buf, len);
}
/*****************************************************************************
*****************************************************************************/
int select(unsigned handle, unsigned access, unsigned *timeout)
{
	static wait_queue_t delay_wq;
/**/
	file_t *f;

/* access==0 means just delay (no checking if I/O is ready) */
	if(access == 0)
	{
/* wake_up() is never called for delay_wq, so return value doesn't matter */
		(void)sleep_on(&delay_wq, timeout);
		return 0; /* 0 means timeout */
	}
	if(handle >= NUM_FILES)
		return -ERR_BAD_ARG;
	f = g_curr_task->files[handle];
	if(f == NULL)
		return -ERR_NOT_OPEN;
	if(f->ops == NULL || f->ops->select == NULL)
		return -1;
	return f->ops->select(f, access, timeout);
}
/*****************************************************************************
*****************************************************************************/
void syscall(uregs_t *regs)
{
	unsigned uvirt_to_kvirt = 0, flags;

/* no address translation needed for kernel threads */
	if(g_curr_task->as != NULL)
	{
/* this value is applied only to the EBX register */
		uvirt_to_kvirt = (unsigned)g_curr_task->as->user_mem -
			g_curr_task->as->virt_adr;
/* NULL pointers are special -- don't translate them (!) */
		if(regs->ebx == 0)
			uvirt_to_kvirt = 0;
	}
/* run syscalls with interrupts enabled */
	flags = enable();
	switch(regs->eax)
	{
	case SYS_OPEN:
		regs->eax = open((char *)(regs->ebx + uvirt_to_kvirt),
			regs->ecx);
		break;
	case SYS_CLOSE:
		regs->eax = close(regs->edx);
		break;
	case SYS_WRITE:
		regs->eax = write(regs->edx,
			(char *)(regs->ebx + uvirt_to_kvirt), regs->ecx);
		break;
	case SYS_READ:
		regs->eax = read(regs->edx,
			(char *)(regs->ebx + uvirt_to_kvirt), regs->ecx);
		break;
	case SYS_SELECT:
		regs->eax = select(regs->edx, regs->ecx,
			(unsigned *)(regs->ebx + uvirt_to_kvirt));
		break;
	case SYS_TIME:
		regs->eax = time(NULL);
		break;
	case SYS_EXIT:
		_exit(regs->ebx);
		break;
	default:
		kprintf("Illegal syscall 0x%X; task 0x%p killed\n",
			regs->eax, g_curr_task);
		_exit(-1);
		break;
	}
	restore_flags(flags);
}
/*****************************************************************************
*****************************************************************************/
int lookup(char *sym_name, unsigned *adr, char underbars)
{
	static struct
	{
		char *name;
		unsigned adr;
	} syms[] =
	{
		{
			"open", (unsigned)open
		}, {
			"close", (unsigned)close
		}, {
			"write", (unsigned)write
		}, {
			"read", (unsigned)read
		}, {
			"select", (unsigned)select
		}, {
			"time", (unsigned)time
		}, {
			"_exit", (unsigned)_exit
		}
	};
/**/
	unsigned i;

	if(underbars)
	{
		if(sym_name[0] != '_')
		{
			kprintf("lookup: symbol '%s' has "
				"no leading underscore\n", sym_name);
			return -ERR_RELOC;
		}
		sym_name++;
	}
	for(i = 0; i < sizeof(syms) / sizeof(syms[0]); i++)
	{
		if(!strcmp(syms[i].name, sym_name))
		{
			*adr = syms[i].adr;
			return 0;
		}
	}
	kprintf("lookup: undefined external symbol '%s'\n",
		sym_name);
	return -ERR_RELOC;
}
