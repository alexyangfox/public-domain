;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Demo of INT 10h AH=0Eh
; This code is public domain (no copyright).
; You can do whatever you want with it.
;
; assemble with NASM:
;	nasm -f bin -o bios0e.com bios0e.asm
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	ORG 100h			; DOS .COM file

	mov si,msg
	call cputs

	mov ax,4C00h			; exit to DOS with errorlevel 0
	int 21h

msg:
	db "Hi, how's it going? Wie geht's? Que pasa?", 13, 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:		putch
; action:	writes character in AL to screen; advances cursor
; in:		AL=char
; out:		(nothing)
; modifies:	(nothing)
; minimum CPU:	8088
; notes:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

putch:
	push bx
	push ax
		mov ah,0Eh		; INT 10h: teletype output
		xor bx,bx		; video page 0
		int 10h
	pop ax
	pop bx
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:		cputs
; action:	writes 0-terminated string to screen
; in:		SI -> string
; out:		(nothing)
; modifies:	(nothing)
; minimum CPU:	8088
; notes:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

cputs:
	push si
	push ax
	cld				; string operations go up
		jmp short cputs_2
cputs_1:
		call putch
cputs_2:
		lodsb
		or al,al
		jne cputs_1
	pop ax
	pop si
	ret
