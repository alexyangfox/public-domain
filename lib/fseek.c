/*
 *	NMH's Simple C Compiler, 2012
 *	fseek()
 */

#include <stdio.h>
#include <syscall.h>
#include <errno.h>

int fseek(FILE *f, int pos, int how) {
	int	fi;
	int	adjust = 0;
	char	b[101];

	fi = (int) f-1;
	if (how != SEEK_SET && how != SEEK_CUR && how != SEEK_END) {
		errno = EINVAL;
		return -1;
	}
	if (SEEK_CUR == how && (_file_mode[fi] & _IOACC) != _IONBF) {
		adjust = _file_end[fi] - _file_ptr[fi];
		if (_FREAD == _file_last[fi]) {
			adjust = -adjust;
			if (_file_ch[fi] != EOF) adjust--;
		}
	}
	if (fflush(f) < 0) return -1;
	_file_ch[fi] = EOF;
	if ((pos = _lseek(_file_fd[fi], pos + adjust, how)) < 0) {
		errno = EIO;
		return -1;
	}
	return pos;
}
