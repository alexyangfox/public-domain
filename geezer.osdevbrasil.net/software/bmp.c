/*****************************************************************************
Code to load .BMP file into memory
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: ?
This code is public domain (no copyright).
You can do whatever you want with it.

EXPORTS:
int read_bmp_file(img_t *img, const char *file_name, unsigned depth);
*****************************************************************************/
#include <stdlib.h> /* NULL, malloc(), free() */
/* FILE, fopen(), fread(), fclose(), printf() */
#include <stdio.h>

/* these work for little-endian CPU only (e.g. x86) */
#define	LE16(X)	(*(uint16_t *)(X))
#define	LE32(X)	(*(uint32_t *)(X))

/* these assume sizeof(unsigned short)==2 and sizeof(unsigned long)== 4 */
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;

typedef struct
{
	unsigned char r, g, b;
} rgb_t;

typedef struct
{
	unsigned long wd, ht;	/* in pixels */
	unsigned char *raster;
/* palette is used for 8-bit color or less */
	unsigned pal_size;
	rgb_t *pal;
} img_t;
/*****************************************************************************
*****************************************************************************/
#define	BMP_HDR_LEN	54

int read_bmp_file(img_t *img, const char *file_name, unsigned depth)
{
	unsigned long size_in_bytes, wd_in_bytes;
	unsigned char buffer[BMP_HDR_LEN];
	unsigned pad, temp;
	unsigned char *dst;
	FILE *infile;

/* open file */
	infile = fopen(file_name, "rb");
	if(infile == NULL)
		return -1;
/* read header */
	if(fread(buffer, 1, BMP_HDR_LEN, infile) != BMP_HDR_LEN)
BADFILE:{
		printf("File '%s' is not an uncompressed .BMP file\n",
			file_name);
		fclose(infile);
		return -1;
	}
/* validate */
	if(buffer[0] != 'B' || buffer[1] != 'M' || /* signature bytes */
		buffer[28] != depth ||		/* color depth */
		LE32(buffer + 30) != 0)	/* compression */
			goto BADFILE;
/* get info */
	img->wd = LE32(buffer + 18);
	wd_in_bytes = (img->wd * depth + 7) >> 3;
	img->ht = LE32(buffer + 22);
	size_in_bytes = wd_in_bytes * img->ht;
	img->raster = (unsigned char *)malloc(size_in_bytes);
	if(img->raster == NULL)
MEM:	{
		printf("Out of memory\n");
		fclose(infile);
		return -1;
	}
/* compute palette size */
	if(depth >= 16)
		img->pal_size = 0;
	else
		img->pal_size = 1 << depth;
/* alloc palette */
	if(img->pal_size != 0)
	{
		img->pal = (rgb_t *)malloc(img->pal_size *
			sizeof(rgb_t));
		if(img->pal == NULL)
		{
			free(img->raster);
			img->raster = NULL;
			goto MEM;
		}
/* read palette (BGR-) */
		for(temp = 0; temp < img->pal_size; temp++)
		{
			if(fread(buffer, 1, 4, infile) != 4)
SHORT:			{
				free(img->raster);
				img->raster = NULL;
				free(img->pal);
				img->pal = NULL;
				printf("Error reading .BMP file '%s'\n",
					file_name);
				fclose(infile);
				return -1;
			}
			img->pal[temp].r = buffer[2];
			img->pal[temp].g = buffer[1];
			img->pal[temp].b = buffer[0];
		}
	}
/* read raster (it's upside-down) */
	dst = img->raster + size_in_bytes - wd_in_bytes;
	pad = 3 - ((wd_in_bytes + 3) & 3);
	for(temp = 0; temp < img->ht; temp++)
	{
/* warning: true-color (24-bit) .BMP files store pixels in BGR order */
		if(fread(dst, 1, wd_in_bytes, infile) != wd_in_bytes)
			goto SHORT;
/* number of bytes/line is a multiple of 4,
so read and discard pad bytes at end of line */
		if(fread(buffer, 1, pad, infile) != pad)
			goto SHORT;
		dst -= wd_in_bytes;
	}
/* done */
	fclose(infile);
	return 0;
}
