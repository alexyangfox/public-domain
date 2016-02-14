;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; DOS program to set 90x30 text mode
; Chris Giese <geezer@execpc.com>	http://www.execpc.com/~geezer
; Release date: ?
; This code is public domain (no copyright).
; You can do whatever you want with it.
;
; VGA-standard sync rates; should work with VGA video board and VGA monitor.
;
; Assemble with NASM:
;	nasm -f bin -o 90x30.com 90x30.asm
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; VGA_AC_INDEX
; VGA_AC_WRITE
; VGA_AC_READ
VGA_MISC_WRITE		EQU	3C2h
VGA_SEQ_INDEX		EQU	3C4h
VGA_SEQ_DATA		EQU	3C5h
; VGA_DAC-READ_INDEX
; VGA_DAC_WRITE_INDEX
; VGA_DAC_DATA
; VGA_MISC_READ
VGA_CRTC_INDEX		EQU	3D4h
VGA_CRTC_DATA		EQU	3D5h
VGA_INSTAT_READ		EQU	3DAh

NUM_SEQ_REGS		EQU	5
NUM_CRTC_REGS		EQU	25
; NUM_GC_REGS
; NUM_AC_REGS

	ORG 100h

; set 80x25 text mode so we're in a known state, and to set 8x16 font
	mov ax,0003h
	int 10h






; set 90x30 mode
	mov si,regs_90x30
	call write_regs

; tell the BIOS what we've done
	mov ax,0040h
	mov ds,ax
	mov es,ax

	mov word [004Ah],90		; columns on screen

	mov word [004Ch],90*30*2	; framebuffer size

	mov cx,8
	mov di,0050h
	xor ax,ax
	rep stosw			; cursor pos for 8 pages

	mov word [0060h],0D0Eh		; cursor shape

	mov byte [0084h],29		; rows on screen, minus one

	mov byte [0085h],16		; char height, in scan-lines

; done; exit to DOS
	mov ax,4C00h
	int 21h

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:		write_regs
; action:	writes register dump to VGA registers
; inputs:	SI->register dump
; outputs:	(nothing)
; modifies:	(nothing)
; minimum CPU:	8088
; notes:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

write_regs:
	push si
	push dx
	push cx
	push ax
		cld

; write MISC register
		mov dx,VGA_MISC_WRITE
		lodsb
		out dx,al

; write SEQuencer registers
		mov cx,NUM_SEQ_REGS
		mov ah,0
write_seq:
		mov dx,VGA_SEQ_INDEX
		mov al,ah
		out dx,al

		mov dx,VGA_SEQ_DATA
		lodsb
		out dx,al

		inc ah
		loop write_seq

; write CRTC registers
; Unlock CRTC registers: enable writes to CRTC regs 0-7
		mov dx,VGA_CRTC_INDEX
		mov al,17
		out dx,al

		mov dx,VGA_CRTC_DATA
		in al,dx
		and al,7Fh
		out dx,al

; Unlock CRTC registers: enable access to vertical retrace regs
		mov dx,VGA_CRTC_INDEX
		mov al,3
		out dx,al

		mov dx,VGA_CRTC_DATA
		in al,dx
		or al,80h
		out dx,al

; make sure CRTC registers remain unlocked
		mov al,[si + 17]
		and al,7Fh
		mov [si + 17],al

		mov al,[si + 3]
		or al,80h
		mov [si + 3],al

; now, finally, write them
		mov cx,NUM_CRTC_REGS
		mov ah,0
write_crtc:
		mov dx,VGA_CRTC_INDEX
		mov al,ah
		out dx,al

		mov dx,VGA_CRTC_DATA
		lodsb
		out dx,al

		inc ah
		loop write_crtc
	pop ax
	pop cx
	pop dx
	pop si
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

regs_90x30:
; MISC
	db 0E7h
; SEQuencer
	db 03h, 01h, 03h, 00h, 02h
; CRTC
	db  6Bh, 59h,  5Ah, 82h, 60h,  8Dh, 0Bh,  3Eh,
	db  00h, 4Fh,  0Dh, 0Eh, 00h,  00h, 00h,  00h,
	db 0EAh, 0Ch, 0DFh, 2Dh, 10h, 0E8h, 05h, 0A3h,
	db 0FFh
; GC (no)
; AC (no)
