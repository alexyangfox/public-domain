#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <errno.h>


/*
 *
 */
 
int main (int argc, char *argv[])
{
	int rc;
	
	if (argc <= 1)
	{
		printf ("unmount: missing operand\n");
		printf ("try 'unmount --help' for more information.\n");
		return -1;
	}

	if (strcmp ("--help", argv[1]) == 0)
	{
		printf ("Usage: unmount DEVICE\n");
		printf ("Unmounts a device\n");
		return 0;
	}
	
	rc = kos_unmount (argv[1]);
	
	if (rc != 0)
	{
		printf ("unmount failed: %s\n", strerror (errno));
		return -1;
	}
	
	return 0;
}