/*****************************************************************************
Turns off transparency in .GIF files
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
This code is public domain (no copyright).
You can do whatever you want with it.

Nov 25, 2004 release:
- Modified to work with animated GIFs
- Now testing fwrite() and fputc() for write errors
- Using setjmp() and longjmp() for error-trapping to reduce line count
  in main routine
*****************************************************************************/
#include <setjmp.h>
#include <string.h>
#include <stdio.h>

#define	ERR_READ	-2
#define	ERR_WRITE	-3
#define	ERR_BAD_FILE	-4

typedef struct
{
	FILE *in;
	FILE *out;
	jmp_buf oops;
} state_t;
/*****************************************************************************
*****************************************************************************/
static unsigned getc_or_die(state_t *state)
{
	int rv;

	rv = fgetc(state->in);
	if(rv == EOF)
		longjmp(state->oops, ERR_READ);
	return rv;
}
/*****************************************************************************
*****************************************************************************/
static void putc_or_die(state_t *state, unsigned c)
{
	if(fputc(c, state->out) == EOF)
		longjmp(state->oops, ERR_WRITE);
}
/*****************************************************************************
*****************************************************************************/
static void read_or_die(state_t *state, unsigned char *buf, unsigned count)
{
	if(fread(buf, 1, count, state->in) != count)
		longjmp(state->oops, ERR_READ);
}
/*****************************************************************************
*****************************************************************************/
static void write_or_die(state_t *state, unsigned char *buf, unsigned count)
{
	if(fwrite(buf, 1, count, state->out) != count)
		longjmp(state->oops, ERR_WRITE);
}
/*****************************************************************************
*****************************************************************************/
static int untran(const char *in_name)
{
	char out_name[L_tmpnam], gce, success = 0;
	unsigned char buf[256];
	state_t state;
	int i;

/* open input file */
	state.in = fopen(in_name, "rb");
	if(state.in == NULL)
	{
		printf("Error: Can't open file '%s'\n", in_name);
		return -1;
	}
/* read header (screen descriptor) */
	if(fread(buf, 1, 13, state.in) != 13)
NOT:	{
		printf("Error: File '%s' is not a .GIF file\n", in_name);
		fclose(state.in);
		return -1;
	}
/* make sure it's a GIF file */
	if(memcmp(buf, "GIF8", 4))
		goto NOT;
/* open output file */
	tmpnam(out_name);
	state.out = fopen(out_name, "wb");
	if(state.out == NULL)
	{
		printf("Error: Can't open temporary output file '%s'\n",
			out_name);
		fclose(state.in);
		return -1;
	}
/* set up error trapping */
	i = setjmp(state.oops);
	if(i != 0)
	{
		if(i == ERR_READ)
			printf("Error: unexpected end of input file '%s'\n",
				in_name);
		else if(i == ERR_WRITE)
			printf("Error writing temporary output file '%s' "
				"(disk full?)\n", out_name);
		else if(i == ERR_BAD_FILE)
			printf("Error: file '%s' is an invalid GIF file\n",
				in_name);
		else
			printf("Unknown software error %d\n", i);
		fclose(state.out);
		remove(out_name);
		fclose(state.in);
		return -1;
	}
/* write header (screen descriptor) */
	write_or_die(&state, buf, 13);
/* copy global palette (color map), if any */
	if(buf[10] & 0x80)
	{
		i = (buf[10] & 7) + 1;
		i = 1 << i;
		for(; i != 0; i--)
		{
			read_or_die(&state, buf, 3);
			write_or_die(&state, buf, 3);
		}
	}
/* main loop */
	while(1)
	{
		i = getc_or_die(&state);
		putc_or_die(&state, i);
/* ';' == end of GIF file */
		if(i == ';')
			break;
/* ',' == image descriptor */
		else if(i == ',')
		{
/* copy image descriptor */
			read_or_die(&state, buf, 9);
			write_or_die(&state, buf, 9);
/* copy local palette, if any */
			if(buf[8] & 0x80)
			{
				i = (buf[8] & 7) + 1;
				i = 1 << i;
				for(; i != 0; i--)
				{
					read_or_die(&state, buf, 3);
					write_or_die(&state, buf, 3);
				}
			}
/* copy initial LZW code size */
			i = getc_or_die(&state);
			putc_or_die(&state, i);
/* copy sub-blocks of LZW-compressed data */
			while(1)
			{
				i = getc_or_die(&state); /* block length */
				putc_or_die(&state, i);
				if(i == 0)
					break;
				read_or_die(&state, buf, i);
				write_or_die(&state, buf, i);
			}
		}
/* '!' == extension block */
		else if(i == '!')
		{
			i = getc_or_die(&state); /* block type */
			putc_or_die(&state, i);
			gce = (i == 0xF9); /* Graphic Control Extension */
/* copy sub-blocks */
			while(1)
			{
				i = getc_or_die(&state); /* block length */
				putc_or_die(&state, i);
				if(i == 0)
					break;
				read_or_die(&state, buf, i);
				if(gce && i == 4)
				{
					buf[0] &= ~0x01;
					success = 1;
				}
				write_or_die(&state, buf, i);
			}
		}
		else
			longjmp(state.oops, ERR_BAD_FILE);
	}
	fclose(state.in);
	fclose(state.out);
	if(!success)
	{
		printf("Warning: no GCE (transparency control) blocks "
			"in .GIF file '%s'\nFile will not be changed\n",
			in_name);
		remove(out_name);
		return 0;
	}
/* replace input file with changed file */
	if(remove(in_name))
	{
		printf("Error: can't overwrite input file '%s'\n", in_name);
		remove(out_name);
		return -1;
	}
	if(rename(out_name, in_name))
	{
		printf("Error: can't rename temporary file '%s' to '%s'\n",
			out_name, in_name);
		return -1;
	}
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	unsigned i;

	if(arg_c < 2)
	{
		printf("Turns off transparency in .GIF files\n");
		return 1;
	}
	for(i = 1; i < arg_c; i++)
		untran(arg_v[i]);
	return 0;
}
