/*----------------------------------------------------------------------------
ZERO/NULL CHARACTER DEVICE

EXPORTS:
int open_zero(file_t *f);
----------------------------------------------------------------------------*/
#include <string.h> /* memset() */
#include "_krnl.h"
/*****************************************************************************
*****************************************************************************/
static int close_zero(file_t *f_UNUSED)
{
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int write_zero(file_t *f_UNUSED,
		unsigned char *buf_UNUSED, unsigned len)
{
	return len;
}
/*****************************************************************************
*****************************************************************************/
static int read_zero(file_t *f_UNUSED, unsigned char *buf, unsigned len)
{
	memset(buf, 0, len);
	return len;
}
/*****************************************************************************
*****************************************************************************/
static int select_zero(file_t *f_UNUSED, unsigned access_UNUSED,
		unsigned *timeout_UNUSED)
{
	return 3; /* this device is always ready to read and write */
}
/*****************************************************************************
*****************************************************************************/
int open_zero(file_t *f)
{
	static const ops_t ops =
	{
		close_zero, write_zero, read_zero, select_zero
	};
/**/

	f->ops = &ops;
	return 0;
}
