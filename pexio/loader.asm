global loader				; Entry point for linker
extern main				; Our kernel's main() is elsewhere

global reloadGDT			; reloadGDT function visible to linker

global loadIDT				; loadIDT function visible to linker
extern IDTPtr				; Our pointer to the IDT

; Multiboot header
MALIGN	equ	1<<0			; Align loaded modules with pages
MEMINFO	equ	1<<1			; Give us a memory map
FLAGS	equ	MALIGN | MEMINFO	; Multiboot flags
MAGIC	equ	0x1BADB002		; Bootloader finds header with this
CHECKS	equ	-(MAGIC + FLAGS)	; Checksum

section	.text				; TEXT section - code
align	4				; 4 byte alignment

MBH:	dd	MAGIC			; Multiboot header, reserving space
	dd	FLAGS
	dd	CHECKS

STACKSIZE	equ	0x4000		; 16kB for kernel stack

loader:
	mov	ESP,	stack+STACKSIZE	; Load stack pointer with stack
	push	eax			; Pass MAGIC to kernel
	push	ebx			; Pass multiboot info

	call main			; Call the kernel

reloadGDT:
	lgdt	[GDTDesc]		; Load the GDT as our GDTPtr
	jmp	0x08:GDTfin		; CS is GDT[1], far jmp flushes GDT
GDTfin:
	mov	AX,		0x10	; DS is GDT[2]
	mov	DS,		AX
	mov	ES,		AX
	mov	FS,		AX
	mov	GS,		AX
	mov	SS,		AX
	ret				; Return

loadIDT:
	lidt	[IDTPtr]		; Load the IDT as our IDTPtr
	ret				; Return

; External declarations of C IRQ handlers
extern CISR0				; Divide by zero exception
global isr0
extern CISR1				; Debug exception
global isr1
extern CISR2				; NMI exception
global isr2
extern CISR3				; Breakpoint exception
global isr3
extern CISR4				; Overflow exception
global isr4
extern CISR5				; Out of bounds exception
global isr5
extern CISR6				; Invalid opcode exception
global isr6
extern CISR7				; No coprocessor exception
global isr7
extern CISR8				; Double fault exception
global isr8
extern CISR9				; Coprocessor segment overrun exception
global isr9
extern CISR10				; Bad TSS exception
global isr10
extern CISR11				; Segment not present exception
global isr11
extern CISR12				; Stack fault exception
global isr12
extern CISR13				; GPF exception
global isr13
extern CISR14				; Page fault exception
global isr14
extern CISR16				; Coprocessor fault exception
global isr16
extern CISR17				; Alignment check exception
global isr17
extern CISR18				; Machine check exception
global isr18
extern CISRReserved			; Reserved IRQ
global isrReserved

extern CIRQ0				; Programmable Interval Timer
global irq0
extern CIRQ1				; Keyboard
global irq1
extern CIRQ7				; IRn not raised for long enough
global irq7

isr0:					; Divide by zero exception
	cli				; Cancel interrupts
	call	CISR0			; Call our C handler
	iret				; Return from interrupt

isr1:					; Debug exception
	cli				; Cancel interrupts
	call	CISR1			; Call our C handler
	iret				; Return from interrupt

isr2:					; NMI exception
	cli				; Cancel interrupts
	call	CISR2			; Call our C handler
	iret				; Return from interrupts

isr3:					; Breakpoint exception
	cli				; Cancel interrupts
	call	CISR3			; Call our C handler
	iret				; Return from interrupt

isr4:					; Overflow exception
	cli				; Cancel interrupts
	call	CISR4			; Call our C handler
	iret				; Return from interrupt

isr5:					; Out of bounds exception
	cli				; Cancel interrupts
	call	CISR5			; Call our C handler
	iret				; Return from interrupt

isr6:					; Invalid opcode exception
	cli				; Cancel interrupts
	call	CISR6			; Call our C handler
	iret				; Return from interrupt

isr7:					; No coprocessor exception
	cli				; Cancel interrupts
	call	CISR7			; Call our C handler
	iret				; Return from interrupt

isr8:					; Double fault exception
	cli				; Cancel interrupts
	call	CISR8			; Call our C handler
	iret				; Return from interrupt

isr9:					; Coprocessor segment overrun exception
	cli				; Cancel interrupts
	call	CISR9			; Call our C handler
	iret				; Return from interrupt

isr10:					; Bad TSS exception
	cli				; Cancel interrupts
	call	CISR10			; Call our C handler
	iret				; Return from interrupt

isr11:					; Segment not present exception
	cli				; Cancel interrupts
	call	CISR11			; Call our C handler
	iret				; Return from interrupt

isr12:					; Stack fault exception
	cli				; Cancel interrupts
	call	CISR12			; Call our C handler
	iret				; Return from interrupt

isr13:					; GPF exception
	cli				; Cancel interrupts
	call	CISR13			; Call our C handler
	iret				; Return from interrupt

isr14:					; Page fault exception
	cli				; Cancel interrupts
	call	CISR14			; Call our C handler
	iret				; Return from interrupt

isr16:					; Coprocessor fault exception
	cli				; Cancel interrupts
	call	CISR16			; Call our C handler
	iret				; Return from interrupt

isr17:					; Alignment check exception
	cli				; Cancel interrupts
	call	CISR17			; Call our C handler
	iret				; Return from interrupt

isr18:					; Machine check exception
	cli				; Cancel interrupts
	call	CISR18			; Call our C handler
	iret				; Return from interrupt

isrReserved:				; Reserved IRQ
	cli				; Cancel interrupts
	call	CISRReserved		; Call our C handler
	iret				; Return from interrupt

irq0:					; Programmable Interval Timer
	call	CIRQ0			; Call our C handler
	iret				; Return from interrupt

irq1:					; Keyboard
	call	CIRQ1			; Call our C handler
	iret				; Return from interrupt

irq7:	; IRn is no longer raised high when first INTA-bar is received
	; from the CPU.
	call	CIRQ7			; Call our C handler
	iret				; Return from interrupt

GDTDesc:				; GDT Pointer
	dw		SIZEOFGDT	; GDT limit
	dd		GDT		; GDT base

GDT:					; Global Descriptor Table
	; NULL descriptor - offset 0x00
	db	0x00			; Lowest byte of limit
	db	0x00			; Second lowest byte of limit
	db	0x00			; Lowest byte of base
	db	0x00			; Second lowest byte of base
	db	0x00			; Third lowest byte of base
	db	0x00			; Byte five
	db	0x00			; Byte six
	db	0x00			; Highest byte of base

	; Ring 0 code descriptor - offset 0x08
	db	0xFF			; Lowest byte of limit
	db	0xFF			; Second lowest byte of limit
	db	0x00			; Lowest byte of base
	db	0x00			; Second lowest byte of base
	db	0x00			; Third lowest byte of base
	db	0x9A			; Byte five
	db	0xCF			; Byte six
	db	0x00			; Highest byte of base
	
	; Ring 0 data descriptor - offset 0x10
	db	0xFF			; Lowest byte of limit
	db	0xFF			; Second lowest byte of limit
	db	0x00			; Lowest byte of base
	db	0x00			; Second lowest byte of base
	db	0x00			; Third lowest byte of base
	db	0x92			; Byte five
	db	0xCF			; Byte six
	db	0x00			; Highest byte of base

	; sizeof(GDT) for GDT pointer
	SIZEOFGDT	equ	$-GDT
	
section	.bss				; BSS section - data
align	32				; 32 byte alignment
stack:
	resb	STACKSIZE		; Reserve 16kB stack on QWord align
