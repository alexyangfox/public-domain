/*
 *	NMH's Simple C Compiler, 2012
 *	setvbuf()
 */

#include <stdio.h>
#include <stdlib.h>

static void freebuf(int fi) {
	if (_file_buf[fi] != NULL && (_file_mode[fi] & _IOUSR) == 0)
		free(_file_buf[fi]);
}

int setvbuf(FILE *f, char *buf, int mode, int size) {
	int	fi;

	fi = (int) f-1;
	if (0 == size) size = BUFSIZ;
	if (buf) {
		freebuf(fi);
		_file_mode[fi] = mode | _IOUSR;
	}
	else {
		if (_IONBF != mode)
			if ((buf = malloc(size)) == NULL)
				return EOF;
		freebuf(fi);
		_file_mode[fi] = mode;
	}
	_file_buf[fi] = buf;
	_file_size[fi] = size;
	return 0;
}
