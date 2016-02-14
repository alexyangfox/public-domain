/*****************************************************************************
Code to copy section of sound from uncompressed .WAV file to new .WAV file
Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: June 10, 2003
This code is public domain (no copyright).
You can do whatever you want with it.

Usage: CUTWAV file start end
'file' = input .WAV file. Must be uncompressed (PCM)
'start' = start of cut, in seconds
'end' = end of cut, in seconds

Output is written to file 'foo.wav' (xxx - fix this)
*****************************************************************************/
#include <string.h> /* memcmp(), strcpy() */
#include <stdlib.h> /* atol() */
/* FILE, SEEK_..., EOF, printf(), fopen(), fclose() */
#include <stdio.h> /* fseek(), ftell(), fread(), fwrite(), fgetc() */

#define	BUF_SIZE	256

/* these assume little-endian CPU like Intel x86 */
#define	read_le16(X)	*(uint16_t *)(X)
#define	read_le32(X)	*(uint32_t *)(X)
#define	write_le16(X,Y)	*(uint16_t *)(X) = (Y)
#define	write_le32(X,Y)	*(uint32_t *)(X) = (Y)

/* these assume sizeof(short)==2 and sizeof(long)==4 */
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
/*****************************************************************************
.WAV file headers:				offset
	char riff_magic[4];	// "RIFF"	// 0
	uint32_t file_len;	// file_len-8	// 4
	char wave_magic[4];	// "WAVE"	// 8

	char fmt_magic[4];	// "fmt "	// 12
	uint32_t fmt_blk_len;			// 16
	uint16_t compression;			// 20
	uint16_t channels;			// 22
	uint32_t sample_rate;			// 24
	uint32_t bytes_per_sec;			// 28
	uint16_t block_align;			// 32
	uint16_t bits_per_sample;		// 34
cound be more values here...use fmt_blk_len when reading file

could be useless "fact" chunk in here

	char data_magic[4];	// "data"	// 36
	uint32_t data_len;	// file_len-44	// 40
						// 44
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	unsigned long cut_start, cut_end, start_pos, end_pos, time;
	unsigned long rate, bytes_per_sec;
	unsigned channels, depth;
	char buf[BUF_SIZE], *data, *out_name = "foo.wav";
	FILE *in, *out;
	int i;

/* check args */
	if(arg_c != 4)
	{
		printf("Extracts segment from uncompressed .WAV file\n"
			"Usage: CUTWAV file start end\n"
			"'start' and 'end' are in seconds\n");
		return 1;
	}
	cut_start = atol(arg_v[2]);
	cut_end = atol(arg_v[3]);
/* open file */
	in = fopen(arg_v[1], "rb");
	if(in == NULL)
	{
		printf("Can't open input file '%s'\n", arg_v[1]);
		return 2;
	}
/* get length */
	fseek(in, 0, SEEK_END);
	end_pos = ftell(in);
	fseek(in, 0, SEEK_SET);
/* validate */
	if(fread(buf, 1, 12, in) != 12)
NOT:	{
		printf("File '%s' is not an uncompressed .WAV file\n",
			arg_v[1]);
		fclose(in);
		return 3;
	}
	if(memcmp(buf, "RIFF", 4) || memcmp(buf + 8, "WAVE", 4))
		goto NOT;
/* skip to 'fmt ' section */
	do
	{
		if(fread(buf, 1, 4, in) != 4)
RD_ERR:		{
			printf("Error reading .WAV file '%s'\n", arg_v[1]);
			fclose(in);
			return 4;
		}
	} while(memcmp(buf, "fmt ", 4) != 0);
/* read info from 'fmt ' section */
	if(fread(buf, 1, 20, in) != 20)
		goto RD_ERR;
/* 1=linear PCM, 2=ADPCM, 85=MP3, etc. */
	if(read_le16(buf + 4) != 1)
		goto NOT;
	channels = read_le16(buf + 6);
	rate = read_le32(buf + 8);
	bytes_per_sec = read_le32(buf + 12);
	depth = read_le16(buf + 18);
/* skip rest of 'fmt ' section */
	for(i = (int)(read_le32(buf) - 16); i != 0; i--)
	{
		if(fgetc(in) == EOF)
			goto RD_ERR;
	}
/* skip to 'data' section. This also skips the useless 'fact' sections. */
	for(data = "data"; *data != '\0'; )
	{
		i = fgetc(in);
		if(i == EOF)
			goto RD_ERR;
		if(i == *data)
			data++;
		else
			data = "data";
	}
/* skip data length
We do NOT handle .WAV files with multiple 'data' sections */
	if(fread(buf, 1, 4, in) != 4)
		goto RD_ERR;
/* valid .WAV file */
	start_pos = ftell(in);
/* convert file size to time */
	time = (end_pos - start_pos) / bytes_per_sec;
/* validate cut times */
	if(cut_start >= cut_end)
	{
		printf("Start time (%lu seconds) greater than or equal to "
			"end time (%lu seconds)\n", cut_start, cut_end);
		fclose(in);
		return 5;
	}
	if(cut_start > time)
	{
		printf("Start time (%lu seconds) beyond end of file "
			"(%lu seconds)\n", cut_start, time);
		fclose(in);
		return 5;
	}
	if(cut_end > time)
	{
		printf("End time (%lu seconds) beyond end of file "
			"(%lu seconds)\n", cut_start, time);
		fclose(in);
		return 5;
	}
/* convert cut start and cut end from seconds to bytes, then to file offset */
	cut_start = cut_start * bytes_per_sec + start_pos;
	cut_end = cut_end * bytes_per_sec + start_pos;
/* convert end to byte count, and seek to start of cut */
	cut_end -= cut_start;
	fseek(in, cut_start, SEEK_SET);
/* create header for new .WAV file */
	strcpy(buf + 0, "RIFF");
	write_le32(buf + 4, 36 + cut_end);
	strcpy(buf + 8, "WAVE""fmt ");
	write_le32(buf + 16, 16); /* length of "fmt " block */
	write_le16(buf + 20, 1); /* compression (1=none=PCM) */
	write_le16(buf + 22, channels);
	write_le32(buf + 24, rate);
	write_le32(buf + 28, bytes_per_sec);
	write_le16(buf + 32, 0); /* block align (?) */
	write_le16(buf + 34, depth);
	strcpy(buf + 36, "data");
	write_le32(buf + 40, cut_end);
/* open output file and write header */
	out = fopen(out_name, "wb");
	if(out == NULL)
	{
		printf("Can't open output file '%s'\n", out_name);
		fclose(in);
		return 6;
	}
	if(fwrite(buf, 1, 44, out) != 44)
WR_ERR:	{
		printf("Error writing output file '%s'\n", out_name);
		fclose(out);
		fclose(in);
		return 7;
	}
/* copy from input to output */
	while(cut_end)
	{
		if(cut_end > BUF_SIZE)
			i = BUF_SIZE;
		else
			i = (int)cut_end;
		if(fread(buf, 1, i, in) != i)
			goto RD_ERR;
		if(fwrite(buf, 1, i, out) != i)
			goto WR_ERR;
		cut_end -= i;
	}
/* close files */
	fclose(out);
	fclose(in);
	return 0;
}
