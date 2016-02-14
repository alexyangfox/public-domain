/*
 *	NMH's Simple C Compiler, 2011,2012
 *	C runtime initialization
 */

#include <stdio.h>
#include <errno.h>

int	errno = EOK;

int	 _file_fd  [FOPEN_MAX];
char	 _file_iom [FOPEN_MAX];
char	 _file_last[FOPEN_MAX];
char	 _file_mode[FOPEN_MAX];
int	 _file_ptr [FOPEN_MAX];
int	 _file_end [FOPEN_MAX];
int	 _file_size[FOPEN_MAX];
int	 _file_ch  [FOPEN_MAX];
char	*_file_buf [FOPEN_MAX];

FILE	*stdin, *stdout, *stderr;

void _init(void) {
	int	i;

	for (i = 0; i < FOPEN_MAX; i++)
		_file_iom[i] = _FCLOSED;
	stdin = fdopen(0, "r");
	stdout = fdopen(1, "w");
	stderr = fdopen(2, "w");
	_file_mode[1] = _IOLBF;
	_file_mode[2] = _IOLBF;
}
