/*
 *	NMH's Simple C Compiler, 2012
 *	ferror()
 */

#include <stdio.h>

int ferror(FILE *f) {
	return (_file_iom[(int) f-1] & _FERROR) != 0;
}
