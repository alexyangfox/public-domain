#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/i386/i386.h>
#include <kernel/dbg.h>
#include <kernel/utility.h>
#include <kernel/proc.h>
#include <kernel/config.h>


extern void RescheduleResume;
extern void V86ProcStart (void);



void InitV86 (void);
struct Process *V86Spawn (void);
int CreateV86AddressSpace (struct AddressSpace *as);
int ArchInitV86Proc (struct Process *proc);
bool PmapV86Init (struct AddressSpace *as);
int32 V86Start (struct ContextState *cs);
int V86HandleOpcode (struct ContextState *state);
void *V86GetAddress (uint32 offset, uint32 segment);
void V86GetInterruptVector (uint32 vector, uint32 *eip, uint32 *cs);


void TestV86(void);
void DrawToScreen (void);


/*
 *
 */





#define PREFIX_O32			0x66
#define PREFIX_A32			0x67

#define OPCODE_INT			0xcd
#define OPCODE_IRET			0xcf
#define OPCODE_CLI			0xfa
#define OPCODE_STI			0xfb
#define OPCODE_PUSHF		0x9c
#define OPCODE_POPF			0x9d

#define OPCODE_INB			0xec
#define OPCODE_INWL			0xed
#define OPCODE_OUTB			0xee
#define OPCODE_OUTWL		0xef

#define OPCODE_OUTB_AL_NN	0xe6		
#define OPCODE_INB_NN_AL	0xe4
#define OPCODE_OUTWL_EAX_NN	0xe7
#define OPCODE_INWL_NN_EAX	0xe5

#define OPCODE_HLT			0xf4


#define V86_EFLAG_MASK			0x00007fff
#define EFLAG_IF				(1<<9)
#define EFLAG_TF				(1<<8)
#define EFLAG_VM				(1<<17)
#define EFLAG_AC				(1<<18)




/*
 *
 */


struct V86Msg
{
	struct Msg msg;
	
	struct ContextState *state;
	int vector;
};








/*
 *
 */

extern bool v86_if;
extern uint32 bioscall_iret_eip;
extern uint32 bioscall_iret_cs;

extern struct Process *v86_proc;
extern int v86_init_pid;
extern struct MsgPort *v86_msgport;
extern struct V86Msg *handler_v86msg;



