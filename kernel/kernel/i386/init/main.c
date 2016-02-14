#include <kernel/types.h>
#include <kernel/vm.h>
#include <kernel/proc.h>
#include <kernel/fs.h>
#include <kernel/dbg.h>
#include <kernel/i386/i386.h>
#include <kernel/i386/multiboot.h>
#include <kernel/i386/init.h>
#include <kernel/utility.h>
#include <kernel/i386/elf.h>
#include <acpi.h>




void InitDebug (void);
void InitI386 (void);
void InitVM (void);
void InitKMalloc (void);
void InitProc (void);
void InitConfigOptions  (void);
void InitACPI (void);
void InitIOManager (void);
void KillUserProcesses (void);
void KLogToScreenEnable (void);
void FiniIOManager (void);
void FiniProc (void);
void FiniI386 (void);




/*
 * Init();
 *
 * Main initialization routine of the kernel.  Called by Entry() in
 * i386/asm/entry.S.  entry.S sets up a temporary stack and initialises
 * the "mbi" pointer to point to the multiboot info structure.
 *
 * Init() is responsible for calling the initialization functions of
 * all the OS subsystems.
 *
 * InitVM() initializes the kernel memory management structures and pagetables,
 * it then enables paging.
 *
 * InitProc() is responsible for turning Init() into the root kernel thread and
 * creating an "idle" thread.
 *
 * Init() then creates the "/sys/boot/init" process and waits for zombie user
 * processes to terminate.
 *
 * Once all user processes are terminated Init() then calls functions to terminate
 * device drivers before shutting down.
 */
 
void Init (void)
{
	int init_pid;
	char *envr[1];
	uint32 signals;
		
		
	InitDebug();
	KPRINTF ("InitDebug() +");

	InitI386();
	KPRINTF ("InitI386() +");
	
	InitVM();
	KPRINTF ("InitVM() +");
	
	InitKMalloc();
	KPRINTF ("InitKMalloc() +");

	InitProc();
	KPRINTF("InitProc() +");
	
	InitConfigOptions();
	KPRINTF("InitConfigOptions() +");

	InitACPI();
	KPRINTF ("InitACPI() +");
		
	InitFS();
	KPRINTF ("InitFS() +");
	
	InitIOManager();
	KPRINTF("InitIOManager() +");
		
	MountBootDevices();
	KPRINTF ("MountBootDevices() +")

	DetermineBootMount();		/* FIXME: needs writer lock? */
	KPRINTF ("DetermineBootMount() +");
	
	envr[0] = '\0';
	init_pid = CreateProcess ("/sys/boot/init", NULL, envr);


	if (init_pid > 0)
	{
		do
		{
			signals = KWait (SIGF_CHLD | SIGF_TERM);
			
			if (signals & SIGF_CHLD)
				WaitPid (-1, NULL, 0);

			if (signals & SIGF_TERM)
			{
				KillUserProcesses();
			}
			
		} while (user_process_cnt > 0);
	}
	else
	{
		KPANIC ("Couldn't find /sys/boot/init");
	}
	
	
	/* FIXME */
	KPRINTF ("Shutdown Sequence");
	KWait (0);
	
	
	
	KLogToScreenEnable();
	
	
	
	KPRINTF ("UnmountAll()");
	UnmountAll();

	KPRINTF ("FiniIOManager()");
	FiniIOManager();
	
	KPRINTF ("FiniFS()");
	FiniFS();
		
	KPRINTF ("FiniProc()");
	FiniProc();
	
	KPRINTF ("FiniI386()");
	FiniI386();
		
	KPRINTF ("Goodnight!");
	FiniACPI();
		
	while (1)
	{	
		KWait (0);
	}
}



