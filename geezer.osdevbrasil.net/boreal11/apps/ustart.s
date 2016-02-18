/*----------------------------------------------------------------------------
ASM STARTUP CODE FOR TASKS
----------------------------------------------------------------------------*/

.include "as.inc"

/* IMPORTS
from C code */
IMP main

/* from C library */
IMP exit
IMP open
/* xxx - these are defined in OS.H */
.equ	O_RDONLY, 0x01
.equ	O_WRONLY, 0x02

EXP uentry
/* open fd 0=stdin:
if(open("/dev/con", O_RDONLY) < 0) error; */
	pushl $O_RDONLY
	pushl $con_dev_name
		call open
	add $8,%esp
	or %eax,%eax
	js error

/* open fd 1=stdout:
if(open("/dev/con", O_WRONLY) < 0) error; */
	pushl $O_WRONLY
	pushl $con_dev_name
		call open
	add $8,%esp
	or %eax,%eax
	js error

/* call application main() */
	call main
	jmp done
error:
	mov $1,%eax
done:
	push %eax
		call exit

/* exit() should not return -- try to generate illegal exception if it does */
		ud2b	/* db 0Fh, 0B9h */

/* if that doesn't work, just go into a loop */
		jmp .

con_dev_name:
	.asciz "/dev/con"
