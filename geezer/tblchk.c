/*----------------------------------------------------------------------------
Displays maximum depth of HTML table nesting
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: May 24, 2005
This code is public domain (no copyright).
You can do whatever you want with it.
----------------------------------------------------------------------------*/
#include <string.h> /* memcpy(), memicmp() */
/* EOF, stdin, FILE, fileno(), printf(), fopen(), fgetc(), fclose() */
#include <stdio.h>
#if defined(__TURBOC__)
#include <io.h> /* isatty() */
#endif

#define	BUF_LEN	10
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	enum
	{
		NORMAL, IN_TAG, AWAITING_CLOSE
	} state = NORMAL;
	int i, table_depth = 0, max_table_depth = 0, open = 0, close = 0;
	char buf[BUF_LEN];
	FILE *in;

	if(arg_c > 2)
USAGE:	{
		printf("Checks depth of TABLE nesting in HTML files\n");
		return 1;
	}
	else if(arg_c == 2)
	{
		in = fopen(arg_v[1], "rb");
		if(in == NULL)
		{
			printf("Can't open input file '%s'\n", arg_v[1]);
			return 1;
		}
	}
	else
	{
		if(isatty(fileno(stdin)))
			goto USAGE;
		in = stdin;
	}
	while(1)
	{
/* ecch... */
		memcpy(buf + 0, buf + 1, BUF_LEN - 1);
		i = fgetc(in);
		if(i == EOF)
			break;
		buf[BUF_LEN - 1] = i;
		switch(state)
		{
		case NORMAL:
			if(i == '<')
				state++;
			break;
		case IN_TAG:
/* should also check for whitespace after tags? */
			if(!memicmp(buf + BUF_LEN - 6, "<table", 6))
			{
				open++;
				state++;
				table_depth++;
				if(table_depth > max_table_depth)
					max_table_depth = table_depth;
			}
			else if(!memicmp(buf + BUF_LEN - 7, "</table", 7))
			{
				close++;
				state++;
				table_depth--;
			}
			break;
		case AWAITING_CLOSE:
			if(i == '>')
				state = NORMAL;
			break;
		default:
			printf("WTF?\n");
			break;
		}
	}
	fclose(in);
	printf("Saw <TABLE> tag %u times, and </TABLE> tag %u times\n",
		open, close);
	printf("Maximum table depth = %u\n", max_table_depth);
	return 0;
}
