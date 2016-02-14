#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>



int main (int argc, char *argv[])
{
	if (argc <= 1)
	{
		printf ("setmap: missing operand\n");
		printf ("try 'setmap --help' for more information.\n");
		return -1;
	}
	
	if (strcmp ("us", argv[1]) == 0)
	{
		return ioctl (0, IOCTL_CON_SETMAP, 0);
	}
	else if (strcmp ("uk", argv[1]) == 0)
	{
		return ioctl (0, IOCTL_CON_SETMAP, 1);
	}
	else if (strcmp ("nl", argv[1]) == 0)
	{
		return ioctl (0, IOCTL_CON_SETMAP, 2);
	}
	else if (strcmp ("--help", argv[1]) == 0)
	{
		printf ("Usage: setmap KEYMAP\n");
		printf ("Change the keymap of the console\n");
		printf ("Where KEYMAP is one of;\n\n");
		printf ("  us,  US Keymap\n");
		printf ("  uk,  United Kingdom Keymap\n");
		printf ("  nl,  Netherlands Keymap\n");
		return 0;
	}

	printf ("setmap: '%s' unknown keymap\n", argv[1]);	
	return -1;
}



