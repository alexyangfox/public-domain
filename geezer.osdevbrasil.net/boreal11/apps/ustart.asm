;-----------------------------------------------------------------------------
; ASM STARTUP CODE FOR TASKS
;-----------------------------------------------------------------------------

%include "nasm.inc"

; IMPORTS
; from C code
IMP main

; from C library
IMP exit
IMP open

; xxx - these are defined in OS.H
O_RDONLY EQU 0x01
O_WRONLY EQU 0x02

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; entry point
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SEGMENT TEXT
..start:

EXP uentry
; open fd 0=stdin:
; if(open("/dev/con", O_RDONLY) < 0) error();
	push dword O_RDONLY
	push dword con_dev_name
		call open
	add esp,byte 8
	or eax,eax
	js error

; open fd 1=stdout:
; if(open("/dev/con", O_WRONLY) < 0) error();
	push dword O_WRONLY
	push dword con_dev_name
		call open
	add esp,byte 8
	or eax,eax
	js error

; call application main()
	call main
	jmp done
error:
	mov eax,1
done:
	push eax
		call exit

; exit() should not return -- try to generate illegal exception if it does
		ud2b	; db 0Fh, 0B9h

; if that doesn't work, just go into a loop
		jmp short $

con_dev_name:
	db "/dev/con", 0
