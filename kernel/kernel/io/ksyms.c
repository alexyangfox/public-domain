#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/fs.h>
#include <kernel/kmalloc.h>
#include <kernel/utility.h>
#include <kernel/error.h>
#include <kernel/symbol.h>
#include <kernel/buffers.h>
#include <kernel/device.h>
#include <kernel/timer.h>




/*
 * Kernel Symbol Table
 */

struct KSym kernel_symbol_table[] =
{
	/* Process Management */
	
	{"current_process",				&current_process},
	{"process",						&process},		
	{"process_cnt",					&process_cnt},		
	{"proc_mutex",					&proc_mutex},
	
	{"KSpawn",						&KSpawn},
	{"Exit",						&Exit},
	{"WaitPid",						&WaitPid},
	{"GetPID",						&GetPID},	
	{"GetPPID",						&GetPPID},
	{"PIDtoProc",					&PIDtoProc},
	
	{"Yield",						&Yield},
	{"ChangePriority",				&ChangePriority},

	{"AllocSignal",					&AllocSignal},	
	{"FreeSignal",					&FreeSignal},	
	{"KSignal",						&KSignal},
	{"KSetSignals",					&KSetSignals},
	{"KWait",						&KWait},
	{"KTimedWait",					&KTimedWait},
	
	
	/* Per process/task error state */
	
	{"GetError",					&GetError},
	{"SetError",					&SetError},
	
	
	/* Un*x-like signal handling for User-Processes */
	
	{"UKill",						&UKill},
	
	
	/* Time */
	
	{"LocalTime",					&LocalTime},
	{"MakeTime",					&MakeTime},
	{"DiffTime",					&DiffTime},
	{"AddTime",						&AddTime},
	{"CompareTime",					&CompareTime},
	
	{"KGetTimeOfDay",				&KGetTimeOfDay},
	
	
	/* Timers & Alarms */
	
	{"SetTimer",					&SetTimer},
	{"CancelTimer",					&CancelTimer},
	{"InitTimer",					&InitTimer},
	{"KSleep",						&KSleep},
	{"KSleep2",						&KSleep2},
	{"KAlarmSet",					&KAlarmSet},
	{"KAlarmCheck",					&KAlarmCheck},
	{"KAlarmCancel",				&KAlarmCancel},
	
	
	/* Synchronization */
	
	{"MutexInit",					&MutexInit},
	{"MutexTryLock",				&MutexTryLock},
	{"MutexLock",					&MutexLock},
	{"MutexUnlock",					&MutexUnlock},
	
	{"CondInit",					&CondInit},
	{"CondSignal",					&CondSignal},
	{"CondBroadcast",				&CondBroadcast},
	{"CondWait",					&CondWait},
	{"CondTimedWait",				&CondTimedWait},
	
	{"RWInit",						&RWInit},
	{"RWReadLock",					&RWReadLock},
	{"RWWriteLock",					&RWWriteLock},
	{"RWUnlock",					&RWUnlock},

	{"RecMutexInit",				&RecMutexInit},
	{"RecMutexTryLock",				&RecMutexTryLock},
	{"RecMutexLock",				&RecMutexLock},
	{"RecMutexUnlock",				&RecMutexUnlock},
	
	
	/* Kernel message passing */
	
	{"CreateMsgPort",				&CreateMsgPort},
	{"DeleteMsgPort",				&DeleteMsgPort},
	{"PutMsg",						&PutMsg},
	{"GetMsg",						&GetMsg},
	{"RemoveMsg",					&RemoveMsg},
	{"ReplyMsg",					&ReplyMsg},


	/* Functions for queueing and storing received messages */

	{"InitMsgQueue",				&InitMsgQueue},
	{"QueueMsg",					&QueueMsg},
	{"DequeueMsg",					&DequeueMsg},
	{"MsgQueueHead",				&MsgQueueHead},
	{"MsgQueueRemove",				&MsgQueueRemove},
	{"MsgQueueNext",				&MsgQueueNext},
	
	
	/* Interrupt Handlers */
	
	{"ISRHandlerInsert",			&ISRHandlerInsert},
	{"ISRHandlerRemove",			&ISRHandlerRemove},
	
		
	/* Memory Management */
	
	{"KMalloc",						&KMalloc},
	{"KFree",						&KFree},
	{"KMap",						&KMap},
	{"KMapPhys",					&KMapPhys},
	{"KMapProtect",					&KMapProtect},
	{"KUnmap",						&KUnmap},
	
	
	/* User/Kernel Memory Copying */
	
	{"CopyIn",						&CopyIn},
	{"CopyOut",						&CopyOut},
	{"CopyInStr",					&CopyInStr},
	
		
	/* Mounting Filesystems/Handlers */
	
	{"mountlist_rwlock", 			&mountlist_rwlock},
	{"MakeMount",					&MakeMount},
	{"AddMount",					&AddMount},
	{"RemMount",					&RemMount},
	{"Mount",						&Mount},
	{"Unmount",						&Unmount},
	
	
	/* Filesystem Handler Buffers */
	
	{"CreateBuf",					&CreateBuf},
	{"FreeBuf", 					&FreeBuf},
	{"SyncBuf", 					&SyncBuf},
	{"InvalidateBuf", 				&InvalidateBuf},		
	{"BufReadBlocks", 				&BufReadBlocks},
	{"BufWriteBlocks", 				&BufWriteBlocks},
	{"BufGetBlock", 				&BufGetBlock},
	{"BufPutBlock", 				&BufPutBlock},
	
	
	/* Filesystem Interface */
	
	{"Open", 						&Open},
	{"Close", 						&Close},
	{"Unlink", 						&Unlink},
	{"Pipe", 						&Pipe},
	{"Dup", 						&Dup},
	{"Dup2", 						&Dup2},
	{"Ftruncate", 					&Ftruncate},
	{"Fstat", 						&Fstat},
	{"Fstatvfs", 					&Fstatvfs},
	{"Stat", 						&Stat},
	{"Isatty", 						&Isatty},
	{"Fsync", 						&Fsync},
	{"Sync", 						&Sync},
	{"Read", 						&Read},
	{"Write", 						&Write},
	{"Seek", 						&Seek},
	{"Getcwd", 						&Getcwd},
	{"Chdir", 						&Chdir},
	{"Mkdir", 						&Mkdir},
	{"Rmdir", 						&Rmdir},
	{"Opendir", 					&Opendir},
	{"Closedir", 					&Closedir},
	{"Readdir", 					&Readdir},
	{"Rewinddir", 					&Rewinddir},
	{"Tcgetattr", 					&Tcgetattr},
	{"Tcsetattr", 					&Tcsetattr},
	{"Fcntl", 						&Fcntl},
	{"Access", 						&Access},
	{"Ioctl", 						&Ioctl},
	{"Umask", 						&Umask},
	{"Format", 						&Format},
	{"Relabel", 					&Relabel},
	{"Rename", 						&Rename},
	{"Tcsetpgrp", 					&Tcsetpgrp},
	{"Tcgetpgrp", 					&Tcgetpgrp},
	{"Chmod", 						&Chmod},
	{"Chown", 						&Chown},
	{"Inhibit", 					&Inhibit},
	{"Uninhibit", 					&Uninhibit},
	{"SetAssign", 					&SetAssign},
	{"GetAssign", 					&GetAssign},


	/* Pathname/Pathinfo Handling */
	
	{"Advance", 					&Advance},	
	{"CompareComponent",			&CompareComponent},
	{"AllocPathname", 				&AllocPathname},
		
	
	/* Device IO */
	
	{"OpenDevice",					&OpenDevice},
	{"CloseDevice",					&CloseDevice},
	{"AddDevice",					&AddDevice},
	{"RemDevice",					&RemDevice},
	{"BeginIO", 					&BeginIO},
	{"SendIO", 						&SendIO},
	{"DoIO", 						&DoIO},
	{"WaitIO", 						&WaitIO},
	{"DoIOAbortable", 				&DoIOAbortable},
	{"WaitIOAbortable", 			&WaitIOAbortable},
	{"CheckIO", 					&CheckIO},
	
	
		
		
	/* Misc, kernel opts, i386 flags? */
	
	{"DoPrintf", 					&DoPrintf},
	{"Rand", 						&Rand},
	{"Snprintf", 					&Snprintf},
	{"Vsnprintf", 					&Vsnprintf},
	{"StrDup", 						&StrDup},
	{"StrLen", 						&StrLen},
	{"StrCmp", 						&StrCmp},
	{"StrChr", 						&StrChr},
	{"AtoI", 						&AtoI},
	{"StrLCat", 					&StrLCat},
	{"StrLCpy", 					&StrLCpy},
	
	
	/* Debugging */
	
	{"KLog",						&KLog},
	{"KPanic",						&KPanic},
	
		
	/* ACPI */
		
	
	/* Terminator */
	
	{NULL,		NULL}
};




/*
 * FindKernelSymbol();
 */

struct KSym *FindKernelSymbol (char *name)
{
	struct KSym *k;
	
	
	k = kernel_symbol_table;
	
	while (k->name != NULL)
	{
		if (StrCmp (k->name, name) == 0)
			return k;
			
		k++;
	}	
	
	KPRINTF ("FindKernelSymbol(%s) FAILED", name);
		
	return NULL;
}

