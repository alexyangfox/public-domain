/*----------------------------------------------------------------------------
SLIP

EXPORTS:
int main(void)
----------------------------------------------------------------------------*/
//#include <string.h> /* strlen() */
#include <setjmp.h> /* jmp_buf, setjmp(), longjmp() */
//#include <conio.h> /* KEY_... */
#include <stdio.h> /* printf() */
#include <os.h> /* O_RDWR, select(), open(), close() */

static jmp_buf g_oops;
/*****************************************************************************
*****************************************************************************/
static int open_or_die(const char *path, unsigned access)
{
	int rv;

	rv = open(path, access);
	if(rv < 0)
	{
		printf("open(%s) returned %d\n", path, rv);
		longjmp(g_oops, 1);
	}
	return rv;
}
/*****************************************************************************
*****************************************************************************/
static void read_or_die(unsigned handle, void *buf, int len)
{
	int i;

	i = read(handle, buf, len);
	if(i != len)
	{
		printf("read returned %d; wanted %u\n", i, len);
		longjmp(g_oops, 1);
	}
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
#define END	0300	/* =192=0xC0; indicates start/end of packet */
#define ESC	0333	/* =219=0xDB; byte stuffing */
#define ESC_END	0334	/* =220=0xDC; ESC ESC_END (0xDB 0xDC) = END (0xC0) */
#define ESC_ESC	0335	/* =221=0xDD; ESC ESC_ESC (0xDB 0xDD) = ESC (0xDB) */

static void do_slip(unsigned byte)
{
	static unsigned char pkt[1066]; /* SLIP MaxTU; per RFC-1055 */
	static char saw_esc = 0;
	static unsigned i;
/**/

/* un-escape the incoming byte */
	if(i >= 1066)
	{
		printf("do_slip: packet too long\n");
		i = 0;
		saw_esc = 0;
	}
	else
	{
		if(saw_esc)
		{
			if(byte == ESC_END)
			{
				pkt[i] = END;
				i++;
			}
			else if(byte == ESC_ESC)
			{
				pkt[i] = ESC;
				i++;
			}
			else
			{
				printf("do_slip: bad byte 0x%02X after ESC\n", byte);
				i = 0;
			}
			saw_esc = 0;
		}
		else
		{
			if(byte == END)
			{
				printf("Got valid SLIP packet:\n");
				dump(pkt, i);
				i = 0;
			}
			/*else*/if(byte == ESC)
				saw_esc = 1;
			else
			{
				pkt[i] = byte;
				i++;
			}
		}
	}

}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	static const char *dev = "/dev/ser0";
//	static const char *dev = "/dev/ser1";
/**/
	volatile int m = -1;
	unsigned char byte;

/* set up error trapping */
	if(setjmp(g_oops) != 0)
	{
		if(m >= 0)
			close(m);
		return 1;
	}
/* open serial device */
	printf("Opening serial device %s\n", dev);
	m = open_or_die(dev, O_RDWR);
	printf("Press Esc to quit\n");
	while(1)
	{
/* check if key pressed */
		if(select(0, 0x01, NULL))
		{
/* read key; exist if Esc */
			read_or_die(0, &byte, 1);
			if(byte == 27)
				break;
		}
/* check if data available from serial port */
		if(select(m, 0x01, NULL))
		{
putchar('*'); fflush(stdout);
/* read byte from port, handle SLIP packet */
			read_or_die(m, &byte, 1);
			do_slip(byte);
		}
	}
	close(m);
	printf("Done\n");
	return 0;
}
