#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/i386/i386.h>
#include <kernel/utility.h>
#include <kernel/i386/init.h>


extern void DoubleFaultHandler (void);
extern void ExceptionDE (void);
extern void ExceptionDB (void);
extern void ExceptionNMI (void);
extern void ExceptionBP (void);
extern void ExceptionOF (void);
extern void ExceptionBR (void);
extern void ExceptionUD (void);
extern void ExceptionNM (void);
extern void ExceptionDF (void);
extern void ExceptionCSO (void);
extern void ExceptionTS (void);
extern void ExceptionNP (void);
extern void ExceptionSS (void);
extern void ExceptionGP (void);
extern void ExceptionPF (void);
extern void ExceptionMF (void);
extern void ExceptionAC (void);
extern void ExceptionMC (void);
extern void ExceptionXF (void);
	
extern void ISR0Stub (void);
extern void ISR1Stub (void);
extern void ISR2Stub (void);
extern void ISR3Stub (void);
extern void ISR4Stub (void);
extern void ISR5Stub (void);
extern void ISR6Stub (void);
extern void ISR7Stub (void);
extern void ISR8Stub (void);
extern void ISR9Stub (void);
extern void ISR10Stub (void);
extern void ISR11Stub (void);
extern void ISR12Stub (void);
extern void ISR13Stub (void);
extern void ISR14Stub (void);
extern void ISR15Stub (void);
extern void SyscallStub (void);

extern void ReloadSegmentRegisters(void);

void InitDescriptorTables (void);
void InitIDTDescEx (int32 idx, void (*func)(void));
void InitIDTDescNP (int32 idx);
void InitIDTDescHardInt (int32 idx, void (*func)(void));
void InitIDTDescSyscall (int32 idx, void (*func)(void));

void InitPIT (void);
void InitPIC (void);
void DetermineCPUFeatures (void);
void DetermineIntelCPUFeatures (void);
void DetermineAMDCPUFeatures (void);
void InitDFHandler(void);








/* InitI386();
*/

void InitI386 (void)
{
	InitDescriptorTables();
	InitPIT();
	InitPIC();
	DetermineCPUFeatures();
	InitDFHandler();
}




/* InitDescriptorTables();
 *
 * Initialise descriptor tables, TSS,task register and segment registers.
 */

void InitDescriptorTables (void)
{
	InitIDTDescEx (0,  ExceptionDE);
	InitIDTDescEx (1,  ExceptionDB);
	InitIDTDescEx (2,  ExceptionNMI);
	InitIDTDescEx (3,  ExceptionBP);
	InitIDTDescEx (4,  ExceptionOF);
	InitIDTDescEx (5,  ExceptionBR);
	InitIDTDescEx (6,  ExceptionUD);
	InitIDTDescEx (7,  ExceptionNM);
	
	idt[8].h = 0x00008500;
	idt[8].l = (DF_TSS_SEGMENT << 16) & 0xffff0000;

	InitIDTDescEx (9,  ExceptionCSO);
	InitIDTDescEx (10, ExceptionTS);
	InitIDTDescEx (11, ExceptionNP);
	InitIDTDescEx (12, ExceptionSS);
	InitIDTDescEx (13, ExceptionGP);
	InitIDTDescEx (14, ExceptionPF);
	InitIDTDescNP  (15);
	InitIDTDescEx (16, ExceptionMF);
	InitIDTDescEx (17, ExceptionAC);
	InitIDTDescEx (18, ExceptionMC);
	InitIDTDescEx (19, ExceptionXF);
	InitIDTDescNP  (20);
	InitIDTDescNP  (21);
	InitIDTDescNP  (22);
	InitIDTDescNP  (23);
	InitIDTDescNP  (24);
	InitIDTDescNP  (25);
	InitIDTDescNP  (26);
	InitIDTDescNP  (27);
	InitIDTDescNP  (28);
	InitIDTDescNP  (29);
	InitIDTDescNP  (30);
	InitIDTDescNP  (31);
	InitIDTDescHardInt (32, ISR0Stub);
	InitIDTDescHardInt (33, ISR1Stub);
	InitIDTDescHardInt (34, ISR2Stub);
	InitIDTDescHardInt (35, ISR3Stub);
	InitIDTDescHardInt (36, ISR4Stub);
	InitIDTDescHardInt (37, ISR5Stub);
	InitIDTDescHardInt (38, ISR6Stub);
	InitIDTDescHardInt (39, ISR7Stub);
	InitIDTDescHardInt (40, ISR8Stub);
	InitIDTDescHardInt (41, ISR9Stub);
	InitIDTDescHardInt (42, ISR10Stub);
	InitIDTDescHardInt (43, ISR11Stub);
	InitIDTDescHardInt (44, ISR12Stub);
	InitIDTDescHardInt (45, ISR13Stub);
	InitIDTDescHardInt (46, ISR14Stub);
	InitIDTDescHardInt (47, ISR15Stub);
	InitIDTDescSyscall (48, SyscallStub);
	
	gdt[0].l = 0;			/* Null segment     */
	gdt[0].h = 0;
	
	gdt[1].l = 0x0000ffff;	/* PL0 code segment */
	gdt[1].h = 0x00cf9a00;
	
	gdt[2].l = 0x0000ffff;	/* PL0 data segment */
	gdt[2].h = 0x00cf9200;
	
	gdt[3].l = 0x0000ffff;	/* PL3 code segment */
	gdt[3].h = 0x00cffa00;
	
	gdt[4].l = 0x0000ffff;	/* PL3 data segment */
	gdt[4].h = 0x00cff200;
	
	gdt[5].l = ((((uint32)&tss)<<16) & 0xffff0000) | (103);
	gdt[5].h = (((uint32)&tss) & 0xff000000) |
					((((uint32)&tss)>>16) & 0x000000ff) | 0x00008900;
	
	gdt[6].l = ((((uint32)&df_tss)<<16) & 0xffff0000) | (103);
	gdt[6].h = (((uint32)&df_tss) & 0xff000000) |
					((((uint32)&df_tss)>>16) & 0x000000ff) | 0x00008900;
	
	
	tss.iomap = 104;
	tss.debug = 0;
	tss.ldt = 0;
	tss.backlink = 0;
	tss.ss0 = PL0_DATA_SEGMENT;
	tss.esp0 = 0x00000000;
	
	
	
	/* Set and load descriptor table pointers                                 */
	
	gdtptr.pad = 0;
	gdtptr.length = sizeof(gdt)-1;
	gdtptr.seg = gdt;
	
	idtptr.pad = 0;
	idtptr.length = sizeof(idt)-1;
	idtptr.seg = idt;
	
	LoadGDT (&gdtptr);
	LoadIDT (&idtptr);
	LoadLDT (0);
	
	LoadTR (TSS_SEGMENT);
	
	ReloadSegmentRegisters();
}






/*
 *
 */

void InitIDTDescEx (int32 idx, void (*func)(void))
{
	idt[idx].l = ((uint32)func & 0x0000ffff) | 0x00080000;
	idt[idx].h = ((uint32)func & 0xffff0000) | 0x00008e00;
}

void InitIDTDescNP (int32 idx)
{
	idt[idx].l = 0x00000000;
	idt[idx].h = 0x00000000;
}


/*
 * Prevent hardware interrupts from being called by PL3
 */

void InitIDTDescHardInt (int32 idx, void (*func)(void))
{
	idt[idx].l = ((uint32)func & 0x0000ffff) | 0x00080000;
	idt[idx].h = ((uint32)func & 0xffff0000) | 0x00008e00;
}


/* 
 * Allow interrupts to be called by PL3.
 */

void InitIDTDescSyscall (int32 idx, void (*func)(void))
{
	idt[idx].l = ((uint32)func & 0x0000ffff) | 0x00080000;
	idt[idx].h = ((uint32)func & 0xffff0000) | 0x0000ee00;
}




/*
 * InitPIC();
 */

void InitPIC (void)
{
	OutByte (INT_MASTER_ICW1, 0x11);
	OutByte (INT_SLAVE_ICW1, 0x11);
	
	OutByte (INT_MASTER_ICW2, 0x20);
	OutByte (INT_SLAVE_ICW2, 0x28);
	
	OutByte (INT_MASTER_ICW3, 0x04);
	OutByte (INT_SLAVE_ICW3, 0x02);
	
	OutByte (INT_MASTER_ICW4, 0x01);
	OutByte (INT_SLAVE_ICW4, 0x01);

	OutByte (INT_MASTER_OCW1, 0x00);
	OutByte (INT_SLAVE_OCW1, 0x00);	
}




/*
 * InitPIT();
 */
 
void InitPIT (void)
{
	OutByte (TMR_TCW, TMRC_COUNTER_0 | TMRC_LOHICOUNTER | TMRC_MODE2);
	OutByte (TMR_TMRCNT0, PIT_100HZ_INTERVAL & 0xff);
	OutByte (TMR_TMRCNT0, (PIT_100HZ_INTERVAL & 0xff00)>>8);
}




/*
 * DetermineCPUFeatures();
 */

void DetermineCPUFeatures (void)
{
	struct CPUIDData cpuid_regs;
	uint32 ef1;
	uint32 ef2;
	
	
	/* Determine if cpu > i386 & WP supported */
	
	if ((ef1 = EFLG_AC & GetEflags()) == 0)
		SetEflags (EFLG_AC);
	else
		ClrEflags (EFLG_AC);
	
	ef2 = EFLG_AC & GetEflags();
	
	if ((ef1 ^ ef2) & EFLG_AC)
		cpu_is_i386 = FALSE;
	else
		cpu_is_i386 = TRUE;
	
	
	
	/* Determine if CPUID is supported */

	if ((ef1 = EFLG_ID & GetEflags()) == 0)
		SetEflags (EFLG_ID);
	else
		ClrEflags (EFLG_ID);
		
	ef2 = EFLG_ID & GetEflags();
		
	if ((ef1 ^ ef2) & EFLG_ID)
		cpu_cpuid = TRUE;
	else
	{
		cpu_cpuid = FALSE;
		return;
	}
	
		
		
	
	
	/* Determine largest standard function and Vendor ID */
	
	GetCPUID (0, &cpuid_regs);
	cpu_largest_std_function = cpuid_regs.eax;
	
	cpu_vendorid[0]  =  cpuid_regs.ebx & 0x000000ff;
	cpu_vendorid[1]  = (cpuid_regs.ebx & 0x0000ff00) >> 8;
	cpu_vendorid[2]  = (cpuid_regs.ebx * 0x00ff0000) >> 16;
	cpu_vendorid[3]  = (cpuid_regs.ebx * 0xff000000) >> 24;

	cpu_vendorid[4]  =  cpuid_regs.edx & 0x000000ff;
	cpu_vendorid[5]  = (cpuid_regs.edx & 0x0000ff00) >> 8;
	cpu_vendorid[6]  = (cpuid_regs.edx * 0x00ff0000) >> 16;
	cpu_vendorid[7]  = (cpuid_regs.edx * 0xff000000) >> 24;

	cpu_vendorid[8]  =  cpuid_regs.ecx & 0x000000ff;
	cpu_vendorid[9]  = (cpuid_regs.ecx & 0x0000ff00) >> 8;
	cpu_vendorid[10] = (cpuid_regs.ecx * 0x00ff0000) >> 16;
	cpu_vendorid[11] = (cpuid_regs.ecx * 0xff000000) >> 24;
	cpu_vendorid[12] = '\0';

	
		
	/* Handle standard feature flags across all processors                    */

	if (1 <= cpu_largest_std_function)
	{
		GetCPUID (1, &cpuid_regs);
		
		cpu_stepping = cpuid_regs.eax & 0x0000000f;
		cpu_model    = (cpuid_regs.eax & 0x000000f0)>>4;
		cpu_family   = (cpuid_regs.eax & 0x00000f00)>>8;

		cpu_fpu      = (cpuid_regs.edx & CPU_STD_FPU)      ? TRUE : FALSE;
		cpu_vme      = (cpuid_regs.edx & CPU_STD_VME)      ? TRUE : FALSE;
		cpu_debug    = (cpuid_regs.edx & CPU_STD_DEBUG)    ? TRUE : FALSE;
		cpu_pse      = (cpuid_regs.edx & CPU_STD_PSE)      ? TRUE : FALSE;
		cpu_tsc      = (cpuid_regs.edx & CPU_STD_TSC)      ? TRUE : FALSE;
		cpu_msr      = (cpuid_regs.edx & CPU_STD_MSR)      ? TRUE : FALSE;
		cpu_pae      = (cpuid_regs.edx & CPU_STD_PAE)      ? TRUE : FALSE;
		cpu_mce      = (cpuid_regs.edx & CPU_STD_MCE)      ? TRUE : FALSE;
		cpu_cmpxchg  = (cpuid_regs.edx & CPU_STD_CMPXCHG)  ? TRUE : FALSE;
		cpu_apic     = (cpuid_regs.edx & CPU_STD_APIC)     ? TRUE : FALSE;
		cpu_sysenter = (cpuid_regs.edx & CPU_STD_SYSENTER) ? TRUE : FALSE;
		cpu_mtrr     = (cpuid_regs.edx & CPU_STD_MTRR)     ? TRUE : FALSE;
		cpu_gpe      = (cpuid_regs.edx & CPU_STD_GPE)      ? TRUE : FALSE;
		cpu_mca      = (cpuid_regs.edx & CPU_STD_MCA)      ? TRUE : FALSE;
		cpu_cmov     = (cpuid_regs.edx & CPU_STD_CMOV)     ? TRUE : FALSE;
		cpu_pat      = (cpuid_regs.edx & CPU_STD_PAT)      ? TRUE : FALSE;
		cpu_pse36    = (cpuid_regs.edx & CPU_STD_PSE36)    ? TRUE : FALSE;
		cpu_mmx      = (cpuid_regs.edx & CPU_STD_MMX)      ? TRUE : FALSE;
		cpu_fxsr     = (cpuid_regs.edx & CPU_STD_FXSR)     ? TRUE : FALSE;
	
	
		/* If we have SSE2 then the mxcsr mask is different */
		
		if (cpu_fxsr == TRUE)
		{
			SetCR4 (GetCR4() | CR4_OSFXSR);
		}
		
		
		
		if (StrCmp (cpu_vendorid, "GeniuneIntel") == 0)
			DetermineIntelCPUFeatures();
		else if (StrCmp (cpu_vendorid, "AuthenticAMD") == 0)
			DetermineAMDCPUFeatures();
	}
}




/*
 * DetermineIntelCPUFeatures (void)
 */

void DetermineIntelCPUFeatures (void)
{
	struct CPUIDData cpuid_regs;
		
	/* Standard feature flags that only apply to Intel (maybe not)            */

	GetCPUID (1, &cpuid_regs);

	cpu_pn       = (cpuid_regs.edx & CPU_STD_PN)       ? TRUE : FALSE;
	cpu_xmm      = (cpuid_regs.edx & CPU_STD_XMM)      ? TRUE : FALSE;


	/* Handle Pentium Pro falsely reporting support for SysEnter              */
	
	if (cpu_family == 6 && (cpu_model<<4 | cpu_stepping) < 0x33)
		cpu_sysenter = 0;
	
}




/*
 * DetermineAMDCPUFeatures();
 */

void DetermineAMDCPUFeatures (void)
{
	struct CPUIDData cpuid_regs;

	
	GetCPUID (0x80000000, &cpuid_regs);
	cpu_largest_ext_function = cpuid_regs.eax;
	
	if (0x80000001 <= cpu_largest_ext_function)
	{
		GetCPUID (0x80000001, &cpuid_regs);
		
		cpu_syscall  = (cpuid_regs.edx & CPU_AMD_SYSCALL)  ? TRUE : FALSE;
	}
}




/*
 *
 */
 
void InitDFHandler (void)
{
	df_tss.backlink = 0;
	df_tss.pad0 = 0;
	
	df_tss.esp0 = (uint32)df_stack + 4092;
	df_tss.ss0 = PL0_DATA_SEGMENT;
	df_tss.pad1 = 0;
	
	df_tss.esp1 = 0;
	df_tss.ss1 = PL0_DATA_SEGMENT;
	df_tss.pad2 = 0;

	df_tss.esp2 = 0;
	df_tss.ss2 = PL0_DATA_SEGMENT; 
	df_tss.pad3 = 0;
	
	df_tss.cr3 = (uint32)0xdead0000;
	
	df_tss.eip = (uint32)&DoubleFaultHandler;
	df_tss.eflags = 0;
	
	df_tss.eax = 0xdeaddfdf;
	df_tss.ecx = 0xdeaddfdf;
	df_tss.edx = 0xdeaddfdf;
	df_tss.ebx = 0xdeaddfdf;
	df_tss.esp = (uint32)df_stack + 4092;
	df_tss.ebp = 0xdeaddfdf;
	df_tss.esi = 0xdeaddfdf;
	df_tss.edi = 0xdeaddfdf;
	
	df_tss.es = PL0_DATA_SEGMENT;
	df_tss.pad4 = 0;
	
	df_tss.cs = PL0_CODE_SEGMENT;
	df_tss.pad5 = 0;
	
	df_tss.ss = PL0_DATA_SEGMENT;
	df_tss.pad6 = 0;
	
	df_tss.ds = PL0_DATA_SEGMENT;
	df_tss.pad7 = 0;
	
	df_tss.fs = PL0_DATA_SEGMENT;
	df_tss.pad8 = 0;
	
	df_tss.gs = PL0_DATA_SEGMENT;
	df_tss.pad9 = 0;

	df_tss.ldt = 0;
	df_tss.pad10 = 0;
	
	df_tss.debug = 0;
	df_tss.iomap = 104;
}





/*
 *
 */

void FiniI386 (void)
{
}


