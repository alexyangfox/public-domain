;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Demo of BIOS calls that check memory size
; Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
; Release date: Sep 6, 2001
; This code is public domain (no copyright).
; You can do whatever you want with it.
;
; This code uses these interrupts:
;	1. INT 15h AX=E820h (32-bit CPU only)
;	2. INT 15h AX=E801h
;	3. INT 15h AH=88h
;	4. INT 12h
;
; assemble with NASM:	nasm -f bin -o biosmem.com biosmem.asm
;
; rewritten Sep 6, 2001
; - trying to handle possible BIOS bugs per Ralf Brown's list and
;	http://marc.theaimsgroup.com/?l=linux-kernel&m=99322719013363&w=2
; - now displaying memory block info, instead of just a size value
; - new wrnum function
;
; xxx - INT 15h AX=E820h shows a 1K block at the top of conventional
; memory for my system -- is this reserved for the EBDA?
; I don't _have_ an EBDA, at least, not with my current CMOS settings...
; will the 1K block disappear if the EBDA is enabled?
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	ORG 100h

	mov si,mem_msg
	call cputs

; check for 32-bit CPU
	call cpu_is_32bit

; try INT 15h AX=E820h
	or ah,ah
	je cant_e820
	call extmem_int15_e820
	jnc ok

; before trying other BIOS calls, use INT 12h to get conventional memory size
cant_e820:
	int 12h

; convert from K in AX to bytes in CX:BX
	xor ch,ch
	mov cl,ah
	mov bh,al
	xor bl,bl
	shl bx,1
	rcl cx,1
	shl bx,1
	rcl cx,1

; set range base (in DX:AX) to 0 and display it
	xor dx,dx
	xor ax,ax
	call display_range

; try INT 15h AX=E801h
	call extmem_int15_e801
	jnc ok

; try INT 15h AH=88h
	call extmem_int15_88
	jnc ok

; uh-oh
	mov si,err_msg
	call cputs

; exit to DOS
ok:
	mov ax,4C00h
	int 21h

got_32bit_cpu:
	db 0
mem_msg:
	db "Memory ranges:"
crlf_msg:
	db 13, 10, 0
base_msg:
	db "   base=0x", 0
size_msg:
	db ", size=0x", 0
err_msg:
	db "*** All BIOS calls to determine extended memory size "
	db "have failed ***", 13, 10, 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:			cpu_is_32bit
; action:		checks for 32-bit CPU
; in:			(nothing)
; out:			AH != 0 if 32-bit CPU
; modifies:		AX
; minimum CPU:		8088
; notes:		C prototype: extern int cpu_is_32bit(void);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

cpu_is_32bit:
	push bx
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
		xor al,al		; zero AL
		xor ah,bh		; 32-bit CPU if we changed NT...
		and ah,70h		; ...or IOPL
	pop bx
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:			display_range
; action:		printf("base=0x%lX, size=0x%lX\n", DX:AX, CX:BX);
; in:			(nothing)
; out:			(nothing)
; modifies:		(nothing)
; minimum CPU:		8088
; notes:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

display_range:
	push si
	push dx
	push bx
	push ax

; if size==0, do nothing
		mov si,cx
		or si,bx
		je display_range_1
		mov si,base_msg
		call cputs
		push bx
			mov bx,16
			call wrnum
			mov si,size_msg
			call cputs
		pop ax
		mov dx,cx
		call wrnum
		mov si,crlf_msg
		call cputs
display_range_1:
	pop ax
	pop bx
	pop dx
	pop si
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:			extmem_int15_e820
; action:		gets extended memory info using INT 15h AX=E820h
; in:			(nothing)
; out:			(nothing)
; modifies:		(nothing)
; minimum CPU:		386+
; notes:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

buffer_e820:
	times 20h db 0
buffer_e820_len	equ $ - buffer_e820

extmem_int15_e820:
	push es
	push di
	push edx
	push ecx
	push ebx
	push eax
		push ds
		pop es
		mov di,buffer_e820
		xor ebx,ebx		; INT 15h AX=E820h continuation value

		mov edx,534D4150h	; "SMAP"
		mov ecx,buffer_e820_len
		mov eax,0000E820h
		int 15h

; CY=1 on first call to INT 15h AX=E820h is an error
		jc extmem_e820_4
extmem_e820_1:
		cmp eax,534D4150h	; "SMAP"

; return EAX other than "SMAP" is an error
		stc
		jne extmem_e820_4
		cmp dword [es:di + 16],1 ; type 1 memory (available to OS)
		jne extmem_e820_2
		push bx
			mov ax,[es:di + 0] ; base
			mov dx,[es:di + 2]
			mov bx,[es:di + 8] ; size
			mov cx,[es:di + 10]
			call display_range
		pop bx
extmem_e820_2:
		or ebx,ebx
		je extmem_e820_3

; "In addition the SMAP signature is restored each call, although not
; required by the specification in order to handle some known BIOS bugs."
;	-- http://marc.theaimsgroup.com/?l=linux-kernel&m=99322719013363&w=2
		mov edx,534D4150h	; "SMAP"
		mov ecx,buffer_e820_len
		mov eax,0000E820h
		int 15h

; "the BIOS is permitted to return a nonzero continuation value in EBX
;  and indicate that the end of the list has already been reached by
;  returning with CF set on the next iteration"
;	-- b-15E820 in Ralf Brown's list
		jnc extmem_e820_1
extmem_e820_3:
		clc
extmem_e820_4:
	pop eax
	pop ebx
	pop ecx
	pop edx
	pop di
	pop es
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:			extmem_int15_e801
; action:		gets extended memory size using INT 15h AX=E801h
; in:			(nothing)
; out:			(nothing)
; modifies:		(nothing)
; minimum CPU:		8088 for code, 286+ for extended memory
; notes:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

extmem_int15_e801:
	push dx
	push cx
	push bx
	push ax
		mov ax,0E801h

; "...the INT 15 AX=0xE801 service is called and the results are sanity
; checked. In particular the code zeroes the CX/DX return values in order
; to detect BIOS implementations that do not set the usable memory data.
; It also handles older BIOSes that return AX/BX but not AX/BX data." (?)
;	-- http://marc.theaimsgroup.com/?l=linux-kernel&m=99322719013363&w=2
		xor dx,dx
		xor cx,cx
		int 15h
		jc extmem_e801_2
		mov si,ax
		or si,bx
		jne extmem_e801_1
		mov ax,cx
		mov bx,dx
extmem_e801_1:
		push bx

; convert from Kbytes in AX to bytes in CX:BX
			xor ch,ch
			mov cl,ah
			mov bh,al
			xor bl,bl
			shl bx,1
			rcl cx,1
			shl bx,1
			rcl cx,1

; set range base (in DX:AX) to 1 meg and display it
			mov dx,10h
			xor ax,ax
			call display_range

; convert stacked value from 64K-blocks to bytes in CX:BX
		pop cx
		xor bx,bx

; set range base (in DX:AX) to 16 meg and display it
		mov dx,100h
		xor ax,ax
		call display_range
extmem_e801_2:
	pop ax
	pop bx
	pop cx
	pop dx
	ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:			extmem_int15_88
; action:		gets extended memory size using INT 15h AH=88h
; in:			(nothing)
; out:			(nothing)
; modifies:		(nothing)
; minimum CPU:		8088 for code, 286+ for extended memory
; notes:		HIMEM.SYS will hook this interrupt and make
;			it return 0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

extmem_int15_88:
	push dx
	push cx
	push bx
	push ax
		mov ax,8855h
		int 15h

; "not all BIOSes correctly return the carry flag, making this call
;  unreliable unless one first checks whether it is supported through
;  a mechanism other than calling the function and testing CF"
;	-- b-1588 in Ralf Brown's list
; test if AL register modified by INT 15h AH=88h
		cmp al,55h
		jne extmem_int15_1
		mov ax,88AAh
		int 15h
		cmp al,0AAh
		stc
		je extmem_int15_2

; convert from Kbytes in AX to bytes in CX:BX
extmem_int15_1:
		xor ch,ch
		mov cl,ah
		mov bh,al
		xor bl,bl
		shl bx,1
		rcl cx,1
		shl bx,1
		rcl cx,1

; set base to 1 meg and display range
		mov dx,10h
		xor ax,ax
		call display_range
extmem_int15_2:
	pop ax
	pop bx
	pop cx
	pop dx
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
	push bx
	push ax
		cld			; string operations go up
		mov ah,0Eh		; INT 10h: teletype output
		xor bx,bx		; video page 0
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; name:			wrnum
; action:		writes 32-bit value to text screen
; in:			32-bit unsigned value in DX:AX, radix in BX
; out:			(nothing)
; modifies:		(nothing)
; minimum CPU:		8088
; notes:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	times 40 db 0
num_buf:
	db 0

wrnum:
	push si
	push dx
	push cx
	push bx
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
	pop bx
	pop cx
	pop dx
	pop si
	ret
