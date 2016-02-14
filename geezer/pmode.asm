;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; pmode.asm - quick start with (32-bit) protected mode
; Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
; Release date: April 2, 2005
; This code is public domain (no copyright).
; You can do whatever you want with it.
;
; Assemble this code with NASM:
;	nasm -f bin -o pmode.com pmode.asm
;
; then run pmode.com from plain DOS or from
; a bootloader that supports .COM files
;
; My recommended practices for protected mode:
;
; 1. For mixed 16- and 32-bit code, such as code that enters
;    protected mode, use NASM.
;
; 2. Use Bochs to debug protected mode code:
;	http://bochs.sourceforge.net
;    Get the version with the built-in debugger. The debugger has a
;    cryptic command-line user interface, but it's very useful.
;
; 3. Use video memory for debugging. After each crucial step in
;    the process of switching to pmode, poke a character into text
;    video memory, so you can see (literally) how far the code gets.
;
; 4. Configure the code and data segment descriptors so
;    everything has the same address in real mode and pmode
;
; 5. To change the segment base addresses once you're in pmode
;    (e.g. to switch to segments with base address = 0):
;    a. Write the new CS and EIP values into the immediate
;       operand of a JMP FAR instruction
;    b. Load DS, SS, ES, FS, and GS
;    c. Use JMP FAR with an immediate operand to load CS and EIP.
;    This method avoids confusing and error-prone JMP or RETF
;    instructions that depend on the DS or SS registers.
;
; 6. Simplify privilege scheme:
;    a. Use only privilege rings 0 and 3
;    b. Use only CPL = RPL = DPL
;    c. Ignore the 'conforming' bit of code segment descriptors
;
; 7. Don't use LDTs, call gates, or task gates. Changing CPU privilege
;    levels can be done with interrupt or trap gates. Task-switching
;    can be done by manipulating the kernel stack. Both of these
;    approaches are also portable to non-x86 CPUs.
;
; 8. Build the IDT at run-time. The 32-bit EIP value stored in each
;    interrupt gate is broken into two 16-bit values, with the other
;    four bytes of the gate between them. No common object file format
;    supports the type of relocation needed for such a value.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	ORG 100h		; .COM file start address

	BITS 16

; check for DOS PSP to see if we booted from DOS or from a bootloader
	mov ax,[es:0]
	cmp ax,20CDh		; "INT 20h"
	jne no_dos
	inc byte [dos]
no_dos:

; check for 32-bit CPU
	pushf
		pushf
		pop bx		; old FLAGS -> BX
		mov ax,bx
		xor ah,70h	; try changing b14 (NT)...
		push ax		; ... or b13:b12 (IOPL)
		popf
		pushf
		pop ax		; new FLAGS -> AX
	popf
	xor ah,bh		; 32-bit CPU if we changed NT...
	and ah,70h		; ...or IOPL
	jne cpu_ok
	mov si,cpu_msg

; display error message and either reboot or exit to DOS
msg_and_exit:
	call cputs
	xor ax,ax
	or al,[dos]
	jne dos_exit

; we were started by a bootloader, so reboot
	mov ah,0		; await key pressed
	int 16h

	int 19h			; re-start the boot process

; we were started from DOS, so exit to DOS
dos_exit:
	mov ax,4C01h
	int 21h

cpu_ok:
; if DOS, check if CPU is in Virtual 8086 mode (Windows DOS box or EMM386)
	xor ax,ax
	or al,[dos]
	je real_mode

	smsw ax			; 'SMSW' is a 286+ instruction
	and al,1
	mov si,v86_msg
	jne msg_and_exit
real_mode:

; point to real-mode text video segment. We will poke characters into
; video memory for debugging purposes. If everything works OK, the top
; line of the display will read "0123456789A". These DEBUG statements
; can be taken out (or just commented out) after you've modified the
; code to your satisfaction, and everything works properly.
	mov ax,0B800h		; DEBUG
	mov es,ax		; DEBUG
	mov byte [es:0],'0'	; DEBUG: this code loaded OK?

; It's good to have pmode code and data segments with the same base
; addresses as the real-mode code. This greatly simplifies the
; transition to protected mode. To make this happen, though, we must
; "patch" or "fix up" some addresses in the protected mode tables.
;
; This patching isn't strictly necessary if you start out with segment
; base address = 0 (all segment registers = 0 in real mode). However,
; the patching code is small, it makes this code more flexible and
; robust, and it doesn't hurt anything to leave it in place.
;
; Set the pmode code segment base = 16 * CS
	xor ebx,ebx
	mov byte [es:2],'1'	; DEBUG: 32-bit instruction above worked OK?
	mov bx,cs		; get real-mode segment value
	shl ebx,4		; EBX = CS * 16

	mov eax,ebx
	shr eax,16
	mov [gdt_cs + 2],bx
	mov [gdt_cs + 4],al
	mov [gdt_cs + 7],ah

; Set the pmode data segment base = 16 * DS. For .COM files like this
; one, CS=DS, but that is not always the case.
	xor ebx,ebx
	mov bx,ds		; get real-mode segment value
	shl ebx,4		; EBX = DS * 16

	mov eax,ebx
	shr eax,16
	mov [gdt_ds + 2],bx
	mov [gdt_ds + 4],al
	mov [gdt_ds + 7],ah

; the GDT address in the GDT "pseudo-descriptor", stored at [gdt_ptr+2],
; is also a linear address, and must also be "patched":
	;mov eax,gdt
	;add eax,ebx
	lea eax,[ebx + gdt]
	mov [gdt_ptr + 2],eax

; Intel segmentation is a form of address translation: the address
; generated by your program is a "virtual" address; different from
; the "physical" address that goes out onto the bus, to the RAM chips.
; The conversion value, virt_to_phys, is just the segment base address:
	mov [virt_to_phys],ebx

; Done patching; ready to move to pmode. Disable interrupts by clearing the
; IF bit in EFLAGS register. If the IOPL or NT bits (also in EFLAGS) happen
; to be set, they may cause problems later. So, instead of using CLI, use
; the code below to zero IF, IOPL, and NT
	push dword 2
	popfd
	mov byte [es:4],'2'	; DEBUG: patching and POPFD worked OK?

; load register GDTR with a pointer to the 6-byte GDT pointer
	lgdt [gdt_ptr]

; set the PE bit in register CR0
	mov eax,cr0
	or al,1
	mov cr0,eax

; Pmode doesn't "kick in" until all the segment registers are reloaded.
; We can still use real-mode addressing with ES, like this:
	mov byte [es:6],'3'	; DEBUG: LGDT and setting PE bit worked OK?

; reload CS with a far jump
	jmp SYS_CODE_SEL:pmode

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:			cputs
; action:		displays text on screen
; in:			0-terminated string at SI
; out:			(nothing)
; modifies:		(nothing)
; minimum CPU:		8088
; notes:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

cputs:
	push si
	push bx
	push ax
		mov ah,0Eh	; INT 10h: teletype output
		xor bx,bx	; video page 0
		jmp short cputs_2
cputs_1:
		int 10h
cputs_2:
		lodsb
		or al,al
		jne cputs_1
	pop ax
	pop bx
	pop si
	ret

cpu_msg:
	db "32-bit CPU required", 13, 10, 0

v86_msg:
	db "CPU is in Virtual 8086 mode (Windows DOS box or EMM386 loaded)"
	db 13, 10, 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 32-bit pmode code starts here
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	BITS 32

pmode:
; This is where a lot of pmode code fails. The main problems are:
; 1. The linear address of the GDT, at [gdt_ptr+2], was not
;    set properly, so register GDTR points to a bogus GDT
; 2. The code segment descriptor, pointed to by selector SYS_CODE_SEL,
;    does not have the proper segment base address, so the far jump
;    above goes off into the weeds, instead of coming here
; 3. You left hardware interrupts enabled. Pmode interrupt
;    handling hasn't bee set up yet, so your program will crash.
;
; Until we reload it, addressing relative to ES still works
; as it did in real mode
	mov byte [es:8],'4'	; DEBUG: far JMP worked OK?

; load pmode data segment selectors into data segment registers
	mov ax,SYS_DATA_SEL
	mov ds,ax
	mov byte [es:0Ah],'5'	; DEBUG: loading DS worked OK?
	mov ss,ax
	mov fs,ax
	mov gs,ax
	mov byte [es:0Ch],'6'	; DEBUG: loading SS,FS,GS worked OK?

; now try to access text video memory using LINEAR_DATA_SEL with a 32-bit address
	mov ax,LINEAR_DATA_SEL
	mov es,ax
	mov byte [es:0B800Eh],'7' ; DEBUG: LINEAR_DATA_SEL works OK?

; because we are lazy, our pmode code will use the same stack used by
; the real mode code. However, the top 16 bits of ESP must be zeroed
	xor eax,eax
	mov ax,sp
	mov esp,eax

; test the stack by calling a subroutine
; demonstrate video memory access with far pointers
	call far_demo
	mov byte [es:0B8012h],'9' ; DEBUG: return (and stack) worked OK?

; demonstrate video memory access with near pointers
	call near_demo

; how to switch to linear code and data segments (base address = 0)
; without crashing:
;
; 1. store new CS and EIP values into the
;    immediate operand of a far JMP instruction:
;    (EBX still contains virt_to_phys)
	lea eax,[ebx + linear]
	;mov [cs:linear - 6],eax	; this causes a reboot
	mov [linear - 6],eax		; xxx - TINY memory model only

; 2. load data segment registers
	mov ax,LINEAR_DATA_SEL
	mov ds,ax
	mov ss,ax
	;mov es,ax
	mov fs,ax
	mov gs,ax
	mov byte [0B8014h],'A' ; DEBUG: linear DS ok?

; 3. use far JMP with immediate operand to load CS and EIP
;
;		offset
; offset	from
; from JMP	'linear'	size	value		description
; --------	--------	----	-----		-----------
; 0		-7		1	0EAh		JMP FAR immediate
; 1		-6		4	-		new EIP value
; 5		-2		2	LINEAR_CODE_SEL	new CS value
; 7		 0		-	-		next instruction
;
	jmp LINEAR_CODE_SEL:0
linear:
	mov byte [0B8016h],'B' ; DEBUG: linear CS ok?

; 4. adjust ESP if necessary or desired:
	add esp,ebx

; demonstrate video memory access with near pointers,
; and code and data segments with base address = 0
	call inv_demo

; go into an infinite loop
	mov byte [0B8018h],'C' ; DEBUG: now in infinite loop
	jmp $

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; writes string at DS:ESI into video memory using far pointers
; assumes ES=selector to data segment with base address = 0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

far_demo:
	mov byte [es:0B8010h],'8'; DEBUG: subroutine call worked OK?
	push edi
	push esi
	push ecx
		mov esi,far_msg		; DS:ESI points to message text
		mov edi,0B80A0h		; ES:EDI points to 2nd line
		mov ecx,far_msg_len	; how many bytes to copy?
		rep movsb		; copy them
	pop ecx
	pop esi
	pop edi
	ret

; the alternating spaces are treated as "attribute" bytes by the VGA.
; Their value is 20h, so the text is black (color 0) on green (color 2).
far_msg:
	db "h e l l o "
far_msg_len equ $ - far_msg

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; writes string at ESI into video memory using near pointers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

near_demo:
	push es
	push edi
	push esi
	push ecx
		push ds
		pop es
		mov esi,near_msg	; DS:ESI points to message text
		mov edi,0B8140h		; ES:EDI points to 3rd line
		sub edi,[virt_to_phys]
		mov ecx,near_msg_len	; how many bytes to copy?
		rep movsb		; copy them
	pop ecx
	pop esi
	pop edi
	pop es
	ret

near_msg:
	db "g o o d b y e "
near_msg_len equ $ - near_msg

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; writes string at ESI into video memory using near pointers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

inv_demo:
	push edi
	push esi
	push ecx

; now we translate the address in ESI, not the address in EDI
		mov esi,inv_near_msg	; DS:ESI points to message text
		add esi,ebx		; (EBX still contains virt_to_phys)
		mov edi,0B81E0h		; ES:EDI points to 4th line
		mov ecx,inv_near_msg_len ; how many bytes to copy?
		rep movsb		; copy them
	pop ecx
	pop esi
	pop edi
	ret

inv_near_msg:
	db "i n v e r s e "
inv_near_msg_len equ $ - inv_near_msg

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

dos:
	db 0

virt_to_phys:
	dd 0

gdt:
; null descriptor
	dw 0			; limit 15:0
	dw 0			; base 15:0
	db 0			; base 23:16
	db 0			; type
	db 0			; limit 19:16, flags
	db 0			; base 31:24

; linear code descriptor
LINEAR_CODE_SEL	equ	$-gdt
	dw 0FFFFh               ; maximum limit 0FFFFFh (1 meg or 4 gig)
	dw 0			; base for LINEAR_DATA_SEL is always 0
	db 0
	db 9Ah			; present,ring 0,code,non-conforming,readable
	db 0CFh                 ; page-granular (4 gig limit), 32-bit
	db 0

; linear data descriptor
LINEAR_DATA_SEL	equ	$-gdt
	dw 0FFFFh               ; maximum limit 0FFFFFh (1 meg or 4 gig)
	dw 0			; base for LINEAR_DATA_SEL is always 0
	db 0
	db 92h			; present, ring 0, data, expand-up, writable
	db 0CFh                 ; page-granular (4 gig limit), 32-bit
	db 0

gdt_cs:
; code descriptor
SYS_CODE_SEL	equ	$-gdt
	dw 0FFFFh		; maximum limit 0FFFFFh (1 meg or 4 gig)
	dw 0			; base address; gets patched above
	db 0
	db 9Ah			; present,ring 0,code,non-conforming,readable
	db 0CFh			; page-granular (4 gig limit), 32-bit
	db 0

gdt_ds:
; data descriptor
SYS_DATA_SEL	equ	$-gdt
	dw 0FFFFh		; maximum limit 0FFFFFh (1 meg or 4 gig)
	dw 0			; base address; gets patched above
	db 0
	db 92h			; present, ring 0, data, expand-up, writable
	db 0CFh			; page-granular (4 gig limit), 32-bit
	db 0
gdt_end:

; To save 6 bytes of memory, this structure can be stored at (gdt+0);
; in the NULL descriptor. That can be confusing, however.
gdt_ptr:
	dw gdt_end - gdt - 1	; GDT limit
	dd gdt			; linear address of GDT; gets patched above
