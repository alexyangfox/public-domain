;-----------------------------------------------------------------------------
; ASM COMPONENT OF KERNEL
;
; EXPORTS
; void halt(void);
; void switch_to(thread_t *t);
; void to_user(void)
;
; extern unsigned g_kvirt_to_phys;
; extern mboot_info_t *g_mboot_info;
; extern char g_d_code[], g_code[], g_d_data[], g_data[], g_d_bss[], g_bss[];
;-----------------------------------------------------------------------------

%include "nasm.inc"

; IMPORTS
; from MAIN.C
IMP fault
IMP kmain

; from THREADS.C
IMP g_curr_task

; from KRNL_END.ASM
IMP g_end

DS_MAGIC EQU 0x3544DA2A

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; symbols to mark start of each segment
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SEGMENT TEXT
EXP g_d_code
EXP g_code

SEGMENT DATA
EXP g_d_data
EXP g_data

SEGMENT BSS
EXP g_d_bss
EXP g_bss

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; kernel entry point -- start here from bootloader
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SEGMENT TEXT
..start:

kentry:
; where did the loader put us?
	call where_am_i
where_am_i:
	pop esi		; ESI=physical adr (where we are)
	sub esi,where_am_i	; subtract virtual adr (where we want to be)
				; now ESI=virt-to-phys

; all accesses to objects in the kernel data segment must be relative
; to ESI until we activate page- or segment-based address translation
; virt_adr + virt_to_phys = virt_adr + (phys - virt) = phys_adr
; phys_adr - virt_to_phys = phys_adr - (phys - virt) = virt_adr

; check if data segment linked/located/loaded properly
	cmp dword [ds_magic + esi],DS_MAGIC
	je ds_ok

; display a blinking white-on-blue 'D' if bad data segment
	mov word [0xB8000],0x9F44
	jmp short $
ds_ok:

; check if booted from Multiboot-compatible loader
	cmp eax,0x2BADB002
	je mb_ok

; display a blinking white-on-blue 'B' if bad bootloader
	mov word [0xB8000],0x9F42
	jmp short $
mb_ok:

; enable CPU caches (486+ only). The cache enable bits are active-low.
; b0 is Protection Enable (PE). For '386, this instruction is a no-op.
	mov eax,1
	mov cr0,eax

; xxx - maybe some more checks here, e.g. correct physical address

; save kernel virt_to_phys conversion value
	mov [g_kvirt_to_phys + esi],esi

; If Multiboot, EBX will contain pointer to g_mboot_info structure.
; Convert this address from physical to virtual and store it
; xxx - if paging, omit conversion (i.e. the "sub esi,ebx")
	sub ebx,esi
	mov [g_mboot_info + esi],ebx

; set up segment-based address translation (SBAT) for kernel
; (xxx - if paging, leave the GDT alone, and write new code here)
; these 9 lines of code are a big no-op if virtual address==physical
; (i.e. if segment base addresses in the GDT remain at zero)
	mov eax,esi
	shr eax,16

	add [gdt_kcode + 2 + esi],si
	adc [gdt_kcode + 4 + esi],al
	adc [gdt_kcode + 7 + esi],ah

	add [gdt_kdata + 2 + esi],si
	adc [gdt_kdata + 4 + esi],al
	adc [gdt_kdata + 7 + esi],ah

	add [gdt_ptr + 2 + esi],esi

; load new GDT
	lgdt [gdt_ptr + esi]
	jmp KERNEL_CS:new_gdt
new_gdt:
	mov ax,KERNEL_DS
	mov ds,eax
	mov ss,eax
	mov es,eax
	mov fs,eax
	mov gs,eax

; NOW we can use absolute addresses
; for things in the kernel data and BSS segments

; set up pmode stack
	mov esp,stack
	mov ebp,0xCF95BB6D ; xxx - EBP_MAGIC; defined in _KRNL.H

; set up TSS descriptor, then load task register
	lea eax,[tss + esi]
	mov [gdt_tss + 2],ax
	shr eax,16
	mov [gdt_tss + 4],al
	mov [gdt_tss + 7],ah
	mov ax,TSS_SELECTOR
	ltr ax

; set up interrupt handlers, then load IDT register
	mov ecx,((idt_end - idt) / 8) ; number of exception handlers
	mov ebx,idt
	mov edx,isr0
build_idt:
	mov eax,edx		; EAX=offset of entry point
	mov [ebx],ax		; set low 16 bits of gate offset
	shr eax,16
	mov [ebx + 6],ax	; set high 16 bits of gate offset
	add ebx,byte 8		; 8 bytes per interrupt gate
	add edx,byte (isr1 - isr0) ; N bytes per stub
	loop build_idt
	add [idt_ptr + 2],esi
	lidt [idt_ptr]

; GRUB 0.90 leaves EFLAGS.NT set. IRET will cause a TSS-based
; task-switch if this bit is set, so clear it now. This also
; sets IOPL=0 and clears the DF (string direction) bit for GCC.
	push dword 2
	popf

	jmp kmain		; jump to C kernel

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Multiboot header, with "aout kludge"
; GRUB will not boot the kernel unless
; - this structure is present within the first 8192 bytes of the kernel file
; - this structure is aligned on a 4-byte boundary
; - the Multiboot magic value is present in the structure
; - the checksum in the structure is correct
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

MBOOT_PAGE_ALIGN EQU 1<<0
MBOOT_MEMORY_INFO EQU 1<<1
MBOOT_AOUT_KLUDGE EQU 1<<16
MBOOT_HEADER_MAGIC EQU 0x1BADB002
MBOOT_HEADER_FLAGS EQU MBOOT_PAGE_ALIGN | MBOOT_MEMORY_INFO | MBOOT_AOUT_KLUDGE
MBOOT_CHECKSUM EQU -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)
%if 1
; paging or SBAT (ld -Ttext=0xC0000000 ...)
VIRT_TO_PHYS EQU (0x100000 - 0xC0000000)
%else
; no address translation (ld -Ttext=0x100000 ...)
VIRT_TO_PHYS EQU 0
%endif

ALIGN 4
mboot:
	dd MBOOT_HEADER_MAGIC
	dd MBOOT_HEADER_FLAGS
	dd MBOOT_CHECKSUM
; aout kludge
	dd mboot + VIRT_TO_PHYS
	dd g_d_code + VIRT_TO_PHYS
	dd g_d_bss + VIRT_TO_PHYS
	dd g_end + VIRT_TO_PHYS
	dd kentry + VIRT_TO_PHYS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:		halt
; action:	halts CPU until hardware interrupt occurs
; in:		(nothing)
; out:		(nothing)
; modifies:	(nothing)
; notes:	C prototype: void halt(void);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SEGMENT TEXT

EXP halt
	hlt
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:		switch_to
; action:	does task-switch by swapping kernel stacks
; in:		pointer to new thread_t on stack, per C prototype
; out:		(nothing)
; modifies:	EAX, g_curr_task
; notes:	C prototype: void switch_to(task_t *t);
;
;     *** WARNING: this code assumes that current kernel ESP is
;     stored in the first field of the thread_t object, at offset zero
;     *** WARNING: The order in which registers are pushed and popped here
;     must agree with the layout of the kregs_t structure in the C code.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

EXP switch_to
	pop eax	; convert CALL stack frame (EIP only)...
	pushf		; ...to partial IRET (EIP, EFLAGS)
	push eax

	push ebp	; callee-save registers used by C
	push edi	; pushing these creates kregs_t stack frame
	push esi
	push ebx

		cli	; interrupts OFF while switching stacks
		mov eax,[g_curr_task]

; store current kernel ESP in thread_t struct of current thread/task
		mov [eax],esp

; get pointer (thread_t t) to new task/thread to run
		mov eax,[esp + 24]

; make this the current thread
		mov [g_curr_task],eax
; load new ESP
		mov esp,[eax]
	pop ebx	; pop C registers
	pop esi
	pop edi
	pop ebp

	pop eax	; convert partial IRET stack frame (EIP, EFLAGS)...
	push cs	; ...to full IRET stack frame (EIP, CS, EFLAGS)
	push eax

	iret		; IRET to new kernel thread

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:		all_ints
; action:	unified IRQ/interrupt/exception handler
; in:		IRET frame, error code, and exception number
; 		all pushed on stack
; out:		(nothing)
; modifies:	(nothing)
; notes:	prototype for C-language interrupt handler:
; 		void fault(uregs_t *regs);
;
;     *** WARNING: The order in which registers are pushed and popped here
;     must agree with the layout of the uregs_t structure in the C code.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

all_ints:
	push gs
	push fs
	push es
	push ds
	pusha
		mov ax,KERNEL_DS
		mov ds,eax
		mov es,eax
		mov fs,eax
		mov gs,eax

; push pointer to stacked uregs_t,
; and call the C-language interrupt handler
		push esp
			call fault
		pop eax

; *** FALL-THROUGH to to_user ***

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:		to_user
; action:	kernel exit code
; in:		uregs_t frame on kernel stack
; out:		(nothing)
; modifies:	(nothing)
; notes:	C prototype: void to_user(void);
;     *** WARNING: code here depends on layout of aspace_t struct in C code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

EXP to_user
		mov eax,[g_curr_task]	; EAX=g_curr_task
		mov eax,[eax + 4]	; EAX=g_curr_task->as
		or eax,eax
		je is_thread		; no address space; nothing to do

; set segment base addresses for task
		mov ebx,[eax]		; EBX=g_curr_task->as->user_mem
		sub ebx,[eax + 4]	;   - g_curr_task->as->virt_adr
		add ebx,[g_kvirt_to_phys] ; + g_kvirt_to_phys
		mov ecx,ebx
		shr ecx,16		; ECX=HIWORD(EBX)

		mov [gdt_ucode + 2],bx
		mov [gdt_ucode + 4],cl
		mov [gdt_ucode + 7],ch

		mov [gdt_udata + 2],bx
		mov [gdt_udata + 4],cl
		mov [gdt_udata + 7],ch

; set segment limits for task
		mov ebx,[eax + 4]	; EBX=g_curr_task->as->virt_adr
		add ebx,[eax + 8]	;   + g_curr_task->as->size
		dec ebx			; -1 to convert from size to limit
		shr ebx,12		; page-granular!
		mov ecx,ebx
		shr ecx,16		; ECX=HIWORD(EBX)
		and cl,0x0F

		mov [gdt_ucode + 0],bx
		and byte [gdt_ucode + 6],0xF0
		or [gdt_ucode + 6],cl

		mov [gdt_udata + 0],bx
		and byte [gdt_udata + 6],0xF0
		or [gdt_udata + 6],cl
is_thread:
		lea eax,[esp + 76]	; EAX=ESP value after IRET
		mov [tss_esp0],eax	; store it in TSS
	popa
	pop ds
	pop es
	pop fs
	pop gs
	add esp,byte 8	; drop exception number and error code
	iret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; IRQ/interrupt/exception handler "stubs"
; The interrupt/trap gates in the IDT must point to these
;
;     *** WARNING: The stubs must be consecutive, and each must be the same size
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

isr0:			; divide error
	push byte 0	; push byte fake error code
	push byte 0	; push byte exception number
	jmp dword all_ints
isr1:			; debug/single step
	push byte 0
	push byte 1
	jmp dword all_ints
isr2:			; non-maskable interrupt
	push byte 0
	push byte 2
	jmp dword all_ints
isr3:			; INT3
	push byte 0
	push byte 3
	jmp dword all_ints
isr4:			; INTO
	push byte 0
	push byte 4
	jmp dword all_ints
isr5:			; BOUND
	push byte 0
	push byte 5
	jmp dword all_ints
isr6:			; invalid opcode
	push byte 0
	push byte 6
	jmp dword all_ints
isr7:			; coprocessor not available
	push byte 0
	push byte 7
	jmp dword all_ints
isr8:			; double fault
	nop		; ...push bytees its own error code
	nop
	push byte 8
	jmp dword all_ints
isr9:			; coproc segment overrun (386/486SX only)
	push byte 0
	push byte 9
	jmp dword all_ints
isr0A:			; bad TSS
	nop
	nop
	push byte 0x0A
	jmp dword all_ints
isr0B:			; segment not present
	nop
	nop
	push byte 0x0B
	jmp dword all_ints
isr0C:			; stack fault
	nop
	nop
	push byte 0x0C
	jmp dword all_ints
isr0D:			; GPF
	nop
	nop
	push byte 0x0D
	jmp dword all_ints
isr0E:			; page fault
	nop
	nop
	push byte 0x0E
	jmp dword all_ints
isr0F:
	push byte 0
	push byte 0x0F
	jmp dword all_ints
isr10:			; FP exception/coprocessor error
	push byte 0
	push byte 0x10
	jmp dword all_ints
isr11:			; alignment check (486+ only)
	push byte 0
	push byte 0x11
	jmp dword all_ints
isr12:			; machine check (Pentium+ only)
	push byte 0
	push byte 0x12
	jmp dword all_ints
isr13:
	push byte 0
	push byte 0x13
	jmp dword all_ints
isr14:
	push byte 0
	push byte 0x14
	jmp dword all_ints
isr15:
	push byte 0
	push byte 0x15
	jmp dword all_ints
isr16:
	push byte 0
	push byte 0x16
	jmp dword all_ints
isr17:
	push byte 0
	push byte 0x17
	jmp dword all_ints
isr18:
	push byte 0
	push byte 0x18
	jmp dword all_ints
isr19:
	push byte 0
	push byte 0x19
	jmp dword all_ints
isr1A:
	push byte 0
	push byte 0x1A
	jmp dword all_ints
isr1B:
	push byte 0
	push byte 0x1B
	jmp dword all_ints
isr1C:
	push byte 0
	push byte 0x1C
	jmp dword all_ints
isr1D:
	push byte 0
	push byte 0x1D
	jmp dword all_ints
isr1E:
	push byte 0
	push byte 0x1E
	jmp dword all_ints
isr1F:
	push byte 0
	push byte 0x1F
	jmp dword all_ints

; isr20 through isr2F are hardware interrupts. The 8259 programmable
; interrupt controller (PIC) chips must be reprogrammed appropriately
isr20:			; IRQ 0/timer interrupt
	push byte 0
	push byte 0x20
	jmp dword all_ints
isr21:			; IRQ 1/keyboard interrupt
	push byte 0
	push byte 0x21
	jmp dword all_ints
isr22:
	push byte 0
	push byte 0x22
	jmp dword all_ints
isr23:
	push byte 0
	push byte 0x23
	jmp dword all_ints
isr24:
	push byte 0
	push byte 0x24
	jmp dword all_ints
isr25:
	push byte 0
	push byte 0x25
	jmp dword all_ints
isr26:			; IRQ 6/floppy interrupt
	push byte 0
	push byte 0x26
	jmp dword all_ints
isr27:
	push byte 0
	push byte 0x27
	jmp dword all_ints
isr28:			; IRQ 8/real-time clock interrupt
	push byte 0
	push byte 0x28
	jmp dword all_ints
isr29:
	push byte 0
	push byte 0x29
	jmp dword all_ints
isr2A:
	push byte 0
	push byte 0x2A
	jmp dword all_ints
isr2B:
	push byte 0
	push byte 0x2B
	jmp dword all_ints
isr2C:
	push byte 0
	push byte 0x2C
	jmp dword all_ints
isr2D:			; IRQ 13/math coprocessor interrupt
	push byte 0
	push byte 0x2D
	jmp dword all_ints
isr2E:			; IRQ 14/primary ATA ("IDE") drive interrupt
	push byte 0
	push byte 0x2E
	jmp dword all_ints
isr2F:			; IRQ 15/secondary ATA drive interrupt
	push byte 0
	push byte 0x2F
	jmp dword all_ints

; syscall software interrupt
isr30:
	push byte 0
	push byte 0x30
	jmp dword all_ints

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; kernel .data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SEGMENT DATA

ds_magic:
	dd DS_MAGIC

idt:
; 48 ring 0 interrupt gates
	times 48	dw 0, KERNEL_CS, 0x8E00, 0

; one ring 3 interrupt gate for syscalls (INT 30h)
	dw 0		; offset 15:0
	dw KERNEL_CS	; selector
	db 0		; (always 0 for interrupt gates)
	db 0xEE		; present,ring 3,'386 interrupt gate
	dw 0		; offset 31:16
idt_end:

idt_ptr:
	dw idt_end - idt - 1 ; IDT limit
	dd idt		; linear adr of IDT

; we don't use the TSS for task-switching, but we still need it to store
; the kernel (ring 0) stack pointer while user (ring 3) code is running,
; and we also need it for the I/O permission bitmap
tss:
	dw 0, 0	; back link
tss_esp0:
	dd 0		; ESP0
	dw KERNEL_DS, 0	; SS0, reserved

	dd 0		; ESP1
	dw 0, 0		; SS1, reserved

	dd 0		; ESP2
	dw 0, 0		; SS2, reserved

	dd 0		; CR3
	dd 0, 0		; EIP, EFLAGS
	dd 0, 0, 0, 0	; EAX, ECX, EDX, EBX
	dd 0, 0, 0, 0	; ESP, EBP, ESI, EDI
	dw 0, 0		; ES, reserved
	dw 0, 0		; CS, reserved
	dw 0, 0		; SS, reserved
	dw 0, 0		; DS, reserved
	dw 0, 0		; FS, reserved
	dw 0, 0		; GS, reserved
	dw 0, 0		; LDT, reserved
	dw 0, 104	; debug, IO permission bitmap (none)

; null descriptor. gdt_ptr could be put here to save a few bytes,
; but that can be confusing
gdt:
	dw 0		; limit 15:0
	dw 0		; base 15:0
	db 0		; base 23:16
	db 0		; type
	db 0		; limit 19:16, flags
	db 0		; base 31:24

; descriptor for task-state segment (TSS)
TSS_SELECTOR EQU ($-gdt)
gdt_tss:
	dw 103
	dw 0
	db 0
	db 0x89	; present, ring 0, available 32-bit TSS
	db 0
	db 0

; ring 0 kernel code segment descriptor
KERNEL_CS EQU ($-gdt)
gdt_kcode:
	dw 0xFFFF
	dw 0
	db 0
	db 0x9A	; present, ring 0, code, non-conforming, readable
	db 0xCF
	db 0

; ring 0 kernel data segment descriptor
KERNEL_DS EQU ($-gdt)
gdt_kdata:
	dw 0xFFFF
	dw 0
	db 0
	db 0x92	; present, ring 0, data, expand-up, writable
	db 0xCF
	db 0

; ring 3 user code segment descriptor
USER_CS EQU (($-gdt)|3)
gdt_ucode:
	dw 0xFFFF
	dw 0
	db 0
	db 0xFA	; present, ring 3, code, non-conforming, readable
	db 0xCF
	db 0

; ring 3 user data segment descriptor
USER_DS EQU (($-gdt)|3)
gdt_udata:
	dw 0xFFFF
	dw 0
	db 0
	db 0xF2	; present, ring 3, data, expand-up, writable
	db 0xCF
	db 0
gdt_end:

gdt_ptr:
	dw gdt_end - gdt - 1 ; GDT limit
	dd gdt		; linear adr of GDT (set above)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; kernel .bss
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SEGMENT BSS

	dd 0
	align 4096 ; 4K kernel stack
stack:

EXP g_kvirt_to_phys
	dd 0
EXP g_mboot_info
	dd 0
