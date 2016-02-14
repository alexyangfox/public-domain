/*
 *	NMH's Simple C Compiler, 2012
 *	fgetc()
 */

#include <stdio.h>
#include <syscall.h>
#include <errno.h>

int _refill(FILE *f);

int fgetc(FILE *f) {
	int	fi;
	char	c, b[1];

	fi = (int) f-1;
	if ((_file_iom[fi] & _FREAD) == 0)
		return EOF;
	if (_file_iom[fi] & _FERROR)
		return EOF;
	_file_last[fi] = _FREAD;
	if (_file_ch[fi] != EOF) {
		c = _file_ch[fi];
		_file_ch[fi] = EOF;
		return c;
	}
	if ((_file_mode[fi] & _IOACC) == _IONBF)
		if (_read(_file_fd[fi], b, 1) == 1)
			return *b;
		else {
			errno = EIO;
			return EOF;
		}
	if (_file_ptr[fi] >= _file_end[fi])
		if (!_refill(f))
			return EOF;
	return _file_buf[fi][_file_ptr[fi]++];
}
