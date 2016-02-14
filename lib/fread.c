/*
 *	NMH's Simple C Compiler, 2012
 *	fread()
 */

#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <errno.h>

int _refill(FILE *f) {
	int	fi, k;

	fi = (int) f-1;
	if (_file_ptr[fi] >= _file_end[fi]) {
		_file_ptr[fi] = 0;
		k = _read(_file_fd[fi], _file_buf[fi], _file_size[fi]);
		if (k <= 0) {
			_file_end[fi] = 0;
			_file_iom[fi] |= _FERROR;
			if (k < 0) errno = EIO;
			return 0;
		}
		_file_end[fi] = k;
	}
	return 1;
}

int _fread(void *p, int size, FILE *f) {
	int	fi, k, len, end, ptr, total;

	fi = (int) f-1;
	if ((_file_iom[fi] & _FREAD) == 0) return 0;
	if (_file_iom[fi] & _FERROR) return 0;
	_file_last[fi] = _FREAD;
	total = size;
	if (_file_ch[fi] != EOF) {
		*(char *)p++ = _file_ch[fi];
		_file_ch[fi] = EOF;
		size--;
	}
	if ((_file_mode[fi] & _IOACC) == _IONBF) {
		if ((total = _read(_file_fd[fi], p, size)) != size) {
			_file_iom[fi] |= _FERROR;
			errno = EIO;
		}
		return total;
	}
	if (!_refill(f)) return 0;
	end = _file_end[fi];
	ptr = _file_ptr[fi];
	k = end - ptr;
	if (size < k) k = size;
	memcpy(p, _file_buf[fi] + ptr, k);
	_file_ptr[fi] += k;
	p += k;
	size -= k;
	len = _file_size[fi];
	while (size > len) {
		if (_read(_file_fd[fi], p, len) != len) {
			_file_iom[fi] |= _FERROR;
			errno = EIO;
			return total-size;
		}
		p += len;
		size -= len;
	}
	if (size != 0) {
		if (!_refill(f)) return total-size;
		memcpy(p, _file_buf[fi], size);
		_file_ptr[fi] = size;
	}
	return total;
}

int fread(void *p, int size, int count, FILE *f) {
	int	k, fi;

	if ((k = _fread(p, size * count, f)) < 0)
		return -1;
	return k / size;
}
