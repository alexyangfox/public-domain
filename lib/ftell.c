/*
 *	NMH's Simple C Compiler, 2012
 *	ftell()
 */

#include <stdio.h>
#include <syscall.h>
#include <errno.h>

int ftell(FILE *f) {
	int	fi;
	int	adjust = 0, pos;

	fi = (int) f-1;
	if ((_file_mode[fi] & _IOACC) != _IONBF) {
		adjust = _file_end[fi] - _file_ptr[fi];
		if (_FREAD == _file_last[fi]) {
			adjust = -adjust;
			if (_file_ch[fi] != EOF) adjust--;
		}
	}
	if ((pos = _lseek(_file_fd[fi], 0, SEEK_CUR)) < 0) {
		errno = EIO;
		return -1;
	}
	return pos + adjust;
}
