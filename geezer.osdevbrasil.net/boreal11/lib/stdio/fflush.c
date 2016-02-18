#include <stdio.h> /* FILE, EOF */
#include <os.h> /* write() */
/*****************************************************************************
*****************************************************************************/
int fflush(FILE *stream)
{
	int bytes_to_write, ret_val;

	ret_val = 0;
	if(stream->size != 0)	/* buffered? */
	{
		bytes_to_write = stream->size - stream->room;
		if(bytes_to_write != 0)
		{
			if(write(stream->handle, stream->buf_base,
				bytes_to_write) != bytes_to_write)
					ret_val = EOF;
			stream->buf_ptr = stream->buf_base;
			stream->room = stream->size;
		}
	}
	return ret_val;
}
