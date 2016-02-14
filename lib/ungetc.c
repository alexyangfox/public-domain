/*
 *	NMH's Simple C Compiler, 2012
 *	ungetc()
 */

#include <stdio.h>
#include <syscall.h>

int ungetc(int c, FILE *f) {
	if (_file_ch[(int) f-1] != EOF) return EOF;
	_file_iom[(int) f-1] &= ~_FERROR;
	return _file_ch[(int) f-1] = c;
}
