/*
 *	NMH's Simple C Compiler, 2012
 *	fclose()
 */

#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

int fclose(FILE *f) {
	int	fi;

	if (NULL == f) return EOF;
	fi = (int) f-1;
	if (_FCLOSED == _file_iom[fi]) return EOF;
	if (_FWRITE == _file_last[fi] && fflush(f))
		return EOF;
	_close(_file_fd[fi]);
	if (_file_buf[fi] != NULL && (_file_mode[fi] & _IOUSR) == 0)
		free(_file_buf[fi]);
	_file_iom[fi] = _FCLOSED;
	return 0;
}
