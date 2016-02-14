#include <kernel/types.h>
#include <kernel/lists.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/msg.h>
#include <kernel/kmalloc.h>




/*
 * CreateMsgPort();
 */
 
struct MsgPort *CreateMsgPort (void)
{
	struct MsgPort *msgport;
	
	KPRINTF ("CreateMsgPort()");

	if ((msgport = KMalloc (sizeof (struct MsgPort))) != NULL)
	{
		if ((msgport->signal = AllocSignal()) != -1)
		{
			msgport->pid = (current_process - process) + 1;
			LIST_INIT (&msgport->msg_list);
			
			return msgport;
		}

		KFree (msgport);
	}
	
	return NULL;
}




/*
 * DeleteMsgPort();
 */

void DeleteMsgPort (struct MsgPort *msgport)
{
	FreeSignal (msgport->signal);
	KFree (msgport);
}



/*
 * InitMsg()
 */






/*
 * PutMsg()
 */

void PutMsg (struct MsgPort *msgport, struct Msg *msg)
{
	DisableInterrupts();

	LIST_ADD_TAIL (&msgport->msg_list, msg, msg_entry);
	msg->state = MSG_STATE_PUT;
	
	KSignal (msgport->pid, msgport->signal);
		
	EnableInterrupts();
}




/*
 * GetMsg()
 */

struct Msg *GetMsg (struct MsgPort *msgport)
{
	struct Msg *msg;
	
	DisableInterrupts();

	msg = LIST_HEAD (&msgport->msg_list);
	
	if (msg != NULL)
	{
		LIST_REM_HEAD (&msgport->msg_list, msg_entry);
		msg->state = MSG_STATE_DEQUEUED;
	}

	EnableInterrupts();

	return msg;
}



/*
 * What uses RemoveMsg()  DoIO()???????
 */
 
void RemoveMsg (struct Msg *msg)
{
	DisableInterrupts();
	
	LIST_REM_ENTRY (&msg->reply_port->msg_list, msg, msg_entry);
	msg->state = MSG_STATE_DEQUEUED;
	
	EnableInterrupts();
}




/*
 * ReplyMsg()
 */
 
void ReplyMsg (struct Msg *msg)
{
	DisableInterrupts();
	
	if (msg->reply_port != NULL)
	{
		LIST_ADD_TAIL (&msg->reply_port->msg_list, msg, msg_entry);
		msg->state = MSG_STATE_REPLY;
	
		KSignal (msg->reply_port->pid, msg->reply_port->signal);
	}
	else
		msg->state = MSG_STATE_DEQUEUED;

	EnableInterrupts();
}




/*
 *
 */

void InitMsgQueue (struct MsgQueue *queue)
{
	LIST_INIT (&queue->msg_list);
}




/*
 *
 */

void QueueMsg (struct MsgQueue *queue, struct Msg *msg)
{
	LIST_ADD_TAIL (&queue->msg_list, msg, msg_entry);
}




/*
 *
 */

struct Msg *DequeueMsg (struct MsgQueue *queue)
{
	struct Msg *msg;
		
	
	msg = LIST_HEAD (&queue->msg_list);
	
	if (msg != NULL)
	{
		LIST_REM_HEAD (&queue->msg_list, msg_entry);
	}
	
	return msg;
}




/*
 *
 */

struct Msg *MsgQueueHead (struct MsgQueue *queue)
{
	struct Msg *msg;
	
	msg = LIST_HEAD (&queue->msg_list);
	
	return msg;
}




/*
 *
 */
 
struct Msg *MsgQueueRemove (struct MsgQueue *queue, struct Msg *msg)
{
	LIST_REM_ENTRY (&queue->msg_list, msg, msg_entry);
	
	return msg;
}




/*
 *
 */
 
struct Msg *MsgQueueNext (struct Msg *msg)
{
	struct Msg *next;
	
	next = LIST_NEXT (msg, msg_entry);

	return next;
}
