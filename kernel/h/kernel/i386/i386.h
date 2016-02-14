#ifndef KERNEL_I386_I386_H
#define KERNEL_I386_I386_H


#include <kernel/types.h>
#include <kernel/config.h>
#include <kernel/lists.h>
#include <kernel/fs.h>




/*
 * VM types
 */

typedef uint32		vm_addr;
typedef uint32		vm_offset;
typedef uint32		vm_size;




/*
 * General VM Constants
 */

#define PAGE_SIZE					4096




/*
 * VM Page directory entry constants.
 * Could divide/round-up the VM constants from config.h
 */

#define VM_KERNEL_PDE_BASE				(VM_KERNEL_BASE / (1024 * PAGE_SIZE))
#define VM_KERNEL_PDE_CEILING			((VM_KERNEL_CEILING / (1024 * PAGE_SIZE)) + 1)
#define VM_USER_PDE_BASE				(VM_USER_BASE / (1024 * PAGE_SIZE))
#define VM_USER_PDE_CEILING				((VM_USER_CEILING / (1024 * PAGE_SIZE)) + 1)




/*
 * Page table entry, page directory entry flags & misc page values
 */

#define PG_AVAILA		(1<<11)
#define PG_AVAILB		(1<<10)
#define PG_LOCK			(1<<9)
#define PG_GLOBAL		(1<<8)
#define PG_LARGEPAGES	(1<<7)
#define PG_DIRTY		(1<<6)
#define PG_ACCESSED		(1<<5)
#define PG_NOCACHE		(1<<4)
#define PG_WRITETHRU	(1<<3)
#define PG_USER			(1<<2)
#define PG_READWRITE	(1<<1)
#define PG_PRESENT		(1<<0)

#define PG_FRAME_MASK	(0xfffff000)

#define PDE_SHIFT		22
#define PTE_SHIFT		12

#define PDE_MASK		0x000003ff  /* Not sure if these are used, wrong value? */
#define PTE_MASK		0x000003ff

#define PDE_ADDR_MASK		0xfffff000
#define PTE_ADDR_MASK		0xfffff000

#define USER_PDE_FLAGS		(PG_USER | PG_READWRITE | PG_PRESENT);
#define KERNEL_PDE_FLAGS	(PG_READWRITE | PG_PRESENT);

/* #define PTE_FLAGS	(PG_READWRITE | PG_PRESENT); */

#define PF_ERRORCODE_PRESENT		(1<<0)
#define PF_ERRORCODE_DIRECTION		(1<<1)
#define PF_ERRORCODE_MODE			(1<<2)
#define PF_ERRORCODE_RSVD			(1<<3)




/*
 * ArchProc - placed at head of struct Process
 */

struct ArchProc
{
	vm_addr stack_pointer;       /* Stack pointer of saved process */
	vm_addr resume_eip;
	vm_addr resume_esp;
	union I387_State *fpu_state;
};



/*
 * struct Pmap
 */

struct Pmap
{
	uint32 *page_directory;
	LIST (PmapDesc) pmapdesc_list;
};




/*
 * struct TablePointer, for GDT & IDT
 */

struct TablePointer
{
	uint16 pad, length;
	struct SegmentDescriptor *seg;
};




/*
 * struct SegmentDescriptor
 */

struct SegmentDescriptor
{
	uint32 l;
	uint32 h;
};




/*
 * struct FarJump
 */

struct FarJump
{
	void *offset;
	uint16 segment;
	uint16 pad;
};




/*
 * struct TSS, Task State Segment
 */

struct TSS
{
	uint16 backlink, pad0;
	uint32 esp0;
	uint16 ss0, pad1;
	uint32 esp1;
	uint16 ss1, pad2;
	uint32 esp2;
	uint16 ss2, pad3;
	uint32 cr3;
	uint32 eip;
	uint32 eflags;
	uint32 eax;
	uint32 ecx;
	uint32 edx;
	uint32 ebx;
	uint32 esp;
	uint32 ebp;
	uint32 esi;
	uint32 edi;
	uint16 es, pad4;
	uint16 cs, pad5;
	uint16 ss, pad6;
	uint16 ds, pad7;
	uint16 fs, pad8;
	uint16 gs, pad9;
	uint16 ldt, pad10;
	uint16 debug, iomap;
} __attribute__ ((packed));




/*
 * State saved on stack during syscall, interrupts and exceptions
 */

struct ContextState
{
	uint32 eax;
	uint32 ebx;
	uint32 ecx;
	uint32 edx;
	uint32 esi;
	uint32 edi;
	uint32 ebp;
	
	uint32 gs;			/* Added */
	uint32 fs;
	uint32 es;
	uint32 ds;
	
	uint32 interrupt;
	uint32 exception;		/* Need to add the ISR idx, add 2 push $0 in ISR, 1 push $0 in exception */
	uint32 error_code;
	uint32 return_eip;
	uint32 return_cs;
	uint32 return_eflags;
	uint32 return_esp;
	uint32 return_ss;
	
	uint32 v86_es;
	uint32 v86_ds;
	uint32 v86_fs;	
	uint32 v86_gs;
};




/*
 * Eflag register
 */

#define EFLG_ID 		(1<<21)
#define EFLG_VIP		(1<<20)
#define EFLG_VIF		(1<<19)
#define EFLG_AC			(1<<18)
#define EFLG_VM			(1<<17)
#define EFLG_RF			(1<<16)
#define EFLG_NT			(1<<14)
#define EFLG_IOPL0		(0<<12)		/* IO Privilege level */
#define EFLG_IOPL1		(1<<12)
#define EFLG_IOPL2		(2<<12)
#define EFLG_IOPL3		(3<<12)
#define EFLG_IOPLSHFT	12
#define EFLG_OF			(1<<11)		/* user: Overflow */
#define EFLG_DF			(1<<10)		/* user: Direction flag */
#define EFLG_IF			(1<<9)		/* sys: Interrupts Enabled */
#define EFLG_TF			(1<<8)		/* sys: Trap flag */
#define EFLG_SF			(1<<7)		/* user: */
#define EFLG_ZF			(1<<6)		/* user: Zero flag */
#define EFLG_AF			(1<<4)		/* user: */
#define EFLG_PF			(1<<2)		/* user: */
#define EFLG_CF			(1<<0)		/* user: */

#define EFLG_SET_MASK	(	EFLG_CF | EFLG_PF | EFLG_AF | EFLG_ZF | EFLG_SF		\
						| 	EFLG_TF | EFLG_DF | EFLG_OF | EFLG_VM			)

#define EFLG_GET_MASK	(	EFLG_CF | EFLG_PF | EFLG_AF | EFLG_ZF | EFLG_SF		\
						| 	EFLG_TF | EFLG_IF | EFLG_DF | EFLG_OF | EFLG_VM	)



#define EFLAGS_SYSTEM_MASK 		(EFLG_ID | EFLG_VIP | EFLG_VIF | EFLG_AC | EFLG_VM		\
									| EFLG_RF | EFLG_NT | EFLG_IF | EFLG_TF)

#define EFLAGS_DEFAULT			EFLG_IF




/*
 * Control Registers
 */

#define CR4_VME 		0x00000001
#define CR4_PVI 		0x00000002
#define CR4_TSD 		0x00000004
#define CR4_DE  		0x00000008
#define CR4_PSE 		0x00000010
#define CR4_PAE 		0x00000020
#define CR4_MCE 		0x00000040
#define CR4_PGE 		0x00000080
#define CR4_PCE 		0x00000100
#define CR4_OSFXSR 		0x00000200
#define CR4_OSXMMEXCPT	0x00000400

#define CR3_PWT 		0x00000008
#define CR3_PCD 		0x00000010

#define CR0_PE  		0x00000001
#define CR0_MP  		0x00000002
#define CR0_EM  		0x00000004
#define CR0_TS  		0x00000008
#define CR0_ET  		0x00000010
#define CR0_NE  		0x00000020
#define CR0_WP  		0x00010000
#define CR0_AM  		0x00040000
#define CR0_NW  		0x20000000
#define CR0_CD  		0x40000000
#define CR0_PG  		0x80000000




/*
 * struct CPUIDData
 */

struct CPUIDData
{
	uint32 eax;
	uint32 ebx;
	uint32 ecx;
	uint32 edx;
};




/*
 * Intel specific CPUID Feature identification flags
 */

#define CPU_STD_FPU			1<<0
#define CPU_STD_VME			1<<1
#define CPU_STD_DEBUG		1<<2
#define CPU_STD_PSE			1<<3
#define CPU_STD_TSC			1<<4
#define CPU_STD_MSR			1<<5
#define CPU_STD_PAE			1<<6
#define CPU_STD_MCE			1<<7
#define CPU_STD_CMPXCHG		1<<8
#define CPU_STD_APIC		1<<9
#define CPU_STD_SYSENTER	1<<11
#define CPU_STD_MTRR		1<<12
#define CPU_STD_GPE			1<<13
#define CPU_STD_MCA			1<<14
#define CPU_STD_CMOV		1<<15
#define CPU_STD_PAT			1<<16
#define CPU_STD_PSE36		1<<17
#define CPU_STD_PN			1<<18
#define CPU_STD_MMX			1<<23
#define CPU_STD_FXSR		1<<24
#define CPU_STD_XMM			1<<25




/* AMD specific CPUID Feature identification flags
*/

#define EXTENDED_FEATURES	0x80000001
#define CPU_AMD_SYSCALL		1<<11
#define CPU_AMD_MMXEXT		1<<22
#define CPU_AMD_3DNOW		1<<31
#define CPU_AMD_3DNOWEXT	1<<30


/*
 * Exceptions
 */

#define EXCEPTION_DE	0
#define EXCEPTION_DB	1
#define EXCEPTION_NMI	2
#define EXCEPTION_BP	3
#define EXCEPTION_OF	4
#define EXCEPTION_BR	5
#define EXCEPTION_UD	6
#define EXCEPTION_NM	7
#define EXCEPTION_DF	8
#define EXCEPTION_CSO	9
#define EXCEPTION_TS	10
#define EXCEPTION_NP	11
#define EXCEPTION_SS	12
#define EXCEPTION_GP	13
#define EXCEPTION_PF	14
#define EXCEPTION_RESV	15
#define EXCEPTION_MF	16
#define EXCEPTION_AC	17
#define EXCEPTION_MC	18
#define EXCEPTION_XF	19




/*
 * Segment selectors
 */

#define PL0_CODE_SEGMENT			0x08
#define PL0_DATA_SEGMENT			0x10 
#define PL3_CODE_SEGMENT			(0x18 | 0x03)
#define PL3_DATA_SEGMENT			(0x20 | 0x03)
#define TSS_SEGMENT					0x28
#define DF_TSS_SEGMENT				0x30

#define CPL_MASK					0x03




/*
 * Standard FPU state
 */

struct I387_FSave
{
	uint16 control_word 			__attribute__((packed));
	uint16 _pad0					__attribute__((packed));
	uint16 status_word				__attribute__((packed));
	uint16 _pad1					__attribute__((packed));
	uint16 tag_word					__attribute__((packed));
	uint16 _pad2					__attribute__((packed));
	uint32 fpu_ip_offset			__attribute__((packed));
	uint16 fpu_ip_selector			__attribute__((packed));
	uint16 fpu_opcode				__attribute__((packed));
	uint32 fpu_operand_offset		__attribute__((packed));
	uint32 fpu_operand_selector		__attribute__((packed));
	uint16 _pad4					__attribute__((packed));
	uint32 st_space[20]				__attribute__((packed));
};




/*
 * FPU FXSave state
 */

struct I387_FXSave
{
	uint16 control_word				__attribute__((packed));
	uint16 status_word				__attribute__((packed));
	uint16 tag_word					__attribute__((packed));
	uint16 fpu_opcode				__attribute__((packed));
	uint32 fpu_ip_offset			__attribute__((packed));
	uint16 fpu_ip_selector 			__attribute__((packed));
	uint32 fpu_operand_offset 		__attribute__((packed));
	uint16 fpu_operand_selector 	__attribute__((packed));
	uint16 _pad0 					__attribute__((packed));
	uint32 mxcsr 					__attribute__((packed));
	uint32 mxcsr_mask				__attribute__((packed));
	uint32 st_space[32]				__attribute__((packed));
	uint32 xmm_space[32]			__attribute__((packed));
	uint32 _pad1[56]				__attribute__((packed));
};




/*
 * Union of FPU states
 */

union I387_State
{
	struct I387_FSave fstate;
	struct I387_FXSave fxstate;
};




/* #define MXCSR_MASK (cpu_has_sse2 ? 0xffff : 0xffbf)  */

#define MXCSR_MASK 					0xffbf




/*
 *
 */
 
#define ISR_TIMER_IDX				0
#define IRQ_KEYBOARD				1
#define IRQ_COM2					3
#define IRQ_COM1					4
#define IRQ_LPT2					5
#define IRQ_FLOPPY					6
#define IRQ_LPT1					7
#define IRQ_RTC						8
#define IRQ_PS2MOUSE				12
#define IRQ_IDE0					14
#define IRQ_IDE1					15







/* DMA Controller ports
*/

#define DMA03_COMMAND		0x08
#define DMA47_COMMAND		0xd0
#define DMA03_MODE			0x0b
#define DMA47_MODE			0xd6
#define DMA03_REQUEST		0x09
#define DMA47_REQUEST		0xd2
#define DMA03_WSMB			0x0a
#define DMA47_WSMB			0xd4
#define DMA03_RWAMB			0x0f
#define DMA47_RWAMB			0xde
#define DMA03_STATUS		0x08
#define DMA47_STATUS		0xd0
#define DMA0_ADDRESS		0x00
#define DMA1_ADDRESS		0x02
#define DMA2_ADDRESS		0x04
#define DMA3_ADDRESS		0x06
#define DMA4_ADDRESS		0xc0
#define DMA5_ADDRESS		0xc4
#define DMA6_ADDRESS		0xc8
#define DMA7_ADDRESS		0xcc
#define DMA0_COUNT			0x01
#define DMA1_COUNT			0x03
#define DMA2_COUNT			0x05
#define DMA3_COUNT			0x07
#define DMA4_COUNT			0xc2
#define DMA5_COUNT			0xc6
#define DMA6_COUNT			0xca
#define DMA7_COUNT			0xce
#define DMA0_LOWPAGE		0x87
#define DMA1_LOWPAGE		0x83
#define DMA2_LOWPAGE		0x81
#define DMA3_LOWPAGE		0x82
#define DMA5_LOWPAGE		0x8b
#define DMA6_LOWPAGE		0x89
#define DMA7_LOWPAGE		0x8a
#define DMA03_CLRBYTE		0x0c
#define DMA47_CLRBYTE		0xd8
#define DMA03_MASTERCLR		0x0d
#define DMA47_MASTERCLR		0xda
#define DMA03_CLRMASK		0x0e
#define DMA47_CLRMASK		0xdc




/* Programmable Interrupt Controller ports
*/

#define INT_MASTER_ICW1		0x020
#define INT_SLAVE_ICW1		0x0A0
#define INT_MASTER_ICW2		0x021
#define INT_SLAVE_ICW2		0x0A1
#define INT_MASTER_ICW3		0x021
#define INT_SLAVE_ICW3		0x0A1
#define INT_MASTER_ICW4		0x021
#define INT_SLAVE_ICW4		0x0A1
#define INT_MASTER_OCW1		0x021
#define INT_SLAVE_OCW1		0x0A1
#define INT_MASTER_OCW2		0x020
#define INT_SLAVE_OCW2		0x0A0
#define INT_MASTER_OCW3		0x020
#define INT_SLAVE_OCW3		0x0A0
#define INT_MASTER_ELCR		0x4D0
#define INT_SLAVE_ELCR		0x4D1




/* Programable Interval Timer ports
*/

#define TMR_TCW				0x043
#define TMRC_COUNTER_0		((0<<7) | (0<<6))
#define TMRC_COUNTER_1		((0<<7) | (1<<6))
#define TMRC_COUNTER_2		((1<<7) | (0<<6))
#define TMRC_LATCH			((0<<5) | (0<<4))
#define TMRC_LOCOUNTER		((0<<5) | (1<<4))
#define TMRC_HICOUNTER		((1<<5) | (0<<4))
#define TMRC_LOHICOUNTER	((1<<5) | (1<<4))
#define TMRC_MODE0			((0<<3) | (0<<2) | (0<<1))
#define TMRC_MODE1			((0<<3) | (0<<2) | (1<<1))
#define TMRC_MODE2			((0<<3) | (1<<2) | (0<<1))
#define TMRC_MODE3			((0<<3) | (1<<2) | (1<<1))
#define TMRC_MODE4			((1<<3) | (0<<2) | (0<<1))
#define TMRC_MODE5			((1<<3) | (0<<2) | (1<<1))
#define TMRC_BCD			(1<<0)
#define TMR_TMRSTS0			0x040
#define TMR_TMRSTS1			0x041
#define TMR_TMRSTS2			0x042
#define TMR_TMRCNT0			0x040
#define TMR_TMRCNT1			0x041
#define TMR_TMRCNT2			0x042




/* Real-Time Clock ports
*/

#define RTC_INDEX			0x70
#define RTC_DATA			0x71




/* CMOS-RAM offsets, CMOS Control Register flags and NMI control bit
*/

#define RTCF_NMI_ENABLE		(1<<7)
#define CMOS_SECONDS		0x00
#define CMOS_SECONDS_ALRM	0x01
#define CMOS_MINUTES		0x02
#define CMOS_MINUTES_ALRM	0x03
#define CMOS_HOURS			0x04
#define CMOS_HOURS_ALRM		0x05
#define CMOS_DAY_OF_WEEK	0x06
#define CMOS_DATE_OF_MONTH	0x07
#define CMOS_MONTH			0x08
#define CMOS_YEAR			0x09



/* CMOS Control Register A - Frequency Selector
*/

#define CMOS_CTRL_REG_A		0x0A


#define RTC_UIP				0x80
#define RTC_DIV_CTL           0x70
#define RTC_REF_CLCK_4MHZ     0x00
#define RTC_REF_CLCK_1MHZ     0x10
#define RTC_REF_CLCK_32KHZ    0x20
#define RTC_DIV_RESET1        0x60
#define RTC_DIV_RESET2        0x70
#define RTC_RATE_SELECT        0x0F





/* CMOS Control Register B - Control
*/

#define CMOS_CTRL_REG_B		0x0B
#define RTC_SET 0x80           /* disable updates for clock setting */
#define RTC_PIE 0x40           /* periodic interrupt enable */
#define RTC_AIE 0x20           /* alarm interrupt enable */
#define RTC_UIE 0x10           /* update-finished interrupt enable */
#define RTC_SQWE 0x08          /* enable square-wave output */
#define RTC_DM_BINARY 0x04     /* all time/date values are BCD if clear */
#define RTC_24H 0x02           /* 24 hour mode - else hours bit 7 means pm */
#define RTC_DST_EN 0x01        /* auto switch DST - works f. USA only */



/* CMOS Control Register C - Interrupt Flags
*/

#define CMOS_CTRL_REG_C		0x0C
#define RTC_IRQF 0x80
#define RTC_PF 0x40
#define RTC_AF 0x20
#define RTC_UF 0x10


/* CMOS Control Register D - Valid
*/

#define CMOS_CTRL_REG_D		0x0D
#define RTC_VALID       RTC_REG_D
#define RTC_VRT 0x80



/* Other CMOS Registers
*/

#define CMOS_FLOPPY_REG		0x10

#define CMOS_FLOPPY_1440	0x04
#define CMOS_FLOPPY_720		0x03
#define CMOS_FLOPPY_1200	0x02
#define CMOS_FLOPPY_360		0x01
#define CMOS_FLOPPY_NONE	0x00







/* Utility macros for reading and writing the CMOS clock
*/

#define CMOS_READ(addr)								\
({													\
	OutByte(RTC_INDEX, (addr));						\
	InByte(RTC_DATA);								\
})
	
#define CMOS_WRITE(val, addr)						\
({													\
	OutByte(RTC_INDEX, (addr));						\
	OutByte(RTC_DATA, (val));						\
})


#ifndef BCD_TO_BIN
#define BCD_TO_BIN(val) ((val)=((val)%16) + ((val)/16)*10)
#endif

#ifndef BIN_TO_BCD
#define BIN_TO_BCD(val) ((val)=(((val)/10)*16) + (val)%10)
#endif






/* Keyboard Controller ports
*/

#define KBD_OUTPUT_BUFFER		0x60
#define KBD_INPUT_BUFFER		0x60

#define KBD_CONTROL_REGISTER	0x64
#define KBD_STATUS_REGISTER		0x64




/* Keyboard Status Register flags
*/

#define KBDS_PARE			(1<<7)
#define KBDS_TIM			(1<<6)
#define KBDS_AUXB			(1<<5)
#define KBDS_KEYL			(1<<4)
#define KBDS_CD				(1<<3)
#define KBDS_SYSF			(1<<2)
#define KBDS_INPB			(1<<1)
#define KBDS_OUTB			(1<<0)




/* Keyboard Control Register commands
*/

#define KBDC_DISABLEAUX		0xa7
#define KBDC_ENABLEAUX		0xa8
#define KBDC_CHECKAUX		0xa9
#define KBDC_SELFTEST		0xaa
#define KBDC_CHECKKEY		0xab
#define KBDC_DISABLE		0xad
#define KBDC_ENABLE			0xae
#define KBDC_READINPUT		0xc0
#define KBDC_READOUTPUT		0xd0
#define KBDC_WRITEOUTPUT	0xd1
#define KBDC_WRITEOUTBUFFER	0xd2




/* PCI Configuration ports
*/

#define PCI_CONFIG_ADDRESS	0x0cf8
#define PCI_CONFIG_DATA		0x0cfc




/*
*/


#define InitTIBDescriptor( va, tls_desc)									\
			(tls_desc)->l = ((uint32)va << 16) | 0x0000ffff; 				\
			(tls_desc)->h = ((uint32)va & 0xff000000) | 0x00cff200 |			\
			0x00c0f200 | (((uint32)va >> 16) & 0x000000ff)


#define GetTIBAddrFromDesc( tls_desc)										\
					((uint32)(((tls_desc)->l >> 16) &0x0000ffff) |			\
								((tls_desc)->h & 0xff000000) |				\
								(((tls_desc)->h << 16) & 0x00ff0000))


#define LoadTIBDescriptor(tls_desc)											\
			g_gdt[6].l = (tls_desc)->l;										\
			g_gdt[6].h = (tls_desc)->h;
			
			



/*
*/

			
static inline void *GetStackPointer (void);
static inline uint16 GetStackSegment (void);
static inline void SetStackSegment (uint16 segment_selector);

static inline uint32 GetCR0 (void);
static inline uint32 GetCR2 (void);
static inline uint32 GetCR3 (void);
static inline uint32 GetCR4 (void);
static inline void SetCR0 (uint32 cr0);
static inline void SetCR2 (uint32 cr2);
static inline void SetCR3 (uint32 cr3);
static inline void SetCR4 (uint32 cr4);
static inline uint64 Timestamp (void);

static inline uint8 InByte (uint16 port);
static inline void OutByte (uint16 port, uint8 val);

static inline uint16 InWord (uint16 port);
static inline void OutWord (uint16 port, uint16 val);

static inline uint32 InLong (uint16 port);
static inline void OutLong (uint16 port, uint32 val);

static inline void FlushCaches (void);
static inline void InvalidatePage(void *address);
static inline uint32 BitScanRev (uint32 v);
static inline uint32 BitScanFwd (uint32 v);
static inline void GetCPUID (uint32 eax, struct CPUIDData *cpuid_regs);

static inline void LoadGDT (struct TablePointer *tb);
static inline void LoadIDT (struct TablePointer *tb);
static inline void LoadLDT (uint16 selector);
static inline void StoreGDT (struct TablePointer *tb);
static inline void StoreIDT (struct TablePointer *tb);

static inline uint16 StoreTR (void);
static inline void LoadTR (uint16 segment_selector);

static inline void EnableInterrupts (void);
static inline uint32 DisableInterrupts (void);
static inline void RestoreInterrupts (uint32 intstate);
static inline void PreemptionPoint (void);

static inline uint32 GetEflags (void);
static inline void SetEflags (uint32 setmask);
static inline void ClrEflags (uint32 clrmask);
static inline uint32 MullFrac (uint32 a, uint32 b);


/* void *GetStackPointer(void);
*/

static inline void *GetStackPointer (void)
{
	void *stack_pointer;
	
	asm volatile ("movl %%esp, %0;"
		: "=r" (stack_pointer)
		:
		);
	
	return stack_pointer;
}




/* uint16 GetStackSegment(void);
*/

static inline uint16 GetStackSegment (void)
{
	uint16 stack_segment;

	asm volatile ("movw %%ss, %0;"
		: "=r" (stack_segment)
		:
		);
		
	return stack_segment;
}



/* void SetStackSegment(uint16 segment_selector);
*/

static inline void SetStackSegment (uint16 segment_selector)
{
	asm volatile ("movw %0, %%ss;"
		:
		: "r" (segment_selector)
		: "memory"
		);
}





/* uint32 GetCR0 (void);
*/

static inline uint32 GetCR0 (void)
{
	uint32 cr0_val;

	asm volatile ("movl %%cr0, %0"
		: "=r" (cr0_val)
		:
		);

	return cr0_val;
}




/* uint32 GetCR2 (void);
*/

static inline uint32 GetCR2 (void)
{
	uint32 cr2_val;

	asm volatile ("movl %%cr2, %0"
		: "=r" (cr2_val)
		:
		);

	return cr2_val;
}




/* uint32 GetCR3 (void);
*/

static inline uint32 GetCR3 (void)
{
	uint32 cr3_val;

	asm volatile ("movl %%cr3, %0"
		: "=r" (cr3_val)
		:
		);

	return cr3_val;
}




/* uint32 GetCR4 (void);
*/

static inline uint32 GetCR4 (void)
{
	uint32 cr4_val;
	
	asm volatile ("movl %%cr4, %0"
		: "=r" (cr4_val)
		:
		);
	
	return cr4_val;
}




/* void SetCR0 (uint32 cr0);
*/

static inline void SetCR0(uint32 cr0_val)
{
	asm volatile ("movl %0, %%cr0"
		: 
		: "r" (cr0_val)
		);
}



/* void SetCR2 (uint32 cr2);
*/

static inline void SetCR2(uint32 cr2_val)
{
	asm volatile ("movl %0, %%cr2"
		: 
		: "r" (cr2_val)
		);
}



/* void SetCR3 (uint32 cr3);
*/

static inline void SetCR3(uint32 cr3_val)
{
	asm volatile ("movl %0, %%cr3"
		: 
		: "r" (cr3_val)
		);
}




/* void SetCR4 (uint32 cr4);
*/

static inline void SetCR4(uint32 cr4_val)
{
	asm volatile ("movl %0, %%cr4"
		: 
		: "r" (cr4_val)
		);
}




/* uint64 Timestamp (void);
*/

static inline uint64 Timestamp (void)
{
	uint64 timestamp;
	
	asm volatile ("rdtsc"
		: "=A" (timestamp)
		:
		);

	return timestamp;
}




/* uint8 InByte (uint16 port);
*/

static inline uint8 InByte (uint16 port)
{
	uint8 val;
	
	asm volatile ("inb %%dx, %%al"
		: "=a" (val)
		: "d" (port)
		);
		
	return val;
}




/* void OutByte (uint16 port, uint8 val);
*/

static inline void OutByte (uint16 port, uint8 val)
{
	asm volatile ("outb %%al, %%dx"
		: 
		: "a" (val), "d" (port)
		);
}



/* uint16 InWord (uint16 port);
*/

static inline uint16 InWord (uint16 port)
{
	uint16 val;
	
	asm volatile ("inw %%dx, %%ax"
		: "=a" (val)
		: "d" (port)
		);
		
	return val;
}



/* void OutWord (uint16 port, uint16 val);
*/

static inline void OutWord (uint16 port, uint16 val)
{
	asm volatile ("outw %%ax, %%dx"
		: 
		: "a" (val), "d" (port)
		);
}




/* uint32 InWord (uint16 port);
*/

static inline uint32 InLong (uint16 port)
{
	uint32 val;
	
	asm volatile ("inl %%dx, %%eax"
		: "=a" (val)
		: "d" (port)
		);
		
	return val;
}




/* void OutWord (uint16 port, uint32 val);
*/

static inline void OutLong (uint16 port, uint32 val)
{
	asm volatile ("outl %%eax, %%dx"
		: 
		: "a" (val), "d" (port)
		);
}

















/* void FlushCaches (void);
*/

static inline void FlushCaches (void)
{
	asm volatile ("wbinvd");
}




/* void InvalidatePage (void *address);
*/

static inline void InvalidatePage(void *address)
{
	asm volatile ("invlpg (%0);"
		:
		: "r" (address)
		: "memory"
		);
}




/* uint32 BitScanRev (uint32 v);
*/

static inline uint32 BitScanRev (uint32 v)
{
	asm volatile ("bsrl %0, %0"
		: "=r" (v)
		: "0" (v)
		);

	return v;
}


/* uint32 BitScanFwd (uint32 v);
*/

static inline uint32 BitScanFwd (uint32 v)
{
	asm volatile ("bsfl %0, %0"
		: "=r" (v)
		: "0" (v)
		);

	return v;
}




/* void GetCPUID (uint32 eax, struct CPUIDData *cpuid_regs);
*/

static inline void GetCPUID (uint32 eax, struct CPUIDData *cpuid_regs)
{
	asm volatile ("cpuid;"
		: 	"=a" (cpuid_regs->eax), "=b" (cpuid_regs->ebx),
			"=c" (cpuid_regs->ecx), "=d" (cpuid_regs->edx)
		: "a" (eax)
		);
}





/* void LoadGDT (struct TablePointer *tb);
*/

static inline void LoadGDT (struct TablePointer *tb)
{
	asm volatile ("lgdt 2(%0);"
		:
		: "r" (tb)
		: "memory"
		);
}



/* void LoadIDT (struct TablePointer *tb);
*/

static inline void LoadIDT (struct TablePointer *tb)
{
	asm volatile ("lidt 2(%0);"
		:
		: "r" (tb)
		: "memory"
		);
}



/* void LoadIDT (struct TablePointer *tb);
*/

static inline void LoadLDT (uint16 selector)
{
	asm volatile ("lldt %0;"
		:
		: "r" (selector)
		: "memory"
		);
}



/* void StoreGDT (struct TablePointer *tb);
*/

static inline void StoreGDT (struct TablePointer *tb)
{
	asm volatile ("sgdt 2(%0);"
		:
		: "r" (tb)
		: "memory"
		);
}



/* void StoreIDT (struct TablePointer *tb);
*/
	
static inline void StoreIDT (struct TablePointer *tb)
{
	asm volatile ("sidt 2(%0);"
		:
		: "r" (tb)
		: "memory"
		);
}




/* uint16 StoreTR (void);
*/

static inline uint16 StoreTR (void)
{
	uint16 task_register;
	
	asm volatile ("str %0;"
		: "=r" (task_register)
		:
		: "memory"
		);
		
	return task_register;
}




/* void LoadTR (uint16 segment_selector);
*/

static inline void LoadTR (uint16 segment_selector)
{
	asm volatile ("ltr %0;"
		:
		: "r" (segment_selector)
		: "memory"
		);
}




/* void EnableInterrupts (void);
*/

static inline void EnableInterrupts (void)
{
	asm volatile ("sti;");
}




/* uint32 DisableInterrupts (void);
*/

static inline uint32 DisableInterrupts (void)
{
	uint32 intstate;
	
	asm volatile ("pushfl;"
		"cli;"
		"movl (%%esp), %0;"
		"andl $0x00000200, %0;"
		"addl $4, %%esp;"
		: "=r" (intstate)
		:
		);
	
	return intstate;
}




/* void RestoreInterrupts (uint32 intstate);
*/

static inline void RestoreInterrupts (uint32 intstate)
{
	asm volatile ("pushfl;"
		"andl $0x00000200, %0;"
		"orl %0, (%%esp);"
		"popfl;"
		:
		: "r" (intstate)
		);
}




/* void PreemptionPoint (void)
*/

static inline void PreemptionPoint (void)
{
	asm volatile ("sti;"
		"nop;"
		"cli;"
		);
}




/* uint32 GetEflags(void);
*/

static inline uint32 GetEflags (void)
{
	uint32 eflags;
	
	asm volatile ("pushfl;"
		"popl %0;"
		: "=r" (eflags)
		:
		);
	
	return eflags;
}




/* void SetEflags(uint32 setmask);
*/

static inline void SetEflags (uint32 setmask)
{
	asm volatile ("pushfl;"
		"movl (%%esp), %%eax;"
		"orl %0, %%eax;"
		"movl %%eax, (%%esp);"
		"popfl;"
		:
		: "r" (setmask)
		: "%eax"
		);
}




/* void ClrEflags(uint32 clrmask);
*/

static inline void ClrEflags (uint32 clrmask)
{
	asm volatile ("pushfl;"
		"movl (%%esp), %%eax;"
		"notl %0;"
		"andl %0, %%eax;"
		"movl %%eax, (%%esp);"
		"popfl;"
		:
		: "r" (clrmask)
		: "%eax"
		);
}


/*
*/

static inline uint32 MullFrac (uint32 u, uint32 v)
{
	uint32 w0, w1;
	
	asm volatile ("mull %3;"
		: "=a" (w0), "=d" (w1)
		: "%0" (u),	"rm" (v)
		);

	return w1;
	

	/* uint32 d0;
		
	asm volatile ("mull %0;"
		:"=d" (a), "=&a" (d0)
		:"1" (a),"0" (b));
	
	return a;
	*/
}




/* extern uint32 *dmamem; */
extern uint32 floppy_dma_base;
extern uint32 floppy_dma_ceiling;

extern struct SegmentDescriptor gdt[7];
extern struct SegmentDescriptor idt[49];
extern struct TablePointer gdtptr;
extern struct TablePointer idtptr;
extern struct TSS tss;

extern bool cpu_is_i386;
extern bool cpu_cpuid;
extern char cpu_vendorid[13];
extern uint32 cpu_largest_std_function;
extern uint32 cpu_largest_ext_function;
extern uint64 cpu_freq;
extern uint32 cpu_stepping;
extern uint32 cpu_model;
extern uint32 cpu_family;
extern uint32 cpu_mxcsr_mask;

extern union I387_State root_fpu_state;
extern char root_current_dir[MAX_PATHNAME_SZ];
extern struct Process *fpu_state_owner;
extern uint32 kernel_cr3;

extern char root_process_name[5];
extern struct Process *root_process;
extern int root_pid;
extern int idle_pid;

extern bool acpi_is_enabled;

/*
 * CPU feature determination, see the Intel & AMD CPUID documentation.
 */

extern bool cpu_fpu;  /* Floating point unit on-board          */
extern bool cpu_vme;
extern bool cpu_debug;
extern bool cpu_pse;  /* Page Size Extension                   */
extern bool cpu_tsc;  /* Timestamp Counter                     */
extern bool cpu_msr;  
extern bool cpu_pae;  /* Physical Address Extension            */
extern bool cpu_mce;
extern bool cpu_cmpxchg;
extern bool cpu_apic;
extern bool cpu_sysenter;  /* Intel sysenter/sysexit calls          */
extern bool cpu_mtrr;
extern bool cpu_gpe;
extern bool cpu_mca;
extern bool cpu_cmov;
extern bool cpu_pat;
extern bool cpu_pse36;  /* Page Size Extension - 36              */
extern bool cpu_pn;
extern bool cpu_mmx;
extern bool cpu_fxsr;
extern bool cpu_xmm;
extern bool cpu_syscall;  /* AMD syscall/sysret calls              */
extern bool cpu_mmxext;
extern bool cpu_3dnow;
extern bool cpu_3dnowext;

extern struct TSS df_tss __attribute__ ((aligned (256)));
extern uint8 df_stack[KERNEL_STACK_SZ] __attribute__ ((aligned (16)));

extern uint8 root_stack[KERNEL_STACK_SZ] __attribute__ ((aligned (16)));


/*
 * Prototypes
 */
 
extern int32 AllocFPUState (struct Process *proc);
extern void FreeFPUState (struct Process *proc);
void FPUSwitchState (void);



void InitV86 (void);
int32 V86GPHandler (struct ContextState *cs);
int V86BiosCall (uint8 vector, struct ContextState *state);






#endif
