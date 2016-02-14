/*****************************************************************************
Adds comment block (e.g. author info) to a GIF89 file
Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: June 27, 2003
This code is public domain (no copyright).
You can do whatever you want with it.

The comment is subject to command-line processing by the host OS,
so it may not look the way you want. Under DOS:
- Spaces in the comment will be compressed
- Characters reserved by the DOS shell are not allowed in the
  comment:  <  >  |  "  /  \  :  *  ?  etc.
- Command line is limited to about 128 characters in length

This code makes only one sub-block in the GIF comment extension block,
so the comment can be no longer than 255 characters.
*****************************************************************************/
#include <stdlib.h> /* NULL, malloc(), free() */
#include <string.h> /* strlen(), strcat(), strncmp(), memcpy() */
/* FILE, SEEK_END, fopen(), fseek(), fread(), fgetc(), fwrite(), fclose() */
#include <stdio.h> /* printf(), sprintf() */
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	char buf[4], *comment;
	unsigned len;
	FILE *f;
	int i;

/* check args */
	if(arg_c < 3)
	{
		printf("Adds comment block to GIF89 file\n"
			"Usage: gifsig file.gif comment\n");
		return 1;
	}
/* determine length of comment */
	len = 0;
	for(i = 2; i < arg_c; i++)
		len += strlen(arg_v[i]);
	len += (arg_c - 3); /* 1 space between each arg */
	if(len > 255)
	{
		printf("Comment is too long (>255 characters)\n");
		return 1;
	}
/* allocate memory for comment */
	comment = malloc(len + 6); /* "!\xFE%c"  "\x00;\x00" */
	if(comment == NULL)
	{
		printf("Out of memory\n");
		return 2;
	}
/* build comment block */
	sprintf(comment, "!\xFE%c", len);
	for(i = 2; i < arg_c; i++)
	{
		strcat(comment, arg_v[i]);
		if(i + 1 < arg_c)
			strcat(comment, " ");
	}
/* can't use strcat() to insert a zero byte, so... */
	len += 3;
	memcpy(comment + len, "\x00;\x00", 3);
	len += 3;
/* open file */
	f = fopen(arg_v[1], "r+b");
	if(f == NULL)
	{
		printf("Can't open file '%s'\n", arg_v[1]);
		free(comment);
		return 3;
	}
/* validate */
	if(fread(buf, 1, 4, f) != 4 || /* too short */
		strncmp(buf, "GIF8", 4))
NOT:	{
		printf("File '%s' is not a .GIF file\n", arg_v[1]);
		fclose(f);
		free(comment);
		return 4;
	}
	fseek(f, -1, SEEK_END);
	i = fgetc(f);
	if(i != ';')
		goto NOT;
/* append comment to GIF file; overwriting the terminal ';' */
	fseek(f, -1, SEEK_END);
	if(fwrite(comment, 1, len, f) != len)
	{
		printf("Error writing file '%s'\n", arg_v[1]);
		fclose(f);
		free(comment);
		return 5;
	}
/* done */
printf("Success\n");
	fclose(f);
	free(comment);
	return 0;
}
