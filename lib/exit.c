/*
 *	NMH's Simple C Compiler, 2011,2012
 *	exit()
 */

#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>

extern int	(*_exitfn)();

void exit(int rc) {
	int	i;

	if (_exitfn) _exitfn();
	for (i = 0; i < FOPEN_MAX; i++)
		fclose((FILE *) (i+1));
	_exit(rc);
}
