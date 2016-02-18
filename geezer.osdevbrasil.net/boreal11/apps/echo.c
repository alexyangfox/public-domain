/*----------------------------------------------------------------------------
TEXT ECHOING TASK

EXPORTS:
int main(void)
----------------------------------------------------------------------------*/
#include <stdio.h> /* stdout, fflush(), printf(), putchar() */
#include <os.h> /* write() */
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	unsigned char key;

	printf("Please type some text\n");
	while(1)
	{
		(void)read(0, &key, 1);
		putchar(key);
		fflush(stdout);
	}
	return 0;
}
