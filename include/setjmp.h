/*
 *	NMH's Simple C Compiler, 2012
 *	setjmp.h
 */

/* should be typedef int jmp_buf[2]; */

#define _JMP_BUFSIZ	2

void	longjmp(int *env, int v);
int	setjmp(int *env);
