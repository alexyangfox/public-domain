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
	int rc = 0;
	int c;
	int t;
	
	char *filesystem = NULL;
	char *mountname = NULL;
	char *label = NULL;
	unsigned long cluster_size = 0;
	unsigned long flags = 0;
	
					
	for (t=1; t<argc; t++)
	{
		if (strcmp ("--help", argv[t]) == 0)
		{
			printf ("Usage : format [-qc] mount [label]\n");
			printf ("  -q                 : Quick format\n");
			printf ("  -c cluster_size    : Cluster size in kb (0 = 512 bytes)\n");
			return 0;
		}
	}
	
	if (argc > 1)
	{
		while ((c = getopt (argc, argv, "c:q")) != -1)
		{
			switch (c)
			{
				case 'c':
					cluster_size = atoi (optarg);
					break;
				
				case 'q':
					flags = FORMATF_QUICK;
					break;
			}
		}

		if (optind < argc)
		{
			mountname = argv[optind];
		}
		else
		{
			errno = EINVAL;
			printf ("%s : No mount specified\n", argv[0]);
			return -1;
		}
		
		if (optind+1 < argc)
		{
			label = argv[optind+1];
		}
		
		
		if (cluster_size == 0)
			cluster_size = 512;
		
		rc = kos_format (mountname, label, flags, cluster_size);
		
		if (rc == -1)
			printf ("%s : %s\n", argv[0], strerror (errno));
	
		return rc;
	}
	else
	{
		printf ("For help : format --help");
		return 0;
	}
}



