#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/mount.h>
#include <errno.h>




/*
 *
 */

int read_mountlist (char *filename);
char *find_equals (char *str);
int match_name (char *name);
char *trim_spaces (char *s);


char filename[256];


/*
 *
 */
 
int main (int argc, char *argv[])
{
	int rc;
	
	
	if (argc <= 1)
	{
		printf ("mount: missing operand\n");
		printf ("try 'mount --help' for more information.\n");
		return -1;
	}

	if (strcmp ("--help", argv[1]) == 0)
	{
		printf ("Usage: mount MOUNTLIST_FILE\n");
		printf ("mounts a device,\n");
		return 0;
	}
	

	if (*argv[1] == '/')
		strlcpy (filename, argv[1], 256);
	else
	{
		strlcpy (filename, "/sys/mnt/", 256);
		strlcat (filename, argv[1], 256);
	}
	
	
	rc = read_mountlist (filename); 	
	
	
	if (rc != 0)
	{
		printf ("mount failed: %s\n", strerror (errno));
		return -1;
	}
	
	return 0;
}





char *keywords[] = {
	"mount",
	"handler",
	"device",
	"startup",
	"hunit",
	"hflags",
	"unit",
	"flags",
	"blocksize"
	"partitionstart"
	"partitionend"
	"reservedblocks"
	"buffers",
	"priority",
	"baud",
	"removable",
	"writable",
	"writethrucritical",
	"writebackdelay",
	"max_transfer",
	"controlflags",
	NULL
};


enum keyword_idx
{
	MOUNT,
	HANDLER,
	DEVICE,
	STARTUP,
	HUNIT,
	HFLAGS,
	UNIT,
	FLAGS,
	BLOCKSIZE,
	PARTITIONSTART,
	PARTITIONEND,
	RESERVEDBLOCKS,
	BUFFERS,
	PRIORITY,
	BAUD,
	REMOVABLE,
	WRITABLE,
	WRITETHRUCRITICAL,
	WRITEBACKDELAY,
	MAXTRANSFER,
	CONTROLFLAGS
};






/* Statically Allocate line buffer, */

char line[256];

struct mountenviron me = 
{
	"", "", "", "",
	0, 0, 0, 0,
	512,
	0, 0, 0,
	32,
	0,
	112,
	1,
	1,
	1,
	0,
	0x10000,
	0x0000
};
	

int read_mountlist (char *filename)
{
	FILE *fp;
	char *name, *value;
	int i;	
	unsigned long ul_val;
	long l_val;
	char *errptr;
	
	
	/* Check if pathname is absolute, otherwise append to /sys/dev/mnt/ */
	
	if ((fp = fopen (filename, "r")) == NULL)
		return -1;
	
			
	while (fgets (line, 256, fp) != NULL)
	{
		if (line[0] == '#')
			continue;
		
		name = line;
				
		if ((value = find_equals (line)) == NULL)
			continue;
		
		*value = '\0';
		value++;
		
		name = trim_spaces (name);
		value = trim_spaces (value);
		
		i = match_name (name);
	
		switch (i)
		{
			case MOUNT:
				strlcpy (me.mount_name, value, MOUNTENVIRON_STR_MAX);
				break;
				
			case HANDLER:
				strlcpy (me.handler_name, value, MOUNTENVIRON_STR_MAX);
				break;
				
			case DEVICE:
				strlcpy (me.device_name, value, MOUNTENVIRON_STR_MAX);
				break;
							
			case STARTUP:
				strlcpy (me.startup_args, value, MOUNTENVIRON_STR_MAX);
				break;
				
			case HUNIT:
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.handler_unit = ul_val;
				break;

			case HFLAGS:
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.handler_flags = ul_val;
				break;
				
			case UNIT:
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.device_unit = ul_val;
				break;

			case FLAGS:
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.device_flags = ul_val;
				break;

			case BLOCKSIZE:
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.block_size = ul_val;
				break;

			case PARTITIONSTART:
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.partition_start = ul_val;
				break;

			case PARTITIONEND:
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.partition_end = ul_val;
				break;

			case RESERVEDBLOCKS:
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.reserved_blocks = ul_val;
				break;

			case BUFFERS:
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.buffer_cnt = ul_val;
				break;
				
			case PRIORITY:
				l_val = strtol  (value, &errptr, 0);
				if (errptr == NULL)
					me.boot_priority = l_val;
				break;
				
			case BAUD:
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.baud = ul_val;
				break;
			
			case REMOVABLE:		/* signed flag */
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.removable = ul_val;
				break;
			
			case WRITABLE:		/* signed flag */
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.writable = ul_val;
				break;


			case WRITETHRUCRITICAL:		/* signed flag */
				l_val = strtol  (value, &errptr, 0);
				if (errptr == NULL)
					me.writethru_critical = l_val;
				break;
			
			case WRITEBACKDELAY:
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.writeback_delay = ul_val;
				break;
			
			case MAXTRANSFER:
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.max_transfer = ul_val;
				break;
				
			case CONTROLFLAGS:
				ul_val = strtoul  (value, &errptr, 0);
				if (errptr == NULL)
					me.control_flags = ul_val;
				break;


				
			default:
				printf ("Variable not recognized : %s\n", name);
		}
	}	
	
	
	fclose (fp);
	
	return kos_mount (&me);
}




char *find_equals (char *str)
{
	char *s = str;
	
	while (*s != '\0')
	{
		if (*s == '=')
			return s;
		s++;
	}
	
	return NULL;
}



int match_name (char *name)
{
	int i = 0;
	char *s = name;
	
	while (*s != '\0')
	{
		*s = tolower (*s);
		s++;
	}
	
	
	while (keywords[i] != NULL)
	{
		if (strcmp (name, keywords[i]) == 0)
			return i;
		
		i ++;
	}
	
	return -1;
}






/* Trim spaces/tabs/n/r, Trim leading/trailing quotation marks around value */

char *trim_spaces (char *str)
{
	char *s = str; 
	char *trimmed_str = str;
    int len;
    
        
    while ( *s != '\0') 
    { 
		if (*s == '\"' || *s == '\t' || *s == '\n' || *s == ' ')
			*s = '\0';
		else
		{
			trimmed_str = s;
			break;
		}
		
		s++;
	}
	
	len = strlen (trimmed_str);
	s = trimmed_str + len;
	
	while (s >= trimmed_str)
	{
		if (*s == '\0' || *s == '\"' || *s == '\t' || *s == '\n' || *s == '\r' || *s == ' ')
		{
			*s = '\0';
		}
		
		else
		{
			break;
		}
		
		s --;
	}
	
	return trimmed_str;
}



