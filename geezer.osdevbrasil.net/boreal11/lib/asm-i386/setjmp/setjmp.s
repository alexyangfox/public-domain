.include "as.inc"

/* int setjmp(jmp_buf buf); */
EXP setjmp
	push %ebx
		mov 8(%esp),%ebx

		mov %edi,(%ebx)		/* buf->edi == 0(%ebx) == EDI */
		mov %esi,4(%ebx)	/* buf->esi == 4(%ebx) == ESI */
		mov %ebp,8(%ebx)	/* buf->ebp == 8(%ebx) == EBP */

		mov %edx,20(%ebx)	/* buf->edx == 20(%ebx) == EDX */
		mov %ecx,24(%ebx)	/* buf->ecx == 24(%ebx) == ECX */
		mov %eax,28(%ebx)	/* buf->eax == 28(%ebx) == EAX */
/* use EBX value saved on stack; not the current value */
		mov (%esp),%eax
		mov %eax,16(%ebx)	/* buf->ebx == 16(%ebx) == EBX */
/* use ESP value after RET; not the current value */
		lea 8(%esp),%eax
		mov %eax,12(%ebx)	/* buf->esp == 32(%ebx) == ESP */
/* use return address of this routine (EIP value saved on stack);
not the current value */
		mov 4(%esp),%eax
		mov %eax,32(%ebx)	/* buf->eip == 36(%ebx) == EIP */
/* none of the PUSH or MOV instructions changed EFLAGS! */
		pushf
		popl 36(%ebx)		/* buf->eflags == 40(%ebx) == EFLAGS */
	pop %ebx
	xor %eax,%eax
	ret
