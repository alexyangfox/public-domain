;-----------------------------------------------------------------------------
; END-OF-KERNEL MARKER
;
; EXPORTS
; extern char g_end[];
;
; Note: the assembled object file produced from this file must be linked LAST
;-----------------------------------------------------------------------------

%include "nasm.inc"

SEGMENT BSS
EXP g_end
	db 0