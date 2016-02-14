/*----------------------------------------------------------------------------
Turns off transparency in .PNG and .GIF files
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer/
This code is public domain (no copyright).
You can do whatever you want with it.

Nov 20, 2008:
- This program now removes alpha and on-off transparency from .PNG files too

Nov 25, 2004:
- Modified to work with animated GIFs
- Now testing fwrite() and fputc() for write errors
- Using setjmp() and longjmp() for error-trapping to reduce line count
  in main routine

Requires zlib. Compile with GCC:
	gcc -o untran.exe -O2 -Wall -W untran.c -lz

This code will also build with 16-bit compilers like Turbo C,
but untran_png() probably won't work.

To do soon:
- I'm not writing a valid CRC value for the IDAT chunk in
  the output file (why not? it's easy enough...)
- support interlaced .PNG
- adapt filter code to work with Borland C, which doesn't
  work properly with negative array indexes
- support .PNG with 16-bit color
- output file should use filtering -- for ideas, look at function
  png_write_find_filter() in file PNGWUTIL.C in the libpng source code
- more output file optimization (other than filtering): count total
  number of colors used, reduce color depth if appropriate
  (wretched Wikipedia images with 15 unique colors that use 32-bit
  pixels -- just to get the alpha channel)
----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>

#if 1
#define	DEBUG(X)	X
#else
#define	DEBUG(X)	/* nothing */
#endif

#if defined(__TURBOC__)
#include <alloc.h> /* farmalloc(), farfree() */
#if __TURBOC__==0x401
#error Sorry, Turbo C++ 3.0 'huge' is broken
#endif
/* "__huge" is legal ANSI C; "huge" is not */
#define HUGE		__huge
#define	MEMCPY(D,S,N)	farmemcpy(D,S,N)
#define	MALLOC(N)	farmalloc(N)
#define	FREE(N)		farfree(N)

#elif defined(__WATCOMC__)&&!defined(__386__)
#include <malloc.h>
#define HUGE		huge
#define	MEMCPY(D,S,N)	_fmemcpy(D,S,N)
#define	MALLOC(N)	_fmalloc(N)
#define	FREE(N)		_ffree(N)

/* modern, sensible compilers :) */
#else
#define HUGE		/* nothing */
#define	MEMCPY(D,S,N)	memcpy(D,S,N)
#define	MALLOC(N)	malloc(N)
#define	FREE(N)		free(N)
#endif
/*----------------------------------------------------------------------------
PNG code
----------------------------------------------------------------------------*/
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
static void write_be32(void *buf_ptr, unsigned long val)
{
	unsigned char *buf = buf_ptr;

	buf[3] = val; /* LSB */
	val >>= 8;
	buf[2] = val;
	val >>= 8;
	buf[1] = val;
	val >>= 8;
	buf[0] = val; /* MSB */
}
/*****************************************************************************
CRC code lifted from the appendix in the .PNG spec
*****************************************************************************/
static unsigned long update_crc(unsigned long c,
		unsigned char *buf, unsigned len)
{
	static unsigned long crc_table[256];
	static char init;
/**/
	unsigned i;

	if(!init)
	{
		unsigned long m;
		unsigned j, k;

		for(j = 0; j < 256; j++)
		{
			m = j;
			for(k = 0; k < 8; k++)
			{
				if(m & 1)
					m = 0xEDB88320L ^ (m >> 1);
				else
					m = m >> 1;
			}
			crc_table[j] = m;
		}
		init = 1;
	}
	for(i = 0; i < len; i++)
		c = crc_table[(unsigned)((c ^ buf[i]) & 0xFF)] ^ (c >> 8);
	return c;
}
/*****************************************************************************
*****************************************************************************/
static unsigned long crc(unsigned char *buf, unsigned len)
{
	return update_crc(0xFFFFFFFFL, buf, len) ^ 0xFFFFFFFFL;
}
/*****************************************************************************
This function loads IDAT chunks, decompresses them using zlib,
and stores the decompressed image in memory.

in		= input file
buf		buf[0-3] = CRC of previous chunk
		buf[4-7] = length of current (first) IDAT chunk
		buf[8-11] = "IDAT"
raster		= where to store decompressed image
raster_size	= size in bytes of decompressed image
*****************************************************************************/
#define	BUF_SIZE	1024

static int read_raster(FILE *in, unsigned char *buf,
		unsigned char HUGE *raster, unsigned long raster_size)
{
	unsigned char in_buf[BUF_SIZE];
	unsigned long chunk_len;
	z_stream state;
	DEBUG(unsigned blah;)
	unsigned j, k;
	int i;

	memset(&state, 0, sizeof(state));
	state.avail_in = 0;
	state.next_in = in_buf;
	state.avail_out = raster_size;
	state.next_out = raster;
/* prepare to inflate (decompress input file) */
	i = inflateInit(&state);
	if(i != 0)
	{
		printf("Error: inflateInit() returned %d\n", i);
		return -1;
	}
/* re-read chunk len */
	chunk_len = read_be32(&buf[4]);
	while(1)
	{
/* (re)fill input buffer */
		if(state.avail_in == 0)
		{
/* if chunk_len==0, get another IDAT chunk. Multiple IDAT chunks
are guaranteed (by the PNG spec) to be consecutive in the PNG file. */
			if(chunk_len == 0)
			{
/* 12 bytes: 4-byte CRC of this chunk, 4-byte length of next chunk,
and 4-byte ID ("IDAT") of next chunk */
				if(fread(buf, 1, 12, in) != 12)
				{
					printf("Error reading .PNG file\n");
					return -1;
				}
				chunk_len = read_be32(&buf[4]);
				if(memcmp(&buf[8], "IDAT", 4))
				{
					printf("Error in .PNG file: "
						"expected 'IDAT' chunk; "
						"got '%4.4s' chunk\n",
						&buf[8]);
					return -1;
				}
			}
			j = (chunk_len > BUF_SIZE) ? BUF_SIZE :
				(unsigned)chunk_len;
			k = fread(in_buf, 1, j, in);
			chunk_len -= k;
			state.avail_in = k;
			state.next_in = in_buf;
		}
/* decompress */
		DEBUG(
			j = state.avail_in;
			blah = state.avail_out;
		)
		i = inflate(&state, Z_SYNC_FLUSH);
		DEBUG(printf("Inflated %u bytes to %u bytes\n",
			j - state.avail_in, blah - state.avail_out);)
		if(i < 0 || i == 2)
		{
			printf("Error: inflate() returned %d\n", i);
			return -1;
		}
/* inflate() returning +1 means end of flate stream */
		if(i == 1)
			break;
	}
	i = inflateEnd(&state);
	if(i != 0)
	{
		printf("Error: inflateEnd() returned %d\n", i);
		return -1;
	}
	return 0;
}
/*****************************************************************************
This function compresses the raster data using zlib and writes the
compressed data in a single, huge IDAT chunk to the file

out		= output file
raster		= raster
bpr_ifb		= bytes per row for raster. Includes filter byte
		  at start of row.
ht		= height of raster
*****************************************************************************/
static int write_raster(FILE *out, unsigned char HUGE *raster,
		unsigned bpr_ifb, unsigned long ht)
{
	unsigned char buf[BUF_SIZE];
	unsigned long pos1, pos2;
	z_stream state;
	unsigned j;
	int i;

/* prepare to deflate (compress output file) */
	memset(&state, 0, sizeof(state));
	state.avail_in = bpr_ifb * ht;
	state.next_in = raster;
	state.avail_out = BUF_SIZE;
	state.next_out = buf;
	i = deflateInit(&state, Z_DEFAULT_COMPRESSION);
	if(i != 0)
	{
		printf("Error: deflateInit() returned %d\n", i);
		return -1;
	}
/* skip 8 bytes in output file for IDAT chunk header
(we'll come back and write it later, once we know the chunk size) */
	pos1 = ftell(out);
	fseek(out, 8, SEEK_CUR);
/* write deflated raster to output file; in a single IDAT chunk.
IMHO, multiple IDAT chunks are pointless. */
	do
	{
		DEBUG(
			unsigned blah;

			blah = state.avail_in;
		)
		i = deflate(&state, Z_FINISH);
		if(i < 0)
		{
			printf("Error: deflate() returned %d\n", i);
			return -1;
		}
		j = BUF_SIZE - state.avail_out;
		DEBUG(printf("Deflated %u bytes to %u bytes\n",
			blah - state.avail_in, j);)
		if(fwrite(buf, 1, j, out) != j)
WR_ERR:		{
			printf("Error writing temporary output file\n");
			return -1;
		}
		state.next_out = buf;
		state.avail_out = BUF_SIZE;
	} while(i != 1);
	i = deflateEnd(&state);
	if(i != 0)
	{
		printf("Error: deflateEnd() returned %d\n", i);
		return -1;
	}
/* go back to start of chunk and write chunk size and signature */
	pos2 = ftell(out);
	fseek(out, pos1, SEEK_SET);
	write_be32(&buf[0], pos2 - pos1 - 8);
	memcpy(&buf[4], "IDAT", 4);
	if(fwrite(buf, 1, 8, out) != 8)
		goto WR_ERR;
	DEBUG(printf("pos2=%lu, pos1=%lu, chunk len=%lu\n",
		pos2, pos1, pos2 - pos1 - 8);)
/* return to end of the new IDAT chunk and write CRC
xxx - I'm just storing 0 here */
	fseek(out, pos2, SEEK_SET);
	write_be32(&buf[0], 0);
	if(fwrite(&buf[0], 1, 4, out) != 4)
		goto WR_ERR;
	return 0;
}
/*****************************************************************************
*****************************************************************************/
#define	ABS(X)	(((X) < 0) ? -(X) : (X))

static int paeth(int left, int above, int above_left)
{
	int pa, pb, pc;

	pa = ABS(above - above_left);
	pb = ABS(left - above_left);
	pc = ABS(above + left - above_left * 2);
	if(pa <= pb && pa <= pc)
		return left;
	if(pb <= pc)
		return above;
	return above_left;
}
/*****************************************************************************
'filtering' (including Paeth filter above) improves lossless compression
-- see the PNG spec

ptr	= points to start of row y in raster (just after the filter byte). I
	  assume the previous row of un-filtered pixels is at ptr[-bpr_xfb-1]
bpr_xfb	= width of row; in BYTES (excluding filter byte)
y	= row number
bpp	= bytes per pixel, including alpha channel (if any)
filter	= filter algorithm number (first byte of decompressed data
	  for row)
*****************************************************************************/
static void unfilter_row(unsigned char HUGE *ptr, unsigned long bpr_xfb,
		unsigned long y, unsigned bpp, unsigned filter)
{
	unsigned char HUGE *prev;
	unsigned long x;

	prev = &ptr[-bpr_xfb - 1];
	DEBUG(printf("applying filter type %u to row %lu\n", filter, y);)
/* none */
	if(filter == 0)
		/* nothing */;
/* sub */
	else if(filter == 1)
	{
		for(x = bpp; x < bpr_xfb; x++)
			ptr[x] += ptr[x - bpp];
	}
/* up */
	else if(filter == 2 && y > 0)
	{
		for(x = 0; x < bpr_xfb; x++)
			ptr[x] += prev[x];
	}
/* average */
	else if(filter == 3)
	{
		if(y == 0)
		{
			for(x = bpp; x < bpr_xfb; x++)
				ptr[x] += ptr[x - bpp] / 2;
		}
		else
		{
			for(x = 0; x < bpp; x++)
				ptr[x] += prev[x] / 2;
			for(x = bpp; x < bpr_xfb; x++)
				ptr[x] += (ptr[x - bpp] + prev[x]) / 2;
		}
	}
/* paeth */
	else if(filter == 4)
	{
		if(y == 0)
			for(x = bpp; x < bpr_xfb; x++)
				ptr[x] += ptr[x - bpr_xfb];
		else
		{
			for(x = 0; x < bpp; x++)
				ptr[x] += prev[x];
			for(x = bpp; x < bpr_xfb; x++)
				ptr[x] += paeth(ptr[x - bpp],
					prev[x], prev[x - bpp]);
		}
	}
}
/*****************************************************************************
*****************************************************************************/
static void unfilter(unsigned char HUGE *raster, unsigned long wd,
		unsigned long ht, unsigned bpp)
{
	unsigned long y, bpr_xfb;
	unsigned filter;

	bpr_xfb = wd * bpp;
	for(y = 0; y < ht; y++)
	{
		filter = raster[0];
		unfilter_row(&raster[1], bpr_xfb, y, bpp, filter);
/* change filter byte for this row to 0 ("no filtering") */
		raster[0] = 0;
/* +1 for filter byte */
		raster += bpr_xfb + 1;
	}
}
/*****************************************************************************
*****************************************************************************/
static void remove_alpha(unsigned char HUGE *raster, unsigned color_type,
		unsigned long wd, unsigned long ht)
{
	unsigned char HUGE *src, HUGE *dst;
	unsigned long y, x;

/* there _is_ no alpha */
	if((color_type & 4) == 0)
		return;
	src = dst = raster;
/* grayscale + alpha */
	if(color_type == 4)
	{
		for(y = 0; y < ht; y++)
		{
/* image must be unfiltered before we do this
(filter byte at start of each row must =0) */
			if(src[0] != 0)
ERR:			{
				printf("Error: trying to remove alpha "
					"channel from filtered image\n");
				return;
			}
			*dst++ = *src++;
/* copy other pixels, omitting transparency byte */
			for(x = 0; x < wd; x++)
			{
				unsigned long alpha, oma; /* One Minus Alpha */

				alpha = 0xFFL * src[1];
				oma = 0xFF - src[1];
/* xxx - this is untested
				dst[0] = (alpha + oma * src[0]) / 0x100; */
				dst[0] = (src[0] * oma + alpha) / 0x100;
				src += 2;
				dst++;
			}
		}
	}
/* RGB + alpha */
	else /*if(color_type == 6)*/
	{
		for(y = 0; y < ht; y++)
		{
/* image must be unfiltered before we do this */
			if(src[0] != 0)
				goto ERR;
			*dst++ = *src++;
/* apply transparency byte assuming WHITE background color, since that's
what most PNGs with broken transparency assume (*cough* Wikipedia *cough*).
Advance pointers such that the transparency byte is overwritten;
i.e. convert RGBA to RGB */
			for(x = 0; x < wd; x++)
			{
				unsigned long alpha, white;

				alpha = src[3]; /* 32-bit */
				white = (0xFF - src[3]) * 0xFF;
				dst[0] = (src[0] * alpha + white) / 0xFF;
				dst[1] = (src[1] * alpha + white) / 0xFF;
				dst[2] = (src[2] * alpha + white) / 0xFF;
				src += 4;
				dst += 3;
			}
		}
	}
}
/*****************************************************************************
Multi-byte values in PNG files are big endian
(MSB at lowest address, or first in file).

Color	Allowed
Type	Bit Depths	Interpretation				Mnemonic
-----	----------	---------------------------------	--------
0       1,2,4,8,16	Each pixel is a grayscale sample.	Y
2       8,16		Each pixel is an R,G,B triple.		RGB
3       1,2,4,8		Each pixel is a palette index;		I
			a PLTE chunk must appear.
4       8,16		Each pixel is a grayscale sample,	YA
			followed by an alpha sample.
6       8,16		Each pixel is an R,G,B triple,		RGBA
			followed by an alpha sample.

Return values:
+2	Possibly-valid .PNG file, but this code doesn't support it
+1	Not a .PNG file
0	Success
-1	Invalid .PNG file
-2	Some other error
*****************************************************************************/
static int untran_png(const char *in_name)
{
/* bpr_ifb = Bytes Per Row, Including Filter Byte */
	unsigned long chunk_len, wd, ht, bpr_ifb;
	unsigned depth, color_type, interlace;
	unsigned char buf[33], HUGE *raster;
	char out_name[L_tmpnam];
	FILE *in, *out;
	int i;

	in = fopen(in_name, "rb");
	if(in == NULL)
	{
		printf("Error: can't open file '%s'\n", in_name);
		return -1;
	}
/* read file header and IHDR chunk; check header */
	if(fread(buf, 1, 33, in) != 33)
	{
NOT:		fclose(in);
		return +1;
	}
	if(memcmp(&buf[0], "\x89PNG\x0D\x0A\x1A\x0A", 8))
		goto NOT;
/* first chunk in PNG file must be IHDR -- verify and check length */
	chunk_len = read_be32(&buf[8]);
	if(memcmp(&buf[12], "IHDR", 4))
	{
		printf("Error in .PNG file '%s': got '%4.4s' chunk; "
			"expected 'IHDR' chunk\n", in_name, &buf[12]);
		fclose(in);
		return -1;
	}
	if(chunk_len != 13)
	{
		printf("Error in .PNG file '%s': 'IHDR' chunk length "
			"is %lu; should be 13\n", in_name, chunk_len);
		fclose(in);
		return -1;
	}
/* get image info from IHDR chunk */
	wd = read_be32(&buf[16]);
	ht = read_be32(&buf[20]);
	depth = buf[24];
	color_type = buf[25];
	DEBUG(printf("Image is %lux%lux%u; color type is %u\n",
		wd, ht, depth, color_type);)
/* sanity checking: color type & depth */
	switch(color_type)
	{
	case 0:
		if(depth != 1 && depth != 2 && depth != 4 &&
			depth != 8 && depth != 16)
BAD:		{
			printf("Error in .PNG file '%s': depth %u is "
				"invalid for color type %u\n", in_name,
				depth, color_type);
			fclose(in);
			return -1;
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
/* bad PNG file (return -1) or a problem with this code? (return +2) */
		printf("Error in .PNG file '%s': color type is %u; "
			"should be 0,2,3,4, or 6\n", in_name, color_type);
		fclose(in);
		return -1;
	}
/* compression scheme (0=zlib) */
	if(buf[26] > 0)
	{
		printf("Errror in .PNG file '%s': compression method %u "
			"not supported\n", in_name, buf[26]);
		fclose(in);
		return +2;
	}
/* filter scheme (0=default) */
	if(buf[27] > 0)
	{
		printf("Error in .PNG file '%s': filter scheme %u "
			"not supported\n", in_name, buf[27]);
		fclose(in);
		return +2;
	}
/* interlace (0=none, 1=Adam-7) */
	interlace = buf[28];
	if(interlace > 1)
	{
		printf("Error in .PNG file '%s': interlace scheme %u "
			"not supported\n", in_name, interlace);
		fclose(in);
		return +2;
	}
/* all checks passed */
	bpr_ifb = (depth * wd + 7) / 8;
	if(color_type == 2)
		bpr_ifb *= 3;	/* RGB */
	else if(color_type == 4)
		bpr_ifb *= 2;	/* YA */
	else if(color_type == 6)
		bpr_ifb *= 4;	/* RGBA */
/* plus the filter byte at the start of each row */
	bpr_ifb++;
#if 1
	if(interlace)
	{
		printf("Software error; .PNG file '%s': interlaced .PNG "
			"not yet supported by this code\n", in_name);
		fclose(in);
		return +2;
	}
	if(depth == 16)
	{
		printf("Software error; .PNG file '%s': 16-bit color "
			"not yet supported by this code\n", in_name);
		fclose(in);
		return +2;
	}
#endif
/* open temporary output file */
	tmpnam(out_name);
	out = fopen(out_name, "wb");
	if(out == NULL)
	{
		printf("Error: can't open temporary output file '%s'\n",
			out_name);
		fclose(in);
		return -2;
	}
/* if color type indicates the presence of an alpha transparency channel,
change it */
#define UNTRAN 1 /* undefine for debugging */

#if defined(UNTRAN)
	if(color_type == 4)	/* 4=grayscale + alpha */
		buf[25] = 0;	/* 0=grayscale only */
	else if(color_type == 6) /* 6=RGB + alpha */
		buf[25] = 2;	/* 2=RGB only */
#endif
/* allocate enough memory for entire raster */
	raster = (unsigned char HUGE *)MALLOC(bpr_ifb * ht);
	if(raster == NULL)
	{
		printf("Error: out of memory\n");
		fclose(out);
		remove(out_name);
		fclose(in);
		return -2;
	}
/* bytes 29-32: CRC at end of IHDR chunk -- recalculate it.
CRC is not done on chunk length or the CRC field itself;
only the chunk type code and chunk data fields. */
	write_be32(&buf[29], crc(&buf[12], 17));
/* write PNG file header and the IHDR chunk */
	if(fwrite(buf, 1, 33, out) != 33)
WR_ERR:	{
		printf("Error writing temporary output file '%s' "
			"(disk full?)\n", out_name);
		FREE(raster);
		fclose(out);
		remove(out_name);
		fclose(in);
		return -2;
	}
/* read other chunks */
	while(1)
	{
/* read chunk length to &buf[4] and chunk ID to &buf[8],
leaving &buf[0] free to hold the CRC of the previous chunk */
		if(fread(&buf[4], 1, 8, in) != 8)
RD_ERR:		{
			printf("Error reading .PNG file '%s'\n", in_name);
			FREE(raster);
			fclose(out);
			remove(out_name);
			fclose(in);
			return -1; /* truncated .PNG file? */
		}
		chunk_len = read_be32(&buf[4]);
		DEBUG(printf("'%4.4s' chunk is %lu bytes long\n",
			&buf[8], chunk_len);)
/* remove on-off transparency by not copying tRNS chunk to output.
Background color is also irrelevant for opaque PNG; so remove bKGD chunk */
		if(!memcmp(&buf[8], "tRNS", 4) ||
			!memcmp(&buf[8], "bKGD", 4))
				fseek(in, chunk_len + 4, SEEK_CUR);
/* remove alpha transparency */
		else if(!memcmp(&buf[8], "IDAT", 4) && (color_type & 0x04))
		{
			unsigned long new_bpr_ifb;
			unsigned bpp;

/* load raster from IDAT chunk(s) */
			if(read_raster(in, buf, raster,
				bpr_ifb * ht))
			{
				FREE(raster);
				fclose(out);
				remove(out_name);
				fclose(in);
				return -1;
			}
/* skip CRC at end of last IDAT chunk */
			fseek(in, 4, SEEK_CUR);
/* Note: this code does not suppport 16-bit samples. */
			if(color_type == 4)	/* 4=grayscale + alpha */
				bpp = 2;
			else/*if(color_type == 6) 6=RGB + alpha */
				bpp = 4;
/* unfilter */
			unfilter(raster, wd, ht, bpp);
#if defined(UNTRAN)
			remove_alpha(raster, color_type, wd, ht);
/* one less byte per pixel once the alpha is removed: */
			bpp--;
/* new value of bpr_ifb; for use by write_raster() */
			new_bpr_ifb = wd * bpp + 1;
#else
			new_bpr_ifb = bpr_ifb;
#endif
			if(write_raster(out, raster, new_bpr_ifb, ht))
			{
				FREE(raster);
				fclose(out);
				remove(out_name);
				fclose(in);
				return -1;
			}
		}
/* copy all other chunks unchanged to the output file */
		else
		{
			if(fwrite(&buf[4], 1, 8, out) != 8)
				goto WR_ERR;
/* +4 to include the (unchanged) CRC at the end of the chunk
### - fread()/fwrite() loop might be faster */
			for(chunk_len += 4; chunk_len != 0; chunk_len--)
			{
				if((i = fgetc(in)) == EOF)
					goto RD_ERR;
				if(fputc(i, out) == EOF)
					goto WR_ERR;
			}
		}
/* done after 'IEND' chunk */
		if(!memcmp(&buf[8], "IEND", 4))
			break;
	}
	FREE(raster);
	fclose(out);
	fclose(in);
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
/*----------------------------------------------------------------------------
GIF code
----------------------------------------------------------------------------*/
/*****************************************************************************
*****************************************************************************/
static int untran_gif(const char *in_name)
{
	char out_name[L_tmpnam], gce, success = 0;
	unsigned char buf[256];
	FILE *in, *out;
	int i;

/* open input file */
	in = fopen(in_name, "rb");
	if(in == NULL)
	{
		printf("Error: can't open file '%s'\n", in_name);
		return -1;
	}
/* read header (screen descriptor) */
	if(fread(buf, 1, 13, in) != 13)
NOT:	{
		fclose(in);
		return +1;
	}
/* make sure it's a GIF file */
	if(memcmp(buf, "GIF8", 4))
		goto NOT;
/* open output file */
	tmpnam(out_name);
	out = fopen(out_name, "wb");
	if(out == NULL)
	{
		printf("Error: can't open temporary output file '%s'\n",
			out_name);
		fclose(in);
		return -2;
	}
/* write header (screen descriptor) */
	if(fwrite(buf, 1, 13, out) != 13)
WR_ERR:	{
		printf("Error writing temporary output file '%s' "
			"(disk full?)\n", out_name);
		fclose(out);
		remove(out_name);
		fclose(in);
		return -2;
	}
/* copy global palette (color map), if any */
	if(buf[10] & 0x80)
	{
		i = (buf[10] & 7) + 1;
		i = 1 << i;
		for(; i != 0; i--)
		{
			if(fread(buf, 1, 3, in) != 3)
RD_ERR:			{
				printf("Error reading .GIF file '%s'\n",
					in_name);
				fclose(out);
				remove(out_name);
				fclose(in);
				return -1; /* truncated .GIF file? */
			}
			if(fwrite(buf, 1, 3, out) != 3)
				goto WR_ERR;
		}
	}
/* main loop */
	while(1)
	{
		if((i = fgetc(in)) == EOF)
			goto RD_ERR;
		if(fputc(i, out) == EOF)
			goto WR_ERR;
/* ';' == end of GIF file */
		if(i == ';')
			break;
/* ',' == image descriptor */
		else if(i == ',')
		{
/* copy image descriptor */
			if(fread(buf, 1, 9, in) != 9)
				goto RD_ERR;
			if(fwrite(buf, 1, 9, out) != 9)
				goto WR_ERR;
/* copy local palette, if any */
			if(buf[8] & 0x80)
			{
				i = (buf[8] & 7) + 1;
				i = 1 << i;
				for(; i != 0; i--)
				{
					if(fread(buf, 1, 3, in) != 3)
						goto RD_ERR;
					if(fwrite(buf, 1, 3, out) != 3)
						goto WR_ERR;
				}
			}
/* copy initial LZW code size */
			if((i = fgetc(in)) == EOF)
				goto RD_ERR;
			if(fputc(i, out) == EOF)
				goto WR_ERR;
/* copy sub-blocks of LZW-compressed data */
			while(1)
			{
				if((i = fgetc(in)) == EOF)
					goto RD_ERR;
				if(fputc(i, out) == EOF)
					goto WR_ERR;
				if(i == 0)
					break;
				if(fread(buf, 1, i, in) != i)
					goto RD_ERR;
				if(fwrite(buf, 1, i, out) != i)
					goto WR_ERR;
			}
		}
/* '!' == extension block */
		else if(i == '!')
		{
			if((i = fgetc(in)) == EOF)
				goto RD_ERR;
			if(fputc(i, out) == EOF)
				goto WR_ERR;
			gce = (i == 0xF9); /* Graphic Control Extension */
/* copy sub-blocks */
			while(1)
			{
				if((i = fgetc(in)) == EOF)
					goto RD_ERR;
				if(fputc(i, out) == EOF)
					goto WR_ERR;
				if(i == 0)
					break;
				if(fread(buf, 1, i, in) != i)
					goto RD_ERR;
				if(gce && i == 4)
				{
/* TURN OFF TRANSPARENCY */
					buf[0] &= ~0x01;
					success = 1;
				}
				if(fwrite(buf, 1, i, out) != i)
					goto WR_ERR;
			}
		}
#if 1
		else
		{
			printf("Error: invalid .GIF file '%s'\n", in_name);
printf("i=%u\n", i);
			fclose(out);
			remove(out_name);
			fclose(in);
			return -1;
		}
#endif
	}
	fclose(in);
	fclose(out);
	if(!success)
	{
		printf("Warning: no GCE (transparency control) blocks "
			"in .GIF file '%s'\n(file will not be changed)\n",
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
	int i;

	if(arg_c < 2)
	{
		printf("Turns off transparency in .PNG and .GIF files\n");
		return 1;
	}
	for(i = 1; i < arg_c; i++)
	{
		if(untran_png(arg_v[i]) == +1)
			if(untran_gif(arg_v[i]) == +1)
				printf("Error: file '%s' is not .PNG nor "
					".GIF\n", arg_v[i]);
	}
	return 0;
}
