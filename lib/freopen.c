/*
 *	NMH's Simple C Compiler, 2012
 *	freopen()
 */

#include <stdio.h>
#include <syscall.h>
#include <errno.h>

int _openfd(char *path, char *mode);
int _openmode(char *mode);

FILE *freopen(char *path, char *mode, FILE *f) {
	int	fd, fi, om;

	if ((om = _openmode(mode)) == _FCLOSED) {
		errno = EINVAL;
		return NULL;
	}
	fi = (int) f-1;
	if (_file_iom[fi] & (_FREAD|_FWRITE) == _FREAD|_FWRITE)
		;
	else if (_file_iom[fi] & _FREAD && *mode != 'r') {
		fclose(f);
		return NULL;
	}
	else if (_file_iom[fi] & _FWRITE && *mode != 'w' && *mode != 'a') {
		fclose(f);
		return NULL;
	}
	if (path) {
		fd = _openfd(path, mode);
		if (fd < 0) {
			fclose(f);
			return NULL;
		}
		_close(_file_fd[fi]);
		_file_fd[fi] = fd;
	}
	_file_iom[fi]  = om;
	_file_last[fi] = _FCLOSED;
	_file_ptr[fi]  = 0;
	_file_end[fi]  = 0;
	_file_ch[fi]   = EOF;
	return f;
}
