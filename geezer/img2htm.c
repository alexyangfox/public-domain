/*----------------------------------------------------------------------------
Generates HTML <IMG> tag with proper WIDTH and HEIGHT
from a given .GIF, .PNG, or .JPG file.
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
This code is public domain (no copyright).
You can do whatever you want with it.

Modified March 2, 2007:
- Eliminated #if statement that depended on CPU byte order
- Some style changes in code
Modified March 3, 2007:
- Fixed stupid unsigned-char-promoted-to-int bug in read_le/be16/32 routines

Only limited error-checking is done on the image. For example,
this program produces output for most of the invalid images in
Willem van Schaik's PNGSUITE.
----------------------------------------------------------------------------*/
#include <string.h> /* memcmp() */
/* SEEK_..., FILE, stderr, fprintf(), printf() */
#include <stdio.h> /* fopen(), fseek(), fread(), fclose() */

static unsigned long g_wd, g_ht;
/*****************************************************************************
*****************************************************************************/
static unsigned read_le16(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] + buf[1] * 0x100;
}
/*****************************************************************************
CAUTION: the 'u' after '0x100' is significant
*****************************************************************************/
/*static unsigned long read_le32(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] +	buf[1] * 0x100u +
		buf[2] * 0x10000L + buf[3] * 0x1000000L;
}*/
/*****************************************************************************
*****************************************************************************/
static unsigned read_be16(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] * 0x100 + buf[1];
}
/*****************************************************************************
*****************************************************************************/
static unsigned long read_be32(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] * 0x1000000L + buf[1] * 0x10000L +
		buf[2] * 0x100u + buf[3];
}
/*****************************************************************************
File	Size
offset	(bytes)	Value(s)		Meaning
------	-------	-----------------	------------------------------------
0	6	"GIF87a"/"GIF89a" 	Magic values that identify .GIF file
6	2	-			"Screen" (bounding box) width
8	2	-			"Screen" (bounding box) height
10	1	-			Packed fields byte
11	1	-			Background color
12	1	-			Unused (GIF87) or aspect ratio (GIF89)
13

xxx - do more validation of the file?
*****************************************************************************/
static int check_gif(FILE *f)
{
	unsigned char buf[13];

/* read file header (the "screen descriptor") */
	fseek(f, 0, SEEK_SET);
	if(fread(buf, 1, 13, f) != 13)
		return +1; /* too short; not GIF */
/* validate */
	if((memcmp(&buf[0], "GIF87a", 6) && memcmp(&buf[0], "GIF89a", 6)))
		return +1;
	g_wd = read_le16(&buf[6]);
	g_ht = read_le16(&buf[8]);
	return 0; /* success */
}
/*****************************************************************************
File	Size
offset	(bytes)	Value(s)		Meaning
------	-------	--------		-------
0	8	"\x89""PNG\r\n\x1A\n"	Magic values that identify .PNG file

8	4	13			Typical IHDR chunk length
12	4	"IHDR"			Other critical chunks: "IEND", "IDAT"

16	4	-			Image width
20	4	-			Image height
24	1	-			Image depth (bits per pixel)
25	1	-			Color type: 0=grayscale,2=RGB,
					  3=palette,4=gray+alpha,6=RGB+alpha
26	1	0			Compression type: 0=flate
27	1	0			Filter scheme: 0=default
28	1	-			Interlace type: 0=none, 1=Adam7
29

xxx - maybe check for illegal color type (e.g. 0, 9), illegal bit depth
(e.g. 0, 3, 99), empty IDAT chunk, incorrect checksums on chunks
*****************************************************************************/
static int check_png(FILE *f)
{
	unsigned char buf[24];

/* read 8-byte file header + 8-byte IHDR chunk header
+ first 8 bytes of the IHDR chunk */
	fseek(f, 0, SEEK_SET);
	if(fread(buf, 1, 24, f) != 24)
		return +1; /* too short; not PNG */
/* validate */
	if(memcmp(&buf[0], "\x89""PNG\r\n\x1A\n", 8))
		return +1;
/* look for IHDR chunk
read_be32(&buf[8]) is the IHDR chunk length */
	if(memcmp(&buf[12], "IHDR", 4))
		return -1; /* invalid PNG file */
/* get width and height */
	g_wd = read_be32(&buf[16]);
	g_ht = read_be32(&buf[20]);
	return 0;
}
/*****************************************************************************
File	Size
offset	(bytes)	Value(s)		Meaning
------	-------	--------		-------
0	2	0xFF, 0xD8		JFIF Start Of Image (SOI) segment

(note: some JPGs have APP1 [Exif] blocks here instead of APP0)
(note: some JPGs have other blocks [such as comments] before the APPn block)
2	2	0xFF, 0xE0		JFIF APP0 segment
4	2	16			Length of APP0 segment
6	5	"JFIF\x00"		Magic values that identify .JPG file
11	2	-			JFIF major, minor version
13	1	-			0=aspect ratio, 1=dots/in, 2=dots/cm
14	2	-			Horizontal aspect ratio or resolution
16	2	-			Vertical aspect ratio or resolution
18	1	usu. 0			Thumbnail width
19	1	usu. 0			Thumbnail height
20

N	2	0xFF, 0xC0/0xC2		JPEG Start Of Frame (SOF) marker
					0xFFC2 marker is for interlaced JPEG
N+2	2	8?			SOF chunk len
N+4	1	usu. 8			8=8-bit color, 12=12-bit color
N+5	2	-			Image height
N+7	2	-			Image width
N+9	1	usu. 3			1=grayscale, 3=color (YCbCr)
N+10

xxx - maybe check that at least one of each of these chunks is present:
DQT (quantization table), DHT (Huffman table), SOF (Start Of Frame),
SOS (Start Of Scan)
xxx - maybe make sure at least one APPn chunk is present
xxx - maybe validate all chunks until you get to SOS.
*****************************************************************************/
static int check_jpg(FILE *f)
{
	unsigned marker, len;
	unsigned char buf[6];

	fseek(f, 0, SEEK_SET);
/* read header and validate */
	if(fread(buf, 1, 6, f) != 6)
		return +1; /* too short; not JPEG */
/* 0xFFD8=SOI; Start Of Image */
	marker = read_be16(&buf[0]);
	if(marker != 0xFFD8)
		return +1;
/* At first, I checked for the APP0 segment here, but then I found
a JPEG with the Exif (APP1) segment instead. Then I tested for any
segment from APP0-APP15...only to later find a JPEG with a comment
block between the SOI and the APP0.

Executive summary: check only for SOI to validate a JPEG file. */
	while(1)
	{
		marker = read_be16(&buf[2]);
		len = read_be16(&buf[4]);
		if(len < 2)
			break;
/* 0xFFCn=SOFn (Start Of Frame), except for 0xFFC4=DHT (Define Huffman Table),
0xFFC8=JPG (Extension), and 0xFFCC=DAC (Define Arithmetic Coding) */
		if(marker >= 0xFFC0 && marker <= 0xFFCF &&
			marker != 0xFFC4 &&
			marker != 0xFFC8 &&
			marker != 0xFFCC)
		{
/* read the SOF chunk, get image width and height, and return */
			if(fread(buf, 1, 6, f) != 6)
				return -1;
			g_ht = read_be16(&buf[1]);
			g_wd = read_be16(&buf[3]);
			return 0;
		}
/* advance to next chunk */
		fseek(f, len - 2, SEEK_CUR);
/* read chunk header */
		if(fread(&buf[2], 1, 4, f) != 4)
			break;
	}
	return -1; /* EOF before SOF chunk seen */
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	static int (* const check[])(FILE *) =
	{
		check_gif, check_png, check_jpg
	};
	static const char *type[] =
	{
		"GIF", "PNG", "JPEG"
	};
/**/
	int i, k = 1;
	unsigned j;
	FILE *in;

/* check command-line args */
	if(arg_c < 2)
	{
		printf("Generates HTML <IMG> tag with proper WIDTH and HEIGHT\n"
			"from a given image file\n");
		return 1;
	}
/* for each arg */
	for(i = 1; i < arg_c; i++)
	{
/* open input file */
		in = fopen(arg_v[i], "rb");
		if(in == NULL)
		{
			fprintf(stderr, "Error: can't open input file "
				"'%s'\n", arg_v[i]);
			continue;
		}
/* for each image file type we know about... */
		for(j = 0; j < sizeof(check) / sizeof(check[0]); j++)
		{
/* ...try to get image width and height from input image file */
			k = check[j](in);
			if(k <= 0)
				break;
		}
/* done with input file; close it */
		fclose(in);
/* success! */
		if(k == 0)
		{
			printf("<img src=\"%s\" width=\"%u\" height=\"%u\">\n",
				arg_v[i], g_wd, g_ht);
		}
/* it's an image file type we recognize,
but there's something wrong with the file */
		else if(k < 0)
		{
			fprintf(stderr, "Error: invalid %s file '%s'\n",
				type[j], arg_v[i]);
		}
/* unrecognized image file format */
		else
		{
			fprintf(stderr, "Error: format of image file '%s' "
				"is unknown\n", arg_v[i]);
		}
	}
	return 0;
}
