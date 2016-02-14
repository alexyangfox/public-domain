;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Enable "unreal" mode
; Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
; Release date: ?
; This code is public domain (no copyright).
; You can do whatever you want with it.
;
; Unreal mode is identical with real mode with one exception: 32-bit
; addresses greater than 0xFFFF are allowed (they do not cause INT 0Dh,
; as they do in true real mode). This lets you do things like this:
;
;; put '!' in upper left corner of screen:
;	xor ax,ax
;	mov es,ax
;	mov byte [es:dword 0B8000h],'!'
;
; This code will fail if run in virtual 8086 mode (Windows DOS box
; or EMM386 loaded). Oh yeah, a 32-bit CPU is required (386+)
;
; This trick doesn't work (well) with the CS register.
; The CS limit goes back to 64K when CS is modified in
; unreal mode. (On some new CPUs, the CS limit might go
; back to 64K immediately after CR0.PE=0)
;
; I don't know offhand if it works with the SS register.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; we're in real mode
BITS 16

; This code is a snippet!
; You must write additional code here to do anything useful.

        ; ...

	push ds
	push es
		xor eax,eax     ; point gdt_ptr to gdt
		mov ax,ds
		shl eax,4
		add eax,gdt     ; EAX=linear address of gdt
		mov [gdt_ptr + 2],eax
		cli             ; interrupts off
		lgdt [gdt_ptr]
		mov eax,cr0
		or al,1
		mov cr0,eax     ; CR0.PE=1: enable protected mode
		mov bx,DATA_SEL ; selector to segment with 4G limit
		mov ds,bx
		mov es,bx       ; set ES/DS segment limits in descriptor caches
		dec al
		mov cr0,eax     ; CR0.PE=0: back to (un)real mode
	pop es                  ; segment regs back to old values,
	pop ds                  ; but now 32-bit addresses are OK in real mode

	; ...

gdt:    dw 0                    ; limit 15:0
	dw 0                    ; base 15:0
	db 0                    ; base 23:16
	db 0                    ; access byte (descriptor type)
	db 0                    ; limit 19:16, flags
        db 0                    ; base 31:24
DATA_SEL        equ     $-gdt
	dw 0FFFFh
	dw 0
	db 0
        db 92h          ; present, ring 0, data, expand-up, writable
	db 0CFh		; page-granular, 32-bit
	db 0
gdt_end:

gdt_ptr:
        dw gdt_end - gdt - 1    ; GDT limit
        dd 0                    ; linear adr of GDT (set above)
