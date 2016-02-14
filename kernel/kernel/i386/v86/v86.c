#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/i386/i386.h>
#include <kernel/dbg.h>
#include <kernel/utility.h>
#include <kernel/proc.h>
#include <kernel/config.h>
#include "v86.h"




/*
 * Initialize the v86 BIOS calling mechanism.  A task is created that
 * has the lower 1MB identity mapped with PL3 privilege level.  The
 * task waits for incoming message requests passed to it by V86BIOSCall().
 * The task is split into two parts due to the way it returns to V86 mode
 * to carry out the request.  The two parts are V86Start() and V86GPHandler().
 * V86Start() is only called the once, the first time the v86 handling task
 * begins,  from then on V86GPHandler() takes over and is run whenever a
 * #GP fault occurs.
 */

void InitV86(void)
{
	v86_init_pid = (current_process - process) + 1;	
		
	if (V86Spawn() != NULL)
	{		
		KWait (SIGF_INIT);
	}
}




/*
 * V86BiosCall() 
 */
 
int V86BiosCall (uint8 vector, struct ContextState *state)
{
	struct V86Msg v86msg;
	
	v86msg.state = state;
	v86msg.vector = vector;
	
	
	/* Replace with DoIO() ??????? */
		
	v86msg.msg.reply_port = &current_process->reply_port;
	v86msg.msg.state = MSG_STATE_DEQUEUED;
	v86msg.msg.data = &v86msg;

	PutMsg (v86_msgport, &v86msg.msg);

	while (v86msg.msg.state != MSG_STATE_REPLY)
		KWait (1 << v86msg.msg.reply_port->signal);
	
	RemoveMsg (&v86msg.msg);

	
	return 0;
}









/*
 * V86Start()
 */

int32 V86Start (struct ContextState *state)
{
	struct Msg *msg;
	uint32 signals;
	
	
	if ((v86_msgport = CreateMsgPort()) != NULL)
	{
		KSignal (v86_init_pid, SIG_INIT);
	}
	else
	{
		KPANIC ("V86 error");
	}

	
	
	while (1)
	{
		signals = KWait (1 << v86_msgport->signal);
		
		if (signals & (1 << v86_msgport->signal))
			if ((msg = GetMsg (v86_msgport)) != NULL)
				break;
	}
	
	
	handler_v86msg = (struct V86Msg *) msg;
	
	state->eax = handler_v86msg->state->eax;
	state->ebx = handler_v86msg->state->ebx;
	state->ecx = handler_v86msg->state->ecx;
	state->edx = handler_v86msg->state->edx;
	state->esi = handler_v86msg->state->esi;
	state->edi = handler_v86msg->state->edi;
	state->ebp = handler_v86msg->state->ebp;
	
	state->gs = PL3_DATA_SEGMENT;
	state->fs = PL3_DATA_SEGMENT;
	state->es = PL3_DATA_SEGMENT;
	state->ds = PL3_DATA_SEGMENT;

	state->v86_gs = handler_v86msg->state->gs;
	state->v86_fs = handler_v86msg->state->fs;
	state->v86_es = handler_v86msg->state->es;
	state->v86_ds = handler_v86msg->state->ds;
	
	*(uint8 *)(0x00010000) = OPCODE_INT;
	*(uint8 *)(0x00010001) = handler_v86msg->vector;
	*(uint8 *)(0x00010002) = OPCODE_IRET;
		
	state->return_eip    = 0x0000;
	state->return_cs     = 0x1000;
	state->return_eflags = EFLG_IF | EFLG_VM;
	state->return_esp    = 0xfff0;
	state->return_ss     = 0x2000;

	return 0;
}




/*
 * V86GPHandler()
 */
 
int32 V86GPHandler (struct ContextState *state)
{
	struct Msg *msg;
	uint32 signals;
	
	
	if (V86HandleOpcode (state) == 1)
	{
		handler_v86msg->state->eax = state->eax;
		handler_v86msg->state->ebx = state->ebx;
		handler_v86msg->state->ecx = state->ecx;
		handler_v86msg->state->edx = state->edx;
		handler_v86msg->state->esi = state->esi;
		handler_v86msg->state->edi = state->edi;
		handler_v86msg->state->ebp = state->ebp;
		handler_v86msg->state->gs = state->v86_gs;
		handler_v86msg->state->fs = state->v86_fs;
		handler_v86msg->state->es = state->v86_es;
		handler_v86msg->state->ds = state->v86_ds;	
		handler_v86msg->state->return_eip = state->return_eip;
		handler_v86msg->state->return_cs = state->return_cs;
		handler_v86msg->state->return_eflags  = state->return_eflags;
		handler_v86msg->state->return_esp  = state->return_esp;
		handler_v86msg->state->return_ss = state->return_ss;
		
		ReplyMsg (&handler_v86msg->msg);
		
	
		if ((msg = GetMsg (v86_msgport)) == NULL)
		{
			while (1)
			{
				signals = KWait (1 << v86_msgport->signal);
			
				if (signals & (1 << v86_msgport->signal))
					if ((msg = GetMsg (v86_msgport)) != NULL)
						break;
			}
		}

	
		handler_v86msg = (struct V86Msg *) msg;
		
		state->eax = handler_v86msg->state->eax;
		state->ebx = handler_v86msg->state->ebx;
		state->ecx = handler_v86msg->state->ecx;
		state->edx = handler_v86msg->state->edx;
		state->esi = handler_v86msg->state->esi;
		state->edi = handler_v86msg->state->edi;
		state->ebp = handler_v86msg->state->ebp;

		state->gs = PL3_DATA_SEGMENT;
		state->fs = PL3_DATA_SEGMENT;
		state->es = PL3_DATA_SEGMENT;
		state->ds = PL3_DATA_SEGMENT;
	
		state->v86_gs = handler_v86msg->state->gs;
		state->v86_fs = handler_v86msg->state->fs;
		state->v86_es = handler_v86msg->state->es;
		state->v86_ds = handler_v86msg->state->ds;
		
		*(uint8 *)(0x00010000) = OPCODE_INT;
		*(uint8 *)(0x00010001) = handler_v86msg->vector;
		*(uint8 *)(0x00010002) = OPCODE_IRET;
		
		state->return_eip    = 0x0000;
		state->return_cs     = 0x1000;
		state->return_eflags = EFLG_IF | EFLG_VM;
		state->return_esp    = 0xfff0;
		state->return_ss     = 0x2000;
	}
	
	return 0;
}








/*
 * Return 0 for continue, 1 for syscall completed, -1 for Critical Error.
 */

int V86HandleOpcode (struct ContextState *state)
{
	bool is_operand32 = FALSE;
	bool is_address32 = FALSE;
	uint32 *sp32;
	uint16 *sp16;
	uint32 eflags;
	uint8 *ip;
		
	uint32 new_ip;
	uint32 new_cs;
	uint8 *vector;

	
	
	
	while (1)
	{
		ip = V86GetAddress (state->return_eip, state->return_cs);
		
		switch (*ip)
		{
			case PREFIX_O32:
				is_operand32 = TRUE;
				state->return_eip = (state->return_eip + 1) & 0x0000ffff;
				continue;
				
			case PREFIX_A32:
				is_address32 = TRUE;
				ip++;
				state->return_eip = (state->return_eip + 1) & 0x0000ffff;
				continue;
			
				
			case OPCODE_PUSHF:
				if (is_operand32 == TRUE)
				{
					state->return_esp = ((state->return_esp & 0x0000ffff) - 4) & 0x0000ffff;
					sp32 = V86GetAddress (state->return_esp, state->return_ss);
					eflags = state->return_eflags & V86_EFLAG_MASK;

					*sp32 = (v86_if == TRUE)
							? eflags | EFLAG_IF
							: eflags & ~EFLAG_IF;
				}
				else
				{
					state->return_esp = ((state->return_esp & 0x0000ffff) - 2) & 0x0000ffff; 
					sp16 = V86GetAddress (state->return_esp, state->return_ss);
					eflags = state->return_eflags & V86_EFLAG_MASK;

					*sp16 = (uint16) (v86_if == TRUE)
							? eflags | EFLAG_IF
							: eflags & ~EFLAG_IF;					
					
				}
			
				state->return_eip = (state->return_eip + 1) & 0x0000ffff;
				return 0;
				
			case OPCODE_POPF:
				if (is_operand32 == TRUE)
				{
					sp32 = V86GetAddress (state->return_esp, state->return_ss);
					eflags = *sp32;
					state->return_eflags = (eflags & V86_EFLAG_MASK) | EFLAG_IF | EFLAG_VM;
					v86_if = (eflags & EFLAG_IF) ? TRUE : FALSE;
					state->return_esp = ((state->return_esp & 0x0000ffff) + 4) & 0x0000ffff;
				}
				else
				{
					sp16 = V86GetAddress (state->return_esp, state->return_ss);
					eflags = *sp16;
					state->return_eflags = (eflags & V86_EFLAG_MASK) | EFLAG_IF | EFLAG_VM;
					v86_if = (eflags & EFLAG_IF) ? TRUE : FALSE;
					state->return_esp = ((state->return_esp & 0x0000ffff) + 2) & 0x0000ffff;
				}

				state->return_eip = (state->return_eip + 1) & 0x0000ffff;
				return 0;

			
			case OPCODE_INT:
			{
				state->return_eip = (state->return_eip + 1) & 0x0000ffff;
				vector = V86GetAddress (state->return_eip, state->return_cs);
				V86GetInterruptVector (*vector, &new_ip, &new_cs);
				state->return_eip = (state->return_eip + 1) & 0x0000ffff;
				
				state->return_esp = ((state->return_esp & 0x0000ffff) - 2) & 0x0000ffff;
				sp16 = V86GetAddress (state->return_esp, state->return_ss);
				*sp16 = (uint16) state->return_eip;
				
				state->return_esp = ((state->return_esp & 0x0000ffff) - 2) & 0x0000ffff;
				sp16 = V86GetAddress (state->return_esp, state->return_ss);
				*sp16 = (uint16) state->return_cs;
				
				state->return_esp = ((state->return_esp & 0x0000ffff) - 2) & 0x0000ffff;
				sp16 = V86GetAddress (state->return_esp, state->return_ss);
				
				eflags = (v86_if == TRUE)
						? (state->return_eflags & V86_EFLAG_MASK) | EFLAG_IF
						: (state->return_eflags & V86_EFLAG_MASK) & ~EFLAG_IF;
				
				*sp16 = (uint16) eflags;
				
				state->return_eflags = (state->return_eflags & ~(EFLAG_IF | EFLAG_TF | EFLAG_AC)) | EFLAG_VM;
				v86_if = FALSE;
				
				state->return_eip = new_ip & 0x0000ffff;
				state->return_cs = new_cs & 0x0000ffff;
				
				return 0;
			}
			
			case OPCODE_IRET:
				if (state->return_eip == 0x0002 && state->return_cs == 0x1000)
				{
					return 1;
				}
				else
				{
					sp16 = V86GetAddress (state->return_esp, state->return_ss);
					eflags = *sp16;
					
					eflags = (eflags & 0x257fd5) | (state->return_eflags & 0x1a0000);
					
					state->return_eflags = eflags | EFLAG_IF | EFLAG_VM;
					v86_if = (eflags & EFLAG_IF) ? TRUE : FALSE;
							
					state->return_esp = ((state->return_esp & 0x0000ffff) + 2) & 0x0000ffff;
					
					sp16 = V86GetAddress (state->return_esp, state->return_ss);
					state->return_cs = *sp16;
					state->return_esp = ((state->return_esp & 0x0000ffff) + 2) & 0x0000ffff;
					
					sp16 = V86GetAddress (state->return_esp, state->return_ss);
					state->return_eip = *sp16;
					state->return_esp = ((state->return_esp & 0x0000ffff) + 2) & 0x0000ffff;

					return 0;
				}
							
				
			case OPCODE_CLI:
				v86_if = FALSE;
				state->return_eip = (state->return_eip + 1) & 0x0000ffff;
				return 0;
							
			case OPCODE_STI:
				v86_if = TRUE;
				state->return_eip = (state->return_eip + 1) & 0x0000ffff;
				return 0;
					
		
			case OPCODE_OUTB:
				OutByte (state->edx, state->eax);
				state->return_eip = (state->return_eip + 1) & 0x0000ffff;
				return 0;
			
			case OPCODE_INB:
				state->eax = InByte (state->edx);
				state->return_eip = (state->return_eip + 1) & 0x0000ffff;
				return 0;
			
			case OPCODE_OUTWL:
				if(is_operand32 == FALSE)
					OutWord (state->edx, state->eax);
				else
					OutLong (state->edx, state->eax);
				
				state->return_eip = (state->return_eip + 1) & 0x0000ffff;
				return 0;
			
			case OPCODE_INWL:
				if(is_operand32 == FALSE)
					state->eax = InWord (state->edx);
				else
					state->eax = InLong (state->edx);

				state->return_eip = (state->return_eip + 1) & 0x0000ffff;
				return 0;

			case OPCODE_OUTB_AL_NN:
				OutByte (*(ip+1), state->eax);
				state->return_eip = (state->return_eip + 2) & 0x0000ffff;
				return 0;

			case OPCODE_INB_NN_AL:
				state->eax = InByte (*(ip+1));
				state->return_eip = (state->return_eip + 2) & 0x0000ffff;
				return 0;
			
			case OPCODE_OUTWL_EAX_NN:
				if (is_operand32 == FALSE)
					OutWord (*(ip+1), state->eax);
				else
					OutLong (*(ip+1), state->eax);

				state->return_eip = (state->return_eip + 2) & 0x0000ffff;
				return 0;
			
			case OPCODE_INWL_NN_EAX:
				if(is_operand32 == FALSE)
					state->eax = InWord (*(ip+1));
				else
					state->eax = InLong (*(ip+1));
				
				state->return_eip = (state->return_eip + 2) & 0x0000ffff;
				return 0;
			
			
			case OPCODE_HLT:
			{
				KPANIC ("Halt in V86");
			}
			
			default:
			{
				KPRINTF ("opcode = %#010x", *ip);
				KPANIC ("#GP Unknown V86 opcode");
			}
		}
	}
}






/*
*/

void *V86GetAddress (uint32 offset, uint32 segment)
{
	uint32 addr;
	
	addr = ((segment & 0x0000ffff) << 4)  + (offset & 0x0000ffff);
	return (void *)addr;
}




/*
*/

void V86GetInterruptVector (uint32 vector, uint32 *eip, uint32 *cs)
{
	uint32 csip;
	uint32 *ivt_base;
	
	ivt_base = (uint32 *)0x00000000;
	
	csip = *(ivt_base + (vector & 0x000000ff));
	
	*eip = (csip & 0x0000ffff);
	*cs = (csip & 0xffff0000) >> 16;
}



