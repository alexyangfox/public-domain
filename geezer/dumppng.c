/*----------------------------------------------------------------------------
.PNG file dumper
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: July 19, 2007
This code is public domain (no copyright).
You can do whatever you want with it.
----------------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
/*****************************************************************************
CAUTION: the 'u' after '0x100' is significant
*****************************************************************************/
static unsigned long read_be32(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] * 0x1000000L + buf[1] * 0x10000L +
		buf[2] * 0x100u + buf[3];
}
/*****************************************************************************
*****************************************************************************/
static int load_ihdr(FILE *in)
{
	static const char *ctype[] =
	{
		"grayscale", "UNKNOWN", "RGB", "palette color"
	};
/**/
	unsigned depth, color_type, interlace;
	unsigned char buf[21];
	unsigned long wd, ht;

	if(fread(buf, 1, 21, in) != 21)
		return -1;
	wd = read_be32(&buf[0]);
	ht = read_be32(&buf[4]);
	depth = buf[8];
	color_type = buf[9];
/* sanity checking: color type & depth */
	switch(color_type)
	{
	case 0:
		if(depth != 1 && depth != 2 && depth != 4 &&
			depth != 8 && depth != 16)
BAD:		{
			printf("depth %u is invalid for color type %u\n",
				depth, color_type);
			return +1;
		}
		break;
	case 3:
		if(depth != 1 && depth != 2 && depth != 4 && depth != 8)
			goto BAD;
		break;
	case 2:
	case 4:
	case 6:
		if(depth != 8 && depth != 16)
			goto BAD;
		break;
	default:
		printf("invalid color type %u\n", color_type);
		return +1;
	}
/* compression scheme (0=zlib) */
	if(buf[10] > 0)
	{
		printf("compression method %u not supported)\n", buf[10]);
		return +1;
	}
/* filter scheme (0=default) */
	if(buf[11] > 0)
	{
		printf("filter scheme %u not supported\n", buf[11]);
		return +1;
	}
/* interlace (0=none, 1=Adam-7) */
	interlace = buf[12];
	if(interlace > 1)
	{
		printf("interlace scheme %u not supported\n", interlace);
		return +1;
	}
	printf("Image:%lux%lux%u", wd, ht, depth);
	printf(", color type:%s", ctype[color_type & 3]);
	printf(", alpha transparency:%s", (color_type & 4) ? "yes" : "no");
	printf(", interlace:%s\n", interlace ? "yes (Adam7)" : "no");
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	unsigned long len, pos;
	unsigned char buf[8];
	FILE *in;
	int i;

/* must have exactly one arg on command line */
	if(arg_c != 2)
	{
		printf(".PNG file dumper\n");
		return 1;
	}
/* open input file */
	in = fopen(arg_v[1], "rb");
	if(in == NULL)
	{
		printf("Error: can't open input file '%s'\n", arg_v[1]);
		return 2;
	}
/* read file header and validate */
	if(fread(buf, 1, 8, in) != 8)
NOT:	{
		printf("Error: file '%s' is not a .PNG file\n", arg_v[1]);
		fclose(in);
		return 3;
	}
	if(memcmp(&buf[0], "\x89PNG\x0D\x0A\x1A\x0A", 8))
		goto NOT;
	printf(	"File   Chunk Chunk\n"
		"offset ID    length\n"
		"------ ----- ------\n");
	do
	{
		pos = ftell(in);
/* load chunk length and ID */
		if(fread(buf, 1, 8, in) != 8)
ERR:		{
			printf("Error reading PNG file '%s'\n", arg_v[1]);
			fclose(in);
			return 5;
		}
		len = read_be32(&buf[0]);
		printf("%6lu %-4.4s  %6lu\n", pos, &buf[4], len);
		if(!memcmp(&buf[4], "IHDR", 4))
		{
			i = load_ihdr(in);
			if(i < 0)
				goto ERR;
			else if(i > 0)
			{
				printf("Error in .PNG file '%s'\n", arg_v[1]);
				fclose(in);
				return 4;
			}
		}
/* skip chunk and 4-byte CRC at end of chunk */
		fseek(in, pos + 8 + len + 4, SEEK_SET);
	} while(memcmp(&buf[4], "IEND", 4));
	fclose(in);
	return 0;
}
