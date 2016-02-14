#include <kernel/config.h>

.global DoCopyInStr
.extern SetError
.extern current_process


.equ RESUME_IP_OFFS,   4
.equ RESUME_SP_OFFS,   8
/.equ VM_USER_BASE,     0x40000000
/.equ VM_USER_CEILING,  0xfffff000
.equ EFAULT,           14
.equ ENAMETOLONG,      91

.text
.align 16


/ ****************************************************************************
/ int DoCopyInStr (void *dst, void *src, int sz)
/
/ Return code : 0 = success, -1 = error
/ Error : set to EFAULT if page fault.
/ Error : set to ENAMETOLONG if string not null-terminated.
/

.equ ARG_COPY_DST,  24
.equ ARG_COPY_SRC,  28
.equ ARG_COPY_SZ,   32


DoCopyInStr:
	pushfl
	push %ebx
	push %ebp
	push %esi
	push %edi

	movl current_process, %eax
	movl %esp, RESUME_SP_OFFS(%eax)
	movl $LFaultExit, RESUME_IP_OFFS(%eax)

	movl ARG_COPY_SRC(%esp), %esi
	movl ARG_COPY_DST(%esp), %edi
	movl ARG_COPY_SZ(%esp),  %ecx

LByteCopy:
	cmp $0, %ecx
	je LCleanup

	cmp $VM_USER_BASE, %esi
	jb LFaultExit
	
	cmp $VM_USER_CEILING, %esi
	jae LFaultExit
	
	movb (%esi), %al
	movb %al, (%edi)
	sub $1, %ecx

	cmp $0, %al
	je LCleanup

	add $1, %esi
	add $1, %edi
	jmp LByteCopy

LCleanup:
	cmpb $0, (%esi)
	jne LStringNotTerminated
	mov $0, %eax
	jmp LCleanupEnd

LStringNotTerminated:
	push $ENAMETOLONG
	call SetError
	add $4, %esp
	mov $-1, %eax
	
LCleanupEnd:
	movl current_process, %ebx
	movl $0x00000000, RESUME_SP_OFFS(%ebx)
	movl $0x00000000, RESUME_IP_OFFS(%ebx)
	pop %edi
	pop %esi
	pop %ebp
	pop %ebx
	popfl
	retl








LFaultExit:
	movl current_process, %eax
	movl $0x00000000, RESUME_SP_OFFS(%eax)
	movl $0x00000000, RESUME_IP_OFFS(%eax)

	push $EFAULT
	call SetError
	add $4, %esp
	mov $-1, %eax

	pop %edi
	pop %esi
	pop %ebp
	pop %ebx
	popfl
	retl



