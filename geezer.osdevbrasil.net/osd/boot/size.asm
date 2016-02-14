; NASM code to probe floppy geometry.
; Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
; Release date: ?
; This code is public domain (no copyright).
; You can do whatever you want with it.
;
; This code does not use the floppy parameter table (INT 1E vector),
; nor INT 13h AH=08, nor the sectors-per-track value in the BIOS
; parameter block (BPB is a feature of the FAT filesystem only)
;
; Disadvantages: the probe takes several seconds, and the probe
; code consumes about 90 bytes

; DOS .COM file
	ORG 100h

; reassure the user
	mov si,msg0
	call cputs

; use drive A:
	mov dl,0

; prepare for INT 13h AH=02h
	push ds
	pop es
	mov dh,0	; head
	mov ch,0	; cylinder 7:0
	mov bx,buffer

disk_change:
	mov cl,0	; cylinder 9:8 and sector
loop_top:
	inc cl

; make 3 attempts to read this sector
	mov di,3
try:
	mov ah,02h	; read
	mov al,1	; 1 sector
	int 13h

; success; increment sector count and loop
	jnc loop_top
	cmp ah,6
	je disk_change
	dec di
	je done

; reset disk
	mov ah,0
	int 13h
	jmp short try

; decrement sector count
done:
	dec cl

; read sector 0 to reset drive
	push cx
		mov ah,02h	; read
		mov al,1	; 1 sector
		mov cl,1
		int 13h
	pop cx

; report results
	mov si,msg1
	call cputs

; convert detected sectors-per-track value (in CL) to ASCII
	mov al,cl
	cbw
	xor dx,dx
	mov bx,10
	call wrnum

	mov si,msg2
	call cputs

; exit to DOS
	mov ax,4C00h
	int 21h

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:			putch
; action:		writes one character to screen
; in:			ASCII character in AL
; out:			(nothing)
; modifies:		(nothing)
; minimum CPU:		8088
; notes:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

putch:
	push bx
	push ax
		mov ah,0Eh	; INT 10h: teletype output
		xor bx,bx	; video page 0
		int 10h
	pop ax
	pop bx
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:			cputs
; action:		writes ASCIZ string to text screen
; in:			0-terminated string at DS:SI
; out:			(nothing)
; modifies:		(nothing)
; minimum CPU:		8088
; notes:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

cputs:
	push si
	push ax
		jmp cputs2
cputs1:
		call putch
cputs2:
		lodsb
		or al,al
		jne cputs1
	pop ax
	pop si
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:			wrnum
; action:		writes 32-bit value to text screen
; in:			32-bit unsigned value in DX:AX, radix in BX
; out:			(nothing)
; modifies:		(nothing)
; minimum CPU:		8088
; notes:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

wrnum:
	push si
	push dx
	push cx
	push ax
		mov si,num_buf

; extended precision division from section 9.3.5
; of Randall Hyde's "Art of Assembly"
; start: DX=dividend MSW, AX=dividend LSW, BX=divisor
wrnum1:
		push ax
			mov ax,dx
			xor dx,dx

; before div: DX=0, AX=dividend MSW, BX=divisor
; after div:  AX=quotient MSW, DX=intermediate remainder
			div bx
			mov cx,ax
		pop ax

; before div: DX=intermediate remainder, AX=dividend LSW, BX=divisor
; after div:  AX=quotient LSW, DX=remainder
		div bx

; end: DX=quotient MSW, AX=quotient LSW, CX=remainder
		xchg dx,cx
		add cl,'0'
		cmp cl,'9'
		jbe wrnum2
		add cl,('A'-('9'+1))
wrnum2:
		dec si
		mov [si],cl

		mov cx,ax
		or cx,dx
		jne wrnum1
		call cputs
	pop ax
	pop cx
	pop dx
	pop si
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	times 40 db 0
num_buf:
	db 0

buffer:
	times 512 db 0

msg0:
	db "probing floppy geometry; please wait a few seconds..."
	db 13, 10, 0
msg1:
	db "This floppy has ", 0
msg2:
	db " sectors per track", 13, 10, 0
