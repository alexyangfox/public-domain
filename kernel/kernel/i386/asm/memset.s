.global MemSet




.text
.align 16


/ ****************************************************************************
/ MemSet (dst, int val, nbytes_sz);
/
.equ MEMSET_DST, 16
.equ MEMSET_VAL, 20
.equ MEMSET_SZ,  24

        
MemSet:
	push %ebx
	push %ebp
	push %edi
	
	movl MEMSET_DST(%esp), %ebx
	movl MEMSET_SZ(%esp),  %ecx
	movl %ebx, %edi
	andl $3, %ebx
	jnz LByteMemSetInit

LQuickMemSetInit:
	movl MEMSET_VAL(%esp), %eax
	andl $0x000000ff, %eax
	movl $0x01010101, %edx
	mull %edx
	
LQuickMemSet:
	cmp $16, %ecx
	jb LByteMemSetInit
	movl %edx,   (%edi)
	movl %edx,  4(%edi)
	movl %edx,  8(%edi)
	movl %edx, 12(%edi)
	sub $16, %ecx	
	add $16, %edi
	jmp LQuickMemSet

LByteMemSetInit:
	movb MEMSET_VAL(%esp), %al

LByteMemSet:
	cmp $0, %ecx
	je LCleanupMemSet
	movb %al, (%edi)
	sub $1, %ecx
	add $1, %edi
	jmp LByteMemSet

LCleanupMemSet:
	pop %edi
	pop %ebp
	pop %ebx
	retl




































	