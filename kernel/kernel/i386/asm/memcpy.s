.global MemCpy




.text
.align 16


/ ****************************************************************************
/ void MemCpy (dst, src, nbytes_sz);

.equ MEMCPY_DST, 20
.equ MEMCPY_SRC, 24
.equ MEMCPY_SZ,  28

        
MemCpy:
	push %ebx
	push %ebp
	push %esi
	push %edi
	
	movl MEMCPY_SRC(%esp), %eax
	movl MEMCPY_DST(%esp), %ebx
	movl MEMCPY_SZ(%esp),  %ecx
	movl %eax, %esi
	movl %ebx, %edi
	andl $3, %eax
	jnz LByteMemCpy
	andl $3, %ebx
	jnz LByteMemCpy

LQuickMemCpy:
	cmp $16, %ecx
	jb LByteMemCpy
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
	jmp LQuickMemCpy
	
LByteMemCpy:
	cmp $0, %ecx
	je LCleanupMemCpy
	movb (%esi), %al
	movb %al, (%edi)
	sub $1, %ecx
	add $1, %esi
	add $1, %edi
	jmp LByteMemCpy

LCleanupMemCpy:
	pop %edi
	pop %esi
	pop %ebp
	pop %ebx
	retl


























	