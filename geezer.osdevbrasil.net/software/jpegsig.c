/*****************************************************************************
Adds comment block (e.g. author info) to a JPEG file
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: Feb 12, 2004
This code is public domain (no copyright).
You can do whatever you want with it.

The comment is subject to command-line processing by the host OS,
so it may not look the way you want. Under DOS:
- Spaces in the comment will be compressed
- Characters reserved by the DOS shell are not allowed in the
  comment:  <  >  |  "  /  \  :  *  ?  etc.
- Command line is limited to about 128 characters in length
*****************************************************************************/
#include <stdlib.h> /* NULL, malloc(), free() */
#include <string.h> /* strlen(), strcat(), strncmp(), memcpy() */
/* FILE, SEEK_END, fopen(), fseek(), fread(), fgetc(), fwrite(), fclose() */
#include <stdio.h> /* printf(), sprintf() */
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	char buf[20], *comment;
	unsigned len;
	FILE *f;
	int i;

/* check args */
	if(arg_c < 3)
	{
		printf("Adds comment block to JPEG file\n"
			"Usage: jpegsig file.jpg comment\n");
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
/* allocate memory for comment block header,
the comment text itself, and JPEG End Of Image */
	comment = malloc(len + 6);
	if(comment == NULL)
	{
		printf("Out of memory\n");
		return 2;
	}
/* FFFE = Comment block */
	comment[0] = 0xFF;
	comment[1] = 0xFE;
	comment[2] = len / 256;
	comment[3] = len & 255;
	comment[4] = '\0';
	for(i = 2; i < arg_c; i++)
	{
		strcat(comment + 4, arg_v[i]);
		if(i + 1 < arg_c)
			strcat(comment + 4, " ");
	}
/* insert FFD9 (EndOfImage) block */
	comment[4 + len + 0] = 0xFF;
	comment[4 + len + 1] = 0xD9;
/* open file */
	f = fopen(arg_v[1], "r+b");
	if(f == NULL)
	{
		printf("Can't open file '%s'\n", arg_v[1]);
		free(comment);
		return 3;
	}
/* validate */
	if(fread(buf, 1, 20, f) != 20)
NOT:	{
		printf("File '%s' is not a JPEG file\n", arg_v[1]);
		fclose(f);
		free(comment);
		return 4;
	}
/* check for EOI at end of JPEG file */
	fseek(f, -2, SEEK_END);
	i = fgetc(f);
	if(i != 0xFF)
		goto NOT;
	i = fgetc(f);
	if(i != 0xD9)
		goto NOT;
/* append comment to JPEG file; overwriting the terminal EOI */
	fseek(f, -2, SEEK_END);
	if(fwrite(comment, 1, len + 6, f) != len + 6)
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
