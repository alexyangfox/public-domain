%include "nasm.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; int setjmp(jmp_buf buf);
;
; Note: this code must agree with the jmp_buf
; structure layout defined in inc/setjmp.h
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

EXP setjmp
	push ebx
		mov ebx,[esp + 8]	; point to jmp_buf

		mov [ebx + 0],edi	; save regs
		mov [ebx + 4],esi
		mov [ebx + 8],ebp

		mov [ebx + 20],edx
		mov [ebx + 24],ecx
		mov [ebx + 28],eax

; use stacked EBX value, not current EBX
		mov eax,[esp]
		mov [ebx + 16],eax

; use ESP value after we RET from this function, not current ESP
		lea eax,[esp + 8]
		mov [ebx + 12],eax

; use return EIP value, not current EIP
		mov eax,[esp + 4]
		mov [ebx + 32],eax

; none of the PUSH or MOV instructions changed EFLAGS!
		pushf
		pop dword [ebx + 36]
	pop ebx
	xor eax,eax
	ret
