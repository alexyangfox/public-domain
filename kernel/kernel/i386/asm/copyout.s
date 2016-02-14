.global DoCopyOut
.extern SetError
.extern current_process
.extern cpu_is_i386


.equ RESUME_IP_OFFS,   4
.equ RESUME_SP_OFFS,   8
.equ USER_AS_OFFS,     16
.equ VM_USER_BASE,     0x40000000
.equ VM_USER_CEILING,  0xfffff000
.equ EFAULT,           14
.equ PF_DIR_READ,      0
.equ PF_DIR_WRITE,     1




.text
.align 16

/ ****************************************************************************
/ CopyIn (dst, src, bytes)
/
/
/

.equ ARG_COPY_DST, 24
.equ ARG_COPY_SRC, 28
.equ ARG_COPY_SZ,  32



DoCopyOut:
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




	
/ Slow byte-copy path. Checking pagetable entry on every page.
/ The I386 does not support Write-Protect of user-mode pages
/ from the kernel.  This means the kernel is allowed to write
/ to read-only user-mode pages.  So we call PageFault()
/ on every page to see whether the page is writable or not.

LI386CopyInit:
	movl %edi, %eax
	andl $0xfffff000,  %eax
	
	push %eax
	push $PF_DIR_WRITE
	push $0
		
	call PageFault
	add $12, %esp
	
	cmp $0, %eax
	jne LFaultExit
	jmp LI386Copy
		
LI386CopyTest:
	movl %edi, %eax
	andl $0x00000fff,  %eax
	cmpl $0, %eax
	jne LI386Copy
	
	movl %edi, %eax
	andl $0xfffff000,  %eax
	
	push %eax
	push $PF_DIR_WRITE
	push $0
		
	call PageFault
	add $12, %esp
	
	cmp $0, %eax
	jne LFaultExit

LI386Copy:
	cmp $0, %ecx
	je LCleanup
	movb (%esi), %al
	movb %al, (%edi)
	sub $1, %ecx
	add $1, %esi
	add $1, %edi
	jmp LI386CopyTest


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


