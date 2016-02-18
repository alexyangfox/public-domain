#include <stdio.h>
#include <os.h> /* write() */
/*****************************************************************************
*****************************************************************************/
int fputc(int c, FILE *stream)
{
	char one_char;
	int ret_val;

	ret_val = c;
	if(stream->flags == _IONBF || stream->size == 0) /* unbuffered */
	{
		one_char = (char)c;
		if(write(stream->handle, &one_char, 1) != 1)
			ret_val = EOF;
	}
	else				/* buffered */
	{
		if(stream->room == 0)
		{
			if(fflush(stream) != 0)
				return EOF;
		}
		*stream->buf_ptr = (char)c;
		stream->buf_ptr++;
		stream->room--;
		if((stream->flags & _IOLBF) && (c == '\n'))
		{
			if(fflush(stream) != 0)
				ret_val = EOF;
		}
	}
	return ret_val;
}
