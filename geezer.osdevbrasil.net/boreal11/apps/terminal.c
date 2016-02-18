/*----------------------------------------------------------------------------
SERIAL PORT TEST

EXPORTS:
int main(void)
----------------------------------------------------------------------------*/
#include <string.h> /* strlen() */
#include <setjmp.h> /* jmp_buf, setjmp(), longjmp() */
#include <conio.h> /* KEY_... */
#include <stdio.h> /* printf() */
#include <os.h> /* O_RDWR, select(), open(), write(), close() */

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
static void write_or_die(unsigned handle, void *buf, int len)
{
	int i;

	i = write(handle, buf, len);
	if(i != len)
	{
		printf("write returned %d; wanted %u\n", i, len);
		longjmp(g_oops, 1);
	}
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
int main(void)
{
//	static const char *dev = "/dev/ser0";
	static const char *dev = "/dev/ser1";
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
	printf("Press F12 to quit, F11 to reset modem\n");
setbuf(stdout, NULL);
	while(1)
	{
/* check if key pressed and read it */
		if(select(0, 0x01, NULL))
		{
			read_or_die(0, &byte, 1);
/* if F12, exit */
			if(byte == KEY_F12)
				break;
/* if F11, send modem reset string ("ATZ\r") */
			else if(byte == KEY_F11)
			{
printf("Resetting modem...\n");
				write_or_die(m, "ATZ\r", 4);
			}
/* translate arrow keys to ANSI */
			else if(byte == KEY_UP)
				write_or_die(m, "\x1B[A", 3);
			else if(byte == KEY_DOWN)
				write_or_die(m, "\x1B[B", 3);
			else if(byte == KEY_RIGHT)
				write_or_die(m, "\x1B[C", 3);
			else if(byte == KEY_LEFT)
				write_or_die(m, "\x1B[D", 3);
/* write byte to modem */
			else if(byte != 0)
			{
				if(byte == '\n') /* LF -> CR */
					byte = '\r';
				write_or_die(m, &byte, 1);
			}
		}
/* check if data available from modem */
		if(select(m, 0x01, NULL))
		{
/* read and display byte */
			read_or_die(m, &byte, 1);
			putchar(byte);
		}
	}
	close(m);
	printf("Done\n");
	return 0;
}
