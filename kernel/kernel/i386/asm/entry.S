#include <kernel/config.h>

.global Entry
.extern mbi
.extern Init
.extern __bss_start
.extern __bss_end
.extern root_stack


.text
.align 64
Entry:
	cld
	xorl %eax,%eax
	movl $(_sbss),%edi
	movl $(_ebss),%eax
	subl %edi,%eax
	rep
	stosb

	movl %ebx, mbi

	movl $(root_stack), %esp
	add $KERNEL_STACK_SZ, %esp
	
	call Init


		

.align  4
     
multiboot_header:
.long 0x1BADB002
.long 0x00000003
.long   -(0x1BADB002 + 0x00000003)
             
             


.end




