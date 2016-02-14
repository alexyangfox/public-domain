; This code snippet is public domain (no copyright).
; You can do whatever you want with it.

; assume:
; - base address of CS and DS descriptors from the bootloader == 0
; - you have a small but working stack (SS and ESP are set)

	BITS 32

GLOBAL entry
entry:
	call where_am_i
where_am_i:
        pop esi                 ; ESI=physical adr of where_am_i  e.g. 0x00100005
        sub esi,where_am_i      ; subtract kernel virtual adr     e.g. 0xC0000005

; now ESI=virt_to_phys. Use it to set up a new GDT that performs
; segment-based address translation. Until then, all data segment
; references are relative to ESI (virt_to_phys)
	mov eax,esi
	shr eax,16
	mov [esi + gdt2 + 2],si ; kernel code segment
	mov [esi + gdt2 + 4],al
	mov [esi + gdt2 + 7],ah
	mov [esi + gdt3 + 2],si ; kernel data segment
	mov [esi + gdt3 + 4],al
	mov [esi + gdt3 + 7],ah
	add [esi + gdt_ptr + 2],esi
	lgdt [esi + gdt_ptr]
	mov ax,SYS_DATA_SEL
	mov ds,eax
	mov ss,eax
	mov es,eax
	mov fs,eax
	mov gs,eax
	jmp SYS_CODE_SEL:pmode
pmode:
; everything should work properly from here on
	...
	...


gdt:
	dd 0, 0

LINEAR_SEL	equ	$-gdt
	dw 0FFFFh	; max. limit
	dw 0
	db 0, 92h, 0CFh, 0

SYS_CODE_SEL	equ	$-gdt
gdt2:
	dw 0FFFFh
	dw 0		; (base gets set above)
	db 0, 9Ah, 0CFh, 0

SYS_DATA_SEL	equ	$-gdt
gdt3:
	dw 0FFFFh
	dw 0		; (base gets set above)
	db 0, 92h, 0CFh, 0
gdt_end:

gdt_ptr:
	dw gdt_end - gdt - 1		; GDT limit
	dd gdt                          ; linear adr of GDT (set above)
