/*
 *	NMH's Simple C Compiler, 2011,2012
 *	fdopen()
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

int _openmode(char *mode) {
	int	n;

	if (strchr(mode, '+')) return _FREAD | _FWRITE;
	else if ('r' == *mode) return _FREAD;
	else if ('w' == *mode) return _FWRITE;
	else if ('a' == *mode) return _FWRITE;
	return _FCLOSED;
}

FILE *fdopen(int fd, char *mode) {
	int	i;

	for (i = 0; i < FOPEN_MAX; i++)
		if (_FCLOSED == _file_iom[i])
			break;
	if (i >= FOPEN_MAX) {
		errno = ENFILE;
		return NULL;
	}
	_file_buf[i]  = NULL;
	_file_fd[i]   = fd;
	_file_iom[i]  = _openmode(mode);
	_file_last[i] = _FCLOSED;
	_file_mode[i] = _IOFBF;
	_file_ptr[i]  = 0;
	_file_end[i]  = 0;
	_file_size[i] = 0;
	_file_ch[i]   = EOF;
	if (_FCLOSED == _file_iom[i]) {
		fclose((FILE *) i+1);
		errno = EINVAL;
		return NULL;
	}
	if (setvbuf((FILE *) (i+1), NULL, _IOFBF, 0) == EOF) {
		fclose((FILE *) i+1);
		return NULL;
	}
	return (FILE *) (i+1);
}
