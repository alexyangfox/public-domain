/*
 *	NMH's Simple C Compiler, 2011,2012
 *	fputc()
 */

#include <stdio.h>
#include <syscall.h>
#include <errno.h>

int _fsync(FILE *f);

int fputc(int c, FILE *f) {
	int	fi;
	char	b[1];

	fi = (int) f-1;
	if ((_file_iom[fi] & _FWRITE) == 0)
		return EOF;
	if (_file_iom[fi] & _FERROR)
		return EOF;
	_file_last[fi] = _FWRITE;
	if ((_file_mode[fi] & _IOACC) == _IONBF) {
		*b = c;
		if (_write(_file_fd[fi], b, 1) == 1)
			return c;
		else {
			errno = EIO;
			return EOF;
		}
	}
	if (_file_end[fi] >= _file_size[fi])
		if (!_fsync(f))
			return EOF;
	_file_buf[fi][_file_end[fi]++] = c;
	if ((_file_mode[fi] & _IOACC) == _IOLBF)
		if (!_fsync(f))
			return EOF;
	return c;
}
