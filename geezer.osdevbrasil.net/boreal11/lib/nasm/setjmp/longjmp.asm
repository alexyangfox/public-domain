%include "nasm.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; void longjmp(jmp_buf buf, int ret_val);
;
; *** WARNING: this code must agree with the jmp_buf
; structure layout defined in inc/setjmp.h
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

EXP longjmp
	mov ebx,[esp + 4]	; point to jmp_buf

	mov eax,[esp + 8]	; make sure return value != 0
	or eax,eax
	jne .1
	mov al,1
.1:
	mov [ebx + 28],eax	; store return value (EAX) in jmp_buf
	mov edi,[ebx + 12]	; point to new stack

	mov eax,[ebx + 36]	; push EFLAGS
	sub edi,4
	mov [edi],eax

	sub edi,4		; push current CS
	mov [edi],cs

	mov eax,[ebx + 32]	; push EIP
	sub edi,4
	mov [edi],eax

	mov [ebx + 12],edi	; update stored ESP
	mov esp,ebx		; make the jmp_buf itself our new stack
	popa			; pop 7  general purpoase registers (not ESP)
	mov esp,[esp - 20]	; NOW pop ESP
	iret			; pop EIP, CS, FLAGS
