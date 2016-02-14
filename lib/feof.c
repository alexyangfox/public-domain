/*
 *	NMH's Simple C Compiler, 2012
 *	feof()
 */

#include <stdio.h>

int feof(FILE *f) {
	return (_file_iom[(int) f-1] & _FERROR) != 0;
}
