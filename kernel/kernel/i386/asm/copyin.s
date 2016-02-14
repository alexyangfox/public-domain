.global DoCopyIn
.extern SetError
.extern current_process


.equ RESUME_IP_OFFS, 4
.equ RESUME_SP_OFFS, 8
.equ USER_AS_OFFS, 16
.equ VM_USER_BASE,     0x40000000
.equ VM_USER_CEILING,  0xfffff000
.equ EFAULT,			14




.text
.align 16

/ ****************************************************************************
/ DoCopyIn (dst, src, bytes)

.equ ARG_COPY_DST, 24
.equ ARG_COPY_SRC, 28
.equ ARG_COPY_SZ,  32


DoCopyIn:
	pushfl
	push %ebx
	push %ebp
	push %esi
	push %edi

	movl current_process, %eax
	movl %esp, RESUME_SP_OFFS(%eax)
	movl $LFaultExit, RESUME_IP_OFFS(%eax)

	movl ARG_COPY_SRC(%esp), %eax
	movl ARG_COPY_DST(%esp), %ebx
	movl ARG_COPY_SZ(%esp),  %ecx
	movl %eax, %esi
	movl %ebx, %edi
	
	andl $3, %eax
	jnz LByteCopy
	andl $3, %ebx
	jnz LByteCopy

LQuickCopy:
	cmp $16, %ecx
	jb LByteCopy
	movl   (%esi), %eax
	movl  4(%esi), %ebx
	movl  8(%esi), %edx
	movl 12(%esi), %ebp
	movl %eax,   (%edi)
	movl %ebx,  4(%edi)
	movl %edx,  8(%edi)
	movl %ebp, 12(%edi)
	sub $16, %ecx
	add $16, %esi
	add $16, %edi
	jmp LQuickCopy
	
	
LByteCopy:
	cmp $0, %ecx
	je LCleanup
	movb (%esi), %al
	movb %al, (%edi)
	sub $1, %ecx
	add $1, %esi
	add $1, %edi
	jmp LByteCopy

LCleanup:
	movl current_process, %eax
	movl $0x00000000, RESUME_SP_OFFS(%eax)
	movl $0x00000000, RESUME_IP_OFFS(%eax)

	movl $0, %eax
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
	
	movl $-1, %eax
	pop %edi
	pop %esi
	pop %ebp
	pop %ebx
	popfl
	retl


