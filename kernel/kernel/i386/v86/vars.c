#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/i386/i386.h>
#include <kernel/dbg.h>
#include <kernel/utility.h>
#include <kernel/proc.h>
#include <kernel/config.h>
#include "v86.h"






bool v86_if;
uint32 bioscall_entry_esp;
uint32 bioscall_entry_ss;

struct Process *v86_proc;
int v86_init_pid;
struct MsgPort *v86_msgport;
struct V86Msg *handler_v86msg;


