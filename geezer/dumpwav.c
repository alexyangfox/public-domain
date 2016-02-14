/*----------------------------------------------------------------------------
.WAV file dumper
Chris Giese	<geezer@execpc.com>	http://my.execpc.com/~geezer
Release date: March 28, 2008
This code is public domain (no copyright).
You can do whatever you want with it.
----------------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
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
static unsigned long read_le32(void *buf_ptr)
{
	unsigned char *buf = buf_ptr;

	return buf[0] + buf[1] * 0x100u + buf[2] * 0x10000L
		+ buf[3] * 0x1000000L;
}
/*****************************************************************************
*****************************************************************************/
#define BPERL		16	/* byte/line for dump */

static void dump(void *data_p, unsigned count)
{
	unsigned char *data = (unsigned char *)data_p;
	unsigned byte1, byte2;

	while(count != 0)
	{
		for(byte1 = 0; byte1 < BPERL; byte1++)
		{
			if(count == 0)
				break;
			printf("%02X ", data[byte1]);
			count--;
		}
		printf("\t");
		for(byte2 = 0; byte2 < byte1; byte2++)
		{
			if(data[byte2] < ' ')
				printf(".");
			else
				printf("%c", data[byte2]);
		}
		printf("\n");
		data += BPERL;
	}
}
/*****************************************************************************
*****************************************************************************/
int main(int arg_c, char *arg_v[])
{
	unsigned long pos, len;
	unsigned char buf[16];
	FILE *f;

	if(arg_c != 2)
	{
		printf(".WAV file dumper\n");
		return 1;
	}
	f = fopen(arg_v[1], "rb");
	if(f == NULL)
	{
		printf("Error: can't open file '%s'\n", arg_v[1]);
		return 2;
	}
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	if(fread(buf, 1, 12, f) != 12)
NOT:	{
		printf("Error: file '%s' is not a .WAV file\n", arg_v[1]);
		fclose(f);
		return 3;
	}
	if(memcmp(&buf[0], "RIFF", 4) || memcmp(&buf[8], "WAVE", 4))
		goto NOT;
	printf("File length from OS=%lu\n", len);
	len = read_le32(&buf[4]);
	printf("Data length from file headers=%lu\n", len);
	while(1)
	{
		if(fread(buf, 1, 8, f) != 8)
			break;
		pos = ftell(f);
		len = read_le32(&buf[4]);
		printf("'%-4.4s' block: file offset=%lu, length=%lu\n",
			buf, pos, len);
		if(!memcmp(&buf[0], "fmt ", 4))
		{
			unsigned long i;
			unsigned j;

			if(fread(buf, 1, 16, f) != 16)
ERR:			{
				printf("Error: unexpected end of .WAV "
					"file '%s'\n", arg_v[1]);
				fclose(f);
				return 3;
			}
			printf("\tcodec=0x%X\n",	read_le16(&buf[0]));
			printf("\tnum channels=%u\n",	read_le16(&buf[2]));
			printf("\tsamples/sec=%lu\n",	read_le32(&buf[4]));
			printf("\tavg. bytes/sec=%lu\n",read_le32(&buf[8]));
			printf("\tblock align=%u\n",	read_le16(&buf[12]));
			printf("\tbits/sample=%u\n",	read_le16(&buf[14]));
			for(i = 16; i < len; )
			{
				j = (len - i < 16) ?
					(unsigned)(len - i)
					: 16;
				if(fread(buf, 1, j, f) != j)
					goto ERR;
				dump(buf, j);
				i += j;
			}
		}
		fseek(f, pos + len, SEEK_SET);
	}
	fclose(f);
	return 0;
}
