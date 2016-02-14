/*
 *	NMH's Simple C Compiler, 2012
 *	fflush()
 */

#include <stdio.h>
#include <syscall.h>
#include <errno.h>

static int _fflush(FILE *f) {
	int	fi, p, e;

	fi = (int) f-1;
	if (_file_iom[fi] & _FERROR) return 0;
	_file_ch[fi] = EOF;
	if (_file_last[fi] != _FWRITE) {
		_file_ptr[fi] = _file_end[fi] = 0;
		return 0;
	}
	if ((_file_mode[fi] & _IOACC) == _IONBF) return 0;
	p = _file_ptr[fi];
	e = _file_end[fi];
	_file_ptr[fi] = _file_end[fi] = 0;
	if (_write(_file_fd[fi], _file_buf[fi] + p, e-p) == e-p)
		return 0;
	errno = EIO;
	return -1;
}

int fflush(FILE *f) {
	int	i, rc = 0;

	if (f != NULL) return _fflush(f);
	for (i = 0; i < FOPEN_MAX; i++)
		if (!_fflush((FILE *) (i+1))) rc = -1;
	return rc;
}
