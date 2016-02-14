/*----------------------------------------------------------------------------
Dumps segments/markers in JPEG files
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
This code is public domain (no copyright).
You can do whatever you want with it.

Apr 28, 2008:
- Now displaying channel IDs in do_scan()

Apr  2, 2008:
- Now dumping image width, height, etc. from ALL types of SOF
   segment (not just SOF0 and SOF2)

Nov 27, 2007:
- Added code to dump image width, height, etc. from SOF segment
  and moved scan-handling code to it's own function (do_scan)

Sep 5, 2007:
- Made it work with multiple files on the command line (dumpjpeg *.jpg)

May 16, 2007:
- Eliminated macro depending on CPU byte order

Dec 11, 2005:
- Initial release
----------------------------------------------------------------------------*/
#include <string.h> /* memcmp() */
/* SEEK_..., FILE, stderr, fprintf(), printf() */
#include <stdio.h> /* fopen(), fseek(), fread(), fclose() */

#if 1
#define	DEBUG(X)	X
#else
#define	DEBUG(X)	/* nothing */
#endif
/*****************************************************************************
*****************************************************************************/
static unsigned read_be16(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] * 0x100 + buf[1];
}
/*****************************************************************************
*****************************************************************************/
static int do_sof(FILE *f)
{
	unsigned char buf[6];

	if(fread(buf, 1, 6, f) != 6)
		return -1;
	printf("Image is %ux%u pixels, has %u channel(s), "
		"and uses %u-bit color\n", read_be16(&buf[3]),
		read_be16(&buf[1]), buf[5], buf[0]);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
static int do_scan(FILE *f, unsigned char *buf)
{
	static const char *id[] =
	{
		"?", "Y", "Cb", "Cr", "I", "Q"
	};
/**/
	int num_chans, i;

/* get number of channels in scan */
	num_chans = fgetc(f);
	if(num_chans == EOF)
		return -1;
	printf("%u channel(s) in this scan: ", num_chans);
/* read two bytes of info for each channel */
	for(i = 0; i < num_chans; i++)
	{
		if(fread(buf, 1, 2, f) != 2)
			return -1;
		if(i > 0)
			printf(", ");
		if(buf[0] < sizeof(id) / sizeof(id[0]))
			printf("%s", id[buf[0]]);
		else
			printf("?");
	}
	printf("\n");
/* skip the spectral selection bytes */
	fseek(f, 3, SEEK_CUR);
/* The compressed data follows the SOS segment, but the only way
to determine the length of the compressed data is to read it
until you encounter another marker. What a crock! */
	while(1)
	{
/* get compressed bytes */
		i = fgetc(f);
		if(i == EOF)
			return -1;
/* ignore everything until 0xFF byte */
		if(i != 0xFF)
			continue;
/* 0xFF could be the MSB of a marker, so store it in buf... */
		buf[0] = i;
		i = fgetc(f);
		if(i == EOF)
			return -1;
/* 0xFF00 is an escaped 0xFF byte */
		if(i == 0)
			continue;
/* anything other than 0xFF00 or 0xFFD0-0xFFD7 is a marker -- store
marker LSB in buf and go back to the main processing loop to handle it */
		buf[1] = i;
		if(i < 0xD0 || i > 0xD7)
			return 0;
/* 0xFFD0-0xFFD7 is a Restart marker,
which is 2 bytes that can just be skipped */
		DEBUG(
			printf("%6lu ", ftell(f) - 2);
			printf("0xFF%02X      0 RST%1u : "
				"Restart #%u\n", i,
				i - 0xD0, i- 0xD0);
		)
	}
}
/*****************************************************************************
*****************************************************************************/
static int dump_jpeg(const char *file_name)
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
	unsigned char buf[10];
	unsigned marker, len;
	unsigned long pos;
	FILE *in;
	int i;

/* open input file */
	in = fopen(file_name, "rb");
	if(in == NULL)
	{
		printf("Error: can't open input file "
			"'%s'\n", file_name);
		return 2;
	}
/* read header and validate
0xFFD8=SOI: Start Of Image */
	if(fread(buf, 1, 2, in) != 2 || read_be16(buf) != 0xFFD8)
	{
		fclose(in);
		printf("Error: file '%s' is not a JPEG file\n", file_name);
		return 3;
	}
	DEBUG(
		printf(	"File '%s':\n", file_name);
		printf(	"Offset Marker Length Marker Name\n"
			"------ ------ ------ -----------\n"
			"     0 0xFFD8      0 %s\n",
			marker_name[0x18]);
	)
	while(1)
	{
/* read 2-byte marker at start of segment */
		if(fread(&buf[0], 1, 2, in) != 2)
ERR:		{
			printf("Error reading JPEG file '%s': unexpected "
				"end-of-file\n", file_name);
			fclose(in);
			return 4;
		}
AGAIN:
		marker = read_be16(&buf[0]);
		DEBUG(
			printf("%6lu ", ftell(in) - 2);
			printf("0x%04X ", marker);
		)
		pos = ftell(in);
/* read 2-byte length after marker. I _think_ EOI is the only segment
without a length field that can occur here... */
		if(marker == 0xFFD9) /* End Of Image (EOI) */
		{
			DEBUG(printf("     0 %s\n",
				marker_name[0x19]);)
			break;
		}
		if(fread(&buf[2], 1, 2, in) != 2)
			goto ERR;
		len = read_be16(&buf[2]);
		DEBUG(
			printf("%6u ", len);
			if(marker >= 0xFFC0)
				puts(marker_name[marker - 0xFFC0]);
			else
				printf("???\n");
		)
/* get image width and height from Start Of Frame (SOF) segment */
		if(marker >= 0xFFC0 && marker <= 0xFFCF && marker != 0xFFC4
			&& marker != 0xFFC8 && marker != 0xFFCC)
		{
			if(do_sof(in) != 0)
				goto ERR;
		}
/* skip over scan (compressed image data) after Start Of Scan (SOS) segment */
		if(marker == 0xFFDA)
		{
			i = do_scan(in, buf);
			if(i < 0)
				goto ERR;
			else
				goto AGAIN;
		}
		fseek(in, pos + len, SEEK_SET);
	}
	fclose(in);
	return 0;
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	int i;

/* check command-line args */
	if(arg_c < 2)
	{
		printf("Dumps segments in JPEG files\n");
		return 1;
	}
	for(i = 1; i < arg_c; i++)
		(void)dump_jpeg(arg_v[i]);
	return 0;
}
