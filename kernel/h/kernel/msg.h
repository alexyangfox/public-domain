#ifndef KERNEL_MSG_H
#define KERNEL_MSG_H

#include <kernel/types.h>
#include <kernel/lists.h>
/* #include <kernel/proc.h> */




struct Msg
{
	LIST_ENTRY (Msg) msg_entry;
	int32 state;
	struct MsgPort *reply_port;
	void *data;
	size_t size;
};


#define MSG_STATE_DEQUEUED	0
#define MSG_STATE_PUT		1
#define MSG_STATE_REPLY		2


/*
 *
 */

struct MsgPort
{
	LIST (Msg) msg_list;
	int pid;
	int32 signal;
};


#define MSGPORT_MODE_SIGNAL	1
#define MSGPORT_MODE_IGNORE	0



struct MsgQueue
{
	LIST (Msg) msg_list;
};




struct MsgPort *CreateMsgPort (void);
void DeleteMsgPort (struct MsgPort *msgport);
int WaitPort (struct MsgPort *msgport);


void PutMsg (struct MsgPort *port, struct Msg *msg);
struct Msg *GetMsg (struct MsgPort *port);
void RemoveMsg (struct Msg *msg);
void ReplyMsg (struct Msg *msg);

void InitMsgQueue (struct MsgQueue *queue);
void QueueMsg (struct MsgQueue *queue, struct Msg *msg);
struct Msg *DequeueMsg (struct MsgQueue *queue);
struct Msg *MsgQueueHead (struct MsgQueue *queue);

struct Msg *MsgQueueRemove (struct MsgQueue *queue, struct Msg *msg);
struct Msg *MsgQueueNext (struct Msg *msg);




#endif

