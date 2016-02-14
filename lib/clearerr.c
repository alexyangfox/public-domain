/*
 *	NMH's Simple C Compiler, 2012
 *	clearerr()
 */

#include <stdio.h>

void clearerr(FILE *f) {
	_file_iom[(int) f-1] &= ~_FERROR;
}
