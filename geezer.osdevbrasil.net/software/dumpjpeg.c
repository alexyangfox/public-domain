/*----------------------------------------------------------------------------
Dumps segments/markers in JPEG files
Chris Giese <geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: Dec 11, 2005
This code is public domain (no copyright).
You can do whatever you want with it.
----------------------------------------------------------------------------*/
#include <string.h> /* memcmp() */
/* SEEK_..., FILE, stderr, fprintf(), printf() */
#include <stdio.h> /* fopen(), fseek(), fread(), fclose() */

#if 0
/* C99 fixed-width types */
#include <stdint.h>
#else
typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned long	uint32_t;
#endif

#if 1
#define	DEBUG(X)	X
#else
#define	DEBUG(X)	/* nothing */
#endif

#if 1
/* little-endian CPU like Intel x86 */
#define	READ_BE16(X)	swap16(*(uint16_t *)(X))
#define	READ_BE32(X)	swap32(*(uint32_t *)(X))
#else
/* big-endian CPU like Motorola 680x0 */
#define	READ_BE16(X)	*(uint16_t *)(X))
#define	READ_BE32(X)	*(uint32_t *)(X)
#endif
/*****************************************************************************
*****************************************************************************/
unsigned swap16(unsigned arg)
{
	return ((arg >> 8) & 0x00FF) | ((arg << 8) & 0xFF00);
}
/*****************************************************************************
*****************************************************************************/
unsigned long swap32(unsigned long arg)
{
	return ((arg >> 24) & 0x00FF) | ((arg >> 8) & 0xFF00) |
		((arg << 8) & 0x00FF0000L) | ((arg << 24) & 0xFF000000L);
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	static const char *marker_name[] =
	{
/* 0xFFC0 */	"SOF0 : Start Of Frame; Huffman baseline DCT",
		"SOF1 : Start Of Frame; Huffman extended sequential DCT",
		"SOF2 : Start Of Frame; Huffman progressive DCT",
		"SOF3 : Start Of Frame; Huffman spatial lossles",
		"DHT  : Define Huffman Table",
		"SOF5 : Start Of Frame; Huffman differential sequential DCT",
		"SOF6 : Start Of Frame; Huffman differential progressive DCT",
		"SOF7 : Start Of Frame; Huffman differential spatial",
/* 0xFFC8 */	"JPG  : Extension",
		"SOF9 : Start Of Frame; Arithmetic extended sequential DCT",
		"SOF10: Start Of Frame; Arithmetic progressive DCT",
		"SOF11: Start Of Frame; Arithmetic spatial lossless",
		"DAC  : Define Arithmetic coding conditioning(s)",
		"SOF13: Start Of Frame; Arithmetic differential sequential DCT",
		"SOF14: Start Of Frame; Arithmetic differential progressive DCT",
		"SOF15: Start Of Frame; Arithmetic spatial",
/* 0xFFD0 */	"RST0 : Restart #0",
		"RST1 : Restart #1",
		"RST2 : Restart #2",
		"RST3 : Restart #3",
		"RST4 : Restart #4",
		"RST5 : Restart #5",
		"RST6 : Restart #6",
		"RST7 : Restart #7",
/* 0xFFD8 */	"SOI  : Start Of Image",
		"EOI  : End Of Image",
		"SOS  : Start Of Scan",
		"DQT  : Define Quantization Table",
		"DNL  : Define Number of Lines",
		"DRI  : Define Restart Interval",
		"DHP  : Define Hierarchical progression",
		"EXP  : Expand Reference Component(s)",
/* 0xFFE0 */	"APP0 : JFIF/JFXX file header",
		"APP1 : Exif/XMP file header",
		"APP2 : ICC file header",
		"APP3 : META file header",
		"APP4 : Application file header #4",
		"APP5 : Application file header #5",
		"APP6 : Application file header #6",
		"APP7 : Application file header #7",
/* 0xFFE8 */	"APP8 : Application file header #8",
		"APP9 : Application file header #9",
		"APP10: Application file header #10",
		"APP11: Application file header #11",
		"APP12: Picture info/\"Ducky\" file header",
		"APP13: Photoshop IRB file header",
		"APP14: Adobe DCT file header",
		"APP15: Application file header #15",
/* 0xFFF0 */	"JPG0 : Extension #0",
		"JPG1 : Extension #1",
		"JPG2 : Extension #2",
		"JPG3 : Extension #3",
		"JPG4 : Extension #4",
		"JPG5 : Extension #5",
		"JPG6 : Extension #6",
		"JPG7 : Extension #7",
/* 0xFFF8 */	"JPG8 : Extension #8",
		"JPG9 : Extension #9",
		"JPG10: Extension #10",
		"JPG11: Extension #11",
		"JPG12: Extension #12",
		"JPG13: Extension #13",
		"JPG14: Extension #14",
		"COM  : Comment",
/* 0xFFFF */	"??"
	};
/**/
	unsigned char buf[4];
	unsigned marker, len;
	FILE *in;
	int i;

/* check command-line args */
	if(arg_c != 2)
	{
		printf("Dumps segments in JPEG files\n");
		return 1;
	}
/* open input file */
	in = fopen(arg_v[1], "rb");
	if(in == NULL)
	{
		printf("Error: can't open input file "
			"'%s'\n", arg_v[1]);
		return 2;
	}
/* read header and validate
0xFFD8=SOI: Start Of Image */
	if(fread(buf, 1, 2, in) != 2 || READ_BE16(buf) != 0xFFD8)
	{
		fclose(in);
		printf("Error: file '%s' is not a JPEG file\n", arg_v[1]);
		return 3;
	}
	DEBUG(
		printf(	"File '%s':\n", arg_v[1]);
		printf(	"Offset Marker Length Marker Name\n"
			"------ ------ ------ -----------\n"
			"     0 0xFFD8      0 %s\n",
			marker_name[0x18]);
	)
	while(1)
	{
/* read 2-byte marker at start of segment */
		if(fread(buf + 0, 1, 2, in) != 2)
ERR:		{
			printf("Error reading JPEG file '%s': unexpected "
				"end-of-file\n", arg_v[1]);
			break;
		}
AGAIN:
		marker = READ_BE16(buf + 0);
		DEBUG(
			printf("%6lu ", ftell(in) - 2);
			printf("0x%04X ", marker);
		)
/* read 2-byte length after marker. I _think_ EOI is the only segment
without a length field that can occur here... */
		if(marker == 0xFFD9) /* EOI */
		{
			DEBUG(printf("     0 %s\n",
				marker_name[0x19]);)
			break;
		}
		if(fread(buf + 2, 1, 2, in) != 2)
			goto ERR;
		len = READ_BE16(buf + 2);
		DEBUG(
			printf("%6u ", len);
			if(marker >= 0xFFC0)
				puts(marker_name[marker - 0xFFC0]);
			else
				printf("???\n");
		)
/* ignore everything but Start Of Scan */
		if(marker != 0xFFDA)
		{
			fseek(in, len - 2, SEEK_CUR);
			continue;
		}
/* The compressed data follows the SOS segment, but the only way
to determine the length of the compressed data is to read it
until you encounter another marker. What a crock! */
		while(1)
		{
/* get compressed bytes */
			i = fgetc(in);
			if(i == EOF)
				goto ERR;
/* ignore everything until 0xFF byte */
			if(i != 0xFF)
				continue;
			buf[0] = i;
			i = fgetc(in);
			if(i == EOF)
				goto ERR;
/* 0xFF00 is an escaped 0xFF byte */
			if(i == 0)
				continue;
/* anything other than 0xFF00 or 0xFFD0-0xFFD7 is a marker --
go back to the main processing loop */
			buf[1] = i;
			if(i < 0xD0 || i > 0xD7)
				goto AGAIN;
/* 0xFFD0-0xFFD7 is a Restart marker,
which is 2 bytes that can just be skipped */
			DEBUG(
				printf("%6lu ", ftell(in) - 2);
				printf("0xFF%02X      0 RST%1u : "
					"Restart #%u\n", i,
					i - 0xD0, i- 0xD0);
			)
		}
	}
	fclose(in);
	return 0;
}
