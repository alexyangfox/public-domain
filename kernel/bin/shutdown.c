#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ksyscalls.h>
#include <unistd.h>
#include <errno.h>




/*
 *
 */

int main (int argc, char *argv[])
{
	int t;
	int rc;
	int how = KOS_REBOOT_HALT;
	char c;
	
	for (t=1; t<argc; t++)
	{
		if (strcmp ("--help", argv[t]) == 0)
		{
			printf ("Usage : shutdown [-rh]\n");
			printf ("Shutdown the computer.  Defaults to Halt.\n");
			printf ("  -r  : Reboot after shutdown\n");
			printf ("  -h  : Halt after shutdown\n");
			return 0;
		}
	}
	
	while ((c = getopt (argc, argv, "rh")) != -1)
	{
		switch (c)
		{
			case 'r':
				how = KOS_REBOOT_REBOOT;
				break;
			
			case 'h':
				how = KOS_REBOOT_HALT;
				break;
		}
	}

	rc = kos_reboot (how);
	
	if (rc == -1)
		printf ("%s : %s\n", argv[0], strerror (errno));

	return rc;
}



