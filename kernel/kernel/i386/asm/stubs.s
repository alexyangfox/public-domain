.global KProcStart
.global UProcStart
.global V86ProcStart
.global ForkStart
.global ReloadSegmentRegisters
.global SyscallStub
.global Reschedule
.global RescheduleResume
.global ISR0Stub
.global ISR1Stub
.global ISR2Stub
.global ISR3Stub
.global ISR4Stub
.global ISR5Stub
.global ISR6Stub
.global ISR7Stub
.global ISR8Stub
.global ISR9Stub
.global ISR10Stub
.global ISR11Stub
.global ISR12Stub
.global ISR13Stub
.global ISR14Stub
.global ISR15Stub
.global ISRResumptionPoint
.global ExceptionDE
.global ExceptionDB
.global ExceptionNMI
.global ExceptionBP
.global ExceptionOF
.global ExceptionBR
.global ExceptionUD
.global ExceptionNM
.global ExceptionDF
.global ExceptionCSO
.global ExceptionTS
.global ExceptionNP
.global ExceptionSS
.global ExceptionGP
.global ExceptionPF
.global ExceptionMF
.global ExceptionAC
.global ExceptionMC
.global ExceptionXF

.extern current_process
.extern isr_depth
.extern interrupt_stack_ceiling
.extern SigReturn
.extern PickProc
.extern SoftClockProcessing
.extern DispatchException
.extern DispatchSyscall
.extern DispatchISR
.extern SyscallUnknown
.extern DeliverUserSignals
.extern Exit
.extern V86Start




.text
.align 64


/ ****************************************************************************
/ Data segment registers set to PL3_DATA_SEGMENT upon entering Kernel.
/ Stack segment register set to PL0_DATA_SEGMENT upon entering kernel.
/ Code segment register set to PL0_CODE_SEGMENT upon entering kernel.

.equ PL0_CODE_SEGMENT, 0x08
.equ PL0_DATA_SEGMENT, 0x10
.equ PL3_CODE_SEGMENT, 0x18
.equ PL3_DATA_SEGMENT, 0x20
.equ IRQ_OFFS, 48
.equ SIGRETURN_IDX, 0xffffffff



.EQU CONTEXTSTATE_CS_OFFS,      60
.EQU CONTEXTSTATE_EFLAGS_OFFS,  64   
.EQU EFLG_VM,         			(1<<17)


/ ****************************************************************************
/ FIXME:  Remove code
/
/ Reboot:
/	cli
/ 	__RebootLoop:
/	inb $0x64, %al
/  	test $0x02, %al
/	jnz __RebootLoop
/	mov $0xfe, %al
/	out %al, $0x64
/
/	__RebootBusy:
/	jmp __RebootBusy
/



/ ****************************************************************************

KProcStart:
	pushl %ebp
	movl %esp, %ebp
	
	movl 8(%esp), %eax
	movl 12(%esp), %ebx
	push %ebx
	
	call *%eax
	
	push %eax
	call Exit




/ ****************************************************************************

UProcStart:
	iret




/ ****************************************************************************

V86ProcStart:
	push %esp
	call V86Start
	addl $4, %esp

	pop %eax
	pop %ebx
	pop %ecx
	pop %edx
	pop %esi
	pop %edi
	pop %ebp

	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $12, %esp
	iret



	
/ ****************************************************************************

ForkStart:
	push %ebp
	push %edi
	push %esi
	push %edx
	push %ecx
	call GetError
	mov %eax, %ebx
	mov $0, %eax
	
	pop %ecx
	pop %edx
	pop %esi
	pop %edi
	pop %ebp
	iret




/ ****************************************************************************
/ Reload segment registers, PL0 cs and ss registers, PL3 for ds,es,fs,gs
/ Only called in init/i386.c

ReloadSegmentRegisters:
	.byte 0xEA
	.long LResume
	.word PL0_CODE_SEGMENT
LResume:
	push %eax
	mov $PL3_DATA_SEGMENT, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov $PL0_DATA_SEGMENT, %ax
	mov %ax, %ss
	pop %eax
	retl




/ ****************************************************************************
/ Dont forget to set segment registers to PL3_DATA_SEGMENT


SyscallStub:
	push $0
	push $0
	push $0
	
	push %ds
	push %es
	push %fs
	push %gs

	push %ebp
	push %edi
	push %esi	
	push %edx
	push %ecx
	push %ebx
	push %eax	


	mov $PL3_DATA_SEGMENT, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	sti

	push $0	
	call SetError
	addl $4, %esp
		
	movl (%esp), %eax
	addl $4, %esp

	cmp $SIGRETURN_IDX, %eax
	je LSyscallSigReturn

	shll $2, %eax
	mov $systable_end, %ebx
	sub $systable_start, %ebx
	cmp %ebx, %eax
	jge  LSyscallIdxOutRange
	
	add $systable_start, %eax
	call *(%eax)
	jmp LSyscallGetError




LSyscallIdxOutRange:
	call SyscallUnknown
	jmp LSyscallGetError

LSyscallSigReturn:
	pop %ebx         
	push $0					 / dummy %ebx
	push $0                  / dummy %eax
	
	push %esp                / pointer to struct RegisterState
	push %ebx                / pointer to struct sigframe
	
	call SigReturn
	add $8, %esp
	
	jmp LSyscallDeliverSignals


LSyscallGetError:

	push %eax
	call GetError
	mov %eax, %ebx
	pop %eax
	addl $4, %esp
	push %ebx
	push %eax
	
	
LSyscallDeliverSignals:

	cli
	
	mov CONTEXTSTATE_EFLAGS_OFFS(%esp), %eax
	andl EFLG_VM, %eax
	cmpl EFLG_VM, %eax
	je LSkipSignalDelivery1

	mov CONTEXTSTATE_CS_OFFS(%esp), %eax
	andl $0x03, %eax
	cmpl $0x03, %eax
	jne LSkipSignalDelivery1
		
	push %esp
	call DeliverUserSignals
	addl $4, %esp

LSkipSignalDelivery1:	
	pop %eax
	pop %ebx	
	pop %ecx	
	pop %edx
	pop %esi
	pop %edi
	pop %ebp
	
	pop %gs
	pop %fs
	pop %es	
	pop %ds
	
	addl $12, %esp
	
	iret







	





/ ****************************************************************************
/ Interrupts are assumed to be disabled by the code calling Reschedule().
/
/ Should use same stack as ExceptionState

Reschedule:
	pushfl
	pusha
	
	push $RescheduleResume

	movl $current_process, %eax
	movl (%eax), %eax
	movl %esp, (%eax)
	
	call PickProc
	
	movl $current_process, %eax
	movl (%eax), %eax
	movl (%eax), %esp

	jmp *(%esp)

RescheduleResume:
	add $4, %esp
	popa
	popfl
	retl




/ ****************************************************************************

/ 32) IRQ 0 - Timer

ISR0Stub:
	pushl $0
	pushl $0
	pushl $0
	jmp ISRWrapper


/ 33) IRQ 1 -

ISR1Stub:
	pushl $0
	pushl $0
	pushl $1
	jmp ISRWrapper


/ 34) IRQ 2 -

ISR2Stub:
	pushl $0
	pushl $0
	pushl $2
	jmp ISRWrapper


/ 35) IRQ 3 -

ISR3Stub:
	pushl $0
	pushl $0
	pushl $3
	jmp ISRWrapper


/ 36) IRQ 4 -

ISR4Stub:
	pushl $0
	pushl $0
	pushl $4
	jmp ISRWrapper


/ 37) IRQ 5 -

ISR5Stub:
	pushl $0
	pushl $0
	pushl $5
	jmp ISRWrapper


/ 38) IRQ 6 -

ISR6Stub:
	pushl $0
	pushl $0
	pushl $6
	jmp ISRWrapper


/ 39) IRQ 7 -

ISR7Stub:
	pushl $0
	pushl $0
	pushl $7
	jmp ISRWrapper


/ 40) IRQ 8 -

ISR8Stub:
	pushl $0
	pushl $0
	pushl $8
	jmp ISRWrapper


/ 41) IRQ 9 -

ISR9Stub:
	pushl $0
	pushl $0
	pushl $9
	jmp ISRWrapper


/ 42) IRQ 10 -

ISR10Stub:
	pushl $0
	pushl $0
	pushl $10
	jmp ISRWrapper


/ 43) IRQ 11 -

ISR11Stub:
	pushl $0
	pushl $0
	pushl $11
	jmp ISRWrapper


/ 44) IRQ 12 -

ISR12Stub:
	pushl $0
	pushl $0
	pushl $12
	jmp ISRWrapper


/ 45) IRQ 13 -

ISR13Stub:
	pushl $0
	pushl $0
	pushl $13
	jmp ISRWrapper


/ 46) IRQ 14 -

ISR14Stub:
	pushl $0
	pushl $0
	pushl $14
	jmp ISRWrapper


/ 47) IRQ 15 -

ISR15Stub:
	pushl $0
	pushl $0
	pushl $15
	jmp ISRWrapper



/ ISR Wrapper

ISRWrapper:
	push %ds
	push %es
	push %fs
	push %gs
	
	push %ebp
	push %edi
	push %esi	
	push %edx
	push %ecx
	push %ebx
	push %eax	
	
	movw $PL3_DATA_SEGMENT, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs




	push $ISRResumptionPoint


	movl IRQ_OFFS(%esp), %ebx

	
	movl isr_depth, %eax
	cmp $0, %eax
	jne LbSkipSwitchToIntStack

	movl current_process, %eax
	movl %esp, (%eax)
	movl interrupt_stack_ceiling, %esp
LbSkipSwitchToIntStack:
	

	
	add $1, isr_depth
	push %ebx

	call DispatchISR

	add $4, %esp
	sub $1, isr_depth

	mov $0x20, %al
	out %al, $0xa0
	out %al, $0x20
	
	
	
	
	movl isr_depth, %eax
	cmpl $0, %eax
	jne LSkipSwitchToProcStack

	add $1, isr_depth
	call SoftClockProcessing
	sub $1, isr_depth
	call PickProc

	movl current_process, %eax
	movl (%eax), %esp

LSkipSwitchToProcStack:
	jmp *(%esp)
	


ISRResumptionPoint:
	add $4, %esp					/ pop the resumption point off stack

	mov CONTEXTSTATE_EFLAGS_OFFS(%esp), %eax
	andl EFLG_VM, %eax
	cmpl EFLG_VM, %eax
	je LSkipSignalDelivery2

	mov CONTEXTSTATE_CS_OFFS(%esp), %eax
	andl $0x03, %eax
	cmpl $0x03, %eax
	jne LSkipSignalDelivery2
		
	push %esp
	call DeliverUserSignals
	addl $4, %esp


LSkipSignalDelivery2:

	pop %eax
	pop %ebx
	pop %ecx
	pop %edx
	pop %esi
	pop %edi
	pop %ebp

	pop %gs
	pop %fs
	pop %es
	pop %ds

	
	addl $12, %esp
	iret




/ ****************************************************************************

/ 0) [DE]  Divide error   EC = N 

ExceptionDE:
	pushl $0xdeadbeef
	pushl $0
	pushl $0
	jmp ExceptionWrapper


/ 1) [DB]  Debug   EC = N

ExceptionDB:
	pushl $0xdeadbeef
	pushl $1
	pushl $0
	jmp ExceptionWrapper


/ 2) [NMI]  Non-maskable interrupt   EC = N

ExceptionNMI:
	pushl $0xdeadbeef
	pushl $2
	pushl $0
	jmp ExceptionWrapper


/ 3) [BP]  Breakpoint   EC = N

ExceptionBP:
	pushl $0xdeadbeef
	pushl $3
	pushl $0
	jmp ExceptionWrapper


/ 4) [OF]  Overflow   EC = N

ExceptionOF:
	pushl $0xdeadbeef
	pushl $4
	pushl $0
	jmp ExceptionWrapper


/ 5) [BR]  Bound range exceeded   EC = N

ExceptionBR:
	pushl $0xdeadbeef
	pushl $5
	pushl $0
	jmp ExceptionWrapper


/ 6) [UD]  Invalid opcode   EC = N

ExceptionUD:
	pushl $0xdeadbeef
	pushl $6
	pushl $0
	jmp ExceptionWrapper


/ 7) [NM]  No math coprocessor   EC = N

ExceptionNM:
	pushl $0xdeadbeef
	pushl $7
	pushl $0
	jmp ExceptionWrapper


/ 8) [DF]  Double fault   EC = Y (zero)

ExceptionDF:
	pushl $8
	pushl $0
	jmp ExceptionWrapper


/ 9) [CSO]  Coprocessor segment overrun   EC = N

ExceptionCSO:
	pushl $0xdeadbeef
	pushl $9
	pushl $0
	jmp ExceptionWrapper


/ 10) [TS]  Invalid TSS   EC = Y

ExceptionTS:
	pushl $10
	pushl $0
	jmp ExceptionWrapper


/ 11) [NP]  Segment not present   EC = Y

ExceptionNP:
	pushl $11
	pushl $0
	jmp ExceptionWrapper


/ 12) [SS]  Stack segment fault   EC = Y

ExceptionSS:
	pushl $12
	pushl $0
	jmp ExceptionWrapper


/ 13) [GP]  General protection   EC = Y

ExceptionGP:
	pushl $13
	pushl $0
	jmp ExceptionWrapper


/ 14) [PF]  Page fault   EC = Y

ExceptionPF:
	pushl $14
	pushl $0
	jmp ExceptionWrapper


/ 16) [MF]  Floating point math error   EC = N

ExceptionMF:
	pushl $0xdeadbeef
	pushl $16
	pushl $0
	jmp ExceptionWrapper


/ 17) [AC]  Alignment check   EC = Y (zero)

ExceptionAC:
	pushl $17
	pushl $0
	jmp ExceptionWrapper


/ 18) [MC]  Machine check   EC = N

ExceptionMC:
	pushl $0x00000000
	pushl $18
	pushl $0
	jmp ExceptionWrapper


/ 19) [XF]  SSE fault   EC = N

ExceptionXF:
	pushl $0x00000000
	pushl $19
	pushl $0
	jmp ExceptionWrapper




/ ExceptionWrapper

ExceptionWrapper:
	push %ds
	push %es
	push %fs
	push %gs
	
	push %ebp
	push %edi
	push %esi	
	push %edx
	push %ecx
	push %ebx
	push %eax	
	
	movw $PL3_DATA_SEGMENT, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs


	
	push %esp
	call DispatchException
	addl $4, %esp


	mov CONTEXTSTATE_EFLAGS_OFFS(%esp), %eax
	andl EFLG_VM, %eax
	cmpl EFLG_VM, %eax
	je LSkipSignalDelivery3

	mov CONTEXTSTATE_CS_OFFS(%esp), %eax
	andl $0x03, %eax
	cmpl $0x03, %eax
	jne LSkipSignalDelivery3
		
	push %esp
	call DeliverUserSignals
	addl $4, %esp


LSkipSignalDelivery3:

	pop %eax
	pop %ebx
	pop %ecx
	pop %edx
	pop %esi
	pop %edi
	pop %ebp

	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $12, %esp
	iret
	









