.global pio_insw
.global pio_insb
.global pio_outsw
.global pio_outsb

.text
.align 16


/ ****************************************************************************
/ void pio_insw (port, buf, nbytes);
/ Input an array from an I/O port.

pio_insw:
	push %ebp
	movl %esp, %ebp
	push %edi
		
	movl  8(%ebp), %edx  / port to read from
	movl 12(%ebp), %edi  / destination addr
	movl 16(%ebp), %ecx  / byte count
	shr	$1, %ecx
	
	cld
	rep insw
		
	pop	%edi
	pop	%ebp
	ret




/ ****************************************************************************
/ void pio_insb (port, buf, nbytes);
/ Input an array from an I/O port.

pio_insb:
	push %ebp
	movl %esp, %ebp
	push %edi
		
	movl  8(%ebp), %edx  / port to read from
	movl 12(%ebp), %edi  / destination addr
	movl 16(%ebp), %ecx  / byte count

	cld
	rep insb
	
	pop	%edi
	pop	%ebp
	ret




/ ****************************************************************************
/ void pio_outsw (port, buf, nbytes);
/ Output an array to an I/O port.

pio_outsw:
	push %ebp
	movl %esp, %ebp
	push %esi
		
	movl  8(%ebp), %edx  / port to read from
	movl 12(%ebp), %esi  / destination addr
	movl 16(%ebp), %ecx  / byte count
	shr	$1, %ecx
	
	cld
	rep outsw
		
	pop	%esi
	pop	%ebp
	ret



/ ****************************************************************************
/ void pio_outsb (port, buf, count);
/ Output an array to an I/O port.

pio_outsb:
	push %ebp
	movl %esp, %ebp
	push %esi
		
	movl  8(%ebp), %edx  / port to read from
	movl 12(%ebp), %esi  / destination addr
	movl 16(%ebp), %ecx  / byte count

	cld
	rep insb
	
	pop	%esi
	pop	%ebp
	ret






