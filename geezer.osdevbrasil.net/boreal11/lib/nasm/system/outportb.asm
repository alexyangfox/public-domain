%include "nasm.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; void outportb(unsigned port, unsigned val);
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

EXP outportb
	push edx
		mov edx,[esp + 8]
		mov eax,[esp + 12]
		out dx,al
	pop edx
	ret
