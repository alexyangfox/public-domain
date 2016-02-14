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
	
	for (t=1; t<argc; t++)
	{
		if (strcmp ("--help", argv[t]) == 0)
		{
			printf ("Usage : relabel device|volume new_volume\n");
			printf ("Relabel a device/volume.");
			return 0;
		}
	}
	
	if (argc == 3)
	{
		
		/* FIXME:  Add kos_relabel syscall to Newlib library */
		/* rc = kos_relabel (argv[1], argv[2]); */
		
		if (rc == -1)
			printf ("%s : %s\n", argv[0], strerror (errno));
	
		return rc;
	}
	else
	{
		printf ("For help : relabel --help");
		return 0;
	}
}



