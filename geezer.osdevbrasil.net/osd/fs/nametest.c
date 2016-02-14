/*****************************************************************************
Identifies illegal characters in filenames by attempting to create
files with names containing each character.

This code is public domain (no copyright).
You can do whatever you want with it.
*****************************************************************************/
#include <sys/stat.h>
#include <stdio.h>

#if defined(__TURBOC__)||defined(__WATCOMC__)
#include <io.h>

#elif defined(__GNUC__)
#include <unistd.h>
#include <fcntl.h>
#endif
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	char buf[16];
	int i, j;

	printf("These characters are illegal for file names:\n");
	for(i = ' '; i < 256; i++)
	{
		sprintf(buf, "%c", i);
		j = creat(buf, S_IWRITE);
		if(j >= 0)
		{
			close(j);
			unlink(buf);
		}
		else
			printf("%c (0x%02X)  ", i, i);
	}
	printf("\n");
	return 0;
}

