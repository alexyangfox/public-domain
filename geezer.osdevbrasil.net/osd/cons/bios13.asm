;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Demo of INT 10h AH=13h
; This code is public domain (no copyright).
; You can do whatever you want with it.
;
; assemble with NASM:
;	nasm -f bin -o bios13.com bios13.asm
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	ORG 100h			; DOS .COM file

; write string
	mov ah,13h
	mov al,1 ; write mode (advance cursor, ASCII string)
	mov bh,0 ; video page
	mov bl,02 ; attribute (green on black)
	mov cx,5 ; string length
	mov dh,2 ; starting row
	mov dl,2 ; starting col
	push cs
	pop es
	mov bp,string1
	int 10h

; same as above, with attribute bytes in the string
	mov ah,13h
	mov al,3 ; write mode (advance cursor, ASCII+attribute string)
	mov bh,0 ; video page
	mov cx,6 ; string length
	mov dh,3 ; starting row
	mov dl,3 ; starting col
	push cs
	pop es
	mov bp,string2
	int 10h

; exit to DOS
	mov ax,4C00h
	int 21h

string1:
	db "hello"
string2:
	db 'C', 01, 'o', 02, 's', 03, 'm', 04, 'o', 05, 's', 06
