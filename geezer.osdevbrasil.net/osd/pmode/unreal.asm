;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Enable "unreal" mode
; This code is public domain (no copyright).
; You can do whatever you want with it.
;
; Unreal mode is identical with real mode with one exception: 32-bit
; addresses are allowed (they do not cause INT 0Dh, as they do in real mode)
;
; This code will fail if run in virtual 8086 mode (Windows DOS box
; or EMM386 loaded). Oh yeah, a 32-bit CPU is required (386+)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; we're in real mode
BITS 16

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
                mov cr0,eax     ; partial switch to 32-bit pmode
                mov bx,DATA_SEL ; selector to segment w/ 4G limit
		mov ds,bx
                mov es,bx       ; set seg limits in descriptor caches
		dec al
                mov cr0,eax     ; back to (un)real mode
        pop es                  ; segment regs back to old values,
        pop ds                  ; but now 32-bit addresses are OK

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
