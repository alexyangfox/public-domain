/*
 *	NMH's Simple C Compiler, 2011,2012
 *	fwrite()
 */

#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <errno.h>

int _fsync(FILE *f) {
	int	fi, ptr, end, k, pn, p;
	char	*buf;

	fi = (int) f-1;
	if (_file_end[fi] >= _file_size[fi]) {
		ptr = _file_ptr[fi];
		end = _file_end[fi];
		_file_end[fi] = _file_ptr[fi] = 0;
		k = _write(_file_fd[fi], _file_buf[fi] + ptr, end-ptr);
		if (k != end-ptr) {
			_file_iom[fi] |= _FERROR;
			errno = EIO;
			return 0;
		}
	}
	else if ((_file_mode[fi] & _IOACC) == _IOLBF) {
		ptr = _file_ptr[fi];
		end = _file_end[fi];
		buf = _file_buf[fi];
		pn = -1;
		for (p = ptr; p < end; p++)
			if ('\n' == buf[p]) pn = p+1;
		if (pn >= 0) {
			k = _write(_file_fd[fi], _file_buf[fi] + ptr, pn-ptr);
			_file_ptr[fi] = pn;
			if (k != pn - ptr) {
				_file_iom[fi] |= _FERROR;
				errno = EIO;
				return 0;
			}
		}
	}
	return 1;
}

int _fwrite(void *p, int size, FILE *f) {
	int	fi, k, len, total;

	fi = (int) f-1;
	if ((_file_iom[fi] & _FWRITE) == 0) return 0;
	if (_file_iom[fi] & _FERROR) return 0;
	_file_last[fi] = _FWRITE;
	if ((_file_mode[fi] & _IOACC) == _IONBF) {
		if ((k = _write(_file_fd[fi], p, size)) != size) {
			_file_iom[fi] |= _FERROR;
			errno = EIO;
		}
		return k;
	}
	total = size;
	len = _file_size[fi];
	k = len - _file_end[fi];
	if (size < k) k = size;
	memcpy(_file_buf[fi] + _file_end[fi], p, k);
	_file_end[fi] += k;
	size -= k;
	p += k;
	if (!_fsync(f)) return 0;
	while (size > len) {
		if ((k = _write(_file_fd[fi], p, len)) != len) {
			_file_iom[fi] |= _FERROR;
			errno = EIO;
			return total - size;
		}
		p += len;
		size -= len;
	}
	if (size != 0) {
		memcpy(_file_buf[fi], p, size);
		_file_end[fi] = size;
	}
	if ((_file_mode[fi] & _IOACC) == _IOLBF && !_fsync(f))
		return total-size;
	return total;
}

int fwrite(void *p, int size, int count, FILE *f) {
	int	k, fi;

	if ((k = _fwrite(p, size * count, f)) < 0)
		return -1;
	return k / size;
}
