#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/dbg.h>
#include <kernel/fs.h>
#include <kernel/kmalloc.h>
#include <kernel/i386/elf.h>
#include <kernel/i386/i386.h>
#include <kernel/utility.h>
#include <kernel/error.h>




/*
 * static prototypes
 */

struct Process *AllocProcessStruct (int type);
void FreeProcessStruct (struct Process *proc);
static int LoadProcess (struct Process *proc, int fd, vm_addr *entry_point);
static void *UMalloc (int32 sz, struct UMallocState *us);




/*
 * AllocProcess();
 */

struct Process *AllocProcess (int type)
{
	struct Process *proc;
	
	
	if ((proc = AllocProcessStruct(type)) != NULL)
	{
		if ((proc->exe_name = KMalloc(PROC_NAME_SZ)) != NULL)
		{
			*proc->exe_name = '\0';
			
			if (0 == ArchAllocProcess (proc))
			{
				if ((proc->kernel_stack = KMap(KERNEL_STACK_SZ, VM_PROT_READWRITE)) != MAP_FAILED)
				{
					return proc;
				}
				
				ArchFreeProcess (proc);
			}
		
			KFree (proc->exe_name);
		}
		FreeProcessStruct (proc);
	}
		
	return NULL;
}




/*
 * FreeProcess();
 */

void FreeProcess (struct Process *proc)
{
	KUnmap (proc->kernel_stack);
	ArchFreeProcess (proc);
	KFree (proc->exe_name);
	FreeProcessStruct (proc);
}




/*
 * SetProcessName();
 *
 *
 * IMPROVEMENT: Replace with either the canonical pathname or the translated
 * pathname.
 */

int SetProcessName (struct Process *proc, char *filename)
{
	int rc;
	
	rc = StrLCpy (proc->exe_name, filename, PROC_NAME_SZ);
	
	rc = 0;	
	return rc;
}




/*
 * CheckELFHeaders();
 *
 * Read ELF Header into the kernel, hence need for SwitchAddressSpace().
 * Check that it is a genuine ELF executable.
 */

int CheckELFHeaders (int fd)
{
	Elf32_EHdr ehdr;
	struct AddressSpace *as;
	int rc;
	
	
	KPRINTF ("CheckElfHeaders()");

	as = SwitchAddressSpace (&kernel_as);
	rc = Read (fd, &ehdr, sizeof (Elf32_EHdr));
	SwitchAddressSpace (as);
	 
	if (rc != -1)
	{
		Seek (fd, 0, SEEK_SET);

		if (ehdr.e_ident[EI_MAG0] == ELFMAG0 && 
			ehdr.e_ident[EI_MAG1] == 'E' && 
			ehdr.e_ident[EI_MAG2] == 'L' && 
			ehdr.e_ident[EI_MAG3] == 'F' && 
			ehdr.e_ident[EI_CLASS] == ELFCLASS32 &&
			ehdr.e_ident[EI_DATA] == ELFDATA2LSB &&
			ehdr.e_type == ET_EXEC &&
			ehdr.e_phnum > 0)
		{
			KPRINTF ("FILE IS ELF EXECUTABLE");
			return 0;
		}
	}
	
	KPRINTF ("FILE IS NOT EXECUTABLE");
		
	SetError (ENOEXEC);
	
	return -1;
}




/*
 * PopulateAddressSpace();
 */

int PopulateAddressSpace (struct Process *newproc, int fd,
								vm_addr *entry_point, vm_addr *stack_pointer)
{
	struct AddressSpace *as;
	vm_addr stack_base;
	int rc;
	
	MutexLock (&loader_mutex);

	as = SwitchAddressSpace (newproc->user_as);
	
	if (LoadProcess (newproc, fd, entry_point) == 0)
	{		
		if ((stack_base = UMap (0, USER_STACK_SZ, VM_PROT_READWRITE, 0)) != MAP_FAILED)
		{
			*stack_pointer = stack_base + USER_STACK_SZ;
			rc = 0;
		}
		else
			rc = -1;
	}
	else
	{
		KPRINTF ("Load process failed");
		rc = -1;
	}

	SwitchAddressSpace (as);


	MutexUnlock (&loader_mutex);

	return rc;
}
						




/*
 * AllocProcessStruct();
 */
 
struct Process *AllocProcessStruct (int type)
{
	struct Process *proc;
	struct Process *pgrp_proc;
	
	
	MutexLock (&proc_mutex);
		
	proc = LIST_HEAD (&free_process_list);
	
	if (proc != NULL)
	{
		LIST_REM_HEAD (&free_process_list, free_entry);
		proc->state = PROC_STATE_INIT;
		proc->parent = current_process;
		
		LIST_INIT (&proc->child_list);
		LIST_ADD_TAIL (&current_process->child_list, proc, child_entry);
		
		proc->pid = (proc - process) + 1;
		proc->uid = current_process->uid;
		proc->gid = current_process->gid;
		proc->euid = current_process->euid;
		proc->egid = current_process->egid;
		proc->umask = current_process->umask;
		
		proc->pgrp = current_process->pgrp;
		proc->pgrp_reference_cnt = 0;
		
		pgrp_proc = PIDtoProc (proc->pgrp);
		pgrp_proc->pgrp_reference_cnt ++;
				
		proc->priority = 0;
		proc->preempt_cnt = 0;	
		proc->quanta_used = 0;
		proc->status = 0;
		proc->free_signals = ~SIGNALS_RESERVED;
		proc->pending_signals = 0;
		proc->waiting_for_signals = 0;
		proc->error = 0;
		
		InitTimer (&proc->ualarm_timer);
		
		proc->reply_port.signal = SIGF_FS;
		proc->reply_port.pid = (proc - process) + 1;
		LIST_INIT (&proc->reply_port.msg_list);
		
		proc->type = type;
		
		if (type == PROC_TYPE_USER)
			user_process_cnt ++;
	}

	MutexUnlock (&proc_mutex);
	
	return proc;
}		




/*
 * FreeProcessStruct();
 *
 * Only parent can free process struct.
 */

void FreeProcessStruct (struct Process *proc)
{	
	struct Process *pgrp_proc;
	
	MutexLock (&proc_mutex);
	
	LIST_REM_ENTRY (&current_process->child_list, proc, child_entry);

	if (proc->type == PROC_TYPE_USER)
		user_process_cnt --;


	proc->state = PROC_STATE_UNALLOC;
	
	pgrp_proc = PIDtoProc (proc->pgrp);
	pgrp_proc->pgrp_reference_cnt --;

	if (current_process->pgrp_reference_cnt == 0)
	{	
		LIST_ADD_HEAD (&free_process_list, proc, free_entry); 
	}

	if (pgrp_proc != pgrp_proc && pgrp_proc->pgrp_reference_cnt == 0)
	{
		pgrp_proc->state = PROC_STATE_UNALLOC;
		LIST_ADD_HEAD (&free_process_list, pgrp_proc, free_entry); 
	}

	
	MutexUnlock (&proc_mutex);
}




/*
 * LoadProcess();
 */

static int LoadProcess (struct Process *proc, int fd, vm_addr *entry_point)
{
	int t;
	int rc;
	struct UMallocState us;
	Elf32_EHdr *ehdr;
	Elf32_PHdr *phdr;
	Elf32_PHdr *phdr_table;
	int32 phdr_cnt;
	off_t phdr_offs, sec_offs;
	vm_addr sec_addr;
	int32 sec_file_sz;
	vm_size sec_mem_sz;
	uint32 sec_prot;
	vm_addr ret_addr;
	

	KPRINTF ("LoadProcess()");


	us.base = NULL;
	us.size = 0;
	
	if ((ehdr = UMalloc (sizeof (Elf32_EHdr), &us)) == NULL)
		return -1;
		
	if (Read (fd, ehdr, sizeof (Elf32_EHdr)) == -1)
		return -1;
		
	phdr_cnt = ehdr->e_phnum;
	phdr_offs = ehdr->e_phoff;
	*entry_point = ehdr->e_entry;
	
	if ((phdr_table = UMalloc (sizeof (Elf32_PHdr) * phdr_cnt, &us)) == NULL)
		return -1;
			
	if (Seek (fd, phdr_offs, SEEK_SET) == -1)
		return -1;

	if (Read (fd, phdr_table, sizeof (Elf32_PHdr) * phdr_cnt) == -1)
		return -1;
		
	for (t=0; t < phdr_cnt; t++)
	{
		phdr = phdr_table+t;
		
		if (phdr->p_type != PT_LOAD)
			continue;
		
		
		sec_addr = phdr->p_vaddr;
		sec_file_sz = phdr->p_filesz;
		sec_mem_sz = phdr->p_memsz;
		sec_offs = phdr->p_offset;
		sec_prot = 0;
		
		
		if (sec_mem_sz < sec_file_sz)
			return -1;
		
		
		if (phdr->p_flags & PF_X)
			sec_prot |= VM_PROT_EXEC;
		if (phdr->p_flags & PF_R)
			sec_prot |= VM_PROT_READ;
		if (phdr->p_flags & PF_W)
			sec_prot |= VM_PROT_WRITE;
		
		
		sec_addr = ALIGN_DOWN (phdr->p_vaddr, phdr->p_align);
		sec_mem_sz = phdr->p_memsz;
		
		KASSERT (current_process->user_as == &proc->as);
				
		if (sec_mem_sz != 0)
		{
			ret_addr = UMap(sec_addr, sec_mem_sz, VM_PROT_READWRITE, MAP_FIXED);
		
			if (ret_addr == MAP_FAILED)
				return -1;
		}
		
		if (sec_file_sz != 0)
		{
			if (Seek (fd, sec_offs, SEEK_SET) == -1)
				return -1;
			
					
			rc = Read (fd, (void *)phdr->p_vaddr, sec_file_sz);
			
			if (rc == -1)
				return -1;
		}
		
		UProtect (sec_addr, sec_prot);
	}
	
	
	return 0;
}




/*
 *
 */

static void *UMalloc (int32 sz, struct UMallocState *us)
{
	vm_addr addr;
	vm_size req_sz;
	void *base;
	
	if (us->base == NULL || us->size < sz)
	{
		if (sz <= 0x1000)
			req_sz = 0x1000;
		else
			req_sz = sz;
		
		if ((addr = UMap (0, req_sz, VM_PROT_READWRITE, 0)) != MAP_FAILED)
		{
			us->base = (void *)addr;
			us->size = req_sz;
		}
		else
			return NULL;
	}
	
	base = us->base;
	
	us->base += sz;
	us->size -= sz;
	
	return base;
}




