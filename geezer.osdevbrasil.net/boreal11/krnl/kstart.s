/*----------------------------------------------------------------------------
ASM COMPONENT OF KERNEL

EXPORTS
void halt(void);
void switch_to(thread_t *t);
void to_user(void)

extern unsigned g_kvirt_to_phys;
extern mboot_info_t *g_mboot_info;
----------------------------------------------------------------------------*/

.include "as.inc"

/* IMPORTS
from MAIN.C */
IMP fault
IMP kmain

/* from THREADS.C */
IMP g_curr_task

/* from linker script */
IMP g_code
IMP g_d_code
IMP g_d_bss
IMP g_end

.equ	DS_MAGIC,0x3544DA2A

/*****************************************************************************
kernel entry point -- start here from bootloader
*****************************************************************************/

.section .dtext, "x"		/* discardable code */

.globl kentry
kentry:
/* where did the loader put us? */
	call where_am_i
where_am_i:
	pop %esi		/* ESI=physical adr (where we are) */
	sub $where_am_i,%esi	/* subtract virtual adr (where we want to be) */
				/* now ESI=virt-to-phys */

/* all accesses to objects in the kernel data segment must be relative
to ESI until we activate page- or segment-based address translation
virt_adr + virt_to_phys = virt_adr + (phys - virt) = phys_adr
phys_adr - virt_to_phys = phys_adr - (phys - virt) = virt_adr */

/* check if data segment linked/located/loaded properly */
	cmpl $DS_MAGIC,(ds_magic)(%esi)
	je ds_ok

/* display a blinking white-on-blue 'D' if bad data segment */
	movw $0x9F44,(0xB8000)
	jmp .
ds_ok:

/* check if booted from Multiboot-compatible loader */
	cmp $0x2BADB002,%eax
	je mb_ok

/* display a blinking white-on-blue 'B' if bad bootloader */
	movw $0x9F42,(0xB8000)
	jmp .
mb_ok:

/* enable CPU caches (486+ only). The cache enable bits are active-low.
b0 is Protection Enable (PE). For '386, this instruction is a no-op. */
	mov $1,%eax
	mov %eax,%cr0

/* xxx - maybe some more checks here, e.g. correct physical address */

/* save kernel virt_to_phys conversion value */
	mov %esi,(g_kvirt_to_phys)(%esi)

/* If Multiboot, EBX will contain pointer to g_mboot_info structure.
Convert this address from physical to virtual and store it
xxx - if paging, omit conversion (i.e. the "sub %esi,%ebx") */
	sub %esi,%ebx
	mov %ebx,(g_mboot_info)(%esi)

/* set up segment-based address translation (SBAT) for kernel
(xxx - if paging, leave the GDT alone, and write new code here)
these 9 lines of code are a big no-op if virtual address==physical
(i.e. if segment base addresses in the GDT remain at zero) */
	mov %esi,%eax
	shr $16,%eax

	add %si,(gdt_kcode + 2)(%esi)
	adc %al,(gdt_kcode + 4)(%esi)
	adc %ah,(gdt_kcode + 7)(%esi)

	add %si,(gdt_kdata + 2)(%esi)
	adc %al,(gdt_kdata + 4)(%esi)
	adc %ah,(gdt_kdata + 7)(%esi)

	add %esi,(gdt_ptr + 2)(%esi)

/* load new GDT */
	lgdt (gdt_ptr)(%esi)
/*	ljmp $KERNEL_CS,$new_gdt	stupid assembler */
	.byte 0xEA
	.long new_gdt
	.word KERNEL_CS
new_gdt:
	mov $KERNEL_DS,%ax
	mov %eax,%ds
	mov %eax,%ss
	mov %eax,%es
	mov %eax,%fs
	mov %eax,%gs

/* NOW we can use absolute addresses
for things in the kernel data and BSS segments */

/* set up pmode stack */
	mov $stack,%esp
	mov $0xCF95BB6D,%ebp /* xxx - EBP_MAGIC; defined in _KRNL.H */

/* set up TSS descriptor, then load task register */
	lea tss(%esi),%eax
	mov %ax,(gdt_tss + 2)
	shr $16,%eax
	mov %al,(gdt_tss + 4)
	mov %ah,(gdt_tss + 7)
	mov $TSS_SELECTOR,%ax
	ltr %ax

/* set up interrupt handlers, then load IDT register */
	mov $((idt_end - idt) / 8),%ecx /* number of exception handlers */
	mov $idt,%ebx
	mov $isr0,%edx
build_idt:
	mov %edx,%eax		/* EAX=offset of entry point */
	mov %ax,(%ebx)		/* set low 16 bits of gate offset */
	shr $16,%eax
	mov %ax,6(%ebx)		/* set high 16 bits of gate offset */
	add $8,%ebx		/* 8 bytes per interrupt gate */
	add $(isr1 - isr0),%edx /* N bytes per stub */
	loop build_idt
	add %esi,(idt_ptr + 2)
	lidt (idt_ptr)

/* GRUB 0.90 leaves EFLAGS.NT set. IRET will cause a TSS-based
task-switch if this bit is set, so clear it now. This also
sets IOPL=0 and clears the DF (string direction) bit for GCC. */
	pushl $2
	popf

	jmp kmain		/* jump to C kernel */

/*****************************************************************************
Multiboot header, with "aout kludge"
GRUB will not boot the kernel unless
- this structure is present within the first 8192 bytes of the kernel file
- this structure is aligned on a 4-byte boundary
- the Multiboot magic value is present in the structure
- the checksum in the structure is correct
*****************************************************************************/

.equ	MBOOT_PAGE_ALIGN, 1<<0
.equ	MBOOT_MEMORY_INFO, 1<<1
.equ	MBOOT_AOUT_KLUDGE, 1<<16
.equ	MBOOT_HEADER_MAGIC, 0x1BADB002
.equ	MBOOT_HEADER_FLAGS, MBOOT_PAGE_ALIGN | MBOOT_MEMORY_INFO | MBOOT_AOUT_KLUDGE
.equ	MBOOT_CHECKSUM, -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)
.if 1
/* paging or SBAT (ld -Ttext=0xC0000000 ...) */
.equ	VIRT_TO_PHYS, (0x100000 - 0xC0000000)
.else
/* no address translation (ld -Ttext=0x100000 ...) */
.equ	VIRT_TO_PHYS, 0
.endif

.p2align 2
mboot:
	.long MBOOT_HEADER_MAGIC
	.long MBOOT_HEADER_FLAGS
	.long MBOOT_CHECKSUM
/* aout kludge */
	.long mboot + VIRT_TO_PHYS
	.long g_d_code + VIRT_TO_PHYS
	.long g_d_bss + VIRT_TO_PHYS
	.long g_end + VIRT_TO_PHYS
	.long kentry + VIRT_TO_PHYS

/*****************************************************************************
name:		halt
action:		halts CPU until hardware interrupt occurs
in:		(nothing)
out:		(nothing)
modifies:	(nothing)
notes:		C prototype: void halt(void);
*****************************************************************************/

.section .text			/* regular (non-discardable) code */

EXP halt
	hlt
	ret

/*****************************************************************************
name:		switch_to
action:		does task-switch by swapping kernel stacks
in:		pointer to new thread_t on stack, per C prototype
out:		(nothing)
modifies:	EAX, g_curr_task
notes:		C prototype: void switch_to(thread_t *t);

    *** WARNING: this code assumes that current kernel ESP is
    stored in the first field of the thread_t object, at offset zero

    *** WARNING: The order in which registers are pushed and popped here
    must agree with the layout of the kregs_t structure in the C code.
*****************************************************************************/

/* Juggling eggs in variable gravity... */
EXP switch_to
	pop %eax	/* convert CALL/RET stack frame (EIP only)... */
	pushf		/* ...to partial IRET stack frame (EIP, EFLAGS) */
	push %eax

	push %ebp	/* callee-save registers used by C */
	push %edi	/* pushing these creates kregs_t stack frame */
	push %esi
	push %ebx

/* interrupts OFF while switching stacks. We saved IF bit (EFLAGS) above */
		cli
		mov (g_curr_task),%eax

/* store current kernel ESP in thread_t struct of current thread/task */
		mov %esp,0(%eax)

/* get pointer (thread_t t) to new task/thread to run */
		mov 24(%esp),%eax

/* make this the current thread */
		mov %eax,(g_curr_task)
/* load new ESP */
		mov 0(%eax),%esp
	pop %ebx	/* pop C registers */
	pop %esi
	pop %edi
	pop %ebp

	pop %eax	/* convert partial IRET stack frame (EIP, EFLAGS)... */
	push %cs	/* ...to full IRET stack frame (EIP, CS, EFLAGS) */
	push %eax

/* IRET to new kernel thread. This pops EFLAGS, including EFLAGS.IF
We use IRET because it's the only instruction
that can load EFLAGS and EIP simultaneously. */
	iret

/*****************************************************************************
name:		all_ints
action:		unified IRQ/interrupt/exception handler
in:		IRET frame, error code, and exception number
		all pushed on stack
out:		(nothing)
modifies:	(nothing)
notes:		prototype for C-language interrupt handler:
		void fault(uregs_t *regs);

    *** WARNING: The order in which registers are pushed and popped here
    must agree with the layout of the uregs_t structure in the C code.
*****************************************************************************/

all_ints:
	push %gs
	push %fs
	push %es
	push %ds
	pusha
		mov $KERNEL_DS,%ax
		mov %eax,%ds
		mov %eax,%es
		mov %eax,%fs
		mov %eax,%gs

/* push pointer to stacked uregs_t,
and call the C-language interrupt handler */
		push %esp
			call fault
		pop %eax

/* *** FALL-THROUGH to to_user *** */

/*****************************************************************************
name:		to_user
action:		kernel exit code
in:		uregs_t frame on kernel stack
out:		(nothing)
modifies:	(nothing)
notes:		C prototype: void to_user(void);

    *** WARNING: code here depends on layout of aspace_t struct in C code
*****************************************************************************/

EXP to_user
		mov (g_curr_task),%eax /* EAX=g_curr_task */
		mov 4(%eax),%eax	/* EAX=g_curr_task->as */
		or %eax,%eax
		je is_thread		/* no address space; nothing to do */

/* set segment base addresses for task */
		mov 0(%eax),%ebx	/* EBX=g_curr_task->as->user_mem */
		sub 4(%eax),%ebx	/*   - g_curr_task->as->virt_adr */
		add (g_kvirt_to_phys),%ebx /* + g_kvirt_to_phys */
		mov %ebx,%ecx
		shr $16,%ecx		/* ECX=HIWORD(EBX) */

		mov %bx,(gdt_ucode + 2)
		mov %cl,(gdt_ucode + 4)
		mov %ch,(gdt_ucode + 7)

		mov %bx,(gdt_udata + 2)
		mov %cl,(gdt_udata + 4)
		mov %ch,(gdt_udata + 7)

/* set segment limits for task */
		mov 4(%eax),%ebx	/* EBX=g_curr_task->as->virt_adr */
		add 8(%eax),%ebx	/*   + g_curr_task->as->size */
		dec %ebx		/* -1 to convert from size to limit */
		shr $12,%ebx		/* page-granular! */
		mov %ebx,%ecx
		shr $16,%ecx		/* ECX=HIWORD(EBX) */
		and $0x0F,%cl

		mov %bx,(gdt_ucode + 0)
		andb $0xF0,(gdt_ucode + 6)
		or %cl,(gdt_ucode + 6)

		mov %bx,(gdt_udata + 0)
		andb $0xF0,(gdt_udata + 6)
		or %cl,(gdt_udata + 6)
is_thread:
		lea 76(%esp),%eax	/* EAX=ESP value after IRET */
		mov %eax,(tss_esp0)	/* store it in TSS */
	popa
	pop %ds
	pop %es
	pop %fs
	pop %gs
	add $8,%esp	/* drop exception number and error code */
	iret

/*****************************************************************************
IRQ/interrupt/exception handler "stubs"
The interrupt/trap gates in the IDT must point to these

    *** WARNING: The stubs must be consecutive,
    and each must be the same size
*****************************************************************************/

/* ARRGGH! Some JMPs are 2 bytes, some are 5. We'll use a macro to fix it: */
.macro jmp32 adr
	.byte 0xE9
	.long (\adr-.-4)
.endm

isr0:			/* divide error */
	push $0		/* push fake error code */
	push $0		/* push exception number */
	jmp32 all_ints
isr1:			/* debug/single step */
	push $0
	push $1
	jmp32 all_ints
isr2:			/* non-maskable interrupt */
	push $0
	push $2
	jmp32 all_ints
isr3:			/* INT3 */
	push $0
	push $3
	jmp32 all_ints
isr4:			/* INTO */
	push $0
	push $4
	jmp32 all_ints
isr5:			/* BOUND */
	push $0
	push $5
	jmp32 all_ints
isr6:			/* invalid opcode */
	push $0
	push $6
	jmp32 all_ints
isr7:			/* coprocessor not available */
	push $0
	push $7
	jmp32 all_ints
isr8:			/* double fault */
	nop		/* ...pushes its own error code */
	nop
	push $8
	jmp32 all_ints
isr9:			/* coproc segment overrun (386/486SX only) */
	push $0
	push $9
	jmp32 all_ints
isr0A:			/* bad TSS */
	nop
	nop
	push $0x0A
	jmp32 all_ints
isr0B:			/* segment not present */
	nop
	nop
	push $0x0B
	jmp32 all_ints
isr0C:			/* stack fault */
	nop
	nop
	push $0x0C
	jmp32 all_ints
isr0D:			/* GPF */
	nop
	nop
	push $0x0D
	jmp32 all_ints
isr0E:			/* page fault */
	nop
	nop
	push $0x0E
	jmp32 all_ints
isr0F:
	push $0
	push $0x0F
	jmp32 all_ints
isr10:			/* FP exception/coprocessor error */
	push $0
	push $0x10
	jmp32 all_ints
isr11:			/* alignment check (486+ only) */
	push $0
	push $0x11
	jmp32 all_ints
isr12:			/* machine check (Pentium+ only) */
	push $0
	push $0x12
	jmp32 all_ints
isr13:
	push $0
	push $0x13
	jmp32 all_ints
isr14:
	push $0
	push $0x14
	jmp32 all_ints
isr15:
	push $0
	push $0x15
	jmp32 all_ints
isr16:
	push $0
	push $0x16
	jmp32 all_ints
isr17:
	push $0
	push $0x17
	jmp32 all_ints
isr18:
	push $0
	push $0x18
	jmp32 all_ints
isr19:
	push $0
	push $0x19
	jmp32 all_ints
isr1A:
	push $0
	push $0x1A
	jmp32 all_ints
isr1B:
	push $0
	push $0x1B
	jmp32 all_ints
isr1C:
	push $0
	push $0x1C
	jmp32 all_ints
isr1D:
	push $0
	push $0x1D
	jmp32 all_ints
isr1E:
	push $0
	push $0x1E
	jmp32 all_ints
isr1F:
	push $0
	push $0x1F
	jmp32 all_ints

/* syscall software interrupt */
isr20:
	push $0
	push $0x20
	jmp32 all_ints

/* unused; reserved */
isr21:
	push $0
	push $0x21
	jmp32 all_ints

isr22:
	push $0
	push $0x22
	jmp32 all_ints

isr23:
	push $0
	push $0x23
	jmp32 all_ints

isr24:
	push $0
	push $0x24
	jmp32 all_ints

isr25:
	push $0
	push $0x25
	jmp32 all_ints

isr26:
	push $0
	push $0x26
	jmp32 all_ints

isr27:
	push $0
	push $0x27
	jmp32 all_ints

/* isr28 through isr37 are hardware interrupts. The 8259 programmable
interrupt controller (PIC) chips must be reprogrammed appropriately */
isr28:			/* IRQ 0/timer interrupt */
	push $0
	push $0x28
	jmp32 all_ints
isr29:			/* IRQ 1/keyboard interrupt */
	push $0
	push $0x29
	jmp32 all_ints
isr2A:
	push $0
	push $0x2A
	jmp32 all_ints
isr2B:			/* IRQ 3/COM2/COM4 serial interrupt */
	push $0
	push $0x2B
	jmp32 all_ints
isr2C:			/* IRQ 4/COM1/COM3 serial interrupt */
	push $0
	push $0x2C
	jmp32 all_ints
isr2D:
	push $0
	push $0x2D
	jmp32 all_ints
isr2E:			/* IRQ 6/floppy interrupt */
	push $0
	push $0x2E
	jmp32 all_ints
isr2F:
	push $0
	push $0x2F
	jmp32 all_ints
isr30:			/* IRQ 8/real-time clock interrupt */
	push $0
	push $0x30
	jmp32 all_ints
isr31:
	push $0
	push $0x31
	jmp32 all_ints
isr32:
	push $0
	push $0x32
	jmp32 all_ints
isr33:
	push $0
	push $0x33
	jmp32 all_ints
isr34:
	push $0
	push $0x34
	jmp32 all_ints
isr35:			/* IRQ 13/math coprocessor interrupt */
	push $0
	push $0x35
	jmp32 all_ints
isr36:			/* IRQ 14/primary ATA ("IDE") drive interrupt */
	push $0
	push $0x36
	jmp32 all_ints
isr37:			/* IRQ 15/secondary ATA drive interrupt */
	push $0
	push $0x37
	jmp32 all_ints

/* evidently new CPUs and PCs support more than 16 IRQs,
so we'll leave room here (isr38...) for future expansion */

/*****************************************************************************
kernel .data
*****************************************************************************/

.section .data

ds_magic:
	.long DS_MAGIC

idt:
/* 32 ring 0 interrupt gates for CPU-reserved exceptions */
	.rept 32
	.word 0, KERNEL_CS, 0x8E00, 0
	.endr

/* one ring 3 interrupt gate for syscalls (INT 20h) */
	.word 0		/* offset 15:0 */
	.word KERNEL_CS	/* selector */
	.byte 0		/* (always 0 for interrupt gates) */
	.byte 0xEE	/* present,ring 3,'386 interrupt gate */
	.word 0		/* offset 31:16 */

/* 23 ring 0 interrupt gates (7 reserved, 16 IRQ) */
	.rept 23
	.word 0, KERNEL_CS, 0x8E00, 0
	.endr
idt_end:

idt_ptr:
	.word idt_end - idt - 1 /* IDT limit */
	.long idt		/* linear adr of IDT */

/* we don't use the TSS for task-switching, but we still need it to store
the kernel (ring 0) stack pointer while user (ring 3) code is running,
and we also need it for the I/O permission bitmap */
tss:
	.word 0, 0	/* back link */
tss_esp0:
	.long 0		/* ESP0 */
	.word KERNEL_DS, 0/* SS0, reserved */

	.long 0		/* ESP1 */
	.word 0, 0	/* SS1, reserved */

	.long 0		/* ESP2 */
	.word 0, 0	/* SS2, reserved */

	.long 0		/* CR3 */
	.long 0, 0	/* EIP, EFLAGS */
	.long 0, 0, 0, 0/* EAX, ECX, EDX, EBX */
	.long 0, 0, 0, 0/* ESP, EBP, ESI, EDI */
	.word 0, 0	/* ES, reserved */
	.word 0, 0	/* CS, reserved */
	.word 0, 0	/* SS, reserved */
	.word 0, 0	/* DS, reserved */
	.word 0, 0	/* FS, reserved */
	.word 0, 0	/* GS, reserved */
	.word 0, 0	/* LDT, reserved */
/*	.word 0, 104	/* debug, IO permission bitmap (none) */
	.word 0, tss_end - tss /* debug, IO permission bitmap (none) */
tss_end:

/* null descriptor. gdt_ptr could be put here to save a few bytes,
but that's confusing */
gdt:
	.word 0		/* limit 15:0 */
	.word 0		/* base 15:0 */
	.byte 0		/* base 23:16 */
	.byte 0		/* type */
	.byte 0		/* limit 19:16, flags */
	.byte 0		/* base 31:24 */

/* descriptor for task-state segment (TSS) */
.equ	TSS_SELECTOR,(.-gdt)
gdt_tss:
	.word 103
	.word 0
	.byte 0
	.byte 0x89	/* present, ring 0, available 32-bit TSS */
	.byte 0
	.byte 0

/* ring 0 kernel code segment descriptor */
.equ	KERNEL_CS,(.-gdt)
gdt_kcode:
	.word 0xFFFF
	.word 0
	.byte 0
	.byte 0x9A	/* present, ring 0, code, non-conforming, readable */
	.byte 0xCF
	.byte 0

/* ring 0 kernel data segment descriptor */
.equ	KERNEL_DS,(.-gdt)
gdt_kdata:
	.word 0xFFFF
	.word 0
	.byte 0
	.byte 0x92	/* present, ring 0, data, expand-up, writable */
	.byte 0xCF
	.byte 0

/* ring 3 user code segment descriptor */
.equ	USER_CS,((.-gdt)|3)
gdt_ucode:
	.word 0xFFFF
	.word 0
	.byte 0
	.byte 0xFA	/* present, ring 3, code, non-conforming, readable */
	.byte 0xCF
	.byte 0

/* ring 3 user data segment descriptor */
.equ	USER_DS,((.-gdt)|3)
gdt_udata:
	.word 0xFFFF
	.word 0
	.byte 0
	.byte 0xF2	/* present, ring 3, data, expand-up, writable */
	.byte 0xCF
	.byte 0
gdt_end:

gdt_ptr:
	.word gdt_end - gdt - 1	/* GDT limit */
	.long gdt		/* linear adr of GDT (set above) */

/*****************************************************************************
kernel .bss
*****************************************************************************/

.section .bss

	.long 0
	.p2align 12 /* 4K kernel stack */
stack:

EXP g_kvirt_to_phys
	.long 0
EXP g_mboot_info
	.long 0
