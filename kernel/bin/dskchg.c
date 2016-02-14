#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ksyscalls.h>
#include <errno.h>


int main (int argc, char *argv[])
{
	if (argc <= 1)
	{
		printf ("diskchange: missing operand\n");
		printf ("try 'setmap --help' for more information.\n");
		return -1;
	}

	if (strcmp ("--help", argv[1]) == 0)
	{
		printf ("Usage: dskchg DEVICE\n");
		printf ("Notify OS of a media-change\n");
		return 0;
	}
	
	kos_inhibit(argv[1]);
	kos_uninhibit(argv[1]);
	
	if (errno != 0)
	{
		printf ("dskchg: %s\n", strerror (errno));
		return -1;
	}
	
	return 0;
}



