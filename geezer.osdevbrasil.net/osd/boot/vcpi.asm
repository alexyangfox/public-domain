;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Switch from V86 mode to real mode, using VCPI (NASM code)
; Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
; Release date: ?
; This code is public domain (no copyright).
; You can do whatever you want with it.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	ORG 100h		; .COM file
;;;;
;;;; now in real mode (plain DOS), virtual 8086 mode with VCPI
;;;; (DOS with EMM386), or virtual 8086 mode w/o VCPI (Windows DOS box)
;;;;

	mov ax,cs
	mov [real_mode_cs],ax

; maybe check for 32-bit CPU here
; maybe allocate your own stack

; zero top 16 bits of ESP
	xor eax,eax
	mov ax,sp
	mov esp,eax

; we'll use this value a lot:
	xor ebp,ebp
	mov bp,cs
	shl ebp,4

; patch things that depend on the load adr
	mov eax,ebp
	mov [gdt3 + 2],ax	; CODE_SEL
	mov [gdt4 + 2],ax	; DATA_SEL
	mov [gdt5 + 2],ax	; CODE_SEL16
	mov [gdt6 + 2],ax	; DATA_SEL16
	shr eax,16
	mov [gdt3 + 4],al
	mov [gdt4 + 4],al
	mov [gdt5 + 4],al
	mov [gdt6 + 4],al
	mov [gdt3 + 7],ah
	mov [gdt4 + 7],ah
	; mov [gdt5 + 7],ah	; no, these are 16-bit descriptors
	; mov [gdt6 + 7],ah	; and have only a 24-bit base address

	mov eax,tss
	add eax,ebp
	mov [gdt7 + 2],ax	; TSS_SEL
	shr eax,16
	mov [gdt7 + 4],al
	mov [gdt7 + 7],ah

	add [gdt_ptr + 2],ebp
	add [idt_ptr + 2],ebp

	add [vcpi_gdtr],ebp
	add [vcpi_idtr],ebp

; check for virtual 8086 mode:
	smsw ax
	test al,1		; look at PE bit of MSW (CR0)
	je near real

; in V86 mode; check for VCPI (e.g. DOS with EMM386 loaded)
	mov ax,0DE00h
	int 67h
	cmp ah,0
	je got_vcpi
	mov si,v86_msg

die:
; exit to DOS with error message
	mov ah,0Eh		; INT 10h: teletype output
	xor bx,bx		; video page 0
	jmp die3
die2:
	int 10h
die3:
	lodsb
	or al,al
	jne die2
	mov ax,4C00h		; DOS terminate
	int 21h

got_vcpi:
; use VCPI to switch from V86 mode to paged pmode
	mov edi,ebp		; find 4K-aligned mem for page table
	add edi,(page_info + 4095)
	and di,0F000h		; EDI=linear adr of page table

	mov eax,edi
	add eax,4096		; linear adr of page dir, 4K above table
	mov [vcpi_cr3],eax

	mov eax,edi
	sub edi,ebp		; DI=offset of page table
	add di,4096		; point to page dir
	or al,7			; ring 3, writable, present
	mov [di + 0],eax	; page dir[0] -> linear adr of page table
	sub di,4096		; INT 67h AX=DE01h will fill pg table @ DI

	mov si,gdt8		; fill 3 VCPI descriptors @ SI
	mov ax,0DE01h
	int 67h
	cmp ah,0
	mov si,vcpi_err_msg
	jne die
	push dword 0		; disable interrupts (set IF=0)...
	popfd			; ...set IOPL=0, and clear the NT bit
	mov esi,ebp
	add esi,vcpi_control_block
	mov ax,0DE0Ch		; switch from V86 mode to paged pmode
	int 67h			; jump to vcpi_to_pmode32

	BITS 32

vcpi_to_pmode32:		; now in ring 0 paged 32-bit pmode
	mov eax,cr0
	and eax,7FFFFFFFh	; turn off paging
	mov cr0,eax
	xor eax,eax
	mov cr3,eax		; flush TLB (the page table cache)
	jmp CODE_SEL16:pmode32_to_pmode16

	BITS 16

pmode32_to_pmode16:		; finish switching to 16-bit pmode
	mov ax,DATA_SEL16
	mov ss,ax
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov gs,ax
	mov eax,cr0
	and al,0FEh		; pmode off
	mov cr0,eax
	jmp far [real_mode_ip]

pmode16_to_real:		; finish switching to real mode
	lidt [real_idt_ptr]
	mov ax,cs
	mov ss,ax
	mov ds,ax
	mov es,ax
	mov fs,ax
	mov gs,ax
real:

;;;;
;;;; your real-mode code here
;;;; if EMM386 was loaded, don't try to return to DOS (it will hang)
;;;;
	mov ax,0B800h
	mov es,ax
	mov byte [es:0],'h'
	mov byte [es:2],'e'
	mov byte [es:4],'l'
	mov byte [es:6],'l'
	mov byte [es:8],'o'
	jmp $

; handler for VCPI exceptions

	BITS 32

unhand:
	mov ax,LINEAR_DATA_SEL
	mov ds,ax
	mov byte [dword 0B8000h],'!'
	jmp $

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

v86_msg:
	db "CPU is in virtual 8086 mode, but no VCPI", 13, 10
	db "Are you running this code from Windows? (don't)", 13, 10, 0

vcpi_err_msg:
	db "VCPI call (INT 67h AX=DE01h) failed (?)", 13, 10, 0

page_info:
	times 1024 dd 0         ; padding to 4K boundary
	times 1024 dd 0         ; 4K page table somewhere in here
	dd 0			; a page directory with one entry

tss:
	dw 0, 0			; back link

	dd 0			; ESP0
	dw DATA_SEL, 0		; SS0, reserved

	dd 0			; ESP1
	dw 0, 0			; SS1, reserved

	dd 0			; ESP2
	dw 0, 0			; SS2, reserved

	dd 0			; CR3
	dd 0, 0			; EIP, EFLAGS
	dd 0, 0, 0, 0		; EAX, ECX, EDX, EBX
	dd 0, 0, 0, 0		; ESP, EBP, ESI, EDI
	dw 0, 0			; ES, reserved
	dw 0, 0			; CS, reserved
	dw 0, 0			; SS, reserved
	dw 0, 0			; DS, reserved
	dw 0, 0			; FS, reserved
	dw 0, 0			; GS, reserved
	dw 0, 0			; LDT, reserved
	dw 0, 0			; debug, IO perm. bitmap

gdt:
	dw 0			; limit 15:0
	dw 0			; base 15:0
	db 0			; base 23:16
	db 0			; type
	db 0			; limit 19:16, flags
	db 0			; base 31:24
LINEAR_CODE_SEL	equ	$-gdt
	dw 0FFFFh
	dw 0
	db 0
	db 9Ah			; present, ring 0, code, non-conforming, readable
	db 0CFh			; page-granular, 32-bit
	db 0
LINEAR_DATA_SEL	equ	$-gdt
	dw 0FFFFh
	dw 0
	db 0
	db 92h			; present, ring 0, data, expand-up, writable
	db 0CFh			; page-granular, 32-bit
	db 0
CODE_SEL	equ	$-gdt
gdt3:
	dw 0FFFFh
	dw 0
	db 0
	db 9Ah			; present, ring 0, code, non-conforming, readable
	db 0CFh			; page-granular, 32-bit
	db 0
DATA_SEL	equ	$-gdt
gdt4:
	dw 0FFFFh
	dw 0
	db 0
	db 92h			; present, ring 0, data, expand-up, writable
	db 0CFh			; page-granular, 32-bit
	db 0
CODE_SEL16	equ	$-gdt
gdt5:
	dw 0FFFFh
	dw 0
	db 0
	db 9Ah			; present, ring 0, code, non-conforming, readable
	db 0			; byte-granular, 16-bit
	db 0
DATA_SEL16	equ	$-gdt
gdt6:
	dw 0FFFFh
	dw 0
	db 0
	db 92h			; present, ring 0, data, expand-up, writable
	db 0			; byte-granular, 16-bit
	db 0
TSS_SEL		equ	$-gdt
gdt7:
	dw 103
	dw 0
	db 0
	db 089h			; ring 0 available 32-bit TSS
	db 0
	db 0
VCPI_CODE_SEL	equ	$-gdt
gdt8:				; these are set by INT 67h AX=DE01h
	dd 0, 0
VCPI_DATA_SEL	equ	$-gdt
	dd 0, 0
VCPI_LINEAR_SEL	equ	$-gdt
	dd 0, 0
gdt_end:

idt:
	%rep 32
		dw unhand	; low 16 bits of ISR offset
		dw CODE_SEL	; selector
		db 0
		db 8Eh		; present, ring 0, 32-bit intr gate
		dw 0		; high 16 bits of ISR (unhand >> 16)
	%endrep
idt_end:

gdt_ptr:
	dw gdt_end - gdt - 1	; GDT limit
	dd gdt			; linear, physical adr of GDT

idt_ptr:
	dw idt_end - idt - 1	; IDT limit
	dd idt			; linear, physical address of IDT

vcpi_control_block:
vcpi_cr3:
	dd 0
vcpi_gdtr:
	dd gdt_ptr
vcpi_idtr:
	dd idt_ptr
;vcpi_ldtr:
	dw 0
;vcpi_tr:
	dw TSS_SEL
;vcpi_eip:
	dd vcpi_to_pmode32
;vcpi_cs:
	dw CODE_SEL

real_idt_ptr:
	dw 3FFh			; limit 1023
	dd 0			; IDT (IVT, actually) at address 0

real_mode_ip:
	dw pmode16_to_real
real_mode_cs:
	dw 0
