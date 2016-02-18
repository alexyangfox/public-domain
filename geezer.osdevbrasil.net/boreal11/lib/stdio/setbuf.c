#include <stdio.h> /* FILE, fflush() */
/*****************************************************************************
*****************************************************************************/
void setbuf(FILE *stream, char *fmt)
{
	fflush(stream); /* ??? */
	stream->buf_base = stream->buf_ptr = fmt;
	if(fmt != NULL)
	{
		stream->size = stream->room=BUFSIZ;
		stream->flags = _IOFBF;
	}
	else
	{
		stream->size = stream->room = 0;
		stream->flags = _IONBF;
	}
}
