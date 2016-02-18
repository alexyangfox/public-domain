#include <stdio.h> /* BUFSIZ, FILE, NULL, _IOLBF */

static char g_stdout_buffer[BUFSIZ];

FILE g_std_streams[] =
{
	{	/* buf_base	buf_ptr		size	room	flags	handle */
/* stdin */	NULL,		NULL,		0,	0,	0,	0
	},
	{
/* stdout */	g_stdout_buffer, g_stdout_buffer, BUFSIZ, BUFSIZ, _IOLBF, 1
	},
	{
/* stderr */	NULL,		NULL,		0,	0,	0,	2
	}
};
