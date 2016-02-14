/*
 *	NMH's Simple C Compiler, 2012
 *	fileno()
 */

#include <stdio.h>

int fileno(FILE *f) {
	return _file_fd[(int) f-1];
}
