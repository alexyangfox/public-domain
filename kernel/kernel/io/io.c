#include <kernel/types.h>
#include <kernel/device.h>
#include <kernel/fs.h>
#include <kernel/lists.h>
#include <kernel/sync.h>
#include <kernel/utility.h>
#include <kernel/kmalloc.h>
#include <kernel/dbg.h>
#include <kernel/proc.h>
#include "io.h"



/*
 * BeginIO();
 *
 * Call the device driver's beginio() interface function.
 */

void BeginIO (void *ioreq, struct MsgPort *reply_port)
{
	struct StdIOReq *stdreq = ioreq;

	stdreq->msg.state = MSG_STATE_DEQUEUED;
	stdreq->msg.data = stdreq;

	if (reply_port == NULL)
		stdreq->msg.reply_port = &current_process->reply_port;
	else
		stdreq->msg.reply_port = reply_port;
	
	stdreq->device->beginio (ioreq);
}




/*
 * SendIO();
 */

void SendIO (void *ioreq, struct MsgPort *reply_port)
{
	struct StdIOReq *stdreq = ioreq;

	stdreq->msg.state = MSG_STATE_DEQUEUED;
	stdreq->msg.data = stdreq;

	if (reply_port == NULL)
		stdreq->msg.reply_port = &current_process->reply_port;
	else
		stdreq->msg.reply_port = reply_port;

	stdreq->flags &= ~IOF_QUICK;
	stdreq->device->beginio (ioreq);
}




/*
 * DoIO();
 */

int DoIO (void *ioreq, struct MsgPort *reply_port)
{
	struct StdIOReq *stdreq = ioreq;

	KASSERT (stdreq != NULL);
	KASSERT (stdreq->device != NULL);
	
	stdreq->msg.state = MSG_STATE_DEQUEUED;
	stdreq->msg.data = stdreq;

	if (reply_port == NULL)
		stdreq->msg.reply_port = &current_process->reply_port;
	else
		stdreq->msg.reply_port = reply_port;

	stdreq->flags |= IOF_QUICK;
	
	stdreq->device->beginio (ioreq);
	
	if ((stdreq->flags & IOF_QUICK) == 0)
		WaitIO (ioreq);
	
	return stdreq->error;
}




/*
 *
 */

int WaitIO (void *ioreq)
{
	struct StdIOReq *stdreq = ioreq;
		
	do
	{	
		KWait (1 << stdreq->msg.reply_port->signal);
	} while (stdreq->msg.state != MSG_STATE_REPLY);
	
	RemoveMsg (&stdreq->msg);
	
	return stdreq->error;
}




/*
 * DoIOAbortable();
 *
 * Similar to DoIO()  except calls WaitIOAbortable() that waits for kernel
 * signals in ksig_mask.  If a signal is received the IO request is aborted
 * by calling AbortIO on the device.  This is currently used by the filesystem
 * on long system calls such as read() and write() that can block for indefinite
 * periods such as waiting for input from the console.  This allows read() and
 * write() to be aborted when user-signals such as ctrl-d and ctrl-c, SIGTERM
 * etc arrive.
 */

int DoIOAbortable (void *ioreq, struct MsgPort *reply_port, uint32 ksig_mask)
{
	struct StdIOReq *stdreq = ioreq;


	KASSERT (stdreq->device != NULL);


	stdreq->msg.state = MSG_STATE_DEQUEUED;
	stdreq->msg.data = stdreq;

	if (reply_port == NULL)
		stdreq->msg.reply_port = &current_process->reply_port;
	else
		stdreq->msg.reply_port = reply_port;

	stdreq->flags |= IOF_QUICK;
	stdreq->device->beginio (ioreq);
	
	if ((stdreq->flags & IOF_QUICK) == 0)
		WaitIOAbortable (ioreq, ksig_mask);
	
	return stdreq->error;
}





/* 
 * WaitIOAbortable();
 *
 * Similar to WaitIO(),  see DoIOAbortable() description.
 */

int WaitIOAbortable (void *ioreq, uint32 ksig_mask)
{
	struct StdIOReq *stdreq = ioreq;
	struct StdIOReq abortreq;
	uint32 signals;

	
	do 
	{	
		signals = KWait (1 << stdreq->msg.reply_port->signal | ksig_mask);
		
		if ((signals & (1 << stdreq->msg.reply_port->signal)) == 0)
		{
			abortreq.device = stdreq->device;
			abortreq.cmd = CMD_ABORT;
			
			abortreq.msg.state = MSG_STATE_DEQUEUED;
			abortreq.msg.data = &abortreq;
			abortreq.msg.reply_port = stdreq->msg.reply_port;
			abortreq.flags |= IOF_QUICK;

			abortreq.abort_ioreq = (struct StdIOReq *) ioreq;
					
			stdreq->device->abortio (&abortreq);
			
			if ((abortreq.flags & IOF_QUICK) == 0)
			{
				do
				{
					KPRINTF ("WaitIOAbortable() aborting wait");
					
					KWait (1 << abortreq.msg.reply_port->signal);
				} while (abortreq.msg.state != MSG_STATE_REPLY);
				
				RemoveMsg (&abortreq.msg);
			}
		}
	} while (stdreq->msg.state != MSG_STATE_REPLY);
	
	RemoveMsg (&stdreq->msg);

	return stdreq->error;
}









/*
 * No idea!
 */

int CheckIO (void *ioreq)
{
	struct StdIOReq *stdreq = ioreq;
	
	if (stdreq->msg.state == MSG_STATE_REPLY)
		return 0;
	else
		return -1;
}




/*
 * Simple!
 */

int AbortIO (void *ioreq, struct MsgPort *reply_port)
{
	struct StdIOReq *stdreq = ioreq;
	
	return stdreq->device->abortio (stdreq);
}
