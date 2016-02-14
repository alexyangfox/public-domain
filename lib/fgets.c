/*
 *	NMH's Simple C Compiler, 2012
 *	fgets()
 */

#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <errno.h>

int _refill(FILE *f);

static char *fgets_raw(char *s, int len, int fi) {
	char	*p;

	p = s;
	while (len-- > 1) {
		if (_read(_file_fd[fi], p, 1) != 1) {
			errno = EIO;
			return NULL;
		}
		if ('\n' == *p++) break;
	}
	*p = 0;
	return s;
}

char *fgets(char *s, int len, FILE *f) {
	int	fi, *pptr, *pend, k;
	char	*p, *buf, *pn;

	fi = (int) f-1;
	if ((_file_iom[fi] & _FREAD) == 0) return NULL;
	if (_file_iom[fi] & _FERROR) return NULL;
	_file_last[fi] = _FREAD;
	p = s;
	if (_file_ch[fi] != EOF) {
		*p++ = _file_ch[fi];
		_file_ch[fi] = EOF;
		len--;
	}
	if ((_file_mode[fi] & _IOACC) == _IONBF)
		return fgets_raw(s, len, fi);
	pptr = _file_ptr + fi;
	pend = _file_end + fi;
	buf = _file_buf[fi];
	pn = NULL;
	while (len > 1 && pn == NULL) {
		if (!_refill(f))
			return NULL;
		if ((pn = memchr(buf + *pptr, '\n', *pend - *pptr)) != NULL)
			k = pn - buf - *pptr + 1;
		else
			k = *pend - *pptr;
		if (len-1 < k) k = len-1;
		memcpy(p, buf + *pptr, k);
		*pptr += k;
		p += k;
		len -= k;
	}
	*p = 0;
	return s;
}
