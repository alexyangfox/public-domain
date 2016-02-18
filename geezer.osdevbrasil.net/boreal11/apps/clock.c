/*----------------------------------------------------------------------------
SIMPLE CLOCK

EXPORTS:
int main(void)
----------------------------------------------------------------------------*/
#include <stdio.h> /* stdout, setbuf(), printf() */
#include <time.h> /* time_t, time() */
#include <os.h> /* select() */
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	unsigned char min, sec, hour, key;
	unsigned timeout;
	time_t t;

setbuf(stdout, NULL);
	while(1)
	{
/* get time */
		t = time(NULL);
/* convert to HH:MM:SS */
		sec = t % 60;
		t /= 60;
		min = t % 60;
		t /= 60;
		hour = t % 24;
		t /= 24;
/* print it */
		printf("time is %02u:%02u:%02u\n", hour, min, sec);
/* 1-second delay */
		timeout = 1000;
/* eat keypress, if any
0=stdin file handle, 1=check for input */
		if(select(0, 1, &timeout))
			(void)read(0, &key, 1);
	}
	return 0;
}
