.ifndef __TL_AS_INC
.set	__TL_AS_INC, 1

/* DOS and Windows compilers use the leading underscore, Linux/ELF does not */

.ifdef UNDERBARS
.macro EXP sym
	.global \sym
	\sym:
	.global _\sym
	_\sym:
.endm

.macro IMP sym
	.extern _\sym
	.equ \sym,_\sym
.endm

.else
.macro EXP sym
	.global \sym
	\sym:
.endm

.macro IMP sym
	.extern \sym
.endm
.endif

.section .text

.endif
